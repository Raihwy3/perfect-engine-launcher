#pragma once
#include <stdint.h>
/* Transpose into the LCD's rotated, column-major RGB565 layout in cache-sized
 * tiles. Both ends of each tile stay hot instead of striding a whole image. */
static inline void pd_bitmap_copy(uint16_t *lcd,const uint16_t *pixels){
 for(int ty=0;ty<240;ty+=8)for(int tx=0;tx<320;tx+=8)
  for(int x=tx;x<tx+8;x++){
   uint16_t *dst=lcd+x*240+239-ty;
   const uint16_t *src=pixels+ty*320+x;
   for(int y=0;y<8;y++)dst[-y]=src[y*320];
  }
}
