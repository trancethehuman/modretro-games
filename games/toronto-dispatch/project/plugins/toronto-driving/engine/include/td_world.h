#ifndef TD_WORLD_H
#define TD_WORLD_H
#include "td_district.h"

#define TD_WORLD_MAX_DISTRICTS 32
#define TD_WORLD_MAX_PORTALS 512
#define TD_TRAFFIC_COUNT 6
#define TD_TRAFFIC_POINTS 16
typedef struct { UBYTE from,to; UWORD u,v,arrival_u,arrival_v; UBYTE vehicle; } td_portal_t;
typedef struct { UBYTE district; UWORD u,v; } td_crossing_t;
/* Targets use native Q4; count/frame apply to the current target leg. */
typedef struct { UWORD u,v; UBYTE count,frame; } td_traffic_sample_t;

/* All failures leave caller outputs unchanged. Buffers must reside in WRAM. */
UBYTE td_world_name(UBYTE district,char *name) BANKED; /* nineteen bytes */
/* Positions are whole pixels. Fewest feasible district hops, then entrance
 * Manhattan distance; this is not a street-level shortest-path planner. */
UBYTE td_world_route(UBYTE from,UBYTE to,UBYTE onfoot,UWORD u,UWORD v,
                    UWORD target_u,UWORD target_v,td_portal_t *portal) BANKED;
/* All positions are Q4. Checks outbound direction and the swept boundary,
 * retains the lateral offset, and supports opposite E/W and N/S seams.
 * Caller validates destination collision before committing/queueing a load. */
UBYTE td_world_crossing(UBYTE district,UBYTE onfoot,UWORD old_u,UWORD old_v,
                       UWORD u,UWORD v,td_crossing_t *crossing) BANKED;
/* Western metadata only: district0 retains its native signal-specific loops.
 * One banked call fills all six slots. Cache samples until a leg changes. */
UBYTE td_world_traffic_init(UBYTE district,UWORD *u,UWORD *v,UBYTE *legs,
                           td_traffic_sample_t *samples) BANKED;
UBYTE td_world_traffic_samples(UBYTE district,const UBYTE *legs,
                              td_traffic_sample_t *samples) BANKED;
#endif
