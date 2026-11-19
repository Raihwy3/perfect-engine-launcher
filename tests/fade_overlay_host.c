
#include "../upstream/perfect_dark/src/game/player.c"
#include <assert.h>
#include <stdio.h>
static s16 view_width=320;
s16 viGetViewLeft(void){return 0;}
s16 viGetViewTop(void){return 0;}
s16 viGetViewWidth(void){return view_width;}
s16 viGetViewHeight(void){return 240;}
int main(void){
 for(int width=320;width<=400;width+=80){
  view_width=width;Gfx commands[64],expected[1];
  Gfx *end=playerDrawFade(commands,150,0,0,.7f);
  gDPSetScissor(expected,G_SC_NON_INTERLACE,0,0,width,240);
  unsigned begin=0,finish=0,clip=0,unanchor=0;
  for(Gfx *p=commands;p<end;p++){
   if((p->words.w0>>24)==G_NOOP){begin+=p->words.w1==0x50444601;finish+=p->words.w1==0x50444600;}
   if(p->words.w0==expected->words.w0 && p->words.w1==expected->words.w1)clip++;
   if((p->words.w0>>24)==G_EXTRAGEOMETRYMODE_EXT)unanchor++;
  }
  assert(begin==1 && finish==1 && clip==1 && unanchor==1);
  assert(playerDrawFade(commands,0,0,0,0)==commands);
 }
 puts("PASS actual fade display lists: 320/400-pixel coverage, cleared HUD aspect anchors, balanced flat scopes and inactive no-op");
}
