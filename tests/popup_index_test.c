/* Compare indexed popup behavior with a separate full-table reference. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define NPARAMS 31
static struct {int popup_of;} PARAMS[NPARAMS];
#ifndef TEST_FALLBACK
#ifdef TEST_EMPTY
#define PLUG_POPUP_COUNT 0
#else
#define PLUG_POPUP_COUNT 3
static const int PLUG_POPUP_INDICES[]={4,18,29};
#endif
#endif
#include "popup.h"
int main(void){
 for(int j=0;j<NPARAMS;j++)PARAMS[j].popup_of=-1;
#ifndef TEST_EMPTY
 PARAMS[4].popup_of=1;PARAMS[18].popup_of=7;PARAMS[29].popup_of=1;
#endif
 for(int picked=0;picked<NPARAMS;picked++)for(int mask=0;mask<8;mask++){
  float actual[NPARAMS],expected[NPARAMS];int hold[NPARAMS],ref[NPARAMS];
  for(int j=0;j<NPARAMS;j++){actual[j]=expected[j]=((j+mask)%3)?1:0;hold[j]=ref[j]=0;}
  for(int j=0;j<NPARAMS;j++)if(PARAMS[j].popup_of==picked && expected[j]>.5f){expected[j]=0;ref[j]=1;}
  popup_picked(actual,hold,picked);assert(!memcmp(actual,expected,sizeof actual)&&!memcmp(hold,ref,sizeof hold));
  for(int j=0;j<NPARAMS;j++)if(j!=picked&&PARAMS[j].popup_of>=0&&expected[j]>.5f){expected[j]=0;ref[j]=1;}
  popup_close_others(actual,hold,picked);assert(!memcmp(actual,expected,sizeof actual)&&!memcmp(hold,ref,sizeof hold));
 }
 puts("Popup index: duplicate targets, unrelated values, empty list and full-table fallback match reference PASS");return 0;
}
