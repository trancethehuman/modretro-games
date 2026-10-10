#ifndef TD_PEOPLE_H
#define TD_PEOPLE_H
#include <gbdk/platform.h>
#include "td_sprites.h"
#include "td_people_data.h"
/* The people of the city are looks (create_people.py): four poses of two
 * frames each, kept in ROM. Walker slot i (actor TD_ACTOR_PEDS+i) draws
 * frames TD_PEOPLE_FRAME(i)..+3 (A, B, then mirrored for walking west),
 * whose two sprite tiles hold the slot's current look and pose; they are
 * rewritten (64 bytes) only when either changes, so any number of looks
 * costs ROM, not video memory. */
#define TD_PEOPLE_FRAME(slot) (TD_FRAME_WALKERS+((slot)<<2))
extern UBYTE td_slot_look[TD_WALKER_SLOTS],td_slot_pose[TD_WALKER_SLOTS];
/* Scene start: find the slots' tiles in the compiled frames and forget
 * what they held. */
void td_people_init(void) BANKED;
/* Write every slot again after the sheet was reloaded (city map, title). */
void td_people_restore(void) BANKED;
/* Show a look in a pose in a slot (and its palette on the slot's actor). */
void td_people_show(UBYTE slot,UBYTE look,UBYTE pose) BANKED;
/* Change only the pose of a slot's current look. */
void td_people_pose(UBYTE slot,UBYTE pose) BANKED;
/* The look for walker route identity `route` at whole-pixel scene position
 * (u, v) of district: the neighbourhood's mix of people, the night crowd
 * after dark, and an officer for one route in eight. */
UBYTE td_people_pick(UBYTE district,UBYTE route,UWORD u,UWORD v) BANKED;
/* A look's text (td_people_text.c): 0 name, 1 home, 2..4 trait lines,
 * 5..6 what they say. dest holds at least 19 bytes. */
void td_people_text(UBYTE look,UBYTE field,char *dest) BANKED;
#define TD_PT_NAME 0
#define TD_PT_HOME 1
#define TD_PT_TRAIT 2
#define TD_PT_TALK 5
#endif
