#pragma once
/* Preserve authored pixel size. Only shrink dialogs that exceed the LCD. */
static inline float pd_menu_fit(int width,int height,float *left,float *top){
 float sx=296.f/(width>0?width:296),sy=216.f/(height>0?height:216);
 float scale=sx<sy?sx:sy;if(scale>1.f)scale=1.f;
 *left=160.f-width*scale*.5f;*top=120.f-height*scale*.5f;
 return scale;
}
