/* Real VST: deliberately blocked UI callbacks must never stall rendering. */
#include "params.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
typedef struct Effect Effect;
typedef intptr_t (*Callback)(Effect*,int32_t,int32_t,intptr_t,void*,float);
struct Effect { int32_t magic;intptr_t(*dispatch)(Effect*,int32_t,int32_t,intptr_t,void*,float);void *p;
 void(*set)(Effect*,int32_t,float);float(*get)(Effect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void *obj,*user;int32_t uid,ver;
 void(*render)(Effect*,float**,float**,int32_t); };
extern Effect *VSTPluginMain(Callback);
static _Thread_local int audio;
static atomic_int callbacks_on_audio,callbacks_total;
static intptr_t host(Effect *e,int32_t op,int32_t i,intptr_t v,void *p,float o) {
 (void)e;(void)i;(void)v;(void)p;(void)o;
 if(op==0 || op==42) { atomic_fetch_add(&callbacks_total,1);if(audio)atomic_fetch_add(&callbacks_on_audio,1);usleep(20000); }
 return op==1?2400:0;
}
static int find(const char *key) { for(int i=0;i<NPARAMS;i++)if(!strcmp(key,PARAMS[i].key))return i;assert(0);return 0; }
static double now(void) { struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9; }
int main(void) {
 Effect *e=VSTPluginMain(host);assert(e);double max=0;
 for(int k=0;k<16;k++) {
  e->set(e,find("o1_wave"),(k%2?0.21f:0.01f));
  e->set(e,find("o1_freq_fine"),e->get(e,find("o1_freq_fine"))+0.01f);
  float l[128],r[128],*out[]={l,r};audio=1;double begin=now();e->render(e,NULL,out,128);double dt=now()-begin;audio=0;
  max=fmax(max,dt);usleep(3000);
 }
 usleep(150000);
 printf("blocked-UI host: audio callbacks=%d total callbacks=%d worst block=%.3f ms (128 frames=2.902 ms)\n",atomic_load(&callbacks_on_audio),atomic_load(&callbacks_total),max*1000);
 e->dispatch(e,1,0,0,NULL,0);
 if(!getenv("REPORT_BASELINE")) { assert(atomic_load(&callbacks_total)>0);assert(atomic_load(&callbacks_on_audio)==0); }
 return 0;
}
