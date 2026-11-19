#pragma once
#include <stdint.h>
/* Source rectangle is in the physical 400x240 top LCD. Clip before reading;
 * nearest sampling preserves the original font pixels without a blur pass. */
static inline int pd_menu_frame_region(int out[4],const int bounds[4]){
 out[0]=bounds[0]<0?0:bounds[0]>400?400:bounds[0];
 out[1]=bounds[1]<0?0:bounds[1]>240?240:bounds[1];
 out[2]=bounds[2]<0?0:bounds[2]>400?400:bounds[2];
 out[3]=bounds[3]<0?0:bounds[3]>240?240:bounds[3];
 return out[2]>out[0] && out[3]>out[1];
}
static inline void pd_menu_frame_copy(uint16_t *lcd,const uint8_t *top,const int bounds[4]){
 int region[4];
 if(!pd_menu_frame_region(region,bounds)){for(int i=0;i<320*240;i++)lcd[i]=0;return;}
 int rows[240];
 for(int y=0;y<240;y++)rows[y]=239-(region[1]+y*(region[3]-region[1])/240);
 for(int x=0;x<320;x++){
  int sx=region[0]+x*(region[2]-region[0])/320;
  const uint8_t *source=top+sx*240*3;
  uint16_t *dest=lcd+x*240+239;
  for(int y=0;y<240;y++){
   const uint8_t *p=source+rows[y]*3;
   dest[-y]=(uint16_t)(((p[2]>>3)<<11)|((p[1]>>2)<<5)|(p[0]>>3));
  }
 }
}

/* A filtered render surface can be 256x512 rather than 240x400. Read the
 * visible LCD region using its actual row stride, without scaling the UI. */
static inline void pd_background_frame_copy(uint16_t *lcd,const uint8_t *source,unsigned stride,const int bounds[4]){
 int region[4];
 if(!pd_menu_frame_region(region,bounds)){for(int i=0;i<320*240;i++)lcd[i]=0;return;}
 int rows[240];for(int y=0;y<240;y++)rows[y]=239-(region[1]+y*(region[3]-region[1])/240);
 for(int x=0;x<320;x++){
  int sx=region[0]+x*(region[2]-region[0])/320;
  const uint8_t *column=source+sx*stride*3;
  for(int y=0;y<240;y++){
   const uint8_t *p=column+rows[y]*3;
   lcd[x*240+239-y]=(uint16_t)(((p[2]>>3)<<11)|((p[1]>>2)<<5)|(p[0]>>3));
  }
 }
}
