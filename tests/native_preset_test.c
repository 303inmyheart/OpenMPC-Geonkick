/* Native factory banks and complete user snapshots through the real VST API. */
#include "../tools/preset_host.h"
static const char *names[9]={"0000-Init","0001-Deep Kick","0002-Tight Kick","0003-Snare","0004-Closed Hat","0005-Open Hat","0006-Metal","0007-Tom","1000-Starter Kit (16 Pads)"};
static void midi(PresetEffect *e,int status,int note,int velocity){
 struct {int32_t type,size,frame,flags,len,offset;unsigned char msg[4];char pad[4];} m={1,32,0,0,0,0,{status,note,velocity,0},{0}};
 struct {int32_t n;intptr_t r;void *events[2];} ev={1,0,{&m,NULL}};e->dispatch(e,25,0,0,&ev,0);
}
static void audio(PresetEffect *e,float *samples,int frames){for(int n=0;n<frames;n+=128){float l[128],r[128],*out[]={l,r};int count=frames-n<128?frames-n:128;e->render(e,NULL,out,count);for(int i=0;i<count;i++){assert(isfinite(l[i])&&l[i]==r[i]);samples[n+i]=l[i];}}}
static void signature(PresetEffect *e,float *data){char key[64];for(int c=0;c<432;c++)for(int ch=0;ch<2;ch++){snprintf(key,sizeof key,"viz_%c_%d",ch?'n':'p',c);data[c*2+ch]=preset_get(e,key);}}
static void match_state(PresetEffect *e,const unsigned char *expected,size_t length){size_t n;unsigned char *state=preset_chunk(e,&n);assert(n==length&&!memcmp(state,expected,n));free(state);}
static float physical(PresetEffect *e,const char *key){int i=preset_index(key);return PARAMS[i].nopts?preset_get(e,key)*(PARAMS[i].nopts-1):PARAMS[i].min+preset_get(e,key)*(PARAMS[i].max-PARAMS[i].min);}
int main(void){
 PresetEffect *e=preset_create();
 for(int p=0;p<8;p++){
  /* Loading a native preset is a complete-instance replacement, as in MPC. */
  preset_set(e,"kit_pad",7);e->dispatch(e,2,0,6,NULL,0);preset_set(e,"kit_mode",1);preset_set(e,"kit_output",.37f);preset_set(e,"o9_sample_on",1);
  size_t n;unsigned char *state=preset_file(names[p],&n);e->dispatch(e,24,0,n,state,0);match_state(e,state,n);
  assert(fabsf(preset_get(e,"preset")-p/7.f)<1e-6f);assert(preset_get(e,"kit_pad")==0&&preset_get(e,"kit_mode")==0&&preset_get(e,"kit_output")==1);
  assert(preset_get(e,"o9_sample_on")==0);
  if(!p)assert(preset_get(e,"filter_on")==0&&preset_get(e,"drive_on")==0);
  free(state);
 }
 size_t n;unsigned char *kit=preset_file(names[8],&n);e->dispatch(e,24,0,n,kit,0);match_state(e,kit,n);free(kit);
 assert(preset_get(e,"kit_mode")==1&&physical(e,"kit_base")==36);
 static const int programs[16]={1,2,3,4,5,6,7,7,1,3,4,5,6,6,7,0};
 static const float tunes[16]={0,0,0,0,0,0,0,7,-5,3,7,-5,-12,7,-5,12};
 for(int pad=0;pad<16;pad++){
  preset_set(e,"kit_pad",pad);assert(fabsf(physical(e,"preset")-programs[pad])<1e-5f);assert(fabsf(physical(e,"tune")-tunes[pad])<1e-5f);
  PresetEffect *ref=preset_create();float initial[864],expected[864],actual[864];signature(ref,initial);ref->dispatch(ref,2,0,programs[pad],NULL,0);preset_set(ref,"tune",tunes[pad]);
  int ready=programs[pad]==0;for(int k=0;k<4000&&!ready;k++){signature(ref,expected);ready=memcmp(initial,expected,sizeof initial)!=0;if(!ready)usleep(2000);}assert(ready);signature(ref,expected);
  ready=0;for(int k=0;k<4000&&!ready;k++){signature(e,actual);ready=!memcmp(actual,expected,sizeof expected);if(!ready)usleep(2000);}assert(ready);
  float got[1024],oracle[1024];midi(e,0xb0,120,0);midi(ref,0xb0,120,0);midi(e,0x90,36+pad,127);midi(ref,0x90,60,127);audio(e,got,1024);audio(ref,oracle,1024);
  float energy=0;for(int k=0;k<1024;k++){assert(fabsf(got[k]-oracle[k])<2e-6f);energy+=got[k]*got[k];}assert(energy>1e-5f);preset_close(ref);
 }
 /* Host Save serializes the complete kit, including learned layout and GUI focus. */
 static const int notes[16]={48,50,52,53,55,57,59,36,60,62,64,65,67,69,71,72};
 preset_set(e,"kit_learn",1);for(int i=0;i<16;i++){float block[128];midi(e,0x90,notes[i],100);audio(e,block,128);}assert(preset_get(e,"kit_mapping")==1);
 preset_set(e,"kit_pad",14);preset_set(e,"o9_freq",999);preset_set(e,"kit_output",.63f);
 size_t saved_size;unsigned char *saved=preset_chunk(e,&saved_size);preset_close(e);e=preset_create();e->dispatch(e,24,0,saved_size,saved,0);match_state(e,saved,saved_size);
 assert(preset_get(e,"kit_mapping")==1&&fabsf(physical(e,"kit_pad")-14)<1e-6f&&fabsf(physical(e,"o9_freq")-999)<.01f);free(saved);preset_close(e);
 puts("Native8 MPC starter presets +16-pad kit: true Init/full-state replacement, all16 sounds match independent legacy-program audio oracles, learned-map/user-save full close/reopen PASS");return 0;
}
