#include "../../src/runtime_3ds.c"
#include <assert.h>
#include <stdlib.h>
#include "pd_cutscene_bars.h"
struct g_vars g_Vars;
struct menu g_Menus[MAX_PLAYERS];
s32 g_StageNum=STAGE_DEFECTION;
static int visible=1;
void pd_bottom_objectives_reset(void){}
int pd_bottom_native_touch(int x,int y){return -1;}
static int objective_page;
int pd_bottom_objectives_visible(void){return objective_page;}
int pd_bottom_objective_indices(int *p,int n){return 1;}
static struct {const char *key;s32 *value,min,max;} registered[64];static unsigned registered_count;
void configRegisterInt(const char*k,s32*v,s32 min,s32 max){assert(registered_count<64);registered[registered_count++]=(__typeof__(*registered)){k,v,min,max};}
s32 configSave(const char *p){(void)p;return 0;}
bool envIsPosInFogMaxDistance(struct coord *p,f32 t){(void)p;(void)t;return visible;}
int main(void){
 assert(registered_count==45);for(unsigned i=0;i<registered_count;i++){assert(registered[i].key && registered[i].value==&values[i] && registered[i].max==maxima[i]);}
 assert(pd_option_get(33)==pd_option_get(26));
 pd_option_set(26,7);assert(pd_option_get(33)==7);pd_option_set(33,2);assert(pd_option_get(33)==2 && pd_option_get(26)==7);pd_option_set(26,3);
 assert(pd_option_get(34)==1 && pd_option_get(35)==9 && pd_option_get(36)==4 && pd_option_get(37)==15 && pd_option_get(39)==10);
 const int maximum[]={1,15,1,19,10,20,1,10,20,7,11,1,7};
 for(int key=32;key<45;key++){int saved=pd_option_get(key);for(int i=0;i<=maximum[key-32];i++){pd_option_set(key,i);assert(pd_option_get(key)==i);}pd_option_set(key,999);assert(pd_option_get(key)==maximum[key-32]);pd_option_set(key,saved);}

 assert(pd_option_get(20)==3 && pd_option_get(21)==3 && pd_option_get(25)==4);
 assert(pd_option_get(30)==1 && pd_option_get(28)==0 && pd_option_get(29)==0 && pd_option_get(31)==0);
 for(int key=20;key<=21;key++){pd_option_set(key,15);assert(pd_option_get(key)==15);pd_option_set(key,3);}
 pd_option_set(25,8);assert(pd_option_get(25)==8);pd_option_set(25,4);
 for(int key=28;key<=31;key++){pd_option_set(key,1);assert(pd_option_get(key)==1);pd_option_set(key,key==30?1:0);}

 assert(pd_option_get(19)==0 && pd_option_get(20)==3 && pd_option_get(21)==3);
 pd_option_set(21,7);assert(pd_option_get(21)==7);pd_option_set(21,3);
 pd_option_set(19,2);pd_option_set(20,7);assert(pd_option_get(19)==2 && pd_option_get(20)==7);
 pd_option_set(19,0);pd_option_set(20,3);
 assert(pd_option_get(18)==0);
 pd_option_set(18,99);assert(pd_option_get(18)==2);
 pd_option_set(18,-1);assert(pd_option_get(18)==0);
 for(int t=0;t<=10;t++){
  float tween=t/10.f;
  struct pd_cutscene_frame a=pd_cutscene_frame(0,10,220,10,180,136,tween);
  assert(a.top==10 && a.height==180);
  a=pd_cutscene_frame(2,10,220,10,180,136,tween);
  assert(a.top==10 && a.height==220);
  a=pd_cutscene_frame(1,10,220,10,180,136,tween);
  assert(a.height>=136 && a.height<=220);
  assert(a.top>=10 && a.top+a.height<=230);
  assert(abs((a.top-10)-(230-a.top-a.height))<=1);
 }
 struct pd_cutscene_frame end=pd_cutscene_frame(1,10,220,10,180,136,1);
 assert(end.top==10 && end.height==220);
 static struct chrdata guard;static struct prop source,target;int value;
 guard.prop=&source;g_Vars.lvframe60=10;
 assert(!pd_guard_los_lookup(&guard,&target,&value));pd_guard_los_store(&guard,&target,1);
 assert(pd_guard_los_lookup(&guard,&target,&value) && value==1);
 g_Vars.lvframe60++;assert(!pd_guard_los_lookup(&guard,&target,&value));
 pd_option_set(4,3);assert(pd_guard_los_lookup(&guard,&target,&value));
 target.pos.x=32;assert(!pd_guard_los_lookup(&guard,&target,&value));target.pos.x=0;
 guard.alertness=99;assert(!pd_guard_los_lookup(&guard,&target,&value));guard.alertness=0;
 g_Vars.in_cutscene=1;assert(!pd_guard_los_lookup(&guard,&target,&value));g_Vars.in_cutscene=0;
 g_Vars.lvframe60=0;assert(!pd_guard_los_lookup(&guard,&target,&value));g_Vars.lvframe60=10;g_StageNum++;assert(!pd_guard_los_lookup(&guard,&target,&value));
 static struct player player;g_Vars.currentplayer=&player;player.visionmode=VISIONMODE_NORMAL;
 struct coord position={0};pd_option_set(3,1);visible=0;assert(!pd_effect_visible(&position,0));visible=1;assert(pd_effect_visible(&position,0));position.x=3000;assert(!pd_effect_visible(&position,1) && pd_effect_visible(&position,2));
 pd_option_set(3,0);visible=0;assert(pd_effect_visible(&position,1));
 static struct menudialog dialog;static struct menuitem item;g_Menus[0].curdialog=&dialog;touch_dialog=&dialog;touch_count=1;touch_items[0]=&item;
 assert(pd_bottom_touch(50,30)==1 && dialog.focuseditem==&item);assert(pd_bottom_touch(10,220)==2 && pd_bottom_touch(300,220)==1);
 assert(pd_bottom_touch(5,60)==3 && pd_bottom_touch(315,60)==4);
 static struct menuitem status_items[3];static struct menudialogdef status_definition;
 status_items[0].type=MENUITEMTYPE_OBJECTIVES;status_items[1].type=MENUITEMTYPE_SELECTABLE;status_items[2].type=MENUITEMTYPE_END;
 status_definition.items=status_items;dialog.definition=&status_definition;objective_page=1;
 assert(pd_bottom_touch(30,74)==1 && dialog.focuseditem==&status_items[1]);
 assert(pd_bottom_touch(5,60)==3 && pd_bottom_touch(315,60)==4);
 assert(pd_bottom_touch(10,220)==2 && pd_bottom_touch(300,220)==1);
 objective_page=0;
 dialog.dimmed=true;assert(pd_bottom_touch(100,220)==5 && pd_bottom_touch(200,220)==6);assert(!pd_touch_look_allowed());g_Menus[0].curdialog=NULL;assert(pd_touch_look_allowed());
 pd_option_set(17,2);guard.team=TEAM_ENEMY;guard.actiontype=ACT_STAND;source.pos.x=3000;g_Vars.lvframe60=20;guard.race=RACE_HUMAN;
 assert(pd_guard_ai_tick(&guard));g_Vars.lvframe60=21;assert(!pd_guard_ai_tick(&guard));g_Vars.lvframe60=24;assert(pd_guard_ai_tick(&guard));
 guard.alertness=100;g_Vars.lvframe60=25;assert(pd_guard_ai_tick(&guard));guard.alertness=0;source.flags=PROPFLAG_ONANYSCREENTHISTICK;assert(pd_guard_ai_tick(&guard));source.flags=0;
 guard.team=TEAM_ALLY;assert(pd_guard_ai_tick(&guard));guard.team=TEAM_ENEMY;pd_runtime_reset();assert(pd_guard_ai_tick(&guard));
 pd_option_set(2,0);assert(pd_adjust_far(100,10000)==10000 && pd_adjust_far(0,0)==0);
 pd_option_set(2,1);assert(pd_adjust_far(100,10000)==5000);
 pd_option_set(2,2);assert(pd_adjust_far(100,10000)==2500 && pd_adjust_far(100,10000)==2500);
 pd_option_set(2,3);assert(pd_adjust_far(100,10000)==1000);
 assert(pd_adjust_far(100,500)==101 && pd_adjust_far(100,100)==100);
 pd_option_set(2,0);assert(pd_adjust_far(100,10000)==10000);
 puts("PASS: LOS expiry/movement/alert/cutscene/stage guards, particle fog/LOD policy, touch menu/footer and look isolation");
}
