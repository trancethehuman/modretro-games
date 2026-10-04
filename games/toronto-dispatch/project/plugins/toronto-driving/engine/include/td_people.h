#ifndef TD_PEOPLE_H
#define TD_PEOPLE_H
#include <gbdk/platform.h>
/* Six loaded civilian routes. Knockback/corpses reset at district load;
 * money and police attention remain in the version-10 cartridge record. */
void td_people_reset(void) BANKED;
UBYTE td_people_present(UBYTE tick) BANKED; /* newly struck visible humans */
UBYTE td_people_second(void) BANKED; /* TRUE when police attention changes */
UBYTE td_people_police(UWORD u,UWORD v) BANKED; /* nearby patrol stops offender */
#endif
