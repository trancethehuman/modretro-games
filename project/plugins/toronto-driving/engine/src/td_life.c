#pragma bank 255
/* Street life for the city scene: struck pedestrians, police attention,
 * pursuit, arrest and hospital recovery. Vehicle handling and the courier's
 * actions are in td_drive.c, presentation in td_life_draw.c. All state here
 * is transient; saved fields live in td (td_game.h). */
#include <string.h>
#include "td_life_int.h"
#include "td_npc.h"
#include "td_audio.h"
#include "td_district.h"
#include "td_overlay.h"
#include "td_radio_data.h"
#include "td_shots.h"
#include "td_anim.h"
#include "td_special.h"
#include "camera.h"
#include "input.h"
#include "system.h"

UBYTE td_ped_ovr,td_tr_ctrl,td_fx_kind,td_beacon_shown,td_life_event,td_life_fine;
UWORD td_beacon_u,td_beacon_v;
UBYTE pk_mode[TD_PEDS],pk_timer[TD_PEDS],pk_look[TD_PEDS],pk_dir[TD_PEDS],pk_span[TD_PEDS];
UWORD pk_u[TD_PEDS],pk_v[TD_PEDS];
BYTE pk_vu[TD_PEDS],pk_vv[TD_PEDS];
UBYTE pk_fresh,pk_lethal,pk_drawn;
UBYTE tr_mode[6],tr_timer[6],tr_head[6],tr_spin;
BYTE tr_pu[6],tr_pv[6];
UWORD tr_au[6],tr_av[6];
UBYTE fx_timer,fx_look;
UWORD fx_u,fx_v;
BYTE fx_du,fx_dv;
UBYTE lf_warm,lf_flash,lf_punch,lf_hurt,lf_down,lf_arrest,lf_bust,lf_cop_cool,lf_rev_wait,lf_shake,lf_chaos,lf_stuck,lf_axis,lf_stun;
UBYTE lf_patrol,lf_lost,lf_drop,lf_amb,lf_vest,lf_calm,lf_unit,lf_beat,lf_seen;
UWORD lf_seen_u,lf_seen_v;
UBYTE lf_aim_who=TD_NONE,lf_aim_time,lf_combo,lf_combo_t;
static UBYTE lf_strafe,lf_strafe_who=TD_NONE;static BYTE lf_strafe_dir;

static void lf_panic_r(UWORD u,UWORD v,UBYTE ru,UBYTE rv);

/* ------------------------------------------------------------ effects */
void td_lf_fx(UBYTE kind,UWORD u,UWORD v,UBYTE timer) BANKED {
    /* A magazine on the ground stays until it is picked up or expires. */
    if(td_fx_kind==FX_DROP&&kind!=FX_DROP)return;
    TD_PALETTE(&actors[TD_ACTOR_FX])=0;td_fx_kind=kind;fx_u=u<<4;fx_v=v<<4;fx_timer=timer;fx_du=fx_dv=0;
}

static void lf_fx_end(void){td_fx_kind=0;actors[TD_ACTOR_FX].flags|=ACTOR_FLAG_HIDDEN;}

/* ------------------------------------------------------------ police attention */
static UBYTE lf_is_officer(UBYTE i){
    if(td_ped_ovr&(1<<i))return pk_look[i]==LF_LOOK_OFFICER&&pk_mode[i]==PK_CHASE;
    return td_ped_route[i]!=TD_NONE&&(td_ped_route[i]&7)==LF_LOOK_OFFICER;
}

static UBYTE lf_police_near(UWORD range_u,UWORD range_v){
    UBYTE i;UWORD pu=td.u>>4,pv=td.v>>4;actor_t *a;
    if((lf_patrol||lf_beat)&&tr_mode[TD_POLICE_SLOT]!=TR_GONE&&lf_dist(td_traffic_u[TD_POLICE_SLOT]>>4,pu)<range_u&&
       lf_dist(td_traffic_v[TD_POLICE_SLOT]>>4,pv)<range_v)return TRUE;
    for(i=0,a=&actors[TD_ACTOR_PEDS];i<TD_PEDS;i++,a++){
        if((a->flags&ACTOR_FLAG_HIDDEN)||!lf_is_officer(i))continue;
        if(lf_dist(a->pos.x>>5,pu)<range_u&&lf_dist(a->pos.y>>5,pv)<range_v)return TRUE;
    }
    return FALSE;
}

/* Witnessed crimes raise attention one star at a time, up to a ceiling
 * that grows with the violence: a scuffle or theft brings one star, gunfire
 * two, a killing three; attacking officers adds a star directly. Unseen
 * mayhem accumulates chaos and brings another star only once a lot has
 * happened, so the police escalate gradually rather than all at once. */
void td_lf_crime(UBYTE kind) BANKED {
    static const UBYTE chaos[5]={2,1,4,4,6},ceiling[5]={1,2,3,4,5};
    UBYTE level=td.wanted;
    lf_chaos+=chaos[kind];
    if(kind>=CR_COP){
        level++;
        if(kind==CR_COP_KILL&&level<3)level=3;
    }else if(lf_police_near(144,112)&&level<ceiling[kind])level++;
    if(lf_chaos>=16){lf_chaos-=16;if(level<4)level++;}
    if(level>TD_WANTED_MAX)level=TD_WANTED_MAX;
    if(level>td.wanted){td.wanted=level;td_message(TD_MSG_STARS);}
    if(td.wanted){td.heat=TD_HEAT_FOR(td.wanted);lf_seen_u=td.u;lf_seen_v=td.v;}
}

/* ------------------------------------------------------------ pedestrians */
static void lf_own_ped(UBYTE i,UBYTE look){
    actor_t *a=&actors[TD_ACTOR_PEDS+i];
    if(!(td_ped_ovr&(1<<i))){pk_u[i]=a->pos.x>>1;pk_v[i]=a->pos.y>>1;pk_look[i]=look;}
    td_ped_ovr|=1<<i;pk_fresh&=~(1<<i);
}

void td_lf_knock(UBYTE i,WORD vu,WORD vv,UBYTE lethal) BANKED {
    UBYTE look,mode=pk_mode[i],bit=1<<i,blame=!(lethal&2);
    lethal&=1;
    if(td_ped_ovr&bit)look=pk_look[i];
    else look=td_ped_route[i]!=TD_NONE?td_ped_route[i]&7:0;
    if((td_ped_ovr&bit)&&(mode==PK_FLY||mode==PK_DEAD))return;
    lf_own_ped(i,look);
    pk_vu[i]=lf_clamp(vu,48);pk_vv[i]=lf_clamp(vv,48);
    pk_span[i]=pk_timer[i]=lethal?32:(vu||vv)?20:12;
    pk_mode[i]=PK_FLY;pk_drawn&=~bit;
    if(lethal)pk_lethal|=bit;else pk_lethal&=~bit;
    td_lf_panic(pk_u[i]>>4,pk_v[i]>>4);
    if(!blame)return;
    if(td_hitstop<3)td_hitstop=3;
    if(look==LF_LOOK_OFFICER){
        td_lf_crime(lethal?CR_COP_KILL:CR_COP);
        /* A downed officer's spare magazine lies where they fell. */
        if(!td_fx_kind)td_lf_fx(FX_DROP,pk_u[i]>>4,(pk_v[i]>>4)+4,255);
    }
    /* Fighting back against someone who started it is no crime. */
    else if(!(td_npc_hostile&bit))td_lf_crime(lethal?CR_KILL:CR_MINOR);
}

/* A jab staggers a walker back a step; they stand dazed for half a second,
 * then a civilian runs and an officer fights on. Witnesses scatter. */
void td_lf_stun(UBYTE i,BYTE vu,BYTE vv) BANKED {
    UBYTE look,bit=1<<i,mode=pk_mode[i];UWORD u,v;
    if(td_ped_ovr&bit)look=pk_look[i];
    else look=td_ped_route[i]!=TD_NONE?td_ped_route[i]&7:0;
    if((td_ped_ovr&bit)&&(mode==PK_FLY||mode==PK_DEAD||mode==PK_DOWN))return;
    lf_own_ped(i,look);
    pk_mode[i]=PK_STUN;pk_timer[i]=28;pk_drawn&=~bit;
    u=pk_u[i]+((WORD)vu<<4);v=pk_v[i]+((WORD)vv<<4);
    if(lf_walk(u>>4,v>>4)){pk_u[i]=u;pk_v[i]=v;}
    lf_panic_r(pk_u[i]>>4,pk_v[i]>>4,64,56);
    if(look==LF_LOOK_OFFICER){if(!td.wanted)td_lf_crime(CR_COP);}
    else if(!(td_npc_hostile&bit))td_lf_crime(CR_MINOR);
}

void td_lf_stagger(UBYTE i,BYTE vu,BYTE vv) BANKED {
    UBYTE bit=1<<i;UWORD u,v;
    lf_own_ped(i,LF_LOOK_OFFICER);lf_vest|=bit;
    pk_mode[i]=PK_CHASE;pk_drawn&=~bit;TD_PALETTE(&actors[TD_ACTOR_PEDS+i])=LF_OFFICER_PAL;
    /* Knocked back a step by the round. */
    u=pk_u[i]+vu*3;v=pk_v[i]+vv*3;
    if(lf_walk(u>>4,v>>4)){pk_u[i]=u;pk_v[i]=v;}
    td_lf_crime(CR_COP);
}

/* Trouble near (u,v): everyone close by and in view runs from the courier
 * for a few seconds (officers excepted). */
static void lf_panic_r(UWORD u,UWORD v,UBYTE ru,UBYTE rv){
    UBYTE i,bit,route;actor_t *a=&actors[TD_ACTOR_PEDS];
    td_npc_alarm=1;td_npc_alarm_u=u;td_npc_alarm_v=v;
    for(i=0,bit=1;i<TD_PEDS;i++,bit<<=1,a++){
        route=td_ped_route[i];
        if((td_ped_ovr&bit)||route==TD_NONE||(a->flags&ACTOR_FLAG_HIDDEN)||(route&7)==LF_LOOK_OFFICER)continue;
        if(lf_dist(a->pos.x>>5,u)>=ru||lf_dist(a->pos.y>>5,v)>=rv)continue;
        lf_own_ped(i,route&7);pk_mode[i]=PK_FLEE;pk_timer[i]=160;pk_drawn&=~bit;
    }
}
void td_lf_panic(UWORD u,UWORD v) BANKED {lf_panic_r(u,v,128,112);}
void td_lf_dodge(UBYTE i) BANKED {
    UBYTE route=td_ped_route[i];
    if((td_ped_ovr&(1<<i))||route==TD_NONE)return;
    lf_own_ped(i,route&7);pk_mode[i]=PK_FLEE;pk_timer[i]=60;pk_drawn&=~(1<<i);
}

/* Gunfire or a crash also sends the drivers near it and in view speeding
 * away (the police car excepted). */
void td_lf_scatter(UWORD u,UWORD v) BANKED {
    UBYTE i,bit;
    lf_panic_r(u,v,128,112);
    for(i=0,bit=1;i<6;i++,bit<<=1){
        if((td_tr_ctrl&bit)||(i==TD_POLICE_SLOT&&(lf_patrol||lf_beat)))continue;
        if(lf_dist(td_traffic_u[i]>>4,u)>=112||lf_dist(td_traffic_v[i]>>4,v)>=96||!lf_on_screen(td_traffic_u[i]>>4,td_traffic_v[i]>>4))continue;
        td_lf_own_car(i,TR_FLEE);tr_timer[i]=200;
    }
}

static void lf_release_ped(UBYTE i){
    UBYTE bit=1<<i;
    td_ped_ovr&=~bit;lf_vest&=~bit;pk_drawn&=~bit;pk_mode[i]=0;td_ped_route[i]=TD_NONE;actors[TD_ACTOR_PEDS+i].flags|=ACTOR_FLAG_HIDDEN;
}

/* Axis-separated walk with wall sliding; returns the facing used. */
static UBYTE lf_step(UBYTE i,WORD du,WORD dv,BYTE speed){
    UWORD u=pk_u[i],v=pk_v[i];WORD su=du>0?speed:du<0?-speed:0,sv=dv>0?speed:dv<0?-speed:0;UBYTE face=pk_dir[i];
    if(lf_abs(du)<(UWORD)speed)su=du;
    if(lf_abs(dv)<(UWORD)speed)sv=dv;
    if(su&&lf_walk((u+su)>>4,v>>4)){u+=su;face=su>0?0:1;}
    if(sv&&lf_walk(u>>4,(v+sv)>>4)){v+=sv;if(lf_abs(dv)>lf_abs(du)||u==pk_u[i])face=sv>0?2:3;}
    if(u==pk_u[i]&&v==pk_v[i]&&(su||sv)){
        /* Blocked: slide along the wall in the other axis. */
        if(!sv&&lf_walk(u>>4,(v+speed)>>4))v+=speed;
        else if(!su&&lf_walk((u+speed)>>4,v>>4))u+=speed;
    }
    pk_u[i]=u;pk_v[i]=v;pk_dir[i]=face;
    return face;
}

static void lf_peds_tick(void){
    UBYTE i,bit,a;WORD du,dv;UWORD u,v,pu=td.u,pv=td.v;
    a=td.speed<0?(UBYTE)-td.speed:(UBYTE)td.speed;
    for(i=0,bit=1;i<TD_PEDS;i++,bit<<=1){
        if(!(td_ped_ovr&bit))continue;
        if(pk_mode[i]>=PK_FLEE){du=(WORD)pu-(WORD)pk_u[i];dv=(WORD)pv-(WORD)pk_v[i];}
        else du=dv=0x7FFF;
        switch(pk_mode[i]){
        case PK_FLY:
            /* Thrown bodies bounce off walls with half their speed. */
            u=pk_u[i]+pk_vu[i];v=pk_v[i]+pk_vv[i];
            if(lf_walk(u>>4,pk_v[i]>>4))pk_u[i]=u;else pk_vu[i]=-(pk_vu[i]>>1);
            if(lf_walk(pk_u[i]>>4,v>>4))pk_v[i]=v;else pk_vv[i]=-(pk_vv[i]>>1);
            if(pk_timer[i]&1){pk_vu[i]-=pk_vu[i]>>3;pk_vv[i]-=pk_vv[i]>>3;}
            if(!--pk_timer[i]){
                if(pk_lethal&bit){pk_mode[i]=PK_DEAD;}
                else{pk_mode[i]=PK_DOWN;pk_timer[i]=150;}
            }
            break;
        case PK_DOWN:
            if(!--pk_timer[i]){pk_mode[i]=PK_FLEE;pk_timer[i]=240;pk_drawn&=~bit;}
            break;
        case PK_DEAD:
            break;
        case PK_STUN:
            if(!--pk_timer[i]){
                pk_mode[i]=pk_look[i]==LF_LOOK_OFFICER&&td.wanted?PK_CHASE:PK_FLEE;pk_timer[i]=160;pk_drawn&=~bit;
            }
            break;
        case PK_FLEE:
            /* Walkers on foot move every other tick with a double step. */
            if(!(td_tick&1))lf_step(i,-du,-dv,20);
            if(pk_timer[i]){
                /* Fear spreads: a running walker starts those beside them. */
                if(!(--pk_timer[i]&31))lf_panic_r(pk_u[i]>>4,pk_v[i]>>4,40,32);
            }
            break;
        case PK_CHASE:
            if(!td.wanted){pk_mode[i]=PK_FLEE;pk_timer[i]=200;break;}
            /* Left far behind, an officer gives up and walks off. */
            if(lf_abs(du)>2560||lf_abs(dv)>2240){pk_mode[i]=PK_FLEE;pk_timer[i]=120;break;}
            if(lf_abs(du)<112&&lf_abs(dv)<112){
                /* Hands on the courier: holding on for a moment arrests a
                 * courier on foot or boxes in a stopped car. */
                if((td.onfoot||a<2)&&lf_bust<LF_BUST_TICKS)lf_bust+=2;
                break;
            }
            /* An officer aiming stands still (lf_police_fire). */
            if(i==lf_aim_who)break;
            /* Armed officers keep their distance and shoot (lf_police_fire);
             * after a shot they sidestep, which spoils the courier's aim. */
            if(td.wanted>=4&&lf_abs(du)<640&&lf_abs(dv)<560){
                if(i==lf_strafe_who&&lf_strafe){
                    lf_strafe--;
                    if(!(td_tick&1)){if(lf_abs(du)>lf_abs(dv))lf_step(i,0,lf_strafe_dir*16,12);else lf_step(i,lf_strafe_dir*16,0,12);}
                }
                break;
            }
            /* Below four stars a walking courier (8 per tick) can outpace them. */
            if(!(td_tick&1))lf_step(i,du,dv,(6+(td.wanted>>1))<<1);
            break;
        }
        /* A running courier bowls over anyone in their path. */
        if(td.onfoot&&td_running&&(pk_mode[i]==PK_FLEE||pk_mode[i]==PK_CHASE||pk_mode[i]==PK_STUN)&&
           lf_abs(du)<128&&lf_abs(dv)<128){
            td_lf_knock(i,du>0?-20:du<0?20:0,dv>0?-20:dv<0?20:0,0);
            lf_shake=3;td_audio_play(TD_AUDIO_IMPACT);continue;
        }
        /* The courier's moving car strikes owned walkers too. */
        if(!td.onfoot&&a>=3&&(pk_mode[i]==PK_FLEE||pk_mode[i]==PK_CHASE||pk_mode[i]==PK_STUN)&&
           lf_abs(du)<160&&lf_abs(dv)<160){
            td_lf_knock(i,lf_div16(td_vx+(td_vx>>1)),lf_div16(td_vy+(td_vy>>1)),a>=16);
            td.speed-=lf_div4(td.speed);td_vx-=lf_div4(td_vx);td_vy-=lf_div4(td_vy);
            td_audio_play(TD_AUDIO_IMPACT);td_message(TD_MSG_PED);
        }
        /* Out of view, the fallen and the departed return to the route pool
         * (checked for one slot per tick). */
        if((td_tick&7)==i&&(pk_mode[i]==PK_DEAD||pk_mode[i]==PK_FLEE)&&!lf_on_screen(pk_u[i]>>4,pk_v[i]>>4))lf_release_ped(i);
    }
}

/* ------------------------------------------------------------ road vehicles */
UBYTE td_lf_tr_heading(UBYTE i) BANKED {
    if(td_tr_ctrl&(1<<i))return tr_head[i];
    return (UBYTE)(actors[2+i].frame-td_traffic_bases[i])&6;
}

void td_lf_own_car(UBYTE i,UBYTE mode) BANKED {
    UBYTE bit=1<<i;
    if(!(td_tr_ctrl&bit)){tr_au[i]=td_traffic_u[i];tr_av[i]=td_traffic_v[i];tr_head[i]=td_lf_tr_heading(i);}
    td_tr_ctrl|=bit;tr_mode[i]=mode;
}

/* A traffic slot entering play out of view takes a new design and colour:
 * sedans, hatchbacks, pickups, coupes, taxis (two in eight: Toronto's are
 * everywhere), vans and motorcycles. */
static const UBYTE lf_design[8]={TD_FRAME_PLAYER_CAR,TD_FRAME_TRAFFIC_COMPACT,TD_FRAME_TRAFFIC_TAXI,TD_FRAME_TRAFFIC_PICKUP,
    TD_FRAME_TRAFFIC_SPORTS,TD_FRAME_TRAFFIC_TAXI,TD_FRAME_PLAYER_VAN,TD_FRAME_PLAYER_MOTORCYCLE};
static const UBYTE lf_colour[8]={TD_PAL_RED,TD_PAL_BLUE,TD_PAL_TEAL,TD_PAL_VIOLET,TD_PAL_NAVY,TD_PAL_RED,TD_PAL_BLUE,TD_PAL_YELLOW};
/* xorshift16 for road vehicle looks, stirred by the update counter so
 * recycled vehicles do not repeat a design within a second. */
static UWORD lf_rng=0xACE1;
static UBYTE lf_random(void){
    lf_rng^=lf_rng<<7;lf_rng^=lf_rng>>9;lf_rng^=lf_rng<<8;
    return (UBYTE)lf_rng^(UBYTE)td_tick;
}
void td_lf_new_look(UBYTE i,UBYTE seed) BANKED {
    UBYTE base;
    if(i==TD_POLICE_SLOT)lf_beat=0;
    base=lf_design[seed&7];
    td_traffic_bases[i]=base;
    TD_PALETTE(&actors[2+i])=base==TD_FRAME_TRAFFIC_TAXI?TD_PAL_YELLOW:lf_colour[(seed>>3)&7];
}

static void lf_free_car(UBYTE i){
    td_tr_ctrl&=~(1<<i);tr_mode[i]=0;
    /* The patrol car changes back to an ordinary car out of view. */
    if(i==TD_POLICE_SLOT&&lf_patrol){lf_patrol=0;td_lf_new_look(i,lf_random());}
}

/* Is a vehicle body at Q4 (u,v) clear of the courier? */
static UBYTE lf_clear_of_player(UWORD u,UWORD v){
    if(td.onfoot){
        /* The courier's parked car is a solid body too. */
        if(td.park_district==td.district&&lf_dist(u,td.park_u)<192&&lf_dist(v,td.park_v)<192)return FALSE;
        return lf_dist(u,td.u)>=176||lf_dist(v,td.v)>=176;
    }
    return lf_dist(u,td.u)>=192||lf_dist(v,td.v)>=192;
}

static UBYTE lf_car_move(UBYTE i,WORD su,WORD sv){
    UWORD u=td_traffic_u[i]+su,v=td_traffic_v[i]+sv;
    if(!td_lf_body(u>>4,v>>4)||!lf_clear_of_player(u,v))return FALSE;
    td_traffic_u[i]=u;td_traffic_v[i]=v;return TRUE;
}

/* Whole-pixel spawn point just outside the view, on walkable (drive=0) or
 * drivable ground, scanning edges in a rotating order. */
static UBYTE lf_spot(UBYTE drive,UWORD *u,UWORD *v){
    UBYTE k,j,s=(UBYTE)sys_time;WORD cu,cv;
    for(k=0;k<12;k++){
        j=(UBYTE)(k+s)%12;
        switch(j&3){
            case 0:cu=scroll_x-32;cv=scroll_y+40+((j>>2)<<4);break;
            case 1:cu=scroll_x+192;cv=scroll_y+40+((j>>2)<<4);break;
            case 2:cu=scroll_x+48+((j>>2)<<5);cv=scroll_y-32;break;
            default:cu=scroll_x+48+((j>>2)<<5);cv=scroll_y+176;break;
        }
        if(cu<16||cv<16||cu>1008||cv>960)continue;
        if(drive?td_lf_drive(cu,cv):lf_walk(cu,cv)){*u=cu;*v=cv;return TRUE;}
    }
    return FALSE;
}

static void lf_cars_tick(void){
    UBYTE i,bit,a;WORD du,dv,su,sv,speed;UWORD u,v;
    a=td.speed<0?(UBYTE)-td.speed:(UBYTE)td.speed;
    for(i=0,bit=1;i<6;i++,bit<<=1){
        if(!(td_tr_ctrl&bit))continue;
        u=td_traffic_u[i];v=td_traffic_v[i];
        switch(tr_mode[i]){
        case TR_PUSH:
            if(tr_pu[i]||tr_pv[i]){
                if(!lf_car_move(i,tr_pu[i],tr_pv[i])){tr_pu[i]=tr_pv[i]=0;}
                if(tr_timer[i]&1){tr_pu[i]-=lf_div4(tr_pu[i]);tr_pv[i]-=lf_div4(tr_pv[i]);}
            }
            if((tr_spin&bit)&&!(tr_timer[i]&3))tr_head[i]=(tr_head[i]+2)&6;
            if(!--tr_timer[i]){tr_mode[i]=LF_IS_PATROL(i)?(td.wanted?TR_CHASE:TR_PARK):i==td_hidden_slot?TR_STILL:TR_RETURN;tr_spin&=~bit;}
            break;
        case TR_RETURN:
            /* Easing back into lane runs every other tick with a double step. */
            if(td_tick&1)break;
            du=(WORD)tr_au[i]-(WORD)u;dv=(WORD)tr_av[i]-(WORD)v;
            if(!du&&!dv){
                /* Back in lane: restore the heading it had before the impact. */
                lf_free_car(i);break;
            }
            su=du>16?16:du<-16?-16:du;sv=dv>16?16:dv<-16?-16:dv;
            if(!lf_car_move(i,su,0))su=0;
            if(!lf_car_move(i,0,sv))sv=0;
            if(!su&&!sv&&!lf_on_screen(u>>4,v>>4)&&!lf_on_screen(tr_au[i]>>4,tr_av[i]>>4)){
                td_traffic_u[i]=tr_au[i];td_traffic_v[i]=tr_av[i];
            }
            break;
        case TR_CHASE:
            if(!td.wanted){tr_mode[i]=TR_PARK;break;}
            if(tr_timer[i]){tr_timer[i]--;break;}
            /* Within a block (112 x 96 px) the patrol follows the courier;
             * out of that it drives to where they were last seen and waits,
             * so a courier who gets away can stay away. */
            du=(WORD)td.u-(WORD)u;dv=(WORD)td.v-(WORD)v;
            if(lf_abs(du)<1792&&lf_abs(dv)<1536){lf_seen_u=td.u;lf_seen_v=td.v;su=1;}
            else{du=(WORD)lf_seen_u-(WORD)u;dv=(WORD)lf_seen_v-(WORD)v;su=0;}
            /* It keeps its distance: 72 px behind a moving car, 40 px from a
             * courier on foot (an officer gets out), alongside a stopped car
             * (boxing it in). It never rams. */
            speed=td.onfoot?640:a<3?224:1152;
            if(lf_abs(du)<speed&&lf_abs(dv)<speed){
                if(su){
                    if(td.onfoot)lf_drop=1;
                    else if(a<3&&lf_bust<LF_BUST_TICKS)lf_bust+=2;
                }
                break;
            }
            /* Sirens: walkers near the patrol car get out of its way. */
            if(!(td_tick&31))lf_panic_r(u>>4,v>>4,48,40);
            /* The patrol car moves every other tick with a double step:
             * 0.56 to 0.81 pixels per tick, so a car at full speed (1.1)
             * outruns it and a courier on foot (0.5) does not. */
            if(td_tick&1)break;
            speed=16+(td.wanted<<1);
            /* Keep the current axis until it is blocked or nearly closed. */
            if(lf_axis==0&&lf_abs(du)<64)lf_axis=1;
            else if(lf_axis==1&&lf_abs(dv)<64)lf_axis=0;
            su=lf_axis?0:(du>0?speed:-speed);sv=lf_axis?(dv>0?speed:-speed):0;
            if(lf_abs(du)<(UWORD)speed&&!lf_axis)su=du;
            if(lf_abs(dv)<(UWORD)speed&&lf_axis)sv=dv;
            if(lf_car_move(i,su,sv)){lf_stuck=0;}
            else{
                /* Turn onto the other axis; if that is blocked too, back off. */
                lf_axis^=1;su=lf_axis?0:(du>0?speed:-speed);sv=lf_axis?(dv>0?speed:-speed):0;
                if(!lf_car_move(i,su,sv)&&++lf_stuck>=6){
                    su=lf_axis?(du>0?-speed:speed):0;sv=lf_axis?0:(dv>0?-speed:speed);
                    lf_car_move(i,su,sv);
                }
            }
            if(su||sv)tr_head[i]=su>0?0:su<0?4:sv>0?2:6;
            /* Wedged out of view for a while: it comes round another way,
             * from just beyond the screen edge. */
            if(lf_stuck>90&&!lf_on_screen(td_traffic_u[i]>>4,td_traffic_v[i]>>4)&&lf_spot(1,&u,&v)){
                td_traffic_u[i]=u<<4;td_traffic_v[i]=v<<4;lf_stuck=0;
            }
            break;
        case TR_STILL:
            break;
        case TR_FLEE:
            /* Away along its heading at a fast clip, turning where blocked,
             * until it is out of view; then it waits to rejoin its lane. */
            su=tr_head[i]==0?24:tr_head[i]==4?-24:0;sv=tr_head[i]==2?24:tr_head[i]==6?-24:0;
            if(!lf_car_move(i,su,sv))tr_head[i]=(tr_head[i]+((tr_timer[i]&8)?2:6))&6;
            if(!lf_on_screen(td_traffic_u[i]>>4,td_traffic_v[i]>>4)||!--tr_timer[i])tr_mode[i]=TR_PARK;
            break;
        case TR_PARK:
        case TR_GONE:
            /* Rejoin the lane where it left once neither spot is visible. */
            if(!lf_on_screen(u>>4,v>>4)&&!lf_on_screen(tr_au[i]>>4,tr_av[i]>>4)&&
               lf_clear_of_player(tr_au[i],tr_av[i])){
                td_traffic_u[i]=tr_au[i];td_traffic_v[i]=tr_av[i];lf_free_car(i);
                actors[2+i].flags&=~ACTOR_FLAG_HIDDEN;
            }
            break;
        }
    }
}

/* ------------------------------------------------------------ ambient traffic */
/* Every eighth update one road vehicle that has drifted far out of view is
 * brought back onto its own loop just beyond the screen edge, heading toward
 * the courier, so the same six vehicles keep the nearby streets busy. Core
 * lanes (TORONTO.c): four east-west avenues, one north-south street and the
 * bus loop, which keeps its route. */
#define LF_VIEW_U 112
#define LF_VIEW_V 96
static UBYTE lf_spot_clear(UBYTE i,UWORD u,UWORD v){
    UBYTE k;
    if(lf_on_screen(u>>4,v>>4))return FALSE;
    if(td.park_district==td.district&&lf_dist(u,td.park_u)<384&&lf_dist(v,td.park_v)<384)return FALSE;
    for(k=0;k<6;k++)if(k!=i&&lf_dist(u,td_traffic_u[k])<384&&lf_dist(v,td_traffic_v[k])<384)return FALSE;
    return TRUE;
}
static void lf_ambient(void){
    UBYTE i=lf_amb,leg;UWORD pu=td.u>>4,pv=td.v>>4,u,v;td_traffic_sample_t sample;
    lf_amb=i==5?0:i+1;
    if(td_tr_ctrl&(1<<i))return;
    u=td_traffic_u[i]>>4;v=td_traffic_v[i]>>4;
    if(lf_on_screen(u,v)||(lf_dist(u,pu)<176&&lf_dist(v,pv)<152))return;
    if(!td_world_traffic_recycle(td.district,i,pu,pv,LF_VIEW_U,LF_VIEW_V,&u,&v,&leg,&sample)||!lf_spot_clear(i,u,v))return;
    td_traffic_u[i]=u;td_traffic_v[i]=v;td_traffic_leg[i]=leg;td_traffic_samples[i]=sample;
    td_lf_new_look(i,lf_random());
    /* One time in four slot 4 comes round as a cruiser on its beat. */
    if(i==TD_POLICE_SLOT){
        if(!lf_patrol&&!(lf_random()&3)){lf_beat=1;td_traffic_bases[i]=TD_FRAME_POLICE;TD_PALETTE(&actors[2+i])=TD_PAL_BLUE;}
    }else if(!(lf_random()&3))td_veh_special(i);
}

/* ------------------------------------------------------------ police response */
static void lf_stand_down(void){
    UBYTE i;
    for(i=0;i<TD_PEDS;i++)if(pk_mode[i]==PK_CHASE){pk_mode[i]=PK_FLEE;pk_timer[i]=200;}
    if(tr_mode[TD_POLICE_SLOT]==TR_CHASE)tr_mode[TD_POLICE_SLOT]=TR_PARK;
    lf_bust=0;
}

static void lf_recruit(void){
    UBYTE i,bit,count=0,target;UWORD pu=td.u>>4,pv=td.v>>4,u,v;actor_t *a;
    if(!td.wanted||td.mode==TD_RIDE)return;
    target=td.wanted<3?1:2;
    for(i=0;i<TD_PEDS;i++)if(pk_mode[i]==PK_CHASE)count++;
    for(i=0,bit=1,a=&actors[TD_ACTOR_PEDS];i<TD_PEDS&&count<target;i++,bit<<=1,a++){
        if((td_ped_ovr&bit)||(a->flags&ACTOR_FLAG_HIDDEN)||!lf_is_officer(i))continue;
        if(lf_dist(a->pos.x>>5,pu)>=176||lf_dist(a->pos.y>>5,pv)>=128)continue;
        lf_own_ped(i,LF_LOOK_OFFICER);pk_mode[i]=PK_CHASE;count++;
    }
    /* Backup on foot from three stars: take a walker slot that is out of view. */
    if(td.wanted>=3&&count<target){
        for(i=0,bit=1,a=&actors[TD_ACTOR_PEDS];i<TD_PEDS;i++,bit<<=1,a++){
            if(td_ped_ovr&bit)continue;
            if(!(a->flags&ACTOR_FLAG_HIDDEN)&&td_ped_route[i]!=TD_NONE&&lf_on_screen(a->pos.x>>5,a->pos.y>>5))continue;
            if(!lf_spot(0,&u,&v))break;
            td_ped_ovr|=bit;pk_fresh&=~bit;pk_u[i]=u<<4;pk_v[i]=v<<4;pk_look[i]=LF_LOOK_OFFICER;pk_mode[i]=PK_CHASE;TD_PALETTE(a)=LF_OFFICER_PAL;
            break;
        }
    }
    /* The patrol car stopped beside a courier on foot: an officer gets out
     * if a walker slot is free and out of view. */
    if(lf_drop&&count<target&&tr_mode[TD_POLICE_SLOT]==TR_CHASE){
        for(i=0,bit=1,a=&actors[TD_ACTOR_PEDS];i<TD_PEDS;i++,bit<<=1,a++){
            if(td_ped_ovr&bit)continue;
            if(!(a->flags&ACTOR_FLAG_HIDDEN)&&td_ped_route[i]!=TD_NONE&&lf_on_screen(a->pos.x>>5,a->pos.y>>5))continue;
            td_ped_ovr|=bit;pk_fresh&=~bit;pk_u[i]=td_traffic_u[TD_POLICE_SLOT];pk_v[i]=td_traffic_v[TD_POLICE_SLOT];
            pk_look[i]=LF_LOOK_OFFICER;pk_mode[i]=PK_CHASE;pk_dir[i]=0;TD_PALETTE(a)=LF_OFFICER_PAL;count++;
            break;
        }
    }
    lf_drop=0;
    if(lf_patrol){
        /* The same patrol car resumes a pursuit it had given up. */
        if(tr_mode[TD_POLICE_SLOT]==TR_PARK){tr_mode[TD_POLICE_SLOT]=TR_CHASE;tr_timer[TD_POLICE_SLOT]=0;}
    }else if(lf_beat&&!(td_tr_ctrl&(1<<TD_POLICE_SLOT))&&lf_dist(td_traffic_u[TD_POLICE_SLOT]>>4,pu)<200&&
             lf_dist(td_traffic_v[TD_POLICE_SLOT]>>4,pv)<180){
        /* The cruiser on its beat nearby takes up the pursuit at once. */
        td_lf_own_car(TD_POLICE_SLOT,TR_CHASE);lf_patrol=1;lf_beat=0;lf_unit=LF_UNIT_CRUISER;lf_stuck=0;
        tr_timer[TD_POLICE_SLOT]=20;
    }else if(!(td_tr_ctrl&(1<<TD_POLICE_SLOT))&&
             !lf_on_screen(td_traffic_u[TD_POLICE_SLOT]>>4,td_traffic_v[TD_POLICE_SLOT]>>4)&&lf_spot(1,&u,&v)){
        /* The slot-4 car leaves its lane out of view; a patrol car arrives
         * from just beyond the screen edge after a short delay. */
        td_lf_own_car(TD_POLICE_SLOT,TR_CHASE);lf_patrol=1;lf_stuck=0;lf_lost=0;TD_PALETTE(&actors[2+TD_POLICE_SLOT])=TD_PAL_BLUE;
        /* Serious trouble brings an SUV; otherwise a cruiser, or now and
         * then an unmarked car. */
        lf_unit=td.wanted>=3?LF_UNIT_SUV:(lf_random()&3)==0?LF_UNIT_UNMARKED:LF_UNIT_CRUISER;
        td_traffic_u[TD_POLICE_SLOT]=u<<4;td_traffic_v[TD_POLICE_SLOT]=v<<4;tr_timer[TD_POLICE_SLOT]=60;
    }
}

/* Sample a straight line every eight pixels: buildings stop bullets. One
 * division sets Q4 increments; each sample is a single collision byte. */
static UBYTE lf_line_clear(UWORD u,UWORD v,UWORD x,UWORD y){
    WORD du=(WORD)x-(WORD)u,dv=(WORD)y-(WORD)v,su,sv;UBYTE i,steps;UWORD span=lf_abs(du)>lf_abs(dv)?lf_abs(du):lf_abs(dv),pu,pv;
    steps=(UBYTE)(span>>3);if(steps>14)return FALSE;
    if(steps<2)return TRUE;
    su=(du*16)/steps;sv=(dv*16)/steps;pu=u<<4;pv=v<<4;
    for(i=1;i<steps;i++){pu+=su;pv+=sv;if(!lf_walk(pu>>4,pv>>4))return FALSE;}
    return TRUE;
}

static void lf_hurt_player(UBYTE damage){
    if(!td.vitality||lf_down)return;
    td.vitality=damage>=td.vitality?0:td.vitality-damage;lf_hurt=30;lf_flash=1;lf_calm=0;
    if(lf_shake<4)lf_shake=4;
    td_audio_play(TD_AUDIO_IMPACT);td_lf_fx(FX_SPARK,td.u>>4,td.v>>4,8);
    if(!td.vitality){lf_down=110;td.speed=0;td_vx=td_vy=0;lf_stand_down();}
    else td_message(TD_MSG_HIT);
}

void td_lf_hurt(UBYTE damage) BANKED {lf_hurt_player(damage);}

/* From four stars officers within range shoot; at five the patrol car's
 * crew does too. Every shot is telegraphed: the shooter stops and aims,
 * flashing, for about a third of a second, so a courier who moves or
 * breaks the line of sight can avoid it. Rounds fly like the courier's
 * (td_shots.c) with a spread that grows with distance and when the
 * courier runs. After a shot the officer sidesteps. About one shot a
 * second, the shooter taking turns among the officers in range. */
static UBYTE lf_shooter_at(UBYTE i,UWORD *u,UWORD *v){
    if(i==8){
        if(!lf_patrol||tr_mode[TD_POLICE_SLOT]!=TR_CHASE)return FALSE;
        *u=td_traffic_u[TD_POLICE_SLOT]>>4;*v=td_traffic_v[TD_POLICE_SLOT]>>4;return TRUE;
    }
    if(!(td_ped_ovr&(1<<i))||pk_mode[i]!=PK_CHASE)return FALSE;
    *u=pk_u[i]>>4;*v=pk_v[i]>>4;return TRUE;
}
static void lf_police_fire(void){
    UBYTE i,k,in_range=TD_NONE,spread;UWORD pu=td.u>>4,pv=td.v>>4,su=0,sv=0,d;
    if(td.wanted<4||lf_down||lf_arrest||td.mode!=TD_ROAM){lf_aim_who=TD_NONE;return;}
    if(lf_aim_who!=TD_NONE){
        if(--lf_aim_time)return;
        i=lf_aim_who;lf_aim_who=TD_NONE;
        if(!lf_shooter_at(i,&su,&sv)||!lf_line_clear(su,sv,pu,pv)){lf_cop_cool=20;return;}
        d=lf_dist(su,pu)+lf_dist(sv,pv);
        spread=(UBYTE)(5+(d>>4))+(td_running?6:0);
        /* The round leaves the muzzle clear of the shooter (9 px along the
         * line of fire; 12 from the patrol car), never inside their reach. */
        {WORD du=(WORD)pu-(WORD)su,dv=(WORD)pv-(WORD)sv;UWORD m=lf_abs(du)>lf_abs(dv)?lf_abs(du):lf_abs(dv);UBYTE out=i==8?12:9;
         if(!m)m=1;su=(UWORD)((WORD)su+(WORD)((du*out)/(WORD)m));sv=(UWORD)((WORD)sv+(WORD)((dv*out)/(WORD)m));}
        if(!td_shot_fire(TD_SHOT_POLICE,su,sv-2,pu,pv,spread)){lf_cop_cool=10;return;}
        lf_cop_cool=100-(td.wanted*10);
        td_anim_spawn(TD_PART_FLASH,0,su,sv-6);
        td_audio_play(TD_AUDIO_IMPACT);
        lf_panic_r(su,sv,96,80);
        if(i<8){lf_strafe_who=i;lf_strafe=24;lf_strafe_dir=(td_tick&1)?1:-1;}
        return;
    }
    if(lf_cop_cool){lf_cop_cool--;return;}
    for(k=0;k<TD_PEDS&&in_range==TD_NONE;k++){
        i=(UBYTE)(k+td_tick)&7;
        if(!lf_shooter_at(i,&su,&sv))continue;
        if(lf_dist(su,pu)<80&&lf_dist(sv,pv)<72&&lf_line_clear(su,sv,pu,pv))in_range=i;
    }
    if(in_range==TD_NONE&&td.wanted>=5&&lf_shooter_at(8,&su,&sv)){
        if(lf_dist(su,pu)<88&&lf_dist(sv,pv)<80&&lf_line_clear(su,sv,pu,pv))in_range=8;
    }
    if(in_range==TD_NONE){lf_cop_cool=8;return;}
    /* Wind-up: a little shorter at five stars. */
    lf_aim_who=in_range;lf_aim_time=td.wanted>=5?16:22;
    if(in_range<8)pk_drawn&=~(1<<in_range);
}

/* ------------------------------------------------------------ public API */
void td_life_reset(UBYTE cold) BANKED {
    memset(pk_mode,0,sizeof(pk_mode));memset(tr_mode,0,sizeof(tr_mode));memset(tr_timer,0,sizeof(tr_timer));
    td_ped_ovr=td_tr_ctrl=pk_fresh=pk_lethal=pk_drawn=tr_spin=0;td_fx_kind=0;td_life_event=0;
    lf_warm=1;lf_stun=lf_flash=lf_punch=lf_hurt=lf_down=lf_arrest=lf_bust=lf_cop_cool=lf_rev_wait=lf_shake=lf_stuck=0;lf_exit_req=lf_spd_acc=lf_spd_ctl=0;lf_a_age=lf_b_age=255;
    lf_patrol=lf_lost=lf_drop=lf_amb=lf_vest=lf_calm=lf_beat=lf_seen=0;lf_seen_u=td.u;lf_seen_v=td.v;
    lf_aim_who=lf_strafe_who=td_hidden_slot=TD_NONE;lf_aim_time=lf_strafe=lf_combo=lf_combo_t=0;td_rolling=td_hitstop=0;
    lf_fx_end();td_shot_reset();
    camera_offset_x=0;
    if(cold){
        lf_chaos=0;
        if(!td.vitality||td.vitality>100)td.vitality=100;
        if(td.ammo>TD_AMMO_MAX)td.ammo=TD_AMMO_START;
        if(td.wanted>TD_WANTED_MAX)td.wanted=0;
        if(td.heat>TD_HEAT_SECONDS)td.heat=TD_HEAT_SECONDS;
    }
}

UBYTE td_life_locked(void) BANKED {return lf_down||lf_arrest||lf_hurt>22;}

static void lf_fx_tick(void){
    if(!td_fx_kind)return;
    if(!fx_timer||!--fx_timer){lf_fx_end();return;}
    if(td_fx_kind==FX_DROP){
        /* Walking over the magazine picks it up. */
        if(td.onfoot&&lf_dist(td.u,fx_u)<160&&lf_dist(td.v,fx_v)<192){
            td.ammo=td.ammo>TD_AMMO_MAX-6?TD_AMMO_MAX:td.ammo+6;td_message(TD_MSG_AMMO);td_audio_play(TD_AUDIO_COMPLETE);
            td_anim_spawn(TD_PART_POP,TD_FRAME_PICKUP_AMMO,fx_u>>4,(fx_v>>4)-8);
            td_fx_kind=0;lf_fx_end();
        }
        return;
    }
    if(td_fx_kind==FX_RUNNER){
        if(lf_walk((fx_u+fx_du)>>4,fx_v>>4))fx_u+=fx_du;else fx_v+=12;
    }
}

void td_life_tick(void) BANKED {
    if(lf_punch)lf_punch--;
    if(lf_combo_t)lf_combo_t--;
    if(lf_hurt)lf_hurt--;
    td_shot_tick();
    if(lf_down){
        if(!--lf_down)td_life_event=TD_EVENT_WASTED;
        lf_peds_tick();lf_cars_tick();lf_fx_tick();return;
    }
    if(lf_arrest){
        if(!--lf_arrest)td_life_event=TD_EVENT_BUSTED;
        lf_fx_tick();return;
    }
    if(td_ped_ovr)lf_peds_tick();
    if(td_tr_ctrl)lf_cars_tick();
    lf_fx_tick();
    if(!(td_tick&7)&&td.mode!=TD_RIDE)lf_ambient();
    if((td_tick&15)==9&&td.mode!=TD_RIDE)td_veh_hidden();
    if(td.wanted&&(td.mode==TD_ROAM||td.mode==TD_WAIT)){
        lf_police_fire();
        if(lf_bust>=LF_BUST_TICKS&&td.vitality&&!td_entry_timer){
            /* Officers hold the courier briefly before the arrest screen. */
            lf_arrest=36;lf_bust=0;td.speed=0;td_vx=td_vy=0;lf_stand_down();
            td_audio_play(TD_AUDIO_FAIL);
        }else if(lf_bust)lf_bust--;
    }else lf_bust=0;
}

/* The police see the courier: the patrol car or an officer within a block
 * with a clear line between them (buildings block it). */
static UBYTE lf_police_sees(void){
    UBYTE i;UWORD pu=td.u>>4,pv=td.v>>4,u,v;actor_t *a;
    if((lf_patrol||lf_beat)&&tr_mode[TD_POLICE_SLOT]!=TR_GONE){
        u=td_traffic_u[TD_POLICE_SLOT]>>4;v=td_traffic_v[TD_POLICE_SLOT]>>4;
        if(lf_dist(u,pu)<112&&lf_dist(v,pv)<96&&lf_line_clear(u,v,pu,pv))return TRUE;
    }
    for(i=0,a=&actors[TD_ACTOR_PEDS];i<TD_PEDS;i++,a++){
        if((a->flags&ACTOR_FLAG_HIDDEN)||!lf_is_officer(i))continue;
        u=a->pos.x>>5;v=a->pos.y>>5;
        if(lf_dist(u,pu)<96&&lf_dist(v,pv)<80&&lf_line_clear(u,v-4,pu,pv-4))return TRUE;
    }
    return FALSE;
}

void td_life_second(void) BANKED {
    /* Out of trouble for a while, the courier recovers to half vitality. */
    if(lf_calm<255)lf_calm++;
    if(!td.wanted&&lf_calm>=6&&td.vitality&&td.vitality<50&&!(td.seconds&1))td.vitality++;
    if(!td.wanted){if(lf_chaos)lf_chaos--;lf_seen=0;return;}
    /* Officers, the patrol car, the helicopter or the police boat with the
     * courier in sight keep the heat on; out of sight it drains, and each
     * star takes a little longer to lose than the one below it. */
    lf_seen=td.mode!=TD_RIDE&&(lf_police_sees()||td_overlay_spotted());
    if(lf_seen){td.heat=TD_HEAT_FOR(td.wanted);lf_seen_u=td.u;lf_seen_v=td.v;}
    else if(td.heat)td.heat--;
    if(!td.heat){
        td.wanted--;td.heat=TD_HEAT_FOR(td.wanted);
        if(!td.wanted){td.heat=0;lf_stand_down();td_message(TD_MSG_LOST);return;}
    }
    lf_recruit();
}

UBYTE td_life_routes(UBYTE moved) BANKED {
    UBYTE i,bit,old[TD_PEDS],pending;
    memcpy(old,td_ped_route,sizeof(old));
    pending=td_refresh_routes(td_ped_route,td_nearby_routes,moved||lf_warm);
    for(i=0,bit=1;i<TD_PEDS;i++,bit<<=1){
        if(td_ped_ovr&bit)td_ped_route[i]=old[i];
        else if(!lf_warm&&td_ped_route[i]!=old[i]&&td_ped_route[i]!=TD_NONE)pk_fresh|=bit;
    }
    /* The scene's first population is placed during the fade-in. */
    if(!pending)lf_warm=0;
    return pending;
}

UBYTE td_life_spray(void) BANKED {
    UBYTE was=td.wanted;
    if(td.cash<TD_SPRAY_PRICE){td_message(TD_MSG_NO_CASH);return FALSE;}
    td.cash-=TD_SPRAY_PRICE;td.wanted=0;td.heat=0;lf_chaos=0;lf_stand_down();td_car_damage=0;
    /* A stolen car comes out another colour (skipping the patrol navy);
     * the depot's own car keeps its livery. */
    if(td_car_colour!=TD_PAL_COURIER){td_car_colour=td_car_colour>=TD_PAL_VIOLET?TD_PAL_RED:td_car_colour+1;if(td_car_colour==TD_PAL_NAVY)td_car_colour++;}
    td_message(TD_MSG_SPRAY);td_audio_play(TD_AUDIO_COMPLETE);
    if(was)td_radio_say(TD_RADIO_SPRAY);
    return TRUE;
}

UBYTE td_life_buy(void) BANKED {
    if(td.cash<TD_SUPPLY_PRICE){td_message(TD_MSG_NO_CASH);return FALSE;}
    td.cash-=TD_SUPPLY_PRICE;
    td.ammo=td.ammo>TD_AMMO_MAX-12?TD_AMMO_MAX:td.ammo+12;
    td.vitality=100;td_car_damage=0;
    td_message(TD_MSG_SUPPLIES);
    return TRUE;
}

void td_life_busted(void) BANKED {
    UWORD fine=td.wanted?(UWORD)td.wanted*25:15;
    if(fine>td.cash)fine=td.cash;
    td.cash-=fine;td_life_fine=(UBYTE)fine;
    td.ammo>>=1;td.wanted=0;td.heat=0;lf_chaos=0;
    lf_stand_down();lf_arrest=0;lf_bust=0;td_life_event=0;
    if(td.job!=TD_NONE){td.job=TD_NONE;td.health=0;}
}

void td_life_hospital(UWORD *u,UWORD *v) BANKED {
    static const BYTE du[9]={0,0,0,-8,8,0,-16,16,0},dv[9]={0,-8,8,0,0,-16,0,0,16};
    UBYTE i;UWORD bill=td.cash<60?td.cash:60,x,y;
    td.cash-=bill;td_life_fine=(UBYTE)bill;
    td.vitality=100;td.wanted=0;td.heat=0;lf_chaos=0;
    lf_stand_down();lf_down=0;lf_hurt=0;td_life_event=0;
    if(td.job!=TD_NONE){td.job=TD_NONE;td.health=0;}
    *u=TD_HOSPITAL_U*16;*v=TD_HOSPITAL_V*16;
    for(i=0;i<9;i++){
        x=TD_HOSPITAL_U+du[i];y=TD_HOSPITAL_V+dv[i];
        if(!td_district_walkable(TD_HOSPITAL_DISTRICT,x,y))continue;
        if(td.park_district==TD_HOSPITAL_DISTRICT&&lf_dist(x<<4,td.park_u)<176&&lf_dist(y<<4,td.park_v)<176)continue;
        *u=x<<4;*v=y<<4;return;
    }
}

