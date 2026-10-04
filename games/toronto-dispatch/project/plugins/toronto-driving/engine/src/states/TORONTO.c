#pragma bank 255
#include <string.h>
#include "states/TORONTO.h"
#include "td_game.h"
#include "td_world_routes.h"
#include "td_audio.h"
#include "td_district.h"
#include "td_world.h"
#include "td_transit.h"
#include "td_streetcar_runtime.h"
#include "td_aircraft.h"
#include "td_aircraft_render.h"
#include "td_people.h"
#include "td_city_sprites.h"
#include "td_traffic.h"
#include "td_boats.h"
#include "td_roads.h"
#include "td_terrain.h"
#include "td_police.h"
#include "td_traffic_lights.h"
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
td_job_t td_job,td_offer;
td_stop_t td_target,td_cursor;
/* Registered engine field: stock bootstrap resets this on cold/soft boot. */
UBYTE td_session_live;
UBYTE td_route_district;
static UBYTE td_transition_pending,td_transition_district;
typedef char td_serialized_state_must_be_58_bytes[(sizeof(td_state_t)==58)?1:-1];
static const BYTE td_dx[]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
static const BYTE td_dy[]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};
static const UWORD td_rows[]={64,176,288,400,528,640,720,784};
static const UWORD td_cols[]={80,208,336,480,560,640,720,816,944};
static UWORD td_traffic_u[6],td_traffic_v[6];
static td_traffic_sample_t td_traffic_samples[6];
static UBYTE td_traffic_leg[6];
static UBYTE td_tick,td_notice_timer,td_red_cooldown,td_turn_tick,td_entry_timer,td_entry_target,td_walk_dir;
UBYTE td_resume_mode;
UBYTE td_board_route;
static WORD td_vx,td_vy;
static UWORD td_last_frame;
static UBYTE td_corner_used;
static UBYTE td_contact_episode;
static UBYTE td_traffic_retreat_mask;
static UBYTE td_vehicle_contact_mask;
static UWORD td_traffic_advance=8;
static UBYTE td_traffic_elapsed,td_police_elapsed;
static UWORD td_police_advance;
static td_police_plan_t td_police_waypoint;
static UWORD td_police_from_u,td_police_from_v;
static UBYTE td_police_stuck;
static UBYTE td_input_edge;
/* Held menu buttons stay consumed until individually released. Reuse the
   original RESULT byte as a bitmask; this remains transient across cold boot. */
static UBYTE td_result_b_release;
static UBYTE td_change_district(UBYTE district,UWORD u,UWORD v);

static UWORD td_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static void td_position(actor_t *a,UWORD u,UWORD v){
    // GBVM actors use five fractional bits; driving state uses four.
    a->pos.x=u*32; a->pos.y=v*32;
}
static void td_frame(actor_t *a,UBYTE f){if(a->frame_start!=f||a->frame_end!=f+1)actor_set_frames(a,f,f+1);a->anim_tick=255;}
static void td_message(UBYTE m){td.msg=m;td_notice_timer=90;if(m==5||m==13||m==19)td_audio_play(TD_AUDIO_IMPACT);td_ui_draw();}
static void td_sound_update(void){td_audio_update(td.speed,td.vehicle,td.onfoot,!!(INPUT_B&&!(td_result_b_release&J_B)),td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE);}
static UBYTE td_near(td_stop_t *s){return s->district==td.district&&td_distance(td.u>>4,s->u)<15&&td_distance(td.v>>4,s->v)<15;}

/* Terrain cache queries live in their own banked unit. */


static UBYTE td_traffic_free(UWORD u,UWORD v){
    UBYTE i;
    if(td_streetcar_view_district==TD_DISTRICT_ISLANDS)return TRUE;
    for(i=0;i<6;i++)if(td_distance(u,td_traffic_u[i])<168&&td_distance(v,td_traffic_v[i])<168)return FALSE;
    return TRUE;
}
static UBYTE td_foot_free(UWORD u,UWORD v){
    return td_road_walkable(u>>4,v>>4)&&!(td.park_district==td.district&&td_distance(u,td.park_u)<168&&td_distance(v,td.park_v)<168)&&td_traffic_free(u,v)&&td_streetcar_runtime_foot_clear(u,v);
}
static UBYTE td_near_car(void){return td.park_district==td.district&&td_distance(td.u,td.park_u)<384&&td_distance(td.v,td.park_v)<384;}
static UBYTE td_door_path(UWORD u,UWORD v,UWORD car_u,UWORD car_v){
    UBYTE i;WORD du=(WORD)car_u-(WORD)u,dv=(WORD)car_v-(WORD)v;
    /* A one-pixel sample catches rails and corners between usable endpoints. */
    for(i=0;i<=24;i++)if(!td_road_walkable((u+du*i/24)>>4,(v+dv*i/24)>>4))return FALSE;
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
        if(!td_streetcar_runtime_parking_allowed(td.district,td.u,td.v)){td_message(17);return;}
        // Choose a visible, walkable door side without spawning inside a building.
        for(i=0;i<4;i++){
            door_u=td.u;door_v=td.v;
            if(i==0)door_v+=288;else if(i==1)door_u+=288;else if(i==2)door_v-=288;else door_u-=288;
            if(td_door_path(td.u,td.v,door_u,door_v)&&td_traffic_free(door_u,door_v)){found=1;break;}
        }
        if(!found){td_message(15);return;}
        td.park_u=td.u;td.park_v=td.v;td.park_district=td.district;td.u=door_u;td.v=door_v;td.onfoot=1;td_entry_target=1;
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
    if((td.district==TD_DISTRICT_ISLANDS)!=(td_target.district==TD_DISTRICT_ISLANDS)){
        td_ferry_beacon();return;
    }
    routed=td_world_route(td.district,td_target.district,td.onfoot,td.u>>4,td.v>>4,td_target.u,td_target.v,&portal);
    td_route_district=routed?portal.to:TD_DISTRICT_NONE;
    td_position(&actors[1],routed?portal.u:td_target.u,(routed?portal.v:td_target.v)-12);
    if(td_target.district!=td.district&&!routed)actors[1].flags|=ACTOR_FLAG_HIDDEN;
    else actors[1].flags&=~ACTOR_FLAG_HIDDEN;
}
static UBYTE td_change_district(UBYTE district,UWORD u,UWORD v){
    if(td.onfoot?!td_district_walkable(district,u>>4,v>>4):!td_district_drivable(district,u>>4,v>>4))return FALSE;
    if(td_district_current()!=district){
        if(!td_district_queue(district))return FALSE;
        td_transition_pending=1;td_transition_district=district;
    }
    td.district=district;td.u=u;td.v=v;td.safe_u=u;td.safe_v=v;
    if(!td.onfoot){td.park_district=district;td.park_u=u;td.park_v=v;}
    td_streetcar_runtime_prepare(0);td_save();return TRUE;
}
static UBYTE td_cross_portal(UWORD old_u,UWORD old_v){
    td_crossing_t crossing;
    if(td_entry_timer||td.mode!=TD_ROAM||(td.u==old_u&&td.v==old_v))return FALSE;
    /* Most motion stays away from seams and needs no ROM bank switch. */
    if(td.u>24*16&&td.u<1000*16&&td.v>24*16&&td.v<952*16)return FALSE;
    if(!td_world_crossing(td.district,td.onfoot,old_u,old_v,td.u,td.v,&crossing))return FALSE;
    return td_change_district(crossing.district,crossing.u,crossing.v);
}
static void td_finish(UBYTE success){
    /* The board-only offer buffer retains the pre-payment balance while the
       result is visible. It is rebuilt before the next offer is accepted. */
    td_offer.reward=td.cash;
    td_audio_play(success?TD_AUDIO_COMPLETE:TD_AUDIO_FAIL);
    if(success){
        if(!(td.complete[td.job>>3]&(1<<(td.job&7)))){td.complete[td.job>>3]|=1<<(td.job&7);td.done++;}
        { UWORD reward=td_job.reward/100*td.health+(td_job.reward%100)*td.health/100+td.left/5;
          td.cash=td.cash>60000-reward?60000:td.cash+reward; }
    }
    td.job=TD_NONE;td.speed=0;td_vx=td_vy=0;td.mode=TD_RESULT;td_set_target();td_save();td_ui_draw();
}
static void td_ready_offer(void){
    UBYTE i;td_board_route=0;
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
    td_audio_play(TD_AUDIO_PICKUP);
    td_set_target();td_save();td_ui_draw();
}
static UBYTE td_origin(void){
    UBYTE i;td_stop_t s;
    for(i=0;i<TD_STOPS;i++){td_get_stop(i,&s);if(s.transit&&td_transit_can_origin(i)&&td_near(&s))return i;}
    return TD_NONE;
}
UBYTE td_service(UBYTE origin) BANKED {
    return td_transit_service(origin);
}
UBYTE td_next_departure(UBYTE origin,UWORD seconds) BANKED {
    return td_transit_departure(origin,origin&63,seconds);
}
static UBYTE td_route_stop(UBYTE origin,UBYTE idx){
    return td_transit_stop(origin,idx);
}
static UBYTE td_wait_at_origin(void){
    td_stop_t origin;
    if(!td.onfoot||(td.transit_origin&63)>=TD_STOPS)return FALSE;
    td_get_stop(td.transit_origin&63,&origin);return td_near(&origin);
}
static UBYTE td_board_current_window(void){
    UBYTE fare;
    if(!td_transit_valid(td.transit_origin,td.transit_target))return FALSE;
    if(!td_wait_at_origin()){td.mode=TD_ROAM;td_save();td_message(6);return TRUE;}
    if(td_transit_departure(td.transit_origin,td.transit_target,td.seconds))return FALSE;
    fare=td_transit_booking_fare(td.transit_origin,td.transit_target,td.job,td.cash,td.district);
    if(td.cash<fare){td.mode=TD_ROAM;td_save();td_message(4);return TRUE;}
    /* A fresh free-roaming trip cannot inherit an old contract failure. */
    if(td.job==TD_NONE)td.health=100;
    td.cash-=fare;td.mode=TD_RIDE;td.reserved=0;td.ride_left=td_transit_duration(td.transit_origin,td.transit_target);
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
    td.transit_target=td_route_stop(origin,td.menu);td_get_stop(td.transit_target,&td_cursor);td.mode=TD_TRANSIT;td_ui_draw();
}
static void td_pause_choose(void){
    if((td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)&&td.menu>1&&td.menu!=8){td_message(2);return;}
    switch(td.menu){
        case 0:td.mode=td_resume_mode;break;
        case 1:td.mode=TD_MAP;td_map_open();break;
        case 2:td.mode=TD_BOARD;td_board_route=0;if(td.job==TD_NONE)td_ready_offer();else{td.menu=td.job;td_get_job(td.menu,&td_offer);}break;
        case 3:td_enter_exit();return;
        case 4:
            if(td.job!=TD_NONE||td.speed>2||td.speed<-2||td.onfoot){td_message(2);return;}
            td.vehicle=(td.vehicle+1)&3;td_save();break;
        case 5:td_transit_open();return;
        case 6:td_save();td.mode=TD_ROAM;td_message(12);break;
        case 7:td.job=TD_NONE;td.speed=0;td.mode=TD_ROAM;td_set_target();td_save();break;
        case 8:td_audio_set_mode((td_audio_get_mode()+1)%TD_AUDIO_MODES);break;
    }
    td_audio_play(TD_AUDIO_MENU);
    td_ui_draw();
}
static void td_menu_update(void){
    if(td.mode==TD_HELP){if(INPUT_A_PRESSED||INPUT_B_PRESSED){td.mode=td_resume_mode;td_ui_draw();}return;}
    if(td.mode==TD_MAP){
        if(INPUT_B_PRESSED||INPUT_START_PRESSED){td_map_close();td.mode=TD_PAUSE;td.menu=1;td_ui_draw();}
        else td_map_update(joy,joy_pressed);
        return;
    }
    if(INPUT_B_PRESSED||INPUT_START_PRESSED){
        td.mode=td.mode==TD_PAUSE?td_resume_mode:TD_ROAM;td_ui_draw();return;
    }
    if(td.mode==TD_PAUSE){
        if(INPUT_DOWN_PRESSED)td.menu=(td.menu+1)%9;
        if(INPUT_UP_PRESSED)td.menu=(td.menu+8)%9;
        if(INPUT_A_PRESSED){td_pause_choose();return;}
    }else if(td.mode==TD_BOARD){
        if(INPUT_SELECT_PRESSED){
            td.menu=(td.menu&0xf8)+8;
            if(td.menu>=TD_QUESTS)td.menu=0;
            td_get_job(td.menu,&td_offer);td_board_route=0;td_ui_draw();return;
        }
        if(INPUT_RIGHT_PRESSED){td.menu=(td.menu+1)%TD_QUESTS;td_get_job(td.menu,&td_offer);td_board_route=0;}
        if(INPUT_LEFT_PRESSED){td.menu=(td.menu+TD_QUESTS-1)%TD_QUESTS;td_get_job(td.menu,&td_offer);td_board_route=0;}
        if(td_offer.count){
            if(td_board_route>=td_offer.count)td_board_route=0;
            if(INPUT_UP_PRESSED)td_board_route=td_board_route?td_board_route-1:td_offer.count-1;
            else if(INPUT_DOWN_PRESSED)td_board_route=td_board_route+1<td_offer.count?td_board_route+1:0;
        }
        if(INPUT_A_PRESSED){
            if(td.job!=TD_NONE){td.mode=TD_ROAM;td_ui_draw();return;}
            if(td.done<td_offer.min_done){td_message(3);return;}
            if(td_offer.vehicle!=TD_NONE&&(td.onfoot||td.vehicle!=td_offer.vehicle)){td_message(2);return;}
            td.job=td.menu;td_job=td_offer;td.stage=0;td.health=100;td.left=td_job.seconds;td.mode=TD_ROAM;td_audio_play(TD_AUDIO_MENU);td_set_target();td_save();td_ui_draw();return;
        }
    }else if(td.mode==TD_TRANSIT){
        if((INPUT_UP_PRESSED||INPUT_DOWN_PRESSED)&&(td.transit_origin&63)==16){
            td.transit_origin^=64;td.menu=0;td.transit_target=td_route_stop(td.transit_origin,0);td_get_stop(td.transit_target,&td_cursor);
        }
        if(INPUT_RIGHT_PRESSED){UBYTE count=td_transit_count(td.transit_origin);if(count){td.menu=(td.menu+1)%count;td.transit_target=td_route_stop(td.transit_origin,td.menu);td_get_stop(td.transit_target,&td_cursor);}}
        if(INPUT_LEFT_PRESSED){UBYTE count=td_transit_count(td.transit_origin);if(count){td.menu=(td.menu+count-1)%count;td.transit_target=td_route_stop(td.transit_origin,td.menu);td_get_stop(td.transit_target,&td_cursor);}}
        if(INPUT_A_PRESSED){
            if(!td_transit_valid(td.transit_origin,td.transit_target))return;
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
    if(!td_streetcar_runtime_landing_clear(td_cursor.district,u,v))return FALSE;
    if(td.park_district==td_cursor.district&&td_distance(u,td.park_u)<168&&td_distance(v,td.park_v)<168)return FALSE;
    /* A remote scene's traffic is not loaded yet. Queen's new platforms are
     * on sidewalks; do not test them against the origin's vehicle cache. */
    return td_cursor.district!=td_streetcar_view_district||td_traffic_free(u,v);
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
static void td_second(void){
    UWORD arrival_u,arrival_v;
    td.seconds++;
    if(td_people_second())td_ui_draw();
    if(td.mode==TD_ROAM&&td.district==TD_DISTRICT_ISLANDS&&td_target.district!=TD_DISTRICT_ISLANDS)
        td_ferry_beacon();
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
                td.ride_left=1;td.reserved=td_service(td.transit_origin)==TD_TRANSIT_STREETCAR?TD_STREETCAR_HOLD:0;td_save();td_ui_draw();return;
            }
            /* Commit an alighted state before queuing another scene. A saved
             * paid ride still belongs to its origin district until arrival. */
            td.mode=td.health?TD_ROAM:TD_RESULT;td.reserved=0;
            if(td_cursor.district!=td.district){
                if(!td_change_district(td_cursor.district,arrival_u,arrival_v)){
                    td.mode=TD_RIDE;td.ride_left=1;td.reserved=td_service(td.transit_origin)==TD_TRANSIT_STREETCAR?TD_STREETCAR_HOLD:0;td_save();td_ui_draw();return;
                }
                td_resume_mode=td.mode;td_set_target();td_audio_play(td.health?TD_AUDIO_TRANSIT:TD_AUDIO_FAIL);return;
            }
            td.u=arrival_u;td.v=arrival_v;td.safe_u=td.u;td.safe_v=td.v;
            td_resume_mode=td.mode;td_set_target();td_audio_play(td.health?TD_AUDIO_TRANSIT:TD_AUDIO_FAIL);
        }
    }
    td_save();td_ui_draw();
}
static UBYTE td_traffic_retreat_clear(UBYTE i,UWORD u,UWORD v){
    UBYTE other,half=i==1||i==4?6:i==3||i==5?7:5;UWORD old_u=td_traffic_u[i],old_v=td_traffic_v[i];
    if(u>old_u?old_u<td.u:u<old_u?old_u>td.u:v>old_v?old_v<td.v:old_v>td.v)return FALSE;
    if(td_distance(td.u,u)+td_distance(td.v,v)<=td_distance(td.u,old_u)+td_distance(td.v,old_v)||
       !td_road_body(old_u>>4,old_v>>4,half)||!td_road_body(u>>4,v>>4,half))return FALSE;
    if(td.park_district==td_streetcar_view_district&&td_distance(u,td.park_u)<180&&
       td_distance(v,td.park_v)<180)return FALSE;
    for(other=0;other<6;other++)if(other!=i&&td_distance(u,td_traffic_u[other])<180&&
        td_distance(v,td_traffic_v[other])<180)return FALSE;
    for(other=9;other<15;other++)if(!(actors[other].flags&ACTOR_FLAG_HIDDEN)&&
        td_distance(u,actors[other].pos.x>>1)<160&&td_distance(v,actors[other].pos.y>>1)<160)return FALSE;
    return td_streetcar_runtime_traffic_retreat_extent(td_streetcar_view_district,old_u,old_v,u,v,half);
}
static UBYTE td_traffic_separate(UBYTE i,UWORD target_u,UWORD target_v,UWORD *u,UWORD *v){
    UWORD from_u,from_v,old_u=td_traffic_u[i],old_v=td_traffic_v[i];
    UBYTE count=td_traffic_samples[i].count;
    if(i==2&&td_police_waypoint.valid){
        from_u=td_police_from_u;from_v=td_police_from_v;
        if(from_u==target_u){
            if(old_u!=from_u||old_v<(from_v<target_v?from_v:target_v)||
                old_v>(from_v>target_v?from_v:target_v))return FALSE;
        }else if(from_v==target_v){
            if(old_v!=from_v||old_u<(from_u<target_u?from_u:target_u)||
                old_u>(from_u>target_u?from_u:target_u))return FALSE;
        }else return FALSE;
    }else if(!td_streetcar_runtime_traffic_segment(td_streetcar_view_district,i,count,td_traffic_leg,
        old_u,old_v,target_u,target_v,&from_u,&from_v))return FALSE;
    if(td_traffic_retreat_clear(i,*u,*v))return TRUE;
    *u=old_u;*v=old_v;
    if(old_u<from_u)*u+=td_distance(old_u,from_u)<8?td_distance(old_u,from_u):8;
    else if(old_u>from_u)*u-=td_distance(old_u,from_u)<8?td_distance(old_u,from_u):8;
    else if(old_v<from_v)*v+=td_distance(old_v,from_v)<8?td_distance(old_v,from_v):8;
    else if(old_v>from_v)*v-=td_distance(old_v,from_v)<8?td_distance(old_v,from_v):8;
    return td_traffic_retreat_clear(i,*u,*v);
}
static void td_traffic_contacts(void);
static void td_traffic_motion_inner(UBYTE mask,td_traffic_epoch_t *epoch){
    /* All enabled districts use the same authored right-hand route cache. */
    UBYTE i,leg,blocked,separating,dirty=0,extents[6]={5,6,5,7,6,7};
    UWORD u,v,target_u,target_v,amount;
    UBYTE pursuit=td.wanted&&td.mode!=TD_RIDE&&!td_streetcar_ride_view&&td.district==td_streetcar_view_district;
    td_traffic_context_t context;
    context.u=td_traffic_u;context.v=td_traffic_v;context.peds=&actors[9];
    context.half_u=context.half_v=extents;
    context.park_u=td.onfoot?td.park_u:td.u;context.park_v=td.onfoot?td.park_v:td.v;
    context.parked_active=td.onfoot?td.park_district==td_streetcar_view_district:
        td.district==td_streetcar_view_district;
    context.priority_mask=(td.wanted?4:0)|(td.seconds%60<30?24:0);
    /* Snapshot only this movement batch. No actor/time/priority changes
       occur until contacts below; sequential accepted endpoints commit. */
    if(!td_traffic_epoch_begin(&context,td_streetcar_view_district,td.seconds,epoch)){td_traffic_contacts();return;}
    for(i=0;i<6;i++){
        if(!(mask&(1<<i)))continue;
        u=td_traffic_u[i];v=td_traffic_v[i];leg=td_traffic_leg[i];blocked=0;
        /* Recovery uses180Q4 against actors rounded down to whole pixels.
           One extra pixel covers that presentation rounding on either side. */
        if(td_distance(td.u,u)>=196||td_distance(td.v,v)>=196)td_traffic_retreat_mask&=~(1<<i);
        if((td.mode==TD_ROAM||td.mode==TD_WAIT)&&td.onfoot&&
           td_distance(td.u,u)<168&&td_distance(td.v,v)<168)td_traffic_retreat_mask|=1<<i;
        target_u=td_traffic_samples[i].u;target_v=td_traffic_samples[i].v;
        if(i==2&&(pursuit||td_police_waypoint.valid)){
            if(!pursuit&&u==target_u&&v==target_v)td_police_waypoint.valid=0;
            else{
                if(!td_police_waypoint.valid||(u==td_police_waypoint.u&&v==td_police_waypoint.v)||td_police_stuck>=64){
                    UBYTE heading=td_police_waypoint.valid?td_police_waypoint.heading:
                        (td_traffic_samples[i].frame&7)/2;
                    if(td_police_plan(td_streetcar_view_district,pursuit?td.wanted:0,u,v,
                        pursuit?td.u:target_u,pursuit?td.v:target_v,heading,&td_police_waypoint)){
                        td_police_from_u=u;td_police_from_v=v;td_police_stuck=0;
                    }else td_police_waypoint.valid=0;
                }
                if(!td_police_waypoint.valid)continue;
                target_u=td_police_waypoint.u;target_v=td_police_waypoint.v;
                if(td_police_stuck<64)td_police_stuck++;
            }
        }
        if(!blocked){
            amount=td_traffic_retreat_mask&(1<<i)?8:td_traffic_advance;
            if(i==2&&pursuit&&!(td_traffic_retreat_mask&(1<<i))){
                if(td_police_advance)amount=td_police_advance;
                amount=amount+amount*td.wanted/2;if(amount>128)amount=128;
            }
            if(u<target_u)u+=td_distance(u,target_u)<amount?td_distance(u,target_u):amount;
            else if(u>target_u)u-=td_distance(u,target_u)<amount?td_distance(u,target_u):amount;
            else if(v<target_v)v+=td_distance(v,target_v)<amount?td_distance(v,target_v):amount;
            else if(v>target_v)v-=td_distance(v,target_v)<amount?td_distance(v,target_v):amount;
            /* The same168-Q4 exclusion used by walking identifies an existing
               obstruction, including a9px offset outside the visual body.
               Continue until recovery's180-Q4 initial path sample plus its
               whole-pixel rounding clears; never end at a cooldown. */
            separating=(td.mode==TD_ROAM||td.mode==TD_WAIT)&&td.onfoot&&(td_traffic_retreat_mask&(1<<i));
            if(separating){
                if(!td_traffic_separate(i,target_u,target_v,&u,&v))continue;
            }else{
                // Incoming road users still yield throughout the wider13px margin.
                if((td.mode==TD_ROAM||td.mode==TD_WAIT)&&td.onfoot&&td_distance(td.u,u)<208&&td_distance(td.v,v)<208)continue;
            }
            /* One banked operation retains admission, whole-body terrain,
               future-tram and pending ownership. Only an accepted commit
               advances the snapshot; publish that same endpoint immediately. */
            if(!td_traffic_epoch_move(epoch,i,u,v,
                !!(td_traffic_retreat_mask&(1<<i)),!!separating))continue;
            td_traffic_u[i]=u;td_traffic_v[i]=v;
            if(i==2&&td_police_waypoint.valid)td_police_stuck=0;
            else if(u==target_u&&v==target_v){
                td_traffic_leg[i]=(leg+1)%td_traffic_samples[i].count;
                /* Banked targets and frames are cached between junctions. */
                dirty=1;
            }
        }
    }
    td_traffic_contacts();
    if(dirty)td_world_traffic_samples(td_streetcar_view_district,td_traffic_leg,td_traffic_samples);
}
/* Keep the large transient snapshot in this tiny caller frame. The hot
   loop's ordinary locals remain within native signed-eight-bit SP reach. */
static void td_traffic_motion(UBYTE mask){
    td_traffic_epoch_t epoch;
    if(td_streetcar_view_district==TD_DISTRICT_ISLANDS)return;
    td_traffic_motion_inner(mask,&epoch);
}
static void td_traffic_step(void){td_traffic_motion(63);}
static void td_traffic_contacts(void){
    static const UBYTE extents[6]={5,6,5,7,6,7};UBYTE i;
    if(td_streetcar_view_district==TD_DISTRICT_ISLANDS)return;
    /* A stationary/queued vehicle is still a solid road user. Contact and
       patrol checks run even when its movement was denied by a red light. */
    if(td_people_police(td_traffic_u[2],td_traffic_v[2])){
        td.speed=0;td_vx=td_vy=0;td.cooldown=120;td_message(20);td_save();
    }
    for(i=0;i<6;i++){
        if(td_distance(td.u,td_traffic_u[i])>=(5+extents[i])*16||
           td_distance(td.v,td_traffic_v[i])>=(5+extents[i])*16){td_vehicle_contact_mask&=~(1<<i);continue;}
        if(td.mode!=TD_ROAM||td.onfoot||td.cooldown||td_contact_episode||
           (td_vehicle_contact_mask&(1<<i)))continue;
        td_vehicle_contact_mask|=1<<i;
        td.speed/=2;td_vx/=2;td_vy/=2;td.cooldown=60;
        if(td.job!=TD_NONE&&td.stage){td.health=td.health>12?td.health-12:0;if(!td.health){td_finish(FALSE);break;}}
        td_message(5);
    }
}
static void td_traffic_present(void){
    UBYTE i,frame,parked_hidden;
    volatile actor_t *parked=&actors[8];
    if(td_streetcar_view_district==TD_DISTRICT_ISLANDS){
        /* The six cached mainland positions do not represent Island actors.
           Hide before civilians inspect road users; keep their caches intact. */
        for(i=2;i<9;i++)actors[i].flags|=ACTOR_FLAG_HIDDEN;
        return;
    }
    for(i=0;i<6;i++){
        frame=td_traffic_samples[i].frame;
        if(i==2&&td_police_waypoint.valid)frame=td_police_waypoint.heading*2;
        actors[i+2].flags&=~ACTOR_FLAG_HIDDEN;
        /* Preserve Q4 collision centres in native Q5 actor coordinates. */
        actors[i+2].pos.x=td_traffic_u[i]*2;actors[i+2].pos.y=td_traffic_v[i]*2;
        td_fleet_present(&actors[i+2],i,(frame&7)/2);
    }
    td_position(&actors[8],td.park_u>>4,td.park_v>>4);td_frame(&actors[8],td_entry_timer?44:td.vehicle*8+((td.heading+1)&15)/2);
    parked_hidden=td.onfoot&&td.park_district==td_streetcar_view_district?0:ACTOR_FLAG_HIDDEN;
    /* Complete the context comparison before this single volatile actor
       write. SDCC reused the comparison's HL for the old branch RMW store,
       overwriting the loaded district with parked-car flags on foot. */
    parked->flags=(parked->flags&~ACTOR_FLAG_HIDDEN)|parked_hidden;
}
static void td_pedestrians(void){
    UBYTE hits=td_people_present(td_tick);UWORD fine;
    if(!hits)return;
    td.speed/=2;td_vx/=2;td_vy/=2;td.cooldown=45;
    td.wanted=td.wanted+hits>3?3:td.wanted+hits;td.wanted_left=30;
    fine=20*hits*td.wanted;td.cash=td.cash>fine?td.cash-fine:0;
    if(td.job!=TD_NONE&&td.stage)td.health=td.health>10*hits?td.health-10*hits:0;
    td_message(19);
    if(td.job!=TD_NONE&&!td.health)td_finish(FALSE);else td_save();
}

static void td_drive(td_terrain_cache_t *cache){
    WORD nu,nv,target_x,target_y;BYTE walk_x,walk_y;UBYTE limit,turn_period,moving=0,slide=0,speed;UWORD u,v;
    if(td.cooldown)td.cooldown--;
    if(td_red_cooldown)td_red_cooldown--;
    if(td_entry_timer){
        if(!td_entry_target){td.u=(td.u*3+td.park_u)/4;td.v=(td.v*3+td.park_v)/4;}
        if(!--td_entry_timer){if(!td_entry_target){td.u=td.park_u;td.v=td.park_v;td.onfoot=0;}td_set_target();td_save();td_ui_draw();}
        td_frame(&PLAYER,td.onfoot?32:td.vehicle*8+((td.heading+1)&15)/2);return;
    }
    if(td.onfoot){
        nu=td.u;nv=td.v;
        walk_x=!!INPUT_RIGHT-!!INPUT_LEFT;walk_y=!!INPUT_DOWN-!!INPUT_UP;
        /* Alternate5/6 per axis: diagonal pace stays below cardinal8 Q4. */
        if(walk_x){nu+=walk_x*(walk_y?5+(td_tick&1):8);td_walk_dir=walk_x>0?0:1;moving=1;}
        if(walk_y){nv+=walk_y*(walk_x?6-(td_tick&1):8);td_walk_dir=walk_y>0?2:3;moving=1;}
        if(td_foot_free(nu,nv)){td.u=nu;td.v=nv;}
        else{if(nu!=(WORD)td.u&&td_foot_free(nu,td.v))td.u=nu;if(nv!=(WORD)td.v&&td_foot_free(td.u,nv))td.v=nv;}
        td.speed=0;td_frame(&PLAYER,32+td_walk_dir*2+(moving?((td_tick>>3)&1):0));
        if(td_input_edge&&INPUT_A_PRESSED&&td_near_car())td_enter_exit();
        if(td_input_edge&&INPUT_B_PRESSED&&!td_entry_timer)td_transit_open();
        return;
    }
    limit=td.vehicle==1?20:td.vehicle==2?28:td.vehicle==3?18:24;
    speed=td.speed<0?-td.speed:td.speed;
    turn_period=speed>18?12:speed>8?10:8;
    if(td.vehicle==1)turn_period+=2;
    // A dedicated yaw counter keeps turns regular as speed changes.
    if(!!INPUT_LEFT!=!!INPUT_RIGHT){if(++td_turn_tick>=turn_period){td_turn_tick=0;if(INPUT_LEFT)td.heading=(td.heading+15)&15;else td.heading=(td.heading+1)&15;
        if(td.job!=TD_NONE&&td.stage&&td_job.kind==5&&speed>18){if(td.health)td.health--;td_message(14);}
    }}
    else td_turn_tick=0;
    if(INPUT_B&&!(td_result_b_release&J_B)){if(td_tick%2==0&&td.speed>-6)td.speed--;}
    else if(INPUT_A&&!(td_result_b_release&J_A)){if(td_tick%4==0&&td.speed<limit)td.speed++;}
    else if(td_tick%8==0){if(td.speed>0)td.speed--;else if(td.speed<0)td.speed++;}
    // Traction eases velocity toward heading instead of instantly rotating momentum.
    target_x=td_dx[td.heading]*td.speed;target_y=td_dy[td.heading]*td.speed;
    td_vx+=(target_x-td_vx)/4;td_vy+=(target_y-td_vy)/4;
    if(!td.speed){td_vx/=2;td_vy/=2;}
    nu=td.u+td_vx/16;nv=td.v+td_vy/16;u=nu>>4;v=nv>>4;
    if(!td_streetcar_runtime_car_clear(td.u,td.v,nu,nv)){
        td.speed=0;td_vx=td_vy=0;
        if(!td.cooldown&&!td_contact_episode){td_contact_episode=1;td.cooldown=60;if(td.job!=TD_NONE&&td.stage){td.health=td.health>12?td.health-12:0;}td_message(5);}
        if(td.job!=TD_NONE&&!td.health)td_finish(FALSE);
        return;
    }
    if(td_terrain_drivable(u,v,cache)){
        if(td.district==0&&!td_red_cooldown&&td.speed>6&&td_distance(u,640)<14&&td_distance(v,528)<14&&
          (td_distance(td.u>>4,640)>=14||td_distance(td.v>>4,528)>=14)){
            UBYTE ax=td_dx[td.heading]<0?-td_dx[td.heading]:td_dx[td.heading];
            UBYTE ay=td_dy[td.heading]<0?-td_dy[td.heading]:td_dy[td.heading];
            if((ax>ay&&td.seconds%12>=7)||(ax<=ay&&td.seconds%12<7)){if(td.cash>=5)td.cash-=5;td_red_cooldown=120;td_message(7);}
        }
        td.u=nu;td.v=nv;
    }else{
        // A glancing curb contact slides along the free axis and preserves forward speed.
        if(nu!=(WORD)td.u&&td_terrain_drivable(nu>>4,td.v>>4,cache)){td.u=nu;td_vy=0;slide=1;}
        if(nv!=(WORD)td.v&&td_terrain_drivable(td.u>>4,nv>>4,cache)){td.v=nv;td_vx=0;slide=1;}
        if(!slide){
            UBYTE held=joy;
            joy&=~(td_result_b_release&(J_A|J_B));
            slide=td_road_corner(&td,&td_vx,&td_vy,&td_corner_used,nu,nv);
            joy=held;
        }
        /* Remove only blocked-axis motion. Repeated curb scrapes must not beat the throttle. */
        if(slide){if(speed>8&&!td.cooldown){td.cooldown=30;if(td.job!=TD_NONE&&td.stage){UBYTE damage=td_job.kind==1?4:1;td.health=td.health>damage?td.health-damage:0;}td_message(5);}}
        else{
            if(td.speed>8&&!td.cooldown){if(td.job!=TD_NONE&&td.stage){UBYTE damage=td_job.kind==1?20:8;td.health=td.health>damage?td.health-damage:0;}td.cooldown=45;td_message(5);}
            td.speed=0;td_vx=td_vy=0;
        }
    }
    td.safe_u=td.u;td.safe_v=td.v;
    td_frame(&PLAYER,td.vehicle*8+((td.heading+1)&15)/2);
    if(td.job!=TD_NONE&&!td.health)td_finish(FALSE);
}
void toronto_init(void) BANKED {
    UBYTE i,cold=!td_session_live,current=td_district_current();
    /* Scene loading/allocation is a frozen transition, never a hidden
       deadline catch-up or an instant completed paid trip on the next frame. */
    td_last_frame=sys_time;
    /* A new scene has replaced VRAM: discard prior roof patches rather than
       restoring old map cells into the newly loaded district. */
    td_aircraft_render_reset();
    td_traffic_lights_reset();
    td_contact_episode=td_traffic_retreat_mask=td_vehicle_contact_mask=0;td_traffic_advance=8;
    td_police_waypoint.valid=td_police_stuck=td_traffic_elapsed=td_police_elapsed=0;td_police_advance=0;
    if(cold){
        td_district_reset();td_transition_pending=0;
        td_tick=td_notice_timer=td_red_cooldown=td_entry_timer=td_turn_tick=td_result_b_release=0;td_vx=td_vy=0;td_last_frame=sys_time;td_corner_used=0;
        if(!td_restore()){
            memset(&td,0,sizeof(td));td.u=560*16;td.v=720*16;td.park_u=td.u;td.park_v=td.v;td.cash=30;td.job=TD_NONE;td.heading=0;td.health=100;
        }
        if(td.job!=TD_NONE&&!td.stage)td.health=100;
        td.speed=0;td_resume_mode=td.mode==TD_WAIT||td.mode==TD_RIDE?td.mode:TD_ROAM;td.mode=TD_HELP;td.msg=0;td.menu=0;
        td_streetcar_runtime_reset();
        if(td_streetcar_runtime_recover_park()==TD_STREETCAR_PARK_MOVED)td_save();
        td_session_live=1;
    }
    td_streetcar_runtime_prepare(0);
    /* Restoring another district redirects through the same genuine VM path.
       Regular crossings keep velocity, mission, parked car, clock and audio. */
    if(current!=td_streetcar_view_district){
        td_transition_district=td_streetcar_view_district;
        td_transition_pending=td_district_queue(td_transition_district)?1:2;
        /* Fullscreen help hides the boot scene; never present remote actors on it. */
        PLAYER.flags|=ACTOR_FLAG_HIDDEN;if(cold)td_audio_init();td_ui_init();return;
    }
    td_transition_pending=0;PLAYER.flags&=~ACTOR_FLAG_HIDDEN;
    td.safe_u=td.u;td.safe_v=td.v;
    if(td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)td_get_stop(td.transit_target,&td_cursor);
    if(td.job!=TD_NONE){td_get_job(td.job,&td_job);if(td.stage>=td_job.count)td.job=TD_NONE;}
    actors_len=TD_ACTORS;
    td_aircraft_render_bind();
    td_aircraft_reset((cold?0x9D27:td_aircraft.seed)^sys_time^td.u^td.v^current);
    td_streetcar_runtime_bind();
    td_city_sprites_bind();
    td_boats_bind();
    for(i=1;i<TD_ACTORS-1;i++){
        actors[i]=PLAYER;actors[i].prev=actors[i].next=NULL;actors[i].flags=ACTOR_FLAG_PERSISTENT;actors[i].collision_group=0;actors[i].script.bank=actors[i].script_update.bank=0;
        // Place copied actors on the inactive list before activating them.
        actors[i].next=actors_inactive_head; if(actors_inactive_head)actors_inactive_head->prev=&actors[i];actors_inactive_head=&actors[i];
        activate_actor(&actors[i]);
    }
    if(td_streetcar_view_district!=TD_DISTRICT_ISLANDS)
        td_world_traffic_init(td_streetcar_view_district,td_traffic_u,td_traffic_v,td_traffic_leg,td_traffic_samples);
    td_people_reset();
    td_frame(&actors[1],40);td_set_target();td_position(&PLAYER,td.u>>4,td.v>>4);
    td_frame(&PLAYER,td.onfoot?32:td.vehicle*8+((td.heading+1)&15)/2);td_traffic_present();td_pedestrians();td_streetcar_runtime_present();
    camera_settings=CAMERA_LOCK_FLAG;camera_offset_x=0;camera_offset_y=-16;camera_deadzone_x=8;camera_deadzone_y=8;
    if(cold)td_audio_init();td_ui_init();
}
void toronto_update(void) BANKED {
    td_terrain_cache_t terrain;
    UWORD now,elapsed,seconds,old_u,old_v,tram_elapsed;UBYTE motion,step,was_entering,contact,consumed=0;
    /* Restore the ordinary background before menus reuse its VRAM tiles. */
    td_aircraft_render_restore();
    if(td_transition_pending){td_last_frame=sys_time;if(td_transition_pending==2&&td_district_queue(td_transition_district))td_transition_pending=1;return;}
    now=sys_time;elapsed=now-td_last_frame;td_last_frame=now;
    td_result_b_release&=joy;
    tram_elapsed=elapsed;
    td_corner_used=0;
    motion=elapsed>4?4:elapsed;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE){td_result_b_release=joy;td_menu_update();td_sound_update();return;}
    if(INPUT_START_PRESSED){td_resume_mode=td.mode;td.mode=TD_PAUSE;td.menu=0;td_audio_play(TD_AUDIO_MENU);td_ui_draw();td_sound_update();return;}
    /* A deliberate cancel wins over departure on the same input frame. */
    if(td.mode==TD_WAIT&&INPUT_B_PRESSED){td.mode=TD_ROAM;consumed=1;td_save();td_ui_draw();}
    if(td_notice_timer){if(!--td_notice_timer){td.msg=0;td_ui_draw();}}
    /* Keep deadlines and transit tied to every VBlank, even when rendering falls behind. */
    seconds=0;
    if(elapsed>=60){seconds=elapsed/60;elapsed%=60;}
    elapsed+=td.subsecond;
    if(elapsed>=60){elapsed-=60;seconds++;}td.subsecond=elapsed;
    while(seconds--&&(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE)){
        td_second();if(td_transition_pending)return;
    }
    td_streetcar_runtime_prepare(tram_elapsed);
    if(td_streetcar_view_district!=td_district_current()){
        td_transition_district=td_streetcar_view_district;
        td_transition_pending=td_district_queue(td_transition_district)?1:2;return;
    }
    if(td.mode==TD_ROAM||td.mode==TD_WAIT){
        contact=td_streetcar_runtime_recover_contact(td.onfoot);
        if(contact==TD_STREETCAR_PARK_UNCHANGED)td_contact_episode=0;
        else if(contact==TD_STREETCAR_CONTACT_INVALID){
            td.speed=0;td_vx=td_vy=0;motion=0;
            if(td.msg!=18)td_message(18);
        }
        else{
            td.speed=0;td_vx=td_vy=0;
            if(contact==TD_STREETCAR_PARK_MOVED&&td.mode==TD_WAIT&&!td_wait_at_origin()){td.mode=TD_ROAM;td_ui_draw();}
            if(!td_contact_episode){
                td_contact_episode=1;
                if(!td.cooldown){
                    td.cooldown=60;
                    if(!td.onfoot&&td.job!=TD_NONE&&td.stage)td.health=td.health>12?td.health-12:0;
                }
                td_message(td.onfoot?18:5);
                if(td.job!=TD_NONE&&!td.health)td_finish(FALSE);
                td_save();
            }else if(contact==TD_STREETCAR_PARK_MOVED)td_save();
            if(contact==TD_STREETCAR_PARK_BLOCKED&&td.msg!=(td.onfoot?18:5)){
                td.msg=td.onfoot?18:5;td_notice_timer=90;td_ui_draw();
            }
        }
    }
    terrain.valid=0;
    for(step=0;step<motion&&(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE);step++){
        td_tick++;td_input_edge=step==0&&!consumed;
        if(td.mode==TD_ROAM){
            old_u=td.u;old_v=td.v;was_entering=td_entry_timer;
            td_drive(&terrain);if(!was_entering&&td_cross_portal(old_u,old_v))return;
        }
    }
    if(motion&&td_streetcar_view_district!=TD_DISTRICT_ISLANDS&&
       (td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE)){
        /* Autonomous road motion uses a sixteen-VBlank quantum. Every
           admitted move still sweeps its whole body; couriers, pedestrians,
           tram contact, capture and collision penalties update each render.
           A delayed update retains the existing eight-pixel catch-up cap. */
        td_traffic_elapsed=tram_elapsed>=16||td_traffic_elapsed+tram_elapsed>=16?
            16:td_traffic_elapsed+tram_elapsed;
        UBYTE mask=td_traffic_retreat_mask;
        UBYTE pursuing=td.wanted&&td.mode!=TD_RIDE&&!td_streetcar_ride_view;
        if(pursuing)td_police_elapsed=tram_elapsed>=16||td_police_elapsed+tram_elapsed>=16?
            16:td_police_elapsed+tram_elapsed;
        else td_police_elapsed=0;
        if(td_traffic_elapsed>=16){
            td_traffic_advance=td_traffic_elapsed*8;td_traffic_elapsed=0;mask=63;
            if(pursuing&&td_police_elapsed<4&&!(td_traffic_retreat_mask&4))mask&=~4;
        }
        td_police_advance=0;
        if(pursuing&&td_police_elapsed>=4){
            td_police_advance=td_police_elapsed*8;td_police_elapsed=0;mask|=4;
        }
        /* Small validated escapes keep their original per-render cadence.
           Pursuing police have a four-VBlank quantum; ordinary road traffic
           uses sixteen. Safety/contact queries never wait for these quanta. */
        if(mask)td_traffic_motion(mask);else td_traffic_contacts();
    }
    if(td.mode==TD_ROAM&&!consumed&&INPUT_SELECT_PRESSED)td_interact();
    if(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE){td_traffic_present();td_pedestrians();}
    td_streetcar_runtime_prepare(0);
    td_position(&PLAYER,td.u>>4,td.v>>4);td_streetcar_runtime_present();
    if(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE)
    {td_aircraft_update(tram_elapsed,td_streetcar_focus_u>>4,td_streetcar_focus_v>>4);td_boats_update(tram_elapsed);}
    td_sound_update();
}
