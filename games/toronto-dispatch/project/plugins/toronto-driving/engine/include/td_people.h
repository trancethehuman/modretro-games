#ifndef TD_PEOPLE_H
#define TD_PEOPLE_H
#include <gbdk/platform.h>
#define TD_PEOPLE_COUNT 8
/* Eight loaded civilian routes. Knockback/corpses reset at district load;
 * money and police attention remain in the version-10 cartridge record. */
void td_people_reset(void) BANKED;
UBYTE td_people_present(UBYTE tick) BANKED; /* newly struck visible humans */
/* A visible route must not be recycled by nearest-route selection. */
UBYTE td_people_route_visible(UBYTE route) BANKED;
UBYTE td_people_shot(UBYTE slot,BYTE dx,BYTE dy) BANKED;
UBYTE td_people_second(void) BANKED; /* TRUE when police attention changes */
UBYTE td_people_police(UWORD u,UWORD v) BANKED; /* nearby patrol stops offender */
#endif
