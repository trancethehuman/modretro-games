#ifndef TD_TRAFFIC_H
#define TD_TRAFFIC_H
#include <gbdk/platform.h>
#include "actor.h"

#define TD_TRAFFIC_SLOTS 6
#define TD_TRAFFIC_PEOPLE 6

/* All pointers, including ctx itself, must reference WRAM (stack is valid).
 * NULL half arrays select the existing 5px traffic footprint. Extents are
 * whole pixels, 1..16, and refer to the current authored cardinal pose.
 * peds points at six contiguous pedestrian actors, or NULL for no people.
 * These pure queries allocate no persistent RAM and mutate no input. */
typedef struct {
    const UWORD *u,*v;
    const actor_t *peds;
    const UBYTE *half_u,*half_v;
    UWORD park_u,park_v;
    UBYTE parked_active,priority_mask;
} td_traffic_context_t;

/* TRUE blocks a red approach; vehicles already beyond the line may clear
 * the junction. All cars and the bus use the same fictional 12sec phase:
 * horizontal green [0,7), vertical green [7,12). Invalid motion fails closed.
 * Priority does not waive a red light. Coordinates are local Q4. */
UBYTE td_traffic_signal_stop(UBYTE district,UWORD seconds,UWORD old_u,
    UWORD old_v,UWORD u,UWORD v) BANKED;

/* TRUE permits a cardinal step <=8 Q4. Tests the complete body sweep against
 * other traffic, visible people and an active parked car. escape allows only
 * monotonic separation from an overlap already present at old_u/old_v;
 * new overlaps remain forbidden. Priority extends approaching civilians'
 * yielding distance, but never exempts an emergency vehicle from clearance.
 * The caller still owns authored-route, terrain, courier and tram guards. */
UBYTE td_traffic_motion_clear(const td_traffic_context_t *ctx,UBYTE slot,
    UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE escape) BANKED;

/* Before crossing a stop line, yield to a vehicle centre already inside the
 * 24px stop-line square. A waiting centre on the line leaves entry clear for
 * current 5..7px service bodies, while its first half-pixel entry immediately
 * owns the junction across a phase change. Parked cars/people retain full-body
 * occupancy of the18px interior. Once past the line, a driver may clear
 * the junction; normal full-body guards still apply. This entry query is
 * separate so a validated existing-overlap retreat need not enter a queue. */
UBYTE td_traffic_junction_clear(const td_traffic_context_t *ctx,UBYTE district,
    UBYTE slot,UWORD old_u,UWORD old_v,UWORD u,UWORD v) BANKED;

/* One banked admission query for elapsed-time autonomous traffic. Cardinal
 * steps1..128Q4 (8px), or zero, retain exact full-body sweeps and old->new
 * red-line/junction entry checks. Validate caller geometry once and search
 * one sorted lane once; no persistent cache or trusted-validation flag.
 * Standalone APIs above retain their strict8Q4 limit for rare retreat/tests.
 * escape permits only monotonic existing-overlap separation; priority never
 * waives lights/occupied bodies. Terrain, routes and tram remain caller-owned. */
UBYTE td_traffic_admit(const td_traffic_context_t *ctx,UBYTE district,UWORD seconds,
    UBYTE slot,UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE escape) BANKED;

/* Caller-owned transient WRAM/stack snapshot, never serialized. Fields are
 * implementation-private: only begin/admit/commit may write them. A fresh
 * begin is required each motion epoch and after scene/time/people/park/pose/
 * priority changes. Live traffic may change only through successful commits
 * followed immediately by the same caller cache update. No module globals.
 * Snapshot is87 bytes on packed native GBVM (host alignment may add one).
 * Fleet16px centre buckets update only through accepted commits. */
typedef struct {
    UWORD u[6],v[6],ped_u[6],ped_v[6],park_u,park_v,pending_u,pending_v;
    UBYTE bucket_u[6],bucket_v[6];
    UBYTE half_u[6],half_v[6],people_mask,parked_active,priority_mask;
    UBYTE district,phase,valid,pending_slot;
} td_traffic_epoch_t;

/* Copies and validates ALL six bodies and every visible foot/park body,
 * including malformed distant actors. Failure invalidates a non-NULL epoch
 * so an old snapshot cannot accidentally survive a failed new begin.
 * Input context/arrays/actors are never mutated; epoch must not alias them. */
UBYTE td_traffic_epoch_begin(const td_traffic_context_t *ctx,UBYTE district,
    UWORD seconds,td_traffic_epoch_t *epoch) BANKED;
/* Strict candidate/cache/endpoint/cardinal/128Q4 admission against the full
 * validated snapshot. Every call discards an older uncommitted candidate.
 * TRUE records a pending candidate but changes NO body position. The caller
 * must then perform its terrain/courier/future-tram guards. Do not commit a
 * candidate rejected by any external guard; a later admit safely replaces it.
 * Legacy public queries above remain independently fail-closed. */
UBYTE td_traffic_epoch_admit(td_traffic_epoch_t *epoch,UBYTE slot,
    UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE escape) BANKED;
/* Rare validated existing-overlap courier retreat: exactly the legacy8Q4
 * signal + body/priority queries, omitting only junction-entry ownership.
 * Caller must first prove its authored/waypoint route, monotonic courier
 * separation and terrain/tram-retreat guards. It uses the same pending/commit
 * protocol so a real retreat is visible to later ordinary movers. */
UBYTE td_traffic_epoch_retreat_admit(td_traffic_epoch_t *epoch,UBYTE slot,
    UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE escape) BANKED;
/* Commit only the matching most recently admitted slot after every external
 * guard passes. FALSE changes no body; TRUE advances the snapshot and clears
 * pending ownership. Then update the caller's matching live coordinates.
 * Red/blocked/aborted proposals are never visible to subsequent movers. */
UBYTE td_traffic_epoch_commit(td_traffic_epoch_t *epoch,UBYTE slot) BANKED;

/* Fused hot-path move: derive the old position from the private snapshot,
 * admit with escape=1, run loaded-scene whole-body road and future-tram
 * guards, then commit in this bank. TRUE means the snapshot already moved;
 * immediately update the matching live traffic cache to exactly u/v.
 * Flags are0/1. retreat selects the existing8Q4/no-entry policy; otherwise
 * normal128Q4/entry applies. separating requires retreat and skips ONLY the
 * future-tram query after the caller has proved authored/waypoint bounds,
 * monotonic courier separation and the full current/future tram-retreat
 * guard. The caller still owns route and courier guards for every move.
 * This convenience API requires square fleet half-extents5..8px. Its epoch
 * district must match the loaded road collision and tram presentation scene;
 * scene/time/actor changes still require begin. No persistent storage.
 * Any failure discards pending ownership and leaves all bodies/buckets fixed;
 * preexisting pending candidates cannot survive or commit after this call. */
UBYTE td_traffic_epoch_move(td_traffic_epoch_t *epoch,UBYTE slot,
    UWORD u,UWORD v,UBYTE retreat,UBYTE separating) BANKED;
#endif
