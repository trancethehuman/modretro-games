#pragma bank 255
#include "td_aircraft.h"

td_aircraft_state_t td_aircraft;
/*15 transient bytes; never serialized and no additional actors/graphics. */
typedef struct {UWORD u,v,last_u,last_v,lost; signed char vx,vy;UBYTE wanted,exposed,phase;} td_aircraft_police_t;
static td_aircraft_police_t td_aircraft_police;
#ifdef __SDCC
typedef char td_aircraft_police_fits[(sizeof(td_aircraft_police_t)<=15)?1:-1];
#endif
void td_aircraft_police_target(UBYTE wanted,UWORD u,UWORD v,UBYTE exposed) BANKED {
    td_aircraft_police.wanted=wanted>3?3:wanted;
    td_aircraft_police.u=u;td_aircraft_police.v=v;
    td_aircraft_police.exposed=exposed&&u<16384&&v<15616;
}
static UBYTE td_aircraft_police_observed_local(void) {
    WORD du,dv;
    if(td_aircraft_police.phase!=1||!td_aircraft.active||!td_aircraft_police.wanted||
       !td_aircraft_police.exposed)return 0;
    du=td_aircraft.u-(WORD)td_aircraft_police.u;
    dv=td_aircraft.v-(WORD)td_aircraft_police.v;
    /*World-space80px square sight range, independent of camera visibility. */
    return du>=-1280&&du<=1280&&dv>=-1280&&dv<=1280;
}
UBYTE td_aircraft_police_observed(void) BANKED {
    return td_aircraft_police_observed_local();
}

static UWORD td_aircraft_random(void){
    UWORD value=td_aircraft.seed;
    value^=value<<7;value^=value>>9;value^=value<<8;
    return td_aircraft.seed=value?value:0x9D27;
}
void td_aircraft_reset(UWORD seed) BANKED {
    td_aircraft_police.u=td_aircraft_police.v=td_aircraft_police.last_u=td_aircraft_police.last_v=0;
    td_aircraft_police.lost=0;td_aircraft_police.vx=td_aircraft_police.vy=0;
    td_aircraft_police.wanted=td_aircraft_police.exposed=td_aircraft_police.phase=0;
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
    td_aircraft.kind=(value&1)?1:(value&256)?2:0;td_aircraft.direction=(value>>1)&3;
    /* 128px clears the whole32px craft, its lower shadow and camera slack. */
    if(td_aircraft.direction&1){u+=jitter;v+=td_aircraft.direction==1?-128:128;}
    else{v+=jitter;u+=td_aircraft.direction==0?-128:128;}
    td_aircraft.u=u*16;td_aircraft.v=v*16;
    td_aircraft.ticks=0;td_aircraft.active=1;
}
static UBYTE td_aircraft_in_view(UWORD focus_u,UWORD focus_v){
    WORD left,top,u=td_aircraft.u>>4,v=td_aircraft.v>>4;
    left=focus_u<80?0:focus_u>944?864:focus_u-80;
    top=focus_v<88?0:focus_v>920?832:focus_v-88;
    /* Conservative complete craft/shadow bounds plus camera deadzone slack.
     * Following a flight must not make its timer erase it in the viewport. */
    return u+32>=left&&u-24<=left+159&&v+48>=top&&v-24<=top+143;
}
static void td_aircraft_police_acquire(void){
    td_aircraft_police.phase=1;td_aircraft_police.lost=0;
    td_aircraft_police.last_u=td_aircraft_police.u;
    td_aircraft_police.last_v=td_aircraft_police.v;
    td_aircraft_police.vx=td_aircraft_police.vy=0;
    /* Existing visible helicopter keeps its world location and rotor phase. */
}
static void td_aircraft_police_leave(UWORD focus_u,UWORD focus_v){
    WORD u=focus_u<80?80:focus_u>944?944:focus_u;
    WORD v=focus_v<88?72:focus_v>920?904:focus_v-16;
    WORD du=td_aircraft.u-u*16,dv=td_aircraft.v-v*16;
    td_aircraft_police.phase=2;
    /* Choose the nearest edge once; never retarget to a moving camera. */
    if((du<0?-du:du)>=(dv<0?-dv:dv))td_aircraft.direction=du<0?2:0;
    else td_aircraft.direction=dv<0?3:1;
}
static signed char td_aircraft_velocity(signed char current,WORD difference,UBYTE limit){
    signed char wanted=difference>64?limit:difference<-64?-(signed char)limit:0;
    if(current<wanted)current++;else if(current>wanted)current--;
    return current;
}
static void td_aircraft_pursue(UWORD elapsed,UWORD focus_u,UWORD focus_v){
    UWORD count=elapsed>16?16:elapsed;WORD du,dv;UBYTE limit;
    while(count--){
        if(td_aircraft_police.phase==1){
            if(!td_aircraft_police.wanted)td_aircraft_police_leave(focus_u,focus_v);
            else if(td_aircraft_police_observed_local()){
                td_aircraft_police.last_u=td_aircraft_police.u;
                td_aircraft_police.last_v=td_aircraft_police.v;td_aircraft_police.lost=0;
            }else if(++td_aircraft_police.lost>=300)td_aircraft_police_leave(focus_u,focus_v);
        }
        if(td_aircraft_police.phase==1){
            /* A visible24px/32px offset leaves the courier clear beneath it.
             * Concealed/out-of-range motion never updates this last sighting. */
            du=(WORD)td_aircraft_police.last_u+384-td_aircraft.u;
            dv=(WORD)td_aircraft_police.last_v-512-td_aircraft.v;
            limit=10+td_aircraft_police.wanted*2;
        }else{
            du=td_aircraft.direction==0?2048:td_aircraft.direction==2?-2048:0;
            dv=td_aircraft.direction==1?2048:td_aircraft.direction==3?-2048:0;limit=16;
        }
        td_aircraft_police.vx=td_aircraft_velocity(td_aircraft_police.vx,du,limit);
        td_aircraft_police.vy=td_aircraft_velocity(td_aircraft_police.vy,dv,limit);
        td_aircraft.u+=td_aircraft_police.vx;td_aircraft.v+=td_aircraft_police.vy;
        td_aircraft.ticks++;
        if(td_aircraft_police.phase==1){
            if((td_aircraft_police.vx<0?-td_aircraft_police.vx:td_aircraft_police.vx)>=
               (td_aircraft_police.vy<0?-td_aircraft_police.vy:td_aircraft_police.vy)){
                if(td_aircraft_police.vx)td_aircraft.direction=td_aircraft_police.vx<0?2:0;
            }else if(td_aircraft_police.vy)td_aircraft.direction=td_aircraft_police.vy<0?3:1;
        }
        if(td_aircraft_police.phase==2&&!td_aircraft_in_view(focus_u,focus_v)){
            td_aircraft.active=0;td_aircraft_police.phase=3;
            /*Thirty seconds to break aerial attention after a real escape.
             * Search does not instantly respawn omnisciently at the courier. */
            td_aircraft.wait=1800;return;
        }
    }
}
void td_aircraft_update(UWORD elapsed,UWORD focus_u,UWORD focus_v) BANKED {
    UWORD remaining,travel;WORD delta;
    if(!elapsed)return;
    if(!td_aircraft.active){
        if(td_aircraft_police.phase==3){
            if(elapsed<td_aircraft.wait){td_aircraft.wait-=elapsed;return;}
            td_aircraft_police.phase=0;
        }
        if(td_aircraft_police.wanted){
            td_aircraft_spawn(focus_u,focus_v);td_aircraft.kind=1;
            td_aircraft_police_acquire();return;
        }
        if(elapsed<td_aircraft.wait){td_aircraft.wait-=elapsed;return;}
        td_aircraft_spawn(focus_u,focus_v);
        /* Never spend a delayed update's residual on a newly created flight:
         * that could materialise it halfway across the screen. The first
         * rendered state is always wholly outside the clamped camera; later
         * updates retain this world path even while the camera moves. */
        return;
    }
    if(td_aircraft_police.wanted&&td_aircraft.kind==1&&!td_aircraft_police.phase)
        td_aircraft_police_acquire();
    if(td_aircraft_police.phase){td_aircraft_pursue(elapsed,focus_u,focus_v);return;}
    remaining=td_aircraft.kind==1?448:224;
    remaining=td_aircraft.ticks<remaining?remaining-td_aircraft.ticks:64;
    travel=elapsed<remaining?elapsed:remaining;
    delta=travel*(td_aircraft.kind==1?12:24);
    switch(td_aircraft.direction){
        case 0:td_aircraft.u+=delta;break;
        case 1:td_aircraft.v+=delta;break;
        case 2:td_aircraft.u-=delta;break;
        default:td_aircraft.v-=delta;break;
    }
    td_aircraft.ticks+=travel;
    if(td_aircraft.ticks>=(td_aircraft.kind==1?448:224)&&!td_aircraft_in_view(focus_u,focus_v)){
        td_aircraft.active=0;
        /* An occasional flight, never a constant swarm; no simulation actors
         * are used in unloaded districts and no mission clock is changed. */
        td_aircraft.wait=1200+(td_aircraft_random()&1023);
    }
}
UBYTE td_aircraft_frame(void) BANKED {
    if(td_aircraft.kind==2)return 14+td_aircraft.direction;
    return td_aircraft.direction+td_aircraft.kind*4+
        (td_aircraft.kind==1&&(td_aircraft.ticks&8)?4:0);
}
