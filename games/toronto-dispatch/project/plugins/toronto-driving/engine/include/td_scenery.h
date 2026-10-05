#ifndef TD_SCENERY_H
#define TD_SCENERY_H
#include <gbdk/platform.h>
#define TD_SCENERY_CLEAR 0
#define TD_SCENERY_BLOCK 1
#define TD_SCENERY_BROKE 2
/* Whole car body sweep, Q4 coordinates. Does not replace raw terrain checks. */
UBYTE td_scenery_contact(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half,BYTE speed) BANKED;
/* Cold session reset only. District/shop transitions preserve broken props. */
void td_scenery_reset(void) BANKED;
/* Every scene init discards old VRAM ownership, never the destruction bitset. */
void td_scenery_render_reset(void) BANKED;
void td_scenery_restore(void) BANKED;
void td_scenery_render(void) BANKED;
#endif
