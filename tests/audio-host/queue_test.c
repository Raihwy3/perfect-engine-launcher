#include "3ds.h"
#include "native_dsp_mix.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
static int fail_linear,fail_sidecar;
void *__real_calloc(size_t,size_t);
void *__wrap_calloc(size_t count,size_t size){if(fail_sidecar){fail_sidecar=0;return NULL;}return __real_calloc(count,size);}
static unsigned flush_count;
static uint32_t play_position;
uint32_t ndspChnGetSamplePos(int ch){assert(ch==0);return play_position;}
static ndspWaveBuf *queued[128];static unsigned count,inits,exits;
static int requested_mode;static void (*callback)(void*);
int pd_option_get(int key){return key==15?requested_mode:0;}
void ndspSetCallback(void (*fn)(void*),void *arg){(void)arg;callback=fn;}
Result ndspInit(void){inits++;return 0;}void ndspExit(void){exits++;}
void *linearAlloc(size_t n){if(fail_linear){fail_linear=0;return NULL;}return malloc(n);}void linearFree(void*p){free(p);}
void ndspSetOutputMode(int x){assert(x==1);}void ndspChnReset(int c){assert(c>=0 && c<24);}
void ndspChnSetFormat(int c,int f){assert(c>=0 && c<24 && f==6);}void ndspChnSetRate(int c,float r){assert(c>=0 && c<24 && r==22020.f);}
void ndspChnSetInterp(int c,int i){(void)c;(void)i;}
void ndspChnSetMix(int c,const float*m){assert(c>=0 && c<24 && m[0]==1 && m[1]==1);}
void DSP_FlushDataCache(void*p,uint32_t n){assert(p && n);flush_count++;}
void ndspChnWaveBufClear(int c){assert(c>=0 && c<24);for(unsigned i=0;i<count;i++)queued[i]->status=NDSP_WBUF_DONE;}
void ndspChnWaveBufAdd(int c,ndspWaveBuf*w){assert(c>=0 && c<24);assert(w->status==0);assert(count<128);queued[count++]=w;w->status=NDSP_WBUF_QUEUED;}
void sysLogPrintf(int l,const char*f,...){(void)l;(void)f;}
void sysFatalError(const char*f,...){(void)f;abort();}
int32_t audioInit(void);void audioShutdown(void);int32_t audioGetBytesBuffered(void);int32_t audioGetSamplesBuffered(void);void audioSetNextBuffer(const int16_t*,uint32_t);void audioEndFrame(void);
int main(int argc,char **argv){
 if(argc>1){
  requested_mode=1;fail_linear=!strcmp(argv[1],"--fail-linear");fail_sidecar=!strcmp(argv[1],"--fail-sidecar");
  assert(fail_linear || fail_sidecar);
  assert(audioInit()==0 && !callback && inits==1);
  int16_t source[368];memset(source,0,sizeof source);
  audioSetNextBuffer(source,sizeof source);audioEndFrame();assert(count==1 && audioGetSamplesBuffered()==184);
  audioShutdown();assert(exits==1);
  puts("PASS: DSP allocation failure falls back to CPU without overrunning the PCM/silence allocation");return 0;
 }

 assert(audioInit()==0 && audioInit()==0 && inits==1);
 int16_t a[6000];for(unsigned i=0;i<6000;i++)a[i]=i;
 audioSetNextBuffer(a,sizeof a);audioEndFrame();assert(count==2 && queued[0]->nsamples==2048 && queued[1]->nsamples==952);
 assert(audioGetBytesBuffered()==12000 && audioGetSamplesBuffered()==3000);
 queued[0]->status=NDSP_WBUF_PLAYING;play_position=1000;assert(audioGetSamplesBuffered()==2000);
 play_position=99999;assert(audioGetSamplesBuffered()==952);
 play_position=0;queued[0]->status=NDSP_WBUF_QUEUED;
 assert(!memcmp(queued[0]->data_vaddr,a,8192));assert(!memcmp(queued[1]->data_vaddr,(char*)a+8192,3808));
 memset(a,0,sizeof a);assert(((const int16_t*)queued[0]->data_vaddr)[100]==100);
 audioSetNextBuffer(a,400);audioEndFrame();assert(count==3 && queued[2]->data_vaddr!=queued[0]->data_vaddr);
 queued[0]->status=NDSP_WBUF_DONE;assert(audioGetBytesBuffered()==4208);
 audioSetNextBuffer(a,400);audioEndFrame();assert(queued[3]!=queued[0]);
 audioShutdown();audioShutdown();assert(exits==1 && audioGetBytesBuffered()==0);
 requested_mode=1;assert(audioInit()==0 && callback);memset(a,0,sizeof a);
 native_dsp_mix_begin(a,184);int16_t *dry=native_dsp_mix_target(1,0,184);assert(dry);for(int i=0;i<184;i++){dry[i*2]=1000;dry[i*2+1]=2000;}native_dsp_mix_end();
 unsigned before=count,flush_before=flush_count;audioSetNextBuffer(a,184*4);audioEndFrame();assert(count==before && audioGetBytesBuffered()==184*4);assert(flush_count==flush_before+2);
 callback(NULL);assert(count==before+24);assert(((const int16_t*)queued[before+1]->data_vaddr)[0]==1000);assert(((const int16_t*)queued[before+1]->data_vaddr)[1]==2000);
 assert(((const int16_t*)queued[before+2]->data_vaddr)[0]==0);assert(native_dsp_mix_peek(a,184)==NULL);
 audioShutdown();assert(exits==2 && callback==NULL);
 puts("PASS: stereo format/rate, byte/sample accounting, split buffers, owned PCM, safe slot reuse, synchronized 24-channel DSP submission, lifecycle");
}
