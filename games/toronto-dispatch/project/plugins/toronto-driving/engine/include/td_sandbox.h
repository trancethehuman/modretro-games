#ifndef TD_SANDBOX_H
#define TD_SANDBOX_H
#include <gbdk/platform.h>
#include "actor.h"
#define TD_SANDBOX_PARKED 4
void td_sandbox_reset(void) BANKED;
UBYTE td_sandbox_bound(UBYTE district) BANKED;
void td_sandbox_bind(UWORD *u,UWORD *v,UBYTE district) BANKED;
void td_sandbox_sync(void) BANKED;
/* 0 no vehicle;1 theft started;2 current owned car entry;3 bounded
 * extraction/parking capacity reached;4 blocked land door approach
 * (consume interaction with notice). */
UBYTE td_sandbox_interact(void) BANKED;
UBYTE td_sandbox_captured(UBYTE slot) BANKED;
UBYTE td_sandbox_owned(UBYTE slot) BANKED;
UBYTE td_sandbox_heading(UBYTE slot) BANKED;
UBYTE td_sandbox_extracting(void) BANKED;
void td_sandbox_tick(void) BANKED;
UBYTE td_sandbox_human_hits(void) BANKED;
void td_sandbox_prepare(void) BANKED;
void td_sandbox_present(void) BANKED;
void td_sandbox_render(void) BANKED;
/* Light vacant scooter impact; shove state reuses unused parked row bits.
 * Returns TRUE after a fresh checked mass transfer or an ongoing shove. */
UBYTE td_sandbox_scooter_ram(UWORD old_u,UWORD old_v,UWORD u,UWORD v) BANKED;
/* One8-active-VBlank scooter impulse quantum; driver extraction is separate.
 * Called only by td_scooter_update. TRUE invalidates stationary-body reuse. */
UBYTE td_sandbox_scooter_step(void) BANKED;
/* Full-body Q4 sweeps against8 fleet bodies and parked rows; existing
 * overlap may separate monotonically, but new penetration fails closed. */
UBYTE td_sandbox_clear(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half,UBYTE skip) BANKED;
UBYTE td_sandbox_foot_clear(UWORD u,UWORD v) BANKED;
#endif
