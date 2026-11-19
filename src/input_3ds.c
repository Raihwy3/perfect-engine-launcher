/* First-player HID bridge. Optional desktop devices are absent. */
#include <ultra64.h>
#include "input.h"
#include "config.h"
#include "constants.h"
#include "pd_runtime.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
extern void pd_hid_sample(unsigned *,int *,int *,int *,int *);
static OSContPad sample;
static f32 sensitivity[4]={1,1,1,1},deadzone[4]={.08f,.08f,.08f,.08f};
static int scaled(int v,int axis){
 float n=(float)v/156.f;
 if(fabsf(n)<=deadzone[axis])return 0;
 n*=80.f*sensitivity[axis];return n>80?80:n< -80?-80:(int)n;
}
static void __attribute__((constructor)) inputConfig3ds(void){
 static const char *scale_keys[]={"3DS.MoveScaleX","3DS.MoveScaleY","3DS.LookScaleX","3DS.LookScaleY"};
 static const char *dead_keys[]={"3DS.MoveDeadzoneX","3DS.MoveDeadzoneY","3DS.LookDeadzoneX","3DS.LookDeadzoneY"};
 for(int i=0;i<4;i++){configRegisterFloat(scale_keys[i],&sensitivity[i],0,2);configRegisterFloat(dead_keys[i],&deadzone[i],0,1);}
}
void pd_save_settings(void){configSave(CONFIG_PATH);}
s32 inputInit(void){return 1;}
void inputUpdate(void){
 unsigned held;int x,y,rx,ry;pd_hid_sample(&held,&x,&y,&rx,&ry);memset(&sample,0,sizeof sample);
 /* PD extended controls: c1 is look, c2 is movement (desktop swapSticks=1).
  * Sensitivity/deadzone slots stay in physical Circle Pad / C-stick order.
  * The menu reads both stick pairs, so Circle Pad navigation remains available. */
 int lookx=scaled(rx,2),looky=scaled(ry,3);
 /* Touch uses the engine's relative free-reticle path, not clamped stick
  * input. Only suppress C-stick look during active gameplay flick mode. */
 if(pd_touch_look_allowed() && pd_option_get(24) && !pd_aiming())lookx=looky=0;
 sample.stick_x=lookx>80?80:lookx< -80?-80:lookx;
 sample.stick_y=looky>80?80:looky< -80?-80:looky;sample.rstick_x=scaled(x,0);sample.rstick_y=scaled(y,1);
 if(held&1)sample.button|=CONT_A|BUTTON_UI_ACCEPT;if(held&2)sample.button|=BUTTON_UI_CANCEL|BUTTON_WPNFORWARD;if(held&8)sample.button|=CONT_START;
 if(held&(256|32768|16384))sample.button|=CONT_G;if(held&512)sample.button|=CONT_R;
 if(held&1024)sample.button|=CONT_EXTRA0;if(held&2048)sample.button|=BUTTON_CROUCH_CYCLE;
 if(held&16)sample.button|=CONT_RIGHT;if(held&32)sample.button|=CONT_LEFT;
 if(held&64)sample.button|=pd_touch_look_allowed()?CONT_L:CONT_UP;if(held&128)sample.button|=CONT_DOWN;
}
s32 inputReadController(s32 idx,OSContPad *p){if(idx){memset(p,0,sizeof *p);return -1;}*p=sample;return 0;}
s32 inputControllerMask(void){return 1;}
s32 inputControllerConnected(s32 idx){return idx==0;}
s32 inputControllerGetDualAnalog(s32 idx){(void)idx;return 1;}
s32 inputControllerGetSticksSwapped(s32 idx){(void)idx;return 1;}
s32 inputControllerGetCancelCButtons(s32 idx){(void)idx;return 0;}
f32 inputControllerGetAxisScale(s32 a,s32 b,s32 c){(void)a;return sensitivity[b*2+c];}
f32 inputControllerGetAxisDeadzone(s32 a,s32 b,s32 c){(void)a;return deadzone[b*2+c];}
s32 inputGetConnectedControllers(s32 *out){if(out)*out=0;return 1;}
const char *inputGetConnectedControllerName(s32 id){(void)id;return "Nintendo 3DS";}
s32 inputGetAssignedControllerId(s32 id){return id? -1:0;}
s32 inputAssignController(s32 p,s32 id){return p==0 && id==0;}
s32 inputButtonPressed(s32 idx,u32 key){return !idx && (sample.button&key)!=0;}
void inputMouseGetRawDelta(s32 *x,s32 *y){*x=*y=0;}
void inputMouseGetScaledDelta(f32 *x,f32 *y){*x=*y=0;}
void inputMouseGetAbsScaledDelta(f32 *x,f32 *y){*x=*y=0;}
void inputMouseGetSpeed(f32 *x,f32 *y){*x=*y=1;}
s32 inputMouseGetPosition(s32 *x,s32 *y){*x=160;*y=110;return 0;}
const u32 *inputKeyGetBinds(s32 a,u32 b){(void)a;(void)b;static const u32 none[4]={0};return none;}
const char *inputGetKeyName(s32 key){(void)key;return "3DS";}
const char *inputGetContKeyName(u32 key){(void)key;return "3DS";}
const char *inputGetClipboard(void){return "";}
s32 inputRumbleSupported(s32 idx){(void)idx;return 0;}
void inputControllerSetSticksSwapped(s32 cidx, s32 swapped){(void)cidx;(void)swapped;}
void inputControllerSetDualAnalog(s32 cidx, s32 enable){(void)cidx;(void)enable;}
void inputControllerSetCancelCButtons(s32 cidx, s32 cancel){(void)cidx;(void)cancel;}
void inputControllerSetAxisScale(s32 cidx, s32 stick, s32 axis, f32 value){(void)cidx;if(stick>=0 && stick<2 && axis>=0 && axis<2)sensitivity[stick*2+axis]=value;}
void inputControllerSetAxisDeadzone(s32 cidx, s32 stick, s32 axis, f32 value){(void)cidx;if(stick>=0 && stick<2 && axis>=0 && axis<2)deadzone[stick*2+axis]=value;}
s32 inputKeyPressed(u32 vk){(void)vk;return 0;}
s32 inputKeyJustPressed(u32 vk){(void)vk;return 0;}
void inputKeyBind(s32 idx, u32 ck, s32 bind, u32 vk){(void)idx;(void)ck;(void)bind;(void)vk;}
s32 inputGetKeyByName(const char *name){(void)name;return 0;}
s32 inputGetContKeyByName(const char *name){(void)name;return 0;}
void inputRumble(s32 idx, f32 strength, f32 time){(void)idx;(void)strength;(void)time;}
f32 inputRumbleGetStrength(s32 cidx){(void)cidx;return 0;}
void inputRumbleSetStrength(s32 cidx, f32 val){(void)cidx;(void)val;}
void inputLockMouse(s32 lock){(void)lock;}
s32 inputMouseIsLocked(void){return 0;}
void inputMouseSetSpeed(f32 x, f32 y){(void)x;(void)y;}
s32 inputMouseIsEnabled(void){return 0;}
void inputMouseEnable(s32 enabled){(void)enabled;}
void inputSaveBinds(void){}
void inputSetDefaultKeyBinds(s32 cidx, s32 n64mode){(void)cidx;(void)n64mode;}
void inputClearLastKey(void){}
s32 inputGetLastKey(void){return 0;}
s32 inputGetMouseLockMode(void){return 0;}
void inputSetMouseLockMode(s32 lockmode){(void)lockmode;}
s32 inputAutoLockMouse(s32 wantlock){(void)wantlock;return 0;}
void inputMouseShowCursor(s32 show){(void)show;}
void inputStartTextInput(void){}
void inputStopTextInput(void){}
s32 inputIsTextInputActive(void){return 0;}
void inputClearLastTextChar(void){}
char inputGetLastTextChar(void){return 0;}
s32 inputTextHandler(char *out, const u32 outSize, s32 *curCol, s32 oskCharsOnly){(void)out;(void)outSize;(void)curCol;(void)oskCharsOnly;return 0;}
void inputClearClipboard(void){}
u32 inputGetKeyModState(void){return 0;}
