#pragma bank 255
#include "td_police.h"
#include "td_police_lanes.h"
#include "td_district.h"
#include "td_roads.h"

UBYTE td_police_plan(UBYTE district,UBYTE wanted,UWORD u,UWORD v,
    UWORD target_u,UWORD target_v,UBYTE heading,td_police_plan_t *out) BANKED {
    if(!out||district>=TD_DISTRICT_COUNT||district==TD_DISTRICT_ISLANDS||
       district!=td_district_current()||wanted>3||heading>3||
       u<8*16||u>1016*16||v<8*16||v>968*16||
       target_u>=1024*16||target_v>=976*16||
       !td_road_body(u>>4,v>>4,5)||
       (u==target_u&&v==target_v))return FALSE;
    return td_police_lanes_next(district,wanted,u,v,target_u,target_v,heading,out);
}
