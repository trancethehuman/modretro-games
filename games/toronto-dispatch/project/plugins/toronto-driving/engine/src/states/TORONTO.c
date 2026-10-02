#pragma bank 255
#include <string.h>
#include "states/TORONTO.h"
#include "td_game.h"
#include "td_world_routes.h"
#include "td_audio.h"
#include "td_district.h"
#include "td_world.h"
#include "td_transit.h"
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
static UBYTE td_transition_pending;
typedef char td_serialized_state_must_be_58_bytes[(sizeof(td_state_t)==58)?1:-1];
static const BYTE td_dx[]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
static const BYTE td_dy[]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};
static const UWORD td_rows[]={64,176,288,400,528,640,720,784};
static const UWORD td_cols[]={80,208,336,480,560,640,720,816,944};
static UWORD td_traffic_u[6],td_traffic_v[6];
static td_traffic_sample_t td_traffic_samples[6];
static UWORD td_nearby_routes[6][2];
static UBYTE td_traffic_leg[6],td_ped_route[6],td_ped_refresh;
static UWORD td_ped_anchor_u,td_ped_anchor_v;
static UBYTE td_tick,td_notice_timer,td_red_cooldown,td_turn_tick,td_entry_timer,td_entry_target,td_walk_dir;
UBYTE td_resume_mode;
static WORD td_vx,td_vy;
static UWORD td_last_frame;
static UBYTE td_corner_used;
static UBYTE td_input_edge;
static UBYTE td_change_district(UBYTE district,UWORD u,UWORD v);

static UWORD td_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static void td_position(actor_t *a,UWORD u,UWORD v){
    // GBVM actors use five fractional bits; driving state uses four.
    a->pos.x=u*32; a->pos.y=v*32;
}
static void td_frame(actor_t *a,UBYTE f){if(a->frame_start!=f||a->frame_end!=f+1)actor_set_frames(a,f,f+1);a->anim_tick=255;}
static void td_message(UBYTE m){td.msg=m;td_notice_timer=90;if(m==5||m==13)td_audio_play(TD_AUDIO_IMPACT);td_ui_draw();}
static void td_sound_update(void){td_audio_update(td.speed,td.vehicle,td.onfoot,!!INPUT_B,td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE);}
static UBYTE td_near(td_stop_t *s){return s->district==td.district&&td_distance(td.u>>4,s->u)<15&&td_distance(td.v>>4,s->v)<15;}
static UBYTE td_drivable(UWORD u,UWORD v){
    UBYTE x,y,left,right,top,bottom;
    if(u<8||v<8||u>1016||v>968)return FALSE;
    left=(u-5)>>3;right=(u+5)>>3;top=(v-5)>>3;bottom=(v+5)>>3;
    /* Eleven pixels can overlap three tile rows/columns: corners alone miss rails. */
    for(y=top;y<=bottom;y++)for(x=left;x<=right;x++)if(tile_at(x,y))return FALSE;
    return TRUE;
}
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
    td_position(&actors[1],routed?portal.u:td_target.u,(routed?portal.v:td_target.v)-12);
    if(td_target.district!=td.district&&!routed)actors[1].flags|=ACTOR_FLAG_HIDDEN;
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
static void td_finish(UBYTE success){
    td_audio_play(success?TD_AUDIO_COMPLETE:TD_AUDIO_FAIL);
    if(success){
        if(!(td.complete[td.job>>3]&(1<<(td.job&7)))){td.complete[td.job>>3]|=1<<(td.job&7);td.done++;}
        { UWORD reward=td_job.reward/100*td.health+(td_job.reward%100)*td.health/100+td.left/5;
          td.cash=td.cash>60000-reward?60000:td.cash+reward; }
    }
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
static UBYTE td_board_current_window(void){
    UBYTE fare;
    if(!td_transit_valid(td.transit_origin,td.transit_target)||td_transit_departure(td.transit_origin,td.transit_target,td.seconds))return FALSE;
    fare=td_transit_fare(td.transit_origin);
    if(td.cash<fare){td.mode=TD_ROAM;td_save();td_message(4);return TRUE;}
    /* A fresh free-roaming trip cannot inherit an old contract failure. */
    if(td.job==TD_NONE)td.health=100;
    td.cash-=fare;td.mode=TD_RIDE;td.ride_left=td_transit_duration(td.transit_origin,td.transit_target);
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
        case 2:td.mode=TD_BOARD;if(td.job==TD_NONE)td_ready_offer();else{td.menu=td.job;td_get_job(td.menu,&td_offer);}break;
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
static void td_second(void){
    UWORD arrival_u,arrival_v;
    td.seconds++;
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
    td_save();td_ui_draw();
}
static UBYTE td_signal_stop(UWORD pos,UBYTE vertical,UBYTE reverse){
    UBYTE i,count=vertical?8:9;UWORD line;
    for(i=0;i<count;i++){
        line=(vertical?td_rows[i]:td_cols[i])*16+(reverse?384:-384);
        if(pos>=line&&pos<line+8)return TRUE;
    }
    return FALSE;
}
static void td_traffic_step(void){
    static const UWORD bus_u[]={144,208,208,640,816,816};
    static const UWORD bus_v[]={64,64,176,176,176,64};
    UBYTE i,leg,phase=td.seconds%12,blocked,dirty=0;UWORD u,v,target_u,target_v;
    for(i=0;i<6;i++){
        u=td_traffic_u[i];v=td_traffic_v[i];leg=td_traffic_leg[i];blocked=0;
        if(td.district){
            target_u=td_traffic_samples[i].u;target_v=td_traffic_samples[i].v;
        }else if(i<4){
            target_u=(leg<2?840:48)*16;target_v=(td_rows[2+i]+(leg==0||leg==3?-8:8))*16;
            if((leg==0||leg==2)&&phase>=7)blocked=td_signal_stop(u,0,leg==2);
        }else if(i==4){
            target_u=(leg==0||leg==3?824:808)*16;target_v=(leg<2?792:48)*16;
            if((leg==0||leg==2)&&phase<7)blocked=td_signal_stop(v,1,leg==2);
        }else {target_u=bus_u[leg]*16;target_v=bus_v[leg]*16;}
        if(!blocked){
            if(u<target_u)u+=td_distance(u,target_u)<8?td_distance(u,target_u):8;
            else if(u>target_u)u-=td_distance(u,target_u)<8?td_distance(u,target_u):8;
            else if(v<target_v)v+=td_distance(v,target_v)<8?td_distance(v,target_v):8;
            else if(v>target_v)v-=td_distance(v,target_v)<8?td_distance(v,target_v):8;
            // Road users yield to a courier crossing on foot, including between catch-up steps.
            if((td.mode==TD_ROAM||td.mode==TD_WAIT)&&td.onfoot&&td_distance(td.u,u)<208&&td_distance(td.v,v)<208)continue;
            td_traffic_u[i]=u;td_traffic_v[i]=v;
            if(u==target_u&&v==target_v){
                td_traffic_leg[i]=(leg+1)%(td.district?td_traffic_samples[i].count:i==5?6:4);
                /* Banked targets and frames are cached between junctions. */
                if(td.district)dirty=1;
            }
        }
        if(td.mode==TD_ROAM&&!td.onfoot&&td_distance(td.u,u)<180&&td_distance(td.v,v)<180&&!td.cooldown){
            td.speed/=2;td_vx/=2;td_vy/=2;td.cooldown=60;
            if(td.job!=TD_NONE&&td.stage){td.health=td.health>12?td.health-12:0;if(!td.health){td_finish(FALSE);break;}}td_message(5);
        }
    }
    if(dirty)td_world_traffic_samples(td.district,td_traffic_leg,td_traffic_samples);
}
static void td_traffic_present(void){
    UBYTE i,leg,frame;
    for(i=0;i<6;i++){
        leg=td_traffic_leg[i];
        if(td.district)frame=td_traffic_samples[i].frame;
        else frame=i<4?leg*2:i==4?(leg==0?2:leg==1?4:leg==2?6:0):8+(leg==2?2:leg==0?4:leg==5?6:0);
        td_position(&actors[i+2],td_traffic_u[i]>>4,td_traffic_v[i]>>4);td_frame(&actors[i+2],frame);
    }
    td_position(&actors[8],td.park_u>>4,td.park_v>>4);td_frame(&actors[8],td_entry_timer?44:td.vehicle*8+((td.heading+1)&15)/2);
    if(td.onfoot&&td.park_district==td.district)actors[8].flags&=~ACTOR_FLAG_HIDDEN;else actors[8].flags|=ACTOR_FLAG_HIDDEN;
}
static UWORD td_pedestrian_u(UBYTE slot){
    UBYTE phase=(td.seconds*12+td.subsecond/5+td_ped_route[slot]*37)&127;
    return td_nearby_routes[slot][0]+(phase<64?phase:127-phase);
}
static void td_pedestrians(void){
    UBYTE i,route,phase,refresh;UWORD u,v,player_u=td.u>>4,player_v=td.v>>4;
    refresh=!--td_ped_refresh||td_distance(player_u,td_ped_anchor_u)>64||td_distance(player_v,td_ped_anchor_v)>64;
    if(refresh){
        td_ped_refresh=16;td_ped_anchor_u=player_u;td_ped_anchor_v=player_v;
        td_refresh_routes(td_ped_route,td_nearby_routes);
    }
    for(i=0;i<6;i++){
        route=td_ped_route[i];if(route==TD_NONE){actors[9+i].flags|=ACTOR_FLAG_HIDDEN;continue;}
        phase=(td.seconds*12+td.subsecond/5+route*37)&127;u=td_pedestrian_u(i);v=td_nearby_routes[i][1];
        td_position(&actors[9+i],u,v);td_frame(&actors[9+i],32+(phase<64?0:2)+((td_tick>>3)&1));
        if(td_distance(player_u,u)<112&&td_distance(player_v,v)<96)actors[9+i].flags&=~ACTOR_FLAG_HIDDEN;
        else actors[9+i].flags|=ACTOR_FLAG_HIDDEN;
        if(td.mode==TD_ROAM&&!(actors[9+i].flags&ACTOR_FLAG_HIDDEN)&&!td.onfoot&&!td.cooldown&&td_distance(td.u>>4,u)<10&&td_distance(td.v>>4,v)<10){td.speed/=2;td_vx/=2;td_vy/=2;td.cooldown=45;td_message(13);}
    }
}
static UBYTE td_corner_slide(WORD nu,WORD nv){
    UBYTE i,j,side,clear;WORD shift,shifted;UWORD ax=td_vx<0?-td_vx:td_vx,ay=td_vy<0?-td_vy:td_vy;
    if(!INPUT_A||INPUT_B||td.speed<3||ax==ay||td_corner_used)return FALSE;
    /* A quantised corner may clip the car by a few pixels although a parallel
       lane is open. Sweep only across usable current and proposed footprints. */
    for(i=1;i<=6;i++)for(side=0;side<2;side++){
        shift=side?-(WORD)i*16:(WORD)i*16;
        if(ay>ax&&nv!=(WORD)td.v){
            shifted=(WORD)td.u+shift;
            if(shifted<128||shifted>1016*16||nv<128||nv>968*16||!td_drivable(shifted>>4,nv>>4))continue;
            clear=1;for(j=1;j<=i;j++)if(!td_drivable(((WORD)td.u+(side?-(WORD)j*16:(WORD)j*16))>>4,td.v>>4)){clear=0;break;}
            if(clear){td.u=shifted;td.v=nv;td_vx=0;td_corner_used=1;return TRUE;}
        }else if(ax>ay&&nu!=(WORD)td.u){
            shifted=(WORD)td.v+shift;
            if(shifted<128||shifted>968*16||nu<128||nu>1016*16||!td_drivable(nu>>4,shifted>>4))continue;
            clear=1;for(j=1;j<=i;j++)if(!td_drivable(td.u>>4,((WORD)td.v+(side?-(WORD)j*16:(WORD)j*16))>>4)){clear=0;break;}
            if(clear){td.u=nu;td.v=shifted;td_vy=0;td_corner_used=1;return TRUE;}
        }
    }
    return FALSE;
}
static void td_drive(void){
    WORD nu,nv,target_x,target_y;BYTE walk_x,walk_y;UBYTE limit,turn_period,moving=0,slide=0,speed;UWORD u,v;
    if(td.cooldown)td.cooldown--;
    if(td_red_cooldown)td_red_cooldown--;
    if(td_entry_timer){
        if(!td_entry_target){td.u=(td.u*3+td.park_u)/4;td.v=(td.v*3+td.park_v)/4;}
        if(!--td_entry_timer){if(!td_entry_target){td.u=td.park_u;td.v=td.park_v;td.onfoot=0;}td_set_target();td_save();}
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
    if(INPUT_B){if(td_tick%2==0&&td.speed>-6)td.speed--;}
    else if(INPUT_A){if(td_tick%4==0&&td.speed<limit)td.speed++;}
    else if(td_tick%8==0){if(td.speed>0)td.speed--;else if(td.speed<0)td.speed++;}
    // Traction eases velocity toward heading instead of instantly rotating momentum.
    target_x=td_dx[td.heading]*td.speed;target_y=td_dy[td.heading]*td.speed;
    td_vx+=(target_x-td_vx)/4;td_vy+=(target_y-td_vy)/4;
    if(!td.speed){td_vx/=2;td_vy/=2;}
    nu=td.u+td_vx/16;nv=td.v+td_vy/16;u=nu>>4;v=nv>>4;
    if(td_drivable(u,v)){
        if(td.district==0&&!td_red_cooldown&&td.speed>6&&td_distance(u,640)<14&&td_distance(v,528)<14&&
          (td_distance(td.u>>4,640)>=14||td_distance(td.v>>4,528)>=14)){
            UBYTE ax=td_dx[td.heading]<0?-td_dx[td.heading]:td_dx[td.heading];
            UBYTE ay=td_dy[td.heading]<0?-td_dy[td.heading]:td_dy[td.heading];
            if((ax>ay&&td.seconds%12>=7)||(ax<=ay&&td.seconds%12<7)){if(td.cash>=5)td.cash-=5;td_red_cooldown=120;td_message(7);}
        }
        td.u=nu;td.v=nv;
    }else{
        // A glancing curb contact slides along the free axis and preserves forward speed.
        if(nu!=(WORD)td.u&&td_drivable(nu>>4,td.v>>4)){td.u=nu;td_vy=0;slide=1;}
        if(nv!=(WORD)td.v&&td_drivable(td.u>>4,nv>>4)){td.v=nv;td_vx=0;slide=1;}
        if(!slide)slide=td_corner_slide(nu,nv);
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
    if(cold){
        td_district_reset();td_transition_pending=0;
        td_tick=td_notice_timer=td_red_cooldown=td_entry_timer=td_turn_tick=0;td_vx=td_vy=0;td_last_frame=sys_time;td_corner_used=0;
        if(!td_restore()){
            memset(&td,0,sizeof(td));td.u=560*16;td.v=720*16;td.park_u=td.u;td.park_v=td.v;td.cash=30;td.job=TD_NONE;td.heading=0;td.health=100;
        }
        if(td.job!=TD_NONE&&!td.stage)td.health=100;
        td.speed=0;td_resume_mode=td.mode==TD_WAIT||td.mode==TD_RIDE?td.mode:TD_ROAM;td.mode=TD_HELP;td.msg=0;td.menu=0;
        td_session_live=1;
    }
    /* Restoring another district redirects through the same genuine VM path.
       Regular crossings keep velocity, mission, parked car, clock and audio. */
    if(current!=td.district){
        td_transition_pending=td_district_queue(td.district)?1:2;
        /* Fullscreen help hides the boot scene; never present remote actors on it. */
        PLAYER.flags|=ACTOR_FLAG_HIDDEN;if(cold)td_audio_init();td_ui_init();return;
    }
    td_transition_pending=0;PLAYER.flags&=~ACTOR_FLAG_HIDDEN;
    td.safe_u=td.u;td.safe_v=td.v;
    if(td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)td_get_stop(td.transit_target,&td_cursor);
    if(td.job!=TD_NONE){td_get_job(td.job,&td_job);if(td.stage>=td_job.count)td.job=TD_NONE;}
    actors_len=TD_ACTORS;
    for(i=1;i<TD_ACTORS;i++){
        actors[i]=PLAYER;actors[i].prev=actors[i].next=NULL;actors[i].flags=ACTOR_FLAG_PERSISTENT;actors[i].collision_group=0;actors[i].script.bank=actors[i].script_update.bank=0;
        // Place copied actors on the inactive list before activating them.
        actors[i].next=actors_inactive_head; if(actors_inactive_head)actors_inactive_head->prev=&actors[i];actors_inactive_head=&actors[i];
        activate_actor(&actors[i]);
    }
    if(td.district)td_world_traffic_init(td.district,td_traffic_u,td_traffic_v,td_traffic_leg,td_traffic_samples);
    for(i=0;i<6;i++){
        if(!td.district){td_traffic_u[i]=(i<4?80+i*120:i==4?824:144)*16;td_traffic_v[i]=(i<4?td_rows[2+i]-8:i==4?240:64)*16;td_traffic_leg[i]=i==5?1:0;}
        td_ped_route[i]=TD_NONE;
    }
    td_ped_refresh=1;td_ped_anchor_u=td.u>>4;td_ped_anchor_v=td.v>>4;
    td_frame(&actors[1],40);td_set_target();td_position(&PLAYER,td.u>>4,td.v>>4);
    td_frame(&PLAYER,td.onfoot?32:td.vehicle*8+((td.heading+1)&15)/2);td_traffic_present();td_pedestrians();
    camera_settings=CAMERA_LOCK_FLAG;camera_offset_x=0;camera_offset_y=-16;camera_deadzone_x=8;camera_deadzone_y=8;
    if(cold)td_audio_init();td_ui_init();
}
void toronto_update(void) BANKED {
    UWORD now,elapsed,seconds,old_u,old_v;UBYTE motion,step,was_entering,consumed=0;
    if(td_transition_pending){if(td_transition_pending==2&&td_district_queue(td.district))td_transition_pending=1;return;}
    now=sys_time;elapsed=now-td_last_frame;td_last_frame=now;
    td_corner_used=0;
    motion=elapsed>4?4:elapsed;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE){td_menu_update();td_sound_update();return;}
    if(INPUT_START_PRESSED){td_resume_mode=td.mode;td.mode=TD_PAUSE;td.menu=0;td_audio_play(TD_AUDIO_MENU);td_ui_draw();td_sound_update();return;}
    /* A deliberate cancel wins over departure on the same input frame. */
    if(td.mode==TD_WAIT&&INPUT_B_PRESSED){td.mode=TD_ROAM;consumed=1;td_save();td_ui_draw();}
    if(td_notice_timer){if(!--td_notice_timer){td.msg=0;td_ui_draw();}}
    /* Keep deadlines and transit tied to every VBlank, even when rendering falls behind. */
    seconds=elapsed/60;elapsed%=60;elapsed+=td.subsecond;
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
    }
    if(td.mode==TD_ROAM&&!consumed&&INPUT_SELECT_PRESSED)td_interact();
    if(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE){td_traffic_present();td_pedestrians();}
    td_position(&PLAYER,td.u>>4,td.v>>4);
    td_sound_update();
}
