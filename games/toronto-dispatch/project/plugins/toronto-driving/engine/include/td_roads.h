#ifndef TD_ROADS_H
#define TD_ROADS_H
#include "td_game.h"
/* Loaded-scene collision queries use whole pixels, including every tile
 * touched by the full body or cardinal swept hull. No trusted fast flag. */
UBYTE td_road_body(UWORD u,UWORD v,UBYTE half) BANKED;
UBYTE td_road_sweep(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half) BANKED;
UBYTE td_road_walkable(UWORD u,UWORD v) BANKED;
UBYTE td_road_corner(td_state_t *state,WORD *vx,WORD *vy,UBYTE *used,WORD nu,WORD nv) BANKED;
#endif
