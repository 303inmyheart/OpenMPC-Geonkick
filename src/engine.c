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
#include <unistd.h>
#include <stdatomic.h>

#define RATE 44100
#define MAX_SAMPLES (RATE*4)
#define PADS 16
#define VOICES 16
#define EDGE_SAMPLES 221 /* 5 ms, endpoints included at 44.1 kHz */
#define SLOTS (PADS+VOICES+4)
#include "factory_types.h"
#include "factory_data.h"
#include "program_bank_data.h"
#include "sample_types.h"
typedef struct {
    atomic_int refs; int size; float *data; unsigned revision,view_revision;
    unsigned char plot[3][PLOT_COLUMNS],layer_plot[3][PLOT_COLUMNS],layer_wave[3][2][PLOT_COLUMNS];
} Cache;
typedef struct { int slot,note,channel,pad,gate; double pos,step; float gain; unsigned age; } Voice;
typedef struct {
    _Atomic float pad_values[PADS][GP_COUNT];
    SampleManager samples;
    SampleAsset *worker_samples[9];struct gkick_buffer sample_views[9];
    atomic_int sound_focus[PADS][3];
    atomic_int factory_id[PADS];
    int factory_was_configured;
    int bank_program;
    _Atomic float *values; /* selected row; only serialized host/audio entry points use it */
    atomic_uint pad_audio[PADS],pad_view[PADS];
    atomic_int selected;
    int kit_mode,kit_base,kit_mapping,kit_notes[PADS];
    int kit_learn,kit_learn_count,kit_stage[PADS],kit_duplicate,kit_learn_done;
    float kit_output,kit_gain;
    unsigned kit_dirty,control_context;
    atomic_uint revision,audio_revision,view_revision,synthesis_count;
    atomic_bool running;
    atomic_int ready[PADS];
    pthread_t worker;
    struct gkick_synth *synth;
    Cache slots[SLOTS];
    Voice voices[VOICES];
    int current,pad_current[PADS];
    unsigned age;
    unsigned display_revision,ui_revision;
    float output_gain[PADS];
    float cut_tail[PADS][EDGE_SAMPLES];
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
    uint32_t hash=2166136261u;
    for(const unsigned char *s=(const unsigned char *)key;*s;s++)hash=(hash^*s)*16777619u;
    unsigned slot=hash&(G_ALIAS_LOOKUP_SIZE-1);
    for(;;){int i=galias_lookup[slot];if(i<0)return -1;if(!strcmp(key,galiases[i].key))return i;slot=(slot+1)&(G_ALIAS_LOOKUP_SIZE-1);}
}
static float at(Engine *e,const char *key) { int i=index_of(key); return i<0?0:atomic_load(&e->values[i]); }
static float pad_at(Engine *e,int pad,const char *key) { int i=index_of(key);return i<0?0:atomic_load(&e->pad_values[pad][i]); }
static void audio_changed(Engine *e) { atomic_fetch_add(&e->audio_revision,1);atomic_fetch_add(&e->pad_audio[atomic_load(&e->selected)],1); }
static void view_changed(Engine *e) { atomic_fetch_add(&e->view_revision,1);atomic_fetch_add(&e->pad_view[atomic_load(&e->selected)],1); }
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
static void stop_all(Engine *e) {
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
        e->cut_tail[v->pad][target]+=voice_sample(c,v->pos+n*v->step)*v->gain*
            edge_gain((EDGE_SAMPLES-1-n)/(float)(EDGE_SAMPLES-1));
        if(++target==EDGE_SAMPLES)target=0;
    }
    stop_voice(e,v);
}
static void adopt(Engine *e) {
    int selected=atomic_load(&e->selected);
    for(int pad=0;pad<PADS;pad++) {
        int slot=atomic_exchange(&e->ready[pad],-1);
        if(slot>=0) {
            int old=e->pad_current[pad];e->pad_current[pad]=slot;drop(e,old);
            if(pad==selected) { e->current=slot;++e->display_revision; }
        }
    }
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
static void kit_feedback(Engine *e);
#include "sample_library.h"
#include "factory_dsp.h"

/* Worker-owned DSP setup. All allocations and upstream mutexes remain here. */
static void configure(Engine *e,const float *p) {
    struct gkick_synth *s=e->synth;
    factory_reset_dsp(e);
    s->length=value(p,"length"); s->amplitude=1;
    for(int l=0;l<3;l++) { char key[24]; snprintf(key,sizeof key,"layer%d",l+1); s->osc_groups_amplitude[l]=value(p,key); s->osc_groups[l]=s->osc_groups_amplitude[l]>0; }
    static const int wave[]={0,1,2,3,4,6};
    for(int i=0;i<9;i++) {
        struct gkick_oscillator *o=s->oscillators[i]; char key[40];
#define OVAL(suffix) (snprintf(key,sizeof key,"o%d_%s",i+1,suffix),value(p,key))
        o->state=OVAL("on")>0?GEONKICK_OSC_STATE_ENABLED:GEONKICK_OSC_STATE_DISABLED;
        o->func=wave[(int)OVAL("wave")];o->sample=NULL;
        if(p[GP_COUNT+i]>0) {
            o->func=GEONKICK_OSC_FUNC_SAMPLE;SampleAsset *a=e->worker_samples[i];
            if(a){e->sample_views[i]=(struct gkick_buffer){.buff=a->data,.max_size=a->count,.size=a->count,.edge_fade=1,.cached_pitch=NAN,.cached_factor=1};o->sample=e->sample_views+i;}
        }
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

        if(p[GP_COUNT+i]>0) {
            float table[FREE_TABLE];o->pitch_shift=120;
            int points=OVAL("sweep")==0 && point_value(p,i,1,"mode")==0?2:FREE_TABLE;
            for(int n=0;n<points;n++) {
                float hz=o->frequency*gkick_envelope_get_value(o->envelopes[GKICK_OSC_FREQUENCY_ENVELOPE],n/(float)(points-1));
                float semitones=12*log2f(fmaxf(.001f,hz)/55.f);
                table[n]=.5f+clampf(semitones,-120,120)/240.f;
            }
            gkick_envelope_set_apply_type(o->envelopes[GKICK_OSC_PITCH_SHIFT_ENVELOPE],GEONKICK_ENVELOPE_APPLY_LINEAR);
            gkick_envelope_set_table(o->envelopes[GKICK_OSC_PITCH_SHIFT_ENVELOPE],table,points);
        }
        o->filter_enabled=OVAL("filter_on")>0;
        gkick_filter_set_type(o->filter,(int)OVAL("filter_type"));
        char filter_prefix[32];snprintf(filter_prefix,sizeof filter_prefix,"o%d_",i+1);
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
    factory_configure(e,p);
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
    float peaks[3][2][PLOT_COLUMNS]={0};
    for(int i=0;i<c->size;i++) {
        float layers[3];float x=gkick_synth_get_value_layers(s,i*dt,layers);
        x=isfinite(x)?clampf(x,-1,1):0;
        float fade=fminf(1,i/32.f)*edge_gain((c->size-1-i)/(float)(EDGE_SAMPLES-1));
        c->data[i]=x*fade;
        int col=((long long)(i+1)*PLOT_COLUMNS-1)/c->size;
        for(int l=0;l<3;l++){float y=isfinite(layers[l])?layers[l]*fade:0;peaks[l][0][col]=fmaxf(peaks[l][0][col],y);peaks[l][1][col]=fmaxf(peaks[l][1][col],-y);}
    }
    for(int l=0;l<3;l++)for(int k=0;k<2;k++)for(int c0=0;c0<PLOT_COLUMNS;c0++)
        c->layer_wave[l][k][c0]=(unsigned char)roundf(clampf(peaks[l][k][c0],0,1)*(PLOT_WAVE_LEVELS-1));
}
static void make_plot(Engine *e,Cache *c,const float *p) {
    struct gkick_synth *s=e->synth;
    int osc=(int)value(p,"plot_osc");
    struct gkick_oscillator *selected=s->oscillators[osc];
    struct gkick_envelope *env=selected->envelopes[value(p,"plot_env")>0?GKICK_OSC_FREQUENCY_ENVELOPE:GKICK_OSC_AMPLITUDE_ENVELOPE];
    struct gkick_oscillator *layer_osc[3];struct gkick_envelope *layer_env[3];int layer_remap[3];
    for(int l=0;l<3;l++) {
        int focus=(int)p[GP_COUNT+9+l];layer_osc[l]=s->oscillators[focus];
        layer_env[l]=layer_osc[l]->envelopes[value(p,"plot_env")>0?GKICK_OSC_FREQUENCY_ENVELOPE:GKICK_OSC_AMPLITUDE_ENVELOPE];
        layer_remap[l]=value(p,"plot_env")>0 && point_value(p,focus,1,"mode")>0;
    }
    for(int column=0;column<PLOT_COLUMNS;column++) {
        int begin=(int)((long long)column*c->size/PLOT_COLUMNS);
        int end=(int)((long long)(column+1)*c->size/PLOT_COLUMNS);
        float positive=0,negative=0;
        for(int i=begin;i<end;i++) { positive=fmaxf(positive,c->data[i]);negative=fmaxf(negative,-c->data[i]); }
        c->plot[0][column]=(unsigned char)roundf(clampf(positive,0,1)*(PLOT_WAVE_LEVELS-1));
        c->plot[1][column]=(unsigned char)roundf(clampf(negative,0,1)*(PLOT_WAVE_LEVELS-1));
        float shape=gkick_envelope_get_value(env,(column+0.5f)/PLOT_COLUMNS);
        if(value(p,"plot_env")>0 && point_value(p,osc,1,"mode")>0)shape=factory_pitch_plot(p,osc,selected,shape);

        c->plot[2][column]=(unsigned char)roundf(clampf(shape,0,1)*(PLOT_ENV_LEVELS-1));
        for(int l=0;l<3;l++) {
            float y=gkick_envelope_get_value(layer_env[l],(column+.5f)/PLOT_COLUMNS);
            if(layer_remap[l])y=factory_pitch_plot(p,(int)p[GP_COUNT+9+l],layer_osc[l],y);
            c->layer_plot[l][column]=(unsigned char)roundf(clampf(y,0,1)*(PLOT_ENV_LEVELS-1));
        }
    }
}
static unsigned snapshot(Engine *e,int pad,float *p,unsigned *view) {
    unsigned a,b;
    do {
        a=atomic_load(&e->revision);
        if(a&1) continue;
        sample_hold(e,pad);
        for(int i=0;i<GP_COUNT;i++) p[i]=atomic_load(&e->pad_values[pad][i]);
        for(int i=0;i<9;i++)p[GP_COUNT+i]=(float)atomic_load(&e->samples.source[pad][i]);
        for(int l=0;l<3;l++)p[GP_COUNT+9+l]=(float)atomic_load(&e->sound_focus[pad][l]);
        p[GP_COUNT+12]=(float)atomic_load(&e->factory_id[pad]);
        unsigned audio=atomic_load(&e->pad_audio[pad]);
        *view=atomic_load(&e->pad_view[pad]);
        b=atomic_load(&e->revision);
        if(a==b) return audio;
        sample_release(e);
    } while(true);
}
static uint64_t worker_clock_ns(void) { struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000u+t.tv_nsec; }
static void *worker(void *arg) {
    Engine *e=arg;unsigned done[PADS]={0},view_done[PADS]={0};float p[SNAP_COUNT];
    int held[PADS]={0},cursor=0;uint64_t next_synthesis=0;
    pthread_setname_np(pthread_self(),"gk-render");
    while(atomic_load(&e->running)) {
        sample_worker(e);
        /* One shared DSP and a global 30-start/s budget. Round-robin prevents
         * a continuously edited pad from starving the other fifteen sounds. */
        int selected=atomic_load(&e->selected),pad=-1;
        for(int k=0;k<PADS;k++) {
            int candidate=(cursor+k)%PADS;
            if(atomic_load(&e->pad_audio[candidate])!=done[candidate] ||
               (candidate==selected && atomic_load(&e->pad_view[candidate])!=view_done[candidate])) {pad=candidate;break;}
        }
        if(pad<0){usleep(2000);continue;}
        unsigned view,rev=snapshot(e,pad,p,&view);
        if(rev!=done[pad] && worker_clock_ns()<next_synthesis){sample_release(e);usleep(2000);continue;}
        int slot=-1;
        for(int i=0;i<SLOTS;i++){int free=0;if(atomic_compare_exchange_strong(&e->slots[i].refs,&free,-1)){slot=i;break;}}
        if(slot>=0) {
            Cache *c=&e->slots[slot];
            if(rev!=done[pad]) {next_synthesis=worker_clock_ns()+33000000u;synthesize(e,c,p,rev);}
            else {configure(e,p);c->size=e->slots[held[pad]].size;c->revision=rev;memcpy(c->data,e->slots[held[pad]].data,c->size*sizeof(float));memcpy(c->layer_wave,e->slots[held[pad]].layer_wave,sizeof c->layer_wave);}
            make_plot(e,c,p);c->view_revision=view;
            atomic_store(&c->refs,2); /* worker + ready/current, per pad */
            drop(e,atomic_exchange(&e->ready[pad],slot));drop(e,held[pad]);
            held[pad]=slot;done[pad]=rev;view_done[pad]=view;cursor=(pad+1)%PADS;
        }
        sample_release(e);usleep(2000);
    }
    sample_collect(e);
    for(int pad=0;pad<PADS;pad++)drop(e,held[pad]);
    return NULL;
}

static void preset(Engine *e,int which) {
    e->bank_program=-1;
    ++e->control_context;sample_reset_pad(e,atomic_load(&e->selected));
    atomic_store(&e->factory_id[atomic_load(&e->selected)],0);
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
#include "kit_controls.h"

static void *create(const char *dir) {
    Engine *e=calloc(1,sizeof *e); if(!e)return NULL;
    e->bank_program=-1;
    sample_init(e,dir);
    e->values=e->pad_values[0];atomic_init(&e->selected,0);e->kit_base=36;e->kit_output=e->kit_gain=1;
    e->current=-1;e->display_revision=1;atomic_init(&e->revision,0);atomic_init(&e->audio_revision,0);atomic_init(&e->view_revision,0);atomic_init(&e->synthesis_count,0);atomic_init(&e->running,false);
    for(int pad=0;pad<PADS;pad++) {
        atomic_init(&e->factory_id[pad],0);
        e->kit_notes[pad]=36+pad;e->kit_stage[pad]=-1;
        atomic_init(&e->ready[pad],-1);atomic_init(&e->pad_audio[pad],0);atomic_init(&e->pad_view[pad],0);e->pad_current[pad]=-1;
        for(int i=0;i<GP_COUNT;i++)atomic_init(&e->pad_values[pad][i],gparams[i].def);
        e->output_gain[pad]=gparams[index_of("volume")].def;
    }
    for(int i=0;i<VOICES;i++)e->voices[i].slot=-1;
    for(int i=0;i<SLOTS;i++){atomic_init(&e->slots[i].refs,0);e->slots[i].data=calloc(MAX_SAMPLES,sizeof(float));if(!e->slots[i].data)goto fail;}
    if(gkick_synth_new(&e->synth,RATE)!=GEONKICK_OK)goto fail;
    float p[SNAP_COUNT];unsigned view;snapshot(e,0,p,&view);synthesize(e,&e->slots[0],p,0);make_plot(e,&e->slots[0],p);sample_release(e);
    /* Initially every pad has the same Init: share immutable audio, not sixteen
     * startup renders. Edits replace only that pad's retained cache. */
    atomic_store(&e->slots[0].refs,PADS*2);e->current=0;
    for(int pad=0;pad<PADS;pad++)e->pad_current[pad]=0;
    atomic_store(&e->running,true);
    if(pthread_create(&e->worker,NULL,worker,e)!=0) { atomic_store(&e->running,false); goto fail; }
    return e;
fail:
    if(e->synth){for(int i=0;i<9;i++)if(e->synth->oscillators[i])e->synth->oscillators[i]->sample=NULL;gkick_synth_free(&e->synth);}
    sample_destroy(e);
    for(int i=0;i<SLOTS;i++)free(e->slots[i].data);
    free(e); return NULL;
}
static void destroy(void *inst) {
    Engine *e=inst; if(!e)return;
    atomic_store(&e->running,false); pthread_join(e->worker,NULL);
    for(int i=0;i<9;i++)e->synth->oscillators[i]->sample=NULL;
    sample_destroy(e);gkick_synth_free(&e->synth); for(int i=0;i<SLOTS;i++)free(e->slots[i].data); free(e);
}
static void hit_pad(Engine *e,int pad,int note,int ch,int vel) {
    adopt(e);int playback=(int)pad_at(e,pad,"playback");
    if(playback==1)for(int i=0;i<VOICES;i++)if(e->voices[i].pad==pad)choke_voice(e,&e->voices[i]);
    int v=0;
    for(int i=0;i<VOICES;i++){if(e->voices[i].slot<0){v=i;break;}if(e->voices[i].age<e->voices[v].age)v=i;}
    choke_voice(e,&e->voices[v]);Voice *voice=&e->voices[v];
    voice->slot=e->pad_current[pad];atomic_fetch_add(&e->slots[voice->slot].refs,1);
    voice->pad=pad;voice->gate=playback==2;voice->pos=0;voice->gain=vel/127.f;
    voice->note=note;voice->channel=ch;voice->age=++e->age;
    voice->step=exp2((pad_at(e,pad,"tune")+(!e->kit_mode && pad_at(e,pad,"keytrack")>0?note-60:0))/12.0);
}static void midi(void *inst,const uint8_t *msg,int len) {
    Engine *e=inst;if(!e||len<3)return;
    int type=msg[0]&0xf0,ch=msg[0]&15,note=msg[1]&127,vel=msg[2]&127;
    if(type==0xb0 && (note==120||note==123)){stop_all(e);sample_preview_stop(e);return;}
    if(type==0x80 || (type==0x90&&!vel)) {
        for(int i=0;i<VOICES;i++)if(e->voices[i].gate && e->voices[i].note==note && e->voices[i].channel==ch)stop_voice(e,&e->voices[i]);
        return;
    }
    if(type!=0x90)return;
    if(e->kit_learn) {int pad=kit_capture_note(e,note);if(pad>=0)hit_pad(e,pad,note,ch,vel);return;}
    int pad=e->kit_mode?kit_route(e,note):atomic_load(&e->selected);
    if(pad>=0 && pad<PADS)hit_pad(e,pad,note,ch,vel);
}

static int sound_proxy(Engine *e,const char *key,char *target,size_t size) {
    if(strncmp(key,"sound_o",7) || key[7]<'1' || key[7]>'3' || key[8]!='_' || !key[9])return 0;
    int slot=key[7]-'0';const char *suffix=key+9;
    int layer=(int)at(e,"plot_osc")/3;
    return snprintf(target,size,"o%d_%s",layer*3+slot,suffix)>0;
}
static void set_param(void *inst,const char *key,const char *text) {
    Engine *e=inst;
    if(key[0]=='f' && key[1]>='1' && key[1]<='9' && key[2]=='_')return; /* private curve storage */
    if(!strcmp(key,"__program_load")) {
        char *end;long index=strtol(text,&end,10);if(end==text||*end||index<0||index>=BANK_PROGRAM_COUNT)return;
        if(index<8){atomic_fetch_add(&e->revision,1);preset(e,(int)index);audio_changed(e);view_changed(e);atomic_fetch_add(&e->revision,1);}
        else kit_load(e,bank_programs[index].state);
        return;
    }
    char proxy[96];if(sound_proxy(e,key,proxy,sizeof proxy)){
        int alias=alias_of(proxy);float before=alias>=0?at(e,galiases[alias].target):0;
        set_param(e,proxy,text);
        /* The overview is an editor for canonical oscillator parameters. Notify
         * the physical fine parameter so hosts record a stable layer target. */
        if(alias>=0 && at(e,galiases[alias].target)!=before)e->dirty_alias[alias]=1;
        return;
    }
    if(!strcmp(key,"sound_layer")) {
        char *end;float value=strtof(text,&end);if(end==text||*end||!isfinite(value)||value<0||value>2)return;
        int layer=(int)lroundf(value),pad=atomic_load(&e->selected),osc=atomic_load(&e->sound_focus[pad][layer]);
        if((int)at(e,"plot_osc")!=osc){char target[16];snprintf(target,sizeof target,"%d",osc);++e->control_context;set_param(e,"plot_osc",target);}
        return;
    }
    if(!strncmp(key,"human_",6))return; /* retired 0.0.20 slots are inert */
    if(sample_set(e,key,text))return;
    if(kit_set(e,key,text))return;
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
        /* Coarse touchscreen pitch movement invalidates the shared overview
         * Q-Link's previous raw coordinate, as a layer/pad change already does. */
        if(a->kind==3)++e->control_context;
        set_param(e,galiases[alias].target,value);
        for(int j=0;j<G_ALIAS_COUNT;j++)if((galiases[j].kind>=3)!=(a->kind>=3) && !strcmp(galiases[j].target,a->target))e->dirty_alias[j]=1;
        return;
    }
    if(!strcmp(key,"state")){kit_load(e,text);return;}
    int i=index_of(key); if(i<0)return;
    char *end;float v=strtof(text,&end);if(end==text||*end||!isfinite(v))return;
    v=clampf(v,gparams[i].lo,gparams[i].hi);if(gparams[i].discrete)v=roundf(v);
    if(!strcmp(key,"audition")) { if(v>0) { int pad=atomic_load(&e->selected);hit_pad(e,pad,e->kit_mode?kit_assigned_note(e,pad):60,0,110); }return; }
    if(!strcmp(key,"reset")) {
        if(v>0) { atomic_fetch_add(&e->revision,1);preset(e,0);audio_changed(e);view_changed(e);atomic_fetch_add(&e->revision,1); }
        return;
    }
    PointKey point;int point_key_changed=point_decode(key,&point);if(point_key_changed && point.field=='s')return;
    if(v==atomic_load(&e->values[i]))return;
    atomic_fetch_add(&e->revision,1);
    if(!strcmp(key,"preset"))preset(e,(int)v);
    else {
        if(point_key_changed)point_prepare(e,&point,v);
        atomic_store(&e->values[i],v);
        if(!strcmp(key,"plot_osc"))atomic_store(&e->sound_focus[atomic_load(&e->selected)][(int)v/3],(int)v);
        e->dirty[i]=1;filter_legacy_changed(e,key,v);
        if(point_key_changed){point_changed(e,&point);e->edit_time_valid=0;
            if(point.field=='l'||point.field=='t')free_store(e,point.osc,point.kind,"n",0);
            e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1;
        }
        if(!strcmp(key,"plot_osc") || !strcmp(key,"plot_env") || !strcmp(key,"point_select")){e->point_dirty|=POINT_ALL;e->free_dirty=(1u<<FREE_NOTIFY_COUNT)-1;}
        if(key[0]=='o' && key[1]>='1' && key[1]<='9' && key[2]=='_' && key[3]) {
            int oscillator=key[1]-'0';const char *suffix=key+3;
            atomic_store(&e->sound_focus[atomic_load(&e->selected)][(oscillator-1)/3],oscillator-1);
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
    if(view_only)view_changed(e);
    else if(!playback_only)audio_changed(e);
    atomic_fetch_add(&e->revision,1);
}
static int get_param(void *inst,const char *key,char *buf,int len) {
    Engine *e=inst;if(!e||len<1)return 0;
    /* Stored canonical scalars have no virtual formatting or editor effects.
     * Read them directly instead of trying every sample/editor/graph adapter. */
    int scalar=index_of(key);
    if(scalar>=0 && strncmp(key,"free_",5) && strncmp(key,"human_",6))
        return snprintf(buf,len,"%.9g",atomic_load(&e->values[scalar]));
    if(!strcmp(key,"__program_index"))return snprintf(buf,len,"%d",e->bank_program>=8?e->bank_program:(int)at(e,"preset"));
    if(!strncmp(key,"__program_name:",15)){
        char *end;long index=strtol(key+15,&end,10);if(end==key+15||*end||index<0||index>=BANK_PROGRAM_COUNT)return 0;
        return snprintf(buf,len,"%s",bank_programs[index].name);
    }
    char proxy[96];if(sound_proxy(e,key,proxy,sizeof proxy))return get_param(e,proxy,buf,len);
    if(!strcmp(key,"sound_layer"))return snprintf(buf,len,"%d",(int)at(e,"plot_osc")/3);
    if(!strcmp(key,"control_context"))return snprintf(buf,len,"%u",e->control_context);
    if(!strncmp(key,"human_",6))return snprintf(buf,len,"0");
    int sample_result=sample_get(e,key,buf,len);if(sample_result>=0)return sample_result;
    int kit_result=kit_get(e,key,buf,len);if(kit_result>=0)return kit_result;
    /* Wrapper serializes this call with MIDI/render. The display thread can
     * adopt a completed sound even while the host isn't processing audio. */
    int filter_result=filter_virtual_get(e,key,buf,len);if(filter_result>0)return filter_result;
    int xy_result=free_xy_get(e,key,buf,len);if(xy_result>0)return xy_result;
    int free_result=free_virtual_get(e,key,buf,len);if(free_result>0)return free_result;
    int point_result=point_virtual_get(e,key,buf,len);if(point_result>0)return point_result;
    if(!strcmp(key,"display_rev")) { sample_poll(e);adopt(e);return snprintf(buf,len,"%u",e->display_revision+e->ui_revision); }
    if(!strcmp(key,"graph_revision"))return snprintf(buf,len,"%u",e->display_revision%1024);
    if(!strcmp(key,"ui_env_focus")) {
        int focus=(int)at(e,"plot_osc");char wave[32];snprintf(wave,sizeof wave,"o%d_wave",focus+1);
        return snprintf(buf,len,"%d",2*focus+(at(e,wave)>=4 && !atomic_load(&e->samples.source[atomic_load(&e->selected)][focus])));
    }
    size_t key_len=strlen(key);
    if(key_len>5 && !strcmp(key+key_len-5,"_unit")) {
        char original[64];if(key_len-5>=sizeof original)return 0;memcpy(original,key,key_len-5);original[key_len-5]=0;
        int alias=alias_of(original),osc;
        if(alias>=0 && galiases[alias].kind%3==0 && sscanf(galiases[alias].target,"o%d_freq",&osc)==1 && atomic_load(&e->samples.source[atomic_load(&e->selected)][osc-1]))return snprintf(buf,len,"st");
        return 0;
    }

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
                    if(atomic_load(&e->samples.source[atomic_load(&e->selected)][osc-1]))return snprintf(buf,len,"%+.2f st",12*log2f(physical/55.f));
                    if(at(e,wave)>=4)return snprintf(buf,len,"NO PITCH");
                }
            }
            int unit=galiases[alias].kind%3;float displayed=unit==1?physical*1000:unit==2?physical*100:physical;
            return snprintf(buf,len,unit?"%.0f":"%.2f",displayed);
        }
        int i=index_of(original);if(i<0)return 0;
        return snprintf(buf,len,"%.3f",atomic_load(&e->values[i]));
    }
    if(!strncmp(key,"ui_view_",8)){
        if(!strcmp(key,"ui_view_mix"))return snprintf(buf,len,"MIX WAVEFORM");
        int l;char tail;if(sscanf(key,"ui_view_l%d%c",&l,&tail)==1 && l>=1&&l<=3)return snprintf(buf,len,"LAYER %d WAVEFORM",l);
    }
    char view_channel,view_tail;int view_col;
    if(sscanf(key,"viz_sound_%c_%d%c",&view_channel,&view_col,&view_tail)==2 && (view_channel=='p'||view_channel=='n') && view_col>=0&&view_col<PLOT_COLUMNS){
        int l=(int)at(e,"plot_osc")/3;
        return snprintf(buf,len,"%.9g",e->slots[e->current].layer_wave[l][view_channel=='n'][view_col]/(float)(PLOT_WAVE_LEVELS-1));
    }
    int layer,column;char extra,field[32];
    if(sscanf(key,"sound_l%d_%31s%c",&layer,field,&extra)==2 && layer>=1 && layer<=3) {
        int o=atomic_load(&e->sound_focus[atomic_load(&e->selected)][layer-1]);
        if(!strcmp(field,"focus"))return snprintf(buf,len,"%d",o);
        if(!strcmp(field,"context")){char k[32];snprintf(k,sizeof k,"o%d_wave",o+1);return snprintf(buf,len,"%d",2*o+(at(e,k)>=4 && !atomic_load(&e->samples.source[atomic_load(&e->selected)][o])));}
    }
    if(sscanf(key,"viz_l%d_e_%d%c",&layer,&column,&extra)==2 && layer>=1&&layer<=3 && column>=0&&column<PLOT_COLUMNS)return snprintf(buf,len,"%.9g",e->slots[e->current].layer_plot[layer-1][column]/(float)(PLOT_ENV_LEVELS-1));
    char wave_channel;
    if(sscanf(key,"viz_l%d_%c_%d%c",&layer,&wave_channel,&column,&extra)==3 && layer>=1&&layer<=3 && (wave_channel=='p'||wave_channel=='n') && column>=0&&column<PLOT_COLUMNS)return snprintf(buf,len,"%.9g",e->slots[e->current].layer_wave[layer-1][wave_channel=='n'][column]/(float)(PLOT_WAVE_LEVELS-1));
    if(!strncmp(key,"viz_",4)) {
        char channel,trailing;int column;
        if(sscanf(key,"viz_%c_%d%c",&channel,&column,&trailing)!=2 || column<0 || column>=PLOT_COLUMNS)return 0;
        int c=channel=='p'?0:channel=='n'?1:channel=='e'?2:-1;if(c<0)return 0;
        return snprintf(buf,len,"%.9g",e->slots[e->current].plot[c][column]/(float)((c==2?PLOT_ENV_LEVELS:PLOT_WAVE_LEVELS)-1));
    }
    if(!strcmp(key,"synthesis_count"))return snprintf(buf,len,"%u",atomic_load(&e->synthesis_count));
    if(!strcmp(key,"render_pending"))return snprintf(buf,len,"%d",(e->slots[e->current].revision!=atomic_load(&e->pad_audio[atomic_load(&e->selected)]) || e->slots[e->current].view_revision!=atomic_load(&e->pad_view[atomic_load(&e->selected)])));
    if(!strcmp(key,"state"))return kit_save(e,buf,len);
    int alias=alias_of(key);
    if(alias>=0) {
        float physical=at(e,galiases[alias].target);
        float v=alias_coordinate(alias,physical);
        return snprintf(buf,len,"%.9g",v);
    }
    int i=index_of(key);if(i<0)return 0;
    return snprintf(buf,len,"%.9g",atomic_load(&e->values[i]));
}
/* Optional same-module float read: canonical storage and fine adapters only.
 * Virtual editors and textual/display controls retain their existing handlers. */
int mpc_engine_get_float(void *inst,const char *key,float *out) {
    Engine *e=inst;int scalar=index_of(key);
    if(scalar>=0 && strncmp(key,"free_",5) && strncmp(key,"human_",6)){*out=atomic_load(&e->values[scalar]);return 1;}
    int alias=alias_of(key);if(alias>=0){*out=alias_coordinate(alias,at(e,galiases[alias].target));return 1;}
    char prefix[32];const char *suffix;
    if(filter_decode(key,prefix,&suffix)){
        int resonance=!strncmp(suffix,"res_",4),fine=strstr(suffix,"_fine")!=NULL;
        float coordinate=filter_coordinate(e,prefix,resonance);
        *out=fine?(resonance?roundf(coordinate*1000):roundf(coordinate*log2f(1000)*240)):coordinate;return 1;
    }
    return 0;
}
unsigned mpc_engine_input_context(void *inst) { return ((Engine *)inst)->control_context; }
int mpc_engine_ui_only_input(const char *key) {
    return !strcmp(key,"sound_layer") || !strncmp(key,"sound_o",7) || !strcmp(key,"sample_preview") || !strcmp(key,"sample_preview_stop");
}
int mpc_engine_changed(void *inst,char *key,int len) {
    Engine *e=inst;
    for(unsigned i=0;i<SAMPLE_NOTIFY_COUNT;i++)if(e->samples.dirty&(UINT64_C(1)<<i)){e->samples.dirty&=~(UINT64_C(1)<<i);snprintf(key,len,"%s",sample_notifications[i]);return 1;}
    for(unsigned i=0;i<KIT_NOTIFY_COUNT;i++)if(e->kit_dirty&(1u<<i)){e->kit_dirty&=~(1u<<i);snprintf(key,len,"%s",kit_notifications[i]);return 1;}
    for(int i=0;i<GP_COUNT;i++)if(e->dirty[i]) { e->dirty[i]=0;snprintf(key,len,"%s",gparams[i].key);return 1; }
    for(int i=0;i<G_ALIAS_COUNT;i++)if(e->dirty_alias[i]) { e->dirty_alias[i]=0;snprintf(key,len,"%s",galiases[i].key);return 1; }
    for(int i=0;i<FREE_NOTIFY_COUNT;i++)if(e->free_dirty&(1u<<i)){e->free_dirty&=~(1u<<i);snprintf(key,len,"%s",free_notifications[i]);return 1;}
    for(int i=0;i<6;i++)if(e->point_dirty&(1u<<i)){e->point_dirty&=~(1u<<i);snprintf(key,len,"%s",point_notifications[i]);return 1;}
    return 0;
}
static void render(void *inst,mpc_sample_t *out,int frames) {
    Engine *e=inst;adopt(e);sample_preview_begin_block(e);float volume[PADS];
    for(int pad=0;pad<PADS;pad++)volume[pad]=factory_output(e,pad);
    for(int n=0;n<frames;n++) {
        float pad_sum[PADS];
        for(int pad=0;pad<PADS;pad++){pad_sum[pad]=e->cut_tail[pad][e->cut_pos];e->cut_tail[pad][e->cut_pos]=0;}
        if(++e->cut_pos==EDGE_SAMPLES)e->cut_pos=0;
        for(int i=0;i<VOICES;i++) {
            Voice *v=&e->voices[i];if(v->slot<0)continue;
            Cache *c=&e->slots[v->slot];if((int)v->pos>=c->size){stop_voice(e,v);continue;}
            pad_sum[v->pad]+=voice_sample(c,v->pos)*v->gain;v->pos+=v->step;
        }
        float sum=0;
        for(int pad=0;pad<PADS;pad++) {
            e->output_gain[pad]+=(volume[pad]-e->output_gain[pad])*(1.f/(0.005f*RATE));
            sum+=pad_sum[pad]*e->output_gain[pad];
        }
        e->kit_gain+=(e->kit_output-e->kit_gain)*(1.f/(0.005f*RATE));
        out[n*2]=out[n*2+1]=(sum+sample_preview_frame(e))*e->kit_gain;
    }
}
static const mpc_engine_t api={create,destroy,midi,set_param,get_param,render,NULL,NULL};
const mpc_engine_t *mpc_engine(void) { return &api; }
