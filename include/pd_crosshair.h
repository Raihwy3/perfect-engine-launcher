#pragma once
#include "pd_runtime.h"
/* Colour and opacity shared by original and smooth custom crosshairs. */
static inline unsigned pd_crosshair_colour(unsigned colour){
 static const unsigned rgb[]={0,0xffffff00,0xffb04000,0xff404000,0x4080ff00,0xd4af3700,0xa060ff00,0x40ff6000};
 int choice=pd_option_get(44);if(choice>0 && choice<8)colour=rgb[choice]|(colour&255);
 int opacity=pd_option_get(42);if(opacity>0)colour=(colour&0xffffff00)|((unsigned)((opacity-1)*255+5)/10);
 return colour;
}
