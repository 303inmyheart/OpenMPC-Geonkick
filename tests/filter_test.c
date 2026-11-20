/* Independent bilinear-response oracle, stability, state and live control checks. */
#include "params.h"
#include "engine.h"
#include "../src/param_defs.h"
#include "../src/dsp/filter.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static const mpc_engine_t *api;
static void set(void *e,const char *k,float v){char b[48];snprintf(b,sizeof b,"%.9g",v);api->set_param(e,k,b);}
static float get(void *e,const char *k){char b[96];assert(api->get_param(e,k,b,sizeof b)>0);return strtof(b,NULL);}
static void ready(void *e){float b[2];for(int i=0;i<4000;i++){api->render(e,b,0);if(!get(e,"render_pending"))return;usleep(2000);}assert(!"worker timeout");}
static void note(void *e){uint8_t stop[]={0xb0,120,0},on[]={0x90,60,127};api->midi(e,stop,3);api->midi(e,on,3);}
static void render(void *e,float *p,int n){for(int i=0;i<n;i+=128)api->render(e,p+2*i,n-i<128?n-i:128);}
int main(void){
 struct gkick_filter *f=NULL;assert(gkick_filter_new(&f,44100)==GEONKICK_OK);
 const double cutoffs[]={20,35,80,500,2000,8000,12000,18000,20000},qs[]={.5,.70710678,1,2,5,20};
 double max_error=0,max_peak=0;
 for(int type=0;type<3;type++)for(unsigned cf=0;cf<sizeof cutoffs/sizeof *cutoffs;cf++)for(unsigned qi=0;qi<sizeof qs/sizeof *qs;qi++){
  double fc=cutoffs[cf],q=qs[qi],g=tan(M_PI*fc/44100),k=1/q,den=1+k*g+g*g;
  double a1=2*(g*g-1)/den,a2=(1-k*g+g*g)/den;
  double b0=type==GEONKICK_FILTER_LOW_PASS?g*g/den:type==GEONKICK_FILTER_HIGH_PASS?1/den:k*g/den;
  double b1=type==GEONKICK_FILTER_LOW_PASS?2*b0:type==GEONKICK_FILTER_HIGH_PASS?-2*b0:0;
  double b2=type==GEONKICK_FILTER_BAND_PASS?-b0:b0,s1=0,s2=0;
  gkick_filter_set_type(f,type);gkick_filter_set_wide(f,1,q);gkick_filter_set_cutoff_freq(f,fc);gkick_filter_init(f);
  unsigned rng=7;
  for(int n=0;n<8192;n++){
   rng=1664525*rng+1013904223;float x=.1*sin(2*M_PI*fc*n/44100)+.01*((rng>>8)/8388608.0-1);
   double expected=b0*x+s1;s1=b1*x-a1*expected+s2;s2=b2*x-a2*expected;float actual;
   gkick_filter_val(f,x,&actual,.5f);assert(isfinite(actual));double err=fabs(actual-expected);max_error=fmax(max_error,err);max_peak=fmax(max_peak,fabs(actual));assert(err<2e-6);assert(fabs(actual)<4);
  }
 }
 printf("WIDE 162 cutoff/Q/type cases: independent bilinear oracle max error %.9g, peak %.6g\n",max_error,max_peak);
 /* CLASSIC reference retains the original arithmetic, including damping. */
 for(int type=0;type<3;type++)for(int i=0;i<3;i++){
  float fc=i==0?30:i==1?1000:8000,factor=i==0?1000:i==1?100:10,Q=10/factor,F=2*sin(M_PI*fc/44100),low=0,band=0;
  gkick_filter_set_type(f,type);gkick_filter_set_wide(f,0,1);gkick_filter_set_cutoff_freq(f,fc);gkick_filter_set_factor(f,factor);gkick_filter_init(f);
  for(int n=0;n<4096;n++){float x=.1f*sin(.1*n),h=x-low-Q*band;band=F*h+band;low=F*band+low;float expected=type==GEONKICK_FILTER_LOW_PASS?low:type==GEONKICK_FILTER_HIGH_PASS?h:band,actual;gkick_filter_val(f,x,&actual,.5f);assert(actual==expected);}
 }
 gkick_filter_free(&f);
 api=mpc_engine();void *a=api->create(NULL),*b=api->create(NULL);assert(a&&b);char key[64],state[65536],again[65536],legacy[65536];
 for(int j=0;j<10;j++){
  char prefix[24]="";if(j)snprintf(prefix,sizeof prefix,"o%d_",j);
  snprintf(key,sizeof key,"%sfilter_model",prefix);assert(get(a,key)==1);
  snprintf(key,sizeof key,"%scutoff_touch",prefix);set(a,key,0);snprintf(key,sizeof key,"%scutoff_wide",prefix);assert(fabsf(get(a,key)-20)<1e-4);
  snprintf(key,sizeof key,"%scutoff_touch",prefix);set(a,key,1);snprintf(key,sizeof key,"%scutoff_wide",prefix);assert(fabsf(get(a,key)-20000)<.02);
  snprintf(key,sizeof key,"%sres_touch",prefix);set(a,key,1);snprintf(key,sizeof key,"%sq_wide",prefix);assert(fabsf(get(a,key)-20)<1e-5);
  snprintf(key,sizeof key,"%sres_touch",prefix);set(a,key,0);snprintf(key,sizeof key,"%sq_wide",prefix);assert(fabsf(get(a,key)-.5)<1e-6);
  snprintf(key,sizeof key,"%scutoff",prefix);set(a,key,1234.567f);snprintf(key,sizeof key,"%scutoff_wide",prefix);assert(fabsf(get(a,key)-1234.567f)<1e-4);
  snprintf(key,sizeof key,"%sfilter_model",prefix);set(a,key,0);snprintf(key,sizeof key,"%scutoff_touch",prefix);set(a,key,1);snprintf(key,sizeof key,"%scutoff",prefix);assert(get(a,key)==8000);
  snprintf(key,sizeof key,"%scutoff_touch",prefix);set(a,key,0);snprintf(key,sizeof key,"%scutoff",prefix);assert(get(a,key)==30);
 }
 assert(api->get_param(a,"state",state,sizeof state)>0);assert(!strncmp(state,"GKMP6 ",6));api->set_param(b,"state",state);api->get_param(b,"state",again,sizeof again);assert(!strcmp(state,again));
 strcpy(legacy,state);memcpy(legacy,"GKMP5",5);char *end=legacy+6;for(int j=0;j<GKMP5_GP_COUNT;j++)strtof(end,&end);*end=0;api->set_param(b,"state",legacy);
 for(int j=0;j<10;j++){char prefix[24]="";if(j)snprintf(prefix,sizeof prefix,"o%d_",j);snprintf(key,sizeof key,"%sfilter_model",prefix);assert(get(b,key)==0);snprintf(key,sizeof key,"%scutoff_wide",prefix);assert(get(b,key)==30);}
 /* A currently sounding cached hit does not change when a new filter renders. */
 set(a,"reset",1);set(b,"reset",1);set(a,"filter_on",1);set(b,"filter_on",1);ready(a);ready(b);float x[8192],y[8192];note(a);note(b);render(a,x,256);render(b,y,256);for(int j=0;j<512;j++)assert(x[j]==y[j]);
 set(a,"cutoff_touch",1);set(a,"res_touch",1);ready(a);render(a,x,4096);render(b,y,4096);for(int j=0;j<8192;j++){assert(isfinite(x[j])&&fabsf(x[j])<=.71);assert(x[j]==y[j]);}
 note(a);render(a,x,4096);float energy=0;for(int j=0;j<8192;j++){assert(isfinite(x[j])&&fabsf(x[j])<=.71);energy+=x[j]*x[j];}assert(energy>.01);
 api->destroy(a);api->destroy(b);puts("CLASSIC bit-identical; 10 independent WIDE paths, fine/touch bounds, old GKMP5 -> CLASSIC, GKMP6 roundtrip and held-hit continuity PASS");return 0;
}
