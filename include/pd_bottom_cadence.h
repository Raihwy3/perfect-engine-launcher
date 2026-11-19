#pragma once
#include <stdint.h>
/* Content is redrawn at most 10 Hz during gameplay; LCD presentation and
 * input remain per-frame. Menus and changes invalidate the retained panel. */
struct pd_bottom_clock {uint64_t last;uint32_t key;int valid;};
static inline int pd_bottom_clock_due(struct pd_bottom_clock *clock,uint64_t now,uint32_t key,int gameplay){
 if(!gameplay){clock->valid=0;return 1;}
 if(clock->valid && clock->key==key && now>=clock->last && now-clock->last<100000)return 0;
 clock->valid=1;clock->key=key;clock->last=now;return 1;
}
