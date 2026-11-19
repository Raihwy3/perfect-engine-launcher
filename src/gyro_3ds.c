/* Gyro uses the engine's existing frame-independent mouselook/free-reticle path. */
#include <ultra64.h>
#include "constants.h"
#include "types.h"
#include "data.h"
#include "bss.h"
#include "pd_runtime.h"
#include <math.h>
int pd_motion_allowed(void){
 if(!pd_touch_look_allowed() || g_Vars.currentplayernum!=0 ||
    !g_Vars.currentplayer || !g_Vars.currentplayerstats || g_Vars.currentplayer->isdead ||
    !(g_Vars.currentplayer->aspect>0) || !__builtin_isfinite(g_Vars.currentplayer->aspect) ||
    g_Vars.currentplayer->pausemode!=PAUSEMODE_UNPAUSED || g_Vars.tickmode!=TICKMODE_NORMAL || g_Vars.lvupdate240<=0)return 0;
 return 1;
}
int pd_aiming(void){return g_Vars.currentplayer && g_Vars.currentplayer->insightaimmode;}
int pd_gyro_allowed(void){
 return pd_motion_allowed() && (pd_option_get(19)==2 || (pd_option_get(19)==1 && g_Vars.currentplayer->insightaimmode));
}
static void pd_turn_camera(float yaw,float pitch){
 if(!__builtin_isfinite(yaw) || !__builtin_isfinite(pitch))return;
 struct player *p=g_Vars.currentplayer;
 p->vv_theta=__builtin_fmodf(p->vv_theta+yaw+360.f,360.f);
 p->vv_verta+=pitch;if(p->vv_verta>89)p->vv_verta=89;if(p->vv_verta< -89)p->vv_verta=-89;
}
static int aim_motion_active;
static float aim_motion_x,aim_motion_y;
/* Keep stock stick position and accumulated motion separate, as in EoG. */
void pd_controls_aim_reticle(float *x,float *y,float dx,float dy){
 struct player *p=g_Vars.currentplayer;
 if(!aim_motion_active){
  aim_motion_x=p->swivelpos[0]-*x;aim_motion_y=p->swivelpos[1]-*y;
  aim_motion_active=1;
 }
 if(!__builtin_isfinite(aim_motion_x))aim_motion_x=0;
 if(!__builtin_isfinite(aim_motion_y))aim_motion_y=0;
 float tx=*x+aim_motion_x+dx,ty=*y+aim_motion_y+dy;
 float cx=__builtin_fmaxf(-.65f,__builtin_fminf(.65f,tx)),cy=__builtin_fmaxf(-.65f,__builtin_fminf(.65f,ty));
 float fov=p->fovy;if(!(fov>1 && fov<=120))fov=60;
 float vertical=__builtin_tanf(fov*.00872664626f);
 pd_turn_camera((tx-cx)*vertical*p->aspect*57.29577951f,-(ty-cy)*vertical*57.29577951f);
 aim_motion_x=cx-*x;aim_motion_y=cy-*y;
 p->swivelpos[0]=cx;p->swivelpos[1]=cy;*x=cx;*y=cy;
}
void pd_gyro_movedata(struct movedata *move){
 float yaw=0,pitch=0,flick=0;int tx=0,ty=0;
 pd_hid_take_gyro(&yaw,&pitch);pd_hid_take_flick(&flick);pd_touch_look(&tx,&ty);
 if(!pd_motion_allowed()){aim_motion_active=0;return;}
 int aim=g_Vars.currentplayer->insightaimmode,locked=aim && pd_option_get(29);
 if(!aim || locked)aim_motion_active=0;
 if(!aim || locked)aim_motion_active=0;
 float touch_gain=(25+25*pd_option_get(21))*.01f;
 float x=tx*touch_gain/40.f,y=-ty*touch_gain/40.f;
 if(locked){pd_turn_camera(tx*touch_gain*.15f,ty*touch_gain*.15f);x=y=0;}
 else if(aim){
  float sx=PLAYER_EXTCFG().mouseaimspeedx*(320.f/1080.f)/g_Vars.currentplayer->aspect;
  float sy=PLAYER_EXTCFG().mouseaimspeedy*(240.f/1080.f);
  x=sx>0?tx*touch_gain/(160.f*sx):0;y=sy>0?ty*touch_gain/(120.f*sy):0;
 }
 if(move->invertpitch)y=-y;
 move->freelookdx+=x;move->freelookdy+=y;
 if(pd_option_get(24))pd_turn_camera(flick,0);
 if(!pd_gyro_allowed() || !__builtin_isfinite(yaw) || !__builtin_isfinite(pitch))return;
 float gain=(25+25*pd_option_get(20))*.01f;
 float fov=g_Vars.currentplayer->fovy;if(!(fov>1 && fov<=120))fov=60;
 float zoom=fov<60?fov/60:1;
 if(locked){pd_turn_camera(-yaw*gain*zoom,pitch*gain*zoom);return;}
 x=-yaw*gain/3.5f;y=-pitch*gain/3.5f;
 if(aim){
  float vertical=__builtin_tanf(fov*.00872664626f);
  float sx=PLAYER_EXTCFG().mouseaimspeedx*(320.f/1080.f)/g_Vars.currentplayer->aspect;
  float sy=PLAYER_EXTCFG().mouseaimspeedy*(240.f/1080.f);
  x=sx>0?-yaw*gain*zoom*.01745329252f/(vertical*g_Vars.currentplayer->aspect*sx):0;
  y=sy>0?pitch*gain*zoom*.01745329252f/(vertical*sy):0;
 }
 if(move->invertpitch)y=-y;
 move->freelookdx+=x;move->freelookdy+=y;
}

/* Called after PD has resolved its own control style and movement mode. */
void pd_controls_classic(struct movedata *m,int x,int y,int zoom){
 static int last_crouch;
 if(!pd_motion_allowed() || !g_Vars.currentplayer->insightaimmode || !pd_option_get(28) ||
    g_Vars.currentplayer->bondmovemode!=MOVEMODE_WALK){last_crouch=0;return;}
 m->analogwalk=m->analogstrafe=0;
 m->digitalstepforward=m->digitalstepback=m->digitalstepleft=m->digitalstepright=0;
 m->analoglean=x/80.f;if(m->analoglean>1)m->analoglean=1;if(m->analoglean< -1)m->analoglean=-1;
 int crouch=y>40?1:y< -40?-1:0;
 if(zoom){m->zoominfovpersec=y>40?y/40.f:0;m->zoomoutfovpersec=y< -40?-y/40.f:0;last_crouch=0;}
 else {if(crouch!=last_crouch){m->crouchup=crouch>0;m->crouchdown=crouch<0;}last_crouch=crouch;}
}
float pd_controls_breathing(float value){return pd_option_get(30)?value:0;}
void pd_controls_scope_head(void){
 if(!pd_motion_allowed() || !pd_option_get(31) || !g_Vars.currentplayer->insightaimmode)return;
 struct player *p=g_Vars.currentplayer;float k=(p->fovy-20)/30.f;
 if(!__builtin_isfinite(k) || k>=1)return;if(k<0)k=0;
 p->headlook.x*=k;p->headlook.y*=k;p->headlook.z=p->headlook.z*k+1-k;
 p->headup.x*=k;p->headup.z*=k;p->headup.y=p->headup.y*k+1-k;
}
