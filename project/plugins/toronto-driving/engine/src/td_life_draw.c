#pragma bank 255
/* Street-life presentation: owned walkers and vehicles, effects, the
 * courier's hurt/knock-out poses and the objective beacon or pointer. */
#include <string.h>
#include "td_life_int.h"
#include "td_audio.h"
#include "td_district.h"
#include "td_daynight.h"
#include "td_shots.h"
#include "camera.h"
#include "input.h"
#include "system.h"


/* Facing (td_walk_dir: east, west, south, north) as unit steps. */
static const BYTE lf_tk_u[4]={1,-1,0,0},lf_tk_v[4]={0,0,1,-1};
void td_life_peds(UBYTE near) BANKED {
    UBYTE i,bit,a,mode,f,hop;actor_t *p;WORD du,dv;
    near&=~pk_fresh;
    a=td.speed<0?(UBYTE)-td.speed:(UBYTE)td.speed;
    /* Walkers in the path of a fast car (where it will be in 16 ticks)
     * usually jump clear; the slow and the unlucky do not. */
    if(td.mode==TD_ROAM&&!td.onfoot&&a>=8&&(td_tick&7)==4){
        WORD au=(WORD)(td.u>>4)+(td_vx>>4),av=(WORD)(td.v>>4)+(td_vy>>4);
        for(i=0,bit=1,p=&actors[TD_ACTOR_PEDS];i<TD_PEDS;i++,bit<<=1,p++){
            if((td_ped_ovr&bit)||(near&bit)||(p->flags&ACTOR_FLAG_HIDDEN))continue;
            if(lf_abs((WORD)(p->pos.x>>5)-au)<24&&lf_abs((WORD)(p->pos.y>>5)-av)<24&&(i+(td_tick>>3))&3)td_lf_dodge(i);
        }
    }
    if(!near&&!pk_fresh&&!td_ped_ovr)return;
    /* A moving car throws struck walkers; one consequence per person. */
    if(near&&td.mode==TD_ROAM&&!td.onfoot&&a>=3){
        for(i=0,bit=1;i<TD_PEDS;i++,bit<<=1){
            if(!(near&bit))continue;
            td_lf_knock(i,lf_div16(td_vx+(td_vx>>1)),lf_div16(td_vy+(td_vy>>1)),a>=16);
            td.speed-=lf_div4(td.speed);td_vx-=lf_div4(td_vx);td_vy-=lf_div4(td_vy);
            if(td.job!=TD_NONE&&td.stage)td.health=td.health>10?td.health-10:0;
            td_audio_play(TD_AUDIO_IMPACT);td_message(TD_MSG_PED);
        }
    }
    /* A running courier bowls over anyone they run into. */
    if(near&&td.mode==TD_ROAM&&td.onfoot&&td_running){
        for(i=0,bit=1;i<TD_PEDS;i++,bit<<=1){
            if(!(near&bit))continue;
            td_lf_knock(i,lf_tk_u[td_walk_dir&3]*22,lf_tk_v[td_walk_dir&3]*22,0);
            lf_shake=3;td_audio_play(TD_AUDIO_IMPACT);
        }
    }
    /* New route walkers stay hidden (native layout) until they are out of
     * view, so nobody appears in the middle of the screen. */
    if(pk_fresh&&!(td_tick&7))for(i=0,bit=1,p=&actors[TD_ACTOR_PEDS];i<TD_PEDS;i++,bit<<=1,p++){
        if(!(pk_fresh&bit))continue;
        if(td_ped_route[i]==TD_NONE||!lf_on_screen(p->pos.x>>5,p->pos.y>>5))pk_fresh&=~bit;
    }
    if(!td_ped_ovr)return;
    for(i=0,bit=1,p=&actors[TD_ACTOR_PEDS];i<TD_PEDS;i++,bit<<=1,p++){
        if(!(td_ped_ovr&bit))continue;
        mode=pk_mode[i];hop=0;
        /* A body on the ground does not move: draw it once. */
        if(pk_drawn&bit)continue;
        if(mode==PK_FLY){
            /* Arc: height grows then falls over the flight. */
            UBYTE t=pk_timer[i],s=pk_span[i];UWORD h=(UWORD)t*(UBYTE)(s-t);hop=(UBYTE)((h+(h<<1))>>5);
            f=TD_FRAME_KNOCK+((t>>2)&3);
        }else if(mode==PK_DOWN||mode==PK_DEAD){
            f=TD_FRAME_KNOCK+4+(i&1);
            lf_place_q4(p,pk_u[i],pk_v[i]);lf_frame(p,f);p->flags&=~ACTOR_FLAG_HIDDEN;pk_drawn|=bit;continue;
        }else if(i==lf_aim_who){
            /* Aiming: the pistol pose towards the courier, flashing red. */
            du=(WORD)td.u-(WORD)pk_u[i];dv=(WORD)td.v-(WORD)pk_v[i];
            f=TD_FRAME_COURIER_SHOOT+(lf_abs(du)>=lf_abs(dv)?(du>=0?0:1):(dv>=0?2:3));
            TD_PALETTE(p)=(td_tick&4)?TD_PEOPLE_PAL(TD_PAL_RED):LF_OFFICER_PAL;
        }else{
            static const UBYTE dir_frame[4]={0,2,4,6};
            /* Dazed by a jab: standing, blinking. */
            if(mode==PK_STUN){f=lf_look_walk[pk_look[i]]+dir_frame[pk_dir[i]&3];if(pk_timer[i]&4)hop=255;}
            else f=lf_look_walk[pk_look[i]]+dir_frame[pk_dir[i]&3]+((td_tick>>3)&1);
            if(pk_look[i]==LF_LOOK_OFFICER)TD_PALETTE(p)=LF_OFFICER_PAL;
        }
        if(hop==255){p->flags|=ACTOR_FLAG_HIDDEN;continue;}
        lf_place_q4(p,pk_u[i],pk_v[i]-((UWORD)hop<<4));lf_frame(p,f);
        if(lf_on_screen(pk_u[i]>>4,pk_v[i]>>4))p->flags&=~ACTOR_FLAG_HIDDEN;
        else p->flags|=ACTOR_FLAG_HIDDEN;
    }
}

static void lf_beacon(void){
    actor_t *b=&actors[1];WORD x,y;UWORD ax,ay,pu=td.u>>4,pv=td.v>>4;UBYTE east,south,d;
    if(!td_beacon_shown){b->flags|=ACTOR_FLAG_HIDDEN;return;}
    x=(WORD)(td_beacon_u-(UWORD)scroll_x);y=(WORD)(td_beacon_v-(UWORD)scroll_y);
    if((UWORD)(x-4)<152&&(UWORD)(y-8)<128){
        lf_place(b,td_beacon_u,td_beacon_v-12-((sys_time>>4)&1));
        lf_frame(b,TD_FRAME_BEACON+((sys_time>>4)&1));
    }else{
        /* Off screen: a pointer on the view edge toward the objective. */
        east=td_beacon_u>=pu;south=td_beacon_v>=pv;
        ax=east?td_beacon_u-pu:pu-td_beacon_u;ay=south?td_beacon_v-pv:pv-td_beacon_v;
        if(ax>(ay<<1))d=east?0:4;
        else if(ay>(ax<<1))d=south?2:6;
        else d=south?(east?1:3):(east?7:5);
        x=x<10?10:x>148?148:x;y=y<14?14:y>126?126:y;
        lf_place(b,scroll_x+x+4,scroll_y+y+4);
        lf_frame(b,TD_FRAME_ARROW+d);
    }
    b->flags&=~ACTOR_FLAG_HIDDEN;
}

/* Lock-on marker over the walker or patrol car a shot would take; its
 * actor only joins GBVM's active list while it is shown. */
/* Actor 23: the lock-on marker on foot; at night in a stolen car of another
 * paint, that car's headlamp beam (the courier's own car carries its beam in
 * its lit frames). */
static const BYTE lf_beam_dx[8]=TD_BEAM_DX;
static const BYTE lf_beam_dy[8]=TD_BEAM_DY;
static const BYTE lf_sight_dx[8]={40,28,0,-28,-40,-28,0,28};
static const BYTE lf_sight_dy[8]={0,28,40,28,0,-28,-40,-28};
static void lf_marker(void){
    actor_t *r=&actors[TD_ACTOR_RETICLE];UBYTE t=td_aim_target,f,d;
    if(td.onfoot&&t!=TD_NONE&&td.mode==TD_ROAM){
        if(t==8)lf_place_q4(r,td_traffic_u[TD_POLICE_SLOT],td_traffic_v[TD_POLICE_SLOT]);
        else{r->pos.x=actors[TD_ACTOR_PEDS+t].pos.x;r->pos.y=actors[TD_ACTOR_PEDS+t].pos.y;}
        f=TD_FRAME_RETICLE;
    }else if(td.onfoot&&td_aim_hold&&td.ammo&&td.mode==TD_ROAM&&(td_tick&4)){
        /* Firing with nothing locked: a blinking sight 40 px along the aim. */
        d=td_aim_dir&7;
        lf_place(r,(UWORD)((WORD)(td.u>>4)+lf_sight_dx[d]),(UWORD)((WORD)(td.v>>4)+lf_sight_dy[d]));
        f=TD_FRAME_RETICLE;
    }else if(!td.onfoot&&td_daynight_lights&&td_car_colour!=TD_PAL_COURIER&&!td_entry_timer&&(td.mode==TD_ROAM||td.mode==TD_WAIT)){
        d=((td.heading+1)&15)>>1;
        lf_place(r,(UWORD)((WORD)(td.u>>4)+lf_beam_dx[d]),(UWORD)((WORD)(td.v>>4)+lf_beam_dy[d]));
        f=TD_FRAME_BEAM+d;
    }else{
        if(r->flags&ACTOR_FLAG_ACTIVE){r->flags|=ACTOR_FLAG_HIDDEN;deactivate_actor(r);}
        return;
    }
    TD_PALETTE(r)=0;
    /* Activation resets the idle animation, so the frame is set after it. */
    if(!(r->flags&ACTOR_FLAG_ACTIVE)){r->flags&=~(ACTOR_FLAG_DISABLED|ACTOR_FLAG_HIDDEN);activate_actor(r);}
    else r->flags&=~ACTOR_FLAG_HIDDEN;
    lf_frame(r,f);
}

void td_life_present(void) BANKED {
    UBYTE i,bit;actor_t *a;
    /* Owned road vehicles: impact slides, pursuit and stolen cars. */
    if(td_tr_ctrl)for(i=0,bit=1,a=&actors[2];i<6;i++,bit<<=1,a++){
        if(!(td_tr_ctrl&bit))continue;
        if(tr_mode[i]==TR_GONE){a->flags|=ACTOR_FLAG_HIDDEN;continue;}
        lf_place_q4(a,td_traffic_u[i],td_traffic_v[i]);
        /* A pursuing patrol car flashes its light bar red and blue. */
        if(LF_IS_PATROL(i)){
            /* A cruiser flashes its light bar red and blue; the SUV shows
             * red and navy; the unmarked car stays navy with a red dash
             * light now and then. */
            lf_frame(a,(lf_unit==LF_UNIT_SUV?TD_FRAME_PLAYER_VAN:lf_unit==LF_UNIT_UNMARKED?TD_FRAME_PLAYER_CAR:TD_FRAME_POLICE)+tr_head[i]);
            TD_PALETTE(a)=!td.wanted?TD_PAL_BLUE:lf_unit==LF_UNIT_CRUISER?((td_tick&8)?TD_PAL_RED:TD_PAL_BLUE):
                          lf_unit==LF_UNIT_SUV?((td_tick&8)?TD_PAL_RED:TD_PAL_NAVY):((td_tick&24)==24?TD_PAL_RED:TD_PAL_NAVY);
        }
        else lf_frame(a,td_traffic_bases[i]+tr_head[i]);
    }
    if(td_fx_kind){
        a=&actors[TD_ACTOR_FX];
        if(td_fx_kind==FX_DROP){
            /* The dropped magazine bobs, and blinks in its last second. */
            lf_place_q4(a,fx_u,fx_v-(((sys_time>>4)&1)<<4));lf_frame(a,TD_FRAME_PICKUP_AMMO);
            if(fx_timer<60&&(fx_timer&4)){a->flags|=ACTOR_FLAG_HIDDEN;goto drawn;}
        }else{
            lf_place_q4(a,fx_u,fx_v);
            lf_frame(a,td_fx_kind==FX_SPARK?TD_FRAME_SPARK:lf_look_walk[fx_look]+(fx_du>0?0:2)+((fx_timer>>2)&1));
        }
        /* GBVM re-checks an off-screen actor only every fourth frame; an
         * effect placed in view shows at once. */
        a->flags&=~(ACTOR_FLAG_HIDDEN|ACTOR_FLAG_DISABLED);
    }
drawn:
    td_shot_present();
    /* The courier lies down when knocked out, flashes when hit. */
    if(lf_down&&td.onfoot)lf_frame(&PLAYER,TD_FRAME_KNOCK+4);
    if(lf_hurt&&!lf_down){if(lf_hurt&4)PLAYER.flags|=ACTOR_FLAG_HIDDEN;else PLAYER.flags&=~ACTOR_FLAG_HIDDEN;}
    else if(lf_flash){lf_flash=0;PLAYER.flags&=~ACTOR_FLAG_HIDDEN;}
    /* The impact shake is applied with the camera look-ahead (td_anim.c). */
    lf_beacon();lf_marker();
}

