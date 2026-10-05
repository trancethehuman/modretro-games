#pragma bank 255
#include "td_traffic.h"
#include "td_district.h"

typedef struct { UWORD u,v; UBYTE arms,tile_x,tile_y; } td_signal_t;
#include "td_traffic_signals.h"

#define td_traffic_distance(a,b) ((a)>(b)?(a)-(b):(b)-(a))
static UBYTE td_traffic_step_limit(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UWORD limit){
    return old_u<1024*16&&u<1024*16&&old_v<976*16&&v<976*16&&
        !(old_u!=u&&old_v!=v)&&
        td_traffic_distance(old_u,u)+td_traffic_distance(old_v,v)<=limit;
}
#define td_traffic_step_valid(a,b,c,d) td_traffic_step_limit(a,b,c,d,8)
static UWORD td_traffic_first(const td_signal_t *signals,UWORD first,UWORD end,UWORD lateral,UBYTE horizontal){
    UWORD middle,centre,lower=lateral>18*16?lateral-18*16:0;
    while(first<end){
        middle=first+(end-first)/2;
        centre=(horizontal?signals[middle].v:signals[middle].u)*16;
        if(centre<lower)first=middle+1;else end=middle;
    }
    return first;
}
UBYTE td_traffic_signal_stop(UBYTE district,UWORD seconds,UWORD old_u,
    UWORD old_v,UWORD u,UWORD v) BANKED {
    const td_signal_t *signals;UWORD i,line,pos,next,lateral,centre;UBYTE horizontal,arm;
    if(district>=TD_DISTRICT_COUNT||district>=TD_TRAFFIC_SIGNAL_DISTRICTS||
       !td_traffic_step_valid(old_u,old_v,u,v))return TRUE;
    if(old_u==u&&old_v==v)return FALSE;
    horizontal=old_u!=u;
    if(horizontal?(seconds%12<7):(seconds%12>=7))return FALSE;
    pos=horizontal?old_u:old_v;next=horizontal?u:v;lateral=horizontal?v:u;
    arm=horizontal?(u>old_u?8:2):(v>old_v?1:4);
    signals=horizontal?td_signals_h:td_signals_v;
    i=td_traffic_first(signals,td_signal_offsets[district],td_signal_offsets[district+1],lateral,horizontal);
    for(;i<td_signal_offsets[district+1];i++){
        centre=(horizontal?signals[i].v:signals[i].u)*16;
        if(centre>lateral+18*16)break;
        if(!(signals[i].arms&arm))continue;
        centre=(horizontal?signals[i].u:signals[i].v)*16;
        line=next>pos?centre-24*16:centre+24*16;
        if(next>pos?(pos<=line&&next>line):(pos>=line&&next<line))return TRUE;
    }
    return FALSE;
}

/* Player physics sweeps horizontal motion at old_v, then vertical motion at
 * the accepted new_u. Reuse the same authored stop lines and fictional phase
 * as NPCs, splitting each validated component into their strict8Q4 queries.
 * Malformed movement is not evidence of a traffic violation. */
UBYTE td_traffic_player_red(UBYTE district,UWORD seconds,UWORD old_u,
    UWORD old_v,UWORD u,UWORD v) BANKED {
    UWORD next,amount;
    if(district>=TD_DISTRICT_COUNT||district>=TD_TRAFFIC_SIGNAL_DISTRICTS||
       old_u>=1024*16||u>=1024*16||old_v>=976*16||v>=976*16||
       td_traffic_distance(old_u,u)>16||td_traffic_distance(old_v,v)>16)return FALSE;
    while(old_u!=u){
        amount=td_traffic_distance(old_u,u);if(amount>8)amount=8;
        next=u>old_u?old_u+amount:old_u-amount;
        if(td_traffic_signal_stop(district,seconds,old_u,old_v,next,old_v))return TRUE;
        old_u=next;
    }
    while(old_v!=v){
        amount=td_traffic_distance(old_v,v);if(amount>8)amount=8;
        next=v>old_v?old_v+amount:old_v-amount;
        if(td_traffic_signal_stop(district,seconds,u,old_v,u,next))return TRUE;
        old_v=next;
    }
    return FALSE;
}
