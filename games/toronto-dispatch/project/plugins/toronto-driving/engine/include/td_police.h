#ifndef TD_POLICE_H
#define TD_POLICE_H
#include <gbdk/platform.h>

/* Six caller-owned WRAM bytes, transient and never serialized. The caller
 * caches this waypoint until arrival, a scene/attention change or a bounded
 * stuck retry. Heading uses east0/south1/west2/north3. */
typedef struct { UWORD u,v; UBYTE heading,valid; } td_police_plan_t;

/* Plan a real cardinal road segment in the currently loaded district. All
 * coordinates are local Q4; wanted0 permits continuous return to a patrol
 * target, while wanted1..3 pursue the supplied courier target. There is no
 * persistent module RAM, position write, actor spawn or save mutation.
 * Inputs/output must be WRAM (stack is valid); failure preserves out exactly.
 * Pursuit is a bounded local road planner, not a whole-city shortest-path
 * search. Return uses finite ROM policies to an exact authored patrol target;
 * no near-target lateral shortcut or centre-line chase is permitted.
 * The caller must still admit EVERY movement through terrain, traffic,
 * courier and future tram sweeps, and must obey red lights. */
UBYTE td_police_plan(UBYTE district,UBYTE wanted,UWORD u,UWORD v,
    UWORD target_u,UWORD target_v,UBYTE heading,td_police_plan_t *out) BANKED;
#endif
