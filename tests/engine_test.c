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
    set(a,"layer3","0.32");set(a,"o9_wave","5");set(a,"o9_on","1");set(a,"o9_drive","7");set(a,"o9_attack","0.1");
    char state[8192],again[8192];assert(api->get_param(a,"state",state,sizeof state)>0);
    set(b,"state",state);assert(api->get_param(b,"state",again,sizeof again)>0);assert(!strcmp(state,again));
    /* A genuine 0.0.1-shaped chunk contains exactly the first 103 values.
     * Loading it preserves those and supplies defaults for every appended key. */
    char legacy[8192];strcpy(legacy,state);memcpy(legacy,"GKMP1",5);
    char *end=legacy+6;for(int i=0;i<103;i++)strtof(end,&end);*end=0;
    set(b,"state",legacy);assert(get(b,"o9_wave")==5);assert(get(b,"o9_drive")==2);assert(get(b,"o9_attack")==0);
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
    /* Each of the nine original oscillator effect paths is independently wired. */
    set(a,"reset","1");
    for(int osc=1;osc<=9;osc++) {
        char key[40];
        for(int j=1;j<=9;j++) { snprintf(key,sizeof key,"o%d_on",j);set(a,key,j==osc?"1":"0"); }
        for(int j=1;j<=3;j++) { snprintf(key,sizeof key,"layer%d",j);set(a,key,(osc-1)/3==j-1?"1":"0"); }
        wait_ready(a);note(a,0xb0,120,0);note(a,0x90,60,127);api->render(a,other,128);assert(energy(other,256)>0.001);
        snprintf(key,sizeof key,"o%d_filter_on",osc);set(a,key,"1");
        snprintf(key,sizeof key,"o%d_cutoff",osc);set(a,key,"200");
        wait_ready(a);note(a,0xb0,120,0);note(a,0x90,60,127);api->render(a,out,128);
        float delta=0;for(int i=0;i<256;i++)delta+=fabsf(out[i]-other[i]);assert(delta>0.001);
        snprintf(key,sizeof key,"o%d_drive_on",osc);set(a,key,"1");
        snprintf(key,sizeof key,"o%d_drive_out",osc);set(a,key,"0");
        wait_ready(a);note(a,0xb0,120,0);note(a,0x90,60,127);api->render(a,out,128);assert(energy(out,256)==0);
    }
    set(a,"reset","1");set(a,"o1_attack","0.1");wait_ready(a);
    note(a,0xb0,120,0);note(a,0x90,60,127);api->render(a,out,128);assert(energy(out,256)<0.02);
    set(a,"reset","1");wait_ready(a);
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
