#ifndef TD_SHOPS_H
#define TD_SHOPS_H
#include <gbdk/platform.h>
/* Genuine room transitions preserve the serialized outdoor courier state. */
UBYTE td_shops_interact(void) BANKED;
UBYTE td_shops_pending(void) BANKED;
void td_shops_reset(void) BANKED;
void td_shop_init(void) BANKED;
void td_shop_update(void) BANKED;
/* Main scene supplies deadline/world-clock and original-audio advancement.
 * FALSE requests a return after mission failure; elapsed is actual VBlanks. */
UBYTE td_shop_world_seconds(UWORD elapsed) BANKED;
#endif
