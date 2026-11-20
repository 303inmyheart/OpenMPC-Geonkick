/* Independent kit routing, state and cache-lifetime regressions. GPL-3.0-or-later. */
#include "params.h"
#include "engine.h"
#include "../src/param_defs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static const mpc_engine_t *api;
static void set(void *e,const char *k,float v){char b[48];snprintf(b,sizeof b,"%.9g",v);api->set_param(e,k,b);}
static float get(void *e,const char *k){char b[128];assert(api->get_param(e,k,b,sizeof b)>0);return strtof(b,NULL);}
static void msg(void *e,int status,int note,int vel){uint8_t m[]={status,note,vel};api->midi(e,m,3);}
static void render(void *e,float *out,int frames){for(int n=0;n<frames;n+=128)api->render(e,out+n*2,frames-n<128?frames-n:128);}
static void ready(void *e){float out[2];for(int i=0;i<8000;i++){api->render(e,out,0);if(get(e,"kit_render_pending")==0 && get(e,"render_pending")==0)return;usleep(2000);}assert(!"kit worker timeout");}
static float energy(float *a,int n){float s=0;for(int i=0;i<n;i++){assert(isfinite(a[i]));s+=a[i]*a[i];}return s;}
static void snapshot(void *e,char *text){assert(api->get_param(e,"state",text,1048576)>0);}
int main(void) {
 api=mpc_engine();void *a=api->create(NULL),*b=api->create(NULL);assert(a&&b);
 float x[8192],y[8192],z[8192];char *state=malloc(1048576),*again=malloc(1048576),*legacy=malloc(1048576);assert(state&&again&&legacy);
 assert(get(a,"kit_mode")==0 && get(a,"kit_pad")==0 && get(a,"kit_base")==36);
 /* Single keeps the established playable instrument; changing the edited
  * pad changes a separate sound without resynthesizing or disturbing a voice. */
 msg(a,0x90,60,127);render(a,x,128);assert(energy(x,256)>.01f);
 unsigned jobs=get(a,"synthesis_count");set(a,"kit_pad",1);ready(a);assert(get(a,"synthesis_count")==jobs);
 set(a,"preset",3);set(a,"o9_level",.1234567f);set(a,"o9_on",1);ready(a);
 set(a,"kit_pad",0);assert(get(a,"preset")==0 && get(a,"o9_on")==0 && fabsf(get(a,"o9_level")-.8f)<1e-6f);
 set(a,"kit_pad",1);assert(get(a,"preset")==3 && get(a,"o9_on")==1 && fabsf(get(a,"o9_level")-.1234567f)<1e-7f);
 /* Independent per-pad programs, level, tune, filter and editable curves. */
 for(int p=0;p<16;p++) {
  set(a,"kit_pad",p);set(a,"reset",1);set(a,"preset",1+p%7);
  set(a,"o1_freq",60+p*31);set(a,"volume",.4f+p*.01f);set(a,"tune",p-8);
  set(a,"filter_on",p%2);set(a,"q_wide",.6f+p*.1f);
  set(a,"plot_osc",p%9);set(a,"plot_env",p%2);set(a,"free_add",1);set(a,"free_level",.1f+p*.04f);
 }
 ready(a);
 for(int p=0;p<16;p++) {
  set(a,"kit_pad",p);assert(get(a,"o1_freq")==60+p*31);assert(get(a,"tune")==p-8);
  assert(fabsf(get(a,"volume")-(.4f+p*.01f))<1e-6f);assert(get(a,"preset")==1+p%7);
  assert(get(a,"plot_osc")==p%9 && get(a,"plot_env")==p%2);assert(get(a,"ui_point_count")>=3);assert(fabsf(get(a,"free_level")-(.1f+p*.04f))<1e-6f);
 }
 /* Complete kit round trip is larger than the old 64 KiB state limit. */
 set(a,"kit_mode",1);set(a,"kit_base",48);set(a,"kit_output",.63f);set(a,"kit_pad",9);snapshot(a,state);
 assert(!strncmp(state,"GKMP8 ",6));assert(strlen(state)>65536);api->set_param(b,"state",state);snapshot(b,again);assert(!strcmp(state,again));
 assert(get(b,"kit_pad")==9 && get(b,"kit_mode")==1 && get(b,"kit_base")==48);
 /* Rejection is atomic even when the final pad/metadata is corrupt. */
 strcpy(legacy,state);strcat(legacy," garbage");api->set_param(b,"state",legacy);snapshot(b,again);assert(!strcmp(state,again));
 strcpy(legacy,state);char *meta=strstr(legacy," KIT1 ");assert(meta);meta[6]='9';api->set_param(b,"state",legacy);snapshot(b,again);assert(!strcmp(state,again));
 /* Each note in range triggers its retained sound; outside notes stay silent.
  * Compare against an independent single-mode instance with the same sound. */
 ready(a);ready(b);
 for(int p=0;p<16;p++) {
  set(b,"kit_mode",0);set(b,"kit_pad",p);ready(b);
  msg(a,0xb0,120,0);msg(b,0xb0,120,0);msg(a,0x90,48+p,100);msg(b,0x90,60,100);
  render(a,x,1024);render(b,y,1024);
  for(int i=0;i<2048;i++)assert(fabsf(x[i]-y[i])<1e-6f);
 }
 msg(a,0xb0,120,0);msg(a,0x90,47,127);msg(a,0x90,64,127);render(a,x,128);assert(energy(x,256)==0);
 /* Per-pad CUT must not choke another pad. Two independently rendered voices
  * are an oracle for the mixed signal after a Cut retrigger on just one pad. */
 for(int p=0;p<2;p++){set(a,"kit_pad",p);set(a,"reset",1);set(a,"o1_sweep",0);set(a,"o1_freq",55+110*p);set(a,"playback",p?0:1);}
 set(a,"kit_output",1);set(a,"kit_mode",1);set(a,"kit_base",36);ready(a);
 api->set_param(b,"state",state);set(b,"kit_mode",0);set(b,"kit_pad",0);set(b,"reset",1);set(b,"o1_sweep",0);set(b,"kit_output",1);set(b,"playback",1);ready(b);
 void *c=api->create(NULL);assert(c);set(c,"o1_sweep",0);set(c,"o1_freq",165);ready(c);
 msg(a,0xb0,120,0);msg(b,0xb0,120,0);render(a,x,4096);render(b,y,4096);render(c,z,4096);msg(a,0x90,36,100);msg(a,0x90,37,100);msg(b,0x90,60,100);msg(c,0x90,60,100);
 render(a,x,777);render(b,y,777);render(c,z,777);
 for(int i=0;i<1554;i++)assert(fabsf(x[i]-(y[i]+z[i]))<3e-5f);
 msg(a,0x90,36,100);msg(b,0x90,60,100);render(a,x,512);render(b,y,512);render(c,z,512);
 for(int i=0;i<1024;i++)assert(fabsf(x[i]-(y[i]+z[i]))<3e-5f);
 /* Gate release follows its voice even if a different pad is being edited. */
 msg(a,0xb0,120,0);set(a,"kit_pad",0);set(a,"playback",2);msg(a,0x92,36,100);set(a,"kit_pad",15);msg(a,0x82,36,0);render(a,x,128);assert(energy(x,256)==0);
 /* Retired Humanizer controls cannot alter timing, velocity or state. */
 snapshot(a,state);set(a,"human_on",1);set(a,"human_timing",30);set(a,"human_velocity",30);snapshot(a,again);assert(!strcmp(state,again));assert(get(a,"human_on")==0);
 /* Old GKMP7 is restored as a single sound; three Humanizer fields consumed
  * and discarded. Other fifteen pads reset to Init, never stale project data. */
 int n=snprintf(legacy,1048576,"GKMP7");for(int i=0;i<GP_COUNT;i++)n+=snprintf(legacy+n,1048576-n," %.9g",gparams[i].def);
 strcat(legacy," 1 27.125 19.625");api->set_param(a,"state",legacy);assert(get(a,"kit_mode")==0 && get(a,"kit_pad")==0 && get(a,"human_on")==0);
 set(a,"kit_pad",15);assert(get(a,"preset")==0 && get(a,"o1_freq")==55);
 ready(a);msg(a,0xb0,120,0);msg(a,0x90,60,100);render(a,x,128);assert(energy(x,256)>.01f);
 /* Stress cache ownership with all sixteen independent edits and retained
  * voices. Selection and non-audio edits must not synthesize extra sounds. */
 set(a,"kit_mode",1);for(int k=0;k<128;k++){set(a,"kit_pad",k%16);set(a,"o1_freq",50+k*3);msg(a,0x90,36+k%16,100);render(a,x,128);energy(x,256);usleep(1000);}
 ready(a);msg(a,0xb0,123,0);render(a,x,128);assert(energy(x,256)==0);
 jobs=get(a,"synthesis_count");for(int p=0;p<16;p++){set(a,"kit_pad",p);set(a,"volume",.5f);set(a,"tune",0);}ready(a);assert(get(a,"synthesis_count")==jobs);
 api->destroy(a);api->destroy(b);api->destroy(c);free(state);free(again);free(legacy);
 puts("16 isolated pad sounds; MIDI bounds, mixed-signal oracle, per-pad Cut/Gate, >64KiB full-kit state, atomic rejection, GKMP7 migration and cache lifetime PASS");return 0;
}
