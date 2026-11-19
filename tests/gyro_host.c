#include "../src/gyro_3ds.c"
#include <assert.h>
#include <stdio.h>
struct g_vars g_Vars;
struct extplayerconfig g_PlayerExtCfg[MAX_PLAYERS];
static struct player player;
static struct playerstats stats;
static int mode=2,gain=3,gameplay=1,tx,ty,classic,lock,sway=1,scope;static float flick;static float yaw=2,pitch=1;
int pd_touch_look_allowed(void){return gameplay;}
int pd_option_get(int key){return key==19?mode:key==20?gain:key==21?3:key==28?classic:key==29?lock:key==30?sway:key==31?scope:key==24?1:0;}
void pd_hid_take_flick(float *x){*x=flick;flick=0;}
int pd_touch_look(int *x,int *y){*x=tx;*y=ty;tx=ty=0;return 0;}
void pd_hid_take_gyro(float *x,float *y){*x=yaw;*y=pitch;yaw=pitch=0;}
int main(void){
 g_Vars.currentplayer=&player;g_Vars.currentplayerstats=&stats;g_Vars.lvupdate240=4;g_Vars.tickmode=TICKMODE_NORMAL;
 player.pausemode=PAUSEMODE_UNPAUSED;player.fovy=60;player.aspect=400.f/240;
 g_PlayerExtCfg[0].mouseaimspeedx=g_PlayerExtCfg[0].mouseaimspeedy=1;
 struct movedata m={0};assert(pd_gyro_allowed());pd_gyro_movedata(&m);assert(m.freelookdx<0 && m.freelookdy<0 && yaw==0);
 float x=m.freelookdx,y=m.freelookdy;pd_gyro_movedata(&m);assert(m.freelookdx==x && m.freelookdy==y);
 mode=1;yaw=2;pitch=1;m=(struct movedata){0};pd_gyro_movedata(&m);assert(m.freelookdx==0 && m.freelookdy==0);
 player.insightaimmode=1;yaw=2;pitch=1;pd_gyro_movedata(&m);assert(m.freelookdx<0 && m.freelookdy>0);
 gain=7;yaw=2;pitch=1;struct movedata high={0};pd_gyro_movedata(&high);assert(__builtin_fabsf(high.freelookdx/m.freelookdx-2)<.001f);
 yaw=2;pitch=1;high=(struct movedata){0};high.invertpitch=1;pd_gyro_movedata(&high);assert(high.freelookdy<0);
 player.isdead=1;yaw=2;pitch=1;high=(struct movedata){0};pd_gyro_movedata(&high);assert(high.freelookdx==0 && yaw==0);player.isdead=0;
 gameplay=0;assert(!pd_gyro_allowed());gameplay=1;g_Vars.currentplayernum=1;assert(!pd_gyro_allowed());g_Vars.currentplayernum=0;
 g_Vars.lvupdate240=0;assert(!pd_gyro_allowed());g_Vars.lvupdate240=4;mode=0;assert(!pd_gyro_allowed());
 player.insightaimmode=1;mode=0;tx=12;ty=8;m=(struct movedata){0};pd_gyro_movedata(&m);
 assert(m.freelookdx>0 && m.freelookdy>0 && tx==0 && ty==0);
 flick=90;player.vv_theta=350;pd_gyro_movedata(&m);assert(__builtin_fabsf(player.vv_theta-80)<.001f);
 classic=1;player.bondmovemode=MOVEMODE_WALK;m=(struct movedata){0};m.analogwalk=70;m.analogstrafe=70;
 pd_controls_classic(&m,-80,-80,0);assert(m.analogwalk==0 && m.analogstrafe==0 && m.analoglean==-1 && m.crouchdown);
 m=(struct movedata){0};pd_controls_classic(&m,80,80,1);assert(m.analoglean==1 && m.zoominfovpersec>0 && !m.crouchup);
 assert(pd_controls_breathing(.5f)==.5f);sway=0;assert(pd_controls_breathing(.5f)==0);
 scope=1;player.fovy=20;player.headlook.x=1;player.headlook.z=.5f;player.headup.y=.5f;
 pd_controls_scope_head();assert(player.headlook.x==0 && player.headlook.z==1 && player.headup.y==1);
 lock=1;mode=2;gain=15;yaw=2;pitch=1;player.vv_theta=80;player.vv_verta=0;m=(struct movedata){0};
 pd_gyro_movedata(&m);assert(player.vv_theta<80 && player.vv_verta>0 && m.freelookdx==0 && m.freelookdy==0);
 /* Gyro offset must coexist with the stock stick circle, rather than
  * selecting the PC mouse-only branch and dropping the stick. */
 lock=0;player.fovy=60;player.swivelpos[0]=player.swivelpos[1]=0;
 aim_motion_active=0;x=y=0;pd_controls_aim_reticle(&x,&y,.1f,-.05f);
 assert(fabsf(x-.1f)<.00001f && fabsf(y+.05f)<.00001f);
 x=.3f;y=.2f;pd_controls_aim_reticle(&x,&y,0,0);
 assert(fabsf(x-.4f)<.00001f && fabsf(y-.15f)<.00001f);
 x=-.3f;y=-.2f;pd_controls_aim_reticle(&x,&y,0,0);
 assert(fabsf(x+.2f)<.00001f && fabsf(y+.25f)<.00001f);
 player.insightaimmode=0;m=(struct movedata){0};pd_gyro_movedata(&m);assert(!aim_motion_active);
 player.insightaimmode=1;player.swivelpos[0]=player.swivelpos[1]=0;
 float theta=player.vv_theta;x=y=0;pd_controls_aim_reticle(&x,&y,1,0);
 assert(x==.65f && player.vv_theta!=theta);
 theta=player.vv_theta;x=y=0;pd_controls_aim_reticle(&x,&y,0,0);
 assert(fabsf(player.vv_theta-theta)<.00001f);
 puts("PASS PD gyro: modes, independent gain, original mouse/reticle units, inversion compensation, consume-once and menu/death/pause/player gating");
}
