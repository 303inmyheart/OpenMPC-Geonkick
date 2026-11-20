/* Actual shared VST chunk capacity + canonical proxy feedback. GPL-3.0-or-later. */
#include "params.h"
#include "../src/param_defs.h"
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
typedef struct Effect Effect;
typedef intptr_t (*Callback)(Effect*,int32_t,int32_t,intptr_t,void*,float);
struct Effect {int32_t magic;intptr_t(*dispatch)(Effect*,int32_t,int32_t,intptr_t,void*,float);void*p;
 void(*set)(Effect*,int32_t,float);float(*get)(Effect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void*obj,*user;int32_t uid,ver;void(*render)(Effect*,float**,float**,int32_t);};
extern Effect *VSTPluginMain(Callback);
static intptr_t host(Effect*e,int32_t op,int32_t i,intptr_t v,void*p,float o){(void)e;(void)i;(void)v;(void)p;(void)o;return op==1?2400:0;}
static int find(const char*k){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,k))return i;assert(0);return -1;}
int main(void){
 Effect *e=VSTPluginMain(host);assert(e);char input[65536],before[65536],after[65536];int n=snprintf(input,sizeof input,"GKMP6");
 for(int i=0;i<GP_COUNT;i++){
  const char *key=gparams[i].key;float v=gparams[i].def;int osc,j;char kind,tail;
  if(sscanf(key,"f%d_%c_%c%d",&osc,&kind,&tail,&j)==4){if(tail=='x')v=j/15.f;else if(tail=='y')v=.23f+.013f*j;else v=tail=='c'?-.27f:.43f;}
  if(key[0]=='f' && key[1]>='1' && key[1]<='9' && key[strlen(key)-1]=='n')v=16;
  if(key[0]=='o' && strstr(key,"_mode"))v=1;
  if(key[0]=='o' && strstr(key,"_seen"))v=1;
  n+=snprintf(input+n,sizeof input-n," %.9g",v);
 }
 assert(n>8192);e->dispatch(e,24,0,n+1,input,0);void *chunk=NULL;intptr_t size=e->dispatch(e,23,0,0,&chunk,0);
 assert(size>8192 && size<65536 && chunk);strcpy(before,chunk);assert(!strncmp(before,"GKMP8 ",6));assert(!strncmp(input+6,before+6,strlen(input+6)));
 /* A complete host rebind echoes each real proxy and must preserve controls,
  * selected point/bank and both independent Bezier handles. */
 const char *keys[]={"xy_x","xy_y","free_level","free_time","free_bend","free_select","free_bank","edit_l0","edit_l1","edit_l2","edit_l3","edit_l4","edit_t1","edit_t2","edit_t3","edit_t4"};
 for(int pass=0;pass<4;pass++)for(unsigned j=0;j<sizeof keys/sizeof *keys;j++){int i=find(keys[j]);float v=e->get(e,i);e->set(e,i,v);}
 size=e->dispatch(e,23,0,0,&chunk,0);strcpy(after,chunk);assert(!strcmp(before,after));
 /* Saved chunk survives native VST close/new-instance restore. */
 e->dispatch(e,1,0,0,NULL,0);e=VSTPluginMain(host);assert(e);e->dispatch(e,24,0,size,after,0);
 e->dispatch(e,23,0,0,&chunk,0);assert(!strcmp(before,chunk));
 printf("Actual shared VST: %ld-byte complete 18x16-point chunk and all proxy echoes preserve exact state PASS\n",(long)size);
 /* Maximum curve population: all18 envelopes on all16 pads have16 points,
  * exercising the actual host's chunk copy, not only the engine serializer. */
 char *kit=malloc(1048576);assert(kit);
 int total=snprintf(kit,1048576,"GKMP8%s KIT1 1 36 0 1",input+5);
 for(int pad=1;pad<16;pad++)total+=snprintf(kit+total,1048576-total," P%s",input+5);
 assert(total>200000 && total<1048576);e->dispatch(e,24,0,total+1,kit,0);
 size=e->dispatch(e,23,0,0,&chunk,0);assert(size==total+1 && !memcmp(kit,chunk,size));
 e->dispatch(e,1,0,0,NULL,0);e=VSTPluginMain(host);assert(e);e->dispatch(e,24,0,size,kit,0);
 e->dispatch(e,23,0,0,&chunk,0);assert(!memcmp(kit,chunk,size));
 printf("Actual shared VST:all16x18x16 points, %ld-byte kit survives full close/reopen PASS\n",(long)size);
 free(kit);e->dispatch(e,1,0,0,NULL,0);return 0;
}
