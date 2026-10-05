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
/* Original on-ground arrow reuses BG-bank1 tiles241..244. Destination beacon
 * and the atlas retain their exact stop coordinates. Prepare after restoring
 * later overlays during ordinary frames; force restore before modal/scene
 * tile reuse. Render after lights/scenery, and reset on scene
 * initialization. Five patch bytes plus fourteen exact-key route-cache bytes;
 * captured BKG page ownership, no OBJ, actors or saved fields. A cached route
 * expires on player-cell/goal/district/walking changes or scene reset. */
void td_guidance_road_reset(void) BANKED;
void td_guidance_road_restore(void) BANKED;
/* Keep only an identically owned patch with the unchanged cached route key.
 * No ROM recomputation; scenery must force restore before replacing its tile. */
void td_guidance_road_prepare(void) BANKED;
void td_guidance_road_render(void) BANKED;
#endif
