#include "pd_runtime.h"
#include "system.h"
void pd_bottom_present(const char *text){(void)text;}
int pd_touch_look(int *x,int *y){*x=*y=0;return 0;}
void pd_audio_dispatch(void (*job)(void *),void *arg){job(arg);}
void pd_audio_join(void){}
void pd_render_state(unsigned state){(void)state;}
uint64_t native_work_profile_clock(void){return sysGetMicroseconds();}
void native_work_profile_record(unsigned stage,uint64_t begin,uint64_t end){if(end>=begin)pd_profile_record(stage,end-begin);}

int native_bottom_gpu_acquire(void){return 1;}
void native_bottom_gpu_release(void){}

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned bitmap_frames;
void pd_bottom_bitmap_present(const uint16_t *pixels){
 unsigned nonzero=0;for(unsigned i=0;i<320*240;i++)if(pixels[i]!=3)nonzero++;
 assert(nonzero>100);bitmap_frames++;
 const char *path=getenv("PD_PROBE_BITMAP");
 if(path && (bitmap_frames==2 || getenv("PD_PROBE_UI"))){FILE *f=fopen(path,"wb");assert(f);fwrite(pixels,2,320*240,f);fclose(f);}
}
extern void pd_viewmodel_report(void);
static void report_bitmap_frames(void){pd_viewmodel_report();fprintf(stderr,"BOTTOM_SOFTWARE frames=%u\n",bitmap_frames);if(getenv("PD_PROBE_GAMEPLAY"))assert(bitmap_frames>=1);}
static void __attribute__((constructor)) bitmap_probe_init(void){atexit(report_bitmap_frames);}

#include <ultra64.h>
#include "types.h"
#include "data.h"
#include "bss.h"
void pd_hid_take_gyro(float *x,float *y){
 static int previous=-1;*x=*y=0;
 if(getenv("PD_PROBE_GYRO") && g_Vars.lvframenum!=previous){
   previous=g_Vars.lvframenum;
   *x=__builtin_sinf(g_Vars.lvframenum*.07f)*.2f;*y=__builtin_cosf(g_Vars.lvframenum*.07f)*.1f;
 }
}

void pd_hid_take_flick(float *yaw){*yaw=0;}

#include "config.h"
void pd_probe_controls_config(void){
 const int keys[]={20,21,22,23,24,25,26,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44};
 const int expected[]={15,15,2,1,1,8,15,1,1,0,1,1,14,0,19,10,20,1,0,20,7,11,1,7};int old[24];
 for(unsigned i=0;i<24;i++){old[i]=pd_option_get(keys[i]);pd_option_set(keys[i],expected[i]);}
 assert(configSave(".pd-controls-roundtrip.ini"));
 for(unsigned i=0;i<24;i++)pd_option_set(keys[i],0);
 assert(configLoad(".pd-controls-roundtrip.ini"));
 for(unsigned i=0;i<24;i++){assert(pd_option_get(keys[i])==expected[i]);pd_option_set(keys[i],old[i]);}
 fprintf(stderr,"PASS real config save/reload: 400%% sensitivities, gyro spaces/axis, flick, 3D controls, crosshair style/opacity/colour, filter and accessibility\n");
}
