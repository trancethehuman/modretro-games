#ifndef TD_GAME_H
#define TD_GAME_H
#include <gbdk/platform.h>
#define TD_QUESTS 88
#define TD_STOPS 51
#define TD_COMPLETE_BYTES 16
/* 0 courier/car, 1 beacon, 2-7 traffic, 8 parked car, 9-14 pedestrians,
 * 15 transit vehicle, 16-19 curbside props. GBVM allows 21. */
#define TD_ACTORS 20
#define TD_ACTOR_TRANSIT 15
#define TD_ACTOR_PROPS 16
#define TD_PROP_SLOTS 4
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
typedef struct { UWORD u,v; char name[19]; UBYTE transit,district,reserved; } td_stop_t;
typedef struct { char title[19]; UBYTE kind,count,vehicle,min_done; UWORD seconds,reward; UBYTE route[12]; } td_job_t;
typedef struct {
    UWORD u,v,park_u,park_v,cash,seconds,left;
    WORD speed;
    UBYTE heading,vehicle,onfoot,mode,menu,job,stage,health,done,subsecond;
    UBYTE complete[TD_COMPLETE_BYTES];
    UBYTE transit_origin,transit_target,ride_left,cooldown,msg;
    UBYTE reserved;
    UWORD safe_u,safe_v,map_x,map_y;
    UBYTE district,park_district;
} td_state_t;
extern td_state_t td;
extern td_job_t td_job,td_offer;
extern td_stop_t td_target,td_cursor;
void td_get_stop(UBYTE index,td_stop_t *dest) BANKED;
/* Whole-pixel parking cue. FALSE leaves outputs unchanged; use WRAM buffers. */
UBYTE td_get_parking(UBYTE stop,UWORD *u,UWORD *v) BANKED;
void td_get_job(UBYTE index,td_job_t *dest) BANKED;
/* Two 18-character lines followed by a terminator; caller provides 37 bytes. */
void td_get_brief(UBYTE index,char *dest) BANKED;
void td_get_street(UWORD u,UWORD v,char *dest) BANKED;
void td_get_west_street(UBYTE district,UWORD u,UWORD v,char *dest) BANKED;
void td_refresh_routes(UBYTE *identities,UWORD (*nearby)[2]) BANKED;
void td_get_district_name(UBYTE index,char *dest) BANKED;
extern UBYTE td_route_district; /* Rebuilt objective cue; not serialized. */
extern UBYTE td_resume_mode;
void td_ui_init(void) BANKED;
void td_ui_draw(void) BANKED;
void td_map_open(void) BANKED;
void td_map_update(UBYTE buttons,UBYTE pressed) BANKED;
void td_map_close(void) BANKED;
void td_save(void) BANKED;
UBYTE td_restore(void) BANKED;
void td_set_target(void) BANKED;
UBYTE td_service(UBYTE origin) BANKED;
UBYTE td_next_departure(UBYTE origin,UWORD seconds) BANKED;
/* Curbside props and transit berths (td_street.c). Not serialized. */
extern UBYTE td_prop_slot[TD_PROP_SLOTS],td_prop_sk[TD_PROP_SLOTS],td_prop_su8[TD_PROP_SLOTS],td_prop_sv8[TD_PROP_SLOTS];
extern UWORD td_prop_su[TD_PROP_SLOTS],td_prop_sv[TD_PROP_SLOTS];
extern UBYTE td_prop_sdown,td_prop_dirty;
void td_street_reset(void) BANKED;
/* Courier position in 8-pixel units. */
void td_street_refresh(UBYTE district,UBYTE pu8,UBYTE pv8) BANKED;
UBYTE td_street_berth(UBYTE stop,UBYTE district,UWORD *u,UWORD *v) BANKED;
#endif
