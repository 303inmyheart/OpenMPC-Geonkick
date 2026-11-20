/* Actual VST multi-pad state, context rebinding and retired controls. GPL-3.0-or-later. */
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
static intptr_t host(Effect*e,int32_t op,int32_t i,intptr_t v,void*p,float o){(void)e;(void)i;(void)v;(void)p;(void)o;return op==1?2400:0;}
static int find(const char*k){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,k))return i;assert(0);return -1;}
static void physical(Effect *e,const char *key,float v){int i=find(key);e->set(e,i,PARAMS[i].nopts?v/(PARAMS[i].nopts-1):(v-PARAMS[i].min)/(PARAMS[i].max-PARAMS[i].min));}
int main(void) {
 Effect *e=VSTPluginMain(host);assert(e);int pad=find("kit_pad"),fine=find("o1_freq_fine"),freq=find("o1_freq");
 char display[128];
 for(int p=0;p<16;p++) {
  physical(e,"kit_pad",p);physical(e,"o1_freq",100+p*35);physical(e,"o9_on",1);physical(e,"layer3",.3f+p*.01f);
  physical(e,"o1_a_mode",1);physical(e,"free_add",1);physical(e,"free_level",.1f+p*.03f);
 }
 for(int p=15;p>=0;p--) {
  physical(e,"kit_pad",p);if(fabsf((20+e->get(e,freq)*11980)-(100+p*35))>=.004f)fprintf(stderr,"pad=%d selected=%g Hz=%g expected=%d\n",p,e->get(e,pad)*15,20+e->get(e,freq)*11980,100+p*35);assert(fabsf((20+e->get(e,freq)*11980)-(100+p*35))<.004f);
  e->dispatch(e,7,find("kit_title"),0,display,0);char wanted[32];snprintf(wanted,sizeof wanted,"PAD %02d",p+1);assert(!strcmp(display,wanted));
  /* Turn immediately after selection, before any asynchronous notifications
   * or host rebind can reset the previous pad's accumulated raw position. */
  float immediate=e->get(e,fine)*(PARAMS[fine].max-PARAMS[fine].min);
  e->set(e,fine,e->get(e,fine)+.01f);
  assert(fabsf(e->get(e,fine)*(PARAMS[fine].max-PARAMS[fine].min)-immediate-1)<.02f);
  /* Read/rebind echoes cannot edit a pad or change its envelope context. */
  for(int i=0;i<NPARAMS;i++)if(!strncmp(PARAMS[i].key,"o",1) || !strncmp(PARAMS[i].key,"xy_",3))e->set(e,i,e->get(e,i));
  assert(roundf(e->get(e,pad)*15)==p);
  float before=e->get(e,fine)*(PARAMS[fine].max-PARAMS[fine].min);
  e->set(e,fine,e->get(e,fine)+.01f);float after=e->get(e,fine)*(PARAMS[fine].max-PARAMS[fine].min);
  assert(fabsf(after-before-1)<.02f);
 }
 physical(e,"kit_mode",1);physical(e,"kit_base",60);physical(e,"kit_output",.42f);physical(e,"kit_pad",12);
 void *chunk=NULL;intptr_t size=e->dispatch(e,23,0,0,&chunk,0);assert(size>65536 && size<1048576 && chunk);
 char *saved=malloc(size);assert(saved);memcpy(saved,chunk,size);
 e->dispatch(e,1,0,0,NULL,0);e=VSTPluginMain(host);assert(e);e->dispatch(e,24,0,size,saved,0);
 intptr_t restored=e->dispatch(e,23,0,0,&chunk,0);assert(restored==size && !memcmp(saved,chunk,size));
 for(int i=0;i<NPARAMS;i++)if(!strncmp(PARAMS[i].key,"human_",6)) {
  assert(e->dispatch(e,26,i,0,NULL,0)==0);e->set(e,i,1);assert(e->get(e,i)==0);
 }
 e->dispatch(e,23,0,0,&chunk,0);assert(!memcmp(saved,chunk,size));
 printf("Actual VST:16 pad contexts and read/write echoes, immediate fine handover, %ld-byte kit chunk close/reopen, retired Humanizer nonautomatable/inert PASS\n",(long)size);
 free(saved);
 /* Learn through actual VST MIDI events, including a repeated press and UI
  * echoes while armed. Native previews must not count as hardware notes. */
 int notes[]={48,50,52,53,55,57,59,36,60,62,64,65,67,69,71,72};
 int learn=find("kit_learn"),mapping=find("kit_mapping");physical(e,"kit_learn",1);
 for(int p=0;p<16;p++) {
  physical(e,"audition",1);e->set(e,learn,e->get(e,learn));
  struct {int32_t type,size,frame,flags,len,offset;unsigned char m[4];char pad[4];} midi={1,32,0,0,0,0,{0x90,notes[p],110,0},{0}};
  struct {int32_t n;intptr_t reserved;void *events[2];} events={1,0,{&midi,NULL}};
  e->dispatch(e,25,0,0,&events,0);float l[128],r[128],*out[]={l,r};e->render(e,NULL,out,128);
  assert(roundf(e->get(e,pad)*15)==p);
 }
 assert(e->get(e,learn)==0 && e->get(e,mapping)==1);
 e->dispatch(e,7,find("kit_learn_status"),0,display,0);assert(!strcmp(display,"16 PADS LEARNED"));
 size=e->dispatch(e,23,0,0,&chunk,0);saved=malloc(size);assert(saved);memcpy(saved,chunk,size);
 e->dispatch(e,1,0,0,NULL,0);e=VSTPluginMain(host);assert(e);e->dispatch(e,24,0,size,saved,0);
 restored=e->dispatch(e,23,0,0,&chunk,0);assert(restored==size && !memcmp(chunk,saved,size));
 assert(e->get(e,mapping)==1 && !e->get(e,learn));
 puts("Actual VST:hardware MIDI learn16 nonchromatic notes, armed echoes/preview isolation, auto-finish, full learned-map close/reopen PASS");
 free(saved);e->dispatch(e,1,0,0,NULL,0);return 0;
}
