#define __3DS__ 1
#include "../../upstream/perfect_dark/src/game/timing.c"
#include <assert.h>
#include <stdio.h>
struct g_vars g_Vars;
s32 g_TickRateDiv=1,g_TickExtraSleep;
static u32 clock_count,clock_step,hitches;
u32 osGetCount(void){clock_count+=clock_step;return clock_count;}
void sysSleep(s64 value){(void)value;}
void sysLogPrintf(s32 level,const char *fmt,...){(void)level;(void)fmt;hitches++;}
int main(void){
 clock_count=10000;frametimeInit();
 for(unsigned i=0;i<1000;i++){
  clock_count+=CYCLES_PER_FRAME*(i%2+1);
  frametimeCalculate();assert(g_Vars.diffframe60==(s32)(i%2+1));assert(g_Vars.diffframe240==(s32)(i%2+1)*4);
 }
 assert(!hitches);
 for(unsigned seconds=1;seconds<=45;seconds++){
  clock_count+=CYCLES_PER_FRAME*60u*seconds;
  frametimeCalculate();assert(g_Vars.diffframe60==6 && g_Vars.diffframe240==24);
  assert((u32)g_Vars.thisframestartt==clock_count);
  clock_count+=CYCLES_PER_FRAME;
  frametimeCalculate();assert(g_Vars.diffframe60==1 && g_Vars.diffframe240==4);
 }
 assert(hitches==45);
 clock_count=0xfffffff0u;frametimeInit();clock_count+=CYCLES_PER_FRAME*2u;
 frametimeCalculate();assert(g_Vars.diffframe60==2 && g_Vars.diffframe240==8);
 clock_count+=CYCLES_PER_FRAME*900u;g_Vars.lostframetime60t=123;g_Vars.lostframetime240t=-123;
 frametimeInit();assert(!g_Vars.lostframetime60t && !g_Vars.lostframetime240t);
 clock_step=CYCLES_PER_FRAME;frametimeCalculate();assert(g_Vars.diffframe60==1);
 puts("PASS actual engine timing: normal 30/60 FPS, 1-45s stalls bounded to six ticks, next frame normal, counter wrap and stage reset");
}
