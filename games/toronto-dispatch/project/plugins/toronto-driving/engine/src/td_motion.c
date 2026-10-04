#pragma bank 255
#include "td_motion.h"
#include "td_game.h"
#include "td_roads.h"
#include "td_district.h"
#include "td_streetcar_runtime.h"
#include "td_city_sprites.h"
#include "actor.h"
#include "input.h"
#include "compat.h"

static const BYTE td_dx[]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
static const BYTE td_dy[]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};
static UBYTE td_player_hurt;
static BYTE td_player_push_x,td_player_push_y;
static UWORD td_motion_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static void td_motion_frame(actor_t *a,UBYTE f){if(a->frame_start!=f||a->frame_end!=f+1)actor_set_frames(a,f,f+1);a->anim_tick=255;}
/* Half the remaining error tracks a turn while retaining momentum. Settle
 * the final Q8 unit exactly: integer truncation must not leave a cardinal
 * heading carrying a permanent orthogonal velocity. */
static WORD td_motion_traction(WORD velocity,WORD target){
    WORD delta=target-velocity;
    return delta>=-1&&delta<=1?target:velocity+delta/2;
}
void td_motion_reset(void) BANKED {
    td_player_hurt=0;td_player_push_x=td_player_push_y=0;
    td_player_sprite_restore();
}
UBYTE td_motion_player_hit(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half) BANKED {
    UWORD low_u,high_u,low_v,high_v,radius=(half+3)*16;
    if(!td.onfoot||(td.mode!=TD_ROAM&&td.mode!=TD_WAIT)||td_entry_timer||td_player_hurt||td.cooldown||
       (old_u==u&&old_v==v)||half<5||half>8)return FALSE;
    low_u=old_u<u?old_u:u;high_u=old_u>u?old_u:u;
    low_v=old_v<v?old_v:v;high_v=old_v>v?old_v:v;
    if((td.u<low_u&&low_u-td.u>=radius)||(td.u>high_u&&td.u-high_u>=radius)||
       (td.v<low_v&&low_v-td.v>=radius)||(td.v>high_v&&td.v-high_v>=radius))return FALSE;
    return td_motion_player_knockback(u>old_u?1:u<old_u?-1:0,v>old_v?1:v<old_v?-1:0);
}
UBYTE td_motion_player_knockback(BYTE dx,BYTE dy) BANKED {
    if(!td.onfoot||(td.mode!=TD_ROAM&&td.mode!=TD_WAIT)||td_entry_timer||td_player_hurt||td.cooldown||(!dx&&!dy))return FALSE;
    td_player_push_x=dx>0?8:dx<0?-8:0;td_player_push_y=dy>0?8:dy<0?-8:0;
    td_player_hurt=72;td.cooldown=120;td_entry_timer=0;td.mode=TD_ROAM;
    td.speed=0;td_vx=td_vy=0;
    if(td.job!=TD_NONE&&td.stage)td.health=td.health>15?td.health-15:0;
    td_motion_notice(5);
    if(td.job!=TD_NONE&&!td.health)td_motion_finish(FALSE);else td_save();
    return TRUE;
}
/* Knockback is bounded to24px and checks each half-pixel terrain step.
 * The courier gets up after72 simulation VBlanks; injury is transient and
 * never changes the58-byte save layout or kills/abandons a fresh courier. */
static UBYTE td_motion_hurt_step(void){
    WORD u,v;
    if(!td_player_hurt)return FALSE;
    if(td_player_hurt>24){
        u=td.u+td_player_push_x;v=td.v+td_player_push_y;
        if(u>=0&&v>=0&&td_district_walkable(td.district,u>>4,v>>4)&&
           !(td.park_district==td.district&&td_motion_distance(u,td.park_u)<168&&td_motion_distance(v,td.park_v)<168)){
            td.u=u;td.v=v;td.safe_u=u;td.safe_v=v;
        }
    }
    td_civilian_present(&PLAYER,0,td_player_hurt>24?TD_CIVILIAN_HIT:TD_CIVILIAN_PRONE);
    if(!--td_player_hurt){td_player_sprite_restore();td_motion_frame(&PLAYER,32);}
    return TRUE;
}

void td_drive(td_terrain_cache_t *cache) BANKED{
    WORD nu,nv,target_x,target_y;BYTE walk_x,walk_y;UBYTE limit,turn_period,moving=0,slide=0,speed;UWORD u,v;
    if(td.cooldown)td.cooldown--;
    if(td_red_cooldown)td_red_cooldown--;
    if(td_motion_hurt_step())return;
    if(td_entry_timer){
        if(!td_entry_target){td.u=(td.u*3+td.park_u)/4;td.v=(td.v*3+td.park_v)/4;}
        if(!--td_entry_timer){if(!td_entry_target){td.u=td.park_u;td.v=td.park_v;td.onfoot=0;}td_set_target();td_save();td_ui_draw();}
        td_motion_frame(&PLAYER,td.onfoot?32:td.vehicle*8+((td.heading+1)&15)/2);return;
    }
    if(td.onfoot){
        nu=td.u;nv=td.v;
        walk_x=!!INPUT_RIGHT-!!INPUT_LEFT;walk_y=!!INPUT_DOWN-!!INPUT_UP;
        /* Alternate5/6 per axis: diagonal pace stays below cardinal8 Q4. */
        if(walk_x){nu+=walk_x*(walk_y?5+(td_tick&1):8);td_walk_dir=walk_x>0?0:1;moving=1;}
        if(walk_y){nv+=walk_y*(walk_x?6-(td_tick&1):8);td_walk_dir=walk_y>0?2:3;moving=1;}
        if(td_motion_foot_clear(nu,nv)){td.u=nu;td.v=nv;}
        else{if(nu!=(WORD)td.u&&td_motion_foot_clear(nu,td.v))td.u=nu;if(nv!=(WORD)td.v&&td_motion_foot_clear(td.u,nv))td.v=nv;}
        td.speed=0;td_motion_frame(&PLAYER,32+td_walk_dir*2+(moving?((td_tick>>3)&1):0));
        if(td_input_edge&&INPUT_A_PRESSED&&td_motion_near_car())td_motion_enter_exit();
        if(td_input_edge&&INPUT_B_PRESSED&&!td_entry_timer)td_motion_transit_open();
        return;
    }
    limit=td.vehicle==1?12:td.vehicle==2?16:td.vehicle==3?10:14;
    speed=td.speed<0?-td.speed:td.speed;
    turn_period=speed>12?14:speed>6?12:10;
    if(td.vehicle==1)turn_period+=2;
    // A dedicated yaw counter keeps turns regular as speed changes.
    if(!!INPUT_LEFT!=!!INPUT_RIGHT){if(++td_turn_tick>=turn_period){td_turn_tick=0;if(INPUT_LEFT)td.heading=(td.heading+15)&15;else td.heading=(td.heading+1)&15;
        if(td.job!=TD_NONE&&td.stage&&td_job.kind==5&&speed>12){if(td.health)td.health--;td_motion_notice(14);}
    }}
    else td_turn_tick=0;
    if(INPUT_B&&!(td_result_b_release&J_B)){if(td_tick%2==0&&td.speed>-4)td.speed--;}
    else if(INPUT_A&&!(td_result_b_release&J_A)){if(td_tick%4==0&&td.speed<limit)td.speed++;}
    else if(td_tick%8==0){if(td.speed>0)td.speed--;else if(td.speed<0)td.speed++;}
    // Traction eases velocity toward heading instead of instantly rotating momentum.
    target_x=td_dx[td.heading]*td.speed;target_y=td_dy[td.heading]*td.speed;
    td_vx=td_motion_traction(td_vx,target_x);td_vy=td_motion_traction(td_vy,target_y);
    if(!td.speed){td_vx/=2;td_vy/=2;}
    nu=td.u+td_vx/16;nv=td.v+td_vy/16;u=nu>>4;v=nv>>4;
    if(!td_streetcar_runtime_car_clear(td.u,td.v,nu,nv)){
        td.speed=0;td_vx=td_vy=0;
        if(!td.cooldown&&!td_contact_episode){td_contact_episode=1;td.cooldown=60;if(td.job!=TD_NONE&&td.stage){td.health=td.health>12?td.health-12:0;}td_motion_notice(5);}
        if(td.job!=TD_NONE&&!td.health)td_motion_finish(FALSE);
        return;
    }
    if(td_terrain_drivable(u,v,cache)){
        if(td.district==0&&!td_red_cooldown&&td.speed>6&&td_motion_distance(u,640)<14&&td_motion_distance(v,528)<14&&
          (td_motion_distance(td.u>>4,640)>=14||td_motion_distance(td.v>>4,528)>=14)){
            UBYTE ax=td_dx[td.heading]<0?-td_dx[td.heading]:td_dx[td.heading];
            UBYTE ay=td_dy[td.heading]<0?-td_dy[td.heading]:td_dy[td.heading];
            if((ax>ay&&td.seconds%12>=7)||(ax<=ay&&td.seconds%12<7)){if(td.cash>=5)td.cash-=5;td_red_cooldown=120;td_motion_notice(7);}
        }
        td.u=nu;td.v=nv;
    }else{
        // A glancing curb contact slides along the free axis and preserves forward speed.
        if(nu!=(WORD)td.u&&td_terrain_drivable(nu>>4,td.v>>4,cache)){td.u=nu;td_vy=0;slide=1;}
        if(nv!=(WORD)td.v&&td_terrain_drivable(td.u>>4,nv>>4,cache)){td.v=nv;td_vx=0;slide=1;}
        if(!slide){
            UBYTE held=joy;
            joy&=~(td_result_b_release&(J_A|J_B));
            slide=td_road_corner(&td,&td_vx,&td_vy,&td_corner_used,nu,nv);
            joy=held;
        }
        /* Remove only blocked-axis motion. Repeated curb scrapes must not beat the throttle. */
        if(slide){if(speed>8&&!td.cooldown){td.cooldown=30;if(td.job!=TD_NONE&&td.stage){UBYTE damage=td_job.kind==1?4:1;td.health=td.health>damage?td.health-damage:0;}td_motion_notice(5);}}
        else{
            if(td.speed>8&&!td.cooldown){if(td.job!=TD_NONE&&td.stage){UBYTE damage=td_job.kind==1?20:8;td.health=td.health>damage?td.health-damage:0;}td.cooldown=45;td_motion_notice(5);}
            td.speed=0;td_vx=td_vy=0;
        }
    }
    td.safe_u=td.u;td.safe_v=td.v;
    td_motion_frame(&PLAYER,td.vehicle*8+((td.heading+1)&15)/2);
    if(td.job!=TD_NONE&&!td.health)td_motion_finish(FALSE);
}
