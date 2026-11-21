/* Native MPC program enumeration/selection/restore, using the actual VST ABI. */
#include "../tools/preset_host.h"
#include "factory_fixtures.h"
static unsigned char *fixture(int program,size_t*size){
 if(program==8)return preset_file("1000-Starter Kit (16 Pads)",size);
 char path[4096];snprintf(path,sizeof path,"%s/%s.chunk",getenv("GK_FACTORY_CHUNKS"),factory_fixtures[program-9].stem);
 FILE*f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);assert(n>0);unsigned char*p=malloc(n);assert(p&&fread(p,1,n,f)==(size_t)n);fclose(f);*size=(size_t)n;return p;
}
int main(void){
 PresetEffect *e=preset_create(),*ref=preset_create();assert(e->np==118&&e->npar==4096);
 char names[118][64];size_t before_size;unsigned char *before=preset_chunk(e,&before_size);
 for(int i=0;i<118;i++){
  memset(names[i],0,sizeof names[i]);assert(e->dispatch(e,29,i,-1,names[i],0)==1);assert(strlen(names[i])>0&&strlen(names[i])<=23);
  for(int j=0;j<i;j++)assert(strcmp(names[i],names[j]));
 }
 size_t n;unsigned char*same=preset_chunk(e,&n);assert(n==before_size&&!memcmp(before,same,n));free(same);free(before);
 for(int i=8;i<118;i++){
  e->dispatch(e,2,0,i,NULL,0);assert(e->dispatch(e,3,0,0,NULL,0)==i);
  size_t expected_size;unsigned char*expected=fixture(i,&expected_size);ref->dispatch(ref,24,0,expected_size,expected,0);free(expected);
  size_t actual_size;unsigned char *actual=preset_chunk(e,&actual_size);expected=preset_chunk(ref,&expected_size);
  /* Old starter fixtures have no program stamp. Everything before PRG1 must
   * match the independent original-state loading path byte for byte. */
  char *a=strstr((char*)actual," PRG1 "),*b=strstr((char*)expected," PRG1 ");assert(a);
  size_t length=(size_t)(a-(char*)actual),ref_length=b?(size_t)(b-(char*)expected):expected_size-1;
  assert(length==ref_length&&!memcmp(actual,expected,length));free(actual);free(expected);
  preset_set(e,"o1_freq",777);before=preset_chunk(e,&before_size);
  /* JUCE FBCh restore then indexed name scan/current-program re-selection. */
  ref->dispatch(ref,24,0,before_size,before,0);assert(ref->dispatch(ref,3,0,0,NULL,0)==i);
  for(int k=0;k<118;k++){char text[64]={0};assert(ref->dispatch(ref,29,k,-1,text,0));assert(!strcmp(text,names[k]));}
  ref->dispatch(ref,2,0,i,NULL,0);same=preset_chunk(ref,&n);assert(n==before_size&&!memcmp(before,same,n));free(before);free(same);
  if(i%20==0){printf("Native program bank%d/118: menu load equals original chunk, edited restore/name scans inert PASS\n",i+1);fflush(stdout);}
 }
 /* The original indices/options remain; Init can actually leave factory mode. */
 for(int i=0;i<8;i++){e->dispatch(e,2,0,i,NULL,0);assert(e->dispatch(e,3,0,0,NULL,0)==i);char text[64]={0};e->dispatch(e,29,i,-1,text,0);assert(!strcmp(text,PARAMS[preset_index("preset")].opts[i]));}
 before=preset_chunk(e,&before_size);e->dispatch(e,2,0,118,NULL,0);e->dispatch(e,2,0,-1,NULL,0);same=preset_chunk(e,&n);assert(n==before_size&&!memcmp(before,same,n));free(before);free(same);
 preset_close(e);preset_close(ref);puts("Actual VST:118 unique menu entries, all110 appended selections equal original states, JUCE-style restored edits/name scans, legacy8 identities and invalid-index isolation PASS");return 0;
}
