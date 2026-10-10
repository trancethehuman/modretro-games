#ifndef TD_LIFE_H
#define TD_LIFE_H
#include <gbdk/platform.h>
#include "td_world.h"
#include "td_game.h"

/* Street life: car handling and collisions, pedestrians knocked down,
 * punches and the courier's pistol, police attention, pursuit, arrest and
 * hospital recovery, car theft and the on-screen objective pointer.
 *
 * Saved state is td.vitality/ammo/wanted/heat (td_game.h). Everything here
 * is transient: a scene load rebuilds it and nothing is serialized. */

#define TD_WANTED_MAX 5
#define TD_AMMO_START 12
#define TD_AMMO_MAX 99
#define TD_HEAT_SECONDS 12
#define TD_SUPPLY_PRICE 20

/* Fictional hospital forecourt in the core district (east of University,
 * south of College). Recovery validates the whole foot position. */
/* TD_HOSPITAL_DISTRICT/U/V: generated with the world (the hospital door). */
#include "td_district_world.h"

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
#define TD_MSG_CASH 27
#define TD_MSG_FIRST_AID 28
#define TD_MSG_AMMO 29
#define TD_MSG_SMOKING 30
#define TD_MSG_WRECKED 31
#define TD_MSG_PARCEL 32
#define TD_MSG_SPRAY 33
#define TD_MSG_SPRAY_NEAR 34
#define TD_MSG_SHARK 35
#define TD_MSG_HELI 36
#define TD_MSG_SHARK_BITE 37
#define TD_MSG_COUNT 38

/* Sidewalk pickups (td_street.c tables, collected in TORONTO.c). */
#define TD_PICKUP_CASH 15
#define TD_PICKUP_FIRST_AID 40
#define TD_PICKUP_AMMO 6

/* Driver state owned by TORONTO.c and shared with this module. */
extern WORD td_vx,td_vy;
extern UBYTE td_tick,td_turn_tick,td_entry_timer,td_entry_target,td_walk_dir,td_corner_used,td_red_cooldown;
extern UWORD td_traffic_u[6],td_traffic_v[6];
extern UBYTE td_traffic_leg[6];
extern td_traffic_sample_t td_traffic_samples[6];
extern UBYTE td_ped_route[TD_PEDS];
extern UWORD td_nearby_routes[TD_PEDS][2];

/* Slot masks read by the native kernels: bit i set means this module owns
 * pedestrian slot i (actors 9..16) or road vehicle i (actors 2..7). */
extern UBYTE td_ped_ovr,td_tr_ctrl;
/* Walker slots with a fresh route still in view: the native layout keeps
 * them hidden until td_life sees them outside the camera. */
extern UBYTE pk_fresh;
/* HOME-bank helper (TORONTO.c): road vehicles in a 16-pixel cell next to the courier. */
UBYTE td_near_cells(void) NONBANKED;
/* Nonzero while actor 20 shows a spark, bullet or fleeing driver. */
extern UBYTE td_fx_kind;
/* Objective position in whole pixels; actor 1 shows a beacon or pointer. */
/* The courier's current car (not saved): damage 0..100 from walls and
 * impacts, smoke from TD_DAMAGE_SMOKE, a failing engine (three-quarter top
 * speed) from TD_DAMAGE_FAIL and a crawl when wrecked; supplies repair it
 * and a stolen car starts fresh. Its colour is a sprite palette offset:
 * the courier's orange, or the stolen car's own paint. */
extern UBYTE td_car_damage,td_car_colour;
#define TD_DAMAGE_SMOKE 40
#define TD_DAMAGE_FAIL 70
#define TD_DAMAGE_WRECK 100
extern UWORD td_beacon_u,td_beacon_v;
extern UBYTE td_beacon_shown;
extern UBYTE td_life_event,td_life_fine;
/* Give traffic slot i a new design and colour (call while it is out of view). */
void td_lf_new_look(UBYTE i,UBYTE seed) BANKED;

void td_life_reset(UBYTE cold) BANKED;
/* One driving step for the courier's vehicle; returns TD_DRIVE_* bits. */
UBYTE td_life_drive(void) BANKED;
/* On-foot A (no own car in reach): steal a nearby road vehicle (TRUE when
 * one was taken), or throw a punch. */
UBYTE td_life_carjack(void) BANKED;
void td_life_punch(void) BANKED;
/* Pressed-button edges are valid on the first motion step of an update
 * (TORONTO.c); td_running is set while the courier runs on foot. */
extern UBYTE td_input_edge,td_running;
/* Set by the A+B chord in a vehicle until the car stops and the courier
 * gets out. */
extern UBYTE lf_exit_req;
/* On-foot B away from TTC: fire the pistol in the walking direction. */
void td_life_foot_b(void) BANKED;
/* On foot: aim direction (eight headings, E=0 clockwise) from the D-pad,
 * and the soft lock-on refreshed every few ticks: the nearest walker or
 * patrol car within about 60 degrees of the aim and 128 px (TD_NONE when
 * there is none). The lock-on marker shows it. */
extern UBYTE td_aim_dir,td_aim_target;
/* Set while B is held on foot: a lock then follows its target anywhere in range. */
extern UBYTE td_aim_hold;
void td_life_aim(void) BANKED;
/* TRUE while the courier is knocked down, being arrested or hurt-stunned. */
UBYTE td_life_locked(void) BANKED;
void td_life_tick(void) BANKED;
void td_life_second(void) BANKED;
/* Pedestrian route refresh that keeps owned slots and hides fresh arrivals
 * until they are outside the camera view. */
UBYTE td_life_routes(UBYTE moved) BANKED;
/* After the native walker layout: contacts and owned-slot presentation. */
void td_life_peds(UBYTE near) BANKED;
void td_life_present(void) BANKED;
/* Pause menu supplies: ammunition and first aid. FALSE when unaffordable. */
UBYTE td_life_buy(void) BANKED;
/* The spray bay: lose the police, repair and repaint the car (price below);
 * FALSE when the courier cannot pay. */
#define TD_SPRAY_PRICE 25
UBYTE td_life_spray(void) BANKED;
/* Apply an arrest (fine, attention cleared, job lost). */
void td_life_busted(void) BANKED;
/* Prepare hospital recovery; outputs the core exit in Q4. */
void td_life_hospital(UWORD *u,UWORD *v) BANKED;
#endif
