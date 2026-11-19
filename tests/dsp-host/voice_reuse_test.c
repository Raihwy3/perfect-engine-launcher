#include "native_dsp_mix.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int main(void){
 assert(native_dsp_mix_init(1));int keys[2];
 native_dsp_mix_begin(&keys[0],8);int16_t *first=native_dsp_mix_target(1,0,8);assert(first);first[0]=1234;native_dsp_mix_end();
 const struct NativeDspMixJob *old=native_dsp_mix_peek(&keys[0],8);assert(old && old->pcm[0]==1234);
 for(unsigned block=0;block<100;block++){
  native_dsp_mix_begin(&keys[1],8);
  for(unsigned v=0;v<NATIVE_DSP_DRY_CHANNELS;v++){int16_t *p=native_dsp_mix_target(1000+block*24+v,0,8);assert(p);for(int i=0;i<16;i++)p[i]=(int16_t)(v+block);}
  assert(!native_dsp_mix_target(1000+block*24+23,0,8));native_dsp_mix_end();
  assert(old->pcm[0]==1234);const struct NativeDspMixJob *job=native_dsp_mix_peek(&keys[1],8);assert(job && job->mask==((1u<<NATIVE_DSP_DRY_CHANNELS)-1));
  native_dsp_mix_release(&keys[1]);
 }
 native_dsp_mix_release(&keys[0]);
 puts("PASS DSP voice reuse: 2300 changing identities remain offloaded, 24th concurrent voice falls back, earlier sealed PCM stays unchanged");
}
