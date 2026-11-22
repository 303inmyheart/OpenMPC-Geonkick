/* Envelope selectors must show the selected layer, not the mixed sound. */
#include "params.h"
#include "engine.h"
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static const mpc_engine_t *api;
static void set(void *e,const char *key,float value){char text[48];snprintf(text,sizeof text,"%.9g",value);api->set_param(e,key,text);}
static float get(void *e,const char *key){char text[80];assert(api->get_param(e,key,text,sizeof text)>0);return strtof(text,NULL);}
static void *create(void){void *e=api->create(NULL);assert(e);return e;}
static void destroy(void *e){api->destroy(e);}
static void block(void *e){float out[256];api->render(e,out,128);}
static void ready(void *e,int playing){
 for(int n=0;n<3000;n++){if(playing)block(e);else {float out[2];api->render(e,out,0);}if(get(e,"render_pending")==0)return;usleep(2000);}assert(!"view timeout");
}
static void setup(void *e){
 for(int i=0;i<9;i++){char key[32];snprintf(key,sizeof key,"o%d_on",i+1);set(e,key,i%3==0);}
 const float frequency[]={55,277,831},attack[]={0,.04f,.11f},decay[]={.05f,.09f,.2f};
 for(int l=0;l<3;l++){char key[32];int o=l*3+1;snprintf(key,sizeof key,"layer%d",l+1);set(e,key,.8f-.1f*l);
  snprintf(key,sizeof key,"o%d_freq",o);set(e,key,frequency[l]);snprintf(key,sizeof key,"o%d_attack",o);set(e,key,attack[l]);snprintf(key,sizeof key,"o%d_decay",o);set(e,key,decay[l]);}
 set(e,"length",.2f);ready(e,0);
}
int main(void){
 api=mpc_engine();void *e=create(),*solo=create();setup(e);setup(solo);
 float before[3][864];char key[48],reference[48];unsigned comparisons=0;
 float syntheses=get(e,"synthesis_count");
 for(int playing=0;playing<2;playing++)for(int kind=0;kind<2;kind++)for(int focus=0;focus<9;focus++){
  int layer=focus/3;set(e,"plot_env",kind);set(e,"plot_osc",focus);
  for(int l=0;l<3;l++){snprintf(key,sizeof key,"layer%d",l+1);set(solo,key,l==layer?.8f-.1f*l:0);}
  ready(solo,playing);ready(e,playing);
  for(int i=0;i<432;i++)for(int ch=0;ch<2;ch++){
   snprintf(key,sizeof key,"viz_sound_%c_%d",ch?'n':'p',i);snprintf(reference,sizeof reference,"viz_%c_%d",ch?'n':'p',i);
   float actual=get(e,key),expected=get(solo,reference);assert(fabsf(actual-expected)<1e-6f);
   if(!playing&&!kind&&focus%3==0)before[layer][i*2+ch]=actual;
   else assert(actual==before[layer][i*2+ch]);
   comparisons++;
  }
  assert(get(e,"plot_osc")==focus);
 }
 assert(memcmp(before[0],before[1],sizeof before[0]));assert(memcmp(before[1],before[2],sizeof before[1]));
 assert(get(e,"synthesis_count")==syntheses);
 destroy(e);destroy(solo);
 printf("Envelope waveform: %u samples match independent solo-layer oracle; all9 direct selectors, Amp/Pitch, stopped/rendering, distinct layers and zero re-synthesis on view changes PASS\n",comparisons);
 return 0;
}
