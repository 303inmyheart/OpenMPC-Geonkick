/* Actual VST: graph feedback is read-only, silent during a gesture, refreshed after it. */
#include "params.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
typedef struct Effect Effect;
typedef intptr_t (*Callback)(Effect*,int32_t,int32_t,intptr_t,void*,float);
struct Effect { int32_t magic;intptr_t(*dispatch)(Effect*,int32_t,int32_t,intptr_t,void*,float);void *p;
 void(*set)(Effect*,int32_t,float);float(*get)(Effect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void *obj,*user;int32_t uid,ver;
 void(*render)(Effect*,float**,float**,int32_t); };
extern Effect *VSTPluginMain(Callback);
static atomic_int graph_reports,control_reports;
static int fine;
static atomic_int column_reports,rebinds;
static intptr_t host(Effect *e,int32_t op,int32_t i,intptr_t v,void *p,float o) {
 (void)v;(void)p;
 if(op==42) { atomic_fetch_add(&rebinds,1);for(int j=0;j<NPARAMS;j++)if(!strncmp(PARAMS[j].key,"viz_",4)) { float value=e->get(e,j);assert(value>=0 && value<=1); } }
 if(op==0 && i>=0 && i<NPARAMS) {
  if(!strncmp(PARAMS[i].key,"viz_",4)) { atomic_fetch_add(&column_reports,1);e->set(e,i,o); }
  else if(!strcmp(PARAMS[i].key,"graph_revision")) { assert(e->dispatch(e,26,i,0,NULL,0)==0);atomic_fetch_add(&graph_reports,1);
   /* Model a visible graph's re-read after its shared invalidation handle. */
   for(int j=0;j<NPARAMS;j++)if(!strncmp(PARAMS[j].key,"viz_",4)) { float value=e->get(e,j);assert(value>=0 && value<=1); }
  }
  else { atomic_fetch_add(&control_reports,1);if(i==fine)e->set(e,i,o); }
 }
 return op==1?2400:0;
}
static int find(const char *key) { for(int i=0;i<NPARAMS;i++)if(!strcmp(key,PARAMS[i].key))return i;assert(0);return 0; }
static void block(Effect *e) { float l[128],r[128],*out[]={l,r};e->render(e,NULL,out,128); }
int main(void) {
 fine=find("o1_freq_fine");Effect *e=VSTPluginMain(host);assert(e);int derived=0;
 for(int i=0;i<NPARAMS;i++)if(!strncmp(PARAMS[i].key,"viz_",4)||!strcmp(PARAMS[i].key,"ui_env_focus")||!strcmp(PARAMS[i].key,"graph_revision")) {
  assert(e->dispatch(e,26,i,0,NULL,0)==0);derived++;
 }
 assert(e->dispatch(e,26,find("sample_preview"),0,NULL,0)==0);
 assert(e->dispatch(e,26,find("sample_preview_stop"),0,NULL,0)==0);
 assert(derived==2263);assert(e->dispatch(e,26,fine,0,NULL,0)==1);
 assert(e->dispatch(e,26,find("o1_freq"),0,NULL,0)==1);
 usleep(250000);atomic_store(&graph_reports,0);
 for(int k=0;k<90;k++) {
  e->set(e,fine,e->get(e,fine)+0.01f);block(e);usleep(3000);
  assert(atomic_load(&graph_reports)==0);
 }
 assert(atomic_load(&control_reports)>0);float final=e->get(e,fine);
 for(int k=0;k<250 && !atomic_load(&graph_reports);k++) { block(e);usleep(10000); }
 assert(atomic_load(&graph_reports)>0);for(int k=0;k<50 && !atomic_load(&rebinds);k++)usleep(10000);assert(atomic_load(&rebinds)>0 && atomic_load(&rebinds)<=2);assert(atomic_load(&column_reports)==0);assert(fabsf(e->get(e,fine)-final)<1e-6f);
 printf("2263 display values non-automatable; zero column callbacks; shared refresh and one host rebind after gesture, direct value reads and frequency echoes PASS\n");
 e->dispatch(e,1,0,0,NULL,0);return 0;
}
