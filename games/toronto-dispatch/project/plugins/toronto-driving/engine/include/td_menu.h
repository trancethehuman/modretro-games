#ifndef TD_MENU_H
#define TD_MENU_H
#include <gbdk/platform.h>
/* Only held directional navigation repeats; action buttons never repeat.
 * Call reset on world/menu transitions. Timing is in active UI VBlanks. */
void td_menu_reset(void) BANKED;
UBYTE td_menu_repeat(UBYTE held,UWORD elapsed) BANKED;
#endif
