/* Minimal VST2 host ABI for native preset export/tests; GPL-3.0-or-later. */
#include "params.h"
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
typedef struct PresetEffect PresetEffect;
typedef intptr_t(*PresetCallback)(PresetEffect*,int32_t,int32_t,intptr_t,void*,float);
struct PresetEffect {int32_t magic;intptr_t(*dispatch)(PresetEffect*,int32_t,int32_t,intptr_t,void*,float);void*p;void(*set)(PresetEffect*,int32_t,float);float(*get)(PresetEffect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void*obj,*user;int32_t uid,ver;void(*render)(PresetEffect*,float**,float**,int32_t);};
extern PresetEffect *VSTPluginMain(PresetCallback);
static intptr_t preset_host(PresetEffect*e,int32_t op,int32_t i,intptr_t v,void*p,float o){(void)e;(void)i;(void)v;(void)p;(void)o;return op==1?2400:0;}
static int preset_index(const char *key){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,key))return i;assert(!"unknown parameter");return -1;}
static void preset_set(PresetEffect *e,const char *key,float v){int i=preset_index(key);e->set(e,i,PARAMS[i].nopts?v/(PARAMS[i].nopts-1):(v-PARAMS[i].min)/(PARAMS[i].max-PARAMS[i].min));}
static float preset_get(PresetEffect *e,const char *key){return e->get(e,preset_index(key));}
static PresetEffect *preset_create(void){PresetEffect *e=VSTPluginMain(preset_host);assert(e&&e->magic==0x56737450&&e->ver==PLUG_VERSION);return e;}
static unsigned char *preset_chunk(PresetEffect *e,size_t *size){void *p=NULL;intptr_t n=e->dispatch(e,23,0,0,&p,0);assert(n>1&&n<=16777216&&p);unsigned char *copy=malloc((size_t)n);assert(copy);memcpy(copy,p,(size_t)n);*size=(size_t)n;return copy;}
static unsigned char *preset_file(const char *stem,size_t *size){const char *dir=getenv("GK_NATIVE_PRESETS");assert(dir&&*dir);char path[4096];assert(snprintf(path,sizeof path,"%s/%s.chunk",dir,stem)<(int)sizeof path);FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>1&&n<=16777216&&!fseek(f,0,SEEK_SET));unsigned char *p=malloc((size_t)n);assert(p&&fread(p,1,(size_t)n,f)==(size_t)n);fclose(f);assert(p[n-1]==0&&!memcmp(p,"GKMP9 ",6));*size=(size_t)n;return p;}
static void preset_close(PresetEffect *e){e->dispatch(e,1,0,0,NULL,0);}
