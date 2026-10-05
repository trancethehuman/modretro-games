#ifndef TD_BOATS_H
#define TD_BOATS_H
#include <gbdk/platform.h>
#include "bankdata.h"

/* Loaded-scene launch. Entire state is transient; save v10 stays58 bytes.
 * Positions/shore/velocities are Q4. Sprite fields use compiled OBJ indices. */
typedef struct {
    UWORD phase,u,v,shore_u,shore_v;
    WORD vx,vy;
    far_ptr_t sprite,scene;
    UBYTE scratch[4],scratch_props[4],base,district,bound;
    UBYTE occupied,moored,heading,tick,turn_tick;
    BYTE speed;
} td_boat_state_t;
extern td_boat_state_t td_boat;
void td_boats_reset(void) BANKED;
/* Capture the authored empty loader before the Toronto actor clones. */
void td_boats_bind(void) BANKED;
void td_boats_update(UWORD elapsed) BANKED;
void td_boats_render(void) BANKED;
/* Fresh on-foot A, or A aboard at a safe dock. Returns0 unchanged,1 boarded,
 * 2 safely landed. Q4 WRAM outputs; never alters td/car/job/cash/save fields. */
UBYTE td_boats_interact(UWORD *u,UWORD *v) BANKED;
UBYTE td_boats_controlled(void) BANKED;
UBYTE td_boats_under_cover(void) BANKED;
/* Raw elapsedVBlanks, not game seconds. Cap16 substeps/8px; caller freezes
 * when paused. A throttle/B brake/reverse; left/right turn, opposing neutral. */
void td_boats_drive(UBYTE keys,UWORD elapsed,UWORD *u,UWORD *v) BANKED;
/* Read-only safe save/reset projection to original validated boarding shore.
 * TRUE writes Q4 land coordinates; it does not exit, warp or save the boat. */
UBYTE td_boats_restore_shore(UWORD *u,UWORD *v) BANKED;
#endif
