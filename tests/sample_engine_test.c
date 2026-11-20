/* Sample import/persistence, lifetime and independent SOUND-tab envelope views. */
#include "params.h"
#include "engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "sample_fixture.h"
static const mpc_engine_t *api;
extern _Thread_local int sample_in_audio;
static void set(void *e,const char *k,const char *v){api->set_param(e,k,v);}
static void number(void *e,const char*k,float v){char b[40];snprintf(b,sizeof b,"%.9g",v);set(e,k,b);}
static float get(void *e,const char *k){char b[100];assert(api->get_param(e,k,b,sizeof b)>0);return strtof(b,NULL);}
static void ready(void *e){float out[2];for(int i=0;i<10000;i++){get(e,"display_rev");api->render(e,out,0);if(!get(e,"sample_pending")&&!get(e,"render_pending")&&!get(e,"kit_render_pending"))return;usleep(2000);}assert(!"sample worker timeout");}
static void note(void *e,int type,int n,int v){uint8_t m[]={type,n,v};sample_in_audio=1;api->midi(e,m,3);sample_in_audio=0;}
static void render(void *e,float *out,int n){for(int i=0;i<n;i+=128){sample_in_audio=1;api->render(e,out+i*2,n-i<128?n-i:128);sample_in_audio=0;}}
static void capture(void*e,float*out,int n){note(e,0xb0,120,0);note(e,0x90,60,127);render(e,out,n);}
static void state(void*e,char*out){assert(api->get_param(e,"state",out,16777216)>0);}
static float smooth(float t){t=fmaxf(0,fminf(1,t));return t*t*(3-2*t);}
int main(void) {
 api=mpc_engine();void*a=api->create(NULL),*b=api->create(NULL);assert(a&&b);float x[20000],y[20000];char dir[]="/tmp/geonkick-import-XXXXXX",path[300],text[128],key[64];assert(mkdtemp(dir));snprintf(path,sizeof path,"%s/import.wav",dir);fixture_wave(path,1,16,1,44100,4410,0,0);
 char *saved=malloc(16777216),*again=malloc(16777216),*bad=malloc(16777216);assert(saved&&again&&bad);
 /* Import while a tone is playing must not change that existing voice. */
 note(a,0x90,60,127);note(b,0x90,60,127);render(a,x,128);render(b,y,128);set(a,"sample_import_path",path);ready(a);
 render(a,x,1024);render(b,y,1024);assert(!memcmp(x,y,2048*sizeof(float)));assert(get(a,"o1_sample_on")==1 && get(a,"o1_source_view")==2);
 /* Raw4410-frame source, one-shot start at t>0, source end fade and cached
  * attack: analytical reference independent of the original Sample routine. */
 capture(a,x,6000);double error=0;
 for(int i=0;i<6000;i++){float source=i>0 && i<=4410?lroundf(fixture_value(i-1,0,4410)*32768)/32768.f:0;float tail=i>0?smooth((4410.f-i)/220):1;float expected=source*.7f*fminf(1,i/32.f)*tail;error=fmax(error,fabs(x[i*2]-expected));}
 printf("Imported sample independent audio oracle max error %.9g\n",error);assert(error<2e-6);assert(!get(a,"sample_pending"));
 /* Source pitch reuses the established5-cent fine controls; touch displays
  * semitone offset and octave playback halves the source duration. */
 number(a,"o1_freq",110);ready(a);capture(a,x,4000);for(int i=2300;i<4000;i++)assert(x[i*2]==0);api->get_param(a,"o1_freq_fine_display",text,sizeof text);assert(strstr(text,"12.00 st"));number(a,"o1_freq",55);ready(a);
 /* Samples are per oscillator and per pad, with no absolute file dependency. */
 for(int pad=0;pad<3;pad++){number(a,"kit_pad",pad);for(int osc=0;osc<9;osc+=4){number(a,"plot_osc",osc);set(a,"sample_import_path",path);ready(a);}}
 state(a,saved);assert(strstr(saved," SMP1 9 "));api->set_param(b,"state",saved);state(b,again);assert(!strcmp(saved,again));unlink(path);ready(b);
 number(a,"kit_pad",0);number(b,"kit_pad",0);number(a,"plot_osc",0);number(b,"plot_osc",0);capture(a,x,6000);capture(b,y,6000);for(int i=0;i<12000;i++)assert(fabsf(x[i]-y[i])<1e-6f);
 /* Bad assets reject the entire restore. File failure leaves current sample. */
 state(b,saved);strcpy(bad,saved);char *smp=strstr(bad," SMP1 ");assert(smp);strcat(bad," garbage");set(b,"state",bad);state(b,again);assert(!strcmp(saved,again));set(b,"sample_import_path",path);ready(b);state(b,again);assert(!strcmp(saved,again));
 strcpy(bad,saved);char *payload=strstr(bad," SMP1 ")+1;
 for(int field=0;field<7;field++){while(*payload && *payload!=' ')payload++;while(*payload==' ')payload++;}
 *payload='!'; /* invalid base64 after a completely valid physical kit/map */
 set(b,"state",bad);state(b,again);assert(!strcmp(saved,again));
 /* Native directory navigation: folders, .wav filtering, rows, paging and a
  * selected-file load. No filesystem operations occur in render/MIDI. */
 fixture_wave(path,3,32,1,48000,4800,0,0);set(a,"sample_directory",dir);ready(a);api->get_param(a,"sample_row0",text,sizeof text);assert(strstr(text,"import.wav"));set(a,"sample_pick0","1");set(a,"sample_load","1");ready(a);assert(get(a,"o1_sample_on")==1);
 /* More than one page, directory ordering, nested folder and unsupported
  * file filtering. These are file-browser behaviors, not layout snapshots. */
 char nested[300],files[10][300];snprintf(nested,sizeof nested,"%s/nested",dir);assert(!mkdir(nested,0700));
 for(int i=0;i<10;i++){snprintf(files[i],sizeof files[i],"%s/file%02d.wav",dir,i);fixture_wave(files[i],1,16,1,44100,128,0,0);}
 set(a,"sample_directory",dir);ready(a);api->get_param(a,"sample_row0",text,sizeof text);assert(strstr(text,"nested"));
 set(a,"sample_pick0","1");ready(a);api->get_param(a,"sample_status",text,sizeof text);assert(strstr(text,"NO WAV"));
 set(a,"sample_up","1");ready(a);set(a,"sample_next","1");ready(a);api->get_param(a,"sample_page",text,sizeof text);assert(!strcmp(text,"2 / 2"));
 set(a,"sample_prev","1");ready(a);api->get_param(a,"sample_page",text,sizeof text);assert(!strcmp(text,"1 / 2"));
 /* Default folder survives plugin recreation, remains independent of kit
  * state, and opening it returns there after navigating elsewhere. */
 void *preferences=api->create(dir);assert(preferences);set(preferences,"sample_directory",nested);ready(preferences);
 set(preferences,"sample_set_default","1");ready(preferences);api->get_param(preferences,"sample_status",text,sizeof text);assert(strstr(text,"DEFAULT FOLDER SAVED"));
 state(preferences,again);assert(!strstr(again,dir));api->destroy(preferences);preferences=api->create(dir);assert(preferences);
 set(preferences,"sample_directory",dir);ready(preferences);set(preferences,"sample_open","1");ready(preferences);api->get_param(preferences,"sample_folder",text,sizeof text);assert(!strcmp(text,"nested"));
 set(preferences,"state",again);set(preferences,"sample_open","1");ready(preferences);api->get_param(preferences,"sample_folder",text,sizeof text);assert(!strcmp(text,"nested"));api->destroy(preferences);
 char prefpath[300];snprintf(prefpath,sizeof prefpath,"%s/.sample-folder",dir);assert(!unlink(prefpath));
 for(int i=0;i<10;i++)unlink(files[i]);rmdir(nested);
 /* Fast reset cancels a queued load for this slot; clearing leaves OSC off. */
 sample_in_audio=1;set(a,"sample_import_path",path);set(a,"reset","1");sample_in_audio=0;ready(a);assert(get(a,"o1_sample_on")==0);set(a,"sample_import_path",path);ready(a);sample_in_audio=1;set(a,"sample_clear","1");sample_in_audio=0;ready(a);assert(get(a,"o1_sample_on")==0 && get(a,"o1_on")==0);
 /* Each SOUND tab binds its own layer's remembered oscillator and curve.
  * Reading another view does not synthesize or mutate the editor target. */
 set(a,"reset","1");number(a,"o2_decay",.05f);number(a,"o5_attack",.3f);number(a,"o9_decay",3);number(a,"length",.6f);ready(a);
 assert(get(a,"sound_l1_focus")==1 && get(a,"sound_l2_focus")==4 && get(a,"sound_l3_focus")==8);
 assert(get(a,"sound_l1_context")==2 && get(a,"sound_l2_context")==8 && get(a,"sound_l3_context")==16);
 assert(get(a,"viz_l1_e_431")==0 && get(a,"viz_l2_e_0")<.02f && get(a,"viz_l3_e_431")>.7f);
 /* Independent per-layer waveform peaks must follow the actual audible
  * layer. The isolated layer exactly matches the mixed waveform, while the
  * other two stay silent; shared master effects are disabled in Init. */
 set(b,"reset","1");number(b,"length",.1f);number(b,"o4_on",1);number(b,"o7_on",1);number(b,"o4_freq",83);number(b,"o7_freq",150);
 for(int active=1;active<=3;active++){
   for(int l=1;l<=3;l++){snprintf(key,sizeof key,"layer%d",l);number(b,key,l==active?1:0);}ready(b);
   int nonzero=0;for(int col=0;col<432;col++)for(int ch=0;ch<2;ch++){
     snprintf(key,sizeof key,"viz_%c_%d",ch?'n':'p',col);float mix=get(b,key);
     for(int l=1;l<=3;l++){snprintf(key,sizeof key,"viz_l%d_%c_%d",l,ch?'n':'p',col);float tap=get(b,key);assert(fabsf(tap-(l==active?mix:0))<1e-6f);if(l==active&&tap>0)nonzero++;}
   }assert(nonzero>100);
 }
 unsigned jobs=get(a,"synthesis_count");int target=get(a,"plot_osc");
 for(int l=1;l<=3;l++)for(int c=0;c<432;c++)for(int ch=0;ch<3;ch++){snprintf(key,sizeof key,"viz_l%d_%c_%d",l,"epn"[ch],c);assert(get(a,key)>=0 && get(a,key)<=1);}
 assert(get(a,"plot_osc")==target && get(a,"synthesis_count")==jobs);
 state(a,saved);assert(strstr(saved," VIEW1 "));set(b,"state",saved);state(b,again);assert(!strcmp(saved,again));assert(get(b,"sound_l1_focus")==1 && get(b,"sound_l2_focus")==4);
 number(a,"o3_sample_on",1);assert(get(a,"sound_l1_focus")==2 && get(a,"plot_osc")==2);number(a,"o3_sample_on",0);
 api->destroy(a);api->destroy(b);free(saved);free(again);free(bad);unlink(path);rmdir(dir);
 puts("Worker import, sample audio/pitch/end boundary, nine slot/pad assets, self-contained project after source removal, malformed/failed/stale loads, native file rows, clear/reset and three independent SOUND curve views PASS");return 0;
}
