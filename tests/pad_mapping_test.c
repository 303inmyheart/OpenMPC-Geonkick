/* Nonchromatic physical-pad mapping, all-or-nothing learn and migration. GPL-3.0-or-later. */
#include "params.h"
#include "engine.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static const mpc_engine_t *api;
static const int notes[]={48,50,52,53,55,57,59,36,60,62,64,65,67,69,71,72};
static void set(void *e,const char *k,float v){char b[48];snprintf(b,sizeof b,"%.9g",v);api->set_param(e,k,b);}
static float get(void *e,const char *k){char b[100];assert(api->get_param(e,k,b,sizeof b)>0);return strtof(b,NULL);}
static void message(void *e,int type,int n,int v){uint8_t m[]={type,n,v};api->midi(e,m,3);}
static void text(void *e,const char *k,char *b,int n){assert(api->get_param(e,k,b,n)>0);}
static void ready(void *e){float b[2];for(int k=0;k<6000;k++){api->render(e,b,0);if(!get(e,"kit_render_pending")&&!get(e,"render_pending"))return;usleep(2000);}assert(!"worker timeout");}
static void capture(void *e,int n,float *out){message(e,0xb0,120,0);message(e,0x90,n,110);for(int i=0;i<1024;i+=128)api->render(e,out+i*2,128);}
static void compare(float *a,float *b){for(int i=0;i<2048;i++)assert(isfinite(a[i]) && fabsf(a[i]-b[i])<1e-6f);}
int main(void) {
 api=mpc_engine();void *a=api->create(NULL),*b=api->create(NULL);assert(a&&b);
 float x[2048],y[2048];char status[100];char *saved=malloc(1048576),*again=malloc(1048576),*bad=malloc(1048576);assert(saved&&again&&bad);
 for(int p=0;p<16;p++){set(a,"kit_pad",p);set(a,"o1_sweep",0);set(a,"o1_freq",55+p*41);}
 set(a,"kit_mode",1);ready(a);text(a,"state",saved,1048576);api->set_param(b,"state",saved);set(b,"kit_mode",0);set(b,"kit_pad",0);ready(b);
 /* Reproduce the reported symptom with a constructed map: pad8 sends36. None of
  * these physical pads sends37, so the old sound2 is unreachable. */
 capture(a,notes[7],x);capture(b,60,y);compare(x,y);
 for(int p=0;p<16;p++)assert(notes[p]!=37);
 set(a,"kit_learn",1);text(a,"kit_learn_status",status,sizeof status);assert(!strcmp(status,"PRESS PAD 01 / 16"));
 set(a,"audition",1);set(a,"kit_learn",1);message(a,0x90,48,0);message(a,0xb0,123,0);
 text(a,"kit_learn_status",status,sizeof status);assert(!strcmp(status,"PRESS PAD 01 / 16"));
 unsigned refresh=get(a,"graph_revision");message(a,0x92,notes[0],100);assert(get(a,"graph_revision")!=refresh);message(a,0x92,notes[1],100);set(a,"kit_learn",0);
 text(a,"state",again,1048576);assert(!strcmp(strstr(saved," MAP1 "),strstr(again," MAP1 ")));assert(get(a,"kit_mapping")==0);
 /* Partial capture is not serialized, and repeated/zero-velocity notes do
  * not steal the next pad's slot. */
 set(a,"kit_learn",1);unsigned jobs=get(a,"synthesis_count");
 for(int p=0;p<16;p++) {
  message(a,0x90,notes[p],110);assert(get(a,"kit_pad")==p);
  if(p==1){
   message(a,0x90,notes[p],110);text(a,"kit_learn_status",status,sizeof status);assert(strstr(status,"SAME NOTE"));
   set(a,"kit_learn",1);message(a,0x90,notes[2],0);
  }
  if(p==4){text(a,"state",again,1048576);api->set_param(b,"state",again);assert(!get(b,"kit_learn") && !get(b,"kit_mapping"));}
 }
 assert(get(a,"synthesis_count")==jobs);assert(!get(a,"kit_learn") && get(a,"kit_mapping")==1);
 text(a,"kit_learn_status",status,sizeof status);assert(!strcmp(status,"16 PADS LEARNED"));
 text(a,"state",saved,1048576);assert(!strncmp(saved,"GKMP9 ",6));api->set_param(b,"state",saved);text(b,"state",again,1048576);assert(!strcmp(saved,again));ready(a);ready(b);
 /* Each actual physical note now reaches its intended pad, independent of
  * gaps, pitch ordering, and the chromatic start knob. */
 set(a,"kit_base",0);assert(get(a,"kit_mapping")==1); // base is irrelevant in Learned mode
 for(int p=0;p<16;p++) {
  set(b,"kit_mode",0);set(b,"kit_pad",p);ready(b);
  capture(a,notes[p],x);capture(b,60,y);compare(x,y);
 }
 capture(a,37,x);for(int i=0;i<2048;i++)assert(x[i]==0);
 /* Both maps survive switching and full restoration. Invalid/duplicate
  * assignments reject the whole saved state, preserving the working kit. */
 set(a,"kit_mapping",0);set(a,"kit_base",36);capture(a,36,x);set(b,"kit_pad",0);capture(b,60,y);compare(x,y);
 set(a,"kit_mapping",1);capture(a,notes[7],x);set(b,"kit_pad",7);capture(b,60,y);compare(x,y);
 text(a,"state",saved,1048576);api->set_param(b,"state",saved);
 for(int invalid=0;invalid<3;invalid++) {
  strcpy(bad,saved);char *map=strstr(bad," MAP1 ");assert(map);int n=snprintf(map,1048576-(map-bad)," MAP1 1");
  for(int p=0;p<16;p++)n+=snprintf(map+n,1048576-(map-bad)-n," %d",p==1?(invalid==0?notes[0]:invalid==1?128:-1):notes[p]);
  api->set_param(b,"state",bad);text(b,"state",again,1048576);assert(!strcmp(saved,again));
 }
 /*0.0.21 kits preserve their original base/note behavior without an invented
  * default hardware map. The new learned note table starts at that same base. */
 strcpy(bad,saved);memcpy(bad,"GKMP8",5);*strstr(bad," MAP1 ")=0;
 api->set_param(b,"state",bad);assert(!get(b,"kit_mapping") && !get(b,"kit_learn"));
 assert(get(b,"kit_base")==36);ready(b);capture(b,36,x);set(a,"kit_mapping",0);capture(a,36,y);compare(x,y);
 api->destroy(a);api->destroy(b);free(saved);free(again);free(bad);
 puts("Nonchromatic16-pad audio oracle; reported pad8->sound1 regression; complete/partial/cancel/repeated/duplicate learn; preview bypass; unique saved map; malformed-map atomicity; GKMP8 migration PASS");return 0;
}
