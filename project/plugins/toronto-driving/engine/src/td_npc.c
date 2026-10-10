#pragma bank 255
/* This module reads the generated people tables (td_people_data.h). */
#define TD_PEOPLE_DATA
/* People with something to do (td_npc.h). A walker slot shows the look its
 * neighbourhood picks (td_people.c). Plain walkers stay with the native
 * route layout; everyone else is taken over here (td_life's owned modes
 * PK_NPC..): people who sit by a wall, sleep, busk or hold up a sign stay
 * in place; joggers, skaters, dogs, the elderly and the antagonists move at
 * their own pace along the sidewalk; tourists stop for a photo; geese stand
 * their ground; raccoons, squirrels and pigeons scatter. Street toughs,
 * rival couriers and road ragers can start a fight; a pickpocket lifts
 * cash and runs. Slots are let go once they are out of the walkers' keep
 * range, and someone placed while their spot is in view stays hidden
 * until it has been out of view once, so nobody appears on screen. */
#include <string.h>
#include "td_life_int.h"
#include "td_npc.h"
#include "td_people.h"
#include "td_anim.h"
#include "td_audio.h"
#include "td_daynight.h"
#include "input.h"
#include "td_interior.h"

UBYTE td_npc_alarm,td_npc_hostile;
UWORD td_npc_alarm_u,td_npc_alarm_v;
static UBYTE np_route[TD_PEDS],np_behave[TD_PEDS],np_timer[TD_PEDS],np_cool[TD_PEDS],np_windup[TD_PEDS],np_cash[TD_PEDS];
/* Bit i: slot i has been out of view since it was placed (drawn when in view). */
static UBYTE np_seen;
/* Bit i: slot i was just taken over and is drawn on the next update
 * whatever its parity (it must not show where the native layout left it). */
static UBYTE np_redraw;
static UBYTE np_tip_cool;
/* Seconds before another pickpocket tries (one at a time, not every block). */
static UBYTE np_theft_cool,np_last_second;
/* Updates since the door within reach was last looked for. */
static UBYTE np_door_tick;
/* Ticks A has been held while standing (a haymaker once charged). */
static UBYTE np_charge;
#define NP_CHARGED 30
extern UBYTE td_swimming,td_ped_flip;
/* Pace along the sidewalk in Q4 per tick (a walker's is about 3), by behaviour. */
static const UBYTE np_pace[TD_BH_INDOOR+1]={
    3,2,9,12,3, 0,0,0,0,0, 4,3,6,7, 2,5,1,6, 3,0};
/* Q4 reach of the walkers' keep range (TD_ROUTE_KEEP_U/V, td_routes.c). */
#define NP_KEEP_U (136*16)
#define NP_KEEP_V (120*16)

static UBYTE np_fixed(UBYTE b){return b==TD_BH_SIT||b==TD_BH_SLEEP||b==TD_BH_BUSK||b==TD_BH_SIGN||b==TD_BH_BEG;}
static UBYTE np_fighter(UBYTE b){return b==TD_BH_TOUGH||b==TD_BH_RIVAL||b==TD_BH_RAGER;}
static UBYTE np_skittish(UBYTE b){return b==TD_BH_SCURRY||b==TD_BH_PIGEON;}

static void np_own(UBYTE i,UWORD u,UWORD v,UBYTE mode){
    UBYTE bit=1<<i;
    if(pk_fresh&bit)np_seen&=~bit;else np_seen|=bit;
    td_ped_ovr|=bit;pk_fresh&=~bit;pk_drawn&=~bit;lf_vest&=~bit;np_redraw|=bit;
    pk_u[i]=u;pk_v[i]=v;pk_look[i]=0;pk_mode[i]=mode;pk_timer[i]=0;
}
/* One step toward (du, dv) at up to speed Q4 on each axis, sliding along
 * walls; faces the way it went (0 east, 1 west, 2 south, 3 north). */
static void np_step(UBYTE i,WORD du,WORD dv,UBYTE speed){
    UWORD u=pk_u[i],v=pk_v[i];WORD su,sv,lim=speed;
    su=du>lim?lim:du<-lim?-lim:du;sv=dv>lim?lim:dv<-lim?-lim:dv;
    if(su&&lf_walk((u+su)>>4,v>>4)){u+=su;pk_dir[i]=su>0?0:1;}
    if(sv&&lf_walk(u>>4,(v+sv)>>4)){v+=sv;if(lf_abs(dv)>lf_abs(du))pk_dir[i]=sv>0?2:3;}
    pk_u[i]=u;pk_v[i]=v;
}
static void np_release(UBYTE i){
    UBYTE bit=1<<i;
    td_ped_ovr&=~bit;pk_drawn&=~bit;td_npc_hostile&=~bit;pk_mode[i]=0;
    td_ped_route[i]=TD_NONE;np_route[i]=TD_NONE;np_cash[i]=0;np_windup[i]=0;
    actors[TD_ACTOR_PEDS+i].flags|=ACTOR_FLAG_HIDDEN;
}

void td_npc_reset(void) BANKED {
    td_people_init();
    memset(np_route,TD_NONE,sizeof(np_route));memset(np_behave,0,sizeof(np_behave));
    memset(np_cash,0,sizeof(np_cash));memset(np_windup,0,sizeof(np_windup));
    np_seen=0;np_redraw=0;td_npc_hostile=0;td_npc_alarm=0;np_tip_cool=0;np_charge=0;np_theft_cool=30;
}

/* Whole-pixel route walker position (the native layout's phase). */
static UWORD np_route_u(UBYTE i,UBYTE route){
    UBYTE s=(UBYTE)td.seconds,base,phase;
    base=(UBYTE)((UBYTE)(s<<3)+(UBYTE)(s<<2)+td.subsecond/5);
    phase=(UBYTE)(base+(UBYTE)(route<<5)+(UBYTE)(route<<2)+route)&127;
    return td_nearby_routes[i][0]+(phase<64?phase:127-phase);
}

/* Someone who stays put sits against the wall beside their route. */
static void np_settle(UBYTE i,UBYTE route){
    UWORD u=td_nearby_routes[i][0]+((route*13)&63),v=td_nearby_routes[i][1];
    if(!lf_walk(u,v-10))v-=3;else if(!lf_walk(u,v+10))v+=3;
    np_own(i,u<<4,v<<4,PK_SIT);
    td_people_pose(i,TD_POSE_ACT);
    np_timer[i]=(UBYTE)(route<<3);
}

void td_npc_refresh(void) BANKED {
    UBYTE i,bit,route,look,b;
    for(i=0,bit=1;i<TD_PEDS;i++,bit<<=1){
        route=td_ped_route[i];
        if(route==TD_NONE){np_route[i]=TD_NONE;continue;}
        if((td_ped_ovr&bit)||route==np_route[i])continue;
        np_route[i]=route;np_windup[i]=0;np_cash[i]=0;np_cool[i]=0;td_npc_hostile&=~bit;
        look=td_people_pick(td.district,route,td_nearby_routes[i][0],td_nearby_routes[i][1]);
        td_people_show(i,look,TD_POSE_SIDE);
        b=np_behave[i]=td_look_behave[look];
        if(np_fixed(b))np_settle(i,route);
        else if(b!=TD_BH_WALK&&b!=TD_BH_PHOTO&&b!=TD_BH_OFFICER&&b!=TD_BH_INDOOR){
            np_own(i,np_route_u(i,route)<<4,td_nearby_routes[i][1]<<4,PK_STROLL);
            pk_dir[i]=route&1;np_timer[i]=route;
        }
    }
}

/* ------------------------------------------------------------ talk */
static void np_say(UBYTE look,const char *a,const char *b){
    char l1[19],l2[19];
    if(a){strcpy(l1,a);strcpy(l2,b);}
    else{td_people_text(look,TD_PT_TALK,l1);td_people_text(look,TD_PT_TALK+1,l2);}
    td_radio_speech(l1,l2);
}
static const char *const np_compass[8]={"E","SE","S","SW","W","NW","N","NE"};
/* "BOX 3 BLOCKS NE": the nearest lost parcel still lying in this scene. */
static UBYTE np_hint(char *line){
    UWORD u,v,au,av,pu=td.u>>4,pv=td.v>>4;UBYTE d,blocks,east,south;
    if(!td_street_hint(td.district,pu,pv,&u,&v))return FALSE;
    east=u>=pu;south=v>=pv;au=east?u-pu:pu-u;av=south?v-pv:pv-v;
    if(au>(av<<1))d=east?0:4;
    else if(av>(au<<1))d=south?2:6;
    else d=south?(east?1:3):(east?7:5);
    blocks=(UBYTE)((au+av)>>7);if(!blocks)blocks=1;if(blocks>9)blocks=9;
    strcpy(line,"BOX 0 BLOCKS ");line[4]='0'+blocks;if(blocks==1)strcpy(line+11," ");
    strcat(line,np_compass[d]);
    return TRUE;
}
UBYTE td_npc_talk(void) BANKED {
    UBYTE i,bit,best=TD_NONE,b,look;UWORD d,best_d=0xFFFF,pu=td.u>>4,pv=td.v>>4,u,v;actor_t *a=&actors[TD_ACTOR_PEDS];
    char dir[19];
    for(i=0,bit=1;i<TD_PEDS;i++,bit<<=1,a++){
        if((a->flags&ACTOR_FLAG_HIDDEN)||td_ped_route[i]==TD_NONE||td_slot_look[i]>=TD_LOOKS)continue;
        /* Not someone running, dazed, down or chasing the courier. */
        if((td_ped_ovr&bit)&&pk_mode[i]<PK_NPC)continue;
        if(td_npc_hostile&bit)continue;
        u=a->pos.x>>5;v=a->pos.y>>5;
        if(lf_dist(u,pu)>=18||lf_dist(v,pv)>=16)continue;
        d=lf_dist(u,pu)+lf_dist(v,pv);
        if(d<best_d){best_d=d;best=i;}
    }
    if(best==TD_NONE)return FALSE;
    look=td_slot_look[best];b=np_behave[best];
    if((b==TD_BH_SIT||b==TD_BH_BEG||b==TD_BH_SIGN)&&td.cash>=2){
        /* A couple of dollars, and now and then they say where they have
         * seen a box lying about. */
        td.cash-=2;td_audio_play(TD_AUDIO_PICKUP);
        td_anim_spawn(TD_PART_POP,TD_FRAME_PICKUP_CASH,td.u>>4,(td.v>>4)-10);
        if(!np_tip_cool&&np_hint(dir)){
            np_tip_cool=3;
            np_say(look,"THANKS. I SAW A",dir);
        }else{
            if(np_tip_cool)np_tip_cool--;
            np_say(look,"THANK YOU, FRIEND.","TAKE CARE OUT HERE.");
        }
        return TRUE;
    }
    if(b==TD_BH_BUSK&&td.cash>=1){
        td.cash--;td_audio_play(TD_AUDIO_COMPLETE);
        td_anim_spawn(TD_PART_POP,TD_FRAME_EMOTE_NOTE,pk_u[best]>>4,(pk_v[best]>>4)-14);
        np_say(look,"MUCH OBLIGED.","THIS ONE'S FOR YOU.");
        return TRUE;
    }
    if(b==TD_BH_SLEEP){np_say(look,"ZZZ...","(THEY ARE ASLEEP.)");return TRUE;}
    np_say(look,0,0);
    if(b==TD_BH_TOUGH&&!np_cool[best]){np_timer[best]=200;}
    return TRUE;
}

/* ------------------------------------------------------------ behaviour */
static void np_emote(UBYTE i,UBYTE frame){td_anim_spawn(TD_PART_POP,frame,pk_u[i]>>4,(pk_v[i]>>4)-14);}

/* Start a fight: the fighter faces the courier, an anger mark goes up. */
static void np_provoke(UBYTE i){
    UBYTE bit=1<<i;
    if(td_npc_hostile&bit)return;
    td_npc_hostile|=bit;pk_mode[i]=PK_HOSTILE;np_timer[i]=0;np_windup[i]=0;np_cool[i]=20;
    np_emote(i,np_behave[i]==TD_BH_GOOSE?TD_FRAME_EMOTE_ALERT:TD_FRAME_EMOTE_ANGER);
    if(np_behave[i]!=TD_BH_GOOSE){td_message(TD_MSG_BRAWL);np_say(td_slot_look[i],0,0);}
    else td_message(TD_MSG_GOOSE);
}

/* The courier is pushed back a few pixels if the ground allows. */
static void np_push(WORD du,WORD dv){
    UWORD u=td.u+du,v=td.v+dv;
    if(lf_walk(u>>4,v>>4)){td.u=u;td.v=v;}
}

static void np_strike(UBYTE i,WORD du,WORD dv){
    UBYTE b=np_behave[i];
    td_lf_hurt(b==TD_BH_GOOSE?2:b==TD_BH_RIVAL?4:7);
    /* Say what hit them (the generic notice is about gunfire). */
    if(td.vitality)td_message(b==TD_BH_GOOSE?TD_MSG_GOOSE:TD_MSG_BRAWL);
    np_push(du<0?-48:du>0?48:0,dv<0?-48:dv>0?48:0);
    td_anim_spawn(TD_PART_POP,TD_FRAME_EMOTE_STAR,td.u>>4,(td.v>>4)-10);
    if(td_hitstop<2)td_hitstop=2;
    lf_shake=3;td_audio_play(TD_AUDIO_IMPACT);
}

static void np_tick(UBYTE i,UBYTE bit){
    UBYTE b=np_behave[i],mode=pk_mode[i],pace=np_pace[b];
    WORD du=(WORD)td.u-(WORD)pk_u[i],dv=(WORD)td.v-(WORD)pk_v[i];
    UWORD adu=lf_abs(du),adv=lf_abs(dv),u;
    if(np_cool[i])np_cool[i]--;
    /* Out of the walkers' keep range: the slot is free again. */
    if(((td_tick&7)==i)&&(adu>NP_KEEP_U||adv>NP_KEEP_V)){np_release(i);return;}
    /* Trouble nearby: people who stay put get up and run, birds and
     * small animals scatter; fighters and geese stand their ground. */
    if(td_npc_alarm&&!(td_npc_hostile&bit)&&lf_dist(pk_u[i]>>4,td_npc_alarm_u)<128&&lf_dist(pk_v[i]>>4,td_npc_alarm_v)<112){
        if(np_skittish(b)){pk_mode[i]=PK_RUN;np_timer[i]=90;return;}
        if(b!=TD_BH_GOOSE&&!np_fighter(b)){pk_mode[i]=PK_FLEE;pk_timer[i]=160;td_people_pose(i,TD_POSE_SIDE);return;}
    }
    switch(mode){
    case PK_SIT:
        np_timer[i]++;
        if(b==TD_BH_PHOTO){
            /* A tourist's photo: camera up, a flash, then on their way. */
            if(np_timer[i]==30)np_emote(i,TD_FRAME_EMOTE_FLASH);
            if(np_timer[i]>=60){pk_mode[i]=PK_STROLL;td_people_pose(i,TD_POSE_SIDE);}
        }else if(b==TD_BH_SLEEP){if(!(np_timer[i]&127)&&lf_on_screen(pk_u[i]>>4,pk_v[i]>>4))np_emote(i,TD_FRAME_EMOTE_ZZZ);}
        else if(b==TD_BH_BUSK){if(!(np_timer[i]&63)&&lf_on_screen(pk_u[i]>>4,pk_v[i]>>4))np_emote(i,TD_FRAME_EMOTE_NOTE);}
        break;
    case PK_STROLL:
        /* Along the sidewalk: turn round at anything solid. */
        if(pace){
            u=pk_u[i]+(pk_dir[i]?-(WORD)pace:(WORD)pace);
            if(lf_walk((u>>4)+(pk_dir[i]?-4:4),pk_v[i]>>4))pk_u[i]=u;else pk_dir[i]^=1;
        }
        if(!td.onfoot||td.mode!=TD_ROAM)break;
        if(b==TD_BH_GOOSE){if(adu<28*16&&adv<20*16&&!np_cool[i])np_provoke(i);}
        else if(np_skittish(b)){
            if(adu<(td_running?64*16:36*16)&&adv<(td_running?48*16:28*16)){pk_mode[i]=PK_RUN;np_timer[i]=90;np_emote(i,TD_FRAME_EMOTE_DUST);}
        }else if(b==TD_BH_TOUGH||b==TD_BH_RAGER){
            /* Linger beside a tough and they take it badly. */
            if(adu<26*16&&adv<18*16){if(++np_timer[i]>=200&&!np_cool[i])np_provoke(i);}
            else if(np_timer[i]>140)np_timer[i]-=2;
        }else if(b==TD_BH_RIVAL){
            /* A rival courier shoulder-checks one carrying a parcel. */
            if(td.job!=TD_NONE&&td.stage&&adu<14*16&&adv<12*16&&!np_cool[i]){
                np_cool[i]=255;np_strike(i,du,dv);
                if(td.health>5)td.health-=5;
                td_message(TD_MSG_SHOVED);np_say(td_slot_look[i],0,0);
            }
        }else if(b==TD_BH_THIEF&&!np_cash[i]&&td.cash&&!np_theft_cool&&!td_running){
            /* A pickpocket closes in, brushes past and runs with the cash. */
            if(adu<56*16&&adv<40*16){
                if(adu<10*16&&adv<10*16){
                    np_cash[i]=td.cash<12?(UBYTE)td.cash:12+(td.seconds&7);td.cash-=np_cash[i];np_theft_cool=120;
                    pk_mode[i]=PK_RUN;np_timer[i]=255;np_emote(i,TD_FRAME_PICKUP_CASH);
                    td_message(TD_MSG_PICKPOCKET);td_audio_play(TD_AUDIO_FAIL);
                }else np_step(i,du,dv,4);
            }
        }
        break;
    case PK_HOSTILE:
        if(adu>160*16||adv>144*16||!td.onfoot||!td.vitality){
            td_npc_hostile&=~bit;pk_mode[i]=PK_STROLL;np_cool[i]=255;np_windup[i]=0;break;
        }
        if(np_windup[i]){
            /* The swing is telegraphed (fists up, flashing): step away,
             * roll through it or punch first for a counter. */
            if(!--np_windup[i]){
                if(adu<18*16&&adv<16*16)np_strike(i,du,dv);
                np_cool[i]=b==TD_BH_GOOSE?50:36;
                if(b==TD_BH_GOOSE&&++np_timer[i]>=3){td_npc_hostile&=~bit;pk_mode[i]=PK_STROLL;np_cool[i]=255;}
            }
            break;
        }
        if(adu<14*16&&adv<12*16){
            if(!np_cool[i]&&!td_rolling){np_windup[i]=b==TD_BH_GOOSE?12:18;td_people_pose(i,TD_POSE_ACT);}
        }else np_step(i,du,dv,b==TD_BH_GOOSE?6:10);
        break;
    case PK_RUN:
        /* Away at speed (pigeons up and away), gone once out of view. */
        np_step(i,-du,-dv,b==TD_BH_THIEF?11:16);
        if(b==TD_BH_PIGEON)pk_v[i]-=24;
        if(!np_timer[i]||!--np_timer[i]||!lf_on_screen(pk_u[i]>>4,pk_v[i]>>4)){
            /* A pickpocket who gets away keeps the cash. */
            np_cash[i]=0;np_release(i);
        }
        break;
    }
}

/* Draw slot i in its current mode (pose frames and visibility). */
static void np_draw(UBYTE i,UBYTE bit){
    actor_t *p=&actors[TD_ACTOR_PEDS+i];UBYTE mode=pk_mode[i],f=TD_PEOPLE_FRAME(i),b=np_behave[i],on;
    /* Drawn within the walkers' keep range, like the native layout. */
    on=lf_dist(pk_u[i],td.u)<136*16&&lf_dist(pk_v[i],td.v)<120*16;
    if(!lf_on_screen(pk_u[i]>>4,pk_v[i]>>4))np_seen|=bit;
    if(!(np_seen&bit)){p->flags|=ACTOR_FLAG_HIDDEN;return;}
    if(mode==PK_SIT){
        if(td_slot_pose[i]!=TD_POSE_ACT)td_people_pose(i,TD_POSE_ACT);
        f+=(np_timer[i]>>(b==TD_BH_BUSK?3:5))&1;
    }else if(mode==PK_HOSTILE&&np_windup[i]){
        if(td_slot_pose[i]!=TD_POSE_ACT)td_people_pose(i,TD_POSE_ACT);f+=(np_windup[i]>>2)&1;
        TD_PALETTE(p)=(np_windup[i]&4)?TD_PEOPLE_PAL(TD_PAL_RED):td_look_pal[td_slot_look[i]];
    }else{
        UBYTE d=pk_dir[i]&3,pose=d==2?TD_POSE_FRONT:d==3?TD_POSE_BACK:b==TD_BH_PIGEON&&mode==PK_RUN?TD_POSE_ACT:TD_POSE_SIDE;
        if(td_slot_pose[i]!=pose)td_people_pose(i,pose);
        if(d==1)f+=2;
        f+=(td_tick>>(np_pace[b]>=6||mode!=PK_STROLL?2:3))&1;
        TD_PALETTE(p)=td_look_pal[td_slot_look[i]];
    }
    if(p->frame_start!=f||p->frame_end!=f+1)actor_set_frames(p,f,f+1);
    p->anim_tick=255;
    lf_place_q4(p,pk_u[i],pk_v[i]);
    if(on)p->flags&=~ACTOR_FLAG_HIDDEN;else p->flags|=ACTOR_FLAG_HIDDEN;
}

void td_npc_tick(void) BANKED {
    UBYTE i,bit,mode;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE){td_npc_alarm=0;np_charge=0;return;}
    /* After a jab, keep A held while standing still to wind up a haymaker;
     * a spark over the courier says it is ready, letting go throws it. */
    if(td.onfoot&&td.mode==TD_ROAM&&!td_swimming&&!td_rolling&&!(joy&(J_LEFT|J_RIGHT|J_UP|J_DOWN))){
        if(INPUT_A){
            if(np_charge<255&&++np_charge==NP_CHARGED){
                td_anim_spawn(TD_PART_POP,TD_FRAME_EMOTE_STAR,td.u>>4,(td.v>>4)-16);td_audio_play(TD_AUDIO_MENU);
            }
        }else{
            if(np_charge>=NP_CHARGED)td_life_haymaker();
            np_charge=0;
        }
    }else np_charge=0;
    for(i=0,bit=1;i<TD_PEDS;i++,bit<<=1){
        if(!(td_ped_ovr&bit)||np_route[i]==TD_NONE)continue;
        mode=pk_mode[i];
        if(mode<PK_NPC){
            /* Street life has them (knocked down, dazed, fleeing). A fighter
             * who was only dazed fights on; a counter or a knock-down ends
             * the fight, and a pickpocket who goes down drops the cash. */
            if(np_cash[i]&&(mode==PK_STUN||mode==PK_FLY)){
                td.cash+=np_cash[i];np_cash[i]=0;td_message(TD_MSG_RECOVERED);td_audio_play(TD_AUDIO_PICKUP);
                td_anim_spawn(TD_PART_POP,TD_FRAME_PICKUP_CASH,pk_u[i]>>4,(pk_v[i]>>4)-10);
            }
            if(td_npc_hostile&bit){
                if(mode==PK_STUN&&np_windup[i]){
                    /* Countered mid-swing: down they go. */
                    np_windup[i]=0;td_npc_hostile&=~bit;
                    td_lf_knock(i,lf_abs((WORD)pk_u[i]-(WORD)td.u)>8?(pk_u[i]>td.u?28:-28):0,pk_v[i]>td.v?10:-10,0);
                    td_message(TD_MSG_COUNTER);if(td_hitstop<6)td_hitstop=6;lf_shake=4;
                    td_anim_spawn(TD_PART_POP,TD_FRAME_EMOTE_STAR_BIG,pk_u[i]>>4,(pk_v[i]>>4)-8);
                }else if(mode==PK_FLY){
                    /* Beaten: they drop what they had on them. */
                    td_npc_hostile&=~bit;
                    if(np_behave[i]!=TD_BH_GOOSE){
                        td.cash+=10;td_message(TD_MSG_REWARD);
                        td_anim_spawn(TD_PART_POP,TD_FRAME_PICKUP_CASH,pk_u[i]>>4,(pk_v[i]>>4)-10);
                    }
                }else if(mode==PK_FLEE&&np_behave[i]!=TD_BH_GOOSE){pk_mode[i]=PK_HOSTILE;np_cool[i]=24;}
            }
            continue;
        }
        np_tick(i,bit);
        /* Drawn on the updates the native layout skips (30 a second, like
         * the walkers), so a busy update does not do both. */
        if((td_ped_ovr&bit)&&(!td_ped_flip||(np_redraw&bit))){np_redraw&=~bit;np_draw(i,bit);}
    }
    td_npc_alarm=0;
    if((UBYTE)td.seconds!=np_last_second){np_last_second=(UBYTE)td.seconds;if(np_theft_cool)np_theft_cool--;}
    /* The door within reach shows its name on the HUD. */
    if(!(++np_door_tick&7)&&td_door_scan())td_ui_draw();
    /* Now and then a tourist in view stops to take a photo. */
    if(!(td_tick&63)){
        i=(UBYTE)(td.seconds+td_tick)&7;bit=1<<i;
        if(!(td_ped_ovr&bit)&&td_ped_route[i]!=TD_NONE&&np_behave[i]==TD_BH_PHOTO&&np_route[i]==td_ped_route[i]){
            actor_t *a=&actors[TD_ACTOR_PEDS+i];
            if(!(a->flags&ACTOR_FLAG_HIDDEN)){
                np_own(i,a->pos.x>>1,a->pos.y>>1,PK_SIT);np_seen|=bit;np_timer[i]=0;
                pk_dir[i]=(UBYTE)td_tick&1;td_people_pose(i,TD_POSE_ACT);
            }
        }
    }
}

static UBYTE np_look_of(UBYTE behave){
    UBYTE k;
    for(k=0;k<TD_LOOKS;k++)if(td_look_behave[k]==behave)return k;
    return 1;
}
/* A driver the courier rammed gets out and comes at them (td_drive.c). */
void td_npc_rager(UWORD u,UWORD v) BANKED {
    UBYTE i,bit;
    for(i=0,bit=1;i<TD_PEDS;i++,bit<<=1){
        if(td_ped_ovr&bit)continue;
        if(td_ped_route[i]!=TD_NONE&&!(actors[TD_ACTOR_PEDS+i].flags&ACTOR_FLAG_HIDDEN))continue;
        if(td_ped_route[i]==TD_NONE)td_ped_route[i]=(UBYTE)(td.seconds|1)&~4;
        np_route[i]=td_ped_route[i];np_behave[i]=TD_BH_RAGER;np_cash[i]=0;np_windup[i]=0;np_cool[i]=30;
        td_people_show(i,np_look_of(TD_BH_RAGER),TD_POSE_SIDE);
        np_own(i,u<<4,v<<4,PK_STROLL);np_seen|=bit;
        return;
    }
}
