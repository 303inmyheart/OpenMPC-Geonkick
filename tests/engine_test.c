/* Independent signal oracle, asynchronous cache lifetime, state and isolation. */
#include "params.h"
#include "engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static const mpc_engine_t *api;
static void set(void *e,const char *k,const char *v) { api->set_param(e,k,v); }
static float get(void *e,const char *k) { char b[80];assert(api->get_param(e,k,b,sizeof b)>0);return strtof(b,NULL); }
static void wait_ready(void *e) {
    float b[2];
    for(int i=0;i<2500;i++) { api->render(e,b,0);if(!get(e,"render_pending"))return;usleep(2000); }
    assert(!"synthesis worker timeout");
}
static void note(void *e,int status,int n,int v) { uint8_t m[]={(uint8_t)status,(uint8_t)n,(uint8_t)v};api->midi(e,m,3); }
static float energy(const float *b,int n) { float s=0;for(int i=0;i<n;i++){assert(isfinite(b[i]));s+=b[i]*b[i];}return s; }
int main(void) {
    api=mpc_engine();void *a=api->create(NULL),*b=api->create(NULL);assert(a&&b);
    float out[256],other[256],phase=0,maxerr=0;
    note(a,0x90,60,127);
    int frames=(int)(0.6f*44100),pos=0;
    while(pos<frames) {
        int n=frames-pos;if(n>127)n=127;api->render(a,out,n);
        for(int i=0;i<n;i++,pos++) {
            float t=pos*(0.6f/frames);
            float amp=fmaxf(0,1-t/0.5f);
            float fade=fminf(1,fminf(pos/32.f,(frames-1-pos)/64.f));
            float expected=sinf(phase)*0.8f*amp*fade*0.7f;
            maxerr=fmaxf(maxerr,fabsf(out[i*2]-expected));
            assert(out[i*2]==out[i*2+1]);
            float freq=220.f*(t<0.04f?1-0.75f*t/0.04f:0.25f);
            phase+=(6.2831853f*freq)/44100;if(phase>6.2831853f)phase-=6.2831853f;
        }
    }
    printf("independent chirp/envelope oracle max error: %.9g\n",maxerr);assert(maxerr<0.0001f);
    api->render(a,out,128);assert(energy(out,256)==0);
    api->render(b,other,128);assert(energy(other,256)==0);
    /* Full-length ignores note off; Gate honors note and channel. */
    note(a,0x90,60,127);note(a,0x80,60,0);api->render(a,out,128);assert(energy(out,256)>0.01);
    note(a,0xb0,123,0);api->render(a,out,128);assert(energy(out,256)==0);
    set(a,"playback","2");note(a,0x91,62,127);note(a,0x80,62,0);api->render(a,out,128);assert(energy(out,256)>0.01);
    note(a,0x81,62,0);api->render(a,out,128);assert(energy(out,256)==0);
    /* Exact round trip includes edits across every layer, not only the selected page. */
    set(a,"layer3","0.32");set(a,"o9_wave","5");set(a,"o9_on","1");
    char state[8192],again[8192];assert(api->get_param(a,"state",state,sizeof state)>0);
    set(b,"state",state);assert(api->get_param(b,"state",again,sizeof again)>0);assert(!strcmp(state,again));
    set(b,"state","GKMP1 nan");assert(get(b,"layer3")==get(a,"layer3"));
    set(b,"o9_level","nan");assert(get(b,"o9_level")==0.8f);
    /* Presets all produce audio; no voices/worker/global state leak into another instance. */
    for(int k=0;k<8;k++) {
        if(k==0)set(a,"reset","1");
        char v[12];snprintf(v,sizeof v,"%d",k);set(a,"preset",v);wait_ready(a);
        note(a,0xb0,120,0);note(a,0x90,60,127);api->render(a,out,128);
        float en=energy(out,256);fprintf(stderr,"preset %d first-block energy %.9g\n",k,en);assert(en>0.00001);
    }
    assert(get(b,"o9_wave")==5);
    /* Old sounds retain their buffers while rapid edits and notes recycle other slots. */
    for(int k=0;k<80;k++) {
        char v[32];snprintf(v,sizeof v,"%g",45+(k%20)*10.0);set(a,"o1_freq",v);
        note(a,0x90,48+k%24,70+k%58);
        for(int j=0;j<3;j++) { api->render(a,out,128);energy(out,256); }
        usleep(2000);
    }
    wait_ready(a);note(a,0xb0,123,0);api->render(a,out,128);assert(energy(out,256)==0);
    api->destroy(a);api->destroy(b);
    puts("Geonkick behavioural tests PASSED");return 0;
}
