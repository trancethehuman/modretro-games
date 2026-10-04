#ifndef TD_AIRCRAFT_H
#define TD_AIRCRAFT_H
#include <gbdk/platform.h>

/* Cosmetic state, deliberately separate from the 58-byte cartridge save.
 * Signed Q4 positions permit a flight to enter/leave the viewport edges. */
typedef struct {
    WORD u,v;
    UWORD wait,ticks,seed;
    UBYTE active,kind,direction;
} td_aircraft_state_t;
extern td_aircraft_state_t td_aircraft;
void td_aircraft_reset(UWORD seed) BANKED;
void td_aircraft_update(UWORD elapsed,UWORD focus_u,UWORD focus_v) BANKED;
UBYTE td_aircraft_frame(void) BANKED;
#endif
