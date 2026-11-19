#include "pd_bottom_cadence.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 struct pd_bottom_clock c={0};unsigned draws=0,retained=0;
 for(unsigned i=0;i<600;i++)if(pd_bottom_clock_due(&c,(uint64_t)i*1000000/60,1,1))draws++;else retained++;
 assert(draws>=90 && draws<=100 && retained>=500);
 assert(pd_bottom_clock_due(&c,10000000,2,1)); /* status/difficulty/stage change */
 assert(!pd_bottom_clock_due(&c,10000001,2,1));
 assert(pd_bottom_clock_due(&c,10000002,2,0)); /* opening menu */
 assert(pd_bottom_clock_due(&c,10000003,2,0)); /* menu remains full-rate */
 assert(pd_bottom_clock_due(&c,10000004,2,1)); /* first gameplay frame after menu */
 assert(pd_bottom_clock_due(&c,5,2,1)); /* clock reset */
 assert(pd_bottom_clock_due(&c,1000005,2,1)); /* stall never catches up */
 assert(!pd_bottom_clock_due(&c,1000006,2,1));
 printf("PASS bottom cadence: %u redraws / %u retained over 600 frames; immediate changes, full-rate menus, reopen/reset/stall\n",draws,retained);
}
