#pragma bank 255
/* Courier vehicle handling, collisions with walls and road vehicles, and
 * the courier's on-foot actions: car theft, punches and the pistol. */
#include <string.h>
#include "td_life_int.h"
#include "td_audio.h"
#include "td_district.h"
#include "td_anim.h"
#include "camera.h"
#include "input.h"
#include "system.h"

static const BYTE lf_dx[16]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
static const BYTE lf_dy[16]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};
static const BYTE lf_face_u[4]={1,-1,0,0},lf_face_v[4]={0,0,1,-1};
static const UBYTE lf_mass_player[4]={4,7,2,2},lf_mass_slot[6]={4,7,4,2,5,7};
static WORD lf_div2(WORD x){return x<0?-(WORD)((UWORD)-x>>1):(WORD)((UWORD)x>>1);}
/* Heading x speed, cached while both are unchanged (the common cruise). */
static UBYTE lf_scale_key_h=255;static WORD lf_scale_key_s;
/* Target velocity for the current heading and speed (td_anim reads the slide). */
WORD lf_scale_x,lf_scale_y;
static WORD lf_scale(BYTE d,WORD s){
    UBYTE m=d<0?(UBYTE)-d:(UBYTE)d;WORD r=0;
    while(m){if(m&1)r+=s;s+=s;m>>=1;}
    return d<0?-r:r;
}
/* Vehicles may use roads, sidewalks and open lots; buildings, water and
 * rails (collision 15) stay solid. Whole-pixel centre, 11/13px bodies. */
static UBYTE lf_drive(UWORD u,UWORD v){
    if(u<8||v<8||u>1016||v>968)return FALSE;
    return lf_area((u-5)>>3,(u+5)>>3,(v-5)>>3,(v+5)>>3);
}

static UBYTE lf_body(UWORD u,UWORD v){
    if(u<8||v<8||u>1016||v>968)return FALSE;
    return lf_area((u-6)>>3,(u+6)>>3,(v-6)>>3,(v+6)>>3);
}

/* 1-D impact with restitution 1/2 along the dominant axis. Velocities are
 * Q4 per tick, positive towards the struck vehicle. */
static void lf_car_hits(UWORD old_u,UWORD old_v){
    UBYTE i,head,horizontal,pm,nm,sum;WORD du,dv,v1,v2,n1,n2,rel,sx,sy,keep;
    UBYTE near=td_near_cells();
    for(i=0;near;i++,near>>=1){
        if(!(near&1)||tr_mode[i]==TR_GONE)continue;
        du=(WORD)td_traffic_u[i]-(WORD)td.u;dv=(WORD)td_traffic_v[i]-(WORD)td.v;
        if(du>=192||du<=-192||dv>=192||dv<=-192)continue;
        horizontal=lf_abs(du)>=lf_abs(dv);
        sx=horizontal?(du>0?1:-1):0;sy=horizontal?0:(dv>0?1:-1);
        v1=lf_div16(horizontal?(sx>0?td_vx:-td_vx):(sy>0?td_vy:-td_vy));
        /* Moving apart is always allowed; closing never leaves the courier
         * overlapping another body. */
        if(v1<=0)continue;
        td.u=old_u;td.v=old_v;
        if(tr_mode[i]==TR_PUSH&&tr_timer[i]>10)continue;
        head=td_lf_tr_heading(i);
        v2=(tr_mode[i]&&tr_mode[i]!=TR_CHASE)?0:8;
        v2=((head==0&&sx>0)||(head==4&&sx<0)||(head==2&&sy>0)||(head==6&&sy<0))?v2:
           ((head==0&&sx<0)||(head==4&&sx>0)||(head==2&&sy<0)||(head==6&&sy>0))?-v2:0;
        pm=lf_mass_player[td.vehicle&3];nm=lf_mass_slot[i];sum=pm+nm;
        /* Restitution 3/4: a head-on or heavier body throws the car back. */
        rel=v1-v2;
        n1=((WORD)pm*v1+(WORD)nm*v2-((((WORD)nm*rel)*3)>>2))/sum;
        n2=((WORD)pm*v1+(WORD)nm*v2+((((WORD)pm*rel)*3)>>2))/sum;
        if(n2<6)n2=6;
        /* The courier keeps the transferred share; a head-on or heavier
         * body throws the car back for a visible rebound. */
        keep=n1<<4;
        if(horizontal)td_vx=sx>0?keep:-keep;else td_vy=sy>0?keep:-keep;
        /* A negative share rolls the car backwards: the visible rebound. */
        td.speed=(WORD)((td.speed*n1)/v1);
        td_lf_own_car(i,TR_PUSH);
        tr_pu[i]=lf_clamp(sx*n2,56);tr_pv[i]=lf_clamp(sy*n2,56);tr_timer[i]=18;
        /* A hard side impact spins the other car. */
        if(!v2&&n2>=14)tr_spin|=1<<i;
        if(rel>=6){
            td_anim_spawn(TD_PART_FLASH,0,(UWORD)((WORD)(td.u>>4)+(lf_div16(du)>>1)),(UWORD)((WORD)(td.v>>4)+(lf_div16(dv)>>1)));
            if(td.job!=TD_NONE&&td.stage){UBYTE damage=rel>=12?12:4;td.health=td.health>damage?td.health-damage:0;}
            td_audio_play(TD_AUDIO_IMPACT);td.cooldown=30;td_message(5);
            if(rel>=14)lf_shake=10;
            /* The driver is jolted: throttle returns after a moment, so the
             * rebound is visible even with A held. */
            if(rel>=10)lf_stun=12;
            /* Only a hard ram counts as an attack on the patrol car. */
            if(LF_IS_PATROL(i)&&rel>=12)td_lf_crime(CR_COP);
        }
    }
}

/* A quantised corner may clip the car although a parallel lane is open:
 * with throttle held (no brake), forward speed at least 3 and a dominant
 * travel axis, slide up to six pixels sideways once per rendered update. */
static UBYTE lf_corner_slide(WORD nu,WORD nv){
    UBYTE i,j,side,clear;WORD shift,shifted;UWORD ax=lf_abs(td_vx),ay=lf_abs(td_vy);
    if(!INPUT_A||INPUT_B||td.speed<3||ax==ay||td_corner_used)return FALSE;
    for(i=1;i<=6;i++)for(side=0;side<2;side++){
        shift=side?-(WORD)i*16:(WORD)i*16;
        if(ay>ax&&nv!=(WORD)td.v){
            shifted=(WORD)td.u+shift;
            if(shifted<128||shifted>1016*16||nv<128||nv>968*16||!lf_drive(shifted>>4,nv>>4))continue;
            clear=1;for(j=1;j<=i;j++)if(!lf_drive(((WORD)td.u+(side?-(WORD)j*16:(WORD)j*16))>>4,td.v>>4)){clear=0;break;}
            if(clear){td.u=shifted;td.v=nv;td_vx=0;td_corner_used=1;return TRUE;}
        }else if(ax>ay&&nu!=(WORD)td.u){
            shifted=(WORD)td.v+shift;
            if(shifted<128||shifted>968*16||nu<128||nu>1016*16||!lf_drive(nu>>4,shifted>>4))continue;
            clear=1;for(j=1;j<=i;j++)if(!lf_drive(td.u>>4,((WORD)td.v+(side?-(WORD)j*16:(WORD)j*16))>>4)){clear=0;break;}
            if(clear){td.u=nu;td.v=shifted;td_vy=0;td_corner_used=1;return TRUE;}
        }
    }
    return FALSE;
}

/* Dust off the bumper after a hard wall or kerb strike. */
static void lf_dust(UBYTE kind){
    UBYTE h=td.heading&15;
    td_anim_spawn(kind,0,(UWORD)((WORD)(td.u>>4)+((lf_dx[h]*7)>>4)),(UWORD)((WORD)(td.v>>4)+((lf_dy[h]*7)>>4)));
}

UBYTE td_life_drive(void) BANKED {
    WORD nu,nv,tx,ty;UBYTE limit,period,slide=0,a,gas,brake,hand,result=0,red;UWORD u,v,old_u=td.u,old_v=td.v;BYTE dir;
    if(td.cooldown)td.cooldown--;
    if(td_red_cooldown)td_red_cooldown--;
    gas=!!INPUT_A;brake=!!INPUT_B;hand=gas&&brake;
    if(lf_stun){lf_stun--;gas=0;}
    if(hand)gas=brake=0;
    if(td_life_locked()){gas=brake=0;hand=1;}
    limit=td.vehicle==1?20:td.vehicle==2?28:td.vehicle==3?18:24;
    /* Throttle builds quickly from rest and tapers near top speed; the brake
     * stops the car first and only engages reverse after a short hold. */
    if(gas){
        if(td.speed<0)td.speed++;
        else if(td.speed<limit){
            UBYTE mask=td.speed<(limit>>1)?1:td.speed<limit-(limit>>2)?3:7;
            if(!(td_tick&mask))td.speed++;
        }
        lf_rev_wait=10;
    }else if(brake){
        if(td.speed>0){td.speed--;lf_rev_wait=10;}
        else if(lf_rev_wait)lf_rev_wait--;
        else if(td.speed>-(WORD)(limit/3)&&!(td_tick&3))td.speed--;
    }else if(hand){
        if(!(td_tick&1)){if(td.speed>0)td.speed--;else if(td.speed<0)td.speed++;}
        lf_rev_wait=10;
    }else{
        if(!(td_tick&7)){if(td.speed>0)td.speed--;else if(td.speed<0)td.speed++;}
        lf_rev_wait=10;
    }
    /* Steering needs genuine movement: the car never rotates at rest. The
     * yaw rate grows with speed, peaks in town traffic and eases at top speed;
     * reversing swings the nose the other way, as a real car does. */
    a=td.speed<0?(UBYTE)-td.speed:(UBYTE)td.speed;
    if(a&&!!INPUT_LEFT!=!!INPUT_RIGHT){
        period=a<3?14:a<7?9:a<15?6:a<21?7:8;
        if(td.vehicle==1)period+=3;else if(td.vehicle>=2)period--;
        if(hand&&a>5)period>>=1;
        if(++td_turn_tick>=period){
            td_turn_tick=0;dir=INPUT_LEFT?-1:1;if(td.speed<0)dir=-dir;
            td.heading=(td.heading+dir)&15;
            if(td.job!=TD_NONE&&td.stage&&td_job.kind==5&&a>18){if(td.health)td.health--;td_message(14);}
        }
    }else td_turn_tick=0;
    /* Traction eases velocity toward the heading; the handbrake (A+B)
     * loosens grip so the tail slides. */
    if(td.heading!=lf_scale_key_h||td.speed!=lf_scale_key_s){
        lf_scale_key_h=td.heading;lf_scale_key_s=td.speed;
        lf_scale_x=lf_scale(lf_dx[td.heading],td.speed);lf_scale_y=lf_scale(lf_dy[td.heading],td.speed);
    }
    tx=lf_scale_x;ty=lf_scale_y;
    if(hand&&a>6){td_vx+=lf_div16(tx-td_vx);td_vy+=lf_div16(ty-td_vy);}
    else{td_vx+=lf_div4(tx-td_vx);td_vy+=lf_div4(ty-td_vy);}
    if(!td.speed){td_vx=lf_div2(td_vx);td_vy=lf_div2(td_vy);}
    nu=td.u+lf_div16(td_vx);nv=td.v+lf_div16(td_vy);u=nu>>4;v=nv>>4;
    if(lf_drive(u,v)){
        if(td.district==0&&!td_red_cooldown&&a>6&&lf_dist(u,640)<14&&lf_dist(v,528)<14&&
          (lf_dist(td.u>>4,640)>=14||lf_dist(td.v>>4,528)>=14)){
            UBYTE ax=lf_abs(lf_dx[td.heading]),ay=lf_abs(lf_dy[td.heading]);
            red=(UBYTE)(td.seconds%12);
            if((ax>ay&&red>=7)||(ax<=ay&&red<7)){if(td.cash>=5)td.cash-=5;td_red_cooldown=120;td_message(7);}
        }
        td.u=nu;td.v=nv;
    }else{
        /* A glancing curb contact slides along the free axis. */
        if(nu!=(WORD)td.u&&lf_drive(nu>>4,td.v>>4)){td.u=nu;td_vy=0;slide=1;}
        if(nv!=(WORD)td.v&&lf_drive(td.u>>4,nv>>4)){td.v=nv;td_vx=0;slide=1;}
        if(!slide)slide=lf_corner_slide(nu,nv);
        if(slide){if(a>8&&!td.cooldown){lf_dust(TD_PART_PUFF);td.cooldown=30;if(td.job!=TD_NONE&&td.stage){UBYTE damage=td_job.kind==1?4:1;td.health=td.health>damage?td.health-damage:0;}td_message(5);}}
        else{
            if(a>8&&!td.cooldown){lf_dust(TD_PART_SMOKE);if(td.job!=TD_NONE&&td.stage){UBYTE damage=td_job.kind==1?20:8;td.health=td.health>damage?td.health-damage:0;}td.cooldown=45;td_message(5);if(a>14)lf_shake=8;}
            /* A solid wall throws a fast car back a little before it settles. */
            if(a>10){td.speed=-lf_div4(td.speed);td_vx=-lf_div4(td_vx);td_vy=-lf_div4(td_vy);}
            else{td.speed=0;td_vx=td_vy=0;}
        }
    }
    lf_car_hits(old_u,old_v);
    /* Hold A+B at rest to leave the vehicle. */
    if(hand&&!td.speed&&lf_abs(td_vx)<32&&lf_abs(td_vy)<32){if(++lf_exit_hold==18)result|=TD_DRIVE_EXIT;}
    else lf_exit_hold=0;
    return result;
}

static UBYTE lf_carjack(void){
    UBYTE i,best=TD_NONE,base,near=td_near_cells();UWORD d,best_d=65535;WORD du,dv;
    for(i=0;near;i++,near>>=1){
        if(!(near&1)||tr_mode[i]==TR_GONE)continue;
        du=(WORD)td_traffic_u[i]-(WORD)td.u;dv=(WORD)td_traffic_v[i]-(WORD)td.v;
        if(lf_abs(du)>=256||lf_abs(dv)>=256)continue;
        d=lf_abs(du)+lf_abs(dv);if(d<best_d){best_d=d;best=i;}
    }
    if(best==TD_NONE)return FALSE;
    base=lf_traffic_base[best];
    td.vehicle=base==TD_FRAME_TRAFFIC_VAN?1:base==TD_FRAME_TRAFFIC_MOTORCYCLE?2:0;
    td.heading=td_lf_tr_heading(best)<<1;
    td.park_u=td_traffic_u[best];td.park_v=td_traffic_v[best];td.park_district=td.district;
    td_lf_own_car(best,TR_GONE);actors[2+best].flags|=ACTOR_FLAG_HIDDEN;
    /* The driver bails out and runs off the far side. */
    fx_look=LF_IS_PATROL(best)?4:best&3;
    td_lf_fx(FX_RUNNER,td.park_u>>4,td.park_v>>4,54);
    fx_du=td.park_u>td.u?12:-12;fx_dv=0;
    td_entry_target=0;td_entry_timer=12;td.speed=0;td_vx=td_vy=0;
    td_audio_play(TD_AUDIO_IMPACT);
    td_lf_crime(LF_IS_PATROL(best)?CR_COP:CR_MINOR);
    td_message(TD_MSG_CARJACK);
    return TRUE;
}

static void lf_punch_now(void){
    UBYTE i,best=TD_NONE,d=td_walk_dir&3,bit;WORD fu,fv,du,dv;actor_t *a;
    if(lf_punch)return;
    lf_punch=16;td_anim_pose(TD_FRAME_COURIER_PUNCH,10);
    fu=(WORD)(td.u>>4)+lf_face_u[d]*7;fv=(WORD)(td.v>>4)+lf_face_v[d]*7;
    for(i=0,bit=1,a=&actors[TD_ACTOR_PEDS];i<TD_PEDS;i++,bit<<=1,a++){
        if(a->flags&ACTOR_FLAG_HIDDEN)continue;
        if((td_ped_ovr&bit)&&(pk_mode[i]==PK_FLY||pk_mode[i]==PK_DEAD||pk_mode[i]==PK_DOWN))continue;
        du=(WORD)(a->pos.x>>5)-fu;dv=(WORD)(a->pos.y>>5)-fv;
        if(lf_abs(du)<9&&lf_abs(dv)<10){best=i;break;}
    }
    td_lf_fx(FX_SPARK,fu,fv,6);
    td_audio_play(TD_AUDIO_MENU);
    if(best!=TD_NONE){td_lf_knock(best,lf_face_u[d]*18,lf_face_v[d]*18,0);td_audio_play(TD_AUDIO_IMPACT);}
}

void td_life_foot_a(void) BANKED {
    if(td_life_locked()||td_entry_timer)return;
    if(!lf_carjack())lf_punch_now();
}

void td_life_foot_b(void) BANKED {
    UBYTE d=td_walk_dir&3;
    if(td_life_locked()||td_entry_timer||lf_punch)return;
    if(!td.ammo){td_message(TD_MSG_NO_AMMO);return;}
    td.ammo--;lf_punch=14;td_anim_pose(TD_FRAME_COURIER_SHOOT,12);
    td_anim_spawn(TD_PART_FLASH,0,(td.u>>4)+lf_face_u[d]*9,(td.v>>4)+lf_face_v[d]*9);
    td_lf_fx(FX_BULLET,(td.u>>4)+lf_face_u[d]*6,(td.v>>4)+lf_face_v[d]*6,22);
    fx_du=lf_face_u[d]*64;fx_dv=lf_face_v[d]*64;
    td_audio_play(TD_AUDIO_IMPACT);
    td_lf_crime(CR_GUN);
}

UBYTE td_lf_drive(UWORD u,UWORD v) BANKED {return lf_drive(u,v);}
UBYTE td_lf_body(UWORD u,UWORD v) BANKED {return lf_body(u,v);}
