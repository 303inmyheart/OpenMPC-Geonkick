/* Replay MPC's first-tab/visible-tab formatting order and UI rebuild feedback. */
#include "params.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>
typedef struct Effect Effect;
typedef intptr_t(*Callback)(Effect*,int32_t,int32_t,intptr_t,void*,float);
struct Effect {int32_t magic;intptr_t(*dispatch)(Effect*,int32_t,int32_t,intptr_t,void*,float);void*p;void(*set)(Effect*,int32_t,float);float(*get)(Effect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void*obj,*user;int32_t uid,ver;void(*render)(Effect*,float**,float**,int32_t);};
extern Effect *VSTPluginMain(Callback);
static _Atomic(Effect*) tracked;
static atomic_int visible,draws;
static _Atomic float drawn_start,drawn_end,drawn_wave_end;
static int find(const char*k){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,k))return i;assert(0);return -1;}
static void format(Effect *e,int layer){char key[40],text[128];if(layer<0)snprintf(key,sizeof key,"ui_view_mix");else snprintf(key,sizeof key,"ui_view_l%d",layer+1);e->dispatch(e,7,find(key),0,text,0);assert(strstr(text,"WAVEFORM"));}
static intptr_t host(Effect*e,int32_t op,int32_t i,intptr_t v,void*p,float o){
 (void)i;(void)v;(void)p;(void)o;
 if(op==42 && e==atomic_load(&tracked)){
  /* Device trace: a rebind formats first-tab labels, then visible-tab labels.
   * In 0.0.25 both changed the display revision and perpetuated more rebinds. */
  int layer=atomic_load(&visible);
  atomic_store(&drawn_start,e->get(e,find("viz_e_0")));
  atomic_store(&drawn_end,e->get(e,find("viz_e_431")));
  atomic_store(&drawn_wave_end,e->get(e,find("viz_sound_p_431"))+e->get(e,find("viz_sound_n_431")));
  format(e,0);usleep(40000);format(e,layer);
  atomic_fetch_add(&draws,1);
 }
 return op==1?2400:0;
}
static void physical(Effect *e,const char *key,float v){int i=find(key);e->set(e,i,PARAMS[i].nopts?v/(PARAMS[i].nopts-1):(v-PARAMS[i].min)/(PARAMS[i].max-PARAMS[i].min));}
static int matches(int layer,float start,float end){return layer==0?start>.9f&&end==0:layer==1?start<.02f:end>.7f;}
int main(void){
 Effect *e=VSTPluginMain(host);assert(e&&e->npar==4096);
 physical(e,"o1_decay",.05f);physical(e,"o4_attack",.3f);physical(e,"o7_decay",3);physical(e,"layer2",1);physical(e,"layer3",1);physical(e,"length",.6f);
 int ready=0;for(int i=0;i<3000;i++){float l[128],r[128],*out[]={l,r};e->render(e,NULL,out,128);if(e->get(e,find("viz_e_431"))>.7f){ready=1;break;}usleep(2000);}assert(ready);
 atomic_store(&tracked,e);
 for(int repeat=0;repeat<2;repeat++)for(int layer=0;layer<4;layer++){
  int target=layer==3?-1:layer;atomic_store(&visible,target);int before=atomic_load(&draws);
  format(e,target);
  /* No render calls: context updates must also work with transport stopped. */
  int settled=0;for(int i=0;i<500;i++){
   if(atomic_load(&draws)>before && matches(target,e->get(e,find("viz_e_0")),e->get(e,find("viz_e_431"))) && matches(target,atomic_load(&drawn_start),atomic_load(&drawn_end))){settled=1;break;}usleep(10000);
  }assert(settled);
  usleep(800000);
  int callbacks=atomic_load(&draws)-before;
  fprintf(stderr,"Layer%d: %d rebind(s), start%.4f/end%.4f\n",target+1,callbacks,atomic_load(&drawn_start),atomic_load(&drawn_end));
  assert(callbacks>=1 && callbacks<=3);
  assert(matches(target,e->get(e,find("viz_e_0")),e->get(e,find("viz_e_431"))));
  if(target==0)assert(atomic_load(&drawn_wave_end)==0);
  else if(target>0)assert(atomic_load(&drawn_wave_end)>0.01f);
  /* A complete host get/name scan must not select another page. */
  char text[128];for(int i=0;i<NPARAMS;i++){e->get(e,i);e->dispatch(e,8,i,0,text,0);}
  assert(matches(target,e->get(e,find("viz_e_0")),e->get(e,find("viz_e_431"))));
 }
 Effect *b=VSTPluginMain(host);assert(b);atomic_store(&visible,1);format(e,1);format(b,0);
 for(int i=0;i<500 && e->get(e,find("viz_e_0"))>=.02f;i++)usleep(10000);
 assert(e->get(e,find("viz_e_0"))<.02f);format(b,-1);usleep(250000);assert(e->get(e,find("viz_e_0"))<.02f);
 b->dispatch(b,1,0,0,NULL,0);assert(find("sample_set_default")==4095);
 atomic_store(&tracked,NULL);e->dispatch(e,1,0,0,NULL,0);
 puts("Actual VST: MPC first-tab/visible-tab rebind replay settles without feedback loop, cached wave/envelope follow all views while stopped, scans inert and instances isolated PASS");return 0;
}
