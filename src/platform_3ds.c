#include "pd_bitmap_copy.h"
#include "pd_gyro_filter.h"
#include "native_motion_sensor.h"
#include "native_flick.h"
#include "pd_log_queue.h"
#include "pd_menu_frame_copy.h"
/* 3DS lifecycle and system services. N64 headers stay out of this unit. */
#include <3ds.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <unistd.h>
#include "native_graphics.h"
#include "pd_build.h"
#include "pd_runtime.h"
#include "pd_paths.h"
extern void audioShutdown(void);
extern void pd_boot_begin(void);
extern void pd_boot_stage(const char *,unsigned,unsigned);
extern void pd_boot_finish(void);
extern int native_update_startup(const char *);
extern void pd_save_settings(void);
u32 __stacksize__=256*1024;
/* Bound both reservations: automatic normal-heap sizing failed in libctru
 * before main on the first hardware test (dump 51). */
u32 __ctru_heap_size=64*1024*1024;
u32 __ctru_linear_heap_size=16*1024*1024;
static const char *folder=PD_DATA_FOLDER;
static int arg_count,exit_ready,failed;static const char **arg_values;
static FILE *log_file;static jmp_buf quit;
static uint64_t tick_epoch;
static PrintConsole bottom;
static Thread audio_worker;
static LightEvent audio_work,audio_done;
static void (*audio_job)(void *);static void *audio_arg;
static jmp_buf audio_fault_jump;static __thread int in_audio_worker;static char audio_fault[768];
static int audio_stalled;
static volatile int audio_stop;static int audio_pending,audio_tried;
void sysLogPrintf(int,const char *,...);
uint64_t sysGetMicroseconds(void);
static void audio_thread(void *unused){
 (void)unused;sysLogPrintf(0,"Audio worker running on core %ld",(long)svcGetProcessorID());
 for(;;){LightEvent_Wait(&audio_work);if(audio_stop)break;in_audio_worker=1;if(!setjmp(audio_fault_jump))audio_job(audio_arg);in_audio_worker=0;LightEvent_Signal(&audio_done);}
}
void sysFatalError(const char *,...);
void pd_audio_join(void){
 if(audio_pending){
  uint64_t start=sysGetMicroseconds();unsigned waits=0;
  while(LightEvent_WaitTimeout(&audio_done,250000000LL)){
   if(sysGetMicroseconds()-start>=2500000){audio_stalled=1;sysFatalError("Audio worker stalled for 2.5 seconds; ending the session safely");}
   if(++waits==1 || waits%4==0)sysLogPrintf(1,"Audio worker still pending after %llu us",(unsigned long long)(sysGetMicroseconds()-start));
  }
  audio_pending=0;
  if(waits)sysLogPrintf(1,"Audio worker recovered after %llu us",(unsigned long long)(sysGetMicroseconds()-start));
 }
 if(audio_fault[0])sysFatalError("Audio worker: %s",audio_fault);
}
void pd_audio_dispatch(void (*job)(void *),void *arg){
 pd_audio_join();
 if(pd_option_get(6) && !audio_tried){audio_tried=1;LightEvent_Init(&audio_work,RESET_ONESHOT);LightEvent_Init(&audio_done,RESET_ONESHOT);audio_worker=threadCreate(audio_thread,NULL,128*1024,0x30,2,false);sysLogPrintf(0,"Audio worker: %s",audio_worker?"created on core 2":"unavailable; synchronous fallback");}
 if(!pd_option_get(6) || !audio_worker){job(arg);return;}
 audio_job=job;audio_arg=arg;audio_pending=1;LightEvent_Signal(&audio_work);
}
static void stop_audio_worker(void){if(audio_stalled)return;audio_fault[0]=0;pd_audio_join();if(audio_worker){audio_stop=1;LightEvent_Signal(&audio_work);threadJoin(audio_worker,U64_MAX);threadFree(audio_worker);audio_worker=NULL;}}
void sysFatalError(const char *fmt,...);
static char log_buffer[32768];
static struct pd_log_queue log_queue;
static LightLock log_lock;
static LightEvent log_work;
static Thread log_worker;
static int log_stop;
static void log_drain(void){
 char line[PD_LOG_LINE];size_t length;
 for(;;){
  LightLock_Lock(&log_lock);length=pd_log_pop(&log_queue,line);LightLock_Unlock(&log_lock);
  if(!length)break;
  fwrite(line,1,length,log_file);
 }
}
static void log_thread(void *unused){
 (void)unused;uint64_t flushed=sysGetMicroseconds();
 for(;;){
  LightEvent_WaitTimeout(&log_work,1000000000LL);
  log_drain();
  LightLock_Lock(&log_lock);int stop=log_stop;LightLock_Unlock(&log_lock);
  uint64_t now=sysGetMicroseconds();
  if(stop || now-flushed>=1000000){fflush(log_file);flushed=now;}
  if(stop)break;
 }
}
static void log_start(void){
 LightLock_Init(&log_lock);LightEvent_Init(&log_work,RESET_ONESHOT);
 if(log_file)log_worker=threadCreate(log_thread,NULL,64*1024,0x3f,1,false);
}
static void log_finish(void){
 if(!log_file)return;
 if(log_worker){
  LightLock_Lock(&log_lock);log_stop=1;LightLock_Unlock(&log_lock);
  LightEvent_Signal(&log_work);threadJoin(log_worker,U64_MAX);threadFree(log_worker);log_worker=NULL;
 }else log_drain();
 if(log_queue.dropped)fprintf(log_file,"Log queue dropped %u old records\n",log_queue.dropped);
 fclose(log_file);log_file=NULL;
}
void pd_log_flush(void){
 static uint64_t last;uint64_t now=sysGetMicroseconds();
 if(log_worker && now-last>=1000000){last=now;LightEvent_Signal(&log_work);}
}
static int bottom_gpu_owned,bottom_needs_refresh,bottom_bitmap_owned;
void pd_bottom_bitmap_present(const uint16_t *pixels){
 if(bottom_gpu_owned || !pixels)return;
 u16 width,height;u16 *lcd=(u16 *)gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&width,&height);
 if(!lcd || width!=240 || height!=320)sysFatalError("Unexpected bottom LCD layout: %u x %u",width,height);
 pd_bitmap_copy(lcd,pixels);
 GSPGPU_FlushDataCache(lcd,320*240*2);
 bottom_bitmap_owned=1;
}

/* Read only the previous completed top-LCD transfer. These callbacks run at
 * FrameEnd (record) and after the next blocking FrameBegin (consume). */
extern uintptr_t pd_bottom_frame_token(void);
extern int pd_bottom_frame_bounds(int *);
static const u8 *menu_frame_source;
static uintptr_t menu_frame_token;
static int menu_frame_source_bounds[4];
static int menu_frame_disabled;
static u8 *menu_background;
static unsigned menu_background_stride,menu_background_rows;
static int menu_background_reported;
static const u8 *menu_preview_source;
static int menu_preview[8],menu_has_preview;
extern int pd_bottom_preview(int *);
extern const uint16_t *pd_bottom_overlay_pixels(void);
extern const uint8_t *pd_bottom_overlay_alpha(void);
void *pd_menu_background_buffer(void){return pd_bottom_frame_token()?menu_background:NULL;}
void pd_menu_background_failed(int result){menu_frame_disabled=1;menu_frame_source=NULL;sysLogPrintf(1,"Background transfer failed: %08lx",(unsigned long)result);}
void pd_menu_background_queued(unsigned stride,unsigned rows){
 menu_background_stride=stride;menu_background_rows=rows;
 if(!menu_background_reported){menu_background_reported=1;sysLogPrintf(0,"Bottom background: %ux%u BGR surface; native UI compositing",stride,rows);}
 menu_frame_token=pd_bottom_frame_token();menu_frame_source=menu_frame_token?menu_background:NULL;
 pd_bottom_frame_bounds(menu_frame_source_bounds);
}
int pd_bottom_frame_supported(void){
 /* Hardware menu-open freeze: keep all menu pixels on the software LCD path.
  * Do not allocate a snapshot or queue a mid-frame GPU transfer. */
 return 0;
}

void pd_menu_frame_record(void){
 if(!pd_bottom_frame_token()){menu_preview_source=NULL;menu_has_preview=0;return;}
 menu_has_preview=pd_bottom_preview(menu_preview);menu_preview_source=NULL;
 if(menu_has_preview){u16 w,h;const u8 *p=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,&w,&h);if(w==240 && h==400)menu_preview_source=p;}
}
void pd_menu_frame_consume(void){
 if(!menu_frame_source || !menu_frame_token || menu_frame_token!=pd_bottom_frame_token())return;
 /* Bounds belong to this completed transfer, not an animating next dialog. */
 if(menu_background_stride<240 || menu_background_stride>256 || menu_background_rows<400 || menu_background_rows>512)return;
 u16 width,height;u16 *lcd=(u16 *)gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&width,&height);
 if(bottom_gpu_owned || !lcd || width!=240 || height!=320)return;
 uint64_t copy_start=sysGetMicroseconds();
 if(R_FAILED(GSPGPU_InvalidateDataCache(menu_frame_source,menu_background_stride*menu_background_rows*3))){
  menu_frame_disabled=1;menu_frame_source=NULL;
  sysLogPrintf(1,"Menu frame read unavailable; software menu fallback");return;
 }
 pd_background_frame_copy(lcd,menu_frame_source,menu_background_stride,menu_frame_source_bounds);
 if(menu_preview_source)GSPGPU_InvalidateDataCache(menu_preview_source,400*240*3);
 const uint16_t *overlay=pd_bottom_overlay_pixels();const uint8_t *alpha=pd_bottom_overlay_alpha();
 for(int x=0;x<320;x++)for(int y=0;y<240;y++){
  unsigned k=y*320+x,a=alpha[k];
  unsigned fg=overlay[k],bg=lcd[x*240+239-y];
  if(menu_preview_source && x>=menu_preview[4] && x<menu_preview[6] && y>=menu_preview[5] && y<menu_preview[7]){
   int sx=menu_preview[0]+(x-menu_preview[4])*(menu_preview[2]-menu_preview[0])/(menu_preview[6]-menu_preview[4]);
   int sy=menu_preview[1]+(y-menu_preview[5])*(menu_preview[3]-menu_preview[1])/(menu_preview[7]-menu_preview[5]);
   if((unsigned)sx<400 && (unsigned)sy<240){const u8 *p=menu_preview_source+(sx*240+239-sy)*3;bg=((p[2]>>3)<<11)|((p[1]>>2)<<5)|(p[0]>>3);}
   if(a==127 && fg==2)a=0;
  }
  unsigned r=((fg>>11)&31)+(((bg>>11)&31)*(255-a))/255;
  unsigned g=((fg>>5)&63)+(((bg>>5)&63)*(255-a))/255;
  unsigned b=(fg&31)+((bg&31)*(255-a))/255;
  if(r>31){r=31;}
  if(g>63){g=63;}
  if(b>31){b=31;}
  lcd[x*240+239-y]=(r<<11)|(g<<5)|b;
 }
 GSPGPU_FlushDataCache(lcd,320*240*2);bottom_bitmap_owned=1;
 pd_profile_record(14,sysGetMicroseconds()-copy_start);
 menu_frame_source=NULL;
}

int native_bottom_gpu_acquire(void){
 bottom_gpu_owned=1;gfxSetScreenFormat(GFX_BOTTOM,GSP_RGB565_OES);gfxSetDoubleBuffering(GFX_BOTTOM,true);return 1;
}
void native_bottom_gpu_release(void){
 bottom_gpu_owned=0;bottom_needs_refresh=1;
 gfxSetDoubleBuffering(GFX_BOTTOM,false);
 /* Leave the last completed image intact until the next native UI frame. */
}
void pd_bottom_present(const char *text){
 if(bottom_gpu_owned)return;
 /* Normal startup keeps the themed bootstrap instead of exposing the
  * diagnostic console while ROM fonts and engine menus initialise. */
 (void)text;extern void pd_boot_wait(void);pd_boot_wait();
}
static int touch_active,touch_x,touch_y,touch_dx,touch_dy;
int pd_touch_look(int *x,int *y){*x=touch_dx;*y=touch_dy;touch_dx=touch_dy=0;return 1;}
uint64_t native_work_profile_clock(void){return sysGetMicroseconds();}
void native_work_profile_record(unsigned stage,uint64_t begin,uint64_t end){if(end>=begin)pd_profile_record(stage,end-begin);}

extern int pd_engine_main(int,const char **);
extern void pd_video_shutdown(void);
extern void pd_video_abort(void);
extern int pd_verify_rom(const char *);
int pd_apt_active(void){return aptMainLoop();}
void pd_request_exit(void){if(exit_ready)longjmp(quit,1);}
void sysInitArgs(int argc,const char **argv){arg_count=argc;arg_values=argv;}
int sysArgCheck(const char *arg){for(int i=1;i<arg_count;i++)if(!strcasecmp(arg_values[i],arg))return 1;return 0;}
const char *sysArgGetString(const char *arg){for(int i=1;i+1<arg_count;i++)if(!strcasecmp(arg_values[i],arg))return arg_values[i+1];return NULL;}
int sysArgGetInt(const char *arg,int def){const char *v=sysArgGetString(arg);return v?(int)strtol(v,NULL,0):def;}
void sysInit(void){if(!tick_epoch)tick_epoch=svcGetSystemTick();}
uint64_t sysGetMicroseconds(void){
 uint64_t ticks=svcGetSystemTick()-tick_epoch;
 return ticks/SYSCLOCK_ARM11*1000000ULL+(ticks%SYSCLOCK_ARM11)*1000000ULL/SYSCLOCK_ARM11;
}
void sysSleep(int64_t hns){if(hns>0)svcSleepThread(hns*100);}
void sysCpuRelax(void){svcSleepThread(1000);}
void sysGetExecutablePath(char *out,uint32_t len){snprintf(out,len,"%s",folder);}
void sysGetHomePath(char *out,uint32_t len){snprintf(out,len,"%s",folder);}
int sysLogIsOpen(void){return log_file!=NULL;}
void sysLogPrintf(int level,const char *fmt,...) {
 if(!log_file)return;
 char line[PD_LOG_LINE];
 int prefix=snprintf(line,sizeof line,"[%llu] %d ",(unsigned long long)sysGetMicroseconds(),level);
 if(prefix<0 || (size_t)prefix>=sizeof line-1)return;
 va_list ap;va_start(ap,fmt);int written=vsnprintf(line+prefix,sizeof line-prefix,fmt,ap);va_end(ap);
 if(written<0)return;
 size_t length=(size_t)prefix+(size_t)written;
 if(length>=sizeof line-1)length=sizeof line-2;
 line[length++]='\n';
 LightLock_Lock(&log_lock);pd_log_push(&log_queue,line,length);LightLock_Unlock(&log_lock);
 if(log_worker)LightEvent_Signal(&log_work);
}
void sysFatalError(const char *fmt,...) {
 char error[768];va_list ap;va_start(ap,fmt);vsnprintf(error,sizeof error,fmt,ap);va_end(ap);
 if(in_audio_worker){snprintf(audio_fault,sizeof audio_fault,"%s",error);longjmp(audio_fault_jump,1);}
 sysLogPrintf(2,"FATAL: %s",error);
 pd_log_flush();
 if(!failed){failed=1;pd_video_abort();}
 consoleSelect(&bottom);consoleClear();
 printf("Perfect Engine Launcher stopped\n\n%s\n\nSee pd-3ds.log\nSTART: exit\n",error);gfxFlushBuffers();
 failed=1;
 while(aptMainLoop()){hidScanInput();if(hidKeysDown()&KEY_START)break;gspWaitForVBlank();}
 pd_request_exit();abort();
}
void *sysMemAlloc(uint32_t bytes){void *p=malloc(bytes?bytes:1);if(!p)sysFatalError("Out of memory requesting %u bytes",bytes);return p;}
void *sysMemZeroAlloc(uint32_t bytes){void *p=calloc(1,bytes?bytes:1);if(!p)sysFatalError("Out of memory requesting %u bytes",bytes);return p;}
void *sysMemRealloc(void *ptr,uint32_t bytes){void *p=realloc(ptr,bytes?bytes:1);if(!p)sysFatalError("Out of memory reallocating %u bytes",bytes);return p;}
void sysMemFree(void *ptr){free(ptr);}
void crashInit(void){}void crashShutdown(void){}
void crashCreateThread(void){}void crashReset(void){}
int main(void) {
 tick_epoch=svcGetSystemTick();
 gfxInitDefault();consoleInit(GFX_BOTTOM,&bottom);osSetSpeedupEnable(true);
 mkdir("sdmc:/3ds",0777);mkdir(folder,0777);
 char path[256];snprintf(path,sizeof path,"%s/pd-3ds.log",folder);log_file=fopen(path,"w");
 if(log_file)setvbuf(log_file,log_buffer,_IOFBF,sizeof log_buffer);
 pd_boot_begin();
 if(!native_update_startup(folder)){if(log_file){fclose(log_file);log_file=NULL;}gfxExit();return 0;}
 log_start();
 sysLogPrintf(0,"Perfect Engine Launcher build " PD_BUILD_ID ": starting title sequence / US Rev 1");
 sysLogPrintf(0,"Log writer: %s",log_worker?"background core 1":"RAM buffer; deferred until exit");
 sysLogPrintf(0,"Heap reservations: normal=%lu linear=%lu",(unsigned long)envGetHeapSize(),(unsigned long)envGetLinearHeapSize());
 bool newer=false;APT_CheckNew3DS(&newer);
 exit_ready=1;
 if(!setjmp(quit)) {
  if(!newer)sysFatalError("Initial diagnostic needs a New 3DS/2DS.\nOld-model memory support is not ready.");
  pd_boot_stage("Checking ROM file",0,10);
  snprintf(path,sizeof path,"%s/%s",folder,PD_ROM_NAME);
  struct stat rom_status;
  if(stat(path,&rom_status)!=0)sysFatalError("ROM not found.\nPlace pd.z64 beside the launcher.\nUse your own US Rev 1 backup.");
  if(rom_status.st_size!=33554432)sysFatalError("Wrong ROM size.\nUse the 32 MiB US Rev 1 ROM.");
  if(!pd_verify_rom(path))sysFatalError("Missing or wrong ROM.\nUse US Rev 1 named pd.z64.");
  sysLogPrintf(0,"Full ROM SHA-1 verified");
  snprintf(path,sizeof path,"%s/pd.ini",folder);
  pd_boot_stage(access(path,F_OK)==0?"Saved settings found":"Using default settings",100,90);
  pd_boot_finish();
  const char *args[]={"perfect-engine-launcher.3dsx","--rom-file",PD_ROM_NAME,"--boot-stage","0x5a","--no-crash-handler","--basedir","$E","--savedir","$E","--portable"};
  
  pd_engine_main(sizeof args/sizeof *args,args);
 }
 exit_ready=0;
 if(!failed)pd_save_settings();
 stop_audio_worker();
 if(!audio_stalled)audioShutdown();
 if(!failed)pd_video_shutdown();
 if(!failed && menu_background){linearFree(menu_background);menu_background=NULL;}
 log_finish();
 gfxExit();return failed?1:0;
}
static struct NativeGyro pd_gyro;
static struct NativeFlick pd_flick;
void pd_hid_take_flick(float *yaw){*yaw=pd_flick.pending;pd_flick.pending=0;}
static int gyro_enabled,gyro_failed,accel_enabled;
static float gyro_coefficient;
void pd_hid_take_gyro(float *yaw,float *pitch){*yaw=pd_gyro.yaw;*pitch=pd_gyro.pitch;pd_gyro.yaw=pd_gyro.pitch=0;}
static void pd_gyro_poll(void){
 int mode=pd_option_get(19),needed=mode || native_motion_calibration_status()>0;
 if(needed && !gyro_enabled && !gyro_failed){
   gyro_enabled=R_SUCCEEDED(HIDUSER_EnableGyroscope());
   if(!gyro_enabled || R_FAILED(HIDUSER_GetGyroscopeRawToDpsCoefficient(&gyro_coefficient)) || !(gyro_coefficient>0)){
     if(gyro_enabled)HIDUSER_DisableGyroscope();
     gyro_enabled=0;gyro_failed=1;
   }
 }
 if(needed && !accel_enabled)accel_enabled=R_SUCCEEDED(HIDUSER_EnableAccelerometer());
 if(!needed && accel_enabled){HIDUSER_DisableAccelerometer();accel_enabled=0;}
 if(!needed){if(gyro_enabled)HIDUSER_DisableGyroscope();gyro_enabled=gyro_failed=0;}
 angularRate rate={0};if(gyro_enabled)hidGyroRead(&rate);
 uint64_t now=(uint64_t)(svcGetSystemTick()*(1000000.0/SYSCLOCK_ARM11));
 accelVector accel={0};if(accel_enabled)hidAccelRead(&accel);
 float yaw=0,pitch=0;
 native_motion_set_filter(pd_option_get(32));
 native_motion_sample(now,gyro_enabled?rate.x/gyro_coefficient:0,
  gyro_enabled?rate.z/gyro_coefficient:0,gyro_enabled?-rate.y/gyro_coefficient:0,
  accel_enabled?accel.x/512.f:0,accel_enabled?accel.z/512.f:0,accel_enabled?-accel.y/512.f:0,
  gyro_enabled && pd_gyro_allowed(),pd_option_get(22),pd_option_get(23),&yaw,&pitch);
 pd_gyro.yaw=native_gyro_bound(pd_gyro.yaw+yaw,15);
 pd_gyro.pitch=native_gyro_bound(pd_gyro.pitch+pitch,15);
}
void pd_hid_sample(unsigned *buttons,int *x,int *y,int *rx,int *ry) {
 hidScanInput();pd_gyro_poll();circlePosition c,r;hidCircleRead(&c);hidCstickRead(&r);
 uint64_t flick_now=(uint64_t)(svcGetSystemTick()*(1000000.0/SYSCLOCK_ARM11));
 native_flick_sample(&pd_flick,flick_now,r.dx,r.dy,pd_option_get(24) && pd_motion_allowed() && !pd_aiming());
 *buttons=hidKeysHeld();*x=c.dx;*y=c.dy;*rx=r.dx;*ry=r.dy;
 touchPosition pos;hidTouchRead(&pos);
 if(*buttons&KEY_TOUCH){
  if(!touch_active){int action=pd_bottom_touch(pos.px,pos.py);if(action==1)*buttons|=KEY_A;else if(action==2)*buttons|=KEY_B;else if(action==3)*buttons|=KEY_LEFT;else if(action==4)*buttons|=KEY_RIGHT;else if(action==5)*buttons|=KEY_UP;else if(action==6)*buttons|=KEY_DOWN;}
  else {touch_dx+=(int)pos.px-touch_x;touch_dy+=touch_y-(int)pos.py;}
  touch_x=pos.px;touch_y=pos.py;touch_active=1;
 }else touch_active=0;
 if((*buttons&(KEY_START|KEY_SELECT))==(KEY_START|KEY_SELECT))pd_request_exit();
}
