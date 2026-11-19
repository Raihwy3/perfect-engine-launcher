static int bottom_active;
extern "C" int native_pica_bottom_active(void){return bottom_active;}
#include "../../src/video_3ds.cpp"
#include <cassert>
#include <cstdio>
GfxRenderingAPI native_pica_api={};GfxWindowManagerAPI native_window_api={};
static unsigned saves;static int vx,vy,vw,vh,sx,sy,sw,sh;static unsigned fbw,fbh;static uint64_t clock_now=1000000,slept;
extern "C" {
void configRegisterInt(const char*,s32*,s32,s32){}void configRegisterUInt(const char*,u32*,u32,u32){}
s32 configSave(const char*){saves++;return 0;}
void gfx_set_texture_filter(FilteringMode){}
u64 sysGetMicroseconds(void){return clock_now;}
void sysSleep(s64 hns){slept=hns;clock_now+=hns/10;}
int pd_apt_active(void){return 1;}void pd_request_exit(void){assert(false);}void gfx_start_frame(void){}
}
int main(){
 native_pica_api.set_viewport=[](int x,int y,int w,int h){vx=x;vy=y;vw=w;vh=h;};
 native_pica_api.set_scissor=[](int x,int y,int w,int h){sx=x;sy=y;sw=w;sh=h;};
 native_pica_api.update_framebuffer_parameters=[](int id,uint32_t w,uint32_t h,uint32_t ms,bool,bool,bool,bool){assert(!id && ms==1);fbw=w;fbh=h;};
 assert(videoGetNumDisplayModes()==2);displaymode mode={};assert(videoGetDisplayMode(&mode,0) && mode.width==400 && mode.height==240);
 assert(videoGetDisplayMode(&mode,1) && mode.width==320 && mode.height==240);assert(!videoGetDisplayMode(&mode,2));
 videoSetDisplayMode(1);assert(videoGetWidth()==320 && videoGetAspect()==4.f/3);
 uint32_t w,h;console_dimensions(&w,&h,nullptr,nullptr);assert(w==320 && h==240);
 console_viewport(0,0,320,240);assert(vx==40 && vy==0 && vw==320 && vh==240);
 console_scissor(0,0,320,240);assert(sx==40 && sy==0 && sw==320 && sh==240);
 bottom_active=1;console_viewport(0,0,320,240);assert(vx==0 && vw==320);console_scissor(0,0,320,240);assert(sx==0 && sw==320);bottom_active=0;
 console_update_fb(0,320,240,1,false,true,true,false);assert(fbw==400 && fbh==240);
 videoSetDisplayMode(0);console_viewport(0,0,400,240);assert(vx==0 && vw==400);assert(videoGetAspect()==5.f/3);
 videoSetTextureFilter(0);assert(videoGetTextureFilter()==0);videoSetTextureFilter(1);assert(videoGetTextureFilter()==1);
 videoSetFramerateLimit(30);videoStartFrame();clock_now+=1000;videoStartFrame();assert(slept==323330);
 videoSetFramerateLimit(60);clock_now+=1000;videoStartFrame();assert(slept==156670 && videoGetFramerateLimit()==60);assert(saves>=6);
 puts("PASS: native/4:3 dimensions, centered viewport/scissor, physical target guard, filtering, 30/60 timing, settings save calls");
}
