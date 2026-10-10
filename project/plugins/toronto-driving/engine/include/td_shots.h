#ifndef TD_SHOTS_H
#define TD_SHOTS_H
#include <gbdk/platform.h>
/* Rounds in flight (td_shots.c): the courier's pistol and the police's.
 * Every round is a fast projectile with a little spread, so a shot can
 * miss and an incoming one can be dodged. Rounds stop at buildings, people
 * and vehicles. One actor each, from TD_ACTOR_SHOTS. Nothing is saved. */
#define TD_SHOTS 4
#define TD_SHOT_COURIER 0
#define TD_SHOT_POLICE 1
/* Fire from (u,v) towards (tu,tv), whole pixels. spread widens the cone:
 * the perpendicular error is up to spread/48 of the travel (0 = dead on).
 * FALSE when every round is still in flight. */
UBYTE td_shot_fire(UBYTE owner,UWORD u,UWORD v,UWORD tu,UWORD tv,UBYTE spread) BANKED;
/* Once per rendered update: move rounds 6 px in two collision steps. */
void td_shot_tick(void) BANKED;
/* Place, show or hide the round actors. */
void td_shot_present(void) BANKED;
void td_shot_reset(void) BANKED;
extern UBYTE td_shot_live;
#endif
