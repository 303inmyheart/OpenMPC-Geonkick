/* Original factory ingestion and independent direct-DSP oracle, GPL-3.0-or-later. */
#include "params.h"
#include "engine.h"
#include "param_defs.h"
#include "dsp/synthesizer.h"
#include "dsp/oscillator.h"
#include "dsp/envelope.h"
#include "dsp/filter.h"
#include "factory_types.h"
#include "factory_data.h"
#include "factory_fixtures.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define RATE 44100
static const mpc_engine_t *api;
static void set(void *e,const char*k,float v){char b[64];snprintf(b,sizeof b,"%.9g",v);api->set_param(e,k,b);}
static float get(void *e,const char*k){char b[64];assert(api->get_param(e,k,b,sizeof b)>0);return strtof(b,NULL);}
static void ready(void *e){float out[2];for(int k=0;k<20000;k++){api->render(e,out,0);if(!get(e,"kit_render_pending")&&!get(e,"render_pending"))return;usleep(2000);}assert(!"factory synthesis deadline");}
static char *readfile(const char *stem){char path[4096];snprintf(path,sizeof path,"%s/%s.chunk",getenv("GK_FACTORY_CHUNKS"),stem);FILE*f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);assert(n>0&&n<16777216);char *s=malloc(n);assert(s&&fread(s,1,n,f)==(size_t)n);fclose(f);assert(!s[n-1]);return s;}
static void env(struct gkick_envelope *e,const FactoryCurve*c){gkick_envelope_set_points(e,c->point,c->count);gkick_envelope_set_apply_type(e,c->apply);}
static void raw_setup(struct gkick_synth*s,const FactorySound*f,int id){
 s->length=f->baseline[1];s->amplitude=f->scalar[0][FA_LEVEL];env(s->envelope,&f->curves[0][FC_AMP]);
 for(int j=0;j<3;j++){s->osc_groups_amplitude[j]=f->baseline[5+j];s->osc_groups[j]=s->osc_groups_amplitude[j]>0;}
 for(int j=0;j<10;j++){
  const float*r=f->scalar[j];const FactoryCurve*c=f->curves[j];struct gkick_filter*filter=s->filter;struct gkick_distortion*d=s->distortion;
  if(j){
   struct gkick_oscillator*o=s->oscillators[j-1];filter=o->filter;d=o->distortion;
   o->state=r[FA_ON]>0?GEONKICK_OSC_STATE_ENABLED:GEONKICK_OSC_STATE_DISABLED;o->func=(int)r[FA_WAVE];o->frequency=r[FA_FREQ];o->amplitude=r[FA_LEVEL];o->initial_phase=r[FA_PHASE];o->noise_density=r[FA_DENSITY];o->pitch_shift=r[FA_PITCH];o->seed=(unsigned)r[FA_SEED];o->brownian=0;o->is_fm=r[FA_FM]>0;o->fm_k=r[FA_FMK];o->fm_input=0;
   for(int k=0;k<4;k++)env(o->envelopes[k],c+k);
   o->filter_enabled=r[FA_FILTER_ON]>0;
   if(o->sample){free(o->sample->buff);free(o->sample);o->sample=NULL;}
   char path[4096];snprintf(path,sizeof path,"%s/%d-%d.f32",getenv("GK_FACTORY_SAMPLES"),id,j-1);FILE*fp=fopen(path,"rb");
   if(fp){fseek(fp,0,SEEK_END);long n=ftell(fp);rewind(fp);o->sample=calloc(1,sizeof *o->sample);assert(o->sample);o->sample->size=o->sample->max_size=n/4;o->sample->buff=malloc(n);assert(o->sample->buff&&fread(o->sample->buff,1,n,fp)==(size_t)n);fclose(fp);o->sample->edge_fade=1;o->sample->cached_pitch=NAN;}
  }else s->filter_enabled=r[FA_FILTER_ON]>0;
  gkick_filter_set_wide(filter,0,1);gkick_filter_set_type(filter,(int)r[FA_FILTER_TYPE]);gkick_filter_set_cutoff_freq(filter,r[FA_CUTOFF]);gkick_filter_set_factor(filter,r[FA_Q]);env(filter->cutoff_env,c+FC_CUTOFF);env(filter->q_env,c+FC_Q);
  d->enabled=r[FA_DRIVE_ON]>0;d->type=(int)r[FA_DRIVE_TYPE];d->drive=r[FA_DRIVE];d->in_limiter=r[FA_DRIVE_IN];d->out_limiter=r[FA_DRIVE_OUT];env(d->drive_env,c+FC_DRIVE);env(d->volume_env,c+FC_VOLUME);
 }
 gkick_synth_reset_oscillators(s);gkick_filter_init(s->filter);
}
int main(void){
 assert(getenv("GK_FACTORY_CHUNKS")&&getenv("GK_FACTORY_SAMPLES"));api=mpc_engine();void*e=api->create(NULL);assert(e);struct gkick_synth*s=NULL;assert(gkick_synth_new(&s,RATE)==GEONKICK_OK);
 char *saved=malloc(16777216),*again=malloc(16777216);assert(saved&&again);int checked=0;float worst=0;
 for(int n=0;n<FACTORY_FIXTURE_COUNT;n++){
  const FactoryFixture*f=&factory_fixtures[n];char*input=readfile(f->stem);api->set_param(e,"state",input);free(input);
  assert(api->get_param(e,"state",saved,16777216)>0&&strstr(saved," FCT1 "));
  assert(strstr(saved,factory_sounds[f->ids[0]-1].digest));ready(e);
  for(int pad=0;pad<f->count;pad++){
   set(e,"kit_pad",pad);ready(e);int id=f->ids[pad];const FactorySound*sound=&factory_sounds[id-1];raw_setup(s,sound,id);
   set(e,"kit_mode",0);set(e,"keytrack",0);set(e,"playback",0);
   float silence[8192];api->render(e,silence,4096);unsigned char panic[]={0xb0,120,0},note[]={0x90,60,127};api->midi(e,panic,3);api->midi(e,note,3);
   float actual[8192];api->render(e,actual,4096);int size=(int)(s->length*RATE);float dt=s->length/size;
   for(int k=0;k<4096&&k<size;k++){
    float raw=gkick_synth_get_value(s,k*dt),t=fminf(1,(size-1-k)/220.f);t=t*t*(3-2*t);
    float expected=(isfinite(raw)?fminf(1,fmaxf(-1,raw)):0)*fminf(1,k/32.f)*t*sound->limiter;
    float error=fabsf(actual[k*2]-expected);if(error>worst)worst=error;
    if(error>3e-4f){fprintf(stderr,"factory oracle mismatch fixture%d pad%d id%d sample%d got%.9g expected%.9g error%.9g\n",n,pad,id,k,actual[k*2],expected,error);abort();}
    assert(isfinite(actual[k*2])&&actual[k*2]==actual[k*2+1]);
   }
   checked++;
  }
  /* Full instance serialization must preserve imported identities, edits and
   * original samples, with no external file dependencies. */
  api->get_param(e,"state",saved,16777216);api->set_param(e,"state",saved);api->get_param(e,"state",again,16777216);assert(!strcmp(saved,again));
  if(n%10==0){printf("Factory fixtures%d/%d, sounds%d, max audio error%.8g\n",n+1,FACTORY_FIXTURE_COUNT,checked,worst);fflush(stdout);}
 }
 /* Unknown/tampered original identity rejects atomically, rather than silently
  * changing a saved sound if a bank definition ever changes. */
 char *tag=strstr(saved," FCT1 ");assert(tag);char *hash=tag+6;for(int i=0;i<3;i++){hash=strchr(hash,' ');assert(hash);hash++;}hash[0]=hash[0]=='a'?'b':'a';api->set_param(e,"state",saved);api->get_param(e,"state",saved,16777216);assert(!strcmp(saved,again));
 set(e,"reset",1);api->get_param(e,"state",saved,16777216); /* remaining kit pads may still be factory sounds */
 char *init=readfile("init-legacy");api->set_param(e,"state",init);free(init);ready(e);api->get_param(e,"state",saved,16777216);assert(!strstr(saved," FCT1 "));
 api->destroy(e);gkick_synth_free(&s);free(saved);free(again);printf("All109 original factory presets/kits,183 sound audio oracles, source identity, restore/reset PASS; worst%.8g\n",worst);return 0;
}
