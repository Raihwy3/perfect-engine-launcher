#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef int32_t Result;
typedef struct {const void *data_vaddr;uint32_t nsamples;uint8_t status;} ndspWaveBuf;
enum {NDSP_WBUF_FREE,NDSP_WBUF_QUEUED,NDSP_WBUF_PLAYING,NDSP_WBUF_DONE};
enum {NDSP_OUTPUT_STEREO=1,NDSP_FORMAT_STEREO_PCM16=6,NDSP_INTERP_POLYPHASE=0};
#define R_FAILED(r) ((r)<0)
Result ndspInit(void);void ndspExit(void);void *linearAlloc(size_t);void linearFree(void *);
void ndspSetOutputMode(int);void ndspChnReset(int);void ndspChnSetFormat(int,int);void ndspChnSetRate(int,float);void ndspChnSetInterp(int,int);void ndspChnSetMix(int,const float*);void ndspChnWaveBufAdd(int,ndspWaveBuf*);void ndspChnWaveBufClear(int);void DSP_FlushDataCache(void*,uint32_t);

void ndspSetCallback(void (*callback)(void *),void *data);

uint32_t ndspChnGetSamplePos(int);
