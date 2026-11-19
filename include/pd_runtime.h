#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct movedata;
void pd_gyro_movedata(struct movedata *move);
void pd_controls_aim_reticle(float *,float *,float,float);
int pd_gyro_allowed(void);
int pd_aiming(void);
int pd_motion_allowed(void);
float pd_menu_spin_angle(void);
void pd_controls_classic(struct movedata *,int,int,int);
void pd_controls_scope_head(void);
float pd_controls_breathing(float);
void pd_hid_take_gyro(float *yaw,float *pitch);
void pd_hid_take_flick(float *yaw);
int pd_option_get(int key);
void pd_option_set(int key,int value);
void pd_runtime_frame(void);
void pd_runtime_reset(void);
void pd_profile_record(unsigned stage,uint64_t elapsed);
void pd_audio_dispatch(void (*job)(void *),void *arg);
void pd_audio_join(void);
void pd_bottom_present(const char *text);
void pd_bottom_bitmap_present(const uint16_t *pixels);
int pd_bottom_touch(int x,int y);
int pd_touch_look(int *dx,int *dy);
int pd_touch_look_allowed(void);
void pd_guard_ray_count(int hit);
struct chrdata;struct prop;struct coord;
int pd_guard_ai_tick(struct chrdata *);
int pd_guard_los_lookup(struct chrdata *,struct prop *,int *);
void pd_guard_los_store(struct chrdata *,struct prop *,int);
int pd_effect_visible(struct coord *,unsigned index);
float pd_adjust_far(float near,float far);
#ifdef __cplusplus
}
#endif

void pd_controls_aim_reticle(float *x,float *y,float dx,float dy);
