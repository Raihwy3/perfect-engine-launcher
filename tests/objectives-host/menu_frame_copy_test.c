#include "pd_menu_frame_copy.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
int main(void){
 uint8_t *allocation=malloc(400*240*3+2),*top=allocation+1;
 uint16_t *output=malloc((320*240+2)*2),*lcd=output+1;
 allocation[0]=allocation[400*240*3+1]=0xcd;output[0]=output[76801]=0xbeef;
 for(int x=0;x<400;x++)for(int row=0;row<240;row++){
  uint8_t *p=top+(x*240+row)*3;p[0]=row;p[1]=x;p[2]=x+row;
 }
 const int cases[][4]={{0,0,400,240},{30,70,290,170},{-10,-20,500,300},{0,0,0,0},{399,239,400,240},{500,300,600,400},{40,0,360,240}};
 for(unsigned i=0;i<sizeof cases/sizeof cases[0];i++){
  pd_menu_frame_copy(lcd,top,cases[i]);int region[4],valid=pd_menu_frame_region(region,cases[i]);
  for(int x=0;x<320;x++)for(int y=0;y<240;y++){
   uint16_t expected=0;
   if(valid){
    int sx=region[0]+x*(region[2]-region[0])/320;
    int sy=region[1]+y*(region[3]-region[1])/240;
    const uint8_t *p=top+(sx*240+239-sy)*3;
    expected=((p[2]>>3)<<11)|((p[1]>>2)<<5)|(p[0]>>3);
   }
   assert(lcd[x*240+239-y]==expected);
  }
 }
 assert(output[0]==0xbeef && output[76801]==0xbeef && allocation[0]==0xcd && allocation[400*240*3+1]==0xcd);
 for(unsigned stride=240;stride<=256;stride+=16){
  unsigned rows=stride==240?400:512;
  uint8_t *padded=malloc(stride*rows*3+2);padded[0]=padded[stride*rows*3+1]=0xad;
  uint8_t *data=padded+1;
  for(unsigned row=0;row<rows;row++)for(unsigned col=0;col<stride;col++){
   uint8_t *p=data+(row*stride+col)*3;p[0]=col;p[1]=row;p[2]=row+col;
  }
  const int areas[][4]={{40,0,360,240},{62,104,338,158}};
  for(unsigned area=0;area<2;area++){
   const int *sample_bounds=areas[area];
   pd_background_frame_copy(lcd,data,stride,sample_bounds);
   for(unsigned x=0;x<320;x++)for(unsigned y=0;y<240;y++){
    unsigned sx=sample_bounds[0]+x*(sample_bounds[2]-sample_bounds[0])/320;
    unsigned sy=sample_bounds[1]+y*(sample_bounds[3]-sample_bounds[1])/240;
    const uint8_t *p=data+(sx*stride+239-sy)*3;
    assert(lcd[x*240+239-y]==(uint16_t)(((p[2]>>3)<<11)|((p[1]>>2)<<5)|(p[0]>>3)));
   }
  }
  assert(padded[0]==0xad && padded[stride*rows*3+1]==0xad);
  assert(output[0]==0xbeef && output[76801]==0xbeef);free(padded);
 }
 puts("PASS normal and padded background surfaces: exact visible pixels, true source stride and canaries");
 free(allocation);free(output);puts("PASS fullscreen window scaling, BGR/RGB565 channels, LCD rotation, clipped/empty/single-pixel rectangles and canaries");
}
