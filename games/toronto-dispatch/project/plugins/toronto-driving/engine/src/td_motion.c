#pragma bank 255
#include "td_motion.h"
#include "td_game.h"
#include "td_roads.h"
#include "td_district.h"
#include "td_streetcar_runtime.h"
#include "td_city_sprites.h"
#include "td_traffic.h"
#include "td_scenery.h"
#include "td_sandbox.h"
#include "actor.h"
#include "input.h"
#include "compat.h"

static const BYTE td_dx[]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
static const BYTE td_dy[]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};
static UBYTE td_player_hurt;
static UBYTE td_impact_recovery;
static BYTE td_player_push_x,td_player_push_y;
static UBYTE td_vehicle_recoil;
static BYTE td_vehicle_recoil_x,td_vehicle_recoil_y;
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
    td_player_hurt=td_impact_recovery=td_vehicle_recoil=0;
    td_player_push_x=td_player_push_y=td_vehicle_recoil_x=td_vehicle_recoil_y=0;
    td_player_sprite_restore();
}
void td_motion_impact(void) BANKED {
    td.speed/=2;td_vx/=2;td_vy/=2;td_impact_recovery=18;
    td_vehicle_recoil_x/=2;td_vehicle_recoil_y/=2;
}
void td_motion_vehicle_impact(void) BANKED {
    WORD x=-td_vx/16,y=-td_vy/16;
    /* At most one pixel per axis for six ticks. Reverse follows its real
       incoming momentum as well; held steering cannot rotate this impulse. */
    if(x>16)x=16;else if(x<-16)x=-16;
    if(y>16)y=16;else if(y<-16)y=-16;
    td_motion_impact();td_vx=-td_vx;td_vy=-td_vy;
    td_vehicle_recoil_x=x;td_vehicle_recoil_y=y;
    td_vehicle_recoil=(x||y)?6:0;td_turn_tick=0;
}
UBYTE td_motion_rebounding(void) BANKED {return td_vehicle_recoil!=0;}
void td_motion_transfer(UBYTE speed) BANKED {
    UBYTE prior=td.speed<0?-td.speed:td.speed;
    if(!prior||speed>prior)return;
    td_vx=td_vx*speed/prior;td_vy=td_vy*speed/prior;
    td.speed=td.speed<0?-(BYTE)speed:speed;td_impact_recovery=12;
}
static UBYTE td_motion_recoil_clear(WORD u,WORD v,td_terrain_cache_t *cache){
    return u>=0&&v>=0&&td_terrain_drivable(u>>4,v>>4,cache)&&
        td_motion_vehicle_clear(td.u,td.v,u,v)&&
        td_streetcar_runtime_car_clear(td.u,td.v,u,v)&&
        td_scenery_contact(td.u,td.v,u,v,7,td.speed)==TD_SCENERY_CLEAR;
}
static UBYTE td_motion_recoil_step(td_terrain_cache_t *cache){
    UBYTE n;BYTE x,y;WORD u,v;
    if(!td_vehicle_recoil)return FALSE;
    /* Two substeps preserve the exact signed impulse without any step
       exceeding half a pixel. Clip a blocked axis against full body terrain
       and vehicles; never teleport through a rear wall or another car. */
    for(n=0;n<2;n++){
        x=n?td_vehicle_recoil_x-td_vehicle_recoil_x/2:td_vehicle_recoil_x/2;
        y=n?td_vehicle_recoil_y-td_vehicle_recoil_y/2:td_vehicle_recoil_y/2;
        u=td.u+x;v=td.v+y;
        if(td_motion_recoil_clear(u,v,cache)){td.u=u;td.v=v;}
        else{
            if(x&&td_motion_recoil_clear(u,td.v,cache))td.u=u;
            else if(x){td_vehicle_recoil_x=0;td_vx=0;}
            if(y&&td_motion_recoil_clear(td.u,v,cache))td.v=v;
            else if(y){td_vehicle_recoil_y=0;td_vy=0;}
        }
    }
    td_vehicle_recoil--;td.safe_u=td.u;td.safe_v=td.v;
    td_motion_frame(&PLAYER,td.vehicle*8+((td.heading+1)&15)/2);
    return TRUE;
}
static UBYTE td_motion_props_contact(UWORD old_u,UWORD old_v,UWORD u,UWORD v){
    WORD du=(WORD)u-old_u,dv=(WORD)v-old_v;UWORD span=du<0?-du:du,other=dv<0?-dv:dv,pu,pv;
    UBYTE n,steps,result;
    if(other>span)span=other;
    if(!span)return TD_SCENERY_CLEAR;
    /* The accepted corner assist can shift at most6px laterally. Divide
       that path (and older high-speed momentum) into the scenery API's
       <=1px queries, retaining the entire swept segment without widening
       its bounded sorted-table search or signed arithmetic. */
    if(span>128)return TD_SCENERY_BLOCK;
    steps=(span+15)/16;pu=old_u;pv=old_v;
    for(n=1;n<=steps;n++){
        u=old_u+du*n/steps;v=old_v+dv*n/steps;
        result=td_scenery_contact(pu,pv,u,v,7,td.speed);
        if(result!=TD_SCENERY_CLEAR)return result;
        pu=u;pv=v;
    }
    return TD_SCENERY_CLEAR;
}
static UBYTE td_motion_corner_clear(UWORD old_u,UWORD old_v,UWORD u,UWORD v){
    WORD du=(WORD)u-old_u,dv=(WORD)v-old_v;UWORD span=du<0?-du:du,other=dv<0?-dv:dv,pu,pv;
    UBYTE n,steps;
    if(other>span)span=other;
    if(!span||span>128)return FALSE;
    steps=(span+15)/16;pu=old_u;pv=old_v;
    for(n=1;n<=steps;n++){
        u=old_u+du*n/steps;v=old_v+dv*n/steps;
        /* Assistance is a geometry correction, never an additional ram.
           Check the whole assisted path against physical cars and rail. */
        if(!td_sandbox_clear(pu,pv,u,v,7,TD_NONE)||
           !td_streetcar_runtime_car_clear(pu,pv,u,v))return FALSE;
        pu=u;pv=v;
    }
    return TRUE;
}
static UBYTE td_motion_scenery_hit(UWORD old_u,UWORD old_v,UWORD u,UWORD v){
    if(td_motion_props_contact(old_u,old_v,u,v)==TD_SCENERY_CLEAR)return FALSE;
    if(!td.cooldown&&!td_contact_episode){td_motion_vehicle_impact();td.cooldown=30;td_motion_notice(5);}
    if(!td_vehicle_recoil)td_vx=td_vy=0;
    td.safe_u=td.u;td.safe_v=td.v;
    return TRUE;
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
    td_vehicle_recoil=0;td_vehicle_recoil_x=td_vehicle_recoil_y=0;
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
    WORD nu,nv,target_x,target_y;BYTE walk_x,walk_y;UBYTE limit,turn_period,moving=0,slide=0,speed,stationary,clear;UWORD u,v;
    if(td.cooldown)td.cooldown--;
    if(td_impact_recovery)td_impact_recovery--;
    if(td_red_cooldown)td_red_cooldown--;
    if(td_player_hurt)cache->stationary_valid=0;
    if(td_motion_hurt_step())return;
    if(td_entry_timer){
        cache->stationary_valid=0;
        td_vehicle_recoil=0;td_vehicle_recoil_x=td_vehicle_recoil_y=0;
        if(!td_entry_target){td.u=(td.u*3+td.park_u)/4;td.v=(td.v*3+td.park_v)/4;}
        if(!--td_entry_timer){if(!td_entry_target){td.u=td.park_u;td.v=td.park_v;td.onfoot=0;}td_set_target();td_save();td_ui_draw();}
        td_motion_frame(&PLAYER,td.onfoot?32:td.vehicle*8+((td.heading+1)&15)/2);return;
    }
    if(td.onfoot){
        cache->stationary_valid=0;
        td_vehicle_recoil=0;td_vehicle_recoil_x=td_vehicle_recoil_y=0;
        nu=td.u;nv=td.v;
        walk_x=!!INPUT_RIGHT-!!INPUT_LEFT;walk_y=!!INPUT_DOWN-!!INPUT_UP;
        /* Alternate5/6 per axis: diagonal pace stays below cardinal8 Q4. */
        if(walk_x){nu+=walk_x*(walk_y?5+(td_tick&1):8);td_walk_dir=walk_x>0?0:1;moving=1;}
        if(walk_y){nv+=walk_y*(walk_x?6-(td_tick&1):8);td_walk_dir=walk_y>0?2:3;moving=1;}
        if(td_motion_foot_clear(nu,nv)){td.u=nu;td.v=nv;}
        else{if(nu!=(WORD)td.u&&td_motion_foot_clear(nu,td.v))td.u=nu;if(nv!=(WORD)td.v&&td_motion_foot_clear(td.u,nv))td.v=nv;}
        td.speed=0;td_motion_frame(&PLAYER,32+td_walk_dir*2+(moving?((td_tick>>3)&1):0));
        if(td_input_edge&&(INPUT_A_PRESSED||INPUT_B_PRESSED)){
            td_result_b_release|=joy&(J_A|J_B);
            td_motion_world_interact();
        }
        return;
    }
    if(td_vehicle_recoil)cache->stationary_valid=0;
    if(td_motion_recoil_step(cache))return;
    limit=td.vehicle==1?12:td.vehicle==2?16:td.vehicle==3?10:14;
    speed=td.speed<0?-td.speed:td.speed;
    turn_period=speed>12?18:speed>6?14:12;
    if(td.vehicle==1)turn_period+=2;
    // A dedicated yaw counter keeps turns regular as speed changes.
    /* Steering requires genuine motion. In particular held steering at
       zero speed cannot accumulate a turn for the next throttle press.
       Reversing turns the body in the opposite yaw direction. */
    if(speed&&(td_vx/16||td_vy/16)&&!!INPUT_LEFT!=!!INPUT_RIGHT){if(++td_turn_tick>=turn_period){td_turn_tick=0;if(!!INPUT_LEFT!=(td.speed<0))td.heading=(td.heading+15)&15;else td.heading=(td.heading+1)&15;
        if(td.job!=TD_NONE&&td.stage&&td_job.kind==5&&speed>12){if(td.health)td.health--;td_motion_notice(14);}
    }}
    else td_turn_tick=0;
    if(INPUT_B&&!(td_result_b_release&J_B)){if(td_tick%2==0&&td.speed>-4)td.speed--;}
    else if(INPUT_A&&!(td_result_b_release&J_A)&&!td_impact_recovery){if(td_tick%4==0&&td.speed<limit)td.speed++;}
    else if(td_tick%8==0){if(td.speed>0)td.speed--;else if(td.speed<0)td.speed++;}
    // Traction eases velocity toward heading instead of instantly rotating momentum.
    target_x=td_dx[td.heading]*td.speed;target_y=td_dy[td.heading]*td.speed;
    td_vx=td_motion_traction(td_vx,target_x);td_vy=td_motion_traction(td_vy,target_y);
    if(!td.speed){td_vx/=2;td_vy/=2;}
    nu=td.u+td_vx/16;nv=td.v+td_vy/16;u=nu>>4;v=nv>>4;
    stationary=nu==(WORD)td.u&&nv==(WORD)td.v;
    if(!stationary)cache->stationary_valid=0;
    if(cache->stationary_valid&&(cache->stationary_u!=td.u||cache->stationary_v!=td.v))cache->stationary_valid=0;
    if(stationary&&(cache->stationary_valid&1))clear=cache->stationary_results&1;
    else{
        clear=td_motion_vehicle_clear(td.u,td.v,nu,nv);
        if(stationary){
            cache->stationary_u=td.u;cache->stationary_v=td.v;
            cache->stationary_valid|=1;
            cache->stationary_results=(cache->stationary_results&~1)|(clear?1:0);
        }
    }
    if(!clear){
        if(!td.cooldown&&!td_contact_episode){td_motion_vehicle_impact();td.cooldown=60;
            if(td.job!=TD_NONE&&td.stage)td.health=td.health>12?td.health-12:0;
            td_motion_notice(5);
        }
        if(!td_vehicle_recoil)td_vx=td_vy=0;
        if(td.job!=TD_NONE&&!td.health)td_motion_finish(FALSE);
        return;
    }
    if(stationary&&(cache->stationary_valid&2))clear=cache->stationary_results&2;
    else{
        clear=td_streetcar_runtime_car_clear(td.u,td.v,nu,nv);
        if(stationary){
            cache->stationary_u=td.u;cache->stationary_v=td.v;
            cache->stationary_valid|=2;
            cache->stationary_results=(cache->stationary_results&~2)|(clear?2:0);
        }
    }
    if(!clear){
        td.speed=0;td_vx=td_vy=0;
        if(!td.cooldown&&!td_contact_episode){td_contact_episode=1;td.cooldown=60;if(td.job!=TD_NONE&&td.stage){td.health=td.health>12?td.health-12:0;}td_motion_notice(5);}
        if(td.job!=TD_NONE&&!td.health)td_motion_finish(FALSE);
        return;
    }
    if(td_terrain_drivable(u,v,cache)){
        /* Furniture supplements the real low-nibble terrain. A prop beyond
           an intervening building/water body cannot be broken remotely. */
        if(td_motion_scenery_hit(td.u,td.v,nu,nv))return;
        if(!td_red_cooldown&&td.speed>6&&td_traffic_player_red(td.district,td.seconds,td.u,td.v,nu,nv)){
            td.cash=td.cash>5?td.cash-5:0;td_red_cooldown=120;td_motion_notice(7);
        }
        td.u=nu;td.v=nv;
    }else{
        // A glancing curb contact slides along the free axis and preserves forward speed.
        if(nu!=(WORD)td.u&&td_terrain_drivable(nu>>4,td.v>>4,cache)){
            if(td_motion_scenery_hit(td.u,td.v,nu,td.v))return;
            td.u=nu;td_vy=0;slide=1;
        }
        if(nv!=(WORD)td.v&&td_terrain_drivable(td.u>>4,nv>>4,cache)){
            if(td_motion_scenery_hit(td.u,td.v,td.u,nv))return;
            td.v=nv;td_vx=0;slide=1;
        }
        if(!slide){
            UBYTE held=joy;
            UWORD old_u=td.u,old_v=td.v;
            joy&=~(td_result_b_release&(J_A|J_B));
            slide=td_road_corner(&td,&td_vx,&td_vy,&td_corner_used,nu,nv);
            joy=held;
            if(slide&&!td_motion_corner_clear(old_u,old_v,td.u,td.v)){
                td.u=old_u;td.v=old_v;td_corner_used=0;slide=0;
            }
            else if(slide&&td_motion_props_contact(old_u,old_v,td.u,td.v)!=TD_SCENERY_CLEAR){
                td.u=old_u;td.v=old_v;
                if(!td.cooldown&&!td_contact_episode){td_motion_vehicle_impact();td.cooldown=30;td_motion_notice(5);}
                if(!td_vehicle_recoil)td_vx=td_vy=0;
                td.safe_u=td.u;td.safe_v=td.v;return;
            }
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
