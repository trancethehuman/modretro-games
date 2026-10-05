#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gbvm_stubs.h"
#include "td_motion.h"
#include "td_game.h"
#include "td_roads.h"

td_state_t td;td_job_t td_job,td_offer;td_stop_t td_target,td_cursor;
actor_t actors[22];UBYTE joy,joy_pressed;
WORD td_vx,td_vy;
UBYTE td_tick,td_red_cooldown,td_turn_tick,td_entry_timer,td_entry_target,td_walk_dir;
UBYTE td_corner_used,td_input_edge,td_result_b_release,td_contact_episode;
static unsigned checks,failures,interactions,impacts;
static UBYTE blocked;
static UWORD terrain_min_u,terrain_min_v;
static UBYTE tram_rear;
static unsigned vehicle_queries,tram_queries;
static void expect(int condition,const char *name){checks++;if(!condition){failures++;if(failures<20)fprintf(stderr,"FAIL %s\n",name);}}
void actor_set_frames(actor_t *a,UBYTE first,UBYTE end){a->frame=a->frame_start=first;a->frame_end=end;}
void td_player_sprite_restore(void){}
void td_civilian_present(actor_t *a,UBYTE variant,UBYTE pose){(void)variant;actor_set_frames(a,pose,pose+1);}
UBYTE td_district_walkable(UBYTE district,UWORD u,UWORD v){(void)district;return u<1024&&v<976;}
UBYTE td_motion_foot_clear(UWORD u,UWORD v){return u<1024*16&&v<976*16;}
UBYTE td_motion_world_interact(void){interactions++;return TRUE;}
void td_motion_notice(UBYTE message){if(message==5)impacts++;}
void td_motion_finish(UBYTE success){(void)success;td.mode=TD_RESULT;}
void td_set_target(void){}
void td_save(void){}
void td_ui_draw(void){}
/* An independent stationary square whose left edge touches the car at the
   starting position. A strict swept interval may move away, never enter. */
UBYTE td_motion_vehicle_clear(UWORD ou,UWORD ov,UWORD u,UWORD v){
    vehicle_queries++;
    if(blocked==2)return FALSE;
    if(!blocked)return TRUE;
    UWORD left=ou<u?ou:u,right=ou>u?ou:u,top=ov<v?ov:v,bottom=ov>v?ov:v;
    return right<=400*16||left>=424*16||bottom<=438*16||top>=462*16;
}
UBYTE td_traffic_player_red(UBYTE d,UWORD s,UWORD ou,UWORD ov,UWORD u,UWORD v){(void)d;(void)s;(void)ou;(void)ov;(void)u;(void)v;return FALSE;}
UBYTE td_sandbox_clear(UWORD ou,UWORD ov,UWORD u,UWORD v,UBYTE half,UBYTE skip){(void)half;(void)skip;return td_motion_vehicle_clear(ou,ov,u,v);}
UBYTE td_scenery_contact(UWORD ou,UWORD ov,UWORD u,UWORD v,UBYTE half,BYTE speed){(void)ou;(void)ov;(void)u;(void)v;(void)half;(void)speed;return 0;}
UBYTE td_streetcar_runtime_car_clear(UWORD ou,UWORD ov,UWORD u,UWORD v){(void)ov;(void)v;tram_queries++;return tram_rear!=2&&(!tram_rear||u>=ou);}
UBYTE td_terrain_drivable(UWORD u,UWORD v,td_terrain_cache_t *cache){(void)cache;return u>=terrain_min_u&&v>=terrain_min_v&&u<1024&&v<976;}
UBYTE td_road_corner(td_state_t *s,WORD *x,WORD *y,UBYTE *used,WORD u,WORD v){(void)s;(void)x;(void)y;(void)used;(void)u;(void)v;return FALSE;}
static void reset(void){memset(&td,0,sizeof(td));td.job=TD_NONE;td.health=100;td.u=400*16;td.v=450*16;td.mode=TD_ROAM;td_vx=td_vy=0;td_tick=td_turn_tick=td_entry_timer=td_entry_target=td_result_b_release=td_contact_episode=joy=joy_pressed=0;blocked=tram_rear=0;terrain_min_u=terrain_min_v=0;interactions=impacts=vehicle_queries=tram_queries=0;td_motion_reset();}
static void step(UBYTE keys){td_terrain_cache_t cache={0};joy_pressed=keys&~joy;joy=keys;td_result_b_release&=joy;td_tick++;td_input_edge=1;td_drive(&cache);}
int main(void){
    {
        td_terrain_cache_t cache={0};reset();UWORD u=td.u,v=td.v;
        for(unsigned i=0;i<4;i++){td_tick++;td_drive(&cache);}
        expect(vehicle_queries==1&&tram_queries==1&&td.u==u&&td.v==v&&td.speed==0,
               "one immutable four-tick stationary batch checks each dynamic hull once and retains exact position");
        td.u++;td_tick++;td_drive(&cache);
        expect(vehicle_queries==2&&tram_queries==2,
               "a distinct Q4 centre in the same pixel requires fresh vehicle and tram clearance");
        blocked=2;cache.stationary_valid=0;td_tick++;td_drive(&cache);
        expect(vehicle_queries==3&&tram_queries==2&&impacts==1&&td.cooldown==60,
               "an invalidated obstacle batch cannot reuse clear geometry and retains its original impact");
        for(unsigned i=0;i<3;i++){td_tick++;td_drive(&cache);}
        expect(vehicle_queries==3&&impacts==1&&td.cooldown==57,
               "cached rejected stationary clearance retains every cooldown tick and the single collision consequence");
        reset();memset(&cache,0,sizeof(cache));tram_rear=2;
        for(unsigned i=0;i<4;i++){td_tick++;td_drive(&cache);}
        expect(vehicle_queries==1&&tram_queries==1&&impacts==1&&td_contact_episode==1&&td.cooldown==57,
               "rejected stationary rail clearance is reused without suppressing contact or timer behavior");
        td.onfoot=1;td_tick++;td_drive(&cache);
        expect(!cache.stationary_valid,"walking invalidates the previous stationary driving hull");
    }
    static const UBYTE limits[4]={14,12,16,10};
    for(UBYTE vehicle=0;vehicle<4;vehicle++)for(UBYTE heading=0;heading<16;heading++){
        reset();td.vehicle=vehicle;td.heading=heading;
        for(unsigned i=0;i<200;i++){step(J_LEFT);expect(td.heading==heading&&td.speed==0&&td_vx==0&&td_vy==0,"a car at actual rest never rotates or accumulates velocity");}
        expect(td_turn_tick==0,"stationary steering cannot accumulate a delayed yaw");
        step(J_A|J_LEFT);expect(td.heading==heading,"the first throttle frame cannot spend a stationary steering counter");
    }
    for(UBYTE vehicle=0;vehicle<4;vehicle++){
        reset();td.vehicle=vehicle;
        for(unsigned i=0;i<180;i++)step(J_A);
        expect(td.speed==limits[vehicle],"each road class reaches its distinct slower cruise limit");
        td_vy=71;for(unsigned i=0;i<30;i++)step(J_A);
        UWORD v=td.v;step(J_A);expect(td_vy==0&&td.v==v,"cardinal east traction settles all vertical drift exactly");
        td.heading=4;step(J_A);expect(td_vx>0&&td_vy>0&&td.speed==limits[vehicle],"turning retains forward momentum while throttle stays held");
        for(unsigned i=0;i<30;i++){ step(J_A); }expect(td_vx==0&&td_vy>0,"cardinal south traction settles all old horizontal drift");
        for(unsigned i=0;i<180;i++){ step(J_B); }expect(td.speed==-4,"braking leads to bounded slow reverse");
    }
    reset();td.speed=4;td_vx=64;for(unsigned i=0;i<12;i++){ step(J_LEFT); }expect(td.heading==15,"left steering while moving forward yaws left");
    reset();td.speed=-4;td_vx=-64;for(unsigned i=0;i<12;i++){ step(J_LEFT); }expect(td.heading==1,"left steering while reversing yaws the car in the opposite direction");
    reset();td.speed=14;td_vx=224;td_motion_impact();expect(td.speed==7&&td_vx==112,"a human impact immediately halves real momentum");
    for(unsigned i=0;i<17;i++){step(J_A);expect(td.speed<=7,"held throttle cannot immediately replace the impact momentum loss");}
    for(unsigned i=0;i<100;i++){ step(J_A); }expect(td.speed==14,"throttle resumes after bounded impact recovery");
    reset();td.speed=14;td_vx=224;td.job=0;td.stage=1;td.health=100;blocked=1;UWORD u=td.u;
    step(J_A);expect(td.u==u&&td_vx<0&&td.health==88&&impacts==1,"a denied physical sweep starts a visible rebound and charges one impact");
    for(unsigned i=0;i<6;i++){UWORD before=td.u;step(J_A|J_LEFT);expect(td.u<=before&&before-td.u<=16&&td.heading==0&&td.speed<=7,"held throttle and steering cannot cancel or rotate the six-tick rebound");}
    expect(td.u==u-6*14&&td.safe_u==td.u,"the unobstructed recoil follows six ticks of the incoming14-Q4 momentum and updates the valid recovery point");
    for(unsigned i=0;i<13;i++)step(J_A);
    expect(td.u<=u&&td.health==88&&impacts==1,"continued held throttle never tunnels into the contacted car or repeats its penalty");
    blocked=0;for(unsigned i=0;i<100;i++){ step(J_A); }expect(td.u>u,"driving resumes after a vehicle body clears");
    for(UBYTE heading=0;heading<16;heading++)for(UBYTE reverse=0;reverse<2;reverse++){
        static const signed char dx[]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
        static const signed char dy[]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};
        reset();td.heading=heading;td.speed=reverse?-4:14;td_vx=dx[heading]*td.speed;td_vy=dy[heading]*td.speed;
        WORD incoming_x=td_vx,incoming_y=td_vy;UWORD start_u=td.u,start_v=td.v;td_motion_vehicle_impact();
        expect((long)incoming_x*td_vx+(long)incoming_y*td_vy<0,"vehicle rebound reverses actual incoming momentum in every heading and in reverse");
        for(unsigned i=0;i<6;i++){UWORD ou=td.u,ov=td.v;step(J_A|J_RIGHT);expect(abs((int)td.u-ou)<=16&&abs((int)td.v-ov)<=16&&td.heading==heading,"every recoil update is bounded and retains body orientation");}
        expect((long)((int)td.u-start_u)*incoming_x+(long)((int)td.v-start_v)*incoming_y<0,"the actual car visibly moves away from its incoming trajectory");
        expect(abs((int)td.u-start_u)<=96&&abs((int)td.v-start_v)<=96,"rebound displacement has an absolute six-pixel per-axis bound");
    }
    reset();td.speed=14;td_vx=224;blocked=1;terrain_min_u=399;u=td.u;step(J_A);
    for(unsigned i=0;i<6;i++)step(J_A);
    expect(td.u>=399*16&&td.u<u&&td_vx==0,"a rear terrain wall clips rebound at the last full-body-valid point");
    reset();td.speed=14;td_vx=224;blocked=2;u=td.u;step(J_A);
    for(unsigned i=0;i<6;i++)step(J_A);
    expect(td.u==u&&td_vx==0,"a second physical body behind the car denies recoil instead of tunnelling");
    reset();td.speed=14;td_vx=224;blocked=1;tram_rear=1;u=td.u;step(J_A);
    for(unsigned i=0;i<6;i++)step(J_A);
    expect(td.u==u&&td_vx==0,"tram clearance applies to every recoil substep");
    reset();td.speed=14;td_vx=224;td_motion_vehicle_impact();td.onfoot=1;td_vx=td_vy=0;u=td.u;step(0);td.onfoot=0;step(0);
    expect(td.u>=u,"leaving a car cancels its transient rebound before a later entry");
    for(UBYTE keys=J_A;keys<=J_B;keys+=J_A){reset();td.onfoot=1;for(unsigned i=0;i<20;i++){ step(keys); }expect(interactions==1&&(td_result_b_release&keys),"a fresh A or B foot interaction runs once and consumes its held action");step(0);step(keys);expect(interactions==2,"release and repress deliberately starts a second interaction");}
    printf("Actual driving feedback unit: %u checks, %u failures; native playback remains separate.\n",checks,failures);return !!failures;
}
