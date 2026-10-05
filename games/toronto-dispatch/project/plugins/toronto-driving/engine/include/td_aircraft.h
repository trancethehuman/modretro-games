#ifndef TD_AIRCRAFT_H
#define TD_AIRCRAFT_H
#include <gbdk/platform.h>

/* Cosmetic state, deliberately separate from the 58-byte cartridge save.
 * Signed Q4 positions permit a flight to enter/leave the viewport edges.
 * Retirement waits for a complete offscreen exit if the camera follows. */
typedef struct {
    WORD u,v;
    UWORD wait,ticks,seed;
    UBYTE active,kind,direction; /* kind0 commuter,1 helicopter,2 ground jet shadow */
} td_aircraft_state_t;
extern td_aircraft_state_t td_aircraft;
void td_aircraft_reset(UWORD seed) BANKED;
void td_aircraft_update(UWORD elapsed,UWORD focus_u,UWORD focus_v) BANKED;
/* Caller supplies actual worldQ4 courier coordinates, not camera/transit focus.
 * Exposure is false indoors/under authored canopy or bridge. Existing ambient
 * planes finish continuously; a helicopter can acquire without changing art. */
void td_aircraft_police_target(UBYTE wanted,UWORD u,UWORD v,UBYTE exposed) BANKED;
UBYTE td_aircraft_police_observed(void) BANKED;
UBYTE td_aircraft_frame(void) BANKED;
#endif
