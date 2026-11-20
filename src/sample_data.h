/* Bounded RIFF/WAVE and self-contained float32 assets. GPL-3.0-or-later.
 * Formats: Microsoft WAVEFORMATEX / WAVEFORMATEXTENSIBLE; no library dependency. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <ctype.h>
static uint16_t sample_u16(const unsigned char *p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t sample_u32(const unsigned char *p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void sample_free(SampleAsset *a){if(a){free(a->data);free(a->encoded);free(a);}}
static const char sample_b64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int sample_b64digit(unsigned char c){const char *p=c?strchr(sample_b64,c):NULL;return p?(int)(p-sample_b64):-1;}
static void sample_name(SampleAsset *a,const char *name) {
    const char *base=strrchr(name,'/');base=base?base+1:name;
    size_t i=0;for(;i<sizeof a->name-1 && base[i];i++)a->name[i]=(unsigned char)base[i]>=32 && (unsigned char)base[i]<127 && base[i]!='\\'?base[i]:'_';a->name[i]=0;
    if(!i)snprintf(a->name,sizeof a->name,"Sample.wav");
    static const char h[]="0123456789abcdef";
    for(i=0;a->name[i];i++){unsigned c=(unsigned char)a->name[i];a->hexname[i*2]=h[c>>4];a->hexname[i*2+1]=h[c&15];}a->hexname[i*2]=0;
}
static int sample_encode(SampleAsset *a) {
    size_t size=(size_t)a->count*4;a->encoded_len=(size+2)/3*4;
    a->encoded=malloc(a->encoded_len+1);if(!a->encoded)return 0;
    const unsigned char *bytes=(const unsigned char*)a->data;size_t i=0,n=0;
    while(i<size){size_t remain=size-i;uint32_t v=(uint32_t)bytes[i++]<<16;if(remain>1)v|=(uint32_t)bytes[i++]<<8;if(remain>2)v|=bytes[i++];
        a->encoded[n++]=sample_b64[v>>18];a->encoded[n++]=sample_b64[(v>>12)&63];a->encoded[n++]=remain>1?sample_b64[(v>>6)&63]:'=';a->encoded[n++]=remain>2?sample_b64[v&63]:'=';
    }a->encoded[n]=0;return 1;
}
static SampleAsset *sample_from_text(unsigned count,const char *hex,size_t hexlen,const char *encoded,size_t enclen) {
    if(count<2 || count>MAX_SAMPLES || hexlen<2 || hexlen>126 || hexlen%2 || enclen!=(((size_t)count*4+2)/3*4))return NULL;
    SampleAsset *a=calloc(1,sizeof *a);if(!a)return NULL;a->count=count;
    for(size_t i=0;i<hexlen;i+=2){int x=isdigit((unsigned char)hex[i])?hex[i]-'0':hex[i]>='a'&&hex[i]<='f'?hex[i]-'a'+10:-1;
        int y=isdigit((unsigned char)hex[i+1])?hex[i+1]-'0':hex[i+1]>='a'&&hex[i+1]<='f'?hex[i+1]-'a'+10:-1;
        if(x<0||y<0 || x*16+y<32 || x*16+y>126 || x*16+y=='/' || x*16+y=='\\')goto fail;
        a->name[i/2]=(char)(x*16+y);
    }
    memcpy(a->hexname,hex,hexlen);a->hexname[hexlen]=0;
    a->data=malloc((size_t)count*4);a->encoded=malloc(enclen+1);if(!a->data||!a->encoded)goto fail;
    memcpy(a->encoded,encoded,enclen);a->encoded[enclen]=0;a->encoded_len=enclen;
    size_t n=0,bytes=(size_t)count*4;unsigned char *out=(unsigned char*)a->data;
    for(size_t i=0;i<enclen;i+=4) {
        int v[4];for(int j=0;j<4;j++){unsigned char c=encoded[i+j];v[j]=c=='='?-2:sample_b64digit(c);if(v[j]==-1)goto fail;}
        if(v[0]<0||v[1]<0 || (i+4<enclen && (v[2]<0||v[3]<0)) || (v[2]==-2 && v[3]!=-2))goto fail;
        if(v[2]==-2 && (v[1]&15)){goto fail;}
        if(v[3]==-2 && v[2]>=0 && (v[2]&3))goto fail;
        uint32_t x=(uint32_t)v[0]<<18|(uint32_t)v[1]<<12|(uint32_t)(v[2]<0?0:v[2])<<6|(uint32_t)(v[3]<0?0:v[3]);
        if(n>=bytes){goto fail;}
        out[n++]=(unsigned char)(x>>16);
        if(v[2]>=0){if(n>=bytes){goto fail;}
        out[n++]=(unsigned char)(x>>8);}
        if(v[3]>=0){if(n>=bytes){goto fail;}
        out[n++]=(unsigned char)x;}
    }
    if(n!=bytes)goto fail;
    for(unsigned i=0;i<count;i++)if(!isfinite(a->data[i]) || fabsf(a->data[i])>1)goto fail;
    return a;
fail:sample_free(a);return NULL;
}
static float sample_decode_value(const unsigned char *p,int format,int bits) {
    if(format==3){if(bits==32){uint32_t u=sample_u32(p);float f;memcpy(&f,&u,4);return f;}
        uint64_t u=(uint64_t)sample_u32(p)|(uint64_t)sample_u32(p+4)<<32;double f;memcpy(&f,&u,8);return (float)f;}
    if(bits==8)return ((int)p[0]-128)/128.f;
    if(bits==16)return (int16_t)sample_u16(p)/32768.f;
    if(bits==24){int32_t v=(int32_t)(p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16);if(v&0x800000)v|=(int32_t)0xff000000;return v/8388608.f;}
    return (int32_t)sample_u32(p)/2147483648.f;
}
static SampleAsset *sample_read_wave(const char *path,char *status,size_t statuslen) {
    SampleAsset *a=NULL;FILE *f=NULL;float *mono=NULL;unsigned char *raw=NULL;
#define WAVE_FAIL(message) do{snprintf(status,statuslen,"%s",message);goto done;}while(0)
    f=fopen(path,"rb");if(!f)WAVE_FAIL("CANNOT OPEN FILE");
    if(fseeko(f,0,SEEK_END)){WAVE_FAIL("CANNOT READ FILE");}
    off_t actual=ftello(f);if(actual<12)WAVE_FAIL("NOT A WAV FILE");rewind(f);
    unsigned char head[40];if(fread(head,1,12,f)!=12 || memcmp(head,"RIFF",4) || memcmp(head+8,"WAVE",4))WAVE_FAIL("RIFF WAV REQUIRED");
    uint64_t end=(uint64_t)sample_u32(head+4)+8;if(end>(uint64_t)actual || end<12)WAVE_FAIL("TRUNCATED WAV");
    unsigned rate=0,channels=0,bits=0,align=0,format=0;uint64_t data_at=0,data_size=0,position=12;
    for(unsigned chunks=0;position+8<=end && chunks<65536;chunks++) {
        if(fseeko(f,(off_t)position,SEEK_SET) || fread(head,1,8,f)!=8)WAVE_FAIL("TRUNCATED WAV");
        unsigned char tag[4];memcpy(tag,head,4);uint32_t n=sample_u32(head+4);uint64_t next=position+8+n+(n&1u);
        if(next>end || next<=position)WAVE_FAIL("BAD WAV CHUNK");
        if(!memcmp(tag,"fmt ",4)) {
            if(rate || n<16){WAVE_FAIL("BAD WAV FORMAT");}
            size_t take=n<40?n:40;if(fread(head,1,take,f)!=take)WAVE_FAIL("TRUNCATED FORMAT");
            format=sample_u16(head);channels=sample_u16(head+2);rate=sample_u32(head+4);align=sample_u16(head+12);bits=sample_u16(head+14);
            if(format==0xfffe) {
                static const unsigned char guid[]={0,0,16,0,128,0,0,170,0,56,155,113};
                if(n<40 || sample_u16(head+16)<22 || sample_u16(head+18)<1 || sample_u16(head+18)>bits || memcmp(head+28,guid,12))WAVE_FAIL("UNSUPPORTED WAV");
                format=sample_u32(head+24);
            }
        }else if(!memcmp(tag,"data",4)) {if(data_at)WAVE_FAIL("MULTIPLE DATA CHUNKS");data_at=position+8;data_size=n;}
        position=next;if(rate && data_at)break;
    }
    if(!rate||!data_at || rate<8000 || rate>192000 || channels<1 || channels>8 ||
       !((format==1 && (bits==8||bits==16||bits==24||bits==32)) || (format==3 && (bits==32||bits==64))) || align!=channels*(bits/8) || !data_size || data_size%align)WAVE_FAIL("UNSUPPORTED WAV");
    uint64_t total=data_size/align;unsigned frames=(unsigned)(total>(uint64_t)rate*4?rate*4:total);
    unsigned count=(unsigned)((uint64_t)frames*RATE/rate);if(frames<2 || count<2)WAVE_FAIL("SAMPLE TOO SHORT");if(count>MAX_SAMPLES)count=MAX_SAMPLES;
    mono=malloc((size_t)frames*sizeof(float));raw=malloc((size_t)4096*align);if(!mono||!raw)WAVE_FAIL("NOT ENOUGH MEMORY");
    if(fseeko(f,(off_t)data_at,SEEK_SET))WAVE_FAIL("CANNOT READ AUDIO");
    for(unsigned i=0;i<frames;) {
        unsigned n=frames-i;if(n>4096)n=4096;if(fread(raw,align,n,f)!=n)WAVE_FAIL("TRUNCATED AUDIO");
        for(unsigned j=0;j<n;j++){float v=sample_decode_value(raw+(size_t)j*align,format,bits);if(!isfinite(v))WAVE_FAIL("INVALID FLOAT AUDIO");mono[i+j]=fminf(1,fmaxf(-1,v));}i+=n;
    }
    a=calloc(1,sizeof *a);if(!a)WAVE_FAIL("NOT ENOUGH MEMORY");a->count=count;a->data=malloc((size_t)count*4);if(!a->data)WAVE_FAIL("NOT ENOUGH MEMORY");
    for(unsigned i=0;i<count;i++){double x=(double)i*rate/RATE;unsigned j=(unsigned)x;float t=(float)(x-j);if(j>=frames-1)a->data[i]=mono[frames-1];else a->data[i]=mono[j]+(mono[j+1]-mono[j])*t;}
    sample_name(a,path);if(!sample_encode(a))WAVE_FAIL("NOT ENOUGH MEMORY");
    snprintf(status,statuslen,total>frames?"LOADED FIRST 4 SECONDS":channels>1?"LOADED MONO CHANNEL 1":"SAMPLE LOADED");
    fclose(f);free(mono);free(raw);return a;
done:if(f)fclose(f);free(mono);free(raw);sample_free(a);return NULL;
#undef WAVE_FAIL
}
