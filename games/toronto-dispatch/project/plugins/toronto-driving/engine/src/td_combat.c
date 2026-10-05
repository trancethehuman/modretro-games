#pragma bank 255
#include "td_combat.h"
#include "td_game.h"
#include "td_people.h"
#include "td_motion.h"
#include "td_district.h"
#include "td_hospital.h"
#include "td_roads.h"
#include "td_audio.h"
#include "td_city_sprites.h"
#include "actor.h"

static UBYTE td_player_delay,td_patrol_delay,td_immunity,td_down;
static UBYTE td_patrol_health,td_patrol_down;
static UBYTE td_arrest;
static UWORD td_combat_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static UBYTE td_combat_timer(UBYTE timer,UWORD elapsed){return elapsed>=timer?0:timer-elapsed;}
static UBYTE td_combat_active(void){
    return td.mode==TD_ROAM&&td.onfoot&&!td_entry_timer&&
        td.district<TD_DISTRICT_COUNT&&td.district==td_district_current();
}
/* Sample the complete line at <=4px gaps: no one-tile wall can be skipped,
 * including diagonals. Geometry queries preserve collision-bank/hit state.
 * Reject world wrapping before signed subtraction/division. */
static UBYTE td_combat_clear(UWORD u,UWORD v,UWORD x,UWORD y,UBYTE police){
    WORD du,dv;UWORD span,other;UBYTE i,steps,j;UWORD pu,pv;actor_t *car;
    if(u>=1024||x>=1024||v>=976||y>=976)return FALSE;
    du=(WORD)x-u;dv=(WORD)y-v;span=du<0?-du:du;other=dv<0?-dv:dv;
    if(other>span)span=other;
    if(span>96)return FALSE;
    if(!span)return td_road_walkable(u,v);
    steps=(span+3)>>2;
    for(i=0;i<=steps;i++){
        pu=u+du*i/steps;pv=v+dv*i/steps;
        if(!td_road_walkable(pu,pv))return FALSE;
        if(police)for(j=2,car=&actors[2];j<19;j++,car++){
            if(j==9){j=17;car=&actors[17];}
            if(j==4||(car->flags&ACTOR_FLAG_HIDDEN))continue;
            if(td_combat_distance(pu,car->pos.x>>5)<8&&td_combat_distance(pv,car->pos.y>>5)<8)return FALSE;
        }
    }
    return TRUE;
}
void td_combat_reset(void) BANKED {
    td_player_delay=td_patrol_delay=td_immunity=td_down=td_patrol_down=td_arrest=0;
    td_patrol_health=50;
}
void td_combat_resume_downed(void) BANKED {
    if(!td.vitality&&td.onfoot)td_down=120;
}
void td_combat_arrested(void) BANKED {
    td_player_delay=td_patrol_delay=0;td_arrest=60;
    if(td.onfoot){
        td_player_sprite_restore();PLAYER.frame=PLAYER.frame_start=32+(td_walk_dir<4?td_walk_dir*2:0);
        PLAYER.frame_end=PLAYER.frame+1;PLAYER.anim_tick=255;
    }
}
static UBYTE td_combat_is_locked(void){return td_down||td_arrest||td_immunity>30;}
UBYTE td_combat_locked(void) BANKED {return td_combat_is_locked();}
UBYTE td_combat_police_disabled(void) BANKED {return td_patrol_down!=0;}
UBYTE td_combat_damage(UBYTE damage,BYTE dx,BYTE dy) BANKED {
    (void)dx;(void)dy; /* Original hit/down art; no unvalidated position shove. */
    if(!damage||!td_combat_active()||!td.vitality||td_immunity||td_down||td_arrest)return 0;
    td.vitality=damage>=td.vitality?0:td.vitality-damage;
    td_immunity=45;td.speed=0;td_vx=td_vy=0;
    td_audio_play(TD_AUDIO_IMPACT);
    if(!td.vitality){td_down=120;return TD_COMBAT_CHANGED|TD_COMBAT_DOWN;}
    return TD_COMBAT_CHANGED;
}
/* All ordinary cars block a ray. Only the actual visible living human or
 * patrol nearest the muzzle can receive a hit, even when actors overlap. */
UBYTE td_combat_fire(void) BANKED {
    UBYTE i,kind,index=TD_NONE,range=81,dist,half,dir=td_walk_dir;
    UWORD u=td.u>>4,v=td.v>>4,x,y;WORD along,across;
    actor_t *target;
    if(!td_combat_active()||td_combat_is_locked()||!td.vitality||td_player_delay||dir>3)return TD_COMBAT_BLOCKED;
    if(!td.ammo)return TD_COMBAT_EMPTY;
    td.ammo--;td_player_delay=18;td.wanted=td.wanted<3?td.wanted+1:3;td.wanted_left=30;
    td_audio_play(TD_AUDIO_IMPACT);
    for(i=2;i<19;i++){
        target=&actors[i];if(target->flags&ACTOR_FLAG_HIDDEN)continue;
        /* Actors9..16 are original civilians;2..8 and17/18 are cars. */
        half=i>=9&&i<=16?3:7;
        x=target->pos.x>>5;y=target->pos.y>>5;
        along=dir<2?(WORD)x-u:(WORD)y-v;across=dir<2?(WORD)y-v:(WORD)x-u;
        if(dir==1||dir==3)along=-along;
        if(along<0||along>80||across<-(WORD)(half+2)||across>(WORD)(half+2))continue;
        dist=along>half?along-half:0;
        if(dist>=range)continue;
        range=dist;index=i;
    }
    if(index!=TD_NONE){
        /* Test the actual cardinal ray to the selected body's front edge;
         * do not bend it toward an offset actor centre around wall corners. */
        x=u+(dir==0?range:dir==1?-(WORD)range:0);
        y=v+(dir==2?range:dir==3?-(WORD)range:0);
        if(td_combat_clear(u,v,x,y,FALSE)){
            if(index>=9&&index<=16){
                kind=td_people_shot(index-9,dir==0?1:dir==1?-1:0,dir==2?1:dir==3?-1:0);
                if(kind){td.wanted=td.wanted<3?td.wanted+1:3;}
            }else if(index==4&&!td_patrol_down){
                td.wanted=3;
                td_patrol_health=td_patrol_health>25?td_patrol_health-25:0;
                if(!td_patrol_health){td_patrol_down=180;td_patrol_delay=0;}
            }
        }
    }
    return TD_COMBAT_SHOT;
}
/* Hospital candidates belong to Core even while another scene is loaded.
 * A seven-pixel foot body covers at most two tile rows/columns: its four
 * corners cover every overlapped tile. Never borrow the old scene's grid. */
static UBYTE td_combat_hospital_clear(UWORD u,UWORD v){
    if(u<3||v<3||u>1020||v>972||
       !td_district_walkable(TD_HOSPITAL_DISTRICT,u-3,v-3)||
       !td_district_walkable(TD_HOSPITAL_DISTRICT,u+3,v-3)||
       !td_district_walkable(TD_HOSPITAL_DISTRICT,u-3,v+3)||
       !td_district_walkable(TD_HOSPITAL_DISTRICT,u+3,v+3))return FALSE;
    /* A remotely retained owned car must still exclude its complete body.
     * The old scene's fleet/tram cannot reject a different district's exit. */
    if(td.park_district==TD_HOSPITAL_DISTRICT&&
       td_combat_distance(u*16,td.park_u)<168&&
       td_combat_distance(v*16,td.park_v)<168)return FALSE;
    return td_district_current()!=TD_HOSPITAL_DISTRICT||td_motion_foot_clear(u*16,v*16);
}
static UBYTE td_combat_recover(void){
    static const BYTE dx[]={0,16,-16,0,0,16,-16,16,-16};
    static const BYTE dy[]={0,0,0,16,-16,16,16,-16,-16};
    UBYTE i;WORD u,v;
    for(i=0;i<9;i++){
        u=TD_HOSPITAL_U+dx[i];v=TD_HOSPITAL_V+dy[i];
        if(!td_combat_hospital_clear(u,v))continue;
        td.district=TD_HOSPITAL_DISTRICT;td.onfoot=1;
        td.u=u*16;td.v=v*16;td.safe_u=td.u;td.safe_v=td.v;
        td.vitality=100;td.ammo=12;td.cash=td.cash>40?td.cash-40:0;
        td.wanted=td.wanted_left=0;td_immunity=30;td_player_delay=td_patrol_delay=td_arrest=0;
        /* Save the final hospital notice with the same single episode commit.
         * The state caller queues Core only after that commit, freezing all
         * old-scene simulation if the VM allocation must be retried. */
        td.msg=25;
        td_motion_reset();td.speed=0;td_vx=td_vy=0;td_player_sprite_restore();
        if(td.job!=TD_NONE){td.health=0;td_motion_finish(FALSE);}
        else td_save();
        return TRUE;
    }
    /* A congested hospital exit delays recovery instead of placing a courier
     * through a car or solid wall. The fee/job outcome happens only once. */
    return FALSE;
}
UBYTE td_combat_update(UWORD elapsed) BANKED {
    UBYTE result=0,was_down=td_down;UWORD u,v,x,y;WORD dx,dy;
    if(td.mode!=TD_ROAM||td.district>=TD_DISTRICT_COUNT||td.district!=td_district_current()||!elapsed)return 0;
    td_player_delay=td_combat_timer(td_player_delay,elapsed);
    td_patrol_delay=td_combat_timer(td_patrol_delay,elapsed);
    td_immunity=td_combat_timer(td_immunity,elapsed);
    td_arrest=td_combat_timer(td_arrest,elapsed);
    if(td_patrol_down){
        td_patrol_down=td_combat_timer(td_patrol_down,elapsed);
        if(!td_patrol_down)td_patrol_health=50;
    }
    if(was_down){
        td_down=td_combat_timer(td_down,elapsed);
        if(!td_down){
            if(td_combat_recover())return TD_COMBAT_CHANGED|TD_COMBAT_RECOVERED;
            td_down=60;
        }
        return 0;
    }
    /* Mode/district were validated above; only timers changed on this path.
     * Reuse that result instead of repeating a banked scene lookup. */
    if(!td.onfoot||td_entry_timer||!td.vitality||td.wanted<2||td_patrol_down||td_patrol_delay||
       (actors[4].flags&ACTOR_FLAG_HIDDEN))return 0;
    u=td.u>>4;v=td.v>>4;x=actors[4].pos.x>>5;y=actors[4].pos.y>>5;
    if(td_combat_distance(u,x)>96||td_combat_distance(v,y)>96||!td_combat_clear(x,y,u,v,TRUE))return 0;
    td_patrol_delay=60;td_audio_play(TD_AUDIO_IMPACT);
    dx=(WORD)u-x;dy=(WORD)v-y;
    result=td_combat_damage(td.wanted>=3?18:12,dx>0?1:dx<0?-1:0,dy>0?1:dy<0?-1:0);
    return result;
}
void td_combat_present(void) BANKED {
    if(!td.onfoot||td.mode!=TD_ROAM)return;
    if(td_down)td_civilian_present(&PLAYER,0,TD_CIVILIAN_PRONE);
    else if(td_immunity>30)td_civilian_present(&PLAYER,0,TD_CIVILIAN_HIT);
    else if(td_player_delay>12&&td_walk_dir<4){
        td_player_sprite_restore();PLAYER.frame=PLAYER.frame_start=45+td_walk_dir;
        PLAYER.frame_end=PLAYER.frame+1;PLAYER.anim_tick=255;
    }
}
UBYTE td_combat_flash(UWORD *u,UWORD *v,UBYTE *direction) BANKED {
    WORD dx,dy;
    if(!u||!v||!direction||td.mode!=TD_ROAM||td_down)return FALSE;
    if(td_player_delay>12){*u=td.u>>4;*v=td.v>>4;*direction=td_walk_dir;return TRUE;}
    if(td_patrol_delay>54&&!(actors[4].flags&ACTOR_FLAG_HIDDEN)){
        *u=actors[4].pos.x>>5;*v=actors[4].pos.y>>5;
        dx=(WORD)(td.u>>4)-*u;dy=(WORD)(td.v>>4)-*v;
        *direction=(dx<0?-dx:dx)>=(dy<0?-dy:dy)?(dx>=0?0:1):(dy>=0?2:3);
        return TRUE;
    }
    return FALSE;
}
