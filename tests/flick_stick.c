#include "native_flick.h"
#include "jsm_smoothing_reference.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 for(int hz=30;hz<=100;hz=hz==30?60:hz==60?100:101)for(int dir=-1;dir<=1;dir+=2){
  struct NativeFlick f={0};uint64_t t=1000000;native_flick_sample(&f,t,0,0,1);
  for(int i=0;i<20;i++){t+=1000000/hz;native_flick_sample(&f,t,dir*55,0,1);}
  assert(fabsf(f.pending-dir*90)<.01f);
  float before=f.pending;t+=16667;native_flick_sample(&f,t,0,0,1);
  t+=16667;native_flick_sample(&f,t,dir*55,0,1);assert(fabsf(f.pending-before)<.01f);
  native_flick_sample(&f,t+1,dir*55,0,0);assert(!f.active && f.pending==0);
 }
 for(int count=1;count<=64;count=count==1?3:count==3?6:count==6?64:65){
  struct NativeFlick f={0},ref={0};f.sample_count=count;
  for(int i=0;i<2000;i++){float v=sinf(i*.73f)*15;
   assert(fabsf(native_flick_smooth(&f,v)-jsm_reference_smooth(&ref,v,.08f*57.2957795131f,.16f*57.2957795131f,count))<.00001f);
  }
 }
 puts("PASS transferred flick: 30/60/100Hz, both directions, pressure dropout, disable reset and 8000 upstream smoothing comparisons");
}
