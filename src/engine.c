/* MPC adapter, GPL-3.0-or-later. Original synthesis lives in src/dsp.
 * Only the worker touches the DSP. Audio uses fixed buffers and reference counts;
 * it never waits for synthesis, allocates, locks a mutex, or touches a file. */
#define _GNU_SOURCE
#include "params.h"
#include "engine.h"
#include "param_defs.h"
#include "dsp/synthesizer.h"
#include "dsp/oscillator.h"
#include "dsp/envelope.h"
#include "dsp/filter.h"
#include "dsp/humanizer.h"
#include <unistd.h>
#include <stdatomic.h>

#define RATE 44100
#define MAX_SAMPLES (RATE*4)
#define VOICES 8
#define PENDING_HITS 64
#define EDGE_SAMPLES 221 /* 5 ms, endpoints included at 44.1 kHz */
#define SLOTS (VOICES+4)
typedef struct {
    atomic_int refs; int size; float *data; unsigned revision,view_revision;
    unsigned char plot[3][PLOT_COLUMNS];
} Cache;
typedef struct { int slot,note,channel; double pos,step; float gain; unsigned age; } Voice;
typedef struct {int slot,note,channel,playback;double step;float gain;uint64_t due;} PendingHit;
typedef struct {
    _Atomic float values[GP_COUNT];
    atomic_uint revision,audio_revision,view_revision,synthesis_count;
    atomic_bool running;
    atomic_int ready;
    pthread_t worker;
    struct gkick_synth *synth;
    Cache slots[SLOTS];
    Voice voices[VOICES];
    PendingHit pending[PENDING_HITS];unsigned pending_count,pending_dropped;
    uint64_t sample_clock;
    struct gkick_humanizer humanizer;
    int last_human_velocity,last_human_delay;
    int current;
    unsigned age;
    unsigned display_revision,ui_revision;
    float output_gain;
    float cut_tail[EDGE_SAMPLES];
    unsigned cut_pos;
    unsigned point_dirty,point_origin,free_dirty;
    float edit_weights[4];int edit_time_valid,edit_osc,edit_kind,edit_bank,edit_count;
    unsigned char dirty[GP_COUNT],dirty_alias[G_ALIAS_COUNT];
} Engine;

/* Same platform helpers as upstream geonkick.c, without its global worker. */
void geonkick_usleep(unsigned long usecs) { usleep(usecs); }
int geonkick_rand(unsigned int *seed) { return rand_r(seed); }

static int index_of(const char *key) {
    uint32_t hash=2166136261u;
    for(const unsigned char *s=(const unsigned char *)key;*s;s++)hash=(hash^*s)*16777619u;
    unsigned slot=hash&(GP_LOOKUP_SIZE-1);
    for(;;){int i=gparam_lookup[slot];if(i<0)return -1;if(!strcmp(key,gparams[i].key))return i;slot=(slot+1)&(GP_LOOKUP_SIZE-1);}
}
static int alias_of(const char *key) {
    for(int i=0;i<G_ALIAS_COUNT;i++)if(!strcmp(key,galiases[i].key))return i;
    return -1;
}
static float at(Engine *e,const char *key) { int i=index_of(key); return i<0?0:atomic_load(&e->values[i]); }
static float alias_coordinate(int alias,float physical) {
    const GAlias *a=&galiases[alias];
    if(a->kind==3)return logf(physical/20.f)/logf(600.f);
    if(a->kind>=4)return (physical-a->lo)/(a->hi-a->lo);
    return a->kind==1?roundf(physical*1000):a->kind==2?roundf(physical*100):physical<=20?FREQ_FINE_MIN:physical>=12000?FREQ_FINE_MAX:roundf(240*log2f(physical/55.f));
}
static float value(const float *p,const char *key) { int i=index_of(key); return i<0?0:p[i]; }
static void put(Engine *e,const char *key,float v) { int i=index_of(key); if(i>=0) atomic_store(&e->values[i],v); }
static void drop(Engine *e,int slot) { if(slot>=0) atomic_fetch_sub(&e->slots[slot].refs,1); }
static void stop_voice(Engine *e,Voice *v) { drop(e,v->slot); v->slot=-1; }
static void stop_pending(Engine *e) {
    for(unsigned i=0;i<e->pending_count;i++)drop(e,e->pending[i].slot);
    e->pending_count=0;
}
static void stop_all(Engine *e) {
    stop_pending(e);
    for(int i=0;i<VOICES;i++) stop_voice(e,&e->voices[i]);
    memset(e->cut_tail,0,sizeof e->cut_tail);e->cut_pos=0;
}
/* C1-continuous endpoints: no sudden fade slope at either boundary. */
static float edge_gain(float t) { t=fminf(1,fmaxf(0,t));return t*t*(3-2*t); }
static float voice_sample(const Cache *c,double pos) {
    int i=(int)pos;if(i>=c->size)return 0;
    float a=c->data[i],b=i+1<c->size?c->data[i+1]:0;
    return a+(b-a)*(float)(pos-i);
}
static void choke_voice(Engine *e,Voice *v) {
    if(v->slot<0)return;
    /* Fixed bounded work, no allocation/lock. Preserve existing short tails
     * during rapid retriggers; do not replace them with a frozen DC sample. */
    Cache *c=&e->slots[v->slot];unsigned target=e->cut_pos;
    for(int n=0;n<EDGE_SAMPLES;n++) {
        e->cut_tail[target]+=voice_sample(c,v->pos+n*v->step)*v->gain*
            edge_gain((EDGE_SAMPLES-1-n)/(float)(EDGE_SAMPLES-1));
        if(++target==EDGE_SAMPLES)target=0;
    }
    stop_voice(e,v);
}
/* Start and pending scheduling are bounded, allocation-free audio operations.
 * A pending hit owns one cache reference, transferred to its voice at start. */
static void start_hit(Engine *e,const PendingHit *hit) {
    if(hit->playback==1)for(int i=0;i<VOICES;i++)choke_voice(e,&e->voices[i]);
    int v=0;
    for(int i=0;i<VOICES;i++){if(e->voices[i].slot<0){v=i;break;}if(e->voices[i].age<e->voices[v].age)v=i;}
    choke_voice(e,&e->voices[v]);Voice *voice=&e->voices[v];
    voice->slot=hit->slot;voice->pos=0;voice->gain=hit->gain;voice->note=hit->note;voice->channel=hit->channel;voice->step=hit->step;voice->age=++e->age;
}
static void remove_pending(Engine *e,unsigned i) {
    drop(e,e->pending[i].slot);--e->pending_count;
    memmove(e->pending+i,e->pending+i+1,(e->pending_count-i)*sizeof(PendingHit));
}
static void queue_hit(Engine *e,const PendingHit *hit) {
    /* Only pathological >64 queued hits in a30ms window reach this bound.
     * Retire the earliest queued hit rather than allocating or blocking. */
    if(e->pending_count==PENDING_HITS){remove_pending(e,0);++e->pending_dropped;}
    unsigned i=e->pending_count++;
    while(i>0 && e->pending[i-1].due>hit->due){e->pending[i]=e->pending[i-1];--i;}
    e->pending[i]=*hit;
}
static void start_due_hits(Engine *e) {
    while(e->pending_count && e->pending[0].due<=e->sample_clock){
        PendingHit hit=e->pending[0];--e->pending_count;
        memmove(e->pending,e->pending+1,e->pending_count*sizeof(PendingHit));
        start_hit(e,&hit);
    }
}
static void adopt(Engine *e) {
    int s=atomic_exchange(&e->ready,-1);
    if(s>=0) { int old=e->current; e->current=s; drop(e,old); ++e->display_revision; }
}
static void envelope(struct gkick_envelope *env,float x1,float y0,float y1,float yend) {
    struct gkick_envelope_point_info pts[]={{0,y0,false},{x1,y1,false},{1,yend,false}};
    gkick_envelope_set_points(env,pts,3);
}
static float clampf(float v,float lo,float hi) { return fminf(hi,fmaxf(lo,v)); }

static void set_param(void *inst,const char *key,const char *text);
#include "point_editor.h"
#include "free_envelope.h"
#include "filter_controls.h"
#include "human_controls.h"

/* Worker-owned DSP setup. All allocations and upstream mutexes remain here. */
static void configure(Engine *e,const float *p) {
    struct gkick_synth *s=e->synth;
    s->length=value(p,"length"); s->amplitude=1;
    for(int l=0;l<3;l++) { char key[24]; snprintf(key,sizeof key,"layer%d",l+1); s->osc_groups_amplitude[l]=value(p,key); s->osc_groups[l]=s->osc_groups_amplitude[l]>0; }
    static const int wave[]={0,1,2,3,4,6};
    for(int i=0;i<9;i++) {
        struct gkick_oscillator *o=s->oscillators[i]; char key[40];
#define OVAL(suffix) (snprintf(key,sizeof key,"o%d_%s",i+1,suffix),value(p,key))
        o->state=OVAL("on")>0?GEONKICK_OSC_STATE_ENABLED:GEONKICK_OSC_STATE_DISABLED;
        o->func=wave[(int)OVAL("wave")];
        o->amplitude=OVAL("level");
        float base=OVAL("freq"), sweep=OVAL("sweep");
        float start=fminf(20000,base*exp2f(sweep/12));
        o->frequency=start;
        o->initial_phase=OVAL("phase")*(float)M_PI/180;
        o->noise_density=OVAL("density");
        o->seed=100u+37u*(unsigned)i; o->brownian=0;
        bool target_on=false;
        if(i%3==0) { snprintf(key,sizeof key,"o%d_on",i+2);target_on=value(p,key)>0; }
        o->is_fm=(i%3==0 && OVAL("fm")>0 && target_on);
        o->fm_k=1;
        if(i%3==1) { snprintf(key,sizeof key,"o%d_fm",i); o->fm_k=value(p,key)*8; }
        float attack=OVAL("attack"),decay=OVAL("decay");
        struct gkick_envelope_point_info pts[4];int n=0;
        pts[n++]=(struct gkick_envelope_point_info){0,attack>0?0:1,false};
        if(attack>0 && attack<s->length)pts[n++]=(struct gkick_envelope_point_info){attack/s->length,1,false};
        if(attack+decay<s->length)pts[n++]=(struct gkick_envelope_point_info){(attack+decay)/s->length,0,false};
        float endlevel=attack>=s->length?s->length/attack:clampf(1-(s->length-attack)/decay,0,1);
        pts[n++]=(struct gkick_envelope_point_info){1,endlevel,false};
        gkick_envelope_set_points(o->envelopes[GKICK_OSC_AMPLITUDE_ENVELOPE],pts,n);
        float pitch_time=OVAL("sweep_time");
        if(pitch_time<s->length)envelope(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE],pitch_time/s->length,1,base/start,base/start);
        else {
            struct gkick_envelope_point_info pitch[]={{0,1,false},{1,1-(1-base/start)*s->length/pitch_time,false}};
            gkick_envelope_set_points(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE],pitch,2);
        }
        gkick_envelope_set_apply_type(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE],GEONKICK_ENVELOPE_APPLY_LINEAR);
        if(point_value(p,i,0,"mode")>0)free_apply(p,i,0,o->envelopes[GKICK_OSC_AMPLITUDE_ENVELOPE],1);
        if(point_value(p,i,1,"mode")>0){o->frequency=point_upper(base);free_apply(p,i,1,o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE],o->frequency);}

        o->filter_enabled=OVAL("filter_on")>0;
        gkick_filter_set_type(o->filter,(int)OVAL("filter_type"));
        char filter_prefix[8];snprintf(filter_prefix,sizeof filter_prefix,"o%d_",i+1);
        filter_configure(o->filter,p,filter_prefix);
        o->distortion->enabled=OVAL("drive_on")>0;
        o->distortion->type=(int)OVAL("drive_type");
        o->distortion->drive=OVAL("drive");
        o->distortion->out_limiter=OVAL("drive_out");
#undef OVAL
    }
    s->filter_enabled=value(p,"filter_on")>0;
    gkick_filter_set_type(s->filter,(int)value(p,"filter_type"));
    filter_configure(s->filter,p,"");
    s->distortion->enabled=value(p,"drive_on")>0;
    s->distortion->type=(int)value(p,"drive_type");
    s->distortion->drive=value(p,"drive");
    s->distortion->out_limiter=value(p,"drive_out");
}
static void synthesize(Engine *e,Cache *c,const float *p,unsigned rev) {
    atomic_fetch_add(&e->synthesis_count,1);
    configure(e,p);
    struct gkick_synth *s=e->synth;
    c->size=(int)(s->length*RATE); c->revision=rev;
    gkick_synth_reset_oscillators(s); gkick_filter_init(s->filter);
    /* Mirrors the upstream gkick_synth_process sample loop, including limiting.
     * A short boundary fade prevents truncation clicks at the fixed sound end. */
    float dt=s->length/c->size;
    for(int i=0;i<c->size;i++) {
        float x=gkick_synth_get_value(s,i*dt);
        x=isfinite(x)?clampf(x,-1,1):0;
        float fade=fminf(1,i/32.f)*edge_gain((c->size-1-i)/(float)(EDGE_SAMPLES-1));
        c->data[i]=x*fade;
    }
}
static void make_plot(Engine *e,Cache *c,const float *p) {
    struct gkick_synth *s=e->synth;
    int osc=(int)value(p,"plot_osc");
    struct gkick_oscillator *selected=s->oscillators[osc];
    struct gkick_envelope *env=selected->envelopes[value(p,"plot_env")>0?GKICK_OSC_FREQUENCY_ENVELOPE:GKICK_OSC_AMPLITUDE_ENVELOPE];
    for(int column=0;column<PLOT_COLUMNS;column++) {
        int begin=(int)((long long)column*c->size/PLOT_COLUMNS);
        int end=(int)((long long)(column+1)*c->size/PLOT_COLUMNS);
        float positive=0,negative=0;
        for(int i=begin;i<end;i++) { positive=fmaxf(positive,c->data[i]);negative=fmaxf(negative,-c->data[i]); }
        c->plot[0][column]=(unsigned char)roundf(clampf(positive,0,1)*(PLOT_WAVE_LEVELS-1));
        c->plot[1][column]=(unsigned char)roundf(clampf(negative,0,1)*(PLOT_WAVE_LEVELS-1));
        float shape=gkick_envelope_get_value(env,(column+0.5f)/PLOT_COLUMNS);
        if(value(p,"plot_env")>0 && point_value(p,osc,1,"mode")>0)shape=(shape*selected->frequency-20)/(selected->frequency-20);

        c->plot[2][column]=(unsigned char)roundf(clampf(shape,0,1)*(PLOT_ENV_LEVELS-1));
    }
}
static unsigned snapshot(Engine *e,float *p,unsigned *view) {
    unsigned a,b;
    do {
        a=atomic_load(&e->revision);
        if(a&1) continue;
        for(int i=0;i<GP_COUNT;i++) p[i]=atomic_load(&e->values[i]);
        unsigned audio=atomic_load(&e->audio_revision);
        *view=atomic_load(&e->view_revision);
        b=atomic_load(&e->revision);
        if(a==b) return audio;
    } while(true);
}
static uint64_t worker_clock_ns(void) { struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000u+t.tv_nsec; }
static void *worker(void *arg) {
    Engine *e=arg; unsigned done=0,view_done=0; float p[GP_COUNT]; int held=0;uint64_t next_synthesis=0;
    pthread_setname_np(pthread_self(),"gk-render");
    while(atomic_load(&e->running)) {
        unsigned view,rev=snapshot(e,p,&view);
        /* Coalesce fast knob input to at most ~30 render starts/second.
         * Complete each chosen snapshot: aborting on every newer input starves
         * audible updates while the wheel is moving. */
        if(rev!=done && worker_clock_ns()<next_synthesis) { usleep(2000);continue; }
        if(rev!=done || view!=view_done) {
            int slot=-1;
            for(int i=0;i<SLOTS;i++) { int free=0; if(atomic_compare_exchange_strong(&e->slots[i].refs,&free,-1)) { slot=i; break; } }
            if(slot>=0) {
                Cache *c=&e->slots[slot];
                if(rev!=done) { next_synthesis=worker_clock_ns()+33000000u;synthesize(e,c,p,rev); }
                else {
                    /* A view switch reuses audio. held owns a reference even
                     * when old notes, the ready queue and UI change slots. */
                    configure(e,p);c->size=e->slots[held].size;c->revision=rev;
                    memcpy(c->data,e->slots[held].data,c->size*sizeof(float));
                }
                {
                    make_plot(e,c,p);c->view_revision=view;
                    atomic_store(&c->refs,2); /* worker + ready/current */
                    drop(e,atomic_exchange(&e->ready,slot));
                    drop(e,held);held=slot;done=rev;view_done=view;
                }
            }
        }
        usleep(2000);
    }
    drop(e,held);return NULL;
}

static void preset(Engine *e,int which) {
    stop_pending(e);
    for(int i=0;i<GP_COUNT;i++) atomic_store(&e->values[i],gparams[i].def);
    put(e,"preset",which);
    switch(which) {
    case 1: put(e,"o1_freq",43); put(e,"o1_decay",1.1); put(e,"length",1.2); put(e,"o1_sweep",30); put(e,"o1_sweep_time",0.065); break;
    case 2: put(e,"length",0.24); put(e,"o1_decay",0.22); put(e,"o1_freq",65); put(e,"o1_sweep_time",0.018); break;
    case 3:
        put(e,"length",0.3); put(e,"o1_freq",180); put(e,"o1_level",0.45); put(e,"o1_sweep",4); put(e,"o1_decay",0.15);
        put(e,"o2_on",1); put(e,"o2_wave",4); put(e,"o2_level",0.65); put(e,"o2_sweep",0); put(e,"o2_decay",0.23); break;
    case 4: case 5:
        put(e,"o1_wave",4); put(e,"o1_sweep",0); put(e,"length",which==4?0.1:0.65); put(e,"o1_decay",which==4?0.075:0.6);
        put(e,"filter_on",1); put(e,"filter_type",1); put(e,"cutoff",6500);put(e,"cutoff_wide",6500);put(e,"filter_model",0); break;
    case 6:
        put(e,"length",0.5); put(e,"o1_wave",1); put(e,"o1_freq",370); put(e,"o1_level",0.35); put(e,"o1_sweep",0); put(e,"o1_fm",0.6);
        put(e,"o2_on",1); put(e,"o2_freq",541); put(e,"o2_sweep",0); put(e,"o2_decay",0.45); break;
    case 7: put(e,"o1_freq",115); put(e,"length",0.6); put(e,"o1_sweep",7); put(e,"o1_sweep_time",0.12); break;
    }
    e->edit_time_valid=0;memset(e->dirty,1,sizeof e->dirty);e->point_dirty|=POINT_ALL;e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1;
}
static void *create(const char *dir) {
    (void)dir;
    Engine *e=calloc(1,sizeof *e); if(!e)return NULL;
    e->current=-1; e->display_revision=1; atomic_init(&e->ready,-1); atomic_init(&e->revision,0); atomic_init(&e->audio_revision,0); atomic_init(&e->view_revision,0); atomic_init(&e->synthesis_count,0); atomic_init(&e->running,false);
    for(int i=0;i<GP_COUNT;i++) atomic_init(&e->values[i],gparams[i].def);
    for(int i=0;i<VOICES;i++)e->voices[i].slot=-1;
    for(int i=0;i<SLOTS;i++) { atomic_init(&e->slots[i].refs,0); e->slots[i].data=calloc(MAX_SAMPLES,sizeof(float)); if(!e->slots[i].data)goto fail; }
    gkick_humanizer_init(&e->humanizer);
    e->output_gain=at(e,"volume");
    if(gkick_synth_new(&e->synth,RATE)!=GEONKICK_OK)goto fail;
    float p[GP_COUNT]; unsigned view; snapshot(e,p,&view); synthesize(e,&e->slots[0],p,0); make_plot(e,&e->slots[0],p); atomic_store(&e->slots[0].refs,2); e->current=0;
    atomic_store(&e->running,true);
    if(pthread_create(&e->worker,NULL,worker,e)!=0) { atomic_store(&e->running,false); goto fail; }
    return e;
fail:
    if(e->synth)gkick_synth_free(&e->synth);
    for(int i=0;i<SLOTS;i++)free(e->slots[i].data);
    free(e); return NULL;
}
static void destroy(void *inst) {
    Engine *e=inst; if(!e)return;
    atomic_store(&e->running,false); pthread_join(e->worker,NULL);
    gkick_synth_free(&e->synth); for(int i=0;i<SLOTS;i++)free(e->slots[i].data); free(e);
}
static void midi(void *inst,const uint8_t *msg,int len) {
    Engine *e=inst; if(!e||len<3)return;
    int type=msg[0]&0xf0,ch=msg[0]&15,note=msg[1]&127,vel=msg[2]&127;
    if(type==0xb0 && (note==120||note==123)) { stop_all(e); return; }
    if(type==0x80 || (type==0x90&&!vel)) {
        if(at(e,"playback")==2)for(int i=0;i<VOICES;i++)if(e->voices[i].note==note && e->voices[i].channel==ch)stop_voice(e,&e->voices[i]);
        for(unsigned i=0;i<e->pending_count;) {
            PendingHit *p=&e->pending[i];
            if((p->playback==2 || at(e,"playback")==2) && p->note==note && p->channel==ch)remove_pending(e,i);else ++i;
        }
        return;
    }
    if(type!=0x90)return;
    adopt(e);
    int delay=0;
    if(at(e,"human_on")>0) {
        gkick_humanizer_enable(&e->humanizer,true);
        float amount=at(e,"human_velocity"),timing=at(e,"human_timing");
        if(e->humanizer.velocity_percent!=amount)gkick_humanizer_set_velocity_percent(&e->humanizer,amount);
        if(e->humanizer.timing!=timing)gkick_humanizer_set_timing_percent(&e->humanizer,timing);
        signed char velocity=(signed char)vel;float ms=0;
        gkick_humanizer_velocity(&e->humanizer,&velocity);gkick_humanizer_timing(&e->humanizer,&ms);
        int lower=(int)fmaxf(1,vel*(1-amount/100)),upper=(int)fminf(127,vel*(1+amount/100));
        vel=(int)clampf((unsigned char)velocity,lower,upper);delay=(int)lroundf(ms*(RATE/1000.f));
    }
    e->last_human_velocity=vel;e->last_human_delay=delay;
    PendingHit hit={.slot=e->current,.note=note,.channel=ch,.playback=(int)at(e,"playback"),
        .step=exp2((at(e,"tune")+(at(e,"keytrack")>0?note-60:0))/12.0),.gain=vel/127.f,.due=e->sample_clock+(unsigned)delay};
    atomic_fetch_add(&e->slots[hit.slot].refs,1);
    if(delay)queue_hit(e,&hit);else start_hit(e,&hit);
}
static void set_param(void *inst,const char *key,const char *text) {
    Engine *e=inst;
    if(human_set(e,key,text))return;
    if(filter_virtual_set(e,key,text))return;
    if(free_xy_set(e,key,text))return;
    if(free_virtual_set(e,key,text))return;
    if(point_virtual_set(e,key,text))return;
    int alias=alias_of(key);
    if(alias>=0) {
        char *end;float v=strtof(text,&end);if(end==text||*end||!isfinite(v))return;
        /* A host echo of an adapter's rounded display coordinate must not
         * quantize physical values restored from old projects/automation. */
        float current=at(e,galiases[alias].target);
        float coordinate=alias_coordinate(alias,current);
        if(v==coordinate || (galiases[alias].kind>=3 && fabsf(v-coordinate)<1e-6f))return;
        const GAlias *a=&galiases[alias];
        float physical=a->kind==3?20.f*expf(logf(600.f)*clampf(v,0,1)):a->kind>=4?a->lo+(a->hi-a->lo)*clampf(v,0,1):a->kind==1?v/1000.f:a->kind==2?v/100.f:clampf(55.f*exp2f(v/240.f),20,12000);
        char value[48];snprintf(value,sizeof value,"%.9g",physical);
        set_param(e,galiases[alias].target,value);
        for(int j=0;j<G_ALIAS_COUNT;j++)if((galiases[j].kind>=3)!=(a->kind>=3) && !strcmp(galiases[j].target,a->target))e->dirty_alias[j]=1;
        return;
    }
    if(!strcmp(key,"state")) {
        int count;
        if(!strncmp(text,"GKMP1 ",6))count=103;
        else if(!strncmp(text,"GKMP2 ",6))count=184;
        else if(!strncmp(text,"GKMP3 ",6))count=LEGACY_GP_COUNT;
        else if(!strncmp(text,"GKMP4 ",6))count=GKMP4_GP_COUNT;
        else if(!strncmp(text,"GKMP5 ",6))count=GKMP5_GP_COUNT;
        else if(!strncmp(text,"GKMP6 ",6))count=GKMP6_GP_COUNT;
        else if(!strncmp(text,"GKMP7 ",6))count=GP_COUNT;
        else return;
        float p[GP_COUNT];const char *s=text+6;char *end;
        for(int i=0;i<GP_COUNT;i++)p[i]=gparams[i].def;
        for(int i=0;i<count;i++) { p[i]=strtof(s,&end); if(end==s||!isfinite(p[i])||p[i]<gparams[i].lo||p[i]>gparams[i].hi)return; s=end; }
        while(*s==' ')s++;
        if(*s || !free_validate(p))return;
        if(count<=GKMP5_GP_COUNT)filter_migrate(p);
        atomic_fetch_add(&e->revision,1);
        for(int i=0;i<GP_COUNT;i++)atomic_store(&e->values[i],gparams[i].discrete?roundf(p[i]):p[i]);
        atomic_fetch_add(&e->audio_revision,1);atomic_fetch_add(&e->view_revision,1);atomic_fetch_add(&e->revision,1); stop_all(e); e->edit_time_valid=0;memset(e->dirty,1,sizeof e->dirty);e->point_dirty|=POINT_ALL;e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1; return;
    }
    if(key[0]=='f' && key[1]>='1' && key[1]<='9' && key[2]=='_')return; /* private curve storage */
    int i=index_of(key); if(i<0)return;
    char *end;float v=strtof(text,&end);if(end==text||*end||!isfinite(v))return;
    v=clampf(v,gparams[i].lo,gparams[i].hi);if(gparams[i].discrete)v=roundf(v);
    if(!strcmp(key,"audition")) { if(v>0) { uint8_t m[]={0x90,60,110};midi(e,m,3); }return; }
    if(!strcmp(key,"reset")) {
        if(v>0) { atomic_fetch_add(&e->revision,1);preset(e,0);atomic_fetch_add(&e->audio_revision,1);atomic_fetch_add(&e->view_revision,1);atomic_fetch_add(&e->revision,1); }
        return;
    }
    PointKey point;int point_key_changed=point_decode(key,&point);if(point_key_changed && point.field=='s')return;
    if(v==atomic_load(&e->values[i]))return;
    atomic_fetch_add(&e->revision,1);
    if(!strcmp(key,"preset"))preset(e,(int)v);
    else {
        if(point_key_changed)point_prepare(e,&point,v);
        atomic_store(&e->values[i],v);
        e->dirty[i]=1;filter_legacy_changed(e,key,v);
        if(point_key_changed){point_changed(e,&point);e->edit_time_valid=0;
            if(point.field=='l'||point.field=='t')free_store(e,point.osc,point.kind,"n",0);
            e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1;
        }
        if(!strcmp(key,"plot_osc") || !strcmp(key,"plot_env") || !strcmp(key,"point_select")){e->point_dirty|=POINT_ALL;e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1;}
        int oscillator=0;char suffix[32];
        if(sscanf(key,"o%d_%31s",&oscillator,suffix)==2 && oscillator>=1 && oscillator<=9) {
            int focus=index_of("plot_osc");
            if(atomic_load(&e->values[focus])!=oscillator-1) { atomic_store(&e->values[focus],oscillator-1);e->dirty[focus]=1;e->point_dirty|=POINT_ALL;e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1; }
            if(!strcmp(suffix,"attack") || !strcmp(suffix,"decay")){point_store(e,oscillator-1,0,"mode",0);e->point_dirty|=POINT_ALL;e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1;}
            if(!strcmp(suffix,"sweep") || !strcmp(suffix,"sweep_time")){point_store(e,oscillator-1,1,"mode",0);e->point_dirty|=POINT_ALL;e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1;}
            if(!strcmp(suffix,"attack") || !strcmp(suffix,"decay")) {
                char other[32];snprintf(other,sizeof other,"o%d_%s",oscillator,!strcmp(suffix,"attack")?"decay":"attack");
                float needed=fminf(4,v+at(e,other));int length=index_of("length");
                if(needed>atomic_load(&e->values[length])) { atomic_store(&e->values[length],needed);e->dirty[length]=1; }
            }
            if(!strcmp(suffix,"freq"))e->point_dirty|=3;
            int env=index_of("plot_env"),mode=-1;
            if(!strcmp(suffix,"attack") || !strcmp(suffix,"decay") || !strcmp(suffix,"density"))mode=0;
            if(!strcmp(suffix,"sweep") || !strcmp(suffix,"sweep_time"))mode=1;
            if(mode>=0 && atomic_load(&e->values[env])!=mode) { atomic_store(&e->values[env],mode);e->dirty[env]=1; }
        }
    }
    bool view_only=!strcmp(key,"plot_osc") || !strcmp(key,"plot_env") || !strcmp(key,"point_select");
    bool playback_only=!strcmp(key,"volume") || !strcmp(key,"tune") || !strcmp(key,"keytrack") || !strcmp(key,"playback");
    if(!strcmp(key,"human_on") || !strcmp(key,"human_velocity") || !strcmp(key,"human_timing")){playback_only=true;++e->ui_revision;}
    if(view_only)atomic_fetch_add(&e->view_revision,1);
    else if(!playback_only)atomic_fetch_add(&e->audio_revision,1);
    atomic_fetch_add(&e->revision,1);
}
static int get_param(void *inst,const char *key,char *buf,int len) {
    Engine *e=inst;if(!e||len<1)return 0;
    /* Wrapper serializes this call with MIDI/render. The display thread can
     * adopt a completed sound even while the host isn't processing audio. */
    if(!strcmp(key,"human_last_velocity"))return snprintf(buf,len,"%d",e->last_human_velocity);
    if(!strcmp(key,"human_last_delay"))return snprintf(buf,len,"%d",e->last_human_delay);
    if(!strcmp(key,"pending_count"))return snprintf(buf,len,"%u",e->pending_count);
    if(!strcmp(key,"pending_dropped"))return snprintf(buf,len,"%u",e->pending_dropped);
    int human_result=human_get(e,key,buf,len);if(human_result>0)return human_result;
    int filter_result=filter_virtual_get(e,key,buf,len);if(filter_result>0)return filter_result;
    int xy_result=free_xy_get(e,key,buf,len);if(xy_result>0)return xy_result;
    int free_result=free_virtual_get(e,key,buf,len);if(free_result>0)return free_result;
    int point_result=point_virtual_get(e,key,buf,len);if(point_result>0)return point_result;
    if(!strcmp(key,"display_rev")) { adopt(e);return snprintf(buf,len,"%u",e->display_revision+e->ui_revision); }
    if(!strcmp(key,"graph_revision"))return snprintf(buf,len,"%u",e->display_revision%1024);
    if(!strcmp(key,"ui_env_focus")) {
        int focus=(int)at(e,"plot_osc");char wave[32];snprintf(wave,sizeof wave,"o%d_wave",focus+1);
        return snprintf(buf,len,"%d",2*focus+(at(e,wave)>=4));
    }
    size_t key_len=strlen(key);
    if(key_len>8 && !strcmp(key+key_len-8,"_display")) {
        char original[64];if(key_len-8>=sizeof original)return 0;
        memcpy(original,key,key_len-8);original[key_len-8]=0;
        int alias=alias_of(original);
        if(alias>=0) {
            float physical=at(e,galiases[alias].target);
            if(galiases[alias].kind%3==0) {
                int osc;char wave[32];
                if(sscanf(galiases[alias].target,"o%d_freq",&osc)==1) {
                    snprintf(wave,sizeof wave,"o%d_wave",osc);
                    if(at(e,wave)>=4)return snprintf(buf,len,"NO PITCH");
                }
            }
            int unit=galiases[alias].kind%3;float displayed=unit==1?physical*1000:unit==2?physical*100:physical;
            return snprintf(buf,len,unit?"%.0f":"%.2f",displayed);
        }
        int i=index_of(original);if(i<0)return 0;
        return snprintf(buf,len,"%.3f",atomic_load(&e->values[i]));
    }
    if(!strncmp(key,"viz_",4)) {
        char channel,trailing;int column;
        if(sscanf(key,"viz_%c_%d%c",&channel,&column,&trailing)!=2 || column<0 || column>=PLOT_COLUMNS)return 0;
        int c=channel=='p'?0:channel=='n'?1:channel=='e'?2:-1;if(c<0)return 0;
        return snprintf(buf,len,"%.9g",e->slots[e->current].plot[c][column]/(float)((c==2?PLOT_ENV_LEVELS:PLOT_WAVE_LEVELS)-1));
    }
    if(!strcmp(key,"synthesis_count"))return snprintf(buf,len,"%u",atomic_load(&e->synthesis_count));
    if(!strcmp(key,"render_pending"))return snprintf(buf,len,"%d",(e->slots[e->current].revision!=atomic_load(&e->audio_revision) || e->slots[e->current].view_revision!=atomic_load(&e->view_revision)));
    if(!strcmp(key,"state")) {
        int n=snprintf(buf,len,"GKMP7");
        for(int i=0;i<GP_COUNT && n<len;i++)n+=snprintf(buf+n,len-n," %.9g",atomic_load(&e->values[i]));
        return n<len?n:0;
    }
    int alias=alias_of(key);
    if(alias>=0) {
        float physical=at(e,galiases[alias].target);
        float v=alias_coordinate(alias,physical);
        return snprintf(buf,len,"%.9g",v);
    }
    int i=index_of(key);if(i<0)return 0;
    return snprintf(buf,len,"%.9g",atomic_load(&e->values[i]));
}
int mpc_engine_changed(void *inst,char *key,int len) {
    Engine *e=inst;for(int i=0;i<GP_COUNT;i++)if(e->dirty[i]) { e->dirty[i]=0;snprintf(key,len,"%s",gparams[i].key);return 1; }
    for(int i=0;i<G_ALIAS_COUNT;i++)if(e->dirty_alias[i]) { e->dirty_alias[i]=0;snprintf(key,len,"%s",galiases[i].key);return 1; }
    for(int i=0;i<FREE_NOTIFY_COUNT;i++)if(e->free_dirty&(1u<<i)){e->free_dirty&=~(1u<<i);snprintf(key,len,"%s",free_notifications[i]);return 1;}
    for(int i=0;i<6;i++)if(e->point_dirty&(1u<<i)){e->point_dirty&=~(1u<<i);snprintf(key,len,"%s",point_notifications[i]);return 1;}
    return 0;
}
static void render(void *inst,mpc_sample_t *out,int frames) {
    Engine *e=inst;adopt(e);float volume=at(e,"volume");
    for(int n=0;n<frames;n++) {
        start_due_hits(e);
        float sum=e->cut_tail[e->cut_pos];e->cut_tail[e->cut_pos]=0;
        if(++e->cut_pos==EDGE_SAMPLES)e->cut_pos=0;
        for(int i=0;i<VOICES;i++) {
            Voice *v=&e->voices[i];if(v->slot<0)continue;
            Cache *c=&e->slots[v->slot];int at=(int)v->pos;
            if(at>=c->size) { stop_voice(e,v);continue; }
            sum+=voice_sample(c,v->pos)*v->gain;v->pos+=v->step;
        }
        /* A 5 ms gain transition avoids block-boundary clicks when OUTPUT
         * changes; the initial value and unchanged gain remain exact. */
        e->output_gain+=(volume-e->output_gain)*(1.f/(0.005f*RATE));
        out[n*2]=out[n*2+1]=sum*e->output_gain;++e->sample_clock;
    }
}
static const mpc_engine_t api={create,destroy,midi,set_param,get_param,render,NULL,NULL};
const mpc_engine_t *mpc_engine(void) { return &api; }
