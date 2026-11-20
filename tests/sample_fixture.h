/* Deterministic generated WAV fixtures; no user/factory samples. */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
static void fixture_u16(FILE *f,unsigned n){fputc(n&255,f);fputc(n>>8&255,f);}
static void fixture_u32(FILE *f,uint32_t n){fixture_u16(f,n);fixture_u16(f,n>>16);}
static float fixture_value(unsigned n,unsigned channel,unsigned frames){return channel?-.9f:.7f*sinf(n*.071f)+.1f*(n/(float)(frames?frames:1));}
static void fixture_wave(const char *path,int format,int bits,int channels,int rate,unsigned frames,int extensible,int pattern) {
 FILE *f=fopen(path,"wb");assert(f);unsigned align=channels*(bits/8),bytes=frames*align,fmt=extensible?40:16;
 fwrite("RIFF",1,4,f);fixture_u32(f,4+8+fmt+8+bytes+(bytes&1));fwrite("WAVEfmt ",1,8,f);fixture_u32(f,fmt);
 fixture_u16(f,extensible?0xfffe:format);fixture_u16(f,channels);fixture_u32(f,rate);fixture_u32(f,rate*align);fixture_u16(f,align);fixture_u16(f,bits);
 if(extensible){fixture_u16(f,22);fixture_u16(f,bits);fixture_u32(f,0);fixture_u32(f,format);static const unsigned char guid[]={0,0,16,0,128,0,0,170,0,56,155,113};fwrite(guid,1,12,f);}
 fwrite("data",1,4,f);fixture_u32(f,bytes);
 for(unsigned i=0;i<frames;i++)for(int c=0;c<channels;c++) {
  double v=pattern?((double)i/(frames?frames:1)*1.5-.75):fixture_value(i,c,frames);
  if(c)v=-.9;
  if(format==3){if(bits==32){float q=(float)v;uint32_t u;memcpy(&u,&q,4);fixture_u32(f,u);}else{uint64_t u;memcpy(&u,&v,8);fixture_u32(f,u);fixture_u32(f,u>>32);}}
  else if(bits==8)fputc((int)lround(v*127)+128,f);
  else {int64_t q=llround(v*(bits==16?32768.:bits==24?8388608.:2147483648.));for(int b=0;b<bits/8;b++)fputc((unsigned long long)q>>(b*8)&255,f);}
 }
 if(bytes&1){fputc(0,f);}
 fclose(f);
}
