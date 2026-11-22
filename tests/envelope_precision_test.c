/* Compare optimized interpolation with the frozen 0.0.34 implementation. */
#include "envelope.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
extern float reference_envelope_get_value(const struct gkick_envelope *,float);
static unsigned checks;
static void check(struct gkick_envelope *e,float x){
 float expected=reference_envelope_get_value(e,x),actual=gkick_envelope_get_value(e,x);
 if(!(isnan(expected)&&isnan(actual)) && memcmp(&expected,&actual,sizeof actual)){
  fprintf(stderr,"Envelope mismatch n=%zu x=%.9g expected=%a actual=%a\n",e->npoints,x,expected,actual);assert(0);
 }++checks;
}
int main(void){
 struct gkick_envelope e={0};struct gkick_envelope_point points[9]={0};
 unsigned seed=303;
 const float special[]={-INFINITY,-1,-0.0f,0,0x1p-149f,0x1p-53f,0x1p-52f,0x1p-51f,.5f,1,INFINITY,NAN};
 const float levels[]={-0.0f,0,.25f,1,-1,FLT_MAX,-FLT_MAX};
 for(int pass=0;pass<100;pass++){
  int count=pass<28?2:3+pass%7;e.npoints=count;e.first=points;e.last=points+count-1;
  for(int i=0;i<count;i++){
   seed=1664525u*seed+1013904223u;
   points[i]=(struct gkick_envelope_point){.prev=i?points+i-1:NULL,.next=i<count-1?points+i+1:NULL,
    .x=i/(float)(count-1),.y=pass<28?levels[pass%7]:(seed/4294967295.0f),.is_control=count>2 && i>0 && i<count-1 && pass%2 && i%2};
  }
  if(pass>=7 && pass<14){points[0].x=0;points[1].x=0x1p-100f;}
  if(pass>=14 && pass<21)points[1].x=points[0].x;
  if(pass>=21 && pass<28){points[0].x=-FLT_MAX;points[1].x=FLT_MAX;}
  for(unsigned i=0;i<sizeof special/sizeof *special;i++)check(&e,special[i]);
  for(int i=0;i<count;i++){check(&e,points[i].x);check(&e,nextafterf(points[i].x,-INFINITY));check(&e,nextafterf(points[i].x,INFINITY));}
  for(int i=0;i<4096;i++)check(&e,i/4095.f);
 }
 float table[]={0,.12345f,.9f,0};e.sample_table=table;e.sample_count=4;
 for(int i=0;i<4096;i++)check(&e,i/4095.f);
 e.sample_count=0;e.npoints=0;for(unsigned i=0;i<sizeof special/sizeof *special;i++)check(&e,special[i]);
 printf("Envelope precision: %u bit-exact reference comparisons, flat/linear/Bezier/table, knots, subnormals, degenerate spans, signed zero and nonfinite inputs PASS\n",checks);
 return 0;
}
