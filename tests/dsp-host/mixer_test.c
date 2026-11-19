#include "../../upstream/perfect_dark/port/src/mixer.c"
#include <stdio.h>
#include "native_dsp_mix.h"
int main(void){
 ENVMIX_STATE baseline_state={0},dsp_state={0};
 aClearBufferImpl(0,sizeof rspa.buf);
 for(int i=0;i<NUM_SAMPLES;i++)BUF_S16(OFS_BASE)[i]=(i*53)%2000-1000;
 rspa.vol[0]=12000;rspa.vol[1]=10000;rspa.target[0]=12000;rspa.target[1]=10000;rspa.rate[0]=rspa.rate[1]=0;rspa.vol_dry=20000;rspa.vol_wet=5000;
 native_dsp_mix_init(0);aEnvMixerImpl(A_INIT,baseline_state,10000);
 int16_t left[NUM_SAMPLES],right[NUM_SAMPLES],wet[2][NUM_SAMPLES];
 memcpy(left,BUF_S16(OFS_MAIN_L),sizeof left);memcpy(right,BUF_S16(OFS_MAIN_R),sizeof right);memcpy(wet[0],BUF_S16(OFS_AUX_L),sizeof left);memcpy(wet[1],BUF_S16(OFS_AUX_R),sizeof right);
 aClearBufferImpl(OFS_MAIN_L,NUM_BYTES);aClearBufferImpl(OFS_MAIN_R,NUM_BYTES);aClearBufferImpl(OFS_AUX_L,NUM_BYTES);aClearBufferImpl(OFS_AUX_R,NUM_BYTES);
 assert(native_dsp_mix_init(1));native_dsp_mix_begin((void*)1,NUM_SAMPLES);aEnvMixerImpl(A_INIT,dsp_state,10000);native_dsp_mix_end();
 const struct NativeDspMixJob *job=native_dsp_mix_peek((void*)1,NUM_SAMPLES);assert(job && job->mask==1);
 for(int i=0;i<NUM_SAMPLES;i++){assert(job->pcm[i*2]==left[i] && job->pcm[i*2+1]==right[i]);assert(BUF_S16(OFS_MAIN_L)[i]==0 && BUF_S16(OFS_MAIN_R)[i]==0);assert(BUF_S16(OFS_AUX_L)[i]==wet[0][i] && BUF_S16(OFS_AUX_R)[i]==wet[1][i]);}
 assert(!memcmp(baseline_state,dsp_state,sizeof baseline_state));
 puts("PASS: DSP dry stems match CPU samples/stereo ordering; wet outputs and envelope state unchanged");
}
