#ifndef TD_GAME_H
#define TD_GAME_H
#include <gbdk/platform.h>
#define TD_QUESTS 72
#define TD_STOPS 27
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
typedef struct { UWORD u,v; char name[19]; UBYTE transit; } td_stop_t;
typedef struct { char title[19]; UBYTE kind,count,vehicle,min_done; UWORD seconds,reward; UBYTE route[12]; } td_job_t;
typedef struct {
    UWORD u,v,park_u,park_v,cash,seconds,left;
    WORD speed;
    UBYTE heading,vehicle,onfoot,mode,menu,job,stage,health,done,subsecond;
    UBYTE complete[9];
    UBYTE transit_origin,transit_target,ride_left,cooldown,msg;
    UWORD safe_u,safe_v,map_x,map_y;
} td_state_t;
extern td_state_t td;
extern td_job_t td_job,td_offer;
extern td_stop_t td_target,td_cursor;
void td_get_stop(UBYTE index,td_stop_t *dest) BANKED;
void td_get_job(UBYTE index,td_job_t *dest) BANKED;
/* Two 18-character lines followed by a terminator; caller provides 37 bytes. */
void td_get_brief(UBYTE index,char *dest) BANKED;
void td_get_street(UWORD u,UWORD v,char *dest) BANKED;
void td_ui_init(void) BANKED;
void td_ui_draw(void) BANKED;
void td_save(void) BANKED;
void td_set_target(void) BANKED;
UBYTE td_service(UBYTE origin) BANKED;
UBYTE td_next_departure(UBYTE origin,UWORD seconds) BANKED;
#endif
