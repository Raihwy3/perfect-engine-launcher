#include <ultra64.h>
#include "input.h"
#include "config.h"
#include "constants.h"
#include <assert.h>
#include <stdio.h>
static int gameplay=1,touchx,touchy,touchgain=3;
static int aiming,flickmode;
int pd_aiming(void){return aiming;}
int pd_option_get(int key){return key==24?flickmode:key==21?touchgain:key==26?3:0;}
int pd_touch_look_allowed(void){return gameplay;}
int pd_touch_look(int *x,int *y){*x=touchx;*y=touchy;touchx=touchy=0;return 0;}
static unsigned held;static int cx,cy,rx,ry;
void pd_hid_sample(unsigned *h,int *x,int *y,int *r,int *s){*h=held;*x=cx;*y=cy;*r=rx;*s=ry;}
void configRegisterFloat(const char*k,f32*v,f32 min,f32 max){(void)k;(void)v;(void)min;(void)max;}
s32 configSave(const char*p){(void)p;return 0;}
static OSContPad readpad(unsigned keys){held=keys;inputUpdate();OSContPad p;assert(inputReadController(0,&p)==0);return p;}
int main(void){
 assert(readpad(64).button==CONT_L);assert(!readpad(4).button);
 gameplay=0;assert(readpad(64).button==CONT_UP);assert(!readpad(4).button);gameplay=1;
 assert(inputControllerGetSticksSwapped(0)==1);
 OSContPad p=readpad(1);assert(p.button&A_BUTTON);assert(p.button&BUTTON_UI_ACCEPT);
 p=readpad(2);assert(p.button&BUTTON_UI_CANCEL);assert(p.button&BUTTON_WPNFORWARD);assert(!(p.button&BUTTON_CANCEL_USE));
 p=readpad(1024);assert(p.button==BUTTON_RELOAD);
 p=readpad(2048);assert(p.button==BUTTON_CROUCH_CYCLE);
 assert(readpad(256).button&Z_TRIG);assert(readpad(32768).button&Z_TRIG);assert(readpad(16384).button&Z_TRIG);assert(readpad(512).button&R_TRIG);
 cx=156;cy=-156;rx=ry=0;p=readpad(0);assert(p.rstick_x==80 && p.rstick_y==-80 && p.stick_x==0 && p.stick_y==0);
 cx=cy=0;rx=156;ry=-156;p=readpad(0);assert(!p.rstick_x && !p.rstick_y && p.stick_x==80 && p.stick_y==-80);
 flickmode=1;p=readpad(0);assert(!p.stick_x && !p.stick_y);aiming=1;p=readpad(0);assert(p.stick_x==80 && p.stick_y==-80);aiming=flickmode=0;
 inputControllerSetAxisScale(0,1,0,.5f);p=readpad(0);assert(p.stick_x==40 && inputControllerGetAxisScale(0,1,0)==.5f);
 inputControllerSetAxisDeadzone(0,1,0,1.f);p=readpad(0);assert(p.stick_x==0);
 rx=ry=0;touchx=10;p=readpad(0);assert(p.stick_x==0 && touchx==10); // Touch is consumed by free-reticle movement, not stick emulation.
 assert(inputReadController(1,&p)==-1 && !p.button && !p.stick_x);
 puts("PASS: Vesper controls, independent movement/look, menu accept/back, crouch/reload, live sensitivity/deadzone, disconnected slots");
}
