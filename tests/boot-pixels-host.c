#include "pd_boot_pixels.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
int main(void){
 for(unsigned width=320;width<=400;width+=80){
  size_t count=width*240;uint16_t *source=malloc(count*2),*rgb=malloc((count+2)*2);
  uint8_t *bgr=malloc(count*3+2);assert(source && rgb && bgr);
  for(unsigned y=0;y<240;y++)for(unsigned x=0;x<width;x++)source[y*width+x]=((x&31)<<11)|((y&63)<<5)|((x+y)&31);
  memset(bgr,0xa5,count*3+2);rgb[0]=rgb[count+1]=0xa55a;
  pd_boot_copy_bgr8(bgr+1,source,width);pd_boot_copy_rgb565(rgb+1,source,width);
  assert(bgr[0]==0xa5 && bgr[count*3+1]==0xa5 && rgb[0]==0xa55a && rgb[count+1]==0xa55a);
  for(unsigned y=0;y<240;y++)for(unsigned x=0;x<width;x++){
   unsigned i=x*240+239-y,c=source[y*width+x];
   assert(rgb[i+1]==c);assert(bgr[1+i*3]==(c&31)*255/31);
   assert(bgr[2+i*3]==((c>>5)&63)*255/63 && bgr[3+i*3]==(c>>11)*255/31);
  }
  free(source);free(rgb);free(bgr);
 }
 puts("PASS boot LCD formats: all 320/400x240 pixels, RGB channels, rotation and canaries");
}
