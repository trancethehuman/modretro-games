#ifndef TD_NAVIGATION_H
#define TD_NAVIGATION_H
#include <gbdk/platform.h>

#define TD_NAVIGATION_NONE 0
#define TD_NAVIGATION_NORTH 1
#define TD_NAVIGATION_EAST 2
#define TD_NAVIGATION_SOUTH 3
#define TD_NAVIGATION_WEST 4
#define TD_NAVIGATION_ARRIVED 5
typedef struct { UWORD u,v; UBYTE direction; } td_navigation_waypoint_t;

/* Whole-pixel coordinates, existing exact stop/parking/portal/ferry goals.
 * Advice follows native collision at half7 car / half3 foot and prefers
 * asphalt while driving. No runtime BFS, persistent state, world changes,
 * banking pointers or movement. Unknown/unreachable input returns NONE.
 * Only the currently loaded district should be presented by the caller. */
UBYTE td_navigation_direction(UBYTE district,UBYTE onfoot,UWORD goal_u,UWORD goal_v,
    UWORD u,UWORD v) BANKED;
/* At most four 8px flow steps, ending before/at the next turn. Output is a
 * ground tile centre and its forward direction. Failure preserves output. */
UBYTE td_navigation_next(UBYTE district,UBYTE onfoot,UWORD goal_u,UWORD goal_v,
    UWORD u,UWORD v,td_navigation_waypoint_t *out) BANKED;
/* Internal bounded ROM decode. Invalid goal/mode/cell returns NONE. */
UBYTE td_navigation_cell(UWORD goal,UBYTE onfoot,UBYTE x,UBYTE y) BANKED;
#endif
