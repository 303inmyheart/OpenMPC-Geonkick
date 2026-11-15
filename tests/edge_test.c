/* Real audio boundaries: short kicks end smoothly, Cut retriggers immediately
 * from the running waveform, and panic clears even queued choke tails. */
#include "params.h"
#include "engine.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static const mpc_engine_t *api;
static void set(void *e,const char *k,const char *v){api->set_param(e,k,v);}
static void ready(void *e){float b[2];char text[40];for(int n=0;n<3000;n++){api->render(e,b,0);api->get_param(e,"render_pending",text,sizeof text);if(!atoi(text))return;usleep(2000);}assert(!"worker timeout");}
static void note(void *e,int status,int pitch,int vel){uint8_t msg[]={status,pitch,vel};api->midi(e,msg,3);}
static void tone(void *e){set(e,"o1_sweep","0");set(e,"o1_freq","20");set(e,"o1_wave","1");set(e,"o1_decay","4");set(e,"length","0.03");ready(e);}
static void render(void *e,float *b,int n){for(int i=0;i<n;i+=127)api->render(e,b+2*i,n-i<127?n-i:127);}
int main(void){
 api=mpc_engine();void*a=api->create(NULL),*b=api->create(NULL);assert(a&&b);
 float x[8192],y[8192];tone(a);note(a,0x90,60,127);render(a,x,2048);
 float jump=0;for(int i=1291;i<1355;i++)jump=fmaxf(jump,fabsf(x[2*i]-x[2*(i-1)]));
 printf("30 ms kick last-boundary maximum sample delta %.7f\n",jump);assert(jump<0.003f);
 for(int i=1324;i<2048;i++)assert(x[2*i]==0);
 for(int k=0;k<2;k++){void*e=k?b:a;note(e,0xb0,120,0);set(e,"length","0.1");set(e,"o1_wave","0");set(e,"o1_freq","55");set(e,"o1_sweep","0");set(e,"o1_decay","4");set(e,"playback",k?"0":"1");ready(e);note(e,0x90,60,127);render(e,x,777);}
 render(b,y,512);note(a,0x90,60,127);render(a,x,512);
 assert(fabsf(x[0]-y[0])<1e-6f); // New hit starts at zero; old waveform continues at exactly this sample.
 jump=0;for(int i=1;i<512;i++)jump=fmaxf(jump,fabsf(x[2*i]-x[2*(i-1)]));
 printf("Cut retrigger first sample continuous; maximum transition delta %.7f\n",jump);assert(jump<0.03f);
 note(b,0xb0,120,0);note(b,0x90,60,127);render(b,y,512);
 for(int i=256;i<512;i++)assert(fabsf(x[2*i]-y[2*i])<1e-6f); // Old sound is gone; the new hit was never delayed.
 for(int n=0;n<24;n++){note(a,0x90,60,127);render(a,x,17);for(int i=0;i<34;i++)assert(isfinite(x[i]));}
 note(a,0xb0,120,0);render(a,x,512);for(int i=0;i<1024;i++)assert(x[i]==0);
 note(a,0x90,60,127);render(a,x,777);note(a,0x90,60,127);note(a,0xb0,123,0);render(a,x,512);for(int i=0;i<1024;i++)assert(x[i]==0);
 api->destroy(a);api->destroy(b);puts("Short-end, immediate Cut retrigger, rapid retriggers and panic tails PASS");return 0;
}
