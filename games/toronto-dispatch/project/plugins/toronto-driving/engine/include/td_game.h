#ifndef TD_GAME_H
#define TD_GAME_H
#include <gbdk/platform.h>
#define TD_QUESTS 88
#define TD_STOPS 51
#define TD_COMPLETE_BYTES 16
/* 0 courier/car, 1 beacon, 2-7 traffic, 8 parked car, 9-16 pedestrians,
 * 17 transit vehicle, 18-19 sidewalk pickups, 20 street-life effects
 * (sparks, tracer rounds, a fleeing driver), 21-22 animation particles
 * (smoke, pops, sparkles) and 23 the lock-on marker. The engine field
 * MAX_ACTORS raises GBVM's pool to 24.
 * Pedestrians, pickups and particles use one 8x16 OBJ each, so all actors
 * stay within 40 hardware sprites; the particles render last, so the
 * 10-per-line limit drops them first. Night headlamps are part of the
 * courier vehicle's own frames. */
#define TD_ACTORS 24
#define TD_PEDS 8
#define TD_ACTOR_PEDS 9
#define TD_ACTOR_FX 20
#define TD_ACTOR_TRANSIT 17
#define TD_ACTOR_PICKUPS 18
#define TD_PICKUP_SLOTS 2
#define TD_PICKUP_TAKEN 8
#define TD_ACTOR_PARTS 21
#define TD_ACTOR_RETICLE 23
/* Per-actor colour: actor.c.patch adds GBVM's move_speed field (unused by
 * this scene, which moves its actors itself) to every sprite palette when
 * an actor is drawn. Vehicle frames use palette 0 and people palette 1,
 * so a vehicle's offset is its OBJ palette and a person's is one lower. */
#define TD_PALETTE(a) ((a)->move_speed)
#define TD_PAL_COURIER 0
#define TD_PAL_RED 2
#define TD_PAL_BLUE 3
#define TD_PAL_YELLOW 4
#define TD_PAL_NAVY 5
#define TD_PAL_TEAL 6
#define TD_PAL_VIOLET 7
#define TD_PEOPLE_PAL(p) ((p)-1)
/* Each traffic slot's design (first frame) is chosen when it enters play. */
extern UBYTE td_traffic_bases[6];
#define TD_PARTS 2
#define TD_STOP_FOOT 1
#define TD_NONE 255
#define TD_ROAM 0
#define TD_PAUSE 1
#define TD_BOARD 2
#define TD_MAP 3
#define TD_TRANSIT 4
#define TD_WAIT 5
#define TD_RIDE 6
#define TD_RESULT 7
#define TD_HELP 8
#define TD_BUSTED 9
#define TD_WASTED 10
typedef struct { UWORD u,v; char name[19]; UBYTE transit,district,reserved; } td_stop_t;
/* A contract: it opens after min_done deliveries and once the contract
 * before it in the story (after, TD_NONE for none) is done. */
typedef struct { char title[19]; UBYTE kind,count,vehicle,min_done,after; UWORD seconds,reward; UBYTE route[12]; } td_job_t;
#define TD_DONE(job) (td.complete[(job)>>3]&(1<<((job)&7)))
typedef struct {
    UWORD u,v,park_u,park_v,cash,seconds,left;
    WORD speed;
    UBYTE heading,vehicle,onfoot,mode,menu,job,stage,health,done,subsecond;
    UBYTE complete[TD_COMPLETE_BYTES];
    UBYTE transit_origin,transit_target,ride_left,cooldown,msg;
    UBYTE reserved;
    UWORD safe_u,safe_v;
    /* Courier vitality 0..100 (separate from cargo health), pistol rounds,
     * police attention in stars 0..5 and seconds until it cools one star. */
    UBYTE vitality,ammo,wanted,heat;
    UBYTE district,park_district;
} td_state_t;
extern td_state_t td;
/* The active contract. The dispatch board browses offers in the same copy
 * (td_offer): it opens only between jobs, or from the pause menu, which
 * reloads the active contract when the board closes. */
extern td_job_t td_job;
#define td_offer td_job
extern td_stop_t td_target,td_cursor;
void td_get_stop(UBYTE index,td_stop_t *dest) BANKED;
/* Whole-pixel parking cue. FALSE leaves outputs unchanged; use WRAM buffers. */
UBYTE td_get_parking(UBYTE stop,UWORD *u,UWORD *v) BANKED;
void td_get_job(UBYTE index,td_job_t *dest) BANKED;
/* Two 18-character lines followed by a terminator; caller provides 37 bytes. */
void td_get_brief(UBYTE index,char *dest) BANKED;
UBYTE td_get_street(UWORD u,UWORD v) BANKED;
void td_get_street_name(UBYTE id,char *dest) BANKED;
extern UBYTE td_station_near;
/* Re-picks at most TD_ROUTE_PICKS out-of-range slots per call; TRUE when
 * more slots still wait for a route (call again soon). Empty slots are only
 * retried when retry_empty is set (the courier has moved since). */
#define TD_ROUTE_PICKS 2
UBYTE td_refresh_routes(UBYTE *identities,UWORD (*nearby)[2],UBYTE retry_empty) BANKED;
void td_get_district_name(UBYTE index,char *dest) BANKED;
extern UBYTE td_route_district; /* Rebuilt objective cue; not serialized. */
extern UBYTE td_resume_mode;
void td_ui_init(void) BANKED;
void td_ui_draw(void) BANKED;
/* Menu cursor/prompt animation and the HUD compass; cheap when unchanged. */
void td_ui_tick(void) BANKED;
void td_ui_hud_tick(void) BANKED;
void td_hud_places(void) BANKED;
/* Dispatcher radio calls (td_ui.c): queue a script from td_radio_data.h;
 * the tick types, holds and closes calls and raises the story beats that
 * come from the city's state (stars, nightfall, quiet stretches). */
void td_radio_say(UBYTE script) BANKED;
/* A contract's briefing (0), first pickup (1) or delivery (2) call. */
void td_radio_contract(UBYTE job,UBYTE part) BANKED;
/* The delivery call and what a first delivery brings: the beat that follows
 * the contract, a delivery-count beat, the openings of chapters it unlocks
 * (td_radio_open() taken before the delivery counted) and the finale. */
void td_radio_done(UBYTE job,UBYTE done_before,UWORD open_before) BANKED;
/* A chatter call for a quiet stretch (general, or the story's current
 * people once their chapter is open). */
void td_radio_chatter(void) BANKED;
/* The story chapters open now, one bit each. */
UWORD td_radio_open(void) BANKED;
/* Lost parcels (td_street.c): the hidden collectibles, found once each.
 * Found ones are bits TD_PARCEL_BIT.. of td.complete (bytes 12..14; the
 * contracts use bytes 0..10). */
#define TD_PARCELS 20
#define TD_PARCEL_BIT 96
#define TD_PARCEL_REWARD 50
#define TD_PARCEL_BONUS 500
UBYTE td_parcels_found(void) BANKED;
UBYTE td_street_parcel(UBYTE slot) BANKED;
/* Animated scenery (td_scenery.c): find the scene's water and screen tiles
 * after it loads; draw their next frame. */
void td_scenery_find(void) BANKED;
void td_scenery_tick(void) BANKED;
/* Its vertical-blank interrupt handler (added by td_scenery_find). */
void td_scenery_vbl(void) NONBANKED;
/* The current page's speaker card into dest; TRUE when it is Rosa. */
UBYTE td_radio_speaker(char *dest) BANKED;
void td_radio_tick(void) BANKED;
UBYTE td_radio_playing(void) BANKED;
/* Playing script (TD_NONE when idle), its page, characters typed, frames
 * the finished page has been up and the one queued script (td_radio.c). */
extern UBYTE td_radio_script,td_radio_next,td_radio_pos,td_radio_hold;
extern UWORD td_radio_page;
/* Stars when the radio last looked: a change brings a police call. */
extern UBYTE td_radio_wanted;
/* The tick only has work during a call, on a change of stars or once a
 * second for the clock and chatter. */
#define TD_RADIO_DUE() (td_radio_script!=TD_NONE||td.wanted!=td_radio_wanted||!(td_tick&63))
/* Updates a finished page stays up (about two seconds). */
#define TD_RADIO_HOLD 120
/* Typed part of the current page's line 0, 1 or 2, padded to 17 characters. */
void td_radio_line(UBYTE line,char *dest) BANKED;
void td_chapter_name(UBYTE chapter,char *dest) BANKED;
/* td_ui.c side of a call: TRUE once the card is up (opens it when the HUD
 * is showing), one typed character, and a repaint for a new page. */
UBYTE td_ui_radio_ready(void) BANKED;
void td_ui_radio_put(UBYTE column,UBYTE line,char c) BANKED;
void td_ui_draw_radio(void) BANKED;
/* Fee paid for the last delivery, for the result card. */
extern UWORD td_last_pay;
void td_map_open(void) BANKED;
void td_map_update(UBYTE buttons,UBYTE pressed) BANKED;
void td_map_close(void) BANKED;
void td_save(void) BANKED;
UBYTE td_restore(void) BANKED;
void td_set_target(void) BANKED;
/* HUD notice (td_ui.c message list) and contract end, shared with td_life.c. */
void td_message(UBYTE m) BANKED;
void td_finish(UBYTE success) BANKED;
UBYTE td_service(UBYTE origin) BANKED;
UBYTE td_next_departure(UBYTE origin,UWORD seconds) BANKED;
/* Sidewalk pickups and transit berths (td_street.c). Not serialized. */
extern UBYTE td_pickup_slot[TD_PICKUP_SLOTS],td_pickup_sk[TD_PICKUP_SLOTS],td_pickup_su8[TD_PICKUP_SLOTS],td_pickup_sv8[TD_PICKUP_SLOTS];
extern UWORD td_pickup_su[TD_PICKUP_SLOTS],td_pickup_sv[TD_PICKUP_SLOTS];
extern UBYTE td_pickup_dirty,td_pickup_taken[TD_PICKUP_TAKEN],td_pickup_taken_at;
/* cold: also forget which pickups were collected (new session). */
void td_street_reset(UBYTE cold) BANKED;
/* Courier position in 8-pixel units. */
void td_street_refresh(UBYTE district,UBYTE pu8,UBYTE pv8) BANKED;
/* Mark a slot's pickup collected and free the slot. */
void td_street_take(UBYTE slot) BANKED;
UBYTE td_street_berth(UBYTE stop,UBYTE district,UWORD *u,UWORD *v) BANKED;
#endif
