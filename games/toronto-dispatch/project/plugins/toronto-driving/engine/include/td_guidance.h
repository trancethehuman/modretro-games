#ifndef TD_GUIDANCE_H
#define TD_GUIDANCE_H
#include <gbdk/platform.h>
/* Caller selects UI tile bank1. Original direction/star/menu patterns241..252. */
void td_guidance_init(void) BANKED;
/* Caller supplies21bytes; aliasing dest/label is supported. Uses current
 * loaded-scene objective/portal beacon, preserving unreachable labels. */
void td_guidance_label(char *dest,const char *label) BANKED;
/* Compact20-column strip: short objective/street, time or wallet, three stars.
 * Caller first writes the full label to dest; no extra persistent buffer. */
void td_guidance_compact(char *dest,UBYTE guided) BANKED;
#endif
