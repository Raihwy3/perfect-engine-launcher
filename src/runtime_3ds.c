#include <ultra64.h>
#include "constants.h"
#include "types.h"
#include "data.h"
#include "bss.h"
#include "game/env.h"
#include "lib/vi.h"
#include "config.h"
#include "system.h"
#include "video.h"
#include "game/objectives.h"
#include "game/options.h"
#include "game/lv.h"
#include "game/menu.h"
#include "game/mainmenu.h"
#include "game/lang.h"
#include "lib/model.h"
#include "pd_runtime.h"
#include "pd_objectives.h"
#include <stdio.h>
#include <string.h>
/* 0 LOD,1 texture LOD,2 view distance,3 effects,4 guard LOS rate,
 * 5 hidden guard LOS,6 audio worker,7 supersampling,8 image filter,
 * 9 vignette,10 bloom,11 grain,12 vignette activation,13 GPU transforms,14 simple glass,15 DSP dry,
 * 16 retained meshes,17 guard AI cadence,18 cutscene bars. */
static s32 values[45]={1,1,0,1,0,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,3,3,0,0,0,4,3,0,0,0,1,0,0,-1,1,9,4,15,0,10,0,0,0,0,0};
static const s32 maxima[45]={4,3,3,3,3,1,1,1,6,2,2,2,3,1,1,1,1,3,2,2,15,15,2,1,1,8,15,0,1,1,1,1,1,15,1,19,10,20,1,10,20,7,11,1,7};
static void __attribute__((constructor)) runtimeConfig(void){
 const char *keys[]={"3DS.ModelLOD","3DS.TextureLOD","3DS.ViewDistance","3DS.EffectsLOD","3DS.GuardLOSRate","3DS.HiddenGuardLOS","3DS.AudioWorker","3DS.Supersampling","3DS.ImageFilter","3DS.Vignette","3DS.Bloom","3DS.Grain","3DS.VignetteMode","3DS.GPUTransforms","3DS.SimpleGlass","3DS.DryDSPMix","3DS.RetainedMeshes","3DS.GuardAICadence","3DS.CutsceneBars","3DS.GyroMode","3DS.GyroSensitivity","3DS.TouchSensitivity","3DS.GyroSpace","3DS.GyroAxis","3DS.FlickStick","3DS.StereoStrength","3DS.LookSensitivity","3DS.ReservedCalibration","3DS.AimMovement","3DS.AimLock","3DS.StaminaSway","3DS.SteadyScope","3DS.GyroFilter","3DS.LookVertical","3DS.AutoConvergence","3DS.Convergence","3DS.PopoutLimit","3DS.HUDDepth","3DS.CrosshairDepth","3DS.GhostContrast","3DS.BlackFloor","3DS.CrosshairStyle","3DS.CrosshairOpacity","3DS.AlwaysCrosshair","3DS.CrosshairColour"};
 for(int i=0;i<45;i++)configRegisterInt(keys[i],&values[i],i==33?-1:0,maxima[i]);
}
int pd_option_get(int key){return key==33 && values[33]<0?values[26]:key>=0 && key<45?values[key]:0;}
void pd_option_set(int key,int value){if(key<0 || key>=45)return;if(value<0)value=0;if(value>maxima[key])value=maxima[key];values[key]=value;configSave(CONFIG_PATH);}
void pd_profile_report(void);
void pd_viewmodel_report(void);
static unsigned ray_calls,ray_hits;
void pd_guard_ray_count(int hit){ray_calls++;ray_hits+=hit!=0;}
extern void pd_render_state(unsigned);
extern char *objectiveGetText(s32);
extern u32 g_ObjectiveStatuses[];
extern s32 g_StageNum;
static struct menuitem *touch_items[6];
static struct menudialog *touch_dialog;
static unsigned touch_count;
extern MenuItemHandlerResult menuhandlerMissionList(s32,struct menuitem *,union handlerdata *);
static struct menuitem *mission_item;
static int mission_page,mission_count,touch_mission_indices[6];
int pd_touch_look_allowed(void){return !g_Menus[0].curdialog && g_Vars.currentplayer && !g_Vars.in_cutscene;}
int pd_bottom_touch(int x,int y){
 int native=pd_bottom_native_touch(x,y);if(native>=0)return native;
 struct menudialog *current=g_Menus[0].curdialog;
 if(pd_bottom_objectives_visible() && current && current->definition){
  if(y<208){if(x<12)return 3;if(x>=308)return 4;}
  if(y>=208){if(x<80)return 2;if(x<240)return x<160?3:4;return 1;}
  int objectives=pd_bottom_objective_indices(NULL,0);
  int abort_y=44+objectives*(objectives>4?22:28);
  if(y>=abort_y-3 && y<abort_y+16){
   struct menuitem *items=current->definition->items;
   for(unsigned i=0;i<96 && items[i].type!=MENUITEMTYPE_END;i++)if(items[i].type==MENUITEMTYPE_SELECTABLE){current->focuseditem=&items[i];return 1;}
  }
  return 0;
 }
 if(!touch_dialog || current!=touch_dialog)return 0;
 if(y<208){if(x<12)return 3;if(x>=308)return 4;}
 if(y>=208){
 if(mission_item && x>=80 && x<240){mission_page+=x<160?-6:6;if(mission_page<0)mission_page=0;if(mission_page>=mission_count)mission_page=mission_count?((mission_count-1)/6)*6:0;return 0;}
 if(x<80)return 2;if(x>=240)return 1;return touch_dialog->dimmed?(x<160?5:6):(x<160?3:4);}
 if(touch_dialog->dimmed)return 0;
 int row=(y-24)/24;if(y<24 || row<0 || (unsigned)row>=touch_count)return 0;
 touch_dialog->focuseditem=touch_items[row];
 if(mission_item){union handlerdata data={0};data.list.value=touch_mission_indices[row];mission_item->handler(MENUOP_SET,mission_item,&data);return 0;}
 return 1;
}
static void append(char *out,size_t capacity,const char *text){size_t n=strlen(out);if(n<capacity-1)snprintf(out+n,capacity-n,"%s",text?text:"");}
void pd_update_view_distance(void);
void pd_runtime_frame(void){
 pd_update_view_distance();
 static unsigned counter;static int dynamic=1;static u64 last_report;
 float fps=videoGetAverageFPS();
 if(++counter%120==0 && values[0]==4 && fps>0){float target=videoGetFramerateLimit();if(fps<target*.8f && dynamic<3)dynamic++;else if(fps>target*.95f && dynamic>1)dynamic--;}
 int lod=values[0]==4?dynamic:values[0];
 modelSetDistanceChecksDisabled(lod==0);
 modelSetDistanceScale(lod==0?1.f:lod==1?1.f:lod==2?1.5f:2.f);
 unsigned state=0;
 if(g_Vars.currentplayer && !g_Menus[0].curdialog){state=1;if(g_Vars.currentplayer->insightaimmode)state|=4;if(g_Vars.currentplayer->crouchpos!=CROUCHPOS_STAND)state|=2;}
 pd_render_state(state);
 uint64_t bottom_start=sysGetMicroseconds();
 int native_presented=pd_bottom_native_present() || pd_bottom_objectives_present();
 pd_profile_record(12,sysGetMicroseconds()-bottom_start);
 if(counter%6==0 && !native_presented){
  struct pd_bottom_row rows[6]={0};
  char text[1600]="PERFECT ENGINE LAUNCHER\n";char line[160];touch_count=0;touch_dialog=g_Menus[0].curdialog;
  if(!touch_dialog)mission_item=NULL;
  if(touch_dialog && touch_dialog->definition){
   append(text,sizeof text,"MENU / TOUCH TO SELECT\n\n");
   struct menuitem *items=touch_dialog->definition->items;
   struct menuitem *previous_mission=mission_item;mission_item=NULL;
   for(unsigned i=0;i<96 && items[i].type!=MENUITEMTYPE_END;i++)if(items[i].type==MENUITEMTYPE_LIST && items[i].handler){mission_item=&items[i];break;}
   if(mission_item){
    if(previous_mission!=mission_item)mission_page=0;
    union handlerdata data={0};mission_item->handler(MENUOP_GETOPTIONCOUNT,mission_item,&data);mission_count=data.list.value;
    for(int index=mission_page;index<mission_count && touch_count<6;index++){
     data=(union handlerdata){0};data.list.value=index;char *label=(char*)mission_item->handler(MENUOP_GETOPTIONTEXT,mission_item,&data);
     if(mission_item->handler==menuhandlerMissionList && index<(int)data.list.unk04u32 && index<17)label=langGet(g_SoloStages[index].name3);
     char clean[40];snprintf(clean,sizeof clean,"%.34s",label?label:"");for(char *p=clean;*p;p++)if(*p=='\n')*p=' ';
     snprintf(rows[touch_count].label,sizeof rows[touch_count].label,"%s",clean);
     union handlerdata selected={0};mission_item->handler(MENUOP_GETSELECTEDINDEX,mission_item,&selected);rows[touch_count].focused=selected.list.value==index;
     snprintf(line,sizeof line,"%2d %s\n\n\n",index+1,clean);append(text,sizeof text,line);touch_items[touch_count]=mission_item;touch_mission_indices[touch_count++]=index;
    }
   }else{
   unsigned focused=0,count=0;struct menuitem *list[96];
   for(unsigned i=0;i<96 && items[i].type!=MENUITEMTYPE_END;i++){
    unsigned type=items[i].type;
    if(type!=MENUITEMTYPE_SELECTABLE && type!=MENUITEMTYPE_DROPDOWN && type!=MENUITEMTYPE_CHECKBOX && type!=MENUITEMTYPE_SLIDER)continue;
    if(items[i].handler && !(items[i].flags & MENUITEMFLAG_SELECTABLE_OPENSDIALOG)){
     union handlerdata data={0};if(items[i].handler(MENUOP_CHECKHIDDEN,&items[i],&data))continue;
    }
    if(&items[i]==touch_dialog->focuseditem)focused=count;list[count++]=&items[i];
   }
   unsigned start=(focused/6)*6;
   for(unsigned i=start;i<count && touch_count<6;i++){
    char *label=menuResolveParam2Text(list[i]);
    char clean[40];snprintf(clean,sizeof clean,"%.34s",label?label:"");for(char *p=clean;*p;p++)if(*p=='\n')*p=' ';
    char value[48]="";struct menuitem *entry=list[i];
    if(entry->handler && !(entry->flags&MENUITEMFLAG_SELECTABLE_OPENSDIALOG)){
     union handlerdata data={0};
     if(entry->type==MENUITEMTYPE_DROPDOWN){entry->handler(MENUOP_GETSELECTEDINDEX,entry,&data);char *selected=(char*)entry->handler(MENUOP_GETOPTIONTEXT,entry,&data);snprintf(value,sizeof value,"%.34s",selected?selected:"");}
     else if(entry->type==MENUITEMTYPE_CHECKBOX)snprintf(value,sizeof value,"%s",entry->handler(MENUOP_GET,entry,&data)?"ON":"OFF");
     else if(entry->type==MENUITEMTYPE_SLIDER){entry->handler(MENUOP_GETSLIDER,entry,&data);snprintf(value,sizeof value,"%lu",(unsigned long)data.slider.value);}
    }
    for(char *p=value;*p;p++)if(*p=='\n')*p=' ';
    snprintf(line,sizeof line,"%c %s\n  %s\n\n",list[i]==touch_dialog->focuseditem?'>':' ',clean,value);
    snprintf(rows[touch_count].label,sizeof rows[touch_count].label,"%s",clean);
    snprintf(rows[touch_count].value,sizeof rows[touch_count].value,"%s",value);
    rows[touch_count].focused=list[i]==touch_dialog->focuseditem;
    append(text,sizeof text,line);touch_items[touch_count++]=list[i];
   }
   }
   append(text,sizeof text,"\x1b[27;1HBACK     <      >     ACCEPT");
  }else if(g_StageNum<STAGE_TITLE && g_StageNum!=STAGE_CITRAINING){
   append(text,sizeof text,"MISSION OBJECTIVES\n\n");
   int count=objectiveGetCount();if(count>10)count=10;
   for(int i=0;i<count;i++)if(objectiveGetDifficultyBits(i)&(1<<lvGetDifficulty())){
    snprintf(line,sizeof line,"[%s] %.100s\n\n",g_ObjectiveStatuses[i]==OBJECTIVE_COMPLETE?"OK":g_ObjectiveStatuses[i]==OBJECTIVE_FAILED?"FAIL":"--",objectiveGetText(i));append(text,sizeof text,line);
   }
   append(text,sizeof text,"\nTOUCH AND DRAG TO LOOK\nSTART: PAUSE");
  }else append(text,sizeof text,"\nNATIVE ENGINE\n\nCircle Pad: navigate\nA: accept    B: back");
  if(pd_bottom_native_present()){touch_dialog=NULL;touch_count=0;mission_item=NULL;}
  else if(pd_bottom_objectives_present()){touch_dialog=NULL;touch_count=0;mission_item=NULL;}
  else if(touch_dialog){if(!pd_bottom_menu_present(touch_dialog,rows,touch_count,mission_item!=NULL))pd_bottom_present(text);}
  else if(!pd_bottom_idle_present())pd_bottom_present(text);
 }
 u64 now=sysGetMicroseconds();if(now-last_report>=5000000){last_report=now;pd_profile_report();pd_bottom_refresh_report();pd_viewmodel_report();sysLogPrintf(0,"Runtime: fps=%.1f lod=%d LOS=%u cached=%u",(double)fps,lod,ray_calls,ray_hits);ray_calls=ray_hits=0;}
}
static unsigned profile_total[15],profile_peak[15],profile_count[15];
void pd_profile_record(unsigned stage,uint64_t elapsed){
 if(stage>=15)return;
 unsigned us=elapsed>0xffffffffu?0xffffffffu:(unsigned)elapsed;
 __atomic_fetch_add(&profile_total[stage],us,__ATOMIC_RELAXED);
 __atomic_fetch_add(&profile_count[stage],1,__ATOMIC_RELAXED);
 unsigned old=__atomic_load_n(&profile_peak[stage],__ATOMIC_RELAXED);
 while(old<us && !__atomic_compare_exchange_n(&profile_peak[stage],&old,us,0,__ATOMIC_RELAXED,__ATOMIC_RELAXED)){}
}
void pd_profile_report(void){
 static const char *names[]={"main","gfx-wait","vi-wait","simulation","scene-build","render-idle","fps-wait","gpu-wait","submit","stage-load","audio","interpret","bottom-copy","bottom-draw","bottom-frame"};
 for(unsigned i=0;i<15;i++){
  unsigned count=__atomic_exchange_n(&profile_count[i],0,__ATOMIC_RELAXED),total=__atomic_exchange_n(&profile_total[i],0,__ATOMIC_RELAXED),peak=__atomic_exchange_n(&profile_peak[i],0,__ATOMIC_RELAXED);
  if(count)sysLogPrintf(0,"Profile %s: calls=%u mean=%.2fms peak=%.2fms",names[i],count,(double)total/count/1000.,(double)peak/1000.);
 }
}

struct RayCache {struct chrdata *chr;struct prop *target;struct coord from,to;int stage,frame,value;};
static struct RayCache rays[256];
static struct RayCache *ray_slot(struct chrdata *chr){return &rays[((uintptr_t)chr>>4)%256];}
static float move_sq(struct coord *a,struct coord *b){float x=a->x-b->x,y=a->y-b->y,z=a->z-b->z;return x*x+y*y+z*z;}
int pd_guard_los_lookup(struct chrdata *chr,struct prop *target,int *value){
 if(!chr || !chr->prop || !target || g_Vars.in_cutscene || g_Vars.normmplayerisrunning)return 0;
 struct RayCache *cache=ray_slot(chr);int period=values[4]==1?2:values[4]==2?4:values[4]==3?12:0;
 if(chr->alertness>=65)period=0;
 if(values[5] && chr->alertness<65 && !(chr->prop->flags&(PROPFLAG_ONANYSCREENTHISTICK|PROPFLAG_ONANYSCREENPREVTICK)) && !cache->value)period=12;
 if(cache->chr==chr && cache->target==target && cache->stage==g_StageNum &&
    g_Vars.lvframe60>=cache->frame && g_Vars.lvframe60-cache->frame<=period &&
    move_sq(&cache->from,&chr->prop->pos)<256.f && move_sq(&cache->to,&target->pos)<256.f){
  *value=cache->value;pd_guard_ray_count(1);return 1;
 }
 pd_guard_ray_count(0);return 0;
}
void pd_guard_los_store(struct chrdata *chr,struct prop *target,int value){
 if(!chr || !chr->prop || !target)return;
 struct RayCache *cache=ray_slot(chr);cache->chr=chr;cache->target=target;cache->from=chr->prop->pos;cache->to=target->pos;cache->stage=g_StageNum;cache->frame=g_Vars.lvframe60;cache->value=value;
}
int pd_effect_visible(struct coord *pos,unsigned index){
 if(!values[3] || !g_Vars.currentplayer || g_Vars.in_cutscene || g_Vars.currentplayer->visionmode!=VISIONMODE_NORMAL)return 1;
 if(!envIsPosInFogMaxDistance(pos,200.f))return 0;
 float distance=move_sq(pos,&g_Vars.currentplayer->cam_pos);
 unsigned stride=distance>16000000.f?4:distance>4000000.f?2:1;
 if(values[3]==1 && stride>2)stride=2;
 if(values[3]>=3 && stride>1)stride*=2;
 return index%stride==0;
}
static float original_near,original_far;
float pd_adjust_far(float near,float far){original_near=near;original_far=far;if(!values[2] || far<=near)return far;float scale=values[2]==1?.5f:values[2]==2?.25f:values[2]==3?.1f:1.f;float result=far*scale;return result>near+1?result:near+1;}
void pd_update_view_distance(void){static int previous=-1;if(previous!=values[2] && original_far>original_near){previous=values[2];viSetZRange(original_near,original_far);envTick();}}



static struct {struct chrdata *chr;int frame,stage;} ai_clocks[256];
void pd_runtime_reset(void){pd_bottom_objectives_reset();memset(rays,0,sizeof rays);memset(ai_clocks,0,sizeof ai_clocks);touch_dialog=NULL;touch_count=0;mission_item=NULL;original_near=original_far=0;}
int pd_guard_ai_tick(struct chrdata *chr){
 if(!values[17] || !chr || !chr->prop || !g_Vars.currentplayer || g_Vars.in_cutscene || g_Vars.normmplayerisrunning ||
    chr->team!=TEAM_ENEMY || CHRRACE(chr)!=RACE_HUMAN || chr->alertness>=65 || (chr->chrflags&CHRCFLAG_NEVERSLEEP) ||
    (chr->actiontype!=ACT_STAND && chr->actiontype!=ACT_PATROL) ||
    (chr->prop->flags&(PROPFLAG_ONANYSCREENTHISTICK|PROPFLAG_ONANYSCREENPREVTICK)) ||
    move_sq(&chr->prop->pos,&g_Vars.currentplayer->cam_pos)<4000000.f)return 1;
 unsigned slot=((uintptr_t)chr>>4)%256;int period=values[17]==1?2:values[17]==2?4:12;
 if(ai_clocks[slot].chr!=chr || ai_clocks[slot].stage!=g_StageNum || g_Vars.lvframe60<ai_clocks[slot].frame || g_Vars.lvframe60-ai_clocks[slot].frame>=period){
  ai_clocks[slot].chr=chr;ai_clocks[slot].stage=g_StageNum;ai_clocks[slot].frame=g_Vars.lvframe60;return 1;
 }
 return 0;
}

/* Menu animation follows elapsed time even when gameplay ticks are paused. */
float pd_menu_spin_angle(void){
 return (sysGetMicroseconds()%20000000ULL)*(18.84955592153876f/20000000.f);
}
