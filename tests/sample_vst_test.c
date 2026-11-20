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
static intptr_t host(Effect*e,int32_t op,int32_t i,intptr_t v,void*p,float o){(void)e;(void)i;(void)v;(void)p;(void)o;return op==1?2400:0;}
static int find(const char*k){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,k))return i;assert(0);return -1;}
static void physical(Effect *e,const char *key,float v){int i=find(key);e->set(e,i,PARAMS[i].nopts?v/(PARAMS[i].nopts-1):(v-PARAMS[i].min)/(PARAMS[i].max-PARAMS[i].min));}
static char *encode(const unsigned char *in,size_t bytes){static const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";char *s=malloc((bytes+2)/3*4+1);assert(s);size_t n=0;
 for(size_t i=0;i<bytes;i+=3){unsigned v=(unsigned)in[i]<<16;if(i+1<bytes)v|=(unsigned)in[i+1]<<8;if(i+2<bytes)v|=in[i+2];s[n++]=alphabet[v>>18];s[n++]=alphabet[v>>12&63];s[n++]=i+1<bytes?alphabet[v>>6&63]:'=';s[n++]=i+2<bytes?alphabet[v&63]:'=';}s[n]=0;return s;}
int main(void) {
 Effect *e=VSTPluginMain(host);assert(e);physical(e,"o1_sweep",0);physical(e,"o1_decay",4);physical(e,"o1_level",1);
 void *chunk=NULL;intptr_t before=e->dispatch(e,23,0,0,&chunk,0);assert(before>0);
 char *state=malloc(16777216),*saved=malloc(16777216);assert(state&&saved);strcpy(state,chunk);
 unsigned count=44100*4;float *data=malloc(count*4);assert(data);for(unsigned i=0;i<count;i++)data[i]=.25f;
 char *pcm=encode((unsigned char*)data,count*4);size_t n=strlen(state);n+=(size_t)sprintf(state+n," SMP1 4");
 const int pads[]={0,0,1,15},osc[]={0,4,0,8};
 for(int i=0;i<4;i++)n+=(size_t)sprintf(state+n," %d %d 1 %u 666978747572652e776176 %s",pads[i],osc[i],count,pcm);
 assert(n>3*1024*1024 && n<16777216);e->dispatch(e,24,0,n+1,state,0);
 intptr_t bytes=e->dispatch(e,23,0,0,&chunk,0);assert(bytes==(intptr_t)n+1 && !memcmp(state,chunk,n+1));memcpy(saved,chunk,n+1);
 assert(e->get(e,find("o1_sample_on"))==1);char text[128];e->dispatch(e,6,find("o1_freq_fine"),0,text,0);assert(!strcmp(text,"st"));e->dispatch(e,7,find("sample_name"),0,text,0);assert(!strcmp(text,"fixture.wav"));
 physical(e,"o1_sample_on",0);assert(!e->get(e,find("sample_source")));e->dispatch(e,6,find("o1_freq_fine"),0,text,0);assert(!strcmp(text,"Hz"));physical(e,"sample_source",1);assert(e->get(e,find("o1_sample_on"))==1);
 e->dispatch(e,1,0,0,NULL,0);e=VSTPluginMain(host);assert(e);e->dispatch(e,24,0,n+1,saved,0);e->dispatch(e,23,0,0,&chunk,0);assert(!memcmp(saved,chunk,n+1));
 int ready=0;for(int i=0;i<2000;i++){float l[128],r[128],*out[]={l,r};e->render(e,NULL,out,128);float peak=e->get(e,find("viz_p_0"));if(peak>.2f&&peak<.35f){ready=1;break;}usleep(4000);}assert(ready);
 struct {int32_t type,size,frame,flags,len,offset;unsigned char msg[4];char pad[4];} m={1,32,0,0,0,0,{0x90,60,127,0},{0}};
 struct {int32_t n;intptr_t r;void *events[2];} ev={1,0,{&m,NULL}};e->dispatch(e,25,0,0,&ev,0);
 float l[128],r[128],*out[]={l,r};e->render(e,NULL,out,128);float energy=0;for(int i=0;i<128;i++){assert(isfinite(l[i])&&l[i]==r[i]);energy+=l[i]*l[i];}assert(energy>.1f);for(int i=64;i<128;i++)assert(fabsf(l[i]-.175f*(1-i/(4.f*44100)))<2e-6f);
 /* A complete valid scalar kit plus NaN asset must reject atomically. */
 data[0]=NAN;char *invalid=encode((unsigned char*)data,count*4);char *tail=strstr(state," SMP1 ");assert(tail);sprintf(tail," SMP1 1 0 0 1 %u 666978747572652e776176 %s",count,invalid);
 e->dispatch(e,24,0,strlen(state)+1,state,0);e->dispatch(e,23,0,0,&chunk,0);assert(!memcmp(saved,chunk,n+1));
 /* Fifteen4-second sources exceed the10MiB decoded budget, but the complete
  * test chunk fits16MiB. Reject before replacing any of the playable four. */
 char *limit=strstr(state," SMP1 ");assert(limit);size_t prefix=(size_t)(limit-state);
 size_t used=prefix+(size_t)sprintf(limit," SMP1 15");
 for(int slot=0;slot<15;slot++)used+=(size_t)sprintf(state+used," %d %d 1 %u 666978747572652e776176 %s",slot/9,slot%9,count,pcm);
 assert(used<16777216);e->dispatch(e,24,0,used+1,state,0);e->dispatch(e,23,0,0,&chunk,0);assert(!memcmp(saved,chunk,n+1));
 printf("Actual VST:%zu-byte four-sample kit, source toggles, full close/reopen, audible Sample MIDI and invalid-PCM atomic rejection PASS\n",n+1);
 e->dispatch(e,1,0,0,NULL,0);free(state);free(saved);free(data);free(pcm);free(invalid);return 0;
}
