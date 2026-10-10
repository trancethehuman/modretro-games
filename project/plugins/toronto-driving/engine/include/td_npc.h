#ifndef TD_NPC_H
#define TD_NPC_H
#include <gbdk/platform.h>
/* People with something to do (td_npc.c): each walker slot shows the look
 * of its neighbourhood (td_people_pick); looks that do more than walk take
 * the slot over (td_life's owned modes from PK_NPC up): people sitting or
 * sleeping by a wall, buskers, protesters, tourists stopping for a photo,
 * joggers, the city's animals, and the antagonists (rival couriers, street
 * toughs, pickpockets, road ragers). */
/* Scene start: forget looks and behaviours. */
void td_npc_reset(void) BANKED;
/* After a route refresh: give new walkers their looks and behaviours. */
void td_npc_refresh(void) BANKED;
/* Once per update: behaviours of the slots they own. */
void td_npc_tick(void) BANKED;
/* Select on foot beside someone: they answer (TRUE when someone did). */
UBYTE td_npc_talk(void) BANKED;
/* Trouble at (u, v) whole pixels: the slots this module owns react
 * (td_life's panic only moves walkers it can see and does not own). */
extern UBYTE td_npc_alarm;
/* Bit i: walker slot i started a fight (hitting them back is no crime). */
extern UBYTE td_npc_hostile;
/* A driver the courier rammed gets out to fight (whole-pixel position). */
void td_npc_rager(UWORD u,UWORD v) BANKED;
extern UWORD td_npc_alarm_u,td_npc_alarm_v;
#endif
