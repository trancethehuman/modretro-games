#pragma bank 255
#include <string.h>
#include <stddef.h>
#include "td_scooter.h"
#include "td_game.h"
#include "td_sandbox.h"
#include "td_traffic.h"
#include "td_ramming.h"
#include "td_motion.h"
#include "td_city_sprites.h"
#include "td_actor_render.h"
#include "td_district.h"
#include "td_scenery.h"
#include "td_boats.h"
#include "td_streetcar_runtime.h"
#include "collision.h"
#include "scroll.h"

typedef struct {UWORD u,v;UBYTE leg,hit,flags;} td_rider_t;
typedef struct {UBYTE left,right,top,bottom,clear,valid;} td_scooter_terrain_cache_t;
static td_rider_t td_scooter_riders[2];
static UBYTE td_scooter_district=TD_NONE,td_scooter_elapsed;
/* SDCC has byte-aligned words. The host's optional structure tail padding
 * is adapter ABI, not mutable GBVM allocation. Check every native field. */
typedef char td_scooter_fields[(offsetof(td_rider_t,flags)==6&&sizeof(UWORD)==2&&sizeof(UBYTE)==1)?1:-1];
#ifdef __SDCC
typedef char td_scooter_budget[(sizeof(td_scooter_riders)+sizeof(td_scooter_district)+sizeof(td_scooter_elapsed)==16)?1:-1];
#endif
#define TD_SCOOTER_ACTIVE 1
#define TD_SCOOTER_WRECK 2
/* Original compressed loops: eastbound road lane, bounded curb connector,
 * westbound sidewalk leg, return connector. Every half7 pixel is audited by
 * check_scooters.py; these do not alter real city roads or transit routes. */
static const UWORD td_scooter_routes[7][2][4][2]={
 {{{488,648},{696,648},{696,664},{488,664}},{{488,728},{696,728},{696,744},{488,744}}},
 {{{568,40},{776,40},{776,24},{568,24}},{{344,144},{552,144},{552,128},{344,128}}},
 {{{376,72},{584,72},{584,88},{376,88}},{{200,192},{408,192},{408,176},{200,176}}},
 {{{32,40},{240,40},{240,24},{32,24}},{{192,152},{400,152},{400,136},{192,136}}},
 {{{32,40},{240,40},{240,24},{32,24}},{{488,144},{696,144},{696,128},{488,128}}},
 {{{0,0},{0,0},{0,0},{0,0}},{{0,0},{0,0},{0,0},{0,0}}},
 {{{32,120},{240,120},{240,136},{32,136}},{{576,248},{784,248},{784,264},{576,264}}}
};
static const BYTE td_scooter_dx[4]={1,0,-1,0},td_scooter_dy[4]={0,1,0,-1};
static const UBYTE td_scooter_fleet_extents[8]={5,6,5,7,6,7,5,5};
static UWORD td_scooter_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static UBYTE td_scooter_active(void){
    return td_scooter_district<7&&td_scooter_district!=5&&
        td_streetcar_view_district==td_scooter_district&&td_district_current()==td_scooter_district&&
        (td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE);
}
static UBYTE td_scooter_view(UWORD u,UWORD v){
    WORD left,top;UWORD pu=(td_streetcar_ride_view?td_streetcar_focus_u:td.u)>>4;
    UWORD pv=(td_streetcar_ride_view?td_streetcar_focus_v:td.v)>>4;
    left=pu<80?0:pu>944?864:pu-80;top=pv<88?0:pv>920?832:pv-88;
    /* Whole mounted/airborne pose plus24px camera guard, not centre-only. */
    return (WORD)(u>>4)+24>=left&&(WORD)(u>>4)-24<=left+159&&
        (WORD)(v>>4)+24>=top&&(WORD)(v>>4)-24<=top+143;
}
static UBYTE td_scooter_terrain(UWORD ou,UWORD ov,UWORD u,UWORD v,UBYTE half,td_scooter_terrain_cache_t *cache){
    UBYTE row,left,right,top,bottom,hx=tile_hit_x,hy=tile_hit_y,clear=TRUE;
    if(!half||half>8||ou<half*16||u<half*16||ov<half*16||v<half*16||
       ou>(1024-half)*16||u>(1024-half)*16||ov>(976-half)*16||v>(976-half)*16||
       (ou!=u&&ov!=v))return FALSE;
    left=((ou<u?ou:u)-half*16)>>7;right=((ou>u?ou:u)+half*16)>>7;
    top=((ov<v?ov:v)-half*16)>>7;bottom=((ov>v?ov:v)+half*16)>>7;
    /* This local result is valid only during one synchronous rider update:
     * its registered collision page and mask15 cannot change. Bounds, half
     * extent and cardinal geometry above are checked on EVERY proposed step.
     * Identical tile unions have identical terrain results; cached hits make
     * no collision call and preserve exactly the current tile_hit globals. */
    if(cache&&cache->valid&&cache->left==left&&cache->right==right&&cache->top==top&&cache->bottom==bottom)return cache->clear;
    for(row=top;row<=bottom;row++)if(tile_col_test_range_x(15,row,left,right)){clear=FALSE;break;}
    tile_hit_x=hx;tile_hit_y=hy;
    if(cache){cache->left=left;cache->right=right;cache->top=top;cache->bottom=bottom;cache->clear=clear;cache->valid=1;}
    return clear;
}
static UBYTE td_scooter_body_clear(UWORD ou,UWORD ov,UWORD u,UWORD v,UWORD cu,UWORD cv,UBYTE radius){
    UWORD r=radius*16,lo=ou<u?ou:u,hi=ou>u?ou:u;
    if(td_scooter_distance(ou,cu)<r&&td_scooter_distance(ov,cv)<r){
        if((u>ou&&ou<cu)||(u<ou&&ou>cu)||(v>ov&&ov<cv)||(v<ov&&ov>cv))return FALSE;
        return td_scooter_distance(u,cu)+td_scooter_distance(v,cv)>
            td_scooter_distance(ou,cu)+td_scooter_distance(ov,cv);
    }
    if((cu<lo&&lo-cu>=r)||(cu>hi&&cu-hi>=r))return TRUE;
    lo=ov<v?ov:v;hi=ov>v?ov:v;
    return (cv<lo&&lo-cv>=r)||(cv>hi&&cv-hi>=r);
}
UBYTE td_scooter_clear(UWORD ou,UWORD ov,UWORD u,UWORD v,UBYTE half,UBYTE skip) BANKED {
    UBYTE i;td_rider_t *rider;
    if(td_streetcar_view_district!=td_scooter_district)return TRUE;
    /* With no new bodies this is an identity extension of the legacy
     * sandbox hull query, including its diagnostic old-endpoint domain. */
    if(!(td_scooter_riders[0].flags&TD_SCOOTER_ACTIVE)&&!(td_scooter_riders[1].flags&TD_SCOOTER_ACTIVE))return TRUE;
    if(!half||half>8||ou>=1024*16||u>=1024*16||ov>=976*16||v>=976*16)return FALSE;
    for(i=0;i<2;i++){
        rider=&td_scooter_riders[i];if(!(rider->flags&TD_SCOOTER_ACTIVE)||skip==240+i)continue;
        if(!td_scooter_body_clear(ou,ov,u,v,rider->u,rider->v,half+((rider->flags&TD_SCOOTER_WRECK)?2:7)))return FALSE;
    }
    return TRUE;
}
UBYTE td_scooter_foot_clear(UWORD u,UWORD v) BANKED {
    UBYTE i;td_rider_t *rider;
    if(u>=1024*16||v>=976*16)return FALSE;
    if(td_streetcar_view_district!=td_scooter_district)return TRUE;
    for(i=0;i<2;i++){
        rider=&td_scooter_riders[i];if(!(rider->flags&TD_SCOOTER_ACTIVE))continue;
        if(td_scooter_distance(u,rider->u)<((rider->flags&TD_SCOOTER_WRECK)?80:168)&&
           td_scooter_distance(v,rider->v)<((rider->flags&TD_SCOOTER_WRECK)?80:168))return FALSE;
    }
    return TRUE;
}
static UBYTE td_scooter_courier_clear(UWORD ou,UWORD ov,UWORD u,UWORD v,UBYTE half){
    if(td_streetcar_ride_view||td.mode==TD_RIDE)return TRUE;
    if(td.district==td_scooter_district&&
       !td_scooter_body_clear(ou,ov,u,v,td.u,td.v,half+(td.onfoot?3:7)))return FALSE;
    if(td.onfoot&&td.park_district==td_scooter_district&&
       !td_scooter_body_clear(ou,ov,u,v,td.park_u,td.park_v,half+7))return FALSE;
    return TRUE;
}
static UBYTE td_scooter_move(UBYTE index,UWORD u,UWORD v,UBYTE half,UBYTE signals,const td_rider_batch_t *batch,td_scooter_terrain_cache_t *terrain){
    td_rider_t *rider=&td_scooter_riders[index];UBYTE i;
    if(signals&&!td_traffic_external_batch_admit(batch,td_scooter_district,td.seconds,rider->u,rider->v,u,v,half))return FALSE;
    if(!td_scooter_terrain(rider->u,rider->v,u,v,half,terrain)||
       !td_sandbox_clear(rider->u,rider->v,u,v,half,240+index)||
       !td_scooter_courier_clear(rider->u,rider->v,u,v,half)||
       !td_streetcar_runtime_traffic_clear_extent(td_scooter_district,u,v,half<5?5:half))return FALSE;
    /* Pushed wrecks still yield to all live people; no new pedestrian slot. */
    if(!signals)for(i=0;i<8;i++)if(!(actors[9+i].flags&ACTOR_FLAG_HIDDEN)&&
       !td_scooter_body_clear(rider->u,rider->v,u,v,actors[9+i].pos.x>>1,actors[9+i].pos.y>>1,half+3))return FALSE;
    if(td_scenery_contact(rider->u,rider->v,u,v,half,1)!=TD_SCENERY_CLEAR)return FALSE;
    rider->u=u;rider->v=v;return TRUE;
}
void td_scooter_reset(void) BANKED {
    memset(td_scooter_riders,0,sizeof(td_scooter_riders));td_scooter_district=TD_NONE;td_scooter_elapsed=0;
}
void td_scooter_bind(UBYTE district) BANKED {
    if(district==td_scooter_district)return;
    memset(td_scooter_riders,0,sizeof(td_scooter_riders));td_scooter_elapsed=0;td_scooter_district=district;
}
static UBYTE td_scooter_admit(UBYTE index,const td_rider_batch_t *batch,td_scooter_terrain_cache_t *terrain){
    UBYTE i;UWORD u,v;td_rider_t *rider=&td_scooter_riders[index];
    for(i=0;i<4;i++){
        u=td_scooter_routes[td_scooter_district][index][i][0]*16;
        v=td_scooter_routes[td_scooter_district][index][i][1]*16;
        if(td_scooter_view(u,v))continue;
        rider->u=u;rider->v=v;
        if(!td_scooter_move(index,u,v,7,TRUE,batch,terrain))continue;
        rider->leg=(i+1)&3;rider->flags=TD_SCOOTER_ACTIVE;rider->hit=0;return TRUE;
    }
    return FALSE;
}
UBYTE td_scooter_update(UWORD elapsed) BANKED {
    UBYTE i,budget,phase,dir,amount,dirty=FALSE;UWORD u,v,tu,tv;
    UBYTE extents[8];td_traffic_context_t ctx;td_rider_batch_t batch;
    td_scooter_terrain_cache_t terrain[2];
    td_rider_t *rider;
    if(!td_scooter_active()||!elapsed)return FALSE;
    /* Preserve pending contact count in the high nibble. Capped8 motion
     * quanta avoid an unbounded catch-up loop after a slow native frame. */
    if(elapsed>64)elapsed=64;budget=((td_scooter_elapsed&7)+elapsed)/8;
    td_scooter_elapsed=(td_scooter_elapsed&0xF0)|(((td_scooter_elapsed&7)+elapsed)&7);
    if(!budget&&(td_scooter_riders[0].flags&TD_SCOOTER_ACTIVE)&&
       (td_scooter_riders[1].flags&TD_SCOOTER_ACTIVE))return FALSE;
    terrain[0].valid=terrain[1].valid=0;
    memcpy(extents,td_scooter_fleet_extents,sizeof(extents));
    /* Fleet, people, their extents, park and priority do not change inside
     * this update. Validate that immutable geometry once for all half-pixel
     * rider quanta. Wrecks, vacant scooters, scenery, courier and other rider
     * bodies still use their existing LIVE full-sweep checks on every move.
     * Failed begin leaves the batch invalid: riders stay blocked while wreck
     * clocks and vacant impulses keep their original independent behavior. */
    ctx.u=td_traffic_u;ctx.v=td_traffic_v;ctx.peds=&actors[9];ctx.half_u=ctx.half_v=extents;
    ctx.park_u=td.park_u;ctx.park_v=td.park_v;ctx.parked_active=td.onfoot&&td.park_district==td_scooter_district;
    ctx.priority_mask=td.wanted?4:0;td_traffic_external_begin(&ctx,&batch);
    for(i=0;i<2;i++)if(!(td_scooter_riders[i].flags&TD_SCOOTER_ACTIVE))dirty|=td_scooter_admit(i,&batch,&terrain[i]);
    while(budget--){
      dirty|=td_sandbox_scooter_step();
      for(i=0;i<2;i++){
        rider=&td_scooter_riders[i];if(!(rider->flags&TD_SCOOTER_ACTIVE))continue;
        if(rider->flags&TD_SCOOTER_WRECK){
            if(rider->hit<96){
                if(rider->hit<24){dir=(rider->flags>>2)&3;amount=(rider->flags>>4)*2;
                    u=rider->u+td_scooter_dx[dir]*amount;v=rider->v+td_scooter_dy[dir]*amount;
                    if(amount&&td_scooter_move(i,u,v,2,FALSE,NULL,&terrain[i]))dirty=TRUE;
                }rider->hit+=8;
            }else if(!td_scooter_view(rider->u,rider->v)){rider->flags=0;dirty=TRUE;}
            continue;
        }
        phase=rider->leg;tu=td_scooter_routes[td_scooter_district][i][phase][0]*16;
        tv=td_scooter_routes[td_scooter_district][i][phase][1]*16;u=rider->u;v=rider->v;
        if(u!=tu){amount=td_scooter_distance(u,tu)<8?td_scooter_distance(u,tu):8;u+=u<tu?amount:-(WORD)amount;}
        else if(v!=tv){amount=td_scooter_distance(v,tv)<8?td_scooter_distance(v,tv):8;v+=v<tv?amount:-(WORD)amount;}
        else{rider->leg=(phase+1)&3;continue;}
        if(td_scooter_move(i,u,v,7,TRUE,&batch,&terrain[i])){dirty=TRUE;if(u==tu&&v==tv)rider->leg=(phase+1)&3;}
      }
    }
    return dirty;
}
UBYTE td_scooter_ram(UWORD ou,UWORD ov,UWORD u,UWORD v) BANKED {
    UBYTE i,dir,pm,keep,impulse,amount,hit=FALSE;WORD du=(WORD)u-ou,dv=(WORD)v-ov,npc;
    UWORD pu,pv;BYTE speed=td.speed<0?-td.speed:td.speed;td_rider_t *rider;
    if(!td_scooter_active()||td.mode!=TD_ROAM||td.onfoot||td_boats_controlled()||speed<3||td_motion_rebounding()||
       (!du&&!dv)||du>16||du< -16||dv>16||dv< -16||(du&&dv))return FALSE;
    dir=du?du>0?0:2:dv>0?1:3;amount=du?du<0?-du:du:dv<0?-dv:dv;
    for(i=0;i<2;i++){
        rider=&td_scooter_riders[i];if(!(rider->flags&TD_SCOOTER_ACTIVE)||
           td_scooter_body_clear(ou,ov,u,v,rider->u,rider->v,7+((rider->flags&TD_SCOOTER_WRECK)?2:7)))continue;
        if(rider->flags&TD_SCOOTER_WRECK)continue;
        npc=dir==0||dir==2?(WORD)td_scooter_routes[td_scooter_district][i][rider->leg][0]*16-rider->u:
            (WORD)td_scooter_routes[td_scooter_district][i][rider->leg][1]*16-rider->v;
        npc=npc>0?1:npc<0?-1:0;if(dir>=2)npc=-npc;
        pm=td.vehicle==1?7:td.vehicle>=2?2:4;impulse=((speed-npc)*pm)/(pm+1);
        impulse=(impulse+1)/2;if(impulse<1)impulse=1;if(impulse>7)impulse=7;
        pu=rider->u+td_scooter_dx[dir]*amount;pv=rider->v+td_scooter_dy[dir]*amount;
        /* The first checked tiny shove admits the grounded bike body; if
         * a wall/fleet/person blocks it, no wreck or transfer is invented. */
        if(!td_scooter_move(i,pu,pv,2,FALSE,NULL,NULL))continue;
        rider->flags=TD_SCOOTER_ACTIVE|TD_SCOOTER_WRECK|(dir<<2)|(impulse<<4);rider->hit=0;
        if((td_scooter_elapsed>>4)<2)td_scooter_elapsed+=16;
        keep=(speed*pm+(npc>0?npc:0))/(pm+1);if(!keep)keep=1;td_motion_transfer(keep);hit=TRUE;
    }
    if(td_sandbox_scooter_ram(ou,ov,u,v))hit=TRUE;
    return hit&&td_sandbox_clear(ou,ov,u,v,7,TD_NONE);
}
UBYTE td_scooter_take_hits(void) BANKED {
    UBYTE hits=td_scooter_elapsed>>4;td_scooter_elapsed&=7;return hits;
}
void td_scooter_render(void) BANKED {
    UBYTE i,dir,lift,age;WORD x,y;actor_t actor;td_rider_t *rider;
    if(!td_scooter_active())return;
    for(i=0;i<2;i++){
        rider=&td_scooter_riders[i];if(!(rider->flags&TD_SCOOTER_ACTIVE))continue;
        x=(WORD)(rider->u>>4)-draw_scroll_x;y=(WORD)(rider->v>>4)-draw_scroll_y;
        if(x<=-16||x>=176||y<=-8||y>=168)continue;
        actor=PLAYER;actor.flags=0;actor.pos.x=rider->u*2;actor.pos.y=rider->v*2;
        if(rider->flags&TD_SCOOTER_WRECK){
            dir=(rider->flags>>2)&3;td_scooter_parked_present(&actor,dir*4);td_actor_render_actor(&actor);
            age=rider->hit;lift=age<48?age/12:age<96?(96-age)/12:0;
            actor.pos.x=(rider->u+td_scooter_dx[dir]*96)*2;
            actor.pos.y=(rider->v+td_scooter_dy[dir]*96-lift*16)*2;
            td_civilian_present(&actor,i+1,age<96?TD_CIVILIAN_HIT:TD_CIVILIAN_PRONE);td_actor_render_actor(&actor);
        }else{
            dir=rider->u< td_scooter_routes[td_scooter_district][i][rider->leg][0]*16?0:
                rider->u>td_scooter_routes[td_scooter_district][i][rider->leg][0]*16?2:
                rider->v<td_scooter_routes[td_scooter_district][i][rider->leg][1]*16?1:3;
            td_vehicle_present(&actor,3,dir*4);td_actor_render_actor(&actor);
        }
    }
}
