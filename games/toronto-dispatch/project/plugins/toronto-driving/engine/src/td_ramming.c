#pragma bank 255
#include <string.h>
#include "td_ramming.h"
#include "td_game.h"
#include "td_motion.h"
#include "td_traffic.h"
#include "td_sandbox.h"
#include "td_scenery.h"
#include "td_boats.h"
#include "td_streetcar_runtime.h"
#include "collision.h"

static UWORD td_ram_anchor_u[8],td_ram_anchor_v[8];
/* Low2 direction, next3 half-Q4 impulse units, high3 remaining push ticks.
   A nonzero impulse with zero ticks means visible route recovery. */
static UBYTE td_ram_state[8];
typedef char td_ram_budget[(sizeof(td_ram_anchor_u)+sizeof(td_ram_anchor_v)+sizeof(td_ram_state)==40)?1:-1];
static const BYTE td_ram_dx[4]={1,0,-1,0},td_ram_dy[4]={0,1,0,-1};
static UWORD td_ram_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static UBYTE td_ram_terrain(UWORD ou,UWORD ov,UWORD u,UWORD v,UBYTE half){
    UBYTE row,left,right,top,bottom,hx=tile_hit_x,hy=tile_hit_y,result=TRUE;
    if(ou<half*16||u<half*16||ov<half*16||v<half*16||
       ou>(1024-half)*16||u>(1024-half)*16||ov>(976-half)*16||v>(976-half)*16)return FALSE;
    left=(((ou<u?ou:u)>>4)-half)>>3;right=(((ou>u?ou:u)>>4)+half)>>3;
    top=(((ov<v?ov:v)>>4)-half)>>3;bottom=(((ov>v?ov:v)>>4)+half)>>3;
    for(row=top;row<=bottom;row++)if(tile_col_test_range_x(15,row,left,right)){result=FALSE;break;}
    tile_hit_x=hx;tile_hit_y=hy;return result;
}
static UBYTE td_ram_move_inner(UBYTE slot,UWORD u,UWORD v,BYTE speed,td_traffic_epoch_t *epoch){
    UBYTE extents[8]={5,6,5,7,6,7,5,5};UWORD ou=td_traffic_u[slot],ov=td_traffic_v[slot];
    td_traffic_context_t context;
    context.u=td_traffic_u;context.v=td_traffic_v;context.peds=&actors[9];context.half_u=context.half_v=extents;
    context.park_u=td.onfoot?td.park_u:td.u;context.park_v=td.onfoot?td.park_v:td.v;
    context.parked_active=td.onfoot?td.park_district==td_streetcar_view_district:td.district==td_streetcar_view_district;
    context.priority_mask=0;
    if(!td_traffic_epoch_begin(&context,td_streetcar_view_district,td.seconds,epoch)||
       !td_traffic_epoch_admit(epoch,slot,ou,ov,u,v,1)||
       !td_ram_terrain(ou,ov,u,v,extents[slot])||
       !td_sandbox_clear(ou,ov,u,v,extents[slot],slot))return FALSE;
    /* A visible courier is a real body; boat/TTC passengers are excluded. */
    if(td.onfoot&&!td_boats_controlled()&&!td_streetcar_ride_view&&(td.mode==TD_ROAM||td.mode==TD_WAIT)){
        UWORD radius=(extents[slot]+3)*16,lu=ou<u?ou:u,hu=ou>u?ou:u,lv=ov<v?ov:v,hv=ov>v?ov:v;
        if(!((td.u<lu&&lu-td.u>=radius)||(td.u>hu&&td.u-hu>=radius)||
             (td.v<lv&&lv-td.v>=radius)||(td.v>hv&&td.v-hv>=radius)))return FALSE;
    }
    /* Every at-most1px body move retains a half-pixel future-tram probe. */
    if(!td_streetcar_runtime_traffic_clear_extent(td_streetcar_view_district,(ou+u)/2,(ov+v)/2,extents[slot])||
       !td_streetcar_runtime_traffic_clear_extent(td_streetcar_view_district,u,v,extents[slot])||
       td_scenery_contact(ou,ov,u,v,extents[slot],speed)!=TD_SCENERY_CLEAR||
       !td_traffic_epoch_commit(epoch,slot))return FALSE;
    td_traffic_u[slot]=u;td_traffic_v[slot]=v;return TRUE;
}
/* Keep111-byte epoch allocation outside the hot inner argument frame. */
static UBYTE td_ram_move(UBYTE slot,UWORD u,UWORD v,BYTE speed){
    td_traffic_epoch_t epoch;return td_ram_move_inner(slot,u,v,speed,&epoch);
}
void td_ramming_reset(void) BANKED {memset(td_ram_state,0,sizeof(td_ram_state));memset(td_ram_anchor_u,0,sizeof(td_ram_anchor_u));memset(td_ram_anchor_v,0,sizeof(td_ram_anchor_v));}
UBYTE td_ramming_active(UBYTE slot) BANKED {return slot<8&&td_ram_state[slot];}
UBYTE td_ramming_try(UBYTE slot,UWORD u,UWORD v) BANKED {
    static const UBYTE masses[8]={1,5,2,6,3,7,1,1};
    WORD du=(WORD)u-td.u,dv=(WORD)v-td.v;UWORD pu,pv,anchor_u,anchor_v;UBYTE dir,amount,pm,nm,relative,impulse,keep,active,headon;
    WORD npc;BYTE speed=td.speed<0?-td.speed:td.speed;
    if(slot>=8||speed<3||td.onfoot||td.mode!=TD_ROAM||td_sandbox_captured(slot)||td_sandbox_owned(slot)||
       td_boats_controlled()||td_motion_rebounding()||(!du&&!dv)||du>16||du< -16||dv>16||dv< -16)return FALSE;
    if((du<0?-du:du)>=(dv<0?-dv:dv)){dir=du>0?0:2;amount=du<0?-du:du;}
    else{dir=dv>0?1:3;amount=dv<0?-dv:dv;}
    if(!amount)return FALSE;
    pu=td_traffic_u[slot]+td_ram_dx[dir]*amount;pv=td_traffic_v[slot]+td_ram_dy[dir]*amount;
    active=td_ram_state[slot]!=0;anchor_u=active?td_ram_anchor_u[slot]:td_traffic_u[slot];anchor_v=active?td_ram_anchor_v[slot]:td_traffic_v[slot];
    if(td_ram_distance(pu,anchor_u)>384||td_ram_distance(pv,anchor_v)>384)return FALSE;
    /* Ordinary authored traffic averages half a Q4 unit per VBlank. Round
       that small velocity up to one along the actual pending cardinal leg. */
    npc=dir==0||dir==2?(WORD)td_traffic_samples[slot].u-(WORD)td_traffic_u[slot]:
        (WORD)td_traffic_samples[slot].v-(WORD)td_traffic_v[slot];
    npc=npc>0?1:npc<0?-1:0;npc*=dir==2||dir==3?-1:1;headon=npc<0;
    pm=td.vehicle==1?7:td.vehicle==2||td.vehicle==3?2:4;nm=masses[slot];
    relative=speed-npc;impulse=(relative*pm)/(pm+nm);impulse=(impulse+1)/2;if(impulse<1)impulse=1;if(impulse>7)impulse=7;
    if(!td_ram_move(slot,pu,pv,relative))return FALSE;
    if(!active){td_ram_anchor_u[slot]=anchor_u;td_ram_anchor_v[slot]=anchor_v;}
    td_ram_state[slot]=dir|(impulse<<2)|(7<<5);
    /* Head-on contact and a heavy body retain the stricter player recoil.
       A faster rear/tangent impact transfers momentum once per identity. */
    if(headon||nm>pm)return FALSE;
    if(!active){keep=(speed*pm+(npc>0?npc:0)*nm)/(pm+nm);if(!keep)keep=1;td_motion_transfer(keep);}
    return td_sandbox_clear(td.u,td.v,u,v,7,TD_NONE);
}
UBYTE td_ramming_step(void) BANKED {
    UBYTE i,state,dir,phase,amount,dirty=FALSE;UWORD u,v;
    for(i=0;i<8;i++){
        state=td_ram_state[i];if(!state)continue;dirty=TRUE;
        if(td_sandbox_captured(i)){td_ram_state[i]=0;continue;}
        dir=state&3;phase=state>>5;u=td_traffic_u[i];v=td_traffic_v[i];
        if(phase){
            amount=((state>>2)&7)*2;u+=td_ram_dx[dir]*amount;v+=td_ram_dy[dir]*amount;
            if(td_ram_distance(u,td_ram_anchor_u[i])<=384&&td_ram_distance(v,td_ram_anchor_v[i])<=384)
                td_ram_move(i,u,v,amount);
            td_ram_state[i]=(state&31)|((phase-1)<<5);
        }else{
            if(u<td_ram_anchor_u[i])u+=td_ram_distance(u,td_ram_anchor_u[i])<8?td_ram_distance(u,td_ram_anchor_u[i]):8;
            else if(u>td_ram_anchor_u[i])u-=td_ram_distance(u,td_ram_anchor_u[i])<8?td_ram_distance(u,td_ram_anchor_u[i]):8;
            else if(v<td_ram_anchor_v[i])v+=td_ram_distance(v,td_ram_anchor_v[i])<8?td_ram_distance(v,td_ram_anchor_v[i]):8;
            else if(v>td_ram_anchor_v[i])v-=td_ram_distance(v,td_ram_anchor_v[i])<8?td_ram_distance(v,td_ram_anchor_v[i]):8;
            if(td_ram_move(i,u,v,1)&&u==td_ram_anchor_u[i]&&v==td_ram_anchor_v[i])td_ram_state[i]=0;
        }
    }
    return dirty;
}
