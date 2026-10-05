#pragma bank 255
#include <string.h>
#include "td_sandbox.h"
#include "td_game.h"
#include "td_people.h"
#include "td_roads.h"
#include "td_district.h"
#include "td_city_sprites.h"
#include "td_actor_render.h"
#include "td_motion.h"
#include "td_streetcar_runtime.h"
#include "input.h"
#include "collision.h"
#include "scroll.h"

typedef struct {UWORD u,v;UBYTE district,vehicle,skin,heading,active,protected;} td_parked_t;
static td_parked_t td_sandbox_parked[TD_SANDBOX_PARKED];
static UWORD *td_sandbox_u,*td_sandbox_v;
static UWORD td_sandbox_seed;
static UBYTE td_sandbox_district,td_sandbox_mask,td_sandbox_owner=TD_NONE,td_sandbox_skin=TD_NONE;
typedef struct {UWORD u,v;BYTE dx,dy;UBYTE timer,kind,recover;} td_driver_t;
static td_driver_t td_sandbox_drivers[2];
static UBYTE td_sandbox_custom_player,td_sandbox_ready;
static UWORD td_sandbox_last_u,td_sandbox_last_v;
static UBYTE td_sandbox_headings[8];
/* Generated from original art's validated half8 curb-bay candidates. */
static const UWORD td_sandbox_curbs[7][8][3]={
{{280,40,0},{408,40,0},{152,152,0},{280,152,0},{152,264,0},{280,264,0},{152,376,0},{280,376,0}},
{{752,88,0},{752,136,0},{320,216,0},{320,264,0},{736,328,0},{320,72,0},{320,120,0},{392,400,4}},
{{880,248,0},{880,296,0},{592,280,0},{592,328,0},{368,328,0},{368,376,0},{904,72,0},{904,120,0}},
{{88,40,0},{88,88,0},{88,264,0},{88,312,0},{768,232,0},{768,280,0},{88,376,0},{88,424,0}},
{{88,104,0},{88,152,0},{800,40,0},{800,88,0},{168,184,4},{168,248,4},{400,264,0},{400,312,0}},
{{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0}},
{{184,848,4},{232,848,4},{312,912,4},{360,912,4},{312,328,4},{360,328,4},{456,624,4},{504,624,4}}
};
static UWORD td_sandbox_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static UBYTE td_sandbox_role(UBYTE slot){return slot<6?slot:slot==7?6:0;}
static UBYTE td_sandbox_class(UBYTE skin){return skin==1||skin==3||skin==5?1:0;}
static UBYTE td_sandbox_in_view(UWORD u,UWORD v){
    WORD left,top;UWORD pu=(td_streetcar_ride_view?td_streetcar_focus_u:td.u)>>4;
    UWORD pv=(td_streetcar_ride_view?td_streetcar_focus_v:td.v)>>4;
    left=pu<80?0:pu>944?864:pu-80;top=pv<88?0:pv>920?832:pv-88;
    return (WORD)(u>>4)+24>=left&&(WORD)(u>>4)-24<=left+159&&
        (WORD)(v>>4)+24>=top&&(WORD)(v>>4)-24<=top+143;
}
static UWORD td_sandbox_random(void){td_sandbox_seed^=td_sandbox_seed<<7;td_sandbox_seed^=td_sandbox_seed>>9;td_sandbox_seed^=td_sandbox_seed<<8;return td_sandbox_seed?td_sandbox_seed:(td_sandbox_seed=0x9d27);}
void td_sandbox_reset(void) BANKED {
    memset(td_sandbox_parked,0,sizeof(td_sandbox_parked));memset(td_sandbox_headings,0,sizeof(td_sandbox_headings));td_sandbox_u=td_sandbox_v=NULL;
    td_sandbox_owner=td_sandbox_skin=TD_NONE;td_sandbox_mask=td_sandbox_custom_player=td_sandbox_ready=0;memset(td_sandbox_drivers,0,sizeof(td_sandbox_drivers));td_sandbox_seed=0x9d27;td_sandbox_last_u=td.u;td_sandbox_last_v=td.v;
}
static UBYTE td_sandbox_parkable(UWORD u,UWORD v){
    UBYTE row,clear=TRUE,hx=tile_hit_x,hy=tile_hit_y;
    if(u<8||v<8||u>1016||v>968)return FALSE;
    /* Parked controllable cars share the accepted player sidewalk mask,
       while the complete half8 footprint still rejects walls and water. */
    for(row=(v-8)>>3;row<=((v+8)>>3);row++)if(tile_col_test_range_x(15,row,(u-8)>>3,(u+8)>>3)){clear=FALSE;break;}
    tile_hit_x=hx;tile_hit_y=hy;return clear;
}
UBYTE td_sandbox_bound(UBYTE district) BANKED {return td_sandbox_ready&&td_sandbox_u&&td_sandbox_v&&td_sandbox_district==district;}
void td_sandbox_bind(UWORD *u,UWORD *v,UBYTE district) BANKED {
    UBYTE i,j,index;UWORD pu,pv;
    if(td_sandbox_bound(district)&&u==td_sandbox_u&&v==td_sandbox_v){td_sandbox_custom_player=0;td_sandbox_last_u=td.u;td_sandbox_last_v=td.v;return;}
    td_sandbox_u=u;td_sandbox_v=v;td_sandbox_district=district;td_sandbox_ready=1;td_sandbox_mask=0;memset(td_sandbox_drivers,0,sizeof(td_sandbox_drivers));td_sandbox_owner=TD_NONE;td_sandbox_last_u=td.u;td_sandbox_last_v=td.v;
    if(district>=7||district==5)return;
    for(i=0;i<2;i++){
        if(td_sandbox_parked[i].protected)continue;
        for(j=0;j<8;j++){
            index=(td_sandbox_random()+j)&7;pu=td_sandbox_curbs[district][index][0]*16;pv=td_sandbox_curbs[district][index][1]*16;
            if(!pu||!pv||!td_sandbox_parkable(pu>>4,pv>>4))continue;
            /* Residents are seeded with a new scene. Later reassignments
             * remain offscreen; protected abandoned vehicles are never replaced. */
            if(td_sandbox_parked[i].active&&td_sandbox_parked[i].district==district&&td_sandbox_in_view(td_sandbox_parked[i].u,td_sandbox_parked[i].v))continue;
            if(i&&td_sandbox_parked[0].active&&td_sandbox_parked[0].district==district&&
                td_sandbox_distance(pu,td_sandbox_parked[0].u)<320&&td_sandbox_distance(pv,td_sandbox_parked[0].v)<320)continue;
            td_sandbox_parked[i].u=pu;td_sandbox_parked[i].v=pv;td_sandbox_parked[i].district=district;
            td_sandbox_parked[i].heading=td_sandbox_curbs[district][index][2];td_sandbox_parked[i].skin=td_sandbox_random()&1;
            td_sandbox_parked[i].vehicle=td_sandbox_parked[i].skin;td_sandbox_parked[i].active=1;break;
        }
    }
}
UBYTE td_sandbox_captured(UBYTE slot) BANKED {return slot<8&&!!(td_sandbox_mask&(1<<slot));}
UBYTE td_sandbox_owned(UBYTE slot) BANKED {return slot<8&&td_sandbox_owner==slot;}
UBYTE td_sandbox_heading(UBYTE slot) BANKED {return slot<8?td_sandbox_headings[slot]:0;}
UBYTE td_sandbox_extracting(void) BANKED {return td_sandbox_drivers[0].timer>60||td_sandbox_drivers[1].timer>60;}
void td_sandbox_sync(void) BANKED {
    UWORD u=td.onfoot?td.park_u:td.u,v=td.onfoot?td.park_v:td.v;
    UBYTE district=td.onfoot?td.park_district:td.district;
    if(td_sandbox_owner<8&&district==td_sandbox_district&&td_sandbox_u){td_sandbox_u[td_sandbox_owner]=u;td_sandbox_v[td_sandbox_owner]=v;td_sandbox_headings[td_sandbox_owner]=td.heading;}
    else if(td_sandbox_owner>=8&&td_sandbox_owner<8+TD_SANDBOX_PARKED){
        td_parked_t *car=&td_sandbox_parked[td_sandbox_owner-8];car->u=u;car->v=v;car->district=district;car->heading=td.heading;car->protected=1;
    }
}
static UBYTE td_sandbox_preserve(void){
    UBYTE i;
    if(td_sandbox_owner!=TD_NONE){td_sandbox_sync();return TRUE;}
    for(i=2;i<TD_SANDBOX_PARKED;i++)if(!td_sandbox_parked[i].active){
        td_parked_t *car=&td_sandbox_parked[i];car->u=td.park_u;car->v=td.park_v;car->district=td.park_district;
        car->vehicle=td.vehicle;car->skin=td_sandbox_skin;car->heading=td.heading;car->active=car->protected=1;return TRUE;
    }
    return FALSE; /* Capacity is explicit: never silently destroy owned cars. */
}
static UBYTE td_sandbox_door_path(UWORD u,UWORD v){
    UBYTE i;WORD du=(WORD)u-td.u,dv=(WORD)v-td.v;
    for(i=0;i<=32;i++)if(!td_road_walkable((td.u+du*(WORD)i/32)>>4,(td.v+dv*(WORD)i/32)>>4))return FALSE;
    return TRUE;
}
UBYTE td_sandbox_interact(void) BANKED {
    UBYTE i,best=TD_NONE,skin,vehicle,heading,driver=TD_NONE;UWORD u=0,v=0,score,min=48*16;
    if(!td.onfoot||td.mode!=TD_ROAM||td_entry_timer||td_sandbox_extracting())return 0;
    if(td.park_district==td.district){
        score=td_sandbox_distance(td.u,td.park_u)+td_sandbox_distance(td.v,td.park_v);
        if(score<min&&td_sandbox_distance(td.u,td.park_u)<26*16&&td_sandbox_distance(td.v,td.park_v)<26*16){min=score;best=254;}
    }
    if(td.district==td_sandbox_district&&td_sandbox_u&&td.district!=5)for(i=0;i<8;i++){
        if(td_sandbox_owner==i)continue;
        score=td_sandbox_distance(td.u,td_sandbox_u[i])+td_sandbox_distance(td.v,td_sandbox_v[i]);
        if(score<min&&td_sandbox_distance(td.u,td_sandbox_u[i])<26*16&&td_sandbox_distance(td.v,td_sandbox_v[i])<26*16){min=score;best=i;}
    }
    for(i=0;i<TD_SANDBOX_PARKED;i++)if(td_sandbox_parked[i].active&&td_sandbox_parked[i].district==td.district&&td_sandbox_owner!=i+8){
        score=td_sandbox_distance(td.u,td_sandbox_parked[i].u)+td_sandbox_distance(td.v,td_sandbox_parked[i].v);
        if(score<min&&td_sandbox_distance(td.u,td_sandbox_parked[i].u)<26*16&&td_sandbox_distance(td.v,td_sandbox_parked[i].v)<26*16){min=score;best=i+8;}
    }
    if(best==TD_NONE)return 0;if(best==254)return 2;
    if(best<8){u=td_sandbox_u[best];v=td_sandbox_v[best];skin=td_sandbox_role(best);vehicle=td_sandbox_class(skin);
        heading=td_sandbox_captured(best)?td_sandbox_headings[best]:(actors[best<6?best+2:best+11].frame&(skin<2?7:3))*(skin<2?2:4);
    }else{td_parked_t *car=&td_sandbox_parked[best-8];u=car->u;v=car->v;skin=car->skin;vehicle=car->vehicle;heading=car->heading;}
    if(best<8&&!td_sandbox_captured(best)){
        for(i=0;i<2;i++)if(!td_sandbox_drivers[i].timer||!td_sandbox_in_view(td_sandbox_drivers[i].u,td_sandbox_drivers[i].v)){driver=i;break;}
        if(driver==TD_NONE)return 3;
    }
    if(!td_road_walkable(td.u>>4,td.v>>4)||!td_sandbox_door_path(u,v))return 4;
    if(!td_sandbox_preserve())return 3;
    td_sandbox_owner=best;td_sandbox_skin=skin;
    if(best<8){td_sandbox_mask|=1<<best;td_sandbox_headings[best]=heading;
        if(driver!=TD_NONE){td_driver_t *person=&td_sandbox_drivers[driver];person->timer=72;person->kind=skin==1||skin==3?1:0;
            person->u=u;person->v=v;person->recover=0;person->dx=td.u>=u?1:-1;person->dy=0;}
    }
    td.vehicle=vehicle;td.heading=heading;td.park_u=u;td.park_v=v;td.park_district=td.district;
    td.speed=0;td_vx=td_vy=0;td_entry_target=0;td_entry_timer=12;
    td.wanted=td.wanted<3?td.wanted+1:3;td.wanted_left=30;td_set_target();td_save();return 1;
}
static UBYTE td_sandbox_driver_land(UWORD u,UWORD v){
    UWORD x=u>>4,y=v>>4;
    return x>=3&&y>=3&&x<=1020&&y<=972&&td_road_walkable(x-3,y-3)&&td_road_walkable(x+3,y-3)&&
        td_road_walkable(x-3,y+3)&&td_road_walkable(x+3,y+3);
}
UBYTE td_sandbox_human_hits(void) BANKED {
    UBYTE i,n,steps,hits=0;WORD du=(WORD)td.u-td_sandbox_last_u,dv=(WORD)td.v-td_sandbox_last_v,x,y;
    UWORD span=du<0?-du:du,other=dv<0?-dv:dv;
    if(other>span)span=other;
    if(td.mode==TD_ROAM&&!td.onfoot&&(td.speed>2||td.speed<-2)&&span&&span<=512){
        steps=(span+15)/16;
        for(i=0;i<2;i++)if(td_sandbox_drivers[i].timer&&td_sandbox_drivers[i].timer<=60&&
            !(td_sandbox_drivers[i].kind&128)&&!(actors[19+i].flags&ACTOR_FLAG_HIDDEN)){
            td_driver_t *person=&td_sandbox_drivers[i];
            for(n=0;n<=steps;n++){
                x=td_sandbox_last_u+du*(WORD)n/steps;y=td_sandbox_last_v+dv*(WORD)n/steps;
                if(td_sandbox_distance(x,person->u)<160&&td_sandbox_distance(y,person->v)<160){
                    person->kind|=128;person->recover=0;person->dx=du>0?1:du<0?-1:0;person->dy=dv>0?1:dv<0?-1:0;hits++;break;
                }
            }
        }
    }
    td_sandbox_last_u=td.u;td_sandbox_last_v=td.v;return hits;
}
void td_sandbox_tick(void) BANKED {
    UBYTE i,attempt;UWORD u,v;BYTE dx,dy;
    for(i=0;i<2;i++)if(td_sandbox_drivers[i].timer){
        td_driver_t *person=&td_sandbox_drivers[i];
        if(person->timer<=60&&!td_sandbox_in_view(person->u,person->v)){person->timer=0;continue;}
        if(person->kind&128){
            if(person->recover<24){person->recover++;if(!(person->recover&1)){
                u=person->u+person->dx*16;v=person->v+person->dy*16;
                if(td_sandbox_driver_land(u,v)){person->u=u;person->v=v;}
            }}continue;
        }
        for(attempt=0;attempt<4;attempt++){
            u=person->u+person->dx*16;v=person->v+person->dy*16;
            /* Initial drag must leave the occupied car body; subsequent
               walking respects every parked/traffic body and solid land. */
            if(td_sandbox_driver_land(u,v)&&(person->timer>60||td_sandbox_clear(person->u,person->v,u,v,3,254))){person->u=u;person->v=v;break;}
            dx=person->dx;dy=person->dy;person->dx=-dy;person->dy=dx;
        }
        if(person->timer>60)person->timer--;
    }
}
void td_sandbox_prepare(void) BANKED {
    if(td_sandbox_custom_player){td_player_sprite_restore();td_sandbox_custom_player=0;}
}
void td_sandbox_present(void) BANKED {
    UBYTE i;
    if(td.onfoot&&td_sandbox_custom_player){td_player_sprite_restore();td_sandbox_custom_player=0;}
    if(!td.onfoot&&td_sandbox_skin<7&&td.vehicle==td_sandbox_class(td_sandbox_skin)){
        td_fleet_present(&PLAYER,td_sandbox_skin,((td.heading+1)&15)/4);td_sandbox_custom_player=1;
    }
    if(td.onfoot){
        if(td_sandbox_skin<7&&td.vehicle==td_sandbox_class(td_sandbox_skin))td_fleet_present(&actors[8],td_sandbox_skin,((td.heading+1)&15)/4);
        else td_vehicle_present(&actors[8],td.vehicle,td.heading);
    }
    for(i=0;i<2;i++){
        actor_t *driver=&actors[19+i];td_driver_t *person=&td_sandbox_drivers[i];
        if(person->timer){
            UBYTE pose,lift=0;
            driver->flags&=~ACTOR_FLAG_HIDDEN;driver->pos.x=person->u*2;
            if(person->kind&128){pose=person->recover<24?TD_CIVILIAN_HIT:TD_CIVILIAN_PRONE;
                lift=person->recover<12?person->recover/3:person->recover<24?(24-person->recover)/3:0;}
            else pose=person->timer>60?TD_CIVILIAN_HIT:((person->dx>=0?0:2)|((person->u>>4)&1));
            driver->pos.y=(person->v-lift*16)*2;td_civilian_present(driver,person->kind&127,pose);
        }else driver->flags|=ACTOR_FLAG_HIDDEN;
    }
}
static UBYTE td_sandbox_body_clear(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UWORD cu,UWORD cv,UBYTE radius){
    UWORD r=radius*16,lo,hi;UBYTE old=td_sandbox_distance(old_u,cu)<r&&td_sandbox_distance(old_v,cv)<r;
    if(old){
        if((u>old_u&&old_u<cu)||(u<old_u&&old_u>cu)||(v>old_v&&old_v<cv)||(v<old_v&&old_v>cv))return FALSE;
        return td_sandbox_distance(u,cu)+td_sandbox_distance(v,cv)>td_sandbox_distance(old_u,cu)+td_sandbox_distance(old_v,cv);
    }
    lo=old_u<u?old_u:u;hi=old_u>u?old_u:u;if((cu<lo&&lo-cu>=r)||(cu>hi&&cu-hi>=r))return TRUE;
    lo=old_v<v?old_v:v;hi=old_v>v?old_v:v;return (cv<lo&&lo-cv>=r)||(cv>hi&&cv-hi>=r);
}
/* The hull includes both endpoints, so a body outside it cannot take the
   existing-overlap branch. Reject here before SDCC pushes the full helper
   argument frame. Ordered differences retain the helper's strict edges and
   do not wrap near the map boundary. Arguments are side-effect-free. */
#define td_sandbox_outside(cu,cv,r) (((cu)<lu&&lu-(cu)>=(r))||\
    ((cu)>hu&&(cu)-hu>=(r))||((cv)<lv&&lv-(cv)>=(r))||\
    ((cv)>hv&&(cv)-hv>=(r)))
/* A bucket spans16 pixels and every combined body radius is <=15 pixels.
   A two-bucket gap is therefore at least257Q4 apart, even at opposite bucket
   edges. Keep the exact word/hull test for every closer body. Ordered byte
   differences remain safe for arbitrary long or wrapping input endpoints. */
#define td_sandbox_bucket_outside(cu,cv) (((cu)<bu0&&(UBYTE)(bu0-(cu))>1)||\
    ((cu)>bu1&&(UBYTE)((cu)-bu1)>1)||((cv)<bv0&&(UBYTE)(bv0-(cv))>1)||\
    ((cv)>bv1&&(UBYTE)((cv)-bv1)>1))
UBYTE td_sandbox_clear(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half,UBYTE skip) BANKED {
    UBYTE i,body,bu0,bu1,bv0,bv1,bu,bv;UWORD lu,hu,lv,hv,radius,cu,cv;
    if(!half||half>8||u>=1024*16||v>=976*16)return FALSE;
    lu=old_u<u?old_u:u;hu=old_u>u?old_u:u;lv=old_v<v?old_v:v;hv=old_v>v?old_v:v;
    bu0=lu>>8;bu1=hu>>8;bv0=lv>>8;bv1=hv>>8;
    if(skip>=8&&td_streetcar_view_district==td_sandbox_district&&td_sandbox_u&&td_streetcar_view_district!=5)for(i=0;i<8;i++){
        if(i==skip||(skip==TD_NONE&&i==td_sandbox_owner))continue;
        cu=td_sandbox_u[i];cv=td_sandbox_v[i];bu=cu>>8;bv=cv>>8;
        if(td_sandbox_bucket_outside(bu,bv))continue;
        body=i==1||i==4?6:i==3||i==5?7:5;radius=(half+body)*16;
        if(!td_sandbox_outside(cu,cv,radius)&&
           !td_sandbox_body_clear(old_u,old_v,u,v,cu,cv,half+body))return FALSE;
    }
    for(i=0;i<TD_SANDBOX_PARKED;i++)if(td_sandbox_parked[i].active&&td_sandbox_parked[i].district==td_streetcar_view_district){
        if(skip==TD_NONE&&td_sandbox_owner==i+8)continue;
        cu=td_sandbox_parked[i].u;cv=td_sandbox_parked[i].v;bu=cu>>8;bv=cv>>8;
        if(td_sandbox_bucket_outside(bu,bv))continue;
        radius=(half+7)*16;
        if(!td_sandbox_outside(cu,cv,radius)&&
           !td_sandbox_body_clear(old_u,old_v,u,v,cu,cv,half+7))return FALSE;
    }
    if(skip<8)for(i=0;i<2;i++)if(td_sandbox_drivers[i].timer&&!(td_sandbox_drivers[i].kind&128)){
        cu=td_sandbox_drivers[i].u;cv=td_sandbox_drivers[i].v;bu=cu>>8;bv=cv>>8;
        if(td_sandbox_bucket_outside(bu,bv))continue;
        radius=(half+3)*16;
        if(!td_sandbox_outside(cu,cv,radius)&&
           !td_sandbox_body_clear(old_u,old_v,u,v,cu,cv,half+3))return FALSE;
    }
    return TRUE;
}
UBYTE td_sandbox_foot_clear(UWORD u,UWORD v) BANKED {
    UBYTE i;
    /* Stop/door/alighting probes are occupancy queries, not motion sweeps
       from the courier in another place or district. Preserve10.5px clearance. */
    if(td_streetcar_view_district==td_sandbox_district&&td_sandbox_u&&td_streetcar_view_district!=5)for(i=0;i<8;i++)
        if(td_sandbox_distance(u,td_sandbox_u[i])<168&&td_sandbox_distance(v,td_sandbox_v[i])<168)return FALSE;
    for(i=0;i<TD_SANDBOX_PARKED;i++)if(td_sandbox_parked[i].active&&td_sandbox_parked[i].district==td_streetcar_view_district&&
        td_sandbox_distance(u,td_sandbox_parked[i].u)<168&&td_sandbox_distance(v,td_sandbox_parked[i].v)<168)return FALSE;
    return TRUE;
}
void td_sandbox_render(void) BANKED {
    UBYTE i;WORD x,y;actor_t car;
    if(td_district_current()!=td_sandbox_district||td_streetcar_view_district!=td_sandbox_district||
       (td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE))return;
    for(i=0;i<TD_SANDBOX_PARKED;i++)if(td_sandbox_parked[i].active&&td_sandbox_parked[i].district==td_streetcar_view_district&&td_sandbox_owner!=i+8){
        /* Both original16x16 vehicle sheets use two8x16 objects at cumulative
           (0,0)/(8,0). Their complete screen pixels span[x-8,x+7] and
           [y-16,y-1]. Cull before copying an actor or reading sprite poses. */
        x=(WORD)(td_sandbox_parked[i].u>>4)-draw_scroll_x;
        y=(WORD)(td_sandbox_parked[i].v>>4)-draw_scroll_y;
        if(x<=-8||x>=168||y<=0||y>=160)continue;
        car=PLAYER;car.pos.x=td_sandbox_parked[i].u*2;car.pos.y=td_sandbox_parked[i].v*2;car.flags=0;
        if(td_sandbox_parked[i].skin<7)td_fleet_present(&car,td_sandbox_parked[i].skin,td_sandbox_parked[i].heading/4);
        else td_vehicle_present(&car,td_sandbox_parked[i].vehicle,td_sandbox_parked[i].heading);
        td_actor_render_actor(&car);
    }
}
