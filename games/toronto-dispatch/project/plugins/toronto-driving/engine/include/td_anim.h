#ifndef TD_ANIM_H
#define TD_ANIM_H
#include <gbdk/platform.h>
/* Animation layer: two particle actors (TD_ACTOR_PARTS), courier action
 * poses and the driving camera. */
#define TD_PART_SMOKE 1 /* tyre smoke and dust: grows, thins and drifts up */
#define TD_PART_PUFF 2  /* short exhaust puff on a hard launch */
#define TD_PART_POP 3   /* an icon rises from a collection, then blinks out */
#define TD_PART_FLASH 4 /* sparkle: delivery, impact and muzzle flash */
#define TD_PART_POP_TICKS 30
/* Whole-pixel centre in the courier's actor convention. A full pool keeps
 * pops over sparkles over smoke; the newcomer is dropped otherwise. */
void td_anim_spawn(UBYTE kind,UBYTE frame,UWORD u,UWORD v) BANKED;
/* Courier pose for a few ticks: TD_FRAME_COURIER_PUNCH or _SHOOT; the
 * walking direction selects the frame. */
void td_anim_pose(UBYTE base,UBYTE ticks) BANKED;
void td_anim_reset(void) BANKED;
/* Once per rendered update (after the other presenters): particle ages and
 * actors, the driving triggers (launch, braking, a braking slide, a sliding
 * tail) and the camera (look-ahead plus impact shake). */
void td_anim_update(void) BANKED;
/* A particle: kind, icon frame (pops), remaining ticks and its GBVM actor
 * position (1/32 px). */
typedef struct {UBYTE kind,frame,time;UWORD x,y;} td_anim_part_t;
extern td_anim_part_t td_anim_parts[2];
extern UBYTE td_anim_pose_base,td_anim_pose_time;
extern BYTE td_look_x,td_look_y;
#endif
