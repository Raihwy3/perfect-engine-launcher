/* Runs before game workers, audio or Citro3D are initialized. */
#include <3ds.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "native_update.h"
#include "native_update_http.h"
#include "native_version.h"
extern void pd_boot_update(const char *,const char *,const char *,const char *,unsigned,unsigned);
#include "native_update_ca.h"
static Thread worker_thread;
static struct UpdateJob {
    int download,cancel,done,ok;
    unsigned received,total;
    char metadata[65537],error[160],file[640];
    struct NativeUpdateRelease release;
} job;
static int cancelled(void *unused){(void)unused;return __atomic_load_n(&job.cancel,__ATOMIC_RELAXED);}
static void received(void *unused,size_t done,size_t total){(void)unused;__atomic_store_n(&job.received,(unsigned)done,__ATOMIC_RELAXED);__atomic_store_n(&job.total,(unsigned)total,__ATOMIC_RELAXED);}
static void worker(void *unused) {
    (void)unused;void *socket_buffer=memalign(0x1000,256*1024);
    if(!socket_buffer)goto end;
    if(R_FAILED(socInit(socket_buffer,256*1024))){free(socket_buffer);goto end;}
    struct NativeUpdateTransfer t={0};t.cancel=cancelled;t.progress=received;
    const char *url=NATIVE_UPDATE_API;
    if(job.download){url=job.release.url;t.file=job.file;t.expected=job.release.bytes;t.digest=job.release.digest;}
    else {t.memory=job.metadata;t.capacity=sizeof job.metadata;}
    job.ok=native_update_http(url,&t,native_update_ca,sizeof native_update_ca-1,job.error,sizeof job.error);
    if(job.ok && !job.download)job.ok=native_update_release(job.metadata,t.length,NATIVE_RELEASE_TAG,&job.release);
    socExit();free(socket_buffer);
end:__atomic_store_n(&job.done,1,__ATOMIC_RELEASE);
}
static int start_job(int download) {
    job.download=download;job.cancel=job.done=job.ok=0;job.received=job.total=0;job.error[0]=0;
    worker_thread=threadCreate(worker,NULL,128*1024,0x3f,0,false);return worker_thread!=NULL;
}
static void draw(const char *stage,const char *message,const char *action,unsigned done,unsigned total) {
    pd_boot_update(stage,message,action,job.release.tag,done,total);
}
static int buttons(void) {
    hidScanInput();u32 keys=hidKeysDown();
    if(keys&KEY_TOUCH){touchPosition p;hidTouchRead(&p);if(p.py>=190 && p.py<=230)keys|=p.px<160?KEY_A:KEY_B;}
    return (keys&KEY_B)?-1:(keys&KEY_A)?1:0;
}
static int prompt(const char *stage,const char *message,const char *action) {
    draw(stage,message,action,0,0);hidScanInput();
    while(aptMainLoop()){int key=buttons();if(key)return key;gspWaitForVBlank();}return 0;
}
static int await_job(int download) {
    u64 started=osGetTime(),paint=0;int aborting=0;
    for(;;) {
        if(__atomic_load_n(&job.done,__ATOMIC_ACQUIRE)) {
            threadJoin(worker_thread,U64_MAX);threadFree(worker_thread);worker_thread=NULL;
            return !aborting && job.ok;
        }
        int active=aptMainLoop();
        if(!active || (!aborting && buttons()<0) || (!download && osGetTime()-started>=4500)) {
            aborting=1;__atomic_store_n(&job.cancel,1,__ATOMIC_RELAXED);
        }
        if(active && osGetTime()>=paint){
            draw(download?"DOWNLOADING RELEASE":"CHECKING FOR UPDATES",
                aborting?"Finishing network request...":download?"Saving launcher update...":"Checking GitHub releases.",
                aborting?"Please wait":download?"B: Cancel":"B: Skip",
                __atomic_load_n(&job.received,__ATOMIC_RELAXED),__atomic_load_n(&job.total,__ATOMIC_RELAXED));
            paint=osGetTime()+200;
        }
        svcSleepThread(16000000);
    }
}

static int stage_progress(size_t done,size_t total) {
    static u64 paint;
    if(!aptMainLoop() || buttons()<0)return 0;
    if(osGetTime()>=paint){draw("PREPARING UPDATE","Checking the launcher files...","B: Cancel",(unsigned)done,(unsigned)total);paint=osGetTime()+100;}
    return 1;
}
/* Returns zero only when the user chooses to leave without starting the game. */
int native_update_startup(const char *directory) {
    char error[160]={0},flag[640],archive[640];
    if(!native_update_recover(directory,error,sizeof error))return prompt("UPDATE RECOVERY",error,"A: Continue    B: Exit")>0;
    snprintf(flag,sizeof flag,"%s/no-update-check.flag",directory);if(access(flag,F_OK)==0)return 1;
    u32 wifi=0;if(R_FAILED(acInit()))return 1;Result connected=ACU_GetWifiStatus(&wifi);acExit();if(R_FAILED(connected) || !wifi)return 1;
    if(!start_job(0) || !await_job(0))return aptMainLoop();
    int choice=prompt("NEW RELEASE AVAILABLE","Install this update?\nROM, saves and settings are kept.","A: Install     B: Not now");
    if(choice<=0)return choice<0;
    snprintf(job.file,sizeof job.file,"%s/launcher-update.zip.part",directory);
    if(!start_job(1) || !await_job(1)) {
        if(!__atomic_load_n(&job.done,__ATOMIC_ACQUIRE))return aptMainLoop();
        return prompt("UPDATE NOT INSTALLED","Download failed.\nThe current launcher is unchanged.","A: Play        B: Exit")>0;
    }
    snprintf(archive,sizeof archive,"%s/launcher-update.zip",directory);
    remove(archive);
    if(rename(job.file,archive) || !native_update_install(archive,directory,stage_progress,error,sizeof error))
        return prompt("UPDATE NOT INSTALLED",error[0]?error:"Unable to save update on SD card.","A: Play        B: Exit")>0;
    return prompt("UPDATE INSTALLED","Relaunch to use the new version.\nB continues this running build.","A: Exit        B: Play")<0;
}
