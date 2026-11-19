extern void pd_probe_controls_config(void);
#include <ultra64.h>
#include "constants.h"
#include "data.h"
#include "types.h"
extern struct g_vars g_Vars;
extern s32 g_StageNum;
extern struct vimode g_ViModes[];
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "game/player.h"
#include "lib/vi.h"
#include "game/mainmenu.h"
#include "game/menu.h"
#include "game/menuitem.h"
#include "game/game_1531a0.h"
#include "game/savebuffer.h"
#include "pd_objectives.h"
extern struct menudialogdef g_SoloMissionPauseMenuDialog;
#include "bss.h"
#include "input.h"
#ifdef __3DS__
#include "pd_runtime.h"
#endif
static int pd_probe_stage(void){const char *s=getenv("PD_PROBE_STAGE");return s?(int)strtoul(s,NULL,0):STAGE_DEFECTION;}
extern char *__real_langGet(s32);
char *__wrap_langGet(s32 id){
 if(getenv("PD_PROBE_LONG_BRIEF") && g_StageNum==pd_probe_stage() && g_Vars.lvframenum>=520 && id && id==g_Briefing.briefingtextnum){
  static char stress[12000];
  if(!stress[0]){
   strcpy(stress,"|Background\n\n");
   while(strlen(stress)<11000)strcat(stress,"A long mission description with SupercalifragilisticexpialidociousExtendedUnbrokenText and normal words. ");
  }
  return stress;
 }
 return __real_langGet(id);
}
static unsigned mission_gameplay_frames,mission_aim_frames;
static unsigned top_native_passes;
static struct menudialog top_original_dialog;
static struct rend_vidat top_original_vi;
static struct menucolumn top_original_columns[12];
static struct menurow top_original_rows[88];
extern struct rend_vidat *g_ViBackData;
int __wrap_videoGetDisplayModeIndex(void){return getenv("PD_PROBE_TOP_4_3")?1:0;}
void pd_top_menu_layout_report(int w,int h,int x,int y,int dw,int dh,int phase){
 struct menu *menu=&g_Menus[g_MpPlayerNum];
 if(phase==2){
  top_original_vi=*g_ViBackData;
  /* During a swipe the rendered dialog need not be curdialog. The dimensions
   * passed in still let the test prove native projection and balanced restore. */
  memset(&top_original_dialog,0,sizeof top_original_dialog);
  top_original_dialog.x=x;top_original_dialog.y=y;top_original_dialog.width=dw;top_original_dialog.height=dh;
  memcpy(top_original_columns,menu->cols,sizeof top_original_columns);
  memcpy(top_original_rows,menu->rows,sizeof top_original_rows);
 }else if(phase==0){
  assert(pd_top_ui_drawing && h==240 && w==(getenv("PD_PROBE_TOP_4_3")?320:400));
  assert(g_ViBackData->x==w && g_ViBackData->y==h && g_ViBackData->viewx==w && g_ViBackData->viewy==h);
  assert(!memcmp(top_original_rows,menu->rows,sizeof top_original_rows));
  Gfx font_setup[32];Gfx *end=text0f153628(font_setup);assert(end[-1].words.w1==G_TF_POINT);
  top_native_passes++;
 }else{
  assert(!pd_top_ui_drawing && !memcmp(&top_original_vi,g_ViBackData,sizeof top_original_vi));
  assert(x==top_original_dialog.x && y==top_original_dialog.y && dw==top_original_dialog.width && dh==top_original_dialog.height);
  assert(!memcmp(top_original_columns,menu->cols,sizeof top_original_columns));
  assert(!memcmp(top_original_rows,menu->rows,sizeof top_original_rows));
 }
}
static void plane_growth_test(void){
 extern Gfx *var800a4638;
 Gfx *saved=var800a4634,*saved_end=var800a4638;
 int clip1=var8009de90,clip2=var8009de94;
 Gfx storage[16];memset(storage,0xa5,sizeof storage);
 var800a4634=storage;var800a4638=storage+8;var8009de90=-1000;var8009de94=1000;
 for(int i=0;i<1000;i++)var800a4634=menugfxDrawPlane(var800a4634,10,10,300,10,0xffffff7f,0xffffff7f,MENUPLANE_00);
 assert(var800a4638!=storage+8 && var800a4638-var800a4634>=2);
 for(unsigned i=8*sizeof(Gfx);i<sizeof storage;i++)assert(((unsigned char *)storage)[i]==0xa5);
 var800a4634=saved;var800a4638=saved_end;var8009de90=clip1;var8009de94=clip2;
 fprintf(stderr,"DEFERRED_PLANE_GROWTH_PASS 1000 planes, protected end commands and canaries\n");
}
static char keyboard_saved[32];static int keyboard_commits;
static MenuItemHandlerResult keyboard_handler(s32 op,struct menuitem *item,union handlerdata *data){
 (void)item;
 if(op==MENUOP_GETTEXT)data->keyboard.string[0]=0;
 if(op==MENUOP_SETTEXT)snprintf(keyboard_saved,sizeof keyboard_saved,"%s",data->keyboard.string);
 if(op==MENUOP_SET)keyboard_commits++;
 return 0;
}
static struct menuitem keyboard_items[]={ {MENUITEMTYPE_KEYBOARD,10,0,0,1,keyboard_handler},{MENUITEMTYPE_END} };
static struct menudialogdef keyboard_dialog={MENUDIALOGTYPE_DEFAULT,(uintptr_t)"Touch keyboard",keyboard_items,NULL,MENUDIALOGFLAG_LITERAL_TEXT,NULL};
static void keyboard_test(void){
 struct menu *menu=&g_Menus[0];struct menudialog *dialog=menu->curdialog;
 assert(dialog && dialog->definition==&keyboard_dialog);
 Gfx font_setup[32];Gfx *font_end=text0f153628(font_setup);
 assert(font_end>font_setup && font_end[-1].words.w1==TEX_FILTER_2D);
 pd_bottom_ui_drawing=1;font_end=text0f153628(font_setup);pd_bottom_ui_drawing=0;
 assert(font_end[-1].words.w1==G_TF_POINT);
 fprintf(stderr,"MENU_FONT_FILTER_PASS smooth top scaling, native bottom sampling\n");
 union menuitemdata *data=(union menuitemdata *)&menu->blocks[menu->rows[menu->cols[dialog->colstart].rowstart].blockindex];
 int top=-1;
 for(int y=25;y<200;y++)if(pd_bottom_native_touch(34,y)==1){top=y;break;}
 assert(top>=40 && top+150<=232);
 for(int row=0;row<5;row++)for(int col=0;col<10;col++){
  int canonical=col;
  if(row==4)canonical=col<2?0:col<5?2:col<8?5:8;
  assert(pd_bottom_native_touch(20+col*28+14,top+row*30+15)==1);
  assert(data->keyboard.row==row && data->keyboard.col==canonical);
 }
 struct menuinputs inputs={0};data->keyboard.string[0]=0;
 assert(pd_bottom_native_touch(34,top+15)==1);inputs.select=1;
 menuitemKeyboardTick(&keyboard_items[0],&inputs,MENUTICKFLAG_ITEMISFOCUSED,data);assert(strlen(data->keyboard.string)==1);
 assert(pd_bottom_native_touch(34,top+135)==1);inputs=(struct menuinputs){0};inputs.select=1;
 menuitemKeyboardTick(&keyboard_items[0],&inputs,MENUTICKFLAG_ITEMISFOCUSED,data);assert(!data->keyboard.string[0]);
 assert(pd_bottom_native_touch(90,top+135)==1);inputs=(struct menuinputs){0};inputs.select=1;
 menuitemKeyboardTick(&keyboard_items[0],&inputs,MENUTICKFLAG_ITEMISFOCUSED,data);assert(data->keyboard.capslock);
 assert(pd_bottom_native_touch(34,top+15)==1);inputs=(struct menuinputs){0};inputs.select=1;
 menuitemKeyboardTick(&keyboard_items[0],&inputs,MENUTICKFLAG_ITEMISFOCUSED,data);assert(strlen(data->keyboard.string)==1);
 assert(pd_bottom_native_touch(286,top+135)==1);inputs=(struct menuinputs){0};inputs.select=1;
 menuitemKeyboardTick(&keyboard_items[0],&inputs,MENUTICKFLAG_ITEMISFOCUSED,data);
 assert(keyboard_commits==1 && strlen(keyboard_saved)==1 && g_Menus[0].curdialog!=dialog);
 fprintf(stderr,"TOUCH_KEYBOARD_PASS 50 cell centres, 28x30 keys, typing/delete/caps/commit; top=%d\n",top);
}
static unsigned menu_stress_opens,menu_stress_swipes,menu_stress_closes;
extern struct menudialogdef g_SoloMissionEndscreenCompletedMenuDialog,g_SoloMissionEndscreenFailedMenuDialog;
extern void __real_videoEndFrame(void);
void __wrap_videoEndFrame(void){
 __real_videoEndFrame();
#ifdef __3DS__
 static int initialized;
 extern void pd_probe_texture_eviction_test(void);
 extern void pd_probe_texture_memory_test(void);
 if(!initialized){if(getenv("PD_PROBE_TEXTURE_MEMORY"))pd_probe_texture_memory_test();if(getenv("PD_PROBE_PLANE_GROWTH"))plane_growth_test();if(getenv("PD_PROBE_CACHE_STRESS"))pd_probe_texture_eviction_test();pd_probe_controls_config();initialized=1;pd_option_set(13,getenv("PD_PROBE_GPU")!=NULL);pd_option_set(14,1);}
 /* Exercise both overrides through the real viewport getters during a
  * single-player cinematic, restoring the user's selected value afterwards. */
 static int bar_checks;
 if(!bar_checks && g_InCutscene && !g_MainIsEndscreen && !var8009dfc0 && PLAYERCOUNT()==1){
  int saved=pd_option_get(18),original_h=playerGetViewportHeight(),original_y=playerGetViewportTop();
  pd_option_set(18,2);
  assert(playerGetViewportHeight()==g_ViModes[g_ViRes].fullheight);
  assert(playerGetViewportTop()==g_ViModes[g_ViRes].fulltop);
  pd_option_set(18,1);
  int h=playerGetViewportHeight(),y=playerGetViewportTop();
  assert(h>=g_ViModes[g_ViRes].cinemaheight && h<=g_ViModes[g_ViRes].fullheight);
  assert(abs((y-g_ViModes[g_ViRes].fulltop)-(g_ViModes[g_ViRes].fullheight-h)/2)<=1);
  pd_option_set(18,saved);
  assert(playerGetViewportHeight()==original_h && playerGetViewportTop()==original_y);
  fprintf(stderr,"CUTSCENE_BARS_PASS original restored; larger framing bounded; off fullscreen\n");bar_checks=1;
 }
 pd_runtime_frame();
#endif
 static unsigned frames;
 if(getenv("PD_PROBE_CUSTOM") && g_StageNum==pd_probe_stage()){
  pd_option_set(41,1+(frames/20)%7);pd_option_set(42,(frames/10)%12);pd_option_set(44,(frames/30)%8);pd_option_set(43,1);
 }
 if(getenv("PD_PROBE_GYRO") && g_StageNum==pd_probe_stage())pd_option_set(19,2);
 if(g_StageNum==pd_probe_stage() && !g_Vars.in_cutscene && !g_Menus[0].curdialog && g_Vars.currentplayer){mission_gameplay_frames++;mission_aim_frames+=g_Vars.currentplayer->insightaimmode!=0;}
 if(getenv("PD_PROBE_GAMEPLAY") && g_StageNum==pd_probe_stage() && g_Vars.lvframenum>=120 && g_Vars.in_cutscene)
  g_CutsceneSkipRequested=true; // Let the original stage script perform its normal skip.
 if(getenv("PD_PROBE_UI") && g_StageNum==pd_probe_stage() && frames==450 && !g_Menus[0].curdialog){
  menuPushRootDialog(&g_SoloMissionPauseMenuDialog,MENUROOT_MAINMENU);
  g_Vars.currentplayer->menuisactive=true;
 }
 if(getenv("PD_PROBE_BRIEFING") && g_StageNum==pd_probe_stage() && frames==520){
  extern struct menudialogdef g_SoloMissionBriefingMenuDialog;
  menuPushDialog(&g_SoloMissionBriefingMenuDialog);
 }
 if(getenv("PD_PROBE_KEYBOARD") && g_StageNum==pd_probe_stage() && frames==520)menuPushDialog(&keyboard_dialog);
 if(getenv("PD_PROBE_KEYBOARD") && g_StageNum==pd_probe_stage() && frames==650)keyboard_test();
 if(getenv("PD_PROBE_INVENTORY") && g_StageNum==pd_probe_stage() && frames==520){
  extern u8 g_InventoryWeapon;
  menuSwipe(1);g_InventoryWeapon=WEAPON_FALCON2;
 }
 if(getenv("PD_PROBE_INVENTORY") && g_StageNum==pd_probe_stage() && frames==650){
  extern struct menudialogdef g_SoloMissionInventoryMenuDialog;
  fprintf(stderr,"INVENTORY_STATE dialog=%d model=%p params=%x rows=%d\n",
   g_Menus[0].curdialog && g_Menus[0].curdialog->definition==&g_SoloMissionInventoryMenuDialog,
   (void *)g_Menus[0].menumodel.bodymodeldef,g_Menus[0].menumodel.curparams,
   g_Menus[0].curdialog?g_Menus[0].curdialog->numcols:0);
 }
 if(getenv("PD_PROBE_MENU_STRESS") && g_StageNum==pd_probe_stage() && g_Vars.lvframenum>=840){
  unsigned phase=g_Vars.lvframenum%120;
  if(phase==0 && !g_Menus[0].curdialog){menuPushDialog(&g_SoloMissionPauseMenuDialog);menu_stress_opens++;}
  if(phase==40 && g_Menus[0].curdialog){menuSwipe(1);menu_stress_swipes++;}
  if(phase==80 && g_Menus[0].curdialog){menuPopDialog();menu_stress_closes++;}
 }
 if(getenv("PD_PROBE_RESULTS") && g_StageNum==pd_probe_stage() && g_Vars.lvframenum==900){
  while(g_Menus[0].curdialog)menuPopDialog();
  g_MissionConfig.stagenum=pd_probe_stage();
  for(int i=0;i<NUM_SOLOSTAGES;i++)if(g_SoloStages[i].stagenum==g_MissionConfig.stagenum)g_MissionConfig.stageindex=i;
  g_MainIsEndscreen=true;
  int success=!strcmp(getenv("PD_PROBE_RESULTS"),"success");
  menuPushRootDialog(success?&g_SoloMissionEndscreenCompletedMenuDialog:&g_SoloMissionEndscreenFailedMenuDialog,MENUROOT_ENDSCREEN);
  g_Vars.currentplayer->menuisactive=true;
  g_MenuData.nextbg=success?MENUBG_SUCCESS:MENUBG_FAILURE;
  fprintf(stderr,"RESULT_SCREEN_OPENED %s\n",success?"success":"failure");
 }
 static unsigned decline_grace;
 if(decline_grace && ++decline_grace>=120){
  fprintf(stderr,"DEFECTION_PRESENTED frames=%u FAILED_MISSION_DECLINE stable for 120 frames\n",frames);exit(0);
 }
 if(getenv("PD_PROBE_DECLINE") && g_StageNum==pd_probe_stage()){
  if(g_Vars.lvframenum==970){
   extern struct menudialogdef g_RetryMissionMenuDialog;
   extern void endscreenResetModels(void);
   endscreenResetModels();menuPushDialog(&g_RetryMissionMenuDialog);
  }
  if(g_Vars.lvframenum==1030){
   extern MenuItemHandlerResult endscreenHandleDeclineMission(s32,struct menuitem *,union handlerdata *);
   endscreenHandleDeclineMission(MENUOP_SET,NULL,NULL);
   decline_grace=1;int count=g_MenuData.count;
   menuPopDialog();menuPopDialog();
   assert(g_MenuData.count==count);
   fprintf(stderr,"FAILED_MISSION_DECLINE returned without duplicate root close\n");
  }
 }
 static int launched_mission;
 if(getenv("PD_PROBE_TITLE") && !launched_mission){
  static unsigned menu_frames;
  ++frames;
  if(g_StageNum==STAGE_CITRAINING && getenv("PD_PROBE_CI_IDLE") && menu_frames==60){
   for(int i=0;i<10 && g_Menus[0].curdialog;i++)menuPopDialog();
   g_Vars.currentplayer->menuisactive=false;
  }
  if(g_StageNum==STAGE_CITRAINING && ++menu_frames==120){
   if(getenv("PD_PROBE_CI_IDLE")){
    if(g_Menus[0].curdialog){fprintf(stderr,"CI menu did not close\n");exit(2);}
    fprintf(stderr,"CI_IDLE_WINDOW verified after closing menus\n");
   }
   fprintf(stderr,"TITLE_MENU_PRESENTED frames=%u menu_frames=%u stage=%d\n",frames,menu_frames,g_StageNum);fflush(stderr);
   if(getenv("PD_PROBE_LAUNCH")){
    g_MissionConfig.stagenum=pd_probe_stage();g_MissionConfig.stageindex=0;
    for(int i=0;i<NUM_SOLOSTAGES;i++)if(g_SoloStages[i].stagenum==g_MissionConfig.stagenum)g_MissionConfig.stageindex=i;g_MissionConfig.difficulty=DIFF_A;
    g_MissionConfig.iscoop=g_MissionConfig.isanti=false;
    menuhandlerAcceptMission(MENUOP_SET,NULL,NULL);
    launched_mission=1;frames=0;return;
   }
   exit(0);
  }
  if(frames>3000){fprintf(stderr,"Title did not reach menu within 3000 frames\n");exit(1);}
  return;
 }
 if(g_StageNum==pd_probe_stage() && g_Vars.lvframenum>0 && ++frames==(getenv("PD_PROBE_FRAMES")?atoi(getenv("PD_PROBE_FRAMES")):120)){
  if(getenv("PD_PROBE_MENU_STRESS")){fprintf(stderr,"MENU_STRESS opens=%u swipes=%u closes=%u\n",menu_stress_opens,menu_stress_swipes,menu_stress_closes);if(menu_stress_opens<5 || menu_stress_closes<5)exit(2);}
  fprintf(stderr,"NATIVE_TOP_MENUS passes=%u size=%dx240 restored logical geometry\n",top_native_passes,getenv("PD_PROBE_TOP_4_3")?320:400);if(getenv("PD_PROBE_UI") || getenv("PD_PROBE_KEYBOARD"))assert(top_native_passes>100);
  fprintf(stderr,"MISSION_INPUT gameplay=%u aiming=%u\n",mission_gameplay_frames,mission_aim_frames);
  if(getenv("PD_PROBE_GAMEPLAY") && (mission_gameplay_frames<120 || mission_aim_frames<1)){fprintf(stderr,"Mission did not exercise gameplay and aiming\n");exit(2);}
  fprintf(stderr,"DEFECTION_PRESENTED frames=%u stage=%d level_frames=%d onscreen_props=%d\n",frames,g_StageNum,g_Vars.lvframenum,g_Vars.numonscreenprops);fflush(stderr);exit(0);
 }
}

extern s32 __real_inputReadController(s32,OSContPad *);
s32 __wrap_inputReadController(s32 index,OSContPad *pad){
 if(!getenv("PD_PROBE_GAMEPLAY") || index || g_StageNum!=pd_probe_stage() || g_Vars.in_cutscene || g_Vars.lvframenum<130 || g_Menus[0].curdialog)
  return __real_inputReadController(index,pad);
 memset(pad,0,sizeof *pad);
 unsigned phase=g_Vars.lvframenum%180;
 pad->stick_x=phase<90?40:-40;pad->stick_y=phase%90<45?15:-15;
 pad->rstick_y=phase<30?20:0;
 if(phase%90<12)pad->button|=CONT_G;
 if(phase%90==50)pad->button|=CONT_EXTRA0;
 if(phase%90>=60)pad->button|=CONT_R;
 return 0;
}
