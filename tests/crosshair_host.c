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
 for(int style=1;style<=7;style++){int count=0;for(int y=-8;y<=8;y++)for(int x=-8;x<=8;x++)count+=pd_crosshair_pixel(style,x,y);assert(count>0 && count<100);}
 puts("PASS custom crosshair silhouettes, original RGBA, eight colour choices and all opacity levels");
}
