#pragma bank 255
/* Street-life presentation: owned walkers and vehicles, effects, the
 * courier's hurt/knock-out poses and the objective beacon or pointer. */
#include <string.h>
#include "td_life_int.h"
#include "td_audio.h"
#include "td_district.h"
#include "camera.h"
#include "input.h"
#include "system.h"

void td_life_peds(UBYTE near) BANKED {
    UBYTE i,bit,a,mode,f,hop;actor_t *p;
    near&=~pk_fresh;
    if(!near&&!pk_fresh&&!td_ped_ovr)return;
    a=td.speed<0?(UBYTE)-td.speed:(UBYTE)td.speed;
    /* A moving car throws struck walkers; one consequence per person. */
    if(near&&td.mode==TD_ROAM&&!td.onfoot&&a>=3){
        for(i=0,bit=1;i<6;i++,bit<<=1){
            if(!(near&bit))continue;
            td_lf_knock(i,lf_div16(td_vx+(td_vx>>1)),lf_div16(td_vy+(td_vy>>1)),a>=16);
            td.speed-=lf_div4(td.speed);td_vx-=lf_div4(td_vx);td_vy-=lf_div4(td_vy);
            if(td.job!=TD_NONE&&td.stage)td.health=td.health>10?td.health-10:0;
            td_audio_play(TD_AUDIO_IMPACT);td_message(TD_MSG_PED);
        }
    }
    /* New route walkers stay hidden (native layout) until they are out of
     * view, so nobody appears in the middle of the screen. */
    if(pk_fresh&&!(td_tick&7))for(i=0,bit=1,p=&actors[9];i<6;i++,bit<<=1,p++){
        if(!(pk_fresh&bit))continue;
        if(td_ped_route[i]==TD_NONE||!lf_on_screen(p->pos.x>>5,p->pos.y>>5))pk_fresh&=~bit;
    }
    if(!td_ped_ovr)return;
    for(i=0,bit=1,p=&actors[9];i<6;i++,bit<<=1,p++){
        if(!(td_ped_ovr&bit))continue;
        mode=pk_mode[i];hop=0;
        /* A body on the ground does not move: draw it once. */
        if(pk_drawn&bit)continue;
        if(mode==PK_FLY){
            /* Arc: height grows then falls over the flight. */
            UBYTE t=pk_timer[i],s=pk_span[i];UWORD h=(UWORD)t*(UBYTE)(s-t);hop=(UBYTE)((h+(h<<1))>>5);
            f=TD_FRAME_KNOCK_WALKER_A+pk_look[i]*TD_KNOCK_FRAMES+((t>>2)&3);
        }else if(mode==PK_DOWN||mode==PK_DEAD){
            f=TD_FRAME_KNOCK_WALKER_A+pk_look[i]*TD_KNOCK_FRAMES+4+(i&1);
            lf_place_q4(p,pk_u[i],pk_v[i]);lf_frame(p,f);p->flags&=~ACTOR_FLAG_HIDDEN;pk_drawn|=bit;continue;
        }else{
            static const UBYTE dir_frame[4]={0,2,4,6};
            f=lf_look_walk[pk_look[i]]+dir_frame[pk_dir[i]&3]+((td_tick>>3)&1);
        }
        lf_place_q4(p,pk_u[i],pk_v[i]-((UWORD)hop<<4));lf_frame(p,f);
        if(lf_on_screen(pk_u[i]>>4,pk_v[i]>>4))p->flags&=~ACTOR_FLAG_HIDDEN;
        else p->flags|=ACTOR_FLAG_HIDDEN;
    }
}

static void lf_beacon(void){
    actor_t *b=&actors[1];WORD x,y;UWORD ax,ay,pu=td.u>>4,pv=td.v>>4;UBYTE east,south,d;
    if(!td_beacon_shown){b->flags|=ACTOR_FLAG_HIDDEN;return;}
    x=(WORD)(td_beacon_u-(UWORD)scroll_x);y=(WORD)(td_beacon_v-(UWORD)scroll_y);
    if((UWORD)(x-4)<152&&(UWORD)(y-8)<108){
        lf_place(b,td_beacon_u,td_beacon_v-12-((sys_time>>4)&1));
        lf_frame(b,TD_FRAME_BEACON+((sys_time>>4)&1));
    }else{
        /* Off screen: a pointer on the view edge toward the objective. */
        east=td_beacon_u>=pu;south=td_beacon_v>=pv;
        ax=east?td_beacon_u-pu:pu-td_beacon_u;ay=south?td_beacon_v-pv:pv-td_beacon_v;
        if(ax>(ay<<1))d=east?0:4;
        else if(ay>(ax<<1))d=south?2:6;
        else d=south?(east?1:3):(east?7:5);
        x=x<10?10:x>148?148:x;y=y<14?14:y>110?110:y;
        lf_place(b,scroll_x+x+4,scroll_y+y+4);
        lf_frame(b,TD_FRAME_ARROW+d);
    }
    b->flags&=~ACTOR_FLAG_HIDDEN;
}

void td_life_present(void) BANKED {
    UBYTE i,bit;actor_t *a;
    /* Owned road vehicles: impact slides, pursuit and stolen cars. */
    if(td_tr_ctrl)for(i=0,bit=1,a=&actors[2];i<6;i++,bit<<=1,a++){
        if(!(td_tr_ctrl&bit))continue;
        if(tr_mode[i]==TR_GONE){a->flags|=ACTOR_FLAG_HIDDEN;continue;}
        lf_place_q4(a,td_traffic_u[i],td_traffic_v[i]);lf_frame(a,lf_traffic_base[i]+tr_head[i]);
    }
    if(td_fx_kind){
        a=&actors[TD_ACTOR_GULL];
        lf_place_q4(a,fx_u,fx_v);
        lf_frame(a,td_fx_kind==FX_SPARK?TD_FRAME_SPARK:td_fx_kind==FX_BULLET?TD_FRAME_BULLET:
                   lf_look_walk[fx_look]+(fx_du>0?0:2)+((fx_timer>>2)&1));
        a->flags&=~ACTOR_FLAG_HIDDEN;
    }
    /* The courier lies down when knocked out, flashes when hit. */
    if(lf_down&&td.onfoot)lf_frame(&PLAYER,TD_FRAME_KNOCK_WALKER_A+5*TD_KNOCK_FRAMES+4);
    if(lf_hurt&&!lf_down){if(lf_hurt&4)PLAYER.flags|=ACTOR_FLAG_HIDDEN;else PLAYER.flags&=~ACTOR_FLAG_HIDDEN;}
    else if(lf_flash){lf_flash=0;PLAYER.flags&=~ACTOR_FLAG_HIDDEN;}
    if(lf_shake){lf_shake--;camera_offset_x=lf_shake?((lf_shake&2)?2:-2):0;}
    lf_beacon();
}

