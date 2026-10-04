#pragma bank 255
#include "td_police.h"
#include "td_district.h"
#include "td_roads.h"

typedef struct { UWORD u,v; UBYTE arms,tile_x,tile_y; } td_signal_t;
#define TD_TRAFFIC_SIGNALS_HORIZONTAL_ONLY
#include "td_traffic_signals.h"

#define td_police_distance(a,b) ((a)>(b)?(a)-(b):(b)-(a))
static const UBYTE td_police_arm[]={2,4,8,1};

/* Find the next existing junction on this actual lane, rather than indexing
 * the police slot's old rectangular patrol leg. Unsigned comparisons precede
 * subtraction so neither west/north nor a world edge can wrap. */
static UBYTE td_police_candidate(UBYTE district,UWORD u,UWORD v,
    UWORD goal_u,UWORD goal_v,UBYTE direction,UBYTE junction,
    UWORD *next_u,UWORD *next_v){
    UWORD i,along,lateral,position=direction&1?v:u;
    UWORD lane=direction&1?u:v,nearest=65535,distance,endpoint=position,projection;
    UBYTE positive=direction<2;
    for(i=td_signal_offsets[district];i<td_signal_offsets[district+1];i++){
        lateral=(direction&1?td_signals_h[i].u:td_signals_h[i].v)*16;
        if(td_police_distance(lateral,lane)>18*16)continue;
        if(!(td_signals_h[i].arms&td_police_arm[(direction+2)&3]))continue;
        along=(direction&1?td_signals_h[i].v:td_signals_h[i].u)*16;
        if(positive?along<=position:along>=position)continue;
        distance=td_police_distance(along,position);
        /* Already inside a junction: select its outgoing leg, not a second
         * waypoint eight pixels away at the same crossing's centre. */
        if(junction&&distance<=24*16)continue;
        if(distance<nearest){nearest=distance;endpoint=along;}
    }
    if(nearest==65535){
        /* A two-arm bend has no traffic-light node. A bounded road probe
         * reaches it without inventing a diagonal or a road through water. */
        if(positive){
            endpoint=position+32*16;
            if(endpoint>(direction&1?968:1016)*16)endpoint=(direction&1?968:1016)*16;
        }else endpoint=position>40*16?position-32*16:8*16;
    }
    projection=direction&1?goal_v:goal_u;
    /* If the goal is before the junction, stop at its road projection. This
     * also permits an exact, continuously driven return to a patrol target. */
    if(positive?(projection>position&&projection<endpoint):
                 (projection<position&&projection>endpoint))endpoint=projection;
    *next_u=direction&1?u:endpoint;*next_v=direction&1?endpoint:v;
    if(endpoint==position)return FALSE;
    if(td_road_sweep(u>>4,v>>4,*next_u>>4,*next_v>>4,5))return TRUE;
    /* A distant matching road row may lie beyond a rail/water interruption.
     * Limit this waypoint to connected ground; never accept just endpoints. */
    if(positive){
        endpoint=position+8*16;
        if(endpoint>(direction&1?968:1016)*16)return FALSE;
    }else{
        if(position<16*16)return FALSE;
        endpoint=position-8*16;
    }
    *next_u=direction&1?u:endpoint;*next_v=direction&1?endpoint:v;
    return td_road_sweep(u>>4,v>>4,*next_u>>4,*next_v>>4,5);
}

UBYTE td_police_plan(UBYTE district,UBYTE wanted,UWORD u,UWORD v,
    UWORD target_u,UWORD target_v,UBYTE heading,td_police_plan_t *out) BANKED {
    UWORD i,distance,closest=65535,next_u,next_v,score,best=65535;
    UBYTE direction,arms=0,found=FALSE;
    td_police_plan_t result;
    if(!out||district>=TD_DISTRICT_COUNT||district>=TD_TRAFFIC_SIGNAL_DISTRICTS||
       district!=td_district_current()||wanted>3||heading>3||
       u<8*16||u>1016*16||v<8*16||v>968*16||
       target_u>=1024*16||target_v>=976*16||
       !td_road_body(u>>4,v>>4,5))return FALSE;
    if(u==target_u&&v==target_v)return FALSE;
    /* A patrol lane may be eight pixels beside the centre used while
     * chasing. Finish a nearby cardinal connection on actual ground, rather
     * than orbiting junctions forever around an already reached projection. */
    if((u==target_u||v==target_v)&&
       td_police_distance(u,target_u)+td_police_distance(v,target_v)<=18*16&&
       td_road_sweep(u>>4,v>>4,target_u>>4,target_v>>4,5)){
        result.u=target_u;result.v=target_v;
        result.heading=u!=target_u?(target_u>u?0:2):(target_v>v?1:3);
        result.valid=TRUE;*out=result;return TRUE;
    }
    for(i=td_signal_offsets[district];i<td_signal_offsets[district+1];i++){
        if(td_police_distance(u,td_signals_h[i].u*16)>18*16||
           td_police_distance(v,td_signals_h[i].v*16)>18*16)continue;
        distance=td_police_distance(u,td_signals_h[i].u*16)+
                 td_police_distance(v,td_signals_h[i].v*16);
        if(distance<closest){closest=distance;arms=td_signals_h[i].arms;}
    }
    /* Between junctions retain the current heading. This avoids turning
     * across a road merely because the courier moves beside a building. */
    if(!arms&&td_police_candidate(district,u,v,target_u,target_v,heading,FALSE,&next_u,&next_v)){
        result.u=next_u;result.v=next_v;result.heading=heading;result.valid=TRUE;
        *out=result;return TRUE;
    }
    for(direction=0;direction<4;direction++){
        if(arms&&!(arms&td_police_arm[direction]))continue;
        if(!td_police_candidate(district,u,v,target_u,target_v,direction,!!arms,&next_u,&next_v))continue;
        score=td_police_distance(next_u,target_u)+td_police_distance(next_v,target_v);
        /* Stable ties keep the previous heading; turning has a small cost,
         * reversing a larger one. A sole connected exit can still reverse. */
        if(direction!=heading)score+=((direction+2)&3)==heading?128:32;
        if(score<best){
            best=score;result.u=next_u;result.v=next_v;
            result.heading=direction;result.valid=TRUE;found=TRUE;
        }
    }
    if(found)*out=result;
    return found;
}
