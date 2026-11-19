/* Renderer callbacks read the persistent 3DS settings on the engine thread. */
#include "pd_runtime.h"
#include <stdint.h>
static unsigned render_state;
void pd_render_state(unsigned state){render_state=state;}
int native_setting_aa(void){return pd_option_get(7);}void native_setting_set_aa(int v){pd_option_set(7,v);}
int native_setting_sharpness(void){return pd_option_get(8);}void native_setting_set_sharpness(int v){pd_option_set(8,v);}
int native_setting_vignette(void){return pd_option_get(9);}void native_setting_set_vignette(int v){pd_option_set(9,v);}
int native_setting_bloom(void){return pd_option_get(10);}void native_setting_set_bloom(int v){pd_option_set(10,v);}
int native_setting_grain(void){return pd_option_get(11);}void native_setting_set_grain(int v){pd_option_set(11,v);}
int native_setting_vignette_mode(void){return pd_option_get(12);}
unsigned native_setting_render_state(void){return render_state;}
int native_setting_watch(void){return 0;}
void native_cart_stats(uint32_t *cached,uint32_t *file){*cached=*file=0;}

int native_setting_stereo_strength(void){return pd_option_get(25)*25;}

int native_setting_stereo_auto(void){return pd_option_get(34);}
int native_setting_stereo_convergence(void){return 10+10*pd_option_get(35);}
int native_setting_stereo_comfort(void){return -20+5*pd_option_get(36);}
int native_setting_stereo_hud_depth(void){return 5*pd_option_get(37);}
int native_setting_stereo_crosshair_hud(void){return pd_option_get(38);}
int native_setting_stereo_ghost_contrast(void){return 50+5*pd_option_get(39);}
int native_setting_stereo_black_floor(void){return pd_option_get(40);}
void native_setting_set_stereo_ghost_contrast(int v){pd_option_set(39,(v-50)/5);}
void native_setting_set_stereo_black_floor(int v){pd_option_set(40,v);}
