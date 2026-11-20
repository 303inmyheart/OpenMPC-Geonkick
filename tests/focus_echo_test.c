/* Real VST regression: native UI read/write echoes must not move envelope focus
 * or quantize oscillator state when the host rebinds after a graph refresh. */
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
static atomic_int echo_enabled,rebinds;static _Thread_local int inside;
static int find(const char*k){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,k))return i;assert(0);return -1;}
static int plain(int i){const param_t*p=&PARAMS[i];return PARAMS[i].key[0]=='o' && !p->nopts&&!p->int_display&&!p->momentary&&!p->string_display&&p->step_target<0&&p->popup_of<0&&!p->nudge_pct&&p->nudge_gain<=1;}
static void rebind(Effect*e){for(int i=0;i<NPARAMS;i++)if(plain(i)){float value=e->get(e,i);e->set(e,i,value);}}
static intptr_t host(Effect*e,int32_t op,int32_t i,intptr_t v,void*p,float o){(void)i;(void)v;(void)p;(void)o;
 if(op==42 && atomic_load(&echo_enabled) && !inside){inside=1;atomic_fetch_add(&rebinds,1);rebind(e);inside=0;}
 return op==1?2400:0;
}
static void block(Effect*e){float l[128],r[128],*out[]={l,r};e->render(e,NULL,out,128);}
int main(void){
 Effect*e=VSTPluginMain(host);assert(e);int focus=find("plot_osc"),env=find("plot_env");
 /* Touch values deliberately do not lie on the legacy physical six-digit grid. */
 for(int osc=1;osc<=9;osc++){
  char k[40];snprintf(k,sizeof k,"o%d_freq_touch",osc);e->set(e,find(k),.312345f+.01f*osc);
  snprintf(k,sizeof k,"o%d_attack_touch",osc);e->set(e,find(k),.0123456f+.001f*osc);
 }
 float before[NPARAMS];for(int i=0;i<NPARAMS;i++)before[i]=e->get(e,i);
 atomic_store(&echo_enabled,1);
 for(int playing=0;playing<2;playing++)for(int selected=0;selected<9;selected++)for(int kind=0;kind<2;kind++){
  e->set(e,focus,selected/8.f);e->set(e,env,(float)kind);
  rebind(e);if(playing)for(int n=0;n<8;n++)block(e);
  usleep(260000);
  if(e->get(e,focus)!=selected/8.f)fprintf(stderr,"focus reset: selected%d, actual%.0f\n",selected,e->get(e,focus)*8);
  assert(e->get(e,focus)==selected/8.f);assert(e->get(e,env)==kind);
  for(int i=0;i<NPARAMS;i++)if(plain(i))assert(e->get(e,i)==before[i]);
 }
 /* A genuinely changed value must still select its edited oscillator. */
 e->set(e,find("o3_level"),.432123f);assert(e->get(e,focus)==2/8.f);
 assert(fabsf(e->get(e,find("o3_level"))-.432123f)<1e-6f);
 assert(atomic_load(&rebinds)>0);printf("All 9 targets x Amp/Pitch, stopped/rendering, exact float echoes and %d async rebinds: focus/state preserved PASS\n",atomic_load(&rebinds));
 e->dispatch(e,1,0,0,NULL,0);return 0;
}
