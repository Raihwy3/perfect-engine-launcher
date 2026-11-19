/* Record actual old/new backend GPU submissions against real Citro3D data
 * types and inline TEV builders. This does not emulate rasterization. */
#include <3ds.h>
#include <citro3d.h>
static unsigned bg_input_dim,bg_output_dim,bg_flags;static Result bg_transfer_result;
Result GX_DisplayTransfer(u32*,u32 input,u32*,u32 output,u32 flags){bg_input_dim=input;bg_output_dim=output;bg_flags=flags;return bg_transfer_result;}
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>
#include <map>
#include "native_gpu_bounds.h"
#ifdef BACKEND_REFERENCE
#include "reference/gfx_pica_before.cpp"
static unsigned merged_draws,material_hits,material_builds;
#else
#include "../../vendor/eog-pica/src/gfx_pica.cpp"
#endif
struct RecordedState {
 C3D_TexEnv env[6];uintptr_t textures[3];u32 texture_params[3];u32 viewport[4],scissor[4];
 unsigned blend_eq,blend_src,blend_dst;
 uintptr_t target;unsigned depth_compare;bool depth,blend,alpha;int alpha_ref;float depth_scale,depth_bias;unsigned buffer_rgb,buffer_alpha;
};
struct RecordedVertex {float pos[4],colour[4],uv[3][2];unsigned used_uv;RecordedState state;};
static RecordedState driver;
static std::vector<RecordedVertex> recorded;
static void *driver_vertices;
static std::map<const C3D_BufInfo *,const void *> driver_buffers;
static bool driver_current_backend;
static unsigned depth_splits,split_vertex_count;
static unsigned custom_reticle_vertices;
extern "C" int pd_bottom_menu_geometry(float *) __attribute__((weak));
static unsigned background_submissions,native_menu_frames,bottom_preview_vertices,preview_fits,retained_panel_frames,frame_bottom_start;
static unsigned preview_geometry_vertices;
static float preview_min_depth=1e20f,preview_max_depth=-1e20f;
extern "C" void pd_bottom_model_fit_report(float x1,float x2,float y1,float y2){
 static uintptr_t previous_id;static float previous_scale;static unsigned steady_frames;
 if(previous_id==inventory_fit_id && previous_scale>0){
  assert(inventory_scale<=previous_scale*(1.f+1.e-5f));steady_frames++;
 }
 previous_id=inventory_fit_id;previous_scale=inventory_scale;
 if(steady_frames==200)fprintf(stderr,"STABLE_PREVIEW_FIT 200 frames without edge-on auto-zoom\n");
 assert(preview_geometry_vertices>12);
 assert(preview_max_depth-preview_min_depth>1e-6f);
 preview_geometry_vertices=0;preview_min_depth=1e20f;preview_max_depth=-1e20f;
 assert(x1>=-.851f && x2<=.851f && y1>=-.851f && y2<=.851f);
 assert(std::fabs(x1+x2)<.001f && std::fabs(y1+y2)<.001f);
 assert(last_viewport[0]==134 && last_viewport[2]==168);
 assert(last_viewport[1]==240-60-112 && last_viewport[3]==112);
 preview_fits++;
}
static bool native_top_rect_scope;static unsigned native_top_rects,native_top_width,native_top_background_vertices;
static bool probe_background_tag;
extern "C" void pd_probe_background_tag(unsigned tag){if((tag&0xfffffffeu)==0x50444d00)probe_background_tag=tag&1;}
extern "C" void pd_probe_top_native_scope(bool active,unsigned width){
 assert(active!=native_top_rect_scope);native_top_rect_scope=active;if(active)native_top_width=width;
}
extern "C" void pd_probe_top_native_rect(float x,float y,float right,float bottom,float outx,float outy,float outright,float outbottom){
 if(!native_top_rect_scope)return;
 assert(!pd_bottom_output && (native_top_width==400 || native_top_width==320));
 assert(std::fabs(x-outx)<.002f && std::fabs(y-outy)<.002f && std::fabs(right-outright)<.002f && std::fabs(bottom-outbottom)<.002f);
 native_top_rects++;
}
static unsigned submissions,bottom_submissions,bottom_vertex_count,viewmodel_triangles;
extern "C" void pd_probe_viewmodel_triangle(void){viewmodel_triangles++;}
static std::vector<float> draw_uniforms;
static GPU_CULLMODE driver_cull=GPU_CULL_NONE;
static shaderProgram_s *driver_program;
#ifndef BACKEND_REFERENCE
static bool watch_owner,allow_watch=true;static int watch_regions[5][4],watch_clears;
extern "C" int native_setting_watch(void){return 0;}
static int test_stereo_auto=1,test_stereo_convergence=100,test_reticle_hud;
extern "C" int native_setting_stereo_auto(void){return test_stereo_auto;}
extern "C" int native_setting_stereo_convergence(void){return test_stereo_convergence;}
extern "C" int native_setting_stereo_comfort(void){return 0;}
extern "C" int native_setting_stereo_hud_depth(void){return 75;}
extern "C" int native_setting_stereo_crosshair_hud(void){return test_reticle_hud;}
extern "C" int native_setting_stereo_ghost_contrast(void){return 100;}
extern "C" int native_setting_stereo_black_floor(void){return 0;}
extern "C" void native_setting_set_stereo_ghost_contrast(int){}
extern "C" void native_setting_set_stereo_black_floor(int){}
extern "C" int native_setting_stereo_strength(void){return 100;}
extern "C" int native_bottom_gpu_acquire(void){watch_owner=allow_watch;return allow_watch;}
extern "C" void native_bottom_gpu_release(void){watch_owner=false;}
extern "C" void native_bottom_watch_regions(const int bounds[5][4]){memcpy(watch_regions,bounds,sizeof watch_regions);}
#endif
Result GSPGPU_FlushDataCache(const void *,u32){return 0;}
Result GSPGPU_InvalidateDataCache(const void *,u32){return 0;}
Result GX_TextureCopy(u32 *src,u32,u32 *dst,u32,u32 size,u32){memcpy(dst,src,size);return 0;}
static std::vector<std::array<float,4>> expected_gpu;
static size_t checked_gpu;
static unsigned indexed_calls;
static uint64_t total_checked,total_retained_hits;
extern "C" void pd_probe_vertex_reference(const float *values,size_t count){for(size_t i=0;i<count;i++)expected_gpu.push_back({values[i*4],values[i*4+1],values[i*4+2],values[i*4+3]});}
extern "C" {
C3D_FVec C3D_FVUnif[2][C3D_FVUNIF_COUNT];
bool C3D_FVUnifDirty[2][C3D_FVUNIF_COUNT];
u16 C3D_BoolUnifs[2];bool C3D_BoolUnifsDirty[2];


C3D_TexEnv *C3D_GetTexEnv(int index){return &driver.env[index];}
void C3D_SetTexEnv(int index,C3D_TexEnv *env){driver.env[index]=*env;}
void C3D_TexEnvBufUpdate(int mode,int mask){if(mode&C3D_RGB)driver.buffer_rgb=mask;if(mode&C3D_Alpha)driver.buffer_alpha=mask;}
void C3D_TexBind(int unit,C3D_Tex *tex){driver.textures[unit]=(uintptr_t)tex->data;driver.texture_params[unit]=tex->param;}
void C3D_CullFace(GPU_CULLMODE mode){driver_cull=mode;}
void C3D_BindProgram(shaderProgram_s *p){driver_program=p;}
void C3D_AlphaTest(bool enabled,GPU_TESTFUNC,int ref){driver.alpha=enabled;driver.alpha_ref=ref;}
void C3D_DepthTest(bool enabled,GPU_TESTFUNC compare,GPU_WRITEMASK){driver.depth=enabled;driver.depth_compare=compare;}
void C3D_DepthMap(bool,float scale,float bias){driver.depth_scale=scale;driver.depth_bias=bias;}
void C3D_AlphaBlend(GPU_BLENDEQUATION eq,GPU_BLENDEQUATION,GPU_BLENDFACTOR src,GPU_BLENDFACTOR dst,GPU_BLENDFACTOR,GPU_BLENDFACTOR){driver.blend=src==GPU_SRC_ALPHA;driver.blend_eq=eq;driver.blend_src=src;driver.blend_dst=dst;}
void C3D_SetViewport(u32 x,u32 y,u32 w,u32 h){driver.viewport[0]=x;driver.viewport[1]=y;driver.viewport[2]=w;driver.viewport[3]=h;}
void C3D_SetScissor(GPU_SCISSORMODE,u32 x,u32 y,u32 w,u32 h){driver.scissor[0]=x;driver.scissor[1]=y;driver.scissor[2]=w;driver.scissor[3]=h;}
bool C3D_FrameDrawOn(C3D_RenderTarget *target){driver.target=(uintptr_t)target;return true;}
void C3D_SetBufInfo(C3D_BufInfo *info){
#ifndef BACKEND_REFERENCE
 if(auto it=driver_buffers.find(info);it!=driver_buffers.end()){driver_vertices=const_cast<void *>(it->second);return;}
 driver_vertices=info==&watch_buffer?watch_vertices:info==&sharp_buffer?sharp_vertices:vertices;
#else
 (void)info;
#endif
}
#ifndef BACKEND_REFERENCE
void BufInfo_Init(C3D_BufInfo *b){*b={};}
int BufInfo_Add(C3D_BufInfo *b,const void *p,ptrdiff_t,int,u64){driver_buffers[b]=p;return b->bufCount++;}
static C3D_RenderTarget *outputs[3];
static int fail_create,live_targets,live_sharp_textures;
static int test_vignette_mode;static unsigned test_render_state;
int native_setting_vignette_mode(void){return test_vignette_mode;}
unsigned native_setting_render_state(void){return test_render_state;}
static int test_sharpness,test_vignette,test_bloom,test_grain,test_aa=0;
static unsigned mock_texture_live,mock_texture_limit;
extern "C" u32 linearSpaceFree(void){unsigned limit=mock_texture_limit?mock_texture_limit:16*1024*1024;return mock_texture_live<limit?limit-mock_texture_live:0;}
extern "C" void pd_probe_texture_memory_frame(int begin){
 in_frame=false;
 if(begin){for(auto &tex:retired)C3D_TexDelete(&tex);retired.clear();texture_reclaim_safe=true;if(!getenv("PD_PROBE_NO_TEXTURE_TRIM"))native_graphics_trim_texture_cache();texture_reclaim_safe=false;in_frame=true;}
}
extern "C" void pd_probe_texture_memory_limit(unsigned bytes){mock_texture_limit=bytes;}

int native_setting_grain(void){return test_grain;}
void native_setting_set_grain(int v){test_grain=v;}
int native_setting_vignette(void){return test_vignette;}
int native_setting_bloom(void){return test_bloom;}
void native_setting_set_vignette(int v){test_vignette=v;}
void native_setting_set_bloom(int v){test_bloom=v;}
int native_setting_aa(void){return test_aa;}
void native_setting_set_aa(int value){test_aa=value;}
int native_setting_sharpness(void){return test_sharpness;}
void native_setting_set_sharpness(int value){test_sharpness=value;}
C3D_RenderTarget *C3D_RenderTargetCreate(int w,int h,GPU_COLORBUF f,C3D_DEPTHTYPE) {
 assert(!in_frame);
 if(fail_create && !--fail_create)return nullptr;
 auto *t=static_cast<C3D_RenderTarget *>(calloc(1,sizeof(C3D_RenderTarget)));assert(t);
 t->frameBuf.width=w;t->frameBuf.height=h;t->frameBuf.colorFmt=f;live_targets++;return t;
}
void C3D_RenderTargetDelete(C3D_RenderTarget *t){assert(!in_frame && t);if(t->linked)C3D_RenderTargetDetachOutput(t);free(t);live_targets--;}
bool C3D_TexInitWithParams(C3D_Tex *t,C3D_TexCube *,C3D_TexInitParams p) {
 assert(!p.onVram || !in_frame);*t={};unsigned bytes=C3D_TexCalcTotalSize(p.width*p.height*4,p.maxLevel);
 if(mock_texture_limit && bytes>linearSpaceFree())return false;
 t->data=calloc(1,bytes);assert(t->data);mock_texture_live+=bytes;
 t->width=p.width;t->height=p.height;t->fmt=p.format;t->size=p.width*p.height*4;t->maxLevel=p.maxLevel;live_sharp_textures++;return true;
}
void C3D_TexGenerateMipmap(C3D_Tex *t,GPU_TEXFACE){assert(t->data);}
void C3D_TexFlush(C3D_Tex *){}
void C3D_TexDelete(C3D_Tex *t){if(t->data){unsigned bytes=C3D_TexCalcTotalSize(t->size,t->maxLevel);assert(bytes<=mock_texture_live);mock_texture_live-=bytes;}free(t->data);t->data=nullptr;live_sharp_textures--;}
C3D_RenderTarget *C3D_RenderTargetCreateFromTex(C3D_Tex *t,GPU_TEXFACE,int,C3D_DEPTHTYPE depth) {
 auto *rt=C3D_RenderTargetCreate(t->width,t->height,GPU_RB_RGBA8,depth);if(rt)rt->frameBuf.colorBuf=t->data;return rt;
}
void C3D_RenderTargetSetOutput(C3D_RenderTarget *t,gfxScreen_t screen,gfx3dSide_t side,u32 flags){
 unsigned id=screen==GFX_BOTTOM?2:side==GFX_RIGHT?1:0;
 if(outputs[id])outputs[id]->linked=false;
 outputs[id]=t;
 if(t){t->screen=screen;t->side=side;t->linked=true;t->transferFlags=flags;
   if(screen==GFX_BOTTOM)assert(flags==(GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8)|GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB565)));
 }
}
void C3D_FrameSplit(u8){assert(in_frame);split_vertex_count=vertex_count;depth_splits++;}
void C3D_FrameBufClear(C3D_FrameBuf *fb,C3D_ClearBits bits,u32,u32){
 if(fb!=&watch_target->frameBuf && in_frame && vertex_count && (bits&C3D_CLEAR_DEPTH))assert(split_vertex_count==vertex_count);
 watch_clears++;
}
#endif
static void record_vertex(int i) {
 if(native_top_rect_scope && probe_background_tag && driver.target==(uintptr_t)target)native_top_background_vertices++;

 if(driver.target==(uintptr_t)watch_target){
  assert(driver.target==(uintptr_t)watch_target && watch_attached);
  assert(driver.viewport[0]>=0 && driver.viewport[1]>=0 && driver.viewport[2]>0 && driver.viewport[3]>0);
  assert(driver.viewport[0]+driver.viewport[2]<=240 && driver.viewport[1]+driver.viewport[3]<=320);
  assert(driver.scissor[0]<=240 && driver.scissor[2]<=240 && driver.scissor[1]<=320 && driver.scissor[3]<=320);
  bottom_vertex_count++;
  if(pd_bottom_output && (driver.viewport[2]!=240 || driver.viewport[3]!=320))bottom_preview_vertices++;
 }

 auto *data=static_cast<Vertex *>(driver_vertices);
 if(reticle_scope && !pd_bottom_output)custom_reticle_vertices++;
   RecordedVertex out={};out.state=driver;
   memcpy(out.pos,data[i].pos,sizeof out.pos);memcpy(out.colour,data[i].color,sizeof out.colour);
#ifndef BACKEND_REFERENCE
   if(world_palette_active) {
     // Emulate the world shader's relative-addressed rigid transform. Read
     // uniforms at submission time so premature palette reuse corrupts tests.
     unsigned row=unsigned(out.pos[3]);assert(row%4==0 && row<WORLD_PALETTE_SIZE*4);
     float position[4]={out.pos[0],out.pos[1],out.pos[2],1};
     for(unsigned j=0;j<4;j++) {
       const auto &m=C3D_FVUnif[0][world_matrix_uniform+row+j];
       out.pos[j]=m.x*position[0]+m.y*position[1]+m.z*position[2]+m.w;
     }
     // Canonical N64 clip depth for existing CPU/reference comparisons.
     out.pos[2]=2*out.pos[2]+out.pos[3];
   }
   else if(!gpu_world)out.pos[2]=2*out.pos[2]+out.pos[3];
#endif
   if(driver.target!=(uintptr_t)watch_target && world_palette_active && !expected_gpu.empty()){
    assert(checked_gpu<expected_gpu.size());const auto &expected=expected_gpu[checked_gpu++];
    for(unsigned axis=0;axis<3;axis++){float a=out.pos[axis]/out.pos[3],b=expected[axis]/expected[3];if(std::fabs(a-b)>0.002f){fprintf(stderr,"GPU/reference mismatch at %zu axis %u: %.6f != %.6f\n",checked_gpu,axis,a,b);abort();}}
    assert(std::fabs(out.pos[3]-expected[3])<=.01f+std::fabs(expected[3])*.0002f);
   }
#ifndef BACKEND_REFERENCE
   if(pd_bottom_output && inventory_pass==2 && out.pos[3]>0){
    float z=out.pos[2]/out.pos[3];
    preview_min_depth=std::min(preview_min_depth,z);preview_max_depth=std::max(preview_max_depth,z);
    preview_geometry_vertices++;
   }
#endif
   memcpy(out.uv,data[i].uv,sizeof out.uv);
   for(const auto &env:driver.env)for(unsigned channel=0;channel<2;channel++)for(unsigned arg=0;arg<3;arg++) {
     unsigned src=((channel?env.srcAlpha:env.srcRgb)>>(arg*4))&15;
     if(src>=GPU_TEXTURE0 && src<=GPU_TEXTURE2)out.used_uv|=1u<<(src-GPU_TEXTURE0);
   }
   recorded.push_back(out);
}
void C3D_DrawArrays(GPU_Primitive_t,int first,int count) {
 if(driver.target==(uintptr_t)watch_target){bottom_submissions++;if(pd_menu_background)background_submissions++;}
 submissions++;draw_uniforms.push_back(C3D_FVUnif[0][0].x);
 for(int i=first;i<first+count;i++)record_vertex(i);
}
void C3D_DrawElements(GPU_Primitive_t,int count,int type,const void *indices) {
 assert(type==C3D_UNSIGNED_SHORT && indices);indexed_calls++;
 if(driver.target==(uintptr_t)watch_target){bottom_submissions++;if(pd_menu_background)background_submissions++;}
 submissions++;draw_uniforms.push_back(C3D_FVUnif[0][0].x);
 auto *elements=static_cast<const uint16_t *>(indices);
 for(int i=0;i<count;i++)record_vertex(elements[i]);
}
}

static bool background_probe;static unsigned background_queued,background_failed,background_width,background_height;
static unsigned char background_probe_buffer[256*512*3];
extern "C" void *pd_menu_background_buffer(void){return background_probe?background_probe_buffer:nullptr;}
extern "C" void pd_menu_background_queued(unsigned width,unsigned height){background_queued++;background_width=width;background_height=height;}
extern "C" void pd_menu_background_failed(int){background_failed++;}
static void check_background_dimensions(){
 background_probe=true;in_frame=true;
 auto width=target->frameBuf.width,height=target->frameBuf.height;int samples=top_sample_scale;
 unsigned queued=background_queued,splits=depth_splits;
 const unsigned cases[][3]={{240,400,1},{256,512,1},{512,512,2}};
 for(const auto &c:cases){
  target->frameBuf.width=c[0];target->frameBuf.height=c[1];top_sample_scale=c[2];
  bg_input_dim=bg_output_dim=0;pd_menu_background_snapshot();
  assert(background_queued==queued && depth_splits==splits && !bg_input_dim && !bg_output_dim);
 }
 target->frameBuf.width=width;target->frameBuf.height=height;top_sample_scale=samples;
 background_probe=false;in_frame=false;
 fprintf(stderr,"PASS disabled menu capture: no GPU transfer/split for normal, padded or AA surfaces\n");
}

extern GfxRenderingAPI gfx_opengl_api;
static void check_pd_adaptive_depth(){
 auto saved=depth_frame;auto *old_target=target;bool old_stereo=stereo_active;
 float old_slider=stereo_slider;
 C3D_RenderTarget t={};t.frameBuf.width=240;t.frameBuf.height=400;
 std::vector<uint32_t> pixels(240*400);
 auto projection=native_stereo::Projection::from(-30010.f/29990.f,-1,-600000.f/29990.f,0);
 float zp=(1+projection.ratio+projection.b/100.f)*.5f;
 uint32_t packed=uint32_t((1-zp)*16777215.f);
 std::fill(pixels.begin(),pixels.end(),packed);t.frameBuf.depthBuf=pixels.data();
 target=&t;stereo_active=true;stereo_slider=1;frontend_ui=false;pd_bottom_output=false;pd_native_menu_seen=false;watch_seen=false;
 depth_copy=static_cast<uint32_t *>(linearAlloc(DEPTH_COPY_CAPACITY));assert(depth_copy);
 depth_frame={};depth_frame.target=&t;depth_frame.projection=projection;depth_frame.usable=true;
 depth_frame.width=240;depth_frame.height=400;depth_frame.scale=-1;depth_frame.samples=1;
 depth_frame.viewport[2]=400;depth_frame.viewport[3]=240;
 depth_frame.tilt.r[0].y=1;depth_frame.tilt.r[1].x=-1;
 in_frame=true;depth_copied=false;queue_stereo_depth();in_frame=false;assert(depth_copied && depth_frame.pixels==depth_copy);
 update_stereo_depth(1.f/60);assert(std::fabs(world_convergence-100)<.02f);
 float held=world_convergence;frontend_ui=true;update_stereo_depth(1.f/60);assert(world_convergence==held);
 frontend_ui=false;depth_copied=false;pd_bottom_output=true;depth_frame.usable=false;
 queue_stereo_depth();assert(!depth_copied);pd_bottom_output=false;
 test_stereo_auto=0;test_stereo_convergence=50;test_reticle_hud=1;reticle_shift=.2f;
 update_stereo_depth(1.f/60);assert(std::fabs(world_convergence-1500.f)<.001f && reticle_shift==0);
 depth_frame.usable=true;depth_copied=false;in_frame=true;queue_stereo_depth();in_frame=false;assert(!depth_copied);
 test_stereo_auto=1;test_stereo_convergence=100;test_reticle_hud=0;
 linearFree(depth_copy);depth_copy=nullptr;depth_copied=false;depth_frame=saved;
 target=old_target;stereo_active=old_stereo;stereo_slider=old_slider;invalidate_stereo_depth();
 fprintf(stderr,"PASS PD adaptive depth: queued scene snapshot, device-depth reconstruction, convergence and pause history\n");
}
static void host_init(){
 clip_stereo_uniform=world_stereo_uniform=4;world_matrix_uniform=5;
 atexit([]{if(getenv("PD_PROBE_CUSTOM")){fprintf(stderr,"CUSTOM_RETICLE vertices=%u\n",custom_reticle_vertices);assert(custom_reticle_vertices>100);}assert(!native_top_rect_scope);if(getenv("PD_PROBE_UI") || getenv("PD_PROBE_KEYBOARD")){assert(native_top_background_vertices>1000);fprintf(stderr,"NATIVE_TOP_BACKGROUND vertices=%u\n",native_top_background_vertices);}if(getenv("PD_PROBE_UI") || getenv("PD_PROBE_KEYBOARD")){assert(native_top_rects>1000);fprintf(stderr,"NATIVE_TOP_PIXEL_RECTS one-to-one=%u width=%u\n",native_top_rects,native_top_width);}
 fprintf(stderr,"PICA_PROBE gpu_vertices_checked=%llu indexed_draws=%u retained_hits=%llu bottom_draws=%u bottom_vertices=%u viewmodel_triangles=%u depth_splits=%u\n",(unsigned long long)total_checked,indexed_calls,(unsigned long long)total_retained_hits,bottom_submissions,bottom_vertex_count,viewmodel_triangles,depth_splits);if(getenv("PD_PROBE_GAMEPLAY")){if(getenv("PD_PROBE_MENU_STRESS")){fprintf(stderr,"BACKGROUND_MIRROR draws=%u\n",background_submissions);assert(background_submissions==0);assert(bottom_submissions>background_submissions);assert(native_menu_frames>100);fprintf(stderr,"INDEPENDENT_NATIVE_MENU frames=%u\n",native_menu_frames);}if(getenv("PD_PROBE_INVENTORY")){fprintf(stderr,"BOTTOM_PREVIEW vertices=%u\n",bottom_preview_vertices);assert(bottom_preview_vertices>100);assert(preview_fits>100);fprintf(stderr,"CENTERED_ROTATING_PREVIEWS fits=%u\n",preview_fits);}if(getenv("PD_PROBE_PANEL_CACHE")){fprintf(stderr,"RETAINED_PANEL frames=%u\n",retained_panel_frames);assert(retained_panel_frames>500);}assert(viewmodel_triangles>1000);assert(depth_splits>0);}});
 gfx_opengl_api.init();
 vertices=static_cast<Vertex *>(calloc(MAX_VERTICES,sizeof(Vertex)));
 watch_vertices=static_cast<Vertex *>(calloc(MAX_VERTICES,sizeof(Vertex)));
 world_indices=static_cast<uint16_t *>(calloc(MAX_VERTICES,sizeof(uint16_t)));
 driver_vertices=vertices;assert(vertices && watch_vertices && world_indices);
 check_pd_adaptive_depth();
 BufInfo_Init(&top_buffer);BufInfo_Add(&top_buffer,vertices,sizeof(Vertex),5,0x43210);
 scalar_ramp.data=calloc(1,256*256*4);
 for(auto &t:noise_samples){t.width=t.height=64;t.data=calloc(1,64*64*4);}
 target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
 watch_target=C3D_RenderTargetCreate(240,320,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
 check_background_dimensions();
}
static void host_start(){pd_probe_texture_memory_frame(1);frame_bottom_start=bottom_submissions;C3D_SetBufInfo(&top_buffer);pd_bottom_output=pd_bottom_seen=pd_menu_background=pd_native_menu_seen=false;watch_seen=false;mesh_cache.begin_frame();retained_meshes.begin_frame();static_attributes.begin_frame();gpu_world=world_palette_active=false;world_palette_count=world_palette_row=0;pending_indexed=false;pending_elements=nullptr;expected_gpu.clear();checked_gpu=0;recorded.clear();pending_draw.clear();vertex_count=world_index_count=0;in_frame=true;tev_valid=false;}
static void host_end(){flush_draw();pd_menu_present();
 if(pd_native_menu_seen && frame_bottom_start==bottom_submissions){assert(watch_attached && pd_bottom_seen);retained_panel_frames++;}
 if(pd_native_menu_seen){
  float rect[6];assert(pd_bottom_menu_geometry && pd_bottom_menu_geometry(rect));
  assert(rect[2]==14 && rect[3]==8 && rect[4]==292 && rect[5]==224);
  native_menu_frames++;
 }

 if(pd_menu_ready() && !pd_native_menu_seen){
   assert(pd_menu_quad && pd_menu_texture.data);
   // PICA's T origin is opposite to the row-major software canvas.
   assert(pd_menu_quad[0].uv[0][1]==1.f);
   assert(pd_menu_quad[2].uv[0][1]==1.f-240.f/256);
   assert(pd_menu_quad[0].pos[1]>pd_menu_quad[2].pos[1]);
 }
assert(!pd_bottom_output);assert(checked_gpu==expected_gpu.size());total_checked+=checked_gpu;total_retained_hits+=retained_meshes.hits;in_frame=false;}
static void host_draw(float *buffer,size_t length,size_t triangles){
 size_t count=triangles*3;
 if(count && active){
  const auto &c=active->cc;size_t stride=length/count,channels=c.opt_alpha?4:3;
  assert(stride>=c.num_inputs*channels);size_t start=stride-c.num_inputs*channels;
  for(unsigned input=0;input<c.num_inputs;input++)for(size_t vertex=1;vertex<count;vertex++){
   if(uniform_input_hint&(1u<<input))for(unsigned rgb=0;rgb<3;rgb++)assert(buffer[vertex*stride+start+input*channels+rgb]==buffer[start+input*channels+rgb]);
   if(c.opt_alpha && (alpha_uniform_hint&(1u<<input)))assert(buffer[vertex*stride+start+input*channels+3]==buffer[start+input*channels+3]);
  }
 }
 draw(buffer,length,triangles);
}
GfxRenderingAPI pd_probe_api = {name,
                                   max_texture,
                                   clip,
                                   unload,
                                   load,
                                   create,
                                   lookup,
                                   info,
                                   clear_shaders,
                                   new_texture,
                                   select_texture,
                                   upload,
                                   sampler,
                                   depth,
                                   depth_range,
                                   viewport,
                                   scissor,
                                   alpha,
                                   host_draw,
                                   host_init,
                                   resize,
                                   host_start,
                                   host_end,
                                   finish,
                                   framebuffer,
                                   update_fb,
                                   bind_fb,
                                   copy_fb,
                                   clear_fb,
                                   resolve,
                                   fb_texture,
                                   select_fb,
                                   delete_texture,
                                   filter,
                                   get_filter,
                                   mipmap,
                                   anisotropy,
                                   max_anisotropy};
union Gfx;
extern "C" {
#include "gfx_api.h"
}
extern "C" void __real_gfx_init(const GfxInitSettings *);
extern "C" void __wrap_gfx_init(const GfxInitSettings *settings){
 GfxInitSettings copy=*settings;copy.rapi=&pd_probe_api;copy.window_settings.width=400;copy.window_settings.height=240;
 gfx_framebuffers_enabled=false;gfx_detail_textures_enabled=false;gfx_msaa_level=1;
 __real_gfx_init(&copy);
}
extern "C" int __wrap_videoFramebuffersSupported(void){return 0;}
extern "C" int __wrap_videoCreateFramebuffer(unsigned,unsigned,int,int){return 0;}
extern "C" void __wrap_videoCopyFramebuffer(int,int,int,int){}
extern "C" void __wrap_videoSetAnisotropicFilter(unsigned){}

extern "C" void __real_gfx_set_mipmap_filter(enum MipmapFilteringMode);
extern "C" void __wrap_gfx_set_mipmap_filter(enum MipmapFilteringMode){__real_gfx_set_mipmap_filter(MIPMAP_DISABLED);}
