#pragma bank 255
#include "td_aircraft.h"

td_aircraft_state_t td_aircraft;

static UWORD td_aircraft_random(void){
    UWORD value=td_aircraft.seed;
    value^=value<<7;value^=value>>9;value^=value<<8;
    return td_aircraft.seed=value?value:0x9D27;
}
void td_aircraft_reset(UWORD seed) BANKED {
    td_aircraft.seed=seed?seed:0x9D27;
    td_aircraft.u=td_aircraft.v=0;td_aircraft.ticks=0;
    td_aircraft.active=td_aircraft.kind=td_aircraft.direction=0;
    td_aircraft.wait=360+(td_aircraft_random()&127);
}
static void td_aircraft_spawn(UWORD focus_u,UWORD focus_v){
    UWORD value=td_aircraft_random();WORD u,v,jitter;
    /* Match the clamped north-up camera, including its 16px HUD offset. */
    u=focus_u<80?80:focus_u>944?944:focus_u;
    v=focus_v<88?72:focus_v>920?904:focus_v-16;
    jitter=(WORD)((value>>4)&63)-32;
    td_aircraft.kind=value&1;td_aircraft.direction=(value>>1)&3;
    if(td_aircraft.direction&1){u+=jitter;v+=td_aircraft.direction==1?-112:112;}
    else{v+=jitter;u+=td_aircraft.direction==0?-112:112;}
    td_aircraft.u=u*16;td_aircraft.v=v*16;
    td_aircraft.ticks=0;td_aircraft.active=1;
}
void td_aircraft_update(UWORD elapsed,UWORD focus_u,UWORD focus_v) BANKED {
    UWORD remaining,travel;WORD delta;
    if(!elapsed)return;
    if(!td_aircraft.active){
        if(elapsed<td_aircraft.wait){td_aircraft.wait-=elapsed;return;}
        elapsed-=td_aircraft.wait;
        td_aircraft_spawn(focus_u,focus_v);
    }
    remaining=(td_aircraft.kind?448:224)-td_aircraft.ticks;
    travel=elapsed<remaining?elapsed:remaining;
    delta=travel*(td_aircraft.kind?12:24);
    switch(td_aircraft.direction){
        case 0:td_aircraft.u+=delta;break;
        case 1:td_aircraft.v+=delta;break;
        case 2:td_aircraft.u-=delta;break;
        default:td_aircraft.v-=delta;break;
    }
    td_aircraft.ticks+=travel;
    if(elapsed>=remaining){
        td_aircraft.active=0;
        /* An occasional flight, never a constant swarm; no simulation actors
         * are used in unloaded districts and no mission clock is changed. */
        td_aircraft.wait=1200+(td_aircraft_random()&1023);
    }
}
UBYTE td_aircraft_frame(void) BANKED {
    return td_aircraft.direction+td_aircraft.kind*4+
        (td_aircraft.kind&&(td_aircraft.ticks&8)?4:0);
}
