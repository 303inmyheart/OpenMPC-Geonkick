/* Explicit SOUND selection: independent audible-wave oracle, proxies and navigation. */
#include "params.h"
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>
typedef struct Effect Effect;
typedef intptr_t(*Callback)(Effect*,int32_t,int32_t,intptr_t,void*,float);
struct Effect {int32_t magic;intptr_t(*dispatch)(Effect*,int32_t,int32_t,intptr_t,void*,float);void*p;void(*set)(Effect*,int32_t,float);float(*get)(Effect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void*obj,*user;int32_t uid,ver;void(*render)(Effect*,float**,float**,int32_t);};
extern Effect *VSTPluginMain(Callback);
static int find(const char*k){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,k))return i;assert(0);return -1;}
static _Atomic(Effect*) recording;
static atomic_int reported[NPARAMS];
static intptr_t host(Effect*e,int32_t op,int32_t i,intptr_t v,void*p,float o){(void)v;(void)p;(void)o;if(op==0 && e==atomic_load(&recording) && i>=0&&i<NPARAMS)atomic_fetch_add(reported+i,1);return op==1?2400:0;}
static void physical(Effect *e,const char *key,float v){int i=find(key);e->set(e,i,PARAMS[i].nopts?v/(PARAMS[i].nopts-1):(v-PARAMS[i].min)/(PARAMS[i].max-PARAMS[i].min));}
static float get(Effect *e,const char *key){return e->get(e,find(key));}
static void captions(Effect *e){char text[128];const char *keys[]={"ui_view_l1","ui_view_l2","ui_view_l3","ui_view_mix"};for(int i=0;i<4;i++)e->dispatch(e,7,find(keys[i]),0,text,0);}
static int matches(int layer,float start,float end){return layer==0?start>.9f&&end==0:layer==1?start<.02f:end>.7f;}
static void ready(Effect *e,int layer){for(int i=0;i<3000;i++){if(matches(layer,get(e,"viz_e_0"),get(e,"viz_e_431")))return;usleep(2000);}assert(!"view/synthesis timeout");}
static void setup(Effect *e){physical(e,"o1_decay",.05f);physical(e,"o4_attack",.3f);physical(e,"o7_decay",3);physical(e,"o4_freq",300);physical(e,"o7_freq",1000);physical(e,"layer2",1);physical(e,"layer3",1);physical(e,"length",.6f);ready(e,2);}
int main(void){
 Effect *e=VSTPluginMain(host),*reference=VSTPluginMain(host);assert(e&&reference&&e->npar==4096);setup(e);setup(reference);
 assert(e->dispatch(e,26,find("sound_layer"),0,NULL,0)==0);
 char key[80],other[80],text[128];
 for(int repeat=0;repeat<2;repeat++)for(int layer=0;layer<3;layer++){
  physical(e,"sound_layer",layer);assert(fabsf(get(e,"sound_layer")-layer/2.f)<1e-6f);ready(e,layer);
  /* Compare each cached layer to an independently audible soloed instance. */
  for(int l=0;l<3;l++){snprintf(key,sizeof key,"layer%d",l+1);physical(reference,key,l==layer?1:0);}physical(reference,"sound_layer",layer);ready(reference,layer);
  for(int col=0;col<432;col++)for(int ch=0;ch<2;ch++){
   snprintf(key,sizeof key,"viz_sound_%c_%d",ch?'n':'p',col);snprintf(other,sizeof other,"viz_%c_%d",ch?'n':'p',col);
   assert(fabsf(get(e,key)-get(reference,other))<1e-6f);
  }
  /* Hidden labels, ordinary scans and MIX/editor return paths are inert. */
  for(int draw=0;draw<3;draw++){captions(e);for(int i=0;i<NPARAMS;i++){e->get(e,i);e->dispatch(e,8,i,0,text,0);}usleep(120000);assert(fabsf(get(e,"sound_layer")-layer/2.f)<1e-6f);assert(matches(layer,get(e,"viz_e_0"),get(e,"viz_e_431")));}
 }
 atomic_store(&recording,e);
 for(int layer=0;layer<3;layer++){
  physical(e,"sound_layer",layer);
  for(int slot=1;slot<=3;slot++){
   char proxy[80],actual[80];snprintf(proxy,sizeof proxy,"sound_o%d_freq_fine",slot);snprintf(actual,sizeof actual,"o%d_freq_fine",layer*3+slot);
   int pi=find(proxy),ai=find(actual);assert(e->dispatch(e,26,pi,0,NULL,0)==0 && e->dispatch(e,26,ai,0,NULL,0)==1);
   assert(fabsf(get(e,proxy)-get(e,actual))<1e-6f);
   int before=(int)lroundf(get(e,actual)*(PARAMS[ai].max-PARAMS[ai].min));float untouched[9];for(int o=1;o<=9;o++){snprintf(key,sizeof key,"o%d_freq_fine",o);untouched[o-1]=get(e,key);}
   atomic_store(reported+ai,0);e->set(e,pi,get(e,proxy)+.01f);int after=(int)lroundf(get(e,actual)*(PARAMS[ai].max-PARAMS[ai].min));assert(after==before+1);
   for(int o=1;o<=9;o++)if(o!=layer*3+slot){snprintf(key,sizeof key,"o%d_freq_fine",o);assert(get(e,key)==untouched[o-1]);}
   for(int wait=0;wait<300 && !atomic_load(reported+ai);wait++){float l[128],r[128],*out[]={l,r};e->render(e,NULL,out,128);usleep(2000);}assert(atomic_load(reported+ai)>0);
   /* Existing direct touch retains its own physical/automation identity. */
   snprintf(key,sizeof key,"o%d_freq_touch",layer*3+slot);physical(e,key,.42f);assert(fabsf(get(e,proxy)-get(e,actual))<1e-6f);
   before=(int)lroundf(get(e,actual)*(PARAMS[ai].max-PARAMS[ai].min));e->set(e,pi,get(e,proxy)+.01f);after=(int)lroundf(get(e,actual)*(PARAMS[ai].max-PARAMS[ai].min));assert(after==before+1);
   snprintf(proxy,sizeof proxy,"sound_o%d_level",slot);snprintf(actual,sizeof actual,"o%d_level",layer*3+slot);physical(e,proxy,.37f);assert(fabsf(get(e,actual)-.37f)<1e-6f);
  }
  snprintf(key,sizeof key,"o%d_sample_on",layer*3+1);physical(e,key,1);e->dispatch(e,6,find("sound_o1_freq_fine"),0,text,0);assert(!strcmp(text,"st"));physical(e,key,0);
 }
 /* Switching layers restores their own remembered oscillator target. */
 physical(e,"o5_level",.31f);physical(e,"sound_layer",0);physical(e,"sound_layer",1);assert(fabsf(get(e,"plot_osc")-.5f)<1e-6f);
 physical(reference,"sound_layer",2);assert(get(e,"sound_layer")==.5f && get(reference,"sound_layer")==1);
 assert(find("sample_set_default")==4095);
 atomic_store(&recording,NULL);e->dispatch(e,1,0,0,NULL,0);reference->dispatch(reference,1,0,0,NULL,0);
 puts("Actual VST: explicit SOUND selector,432-column solo-audio waveform oracle, all9 scoped fine/touch/level inputs, automation isolation, sample units, remembered targets, inert label/navigation scans and independent instances PASS");return 0;
}
