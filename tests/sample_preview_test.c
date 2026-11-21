/* Preview is independent of the kit, bounded, cancellable and RT-safe. */
#include "params.h"
#include "engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sample_fixture.h"
extern _Thread_local int sample_in_audio;
static const mpc_engine_t *api;
static float get(void *e,const char*k){char b[128];assert(api->get_param(e,k,b,sizeof b)>0);return strtof(b,NULL);}
static void ready(void *e){for(int i=0;i<6000;i++){get(e,"display_rev");if(!get(e,"sample_pending"))return;usleep(2000);}assert(!"preview worker timeout");}
static void render(void *e,float *out,int n){sample_in_audio=1;api->render(e,out,n);sample_in_audio=0;}
int main(void){
 api=mpc_engine();void *e=api->create(NULL),*other=api->create(NULL);assert(e&&other);
 char dir[]="/tmp/geonkick-preview-XXXXXX",path[300],otherpath[300];assert(mkdtemp(dir));snprintf(path,sizeof path,"%s/a.wav",dir);snprintf(otherpath,sizeof otherpath,"%s/b.wav",dir);
 fixture_wave(path,1,16,1,44100,4410,0,0);fixture_wave(otherpath,1,16,1,44100,8820,0,0);
 char *before=malloc(16777216),*after=malloc(16777216);assert(before&&after);assert(api->get_param(e,"state",before,16777216)>0);
 api->set_param(e,"sample_directory",dir);ready(e);api->set_param(e,"sample_pick0","1");ready(e);assert(get(e,"sample_preview_active"));
 float out[20000],silent[20000];render(e,out,6000);render(other,silent,6000);double energy=0,maxstep=0;
 for(int i=0;i<6000;i++){assert(isfinite(out[i*2])&&out[i*2]==out[i*2+1]);assert(silent[i*2]==0);energy+=out[i*2]*out[i*2];if(i)maxstep=fmax(maxstep,fabsf(out[i*2]-out[(i-1)*2]));if(i>=4410)assert(out[i*2]==0);}
 assert(energy>1 && maxstep<.03);assert(!get(e,"sample_preview_active"));
 assert(api->get_param(e,"state",after,16777216)>0 && !strcmp(before,after));assert(!get(e,"o1_sample_on"));
 /* Stop before decode finishes must never start playback later. */
 api->set_param(e,"sample_preview_path",path);api->set_param(e,"sample_preview_stop","1");ready(e);render(e,out,512);for(int i=0;i<1024;i++)assert(out[i]==0);
 /* Replacing a request coalesces stale work; repeated tap stops the current file. */
 for(int i=0;i<7;i++)api->set_param(e,"sample_preview_path",i%2?path:otherpath);ready(e);render(e,out,128);assert(get(e,"sample_preview_active"));
 api->set_param(e,"sample_preview_stop","1");render(e,out,512);assert(out[1022]==0 && !get(e,"sample_preview_active"));
 api->set_param(e,"sample_pick1","1");ready(e);render(e,out,128);api->set_param(e,"sample_pick1","1");render(e,out,512);assert(out[1022]==0 && !get(e,"sample_preview_active"));
 /* Selecting/load still requires explicit commit, and source embeds normally. */
 api->set_param(e,"sample_pick0","1");ready(e);api->set_param(e,"sample_load","1");ready(e);assert(get(e,"o1_sample_on")==1 && !get(e,"sample_preview_active"));render(e,out,512);assert(out[1022]==0);
 /* Pending/completed assets close without leaks; RT guard wraps allocation/IO. */
 api->set_param(e,"sample_preview_path",path);ready(e);render(e,out,128);api->set_param(e,"sample_preview_path",otherpath);ready(e);
 api->destroy(e);api->destroy(other);free(before);free(after);assert(!unlink(path)&&!unlink(otherpath)&&!rmdir(dir));
 puts("Tap preview: audible isolated audio, unchanged kit/state, 5ms edges, stop/cancel, latest rapid request, repeat-tap stop, explicit Load and RT allocation/IO guard PASS");return 0;
}
