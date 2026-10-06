#ifndef TD_LIFE_INT_H
#define TD_LIFE_INT_H
/* Shared internals of the street-life module (td_life.c, td_drive.c,
 * td_life_draw.c). Not part of the city-state interface in td_life.h. */
#include "td_life.h"
#include "td_game.h"
#include "td_sprites.h"
#include "actor.h"
#include "scroll.h"
#include "collision.h"

/* Owned pedestrian slots (actors 9..16), positions in Q4. */
#define PK_FLY 1    /* thrown through the air */
#define PK_DOWN 2   /* on the ground, gets up later */
#define PK_DEAD 3   /* stays down until out of view */
#define PK_FLEE 4   /* running away from the courier */
#define PK_CHASE 5  /* officer pursuing the courier */
extern UBYTE pk_mode[TD_PEDS],pk_timer[TD_PEDS],pk_look[TD_PEDS],pk_dir[TD_PEDS],pk_span[TD_PEDS];
extern UWORD pk_u[TD_PEDS],pk_v[TD_PEDS];
extern BYTE pk_vu[TD_PEDS],pk_vv[TD_PEDS];
extern UBYTE pk_fresh,pk_lethal,pk_drawn;

/* Owned road vehicles (actors 2..7), positions in td_traffic_u/v (Q4). */
#define TR_PUSH 1    /* shoved by an impact */
#define TR_RETURN 2  /* easing back into its lane */
#define TR_CHASE 3   /* patrol car pursuing the courier */
#define TR_PARK 4    /* pursuit over: waits until out of view */
#define TR_GONE 5    /* stolen: hidden until its lane point is out of view */
#define TD_POLICE_SLOT 4
extern UBYTE tr_mode[6],tr_timer[6],tr_head[6],tr_spin;
extern BYTE tr_pu[6],tr_pv[6];
extern UWORD tr_au[6],tr_av[6];

/* Effects actor 20: spark, bullet or a driver running from a stolen car. */
#define FX_SPARK 1
#define FX_BULLET 2
#define FX_RUNNER 3
extern UBYTE fx_timer,fx_look;
/* A pistol round flies for this many updates (8 px each). */
#define LF_BULLET_TICKS 20
extern UWORD fx_u,fx_v;
extern BYTE fx_du,fx_dv;

extern UBYTE lf_warm,lf_flash,lf_punch,lf_hurt,lf_down,lf_arrest,lf_bust,lf_cop_cool,lf_exit_hold,lf_rev_wait,lf_shake,lf_chaos,lf_stuck,lf_axis,lf_stun;
/* Slot 4 is an ordinary blue car until a pursuit needs it: it becomes the
 * patrol car only while out of view and changes back the same way. */
extern UBYTE lf_patrol,lf_lost;
extern WORD lf_scale_x,lf_scale_y;
/* Ticks of firm contact before officers make an arrest. */
#define LF_BUST_TICKS 90
/* ROM tables are read from code in this file's own bank, so every module
 * keeps a private copy: a const table in another bank is not mapped.
 * A walker's look is its route identity&7 (TORONTO.c td_walker_bases):
 * look 5 is a police officer (navy cap), so one route in eight is an
 * officer; LF_LOOK_COURIER is the courier. Clothing colour is the actor's
 * palette offset. Road slot designs live in td_traffic_bases; slot 4 shows
 * the patrol car only while lf_patrol is set. */
#define LF_LOOK_OFFICER 5
#define LF_LOOK_COURIER 8
static const UBYTE lf_look_walk[9]={TD_FRAME_PERSON_SHORT,TD_FRAME_PERSON_LONG,TD_FRAME_PERSON_BUN,TD_FRAME_PERSON_PACK,
    TD_FRAME_PERSON_UMBRELLA,TD_FRAME_PERSON_CAP,TD_FRAME_PERSON_CAP,TD_FRAME_PERSON_LONG,TD_FRAME_PERSON_SHORT};
/* Civilian clothing colours by (route>>3)&3 (offsets for the people palette). */
static const UBYTE lf_civilian_pal[4]={TD_PEOPLE_PAL(TD_PAL_RED),TD_PEOPLE_PAL(TD_PAL_YELLOW),TD_PEOPLE_PAL(TD_PAL_TEAL),TD_PEOPLE_PAL(TD_PAL_VIOLET)};
#define LF_OFFICER_PAL TD_PEOPLE_PAL(TD_PAL_NAVY)
#define LF_IS_PATROL(i) ((i)==TD_POLICE_SLOT&&lf_patrol)

#define CR_MINOR 0
#define CR_GUN 1
#define CR_KILL 2
#define CR_COP 3
#define CR_COP_KILL 4

static UWORD lf_abs(WORD x){return x<0?(UWORD)-x:(UWORD)x;}
static UWORD lf_dist(UWORD a,UWORD b){return a>b?a-b:b-a;}
static WORD lf_div4(WORD x){return x<0?-(WORD)((UWORD)-x>>2):(WORD)((UWORD)x>>2);}
static WORD lf_div16(WORD x){return x<0?-(WORD)((UWORD)-x>>4):(WORD)((UWORD)x>>4);}
static BYTE lf_clamp(WORD x,BYTE lim){return x>lim?lim:x<-lim?-lim:(BYTE)x;}
static void lf_frame(actor_t *a,UBYTE f){
    if(a->frame_start!=f||a->frame_end!=f+1)actor_set_frames(a,f,f+1);
    a->anim_tick=255;
}
static void lf_place(actor_t *a,UWORD u,UWORD v){a->pos.x=u<<5;a->pos.y=v<<5;}
/* Q4 world position to a GBVM Q5 actor position on whole pixels. */
static void lf_place_q4(actor_t *a,UWORD u,UWORD v){a->pos.x=(u&0xFFF0)<<1;a->pos.y=(v&0xFFF0)<<1;}
/* Whole pixels: TRUE inside the camera view plus a margin that covers the
 * largest walker/vehicle frame, so anything spawned outside it arrives from
 * beyond the screen edge. */
static UBYTE lf_on_screen(UWORD u,UWORD v){
    return (UWORD)(u-(UWORD)scroll_x+24)<208&&(UWORD)(v-(UWORD)scroll_y+24)<192;
}
/* Cheap byte prefilter for Q4 positions: TRUE when the 16-pixel cells
 * (high bytes) of a and b differ by at most one, i.e. within 16..32 px. */
#define LF_NEAR_CELL(a,b) ((UBYTE)((UBYTE)((a)>>8)-(UBYTE)((b)>>8)+1)<3)
/* Collision probes read the loaded scene directly through TORONTO.c's
 * HOME-bank scanner (every city scene is 128 tiles wide); host builds use
 * tile_at. Collision 15 blocks; sidewalks/lots (16) and roads (0) are open. */
#ifdef __SDCC
extern const UBYTE *td_scan_row;
extern UBYTE td_scan_cols,td_scan_rows;
UBYTE td_scan_clear(void) NONBANKED;
static UBYTE lf_area(UBYTE l,UBYTE r,UBYTE t,UBYTE b){
    if(r>=image_tile_width||b>=image_tile_height)return FALSE;
    /* Row offset t*128 without a 16-bit shift loop. */
    td_scan_row=collision_ptr+(((UWORD)(UBYTE)(t>>1))<<8)+((t&1)?128:0)+l;
    td_scan_cols=r-l+1;td_scan_rows=b-t+1;
    return td_scan_clear();
}
#else
static UBYTE lf_area(UBYTE l,UBYTE r,UBYTE t,UBYTE b){
    UBYTE x,y;
    if(r>=image_tile_width||b>=image_tile_height)return FALSE;
    for(y=t;y<=b;y++)for(x=l;x<=r;x++)if(tile_at(x,y)&15)return FALSE;
    return TRUE;
}
#endif
static UBYTE lf_walk(UWORD u,UWORD v){
    if(u>=1024||v>=976)return FALSE;
    return lf_area((UBYTE)(u>>3),(UBYTE)(u>>3),(UBYTE)(v>>3),(UBYTE)(v>>3));
}

/* Cross-bank helpers. */
void td_lf_fx(UBYTE kind,UWORD u,UWORD v,UBYTE timer) BANKED;
void td_lf_crime(UBYTE kind) BANKED;
void td_lf_knock(UBYTE i,WORD vu,WORD vv,UBYTE lethal) BANKED;
void td_lf_own_car(UBYTE i,UBYTE mode) BANKED;
UBYTE td_lf_tr_heading(UBYTE i) BANKED;
/* Give traffic slot i a new design and colour (call while it is out of view). */
void td_lf_new_look(UBYTE i,UBYTE seed) BANKED;
/* Vehicle bodies (whole-pixel centre): 11px courier car, 13px road vehicle. */
UBYTE td_lf_drive(UWORD u,UWORD v) BANKED;
UBYTE td_lf_body(UWORD u,UWORD v) BANKED;
#endif
