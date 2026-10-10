#ifndef TD_MENU_H
#define TD_MENU_H
#include <gbdk/platform.h>
/* Menus (td_menu.c): one update while td.mode is not a play mode, and the
 * dispatch board's first offer the courier can take. */
void td_menu_update(void) BANKED;
void td_ready_offer(void) BANKED;
/* TTC: board in the current second's window (TORONTO.c); TRUE when the
 * wait ended (boarded or short of the fare). */
UBYTE td_board_current_window(void) BANKED;
#endif
