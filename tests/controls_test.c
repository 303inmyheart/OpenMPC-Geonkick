/* Actual VST wheel/Q-Link regression: fine Hz steps, echoes and legacy Hz. */
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
struct Effect { int32_t magic;intptr_t(*dispatch)(Effect*,int32_t,int32_t,intptr_t,void*,float);void *p;
 void(*set)(Effect*,int32_t,float);float(*get)(Effect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void *obj,*user;int32_t uid,ver;
 void(*render)(Effect*,float**,float**,int32_t); };
extern Effect *VSTPluginMain(Callback);
static atomic_int echo_index=-1,echoes;
static intptr_t host(Effect *e,int32_t op,int32_t index,intptr_t value,void *ptr,float opt) {
    (void)value;(void)ptr;
    if(op==0 && index==atomic_load(&echo_index)) { atomic_fetch_add(&echoes,1);e->set(e,index,opt); }
    return op==1?2400:0;
}
static int find(const char *key) { for(int i=0;i<NPARAMS;i++)if(!strcmp(key,PARAMS[i].key))return i;assert(0);return 0; }
static void block(Effect *e) { float l[128],r[128],*out[]={l,r};e->render(e,NULL,out,128); }
static float hz(Effect *e,int index) { return 20+e->get(e,index)*11980; }
int main(void) {
    Effect *e=VSTPluginMain(host);assert(e&&e->magic==0x56737450);
    float largest=0;
    for(int oscillator=1;oscillator<=9;oscillator++) {
        char key[40];snprintf(key,sizeof key,"o%d_freq_fine",oscillator);int fine=find(key);
        snprintf(key,sizeof key,"o%d_freq",oscillator);int original=find(key);
        atomic_store(&echo_index,fine);
        e->set(e,original,(55-20.f)/11980);block(e);assert(fabsf(hz(e,original)-55)<0.002f);
        for(int step=1;step<=6;step++) {
            float before=hz(e,original);e->set(e,fine,e->get(e,fine)+0.01f);block(e);
            float after=hz(e,original),expected=55*exp2f(step/240.f);
            assert(after>before && fabsf(after-expected)<0.003f);largest=fmaxf(largest,after-before);
            float settled=e->get(e,fine);block(e);assert(fabsf(e->get(e,fine)-settled)<0.000001f);
        }
        for(int step=1;step<=6;step++) { e->set(e,fine,e->get(e,fine)-1.f/128);block(e);assert(fabsf(hz(e,original)-55*exp2f((6-step)/240.f))<0.003f); }
        assert(fabsf(hz(e,original)-55)<0.003f);
        /* MPC keeps accumulating raw input independently of our accepted
         * fine value. Reproduce both event rates, reversals and clamped tails. */
        float span=PARAMS[fine].max-PARAMS[fine].min;
        for(int mode=0;mode<2;mode++) {
            float raw=e->get(e,fine),start=raw*span,rate=mode?1.f/128:0.01f;
            for(int k=1;k<=90;k++) {
                raw=fminf(1,raw+rate);e->set(e,fine,raw);block(e);
                if(fabsf(e->get(e,fine)*span-start-k)>=0.02f)fprintf(stderr,"raw regression osc=%d mode=%d k=%d raw=%g actual=%g expected=%g\n",oscillator,mode,k,raw,e->get(e,fine)*span,start+k);
                assert(fabsf(e->get(e,fine)*span-start-k)<0.02f);
            }
            for(int k=1;k<=90;k++) {
                raw=fmaxf(0,raw-rate);e->set(e,fine,raw);block(e);
                assert(fabsf(e->get(e,fine)*span-start-(90-k))<0.02f);
            }
        }
        e->set(e,fine,0);block(e);assert(fabsf(hz(e,original)-20)<0.01f);
        e->set(e,fine,1);block(e);assert(fabsf(hz(e,original)-12000)<0.02f);
        /* Existing linear-Hz automation retains its exact physical meaning. */
        e->set(e,original,0.5f);block(e);assert(fabsf(hz(e,original)-6010)<0.02f);
        e->set(e,fine,e->get(e,fine));block(e);
        usleep(250000);block(e);assert(fabsf(hz(e,original)-6010)<0.02f);
    }
    atomic_store(&echo_index,-1);
    int fine=find("o1_freq_fine"),original=find("o1_freq");
    float span=PARAMS[fine].max-PARAMS[fine].min;
    for(int position=30;position<(int)span-30;position++) {
        float physical=55*exp2f((position+PARAMS[fine].min)/240.f);
        e->set(e,original,(physical-20)/11980);float before=e->get(e,fine)*span;
        float request=roundf((e->get(e,fine)+0.01f)*1000)/1000;
        e->set(e,fine,request);assert(fabsf(e->get(e,fine)*span-before-1)<0.01f);
    }
    for(int i=0;i<NPARAMS;i++)if(PARAMS[i].nudge_pct>0 && !strcmp(PARAMS[i].unit,"ms")) {
        e->set(e,i,0.5f);float before=e->get(e,i);float range=PARAMS[i].max-PARAMS[i].min;
        e->set(e,i,before+0.01f);assert(fabsf((e->get(e,i)-before)*range-1)<0.01f);
    }
    puts("Rounded small gestures across the complete pitch range and exact-grid ms nudges: one step, no integer-boundary jumps PASS");
    atomic_store(&echo_index,-1);assert(atomic_load(&echoes)>0);
    printf("9 oscillator controls: accumulated raw wheel/Q-Link streams, reversals, fine steps, echoes, edges and old automation PASS; max bass step %.4f Hz\n",largest);
    e->dispatch(e,1,0,0,NULL,0);return 0;
}
