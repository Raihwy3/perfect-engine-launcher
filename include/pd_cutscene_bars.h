#pragma once
/* Original viewport placement stays owned by the game. Overrides affect only
 * cinematic framing and follow its existing transition back to gameplay. */
struct pd_cutscene_frame {int top,height;};
static inline struct pd_cutscene_frame pd_cutscene_frame(int mode,int fulltop,int fullheight,int originaltop,int originalheight,int cinemaheight,float tween){
 struct pd_cutscene_frame frame={originaltop,originalheight};
 if(mode==0)return frame;
 if(tween<0)tween=0;if(tween>1)tween=1;
 if(cinemaheight<1)cinemaheight=1;if(cinemaheight>fullheight)cinemaheight=fullheight;
 frame.height=mode==2?fullheight:(int)(cinemaheight+(fullheight-cinemaheight)*tween);
 frame.top=fulltop+(fullheight-frame.height)/2;
 return frame;
}
