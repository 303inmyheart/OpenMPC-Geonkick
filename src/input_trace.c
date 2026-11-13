/* Temporary input diagnosis. No audio-side file I/O. MPC process only, /tmp only. */
#define _GNU_SOURCE
#include "params.h"
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#define CAPACITY 4096
static struct Event { atomic_uint_fast64_t seq,ns;atomic_int kind,index,tid;_Atomic float value; } events[CAPACITY];
static atomic_uint_fast64_t produced;
static atomic_int enabled,running;
static pthread_t writer;
static int started;
static uint64_t clock_ns(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint64_t)t.tv_sec*1000000000u+t.tv_nsec;}
void wrap_trace(int kind,int index,float value){
 if(!atomic_load_explicit(&enabled,memory_order_relaxed) || (kind!=0 && kind!=2) || index<0 || index>=NPARAMS)return;
 if(PARAMS[index].nudge_pct<=0){const char *key=PARAMS[index].key;size_t len=strlen(key);if(key[0]!='o' || len<6 || strcmp(key+len-5,"_freq"))return;}
 uint64_t n=atomic_fetch_add_explicit(&produced,1,memory_order_relaxed);struct Event *e=&events[n%CAPACITY];
 atomic_store_explicit(&e->seq,0,memory_order_release);
 atomic_store_explicit(&e->ns,clock_ns(),memory_order_relaxed);atomic_store_explicit(&e->kind,kind,memory_order_relaxed);
 atomic_store_explicit(&e->index,index,memory_order_relaxed);atomic_store_explicit(&e->tid,(int)syscall(SYS_gettid),memory_order_relaxed);
 atomic_store_explicit(&e->value,value,memory_order_relaxed);atomic_store_explicit(&e->seq,n+1,memory_order_release);
}
static void *write_events(void *unused){
 (void)unused;pthread_setname_np(pthread_self(),"gk-input-log");char path[160];snprintf(path,sizeof path,"/tmp/geonkick-input-010-%ld-%llu.tsv",(long)getpid(),(unsigned long long)clock_ns());
 FILE *f=fopen(path,"wx");if(!f)return NULL;
 fprintf(f,"ns\ttid\tkind\tindex\tkey\tnormalized\n");uint64_t next=0;size_t bytes=0;
 while(atomic_load(&running) && bytes<1024*1024){
  uint64_t head=atomic_load_explicit(&produced,memory_order_acquire);if(head-next>CAPACITY)next=head-CAPACITY;
  while(next<head && bytes<1024*1024){
   uint64_t ticket=next++;struct Event *e=&events[ticket%CAPACITY];if(atomic_load_explicit(&e->seq,memory_order_acquire)!=ticket+1)continue;
   uint64_t ns=atomic_load(&e->ns);int kind=atomic_load(&e->kind),index=atomic_load(&e->index),tid=atomic_load(&e->tid);float value=atomic_load(&e->value);
   if(atomic_load_explicit(&e->seq,memory_order_acquire)!=ticket+1)continue;
   if(index>=0 && index<NPARAMS){int n=fprintf(f,"%llu\t%d\t%d\t%d\t%s\t%.9g\n",(unsigned long long)ns,tid,kind,index,PARAMS[index].key,(double)value);if(n>0)bytes+=(size_t)n;}
  }
  fflush(f);usleep(50000);
 }
 fclose(f);atomic_store(&enabled,0);return NULL;
}
__attribute__((constructor)) static void start_trace(void){
 char exe[512];ssize_t n=readlink("/proc/self/exe",exe,sizeof exe-1);if(n<=0)return;exe[n]=0;const char *name=strrchr(exe,'/');if(!name || strcmp(name+1,"MPC"))return;
 atomic_store(&running,1);pthread_attr_t a;struct sched_param sp={0};pthread_attr_init(&a);pthread_attr_setinheritsched(&a,PTHREAD_EXPLICIT_SCHED);pthread_attr_setschedpolicy(&a,SCHED_OTHER);pthread_attr_setschedparam(&a,&sp);
 started=pthread_create(&writer,&a,write_events,NULL)==0;pthread_attr_destroy(&a);if(started)atomic_store(&enabled,1);
}
__attribute__((destructor)) static void stop_trace(void){atomic_store(&enabled,0);if(started){atomic_store(&running,0);pthread_join(writer,NULL);}}
