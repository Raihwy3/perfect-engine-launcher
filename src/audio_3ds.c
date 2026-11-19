/* Host PCM output. Keep SDK types separate from the N64 engine headers.
 * The engine supplies native-endian, interleaved stereo PCM16 at 22020 Hz. */
#include <3ds.h>
#include <stdint.h>
#include <string.h>
#include "pd_runtime.h"
#include "native_dsp_mix.h"
extern void sysLogPrintf(int,const char *,...);
extern void sysFatalError(const char *,...);
#define SLOTS 16
#define SLOT_BYTES 8192
#define CHANNELS (NATIVE_DSP_DRY_CHANNELS+1)
static ndspWaveBuf waves[CHANNELS][SLOTS];
static unsigned char pending[SLOTS];static unsigned consume;
static int dsp_mode;
static void dsp_callback(void *unused){
 (void)unused;
 for(unsigned n=0;n<SLOTS && __atomic_load_n(&pending[consume],__ATOMIC_ACQUIRE);n++){
  for(unsigned channel=0;channel<CHANNELS;channel++)ndspChnWaveBufAdd(channel,&waves[channel][consume]);
  __atomic_store_n(&pending[consume],0,__ATOMIC_RELEASE);consume=(consume+1)%SLOTS;
 }
}
static unsigned char *pcm,*silence;
static const void *next_buffer;
static uint32_t next_bytes;
static int ready;
static unsigned dropped,produce,underruns;
static int played;
int32_t audioInit(void) {
 if(ready)return 0;
 Result result=ndspInit();
 if(R_FAILED(result))sysFatalError("DSP audio init failed: %08lx\nCheck sd:/3ds/dspfirm.cdc",(unsigned long)result);
 dsp_mode=pd_option_get(15)!=0;native_dsp_mix_init(0);
 pcm=linearAlloc(SLOTS*SLOT_BYTES*(dsp_mode?CHANNELS:1)+SLOT_BYTES);
 if(dsp_mode && (!pcm || !native_dsp_mix_init(1))){if(pcm)linearFree(pcm);dsp_mode=0;pcm=linearAlloc(SLOTS*SLOT_BYTES+SLOT_BYTES);sysLogPrintf(1,"DSP voice mixer allocation failed; using original CPU mix");}
 if(!pcm){ndspExit();sysFatalError("Audio PCM allocation failed");}
 dropped=underruns=played=0;memset(waves,0,sizeof waves);memset(pending,0,sizeof pending);consume=produce=0;
 ndspSetOutputMode(NDSP_OUTPUT_STEREO);
 for(unsigned channel=0;channel<(dsp_mode?CHANNELS:1);channel++){
  ndspChnReset(channel);ndspChnSetFormat(channel,NDSP_FORMAT_STEREO_PCM16);
  ndspChnSetRate(channel,22020.f);ndspChnSetInterp(channel,NDSP_INTERP_POLYPHASE);
  float mix[12]={1.f,1.f};ndspChnSetMix(channel,mix);
 }
 if(dsp_mode)ndspSetCallback(dsp_callback,NULL);
 silence=pcm+SLOTS*SLOT_BYTES*(dsp_mode?CHANNELS:1);
 memset(silence,0,SLOT_BYTES);DSP_FlushDataCache(silence,SLOT_BYTES);
 ready=1;sysLogPrintf(0,"Dry voice mixing: %s (wet/reverb remains original CPU)",dsp_mode?"23 DSP channels":"CPU");sysLogPrintf(0,"DSP audio: engine stereo PCM16, 22020 Hz, owned queue buffers");return 0;
}
int32_t audioGetBytesBuffered(void) {
 if(!ready)return 0;
 uint32_t bytes=0,position=ndspChnGetSamplePos(0);
 for(unsigned i=0;i<SLOTS;i++)if(__atomic_load_n(&pending[i],__ATOMIC_ACQUIRE) || waves[0][i].status==NDSP_WBUF_QUEUED || waves[0][i].status==NDSP_WBUF_PLAYING){
  uint32_t samples=waves[0][i].nsamples;
  if(!__atomic_load_n(&pending[i],__ATOMIC_ACQUIRE) && waves[0][i].status==NDSP_WBUF_PLAYING)samples-=position<samples?position:samples;
  bytes+=samples*4;
 }
 return bytes;
}
int32_t audioGetSamplesBuffered(void){return audioGetBytesBuffered()/4;}
void audioSetNextBuffer(const int16_t *buffer,uint32_t bytes){next_buffer=buffer;next_bytes=bytes;}
static int slot_busy(unsigned slot){
 if(__atomic_load_n(&pending[slot],__ATOMIC_ACQUIRE))return 1;
 for(unsigned ch=0;ch<(dsp_mode?CHANNELS:1);ch++)if(waves[ch][slot].status==NDSP_WBUF_QUEUED || waves[ch][slot].status==NDSP_WBUF_PLAYING)return 1;
 return 0;
}
void audioEndFrame(void) {
 if(!ready || !next_buffer || !next_bytes)return;
 uint32_t bytes=next_bytes,original=bytes;const unsigned char *source=next_buffer;const void *key=next_buffer;
 next_buffer=NULL;next_bytes=0;
 if(bytes%4)sysFatalError("Unaligned engine audio buffer: %lu",(unsigned long)bytes);
 const struct NativeDspMixJob *job=dsp_mode?native_dsp_mix_peek(key,bytes/4):NULL;
 unsigned queued=audioGetSamplesBuffered();
 if(played && !queued)underruns++;
 played=1;
 if(queued>=8192){native_dsp_mix_release(key);return;}
 while(bytes){
  unsigned slot=produce;
  if(slot_busy(slot)){if(++dropped%60==1)sysLogPrintf(1,"Audio queue full: %u dropped buffers",dropped);break;}
  uint32_t chunk=bytes>SLOT_BYTES?SLOT_BYTES:bytes;
  for(unsigned ch=0;ch<(dsp_mode?CHANNELS:1);ch++){
   ndspWaveBuf *wave=&waves[ch][slot];memset(wave,0,sizeof *wave);
   void *dst=pcm+(slot*(dsp_mode?CHANNELS:1)+ch)*SLOT_BYTES;
   if(!ch)memcpy(dst,source,chunk);
   else if(job && (job->mask&(1u<<(ch-1))))memcpy(dst,job->pcm+(ch-1)*NATIVE_DSP_MIX_FRAMES*2+(original-bytes)/2,chunk);
   else dst=silence;
   if(dst!=silence)DSP_FlushDataCache(dst,chunk);
   wave->data_vaddr=dst;wave->nsamples=chunk/4;
  }
  if(dsp_mode)__atomic_store_n(&pending[slot],1,__ATOMIC_RELEASE);
  else ndspChnWaveBufAdd(0,&waves[0][slot]);
  source+=chunk;bytes-=chunk;produce=(produce+1)%SLOTS;
 }
 native_dsp_mix_release(key);
 static unsigned reports;
 if(++reports%300==0)sysLogPrintf(0,"Audio health: queued=%u frames underruns=%u dropped=%u",audioGetSamplesBuffered(),underruns,dropped);
}

void audioShutdown(void){
 if(!ready)return;
 if(dsp_mode)ndspSetCallback(NULL,NULL);
 for(unsigned ch=0;ch<(dsp_mode?CHANNELS:1);ch++)ndspChnWaveBufClear(ch);
 ndspExit();native_dsp_mix_init(0);linearFree(pcm);pcm=NULL;ready=0;next_buffer=NULL;next_bytes=0;
}
