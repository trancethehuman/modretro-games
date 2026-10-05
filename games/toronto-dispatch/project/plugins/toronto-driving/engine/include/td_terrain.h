#ifndef TD_TERRAIN_H
#define TD_TERRAIN_H
#include <gbdk/platform.h>

/* Caller-owned automatic storage, never saved or retained between updates.
 * Set valid=stationary_valid=0 before driving. Keep the collision resource unchanged
 * for this lifetime; a successful scene/portal transition ends the update. */
typedef struct {
    UWORD u,v;UBYTE valid,result;
    /* Exact Q4 stationary queries within one immutable movement batch.
       Bits1/2 retain vehicle/tram results, including rejected clearance.
       Any active ramming step invalidates these before the next drive. */
    UWORD stationary_u,stationary_v;UBYTE stationary_valid,stationary_results;
} td_terrain_cache_t;
typedef char td_terrain_cache_must_be_twelve_bytes[(sizeof(td_terrain_cache_t)==12)?1:-1];

/* Exact whole-pixel centre, full half7 terrain body. The non-NULL cache must
 * be in WRAM (stack is valid). Misses use the unchanged public road query;
 * hits preserve incoming tile-hit globals without reading collision ROM.
 * Dynamic tram, vehicle and pedestrian clearance is deliberately separate. */
UBYTE td_terrain_drivable(UWORD u,UWORD v,td_terrain_cache_t *cache) BANKED;
#endif
