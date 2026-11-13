/* MPC adapter, GPL-3.0-or-later. Original synthesis lives in src/dsp.
 * Only the worker touches the DSP. Audio uses fixed buffers and reference counts;
 * it never waits for synthesis, allocates, locks a mutex, or touches a file. */
#include "params.h"
#include "engine.h"
#include "param_defs.h"
#include "dsp/synthesizer.h"
#include "dsp/oscillator.h"
#include "dsp/envelope.h"
#include "dsp/filter.h"
#include <unistd.h>
#include <stdatomic.h>

#define RATE 44100
#define MAX_SAMPLES (RATE*4)
#define VOICES 8
#define SLOTS (VOICES+4)
typedef struct {
    atomic_int refs; int size; float *data; unsigned revision;
    unsigned char plot[3][PLOT_COLUMNS];
} Cache;
typedef struct { int slot,note,channel; double pos,step; float gain; unsigned age; } Voice;
typedef struct {
    _Atomic float values[GP_COUNT];
    atomic_uint revision;
    atomic_bool running;
    atomic_int ready;
    pthread_t worker;
    struct gkick_synth *synth;
    Cache slots[SLOTS];
    Voice voices[VOICES];
    int current;
    unsigned age;
    unsigned display_revision;
    unsigned char dirty[GP_COUNT];
} Engine;

/* Same platform helpers as upstream geonkick.c, without its global worker. */
void geonkick_usleep(unsigned long usecs) { usleep(usecs); }
int geonkick_rand(unsigned int *seed) { return rand_r(seed); }

static int index_of(const char *key) {
    for(int i=0;i<GP_COUNT;i++) if(!strcmp(key,gparams[i].key)) return i;
    return -1;
}
static float at(Engine *e,const char *key) { int i=index_of(key); return i<0?0:atomic_load(&e->values[i]); }
static float value(const float *p,const char *key) { int i=index_of(key); return i<0?0:p[i]; }
static void put(Engine *e,const char *key,float v) { int i=index_of(key); if(i>=0) atomic_store(&e->values[i],v); }
static void drop(Engine *e,int slot) { if(slot>=0) atomic_fetch_sub(&e->slots[slot].refs,1); }
static void stop_voice(Engine *e,Voice *v) { drop(e,v->slot); v->slot=-1; }
static void stop_all(Engine *e) { for(int i=0;i<VOICES;i++) stop_voice(e,&e->voices[i]); }
static void adopt(Engine *e) {
    int s=atomic_exchange(&e->ready,-1);
    if(s>=0) { int old=e->current; e->current=s; drop(e,old); ++e->display_revision; }
}
static void envelope(struct gkick_envelope *env,float x1,float y0,float y1,float yend) {
    struct gkick_envelope_point_info pts[]={{0,y0,false},{x1,y1,false},{1,yend,false}};
    gkick_envelope_set_points(env,pts,3);
}
static float clampf(float v,float lo,float hi) { return fminf(hi,fmaxf(lo,v)); }

/* Worker-owned DSP setup. All allocations and upstream mutexes remain here. */
static void configure(Engine *e,const float *p) {
    struct gkick_synth *s=e->synth;
    s->length=value(p,"length"); s->amplitude=1;
    for(int l=0;l<3;l++) { char key[24]; snprintf(key,sizeof key,"layer%d",l+1); s->osc_groups[l]=true; s->osc_groups_amplitude[l]=value(p,key); }
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
        o->is_fm=(i%3==0 && OVAL("fm")>0);
        o->fm_k=1;
        if(i%3==1) { snprintf(key,sizeof key,"o%d_fm",i); o->fm_k=value(p,key)*8; }
        float attack=OVAL("attack"),decay=OVAL("decay");
        if(attack>0) {
            float end=clampf((attack+decay)/s->length,0.0002f,0.9999f);
            float peak=clampf(attack/s->length,0.0001f,end-0.0001f);
            struct gkick_envelope_point_info pts[]={{0,0,false},{peak,1,false},{end,0,false},{1,0,false}};
            gkick_envelope_set_points(o->envelopes[GKICK_OSC_AMPLITUDE_ENVELOPE],pts,4);
        } else envelope(o->envelopes[GKICK_OSC_AMPLITUDE_ENVELOPE],clampf(decay/s->length,0.0001f,0.9999f),1,0,0);
        float pt=clampf(OVAL("sweep_time")/s->length,0.0001f,0.9999f);
        envelope(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE],pt,1,base/start,base/start);
        gkick_envelope_set_apply_type(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE],GEONKICK_ENVELOPE_APPLY_LINEAR);
        o->filter_enabled=OVAL("filter_on")>0;
        gkick_filter_set_type(o->filter,(int)OVAL("filter_type"));
        gkick_filter_set_cutoff_freq(o->filter,OVAL("cutoff"));
        gkick_filter_set_factor(o->filter,OVAL("q"));
        o->distortion->enabled=OVAL("drive_on")>0;
        o->distortion->type=(int)OVAL("drive_type");
        o->distortion->drive=OVAL("drive");
        o->distortion->out_limiter=OVAL("drive_out");
#undef OVAL
    }
    s->filter_enabled=value(p,"filter_on")>0;
    gkick_filter_set_type(s->filter,(int)value(p,"filter_type"));
    gkick_filter_set_cutoff_freq(s->filter,value(p,"cutoff"));
    gkick_filter_set_factor(s->filter,value(p,"q"));
    s->distortion->enabled=value(p,"drive_on")>0;
    s->distortion->type=(int)value(p,"drive_type");
    s->distortion->drive=value(p,"drive");
    s->distortion->out_limiter=value(p,"drive_out");
}
static void synthesize(Engine *e,Cache *c,const float *p,unsigned rev) {
    configure(e,p);
    struct gkick_synth *s=e->synth;
    c->size=(int)(s->length*RATE); c->revision=rev;
    gkick_synth_reset_oscillators(s); gkick_filter_init(s->filter);
    /* Mirrors the upstream gkick_synth_process sample loop, including limiting.
     * A short boundary fade prevents truncation clicks at the fixed sound end. */
    float dt=s->length/c->size;
    for(int i=0;i<c->size;i++) {
        if(!(i%1024) && atomic_load(&e->revision)!=rev)return;
        float x=gkick_synth_get_value(s,i*dt);
        x=isfinite(x)?clampf(x,-1,1):0;
        float fade=fminf(1,fminf(i/32.f,(c->size-1-i)/64.f));
        c->data[i]=x*fade;
    }
    int osc=(int)value(p,"plot_osc");
    struct gkick_oscillator *selected=s->oscillators[osc];
    struct gkick_envelope *env=selected->envelopes[value(p,"plot_env")>0?GKICK_OSC_FREQUENCY_ENVELOPE:GKICK_OSC_AMPLITUDE_ENVELOPE];
    for(int column=0;column<PLOT_COLUMNS;column++) {
        int begin=(int)((long long)column*c->size/PLOT_COLUMNS);
        int end=(int)((long long)(column+1)*c->size/PLOT_COLUMNS);
        float positive=0,negative=0;
        for(int i=begin;i<end;i++) { positive=fmaxf(positive,c->data[i]);negative=fmaxf(negative,-c->data[i]); }
        c->plot[0][column]=(unsigned char)roundf(clampf(positive,0,1)*63);
        c->plot[1][column]=(unsigned char)roundf(clampf(negative,0,1)*63);
        float shape=gkick_envelope_get_value(env,(column+0.5f)/PLOT_COLUMNS);
        c->plot[2][column]=(unsigned char)roundf(clampf(shape,0,1)*63);
    }
}
static unsigned snapshot(Engine *e,float *p) {
    unsigned a,b;
    do {
        a=atomic_load(&e->revision);
        if(a&1) continue;
        for(int i=0;i<GP_COUNT;i++) p[i]=atomic_load(&e->values[i]);
        b=atomic_load(&e->revision);
        if(a==b) return a;
    } while(true);
}
static void *worker(void *arg) {
    Engine *e=arg; unsigned done=0; float p[GP_COUNT];
    while(atomic_load(&e->running)) {
        unsigned rev=snapshot(e,p);
        if(rev!=done) {
            int s=-1;
            for(int i=0;i<SLOTS;i++) { int free=0; if(atomic_compare_exchange_strong(&e->slots[i].refs,&free,-1)) { s=i; break; } }
            if(s>=0) {
                synthesize(e,&e->slots[s],p,rev);
                if(atomic_load(&e->revision)==rev) {
                    atomic_store(&e->slots[s].refs,1);
                    drop(e,atomic_exchange(&e->ready,s));
                    done=rev;
                } else atomic_store(&e->slots[s].refs,0);
            }
        }
        usleep(10000);
    }
    return NULL;
}

static void preset(Engine *e,int which) {
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
        put(e,"filter_on",1); put(e,"filter_type",1); put(e,"cutoff",6500); break;
    case 6:
        put(e,"length",0.5); put(e,"o1_wave",1); put(e,"o1_freq",370); put(e,"o1_level",0.35); put(e,"o1_sweep",0); put(e,"o1_fm",0.6);
        put(e,"o2_on",1); put(e,"o2_freq",541); put(e,"o2_sweep",0); put(e,"o2_decay",0.45); break;
    case 7: put(e,"o1_freq",115); put(e,"length",0.6); put(e,"o1_sweep",7); put(e,"o1_sweep_time",0.12); break;
    }
    memset(e->dirty,1,sizeof e->dirty);
}
static void *create(const char *dir) {
    (void)dir;
    Engine *e=calloc(1,sizeof *e); if(!e)return NULL;
    e->current=-1; e->display_revision=1; atomic_init(&e->ready,-1); atomic_init(&e->revision,0); atomic_init(&e->running,false);
    for(int i=0;i<GP_COUNT;i++) atomic_init(&e->values[i],gparams[i].def);
    for(int i=0;i<VOICES;i++)e->voices[i].slot=-1;
    for(int i=0;i<SLOTS;i++) { atomic_init(&e->slots[i].refs,0); e->slots[i].data=calloc(MAX_SAMPLES,sizeof(float)); if(!e->slots[i].data)goto fail; }
    if(gkick_synth_new(&e->synth,RATE)!=GEONKICK_OK)goto fail;
    float p[GP_COUNT]; snapshot(e,p); synthesize(e,&e->slots[0],p,0); atomic_store(&e->slots[0].refs,1); e->current=0;
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
        return;
    }
    if(type!=0x90)return;
    adopt(e);
    if(at(e,"playback")==1)stop_all(e);
    int v=0;
    for(int i=0;i<VOICES;i++) { if(e->voices[i].slot<0) { v=i;break; } if(e->voices[i].age<e->voices[v].age)v=i; }
    stop_voice(e,&e->voices[v]);
    Voice *voice=&e->voices[v];
    voice->slot=e->current; atomic_fetch_add(&e->slots[voice->slot].refs,1);
    voice->pos=0; voice->gain=vel/127.f; voice->note=note; voice->channel=ch; voice->age=++e->age;
    voice->step=exp2((at(e,"tune")+(at(e,"keytrack")>0?note-60:0))/12.0);
}
static void set_param(void *inst,const char *key,const char *text) {
    Engine *e=inst;
    if(!strcmp(key,"state")) {
        int count;
        if(!strncmp(text,"GKMP1 ",6))count=103;
        else if(!strncmp(text,"GKMP2 ",6))count=184;
        else if(!strncmp(text,"GKMP3 ",6))count=GP_COUNT;
        else return;
        float p[GP_COUNT];const char *s=text+6;char *end;
        for(int i=0;i<GP_COUNT;i++)p[i]=gparams[i].def;
        for(int i=0;i<count;i++) { p[i]=strtof(s,&end); if(end==s||!isfinite(p[i])||p[i]<gparams[i].lo||p[i]>gparams[i].hi)return; s=end; }
        while(*s==' ')s++;
        if(*s)return;
        atomic_fetch_add(&e->revision,1);
        for(int i=0;i<GP_COUNT;i++)atomic_store(&e->values[i],gparams[i].discrete?roundf(p[i]):p[i]);
        atomic_fetch_add(&e->revision,1); stop_all(e); memset(e->dirty,1,sizeof e->dirty); return;
    }
    int i=index_of(key); if(i<0)return;
    char *end;float v=strtof(text,&end);if(end==text||*end||!isfinite(v))return;
    v=clampf(v,gparams[i].lo,gparams[i].hi);if(gparams[i].discrete)v=roundf(v);
    if(!strcmp(key,"audition")) { if(v>0) { uint8_t m[]={0x90,60,110};midi(e,m,3); }return; }
    if(!strcmp(key,"reset")) {
        if(v>0) { atomic_fetch_add(&e->revision,1);preset(e,0);atomic_fetch_add(&e->revision,1); }
        return;
    }
    if(v==atomic_load(&e->values[i]))return;
    atomic_fetch_add(&e->revision,1);
    if(!strcmp(key,"preset"))preset(e,(int)v);
    else {
        atomic_store(&e->values[i],v);
        int oscillator=0;char suffix[32];
        if(sscanf(key,"o%d_%31s",&oscillator,suffix)==2 && oscillator>=1 && oscillator<=9) {
            int focus=index_of("plot_osc");
            if(atomic_load(&e->values[focus])!=oscillator-1) { atomic_store(&e->values[focus],oscillator-1);e->dirty[focus]=1; }
        }
    }
    atomic_fetch_add(&e->revision,1);
}
static int get_param(void *inst,const char *key,char *buf,int len) {
    Engine *e=inst;if(!e||len<1)return 0;
    /* Wrapper serializes this call with MIDI/render. The display thread can
     * adopt a completed sound even while the host isn't processing audio. */
    if(!strcmp(key,"display_rev")) { adopt(e);return snprintf(buf,len,"%u",e->display_revision); }
    size_t key_len=strlen(key);
    if(key_len>8 && !strcmp(key+key_len-8,"_display")) {
        char original[64];if(key_len-8>=sizeof original)return 0;
        memcpy(original,key,key_len-8);original[key_len-8]=0;
        int i=index_of(original);if(i<0)return 0;
        return snprintf(buf,len,"%.3f",atomic_load(&e->values[i]));
    }
    if(!strncmp(key,"viz_",4)) {
        char channel,trailing;int column;
        if(sscanf(key,"viz_%c_%d%c",&channel,&column,&trailing)!=2 || column<0 || column>=PLOT_COLUMNS)return 0;
        int c=channel=='p'?0:channel=='n'?1:channel=='e'?2:-1;if(c<0)return 0;
        return snprintf(buf,len,"%.9g",e->slots[e->current].plot[c][column]/63.f);
    }
    if(!strcmp(key,"render_pending"))return snprintf(buf,len,"%d",e->slots[e->current].revision!=atomic_load(&e->revision));
    if(!strcmp(key,"state")) {
        int n=snprintf(buf,len,"GKMP3");
        for(int i=0;i<GP_COUNT && n<len;i++)n+=snprintf(buf+n,len-n," %.9g",atomic_load(&e->values[i]));
        return n<len?n:0;
    }
    int i=index_of(key);if(i<0)return 0;
    return snprintf(buf,len,"%.9g",atomic_load(&e->values[i]));
}
int mpc_engine_changed(void *inst,char *key,int len) {
    Engine *e=inst;for(int i=0;i<GP_COUNT;i++)if(e->dirty[i]) { e->dirty[i]=0;snprintf(key,len,"%s",gparams[i].key);return 1; }return 0;
}
static void render(void *inst,mpc_sample_t *out,int frames) {
    Engine *e=inst;adopt(e);float volume=at(e,"volume");
    for(int n=0;n<frames;n++) {
        float sum=0;
        for(int i=0;i<VOICES;i++) {
            Voice *v=&e->voices[i];if(v->slot<0)continue;
            Cache *c=&e->slots[v->slot];int at=(int)v->pos;
            if(at>=c->size) { stop_voice(e,v);continue; }
            float a=c->data[at],b=at+1<c->size?c->data[at+1]:0;
            sum+=(a+(b-a)*(float)(v->pos-at))*v->gain;v->pos+=v->step;
        }
        out[n*2]=out[n*2+1]=sum*volume;
    }
}
static const mpc_engine_t api={create,destroy,midi,set_param,get_param,render,NULL,NULL};
const mpc_engine_t *mpc_engine(void) { return &api; }
