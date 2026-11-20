/* Fixed point envelopes: sound preservation, editable DSP and state migration. */
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
static void set(void *e,const char *k,const char *v){api->set_param(e,k,v);}
static float get(void *e,const char *k){char b[80];assert(api->get_param(e,k,b,sizeof b)>0);return strtof(b,NULL);}
static void ready(void *e){float b[2];for(int i=0;i<3000;i++){api->render(e,b,0);if(!get(e,"render_pending"))return;usleep(2000);}assert(!"worker timeout");}
static void capture(void *e,float *out,int frames){uint8_t stop[]={0xb0,120,0},note[]={0x90,60,127};api->midi(e,stop,3);api->midi(e,note,3);for(int i=0;i<frames;i+=128)api->render(e,out+i*2,frames-i<128?frames-i:128);}
int main(void){
 api=mpc_engine();void *a=api->create(NULL),*b=api->create(NULL);assert(a&&b);
 float original[8192],points[8192];char key[64],state[65536],again[65536],legacy[65536];
 /* Converting both simple curves must preserve a nontrivial complete sound. */
 set(a,"length","0.08");set(a,"o1_attack","0.01");set(a,"o1_decay","0.05");set(a,"o1_sweep_time","0.03");ready(a);capture(a,original,4096);
 set(a,"o1_a_mode","1");set(a,"o1_p_mode","1");ready(a);capture(a,points,4096);
 float error=0;for(int i=0;i<8192;i++)error=fmaxf(error,fabsf(original[i]-points[i]));
 printf("Simple -> Points full audio max error %.9g\n",error);assert(error<0.0001f);
 for(int o=1;o<=9;o++)for(int kind=0;kind<2;kind++){
  snprintf(key,sizeof key,"o%d_%c_mode",o,kind?'p':'a');set(a,key,"0");set(a,key,"1");
  assert(get(a,"ui_point_target")==2*(o-1)+kind);assert(get(a,"ui_point_mode")==1);
  for(int j=1;j<=4;j++){snprintf(key,sizeof key,"o%d_%c_t%d",o,kind?'p':'a',j);assert(get(a,key)>0);}
 }
 /* Touching a point selects its oscillator, envelope and point. All 18 curves stay independent. */
 set(a,"o7_p_l3","0.123456");assert(get(a,"plot_osc")==6);assert(get(a,"plot_env")==1);assert(get(a,"point_select")==3);
 assert(fabsf(get(a,"point_level_drag")-0.123456f)<1e-7);assert(get(a,"point_level_fine")==1235);
 set(a,"point_level_drag","0.654321");assert(fabsf(get(a,"o7_p_l3")-0.654321f)<1e-7);
 set(a,"point_level_fine","5001");assert(fabsf(get(a,"o7_p_l3")-0.5001f)<1e-7);
 assert(get(a,"o6_p_l3")!=get(a,"o7_p_l3"));
 set(a,"point_time_drag","0");assert(fabsf(get(a,"o7_p_t3")-0.0001f)<1e-7);
 set(a,"point_select","0");set(a,"point_time_drag","0.8");assert(fabsf(get(a,"o7_p_t3")-0.0001f)<1e-7);
 char display[80];api->get_param(a,"point_time_fine_display",display,sizeof display);assert(strstr(display,"START"));
 /* Full state keeps points/modes/fractions and older 186-value chunks stay Simple. */
 assert(api->get_param(a,"state",state,sizeof state)>0);assert(!strncmp(state,"GKMP9 ",6));
 set(b,"state",state);assert(api->get_param(b,"state",again,sizeof again)>0);assert(!strcmp(state,again));
 strcpy(legacy,state);memcpy(legacy,"GKMP3",5);char *end=legacy+6;for(int i=0;i<LEGACY_GP_COUNT;i++)strtof(end,&end);*end=0;
 set(b,"state",legacy);for(int o=1;o<=9;o++)for(int kind=0;kind<2;kind++){snprintf(key,sizeof key,"o%d_%c_mode",o,kind?'p':'a');assert(get(b,key)==0);}
 api->get_param(a,"state",state,sizeof state);
 set(a,"state","GKMP4 nan");api->get_param(a,"state",again,sizeof again);assert(!strcmp(state,again));
 strcpy(legacy,state);legacy[strlen(legacy)-1]='x';set(a,"state",legacy);api->get_param(a,"state",again,sizeof again);assert(!strcmp(state,again));
 set(a,"reset","1");set(a,"o1_a_mode","1");set(a,"o1_a_l2","0.42");set(a,"o1_a_mode","0");set(a,"o1_a_mode","1");assert(fabsf(get(a,"o1_a_l2")-0.42f)<1e-7);
 set(a,"o1_attack","0.02");assert(get(a,"o1_a_mode")==0);set(a,"plot_env","0");set(a,"point_copy","1");assert(get(a,"o1_a_mode")==1);assert(get(a,"o1_a_l2")!=0.42f);
 /* Custom zero amplitude is silent; custom pitch endpoints/time extremes stay finite. */
 for(int j=0;j<5;j++){snprintf(key,sizeof key,"o1_a_l%d",j);set(a,key,"0");}
 ready(a);capture(a,points,4096);for(int i=0;i<8192;i++)assert(points[i]==0);
 set(a,"o1_a_l1","1");set(a,"o1_p_mode","1");
 for(int j=0;j<5;j++){snprintf(key,sizeof key,"o1_p_l%d",j);set(a,key,j%2?"1":"0");}
 for(int j=1;j<=4;j++){snprintf(key,sizeof key,"o1_p_t%d",j);set(a,key,j%2?"0":"1");}
 ready(a);capture(a,points,4096);float energy=0;for(int i=0;i<8192;i++){assert(isfinite(points[i]));energy+=points[i]*points[i];}assert(energy>0.01f);
 unsigned count=get(a,"synthesis_count");set(a,"point_select","2");ready(a);assert(get(a,"synthesis_count")==count);
 api->destroy(a);api->destroy(b);puts("18 envelope contexts, fine/direct controls, safe durations, custom DSP, GKMP9 roundtrip / GKMP3 migration and view-only selection PASS");return 0;
}
