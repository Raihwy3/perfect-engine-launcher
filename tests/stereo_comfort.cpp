#include "native_stereo_options.h"
#include "native_weapon_stereo.h"
#include <cassert>
#include <cmath>
struct Row{float x,y,z,w;};struct Matrix{Row r[4];};
int main(){
 Matrix flat={{{0,1,0,0},{-1,0,0,0},{0,0,1,0},{0,0,0,1}}},eye;
 for(int strength=0;strength<=200;strength+=25)for(int step=0;step<=100;step++){
  float slider=native_stereo_options::slider(step*.01f,strength);
  float limit=native_stereo_options::shift_limit(slider);
  assert(limit>=0 && limit<=.02f);
  for(float convergence: {300.f,3000.f,6000.f})for(float distance: {1.f,4.f,20.f,100.f,3000.f,100000.f}){
   for(bool right:{false,true}){
    native_world_stereo(eye,flat,slider,right);
    float unbounded=eye.r[1].w+native_stereo_eye_translation(slider,right,false,convergence)/distance;
    float applied=std::clamp(unbounded,-limit,limit);
    assert(std::fabs(applied)*200.f<=4.00001f);
   }
  }
 }
 assert(native_stereo_options::shift_limit(0)==0);
}
