#ifndef TD_INTERIOR_H
#define TD_INTERIOR_H
#include <gbdk/platform.h>
#include "td_interior_data.h"
/* Inside buildings (td_interior.c). An interior is a scene of the same
 * scene type; while the courier is inside, td_interior is its index and
 * the city state (td.u, td.v, td.district, the parked car) stays at the
 * door outside, so a save made inside resumes at that door. Doors are
 * listed per scene (td_doors.h); A beside one goes in (refused with the
 * police after the courier). Inside, A talks to people, reads plaques and
 * uses the elevator or the delivery desk; walking out of the doors leaves.
 * The pause menu's Gallery is an interior of its own. Not saved. */
extern UBYTE td_interior;
/* Cold boot: outside. */
void td_interior_reset(void) BANKED;
/* Scene start while td_interior is set: TRUE when the loaded scene is that
 * interior (and it is now set up). */
UBYTE td_interior_init(void) BANKED;
/* Once per update inside (after the menus and the clock). */
void td_interior_update(UBYTE motion) BANKED;
/* On foot outside, A: go in at a door within reach (TRUE when the courier
 * goes in or is turned away). */
UBYTE td_interior_door(void) BANKED;
/* The door within reach (HUD prompt), TD_NONE when none; refreshed a few
 * times a second. */
extern UBYTE td_door_near;
UBYTE td_door_scan(void) BANKED;
/* Name of door d (19 bytes). */
void td_door_name(UBYTE d,char *dest) BANKED;
/* Pause menu: go to the gallery (TRUE when on the way). */
UBYTE td_gallery_open(void) BANKED;
/* Put the interior's own palettes back (after the city map). */
void td_interior_palettes(void) BANKED;
/* A text card (td.mode TD_CARD, td_ui.c): five lines of 18 characters. */
extern char td_card[5][19];
#endif
