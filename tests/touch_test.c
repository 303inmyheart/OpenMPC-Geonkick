/* Actual VST: continuous Touch handles and fine Q-Link handles share physical values. */
#include "params.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
typedef struct Effect Effect;
typedef intptr_t (*Callback)(Effect*,int32_t,int32_t,intptr_t,void*,float);
struct Effect { int32_t magic;intptr_t(*dispatch)(Effect*,int32_t,int32_t,intptr_t,void*,float);void *p;
 void(*set)(Effect*,int32_t,float);float(*get)(Effect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void *obj,*user;int32_t uid,ver;
 void(*render)(Effect*,float**,float**,int32_t); };
extern Effect *VSTPluginMain(Callback);
static int is_touch(int i){size_t n=strlen(PARAMS[i].key);return n>=6&&!strcmp(PARAMS[i].key+n-6,"_touch");}
static intptr_t host(Effect *e,int32_t op,int32_t i,intptr_t v,void *p,float o){(void)v;(void)p;if(op==0&&i>=0&&i<NPARAMS&&is_touch(i))e->set(e,i,o);return op==1?2400:0;}
static int find(const char *key){for(int i=0;i<NPARAMS;i++)if(!strcmp(key,PARAMS[i].key))return i;assert(0);return 0;}
static void block(Effect *e){float l[128],r[128],*out[]={l,r};e->render(e,NULL,out,128);}
static float hz(Effect *e,int i){return 20+e->get(e,i)*11980;}
int main(void){
 Effect *e=VSTPluginMain(host);assert(e);int touch_count=0;
 for(int osc=1;osc<=9;osc++){
  char key[50];snprintf(key,sizeof key,"o%d_freq_touch",osc);int touch=find(key);
  snprintf(key,sizeof key,"o%d_freq_fine",osc);int fine=find(key);
  snprintf(key,sizeof key,"o%d_freq",osc);int original=find(key);
  float goals[]={0.25f,0.255f,0.8f,0.1f,0.9f,0.312345f};
  for(unsigned n=0;n<sizeof goals/sizeof goals[0];n++){
   float goal=goals[n];e->set(e,touch,goal);float expected=20*powf(600,goal);
   assert(fabsf(e->get(e,touch)-goal)<2e-6f);assert(fabsf(hz(e,original)-expected)<0.15f);
   block(e);usleep(20000);assert(fabsf(e->get(e,touch)-goal)<2e-6f);
  }
  float base=hz(e,original),pitch=240*log2f(base/55);assert(fabsf(pitch-roundf(pitch))>0.01f);
  float wanted=55*exp2f((roundf(pitch)+1)/240.f);
  e->set(e,fine,e->get(e,fine)+1.f/128);block(e);assert(fabsf(hz(e,original)-wanted)<0.01f);
  assert(fabsf(e->get(e,touch)-logf(wanted/20)/logf(600))<2e-6f);
  e->set(e,original,0.5f);block(e);e->set(e,touch,e->get(e,touch));usleep(250000);block(e);
  assert(fabsf(hz(e,original)-6010)<0.01f);
 }
 for(int i=0;i<NPARAMS;i++)if(is_touch(i)){
  touch_count++;if(strstr(PARAMS[i].key,"_freq_touch"))continue;
  char target[80];strcpy(target,PARAMS[i].key);target[strlen(target)-6]=0;int original=find(target);
  float goals[]={0.123456f,0.765432f};
  for(int j=0;j<2;j++){e->set(e,i,goals[j]);block(e);assert(fabsf(e->get(e,i)-goals[j])<2e-6f);assert(fabsf(e->get(e,original)-goals[j])<2e-6f);}
 }
 assert(touch_count==46);puts("46 continuous touch controls: direct motion, display readback, preserved fractions, fine Q-Link handover and legacy Hz echoes PASS");
 /* Selected point fine entry must survive asynchronous counterpart feedback. */
 int mode=find("o1_a_mode"),select=find("point_select"),fine=find("point_level_fine"),drag=find("point_level_drag");
 e->set(e,mode,1);e->set(e,select,0.5f);e->set(e,drag,0.432123f);block(e);usleep(50000);
 assert(fabsf(e->get(e,drag)-0.432123f)<2e-6f);
 float raw=e->get(e,fine);int start=roundf(raw*10000);
 for(int j=0;j<12;j++){raw+=1.f/128;e->set(e,fine,raw);block(e);usleep(20000);assert(roundf(e->get(e,fine)*10000)==start+j+1);}
 assert(fabsf(e->get(e,drag)-(start+12)/10000.f)<2e-6f);
 e->set(e,select,1);block(e);usleep(50000);assert(e->get(e,drag)<1e-6f);
 puts("Point editor continuous touch, accumulated fine Q-Link motion and context handover PASS");
 e->dispatch(e,1,0,0,NULL,0);return 0;
}
