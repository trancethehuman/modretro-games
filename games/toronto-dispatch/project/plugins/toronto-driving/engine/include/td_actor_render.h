#ifndef TD_ACTOR_RENDER_H
#define TD_ACTOR_RENDER_H
#include <gbdk/platform.h>
#include <gbdk/metasprites.h>
#include "actor.h"
/* Append an entire clipped ground pose only when OAM and every affected
 * scanline have capacity. Player remains first; no rotating sprite priority.
 * Coordinates stay signed until the actual hardware-object clip. */
void td_actor_render(const metasprite_t *pose,UBYTE bank,UBYTE base,WORD x,WORD y) BANKED;
/* Banked actor-coordinate and frame lookup keeps the fixed core compact. */
void td_actor_render_actor(actor_t *actor) BANKED;
#endif
