#ifndef TD_SPECIAL_H
#define TD_SPECIAL_H
#include <gbdk/platform.h>
#include "td_special_data.h"
/* Special vehicles (ambulance, fire truck, TTC bus, box truck, garbage
 * truck, tank) share one block of eight sprite tiles: sheet frames
 * TD_FRAME_SPECIAL..+7 (east, south, west and north views, diagonals
 * repeating the cardinal ones) hold placeholders, and td_special.c writes
 * the design in use over them in VRAM. One design shows at a time:
 * td_special_kind (TD_SPECIAL_*, 0 none). */
extern UBYTE td_special_kind;
/* The courier's vehicle is a special one (its kind), not saved: a save
 * keeps it as a van. */
extern UBYTE td_player_special;
/* Scene start: find the block's VRAM tiles in the compiled frames and
 * write the current design (kept only for the courier's own vehicle). */
void td_special_init(void) BANKED;
/* Write the design again after the sheet was reloaded (city map, title). */
void td_special_restore(void) BANKED;
/* Switch the block to another design. Only call while no vehicle showing
 * the current one is in view (td_special_free): every user changes. */
void td_special_load(UBYTE kind) BANKED;
#endif
