/* Includes production gameplay so these fixtures exercise its static routines. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "gbvm_stubs.h"
#define TD_WORLD_ROUTE_DATA
#define TD_WORLD_DATA
#include "engine_under_test.c"
#define td_get_stop td_authored_get_stop
#define td_get_job td_authored_get_job
#define td_get_brief td_authored_get_brief
#define td_get_street td_authored_get_street
#define td_get_west_street td_authored_get_west_street
#include "content_under_test.c"
#undef td_get_stop
#undef td_get_job
#undef td_get_brief
#undef td_get_street
#undef td_get_west_street
#include "native_collision_fixture.h"

actor_t actors[21];
actor_t *actors_inactive_head;
UBYTE actors_len;
UWORD camera_x,camera_y,image_width=1024,image_height=976,sys_time;
UBYTE camera_settings,joy,joy_pressed;
BYTE camera_offset_x,camera_offset_y,camera_deadzone_x,camera_deadzone_y;
UBYTE td_test_sram[8192];

static unsigned failures,checks,stop_reads,ui_draws;
static unsigned audio_updates,audio_inits;
static UBYTE audio_mode,audio_active,audio_cue;
static UBYTE stop0_here;
static UBYTE authored_content;
static UBYTE test_current_district,test_queued_district,test_queue_fail;
static unsigned test_queue_calls,test_reset_calls;
static actor_t *test_actors_active_head;
static scene_t test_native_scenes[TD_DISTRICT_COUNT];
/* Map presentation is stubbed here; the real atlas data/API is checked by
 * test_atlas.py. These counters verify the actual driver/UI handoff only. */
static unsigned test_map_opens,test_map_updates,test_map_closes;
static UBYTE test_map_active,test_map_buttons,test_map_pressed,test_map_camera_settings;
static UWORD test_map_camera_x,test_map_camera_y;
static enum { CLEAR_GROUND,EAST_WALL,SOUTH_CURB,NATIVE_GRID,HIDDEN_NPC_TILE,SOUTHWEST_CORNER,ALIGHT_BARRIER } geometry;
static jmp_buf interrupted_save;
static unsigned sram_writes,sram_interrupt_after;
static int sram_interrupt_enabled;
static unsigned sram_offsets[256];
static UBYTE sram_values[256];

static void expect(int condition,const char *name) {
    checks++;
    if (!condition) { failures++; fprintf(stderr,"FAIL %s\n",name); }
}

void td_host_sram_store(volatile UBYTE *address,UBYTE value) {
    ptrdiff_t offset=address-td_test_sram;
    if(offset<0||(size_t)offset>=sizeof(td_test_sram)) {fprintf(stderr,"SRAM store outside host buffer\n");abort();}
    *address=value;
    if(sram_writes<256) {sram_offsets[sram_writes]=(unsigned)offset;sram_values[sram_writes]=value;}
    sram_writes++;
    if(sram_interrupt_enabled&&sram_writes==sram_interrupt_after)longjmp(interrupted_save,1);
}

static UBYTE district_tile(UBYTE district,UBYTE x,UBYTE y) {
    if(district>=TD_DISTRICT_COUNT)return 15;
    if (x>=128 || y>=122) return 15;
    if (geometry==EAST_WALL && x>=50) return 15;
    if (geometry==SOUTH_CURB && y>=50) return 15;
    if (geometry==SOUTHWEST_CORNER && y>=50 && x<50) return 15;
    if (geometry==ALIGHT_BARRIER&&x==71&&y==90) return 15;
    if (geometry==NATIVE_GRID||geometry==ALIGHT_BARRIER) return x<native_widths[district]&&y<native_heights[district]?native_collision[district][y*native_widths[district]+x]:15;
    if (geometry==HIDDEN_NPC_TILE && x==48 && y==53) return 15;
    return 0;
}
UBYTE tile_at(UBYTE x,UBYTE y) {return district_tile(test_current_district,x,y);}
void actor_set_frames(actor_t *actor,UBYTE first,UBYTE end) {
    actor->frame=actor->frame_start=first; actor->frame_end=end;
}
static void test_unlink_actor(actor_t *actor,actor_t **head) {
    if(actor->prev)actor->prev->next=actor->next;
    else if(*head==actor)*head=actor->next;
    if(actor->next)actor->next->prev=actor->prev;
    actor->prev=actor->next=NULL;
}
void activate_actor(actor_t *actor) {
    if(actor->flags&ACTOR_FLAG_ACTIVE)return;
    test_unlink_actor(actor,&actors_inactive_head);
    actor->flags|=ACTOR_FLAG_ACTIVE;
    actor->next=test_actors_active_head;
    if(test_actors_active_head)test_actors_active_head->prev=actor;
    test_actors_active_head=actor;
    /* Pinned actor.c calls actor_set_anim_idle during activation. Both
       authored default states begin at frame0; a hidden loader must select
       frame8 afterwards rather than relying on its editor-only frame field. */
    actor->frame=0;
}
void deactivate_actor(actor_t *actor) {
    if(!(actor->flags&ACTOR_FLAG_ACTIVE))return;
    test_unlink_actor(actor,&test_actors_active_head);
    actor->flags&=~ACTOR_FLAG_ACTIVE;
    actor->next=actors_inactive_head;
    if(actors_inactive_head)actors_inactive_head->prev=actor;
    actors_inactive_head=actor;
}
void MemcpyBanked(void *dest,const void *src,size_t length,UBYTE bank) {
    (void)bank;memcpy(dest,src,length);
}
UBYTE ReadBankedUBYTE(const UBYTE *src,UBYTE bank) {
    if(!bank||bank>TD_DISTRICT_COUNT)return 15;
    UBYTE district=bank-1;
    uintptr_t start=(uintptr_t)native_collision[district],address=(uintptr_t)src;
    size_t count=(size_t)native_widths[district]*native_heights[district];
    if(address<start||address-start>=count)return 15;
    size_t index=address-start;
    return district_tile(district,index%native_widths[district],index/native_widths[district]);
}
void td_ui_init(void) { ui_draws++; }
void td_ui_draw(void) { ui_draws++; }
void td_map_open(void) {
    test_map_opens++;test_map_active=1;
    test_map_camera_x=camera_x;test_map_camera_y=camera_y;test_map_camera_settings=camera_settings;
    camera_settings=0;
}
void td_map_update(UBYTE buttons,UBYTE pressed) {
    test_map_updates++;test_map_buttons=buttons;test_map_pressed=pressed;
}
void td_map_close(void) {
    test_map_closes++;
    if(test_map_active){camera_x=test_map_camera_x;camera_y=test_map_camera_y;camera_settings=test_map_camera_settings;}
    test_map_active=0;
}
void td_audio_init(void) {audio_inits++;audio_mode=TD_AUDIO_FULL;audio_active=0;audio_cue=255;}
void td_audio_update(WORD speed,UBYTE vehicle,UBYTE onfoot,UBYTE braking,UBYTE active) {
    (void)speed;(void)vehicle;(void)onfoot;(void)braking;audio_updates++;audio_active=active;
}
void td_audio_play(UBYTE cue) {if(audio_mode!=TD_AUDIO_SILENT)audio_cue=cue;}
void td_audio_set_mode(UBYTE mode) {audio_mode=mode;}
UBYTE td_audio_get_mode(void) {return audio_mode;}
void td_get_street(UWORD u,UWORD v,char *out) { (void)u;(void)v;strcpy(out,"TEST ROAD"); }
void td_get_stop(UBYTE index,td_stop_t *out) {
    if(authored_content){stop_reads++;td_authored_get_stop(index,out);return;}
    stop_reads++;memset(out,0,sizeof(*out));out->u=900;out->v=900;
    out->transit=index==0?1:0;
    if(index==0&&stop0_here) {out->u=400;out->v=450;}
}
void td_get_job(UBYTE index,td_job_t *out) {
    if(authored_content){td_authored_get_job(index,out);return;}
    (void)index;memset(out,0,sizeof(*out));out->count=2;
    out->vehicle=TD_NONE;out->seconds=120;out->reward=150;out->route[1]=1;
}
UBYTE td_district_current(void) {return test_current_district;}
UBYTE td_district_scene(UBYTE district,far_ptr_t *out) {
    if(!out||district>=TD_DISTRICT_COUNT)return FALSE;
    test_native_scenes[district].width=native_widths[district];
    test_native_scenes[district].height=native_heights[district];
    test_native_scenes[district].collisions.bank=district+1;
    test_native_scenes[district].collisions.ptr=native_collision[district];
    out->bank=district+1;out->ptr=&test_native_scenes[district];return TRUE;
}
void td_district_reset(void) {test_reset_calls++;test_queued_district=TD_DISTRICT_NONE;}
UBYTE td_district_queue(UBYTE district) {
    test_queue_calls++;
    if(test_queue_fail||district>=TD_DISTRICT_COUNT||district==test_current_district)return FALSE;
    test_queued_district=district;return TRUE;
}
UBYTE td_district_drivable(UBYTE district,UWORD u,UWORD v) {
    if(district>=TD_DISTRICT_COUNT||u<8||v<8||u>1016||v>968)return FALSE;
    for(unsigned y=(v-5)>>3;y<=((v+5)>>3);y++)
        for(unsigned x=(u-5)>>3;x<=((u+5)>>3);x++)
            if(district_tile(district,x,y))return FALSE;
    return TRUE;
}
UBYTE td_district_walkable(UBYTE district,UWORD u,UWORD v) {
    return district<TD_DISTRICT_COUNT&&u<1024&&v<976&&!(district_tile(district,u>>3,v>>3)&15);
}

static void reset_case(void) {
    memset(&td,0,sizeof(td));memset(&td_job,0,sizeof(td_job));
    memset(&td_offer,0,sizeof(td_offer));memset(actors,0,sizeof(actors));
    memset(&td_cursor,0,sizeof(td_cursor));memset(&td_target,0,sizeof(td_target));
    memset(td_test_sram,0,sizeof(td_test_sram));
    td.u=400*16;td.v=450*16;td.park_u=100*16;td.park_v=100*16;
    td.job=TD_NONE;td.mode=TD_ROAM;td.health=100;td.cash=30;
    td.safe_u=td.u;td.safe_v=td.v;
    td_tick=td_notice_timer=td_red_cooldown=td_turn_tick=td_entry_timer=0;
    td_entry_target=td_walk_dir=td_input_edge=0;
    td_session_live=td_transition_pending=test_current_district=test_queue_fail=0;test_queued_district=TD_DISTRICT_NONE;
    td_vx=td_vy=0;td_last_frame=0;td_resume_mode=TD_ROAM;
    td_route_district=TD_DISTRICT_NONE;memset(td_traffic_samples,0,sizeof(td_traffic_samples));
    td_corner_used=0;
    memset(td_nearby_routes,0,sizeof(td_nearby_routes));
    joy=joy_pressed=0;sys_time=0;stop_reads=ui_draws=0;
    stop0_here=authored_content=0;test_queue_calls=test_reset_calls=0;
    test_map_opens=test_map_updates=test_map_closes=0;
    test_map_active=test_map_buttons=test_map_pressed=test_map_camera_settings=0;
    test_map_camera_x=test_map_camera_y=0;
    td_save_slot=TD_NONE;td_save_seq=0;actors_inactive_head=test_actors_active_head=NULL;actors_len=0;
    sram_writes=sram_interrupt_after=0;sram_interrupt_enabled=0;
    geometry=CLEAR_GROUND;
    td_audio_init();audio_updates=audio_inits=0;
    for(unsigned i=0;i<6;i++) { td_traffic_u[i]=30000;td_traffic_v[i]=30000;td_traffic_leg[i]=0;td_ped_route[i]=TD_NONE; }
    td_ped_refresh=1;td_ped_anchor_u=td.u>>4;td_ped_anchor_v=td.v>>4;
    td_streetcar_runtime_reset();
}

static void driving_tick(UBYTE held) {
    joy_pressed=held & ~joy;joy=held;td_tick++;sys_time++;td_corner_used=0;td_drive();
}
static void world_tick(UBYTE held,UWORD elapsed) {
    joy_pressed=held & ~joy;joy=held;sys_time+=elapsed;toronto_update();
}
static WORD prime_car(void) {
    for(unsigned i=0;i<180;i++) driving_tick(J_A);
    expect(td.speed>0,"clear-ground acceleration reaches forward speed");
    return td.speed;
}

static void test_acceleration_and_turning(void) {
    reset_case();WORD baseline=prime_car(),minimum=td.speed;
    UWORD initial_u=td.u,initial_v=td.v;unsigned distance=0;
    for(unsigned i=0;i<100;i++) {
        UWORD old_u=td.u,old_v=td.v;driving_tick(J_A|J_RIGHT);
        if(td.speed<minimum)minimum=td.speed;
        distance+=td_distance(old_u,td.u)+td_distance(old_v,td.v);
    }
    expect(td.heading!=0,"held steering changes vehicle heading");
    expect(minimum>=baseline*3/4,"acceleration plus steering retains speed on clear ground");
    expect(distance>32*16,"turning advances along a substantial path");
    expect(td.u!=initial_u||td.v!=initial_v,"turning does not leave the car stationary");

    reset_case();baseline=prime_car();initial_u=td.u;
    for(unsigned i=0;i<80;i++)driving_tick(J_A|J_LEFT|J_RIGHT);
    expect(td.heading==0,"opposing steering inputs cancel rather than picking a direction");
    expect(td.u>initial_u&&td.speed>=baseline,"opposing steering inputs preserve acceleration");
}

static void test_glancing_contact(void) {
    reset_case();WORD baseline=prime_car();
    td.u=300*16;td.v=394*16;td.heading=1;geometry=SOUTH_CURB;
    UWORD start=td.u;WORD minimum=td.speed;
    for(unsigned i=0;i<80;i++) {
        driving_tick(J_A);if(td.speed<minimum)minimum=td.speed;
        expect(td.v>>4<395,"glancing contact remains outside curb footprint");
    }
    expect(td.u>start+32*16,"glancing contact slides forward beside curb");
    expect(minimum>=baseline*3/4,"continuous glancing contact does not repeatedly drain speed");
    expect(td.speed>=baseline-2,"acceleration restores speed during continued curb sliding");
}

static void test_wall_and_brake(void) {
    reset_case();prime_car();td.u=394*16;td.v=450*16;geometry=EAST_WALL;
    UWORD before=td.u;driving_tick(J_A);
    expect(td.u==before,"head-on wall blocks forward movement");
    expect(td.speed==0&&td_vx==0&&td_vy==0,"head-on wall stops signed speed and both velocity axes");
    geometry=CLEAR_GROUND;
    for(unsigned i=0;i<120;i++)driving_tick(J_B);
    expect(td.speed<0&&td.u<before,"braking and reversing recover away from wall");

    reset_case();WORD baseline=prime_car();
    for(unsigned i=0;i<20;i++)driving_tick(J_A|J_B);
    expect(td.speed<baseline,"brake wins when acceleration is held simultaneously");
    for(unsigned i=0;i<100;i++)driving_tick(J_A|J_B);
    expect(td.speed<0,"held brake permits reverse despite held acceleration");
}

static void test_momentum_and_coasting(void) {
    reset_case();prime_car();td.heading=4;
    driving_tick(J_A);
    expect(td_vx>0&&td_vy>0,"traction retains previous momentum while adding the new heading");

    reset_case();WORD baseline=prime_car();UWORD before=td.u;
    for(unsigned i=0;i<40;i++)driving_tick(0);
    expect(td.u>before,"released throttle continues rolling");
    expect(td.speed>0&&td.speed<baseline,"coasting decelerates gradually rather than stopping instantly");
    for(unsigned i=0;i<240;i++)driving_tick(0);
    expect(td.speed==0,"coasting eventually reaches rest");
}

static void test_pressed_edge_once(void) {
    reset_case();td.onfoot=1;joy=joy_pressed=J_B;sys_time=4;
    toronto_update();
    expect(stop_reads==TD_STOPS,"one failed transit press is processed once across motion substeps");
    unsigned before=stop_reads;joy_pressed=0;sys_time+=4;toronto_update();
    expect(stop_reads==before,"held transit button does not repeat the pressed action");
}

static void test_passenger_comfort(void) {
    reset_case();prime_car();td.job=0;td_job.kind=5;td.stage=1;td.health=100;
    for(unsigned i=0;i<36;i++)driving_tick(J_A|J_RIGHT);
    expect(td.health<100,"fast passenger turns reduce comfort");
    expect(td.health>0&&td.job!=TD_NONE,"a few fast turns do not instantly fail the passenger job");

    reset_case();td.job=0;td_job.kind=5;td.stage=1;td.speed=5;td_vx=80;td.health=100;
    for(unsigned i=0;i<36;i++)driving_tick(J_RIGHT);
    expect(td.health==100,"slow passenger turns preserve comfort");
}

static void test_entry_collision(void) {
    reset_case();geometry=NATIVE_GRID;
    td.park_u=76*16;td.park_v=744*16;td.u=92*16;td.v=766*16;td.onfoot=1;
    expect(td_drivable(td.park_u>>4,td.park_v>>4),"rail fixture parked car has a usable footprint");
    expect(td_walkable(td.u>>4,td.v>>4),"rail fixture courier endpoint is walkable");
    UWORD before_u=td.u,before_v=td.v;td_enter_exit();
    expect(td.onfoot&&td_entry_timer==0,"entry cannot animate through native rail collision");
    expect(td.u==before_u&&td.v==before_v,"rejected entry keeps the courier position");

    reset_case();td.park_u=500*16;td.park_v=450*16;td.u=518*16;td.v=450*16;td.onfoot=1;
    td_enter_exit();expect(td_entry_timer>0,"clear door approach begins an entry animation");
    for(unsigned i=0;i<24;i++)driving_tick(0);
    expect(!td.onfoot&&td.u==td.park_u&&td.v==td.park_v,"clear entry finishes at the parked vehicle");
}

static void test_hidden_pedestrian(void) {
    reset_case();td.u=398*16;td.v=424*16;td.subsecond=35;
    td.speed=20;td_vx=320;
    td_ped_refresh=10;td_ped_route[0]=0;
    td_nearby_routes[0][0]=td_district_routes[0][0][0];
    td_nearby_routes[0][1]=td_district_routes[0][0][1];
    td_pedestrians();
    expect(actors[9].flags&ACTOR_FLAG_HIDDEN,"distant fixed-route pedestrian is hidden");
    expect(td.speed==20&&td_vx==320,"hidden pedestrian does not slow a nearby vehicle");
}

static void test_signal_and_autonomous_traffic(void) {
    reset_case();td.onfoot=1;td.seconds=8;
    td_traffic_u[0]=184*16;td_traffic_v[0]=288*16;UWORD before=td_traffic_u[0];
    td_traffic_step();expect(td_traffic_u[0]==before,"red traffic stops before the Bathurst intersection in world units");
    td.seconds=0;td_traffic_step();expect(td_traffic_u[0]>before,"green traffic leaves the stop line");

    td.seconds=8;td_traffic_u[0]=182*16+6;before=td_traffic_u[0];
    td_traffic_step();expect(td_traffic_u[0]>before,"red traffic does not stop at arbitrary eight-pixel intervals");

    td.seconds=0;td_traffic_u[4]=824*16;td_traffic_v[4]=376*16;before=td_traffic_v[4];
    td_traffic_step();expect(td_traffic_v[4]==before,"vertical red traffic stops before Dundas");
    td.seconds=8;td_traffic_step();expect(td_traffic_v[4]>before,"vertical green traffic resumes");

    reset_case();td.mode=TD_WAIT;td.onfoot=1;td_traffic_u[0]=100*16;td_traffic_v[0]=280*16;before=td_traffic_u[0];sys_time=4;
    toronto_update();expect(td_traffic_u[0]>before,"traffic continues moving during unpaused transit waiting");
    expect(actors[9].pos.x||actors[9].pos.y,"pedestrians update during unpaused transit waiting");
}

static void test_city_routes_and_walking(void) {
    reset_case();geometry=NATIVE_GRID;
    expect(!td_drivable(76,756),"car footprint rejects a narrow solid rail under its centre");
    reset_case();geometry=NATIVE_GRID;td.u=793*16+2;td.v=730*16+12;
    td.heading=4;td.speed=21;td_vx=0;td_vy=336;
    for(unsigned i=0;i<16;i++)driving_tick(J_A);
    expect(td.speed>=21&&td.v>738*16,"held throttle clears a small quantised corner overlap without losing forward speed");
    expect(td_drivable(td.u>>4,td.v>>4),"corner slide retains a collision-valid car footprint");

    reset_case();geometry=EAST_WALL;td.u=394*16;td.v=450*16;td.heading=0;td.speed=24;td_vx=384;
    driving_tick(J_A);expect(td.speed==0&&td.u==394*16,"corner assist cannot bypass a broad head-on wall");

    reset_case();geometry=NATIVE_GRID;toronto_init();td.mode=TD_ROAM;
    td.u=560*16;td.v=720*16;td.onfoot=0;
    int usable=1,continuous=1;
    for(unsigned step=0;step<12000;step++) {
        UWORD old_u=td_traffic_u[5],old_v=td_traffic_v[5];
        td.seconds=step/60;td.subsecond=step%60;td_traffic_step();
        if(td_distance(old_u,td_traffic_u[5])+td_distance(old_v,td_traffic_v[5])>8)continuous=0;
        for(unsigned i=0;i<6;i++)if(!td_drivable(td_traffic_u[i]>>4,td_traffic_v[i]>>4))usable=0;
    }
    expect(continuous,"autonomous bus has continuous movement through clock changes and route loops");
    expect(usable,"all six vehicles follow usable native road footprints through a long route run");

    reset_case();geometry=NATIVE_GRID;int sidewalk=1;
    for(unsigned route=0;route<TD_PEDESTRIAN_ROUTES;route++)
        for(unsigned offset=0;offset<64;offset++)
            if(!td_walkable(td_district_routes[td.district][route][0]+offset,td_district_routes[td.district][route][1]))sidewalk=0;
    expect(sidewalk,"every fixed pedestrian route stays on native walkable collision");
    td.u=120*16;td.v=40*16;td_pedestrians();
    UBYTE identity=td_ped_route[0];UWORD npc_u=actors[9].pos.x,npc_v=actors[9].pos.y;
    expect(identity!=TD_NONE,"city selects a real nearby pedestrian identity");
    td.u=136*16;td_pedestrians();
    expect(td_ped_route[0]==identity&&actors[9].pos.x==npc_u&&actors[9].pos.y==npc_v,
           "crossing the old camera segment boundary does not teleport a visible pedestrian");

    reset_case();td.onfoot=1;td_traffic_u[0]=410*16;td_traffic_v[0]=450*16;
    UWORD player_u=td.u;for(unsigned i=0;i<40;i++)driving_tick(J_RIGHT);
    expect(td.u==player_u,"walker cannot pass through an occupied traffic vehicle");
    td_traffic_u[0]=500*16;driving_tick(J_RIGHT);
    expect(td.u>player_u,"walker can continue after traffic clears");
    player_u=td.u;UWORD player_v=td.v;driving_tick(J_LEFT|J_RIGHT|J_UP|J_DOWN);
    expect(td.u==player_u&&td.v==player_v,"opposing walking inputs cancel on both axes");

    reset_case();td.onfoot=1;td.u=184*16;td.v=280*16;
    td_traffic_u[0]=171*16;td_traffic_v[0]=280*16;player_u=td_traffic_u[0];
    td_traffic_step();expect(td_traffic_u[0]==player_u,"approaching traffic yields to a courier crossing on foot");
    td.v+=40*16;td_traffic_step();expect(td_traffic_u[0]>player_u,"yielding traffic continues once the courier clears");
    td.v=280*16;td.mode=TD_WAIT;player_u=td_traffic_u[0];td_traffic_step();
    expect(td_traffic_u[0]==player_u,"traffic also yields to a stationary courier waiting for transit");

    reset_case();td_traffic_u[0]=400*16;td_traffic_v[0]=468*16;
    td_traffic_u[1]=418*16;td_traffic_v[1]=450*16;td_enter_exit();
    expect(td.onfoot&&td.u==400*16&&td.v==432*16,"car exit skips doors occupied by moving vehicles");

    reset_case();td_traffic_u[0]=300*16;td_traffic_v[0]=280*16;
    td_traffic_step();expect(!actors[2].pos.x&&!actors[2].pos.y,"motion substeps do not perform duplicate actor presentation");
    td_traffic_present();expect(actors[2].pos.x==(td_traffic_u[0]>>4)*32,"actor presentation reflects the final integrated position");
}

static void test_audio_event_integration(void) {
    reset_case();sys_time=2;toronto_update();
    expect(audio_updates==1&&audio_active,"roaming flushes audio once per rendered update");
    joy=joy_pressed=J_START;sys_time+=2;toronto_update();
    expect(td.mode==TD_PAUSE&&!audio_active&&audio_updates==2,"entering pause stops world audio on the same update");
    joy=joy_pressed=0;sys_time+=240;toronto_update();
    expect(!audio_active&&audio_updates==3,"paused frames continue to service audio without advancing the world");
    td.menu=8;td_resume_mode=TD_RIDE;
    for(unsigned mode=1;mode<=3;mode++) {
        joy=joy_pressed=J_A;sys_time+=2;toronto_update();
        expect(audio_mode==mode%3&&td.mode==TD_PAUSE,"audio mode can be cycled while a transit trip is paused");
    }
    reset_case();td.job=0;td_job.count=2;td_target.u=td.u>>4;td_target.v=td.v>>4;
    td_interact();expect(audio_cue==TD_AUDIO_PICKUP,"pickup queues its distinct audio cue");
    td.left=100;td_target.u=td.u>>4;td_target.v=td.v>>4;td_interact();expect(audio_cue==TD_AUDIO_COMPLETE&&td.mode==TD_RESULT,"final handoff queues completion audio for the result screen");
    reset_case();td.job=0;td_finish(FALSE);expect(audio_cue==TD_AUDIO_FAIL,"failed contracts queue failure audio");
    reset_case();td_message(5);expect(audio_cue==TD_AUDIO_IMPACT,"vehicle impacts queue their audio feedback");
}

static void test_bounded_corner_assist(void) {
    for(unsigned shift=1;shift<=7;shift++) {
        reset_case();geometry=SOUTHWEST_CORNER;td.u=(405-shift)*16;td.v=394*16;
        td.heading=4;td.speed=16;td_vx=0;td_vy=256;
        UWORD old_u=td.u,old_v=td.v;driving_tick(J_A);
        if(shift<=6) {
            expect(td.speed==16&&td.heading==4&&td.v>old_v,"small corner clearance retains heading, throttle and dominant travel");
            expect((unsigned)(td.u-old_u)==shift*16&&td_drivable(td.u>>4,td.v>>4),"corner assist chooses the nearest collision-valid lateral clearance");
        }else expect(td.speed==0&&td.u==old_u&&td.v==old_v,"seven-pixel blocked corner exceeds assistance budget and remains solid");
    }
    const UBYTE prohibited[]={0,J_A|J_B,J_B};
    for(unsigned input=0;input<3;input++) {
        reset_case();geometry=SOUTHWEST_CORNER;td.u=404*16;td.v=394*16;td.speed=16;td_vy=256;joy=prohibited[input];
        expect(!td_corner_slide(td.u,395*16),"coasting and braking do not invoke throttle corner assistance");
    }
    reset_case();geometry=SOUTHWEST_CORNER;td.u=404*16;td.v=394*16;td.speed=-6;td_vy=256;joy=J_A;
    expect(!td_corner_slide(td.u,395*16),"reverse does not invoke forward corner assistance");
    td.speed=16;td_vx=td_vy=256;
    expect(!td_corner_slide(td.u,395*16),"equal diagonal velocity has no arbitrary assistance axis");
    td_vx=0;td_vy=256;td_corner_used=1;
    expect(!td_corner_slide(td.u,395*16),"catch-up steps cannot apply multiple lateral assists in one rendered update");
    reset_case();joy=J_A;td.speed=16;td_vy=256;td.v=968*16;
    expect(!td_corner_slide(td.u,td.v+16),"corner assistance cannot push the car beyond the southern map bound");
}

static void test_clock(void) {
    reset_case();td.mode=TD_RIDE;td.ride_left=255;
    for(unsigned i=0;i<15;i++) {sys_time+=8;toronto_update();}
    expect(td.seconds==2&&td.subsecond==0,"clock counts all VBlanks when motion catch-up is capped");
    expect(td.ride_left==253,"ride timer receives the full elapsed seconds");

    reset_case();td.mode=TD_RIDE;td.ride_left=255;sys_time=257;toronto_update();
    expect(td.seconds==4&&td.subsecond==17,"elapsed gaps larger than one byte retain full clock time");

    reset_case();td.mode=TD_RIDE;td.ride_left=255;td_last_frame=65530;sys_time=10;toronto_update();
    expect(td.seconds==0&&td.subsecond==16,"VBlank counter wrap preserves elapsed time");

    reset_case();td.mode=TD_PAUSE;sys_time=240;toronto_update();
    expect(td.seconds==0&&td.subsecond==0,"pause freezes the world clock");

    reset_case();prime_car();td.u=400*16;td.v=450*16;UWORD before=td.u;
    joy=J_A;joy_pressed=0;sys_time=600;toronto_update();
    expect(td.seconds==10&&td.subsecond==0,"large gap advances full clock independently of movement");
    expect(td.u>=before&&td.u-before<16*16,"large gap does not teleport through hundreds of motion steps");
}

static void refresh_record_crc(volatile UBYTE *record) {
    UWORD crc=0xFFFF;
    for(unsigned i=2;i<=4;i++)crc=td_crc_byte(crc,record[i]);
    for(unsigned i=0;i<sizeof(td);i++)crc=td_crc_byte(crc,record[8+i]);
    record[5]=crc;record[6]=crc>>8;
}

static void test_atomic_saves(void) {
    UBYTE old_image[sizeof(td_test_sram)],new_image[sizeof(td_test_sram)];
    reset_case();td.cash=111;td_save();
    UBYTE stable_slot=0,stable_seq=0,newest_slot=0;
    td_state_t stable,candidate;
    /* Exercise both an empty slot and replacement of the older committed slot. */
    for(unsigned direction=0;direction<2;direction++) {
        stable_slot=td_save_slot;stable_seq=td_save_seq;stable=td;
        memcpy(old_image,td_test_sram,sizeof(old_image));
        td.cash+=111;td.complete[0]=direction?31:15;td.done=direction?5:4;candidate=td;
        sram_writes=0;td_save();unsigned count=sram_writes;newest_slot=td_save_slot;
        memcpy(new_image,td_test_sram,sizeof(new_image));
        expect(count>sizeof(td),"save trace observes payload plus record metadata writes");
        expect(sram_offsets[0]==(unsigned)(td_save_address(newest_slot)-td_test_sram)&&sram_values[0]==0,
               "save invalidates the destination record before writing it");
        expect(sram_offsets[count-1]==sram_offsets[0]&&sram_values[count-1]==0x54,
               "save commits its magic byte after every payload and checksum store");
        /* Interrupt after every real store, not a fabricated serializer or image prefix. */
        for(volatile unsigned cut=1;cut<=count;cut++) {
            memcpy(td_test_sram,old_image,sizeof(old_image));td=candidate;
            td_save_slot=stable_slot;td_save_seq=stable_seq;
            sram_writes=0;sram_interrupt_after=cut;sram_interrupt_enabled=1;
            if(setjmp(interrupted_save)==0) {
                td_save();expect(0,"configured SRAM interruption must occur");
            }
            sram_interrupt_enabled=0;memset(&td,0,sizeof(td));
            expect(td_restore(),"a power interruption retains a valid committed snapshot");
            expect(td.cash==(cut==count?candidate.cash:stable.cash)&&td.done==(cut==count?candidate.done:stable.done),
                   "interrupted snapshot is ignored until its final commit store");
        }
    }

    memcpy(td_test_sram,new_image,sizeof(new_image));
    td_save_address(newest_slot)[8+offsetof(td_state_t,cash)]^=1;
    expect(td_restore()&&td.cash==stable.cash,"payload CRC corruption falls back to the older slot");
    td_save_address(stable_slot)[8+offsetof(td_state_t,cash)]^=1;
    expect(!td_restore(),"both corrupt CRC records are rejected");

    reset_case();
    for(unsigned i=0;i<260;i++) {td.cash=100+i;td_save();}
    td.cash=0;
    expect(td_restore()&&td.cash==359,"sequence wrap selects the latest committed record");
    UWORD known_crc=0xFFFF;const char *text="123456789";
    while(*text)known_crc=td_crc_byte(known_crc,*text++);
    expect(known_crc==0x29B1,"CRC16 agrees with the standard CCITT-FALSE check vector");
}

static void test_valid_crc_invalid_states(void) {
    UBYTE valid_image[sizeof(td_test_sram)];
    reset_case();td.cash=111;td_save();td.cash=222;td_save();
    UBYTE newest_slot=td_save_slot;memcpy(valid_image,td_test_sram,sizeof(valid_image));
    for(unsigned invalid=0;invalid<23;invalid++) {
        memcpy(td_test_sram,valid_image,sizeof(valid_image));
        volatile UBYTE *record=td_save_address(newest_slot);
        td_state_t bad;memcpy(&bad,(const void *)(record+8),sizeof(bad));
        switch(invalid) {
            case 0:bad.heading=16;break;
            case 1:bad.vehicle=4;break;
            case 2:bad.onfoot=2;break;
            case 3:bad.health=101;break;
            case 4:bad.subsecond=60;break;
            case 5:bad.mode=TD_HELP+1;break;
            case 6:bad.u=1024*16;break;
            case 7:bad.v=976*16;break;
            case 8:bad.park_u=1024*16;break;
            case 9:bad.park_v=976*16;break;
            case 10:bad.done=1;break;
            case 11:bad.job=TD_QUESTS;break;
            case 12:bad.job=0;bad.left=120;bad.stage=2;break;
            case 13:bad.job=0;bad.left=0;break;
            case 14:bad.job=0;bad.left=120;bad.health=0;break;
            case 15:bad.mode=TD_WAIT;bad.transit_origin=0;bad.transit_target=12;break;
            case 16:bad.mode=TD_WAIT;bad.onfoot=1;bad.transit_origin=0;bad.transit_target=0;break;
            case 17:bad.mode=TD_WAIT;bad.onfoot=1;bad.transit_origin=TD_STOPS;bad.transit_target=12;break;
            case 18:bad.mode=TD_WAIT;bad.onfoot=1;bad.transit_origin=64;bad.transit_target=16;break;
            case 19:bad.mode=TD_RIDE;bad.onfoot=1;bad.transit_origin=0;bad.transit_target=12;bad.ride_left=0;break;
            case 20:bad.mode=TD_RIDE;bad.onfoot=1;bad.transit_origin=0;bad.transit_target=12;bad.ride_left=255;break;
            case 21:bad.mode=TD_WAIT;bad.onfoot=1;bad.transit_origin=128;bad.transit_target=16;break;
            case 22:bad.mode=TD_WAIT;bad.onfoot=1;bad.transit_origin=0;bad.transit_target=18;break;
        }
        memcpy((void *)(record+8),&bad,sizeof(bad));refresh_record_crc(record);
        if(!(td_restore()&&td.cash==111)) {
            char name[96];snprintf(name,sizeof(name),"CRC-valid invalid state %u is rejected in favour of the older slot",invalid);
            expect(0,name);
        } else expect(1,"CRC-valid invalid state rejected");
    }
}

static void test_legacy_and_transit_recovery(void) {
    reset_case();td.cash=333;td.done=1;td.complete[0]=1;td.job=0;td.stage=1;td.left=80;td.health=99;
    volatile UBYTE *legacy=td_save_address(0);const UBYTE *bytes=(const UBYTE *)&td;UBYTE xor=0,old[48];
    memcpy(old,bytes,26);memcpy(old+26,td.complete,9);memcpy(old+35,bytes+42,5);memcpy(old+40,bytes+48,8);
    for(unsigned i=0;i<48;i++) {legacy[4+i]=old[i];xor^=old[i];}
    legacy[0]=0x54;legacy[1]=0xD7;legacy[2]=4;legacy[3]=xor;memset(&td,0,sizeof(td));
    expect(td_restore()&&td.cash==333&&td.done==1,"legacy migration retains earnings and unique completion");
    expect(td.job==TD_NONE&&td.stage==0&&td.left==0&&td.health==100,"legacy changed campaign retires active work safely");
    expect(td_save_address(td_save_slot)[2]==TD_SAVE_VERSION,"legacy migration writes a current dual-slot record");

    reset_case();td.mode=TD_RIDE;td.onfoot=1;td.transit_origin=0;td.transit_target=17;td.ride_left=4;td.cash=27;
    td_save();memset(&td,0,sizeof(td));toronto_init();
    expect(td.mode==TD_HELP&&td.cash==27&&td.ride_left==4,"restart retains a paid ride behind the help screen");
    joy=joy_pressed=J_A;sys_time++;toronto_update();
    expect(td.mode==TD_RIDE&&td.cash==27&&td.ride_left==4,"leaving help resumes the paid ride without a second fare");
    joy=joy_pressed=0;sys_time+=120;toronto_update();
    expect(td.mode==TD_RIDE&&td.ride_left==2,"ride advances before testing its next recovery checkpoint");
    memset(&td,0,sizeof(td));actors_inactive_head=NULL;td_session_live=0;toronto_init();
    expect(td.mode==TD_HELP&&td.ride_left==2&&td.seconds==2&&td.cash==27,"restart resumes the most recent second of a paid trip");
    joy=joy_pressed=J_A;sys_time++;toronto_update();joy=joy_pressed=0;sys_time+=240;toronto_update();
    expect(td.mode==TD_ROAM&&td.u==900*16&&td.v==900*16&&td.cash==27,"recovered ride reaches its retained destination once");

    reset_case();td.mode=TD_RIDE;td.onfoot=1;td.transit_origin=0;td.transit_target=17;td.ride_left=3;
    td.cash=27;td.job=0;td_job.kind=0;td.left=1;td_cursor.u=500;td_cursor.v=500;sys_time=60;
    toronto_update();expect(td.mode==TD_RIDE&&td.job==TD_NONE&&td.health==0,"deadline expiry continues an already-paid ride");
    sys_time+=180;toronto_update();
    expect(td.mode==TD_RESULT&&td.u==500*16&&td.v==500*16&&td.cash==27,"expired parcel reaches destination before failure result");
}

static void test_wait_cancellation(void) {
    reset_case();stop0_here=1;td.onfoot=1;td.mode=TD_WAIT;td.transit_origin=0;td.transit_target=12;td.seconds=5;
    td_save();joy=joy_pressed=J_B;sys_time=1;toronto_update();
    expect(td.mode==TD_ROAM&&td.cash==30,"cancelling wait returns to roaming without reopening the transit menu");
    memset(&td,0,sizeof(td));expect(td_restore()&&td.mode==TD_ROAM&&td.cash==30,"cancelled waiting state remains cancelled after restart");

    reset_case();stop0_here=1;td.onfoot=1;td.mode=TD_WAIT;td.transit_origin=0;td.transit_target=12;
    td.seconds=17;td.subsecond=59;td_save();joy=joy_pressed=J_B;sys_time=1;toronto_update();
    expect(td.mode==TD_ROAM&&td.cash==30,"cancel input wins on the exact scheduled boarding tick");
    memset(&td,0,sizeof(td));expect(td_restore()&&td.mode==TD_ROAM&&td.cash==30,"boarding-tick cancellation is saved without a fare charge");
}

static void test_entry_transit_exclusion(void) {
    reset_case();stop0_here=1;td.onfoot=1;td.park_u=418*16;td.park_v=450*16;
    world_tick(J_A|J_B,1);
    expect(td.mode==TD_ROAM&&td.onfoot&&td_entry_timer>0,
           "simultaneous entry and transit input keeps the accepted car entry in roaming");
    for(unsigned i=0;i<16;i++)world_tick(0,1);
    expect(td.mode==TD_ROAM&&!td.onfoot&&td.u==td.park_u&&td.v==td.park_v,
           "accepted car entry finishes locally instead of being suspended by a transit menu");

    reset_case();stop0_here=1;td.onfoot=1;td.park_u=418*16;td.park_v=450*16;
    world_tick(J_A,1);world_tick(J_START,1);
    UBYTE remaining=td_entry_timer;
    expect(td.mode==TD_PAUSE&&remaining>0,"active car entry can be deliberately paused");
    td.menu=5;world_tick(J_A,1);
    expect(td.mode==TD_PAUSE&&td_entry_timer==remaining&&td.onfoot&&td_resume_mode==TD_ROAM,
           "paused transit selection cannot interrupt an unfinished car entry");
    world_tick(J_B,1);
    for(unsigned i=0;i<16;i++)world_tick(0,1);
    expect(td.mode==TD_ROAM&&!td.onfoot&&td.u==td.park_u&&td.v==td.park_v,
           "rejected paused transit leaves car entry able to resume at the parked vehicle");
}

static void test_fresh_transit_after_failure(void) {
    reset_case();stop0_here=1;td.onfoot=1;td.job=0;td.left=1;td_job.kind=0;
    world_tick(0,60);
    expect(td.mode==TD_RESULT&&td.job==TD_NONE&&td.health==0,
           "an actual missed deadline produces a failed contract before free roaming");
    world_tick(J_B,1);world_tick(0,1);world_tick(J_B,1);
    expect(td.mode==TD_TRANSIT&&td.job==TD_NONE,"a failed contract permits a new free-roaming transit booking");
    world_tick(J_RIGHT,1);td.seconds=2;world_tick(J_A,1);
    expect(td.mode==TD_WAIT&&td.transit_target==12,"fresh trip selects a different served destination");
    td.seconds=17;td.subsecond=59;world_tick(0,1);
    expect(td.mode==TD_RIDE&&td.cash==27,"fresh free-roaming trip boards and charges exactly one fare");
    world_tick(0,60);
    expect(td.mode==TD_ROAM&&td.job==TD_NONE&&td.u==900*16&&td.v==900*16&&td.cash==27,
           "past contract failure cannot turn a fresh free-roaming arrival into another failure result");

    reset_case();stop0_here=1;td.onfoot=1;td.job=0;td.left=2;td_job.kind=0;
    world_tick(J_B,1);
    for(unsigned i=0;i<6;i++){world_tick(J_RIGHT,1);world_tick(0,1);}
    td.seconds=2;world_tick(J_A,1);td.seconds=17;td.subsecond=59;world_tick(0,1);
    expect(td.mode==TD_RIDE&&td.transit_target==17&&td.cash==27&&td.job==0,
           "an active parcel boards a longer paid trip before its deadline");
    world_tick(0,60);
    expect(td.mode==TD_RIDE&&td.job==TD_NONE&&td.health==0&&td.cash==27,
           "actual deadline expiry during the fresh paid trip retains failure while travel continues");
    world_tick(0,180);
    expect(td.mode==TD_RESULT&&td.u==900*16&&td.v==900*16&&td.cash==27,
           "a parcel expiring on its own paid trip still reaches the destination and shows its failure result");
}

static void native_case(void) {
    reset_case();geometry=NATIVE_GRID;authored_content=1;
    td.u=td.park_u=td.safe_u=560*16;td.v=td.park_v=td.safe_v=720*16;
}

static void test_pickup_damage_lifecycle(void) {
    const UBYTE jobs[3]={0,1,5};
    const char *kinds[3]={"parcel","fragile","passenger"};
    const char *hazards[4]={"traffic impact","head-on wall","glancing curb","fast steering"};
    /* Each carrying case starts afresh and collects through the actual
       interaction handler, so a pre-pickup defect cannot cascade into its
       expected post-pickup penalty. Retired cases retain stale route data. */
    for(UBYTE kind=0;kind<3;kind++)for(UBYTE hazard=0;hazard<4;hazard++)for(UBYTE lifecycle=0;lifecycle<3;lifecycle++) {
        native_case();td.health=37;
        if(kind==2){td.done=8;td.complete[0]=0x4F;td.complete[10]=7;}
        td.mode=TD_BOARD;td.menu=jobs[kind];td_get_job(td.menu,&td_offer);
        world_tick(J_A,1);
        expect(td.mode==TD_ROAM&&td.job==jobs[kind]&&td.stage==0&&td.health==100,
               "actual authored offer acceptance begins an empty approach at stage0 with fresh cargo condition");
        if(lifecycle) {
            world_tick(0,0);world_tick(J_SELECT,1);
            expect(td.mode==TD_ROAM&&td.job==jobs[kind]&&td.stage==1&&td.health==100,
                   "an actual stopped Union pickup advances the accepted authored job into carrying");
        }
        if(lifecycle==2){td.job=TD_NONE;td.stage=9;td.health=37;}
        td.cooldown=td_turn_tick=td_tick=0;td.speed=24;td.heading=0;
        td_vx=384;td_vy=0;joy=joy_pressed=0;
        td.u=400*16;td.v=450*16;td.safe_u=td.u;td.safe_v=td.v;
        UBYTE damage=0;
        if(hazard==0) {
            geometry=CLEAR_GROUND;td_traffic_u[0]=td.u;td_traffic_v[0]=td.v;
            td_traffic_step();damage=12;
            expect(td.speed==12&&td_vx==192&&td.cooldown==60&&td.msg==5,
                   "traffic still slows and warns an empty, carrying or retired vehicle");
        }else if(hazard==1) {
            geometry=EAST_WALL;td.u=394*16;td.safe_u=td.u;
            driving_tick(0);damage=kind==1?20:8;
            expect(td.u==394*16&&td.speed==0&&!td_vx&&!td_vy&&td.cooldown==45&&td.msg==5,
                   "a broad wall still stops the vehicle and applies collision cooldown before and after pickup");
        }else if(hazard==2) {
            geometry=EAST_WALL;td.u=394*16;td.safe_u=td.u;td.heading=1;td_vy=128;
            driving_tick(0);damage=kind==1?4:1;
            expect(td.u==394*16&&td.v>450*16&&td.speed==24&&!td_vx&&td_vy>0&&td.cooldown==30&&td.msg==5,
                   "glancing curb recovery keeps forward speed and its warning independently of cargo occupancy");
        }else {
            geometry=CLEAR_GROUND;td_turn_tick=11;driving_tick(J_RIGHT);
            damage=kind==2?1:0;
            expect(td.heading==1&&td.speed==24,
                   "the same high-speed steering input turns empty, occupied and retired cars normally");
            if(kind==2)expect(td.msg==(lifecycle==1?14:0),
                   "rider comfort alerts belong only to an actually occupied passenger job");
        }
        UBYTE expected=lifecycle==2?37:lifecycle==1?100-damage:100;
        char label[150];snprintf(label,sizeof(label),"%s %s condition after %s is%u",kinds[kind],
            lifecycle==0?"before pickup":lifecycle==1?"after actual pickup":"without job, stale stage9",
            hazards[hazard],expected);
        expect(td.health==expected,label);
        expect(td.mode==TD_ROAM&&td.job==(lifecycle==2?TD_NONE:jobs[kind])&&td.stage==(lifecycle==2?9:lifecycle),
               "physical hazards preserve the actual lifecycle without failing an empty approach or reviving retired work");
    }
    /* Pickup eligibility does not defer the contract clock. */
    for(UBYTE kind=0;kind<3;kind++) {
        native_case();if(kind==2){td.done=8;td.complete[0]=0x4F;td.complete[10]=7;}
        td.mode=TD_BOARD;td.menu=jobs[kind];td_get_job(td.menu,&td_offer);world_tick(J_A,1);
        td.left=1;UWORD previous_seconds=td.seconds,previous_cash=td.cash;td_second();
        expect(td.mode==TD_RESULT&&td.job==TD_NONE&&td.stage==0&&td.health==0&&td.left==0&&
               td.seconds==previous_seconds+1&&td.cash==previous_cash,
               "an accepted parcel, fragile or passenger deadline still expires before its first pickup without rewarding the approach");
        expect(td_target.district==0&&td_target.u==560&&td_target.v==720,
               "pre-pickup timeout clears the former objective and restores Union free-roam guidance");
    }
    /* Old version6 saves can contain approach damage. Validate their real
       CRC/semantic records unchanged, then recover only acceptedstage0
       condition on cold startup, including before a remote-scene redirect. */
    const UBYTE old_health[5]={37,1,67,37,0};
    for(UBYTE recovery=0;recovery<5;recovery++) {
        native_case();td.cash=444;td.seconds=81;td.subsecond=13;
        td.health=old_health[recovery];td.job=recovery==4?TD_NONE:1;
        td.stage=recovery==3?1:recovery==4?7:0;td.left=recovery==4?0:110;
        td.mode=recovery==4?TD_RESULT:TD_ROAM;
        if(recovery==2){td.district=3;td.onfoot=1;td.u=td.safe_u=128*16;td.v=td.safe_v=528*16;}
        td_save();volatile UBYTE *legacy_record=td_save_address(td_save_slot);
        legacy_record[2]=6;refresh_record_crc(legacy_record);
        td_state_t saved=td;saved.mode=TD_ROAM;memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&saved,sizeof(td)),
               "actual version6 SRAM records validate their original approach, carried or no-job condition before cold-start normalization");
        memset(&td,0,sizeof(td));td_session_live=0;actors_inactive_head=NULL;toronto_init();
        expect(td.mode==TD_HELP&&td_resume_mode==TD_ROAM&&td.job==saved.job&&td.stage==saved.stage&&
               td.health==(recovery<3?100:saved.health),
               "cold recovery restores fresh uncollected cargo while preserving carried damage and a retired failure condition");
        expect(td.cash==saved.cash&&td.left==saved.left&&td.seconds==saved.seconds&&td.subsecond==saved.subsecond&&
               td.district==saved.district&&td.u==saved.u&&td.v==saved.v&&
               td.park_district==saved.park_district&&td.park_u==saved.park_u&&td.park_v==saved.park_v,
               "cold approach-condition recovery preserves earnings, deadline, clock, player and parked-car districts");
        expect(recovery==2?(td_transition_pending==1&&test_queued_district==3&&test_current_district==0):
                            (!td_transition_pending&&test_queued_district==TD_DISTRICT_NONE),
               "remote uncollected condition is recovered before the actual destination scene is queued, without redirecting local saves");
    }
    /* Invalid condition must fail semantic validation before normalization.
       Real CRC-valid newer records fall back to the older committed state. */
    const UBYTE invalid_health[2]={0,101};
    for(UBYTE fault=0;fault<2;fault++) {
        native_case();td.cash=111;td_save();
        td.job=1;td.stage=0;td.left=110;td.health=invalid_health[fault];td.cash=999;td_save();
        UBYTE sequence;td_state_t raw;
        expect(td_read_slot(td_save_slot,&raw,&sequence)&&raw.job==1&&raw.stage==0&&raw.health==invalid_health[fault],
               "invalid approach-condition fixture is a genuine committed CRC-valid newer current-version record");
        memset(&td,0,sizeof(td));td_session_live=0;actors_inactive_head=NULL;toronto_init();
        expect(td.mode==TD_HELP&&td.job==TD_NONE&&td.health==100&&td.cash==111,
               "zero or over100 active approach condition is rejected instead of being laundered by cold-start normalization");
    }
}

static void test_finished_job_target(void) {
    for(UBYTE success=0;success<2;success++) {
        td_stop_t depot,old_target;
        native_case();td_get_stop(0,&depot);
        td.job=success?0:80;td_get_job(td.job,&td_job);td.stage=1;
        td.health=100;td.left=success?99:1;td.mode=TD_ROAM;td_set_target();
        old_target=td_target;
        expect(old_target.district!=depot.district||old_target.u!=depot.u||old_target.v!=depot.v,
               "finished-job fixture begins with an actual non-depot client objective");
        if(success) {
            td.u=old_target.u*16;td.v=old_target.v*16;td.speed=0;
            td_interact();
        } else td_second();
        expect(td.mode==TD_RESULT&&td.job==TD_NONE,
               "actual delivery and deadline failure both retain the result screen after retiring the job");
        expect(td_target.district==depot.district&&td_target.u==depot.u&&td_target.v==depot.v&&
               !strcmp(td_target.name,depot.name),
               "delivery and failure immediately replace the old objective with the authored Union depot");
        expect(actors[1].pos.x==depot.u*32&&actors[1].pos.y==(depot.v-12)*32&&
               !(actors[1].flags&ACTOR_FLAG_HIDDEN)&&td_route_district==TD_DISTRICT_NONE,
               "retiring a core job restores the visible Union beacon rather than the former client or seam");
        /* The discarded job/stage remains in RAM; it must not select a client. */
        td.stage=11;td_job.route[11]=42;
        world_tick(J_B,120);
        expect(td.mode==TD_ROAM&&td.job==TD_NONE&&td.stage==11,
               "B leaves the real result handler for free roam without changing the retired stage");
        expect(td_target.district==depot.district&&td_target.u==depot.u&&td_target.v==depot.v&&
               actors[1].pos.x==depot.u*32&&actors[1].pos.y==(depot.v-12)*32,
               "free-roam objective and beacon remain Union despite stale active-job route data");
    }
}

/* Independently authored expectations for every boarding origin, including
   Wellesley's bus selector and the three separate Island return routes. */
static const struct {
    UBYTE origin,target,phase,period,fare,duration;
} transit_cases[]={
    {0,12,0,18,3,1},{12,17,2,18,3,3},{13,0,4,18,3,2},
    {14,0,6,18,3,2},{15,0,8,18,3,3},{16,0,10,18,3,3},{17,0,12,18,3,4},
    {18,19,0,24,2,6},{80,19,4,24,2,4},{19,18,8,24,2,6},
    {10,20,0,30,4,8},{20,10,7,30,4,8},{21,10,14,30,4,8},{22,10,21,30,4,8}
};

static void transit_menu_case(unsigned index,UWORD second) {
    native_case();td_stop_t origin;
    td_get_stop(transit_cases[index].origin&63,&origin);
    test_current_district=td.district=origin.district;
    td.onfoot=1;td.u=td.safe_u=origin.u*16;td.v=td.safe_v=origin.v*16;
    td.mode=TD_TRANSIT;td.transit_origin=transit_cases[index].origin;
    td.transit_target=transit_cases[index].target;
    td_get_stop(td.transit_target,&td_cursor);td.seconds=second;td.subsecond=37;
}

static void test_current_transit_window(void) {
    for(unsigned service=0;service<sizeof(transit_cases)/sizeof(transit_cases[0]);service++) {
        for(int edge=-1;edge<=2;edge++) {
            UWORD second=transit_cases[service].period*2+transit_cases[service].phase+edge;
            UBYTE open=edge==0||edge==1;
            transit_menu_case(service,second);
            expect(td_next_departure(td.transit_origin,td.seconds)==(open?0:edge==-1?1:transit_cases[service].period-2),
                   "countdown identifies both open seconds and their immediately adjacent closed edges");
            world_tick(J_A,1);
            expect(td.mode==(open?TD_RIDE:TD_WAIT),"confirmation boards in either open second and waits on either closed edge");
            expect(td.cash==(open?30-transit_cases[service].fare:30),"current-window confirmation charges the correct service fare exactly once");
            expect(td.seconds==second&&td.subsecond==37,"booking does not add a world second or consume contract time");
            expect(td.ride_left==(open?transit_cases[service].duration:0),"confirmation preserves the complete ride duration without consuming its first second");
            td_state_t booked=td;memset(&td,0,sizeof(td));
            expect(td_restore()&&memcmp(&td,&booked,sizeof(td))==0,"confirmation saves the correct paid ride or unpaid wait with native collision fixtures");
            world_tick(J_A,0);world_tick(0,0);world_tick(J_A,0);
            expect(td.cash==booked.cash&&td.ride_left==booked.ride_left,"held and repeated confirm presses cannot recharge or advance an accepted trip");
            if(!open) {
                UWORD vblanks=(edge==-1?1:transit_cases[service].period-2)*60-37;
                world_tick(0,vblanks);
                expect(td.mode==TD_RIDE&&td.cash==30-transit_cases[service].fare,
                       "closed-window wait boards once at the next genuine departure");
                expect(td.ride_left==transit_cases[service].duration,
                       "scheduled boarding also retains every ride second on the boarding tick");
                booked=td;
            }
            UWORD arrival_u=td_cursor.u*16+(td.transit_target==0?12*16:0),arrival_v=td_cursor.v*16;
            world_tick(0,transit_cases[service].duration*60);
            expect(td.mode==TD_ROAM&&td.u==arrival_u&&td.v==arrival_v&&td.cash==booked.cash,
                   "each accepted service reaches its selected destination with one fare and its authored duration");
        }
    }
    transit_menu_case(0,1);td.transit_target=td.transit_origin&63;sram_writes=0;
    world_tick(J_A,1);
    expect(td.mode==TD_TRANSIT&&td.cash==30&&sram_writes==0,"confirming the current stop cannot buy or save an empty trip");
}

static void test_transit_funds_pause_and_deadline(void) {
    const unsigned services[]={0,8,12};
    for(unsigned i=0;i<3;i++) {
        unsigned service=services[i];UWORD phase=transit_cases[service].phase;
        transit_menu_case(service,phase+1);td.cash=transit_cases[service].fare-1;
        world_tick(J_A,1);
        expect(td.mode==TD_ROAM&&td.msg==4&&td.cash==transit_cases[service].fare-1&&td.ride_left==0,
               "an open departure with insufficient funds returns to roaming without charging or boarding");
        memset(&td,0,sizeof(td));
        expect(td_restore()&&td.mode==TD_ROAM&&td.cash==transit_cases[service].fare-1,
               "rejected immediate boarding saves an unpaid roaming state");
        transit_menu_case(service,phase);td.cash=transit_cases[service].fare;
        world_tick(J_A,1);
        expect(td.mode==TD_RIDE&&td.cash==0,"the exact fare is sufficient for immediate boarding on each service");
        transit_menu_case(service,phase+2);td.cash=transit_cases[service].fare-1;
        world_tick(J_A,1);
        expect(td.mode==TD_WAIT&&td.msg==0&&td.cash==transit_cases[service].fare-1,
               "a closed window defers its funds check until the scheduled boarding opportunity");
        world_tick(0,(transit_cases[service].period-2)*60-37);
        expect(td.mode==TD_ROAM&&td.msg==4&&td.cash==transit_cases[service].fare-1,
               "an insufficient scheduled fare uses the same unpaid rejection as immediate boarding");
    }

    transit_menu_case(6,transit_cases[6].phase+1);td.job=0;td_get_job(0,&td_job);td.left=2;
    world_tick(J_A,1);
    expect(td.mode==TD_RIDE&&td.left==2&&td.ride_left==4&&td.cash==27,
           "last-window-second confirmation boards an active parcel without decrementing its deadline");
    UWORD second=td.seconds;world_tick(J_START,300);
    expect(td.mode==TD_PAUSE&&td_resume_mode==TD_RIDE&&td.seconds==second&&td.left==2&&td.ride_left==4,
           "pause on the boarding frame freezes the paid ride and parcel deadline");
    td.menu=1;world_tick(J_A,300);world_tick(0,300);
    expect(td.mode==TD_MAP&&td.seconds==second&&td.left==2&&td.ride_left==4,
           "map inspection preserves the paused immediate ride");
    world_tick(J_B,1);world_tick(0,1);world_tick(J_B,1);
    expect(td.mode==TD_RIDE&&td.cash==27&&td.ride_left==4,"leaving map and pause resumes the existing paid ride without another fare");
    world_tick(0,23);world_tick(0,60);
    expect(td.job==TD_NONE&&td.health==0&&td.mode==TD_RIDE&&td.ride_left==2&&td.cash==27,
           "a deadline expiring after immediate boarding fails the parcel but keeps its already-paid trip moving");
    UWORD arrival_u=(td_cursor.u+12)*16,arrival_v=td_cursor.v*16;world_tick(0,120);
    expect(td.mode==TD_RESULT&&td.u==arrival_u&&td.v==arrival_v&&td.cash==27,
           "an expired immediately-boarded parcel arrives once before showing the failure result");

    transit_menu_case(0,2);td.job=0;td_get_job(0,&td_job);td.left=1;world_tick(J_A,1);world_tick(0,23);
    expect(td.mode==TD_RESULT&&td.job==TD_NONE&&td.cash==30,
           "a parcel deadline expiring while the departure is closed fails without charging a fare");
}

static void test_immediate_transit_interrupted_save(void) {
    const unsigned services[]={1,8,12};
    for(unsigned i=0;i<3;i++) {
        unsigned service=services[i];transit_menu_case(service,transit_cases[service].phase+1);
        td.mode=TD_ROAM;td_save();td_state_t stable=td;
        UBYTE stable_slot=td_save_slot,stable_seq=td_save_seq;
        UBYTE stable_image[sizeof(td_test_sram)];memcpy(stable_image,td_test_sram,sizeof(stable_image));
        td.mode=TD_TRANSIT;td_state_t menu=td;sram_writes=0;world_tick(J_A,1);
        td_state_t paid=td;unsigned count=sram_writes;
        expect(paid.mode==TD_RIDE&&paid.cash==30-transit_cases[service].fare&&count==sizeof(td)+9,
               "immediate boarding commits its fare and full ride in one actual save record");
        for(volatile unsigned cut=1;cut<=count;cut++) {
            memcpy(td_test_sram,stable_image,sizeof(stable_image));td=menu;
            td_save_slot=stable_slot;td_save_seq=stable_seq;joy=joy_pressed=0;sys_time=td_last_frame=0;
            sram_writes=0;sram_interrupt_after=cut;sram_interrupt_enabled=1;
            if(setjmp(interrupted_save)==0){world_tick(J_A,1);expect(0,"boarding interruption must reach its configured real SRAM store");}
            sram_interrupt_enabled=0;memset(&td,0,sizeof(td));
            expect(td_restore(),"interruption during immediate boarding always retains a committed snapshot");
            expect(memcmp(&td,cut==count?&paid:&stable,sizeof(td))==0,
                   "uncommitted boarding restores an unpaid state while committed boarding restores the fare and full ride together");
        }
        td_session_live=0;actors_inactive_head=NULL;toronto_init();
        expect(td.mode==TD_HELP&&td.cash==paid.cash&&td.ride_left==paid.ride_left&&td_resume_mode==TD_RIDE,
               "cold recovery of committed immediate boarding retains the paid trip behind help");
        world_tick(0,1);world_tick(J_A,1);world_tick(0,transit_cases[service].duration*60);
        expect(td.mode==TD_ROAM&&td.cash==paid.cash&&td.u==td_fixture_stops[paid.transit_target].u*16&&td.v==td_fixture_stops[paid.transit_target].v*16,
               "recovered immediate boarding reaches the original destination without a second fare");
    }
}

static void old_word(UBYTE *old,unsigned offset,UWORD value) {
    old[offset]=value;old[offset+1]=value>>8;
}

/* Independent, fixed v5 disk layout: 48 bytes and exactly nine bitmap bytes.
   Do not memcpy the current58-byte structure into a fabricated legacy record. */
static void write_v5(UBYTE slot,const td_state_t *state,UBYTE sequence) {
    UBYTE old[48]={0};volatile UBYTE *record=td_save_address(slot);UWORD crc=0xFFFF;
    old_word(old,0,state->u);old_word(old,2,state->v);old_word(old,4,state->park_u);old_word(old,6,state->park_v);
    old_word(old,8,state->cash);old_word(old,10,state->seconds);old_word(old,12,state->left);old_word(old,14,state->speed);
    old[16]=state->heading;old[17]=state->vehicle;old[18]=state->onfoot;old[19]=state->mode;
    old[20]=state->menu;old[21]=state->job;old[22]=state->stage;old[23]=state->health;
    old[24]=state->done;old[25]=state->subsecond;memcpy(old+26,state->complete,9);
    old[35]=state->transit_origin;old[36]=state->transit_target;old[37]=state->ride_left;
    old[38]=state->cooldown;old[39]=state->msg;
    old_word(old,40,state->safe_u);old_word(old,42,state->safe_v);old_word(old,44,state->map_x);old_word(old,46,state->map_y);
    record[0]=0x54;record[1]=0xD7;record[2]=5;record[3]=48;record[4]=sequence;record[7]=0;
    for(unsigned i=2;i<=4;i++)crc=td_crc_byte(crc,record[i]);
    for(unsigned i=0;i<48;i++){record[8+i]=old[i];crc=td_crc_byte(crc,old[i]);}
    record[5]=crc;record[6]=crc>>8;
}

static td_state_t legacy_work(UBYTE mode) {
    td_state_t state=td;
    state.cash=537;state.seconds=113;state.left=135;state.speed=2;state.heading=5;
    state.job=4;state.stage=3;state.health=89;state.done=4;state.subsecond=29;
    state.complete[0]=7;state.complete[8]=128;state.mode=mode;state.menu=4;
    state.transit_origin=0;state.transit_target=17;state.ride_left=3;state.cooldown=9;state.msg=12;
    state.map_x=1400;state.map_y=2200;state.onfoot=mode!=TD_ROAM;
    return state;
}

static void test_v5_migration_and_interrupted_upgrade(void) {
    for(unsigned mode=0;mode<3;mode++) {
        native_case();td_state_t legacy=legacy_work(mode==0?TD_ROAM:mode==1?TD_WAIT:TD_RIDE);
        write_v5(0,&legacy,37);memset(&td,0,sizeof(td));
        expect(td_restore(),"v5 CRC record restores with actual core collision and authored contract");
        expect(memcmp(&td,&legacy,sizeof(td))==0,"v5 to current migration preserves active job, cash, clock, bitmap, transit and map fields");
        int zero=td.district==0&&td.park_district==0&&td.reserved==0;
        for(unsigned i=9;i<TD_COMPLETE_BYTES;i++)if(td.complete[i])zero=0;
        expect(zero,"migration extends the bitmap with zero bytes and supplies core districts");
        expect(td_save_address(0)[2]==5,"reading v5 alone does not overwrite its committed snapshot");
        td_save();
        expect(td_save_address(td_save_slot)[2]==TD_SAVE_VERSION&&td_save_address(td_save_slot)[3]==58,
               "first save after migration writes the actual58-byte current record to the other slot");
        memset(&td,0,sizeof(td));expect(td_restore()&&memcmp(&td,&legacy,sizeof(td))==0,
               "upgraded current record restores all preserved legacy fields");
    }
    native_case();td_state_t stable=legacy_work(TD_RIDE);write_v5(0,&stable,88);
    expect(td_restore(),"interruption fixture starts from a genuine decoded v5 snapshot");
    UBYTE stable_image[sizeof(td_test_sram)];memcpy(stable_image,td_test_sram,sizeof(stable_image));
    td_state_t candidate=stable;candidate.cash+=111;candidate.complete[9]=1;candidate.done++;
    candidate.stage=4;candidate.left=119;candidate.ride_left=2;td=candidate;sram_writes=0;td_save();
    unsigned count=sram_writes;
    expect(count==sizeof(td)+9,"upgrade trace counts every real current payload and metadata store");
    for(volatile unsigned cut=1;cut<=count;cut++) {
        memcpy(td_test_sram,stable_image,sizeof(stable_image));
        expect(td_restore()&&memcmp(&td,&stable,sizeof(td))==0,"each upgrade interruption begins from retained v5 state");
        td=candidate;sram_writes=0;sram_interrupt_after=cut;sram_interrupt_enabled=1;
        if(setjmp(interrupted_save)==0){td_save();expect(0,"upgrade interruption must reach the configured real byte store");}
        sram_interrupt_enabled=0;memset(&td,0,sizeof(td));
        expect(td_restore(),"every interrupted v5 upgrade retains a committed recoverable record");
        expect(memcmp(&td,cut==count?&candidate:&stable,sizeof(td))==0,
               "v5 upgrade preserves complete old state until the final current commit byte");
    }
}

static void test_district_semantic_fallback(void) {
    native_case();td.cash=111;td_save();td.cash=222;td_save();
    UBYTE newest=td_save_slot,image[sizeof(td_test_sram)];memcpy(image,td_test_sram,sizeof(image));
    expect(td_district_drivable(0,560,720)&&!td_district_drivable(1,560,720),
           "semantic fixture uses a real core road that is blocked in the requested western district");
    for(unsigned fault=0;fault<7;fault++) {
        memcpy(td_test_sram,image,sizeof(image));volatile UBYTE *record=td_save_address(newest);
        td_state_t bad;memcpy(&bad,(const void *)(record+8),sizeof(bad));
        switch(fault) {
            case 0:bad.district=TD_DISTRICT_COUNT;break;
            case 1:bad.park_district=TD_DISTRICT_COUNT;break;
            case 2:bad.district=1;break;
            case 3:bad.park_district=1;break;
            case 4:
#if TD_QUESTS < TD_COMPLETE_BYTES * 8
                bad.complete[TD_QUESTS>>3]=1<<(TD_QUESTS&7);bad.done=1;
#else
                /* A full bitmap has no unused bit; retain count validation. */
                bad.complete[0]=1;bad.done=0;
#endif
                break;
            case 5:bad.reserved=1;break;
            case 6:
                bad.district=1;bad.u=800*16;bad.v=64*16;bad.onfoot=1;
                bad.mode=TD_RIDE;bad.transit_origin=0;bad.transit_target=17;bad.ride_left=3;break;
        }
        memcpy((void *)(record+8),&bad,sizeof(bad));refresh_record_crc(record);
        char name[120];snprintf(name,sizeof(name),"CRC-valid district/unused-bitmap semantic fault%u falls back to the older snapshot",fault);
        expect(td_restore()&&td.cash==111&&td.district==0&&td.park_district==0,name);
    }
}

static const UBYTE test_player_sprite=1,test_tram_sprite=2;
static void load_authored_scene_fixture(void) {
    /* Model load_scene's actual authored order and preloaded sheet binding.
       The real resources have one scriptless inactive loader in Queen0/1/3. */
    memset(actors,0,sizeof(actors));actors_inactive_head=test_actors_active_head=NULL;
    PLAYER.sprite.bank=1;PLAYER.sprite.ptr=&test_player_sprite;PLAYER.base_tile=4;
    actors_len=1;
    if(test_current_district==0||test_current_district==1||test_current_district==3){
        actors_len=2;actors[1].sprite.bank=2;actors[1].sprite.ptr=&test_tram_sprite;
        actors[1].base_tile=96;actors[1].frame=0;actors[1].anim_tick=255;
        actors_inactive_head=&actors[1];
    }
}
static void apply_queued_scene(void) {
    expect(test_queued_district<TD_DISTRICT_COUNT,"scene fixture applies a genuinely queued district");
    test_current_district=test_queued_district;test_queued_district=TD_DISTRICT_NONE;
    load_authored_scene_fixture();toronto_init();
}

static void test_safe_transit_alighting(void) {
    /* Bloor->Union reproduces the real return onto the parked car. Add a
       second obstruction case to force the opposite safe walking side. */
    for(UBYTE traffic=0;traffic<2;traffic++) {
        transit_menu_case(6,12);td.subsecond=0;
        td_stop_t depot;td_get_stop(0,&depot);
        if(traffic) {
            td.park_u=640*16;td.park_v=640*16;
            td_traffic_u[0]=560*16;td_traffic_v[0]=720*16;
            td_traffic_u[1]=572*16;td_traffic_v[1]=720*16;
        }
        world_tick(J_A,1);world_tick(0,240);
        UWORD expected_u=(traffic?548:572)*16;
        expect(td.mode==TD_ROAM&&td.onfoot&&td.cash==27&&td.u==expected_u&&td.v==720*16&&
               td.safe_u==td.u&&td.safe_v==td.v&&td_foot_free(td.u,td.v)&&td_near(&depot),
               "Union returns alight beside the actual parked car or loaded traffic at a clear reachable stop-side point");
        td_state_t arrived=td;memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&arrived,sizeof(td)),
               "safe Union alighting commits its actual clear foot position and single paid fare");
        world_tick(traffic?J_LEFT:J_RIGHT,1);
        expect(td.u==expected_u+(traffic?-8:8)&&td.v==720*16&&td.onfoot&&td.cash==27,
               "the first ordinary walking step escapes the alighting obstacle instead of trapping the courier inside it");
    }

    /* Four close traffic centres reject all12px points but leave the18px
       fallback usable. The courier can walk back into handoff range. */
    transit_menu_case(6,12);td.mode=TD_RIDE;td.cash=27;td.ride_left=1;
    td_traffic_u[0]=563*16;td_traffic_v[0]=720*16;
    td_traffic_u[1]=557*16;td_traffic_v[1]=720*16;
    td_traffic_u[2]=560*16;td_traffic_v[2]=723*16;
    td_traffic_u[3]=560*16;td_traffic_v[3]=717*16;
    td_second();
    expect(td.mode==TD_ROAM&&td.u==578*16&&td.v==720*16&&td_foot_free(td.u,td.v),
           "blocked12px landings fall back to an actual clear18px position rather than the vehicle centre");
    for(unsigned i=0;i<8;i++)driving_tick(J_LEFT);
    td_stop_t depot;td_get_stop(0,&depot);
    expect(td.u==574*16&&td_near(&depot)&&td_foot_free(td.u,td.v),
           "the18px fallback can approach the real stop into interaction range without entering the parked-car or traffic boxes");

    /* A temporary crowd covers the centre and every12/18px endpoint. The
       paid trip remains saved at Bloor while its parcel genuinely expires. */
    transit_menu_case(6,12);td.mode=TD_RIDE;td.cash=27;td.ride_left=1;
    td.park_u=640*16;td.park_v=640*16;
    td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=2;td_set_target();
    const WORD blocked_u[5]={560,578,542,560,560},blocked_v[5]={720,720,720,738,702};
    for(unsigned i=0;i<5;i++){td_traffic_u[i]=blocked_u[i]*16;td_traffic_v[i]=blocked_v[i]*16;}
    UWORD origin_u=td.u,origin_v=td.v;td_second();
    expect(td.mode==TD_RIDE&&td.ride_left==1&&td.cash==27&&td.u==origin_u&&td.v==origin_v&&
           td.job==0&&td.left==1&&!td_transition_pending,
           "all blocked landings retain the paid origin ride for retry without moving the courier or charging again");
    td_state_t retry=td;memset(&td,0,sizeof(td));
    expect(td_restore()&&!memcmp(&td,&retry,sizeof(td)),
           "a fully blocked alighting attempt remains a valid recoverable paid origin save");
    td_second();
    expect(td.mode==TD_RIDE&&td.ride_left==1&&td.cash==27&&td.job==TD_NONE&&td.health==0&&
           td.u==origin_u&&td.v==origin_v&&td_target.district==0&&td_target.u==560&&td_target.v==720,
           "deadline expiry while alighting is obstructed clears the job and objective while preserving the paid retry");
    retry=td;memset(&td,0,sizeof(td));
    expect(td_restore()&&!memcmp(&td,&retry,sizeof(td)),
           "the expired paid retry restores with failure condition instead of losing its ride or charging another fare");
    for(unsigned i=0;i<6;i++){td_traffic_u[i]=30000;td_traffic_v[i]=30000;}
    td_second();
    expect(td.mode==TD_RESULT&&td.job==TD_NONE&&td.health==0&&td.cash==27&&td.u==560*16&&td.v==720*16&&
           td.safe_u==td.u&&td.safe_v==td.v&&td_near(&depot),
           "removing the obstruction completes the expired paid journey once at its clear destination before showing failure");
    td_state_t failed=td;memset(&td,0,sizeof(td));
    expect(td_restore()&&td.mode==TD_ROAM&&td.job==TD_NONE&&td.health==0&&td.cash==27&&
           td.u==failed.u&&td.v==failed.v,
           "a failed but safely alighted destination remains a valid persistent free-roam state");
    td=failed;world_tick(J_B,1);world_tick(J_RIGHT,1);
    expect(td.mode==TD_ROAM&&td.u==560*16+8&&td.v==720*16&&td.cash==27,
           "dismissed expired-trip results allow the first walking step from the clear destination");

    /* Keep the registered grid except one synthetic solid tile between
       Union and the east18px endpoint. Clear endpoints cannot bypass it. */
    transit_menu_case(6,12);geometry=ALIGHT_BARRIER;td.mode=TD_RIDE;td.cash=27;td.ride_left=1;
    td_traffic_u[0]=542*16;td_traffic_v[0]=720*16;
    td_traffic_u[1]=560*16;td_traffic_v[1]=738*16;
    td_traffic_u[2]=560*16;td_traffic_v[2]=702*16;
    expect(td_district_walkable(0,578,720)&&!td_district_walkable(0,572,720),
           "alighting barrier fixture has a clear distant endpoint with a real intervening collision tile");
    td_second();
    expect(td.mode==TD_RIDE&&td.ride_left==1&&td.cash==27&&td.u!=578*16,
           "alighting rejects a clear18px endpoint when its swept walking path crosses solid collision");
    geometry=NATIVE_GRID;td_second();
    expect(td.mode==TD_ROAM&&td.u==572*16&&td.v==720*16&&td.cash==27,
           "removing the collision barrier lets the same paid trip retry its nearest safe landing");

    /* Synthetic left-boundary stop: both west offsets underflow, while
       traffic covers every in-bounds cardinal point. No wrapped landing. */
    reset_case();td.onfoot=1;td.mode=TD_RIDE;td.cash=27;td.ride_left=1;
    td.transit_origin=0;td.transit_target=12;td_cursor.u=8;td_cursor.v=450;
    td.park_u=8*16;td.park_v=450*16;
    td_traffic_u[0]=26*16;td_traffic_v[0]=450*16;
    td_traffic_u[1]=8*16;td_traffic_v[1]=468*16;
    td_traffic_u[2]=8*16;td_traffic_v[2]=432*16;
    td_second();
    expect(td.mode==TD_RIDE&&td.ride_left==1&&td.u==400*16&&td.v==450*16&&td.cash==27,
           "blocked boundary alighting retries instead of wrapping negative cardinal offsets across the map");
    retry=td;memset(&td,0,sizeof(td));
    expect(td_restore()&&!memcmp(&td,&retry,sizeof(td)),
           "the boundary retry retains a semantically valid paid origin save");
    td_traffic_u[0]=td_traffic_v[0]=30000;td_second();
    expect(td.mode==TD_ROAM&&td.u==20*16&&td.v==450*16&&td.cash==27&&td_foot_free(td.u,td.v),
           "a boundary retry alights at the newly clear in-bounds stop-side point without another fare");

    /* A remote Queen platform ignores the old scene's vehicle cache but
       still respects a genuinely parked vehicle in its destination scene. */
    for(UBYTE parked_remote=0;parked_remote<2;parked_remote++) {
        native_case();td_stop_t origin,destination;td_get_stop(45,&origin);td_get_stop(50,&destination);
        td_session_live=1;test_current_district=td.district=origin.district;td.onfoot=1;
        td.u=td.safe_u=origin.u*16;td.v=td.safe_v=origin.v*16;
        td.mode=TD_RIDE;td.cash=27;td.ride_left=1;td.transit_origin=45;td.transit_target=50;td_cursor=destination;
        td_traffic_u[0]=destination.u*16;td_traffic_v[0]=destination.v*16;
        if(parked_remote){td.park_district=3;td.park_u=880*16;td.park_v=514*16;}
        expect(td_district_drivable(td.park_district,td.park_u>>4,td.park_v>>4),
               "remote parked-car alighting fixture uses an actual legal road footprint beside the Queen sidewalk");
        td_second();UWORD expected_u=(880+(parked_remote?12:0))*16;
        expect(td_transition_pending==1&&td.mode==TD_ROAM&&td.district==3&&td.u==expected_u&&td.v==524*16&&td.cash==27,
               "remote Queen alighting ignores origin traffic coordinates but chooses a safe side beside its remote parked car");
        td_state_t arrived=td;memset(&td,0,sizeof(td));
        expect(td_restore()&&td.mode==TD_ROAM&&td.district==3&&td.u==expected_u&&td.v==524*16&&td.cash==27,
               "remote Queen safe landing saves the actual destination point rather than a paid ride at the wrong origin");
        td=arrived;apply_queued_scene();
        expect(td_near(&destination)&&td_foot_free(td.u,td.v),
               "the loaded remote Queen landing remains close to its real platform and outside loaded traffic or the parked car");
        world_tick(J_RIGHT,1);
        expect(td.u==expected_u+8&&td.v==524*16&&td.onfoot&&td.cash==27,
               "the first ordinary walking step leaves either remote Queen landing freely");
    }
}

/* Real registered stops, independent Queen timetable expectations, and the
   production driver/save code. Map rendering itself is covered by test_atlas. */
static void test_cross_district_streetcar(void) {
    for(UBYTE index=0;index<8;index++)for(UBYTE scenario=0;scenario<3;scenario++) {
        UBYTE origin_id=43+index,target_id=origin_id<48?50:43;
        UBYTE phase=target_id>origin_id?index*4:32+(7-index)*4;
        UBYTE duration=(target_id>origin_id?target_id-origin_id:origin_id-target_id)*4;
        UBYTE waiting=scenario==1,failing=scenario==2;
        td_stop_t origin,destination,depot;
        native_case();td_get_stop(origin_id,&origin);td_get_stop(target_id,&destination);td_get_stop(0,&depot);
        test_current_district=td.district=origin.district;td_session_live=1;td.onfoot=1;
        td.u=td.safe_u=origin.u*16;td.v=td.safe_v=origin.v*16;
        td.mode=TD_TRANSIT;td.transit_origin=origin_id;td.transit_target=target_id;
        td_cursor=destination;td.seconds=64*2+phase+(waiting?2:0);td.subsecond=13;
        if(failing){td.job=0;td_get_job(td.job,&td_job);td.stage=1;td.left=2;td_set_target();}
        expect(origin.transit==4&&destination.transit==4&&origin.district!=destination.district&&
               td_district_walkable(origin.district,origin.u,origin.v)&&
               td_district_walkable(destination.district,destination.u,destination.v),
               "all eight Queen boarding fixtures use actual walkable stops across registered districts");
        world_tick(J_A,1);
        expect(td.mode==(waiting?TD_WAIT:TD_RIDE)&&td.cash==(waiting?30:27)&&
               td.ride_left==(waiting?0:duration),
               "Queen confirmation waits when closed or immediately pays one fare for the full cross-district journey");
        td_state_t booked=td;
        UWORD second=td.seconds;UBYTE subsecond=td.subsecond,ride_left=td.ride_left;
        world_tick(J_START,300);td.menu=1;world_tick(J_A,300);
        td_state_t frozen=td;world_tick(J_RIGHT,300);world_tick(0,300);
        expect(td.mode==TD_MAP&&!memcmp(&td,&frozen,sizeof(td))&&td.seconds==second&&
               td.subsecond==subsecond&&td.ride_left==ride_left,
               "Queen unpaid waits and paid rides freeze all persistent state while browsing the paused atlas");
        world_tick(J_B,300);world_tick(0,0);world_tick(J_B,300);world_tick(0,0);
        expect(td.mode==(waiting?TD_WAIT:TD_RIDE)&&td.seconds==second&&td.subsecond==subsecond&&
               td.cash==(waiting?30:27)&&td.ride_left==ride_left,
               "closing Queen map and pause resumes the same origin trip without time advancement or another fare");
        if(waiting)world_tick(0,62*60-td.subsecond);
        expect(td.mode==TD_RIDE&&td.cash==27&&td.ride_left==duration&&td.district==origin.district,
               "scheduled Queen boarding commits one fare while keeping the paid rider in the origin district");
        /* Atlas navigation changes the unsaved menu selection. The paid
           record belongs to immediate confirmation or later WAIT boarding. */
        td_state_t paid=waiting?td:booked;memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&paid,sizeof(td)),
               "native semantic validation accepts every Queen paid ride at its actual origin and remote target");
        td_session_live=0;actors_inactive_head=NULL;toronto_init();
        expect(td.mode==TD_HELP&&td_resume_mode==TD_RIDE&&td.district==origin.district&&
               td.cash==27&&td.ride_left==duration&&td.seconds==paid.seconds&&
               td.u==origin.u*16&&td.v==origin.v*16&&!test_queue_calls&&
               td_cursor.district==destination.district&&td_cursor.u==destination.u&&td_cursor.v==destination.v,
               "cold Queen recovery preserves its paid origin state and restores the remote alighting stop behind help");
        world_tick(J_A,1);world_tick(0,0);
        expect(td.mode==TD_RIDE&&td.cash==27&&td.ride_left==duration,
               "leaving recovered Queen help resumes the paid ride without charging again");
        if(failing) {
            world_tick(0,120-td.subsecond);
            expect(td.mode==TD_RIDE&&td.job==TD_NONE&&td.health==0&&td.cash==27&&
                   td.ride_left==duration-2&&td.district==origin.district,
                   "an actual parcel deadline expiring during Queen travel retires the job but continues the paid origin ride");
            expect(td_target.district==depot.district&&td_target.u==depot.u&&td_target.v==depot.v,
                   "Queen deadline expiry immediately clears the former client objective to Union");
        }
        test_queue_fail=1;
        /* This case isolates final arrival queue failure. Intermediate
           presentation loading has separate real-update fixtures below. */
        UBYTE remaining=td.ride_left;td.subsecond=0;
        while(remaining--)td_second();
        expect(td.mode==TD_RIDE&&td.ride_left==1&&td.cash==27&&td.district==origin.district&&
               td.u==origin.u*16&&td.v==origin.v*16&&!td_transition_pending&&test_queue_calls==1&&
               test_queued_district==TD_DISTRICT_NONE,
               "failed Queen scene allocation retains the paid rider at the origin with one retry second");
        td_state_t retry=td;memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&retry,sizeof(td)),
               "failed Queen arrival commits a recoverable origin ride rather than an invalid remote paid state");
        actor_t old_actors[21];UWORD old_traffic_u[6],old_traffic_v[6];
        memcpy(old_actors,actors,sizeof(actors));memcpy(old_traffic_u,td_traffic_u,sizeof(td_traffic_u));
        memcpy(old_traffic_v,td_traffic_v,sizeof(td_traffic_v));
        UBYTE old_tick=td_tick;UWORD old_second=td.seconds;
        test_queue_fail=0;world_tick(J_RIGHT,180);
        expect(td_transition_pending==1&&test_queue_calls==2&&test_queued_district==destination.district&&
               td.district==destination.district&&td.u==destination.u*16&&td.v==destination.v*16&&
               td.safe_u==td.u&&td.safe_v==td.v&&td.mode==(failing?TD_RESULT:TD_ROAM)&&td.cash==27,
               "Queen arrival retries the queue once and commits the selected destination with success or failure result");
        expect(td.seconds==old_second+1&&td_tick==old_tick&&!memcmp(&actors[0],&old_actors[0],sizeof(actor_t))&&
               !memcmp(&actors[2],&old_actors[2],sizeof(actors)-2*sizeof(actor_t))&&
               !memcmp(td_traffic_u,old_traffic_u,sizeof(td_traffic_u))&&
               !memcmp(td_traffic_v,old_traffic_v,sizeof(td_traffic_v)),
               "queued Queen arrival stops subsequent clock and motion catchup before moving old-scene road users; only the objective cue may change");
        td_state_t arrived=td;memset(&td,0,sizeof(td));
        expect(td_restore()&&td.mode==TD_ROAM&&td.district==destination.district&&td.cash==27&&
               td.u==arrived.u&&td.v==arrived.v&&td.health==arrived.health&&td.job==arrived.job,
               "alighted Queen saves validate at the actual destination, including a retired failed parcel");
        td=arrived;apply_queued_scene();
        expect(!td_transition_pending&&test_current_district==destination.district&&
               td.mode==(failing?TD_RESULT:TD_ROAM)&&td.cash==27&&td.onfoot&&
               PLAYER.pos.x==destination.u*32&&PLAYER.pos.y==destination.v*32,
               "loaded Queen destination presents the courier on foot and retains its intended arrival mode");
        expect(td.park_district==0&&td.park_u==560*16&&td.park_v==720*16,
               "cross-district Queen travel never teleports the car parked at Union");
    }
}

static void test_reciprocal_portals(void) {
    for(unsigned index=0;index<TD_PORTALS;index++)for(unsigned foot=0;foot<2;foot++) {
        const td_portal_t *portal=&td_portals[index];native_case();
        test_current_district=td.district=portal->from;td_session_live=1;td.onfoot=foot;
        td.u=portal->u*16;td.v=portal->v*16;
        td.park_district=foot?(portal->from?0:2):portal->from;
        td.park_u=foot?(td.park_district?736:560)*16:td.u;
        td.park_v=foot?(td.park_district?640:720)*16:td.v;
        td.job=72;td.stage=1;td.left=170;td.done=3;td.complete[0]=7;td.health=93;
        UBYTE horizontal=portal->u==24||portal->u==1000,negative=(horizontal?portal->u:portal->v)==24;
        td.cash=333;td.seconds=137;td.subsecond=21;td.heading=horizontal?(negative?8:0):(negative?12:4);
        td.speed=foot?0:11;td_vx=horizontal?(negative?-144:144):32;td_vy=horizontal?32:(negative?-144:144);
        td_get_job(td.job,&td_job);td_set_target();
        UWORD old_u=td.u+(horizontal?(negative?16:-16):0),old_v=td.v+(horizontal?0:(negative?16:-16));td_state_t before=td;
        WORD old_vx=td_vx,old_vy=td_vy;audio_mode=TD_AUDIO_EFFECTS;
        UBYTE crossed=td_cross_portal(old_u,old_v);
        if(!foot&&!portal->vehicle) {
            expect(!crossed&&!test_queue_calls&&!memcmp(&td,&before,sizeof(td)),
                   "vehicle cannot cross a foot-only portal or alter campaign state");continue;
        }
        expect(crossed&&test_queue_calls==1&&test_queued_district==portal->to,
               "every reciprocal permitted portal queues its correct next district");
        expect(td.district==portal->to&&td.u==portal->arrival_u*16&&td.v==portal->arrival_v*16,
               "portal arrival uses its actual matched district and lane coordinates");
        expect(td.job==before.job&&td.stage==before.stage&&td.left==before.left&&td.cash==before.cash&&td.done==before.done&&td.health==before.health&&
               td.seconds==before.seconds&&td.subsecond==before.subsecond,
               "district crossing preserves job, unique completion, condition, earnings and global clock");
        expect(td.speed==before.speed&&td.heading==before.heading&&td_vx==old_vx&&td_vy==old_vy,
               "queued district crossing preserves heading, speed and both velocity axes");
        expect(foot?(td.park_district==before.park_district&&td.park_u==before.park_u&&td.park_v==before.park_v):
                    (td.park_district==portal->to&&td.park_u==td.u&&td.park_v==td.v),
               "foot crossing retains remote parked car while driving crossing carries it to the arrival district");
        apply_queued_scene();
        expect(!td_transition_pending&&td.mode==TD_ROAM&&td.job==72&&td.seconds==137&&td.left==170,
               "loaded reciprocal scene resumes the same world state without boot help or campaign reset");
        expect(td_vx==old_vx&&td_vy==old_vy&&td.speed==before.speed&&audio_mode==TD_AUDIO_EFFECTS&&audio_inits==0,
               "ordinary scene initialization retains momentum and current audio preference");
        expect(PLAYER.pos.x==(td.u>>4)*32&&PLAYER.pos.y==(td.v>>4)*32&&!(PLAYER.flags&ACTOR_FLAG_HIDDEN),
               "first loaded frame places the visible courier at its arrival rather than the old scene position");
        expect(!!(actors[8].flags&ACTOR_FLAG_HIDDEN)==!(td.onfoot&&td.park_district==td.district),
               "first loaded frame does not draw a car parked in another district");
    }
}

static void test_queue_failure_and_remote_boot(void) {
    native_case();td.u=24*16;td.v=64*16;td.park_u=td.u;td.park_v=td.v;
    td.job=72;td.stage=1;td.left=170;td.done=3;td.complete[0]=7;td_get_job(72,&td_job);
    td.speed=11;td_vx=-144;td_vy=32;td_state_t before=td;
    test_queue_fail=1;
    expect(!td_cross_portal(25*16,td.v)&&!memcmp(&td,&before,sizeof(td))&&!td_transition_pending&&sram_writes==0,
           "failed scene queue leaves district, vehicle, job and persistent state unchanged");
    expect(td_vx==-144&&td_vy==32,"failed allocation does not drain either velocity axis");
    test_queue_fail=0;
    expect(td_cross_portal(25*16,td.v)&&td.district==1&&test_queue_calls==2,
           "the same valid portal can retry after queue allocation recovers");

    native_case();td.district=1;td.u=800*16;td.v=64*16;td.safe_u=td.u;td.safe_v=td.v;
    td.onfoot=1;td.job=72;td.stage=1;td.left=170;td.cash=444;td.done=3;td.complete[0]=7;
    td.seconds=81;td.subsecond=13;td_save();memset(&td,0,sizeof(td));
    test_queue_fail=1;audio_mode=TD_AUDIO_SILENT;toronto_init();
    expect(td_session_live&&td.district==1&&test_current_district==0&&td_transition_pending==2&&td.mode==TD_HELP,
           "remote saved boot retains the restored district while a failed redirect waits for retry");
    expect(PLAYER.flags&ACTOR_FLAG_HIDDEN,"boot redirect hides the courier on the incorrect initial scene");
    expect(audio_mode==TD_AUDIO_FULL&&audio_inits==1&&test_reset_calls==1,
           "cold remote boot resets session bridge and audio preference exactly once");
    td_state_t restored=td;memset(td_test_sram,0,sizeof(td_test_sram));
    sys_time=120;toronto_update();
    expect(td_transition_pending==2&&!memcmp(&td,&restored,sizeof(td)),
           "pending boot redirect neither advances clocks nor rereads an invalidated save");
    test_queue_fail=0;toronto_update();
    expect(td_transition_pending==1&&test_queued_district==1,"pending boot redirect retries through the scene queue");
    audio_mode=TD_AUDIO_EFFECTS;apply_queued_scene();
    expect(td.cash==444&&td.job==72&&td.stage==1&&td.left==170&&td.seconds==81&&td.subsecond==13&&td.mode==TD_HELP,
           "remote scene entry keeps the first restored work state after its old SRAM is removed");
    expect(audio_inits==1&&audio_mode==TD_AUDIO_EFFECTS&&test_reset_calls==1,
           "remote scene entry does not repeat cold restore, bridge reset or audio initialization");
}

static void test_car_entry_at_portal(void) {
    native_case();td_session_live=1;td.onfoot=1;
    td.u=25*16;td.v=640*16;td.park_u=24*16;td.park_v=td.v;
    td.safe_u=td.u;td.safe_v=td.v;td.heading=8;
    td_entry_timer=1;td_entry_target=0;
    expect(td_door_path(td.u,td.v,td.park_u,td.park_v)&&td_drivable(24,640),
           "last-entry seam fixture uses a genuine reachable car on the registered King road port");
    world_tick(0,1);
    expect(!td.onfoot&&!td_entry_timer&&td.u==24*16&&td.v==640*16&&td.district==0&&
           !test_queue_calls&&!td_transition_pending,
           "finishing car entry at a seam cannot mistake the animation snap for outbound driving");
    for(unsigned i=0;i<30&&!td_transition_pending;i++)world_tick(J_A,1);
    expect(td_transition_pending&&td.district==1&&test_queue_calls==1&&test_queued_district==1&&
           td.park_district==1&&td.park_u==td.u&&td.park_v==td.v,
           "intentional outward acceleration after completed seam entry still crosses and carries the car once");
}

static void test_first_frame_actors(void) {
    const UWORD locations[TD_DISTRICT_COUNT][2]={{560,720},{800,64},{736,640},{224,528}};
    for(unsigned district=0;district<TD_DISTRICT_COUNT;district++) {
        native_case();td_session_live=1;test_current_district=td.district=td.park_district=district;
        td.u=(locations[district][0]+18)*16;td.v=locations[district][1]*16;
        td.park_u=locations[district][0]*16;td.park_v=td.v;td.onfoot=1;toronto_init();
        expect(actors_len==TD_ACTORS&&PLAYER.pos.x==(td.u>>4)*32&&PLAYER.pos.y==(td.v>>4)*32,
               "each registered district initializes all actors and courier coordinates before its first update");
        expect(actors[8].pos.x==(td.park_u>>4)*32&&actors[8].pos.y==(td.park_v>>4)*32&&!(actors[8].flags&ACTOR_FLAG_HIDDEN),
               "first scene frame shows the locally parked vehicle at its real saved position");
        int traffic=1;unsigned visible=0;
        for(unsigned i=0;i<6;i++) {
            if(actors[i+2].pos.x!=(td_traffic_u[i]>>4)*32||actors[i+2].pos.y!=(td_traffic_v[i]>>4)*32||
               !td_drivable(td_traffic_u[i]>>4,td_traffic_v[i]>>4))traffic=0;
            if(!(actors[i+9].flags&ACTOR_FLAG_HIDDEN)) {
                visible++;
                expect(td_ped_route[i]<td_route_counts[td.district]&&td_walkable(actors[i+9].pos.x/32,actors[i+9].pos.y/32),
                       "first-frame visible pedestrian has a valid identity on actual scene walking terrain");
            }
        }
        expect(traffic,"first scene frame places every traffic actor on its actual initialized clear road footprint");
        expect(visible>0,"each selected neighborhood starts with visible native pedestrians");
    }
}

static void test_walk_pace_dispatch_and_foot_delivery(void) {
    reset_case();td.onfoot=1;UWORD start=td.u;
    for(unsigned i=0;i<60;i++)driving_tick(J_RIGHT);
    unsigned cardinal=td.u-start;
    reset_case();td.onfoot=1;UWORD start_u=td.u,start_v=td.v;
    for(unsigned i=0;i<60;i++)driving_tick(J_RIGHT|J_DOWN);
    unsigned diagonal_u=td.u-start_u,diagonal_v=td.v-start_v;
    expect(diagonal_u*diagonal_u+diagonal_v*diagonal_v<=cardinal*cardinal&&diagonal_u==diagonal_v,
           "diagonal walking over an equal interval is symmetric and no faster than cardinal walking");
    for(unsigned parity=0;parity<2;parity++) {
        reset_case();td.onfoot=1;td_tick=parity;start_u=td.u;start_v=td.v;driving_tick(J_RIGHT|J_DOWN);
        diagonal_u=td.u-start_u;diagonal_v=td.v-start_v;
        expect(diagonal_u*diagonal_u+diagonal_v*diagonal_v<=8*8,"each alternating diagonal step stays below cardinal pace");
    }
    native_case();td.done=3;td.complete[0]=7;td_ready_offer();
    expect(td.menu==4&&td_offer.vehicle==TD_NONE,"dispatch after three car completions skips truck-only job04 for compatible relay05");
    td.vehicle=1;td_ready_offer();expect(td.menu==3&&td_offer.vehicle==1,"truck dispatch still offers the first unlocked truck contract");
    td.onfoot=1;td_ready_offer();expect(td.menu==4&&td_offer.vehicle==TD_NONE,"walking dispatch skips vehicle-required offers without hiding compatible packages");

    native_case();test_current_district=td.district=2;td.job=77;td_get_job(td.job,&td_job);td.stage=2;td.left=180;
    td_set_target();td.u=736*16;td.v=640*16;td.park_district=2;td.park_u=td.u;td.park_v=td.v;
    expect(td_target.reserved&TD_STOP_FOOT,"authored Lodge fixture carries its actual native foot-only delivery flag");
    expect(td_drivable(736,640)&&td_near(&td_target),"Lodge approach fixture is a genuine driveable parking point inside the interaction radius");
    td_interact();expect(td.stage==2&&td.job==77&&td.msg==16,"car-door proximity cannot hand off a flagged park-and-walk parcel");
    td.onfoot=1;td.u=784*16;td.v=608*16;td_set_target();td_interact();
    expect(td.stage==3&&td.job==77,"on-foot Lodge courier can complete the same flagged handoff");
    td.stage=2;td_set_target();td.district=1;td.u=784*16;td.v=608*16;td_interact();
    expect(td.stage==2&&td.msg==6,"equal local coordinates in the wrong district cannot complete the Lodge objective");
}

static void test_park_delivery_guidance(void) {
    /* Expected road/client coordinates come from independent registered-grid fixtures.
       The lookup itself runs unchanged generated native content, not a mock. */
    const UBYTE stops[3]={34,36,41},jobs[3]={77,84,85},stages[3]={2,3,2};
    const UWORD anchors[3][2]={{736,640},{224,144},{816,312}};
    for(unsigned i=0;i<3;i++) {
        native_case();td.job=jobs[i];td_get_job(td.job,&td_job);td.stage=stages[i];
        td_stop_t client;td_get_stop(stops[i],&client);
        expect(td_job.route[td.stage]==stops[i]&&(client.reserved&TD_STOP_FOOT),
               "parking guidance fixture is an authored foot-only handoff in its actual contract");
        test_current_district=td.district=td.park_district=client.district;
        td.u=td.park_u=anchors[i][0]*16;td.v=td.park_v=anchors[i][1]*16;
        td.left=180;td_set_target();
        expect(td_target.u==anchors[i][0]&&td_target.v==anchors[i][1]&&
               !strcmp(td_target.name,client.name)&&td_target.reserved==client.reserved,
               "driver beacon uses the legal road approach while retaining client identity and foot-only restriction");
        expect(td_drivable(anchors[i][0],anchors[i][1])&&td_near(&td_target)&&
               actors[1].pos.x==anchors[i][0]*32&&actors[1].pos.y==(anchors[i][1]-12)*32,
               "parking marker is displayed at a real driveable approach");
        td_interact();expect(td.stage==stages[i]&&td.msg==16&&td.job==jobs[i],
               "Select at the road approach explains park then walk without delivering from the car");
        td_enter_exit();
        expect(td.onfoot&&td_entry_timer&&td_target.u==client.u&&td_target.v==client.v,
               "actual exit immediately moves the objective from parking approach to the client");
        for(unsigned tick=0;tick<12;tick++)driving_tick(0);
        td.u=anchors[i][0]*16;td.v=anchors[i][1]*16;td_interact();
        expect(td.stage==stages[i]&&td.msg==6,"walking at the parking approach cannot hand off a parcel for the distant client");
        td.u=client.u*16;td.v=client.v*16;td_interact();
        expect(td.stage==stages[i]+1&&td.job==jobs[i],"walking to the true client advances exactly one park handoff");
        td.stage=stages[i];td_set_target();td.u=td.park_u+18*16;td.v=td.park_v;
        td_enter_exit();for(unsigned tick=0;tick<12;tick++)driving_tick(0);
        expect(!td.onfoot&&td_target.u==anchors[i][0]&&td_target.v==anchors[i][1],
               "re-entering the actual parked vehicle restores the road approach guidance");
    }
    for(unsigned stop=0;stop<=TD_STOPS;stop++) {
        UWORD u=1234,v=5678;UBYTE found=td_get_parking(stop,&u,&v);
        int expected=stop==34||stop==36||stop==41;
        expect(found==expected&&(expected||(u==1234&&v==5678)),
               "native parking getter leaves ordinary stops and invalid IDs unchanged");
    }
    UWORD u=1234,v=5678;
    expect(!td_get_parking(36,NULL,&v)&&v==5678&&!td_get_parking(36,&u,NULL)&&u==1234,
           "native parking lookup rejects missing outputs without a partial write");
    native_case();td.job=84;td_get_job(td.job,&td_job);td.stage=3;td.district=test_current_district=1;
    td.u=736*16;td.v=640*16;td_set_target();
    expect(td_route_district==0,"remote eastern parking destination guides a western driver through the core graph branch");
}

static void test_atlas_driver_handoff_and_freeze(void) {
    for(UBYTE district=0;district<TD_DISTRICT_COUNT;district++) {
        native_case();test_current_district=td.district=district;
        td.onfoot=district&1;td.park_district=(district+1)%TD_DISTRICT_COUNT;
        td.park_u=400*16;td.park_v=528*16;td.speed=0;
        td.job=80;td_get_job(td.job,&td_job);td.stage=1;td.left=199;td.seconds=100;
        td.map_x=43210;td.map_y=32109;td_set_target();
        camera_x=12000+district*16;camera_y=8000+district*16;camera_settings=0x5A;
        UWORD original_camera_x=camera_x,original_camera_y=camera_y;
        world_tick(J_START,300);
        expect(td.mode==TD_PAUSE&&td_resume_mode==TD_ROAM&&td.seconds==100&&td.left==199,
               "opening pause for the atlas preserves the active cross-district contract clock");
        td.menu=1;world_tick(J_A,300);
        expect(td.mode==TD_MAP&&test_map_opens==1&&test_map_active&&camera_settings==0,
               "actual pause choice opens the dedicated atlas UI exactly once");
        expect(td.map_x==43210&&td.map_y==32109,
               "atlas browsing does not reuse serialized legacy camera fields");
        td_state_t frozen=td;td_job_t frozen_job=td_job;
        td_stop_t frozen_target=td_target,frozen_cursor=td_cursor;
        actor_t frozen_actors[21];memcpy(frozen_actors,actors,sizeof(actors));
        UWORD frozen_traffic_u[6],frozen_traffic_v[6];UBYTE frozen_traffic_leg[6],frozen_ped_route[6];
        memcpy(frozen_traffic_u,td_traffic_u,sizeof(frozen_traffic_u));
        memcpy(frozen_traffic_v,td_traffic_v,sizeof(frozen_traffic_v));
        memcpy(frozen_traffic_leg,td_traffic_leg,sizeof(frozen_traffic_leg));
        memcpy(frozen_ped_route,td_ped_route,sizeof(frozen_ped_route));
        unsigned updates=test_map_updates;
        world_tick(J_RIGHT|J_DOWN,600);
        expect(test_map_updates==updates+1&&test_map_buttons==(J_RIGHT|J_DOWN)&&test_map_pressed==(J_RIGHT|J_DOWN),
               "actual map handler forwards held and first-pressed atlas pan inputs once per render");
        world_tick(J_RIGHT|J_DOWN,600);
        expect(test_map_updates==updates+2&&test_map_buttons==(J_RIGHT|J_DOWN)&&test_map_pressed==0,
               "held atlas pan forwards no repeated press edge");
        world_tick(J_A,600);
        expect(test_map_updates==updates+3&&test_map_buttons==J_A&&test_map_pressed==J_A,
               "atlas objective focus is delegated to its dedicated map UI");
        expect(!memcmp(&td,&frozen,sizeof(td))&&!memcmp(&td_job,&frozen_job,sizeof(td_job))&&
               !memcmp(&td_target,&frozen_target,sizeof(td_target))&&!memcmp(&td_cursor,&frozen_cursor,sizeof(td_cursor)),
               "atlas inputs preserve every mission, transit, pose, save field and target across all registered scenes");
        expect(!memcmp(actors,frozen_actors,sizeof(actors))&&
               !memcmp(td_traffic_u,frozen_traffic_u,sizeof(frozen_traffic_u))&&
               !memcmp(td_traffic_v,frozen_traffic_v,sizeof(frozen_traffic_v))&&
               !memcmp(td_traffic_leg,frozen_traffic_leg,sizeof(frozen_traffic_leg))&&
               !memcmp(td_ped_route,frozen_ped_route,sizeof(frozen_ped_route)),
               "atlas browsing freezes actors, autonomous traffic and persistent pedestrian identities");
        expect(test_current_district==district&&!test_queue_calls&&
               camera_x==original_camera_x&&camera_y==original_camera_y,
               "atlas pan/focus does not load another scene or pan the native gameplay camera");
        updates=test_map_updates;
        UBYTE exit=district&1?J_START:J_B;world_tick(exit,600);
        expect(td.mode==TD_PAUSE&&td.menu==1&&test_map_closes==1&&!test_map_active&&test_map_updates==updates,
               "B and Start close the atlas once without also forwarding an update");
        expect(camera_x==original_camera_x&&camera_y==original_camera_y&&camera_settings==0x5A,
               "closing the atlas restores the captured native camera through the UI close contract");
        world_tick(0,1);world_tick(J_B,600);
        expect(td.mode==TD_ROAM&&td.seconds==100&&td.left==199&&td.park_district==(district+1)%TD_DISTRICT_COUNT,
               "leaving atlas and pause preserves the suspended contract and remotely parked car");
        world_tick(0,60);
        expect(td.seconds==101&&td.left==198,
               "the game resumes one real second rather than catching up paused atlas inspection time");
    }
}

static void queen_runtime_case(UBYTE closed) {
    native_case();td_stop_t origin;td_get_stop(46,&origin);td_get_stop(48,&td_cursor);
    td_session_live=1;td.onfoot=1;td.u=td.safe_u=origin.u*16;td.v=td.safe_v=origin.v*16;
    td.seconds=12+(closed?2:0);td.subsecond=0;
    load_authored_scene_fixture();toronto_init();
    td.mode=TD_TRANSIT;td.transit_origin=46;td.transit_target=48;td_get_stop(48,&td_cursor);
    td_streetcar_runtime_reset();td_streetcar_runtime_prepare(0);
    world_tick(J_A,1);
    expect(td.mode==(closed?TD_WAIT:TD_RIDE)&&td.cash==(closed?30:27)&&td.ride_left==(closed?0:8),
           "visual Queen fixture books the real Yonge-to-Saulter trip with its exact fare and eight ride seconds");
}

static void advance_queen_to_east_view(void) {
    for(unsigned second=0;second<8&&!td_transition_pending;second++)world_tick(0,60);
    expect(td_transition_pending==1&&test_queued_district==3&&td.mode==TD_RIDE&&td.ride_left,
           "ordinary paid Queen updates queue the actual East scene before alighting");
}

static void test_streetcar_loader_binding(void) {
    for(UBYTE district=0;district<TD_DISTRICT_COUNT;district++)for(UBYTE active=0;active<2;active++){
        native_case();test_current_district=district;load_authored_scene_fixture();
        if(active&&district!=2)activate_actor(&actors[1]);
        actors[1].script.bank=7;actors[1].script_update.bank=8;
        actors[1].hscript_hit=111;actors[1].hscript_update=222;
        UBYTE bound=td_streetcar_runtime_bind();actor_t *tram=&actors[15];
        expect(bound==(district!=2)&&tram->sprite.ptr==(district==2?&test_player_sprite:&test_tram_sprite)&&
               tram->base_tile==(district==2?4:96),
               "binding captures the genuine preloaded authored sheet before the caller reuses slot1; High Park has no tram loader");
        expect((tram->flags&(ACTOR_FLAG_ACTIVE|ACTOR_FLAG_PERSISTENT|ACTOR_FLAG_HIDDEN))==
               (ACTOR_FLAG_ACTIVE|ACTOR_FLAG_PERSISTENT|ACTOR_FLAG_HIDDEN)&&
               !tram->collision_group&&!tram->script.bank&&!tram->script_update.bank&&
               !tram->hscript_hit&&!tram->hscript_update&&(district==2||tram->frame==8),
               "bound tram is initially hidden and linked once without the loader's scripts, collision or animation handles");
        expect(test_actors_active_head==tram&&!tram->prev&&!tram->next&&
               (district==2||(!actors[1].prev&&!actors[1].next))&&actors_inactive_head==NULL,
               "inactive and active authored loaders are detached cleanly before custom tram activation");
    }
}

static void test_queen_visual_save_follow_and_loaded_arrival(void) {
    queen_runtime_case(0);td_state_t paid=td;advance_queen_to_east_view();
    expect(td.district==paid.district&&td.u==paid.u&&td.v==paid.v&&td.safe_u==paid.safe_u&&td.safe_v==paid.safe_v&&
           td.cash==paid.cash&&td.park_district==paid.park_district&&td.park_u==paid.park_u&&td.park_v==paid.park_v,
           "visual scene following retains the serialized origin, cash and separately parked Union car");
    td_state_t queued=td;unsigned writes=sram_writes;apply_queued_scene();
    expect(!memcmp(&td,&queued,58)&&sram_writes==writes&&td.district==0&&test_current_district==3&&
           td_streetcar_view_district==3&&td_streetcar_ride_view,
           "loading the paid rider's visual scene preserves every58-byte saved field without writing a new trip");
    expect(actors[15].sprite.ptr==&test_tram_sprite&&actors[15].base_tile==96&&
           !(actors[15].flags&ACTOR_FLAG_HIDDEN)&&(PLAYER.flags&ACTOR_FLAG_HIDDEN)&&
           PLAYER.pos.x==td_streetcar_focus_u*2&&PLAYER.pos.y==td_streetcar_focus_v*2&&
           (actors[8].flags&ACTOR_FLAG_HIDDEN),
           "the first East ride frame uses the authored tram, tracks its Q4 focus and hides the courier and remotely parked car");
    unsigned queues=test_queue_calls;test_queue_fail=1;
    world_tick(0,td.ride_left*60-td.subsecond);
    expect(td.mode==TD_ROAM&&td.district==3&&td.u==128*16&&td.v==556*16&&
           !td_transition_pending&&test_queue_calls==queues&&td.cash==27&&!(td.reserved&TD_STREETCAR_HOLD),
           "already-loaded booked target commits safe alighting even when allocation would fail; it requests no redundant scene");
    expect(td.park_district==0&&td.park_u==560*16&&td.park_v==720*16&&td_foot_free(td.u,td.v),
           "Queen alighting leaves the origin car behind and presents a clear sidewalk foot position");
    td_state_t arrived=td;memset(&td,0,sizeof(td));
    expect(td_restore()&&!memcmp(&td,&arrived,58),"already-loaded Queen arrival restores the exact committed destination without an extra fare");
}

static void test_queen_cold_derived_view(void) {
    queen_runtime_case(0);advance_queen_to_east_view();apply_queued_scene();td_save();
    td_state_t paid=td;UBYTE record[67];memcpy(record,(const void*)td_save_address(td_save_slot),sizeof(record));
    memset(&td,0,sizeof(td));td_session_live=0;test_current_district=0;test_queue_calls=0;
    load_authored_scene_fixture();toronto_init();
    expect(td.mode==TD_HELP&&td_resume_mode==TD_RIDE&&td.district==paid.district&&td.u==paid.u&&td.v==paid.v&&
           td.cash==paid.cash&&td.ride_left==paid.ride_left&&td.seconds==paid.seconds&&td.subsecond==paid.subsecond&&
           td.park_district==paid.park_district&&td.park_u==paid.park_u&&td.park_v==paid.park_v,
           "cold paid reset restores the origin and car while retaining the actual remaining Queen trip behind help");
    expect(td_transition_pending==1&&test_queued_district==3&&td_streetcar_view_district==3&&
           !memcmp(record,(const void*)td_save_address(td_save_slot),sizeof(record)),
           "cold reset derives the moving East view from the paid clock without modifying the committed origin record");
    td_state_t restored=td;memset(td_test_sram,0,sizeof(td_test_sram));apply_queued_scene();
    expect(!memcmp(&td,&restored,58)&&td_cursor.district==3&&td_cursor.u==128&&td_cursor.v==556&&
           (PLAYER.flags&ACTOR_FLAG_HIDDEN)&&!(actors[15].flags&ACTOR_FLAG_HIDDEN),
           "warm visual entry does not restore twice and presents the recovered paid rider on the correct native tram");
}

static void test_queen_visual_queue_retry_clock(void) {
    queen_runtime_case(0);test_queue_fail=1;
    for(unsigned second=0;second<8&&!td_transition_pending;second++)world_tick(0,60);
    expect(td_transition_pending==2&&td.mode==TD_RIDE&&td.district==0&&td.cash==27&&
           td_streetcar_view_district==3&&test_queued_district==TD_DISTRICT_NONE,
           "failed visual scene allocation preserves the paid origin trip and records the derived East retry");
    td_state_t frozen=td;unsigned writes=sram_writes;actor_t old_actors[21];memcpy(old_actors,actors,sizeof(actors));
    world_tick(0,600);
    expect(td_transition_pending==2&&!memcmp(&td,&frozen,58)&&sram_writes==writes&&!memcmp(actors,old_actors,sizeof(actors)),
           "pending failed presentation allocation freezes the saved trip, deadline, cash and native road users");
    test_queue_fail=0;world_tick(0,600);
    expect(td_transition_pending==1&&test_queued_district==3&&!memcmp(&td,&frozen,58),
           "retrying a paid presentation load queues East without applying the frozen interval or moving saved origin coordinates");
    apply_queued_scene();world_tick(0,1);
    expect(td.mode==TD_RIDE&&td.seconds==frozen.seconds&&td.subsecond==frozen.subsecond+1&&
           td.ride_left==frozen.ride_left&&td.cash==frozen.cash&&td.district==frozen.district,
           "the first loaded paid-ride frame cannot catch up the long frozen allocation retry and consume its remaining trip");
}

static void test_streetcar_pause_and_map_pose(void) {
    for(UBYTE closed=0;closed<2;closed++){
        queen_runtime_case(closed);world_tick(0,1);
        td_state_t running=td;actor_t tram=actors[15];
        UWORD focus_u=td_streetcar_focus_u,focus_v=td_streetcar_focus_v;
        UBYTE district=td_streetcar_view_district;
        world_tick(J_START,600);
        expect(td.mode==TD_PAUSE&&td.seconds==running.seconds&&td.subsecond==running.subsecond&&
               !memcmp(&tram,&actors[15],sizeof(tram)),
               "opening pause freezes the actual autonomous tram pose on the same paid or waiting update");
        td.menu=1;world_tick(J_A,600);td_state_t frozen=td;
        world_tick(J_RIGHT|J_DOWN,600);world_tick(J_A,600);world_tick(0,600);
        expect(td.mode==TD_MAP&&!memcmp(&td,&frozen,58)&&!memcmp(&tram,&actors[15],sizeof(tram))&&
               td_streetcar_focus_u==focus_u&&td_streetcar_focus_v==focus_v&&td_streetcar_view_district==district,
               "map pan/focus leaves the autonomous pose, derived ride view and every persistent trip field frozen");
        world_tick(J_B,600);world_tick(0,1);world_tick(J_B,600);
        expect(td.mode==(closed?TD_WAIT:TD_RIDE)&&td.cash==running.cash&&td.ride_left==running.ride_left&&
               td.seconds==running.seconds&&td.subsecond==running.subsecond,
               "closing map and pause preserves the single paid fare or unpaid wait without paused-time catchup");
        world_tick(0,60);
        expect(td.seconds==running.seconds+1&&td.cash==running.cash,
               "resuming the world advances one actual second rather than the long paused inspection interval");
    }
}

static void test_streetcar_rail_parking_and_v6_recovery(void) {
    native_case();td.u=td.safe_u=676*16;td.v=td.safe_v=536*16;
    td_streetcar_runtime_prepare(0);td_state_t driver=td;unsigned writes=sram_writes;td_enter_exit();
    expect(!td.onfoot&&!td_entry_timer&&td.msg==17&&td.u==driver.u&&td.v==driver.v&&
           td.park_u==driver.park_u&&td.park_v==driver.park_v&&sram_writes==writes,
           "attempting to exit a stationary car on Queen's rail corridor refuses parking even while the tram is elsewhere");
    native_case();td.onfoot=1;td.u=td.safe_u=676*16;td.v=td.safe_v=556*16;
    td.park_u=676*16;td.park_v=536*16;td.cash=141;td_save();
    volatile UBYTE *old=td_save_address(td_save_slot);old[2]=6;refresh_record_crc(old);
    td_state_t legacy=td;memset(&td,0,sizeof(td));
    expect(td_restore()&&!memcmp(&td,&legacy,58),"a genuine CRC-valid58-byte version6 on-foot rail-parking record remains readable");
    td_session_live=0;load_authored_scene_fixture();toronto_init();
    expect(td.mode==TD_HELP&&td.onfoot&&td.u==legacy.u&&td.v==legacy.v&&td.cash==legacy.cash&&
           td.seconds==legacy.seconds&&td.park_district==legacy.park_district&&
           (td.park_u!=legacy.park_u||td.park_v!=legacy.park_v)&&
           td_district_drivable(0,td.park_u>>4,td.park_v>>4)&&
           td_streetcar_runtime_parking_allowed(0,td.park_u,td.park_v),
           "cold legacy recovery moves only the rail-parked car to a connected real cross-street approach while preserving the courier and earnings");
    expect(td_save_address(td_save_slot)[2]==7,"successful old rail-car recovery commits a current version7 record");
    td_state_t recovered=td;memset(&td,0,sizeof(td));
    expect(td_restore()&&td.mode==TD_ROAM&&td.park_u==recovered.park_u&&td.park_v==recovered.park_v&&td.cash==141,
           "the recovered car location is durable and reloads without discarding the valid old earnings");
}

static void test_streetcar_contacts_and_future_traffic_yield(void) {
    for(UBYTE foot=0;foot<2;foot++)for(UBYTE carrying=0;carrying<2;carrying++){
        native_case();td.job=0;td.left=120;td_get_job(0,&td_job);td_set_target();
        if(carrying){td_interact();expect(td.stage==1,"the tram cargo-contact fixture actually collects its Union parcel before testing carrying damage");}
        td.onfoot=foot;td.u=td.safe_u=676*16;td.v=td.safe_v=536*16;td.seconds=12;
        td_state_t before=td;world_tick(0,1);td_streetcar_runtime_prepare(0);
        expect(td.u!=before.u||td.v!=before.v,"a stationary car or walker overlapping the autonomous tram is separated on its first ordinary update");
        expect(td.district==before.district&&td.cash==before.cash&&td.job==before.job&&td.stage==before.stage&&
               td.left==before.left&&td.safe_u==td.u&&td.safe_v==td.v&&
               (foot?td_streetcar_runtime_foot_clear(td.u,td.v):td_streetcar_runtime_car_clear(td.u,td.v,td.u,td.v)),
               "tram separation reaches a verified clear native position without changing fare, cargo lifecycle or deadline");
        expect(td.health==(carrying&&!foot?88:100),
               "tram contact damages only an occupied carrying vehicle; an uncollected parcel or walker retains condition");
        expect(foot?(td.park_u==before.park_u&&td.park_v==before.park_v):
                    (td.park_u==td.u&&td.park_v==td.v),
               "foot separation leaves the parked car unchanged while occupied-car separation carries that vehicle");
    }
    native_case();td.seconds=14;td.subsecond=0;td_streetcar_runtime_prepare(0);
    td_streetcar_box_t ahead={735*16,531*16,745*16,541*16,0};
    expect(td_streetcar_sweep(td.seconds,td.subsecond,0,&ahead)==TD_STREETCAR_CLEAR&&
           !td_streetcar_runtime_traffic_clear(0,740*16,536*16),
           "traffic yields to the upcoming one-second Queen body sweep before an actual present overlap");
    td_traffic_u[0]=740*16;td_traffic_v[0]=536*16;td_traffic_leg[0]=0;
    UWORD u=td_traffic_u[0],v=td_traffic_v[0];td_traffic_step();
    expect(td_traffic_u[0]==u&&td_traffic_v[0]==v,"the production autonomous traffic step respects future tram occupancy instead of entering its path");
    expect(td_streetcar_runtime_traffic_clear(0,740*16,520*16),
           "opposite-lane road traffic retains a usable passing lane beside the single tram");
}

static void test_queen_hold_v7_recovery_and_invalid_flags(void) {
    queen_runtime_case(0);advance_queen_to_east_view();apply_queued_scene();
    const WORD blocked_u[5]={128,146,110,128,128},blocked_v[5]={556,556,556,574,538};
    for(unsigned i=0;i<5;i++){td_traffic_u[i]=blocked_u[i]*16;td_traffic_v[i]=blocked_v[i]*16;}
    UBYTE left=td.ride_left;while(left--)td_second();
    expect(td.mode==TD_RIDE&&td.ride_left==1&&(td.reserved&TD_STREETCAR_HOLD)&&td.cash==27&&td.district==0,
           "an actual blocked booked landing produces a paid version7 hold at the saved origin");
    for(unsigned second=0;second<128;second++)td_second();td_streetcar_runtime_prepare(0);
    expect(td_streetcar_ride_view&&td_streetcar_view_district==3&&td_streetcar_focus_u==128*16&&td_streetcar_focus_v==536*16,
           "two complete timetable cycles cannot send a paid held rider away from the booked Saulter doors");
    td_state_t held=td;expect(td_save_address(td_save_slot)[2]==7,"real held alighting writes the version7 semantic discriminator");
    td_session_live=0;memset(&td,0,sizeof(td));test_current_district=0;load_authored_scene_fixture();toronto_init();
    expect(td.mode==TD_HELP&&td_resume_mode==TD_RIDE&&td.reserved==TD_STREETCAR_HOLD&&
           td.u==held.u&&td.v==held.v&&td.cash==held.cash&&td.seconds==held.seconds&&
           td_streetcar_view_district==3&&test_queued_district==3,
           "cold CRC-valid held recovery derives its booked target doors while preserving paid origin bytes and cash");
    apply_queued_scene();world_tick(J_A,1);world_tick(0,1);
    for(unsigned i=0;i<6;i++)td_traffic_u[i]=td_traffic_v[i]=30000;
    world_tick(0,60-td.subsecond);
    expect(td.mode==TD_ROAM&&td.district==3&&td.u==128*16&&td.v==556*16&&!td.reserved&&td.cash==27,
           "a recovered held rider safely alights once after the actual obstruction clears and clears the hold without another fare");
    for(UBYTE fault=0;fault<5;fault++){
        native_case();td=held;td.cash=111;td_save();td.cash=222;td_save();
        volatile UBYTE *record=td_save_address(td_save_slot);td_state_t bad=td;
        if(fault==0)bad.mode=TD_ROAM;
        if(fault==1){bad.transit_origin=0;bad.transit_target=12;bad.u=bad.safe_u=560*16;bad.v=bad.safe_v=720*16;}
        if(fault==2)bad.ride_left=2;
        if(fault==3)bad.reserved=2;
        memcpy((void*)(record+8),&bad,58);if(fault==4)record[2]=6;refresh_record_crc(record);
        expect(td_restore()&&td.cash==111&&td.reserved==TD_STREETCAR_HOLD&&td.mode==TD_RIDE,
               "CRC-valid nonride/nonQueen/left2/unknown-bit/version6 hold faults fall back to the older valid paid record");
    }
}

static void displaced_queen_wait_case(UBYTE obstructed) {
    native_case();td_get_stop(48,&td_cursor);
    td.onfoot=1;td.mode=TD_WAIT;td.transit_origin=46;td.transit_target=48;
    td.u=td.safe_u=690*16;td.v=td.safe_v=542*16;td.seconds=12;
    for(unsigned i=9;i<15;i++)actors[i].flags|=ACTOR_FLAG_HIDDEN;
    if(obstructed){
        /* A real walkable pedestrian position blocks the nearer downward
           escape while leaving the short horizontal path beside the nose. */
        actors[9].flags&=~ACTOR_FLAG_HIDDEN;
        actors[9].pos.x=690*32;actors[9].pos.y=550*32;
        expect(td_district_walkable(0,690,550),"WAIT obstruction is a real native walkable pedestrian point");
    }
    td_stop_t origin;td_get_stop(46,&origin);
    expect(td_near(&origin)&&td_district_walkable(0,690,542),
           "contact WAIT starts at a genuine walkable point inside both strict platform-radius axes");
    td_streetcar_pose_t pose;td_streetcar_box_t tram,body;
    expect(td_streetcar_pose(td.seconds,td.subsecond,&pose)&&td_streetcar_bounds(&pose,&tram)&&
           td_streetcar_runtime_box(td.u,td.v,3*16,3*16,td.district,&body)&&
           td_streetcar_runtime_overlap(&tram,&body),
           "WAIT recovery fixture actually overlaps the autonomous Queen body before its first ordinary update");
}

static void test_displaced_streetcar_wait(void) {
    displaced_queen_wait_case(1);td_state_t booked=td;world_tick(0,1);
    td_stop_t origin;td_get_stop(46,&origin);
    expect(td.u==694*16&&td.v==542*16&&!td_near(&origin)&&td_streetcar_runtime_foot_clear(td.u,td.v),
           "real contact recovery takes the verified18px lateral point when the nearer curb escape is occupied");
    expect(td.mode==TD_ROAM&&td.cash==booked.cash&&td.transit_origin==booked.transit_origin&&
           td.transit_target==booked.transit_target&&!td.ride_left,
           "recovery outside the booked platform cancels the wait without charging or discarding its destination");
    td_state_t cancelled=td;memset(&td,0,sizeof(td));
    expect(td_restore()&&td.mode==TD_ROAM&&td.u==cancelled.u&&td.v==cancelled.v&&td.cash==booked.cash,
           "displaced wait cancellation is persisted before a reset can resume the stale booking");
    world_tick(0,60-td.subsecond);
    expect(td.mode==TD_ROAM&&td.cash==booked.cash,
           "the following open-window second cannot charge a courier who was separated beyond the stop radius");

    displaced_queen_wait_case(0);booked=td;world_tick(0,1);
    td_get_stop(46,&origin);
    expect(td.mode==TD_WAIT&&td_near(&origin)&&td.cash==booked.cash&&
           td.transit_origin==booked.transit_origin&&td.transit_target==booked.transit_target,
           "contact recovery still inside the platform preserves an ordinary valid wait and its uncharged fare");
    world_tick(0,60-td.subsecond);
    expect(td.mode==TD_RIDE&&td.cash==booked.cash-3&&td.ride_left==8,
           "the retained valid wait boards once during the next open second with the original destination and duration");

    for(unsigned service=0;service<sizeof(transit_cases)/sizeof(transit_cases[0]);service++)
        for(UBYTE fault=0;fault<3;fault++){
            transit_menu_case(service,transit_cases[service].phase);
            if(fault==0)td.u+=15*16;
            if(fault==1)td.v+=15*16;
            if(fault==2)td.district=(td.district+1)%TD_DISTRICT_COUNT;
            booked=td;world_tick(J_A,1);
            expect(td.mode==TD_ROAM&&td.cash==booked.cash&&!td.ride_left&&
                   td.transit_origin==booked.transit_origin&&td.transit_target==booked.transit_target,
                   "every legacy service rejects an open-window boarding outside either strict15px axis or its origin district without fare");
        }
    queen_runtime_case(1);td.u+=15*16;booked=td;td_second();
    expect(td.mode==TD_ROAM&&td.cash==booked.cash&&!td.ride_left&&td.seconds==booked.seconds+1,
           "an already displaced closed-window Queen wait cancels before departure while the real clock still advances");
}

static void test_streetcar_cue_and_blocked_contact(void) {
    const WORD deltas[]={-32768,-16383,-2731,-13,-1,0,1,13,2731,16383,32767};
    for(unsigned i=0;i<sizeof(deltas)/sizeof(deltas[0]);i++)for(UBYTE cue=0;cue<=12;cue++){
        long expected=(long)deltas[i]*cue/12;
        expect(td_streetcar_runtime_cue_offset(deltas[i],cue)==expected,
               "bounded native cue scaling preserves signed fractional interpolation at16-bit extrema and ordinary offsets");
    }
    expect(td_streetcar_runtime_cue_offset(-32768,255)==-32768&&
           td_streetcar_runtime_cue_offset(32767,255)==32767,
           "an excessive transient cue cannot extrapolate outside its saved origin and tram endpoints");
    const UWORD origins[4][2]={{8*16,8*16},{1015*16,967*16},{980*16,556*16},{116*16,556*16}};
    for(unsigned i=0;i<4;i++)for(UBYTE cue=1;cue<=12;cue++){
        native_case();td.u=origins[i][0];td.v=origins[i][1];td.onfoot=1;td.mode=TD_RIDE;
        td_streetcar_ride_view=1;td_streetcar_cue=cue;
        expect(td_streetcar_pose(12,0,&td_streetcar_display),"cue fixture samples the actual Queen Yonge doors");
        td_streetcar_focus_u=td_streetcar_display.u;td_streetcar_focus_v=td_streetcar_display.v;
        td_state_t saved=td;td_streetcar_runtime_present();
        long x=td_streetcar_display.u,y=td_streetcar_display.v+14*16;
        x+=((long)td.u-x)*cue/12;y+=((long)td.v-y)*cue/12;
        expect(PLAYER.pos.x==x*2&&PLAYER.pos.y==y*2&&!(PLAYER.flags&ACTOR_FLAG_HIDDEN)&&
               !memcmp(&td,&saved,58),
               "actual courier presentation matches independent wide-arithmetic endpoints without mutating the saved ride");
    }
    native_case();td.onfoot=1;td.mode=TD_WAIT;td.transit_origin=46;td.transit_target=48;
    td.u=td.safe_u=676*16;td.v=td.safe_v=536*16;td.seconds=12;
    actors[2].pos.x=676*32;actors[2].pos.y=536*32;
    td_state_t trapped=td;
    expect(td_streetcar_runtime_recover_contact(1)==TD_STREETCAR_PARK_BLOCKED&&
           !memcmp(&td,&trapped,58),
           "fully obstructed contact reports BLOCKED and preserves state instead of inventing an unvalidated escape");
}

static void test_streetcar_native_q5_bounds(void) {
    native_case();td_streetcar_bound=td_streetcar_valid=1;
    unsigned bad_pose=0,bad_size=0,bad_scaled=0,bad_state=0;UBYTE headings=0;
    for(unsigned phase=0;phase<64*60;phase++){
        td.seconds=phase/60;td.subsecond=phase%60;
        if(!td_streetcar_pose(td.seconds,td.subsecond,&td_streetcar_display)){bad_pose++;continue;}
        test_current_district=td_streetcar_display.district;
        td_state_t saved=td;td_streetcar_runtime_present();
        actor_t *tram=&actors[TD_STREETCAR_ACTOR];
        unsigned width=td_streetcar_display.heading&4?12:28,height=40-width;
        headings|=1<<(td_streetcar_display.heading>>2);
        if(tram->bounds.left!=-(WORD)(width*16)||tram->bounds.right!=(WORD)(width*16)-1||
           tram->bounds.top!=-(WORD)(height*16)||tram->bounds.bottom!=(WORD)(height*16)-1||
           (tram->flags&ACTOR_FLAG_HIDDEN))bad_size++;
        td_streetcar_box_t physical;
        if(!td_streetcar_bounds(&td_streetcar_display,&physical)||
           (long)tram->pos.x+tram->bounds.left!=(long)physical.left*2||
           (long)tram->pos.x+tram->bounds.right!=(long)physical.right*2+1||
           (long)tram->pos.y+tram->bounds.top!=(long)physical.top*2||
           (long)tram->pos.y+tram->bounds.bottom!=(long)physical.bottom*2+1)bad_scaled++;
        if(memcmp(&td,&saved,58))bad_state++;
    }
    expect(!bad_pose&&headings==15,"one actual64-second streetcar cycle exercises all four native actor orientations");
    expect(!bad_size,"every native tram presentation uses exact28x12 or12x28 inclusive Q5 actor bounds");
    expect(!bad_scaled,"every Q5 actor endpoint agrees with the actual Q4 physical body including the last half-subpixel");
    expect(!bad_state,"all autonomous Q5-bound presentations preserve the serialized game state");
}

/* Deliberately include the East bend's empty interior, each seam, the
   turnaround lane and ordinary off-corridor traffic; none are generated
   from the production path table. */
static const UWORD traffic_probe_points[4][6][2]={
    {{740*16,536*16},{740*16,520*16},{24*16,536*16},{1000*16,520*16},{640*16,528*16},{560*16,720*16}},
    {{836*16,536*16},{836*16,520*16},{912*16,528*16},{1000*16,536*16},{800*16,480*16},{512*16,536*16}},
    {{740*16,536*16},{740*16,520*16},{24*16,536*16},{1000*16,520*16},{640*16,528*16},{560*16,720*16}},
    {{680*16,536*16},{680*16,504*16},{664*16,488*16},{664*16,520*16},{400*16,504*16},{24*16,536*16}}
};
static void traffic_probe_boxes(UBYTE district,td_streetcar_box_t boxes[6]) {
    for(unsigned i=0;i<6;i++){
        UWORD u=traffic_probe_points[district][i][0],v=traffic_probe_points[district][i][1];
        boxes[i].left=u-80;boxes[i].right=u+80;boxes[i].top=v-80;boxes[i].bottom=v+80;
        boxes[i].district=district;
    }
}
static void test_traffic_lookahead_truth(void) {
    native_case();
    for(UBYTE district=0;district<4;district++){
        td_streetcar_box_t boxes[6];traffic_probe_boxes(district,boxes);
        for(unsigned phase=0;phase<64*60;phase++){
            td.seconds=65472+phase/60;td.subsecond=phase%60;td_state_t before=td;
            for(UBYTE i=0;i<6;i++){
                UBYTE clear=td_streetcar_sweep((UWORD)(td.seconds+1),td.subsecond,60,&boxes[i])==TD_STREETCAR_CLEAR;
                expect(td_streetcar_runtime_traffic_clear(district,traffic_probe_points[district][i][0],
                       traffic_probe_points[district][i][1])==clear,
                       "scalar traffic lookahead equals the exact future60VBlank body oracle through the complete rollover cycle");
            }
            expect(!memcmp(&td,&before,58),"all scalar traffic queries preserve clock, fare, ride and mission state");
        }
    }
    td.seconds=65535;td.subsecond=59;
    expect(!td_streetcar_runtime_traffic_clear(TD_DISTRICT_COUNT,traffic_probe_points[0][0][0],traffic_probe_points[0][0][1]),
           "an unknown traffic district fails closed");
    td.subsecond=60;
    expect(!td_streetcar_runtime_traffic_clear(0,traffic_probe_points[0][0][0],traffic_probe_points[0][0][1]),
           "invalid traffic subsecond fails closed");
    td.subsecond=0;
    UWORD invalid[6][2]={{79,6000},{16304,6000},{6000,79},{6000,15536},{65535,6000},{6000,65535}};
    for(unsigned i=0;i<6;i++)expect(!td_streetcar_runtime_traffic_clear(0,invalid[i][0],invalid[i][1]),
           "scalar traffic rejects actual footprint-bound failures including unsigned overflow coordinates");
}

static void test_sweep_section_scan_equivalence(void) {
    const UWORD intervals[]={0,1,59,60,61,119,120,121,239,240,241,3839,3840,65535};
    /* Recorded from the original full16-section scan before narrowing it.
       Existing independent corridor/body fixtures establish that baseline. */
    const uint32_t golden[4]={0x496959e1u,0x6ca5fcfeu,0xb8588dc5u,0x3321d730u};
    for(UBYTE district=0;district<4;district++){
        td_streetcar_box_t boxes[6];traffic_probe_boxes(district,boxes);uint32_t digest=2166136261u;
        for(unsigned phase=0;phase<64*60;phase++)for(unsigned n=0;n<sizeof(intervals)/sizeof(intervals[0]);n++)
            for(unsigned i=0;i<6;i++){
                UBYTE value=td_streetcar_sweep(65472+phase/60,phase%60,intervals[n],&boxes[i]);
                digest=(digest^value)*16777619u;
            }
        if(digest!=golden[district])fprintf(stderr,"Section scan baseline district%u digest%08x\n",district,digest);
        expect(digest==golden[district],"all phases, short-window endpoints and bounded long sweeps retain the original full-scan result digest");
    }
}

int main(void) {
    expect(sizeof(td_state_t)==58&&offsetof(td_state_t,district)==56,"host fixture retains the current serialized state layout");
    test_acceleration_and_turning();test_glancing_contact();test_wall_and_brake();
    test_momentum_and_coasting();test_pressed_edge_once();test_clock();
    test_passenger_comfort();test_entry_collision();test_hidden_pedestrian();
    test_signal_and_autonomous_traffic();
    test_city_routes_and_walking();
    test_audio_event_integration();
    test_bounded_corner_assist();
    test_atomic_saves();test_valid_crc_invalid_states();test_legacy_and_transit_recovery();
    test_wait_cancellation();
    test_entry_transit_exclusion();test_fresh_transit_after_failure();
    test_pickup_damage_lifecycle();
    test_finished_job_target();
    test_current_transit_window();test_transit_funds_pause_and_deadline();test_immediate_transit_interrupted_save();
    test_safe_transit_alighting();
    test_cross_district_streetcar();
    test_v5_migration_and_interrupted_upgrade();test_district_semantic_fallback();
    test_reciprocal_portals();test_queue_failure_and_remote_boot();test_car_entry_at_portal();test_first_frame_actors();
    test_walk_pace_dispatch_and_foot_delivery();
    test_park_delivery_guidance();
    test_atlas_driver_handoff_and_freeze();
    test_streetcar_loader_binding();test_queen_visual_save_follow_and_loaded_arrival();
    test_queen_cold_derived_view();test_queen_visual_queue_retry_clock();test_streetcar_pause_and_map_pose();
    test_streetcar_rail_parking_and_v6_recovery();test_streetcar_contacts_and_future_traffic_yield();
    test_queen_hold_v7_recovery_and_invalid_flags();
    test_displaced_streetcar_wait();
    test_streetcar_cue_and_blocked_contact();
    test_streetcar_native_q5_bounds();
    test_traffic_lookahead_truth();test_sweep_section_scan_equivalence();
    printf("Host engine regressions: %u checks, %u failures. Hardware/emulator evidence remains separate.\n",checks,failures);
    return failures?1:0;
}
