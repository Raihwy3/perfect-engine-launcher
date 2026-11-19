#pragma once
#include "pd_runtime.h"
/* Silhouettes adapted from Eyes of Gold (MIT). No game image data. */
static inline int pd_crosshair_pixel(int style,int x,int y){
    int ax=x<0?-x:x,ay=y<0?-y:y,r=x*x+y*y;
    if(style==1)return ax<=1 && ay<=1;
    if(style==2 || style==3 || style==6){
        int arms=(!y && ax>=3 && ax<=7) || (!x && ay>=3 && ay<=7 && (style!=6 || y>0));
        return arms || (style==3 && !x && !y);
    }
    if(style==4 || style==5)return (r>=20 && r<=36) || (style==5 && !x && !y);
    if(style==7)return ax+ay>=5 && ax+ay<=6;
    return 0;
}
static inline unsigned pd_crosshair_colour(unsigned colour){
 static const unsigned rgb[]={0,0xffffff00,0xffb04000,0xff404000,0x4080ff00,0xd4af3700,0xa060ff00,0x40ff6000};
 int choice=pd_option_get(44);if(choice>0 && choice<8)colour=rgb[choice]|(colour&255);
 int opacity=pd_option_get(42);if(opacity>0)colour=(colour&0xffffff00)|((unsigned)((opacity-1)*255+5)/10);
 return colour;
}
