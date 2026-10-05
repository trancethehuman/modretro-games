#ifndef TD_ACTOR_RENDER_H
#define TD_ACTOR_RENDER_H
#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
#include "actor.h"
/* All frames of the five native ground sheets are gated from compiled ROM.
 * These cumulative OAM origins include tram and indoor workshop actors. */
#define TD_GROUND_MIN_X (-8)
#define TD_GROUND_MAX_X 16
#define TD_GROUND_MIN_Y 0
#define TD_GROUND_MAX_Y 16
/* Append an entire clipped ground pose only when OAM and every affected
 * scanline have capacity. Player remains first; no rotating sprite priority.
 * Coordinates stay signed until the actual hardware-object clip. */
void td_actor_render(const metasprite_t *pose,UBYTE bank,UBYTE base,WORD x,WORD y) BANKED;
/* Fixed-bank count/table/frame lookup restores the exact caller ROM bank. */
const metasprite_t *td_actor_render_pose(const void *descriptor,UBYTE bank,UBYTE frame) NONBANKED;
/* Banked actor-coordinate and frame lookup keeps the fixed core compact. */
void td_actor_render_actor(actor_t *actor) BANKED;
/* Scene-bounded stable roles2..last then marker1. No manual ROM switching. */
void td_actor_render_ground(UBYTE window_hide_actors) BANKED;
/* Restore aircraft then scenery overlays before ground actors; no WRAM. */
void td_actor_render_before(void) BANKED;
/* Existing post-ground overlays stay ordered in a switched ROM bank.
 * Caller restores its saved ROM bank before this dispatch; no state/WRAM. */
void td_actor_render_after(void) BANKED;
#endif
