#pragma bank 255
#include <string.h>
#include "td_people.h"
#include "td_police.h"
#include "td_game.h"
#include "td_city_sprites.h"
#include "td_streetcar_runtime.h"
#include "td_roads.h"
#include "collision.h"
#include "actor.h"

/* Eleven transient bytes per nearby person. A struck identity flies briefly
 * then stays prone; a district-local bitset prevents route refresh respawns.
 * Neither corpses nor animation state is part of the 58-byte cartridge save. */
typedef struct {
    UBYTE route,stun,phase,lag,recover,state,timer;
    signed char dx,dy,ox,oy;
} td_person_t;
#define TD_PERSON_ADMITTED 128
#define TD_PERSON_SOCIAL 24
#define TD_PERSON_APPROACH 8
#define TD_PERSON_TALK 16
#define TD_PERSON_RETURN 24
static UBYTE td_people_dead[32],td_people_last_tick,td_people_boot,td_people_last_clock;
static td_person_t td_people[TD_PEOPLE_COUNT];
static UWORD td_nearby_routes[TD_PEOPLE_COUNT][2];
static UBYTE td_ped_route[TD_PEOPLE_COUNT],td_ped_refresh;
static UWORD td_ped_anchor_u,td_ped_anchor_v,td_people_last_u,td_people_last_v;
static UWORD td_people_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}

/* Include the entire small sheet plus an eight-pixel camera deadzone margin.
 * A new identity may be armed outside this rectangle, never inside it. */
static UBYTE td_person_in_view(UWORD u,UWORD v){
    WORD left,top;UWORD pu=(td_streetcar_ride_view?td_streetcar_focus_u:td.u)>>4;
    UWORD pv=(td_streetcar_ride_view?td_streetcar_focus_v:td.v)>>4;
    left=pu<80?0:pu>944?864:pu-80;
    top=pv<88?0:pv>920?832:pv-88;
    return (WORD)u+12>=left&&(WORD)u-12<=left+160&&
        (WORD)v+12>=top&&(WORD)v-20<=top+144;
}
UBYTE td_people_route_visible(UBYTE route) BANKED {
    UBYTE i;
    for(i=0;i<TD_PEOPLE_COUNT;i++)if(td_people[i].route==route&&
        !(actors[9+i].flags&ACTOR_FLAG_HIDDEN)&&
        td_person_in_view(actors[9+i].pos.x>>5,actors[9+i].pos.y>>5))return TRUE;
    return FALSE;
}
/* Fleet radii never exceed160Q4. A two-bucket16px gap cannot overlap;
   near buckets still use the unchanged exact centres and strict margins. */
#define td_person_bucket_far(a,b) (((a)<(b)&&(UBYTE)((b)-(a))>1)||\
    ((a)>(b)&&(UBYTE)((a)-(b))>1))
static UBYTE td_person_road_clear(UWORD u,UWORD v){
    static const UBYTE fleet_radius[8]={144,144,144,160,144,160,144,144};
    UBYTE i,bu,bv,cu,cv;UWORD pu=u*16,pv=v*16,fu,fv;actor_t *car;
    bu=pu>>8;bv=pv>>8;
    /* Bus/fire bodies have a 7px half extent, humans 3px. Retain the
       existing 9px margin for smaller vehicles and compare exact Q4
       centres so half-pixel fleet advances cannot admit an overlap. */
    /* The for-update executes on both hidden/far continue paths. Start at
     * the six contiguous authored roles, jump exactly once to added cars
     *17/18, and keep the existing local pointer. No actor stride product. */
    for(i=0,car=&actors[2];i<8;i++,car=i==6?&actors[17]:car+1){
        if(car->flags&ACTOR_FLAG_HIDDEN)continue;
        /* Q5 centre>>9 is exactly the Q4 centre's high byte. Read only
         * those bytes first; distant actors never materialise either full
         * Q4 coordinate or repeat the large native actor-stride lookup. */
        cu=(UBYTE)(car->pos.x>>8)>>1;cv=(UBYTE)(car->pos.y>>8)>>1;
        if(td_person_bucket_far(bu,cu)||td_person_bucket_far(bv,cv))continue;
        fu=car->pos.x>>1;fv=car->pos.y>>1;
        if(td_people_distance(pu,fu)<fleet_radius[i]&&
           td_people_distance(pv,fv)<fleet_radius[i])return FALSE;
    }
    if(td.onfoot&&td.park_district==td_streetcar_view_district&&td_people_distance(pu,td.park_u)<144&&
        td_people_distance(pv,td.park_v)<144)return FALSE;
    return td_streetcar_runtime_pedestrian_clear(td_streetcar_view_district,pu,pv);
}

/* Only a proposed walking step yields to the physically occupied local car.
 * Keep the original road-only fallback for an already visible/recovering
 * human; a driver causing an overlap must not erase that human's history. */
static UBYTE td_person_courier_blocks(UWORD u,UWORD v){
    return !td.onfoot&&!td_streetcar_ride_view&&td.district==td_streetcar_view_district&&
        td_people_distance(u*16,td.u)<176&&
        td_people_distance(v*16,td.v)<176;
}

void td_people_reset(void) BANKED {
    UBYTE i;memset(td_people,0,sizeof(td_people));memset(td_people_dead,0,sizeof(td_people_dead));
    td_people_last_tick=0;td_people_boot=1;
    td_people_last_clock=(td.seconds*12+td.subsecond/5)&127;
    for(i=0;i<TD_PEOPLE_COUNT;i++)td_ped_route[i]=td_people[i].route=TD_NONE;
    td_ped_refresh=1;td_ped_anchor_u=td.u>>4;td_ped_anchor_v=td.v>>4;
    td_people_last_u=td.u;td_people_last_v=td.v;
}

/* Sweep actual movement in increments no larger than one pixel. A scene change must reset
 * this anchor; reject a remote discontinuity instead of striking distant NPCs. */
static UBYTE td_person_sweep(UWORD u,UWORD v,UBYTE radius){
    UBYTE n,steps;WORD du=(WORD)td.u-td_people_last_u,dv=(WORD)td.v-td_people_last_v;
    UWORD span=du<0?-du:du,other=dv<0?-dv:dv;WORD x,y;
    UWORD low,high,point;
    if(other>span)span=other;
    if(!span||span>512)return FALSE;
    /* Every divided sample lies inside the endpoint rectangle. Reject only
     * beyond its strict radius, before the sample divisions.
     * Keep the original oracle for malformed wrapping16-bit endpoints. */
    if(td_people_distance(td.u,td_people_last_u)<=512){
        low=td.u<td_people_last_u?td.u:td_people_last_u;
        high=td.u>td_people_last_u?td.u:td_people_last_u;point=u*16;
        if((point<low&&low-point>=radius)||(point>high&&point-high>=radius))return FALSE;
    }
    if(td_people_distance(td.v,td_people_last_v)<=512){
        low=td.v<td_people_last_v?td.v:td_people_last_v;
        high=td.v>td_people_last_v?td.v:td_people_last_v;point=v*16;
        if((point<low&&low-point>=radius)||(point>high&&point-high>=radius))return FALSE;
    }
    /* span<=512 and steps<=32 keep du*n/dv*n within signed16-bit range.
     * Avoid pulling 32-bit arithmetic helpers into the scarce fixed ROM bank. */
    steps=(span+15)/16;
    for(n=0;n<=steps;n++){
        x=td_people_last_u+du*(WORD)n/steps;y=td_people_last_v+dv*(WORD)n/steps;
        if(td_people_distance(x,u*16)<radius&&td_people_distance(y,v*16)<radius)return TRUE;
    }
    return FALSE;
}
static UBYTE td_person_contact(UWORD u,UWORD v){return td_person_sweep(u,v,160);}

static UBYTE td_person_social_body_clear(UWORD u,UWORD v){
    UBYTE left,right,top,bottom,axis,clear=TRUE,hx=tile_hit_x,hy=tile_hit_y;
    if(u<3||v<3||u>1020||v>972)return FALSE;
    left=(u-3)>>3;right=(u+3)>>3;top=(v-3)>>3;bottom=(v+3)>>3;
    /* Six pixels span at most two tiles per axis. Their unique rectangle
       is exactly the four original corner queries, with no intermediate
       tile or additional collision bit. Read each tile once and switch
       the collision bank once per shorter row/column, preserving hit state. */
    if(bottom-top<=right-left){
        for(axis=top;axis<=bottom;axis++)if(tile_col_test_range_x(15,axis,left,right)){clear=FALSE;break;}
    }else{
        for(axis=left;axis<=right;axis++)if(tile_col_test_range_y(15,axis,top,bottom)){clear=FALSE;break;}
    }
    tile_hit_x=hx;tile_hit_y=hy;return clear;
}
static UBYTE td_person_social_clear(UWORD u,UWORD v){
    return td_person_social_body_clear(u,v)&&
        td_person_road_clear(u,v)&&!td_person_courier_blocks(u,v);
}
/* Social movement stays tied to its original route. After chatting, return
 * visibly to that route instead of jumping back to a global clock position. */
static void td_person_social_step(UBYTE i,UBYTE elapsed,UBYTE walking,UWORD *u,UWORD *v){
    td_person_t *person=&td_people[i],*other;
    UBYTE mode=person->state&TD_PERSON_SOCIAL,partner=person->state&7;
    WORD du,dv,nu,nv;
    if(!mode){if(person->timer)person->timer=elapsed>=person->timer?0:person->timer-elapsed;return;}
    if(mode!=TD_PERSON_RETURN){
        other=&td_people[partner];
        if(other->stun||other->route==TD_NONE||!(other->state&TD_PERSON_SOCIAL)||
           (other->state&7)!=i||(actors[9+partner].flags&ACTOR_FLAG_HIDDEN)){
            person->state=(person->state&TD_PERSON_ADMITTED)|TD_PERSON_RETURN;mode=TD_PERSON_RETURN;
        }
    }
    if(mode==TD_PERSON_TALK){
        if(elapsed>=person->timer){person->state=(person->state&TD_PERSON_ADMITTED)|TD_PERSON_RETURN;mode=TD_PERSON_RETURN;}
        else{person->timer-=elapsed;return;}
    }
    if(mode==TD_PERSON_APPROACH){
        if(elapsed>=person->timer){person->state=(person->state&TD_PERSON_ADMITTED)|TD_PERSON_RETURN;mode=TD_PERSON_RETURN;}
        else person->timer-=elapsed;
    }
    if(!walking)return;
    if(mode==TD_PERSON_APPROACH){
        du=(WORD)(actors[9+partner].pos.x>>5)-*u;
        dv=(WORD)(actors[9+partner].pos.y>>5)-*v;
        if(td_people_distance(actors[9+partner].pos.x>>5,*u)<=8&&
           td_people_distance(actors[9+partner].pos.y>>5,*v)<=8){
            person->state=(person->state&TD_PERSON_ADMITTED)|TD_PERSON_TALK|partner;
            person->timer=120;return;
        }
    }
    else if(mode==TD_PERSON_RETURN){du=-person->ox;dv=-person->oy;}
    else return; /* Only approach/return states may consume a walking step. */
    nu=(WORD)*u+(du>0?1:du<0?-1:0);nv=(WORD)*v+(dv>0?1:dv<0?-1:0);
    if(td_people_distance(nu,td_nearby_routes[i][0]+(person->phase<64?person->phase:127-person->phase))>32||
       td_people_distance(nv,td_nearby_routes[i][1])>32)return;
    if(td_person_social_clear(nu,nv)){
        person->ox+=nu-*u;person->oy+=nv-*v;*u=nu;*v=nv;
    }
    if(mode==TD_PERSON_RETURN&&!person->ox&&!person->oy){
        person->state&=TD_PERSON_ADMITTED;person->timer=180;
    }
}
static void td_people_social_pairs(void){
    UBYTE i,j;UWORD u,v;
    for(i=0;i<TD_PEOPLE_COUNT;i++){
        if(td_people[i].stun||td_people[i].timer||!(td_people[i].state&TD_PERSON_ADMITTED)||
           (td_people[i].state&TD_PERSON_SOCIAL)||(actors[9+i].flags&ACTOR_FLAG_HIDDEN))continue;
        u=actors[9+i].pos.x>>5;v=actors[9+i].pos.y>>5;
        if(!td_person_in_view(u,v))continue;
        for(j=i+1;j<TD_PEOPLE_COUNT;j++){
            if(td_people[j].stun||td_people[j].timer||!(td_people[j].state&TD_PERSON_ADMITTED)||
               (td_people[j].state&TD_PERSON_SOCIAL)||(actors[9+j].flags&ACTOR_FLAG_HIDDEN))continue;
            if(td_people_distance(u,actors[9+j].pos.x>>5)>64||
               td_people_distance(v,actors[9+j].pos.y>>5)>64)continue;
            td_people[i].state=TD_PERSON_ADMITTED|TD_PERSON_APPROACH|j;
            td_people[j].state=TD_PERSON_ADMITTED|TD_PERSON_APPROACH|i;
            td_people[i].timer=td_people[j].timer=180;break;
        }
    }
}

UBYTE td_people_present(UBYTE tick) BANKED {
    UBYTE i,route,phase,raw,refresh,hits=0,visible,continuous,clock_phase,elapsed,pose,lift,walking;UWORD u,v;
    WORD du,dv;
    UWORD player_u=(td_streetcar_ride_view?td_streetcar_focus_u:td.u)>>4;
    UWORD player_v=(td_streetcar_ride_view?td_streetcar_focus_v:td.v)>>4;
    td_person_t *person;
    refresh=!--td_ped_refresh||td_people_distance(player_u,td_ped_anchor_u)>64||td_people_distance(player_v,td_ped_anchor_v)>64;
    if(refresh){
        td_ped_refresh=16;td_ped_anchor_u=player_u;td_ped_anchor_v=player_v;
        td_refresh_routes(td_ped_route,td_nearby_routes);
    }
    clock_phase=(td.seconds*12+td.subsecond/5)&127;
    walking=clock_phase!=td_people_last_clock;td_people_last_clock=clock_phase;
    elapsed=tick-td_people_last_tick;td_people_last_tick=tick;
    if(elapsed>24)elapsed=24;
    for(i=0;i<TD_PEOPLE_COUNT;i++){
        route=td_ped_route[i];person=&td_people[i];
        continuous=person->route==route&&(person->state&TD_PERSON_ADMITTED)&&!(actors[9+i].flags&ACTOR_FLAG_HIDDEN);
        raw=(clock_phase+route*37)&127;
        if(person->route!=route){
            memset(person,0,sizeof(*person));person->route=route;
            person->phase=raw;
        }
        if(route==TD_NONE){actors[9+i].flags|=ACTOR_FLAG_HIDDEN;continue;}
        /* A dead route whose local visual slot was replaced cannot reappear as
         * a fresh walking human before the next district/reset load. */
        if(!person->stun&&(td_people_dead[route>>3]&(1<<(route&7)))){
            actors[9+i].flags|=ACTOR_FLAG_HIDDEN;continue;
        }
        if(person->stun&&person->recover<24){
            person->recover=person->recover+elapsed>24?24:person->recover+elapsed;
        }
        phase=person->stun||(person->state&TD_PERSON_SOCIAL)?person->phase:(raw-person->lag)&127;
        if(person->state&TD_PERSON_SOCIAL)person->lag=(raw-phase)&127;
        u=td_nearby_routes[i][0]+(phase<64?phase:127-phase);v=td_nearby_routes[i][1];
        u=(WORD)u+person->ox;v=(WORD)v+person->oy;
        if(!person->stun&&(!td_person_road_clear(u,v)||td_person_courier_blocks(u,v))){
            phase=person->phase;person->lag=(raw-phase)&127;
            u=(WORD)(td_nearby_routes[i][0]+(phase<64?phase:127-phase))+person->ox;
            if(!td_person_road_clear(u,v)){actors[9+i].flags|=ACTOR_FLAG_HIDDEN;person->state&=~TD_PERSON_ADMITTED;continue;}
        }
        person->phase=phase;
        if(!person->stun)td_person_social_step(i,elapsed,walking,&u,&v);
        /* Scene loading seeds already-present residents once. Thereafter a
         * new/hidden/recycled identity must first exist entirely offscreen.
         * Arming it outside the camera lets ordinary world motion enter. */
        if(!(person->state&TD_PERSON_ADMITTED)){
            if(td_people_boot||!td_person_in_view(u,v))person->state|=TD_PERSON_ADMITTED;
            else{actors[9+i].flags|=ACTOR_FLAG_HIDDEN;continue;}
        }
        visible=td_people_distance(player_u,u)<176&&td_people_distance(player_v,v)<144;
        /* A new identity or hidden human has no visible crossing history.
         * Defer appearance inside the occupied courier or its prior sweep;
         * half7 vehicle + half3 human gives a10px impact envelope;
         * one more pixel keeps newly appearing people clear of that edge. The
         * endpoint check also protects stationary/discontinuous movement.
         * A paid remote view must not inherit the origin's car occupancy. */
        if(visible&&!continuous&&!td.onfoot&&!td_streetcar_ride_view&&
           td.district==td_streetcar_view_district&&
           ((td_people_distance(td.u,u*16)<176&&td_people_distance(td.v,v*16)<176)||
            td_person_sweep(u,v,176))){
            volatile actor_t *appearing=&actors[9+i];
            /* Reload the actual actor after the context/query predicates;
             * avoid the pinned SDCC conditional compound-store pattern. */
            appearing->flags=appearing->flags|ACTOR_FLAG_HIDDEN;
            person->state&=~TD_PERSON_ADMITTED;continue;
        }
        if(visible)actors[9+i].flags&=~ACTOR_FLAG_HIDDEN;else actors[9+i].flags|=ACTOR_FLAG_HIDDEN;
        if(visible&&continuous&&!person->stun&&td.mode==TD_ROAM&&!td.onfoot&&
           (td.speed>2||td.speed<-2)&&td_person_contact(u,v)){
            person->stun=6;person->phase=phase;person->recover=person->lag=0;
            person->state&=TD_PERSON_ADMITTED;
            du=(WORD)td.u-td_people_last_u;dv=(WORD)td.v-td_people_last_v;
            person->dx=du>0?1:du<0?-1:0;person->dy=dv>0?1:dv<0?-1:0;
            td_people_dead[route>>3]|=1<<(route&7);hits++;
        }
        pose=(phase<64?0:2)+((tick>>3)&1);lift=0;
        if((person->state&TD_PERSON_SOCIAL)==TD_PERSON_TALK){
            UBYTE partner=person->state&7;
            /* Facing each other with a restrained alternating hand/step pose
             * reuses original frames and consumes no extra hardware object. */
            pose=(actors[9+partner].pos.x>u*32?0:2)+((tick>>4)&1);
        }
        if(person->stun){
            /* Twelve ground pixels at most, with a four-pixel visual arc.
             * Stop displacement before solid land/water/rail tiles. The
             * frozen route phase and lag never advance again in this load. */
            raw=person->recover>>1;
            while(person->lag<raw){
                du=(WORD)u+person->dx*(person->lag+1);
                dv=(WORD)v+person->dy*(person->lag+1);
                if(du<3||dv<3||du>1020||dv>972||!td_road_walkable(du,dv))break;
                person->lag++;
            }
            u=(WORD)u+person->dx*person->lag;v=(WORD)v+person->dy*person->lag;
            if(person->recover<24){
                lift=person->recover<12?person->recover/3:(24-person->recover)/3;
                pose=TD_CIVILIAN_HIT;
            }else pose=TD_CIVILIAN_PRONE;
        }
        actors[9+i].pos.x=u*32;actors[9+i].pos.y=(v-lift)*32;
        td_civilian_present(&actors[9+i],route%4,pose);
    }
    td_people_boot=0;td_people_social_pairs();
    td_people_last_u=td.u;td_people_last_v=td.v;return hits;
}

UBYTE td_people_second(void) BANKED {
    /* Corpses remain down until district load; police time still advances. */
    if(td.wanted&&td_police_observed()){td.wanted_left=30;return FALSE;}
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
    if(td_police_observed())td.wanted_left=30;
    /* A32px stop range includes the24px traffic stop line plus the bounded
       eight-pixel advance. Capture must work when the courier itself keeps
       a law-abiding patrol outside an occupied junction. */
    if(td_people_distance(td.u,u)>=32*16||td_people_distance(td.v,v)>=32*16)return FALSE;
    fine=25*td.wanted*td.wanted;td.cash=td.cash>fine?td.cash-fine:0;
    td.wanted=td.wanted_left=0;return TRUE;
}
