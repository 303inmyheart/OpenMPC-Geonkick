/* Real VST fine hardware, coarse touch, feedback echoes and Classic clamps. */
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
struct Effect {int32_t magic;intptr_t(*dispatch)(Effect*,int32_t,int32_t,intptr_t,void*,float);void*p;
 void(*set)(Effect*,int32_t,float);float(*get)(Effect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void*obj,*user;int32_t uid,ver;void(*render)(Effect*,float**,float**,int32_t);};
extern Effect *VSTPluginMain(Callback);
static atomic_int watched,reports;static _Atomic float accepted;
static intptr_t host(Effect*e,int32_t op,int32_t i,intptr_t v,void*p,float n){(void)v;(void)p;
 if(op==0 && i==atomic_load(&watched)){atomic_store(&accepted,n);atomic_fetch_add(&reports,1);e->set(e,i,n);}
 return op==1?2400:0;
}
static int find(const char*k){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,k))return i;assert(0);return -1;}
static int key(const char*p,const char*s){char k[64];snprintf(k,sizeof k,"%s%s",p,s);return find(k);}
static float physical(Effect*e,int i){return PARAMS[i].min+e->get(e,i)*(PARAMS[i].max-PARAMS[i].min);}
int main(void){
 atomic_store(&watched,-1);Effect*e=VSTPluginMain(host);assert(e);
 for(int j=0;j<10;j++){
  char prefix[24]="";if(j)snprintf(prefix,sizeof prefix,"o%d_",j);
  int model=key(prefix,"filter_model"),touch=key(prefix,"cutoff_touch"),fine=key(prefix,"cutoff_fine"),wide=key(prefix,"cutoff_wide");
  e->set(e,model,1);e->set(e,touch,.312345f);float before=physical(e,wide),goal=20*powf(1000,.312345f);
  assert(fabsf(before-goal)<.002f);assert(fabsf(e->get(e,touch)-.312345f)<2e-6f);
  float raw=e->get(e,fine);int start=roundf(physical(e,fine));
  for(int n=0;n<12;n++){raw+=1.f/128;e->set(e,fine,raw);assert(roundf(physical(e,fine))==start+n+1);}
  float fc=physical(e,wide);assert(fabsf(fc-20*exp2f((start+12)/240.f))<.005f);
  e->set(e,touch,e->get(e,touch));assert(fabsf(physical(e,wide)-fc)<.005f);
  int res=key(prefix,"res_touch"),resfine=key(prefix,"res_fine"),q=key(prefix,"q_wide");
  e->set(e,res,.765432f);assert(fabsf(physical(e,q)-.5f*powf(40,.765432f))<.0001f);
  raw=e->get(e,resfine);start=roundf(physical(e,resfine));for(int n=0;n<12;n++){raw+=1.f/128;e->set(e,resfine,raw);assert(roundf(physical(e,resfine))==start+n+1);}
  e->set(e,model,0);atomic_store(&watched,touch);atomic_store(&reports,0);e->set(e,touch,1);
  for(int n=0;n<100 && !atomic_load(&reports);n++)usleep(5000);
  assert(atomic_load(&reports));float clamped=logf(8000.f/20)/logf(1000);
  assert(fabsf(e->get(e,touch)-clamped)<2e-6f);assert(fabsf(atomic_load(&accepted)-clamped)<2e-6f);
  assert(physical(e,key(prefix,"cutoff"))==8000);atomic_store(&watched,-1);
 }
 /* Humanizer hardware aliases use the same established direct/fine split. */
 for(int timing=0;timing<2;timing++){
  int touch=find(timing?"human_timing_touch":"human_velocity_touch"),fine=find(timing?"human_timing_fine":"human_velocity_fine"),original=find(timing?"human_timing":"human_velocity");
  e->set(e,touch,.371234f);assert(fabsf(physical(e,original)-30*.371234f)<.0001f);
  assert(fabsf(e->get(e,touch)-.371234f)<2e-6f);
  float raw=e->get(e,fine);int start=roundf(physical(e,fine));
  for(int n=0;n<12;n++){raw+=1.f/128;e->set(e,fine,raw);assert(roundf(physical(e,fine))==start+n+1);}
  assert(fabsf(physical(e,original)-(start+12)/(timing?4.f:10.f))<1e-5f);
 }
 e->dispatch(e,1,0,0,NULL,0);puts("10 filter paths: direct Touch, 5-cent/0.1-percent accumulated Q-Links, echo handover and stopped Classic clamp feedback plus Humanizer fine/direct aliases PASS");return 0;
}
