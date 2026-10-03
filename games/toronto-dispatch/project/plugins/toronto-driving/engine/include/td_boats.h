#ifndef TD_BOATS_H
#define TD_BOATS_H
#include <gbdk/platform.h>
#include "bankdata.h"

/* Cosmetic loaded-scene craft. No actor slot or SRAM fields. Native size13. */
typedef struct {
    UWORD phase;
    far_ptr_t sprite,scene;
    UBYTE base,scratch,scratch_props,district,bound;
} td_boat_state_t;
extern td_boat_state_t td_boat;
void td_boats_reset(void) BANKED;
/* Capture the authored empty loader before the Toronto actor clones. */
void td_boats_bind(void) BANKED;
void td_boats_update(UWORD elapsed) BANKED;
/* Append after ground actors, before aircraft; omit under OAM/UI pressure. */
void td_boats_render(void) BANKED;
#endif
