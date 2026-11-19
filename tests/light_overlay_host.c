#include "../upstream/perfect_dark/src/game/sky.c"
#include <assert.h>
#include <stdio.h>
struct g_vars g_Vars;
s32 g_InCutscene;
static s16 width=320;
s16 viGetViewLeft(void){return 0;}
s16 viGetViewTop(void){return 0;}
s16 viGetViewWidth(void){return width;}
s16 viGetViewHeight(void){return 240;}
int main(void){
 static struct player player;
 g_Vars.currentplayer=&player;
 for(width=320;width<=400;width+=80){
  player.overexposurered=player.overexposuregreen=player.overexposureblue=80;
  Gfx commands[64],clipcmd[1],fillcmd[1];
  Gfx *end=skyRenderOverexposure(commands);
  gDPSetScissor(clipcmd,G_SC_NON_INTERLACE,0,0,width,240);
  gDPFillRectangle(fillcmd,0,0,width,240);
  unsigned begin=0,finish=0,clip=0,fill=0,unanchor=0;
  for(Gfx *p=commands;p<end;p++){
   if((p->words.w0>>24)==G_NOOP){begin+=p->words.w1==0x50444601;finish+=p->words.w1==0x50444600;}
   clip+=(p->words.w0==clipcmd->words.w0 && p->words.w1==clipcmd->words.w1);
   fill+=(p->words.w0==fillcmd->words.w0 && p->words.w1==fillcmd->words.w1);
   unanchor+=(p->words.w0>>24)==G_EXTRAGEOMETRYMODE_EXT;
  }
  assert(begin==1 && finish==1 && clip==1 && fill==1 && unanchor==1);
 }
 puts("PASS actual bright-light display lists cover 320/400 widths, clear HUD anchors and balance stereo scopes");
}
