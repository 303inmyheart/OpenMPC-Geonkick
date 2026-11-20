/* Actual VST: multi-megabyte embedded samples, source controls and close/reopen. */
#include "params.h"
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
typedef struct Effect Effect;
typedef intptr_t(*Callback)(Effect*,int32_t,int32_t,intptr_t,void*,float);
struct Effect {int32_t magic;intptr_t(*dispatch)(Effect*,int32_t,int32_t,intptr_t,void*,float);void*p;void(*set)(Effect*,int32_t,float);float(*get)(Effect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void*obj,*user;int32_t uid,ver;void(*render)(Effect*,float**,float**,int32_t);};
extern Effect *VSTPluginMain(Callback);
extern const char *test_plugin_path;
static intptr_t host(Effect*e,int32_t op,int32_t i,intptr_t v,void*p,float o){(void)e;(void)i;(void)v;(void)p;(void)o;return op==1?2400:0;}
static int find(const char*k){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,k))return i;assert(0);return -1;}
static void physical(Effect *e,const char *key,float v){int i=find(key);e->set(e,i,PARAMS[i].nopts?v/(PARAMS[i].nopts-1):(v-PARAMS[i].min)/(PARAMS[i].max-PARAMS[i].min));}
int main(void) {
 Effect *e=VSTPluginMain(host);assert(e && e->npar==4096);
 physical(e,"o1_decay",.05f);physical(e,"o4_attack",.3f);physical(e,"o7_decay",3);physical(e,"length",.6f);
 int ready=0;for(int i=0;i<3000;i++){float l[128],r[128],*out[]={l,r};e->render(e,NULL,out,128);if(e->get(e,find("viz_e_431"))>.7f){ready=1;break;}usleep(2000);}assert(ready);
 char text[128];
 for(int repeat=0;repeat<10;repeat++) {
  e->dispatch(e,7,find("ui_view_l1"),0,text,0);assert(!strcmp(text,"LAYER 1 WAVEFORM"));assert(e->get(e,find("viz_e_431"))==0);assert(e->get(e,find("viz_e_0"))>.9f);
  e->dispatch(e,7,find("ui_view_l2"),0,text,0);assert(e->get(e,find("viz_e_0"))<.02f);
  /* Engine/display reporter enumeration is NOT a view change. */
  for(int i=0;i<NPARAMS;i++){e->get(e,i);e->dispatch(e,8,i,0,text,0);}
  assert(e->get(e,find("viz_e_0"))<.02f);
  e->dispatch(e,7,find("ui_view_l3"),0,text,0);assert(e->get(e,find("viz_e_431"))>.7f);
  e->dispatch(e,7,find("ui_view_mix"),0,text,0);assert(!strcmp(text,"MIX WAVEFORM"));assert(e->get(e,find("viz_e_431"))>.7f);
 }
 Effect *b=VSTPluginMain(host);assert(b);e->dispatch(e,7,find("ui_view_l2"),0,text,0);
 b->dispatch(b,7,find("ui_view_l1"),0,text,0);assert(b->get(b,find("viz_e_0"))>.9f);assert(e->get(e,find("viz_e_0"))<.02f);
 b->dispatch(b,7,find("ui_view_mix"),0,text,0);assert(e->get(e,find("viz_e_0"))<.02f);b->dispatch(b,1,0,0,NULL,0);
 assert(find("sample_set_default")==4095);
 e->dispatch(e,1,0,0,NULL,0);puts("Actual VST:4096 native handle ceiling, full-resolution view formatting selects cached layer, ordinary scans/names leave view intact, mixed/editor view restored PASS");return 0;
}
