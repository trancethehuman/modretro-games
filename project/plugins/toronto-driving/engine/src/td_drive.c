#pragma bank 255
/* Courier vehicle handling, collisions with walls and road vehicles, and
 * the courier's on-foot actions: car theft, punches and the pistol. */
#include <string.h>
#include "td_life_int.h"
#include "td_audio.h"
#include "td_district.h"
#include "td_world.h"
#include "td_anim.h"
#include "td_shots.h"
#include "camera.h"
#include "input.h"
#include "system.h"

static const BYTE lf_dx[16]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
static const BYTE lf_dy[16]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};
static const BYTE lf_face_u[4]={1,-1,0,0},lf_face_v[4]={0,0,1,-1};
static const UBYTE lf_mass_player[4]={4,7,2,2};
/* Mass of a traffic design: vans 7, pickups 6, motorcycles 2, cars 4. */
static UBYTE lf_mass_slot(UBYTE i){
    UBYTE b=td_traffic_bases[i];
    if(LF_IS_PATROL(i))return 5;
    return b==TD_FRAME_PLAYER_VAN?7:b==TD_FRAME_TRAFFIC_PICKUP?6:b==TD_FRAME_PLAYER_MOTORCYCLE?2:4;
}
UBYTE td_car_damage,td_car_colour;
/* Wear on the courier's car; vans shrug off half. The first warning comes
 * when the engine starts failing, another when it is wrecked. */
static void lf_wear(UBYTE points){
    UBYTE before=td_car_damage;
    if(td.vehicle==1)points=(points+1)>>1;
    td_car_damage=points>=TD_DAMAGE_WRECK-td_car_damage?TD_DAMAGE_WRECK:td_car_damage+points;
    if(before<TD_DAMAGE_WRECK&&td_car_damage==TD_DAMAGE_WRECK)td_message(TD_MSG_WRECKED);
    else if(before<TD_DAMAGE_FAIL&&td_car_damage>=TD_DAMAGE_FAIL)td_message(TD_MSG_SMOKING);
}
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
 * rails (collision 15) stay solid. Whole-pixel centre, 13/15px bodies for
 * the 20x14 full-size cars. */
static UBYTE lf_drive(UWORD u,UWORD v){
    if(u<8||v<8||u>1016||v>968)return FALSE;
    return lf_area((u-6)>>3,(u+6)>>3,(v-6)>>3,(v+6)>>3);
}

static UBYTE lf_body(UWORD u,UWORD v){
    if(u<8||v<8||u>1016||v>968)return FALSE;
    return lf_area((u-7)>>3,(u+7)>>3,(v-7)>>3,(v+7)>>3);
}

/* 1-D impact with restitution 1/2 along the dominant axis. Velocities are
 * Q4 per tick, positive towards the struck vehicle. */
/* Lateral offsets (Q4) between centres for a glancing clip: bodies collide
 * within 192, so 120 leaves less than 5 px of overlap. */
#define LF_CLIP_SURE 120
#define LF_CLIP_MAYBE 84
static void lf_car_hits(UWORD old_u,UWORD old_v){
    UBYTE i,head,horizontal,pm,nm,sum;WORD du,dv,v1,v2,n1,n2,rel,sx,sy,keep,shove;UWORD lat;BYTE side;
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
        /* A glancing blow clips the other car instead of stopping dead: the
         * bodies overlap by a few pixels across the line of travel (always
         * from LF_CLIP_SURE, every other time from LF_CLIP_MAYBE). The
         * courier is pushed clear sideways and keeps most of its speed; the
         * other car is shoved aside and may spin. */
        lat=horizontal?lf_abs(dv):lf_abs(du);
        if(lat>=LF_CLIP_SURE||(lat>=LF_CLIP_MAYBE&&(td_tick&1))){
            if(tr_mode[i]==TR_PUSH&&tr_timer[i]>8)continue;
            side=(horizontal?dv:du)>0?-1:1;
            shove=(WORD)(196-lat)*side;
            if(horizontal){if(lf_drive(td.u>>4,(td.v+shove)>>4))td.v+=shove;else{td.u=old_u;td.v=old_v;}td_vy+=side*48;}
            else{if(lf_drive((td.u+shove)>>4,td.v>>4))td.u+=shove;else{td.u=old_u;td.v=old_v;}td_vx+=side*48;}
            td.speed-=td.speed>>3;
            td_lf_own_car(i,TR_PUSH);
            tr_pu[i]=lf_clamp(horizontal?sx*(v1>>1):-side*(6+(v1>>2)),40);
            tr_pv[i]=lf_clamp(horizontal?-side*(6+(v1>>2)):sy*(v1>>1),40);
            tr_timer[i]=14;if(v1>=8)tr_spin|=1<<i;
            td_anim_spawn(TD_PART_FLASH,0,(UWORD)((WORD)(td.u>>4)+(lf_div16(du)>>1)),(UWORD)((WORD)(td.v>>4)+(lf_div16(dv)>>1)));
            td_audio_play(TD_AUDIO_IMPACT);lf_wear((UBYTE)(v1>>2)+1);
            if(td.job!=TD_NONE&&td.stage)td.health=td.health>3?td.health-3:0;
            if(v1>=12)lf_shake=4;
            if(LF_IS_PATROL(i)&&v1>=12)td_lf_crime(CR_COP);
            continue;
        }
        td.u=old_u;td.v=old_v;
        if(tr_mode[i]==TR_PUSH&&tr_timer[i]>10)continue;
        head=td_lf_tr_heading(i);
        v2=(tr_mode[i]&&tr_mode[i]!=TR_CHASE)?0:8;
        v2=((head==0&&sx>0)||(head==4&&sx<0)||(head==2&&sy>0)||(head==6&&sy<0))?v2:
           ((head==0&&sx<0)||(head==4&&sx>0)||(head==2&&sy<0)||(head==6&&sy>0))?-v2:0;
        pm=lf_mass_player[td.vehicle&3];nm=lf_mass_slot(i);sum=pm+nm;
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
            td_audio_play(TD_AUDIO_IMPACT);td.cooldown=30;td_message(5);lf_wear((UBYTE)rel);
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

/* Handling (sixteenths of a speed unit per tick, by vehicle: car, van,
 * motorcycle, scooter). Throttle pulls hardest from rest and tapers towards
 * top speed; vans are heavy and take longest to start and stop. */
static const UBYTE lf_top[4]={20,17,23,15};
static const UBYTE lf_pull[4][3]={{4,3,2},{3,2,1},{6,4,3},{5,3,2}};
static const UBYTE lf_stop[4]={10,7,11,10};
#define LF_ROLL 2
#define LF_REVERSE 3
/* Brake force for the A+B exit and for throttle against reverse. */
#define LF_HALT 32
#define LF_CHORD 6
UBYTE lf_spd_acc,lf_spd_ctl,lf_a_age=255,lf_b_age=255,lf_exit_req;
/* Move speed towards zero (sign of dir) by rate sixteenths; never past it. */
static void lf_speed_add(BYTE dir,UBYTE rate){
    lf_spd_acc+=rate;
    while(lf_spd_acc>=16){
        lf_spd_acc-=16;
        if(dir>0)td.speed++;
        else if(dir<0)td.speed--;
        else if(td.speed>0)td.speed--;else if(td.speed<0)td.speed++;
    }
}

UBYTE td_life_drive(void) BANKED {
    WORD nu,nv,tx,ty;UBYTE limit,period,slide=0,a,gas,brake,drift,result=0,red,ctl,kind=td.vehicle&3;UWORD u,v,old_u=td.u,old_v=td.v;BYTE dir;
    if(td.cooldown)td.cooldown--;
    if(td_red_cooldown)td_red_cooldown--;
    gas=!!INPUT_A;brake=!!INPUT_B;
    /* A and B pressed together (within a few ticks of each other) gets the
     * courier out: the car brakes to a stop and the door opens. Holding the
     * throttle and then pressing B is ordinary braking. */
    if(td_input_edge&&INPUT_A_PRESSED)lf_a_age=0;else if(lf_a_age<255)lf_a_age++;
    if(td_input_edge&&INPUT_B_PRESSED)lf_b_age=0;else if(lf_b_age<255)lf_b_age++;
    if(gas&&brake&&lf_a_age<LF_CHORD&&lf_b_age<LF_CHORD&&!lf_exit_req){lf_exit_req=1;lf_a_age=lf_b_age=255;}
    /* Otherwise the brake wins when both are held. */
    if(lf_exit_req||brake)gas=0;
    if(lf_exit_req)brake=0;
    if(lf_stun){lf_stun--;gas=0;}
    if(td_life_locked()){gas=brake=0;lf_exit_req=0;}
    limit=lf_top[kind];
    /* A failing engine loses a quarter of its top speed; a wreck crawls. */
    if(td_car_damage>=TD_DAMAGE_WRECK)limit=5;else if(td_car_damage>=TD_DAMAGE_FAIL)limit-=limit>>2;
    /* Speed changes accumulate, so the car gathers and sheds speed with its
     * weight; the brake stops the car first and only engages reverse after a
     * short hold. */
    ctl=lf_exit_req?4:gas?1:brake?2:3;
    if(ctl!=lf_spd_ctl){lf_spd_ctl=ctl;lf_spd_acc=0;}
    if(lf_exit_req){
        lf_speed_add(0,LF_HALT);lf_rev_wait=10;
    }else if(gas){
        if(td.speed<0)lf_speed_add(0,LF_HALT>>1);
        else if(td.speed<limit)lf_speed_add(1,lf_pull[kind][td.speed<(limit>>1)?0:td.speed<limit-(limit>>2)?1:2]);
        else if(td.speed>limit)lf_speed_add(0,LF_ROLL<<1);
        lf_rev_wait=10;
    }else if(brake){
        if(td.speed>0){lf_speed_add(0,lf_stop[kind]);lf_rev_wait=10;}
        else if(lf_rev_wait)lf_rev_wait--;
        else if(td.speed>-(WORD)(limit/3))lf_speed_add(-1,LF_REVERSE);
    }else{
        lf_speed_add(0,LF_ROLL);
        lf_rev_wait=10;
    }
    /* Steering needs genuine movement: the car never rotates at rest. The
     * yaw rate grows with speed, peaks at town speeds and eases at speed (a
     * heavy car runs wide); reversing swings the nose the other way, as a
     * real car does. Braking hard into a turn at speed lets the tail go. */
    a=td.speed<0?(UBYTE)-td.speed:(UBYTE)td.speed;
    drift=brake&&a>12&&!!INPUT_LEFT!=!!INPUT_RIGHT;
    if(a&&!!INPUT_LEFT!=!!INPUT_RIGHT){
        period=a<3?16:a<7?10:a<12?7:a<16?8:10;
        if(kind==1)period+=3;else if(kind>=2)period--;
        if(drift)period-=2;
        if(++td_turn_tick>=period){
            td_turn_tick=0;dir=INPUT_LEFT?-1:1;if(td.speed<0)dir=-dir;
            td.heading=(td.heading+dir)&15;
            if(td.job!=TD_NONE&&td.stage&&td_job.kind==5&&a>15){if(td.health)td.health--;td_message(14);}
        }
    }else td_turn_tick=0;
    /* Traction eases velocity toward the heading: firmly at town speeds,
     * less at speed (momentum carries the car wide) and least in a braking
     * slide. */
    if(td.heading!=lf_scale_key_h||td.speed!=lf_scale_key_s){
        lf_scale_key_h=td.heading;lf_scale_key_s=td.speed;
        lf_scale_x=lf_scale(lf_dx[td.heading],td.speed);lf_scale_y=lf_scale(lf_dy[td.heading],td.speed);
    }
    tx=lf_scale_x;ty=lf_scale_y;
    if(drift){td_vx+=lf_div16(tx-td_vx);td_vy+=lf_div16(ty-td_vy);}
    else if(a>15){WORD ex=lf_div16(tx-td_vx),ey=lf_div16(ty-td_vy);td_vx+=ex+ex+ex;td_vy+=ey+ey+ey;}
    else{td_vx+=lf_div4(tx-td_vx);td_vy+=lf_div4(ty-td_vy);}
    if(!td.speed){td_vx=lf_div2(td_vx);td_vy=lf_div2(td_vy);}
    nu=td.u+lf_div16(td_vx);nv=td.v+lf_div16(td_vy);u=nu>>4;v=nv>>4;
    if(lf_drive(u,v)){
        /* Entering a signal junction against the light costs a fine. */
        if(!td_red_cooldown&&a>6){
            UBYTE k;
            for(k=0;k<TD_SIGNALS;k++){
                if(td_signal_u[k]==0xFFFF||lf_dist(u,td_signal_u[k])>=14||lf_dist(v,td_signal_v[k])>=14)continue;
                if(lf_dist(td.u>>4,td_signal_u[k])<14&&lf_dist(td.v>>4,td_signal_v[k])<14)break;
                {UBYTE ax=lf_abs(lf_dx[td.heading]),ay=lf_abs(lf_dy[td.heading]);
                red=(UBYTE)(td.seconds%12);
                if((ax>ay&&red>=7)||(ax<=ay&&red<7)){if(td.cash>=5)td.cash-=5;td_red_cooldown=120;td_message(7);}}
                break;
            }
        }
        td.u=nu;td.v=nv;
    }else{
        /* A glancing curb contact slides along the free axis. */
        if(nu!=(WORD)td.u&&lf_drive(nu>>4,td.v>>4)){td.u=nu;td_vy=0;slide=1;}
        if(nv!=(WORD)td.v&&lf_drive(td.u>>4,nv>>4)){td.v=nv;td_vx=0;slide=1;}
        if(!slide)slide=lf_corner_slide(nu,nv);
        if(slide){if(a>8&&!td.cooldown){lf_dust(TD_PART_PUFF);td.cooldown=30;if(td.job!=TD_NONE&&td.stage){UBYTE damage=td_job.kind==1?4:1;td.health=td.health>damage?td.health-damage:0;}td_message(5);lf_wear(2);}}
        else{
            if(a>8&&!td.cooldown){lf_dust(TD_PART_SMOKE);if(td.job!=TD_NONE&&td.stage){UBYTE damage=td_job.kind==1?20:8;td.health=td.health>damage?td.health-damage:0;}td.cooldown=45;td_message(5);lf_wear(a);if(a>14)lf_shake=8;}
            /* A solid wall throws a fast car back a little before it settles. */
            if(a>10){td.speed=-lf_div4(td.speed);td_vx=-lf_div4(td_vx);td_vy=-lf_div4(td_vy);}
            else{td.speed=0;td_vx=td_vy=0;}
        }
    }
    lf_car_hits(old_u,old_v);
    /* The A+B exit: out as soon as the car has all but stopped. */
    if(lf_exit_req&&td.speed<=2&&td.speed>=-2){lf_exit_req=0;td.speed=0;result|=TD_DRIVE_EXIT;}
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
    base=td_traffic_bases[best];
    td.vehicle=base==TD_FRAME_PLAYER_VAN||base==TD_FRAME_TRAFFIC_PICKUP?1:base==TD_FRAME_PLAYER_MOTORCYCLE?2:0;
    td.heading=td_lf_tr_heading(best)<<1;
    /* A fresh car in its own paint. */
    td_car_damage=0;td_car_colour=TD_PALETTE(&actors[2+best]);
    td.park_u=td_traffic_u[best];td.park_v=td_traffic_v[best];td.park_district=td.district;
    td_lf_own_car(best,TR_GONE);actors[2+best].flags|=ACTOR_FLAG_HIDDEN;
    /* The driver bails out and runs off the far side. */
    fx_look=LF_IS_PATROL(best)?LF_LOOK_OFFICER:best&3;
    td_lf_fx(FX_RUNNER,td.park_u>>4,td.park_v>>4,54);
    TD_PALETTE(&actors[TD_ACTOR_FX])=LF_IS_PATROL(best)?LF_OFFICER_PAL:lf_civilian_pal[best&3];
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

UBYTE td_life_carjack(void) BANKED {
    if(td_life_locked()||td_entry_timer)return FALSE;
    return lf_carjack();
}
void td_life_punch(void) BANKED {
    if(td_life_locked()||td_entry_timer)return;
    lf_punch_now();
}

/* ------------------------------------------------------------ aiming */
UBYTE td_aim_dir,td_aim_target=TD_NONE;
/* Sixteen headings from a pixel offset (E=0 clockwise, +y south). */
static UBYTE lf_heading16(WORD dx,WORD dy){
    UWORD ax=lf_abs(dx),ay=lf_abs(dy),ax2=ax<<1,ay2=ay<<1;UBYTE k;
    /* Ratio thresholds with shifts and adds (no multiply helper calls). */
    if((ay2<<1)+ay<ax)k=0;else if(ay2+ay<ax2)k=1;else if(ay2<ax2+ax)k=2;else if(ay<(ax2<<1)+ax)k=3;else k=4;
    if(dx>=0)return dy>=0?k:(UBYTE)(16-k)&15;
    return dy>=0?8-k:8+k;
}
/* Nearest walker (0..7) or the patrol car (8) within about 60 degrees of
 * the aim and 128 px, favouring targets straight ahead. */
#define LF_AIM_RANGE 128
#define LF_AIM_KEEP 152
UBYTE td_aim_hold;
static UBYTE lf_aim_diff(WORD du,WORD dv){
    UBYTE diff=(UBYTE)(lf_heading16(du,dv)-(td_aim_dir<<1))&15;
    return diff>8?16-diff:diff;
}
static UBYTE lf_ped_aimable(UBYTE i,UBYTE bit,actor_t *a){
    if(a->flags&ACTOR_FLAG_HIDDEN)return FALSE;
    return !((td_ped_ovr&bit)&&(pk_mode[i]==PK_FLY||pk_mode[i]==PK_DEAD||pk_mode[i]==PK_DOWN));
}
static UBYTE lf_find_target(void){
    UBYTE i,bit,best=TD_NONE,diff;UWORD score,best_score=65535;WORD du,dv;
    UWORD pu=td.u>>4,pv=td.v>>4;actor_t *a;
    for(i=0,bit=1,a=&actors[TD_ACTOR_PEDS];i<TD_PEDS;i++,bit<<=1,a++){
        if(!lf_ped_aimable(i,bit,a))continue;
        du=(WORD)(a->pos.x>>5)-(WORD)pu;dv=(WORD)(a->pos.y>>5)-(WORD)pv;
        score=lf_abs(du)+lf_abs(dv);if(score>=LF_AIM_RANGE||score<4)continue;
        diff=lf_aim_diff(du,dv);if(diff>3)continue;
        score+=diff*20;
        if(score<best_score){best_score=score;best=i;}
    }
    if(LF_IS_PATROL(TD_POLICE_SLOT)&&tr_mode[TD_POLICE_SLOT]!=TR_GONE){
        du=(WORD)(td_traffic_u[TD_POLICE_SLOT]>>4)-(WORD)pu;dv=(WORD)(td_traffic_v[TD_POLICE_SLOT]>>4)-(WORD)pv;
        score=lf_abs(du)+lf_abs(dv);diff=lf_aim_diff(du,dv);
        if(score<LF_AIM_RANGE&&diff<=3&&score+diff*20<best_score)best=8;
    }
    return best;
}
/* A lock holds while B is held (strafing keeps firing at the same target)
 * or while the target stays inside the aim cone, so a closer walker
 * crossing the cone does not steal it. */
static UBYTE lf_keep_target(UBYTE t){
    WORD du,dv;UWORD pu=td.u>>4,pv=td.v>>4;actor_t *a;
    if(t==8){
        if(!LF_IS_PATROL(TD_POLICE_SLOT)||tr_mode[TD_POLICE_SLOT]==TR_GONE)return FALSE;
        du=(WORD)(td_traffic_u[TD_POLICE_SLOT]>>4)-(WORD)pu;dv=(WORD)(td_traffic_v[TD_POLICE_SLOT]>>4)-(WORD)pv;
    }else{
        a=&actors[TD_ACTOR_PEDS+t];
        if(!lf_ped_aimable(t,1<<t,a))return FALSE;
        du=(WORD)(a->pos.x>>5)-(WORD)pu;dv=(WORD)(a->pos.y>>5)-(WORD)pv;
    }
    if(lf_abs(du)+lf_abs(dv)>=LF_AIM_KEEP)return FALSE;
    return td_aim_hold||lf_aim_diff(du,dv)<=3;
}
static void lf_target_at(UBYTE t,UWORD *u,UWORD *v){
    if(t==8){*u=td_traffic_u[TD_POLICE_SLOT]>>4;*v=td_traffic_v[TD_POLICE_SLOT]>>4;}
    else{*u=actors[TD_ACTOR_PEDS+t].pos.x>>5;*v=actors[TD_ACTOR_PEDS+t].pos.y>>5;}
}
void td_life_aim(void) BANKED {
    if(!td.onfoot||!td.ammo||td.mode!=TD_ROAM||td_life_locked()){td_aim_target=TD_NONE;return;}
    if(td_aim_target==TD_NONE||!lf_keep_target(td_aim_target))td_aim_target=lf_find_target();
}

/* The pistol: a round towards the locked-on target, or along the aim;
 * holding B keeps firing about six times a second. Rounds fly
 * (td_shots.c) with a spread that grows when the courier walks or runs, so
 * a moving shooter or a moving target can miss. */
void td_life_foot_b(void) BANKED {
    UBYTE h,d,spread;UWORD tu,tv,pu=td.u>>4,pv=td.v>>4;
    if(td_life_locked()||td_entry_timer||lf_punch)return;
    if(!td.ammo){td_message(TD_MSG_NO_AMMO);return;}
    td_life_aim();
    if(td_aim_target!=TD_NONE){lf_target_at(td_aim_target,&tu,&tv);h=lf_heading16((WORD)tu-(WORD)pu,(WORD)tv-(WORD)pv);}
    else{h=td_aim_dir<<1;tu=(UWORD)((WORD)pu+lf_dx[h]*8);tv=(UWORD)((WORD)pv+lf_dy[h]*8);}
    spread=td_running?12:(INPUT_LEFT||INPUT_RIGHT||INPUT_UP||INPUT_DOWN)?7:3;
    if(!td_shot_fire(TD_SHOT_COURIER,pu+((lf_dx[h]*8)>>4),pv+((lf_dy[h]*8)>>4),tu,tv,spread))return;
    /* Face the shot: the nearest of the four walking directions. */
    d=((h+2)&15)>>2;td_walk_dir=d==0?0:d==1?2:d==2?1:3;
    td.ammo--;lf_punch=10;td_anim_pose(TD_FRAME_COURIER_SHOOT,10);
    td_anim_spawn(TD_PART_FLASH,0,pu+((lf_dx[h]*9)>>4),pv+((lf_dy[h]*9)>>4)-6);
    if(lf_shake<2)lf_shake=2;
    td_audio_play(TD_AUDIO_IMPACT);
    td_lf_crime(CR_GUN);td_lf_panic(pu,pv);
}

UBYTE td_lf_drive(UWORD u,UWORD v) BANKED {return lf_drive(u,v);}
UBYTE td_lf_body(UWORD u,UWORD v) BANKED {return lf_body(u,v);}
