#pragma once
#include <stdint.h>
/* CPU startup canvases are RGB565; the top LCD stays in its normal BGR8 mode. */
static inline void pd_boot_copy_bgr8(uint8_t *lcd,const uint16_t *pixels,unsigned width){
 for(unsigned y=0;y<240;y++)for(unsigned x=0;x<width;x++){
  unsigned c=pixels[y*width+x],offset=(x*240+239-y)*3;
  lcd[offset]=(c&31)*255/31;
  lcd[offset+1]=((c>>5)&63)*255/63;
  lcd[offset+2]=(c>>11)*255/31;
 }
}
static inline void pd_boot_copy_rgb565(uint16_t *lcd,const uint16_t *pixels,unsigned width){
 for(unsigned y=0;y<240;y++)for(unsigned x=0;x<width;x++)lcd[x*240+239-y]=pixels[y*width+x];
}
