#ifndef TD_SCOOTER_H
#define TD_SCOOTER_H
#include <gbdk/platform.h>
/* Two independent transient riders, sixteen bytes total. No save/actor slots.
 * Bind preserves identities on same-district shop returns. */
void td_scooter_reset(void) BANKED;
void td_scooter_bind(UBYTE district) BANKED;
/* Actual elapsed VBlanks; internal modal/scene guards freeze all state.
 * TRUE invalidates stationary collision reuse after any accepted movement. */
UBYTE td_scooter_update(UWORD elapsed) BANKED;
void td_scooter_render(void) BANKED;
/* Q4 full-body sweep/occupancy. Skip240/241 is the corresponding rider.
 * Existing overlap may separate monotonically, never deepen or tunnel. */
UBYTE td_scooter_clear(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half,UBYTE skip) BANKED;
UBYTE td_scooter_foot_clear(UWORD u,UWORD v) BANKED;
/* Player vehicle collision fallback: checked light-body impulse, original
 * speed/mass transfer once, non-graphic rider arc/prone and grounded bike.
 * Includes parked scooters. TRUE means the proposed player sweep is clear. */
UBYTE td_scooter_ram(UWORD old_u,UWORD old_v,UWORD u,UWORD v) BANKED;
/* Consume fresh occupied-rider collisions for existing heat/fine/cargo policy.
 * Momentum is already transferred; do not apply a second pedestrian slowdown. */
UBYTE td_scooter_take_hits(void) BANKED;
#endif
