/* Optional actual-VST comparison: takes old0.0.18 and new library paths. */
#include "../src/param_defs.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
typedef struct Effect Effect;
typedef intptr_t (*Callback)(Effect*,int32_t,int32_t,intptr_t,void*,float);
struct Effect {int32_t magic;intptr_t(*dispatch)(Effect*,int32_t,int32_t,intptr_t,void*,float);void*p;void(*set)(Effect*,int32_t,float);float(*get)(Effect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void*obj,*user;int32_t uid,ver;void(*render)(Effect*,float**,float**,int32_t);};
typedef struct {int32_t type,size,frame,flags,len,offset;unsigned char m[4];char x[4];} ME;
typedef struct {int32_t n;intptr_t r;void *ev[2];} EV;
static intptr_t host(Effect*e,int32_t op,int32_t i,intptr_t v,void*p,float o){(void)e;(void)i;(void)v;(void)p;(void)o;return op==1?2400:0;}
static void put(float*p,const char*k,float v){for(int i=0;i<GP_COUNT;i++)if(!strcmp(gparams[i].key,k)){p[i]=v;return;}assert(0);}
static void block(Effect*e,float *out){float l[128],r[128],*buf[]={l,r};e->render(e,NULL,buf,128);if(out)for(int i=0;i<128;i++){out[i*2]=l[i];out[i*2+1]=r[i];}}
int main(int argc,char**argv){assert(argc==3||argc==4);int extended=argc==4 && !strcmp(argv[3],"GKMP6"),count=extended?GKMP6_GP_COUNT:GKMP5_GP_COUNT;void *libs[2];Effect *e[2];
 for(int j=0;j<2;j++){libs[j]=dlopen(argv[j+1],RTLD_NOW|RTLD_LOCAL);if(!libs[j]){puts(dlerror());return 1;}Effect*(*entry)(Callback)=dlsym(libs[j],"VSTPluginMain");assert(entry);e[j]=entry(host);assert(e[j]);}
 float p[GP_COUNT],out[2][16384];char text[65536],key[48];double maxerr=0;
 for(int target=0;target<10;target++)for(int type=0;type<3;type++){
  for(int i=0;i<GP_COUNT;i++)p[i]=gparams[i].def;
  int osc=target?target:1;for(int i=1;i<=9;i++){snprintf(key,sizeof key,"o%d_on",i);put(p,key,i==osc);}
  for(int i=1;i<=3;i++){snprintf(key,sizeof key,"layer%d",i);put(p,key,i==(osc-1)/3+1);}
  put(p,"length",.12f);snprintf(key,sizeof key,"o%d_wave",osc);put(p,key,1);snprintf(key,sizeof key,"o%d_freq",osc);put(p,key,120+37*target);
  snprintf(key,sizeof key,"o%d_sweep",osc);put(p,key,0);snprintf(key,sizeof key,"o%d_decay",osc);put(p,key,.09f);
  char prefix[24]="";if(target)snprintf(prefix,sizeof prefix,"o%d_",target);
  snprintf(key,sizeof key,"%sfilter_on",prefix);put(p,key,1);snprintf(key,sizeof key,"%sfilter_type",prefix);put(p,key,type);
  snprintf(key,sizeof key,"%scutoff",prefix);put(p,key,type==0?200:type==1?3000:8000);snprintf(key,sizeof key,"%sq",prefix);put(p,key,type==2?10:100);
  if(extended){snprintf(key,sizeof key,"%scutoff_wide",prefix);put(p,key,type==0?20:type==1?1000:20000);snprintf(key,sizeof key,"%sq_wide",prefix);put(p,key,type==0?.5f:type==1?1:20);}
  int n=snprintf(text,sizeof text,extended?"GKMP6":"GKMP5");for(int i=0;i<count;i++)n+=snprintf(text+n,sizeof text-n," %.9g",p[i]);
  for(int j=0;j<2;j++){e[j]->dispatch(e[j],24,0,n+1,text,0);}
  for(int k=0;k<100;k++){for(int j=0;j<2;j++)block(e[j],NULL);usleep(4000);}
  ME m={1,sizeof m,0,0,0,0,{0x90,60,127,0},{0}};EV event={1,0,{&m,0}};
  for(int j=0;j<2;j++){e[j]->dispatch(e[j],25,0,0,&event,0);for(int k=0;k<64;k++)block(e[j],out[j]+256*k);}
  double energy=0;for(int i=0;i<16384;i++){maxerr=fmax(maxerr,fabs(out[0][i]-out[1][i]));energy+=out[0][i]*out[0][i];assert(fabs(out[0][i]-out[1][i])<1e-6);}
  assert(energy>1e-6);
 }
 for(int j=0;j<2;j++){e[j]->dispatch(e[j],1,0,0,NULL,0);dlclose(libs[j]);}
 printf("Actual old/current VST,30 old-project filter fixtures (global+9osc x3types): max audio difference %.9g PASS\n",maxerr);return 0;
}
