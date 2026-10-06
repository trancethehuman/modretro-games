#ifndef TD_LIFE_H
#define TD_LIFE_H
#include <gbdk/platform.h>
#include "td_world.h"

/* Street life: car handling and collisions, pedestrians knocked down,
 * punches and the courier's pistol, police attention, pursuit, arrest and
 * hospital recovery, car theft and the on-screen objective pointer.
 *
 * Saved state is td.vitality/ammo/wanted/heat (td_game.h). Everything here
 * is transient: a scene load rebuilds it and nothing is serialized. */

#define TD_WANTED_MAX 5
#define TD_AMMO_START 12
#define TD_AMMO_MAX 99
#define TD_HEAT_SECONDS 20
#define TD_SUPPLY_PRICE 20

/* Fictional hospital forecourt in the core district (east of University,
 * south of College). Recovery validates the whole foot position. */
#define TD_HOSPITAL_DISTRICT 0
#define TD_HOSPITAL_U 504
#define TD_HOSPITAL_V 344

/* td_life_event: raised by td_life_tick, consumed by the city state. */
#define TD_EVENT_NONE 0
#define TD_EVENT_BUSTED 1
#define TD_EVENT_WASTED 2

/* td_life_drive results. */
#define TD_DRIVE_EXIT 1

/* Messages added to the HUD notice list (td_ui.c). */
#define TD_MSG_NO_AMMO 17
#define TD_MSG_CARJACK 18
#define TD_MSG_HIT 19
#define TD_MSG_LOST 20
#define TD_MSG_STARS 21
#define TD_MSG_SUPPLIES 22
#define TD_MSG_NO_CASH 23
#define TD_MSG_PED 24
#define TD_MSG_EXIT_HINT 25
#define TD_MSG_HOSPITAL 26
#define TD_MSG_COUNT 27

/* Driver state owned by TORONTO.c and shared with this module. */
extern WORD td_vx,td_vy;
extern UBYTE td_tick,td_turn_tick,td_entry_timer,td_entry_target,td_walk_dir,td_corner_used,td_red_cooldown;
extern UWORD td_traffic_u[6],td_traffic_v[6];
extern UBYTE td_traffic_leg[6];
extern td_traffic_sample_t td_traffic_samples[6];
extern UBYTE td_ped_route[6];
extern UWORD td_nearby_routes[6][2];

/* Slot masks read by the native kernels: bit i set means this module owns
 * pedestrian slot i (actors 9..14) or road vehicle i (actors 2..7). */
extern UBYTE td_ped_ovr,td_tr_ctrl;
/* Walker slots with a fresh route still in view: the native layout keeps
 * them hidden until td_life sees them outside the camera. */
extern UBYTE pk_fresh;
/* HOME-bank helper (TORONTO.c): road vehicles in a 16-pixel cell next to the courier. */
UBYTE td_near_cells(void) NONBANKED;
/* Nonzero while actor 20 shows a spark, bullet or fleeing driver. */
extern UBYTE td_fx_kind;
/* Objective position in whole pixels; actor 1 shows a beacon or pointer. */
extern UWORD td_beacon_u,td_beacon_v;
extern UBYTE td_beacon_shown;
extern UBYTE td_life_event,td_life_fine;

void td_life_reset(UBYTE cold) BANKED;
/* One driving step for the courier's vehicle; returns TD_DRIVE_* bits. */
UBYTE td_life_drive(void) BANKED;
/* On-foot A (no own car in reach): steal a nearby road vehicle or punch. */
void td_life_foot_a(void) BANKED;
/* On-foot B away from TTC: fire the pistol in the walking direction. */
void td_life_foot_b(void) BANKED;
/* TRUE while the courier is knocked down, being arrested or hurt-stunned. */
UBYTE td_life_locked(void) BANKED;
void td_life_tick(void) BANKED;
void td_life_second(void) BANKED;
/* Pedestrian route refresh that keeps owned slots and hides fresh arrivals
 * until they are outside the camera view. */
void td_life_routes(void) BANKED;
/* After the native walker layout: contacts and owned-slot presentation. */
void td_life_peds(UBYTE near) BANKED;
void td_life_present(void) BANKED;
/* Pause menu supplies: ammunition and first aid. FALSE when unaffordable. */
UBYTE td_life_buy(void) BANKED;
/* Apply an arrest (fine, attention cleared, job lost). */
void td_life_busted(void) BANKED;
/* Prepare hospital recovery; outputs the core exit in Q4. */
void td_life_hospital(UWORD *u,UWORD *v) BANKED;
#endif
