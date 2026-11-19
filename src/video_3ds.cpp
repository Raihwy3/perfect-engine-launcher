/* Original Perfect Dark frame API -> Fast3D interpreter -> PICA. */
#include "gfx_pc.h"
extern "C" {
#include "video.h"
#include "config.h"
}
#include "native_graphics.h"
#include "native_settings.h"
#include "pd_runtime.h"
extern GfxRenderingAPI native_pica_api;
extern "C" int native_pica_bottom_active(void);
extern GfxWindowManagerAPI native_window_api;
extern "C" int pd_apt_active(void);
extern "C" void pd_log_flush(void);
extern "C" void pd_request_exit(void);
static s32 display_mode=0,frame_limit=30;
static u32 texture_filter=1;
static GfxRenderingAPI console_api;
static GfxWindowManagerAPI console_window;
static int render_width(){return display_mode==1?320:400;}
static void console_dimensions(uint32_t *w,uint32_t *h,int32_t *x,int32_t *y){*w=render_width();*h=240;if(x)*x=0;if(y)*y=0;}
static void console_viewport(int x,int y,int w,int h){native_pica_api.set_viewport(x+(native_pica_bottom_active()?0:(400-render_width())/2),y,w,h);}
static void console_scissor(int x,int y,int w,int h){native_pica_api.set_scissor(x+(native_pica_bottom_active()?0:(400-render_width())/2),y,w,h);}
/* The physical target stays 400x240; 4:3 uses a centered 320x240 viewport. */
static void console_update_fb(int id,uint32_t w,uint32_t h,uint32_t msaa,bool invert,bool target,bool depth,bool extract){
 if(id==0 && (w==320 || w==400) && h==240)w=400;
 native_pica_api.update_framebuffer_parameters(id,w,h,msaa,invert,target,depth,extract);
}
static void __attribute__((constructor)) videoConfig3ds(){
 configRegisterInt("3DS.Resolution",&display_mode,0,1);
 configRegisterInt("3DS.FramerateLimit",&frame_limit,30,60);
 configRegisterUInt("3DS.TextureFilter",&texture_filter,0,1);
}
static bool ready;
static uint64_t last_frame,batch_start;
static float average_fps;
extern "C" void pd_native_graphics_abort(void);
extern "C" void native_graphics_destroy_interpreter(void){gfx_destroy();}
extern "C" unsigned videoGetFrameCount(void){return native_graphics_frames();}
extern "C" int videoInit(void){
 gfx_current_native_viewport={0,0,320,220};gfx_current_native_aspect=320.f/220;
 gfx_framebuffers_enabled=false;gfx_detail_textures_enabled=false;gfx_msaa_level=1;
 GfxInitSettings settings={};console_api=native_pica_api;console_api.set_viewport=console_viewport;console_api.set_scissor=console_scissor;console_api.update_framebuffer_parameters=console_update_fb;
 console_window=native_window_api;console_window.get_dimensions=console_dimensions;
 settings.rapi=&console_api;settings.wapi=&console_window;
 settings.window_settings.width=400;settings.window_settings.height=240;settings.window_settings.title="Perfect Engine Launcher";
 gfx_init(&settings);gfx_set_texture_filter(texture_filter?FILTER_LINEAR:FILTER_NONE);gfx_set_mipmap_filter(MIPMAP_DISABLED);ready=true;
 sysLogPrintf(LOG_NOTE,"PICA initialized; framebuffers/effects disabled for bring-up");return 0;
}
extern "C" void videoStartFrame(void){
 if(!pd_apt_active())pd_request_exit();
 uint64_t interval=frame_limit==60?16667:33333;
 uint64_t now=sysGetMicroseconds();if(last_frame && now-last_frame<interval)sysSleep((interval-(now-last_frame))*10);
 last_frame=sysGetMicroseconds();gfx_start_frame();
}
extern "C" void videoSubmitCommands(Gfx *commands){if(commands){uint64_t start=sysGetMicroseconds();gfx_run(commands);pd_profile_record(11,sysGetMicroseconds()-start);}}
extern "C" void videoEndFrame(void){
 gfx_end_frame();pd_runtime_frame();pd_log_flush();static unsigned frame;uint64_t now=sysGetMicroseconds();
 if(!batch_start)batch_start=now;
 if(++frame%120==0){average_fps=120000000.f/(now-batch_start);batch_start=now;sysLogPrintf(LOG_NOTE,"Presented frame %u: triangles=%u uploads=%u fps=%.2f",frame,native_graphics_triangles(),native_graphics_uploads(),(double)average_fps);}
}
extern "C" void pd_video_shutdown(void){if(ready){ready=false;native_graphics_shutdown();}}
extern "C" void pd_video_abort(void){if(ready){ready=false;pd_native_graphics_abort();}}
extern "C" void videoShutdown(void){pd_video_shutdown();}
extern "C" void videoClearScreen(void){}
extern "C" void *videoGetWindowHandle(void){return nullptr;}
extern "C" void videoUpdateNativeResolution(int w,int h){if(w>0 && h>0){gfx_current_native_viewport.width=w;gfx_current_native_viewport.height=h;gfx_current_native_aspect=float(w)/h;}}
extern "C" int videoGetNativeWidth(void){return gfx_current_native_viewport.width;}
extern "C" int videoGetNativeHeight(void){return gfx_current_native_viewport.height;}
extern "C" int videoGetWidth(void){return render_width();}
extern "C" int videoGetHeight(void){return 240;}
extern "C" float videoGetAspect(void){return float(render_width())/240;}
extern "C" float videoGetAverageFPS(void){return average_fps;}
extern "C" void videoResetTextureCache(void){gfx_texture_cache_clear();}
extern "C" void videoFreeCachedTexture(const void *p){gfx_texture_cache_delete((const uint8_t*)p);}
extern "C" void videoFreeCachedTextures(const void *p,const void *q){gfx_texture_cache_delete_range((const uint8_t*)p,(const uint8_t*)q);}
extern "C" int videoCreateFramebuffer(u32,u32,int,int){return 0;}
extern "C" void videoSetFramebuffer(int){}
extern "C" void videoResetFramebuffer(void){}
extern "C" void videoCopyFramebuffer(int,int,int,int){}
extern "C" void videoResizeFramebuffer(int,u32,u32,int,int){}
extern "C" int videoFramebuffersSupported(void){return 0;}
extern "C" int videoGetFullscreen(void){return 0;}
extern "C" int videoGetFullscreenMode(void){return 0;}
extern "C" int videoGetMaximizeWindow(void){return 0;}
extern "C" void videoSetMaximizeWindow(s32){}
extern "C" int videoGetCenterWindow(void){return 0;}
extern "C" void videoSetCenterWindow(s32){}
extern "C" u32 videoGetTextureFilter(void){return texture_filter;}
extern "C" int videoGetTextureFilter2D(void){return 1;}
extern "C" u32 videoGetAnisotropicFilter(void){return 0;}
extern "C" u32 videoGetMaxAnisotropyLevel(void){return 0;}
extern "C" int videoGetDetailTextures(void){return 0;}
extern "C" int videoGetDisplayModeIndex(void){return display_mode;}
extern "C" int videoGetDisplayMode(displaymode *mode,const s32 index){if(index<0 || index>1)return 0;mode->width=index?320:400;mode->height=240;return 1;}
extern "C" int videoGetNumDisplayModes(void){return 2;}
extern "C" int videoGetVsync(void){return 1;}
extern "C" int videoGetFramerateLimit(void){return frame_limit==60?60:30;}
extern "C" int videoGetDisplayFPS(void){return 0;}
extern "C" int videoGetMSAA(void){return 1;}
extern "C" float videoGetGlareBrightness(void){return 1.f;}
extern "C" float videoGetOverexposureScale(void){return 1.f;}
extern "C" void videoSetWindowOffset(s32 , s32){}
extern "C" void videoSetFullscreen(s32){}
extern "C" void videoSetFullscreenMode(s32){}
extern "C" void videoSetTextureFilter(u32 value){texture_filter=value?1:0;if(ready)gfx_set_texture_filter(texture_filter?FILTER_LINEAR:FILTER_NONE);configSave(CONFIG_PATH);}
extern "C" void videoSetTextureFilter2D(s32){}
extern "C" void videoSetAnisotropicFilter(u32){}
extern "C" void videoSetDetailTextures(s32){}
extern "C" void videoSetDisplayMode(const s32 value){if(value>=0 && value<=1){display_mode=value;configSave(CONFIG_PATH);}}
extern "C" void videoSetVsync(const s32){}
extern "C" void videoSetFramerateLimit(const s32 value){frame_limit=value==60?60:30;configSave(CONFIG_PATH);}
extern "C" void videoSetDisplayFPS(const s32){}
extern "C" void videoSetMSAA(const s32){}
extern "C" void videoSetGlareBrightness(f32){}
extern "C" void videoSetOverexposureScale(f32){}

extern "C" void pd_bottom_frame_geometry(float *width,float *height){*width=gfx_current_native_viewport.width;*height=gfx_current_native_viewport.height;}
