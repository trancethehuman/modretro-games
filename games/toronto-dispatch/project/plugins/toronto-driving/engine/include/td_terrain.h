#ifndef TD_TERRAIN_H
#define TD_TERRAIN_H
#include <gbdk/platform.h>

/* Caller-owned automatic storage, never saved or retained between updates.
 * Set valid=0 before querying. Keep the loaded collision resource unchanged
 * for this lifetime; a successful scene/portal transition ends the update. */
typedef struct {UWORD u,v;UBYTE valid,result;} td_terrain_cache_t;
typedef char td_terrain_cache_must_be_six_bytes[(sizeof(td_terrain_cache_t)==6)?1:-1];

/* Exact whole-pixel centre, full half5 terrain body. The non-NULL cache must
 * be in WRAM (stack is valid). Misses use the unchanged public road query;
 * hits preserve incoming tile-hit globals without reading collision ROM.
 * Dynamic tram, vehicle and pedestrian clearance is deliberately separate. */
UBYTE td_terrain_drivable(UWORD u,UWORD v,td_terrain_cache_t *cache) BANKED;
#endif
