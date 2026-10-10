#pragma bank 255
/* Rounds in flight: the courier's pistol and the police's. A round is a
 * projectile, 6 px per rendered update in two 3-pixel collision steps, fired
 * with a little random spread; it can miss a moving target, and an incoming
 * round can be dodged by moving. It stops at a building, a person or a
 * vehicle. Officers wear vests: the first of the courier's rounds staggers
 * one, the second puts them down. Everything here is transient. */
#include <string.h>
#include "td_life_int.h"
#include "td_shots.h"
#include "td_audio.h"
#include "camera.h"

/* Heads relative to the actor point, per heading (create_sprites.py). */
static const BYTE sh_head_dx[8]=TD_SHOT_HEAD_DX;
static const BYTE sh_head_dy[8]=TD_SHOT_HEAD_DY;
#define SH_STEP 48   /* Q4 per collision step: 3 px */
#define SH_LIFE 28   /* updates: 168 px of flight */
UBYTE td_shot_live;
static UBYTE sh_life[TD_SHOTS],sh_owner[TD_SHOTS],sh_dir[TD_SHOTS];
static UWORD sh_u[TD_SHOTS],sh_v[TD_SHOTS];
static BYTE sh_du[TD_SHOTS],sh_dv[TD_SHOTS];
static UWORD sh_rng=0x5EED;
static UBYTE sh_random(void){
    sh_rng^=sh_rng<<7;sh_rng^=sh_rng>>9;sh_rng^=sh_rng<<8;
    return (UBYTE)sh_rng^(UBYTE)td_tick;
}
/* Eight headings (E=0 clockwise, +y south) from a velocity. */
static UBYTE sh_heading8(WORD dx,WORD dy){
    UWORD ax=lf_abs(dx),ay=lf_abs(dy);UBYTE k;
    /* tan 22.5 is about 0.41: ratios beyond 5/2 are cardinal. */
    if((ay<<1)+(ay>>1)<ax)k=0;else if((ax<<1)+(ax>>1)<ay)k=2;else k=1;
    if(dx>=0)return dy>=0?k:(UBYTE)(8-k)&7;
    return dy>=0?4-k:4+k;
}

void td_shot_reset(void) BANKED {
    UBYTE k;actor_t *a=&actors[TD_ACTOR_SHOTS];
    td_shot_live=0;memset(sh_life,0,sizeof(sh_life));
    for(k=0;k<TD_SHOTS;k++,a++){a->flags|=ACTOR_FLAG_HIDDEN;if(a->flags&ACTOR_FLAG_ACTIVE)deactivate_actor(a);}
}

UBYTE td_shot_fire(UBYTE owner,UWORD u,UWORD v,UWORD tu,UWORD tv,UBYTE spread) BANKED {
    UBYTE k;WORD du=(WORD)tu-(WORD)u,dv=(WORD)tv-(WORD)v,len;BYTE r=0;
    for(k=0;k<TD_SHOTS&&sh_life[k];k++);
    if(k==TD_SHOTS)return FALSE;
    if(!du&&!dv)du=1;
    /* Keep the products below inside sixteen bits. */
    while(du>255||du<-255||dv>255||dv<-255){du>>=1;dv>>=1;}
    len=(WORD)(lf_abs(du)>lf_abs(dv)?lf_abs(du)+((lf_abs(dv)*3)>>3):lf_abs(dv)+((lf_abs(du)*3)>>3));
    if(!len)len=1;
    if(spread){r=(BYTE)(sh_random()%(UBYTE)(spread*2+1))-(BYTE)spread;}
    /* Along the aim at SH_STEP, plus r/SH_STEP of it across. */
    sh_du[k]=(BYTE)(((WORD)du*SH_STEP-(WORD)dv*r)/len);
    sh_dv[k]=(BYTE)(((WORD)dv*SH_STEP+(WORD)du*r)/len);
    sh_u[k]=u<<4;sh_v[k]=v<<4;sh_owner[k]=owner;sh_life[k]=SH_LIFE;
    sh_dir[k]=sh_heading8(sh_du[k],sh_dv[k]);
    td_shot_live|=1<<k;
    return TRUE;
}

/* A round at (bu,bv), whole pixels: TRUE when it struck someone or something. */
static UBYTE sh_hit(UBYTE k,UWORD bu,UWORD bv){
    UBYTE i,bit,look,owner=sh_owner[k];actor_t *a;UWORD pu,pv,reach;
    for(i=0,bit=1,a=&actors[TD_ACTOR_PEDS];i<TD_PEDS;i++,bit<<=1,a++){
        if(a->flags&ACTOR_FLAG_HIDDEN)continue;
        if((td_ped_ovr&bit)&&(pk_mode[i]==PK_FLY||pk_mode[i]==PK_DEAD||pk_mode[i]==PK_DOWN))continue;
        if(lf_dist(a->pos.x>>5,bu)>=5||lf_dist(a->pos.y>>5,bv)>=7)continue;
        look=(td_ped_ovr&bit)?pk_look[i]:(td_ped_route[i]!=TD_NONE?td_ped_route[i]&7:0);
        if(owner==TD_SHOT_COURIER&&look==LF_LOOK_OFFICER&&!(lf_vest&bit)){td_lf_stagger(i,sh_du[k]>>3,sh_dv[k]>>3);if(td_hitstop<2)td_hitstop=2;}
        else td_lf_knock(i,sh_du[k]>>2,sh_dv[k]>>2,owner==TD_SHOT_COURIER?1:3);
        td_lf_fx(FX_SPARK,bu,bv,6);
        return TRUE;
    }
    if(owner==TD_SHOT_POLICE&&td.mode==TD_ROAM){
        pu=td.u>>4;pv=td.v>>4;reach=td.onfoot?5:9;
        /* A rolling courier lets the round pass. */
        if(!td_rolling&&lf_dist(pu,bu)<reach&&lf_dist(pv,bv)<reach+2){
            /* Just hit: a moment's grace, and the round flies past. */
            if(lf_hurt>14)return FALSE;
            td_lf_hurt(td.onfoot?4+td.wanted:(UBYTE)((4+td.wanted)>>1));
            td_lf_fx(FX_SPARK,bu,bv,6);
            return TRUE;
        }
    }
    /* The courier's parked car is cover: a police round stops in it. */
    if(td.onfoot&&td.park_district==td.district&&lf_dist(td.park_u>>4,bu)<9&&lf_dist(td.park_v>>4,bv)<9){
        if(owner==TD_SHOT_POLICE){
            if(td_car_damage<TD_DAMAGE_WRECK-1)td_car_damage++;
            if(lf_dist(td.u>>4,bu)<40&&lf_dist(td.v>>4,bv)<40)td_message(TD_MSG_COVER);
        }
        td_lf_fx(FX_SPARK,bu,bv,6);
        return TRUE;
    }
    for(i=0;i<6;i++){
        if(tr_mode[i]==TR_GONE)continue;
        if(lf_dist(td_traffic_u[i]>>4,bu)>=9||lf_dist(td_traffic_v[i]>>4,bv)>=9)continue;
        if(owner==TD_SHOT_POLICE&&lf_dist(td.u>>4,bu)<40&&lf_dist(td.v>>4,bv)<40)td_message(TD_MSG_COVER);
        if(owner==TD_SHOT_COURIER&&LF_IS_POLICE(i))td_lf_crime(CR_COP);
        /* A civilian driver under fire floors it. */
        else if(owner==TD_SHOT_COURIER&&!(td_tr_ctrl&(1<<i))){td_lf_own_car(i,TR_FLEE);tr_timer[i]=200;}
        td_lf_fx(FX_SPARK,bu,bv,6);
        return TRUE;
    }
    return FALSE;
}

void td_shot_tick(void) BANKED {
    UBYTE k,step,bit;UWORD bu,bv;
    if(!td_shot_live)return;
    for(k=0,bit=1;k<TD_SHOTS;k++,bit<<=1){
        if(!sh_life[k])continue;
        if(!--sh_life[k]){td_shot_live&=~bit;continue;}
        for(step=0;step<2;step++){
            sh_u[k]+=sh_du[k];sh_v[k]+=sh_dv[k];bu=sh_u[k]>>4;bv=sh_v[k]>>4;
            if(!lf_walk(bu,bv)){td_lf_fx(FX_SPARK,bu,bv,6);sh_life[k]=0;td_shot_live&=~bit;break;}
            if(sh_hit(k,bu,bv)){sh_life[k]=0;td_shot_live&=~bit;break;}
        }
    }
}

void td_shot_present(void) BANKED {
    UBYTE k,d,f;actor_t *a=&actors[TD_ACTOR_SHOTS];
    for(k=0;k<TD_SHOTS;k++,a++){
        if(!sh_life[k]){
            if(a->flags&ACTOR_FLAG_ACTIVE){a->flags|=ACTOR_FLAG_HIDDEN;deactivate_actor(a);}
            continue;
        }
        d=sh_dir[k];f=TD_FRAME_SHOT+d;
        /* The head sits on the round at chest height, its trail behind. */
        lf_place(a,(UWORD)((sh_u[k]>>4)-sh_head_dx[d]),(UWORD)((sh_v[k]>>4)-7-sh_head_dy[d]));
        /* Police rounds burn red (offset from the round's yellow palette). */
        TD_PALETTE(a)=sh_owner[k]==TD_SHOT_POLICE?(UBYTE)(TD_PAL_RED-TD_PAL_YELLOW):0;
        if(!(a->flags&ACTOR_FLAG_ACTIVE)){
            a->flags&=~(ACTOR_FLAG_DISABLED|ACTOR_FLAG_HIDDEN);activate_actor(a);
            a->frame=a->frame_start=f;a->frame_end=f+1;a->anim_tick=255;
        }else{
            a->flags&=~(ACTOR_FLAG_DISABLED|ACTOR_FLAG_HIDDEN);
            lf_frame(a,f);
        }
    }
}
