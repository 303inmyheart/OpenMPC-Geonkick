/* Host-cache regression: action release without any audio callbacks. */
#include "params.h"
#include <assert.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
typedef struct Effect Effect;
typedef intptr_t (*Callback)(Effect*,int32_t,int32_t,intptr_t,void*,float);
struct Effect {int32_t magic;intptr_t(*dispatch)(Effect*,int32_t,int32_t,intptr_t,void*,float);void*p;
 void(*set)(Effect*,int32_t,float);float(*get)(Effect*,int32_t);int32_t np,npar,ni,no,flags;intptr_t r1,r2;int32_t a,b,c;float io;void*obj,*user;int32_t uid,ver;void(*render)(Effect*,float**,float**,int32_t);};
extern Effect *VSTPluginMain(Callback);
static atomic_int watched,release_count,feedback_off_main;static pthread_t main_thread;
static intptr_t host(Effect*e,int32_t op,int32_t i,intptr_t v,void*p,float n){(void)v;(void)p;
 if(op==0 && i==atomic_load(&watched) && n==0){atomic_fetch_add(&release_count,1);if(!pthread_equal(pthread_self(),main_thread))atomic_store(&feedback_off_main,1);e->set(e,i,n);}
 return op==1?2400:0;
}
static int find(const char*k){for(int i=0;i<NPARAMS;i++)if(!strcmp(PARAMS[i].key,k))return i;assert(0);return -1;}
static void wait_release(void){for(int n=0;n<100 && !atomic_load(&release_count);n++)usleep(5000);assert(atomic_load(&release_count));}
int main(void){
 main_thread=pthread_self();atomic_store(&watched,-1);Effect*e=VSTPluginMain(host);assert(e);
 int select=find("free_select");e->set(e,find("o1_a_mode"),1);e->set(e,select,2/15.f);
 const char *keys[]={"free_prev","free_next","free_add","free_delete"};
 for(unsigned i=0;i<sizeof keys/sizeof *keys;i++){
  int action=find(keys[i]);atomic_store(&release_count,0);atomic_store(&watched,action);
  e->set(e,action,1);wait_release();assert(e->get(e,action)==0);usleep(210000);
 }
 /* Mouse-up can be a second high pulse after the first reset. It must
  * reset the host cache too, even when debounced before engine dispatch. */
 e->set(e,select,1/15.f);atomic_store(&watched,find("free_next"));atomic_store(&release_count,0);
 e->set(e,find("free_next"),1);wait_release();usleep(20000);atomic_store(&release_count,0);
 e->set(e,find("free_next"),1);wait_release();assert(fabsf(e->get(e,select)-2/15.f)<1e-7f);usleep(210000);
 /* A second isolated tap must act once, and the host zero echo must not act. */
 e->set(e,select,1/15.f);atomic_store(&watched,find("free_next"));atomic_store(&release_count,0);
 e->set(e,find("free_next"),1);wait_release();assert(fabsf(e->get(e,select)-2/15.f)<1e-7f);
 usleep(210000);atomic_store(&release_count,0);e->set(e,find("free_next"),1);wait_release();assert(fabsf(e->get(e,select)-3/15.f)<1e-7f);
 assert(atomic_load(&feedback_off_main));
 e->dispatch(e,1,0,0,NULL,0);puts("All four point buttons reset while stopped, repeated taps act once, zero echo inert, feedback off audio PASS");return 0;
}
