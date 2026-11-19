#pragma once
#include <stdint.h>
/* Shared with the original menu gradient renderer: the middle stop is halfway
 * down the rectangle, not halfway across it. */
static inline int pd_menu_gradient_midpoint(int start,int end){return (start+end)/2;}
void pd_menu_default_theme(uint32_t palette[6]);
