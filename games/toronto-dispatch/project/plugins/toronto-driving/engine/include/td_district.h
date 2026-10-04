#ifndef TD_DISTRICT_H
#define TD_DISTRICT_H

#include <gbdk/platform.h>
#include "bankdata.h"

#define TD_DISTRICT_CITY 0
#define TD_DISTRICT_WEST 1
#define TD_DISTRICT_HIGH_PARK 2
#define TD_DISTRICT_EAST 3
#define TD_DISTRICT_PORT_LANDS 4
/* Public paths are ferry-accessed and have no ordinary road seam. */
#define TD_DISTRICT_ISLANDS 5
#define TD_DISTRICT_COUNT 6
#define TD_DISTRICT_NONE 255
#define TD_DISTRICT_TILE_WIDTH 128
#define TD_DISTRICT_TILE_HEIGHT 122
#define TD_DISTRICT_PIXEL_WIDTH 1024
#define TD_DISTRICT_PIXEL_HEIGHT 976

/* Identify the actual loaded resource, and acknowledge a matching queued load. */
UBYTE td_district_current(void) BANKED;
/* Copy a compiler-resolved scene far pointer; FALSE leaves dest unchanged. */
UBYTE td_district_scene(UBYTE district,far_ptr_t *dest) BANKED;
/* Queue one locked, faded GBVM change-scene script. FALSE changes no scene.
 * The caller owns pending arrival coordinates and commits them in state_init. */
UBYTE td_district_queue(UBYTE district) BANKED;
/* Call only on session boot after GBVM has discarded all old script contexts. */
void td_district_reset(void) BANKED;

/* These look up the requested district without changing active scene globals.
 * Tile coordinates are tiles; walkable/drivable coordinates are whole pixels.
 * Unknown districts, out-of-bounds tiles and invalid metadata are blocked. */
UBYTE td_district_tile(UBYTE district,UBYTE x,UBYTE y) BANKED;
UBYTE td_district_walkable(UBYTE district,UWORD u,UWORD v) BANKED;
UBYTE td_district_drivable(UBYTE district,UWORD u,UWORD v) BANKED;

#endif
