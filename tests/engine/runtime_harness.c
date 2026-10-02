/* Includes production gameplay so these fixtures exercise its static routines. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "gbvm_stubs.h"
#define TD_WORLD_ROUTE_DATA
#include "engine_under_test.c"
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
static enum { CLEAR_GROUND,EAST_WALL,SOUTH_CURB,NATIVE_GRID,HIDDEN_NPC_TILE,SOUTHWEST_CORNER } geometry;
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
    if (geometry==NATIVE_GRID) return x<native_widths[district]&&y<native_heights[district]?native_collision[district][y*native_widths[district]+x]:15;
    if (geometry==HIDDEN_NPC_TILE && x==48 && y==53) return 15;
    return 0;
}
UBYTE tile_at(UBYTE x,UBYTE y) {return district_tile(test_current_district,x,y);}
void actor_set_frames(actor_t *actor,UBYTE first,UBYTE end) {
    actor->frame=actor->frame_start=first; actor->frame_end=end;
}
void activate_actor(actor_t *actor) { (void)actor; }
void td_ui_init(void) { ui_draws++; }
void td_ui_draw(void) { ui_draws++; }
void td_audio_init(void) {audio_inits++;audio_mode=TD_AUDIO_FULL;audio_active=0;audio_cue=255;}
void td_audio_update(WORD speed,UBYTE vehicle,UBYTE onfoot,UBYTE braking,UBYTE active) {
    (void)speed;(void)vehicle;(void)onfoot;(void)braking;audio_updates++;audio_active=active;
}
void td_audio_play(UBYTE cue) {if(audio_mode!=TD_AUDIO_SILENT)audio_cue=cue;}
void td_audio_set_mode(UBYTE mode) {audio_mode=mode;}
UBYTE td_audio_get_mode(void) {return audio_mode;}
void td_get_street(UWORD u,UWORD v,char *out) { (void)u;(void)v;strcpy(out,"TEST ROAD"); }
void td_get_stop(UBYTE index,td_stop_t *out) {
    if(authored_content){stop_reads++;if(index<TD_STOPS)*out=td_fixture_stops[index];return;}
    stop_reads++;memset(out,0,sizeof(*out));out->u=900;out->v=900;
    out->transit=index==0?1:0;
    if(index==0&&stop0_here) {out->u=400;out->v=450;}
}
void td_get_job(UBYTE index,td_job_t *out) {
    if(authored_content){if(index<TD_QUESTS)*out=td_fixture_jobs[index];return;}
    (void)index;memset(out,0,sizeof(*out));out->count=2;
    out->vehicle=TD_NONE;out->seconds=120;out->reward=150;out->route[1]=1;
}
UBYTE td_district_current(void) {return test_current_district;}
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
    td_corner_used=0;
    memset(td_nearby_routes,0,sizeof(td_nearby_routes));
    joy=joy_pressed=0;sys_time=0;stop_reads=ui_draws=0;
    stop0_here=authored_content=0;test_queue_calls=test_reset_calls=0;
    td_save_slot=TD_NONE;td_save_seq=0;actors_inactive_head=NULL;
    sram_writes=sram_interrupt_after=0;sram_interrupt_enabled=0;
    geometry=CLEAR_GROUND;
    td_audio_init();audio_updates=audio_inits=0;
    for(unsigned i=0;i<6;i++) { td_traffic_u[i]=30000;td_traffic_v[i]=30000;td_traffic_leg[i]=0;td_ped_route[i]=TD_NONE; }
    td_ped_refresh=1;td_ped_anchor_u=td.u>>4;td_ped_anchor_v=td.v>>4;
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
    reset_case();prime_car();td.job=0;td_job.kind=5;td.health=100;
    for(unsigned i=0;i<36;i++)driving_tick(J_A|J_RIGHT);
    expect(td.health<100,"fast passenger turns reduce comfort");
    expect(td.health>0&&td.job!=TD_NONE,"a few fast turns do not instantly fail the passenger job");

    reset_case();td.job=0;td_job.kind=5;td.speed=5;td_vx=80;td.health=100;
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

    reset_case();td.mode=TD_WAIT;td.onfoot=1;td_traffic_u[0]=100*16;before=td_traffic_u[0];sys_time=4;
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
    expect(td_save_address(td_save_slot)[2]==6,"legacy migration writes a current dual-slot record");

    reset_case();td.mode=TD_RIDE;td.onfoot=1;td.transit_origin=0;td.transit_target=12;td.ride_left=4;td.cash=27;
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

    reset_case();td.mode=TD_RIDE;td.onfoot=1;td.transit_origin=0;td.transit_target=12;td.ride_left=3;
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
    world_tick(J_RIGHT,1);world_tick(J_A,1);
    expect(td.mode==TD_WAIT&&td.transit_target==12,"fresh trip selects a different served destination");
    td.seconds=17;td.subsecond=59;world_tick(0,1);
    expect(td.mode==TD_RIDE&&td.cash==27,"fresh free-roaming trip boards and charges exactly one fare");
    world_tick(0,60);
    expect(td.mode==TD_ROAM&&td.job==TD_NONE&&td.u==900*16&&td.v==900*16&&td.cash==27,
           "past contract failure cannot turn a fresh free-roaming arrival into another failure result");

    reset_case();stop0_here=1;td.onfoot=1;td.job=0;td.left=2;td_job.kind=0;
    world_tick(J_B,1);
    for(unsigned i=0;i<6;i++){world_tick(J_RIGHT,1);world_tick(0,1);}
    world_tick(J_A,1);td.seconds=17;td.subsecond=59;world_tick(0,1);
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
        expect(memcmp(&td,&legacy,sizeof(td))==0,"v5 to v6 migration preserves active job, cash, clock, bitmap, transit and map fields");
        int zero=td.district==0&&td.park_district==0&&td.reserved==0;
        for(unsigned i=9;i<TD_COMPLETE_BYTES;i++)if(td.complete[i])zero=0;
        expect(zero,"migration extends the bitmap with zero bytes and supplies core districts");
        expect(td_save_address(0)[2]==5,"reading v5 alone does not overwrite its committed snapshot");
        td_save();
        expect(td_save_address(td_save_slot)[2]==6&&td_save_address(td_save_slot)[3]==58,
               "first save after migration writes the actual58-byte v6 record to the other slot");
        memset(&td,0,sizeof(td));expect(td_restore()&&memcmp(&td,&legacy,sizeof(td))==0,
               "upgraded current record restores all preserved legacy fields");
    }
    native_case();td_state_t stable=legacy_work(TD_RIDE);write_v5(0,&stable,88);
    expect(td_restore(),"interruption fixture starts from a genuine decoded v5 snapshot");
    UBYTE stable_image[sizeof(td_test_sram)];memcpy(stable_image,td_test_sram,sizeof(stable_image));
    td_state_t candidate=stable;candidate.cash+=111;candidate.complete[9]=1;candidate.done++;
    candidate.stage=4;candidate.left=119;candidate.ride_left=2;td=candidate;sram_writes=0;td_save();
    unsigned count=sram_writes;
    expect(count==sizeof(td)+9,"upgrade trace counts every real v6 payload and metadata store");
    for(volatile unsigned cut=1;cut<=count;cut++) {
        memcpy(td_test_sram,stable_image,sizeof(stable_image));
        expect(td_restore()&&memcmp(&td,&stable,sizeof(td))==0,"each upgrade interruption begins from retained v5 state");
        td=candidate;sram_writes=0;sram_interrupt_after=cut;sram_interrupt_enabled=1;
        if(setjmp(interrupted_save)==0){td_save();expect(0,"upgrade interruption must reach the configured real byte store");}
        sram_interrupt_enabled=0;memset(&td,0,sizeof(td));
        expect(td_restore(),"every interrupted v5 upgrade retains a committed recoverable record");
        expect(memcmp(&td,cut==count?&candidate:&stable,sizeof(td))==0,
               "v5 upgrade preserves complete old state until the final v6 commit byte");
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
            case 4:bad.complete[10]=1;bad.done=1;break;
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

static void apply_queued_scene(void) {
    expect(test_queued_district<TD_DISTRICT_COUNT,"scene fixture applies a genuinely queued district");
    test_current_district=test_queued_district;test_queued_district=TD_DISTRICT_NONE;
    memset(actors,0,sizeof(actors));actors_inactive_head=NULL;toronto_init();
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
        td.cash=333;td.seconds=137;td.subsecond=21;td.heading=portal->u==24?8:0;
        td.speed=foot?0:11;td_vx=portal->u==24?-144:144;td_vy=32;
        td_get_job(td.job,&td_job);td_set_target();
        UWORD old_u=td.u+(portal->u==24?16:-16);td_state_t before=td;
        WORD old_vx=td_vx,old_vy=td_vy;audio_mode=TD_AUDIO_EFFECTS;
        UBYTE crossed=td_cross_portal(old_u);
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
    expect(!td_cross_portal(25*16)&&!memcmp(&td,&before,sizeof(td))&&!td_transition_pending&&sram_writes==0,
           "failed scene queue leaves district, vehicle, job and persistent state unchanged");
    expect(td_vx==-144&&td_vy==32,"failed allocation does not drain either velocity axis");
    test_queue_fail=0;
    expect(td_cross_portal(25*16)&&td.district==1&&test_queue_calls==2,
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

static void test_first_frame_actors(void) {
    const UWORD locations[3][2]={{560,720},{800,64},{736,640}};
    for(unsigned district=0;district<3;district++) {
        native_case();td_session_live=1;test_current_district=td.district=td.park_district=district;
        td.u=(locations[district][0]+18)*16;td.v=locations[district][1]*16;
        td.park_u=locations[district][0]*16;td.park_v=td.v;td.onfoot=1;toronto_init();
        expect(actors_len==15&&PLAYER.pos.x==(td.u>>4)*32&&PLAYER.pos.y==(td.v>>4)*32,
               "each actual scene initializes all actors and courier coordinates before its first update");
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
    td_set_target();td.u=784*16;td.v=621*16;td.park_district=2;td.park_u=td.u;td.park_v=td.v;
    expect(td_target.reserved&TD_STOP_FOOT,"authored Lodge fixture carries its actual native foot-only delivery flag");
    expect(td_drivable(784,621)&&td_near(&td_target),"Lodge doorstep fixture is a genuine driveable car position inside the interaction radius");
    td_interact();expect(td.stage==2&&td.job==77&&td.msg==16,"car-door proximity cannot hand off a flagged park-and-walk parcel");
    td.onfoot=1;td.u=784*16;td.v=608*16;td_interact();
    expect(td.stage==3&&td.job==77,"on-foot Lodge courier can complete the same flagged handoff");
    td.stage=2;td_set_target();td.district=1;td.u=784*16;td.v=608*16;td_interact();
    expect(td.stage==2&&td.msg==6,"equal local coordinates in the wrong district cannot complete the Lodge objective");
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
    test_v5_migration_and_interrupted_upgrade();test_district_semantic_fallback();
    test_reciprocal_portals();test_queue_failure_and_remote_boot();test_first_frame_actors();
    test_walk_pace_dispatch_and_foot_delivery();
    printf("Host engine regressions: %u checks, %u failures. Hardware/emulator evidence remains separate.\n",checks,failures);
    return failures?1:0;
}
