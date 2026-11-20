/* Sample-accurate humanizer audio, pending lifetime, panic/Gate/Cut and migration. */
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
static void set(void *e,const char*k,float v){char b[48];snprintf(b,sizeof b,"%.9g",v);api->set_param(e,k,b);}
static float get(void *e,const char*k){char b[80];assert(api->get_param(e,k,b,sizeof b)>0);return strtof(b,NULL);}
static void ready(void*e){float x[2];for(int i=0;i<4000;i++){api->render(e,x,0);if(!get(e,"render_pending"))return;usleep(2000);}assert(!"worker timeout");}
static void msg(void*e,int type,int note,int vel){uint8_t m[]={(uint8_t)type,(uint8_t)note,(uint8_t)vel};api->midi(e,m,3);}
static void render(void*e,float*p,int frames){float scratch[256];for(int n=0;n<frames;n+=128)api->render(e,p?p+2*n:scratch,frames-n<128?frames-n:128);}
static int delayed(void*e,int channel){
 for(int i=0;i<64;i++){msg(e,0xb0,120,0);msg(e,0x90|channel,60,80);int d=get(e,"human_last_delay");if(d>=500)return d;}assert(!"no delayed hit");return 0;
}
int main(void){
 api=mpc_engine();void *a=api->create(NULL),*b=api->create(NULL);assert(a&&b);float x[16384],y[16384];
 assert(get(a,"human_on")==0);ready(a);ready(b);msg(b,0x90,60,80);render(b,y,4096);
 /* Off and enabled zero amounts are sample-identical to the unmodified hit. */
 set(a,"human_velocity",30);set(a,"human_timing",30);msg(a,0x90,60,80);render(a,x,4096);for(int i=0;i<8192;i++)assert(x[i]==y[i]);
 set(a,"human_on",1);set(a,"human_velocity",0);set(a,"human_timing",0);msg(a,0xb0,120,0);msg(a,0x90,60,80);render(a,x,4096);for(int i=0;i<8192;i++)assert(x[i]==y[i]);
 unsigned synths=get(a,"synthesis_count");set(a,"human_velocity",30);set(a,"human_timing",30);ready(a);assert(get(a,"synthesis_count")==synths);
 int different_v=0,different_d=0,last_v=-1,last_d=-1;
 for(int n=0;n<64;n++){
  msg(a,0xb0,120,0);msg(a,0x90,60,80);int v=get(a,"human_last_velocity"),delay=get(a,"human_last_delay");
  assert(v>=56&&v<=104&&delay>=0&&delay<=1323);if(last_v>=0)different_v|=v!=last_v;if(last_d>=0)different_d|=delay!=last_d;last_v=v;last_d=delay;
  render(a,x,4096+delay);for(int i=0;i<delay*2;i++)assert(x[i]==0);
  for(int i=0;i<8192;i++)assert(fabsf(x[2*delay+i]-y[i]*(v/80.f))<2e-6f);
 }
 assert(different_v&&different_d);
 /* Gate cancel is note/channel scoped, including notes not started yet. */
 set(a,"human_velocity",0);set(a,"playback",2);int delay=delayed(a,1);msg(a,0x80,60,0);assert(get(a,"pending_count")==1);
 render(a,x,200);for(int i=0;i<400;i++)assert(x[i]==0);msg(a,0x81,60,0);assert(get(a,"pending_count")==0);render(a,x,2048);for(int i=0;i<4096;i++)assert(x[i]==0);
 /* A pending hit pins its arrival-time cache even while a new sound renders. */
 set(a,"playback",0);delay=delayed(a,0);set(a,"o1_freq",12000);ready(a);render(a,x,delay);for(int i=0;i<delay*2;i++)assert(x[i]==0);
 msg(b,0xb0,120,0);msg(b,0x90,60,80);render(a,x,4096);render(b,y,4096);for(int i=0;i<8192;i++)assert(x[i]==y[i]);
 /* Dense note streams stay bounded, and panic clears future as well as active hits. */
 for(int n=0;n<256;n++)msg(a,0x90,n%128,80);
 assert(get(a,"pending_count")<=64);assert(get(a,"pending_dropped")>0);
 msg(a,0xb0,123,0);assert(get(a,"pending_count")==0);render(a,x,4096);for(int i=0;i<8192;i++)assert(x[i]==0);
 delayed(a,0);set(a,"reset",1);assert(get(a,"pending_count")==0);ready(a);
 /* Cut starts at the randomized onset; the old hit is not choked early. */
 set(a,"reset",1);set(b,"reset",1);ready(a);ready(b);
 for(int attempt=0;attempt<64;attempt++){
  set(a,"human_on",0);set(a,"playback",1);msg(a,0xb0,120,0);msg(b,0xb0,120,0);
  msg(a,0x90,60,80);msg(b,0x90,60,80);render(a,x,200);render(b,y,200);
  set(a,"human_on",1);set(a,"human_timing",30);set(a,"human_velocity",0);msg(a,0x90,61,80);delay=get(a,"human_last_delay");
  if(delay>500)break;
 }
 assert(delay>500);render(a,x,delay);render(b,y,delay);for(int i=0;i<delay*2;i++)assert(x[i]==y[i]);
 render(a,x,1);render(b,y,1);assert(x[0]==y[0]);msg(a,0xb0,120,0);
 char state[65536],again[65536],legacy[65536];set(a,"human_velocity",7.125f);set(a,"human_timing",13.376f);assert(api->get_param(a,"state",state,sizeof state)>0);assert(!strncmp(state,"GKMP7 ",6));api->set_param(b,"state",state);api->get_param(b,"state",again,sizeof again);assert(!strcmp(state,again));
 strcpy(legacy,state);memcpy(legacy,"GKMP6",5);char*end=legacy+6;for(int i=0;i<GKMP6_GP_COUNT;i++)strtof(end,&end);*end=0;api->set_param(b,"state",legacy);assert(get(b,"human_on")==0);assert(get(b,"human_velocity")==5&&get(b,"human_timing")==5);
 api->destroy(a);api->destroy(b);puts("Humanizer: off/zero exact,64 sample/velocity oracles, Gate/channel cancel, pinned cache, bounded overflow, panic/reset, Cut onset and GKMP6 migration PASS");return 0;
}
