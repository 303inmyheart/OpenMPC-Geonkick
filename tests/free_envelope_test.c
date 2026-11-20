/* Variable envelopes: independent sound/state/limits regression. GPL-3.0-or-later. */
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
static float get(void *e,const char *k){char b[96];assert(api->get_param(e,k,b,sizeof b)>0);return strtof(b,NULL);}
static void ready(void *e){float b[2];for(int i=0;i<4000;i++){api->render(e,b,0);if(!get(e,"render_pending"))return;usleep(2000);}assert(!"worker timeout");}
static void capture(void *e,float *out){uint8_t stop[]={0xb0,120,0},note[]={0x90,60,127};api->midi(e,stop,3);api->midi(e,note,3);for(int i=0;i<4096;i+=128)api->render(e,out+i*2,128);}
static int idx(const char *k){for(int i=0;i<GP_COUNT;i++)if(!strcmp(gparams[i].key,k))return i;assert(0);return 0;}
static void state(void *e,char *out){assert(api->get_param(e,"state",out,65536)>0);}
static void validate(void *e,int o,char kind,int n){char key[64];float prev=-1;
 for(int j=0;j<n;j++){snprintf(key,sizeof key,"f%d_%c_x%d",o,kind,j);float x=get(e,key);assert(x>prev);if(j==0)assert(x==0);if(j==n-1)assert(x==1);prev=x;}
}
int main(void){
 api=mpc_engine();void *a=api->create(NULL),*b=api->create(NULL);assert(a&&b);
 char saved[65536],again[65536],legacy[65536],key[64];float before[8192],after[8192];
 set(a,"length",.09f);set(a,"o1_attack",.01f);set(a,"o1_decay",.06f);set(a,"o1_sweep",0);set(a,"o1_a_mode",1);ready(a);capture(a,before);
 set(a,"free_add",1);assert(get(a,"f1_a_n")==6);ready(a);capture(a,after);float error=0;
 for(int i=0;i<8192;i++)error=fmaxf(error,fabsf(before[i]-after[i]));
 printf("Legacy -> variable table max audio error %.9g\n",error);assert(error<.001f);
 set(a,"free_select",1);set(a,"free_bend",.73f);ready(a);capture(a,before);
 set(a,"free_add",1);ready(a);capture(a,after);error=0;for(int i=0;i<8192;i++)error=fmaxf(error,fabsf(before[i]-after[i]));
 printf("Cubic insertion max audio error %.9g\n",error);assert(error<.00001f);
 /* All contexts carry independent 16-point curves and both control ordinates. */
 for(int o=1;o<=9;o++)for(int k=0;k<2;k++){
  set(a,"plot_osc",o-1);set(a,"plot_env",k);snprintf(key,sizeof key,"o%d_%c_mode",o,k?'p':'a');set(a,key,1);
  snprintf(key,sizeof key,"f%d_%c_n",o,k?'p':'a');while(get(a,key)<16)set(a,"free_add",1);assert(get(a,key)==16);
  set(a,"free_add",1);assert(get(a,key)==16);validate(a,o,k?'p':'a',16);
  set(a,"free_select",5);set(a,"free_level",.1234567f+.01f*o);set(a,"free_bend",k?-.68f:.49f);
  set(a,"free_time",0);validate(a,o,k?'p':'a',16);set(a,"free_time",1);validate(a,o,k?'p':'a',16);
  set(a,"free_bank",4);set(a,"edit_l1",.3333333f);assert(get(a,"free_select")==13);
  set(a,"edit_t1",.9f);validate(a,o,k?'p':'a',16);
 }
 state(a,saved);assert(!strncmp(saved,"GKMP8 ",6));assert(strlen(saved)>8192);
 api->set_param(b,"state",saved);state(b,again);assert(!strcmp(saved,again));
 /* Malformed point lists reject the entire state atomically. */
 float values[GP_COUNT];char *end=saved+6;for(int i=0;i<GP_COUNT;i++)values[i]=strtof(end,&end);
 int bad[]={idx("f1_a_n"),idx("f1_a_x0"),idx("f1_a_x1"),idx("f1_a_x15")};float invalid[]={1,.1f,0,.9f};
 for(int badi=0;badi<4;badi++){
  int n=snprintf(legacy,sizeof legacy,"GKMP8");for(int i=0;i<GP_COUNT;i++)n+=snprintf(legacy+n,sizeof legacy-n," %.9g",i==bad[badi]?invalid[badi]:values[i]);
  api->set_param(a,"state",legacy);state(a,again);assert(!strcmp(saved,again));
 }
 /* Real GKMP4 layout: old curves survive, extension migrates lazily. */
 strcpy(legacy,saved);memcpy(legacy,"GKMP4",5);end=legacy+6;for(int i=0;i<GKMP4_GP_COUNT;i++)strtof(end,&end);*end=0;
 api->set_param(b,"state",legacy);assert(get(b,"f1_a_n")==0);assert(get(b,"o1_a_mode")==1);set(b,"plot_osc",0);set(b,"plot_env",0);set(b,"free_add",1);assert(get(b,"f1_a_n")==6);
 /* Deletion can reach two endpoints; crossing and endpoint removal forbidden. */
 set(a,"plot_osc",0);set(a,"plot_env",0);
 for(int n=16;n>2;n--){set(a,"free_select",1);set(a,"free_delete",1);assert(get(a,"f1_a_n")==n-1);validate(a,1,'a',n-1);}
 set(a,"free_select",0);set(a,"free_delete",1);assert(get(a,"f1_a_n")==2);set(a,"free_select",1);set(a,"free_delete",1);assert(get(a,"f1_a_n")==2);
 ready(a);unsigned renders=get(a,"synthesis_count");set(a,"free_bank",4);set(a,"free_select",1);ready(a);assert(get(a,"synthesis_count")==renders);
 set(a,"free_add",1);assert(get(a,"f1_a_n")==3);set(a,"free_add",1);assert(get(a,"f1_a_n")==4);set(a,"free_add",1);assert(get(a,"f1_a_n")==5);
 /* Adjacent native time writes must restore untouched points exactly. */
 set(b,"reset",1);set(b,"o1_a_mode",1);
 for(int j=1;j<=4;j++){snprintf(key,sizeof key,"o1_a_t%d",j);set(b,key,.25f);}
 set(b,"free_select",2);set(b,"edit_t2",.35f);set(b,"edit_t3",.15f);
 assert(fabsf(get(b,"f1_a_x1")-.25f)<1e-6f);assert(fabsf(get(b,"f1_a_x2")-.6f)<1e-6f);assert(fabsf(get(b,"f1_a_x3")-.75f)<1e-6f);assert(get(b,"free_select")==2);
 set(b,"free_next",1);assert(get(b,"free_select")==3);set(b,"free_prev",1);assert(get(b,"free_select")==2);
 /* Absolute XY coordinates share the exact waveform/curve rectangle. */
 for(int o=1;o<=9;o++)for(int kind=0;kind<2;kind++){
  set(a,"plot_osc",o-1);set(a,"plot_env",kind);
  snprintf(key,sizeof key,"f%d_%c_n",o,kind?'p':'a');int n=(int)get(a,key);
  if(n<2){set(a,"free_add",1);n=(int)get(a,key);}
  assert(get(a,"ui_point_count")==n);
  for(int j=0;j<n;j++){
   set(a,"free_select",j);snprintf(key,sizeof key,"f%d_%c_x%d",o,kind?'p':'a',j);float x=get(a,key);
   snprintf(key,sizeof key,"f%d_%c_y%d",o,kind?'p':'a',j);float y=get(a,key);
   assert(fabsf((get(a,"xy_x")*1100+90)-(108+x*1064))<.0003f);
   assert(fabsf((250+170*(1-get(a,"xy_y")))-(268+134*(1-y)))<.0001f);
   snprintf(key,sizeof key,"point_x_%d",j);assert(fabsf(get(a,key)-get(a,"xy_x"))<1e-7f);
   snprintf(key,sizeof key,"point_y_%d",j);assert(fabsf(get(a,key)-get(a,"xy_y"))<1e-7f);
  }
  set(a,"free_select",1);set(a,"xy_y",(18+134*.246813f)/170);assert(fabsf(get(a,"free_level")-.246813f)<1e-6f);
 }
 set(a,"plot_osc",0);set(a,"plot_env",0);
 /* Legacy automation and COPY SIMPLE remain authoritative, not stale curves. */
 set(a,"o1_a_l2",.17f);assert(get(a,"f1_a_n")==0);set(a,"free_add",1);assert(get(a,"f1_a_n")==6);set(a,"point_copy",1);assert(get(a,"f1_a_n")==0);
 api->destroy(a);api->destroy(b);puts("18 variable envelopes, 2..16 points, cubic split, touch viewport, bounds, atomic GKMP8/GKMP4 migration and no-synthesis selection PASS");return 0;
}
