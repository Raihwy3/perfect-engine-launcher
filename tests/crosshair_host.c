#include "pd_crosshair.h"
#include <assert.h>
#include <stdio.h>
static int value[45];
int pd_option_get(int key){return value[key];}
int main(void){
 assert(pd_crosshair_colour(0x12ab34cd)==0x12ab34cd);
 for(int colour=0;colour<8;colour++)for(int opacity=0;opacity<12;opacity++){
  value[44]=colour;value[42]=opacity;unsigned c=pd_crosshair_colour(0x12ab34cd);
  assert((c&255)==(opacity?((unsigned)((opacity-1)*255+5)/10):205));
  if(!colour)assert((c&0xffffff00)==0x12ab3400);
 }

 puts("PASS original crosshair RGBA, eight colour choices and all opacity levels");
}
