#include "pd_triangle_facing.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
static uint32_t random_state=0x12345678;
static float sample(){random_state=random_state*1664525+1013904223;return (int32_t(random_state)>>8)/65536.f;}
int main(){
 assert(native_triangle_facing(0,1,0,1,0,1,-1,0,1)>0);
 assert(native_triangle_facing(0,1,0,-1,0,1,1,0,1)<0);
 assert(native_triangle_facing(1,1,1,1,1,1,1,1,1)==0);
 for(unsigned i=0;i<100000;i++){
  float a[9];for(float &v:a)v=sample();
  if(i%7==0)a[2]=0; // Projected divisions previously produced Inf/NaN here.
  long double expected=(long double)a[0]*((long double)a[7]*a[5]-(long double)a[4]*a[8])+
   (long double)a[1]*((long double)a[3]*a[8]-(long double)a[6]*a[5])+
   (long double)a[2]*((long double)a[6]*a[4]-(long double)a[3]*a[7]);
  float actual=native_triangle_facing(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]);
  assert(std::isfinite(actual));assert((actual>0)==(expected>0) && (actual<0)==(expected<0));
 }
 puts("PASS homogeneous triangle facing: 100000 mixed/zero-W cases agree with long-double determinant; degeneracy preserved");
}
