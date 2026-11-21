/* Actual shared-library restoration of the largest original kit. GPL-3.0-or-later. */
#include "../tools/preset_host.h"
#include "factory_fixtures.h"
int main(void){
 PresetEffect *e=preset_create();
 for(int example=0;example<2;example++){
  int fixture=example?FACTORY_FIXTURE_COUNT-1:0;char file[4096];snprintf(file,sizeof file,"%s/%s.chunk",getenv("GK_FACTORY_CHUNKS"),factory_fixtures[fixture].stem);
  FILE *f=fopen(file,"rb");assert(f);fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);assert(n>0&&n<16777216);char *raw=malloc(n);assert(raw&&fread(raw,1,n,f)==(size_t)n);fclose(f);
  e->dispatch(e,24,0,n,raw,0);free(raw);size_t size;unsigned char *saved=preset_chunk(e,&size);assert(strstr((char*)saved," FCT1 "));
  assert(strstr((char*)saved,example?" MAP1 ":" SMP1 "));if(!example)assert(size>10000000);
  preset_close(e);e=preset_create();e->dispatch(e,24,0,size,saved,0);size_t again_size;unsigned char *again=preset_chunk(e,&again_size);
  assert(again_size==size&&!memcmp(saved,again,size));free(saved);free(again);
  /* Canonical control changes are saved together with the imported source ID. */
  preset_set(e,"o1_freq",123);saved=preset_chunk(e,&size);assert(strstr((char*)saved," FCT1 "));preset_close(e);e=preset_create();e->dispatch(e,24,0,size,saved,0);
  assert(fabsf(e->get(e,preset_index("o1_freq"))-(123-20.f)/(12000-20))<1e-6f);free(saved);
  printf("Actual factory VST fixture%d: %zu-byte state close/reopen and canonical edits PASS\n",fixture,size);
 }
 preset_close(e);return 0;
}
