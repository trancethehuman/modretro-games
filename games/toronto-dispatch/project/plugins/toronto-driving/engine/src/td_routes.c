#pragma bank 255
#include "td_game.h"
#include "td_people.h"
#include "td_streetcar_runtime.h"
#include "td_world.h"
#include "actor.h"
#define TD_WORLD_ROUTE_DATA
#include "td_world_routes.h"

static UWORD td_route_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static UWORD td_route_position(UBYTE identity,UWORD start,UBYTE clock){
    UBYTE phase=(clock+identity*37)&127;
    return start+(phase<64?phase:127-phase);
}

/* The canonical generator sorts every used route by(y,x), preserving
 * identity order. Find only the possible inclusive vertical candidates;
 * selection below still visits those identities in the original order. */
static UBYTE td_route_first(const UWORD (*routes)[2],UBYTE end,UWORD lower){
    UBYTE first=0,middle;
    while(first<end){
        middle=first+(end-first)/2;
        if(routes[middle][1]<lower)first=middle+1;else end=middle;
    }
    return first;
}

/* Scan ROM only when the viewport changes. Keep eight identities and their
   coordinates in WRAM, rather than copying the whole 512-byte route table. */
void td_refresh_routes(UBYTE *identities,UWORD (*nearby)[2]) BANKED {
    UBYTE i,j,k,route,first,clock;UWORD score,best,u,upper,pu=(td_streetcar_ride_view?td_streetcar_focus_u:td.u)>>4,pv=(td_streetcar_ride_view?td_streetcar_focus_v:td.v)>>4;
    UBYTE district=td_streetcar_ride_view?td_streetcar_view_district:td.district;
    const UWORD (*routes)[2];
    /* A malformed derived view must never index ROM beyond the registered
       rows. Hide all identities while preserving their cached coordinates. */
    if(district>=TD_DISTRICT_COUNT){
        for(i=0;i<TD_PEOPLE_COUNT;i++)identities[i]=TD_NONE;
        return;
    }
    routes=td_district_routes[district];
    clock=(td.seconds*12+td.subsecond/5)&127;upper=pv+112;
    first=td_route_first(routes,td_route_counts[district],pv>112?pv-112:0);
    for(i=0;i<TD_PEOPLE_COUNT;i++){
        route=identities[i];
        if(route<td_route_counts[district]&&(td_people_route_visible(route)||(
           td_route_distance(pu,td_route_position(route,nearby[i][0],clock))<144&&
           td_route_distance(pv,nearby[i][1])<112)))continue;
        identities[i]=TD_NONE;best=65535;
        for(j=first;j<td_route_counts[district];j++){
            if(routes[j][1]>upper)break;
            if(td_route_distance(pu,routes[j][0]+32)>176)continue;
            for(k=0;k<TD_PEOPLE_COUNT;k++)if(identities[k]==j)break;
            if(k<TD_PEOPLE_COUNT)continue;
            u=td_route_position(j,routes[j][0],clock);
            score=td_route_distance(pu,u)+td_route_distance(pv,routes[j][1]);
            if(score<best){best=score;identities[i]=j;nearby[i][0]=routes[j][0];nearby[i][1]=routes[j][1];}
        }
    }
}

void td_ferry_beacon(void) BANKED {
    td_stop_t approach;td_portal_t portal;
    UBYTE i,best_id=20,routed;UWORD score,best=0xFFFF;
    if(td.district==TD_DISTRICT_ISLANDS&&td_target.district!=TD_DISTRICT_ISLANDS){
        /* A local dock cue is separate from the ordinary road seam graph.
           This bounded distance chooses guidance, not a shortest foot path. */
        for(i=20;i<=22;i++){
            td_get_stop(i,&approach);
            score=td_route_distance(td.u>>4,approach.u)+td_route_distance(td.v>>4,approach.v);
            if(score<best){best=score;best_id=i;}
        }
        td_get_stop(best_id,&approach);
    }else td_get_stop(10,&approach);
    routed=td_world_route(td.district,approach.district,td.onfoot,td.u>>4,td.v>>4,approach.u,approach.v,&portal);
    td_route_district=routed?portal.to:TD_DISTRICT_NONE;
    actors[1].pos.x=(routed?portal.u:approach.u)*32;
    actors[1].pos.y=((routed?portal.v:approach.v)-12)*32;
    if(td.district!=approach.district&&!routed)actors[1].flags|=ACTOR_FLAG_HIDDEN;
    else actors[1].flags&=~ACTOR_FLAG_HIDDEN;
}
