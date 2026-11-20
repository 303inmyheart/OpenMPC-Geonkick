/* WAV conversion and asset validation, independent numerical reference. */
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define RATE 44100
#define MAX_SAMPLES (RATE*4)
#define GP_COUNT 1551
#include "../src/sample_types.h"
#include "../src/sample_data.h"
#include "sample_fixture.h"
int main(void) {
 char dir[]="/tmp/geonkick-wave-XXXXXX",path[256],status[100];assert(mkdtemp(dir));snprintf(path,sizeof path,"%s/generated.wav",dir);
 int formats[]={1,1,1,1,3,3,1,3},bits[]={8,16,24,32,32,64,24,32};
 for(int mode=0;mode<8;mode++) {
  fixture_wave(path,formats[mode],bits[mode],2,44100,1001,mode>=6,0);SampleAsset *a=sample_read_wave(path,status,sizeof status);assert(a&&a->count==1001);
  for(unsigned i=0;i<a->count;i++) {
   double v=fixture_value(i,0,1001);if(formats[mode]==1)v=bits[mode]==8?lround(v*127)/128.:llround(v*(bits[mode]==16?32768.:bits[mode]==24?8388608.:2147483648.))/(bits[mode]==16?32768.:bits[mode]==24?8388608.:2147483648.);
   assert(fabs(a->data[i]-v)<1.2e-7);
  }
  SampleAsset *b=sample_from_text(a->count,a->hexname,strlen(a->hexname),a->encoded,a->encoded_len);assert(b && !memcmp(a->data,b->data,a->count*4));sample_free(b);
  char saved=a->encoded[0];a->encoded[0]='?';assert(!sample_from_text(a->count,a->hexname,strlen(a->hexname),a->encoded,a->encoded_len));a->encoded[0]=saved;
  assert(!sample_from_text(a->count+1,a->hexname,strlen(a->hexname),a->encoded,a->encoded_len));sample_free(a);
 }
 const int rates[]={8000,22050,48000,96000,192000};
 for(unsigned r=0;r<sizeof rates/sizeof *rates;r++) {
  int rate=rates[r];fixture_wave(path,3,64,1,rate,1000,0,1);SampleAsset *a=sample_read_wave(path,status,sizeof status);assert(a && a->count==(unsigned)(1000LL*44100/rate));
  for(unsigned i=0;i<a->count;i++){double position=fmin(999.,(double)i*rate/44100);double expected=position/1000*1.5-.75;assert(fabs(a->data[i]-expected)<1.5e-7);}sample_free(a);
 }
 fixture_wave(path,1,16,1,44100,44100*5,0,0);SampleAsset *a=sample_read_wave(path,status,sizeof status);assert(a&&a->count==MAX_SAMPLES&&strstr(status,"FIRST 4"));sample_free(a);
 fixture_wave(path,3,32,1,44100,100,0,0);FILE *f=fopen(path,"r+b");assert(f);fseek(f,44,SEEK_SET);fixture_u32(f,0x7fc00000);fclose(f);assert(!sample_read_wave(path,status,sizeof status));
 fixture_wave(path,1,16,1,44100,100,0,0);f=fopen(path,"r+b");assert(f);fseek(f,32,SEEK_SET);fixture_u16(f,7);fclose(f);assert(!sample_read_wave(path,status,sizeof status));
 f=fopen(path,"wb");assert(f);fwrite("RIFF",1,4,f);fixture_u32(f,9999);fwrite("WAVE",1,4,f);fclose(f);assert(!sample_read_wave(path,status,sizeof status));
 fixture_wave(path,1,16,1,44100,1,0,0);assert(!sample_read_wave(path,status,sizeof status));
 unlink(path);rmdir(dir);puts("WAV PCM8/16/24/32, float32/64, extensible, first-channel mono, five-rate analytic resampling, 4s cap, exact base64 roundtrip and corrupt-input rejection PASS");return 0;
}
