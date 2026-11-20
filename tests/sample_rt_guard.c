/* Link-wrap port calls only: worker/UI allocations are permitted; audio is not. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
_Thread_local int sample_in_audio;
void *__real_malloc(size_t);void *__real_calloc(size_t,size_t);void *__real_realloc(void*,size_t);void __real_free(void*);
FILE *__real_fopen(const char*,const char*);DIR *__real_opendir(const char*);
void *__wrap_malloc(size_t n){assert(!sample_in_audio);return __real_malloc(n);}
void *__wrap_calloc(size_t n,size_t s){assert(!sample_in_audio);return __real_calloc(n,s);}
void *__wrap_realloc(void*p,size_t n){assert(!sample_in_audio);return __real_realloc(p,n);}
void __wrap_free(void*p){assert(!sample_in_audio);__real_free(p);}
FILE *__wrap_fopen(const char*p,const char*m){assert(!sample_in_audio);return __real_fopen(p,m);}
DIR *__wrap_opendir(const char*p){assert(!sample_in_audio);return __real_opendir(p);}
