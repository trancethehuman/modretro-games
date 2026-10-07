#pragma bank 255
#include <string.h>
#include "states/TORONTO.h"
#include "td_game.h"
#include "td_radio_data.h"
#include "td_world_routes.h"
#include "td_audio.h"
#include "td_district.h"
#include "td_world.h"
#include "td_transit.h"
#include "td_sprites.h"
#include "td_life.h"
#include "td_anim.h"
#include "td_daynight.h"
#include "actor.h"
#include "camera.h"
#include "scroll.h"
#include "collision.h"
#include "input.h"
#include "data_manager.h"
#include "ui.h"
#include "compat.h"
#include "system.h"

td_state_t td;
#ifdef __SDCC
/* The engine field MAX_ACTORS must hold every Toronto actor. */
typedef char td_actor_pool_fits[(MAX_ACTORS>=TD_ACTORS)?1:-1];
#endif
/* Lit vehicle frames follow the 32 daytime ones; frame indices are bytes. */
typedef char td_lit_frames_fit[(TD_FRAME_PLAYER_CAR_LIT+32<=TD_SPRITE_FRAMES&&TD_SPRITE_FRAMES<=256)?1:-1];
td_job_t td_job,td_offer;
td_stop_t td_target,td_cursor;
/* Registered engine field: stock bootstrap resets this on cold/soft boot. */
UBYTE td_session_live;
UBYTE td_route_district;
static UBYTE td_transition_pending;
typedef char td_serialized_state_must_be_58_bytes[(sizeof(td_state_t)==58)?1:-1];
static const BYTE td_dx[]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
static const BYTE td_dy[]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};
/* Core street centrelines (scripts/city_layout.py). */
static const UWORD td_rows[]={64,176,288,400,528,640,736,824};
/* Global: td_life.c moves owned vehicles (impacts, pursuit, theft). */
UWORD td_traffic_u[6],td_traffic_v[6];
td_traffic_sample_t td_traffic_samples[6];
/* Global: read by the native presentation routine and emulator checks. */
UWORD td_nearby_routes[TD_PEDS][2];
UBYTE td_ped_route[TD_PEDS];
UBYTE td_traffic_leg[6];
static UBYTE td_ped_refresh;
static UWORD td_ped_anchor_u,td_ped_anchor_v;
UBYTE td_tick,td_red_cooldown,td_turn_tick,td_entry_timer,td_entry_target,td_walk_dir;
static UBYTE td_notice_timer;
UBYTE td_resume_mode;
WORD td_vx,td_vy;
static UWORD td_last_frame;
UBYTE td_corner_used;
static UBYTE td_input_edge;
static UBYTE td_change_district(UBYTE district,UWORD u,UWORD v);

static UWORD td_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
/* SDCC has no hardware multiply/divide: its runtime helpers cost thousands of
 * cycles per call. Hot paths below use exact shift/add equivalents instead. */
static WORD td_div2(WORD x){return x<0?-(WORD)((UWORD)-x>>1):(WORD)((UWORD)x>>1);}
static WORD td_div4(WORD x){return x<0?-(WORD)((UWORD)-x>>2):(WORD)((UWORD)x>>2);}
static WORD td_div16(WORD x){return x<0?-(WORD)((UWORD)-x>>4):(WORD)((UWORD)x>>4);}
/* Exact d*s for the bounded heading component |d|<=16. */
static WORD td_scale(BYTE d,WORD s){
    UBYTE m=d<0?(UBYTE)-d:(UBYTE)d;WORD r=0;
    while(m){if(m&1)r+=s;s+=s;m>>=1;}
    return d<0?-r:r;
}
/* The signal phase changes once per game second; cache td.seconds%12. */
static UWORD td_phase_seconds;
static UBYTE td_phase_value;
static UBYTE td_signal_phase(void){
    if(td.seconds!=td_phase_seconds){td_phase_seconds=td.seconds;td_phase_value=td.seconds%12;}
    return td_phase_value;
}
/* Pedestrian phases use seven bits of seconds*12+subsecond/5+route*37, so
 * eight-bit wraparound is exact and avoids runtime multiply/divide calls. */
static const UBYTE td_fifth[60]={0,0,0,0,0,1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,4,5,5,5,5,5,
    6,6,6,6,6,7,7,7,7,7,8,8,8,8,8,9,9,9,9,9,10,10,10,10,10,11,11,11,11,11};
static UBYTE td_ped_base(void){
    UBYTE s=(UBYTE)td.seconds;
    return (UBYTE)((UBYTE)(s<<3)+(UBYTE)(s<<2)+(td.subsecond<60?td_fifth[td.subsecond]:td.subsecond/5));
}
static UBYTE td_ped_phase(UBYTE base,UBYTE route){
    return (UBYTE)(base+(UBYTE)(route<<5)+(UBYTE)(route<<2)+route)&127;
}
static void td_position(actor_t *a,UWORD u,UWORD v){
    // GBVM actors use five fractional bits; driving state uses four.
    a->pos.x=u*32; a->pos.y=v*32;
}
static void td_frame(actor_t *a,UBYTE f){if(a->frame_start!=f||a->frame_end!=f+1)actor_set_frames(a,f,f+1);a->anim_tick=255;}
/* The courier's vehicle; from dusk to dawn its frames carry the headlamp beam. */
/* The lit composites carry the beam in the courier's palette; a stolen car
 * in another paint draws its beam with a separate actor (td_life_draw.c). */
static UBYTE td_vehicle_frame(void){return (td_daynight_lights&&td_car_colour==TD_PAL_COURIER?TD_FRAME_PLAYER_CAR_LIT:0)+(td.vehicle<<3)+(((td.heading+1)&15)>>1);}
void td_message(UBYTE m) BANKED {td.msg=m;td_notice_timer=90;if(m==5||m==13)td_audio_play(TD_AUDIO_IMPACT);td_ui_draw();}
static void td_sound_update(void){td_audio_update(td.speed,td.vehicle,td.onfoot,!!INPUT_B,td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE);}
static UBYTE td_near(td_stop_t *s){return s->district==td.district&&td_distance(td.u>>4,s->u)<15&&td_distance(td.v>>4,s->v)<15;}
#ifdef __SDCC
/* tile_at() multiplies and switches banks for every byte. This HOME routine
 * scans td_scan_rows x td_scan_cols collision bytes with one switch, keeping
 * the fixed bank small; it returns TRUE only if every byte is zero. */
/* Global so native emulator checks can inspect the scan request. */
const UBYTE *td_scan_row;
UBYTE td_scan_cols,td_scan_rows;
UBYTE td_scan_clear(void) NONBANKED NAKED {
    __asm
        ldh a, (__current_bank)
        push af
        ld a, (_collision_bank)
        ldh (__current_bank), a
        ld (_rROMB0), a
        ld hl, #_td_scan_row
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        ld a, (_td_scan_rows)
        ld b, a
    1$:
        push hl
        ld a, (_td_scan_cols)
        ld c, a
    2$:
        ld a, (hl+)
        and a, #15
        jr nz, 3$
        dec c
        jr nz, 2$
        pop hl
        ld a, (_image_tile_width)
        add a, l
        ld l, a
        adc a, h
        sub a, l
        ld h, a
        dec b
        jr nz, 1$
        ld e, #1
        jr 4$
    3$:
        pop hl
        ld e, #0
    4$:
        pop af
        ldh (__current_bank), a
        ld (_rROMB0), a
        ld a, e
        ret
    __endasm;
}
#endif
static UBYTE td_walkable(UWORD u,UWORD v){
    if(u>=1024||v>=976)return FALSE;
    return !(tile_at(u>>3,v>>3)&15);
}
static UBYTE td_traffic_free(UWORD u,UWORD v){
    UBYTE i;
    for(i=0;i<6;i++)if(td_distance(u,td_traffic_u[i])<168&&td_distance(v,td_traffic_v[i])<168)return FALSE;
    return TRUE;
}
static UBYTE td_foot_free(UWORD u,UWORD v){
    return td_walkable(u>>4,v>>4)&&!(td.park_district==td.district&&td_distance(u,td.park_u)<168&&td_distance(v,td.park_v)<168)&&td_traffic_free(u,v);
}
static UBYTE td_near_car(void){return td.park_district==td.district&&td_distance(td.u,td.park_u)<384&&td_distance(td.v,td.park_v)<384;}
static UBYTE td_door_path(UWORD u,UWORD v,UWORD car_u,UWORD car_v){
    UBYTE i;WORD du=(WORD)car_u-(WORD)u,dv=(WORD)car_v-(WORD)v;
    /* A one-pixel sample catches rails and corners between usable endpoints. */
    for(i=0;i<=24;i++)if(!td_walkable((u+du*i/24)>>4,(v+dv*i/24)>>4))return FALSE;
    return TRUE;
}
static void td_enter_exit(void){
    UWORD door_u=td.u,door_v=td.v;UBYTE found=0,i;
    if(td.speed>2||td.speed<-2){td_message(11);return;}
    if(td.onfoot){
        if(!td_near_car()){td_message(9);return;}
        if(!td_door_path(td.u,td.v,td.park_u,td.park_v)){td_message(15);return;}
        td_entry_target=0;
    }else{
        // Choose a visible, walkable door side without spawning inside a building.
        for(i=0;i<4;i++){
            door_u=td.u;door_v=td.v;
            if(i==0)door_v+=288;else if(i==1)door_u+=288;else if(i==2)door_v-=288;else door_u-=288;
            if(td_door_path(td.u,td.v,door_u,door_v)&&td_traffic_free(door_u,door_v)){found=1;break;}
        }
        if(!found){td_message(15);return;}
        td.park_u=td.u;td.park_v=td.v;td.park_district=td.district;td.u=door_u;td.v=door_v;td.onfoot=1;td_entry_target=1;
        td_aim_dir=((td.heading+1)&15)>>1;
    }
    td.speed=0;td_vx=td_vy=0;td_entry_timer=12;td.mode=TD_ROAM;td_set_target();td_save();td_ui_draw();
}
void td_get_district_name(UBYTE index,char *dest) BANKED {
    if(!td_world_name(index,dest))strcpy(dest,"UNKNOWN DISTRICT");
}
void td_set_target(void) BANKED {
    td_portal_t portal;UBYTE routed,stop=td.job!=TD_NONE?td_job.route[td.stage]:0;
    td_get_stop(stop,&td_target);
    /* Show the legal approach while driving, then the actual client on foot. */
    if(!td.onfoot&&(td_target.reserved&TD_STOP_FOOT))td_get_parking(stop,&td_target.u,&td_target.v);
    routed=td_world_route(td.district,td_target.district,td.onfoot,td.u>>4,td.v>>4,td_target.u,td_target.v,&portal);
    td_route_district=routed?portal.to:TD_DISTRICT_NONE;
    td_beacon_u=routed?portal.u:td_target.u;td_beacon_v=routed?portal.v:td_target.v;
    td_beacon_shown=td_target.district==td.district||routed;
    td_position(&actors[1],td_beacon_u,td_beacon_v-12);
    if(!td_beacon_shown)actors[1].flags|=ACTOR_FLAG_HIDDEN;
    else actors[1].flags&=~ACTOR_FLAG_HIDDEN;
}
static UBYTE td_change_district(UBYTE district,UWORD u,UWORD v){
    if(td.onfoot?!td_district_walkable(district,u>>4,v>>4):!td_district_drivable(district,u>>4,v>>4))return FALSE;
    if(!td_district_queue(district))return FALSE;
    td.district=district;td.u=u;td.v=v;td.safe_u=u;td.safe_v=v;
    if(!td.onfoot){td.park_district=district;td.park_u=u;td.park_v=v;}
    td_transition_pending=1;td_save();return TRUE;
}
static UBYTE td_cross_portal(UWORD old_u,UWORD old_v){
    td_crossing_t crossing;
    if(td_entry_timer||td.mode!=TD_ROAM||(td.u==old_u&&td.v==old_v))return FALSE;
    /* Most motion stays away from seams and needs no ROM bank switch. */
    if(td.u>24*16&&td.u<1000*16&&td.v>24*16&&td.v<952*16)return FALSE;
    if(!td_world_crossing(td.district,td.onfoot,old_u,old_v,td.u,td.v,&crossing))return FALSE;
    return td_change_district(crossing.district,crossing.u,crossing.v);
}
void td_finish(UBYTE success) BANKED {
    UBYTE done_before=td.done;
    td_audio_play(success?TD_AUDIO_COMPLETE:TD_AUDIO_FAIL);
    if(success){
        if(!(td.complete[td.job>>3]&(1<<(td.job&7)))){td.complete[td.job>>3]|=1<<(td.job&7);td.done++;}
        { UWORD reward=td_job.reward/100*td.health+(td_job.reward%100)*td.health/100+td.left/5;
          td.cash=td.cash>60000-reward?60000:td.cash+reward;td_last_pay=reward; }
        /* The fee pops up over the courier once the result closes. */
        td_anim_spawn(TD_PART_POP,TD_FRAME_PICKUP_CASH,td.u>>4,(td.v>>4)-10);
        td_anim_spawn(TD_PART_FLASH,0,td.u>>4,(td.v>>4)-12);
    }
    /* Rosa calls once the result card closes: a new chapter, the west/east
     * routes opening, the last contract, or a word on the delivery. */
    if(!success)td_radio_say(TD_RADIO_FAIL);
    else if(td.done==TD_QUESTS)td_radio_say(TD_RADIO_MASTER);
    else if(td.done!=done_before&&td.done%6==0&&td.done<=6*TD_RADIO_CHAPTER_COUNT)td_radio_say(TD_RADIO_CHAPTER+td.done/6-1);
    else if(td.done!=done_before&&td.done==3)td_radio_say(TD_RADIO_OPEN_ENDS);
    else td_radio_say(TD_RADIO_DONE+td.done%TD_RADIO_DONE_COUNT);
    td.job=TD_NONE;td.speed=0;td_vx=td_vy=0;td.mode=TD_RESULT;td_set_target();td_save();td_ui_draw();
}
static void td_ready_offer(void){
    UBYTE i;
    for(i=0;i<TD_QUESTS;i++){
        td_get_job(i,&td_offer);
        if(td.done>=td_offer.min_done&&!(td.complete[i>>3]&(1<<(i&7)))&&(td_offer.vehicle==TD_NONE||(!td.onfoot&&td.vehicle==td_offer.vehicle))){td.menu=i;return;}
    }
    td.menu=0;td_get_job(0,&td_offer);
}
static void td_interact(void){
    if(td.speed>2||td.speed<-2){td_message(1);return;}
    if(td.job==TD_NONE){td.mode=TD_BOARD;td_ready_offer();td_ui_draw();return;}
    if(!td_near(&td_target)){td_message(6);return;}
    if((td_target.reserved&TD_STOP_FOOT)&&!td.onfoot){td_message(16);return;}
    if(td_job.vehicle!=TD_NONE&&(td.onfoot||td.vehicle!=td_job.vehicle)){td_message(2);return;}
    td.stage++;
    if(td.stage==td_job.count){td_finish(TRUE);return;}
    td_audio_play(TD_AUDIO_PICKUP);td_anim_spawn(TD_PART_POP,TD_FRAME_PARCEL,td.u>>4,(td.v>>4)-10);
    if(td.stage==1)td_radio_say(TD_RADIO_PICKUP);
    td_set_target();td_save();td_ui_draw();
}
/* Stations reach the sign on the sidewalk beside the curb-lane stop. The
 * current district's boarding stops are cached so the HUD prompt can look
 * for one cheaply. */
#define TD_STATION_CACHE 20
/* Positions kept in 4-pixel units (bytes); ample for a 24-pixel reach. */
static UBYTE td_st_district=255,td_st_count,td_st_id[TD_STATION_CACHE],td_st_u[TD_STATION_CACHE],td_st_v[TD_STATION_CACHE];
static UBYTE td_origin(void){
    UBYTE i,pu=(UBYTE)(td.u>>6),pv=(UBYTE)(td.v>>6);td_stop_t s;
    if(td_st_district!=td.district){
        td_st_district=td.district;td_st_count=0;
        for(i=0;i<TD_STOPS&&td_st_count<TD_STATION_CACHE;i++){
            td_get_stop(i,&s);
            if(s.transit&&s.district==td.district&&td_transit_can_origin(i)){
                td_st_id[td_st_count]=i;td_st_u[td_st_count]=(UBYTE)(s.u>>2);td_st_v[td_st_count]=(UBYTE)(s.v>>2);td_st_count++;
            }
        }
    }
    for(i=0;i<td_st_count;i++)
        if((UBYTE)(pu-td_st_u[i]+5)<11&&(UBYTE)(pv-td_st_v[i]+5)<11)return td_st_id[i];
    return TD_NONE;
}
/* The station within reach on foot, refreshed a few times a second for the HUD. */
UBYTE td_station_near=TD_NONE;
static UBYTE td_station_tick;
UBYTE td_service(UBYTE origin) BANKED {
    return td_transit_service(origin);
}
UBYTE td_next_departure(UBYTE origin,UWORD seconds) BANKED {
    return td_transit_departure(origin,origin&63,seconds);
}
static UBYTE td_route_stop(UBYTE origin,UBYTE idx){
    return td_transit_stop(origin,idx);
}
/* Visible transit. While the courier waits, the scheduled bus, streetcar or
 * ferry decelerates into its berth so that it stops exactly as the boarding
 * window opens; it then leaves with the courier aboard, and at the
 * destination it sets the courier down, dwells and pulls away. Positions are
 * derived from the timetable clock, so nothing here is saved or affects
 * when a journey departs or arrives. Line 1 runs underground. */
static UBYTE td_tv_phase,td_tv_stop,td_tv_heading,td_tv_service,td_tv_shown,td_ride_hidden;
static UWORD td_tv_frames;
static void td_tv_begin(UBYTE phase,UBYTE stop){
    td_tv_phase=phase;td_tv_stop=stop;td_tv_frames=0;
    td_tv_service=td_transit_service(td.transit_origin);
    td_tv_heading=td_transit_heading(td.transit_origin,td.transit_target);
}
/* offset: pixels from the berth, behind it when arriving, ahead when leaving. */
static void td_tv_show(UBYTE service,UBYTE stop,UBYTE heading,UWORD offset,UBYTE arriving){
    actor_t *a=&actors[TD_ACTOR_TRANSIT];UWORD u,v;UBYTE frame;
    td_tv_shown=0;
    UBYTE berth;
    if(service==TD_TRANSIT_TRAIN||heading>TD_HEADING_NORTH||offset>240||!(berth=td_street_berth(stop,td.district,&u,&v))){a->flags|=ACTOR_FLAG_HIDDEN;return;}
    /* Ferries come in from, and leave over, the open water beside the dock. */
    if(berth>1){if(berth==2)v+=offset;else v-=offset;offset=0;}
    if(heading==TD_HEADING_EAST){v+=8;if(arriving)u-=offset;else u+=offset;}
    else if(heading==TD_HEADING_WEST){v-=8;if(arriving)u+=offset;else u-=offset;}
    else if(heading==TD_HEADING_SOUTH){if(arriving)v-=offset;else v+=offset;}
    else{if(arriving)v+=offset;else v-=offset;}
    if(u>4000||v>4000){a->flags|=ACTOR_FLAG_HIDDEN;return;}
    /* Buses and streetcars run east-west, ferries north-south. Tall frames
     * grow upward from their bottom row; centre them on (u,v). */
    if(service==TD_TRANSIT_FERRY){frame=heading==TD_HEADING_NORTH?TD_FRAME_FERRY_N:TD_FRAME_FERRY_S;v+=TD_ANCHOR_FERRY_S_DY;}
    else frame=(service==TD_TRANSIT_BUS?TD_FRAME_BUS_E:TD_FRAME_STREETCAR_E)+(heading&1);
    td_position(a,u,v);td_frame(a,frame);a->flags&=~ACTOR_FLAG_HIDDEN;td_tv_shown=1;
}
static void td_transit_present(UBYTE frames){
    UBYTE departure;UWORD wait;
    td_tv_frames+=frames;
    if(td.mode==TD_WAIT){
        departure=td_transit_departure(td.transit_origin,td.transit_target,td.seconds);
        if(departure==TD_TRANSIT_NONE||departure>4)wait=999;
        else wait=departure?departure*60-td.subsecond:0;
        td_tv_show(td_transit_service(td.transit_origin),td.transit_origin&63,
                   td_transit_heading(td.transit_origin,td.transit_target),wait>240?999:(wait*wait)>>8,1);
    }else if(td_tv_phase){
        wait=td_tv_frames;
        if(td_tv_phase==2)wait=wait<90?0:wait-90;
        if(wait>248)td_tv_phase=0;
        td_tv_show(td_tv_service,td_tv_stop,td_tv_heading,wait>248?999:(wait*wait)>>8,0);
    }else td_tv_show(TD_TRANSIT_TRAIN,0,0,0,0);
    /* The courier is aboard (or below ground) for the whole ride. */
    if(td.mode==TD_RIDE){PLAYER.flags|=ACTOR_FLAG_HIDDEN;td_ride_hidden=1;}
    else if(td_ride_hidden){td_ride_hidden=0;PLAYER.flags&=~ACTOR_FLAG_HIDDEN;}
}
/* Sidewalk pickups: two nearby slots from the district table. Walking or
 * driving over one collects it: cash, first aid (not at full vitality) or
 * ammunition (not when full). While a transit vehicle is shown the pickups
 * are hidden so actors never need more than 40 hardware sprites. Actors are
 * only rewritten when a slot changes or its highlight blinks. */
static UBYTE td_pickups_hidden;
static void td_pickup_collect(UBYTE i){
    UBYTE k=td_pickup_sk[i];
    if(k==1){
        if(td.vitality>=100)return;
        td.vitality=td.vitality>100-TD_PICKUP_FIRST_AID?100:td.vitality+TD_PICKUP_FIRST_AID;td_message(TD_MSG_FIRST_AID);
    }else if(k==2){
        if(td.ammo>=TD_AMMO_MAX)return;
        td.ammo=td.ammo>TD_AMMO_MAX-TD_PICKUP_AMMO?TD_AMMO_MAX:td.ammo+TD_PICKUP_AMMO;td_message(TD_MSG_AMMO);
    }else{
        td.cash=td.cash>60000-TD_PICKUP_CASH?60000:td.cash+TD_PICKUP_CASH;td_message(TD_MSG_CASH);
    }
    td_audio_play(TD_AUDIO_PICKUP);
    td_anim_spawn(TD_PART_POP,TD_FRAME_PICKUP_CASH+k,td_pickup_su[i],td_pickup_sv[i]);
    td_street_take(i);
}
static void td_pickups_present(void){
    UBYTE i,mask,pu8,pv8;actor_t *a;UWORD pu=td.u>>4,pv=td.v>>4;
    pu8=pu>>3;pv8=pv>>3;
    td_street_refresh(td.district,pu8,pv8);
    if(td_tv_shown!=td_pickups_hidden){td_pickups_hidden=td_tv_shown;td_pickup_dirty=(1<<TD_PICKUP_SLOTS)-1;}
    if(td.mode==TD_ROAM&&!td_entry_timer&&!td_pickups_hidden&&!td_life_locked()){
        for(i=0;i<TD_PICKUP_SLOTS;i++){
            if(td_pickup_slot[i]==TD_NONE||(UBYTE)(td_pickup_su8[i]-pu8+2)>4||(UBYTE)(td_pickup_sv8[i]-pv8+2)>4)continue;
            if(td_distance(td_pickup_su[i],pu)<9&&td_distance(td_pickup_sv[i],pv)<10)td_pickup_collect(i);
        }
    }
    /* Pickups bob a pixel every quarter second. */
    if(!(td_tick&15))td_pickup_dirty=(1<<TD_PICKUP_SLOTS)-1;
    if(!td_pickup_dirty)return;
    for(i=0,mask=1,a=&actors[TD_ACTOR_PICKUPS];i<TD_PICKUP_SLOTS;i++,a++,mask<<=1){
        if(!(td_pickup_dirty&mask))continue;
        if(td_pickup_slot[i]==TD_NONE||td_pickups_hidden){a->flags|=ACTOR_FLAG_HIDDEN;continue;}
        td_position(a,td_pickup_su[i],td_pickup_sv[i]-((td_tick>>4)&1));
        td_frame(a,TD_FRAME_PICKUP_CASH+td_pickup_sk[i]);
        a->flags&=~ACTOR_FLAG_HIDDEN;
    }
    td_pickup_dirty=0;
}
static UBYTE td_board_current_window(void){
    UBYTE fare;
    if(!td_transit_valid(td.transit_origin,td.transit_target)||td_transit_departure(td.transit_origin,td.transit_target,td.seconds))return FALSE;
    fare=td_transit_fare(td.transit_origin);
    if(td.cash<fare){td.mode=TD_ROAM;td_save();td_message(4);return TRUE;}
    /* A fresh free-roaming trip cannot inherit an old contract failure. */
    if(td.job==TD_NONE)td.health=100;
    td.cash-=fare;td.mode=TD_RIDE;td.ride_left=td_transit_duration(td.transit_origin,td.transit_target);
    td_tv_begin(1,td.transit_origin&63);
    td_audio_play(TD_AUDIO_TRANSIT);td_save();return TRUE;
}
static void td_transit_open(void){
    UBYTE origin;
    if(td_entry_timer){td_message(2);return;}
    if(!td.onfoot){td_message(9);return;}
    if(td.job!=TD_NONE&&(td_job.kind==3||td_job.kind==5)){td_message(8);return;}
    origin=td_origin();if(origin==TD_NONE){td_message(6);return;}
    td.transit_origin=origin;td.menu=0;
    if(origin>=TD_TRANSIT_QUEEN_FIRST&&origin<TD_TRANSIT_QUEEN_FIRST+TD_TRANSIT_QUEEN_COUNT){
        td.menu=origin-TD_TRANSIT_QUEEN_FIRST+1;
        if(td.menu>=TD_TRANSIT_QUEEN_COUNT)td.menu=TD_TRANSIT_QUEEN_COUNT-2;
    }
    td.transit_target=td_route_stop(origin,td.menu);
    /* Never offer the stop the courier is standing at. */
    if(td.transit_target==origin){td.menu=(td.menu+1)%td_transit_count(origin);td.transit_target=td_route_stop(origin,td.menu);}
    td_get_stop(td.transit_target,&td_cursor);td.mode=TD_TRANSIT;td_ui_draw();
}
static void td_transit_step(BYTE delta){
    UBYTE count=td_transit_count(td.transit_origin),n;
    for(n=0;n<count;n++){
        td.menu=(td.menu+count+delta)%count;td.transit_target=td_route_stop(td.transit_origin,td.menu);
        if(td.transit_target!=(td.transit_origin&63))break;
    }
    td_get_stop(td.transit_target,&td_cursor);
}
static void td_pause_choose(void){
    if((td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)&&td.menu>1&&td.menu!=8){td_message(2);return;}
    switch(td.menu){
        case 0:td.mode=td_resume_mode;break;
        case 1:td.mode=TD_MAP;td_map_open();break;
        case 2:td.mode=TD_BOARD;if(td.job==TD_NONE)td_ready_offer();else{td.menu=td.job;td_get_job(td.menu,&td_offer);}break;
        case 3:td_enter_exit();return;
        case 4:
            if(td.job!=TD_NONE||td.speed>2||td.speed<-2||td.onfoot){td_message(2);return;}
            td.vehicle=(td.vehicle+1)&3;td_car_damage=0;td_car_colour=TD_PAL_COURIER;td_save();break;
        case 5:td_transit_open();return;
        case 6:if(td_life_buy())td_save();td.mode=TD_ROAM;break;
        case 7:td.job=TD_NONE;td.speed=0;td.mode=TD_ROAM;td_set_target();td_save();break;
        case 8:td_audio_set_mode((td_audio_get_mode()+1)%TD_AUDIO_MODES);break;
    }
    td_audio_play(TD_AUDIO_MENU);
    td_ui_draw();
}
static void td_menu_update(void){
    td_ui_tick();
    if(td.mode==TD_HELP){
        if(INPUT_A_PRESSED||INPUT_B_PRESSED){
            /* A fresh shift starts with Rosa's welcome. */
            if(!td.done&&td.job==TD_NONE)td_radio_say(TD_RADIO_INTRO);
            td.mode=td_resume_mode;td_ui_draw();
        }
        return;
    }
    if(td.mode==TD_BUSTED||td.mode==TD_WASTED){if(INPUT_A_PRESSED||INPUT_B_PRESSED){td.mode=TD_ROAM;td_resume_mode=TD_ROAM;td_ui_draw();}return;}
    if(td.mode==TD_MAP){
        if(INPUT_B_PRESSED||INPUT_START_PRESSED){td_map_close();td.mode=TD_PAUSE;td.menu=1;td_ui_draw();}
        else td_map_update(joy,joy_pressed);
        return;
    }
    if(INPUT_B_PRESSED||INPUT_START_PRESSED){td.mode=td.mode==TD_PAUSE?td_resume_mode:TD_ROAM;td_ui_draw();return;}
    if(td.mode==TD_PAUSE){
        if(INPUT_DOWN_PRESSED)td.menu=(td.menu+1)%9;
        if(INPUT_UP_PRESSED)td.menu=(td.menu+8)%9;
        if(INPUT_A_PRESSED){td_pause_choose();return;}
    }else if(td.mode==TD_BOARD){
        if(INPUT_RIGHT_PRESSED){td.menu=(td.menu+1)%TD_QUESTS;td_get_job(td.menu,&td_offer);}
        if(INPUT_LEFT_PRESSED){td.menu=(td.menu+TD_QUESTS-1)%TD_QUESTS;td_get_job(td.menu,&td_offer);}
        if(INPUT_A_PRESSED){
            if(td.job!=TD_NONE){td_message(2);return;}
            if(td.done<td_offer.min_done){td_message(3);return;}
            if(td_offer.vehicle!=TD_NONE&&(td.onfoot||td.vehicle!=td_offer.vehicle)){td_message(2);return;}
            td.job=td.menu;td_job=td_offer;td.stage=0;td.health=100;td.left=td_job.seconds;td.mode=TD_ROAM;td_audio_play(TD_AUDIO_MENU);
            td_radio_say(TD_RADIO_JOB+td_job.kind);td_set_target();td_save();td_ui_draw();return;
        }
    }else if(td.mode==TD_TRANSIT){
        if((INPUT_UP_PRESSED||INPUT_DOWN_PRESSED)&&(td.transit_origin&63)==16){
            td.transit_origin^=64;td.menu=0;td.transit_target=td_route_stop(td.transit_origin,0);td_get_stop(td.transit_target,&td_cursor);
        }
        if(INPUT_RIGHT_PRESSED&&td_transit_count(td.transit_origin))td_transit_step(1);
        if(INPUT_LEFT_PRESSED&&td_transit_count(td.transit_origin))td_transit_step(-1);
        if(INPUT_A_PRESSED){
            if(!td_transit_valid(td.transit_origin,td.transit_target))return;
            /* Short of the fare: say so before waiting for a departure. */
            if(td.cash<td_transit_fare(td.transit_origin)){td.mode=TD_ROAM;td_save();td_message(4);return;}
            td.mode=TD_WAIT;td.speed=0;
            /* The displayed two-second window includes the current second;
               confirmation must not wait for another clock tick to board. */
            if(!td_board_current_window())td_save();
            td_ui_draw();return;
        }
    }else if(td.mode==TD_RESULT&&INPUT_A_PRESSED){td.mode=TD_BOARD;td_ready_offer();}
    if(INPUT_A_PRESSED||INPUT_UP_PRESSED||INPUT_DOWN_PRESSED||INPUT_LEFT_PRESSED||INPUT_RIGHT_PRESSED)td_ui_draw();
}
static UBYTE td_alight_clear(UWORD u,UWORD v){
    if(u>=1024*16||v>=976*16||!td_district_walkable(td_cursor.district,u>>4,v>>4))return FALSE;
    if(td.park_district==td_cursor.district&&td_distance(u,td.park_u)<168&&td_distance(v,td.park_v)<168)return FALSE;
    /* A remote scene's traffic is not loaded yet. Queen's new platforms are
     * on sidewalks; do not test them against the origin's vehicle cache. */
    return td_cursor.district!=td.district||td_traffic_free(u,v);
}
static UBYTE td_alight_position(UWORD *dest_u,UWORD *dest_v){
    UWORD origin_u=td_cursor.u*16,origin_v=td_cursor.v*16,u,v;
    UBYTE i,n,clear;WORD du,dv,offset;
    if(td_alight_clear(origin_u,origin_v)){*dest_u=origin_u;*dest_v=origin_v;return TRUE;}
    /* Stay within the stop's neighbourhood and retain foot access to it.
     * Twelve pixels clears the10.5px vehicle exclusion and remains inside
     * the15px interaction radius;18px is the existing car-door fallback. */
    for(i=0;i<8;i++){
        offset=i<4?192:288;du=dv=0;
        switch(i&3){case 0:du=offset;break;case 1:du=-offset;break;case 2:dv=offset;break;case 3:dv=-offset;break;}
        u=origin_u+du;v=origin_v+dv;
        if(!td_alight_clear(u,v))continue;
        clear=1;
        for(n=0;n<=18;n++)if(!td_district_walkable(td_cursor.district,(origin_u+du*n/18)>>4,(origin_v+dv*n/18)>>4)){clear=0;break;}
        if(clear){*dest_u=u;*dest_v=v;return TRUE;}
    }
    return FALSE;
}
static UBYTE td_ui_pending,td_dn_pending;
static void td_second(void){
    UWORD arrival_u,arrival_v;
    td.seconds++;
    /* The next update starts in VBlank: change the palettes there. */
    td_dn_pending=1;
    if(td.job!=TD_NONE){if(td.left)td.left--;if(!td.left){td.health=0;
        /* A failed parcel still finishes its already-paid trip; never strand it in transit. */
        if(td.mode==TD_RIDE){td.job=TD_NONE;td_set_target();td_save();}else{td_finish(FALSE);return;}
    }}
    if(td.mode==TD_WAIT){
        if(td_board_current_window()&&td.mode!=TD_RIDE)return;
    }else if(td.mode==TD_RIDE){
        if(td.ride_left)td.ride_left--;
        if(!td.ride_left){
            if(!td_alight_position(&arrival_u,&arrival_v)){
                td.ride_left=1;td_save();td_ui_draw();return;
            }
            /* Commit an alighted state before queuing another scene. A saved
             * paid ride still belongs to its origin district until arrival. */
            td.mode=td.health?TD_ROAM:TD_RESULT;
            td_tv_begin(2,td.transit_target);
            if(td_cursor.district!=td.district){
                if(!td_change_district(td_cursor.district,arrival_u,arrival_v)){
                    td.mode=TD_RIDE;td.ride_left=1;td_save();td_ui_draw();return;
                }
                td_resume_mode=td.mode;td_audio_play(td.health?TD_AUDIO_TRANSIT:TD_AUDIO_FAIL);return;
            }
            td.u=arrival_u;td.v=arrival_v;td.safe_u=td.u;td.safe_v=td.v;
            td_resume_mode=td.mode;td_set_target();td_audio_play(td.health?TD_AUDIO_TRANSIT:TD_AUDIO_FAIL);
        }
    }
    td_life_second();
    /* The HUD repaint follows on the next frame so the save and the repaint
     * never share one frame's CPU time; clocks and state are already final. */
    td_save();td_ui_pending=1;
}
/* Q4 stop lines: td_rows/td_cols*16 -/+ 384 for forward/reverse approaches. */
#define TD_STOP_LINES(a,b,c,d,e,f,g,h,o) {a*16+o,b*16+o,c*16+o,d*16+o,e*16+o,f*16+o,g*16+o,h*16+o}
static const UWORD td_stop_rows[2][8]={TD_STOP_LINES(64,176,288,400,528,640,736,824,-384),TD_STOP_LINES(64,176,288,400,528,640,736,824,384)};
/* Nine entries for the fixed-count scan: Broadview is repeated. */
#define TD_STOP_COLS(o) {64*16+o,160*16+o,256*16+o,384*16+o,512*16+o,640*16+o,784*16+o,928*16+o,928*16+o}
static const UWORD td_stop_cols[2][9]={TD_STOP_COLS(-384),TD_STOP_COLS(384)};
static UBYTE td_signal_stop(UWORD pos,UBYTE vertical,UBYTE reverse){
    UBYTE count=vertical?8:9;const UWORD *line=vertical?td_stop_rows[reverse?1:0]:td_stop_cols[reverse?1:0];
    do{if(pos>=*line&&pos<*line+8)return TRUE;line++;}while(--count);
    return FALSE;
}
/* Core loops in Q4: east/westbound lanes on td_rows[2..5] sit 8px either side
 * of the centreline; the bus loop visits its six fixed junctions. */
static const UWORD td_core_lane_v[4]={288*16,400*16,528*16,640*16};
/* 94 Wellesley: Ossington station, Ossington, Harbord/Wellesley, Parliament,
 * Castle Frank on Bloor, then back west along Bloor. */
static const UWORD td_bus_u[6]={160*16,160*16,784*16,784*16,836*16,512*16};
static const UWORD td_bus_v[6]={64*16,176*16,176*16,64*16,64*16,64*16};
/* A road vehicle never drives into the courier's car: a step that would end
 * overlapping it is undone, so traffic queues behind or stops in front.
 * Vehicles already overlapping (an impact in progress) may move apart. */
/* Bit i set when road vehicle i lies in the courier's 16-pixel cell or a
 * neighbouring one (Q4 high bytes differ by at most one on both axes), i.e.
 * the only vehicles that can touch the courier or the car this step. */
#ifdef __SDCC
UBYTE td_nc_uh,td_nc_vh;
UBYTE td_near_cells(void) NONBANKED NAKED {
    __asm
        ld a, (_td+1)
        ld (_td_nc_uh), a
        ld a, (_td+3)
        ld (_td_nc_vh), a
        ld hl, #(_td_traffic_u+1)
        ld de, #(_td_traffic_v+1)
        ld bc, #0x0100
    1$:
        ld a, (_td_nc_uh)
        sub a, (hl)
        inc a
        cp a, #3
        jr nc, 2$
        ld a, (_td_nc_vh)
        push hl
        ld h, d
        ld l, e
        sub a, (hl)
        pop hl
        inc a
        cp a, #3
        jr nc, 2$
        ld a, c
        or a, b
        ld c, a
    2$:
        inc hl
        inc hl
        inc de
        inc de
        sla b
        ld a, b
        cp a, #64
        jr nz, 1$
        ld a, c
        ret
    __endasm;
}
#else
UBYTE td_near_cells(void){
    UBYTE i,m=0,uh=(UBYTE)(td.u>>8),vh=(UBYTE)(td.v>>8);
    for(i=0;i<6;i++)if((UBYTE)(uh-(UBYTE)(td_traffic_u[i]>>8)+1)<3&&(UBYTE)(vh-(UBYTE)(td_traffic_v[i]>>8)+1)<3)m|=1<<i;
    return m;
}
#endif
static void td_traffic_block(const UWORD *old_u,const UWORD *old_v,const UBYTE *old_leg){
    UBYTE i,uh=(UBYTE)(td.u>>8),vh=(UBYTE)(td.v>>8);
    for(i=0;i<6;i++){
        /* Byte prefilter: only vehicles in a neighbouring 16-pixel cell. */
        if((UBYTE)((UBYTE)(td_traffic_u[i]>>8)-uh+1)>=3||(UBYTE)((UBYTE)(td_traffic_v[i]>>8)-vh+1)>=3)continue;
        if(td_distance(td_traffic_u[i],td.u)>=200||td_distance(td_traffic_v[i],td.v)>=200)continue;
        if(td_distance(old_u[i],td.u)<200&&td_distance(old_v[i],td.v)<200)continue;
        td_traffic_u[i]=old_u[i];td_traffic_v[i]=old_v[i];td_traffic_leg[i]=old_leg[i];
    }
}
#ifdef __SDCC
/* Traffic stepping kernel. Inputs: td_tk_i (first vehicle), td_tk_phase,
 * td_tk_district, td_tk_yield, td_tk_hit, td_tk_pu/pv (player Q4). It moves
 * vehicles td_tk_i..5 exactly as the C reference below and returns the index
 * of the first vehicle that strikes the car (C applies the effects and
 * resumes after it), or 6 when done. td_tk_dirty records district arrivals. */
UBYTE td_tk_i,td_tk_phase,td_tk_district,td_tk_yield,td_tk_hit,td_tk_dirty,td_tk_leg;
/* Bit masks for slot indices 0..5, shared by the kernels below. */
const UBYTE td_slot_bits[6]={1,2,4,8,16,32};
UWORD td_tk_pu,td_tk_pv,td_tk_u,td_tk_v,td_tk_tu,td_tk_tv;
UBYTE td_traffic_kernel(void) NAKED {
    __asm
    60$:
        ld a, (_td_tk_i)
        cp a, #6
        jr c, 61$
        ld a, #6
        ret
    61$:
        ld c, a
        ld b, #0
        ld hl, #_td_slot_bits
        add hl, bc
        ld a, (_td_tr_ctrl)
        and a, (hl)
        jp nz, 85$
        ld hl, #_td_traffic_u
        add hl, bc
        add hl, bc
        ld a, (hl+)
        ld (_td_tk_u), a
        ld a, (hl)
        ld (_td_tk_u+1), a
        ld hl, #_td_traffic_v
        add hl, bc
        add hl, bc
        ld a, (hl+)
        ld (_td_tk_v), a
        ld a, (hl)
        ld (_td_tk_v+1), a
        ld hl, #_td_traffic_leg
        add hl, bc
        ld a, (hl)
        ld (_td_tk_leg), a
        ld a, (_td_tk_district)
        or a, a
        jr z, 62$
        ld hl, #_td_traffic_samples
        add hl, bc
        add hl, bc
        add hl, bc
        add hl, bc
        add hl, bc
        add hl, bc
        ld de, #_td_tk_tu
        ld a, (hl+)
        ld (de), a
        inc de
        ld a, (hl+)
        ld (de), a
        inc de
        ld a, (hl+)
        ld (de), a
        inc de
        ld a, (hl)
        ld (de), a
        jp 70$
    62$:
        ld a, c
        cp a, #4
        jp nc, 66$
        ld de, #0x3480
        ld a, (_td_tk_leg)
        cp a, #2
        jr c, 63$
        ld de, #0x0300
    63$:
        ld a, e
        ld (_td_tk_tu), a
        ld a, d
        ld (_td_tk_tu+1), a
        ld hl, #_td_core_lane_v
        add hl, bc
        add hl, bc
        ld a, (hl+)
        ld e, a
        ld d, (hl)
        ld hl, #0x0080
        ld a, (_td_tk_leg)
        or a, a
        jr z, 64$
        cp a, #3
        jr nz, 65$
    64$:
        ld hl, #0xff80
    65$:
        add hl, de
        ld a, l
        ld (_td_tk_tv), a
        ld a, h
        ld (_td_tk_tv+1), a
        ld a, (_td_tk_leg)
        or a, a
        jr z, 165$
        cp a, #2
        jp nz, 70$
    165$:
        ld a, (_td_tk_phase)
        cp a, #7
        jp c, 70$
        ld hl, #_td_stop_cols
        ld a, (_td_tk_leg)
        or a, a
        jr z, 166$
        ld de, #18
        add hl, de
    166$:
        ld b, #9
        ld a, (_td_tk_u)
        ld e, a
        ld a, (_td_tk_u+1)
        ld d, a
        call 90$
        jp nz, 80$
        jp 70$
    66$:
        jr nz, 68$
        ld de, #0x3180
        ld a, (_td_tk_leg)
        or a, a
        jr z, 67$
        cp a, #3
        jr z, 67$
        ld de, #0x3080
    67$:
        ld a, e
        ld (_td_tk_tu), a
        ld a, d
        ld (_td_tk_tu+1), a
        ld de, #0x3180
        ld a, (_td_tk_leg)
        cp a, #2
        jr c, 167$
        ld de, #0x0300
    167$:
        ld a, e
        ld (_td_tk_tv), a
        ld a, d
        ld (_td_tk_tv+1), a
        ld a, (_td_tk_leg)
        or a, a
        jr z, 168$
        cp a, #2
        jr nz, 70$
    168$:
        ld a, (_td_tk_phase)
        cp a, #7
        jr nc, 70$
        ld hl, #_td_stop_rows
        ld a, (_td_tk_leg)
        or a, a
        jr z, 169$
        ld de, #16
        add hl, de
    169$:
        ld b, #8
        ld a, (_td_tk_v)
        ld e, a
        ld a, (_td_tk_v+1)
        ld d, a
        call 90$
        jp nz, 80$
        jr 70$
    68$:
        ld a, (_td_tk_leg)
        ld c, a
        ld b, #0
        ld hl, #_td_bus_u
        add hl, bc
        add hl, bc
        ld a, (hl+)
        ld (_td_tk_tu), a
        ld a, (hl)
        ld (_td_tk_tu+1), a
        ld hl, #_td_bus_v
        add hl, bc
        add hl, bc
        ld a, (hl+)
        ld (_td_tk_tv), a
        ld a, (hl)
        ld (_td_tk_tv+1), a
    70$:
        ld hl, #_td_tk_u
        ld de, #_td_tk_tu
        call 95$
        jr z, 72$
        call 97$
        ld hl, #_td_tk_u
        jr c, 71$
        call 98$
        jr 74$
    71$:
        call 99$
        jr 74$
    72$:
        ld hl, #_td_tk_v
        ld de, #_td_tk_tv
        call 95$
        jr z, 74$
        call 97$
        ld hl, #_td_tk_v
        jr c, 73$
        call 98$
        jr 74$
    73$:
        call 99$
    74$:
        ld a, (_td_tk_yield)
        or a, a
        jr z, 75$
        ld hl, #_td_tk_pu
        ld de, #_td_tk_u
        call 92$
        ld a, d
        or a, a
        jr nz, 75$
        ld a, e
        cp a, #208
        jr nc, 75$
        ld hl, #_td_tk_pv
        ld de, #_td_tk_v
        call 92$
        ld a, d
        or a, a
        jr nz, 75$
        ld a, e
        cp a, #208
        jp c, 85$
    75$:
        ld a, (_td_tk_i)
        ld c, a
        ld b, #0
        ld hl, #_td_traffic_u
        add hl, bc
        add hl, bc
        ld a, (_td_tk_u)
        ld (hl+), a
        ld a, (_td_tk_u+1)
        ld (hl), a
        ld hl, #_td_traffic_v
        add hl, bc
        add hl, bc
        ld a, (_td_tk_v)
        ld (hl+), a
        ld a, (_td_tk_v+1)
        ld (hl), a
        ld hl, #_td_tk_u
        ld de, #_td_tk_tu
        call 95$
        jr nz, 80$
        ld hl, #_td_tk_v
        ld de, #_td_tk_tv
        call 95$
        jr nz, 80$
        ld a, (_td_tk_i)
        ld c, a
        ld b, #0
        ld a, (_td_tk_district)
        or a, a
        jr z, 76$
        ld hl, #(_td_traffic_samples + 4)
        add hl, bc
        add hl, bc
        add hl, bc
        add hl, bc
        add hl, bc
        add hl, bc
        ld e, (hl)
        ld a, #1
        ld (_td_tk_dirty), a
        jr 77$
    76$:
        ld e, #4
        ld a, c
        cp a, #5
        jr nz, 77$
        ld e, #6
    77$:
        ld a, e
        or a, a
        jr z, 80$
        ld a, (_td_tk_leg)
        inc a
        jr nz, 78$
        ld a, #255
        sub a, e
        inc a
    78$:
        cp a, e
        jr c, 79$
        sub a, e
        jr 78$
    79$:
        ld hl, #_td_traffic_leg
        add hl, bc
        ld (hl), a
    80$:
        ld a, (_td_tk_hit)
        or a, a
        jr z, 85$
        ld hl, #_td_tk_pu
        ld de, #_td_tk_u
        call 92$
        ld a, d
        or a, a
        jr nz, 85$
        ld a, e
        cp a, #180
        jr nc, 85$
        ld hl, #_td_tk_pv
        ld de, #_td_tk_v
        call 92$
        ld a, d
        or a, a
        jr nz, 85$
        ld a, e
        cp a, #180
        jr nc, 85$
        ld a, (_td_tk_i)
        ret
    85$:
        ld hl, #_td_tk_i
        inc (hl)
        jp 60$
    ; A=1/NZ if DE (pos) lies in [line, line+8) for one of B UWORD lines at HL.
    90$:
        ld a, (hl+)
        ld c, a
        ld a, e
        sub a, c
        ld c, a
        ld a, (hl+)
        push hl
        ld h, a
        ld a, d
        sbc a, h
        pop hl
        jr nz, 91$
        ld a, c
        cp a, #8
        jr c, 191$
    91$:
        dec b
        jr nz, 90$
        xor a, a
        ret
    191$:
        ld a, #1
        or a, a
        ret
    ; DE = |(HL) - (DE)| for UWORD variables.
    92$:
        ld a, (de)
        ld c, a
        inc de
        ld a, (de)
        ld b, a
        ld a, (hl+)
        sub a, c
        ld e, a
        ld a, (hl)
        sbc a, b
        ld d, a
        ret nc
        xor a, a
        sub a, e
        ld e, a
        ld a, #0
        sbc a, d
        ld d, a
        ret
    ; Z if UWORD (HL) == (DE); preserves HL and DE.
    95$:
        ld a, (de)
        cp a, (hl)
        ret nz
        inc de
        inc hl
        ld a, (de)
        cp a, (hl)
        dec de
        dec hl
        ret
    ; DE = (DE) - (HL) as UWORD; carry set if (DE) < (HL).
    97$:
        ld a, (hl+)
        ld c, a
        ld b, (hl)
        ld a, (de)
        ld l, a
        inc de
        ld a, (de)
        ld h, a
        ld a, l
        sub a, c
        ld e, a
        ld a, h
        sbc a, b
        ld d, a
        ret
    ; (HL) += min(DE, 8)
    98$:
        ld a, d
        or a, a
        jr nz, 198$
        ld a, e
        cp a, #8
        jr c, 199$
    198$:
        ld e, #8
    199$:
        ld a, (hl)
        add a, e
        ld (hl+), a
        ld a, (hl)
        adc a, #0
        ld (hl), a
        ret
    ; (HL) -= min(-DE, 8)
    99$:
        xor a, a
        sub a, e
        ld e, a
        ld a, #0
        sbc a, d
        ld d, a
        jr nz, 196$
        ld a, e
        cp a, #8
        jr c, 197$
    196$:
        ld e, #8
    197$:
        ld a, (hl)
        sub a, e
        ld (hl+), a
        ld a, (hl)
        sbc a, #0
        ld (hl), a
        ret
    __endasm;
}
static void td_traffic_step(void){
    UWORD old_u[6],old_v[6];UBYTE old_leg[6];
    td_tk_phase=td_signal_phase();td_tk_district=td.district;td_tk_dirty=0;
    /* Road users yield to a courier crossing on foot, including between catch-up steps. */
    td_tk_yield=(td.mode==TD_ROAM||td.mode==TD_WAIT)&&td.onfoot;
    td_tk_pu=td.u;td_tk_pv=td.v;td_tk_i=0;td_tk_hit=0;
    if(td.onfoot||td.mode!=TD_ROAM||!td_near_cells()){td_traffic_kernel();}
    else{
        memcpy(old_u,td_traffic_u,sizeof(old_u));memcpy(old_v,td_traffic_v,sizeof(old_v));memcpy(old_leg,td_traffic_leg,sizeof(old_leg));
        td_traffic_kernel();
        td_traffic_block(old_u,old_v,old_leg);
    }
    if(td_tk_dirty)td_world_traffic_samples(td.district,td_traffic_leg,td_traffic_samples);
}
#else
static void td_traffic_step(void){
    UBYTE i,leg,phase=td_signal_phase(),dirty=0,yield,hit,district=td.district;
    UWORD u,v,target_u,target_v,gap,pu=td.u,pv=td.v;
    UWORD *traffic_u=td_traffic_u,*traffic_v=td_traffic_v;UBYTE *legs=td_traffic_leg;
    const td_traffic_sample_t *sample=td_traffic_samples;
    /* Road users yield to a courier crossing on foot, including between catch-up steps. */
    UWORD old_u[6],old_v[6];UBYTE old_leg[6];
    yield=(td.mode==TD_ROAM||td.mode==TD_WAIT)&&td.onfoot;
    hit=0;
    memcpy(old_u,td_traffic_u,sizeof(old_u));memcpy(old_v,td_traffic_v,sizeof(old_v));memcpy(old_leg,td_traffic_leg,sizeof(old_leg));
    for(i=0;i<6;i++,traffic_u++,traffic_v++,legs++,sample++){
        if(td_tr_ctrl&(1<<i))continue;
        u=*traffic_u;v=*traffic_v;leg=*legs;
        if(district){target_u=sample->u;target_v=sample->v;}
        else if(i<4){
            target_u=leg<2?840*16:48*16;target_v=td_core_lane_v[i];
            if(leg==0||leg==3)target_v-=128;else target_v+=128;
            if((leg==0||leg==2)&&phase>=7&&td_signal_stop(u,0,leg==2))goto collide;
        }else if(i==4){
            target_u=leg==0||leg==3?792*16:776*16;target_v=leg<2?792*16:48*16;
            if((leg==0||leg==2)&&phase<7&&td_signal_stop(v,1,leg==2))goto collide;
        }else{target_u=td_bus_u[leg];target_v=td_bus_v[leg];}
        if(u<target_u){gap=target_u-u;u+=gap<8?gap:8;}
        else if(u>target_u){gap=u-target_u;u-=gap<8?gap:8;}
        else if(v<target_v){gap=target_v-v;v+=gap<8?gap:8;}
        else if(v>target_v){gap=v-target_v;v-=gap<8?gap:8;}
        if(yield){
            gap=pu>u?pu-u:u-pu;
            if(gap<208){gap=pv>v?pv-v:v-pv;if(gap<208)continue;}
        }
        *traffic_u=u;*traffic_v=v;
        if(u==target_u&&v==target_v){
            *legs=(leg+1)%(district?sample->count:i==5?6:4);
            /* Banked targets and frames are cached between junctions. */
            if(district)dirty=1;
        }
collide:
        if(hit&&!td.cooldown){
            gap=pu>u?pu-u:u-pu;
            if(gap<180){
                gap=pv>v?pv-v:v-pv;
                if(gap<180){
                    td.speed/=2;td_vx/=2;td_vy/=2;td.cooldown=60;
                    if(td.job!=TD_NONE&&td.stage){td.health=td.health>12?td.health-12:0;if(!td.health){td_finish(FALSE);break;}}td_message(5);
                }
            }
        }
    }
    if(!td.onfoot&&td.mode==TD_ROAM)td_traffic_block(old_u,old_v,old_leg);
    if(dirty)td_world_traffic_samples(td.district,td_traffic_leg,td_traffic_samples);
}
#endif
/* td_frame() without a call: unchanged frames only refresh the paused tick. */
#define TD_FRAME(a,f) do{UBYTE td_f=(f);if((a)->frame_start!=td_f||(a)->frame_end!=td_f+1)actor_set_frames((a),td_f,td_f+1);(a)->anim_tick=255;}while(0)
/* Whole-pixel world position to GBVM Q5: (u>>4)*32 == (u&0xFFF0)<<1. */
#define TD_Q4_TO_ACTOR(q) ((UWORD)(((q)&0xFFF0)<<1))
/* Presentation only: each traffic slot keeps its heading (frame&7) and
 * draws its slot's design and colour; walkers take a design from their
 * route identity and a clothing colour from td_pedestrians. */
/* Slot 4 is an ordinary car; td_life.c turns it into the patrol car out of
 * view when a pursuit starts and draws it while it is owned. */
UBYTE td_traffic_bases[6];
/* Walker design per route identity&7; look 5 (cap, navy) is an officer. */
const UBYTE td_walker_bases[8]={TD_FRAME_PERSON_SHORT,TD_FRAME_PERSON_LONG,TD_FRAME_PERSON_BUN,TD_FRAME_PERSON_PACK,
    TD_FRAME_PERSON_UMBRELLA,TD_FRAME_PERSON_CAP,TD_FRAME_PERSON_CAP,TD_FRAME_PERSON_LONG};
#ifdef __SDCC
#include <stddef.h>
/* The assembly below addresses actor_t fields directly. */
typedef char td_actor_layout_matches_asm[(sizeof(actor_t)==56&&offsetof(actor_t,pos)==1&&
    offsetof(actor_t,frame)==15&&offsetof(actor_t,frame_start)==16&&offsetof(actor_t,frame_end)==17&&
    offsetof(actor_t,anim_tick)==18&&sizeof(td_traffic_sample_t)==6&&offsetof(td_traffic_sample_t,frame)==5&&
    ACTOR_FLAG_HIDDEN==2)?1:-1];
/* Scratch shared with the presentation routines (WRAM, main loop only). */
UBYTE td_pl_base,td_pl_step,td_pl_phase,td_pl_close,td_pl_mask,td_pl_count,td_pl_near,td_pl_district;
UWORD td_pl_pu,td_pl_pv,td_pl_u,td_pl_v;
const UBYTE *td_pl_rp;const UWORD *td_pl_np;actor_t *td_pl_ap;
/* Pedestrian slots 9..16: the C reference below (host builds) defines the
 * exact behaviour; this routine returns its `near` mask in A. The literal
 * slot count and first actor match TD_PEDS and TD_ACTOR_PEDS. */
typedef char td_ped_layout_literals_match[(TD_PEDS==8&&TD_ACTOR_PEDS==9)?1:-1];
UBYTE td_ped_layout(void) NAKED {
    __asm
        xor a, a
        ld (_td_pl_near), a
        inc a
        ld (_td_pl_mask), a
        ld a, #8
        ld (_td_pl_count), a
        ld hl, #(_actors + 9*56)
        ld a, l
        ld (_td_pl_ap), a
        ld a, h
        ld (_td_pl_ap+1), a
        ld hl, #_td_ped_route
        ld a, l
        ld (_td_pl_rp), a
        ld a, h
        ld (_td_pl_rp+1), a
        ld hl, #_td_nearby_routes
        ld a, l
        ld (_td_pl_np), a
        ld a, h
        ld (_td_pl_np+1), a
    10$:
        ld a, (_td_pl_mask)
        ld hl, #_td_ped_ovr
        and a, (hl)
        jp nz, 30$
        ld hl, #_td_pl_rp
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        ld b, (hl)
        ld hl, #_td_pl_ap
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        ld a, b
        inc a
        jr nz, 11$
        set 1, (hl)
        jp 30$
    11$:
        ld a, b
        add a, a
        add a, a
        ld c, a
        add a, a
        add a, a
        add a, a
        add a, c
        add a, b
        ld c, a
        ld a, (_td_pl_base)
        add a, c
        and a, #0x7f
        ld (_td_pl_phase), a
        cp a, #64
        jr c, 12$
        cpl
        sub a, #128
    12$:
        ld c, a
        push hl
        ld hl, #_td_pl_np
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        ld a, (hl+)
        add a, c
        ld (_td_pl_u), a
        ld e, a
        ld a, (hl+)
        adc a, #0
        ld (_td_pl_u+1), a
        ld d, a
        ld a, (hl+)
        ld (_td_pl_v), a
        ld c, a
        ld a, (hl+)
        ld (_td_pl_v+1), a
        ld b, a
        pop hl
        inc hl
        sla e
        rl d
        sla e
        rl d
        sla e
        rl d
        sla e
        rl d
        sla e
        rl d
        ld a, e
        ld (hl+), a
        ld a, d
        ld (hl+), a
        sla c
        rl b
        sla c
        rl b
        sla c
        rl b
        sla c
        rl b
        sla c
        rl b
        ld a, c
        ld (hl+), a
        ld a, b
        ld (hl), a
        ld hl, #_td_pl_rp
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        ld a, (hl)
        and a, #7
        add a, #<(_td_walker_bases)
        ld l, a
        ld a, #0
        adc a, #>(_td_walker_bases)
        ld h, a
        ld b, (hl)
        ld a, (_td_pl_phase)
        cp a, #64
        jr c, 13$
        inc b
        inc b
    13$:
        ld a, (_td_pl_step)
        add a, b
        ld b, a
        ld hl, #_td_pl_ap
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        ld de, #16
        add hl, de
        ld a, (hl+)
        cp a, b
        jr nz, 14$
        ld a, b
        inc a
        cp a, (hl)
        jr z, 15$
    14$:
        ld a, b
        inc a
        ld (hl-), a
        ld a, b
        ld (hl-), a
        ld (hl+), a
        inc hl
    15$:
        inc hl
        ld (hl), #0xff
        ld hl, #_td_pl_u
        ld a, (_td_pl_pu)
        sub a, (hl)
        ld e, a
        inc hl
        ld a, (_td_pl_pu+1)
        sbc a, (hl)
        ld d, a
        jr nc, 16$
        xor a, a
        sub a, e
        ld e, a
        ld a, #0
        sbc a, d
        ld d, a
    16$:
        ld a, d
        or a, a
        jr nz, 20$
        ld a, e
        cp a, #112
        jr nc, 20$
        cp a, #10
        ld a, #0
        rla
        ld (_td_pl_close), a
        ld hl, #_td_pl_v
        ld a, (_td_pl_pv)
        sub a, (hl)
        ld e, a
        inc hl
        ld a, (_td_pl_pv+1)
        sbc a, (hl)
        ld d, a
        jr nc, 17$
        xor a, a
        sub a, e
        ld e, a
        ld a, #0
        sbc a, d
        ld d, a
    17$:
        ld a, d
        or a, a
        jr nz, 20$
        ld a, e
        cp a, #96
        jr nc, 20$
        ld hl, #_td_pl_ap
        ld c, a
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        res 1, (hl)
        ld a, c
        cp a, #10
        jr nc, 30$
        ld a, (_td_pl_close)
        or a, a
        jr z, 30$
        ld a, (_td_pl_mask)
        ld hl, #_td_pl_near
        or a, (hl)
        ld (hl), a
        jr 30$
    20$:
        ld hl, #_td_pl_ap
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        set 1, (hl)
    30$:
        ld a, (_td_pl_mask)
        ld hl, #_pk_fresh
        and a, (hl)
        jr z, 32$
        ld hl, #_td_pl_ap
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        set 1, (hl)
    32$:
        ld hl, #_td_pl_ap
        ld a, (hl)
        add a, #56
        ld (hl+), a
        ld a, (hl)
        adc a, #0
        ld (hl), a
        ld hl, #_td_pl_rp
        inc (hl)
        jr nz, 31$
        inc hl
        inc (hl)
    31$:
        ld hl, #_td_pl_np
        ld a, (hl)
        add a, #4
        ld (hl+), a
        ld a, (hl)
        adc a, #0
        ld (hl), a
        ld hl, #_td_pl_mask
        sla (hl)
        ld hl, #_td_pl_count
        dec (hl)
        jp nz, 10$
        ld a, (_td_pl_near)
        ret
    __endasm;
}
#else
/* Reference semantics: position, frame and visibility for slots 9..16.
 * Returns a mask of visible walkers within ten pixels of the player. */
static UBYTE td_ped_layout_c(UBYTE base,UBYTE step,UWORD player_u,UWORD player_v){
    UBYTE i,route,phase,near=0;UWORD u,v,gap;actor_t *a=&actors[TD_ACTOR_PEDS];
    const UBYTE *routes=td_ped_route;const UWORD (*nearby)[2]=td_nearby_routes;
    for(i=0;i<TD_PEDS;i++,a++,routes++,nearby++){
        if(td_ped_ovr&(1<<i))continue;
        route=*routes;if(route==TD_NONE){a->flags|=ACTOR_FLAG_HIDDEN;continue;}
        phase=td_ped_phase(base,route);
        u=(*nearby)[0];v=(*nearby)[1];
        u+=phase<64?phase:127-phase;
        a->pos.x=u<<5;a->pos.y=v<<5;TD_FRAME(a,td_walker_bases[route&7]+(phase<64?0:2)+step);
        gap=player_u>u?player_u-u:u-player_u;
        if(gap<112&&(player_v>v?player_v-v:v-player_v)<96){
            a->flags&=~ACTOR_FLAG_HIDDEN;
            if(gap<10&&(player_v>v?player_v-v:v-player_v)<10)near|=1<<i;
        }else a->flags|=ACTOR_FLAG_HIDDEN;
        if(pk_fresh&(1<<i))a->flags|=ACTOR_FLAG_HIDDEN;
    }
    return near;
}
#endif
#ifdef __SDCC
/* Traffic slots 2..7: same positions/frames as the C reference below. */
void td_traffic_layout(void) NAKED {
    __asm
        ld hl, #(_actors + 2*56)
        ld a, l
        ld (_td_pl_ap), a
        ld a, h
        ld (_td_pl_ap+1), a
        xor a, a
        ld (_td_pl_count), a
    40$:
        ld a, (_td_pl_count)
        ld c, a
        ld b, #0
        ld hl, #_td_slot_bits
        add hl, bc
        ld a, (_td_tr_ctrl)
        and a, (hl)
        jp nz, 52$
        ld a, (_td_pl_district)
        or a, a
        jr z, 41$
        ld hl, #(_td_traffic_samples + 5)
        add hl, bc
        add hl, bc
        add hl, bc
        add hl, bc
        add hl, bc
        add hl, bc
        ld e, (hl)
        jr 49$
    41$:
        ld hl, #_td_traffic_leg
        add hl, bc
        ld b, (hl)
        ld a, c
        cp a, #4
        jr nc, 42$
        ld a, b
        add a, a
        ld e, a
        jr 49$
    42$:
        jr nz, 45$
        ld e, #2
        ld a, b
        or a, a
        jr z, 49$
        ld e, #4
        dec a
        jr z, 49$
        ld e, #6
        dec a
        jr z, 49$
        ld e, #0
        jr 49$
    45$:
        ld e, #10
        ld a, b
        cp a, #2
        jr z, 49$
        ld e, #12
        or a, a
        jr z, 49$
        ld e, #14
        cp a, #5
        jr z, 49$
        ld e, #8
    49$:
        ld a, (_td_pl_count)
        add a, #<(_td_traffic_bases)
        ld l, a
        ld a, #0
        adc a, #>(_td_traffic_bases)
        ld h, a
        ld a, e
        and a, #7
        add a, (hl)
        ld (_td_pl_phase), a
        ld a, (_td_pl_count)
        add a, a
        ld c, a
        ld b, #0
        ld hl, #_td_traffic_u
        add hl, bc
        ld a, (hl+)
        and a, #0xf0
        ld e, a
        ld d, (hl)
        sla e
        rl d
        ld hl, #_td_traffic_v
        add hl, bc
        ld a, (hl+)
        and a, #0xf0
        ld c, a
        ld b, (hl)
        sla c
        rl b
        ld hl, #_td_pl_ap
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        inc hl
        ld a, e
        ld (hl+), a
        ld a, d
        ld (hl+), a
        ld a, c
        ld (hl+), a
        ld a, b
        ld (hl), a
        ld de, #12
        add hl, de
        ld a, (_td_pl_phase)
        ld b, a
        ld a, (hl+)
        cp a, b
        jr nz, 50$
        ld a, b
        inc a
        cp a, (hl)
        jr z, 51$
    50$:
        ld a, b
        inc a
        ld (hl-), a
        ld a, b
        ld (hl-), a
        ld (hl+), a
        inc hl
    51$:
        inc hl
        ld (hl), #0xff
    52$:
        ld hl, #_td_pl_ap
        ld a, (hl)
        add a, #56
        ld (hl+), a
        ld a, (hl)
        adc a, #0
        ld (hl), a
        ld hl, #_td_pl_count
        inc (hl)
        ld a, (hl)
        cp a, #6
        jp nz, 40$
        ret
    __endasm;
}
#endif
static void td_traffic_present(void){
#ifdef __SDCC
    td_pl_district=td.district;td_traffic_layout();
#else
    UBYTE i,leg,frame,district=td.district;actor_t *a=&actors[2];
    const UWORD *traffic_u=td_traffic_u,*traffic_v=td_traffic_v;const UBYTE *legs=td_traffic_leg;
    const td_traffic_sample_t *sample=td_traffic_samples;
    for(i=0;i<6;i++,a++,traffic_u++,traffic_v++,legs++,sample++){
        if(td_tr_ctrl&(1<<i))continue;
        if(district)frame=sample->frame;
        else{
            leg=*legs;
            frame=i<4?leg<<1:i==4?(leg==0?2:leg==1?4:leg==2?6:0):8+(leg==1?2:leg==0||leg==5?4:leg==3?6:0);
        }
        a->pos.x=TD_Q4_TO_ACTOR(*traffic_u);a->pos.y=TD_Q4_TO_ACTOR(*traffic_v);TD_FRAME(a,td_traffic_bases[i]+(frame&7));
    }
#endif
    TD_PALETTE(&actors[8])=td_car_colour;
    td_position(&actors[8],td.park_u>>4,td.park_v>>4);td_frame(&actors[8],td_entry_timer?TD_FRAME_CAR_DOOR_OPEN:(td.vehicle<<3)+(((td.heading+1)&15)>>1));
    if(td.onfoot&&td.park_district==td.district)actors[8].flags&=~ACTOR_FLAG_HIDDEN;else actors[8].flags|=ACTOR_FLAG_HIDDEN;
}
/* Clothing colours: civilians by route identity, navy for officers. */
static const UBYTE td_civilian_pal[4]={TD_PEOPLE_PAL(TD_PAL_RED),TD_PEOPLE_PAL(TD_PAL_YELLOW),TD_PEOPLE_PAL(TD_PAL_TEAL),TD_PEOPLE_PAL(TD_PAL_VIOLET)};
static void td_walker_colours(void){
    UBYTE i,bit,route;actor_t *a=&actors[TD_ACTOR_PEDS];
    for(i=0,bit=1;i<TD_PEDS;i++,bit<<=1,a++){
        route=td_ped_route[i];
        if((td_ped_ovr&bit)||route==TD_NONE)continue;
        TD_PALETTE(a)=(route&7)==5?TD_PEOPLE_PAL(TD_PAL_NAVY):td_civilian_pal[(route>>3)&3];
    }
}
static void td_pedestrians(void){
    UBYTE refresh,near,moved;UWORD player_u=td.u>>4,player_v=td.v>>4;
    /* Empty slots look for a route again once the courier has moved. */
    moved=td_distance(player_u,td_ped_anchor_u)>24||td_distance(player_v,td_ped_anchor_v)>24;
    refresh=!--td_ped_refresh||td_distance(player_u,td_ped_anchor_u)>64||td_distance(player_v,td_ped_anchor_v)>64;
    if(refresh){
        td_ped_refresh=16;td_ped_anchor_u=player_u;td_ped_anchor_v=player_v;
        /* Slots still waiting for a route are picked over the next frames. */
        if(td_life_routes(moved))td_ped_refresh=2;
        td_walker_colours();
    }
#ifdef __SDCC
    td_pl_base=td_ped_base();td_pl_step=(td_tick>>3)&1;td_pl_pu=player_u;td_pl_pv=player_v;
    near=td_ped_layout();
#else
    near=td_ped_layout_c(td_ped_base(),(td_tick>>3)&1,player_u,player_v);
#endif
    /* Struck walkers are thrown and fall; owned slots are drawn by td_life. */
    td_life_peds(near);
}
/* D-pad (x,y) to aim heading: index (y+1)*3+(x+1), E=0 clockwise. */
static const UBYTE td_aim_of[9]={5,6,7,4,0,0,3,2,1};
static UBYTE td_fire_hold;
static void td_drive(void){
    WORD nu,nv;BYTE walk_x,walk_y;UBYTE moving=0;
    if(td_entry_timer){
        if(!td_entry_target){td.u=(td.u*3+td.park_u)/4;td.v=(td.v*3+td.park_v)/4;}
        if(!--td_entry_timer){if(!td_entry_target){td.u=td.park_u;td.v=td.park_v;td.onfoot=0;}td_set_target();td_save();}
        td_frame(&PLAYER,td.onfoot?TD_FRAME_COURIER_WALK:td_vehicle_frame());return;
    }
    if(td.onfoot){
        if(td_life_locked()){td.speed=0;return;}
        nu=td.u;nv=td.v;
        walk_x=!!INPUT_RIGHT-!!INPUT_LEFT;walk_y=!!INPUT_DOWN-!!INPUT_UP;
        /* Alternate5/6 per axis: diagonal pace stays below cardinal8 Q4. */
        if(walk_x){nu+=walk_x*(walk_y?5+(td_tick&1):8);td_walk_dir=walk_x>0?0:1;moving=1;}
        if(walk_y){nv+=walk_y*(walk_x?6-(td_tick&1):8);td_walk_dir=walk_y>0?2:3;moving=1;}
        /* Aim follows the D-pad in eight directions; the lock-on refreshes
         * every eighth tick. */
        if(moving)td_aim_dir=td_aim_of[(UBYTE)((walk_y+1)*3+walk_x+1)];
        if(!(td_tick&7))td_life_aim();
        if(td_foot_free(nu,nv)){td.u=nu;td.v=nv;}
        else{if(nu!=(WORD)td.u&&td_foot_free(nu,td.v))td.u=nu;if(nv!=(WORD)td.v&&td_foot_free(td.u,nv))td.v=nv;}
        td.speed=0;
        if(td_anim_pose_time){td_anim_pose_time--;td_frame(&PLAYER,td_anim_pose_base+td_walk_dir);}
        else td_frame(&PLAYER,TD_FRAME_COURIER_WALK+td_walk_dir*2+(moving?((td_tick>>3)&1):0));
        /* A: own car, then a nearby road vehicle, else a punch.
         * B: TTC at a station, otherwise the pistol. */
        if(td_input_edge&&INPUT_A_PRESSED){if(td_near_car())td_enter_exit();else td_life_foot_a();}
        /* B at a station opens the TTC; elsewhere it fires, and holding it
         * keeps firing at the pistol's rate. */
        if(td_input_edge&&INPUT_B_PRESSED&&!td_entry_timer){if(td_origin()!=TD_NONE)td_transit_open();else{td_fire_hold=1;td_life_foot_b();}}
        else if(td_fire_hold&&td_input_edge&&INPUT_B)td_life_foot_b();
        if(!INPUT_B)td_fire_hold=0;
        td_aim_hold=td_fire_hold;
        return;
    }
    if(td_life_drive()&TD_DRIVE_EXIT){td_enter_exit();return;}
    td.safe_u=td.u;td.safe_v=td.v;
    td_frame(&PLAYER,td_vehicle_frame());
    if(td.job!=TD_NONE&&!td.health)td_finish(FALSE);
}
/* Arrest and knock-out raised by td_life_tick. */
static void td_life_handle(void){
    UBYTE event=td_life_event;UWORD u,v;
    td_life_event=TD_EVENT_NONE;
    td.speed=0;td_vx=td_vy=0;
    if(event==TD_EVENT_BUSTED){
        td_life_busted();
        /* Officers take the courier out of the vehicle; it stays parked. */
        if(!td.onfoot)td_enter_exit();
        td.mode=TD_BUSTED;td_radio_say(TD_RADIO_BUSTED);
    }else if(event==TD_EVENT_WASTED){
        if(!td.onfoot){td.park_u=td.u;td.park_v=td.v;td.park_district=td.district;td.onfoot=1;}
        td_entry_timer=0;
        td_life_hospital(&u,&v);
        td.mode=TD_WASTED;td_resume_mode=TD_ROAM;td_radio_say(TD_RADIO_WASTED);
        if(td.district!=TD_HOSPITAL_DISTRICT){
            if(td_change_district(TD_HOSPITAL_DISTRICT,u,v)){td_audio_play(TD_AUDIO_FAIL);return;}
        }else{td.u=u;td.v=v;td.safe_u=u;td.safe_v=v;}
        td_position(&PLAYER,td.u>>4,td.v>>4);
    }else return;
    td_resume_mode=TD_ROAM;td_audio_play(TD_AUDIO_FAIL);td_set_target();td_save();td_ui_draw();
}
void toronto_init(void) BANKED {
    UBYTE i,cold=!td_session_live,current=td_district_current();
    if(cold){
        td_district_reset();td_transition_pending=0;
        td_tick=td_notice_timer=td_red_cooldown=td_entry_timer=td_turn_tick=0;td_vx=td_vy=0;td_last_frame=sys_time;td_corner_used=0;
        if(!td_restore()){
            memset(&td,0,sizeof(td));td.u=576*16;td.v=740*16;td.park_u=td.u;td.park_v=td.v;td.cash=30;td.job=TD_NONE;td.heading=0;td.health=100;
            td.vitality=100;td.ammo=TD_AMMO_START;
        }
        if(td.job!=TD_NONE&&!td.stage)td.health=100;
        td.speed=0;td_resume_mode=td.mode==TD_WAIT||td.mode==TD_RIDE?td.mode:TD_ROAM;td.mode=TD_HELP;td.msg=0;td.menu=0;
        td_session_live=1;td_tv_phase=0;td_ride_hidden=0;
    }
    /* Restoring another district redirects through the same genuine VM path.
       Regular crossings keep velocity, mission, parked car, clock and audio. */
    if(current!=td.district){
        td_transition_pending=td_district_queue(td.district)?1:2;
        /* Fullscreen help hides the boot scene; never present remote actors on it. */
        PLAYER.flags|=ACTOR_FLAG_HIDDEN;if(cold)td_audio_init();td_ui_init();return;
    }
    td_transition_pending=0;PLAYER.flags&=~ACTOR_FLAG_HIDDEN;
    /* The scene fade-in applies the time of day's palettes. */
    td_daynight_apply(TD_DN_FORCE);td_dn_pending=0;
    td.safe_u=td.u;td.safe_v=td.v;
    if(td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)td_get_stop(td.transit_target,&td_cursor);
    if(td.job!=TD_NONE){td_get_job(td.job,&td_job);if(td.stage>=td_job.count)td.job=TD_NONE;}
    actors_len=TD_ACTORS;
    /* No palette offsets yet (GBVM set the player's move speed). */
    TD_PALETTE(&PLAYER)=TD_PAL_COURIER;
    for(i=1;i<TD_ACTORS;i++){
        actors[i]=PLAYER;actors[i].prev=actors[i].next=NULL;actors[i].flags=ACTOR_FLAG_PERSISTENT;actors[i].collision_group=0;actors[i].script.bank=actors[i].script_update.bank=0;
        // Place copied actors on the inactive list before activating them.
        actors[i].next=actors_inactive_head; if(actors_inactive_head)actors_inactive_head->prev=&actors[i];actors_inactive_head=&actors[i];
        activate_actor(&actors[i]);
    }
    if(td.district)td_world_traffic_init(td.district,td_traffic_u,td_traffic_v,td_traffic_leg,td_traffic_samples);
    for(i=0;i<6;i++)
        if(!td.district){td_traffic_u[i]=(i<4?80+i*120:i==4?792:160)*16;td_traffic_v[i]=(i<4?td_rows[2+i]-8:i==4?240:64)*16;td_traffic_leg[i]=0;}
    for(i=0;i<TD_PEDS;i++)td_ped_route[i]=TD_NONE;
    td_ped_refresh=1;td_ped_anchor_u=td.u>>4;td_ped_anchor_v=td.v>>4;
    td_life_reset(cold);td_anim_reset();
    for(i=0;i<6;i++)td_lf_new_look(i,(UBYTE)(i*37+td.seconds+(td.district<<3)));
    td_frame(&actors[1],TD_FRAME_BEACON);td_set_target();td_position(&PLAYER,td.u>>4,td.v>>4);
    td_frame(&PLAYER,td.onfoot?TD_FRAME_COURIER_WALK:td_vehicle_frame());td_traffic_present();td_pedestrians();
    td_street_reset(cold);td_transit_present(0);td_pickups_present();
    camera_settings=CAMERA_LOCK_FLAG;camera_offset_x=0;camera_offset_y=0;camera_deadzone_x=8;camera_deadzone_y=8;
    if(cold)td_audio_init();td_ui_init();
}
void toronto_update(void) BANKED {
    UWORD now,elapsed,seconds,old_u,old_v;UBYTE motion,step,was_entering,consumed=0;
    if(td_transition_pending){if(td_transition_pending==2&&td_district_queue(td.district))td_transition_pending=1;return;}
    if(td_dn_pending){td_dn_pending=0;td_daynight_apply(TD_DN_HW);}
    /* A station within reach replaces the street name with its B prompt. */
    if(++td_station_tick>=12){UBYTE near;td_station_tick=0;near=td.onfoot&&td.mode==TD_ROAM&&!td.wanted?td_origin():TD_NONE;if(near!=td_station_near){td_station_near=near;td_ui_pending=1;}}
    if(td_ui_pending){td_ui_pending=0;td_ui_draw();}
    now=sys_time;elapsed=now-td_last_frame;td_last_frame=now;
    td_corner_used=0;
    motion=elapsed>4?4:elapsed;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE){td_menu_update();td_sound_update();return;}
    if(INPUT_START_PRESSED){td_resume_mode=td.mode;td.mode=TD_PAUSE;td.menu=0;td_audio_play(TD_AUDIO_MENU);td_ui_draw();td_sound_update();return;}
    /* A deliberate cancel wins over departure on the same input frame. */
    if(td.mode==TD_WAIT&&INPUT_B_PRESSED){td.mode=TD_ROAM;consumed=1;td_save();td_ui_draw();}
    if(td_notice_timer){if(!--td_notice_timer){td.msg=0;td_ui_draw();}}
    /* Keep deadlines and transit tied to every VBlank, even when rendering falls behind. */
    seconds=0;if(elapsed>=60){seconds=elapsed/60;elapsed%=60;}
    elapsed+=td.subsecond;
    if(elapsed>=60){elapsed-=60;seconds++;}td.subsecond=elapsed;
    while(seconds--&&(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE)){
        td_second();if(td_transition_pending)return;
    }
    for(step=0;step<motion&&(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE);step++){
        td_tick++;td_input_edge=step==0&&!consumed;
        if(td.mode==TD_ROAM){
            old_u=td.u;old_v=td.v;was_entering=td_entry_timer;
            td_drive();if(!was_entering&&td_cross_portal(old_u,old_v))return;
        }
        td_traffic_step();
        /* Street life runs once per rendered update, not per catch-up step. */
        if(!step&&(td.mode==TD_ROAM||td.mode==TD_WAIT)){
            td_life_tick();
            if(td_life_event){td_life_handle();td_sound_update();return;}
        }
    }
    if(td.mode==TD_ROAM&&!consumed&&INPUT_SELECT_PRESSED)td_interact();
    if(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE){
        td_traffic_present();td_pedestrians();td_transit_present(motion);td_pickups_present();
        td_life_present();td_anim_update();if(TD_RADIO_DUE())td_radio_tick();
    }
    if(!(td_tick&7))td_ui_hud_tick();
    TD_PALETTE(&PLAYER)=td.onfoot?TD_PAL_COURIER:td_car_colour;
    td_position(&PLAYER,td.u>>4,td.v>>4);
    td_sound_update();
}
