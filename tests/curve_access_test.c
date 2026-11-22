/* Sparse editor snapshots vs independent complete snapshots; key-based write oracle. */
#include "../src/engine.c"
#include <assert.h>
int main(void){
 Engine *e=calloc(1,sizeof *e);assert(e);e->values=e->pad_values[0];
 for(int i=0;i<GP_COUNT;i++)atomic_init(&e->values[i],gparams[i].def);
 float full[GP_COUNT];FreeCurve expected,actual,input;
 for(int o=0;o<9;o++)for(int k=0;k<2;k++)for(int n=0;n<=16;n++){
  if(n==1)continue;
  memset(&input,0,sizeof input);input.n=n;
  for(int j=0;j<16;j++){input.x[j]=n>=2?j/(float)(n-1):j/15.f;input.y[j]=(j%5)/4.f;if(j<15){input.b[j]=j%2?.7f:-.4f;input.d[j]=j%3?.25f:-.5f;}}
  free_write(e,o,k,&input);
  /* Independently find each saved field by its stable string identity. */
  for(int j=0;j<16;j++){
   char key[48];free_key(key,o,k,"n");assert(at(e,key)==n);
   snprintf(key,sizeof key,"f%d_%c_x%d",o+1,k?'p':'a',j);assert(at(e,key)==(j<n?input.x[j]:j/15.f));
   snprintf(key,sizeof key,"f%d_%c_y%d",o+1,k?'p':'a',j);assert(at(e,key)==(j<n?input.y[j]:0));
   if(j<15){snprintf(key,sizeof key,"f%d_%c_b%d",o+1,k?'p':'a',j);assert(at(e,key)==(j<n-1?input.b[j]:0));snprintf(key,sizeof key,"f%d_%c_c%d",o+1,k?'p':'a',j);assert(at(e,key)==(j<n-1?input.d[j]:0));}
  }
  for(int seen=0;seen<2;seen++){
   point_store(e,o,k,"seen",seen);
   for(int i=0;i<GP_COUNT;i++)full[i]=atomic_load(&e->values[i]);
   free_read(full,o,k,&expected);free_current(e,o,k,&actual);assert(!memcmp(&expected,&actual,sizeof actual));
  }
 }
 /* Numeric reads must round-trip through the established textual API. */
 unsigned seed=42;int checks=0;
 for(int pass=0;pass<12;pass++){
  for(int i=0;i<GP_COUNT;i++){
   seed=1664525u*seed+1013904223u;
   float v=gparams[i].lo+(gparams[i].hi-gparams[i].lo)*(seed/4294967295.0);
   if(gparams[i].discrete)v=roundf(v);
   atomic_store(&e->values[i],v);
  }
  for(int i=0;i<NPARAMS;i++)if(!PARAMS[i].string_display){
   float raw;char text[128];if(!mpc_engine_get_float(e,PARAMS[i].key,&raw))continue;
   assert(get_param(e,PARAMS[i].key,text,sizeof text)>0);
   assert(raw==strtof(text,NULL));checks++;
  }
 }
 printf("Numeric fast reads vs established text API: %d scalar/alias/filter checks PASS\n",checks);
 free(e);puts("All18 amplitude/pitch curves,0/2-16 knots, simple/five-point fallbacks: sparse snapshots match complete snapshots; stable-key write oracle PASS");return 0;
}
