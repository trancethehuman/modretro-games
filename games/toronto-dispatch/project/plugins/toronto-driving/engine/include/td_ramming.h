#ifndef TD_RAMMING_H
#define TD_RAMMING_H
#include <gbdk/platform.h>
#include "td_world.h"
/* Existing live fleet cache; physical impulses never create visual clones. */
extern UWORD td_traffic_u[8],td_traffic_v[8];
extern td_traffic_sample_t td_traffic_samples[8];
/* Exactly40 transient bytes: eight original positions and packed impulses.
 * A route resumes only after checked visible recovery to its original point. */
void td_ramming_reset(void) BANKED;
UBYTE td_ramming_active(UBYTE slot) BANKED;
UBYTE td_ramming_try(UBYTE slot,UWORD u,UWORD v) BANKED;
/* TRUE conservatively invalidates caller-owned stationary collision reuse
 * when any impulse/recovery identity existed at entry, even if blocked. */
UBYTE td_ramming_step(void) BANKED;
#endif
