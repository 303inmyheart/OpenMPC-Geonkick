/* Export authentic complete native states, not guessed parameter arrays. GPL-3.0-or-later. */
#include "preset_host.h"
static void save(PresetEffect *e,const char *stem){
 const char *dir=getenv("GK_NATIVE_PRESETS");assert(dir&&*dir);char path[4096];assert(snprintf(path,sizeof path,"%s/%s.chunk",dir,stem)<(int)sizeof path);
 size_t size;unsigned char *p=preset_chunk(e,&size);assert(!memcmp(p,"GKMP9 ",6));FILE *f=fopen(path,"wb");assert(f&&fwrite(p,1,size,f)==size);assert(!fclose(f));free(p);
}
int main(void){
 for(int program=0;program<8;program++){
  PresetEffect *e=preset_create();char name[64]={0},stem[96];assert(e->dispatch(e,29,program,0,name,0));
  assert(*name&&!strchr(name,'/')&&!strchr(name,'\\'));e->dispatch(e,2,0,program,NULL,0);assert(e->dispatch(e,3,0,0,NULL,0)==program);
  snprintf(stem,sizeof stem,"%04d-%s",program,name);save(e,stem);preset_close(e);
 }
 PresetEffect *kit=preset_create();
 static const int programs[16]={1,2,3,4,5,6,7,7,1,3,4,5,6,6,7,0};
 static const float tunes[16]={0,0,0,0,0,0,0,7,-5,3,7,-5,-12,7,-5,12};
 for(int pad=0;pad<16;pad++){
  preset_set(kit,"kit_pad",pad);kit->dispatch(kit,2,0,programs[pad],NULL,0);preset_set(kit,"tune",tunes[pad]);
 }
 preset_set(kit,"kit_mode",1);preset_set(kit,"kit_pad",0);save(kit,"1000-Starter Kit (16 Pads)");preset_close(kit);
 puts("Exported8 complete native sound presets +16-pad starter kit from actual VST chunks");return 0;
}
