#pragma once
#include <ultra64.h>
#include <stdint.h>
extern int pd_rendering_objectives;
int pd_bottom_objectives_visible(void);
int pd_bottom_objective_indices(int *indices,int capacity);
Gfx *pd_render_bottom_objectives(Gfx *gdl);

struct fontchar;struct font;
Gfx *pd_bottom_text(Gfx *,s32 *,s32 *,char *,struct fontchar *,struct font *,u32,s32,s32,s32,s32);
Gfx *pd_bottom_line(Gfx *,s32,s32,s32,s32,u32,u32,int);
int pd_bottom_objectives_present(void);
void pd_bottom_objectives_reset(void);

/* Bottom LCD menus use the same ROM fonts and palette as the engine. */
struct menudialog;
struct pd_bottom_row { char label[128], value[64]; int focused; };
int pd_bottom_menu_present(struct menudialog *, const struct pd_bottom_row *, unsigned, int);
int pd_bottom_idle_present(void);

extern int pd_bottom_native_capturing;
extern int pd_bottom_ui_drawing;
struct menurendercontext;
void pd_bottom_native_begin(struct menudialog *);
void pd_bottom_native_end(void);
int pd_bottom_native_present(void);
int pd_bottom_menu_canvas_valid(void);
int pd_bottom_menu_geometry(float *);
int pd_bottom_native_touch(int,int);
void pd_bottom_native_hit(struct menurendercontext *,int,int,int);
void pd_bottom_native_text(s32,s32,char *,struct fontchar *,struct font *,u32,s32,s32,s32,s32);

uintptr_t pd_bottom_frame_token(void);
int pd_bottom_frame_bounds(int *);

void pd_bottom_native_context(struct menurendercontext *);
const uint16_t *pd_bottom_overlay_pixels(void);
const uint8_t *pd_bottom_overlay_alpha(void);

void pd_bottom_standalone_window(int);

extern int pd_bottom_text_overlay;

int pd_bottom_mission_panel_visible(void);
int pd_bottom_refresh_due(void);
void pd_bottom_refresh_report(void);

void pd_bottom_keyboard_hit(struct menurendercontext *,int,int,int,int,int,int);

extern int pd_top_ui_drawing;
