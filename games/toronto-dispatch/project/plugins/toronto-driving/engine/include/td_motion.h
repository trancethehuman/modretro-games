#ifndef TD_MOTION_H
#define TD_MOTION_H
#include <gbdk/platform.h>
#include "td_terrain.h"
/* Existing transient Core fields; none is serialized. */
extern WORD td_vx,td_vy;
extern UBYTE td_tick,td_red_cooldown,td_turn_tick,td_entry_timer,td_entry_target,td_walk_dir;
extern UBYTE td_corner_used,td_input_edge,td_result_b_release,td_contact_episode;
void td_drive(td_terrain_cache_t *cache) BANKED;
void td_motion_reset(void) BANKED;
/* A contact visibly removes momentum and prevents held throttle from
 * immediately replacing it; transient recovery never affects save layout. */
void td_motion_impact(void) BANKED;
/* Vehicle contact rebounds along its actual incoming momentum for six
 * checked ticks; every half-pixel step still obeys full-body clearance. */
void td_motion_vehicle_impact(void) BANKED;
UBYTE td_motion_rebounding(void) BANKED;
void td_motion_transfer(UBYTE speed) BANKED;
/* Swept moving fleet body; FALSE for parked traffic, protected states or
 * repeated contacts during recovery. Coordinates are Q4, half is5..8px. */
UBYTE td_motion_player_hit(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half) BANKED;
UBYTE td_motion_player_knockback(BYTE dx,BYTE dy) BANKED;
/* Small Core callbacks retain the existing menu/notice/terrain behavior. */
void td_motion_notice(UBYTE message) BANKED;
void td_motion_finish(UBYTE success) BANKED;
UBYTE td_motion_foot_clear(UWORD u,UWORD v) BANKED;
UBYTE td_motion_near_car(void) BANKED;
void td_motion_enter_exit(void) BANKED;
void td_motion_transit_open(void) BANKED;
UBYTE td_motion_world_interact(void) BANKED;
/* Swept road-car/parked-body admission; separating/reversing may recover. */
UBYTE td_motion_vehicle_clear(UWORD old_u,UWORD old_v,UWORD u,UWORD v) BANKED;
#endif
