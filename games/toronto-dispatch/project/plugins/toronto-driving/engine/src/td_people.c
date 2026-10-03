#pragma bank 255
#include <string.h>
#include "td_people.h"
#include "td_game.h"
#include "td_city_sprites.h"
#include "td_streetcar_runtime.h"
#include "actor.h"

/* Preserve route identity across nearby-list refreshes. Five bytes per slot:
 * identity, six-second recovery, frozen triangle phase, accumulated walk lag,
 * and recovery flag. A recovered human resumes from the same position. */
typedef struct { UBYTE route,stun,phase,lag,recover; } td_person_t;
static td_person_t td_people[6];
static UWORD td_nearby_routes[6][2];
static UBYTE td_ped_route[6],td_ped_refresh;
static UWORD td_ped_anchor_u,td_ped_anchor_v,td_people_last_u,td_people_last_v;
static UWORD td_people_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static UBYTE td_person_road_clear(UWORD u,UWORD v){
    UBYTE i;
    for(i=2;i<8;i++)if(!(actors[i].flags&ACTOR_FLAG_HIDDEN)&&
        td_people_distance(u,actors[i].pos.x>>5)<9&&td_people_distance(v,actors[i].pos.y>>5)<9)return FALSE;
    if(td.onfoot&&td.park_district==td.district&&td_people_distance(u,td.park_u>>4)<9&&
        td_people_distance(v,td.park_v>>4)<9)return FALSE;
    return td_streetcar_runtime_pedestrian_clear(td_streetcar_view_district,u*16,v*16);
}

void td_people_reset(void) BANKED {
    UBYTE i;memset(td_people,0,sizeof(td_people));
    for(i=0;i<6;i++)td_ped_route[i]=td_people[i].route=TD_NONE;
    td_ped_refresh=1;td_ped_anchor_u=td.u>>4;td_ped_anchor_v=td.v>>4;
    td_people_last_u=td.u;td_people_last_v=td.v;
}

/* Sweep actual movement in increments no larger than one pixel. A scene change must reset
 * this anchor; reject a remote discontinuity instead of striking distant NPCs. */
static UBYTE td_person_contact(UWORD u,UWORD v){
    UBYTE n,steps;WORD du=(WORD)td.u-td_people_last_u,dv=(WORD)td.v-td_people_last_v;
    UWORD span=du<0?-du:du,other=dv<0?-dv:dv;WORD x,y;
    UWORD low,high,point;
    if(other>span)span=other;
    if(!span||span>512)return FALSE;
    /* Every divided sample lies inside the endpoint rectangle. Reject only
     * beyond its strict128-Q4 contact radius, before the sample divisions.
     * Keep the original oracle for malformed wrapping16-bit endpoints. */
    if(td_people_distance(td.u,td_people_last_u)<=512){
        low=td.u<td_people_last_u?td.u:td_people_last_u;
        high=td.u>td_people_last_u?td.u:td_people_last_u;point=u*16;
        if((point<low&&low-point>=128)||(point>high&&point-high>=128))return FALSE;
    }
    if(td_people_distance(td.v,td_people_last_v)<=512){
        low=td.v<td_people_last_v?td.v:td_people_last_v;
        high=td.v>td_people_last_v?td.v:td_people_last_v;point=v*16;
        if((point<low&&low-point>=128)||(point>high&&point-high>=128))return FALSE;
    }
    /* span<=512 and steps<=32 keep du*n/dv*n within signed16-bit range.
     * Avoid pulling 32-bit arithmetic helpers into the scarce fixed ROM bank. */
    steps=(span+15)/16;
    for(n=0;n<=steps;n++){
        x=td_people_last_u+du*(WORD)n/steps;y=td_people_last_v+dv*(WORD)n/steps;
        if(td_people_distance(x,u*16)<128&&td_people_distance(y,v*16)<128)return TRUE;
    }
    return FALSE;
}

UBYTE td_people_present(UBYTE tick) BANKED {
    UBYTE i,route,phase,raw,refresh,hits=0,visible,clock_phase;UWORD u,v;
    UWORD player_u=(td_streetcar_ride_view?td_streetcar_focus_u:td.u)>>4;
    UWORD player_v=(td_streetcar_ride_view?td_streetcar_focus_v:td.v)>>4;
    td_person_t *person;
    refresh=!--td_ped_refresh||td_people_distance(player_u,td_ped_anchor_u)>64||td_people_distance(player_v,td_ped_anchor_v)>64;
    if(refresh){
        td_ped_refresh=16;td_ped_anchor_u=player_u;td_ped_anchor_v=player_v;
        td_refresh_routes(td_ped_route,td_nearby_routes);
    }
    clock_phase=(td.seconds*12+td.subsecond/5)&127;
    for(i=0;i<6;i++){
        route=td_ped_route[i];person=&td_people[i];
        raw=(clock_phase+route*37)&127;
        if(person->route!=route){
            memset(person,0,sizeof(*person));person->route=route;
            person->phase=raw;
        }
        if(route==TD_NONE){actors[9+i].flags|=ACTOR_FLAG_HIDDEN;continue;}
        if(person->recover){person->lag=(raw-person->phase)&127;person->recover=0;}
        phase=person->stun?person->phase:(raw-person->lag)&127;
        u=td_nearby_routes[i][0]+(phase<64?phase:127-phase);v=td_nearby_routes[i][1];
        if(!person->stun&&!td_person_road_clear(u,v)){
            phase=person->phase;person->lag=(raw-phase)&127;
            u=td_nearby_routes[i][0]+(phase<64?phase:127-phase);
            if(!td_person_road_clear(u,v)){actors[9+i].flags|=ACTOR_FLAG_HIDDEN;continue;}
        }
        person->phase=phase;
        visible=td_people_distance(player_u,u)<112&&td_people_distance(player_v,v)<96;
        if(visible)actors[9+i].flags&=~ACTOR_FLAG_HIDDEN;else actors[9+i].flags|=ACTOR_FLAG_HIDDEN;
        if(visible&&!person->stun&&td.mode==TD_ROAM&&!td.onfoot&&
           (td.speed>2||td.speed<-2)&&td_person_contact(u,v)){
            person->stun=6;person->phase=phase;hits++;
        }
        actors[9+i].pos.x=u*32;actors[9+i].pos.y=v*32;
        td_civilian_present(&actors[9+i],route%2,person->stun?4:(phase<64?0:2)+((tick>>3)&1));
    }
    td_people_last_u=td.u;td_people_last_v=td.v;return hits;
}

UBYTE td_people_second(void) BANKED {
    UBYTE i;for(i=0;i<6;i++)if(td_people[i].stun&&!--td_people[i].stun)td_people[i].recover=1;
    if(td.wanted&&td.wanted_left&&!--td.wanted_left){
        td.wanted--;if(td.wanted)td.wanted_left=30;
        td_save();return TRUE;
    }
    return FALSE;
}

UBYTE td_people_police(UWORD u,UWORD v) BANKED {
    UWORD fine;
    if(td.mode!=TD_ROAM||!td.wanted)return FALSE;
    if(td_people_distance(td.u,u)>=96*16||td_people_distance(td.v,v)>=96*16)return FALSE;
    /* Stay wanted while the nearby road patrol pursues. Thirty active
       seconds outside this range cool one level; capture clears the chase. */
    td.wanted_left=30;
    /* A32px stop range includes the24px traffic stop line plus the bounded
       eight-pixel advance. Capture must work when the courier itself keeps
       a law-abiding patrol outside an occupied junction. */
    if(td_people_distance(td.u,u)>=32*16||td_people_distance(td.v,v)>=32*16)return FALSE;
    fine=25*td.wanted*td.wanted;td.cash=td.cash>fine?td.cash-fine:0;
    td.wanted=td.wanted_left=0;return TRUE;
}
