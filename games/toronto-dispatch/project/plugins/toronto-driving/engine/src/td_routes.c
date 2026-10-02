#pragma bank 255
#include "td_game.h"
#include "td_streetcar_runtime.h"
#define TD_WORLD_ROUTE_DATA
#include "td_world_routes.h"

static UWORD td_route_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static UWORD td_route_position(UBYTE identity,UWORD start){
    UBYTE phase=(td.seconds*12+td.subsecond/5+identity*37)&127;
    return start+(phase<64?phase:127-phase);
}

/* Scan ROM only when the viewport changes. Keep six identities and their
   coordinates in WRAM, rather than copying the whole 512-byte route table. */
void td_refresh_routes(UBYTE *identities,UWORD (*nearby)[2]) BANKED {
    UBYTE i,j,k,route;UWORD score,best,u,pu=(td_streetcar_ride_view?td_streetcar_focus_u:td.u)>>4,pv=(td_streetcar_ride_view?td_streetcar_focus_v:td.v)>>4;
    UBYTE district=td_streetcar_ride_view?td_streetcar_view_district:td.district;
    const UWORD (*routes)[2]=td_district_routes[district];
    for(i=0;i<6;i++){
        route=identities[i];
        if(route<td_route_counts[district]&&
           td_route_distance(pu,td_route_position(route,nearby[i][0]))<144&&
           td_route_distance(pv,nearby[i][1])<112)continue;
        identities[i]=TD_NONE;best=65535;
        for(j=0;j<td_route_counts[district];j++){
            if(td_route_distance(pu,routes[j][0]+32)>176||td_route_distance(pv,routes[j][1])>112)continue;
            for(k=0;k<6;k++)if(identities[k]==j)break;
            if(k<6)continue;
            u=td_route_position(j,routes[j][0]);
            score=td_route_distance(pu,u)+td_route_distance(pv,routes[j][1]);
            if(score<best){best=score;identities[i]=j;nearby[i][0]=routes[j][0];nearby[i][1]=routes[j][1];}
        }
    }
}
