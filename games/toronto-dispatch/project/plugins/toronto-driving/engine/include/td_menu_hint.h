#ifndef TD_MENU_HINT_H
#define TD_MENU_HINT_H
#include <gbdk/platform.h>
/* Read-only paused-action guidance. Caller supplies at least21 WRAM bytes;
 * the helper copies its own ROM text before crossing the BANKED boundary. */
void td_menu_hint(char *dest) BANKED;
#endif
