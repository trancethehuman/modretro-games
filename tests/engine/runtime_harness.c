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
#include "street_names_under_test.c"
#include "places_under_test.c"
#undef td_get_stop
#undef td_get_job
#undef td_get_brief
#undef td_get_street
#undef td_get_west_street
#include "native_collision_fixture.h"

actor_t actors[24];
typedef char host_actor_pool_matches[(sizeof(actors)/sizeof(actors[0])==TD_ACTORS)?1:-1];
actor_t *actors_inactive_head;
UBYTE actors_len;
UWORD camera_x,camera_y,image_width=1024,image_height=976,sys_time;
UBYTE camera_settings,joy,joy_pressed;
WORD scroll_x,scroll_y;
UBYTE image_tile_width=128,image_tile_height=122;
BYTE camera_offset_x,camera_offset_y,camera_deadzone_x,camera_deadzone_y;
UBYTE td_test_sram[8192];

static unsigned failures,checks,stop_reads,ui_draws;
static unsigned audio_updates,audio_inits;
static UBYTE audio_mode,audio_active,audio_cue;
static UBYTE stop0_here;
static UBYTE authored_content;
static UBYTE test_current_district,test_queued_district,test_queue_fail;
static unsigned test_queue_calls,test_reset_calls;
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
    if (geometry==ALIGHT_BARRIER&&x==73&&y==92) return 15;
    if (geometry==NATIVE_GRID||geometry==ALIGHT_BARRIER) return x<native_widths[district]&&y<native_heights[district]?native_collision[district][y*native_widths[district]+x]:15;
    if (geometry==HIDDEN_NPC_TILE && x==48 && y==53) return 15;
    return 0;
}
UBYTE tile_at(UBYTE x,UBYTE y) {return district_tile(test_current_district,x,y);}
void actor_set_frames(actor_t *actor,UBYTE first,UBYTE end) {
    actor->frame=actor->frame_start=first; actor->frame_end=end;
}
/* Like GBVM, activation resets the idle animation (frame 0 onwards). */
void activate_actor(actor_t *actor) { actor->flags|=ACTOR_FLAG_ACTIVE;actor->frame=actor->frame_start=0;actor->frame_end=TD_SPRITE_FRAMES-1; }
void deactivate_actor(actor_t *actor) { actor->flags&=~ACTOR_FLAG_ACTIVE; }
void td_ui_init(void) { ui_draws++; }
void td_ui_draw(void) { ui_draws++; }
void td_ui_tick(void) {}
void td_ui_hud_tick(void) {}
void td_hud_places(void) {}
/* Radio calls are drawn by td_ui.c (atlas_ui_harness); here the runtime
 * only records which script it queued last. */
UBYTE radio_said=255,td_radio_script=255,td_radio_wanted;UWORD td_last_pay;
void td_radio_say(UBYTE script) { radio_said=script; }
static unsigned radio_contract=65535,radio_done=65535,radio_done_before=65535;
static UWORD radio_open_now,radio_open_before;
void td_radio_contract(UBYTE job,UBYTE part) { radio_contract=job*3u+part; }
void td_radio_done(UBYTE job,UBYTE done_before,UWORD open_before) {
    radio_contract=job*3u+2;radio_done=job;radio_done_before=done_before;radio_open_before=open_before;
}
UWORD td_radio_open(void) { return radio_open_now; }
void td_radio_tick(void) {}
/* Tile uploads: animated scenery writes one tile at a time. */
UBYTE VBK_REG;static unsigned bkg_uploads;static UBYTE bkg_last_first,bkg_last_bank;static const UBYTE *bkg_last_data;
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *tiles){bkg_uploads+=count;bkg_last_first=first;bkg_last_bank=VBK_REG;bkg_last_data=tiles;}
UBYTE td_radio_playing(void) { return radio_said; }
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
UBYTE td_get_street(UWORD u,UWORD v) { (void)u;(void)v;return 0; }
void td_get_stop(UBYTE index,td_stop_t *out) {
    if(authored_content){stop_reads++;td_authored_get_stop(index,out);return;}
    stop_reads++;memset(out,0,sizeof(*out));out->u=900;out->v=900;
    out->transit=index==0?1:0;
    if(index==0&&stop0_here) {out->u=400;out->v=450;}
}
void td_get_job(UBYTE index,td_job_t *out) {
    if(authored_content){td_authored_get_job(index,out);return;}
    (void)index;memset(out,0,sizeof(*out));out->count=2;
    out->vehicle=TD_NONE;out->after=TD_NONE;out->seconds=120;out->reward=150;out->route[1]=1;
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
    memset(&td,0,sizeof(td));memset(&td_job,0,sizeof(td_job));td_ped_flip=0;
    memset(actors,0,sizeof(actors));
    memset(&td_cursor,0,sizeof(td_cursor));memset(&td_target,0,sizeof(td_target));
    memset(td_test_sram,0,sizeof(td_test_sram));td_car_damage=0;td_car_colour=0;
    for(unsigned i=0;i<6;i++)td_traffic_bases[i]=TD_FRAME_PLAYER_CAR;
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
    stop0_here=authored_content=0;td_st_district=255;test_queue_calls=test_reset_calls=0;
    test_map_opens=test_map_updates=test_map_closes=0;
    test_map_active=test_map_buttons=test_map_pressed=test_map_camera_settings=0;
    test_map_camera_x=test_map_camera_y=0;
    td_save_slot=TD_NONE;td_save_seq=0;actors_inactive_head=NULL;
    sram_writes=sram_interrupt_after=0;sram_interrupt_enabled=0;
    geometry=CLEAR_GROUND;
    td_audio_init();audio_updates=audio_inits=0;
    for(unsigned i=0;i<6;i++) { td_traffic_u[i]=30000;td_traffic_v[i]=30000;td_traffic_leg[i]=0; }
    for(unsigned i=0;i<TD_PEDS;i++)td_ped_route[i]=TD_NONE;
    td_ped_refresh=1;td_ped_anchor_u=td.u>>4;td_ped_anchor_v=td.v>>4;
    td.vitality=100;td.ammo=0;td.wanted=td.heat=0;scroll_x=scroll_y=0;td_life_reset(0);lf_chaos=0;
}

static void native_case(void);
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
    td.u=300*16;td.v=393*16;td.heading=1;geometry=SOUTH_CURB;
    UWORD start=td.u;WORD minimum=td.speed;
    for(unsigned i=0;i<80;i++) {
        driving_tick(J_A);if(td.speed<minimum)minimum=td.speed;
        expect(td.v>>4<394,"glancing contact remains outside curb footprint");
    }
    expect(td.u>start+32*16,"glancing contact slides forward beside curb");
    expect(minimum>=baseline*3/4,"continuous glancing contact does not repeatedly drain speed");
    expect(td.speed>=baseline-2,"acceleration restores speed during continued curb sliding");
}

static void test_wall_and_brake(void) {
    reset_case();prime_car();td.u=393*16;td.v=450*16;geometry=EAST_WALL;
    UWORD before=td.u;driving_tick(J_A);
    expect(td.u==before,"head-on wall blocks forward movement");
    expect(td.speed<0&&td_vx<0&&td_vy==0,"a fast head-on wall impact throws the car back instead of passing through");
    geometry=CLEAR_GROUND;
    for(unsigned i=0;i<120;i++)driving_tick(J_B);
    expect(td.speed<0&&td.u<before,"braking and reversing recover away from wall");

    reset_case();WORD baseline=prime_car();
    for(unsigned i=0;i<20;i++)driving_tick(J_A|J_B);
    expect(td.speed<baseline,"A+B acts as a handbrake while moving");
    for(unsigned i=0;i<60&&td.speed;i++)driving_tick(J_A|J_B);
    expect(td.speed==0&&!td.onfoot,"the handbrake stops the car without engaging reverse");
    for(unsigned i=0;i<24;i++)driving_tick(J_A|J_B);
    expect(td.onfoot&&td_entry_timer,"holding A+B at rest leaves the vehicle");

    reset_case();UBYTE heading=td.heading;
    for(unsigned i=0;i<90;i++)driving_tick(J_RIGHT);
    expect(td.heading==heading&&td.speed==0,"steering never rotates a stationary car");
    for(unsigned i=0;i<12;i++)driving_tick(J_LEFT|J_A|J_B);
    expect(td.heading==heading&&!td.onfoot,"steering with the handbrake held at rest does not rotate the car");
    for(unsigned i=0;i<9;i++)driving_tick(J_B);
    expect(td.speed==0,"a short brake hold at rest does not lurch into reverse");
    for(unsigned i=0;i<40;i++)driving_tick(J_B|J_RIGHT);
    UBYTE swing=(UBYTE)(heading-td.heading)&15;
    expect(td.speed<0&&swing>=1&&swing<=8,"reversing with right steering swings the nose the other way");
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
    td.park_u=340*16;td.park_v=814*16;td.u=356*16;td.v=790*16;td.onfoot=1;
    expect(lf_drive(td.park_u>>4,td.park_v>>4),"rail fixture parked car has a usable footprint");
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
    td_traffic_u[0]=232*16;td_traffic_v[0]=288*16;UWORD before=td_traffic_u[0];
    td_traffic_step();expect(td_traffic_u[0]==before,"red traffic stops before the Bathurst intersection in world units");
    td.seconds=0;td_traffic_step();expect(td_traffic_u[0]>before,"green traffic leaves the stop line");

    td.seconds=8;td_traffic_u[0]=230*16+6;before=td_traffic_u[0];
    td_traffic_step();expect(td_traffic_u[0]>before,"red traffic does not stop at arbitrary eight-pixel intervals");

    td.seconds=0;td_traffic_u[4]=792*16;td_traffic_v[4]=376*16;before=td_traffic_v[4];
    td_traffic_step();expect(td_traffic_v[4]==before,"vertical red traffic stops before Dundas");
    td.seconds=8;td_traffic_step();expect(td_traffic_v[4]>before,"vertical green traffic resumes");

    reset_case();td.mode=TD_WAIT;td.onfoot=1;td_traffic_u[0]=100*16;before=td_traffic_u[0];sys_time=4;
    toronto_update();expect(td_traffic_u[0]>before,"traffic continues moving during unpaused transit waiting");
    expect(actors[9].pos.x||actors[9].pos.y,"pedestrians update during unpaused transit waiting");
}

static void test_city_routes_and_walking(void) {
    reset_case();geometry=NATIVE_GRID;
    expect(!lf_drive(120,796),"car footprint rejects a narrow solid rail under its centre");
    reset_case();geometry=NATIVE_GRID;td.u=793*16+2;td.v=730*16+12;
    td.heading=4;td.speed=21;td_vx=0;td_vy=336;
    for(unsigned i=0;i<16;i++)driving_tick(J_A);
    expect(td.speed>=21&&td.v>738*16,"held throttle clears a small quantised corner overlap without losing forward speed");
    expect(lf_drive(td.u>>4,td.v>>4),"corner slide retains a collision-valid car footprint");

    reset_case();geometry=EAST_WALL;td.u=393*16;td.v=450*16;td.heading=0;td.speed=24;td_vx=384;
    driving_tick(J_A);expect(td.speed<=0&&td.u==393*16,"corner assist cannot bypass a broad head-on wall");

    reset_case();geometry=NATIVE_GRID;toronto_init();td.mode=TD_ROAM;
    td.u=576*16;td.v=740*16;td.onfoot=0;
    int usable=1,continuous=1,on_asphalt=1;
    for(unsigned step=0;step<12000;step++) {
        UWORD old_u=td_traffic_u[5],old_v=td_traffic_v[5];
        td.seconds=step/60;td.subsecond=step%60;td_traffic_step();
        /* At most 8 per axis: diagonal legs round Queen's Park Crescent move both. */
        if(td_distance(old_u,td_traffic_u[5])>8||td_distance(old_v,td_traffic_v[5])>8)continuous=0;
        for(unsigned i=0;i<6;i++)if(!lf_drive(td_traffic_u[i]>>4,td_traffic_v[i]>>4))usable=0;
        if(tile_at(td_traffic_u[5]>>7,td_traffic_v[5]>>7))on_asphalt=0;
    }
    expect(continuous,"autonomous bus has continuous movement through clock changes and route loops");
    expect(on_asphalt,"the bus keeps to the asphalt, round Queen's Park Crescent too");
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
    {UBYTE first=td_ped_flip;td_pedestrians();expect(td_ped_flip!=first,"walkers alternate between laid-out and skipped updates");
     if(!td_ped_flip)td_pedestrians();
     actors[9].pos.x^=32;npc_u=actors[9].pos.x;td_pedestrians();
     expect(!td_ped_flip&&actors[9].pos.x==npc_u,"a skipped update leaves walkers where they were");
     td_pedestrians();expect(actors[9].pos.x!=npc_u,"the next update lays them out again");}

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
    expect(radio_said>=TD_RADIO_FAIL&&radio_said<TD_RADIO_FAIL+TD_RADIO_FAIL_COUNT,"the radio reports a failed contract");
    /* Contract calls: the client at the first pickup and on the delivery;
     * the radio learns what the delivery changed (count and open chapters
     * before it) to tell the story it moves on. */
    reset_case();td.job=7;td_job.count=3;td_target.u=td.u>>4;td_target.v=td.v>>4;radio_contract=65535;
    td_interact();expect(radio_contract==7*3+1,"the first pickup plays that contract's pickup call");
    reset_case();td.job=9;td.done=5;td.health=100;td.left=50;td_job.reward=100;radio_open_now=0x0F;radio_done=65535;td_finish(TRUE);
    expect(td.done==6&&radio_done==9&&radio_done_before==5&&radio_open_before==0x0F&&td_last_pay==110,
           "a new delivery hands the radio its contract, the count before it and the chapters open before it");
    reset_case();td.job=9;td.done=6;td.complete[1]=2;radio_done=65535;td_finish(TRUE);
    expect(radio_done==9&&radio_done_before==6&&td.done==6,"a replay leaves the count unchanged for the radio");
    reset_case();td.job=4;td.done=TD_QUESTS-1;radio_done=65535;td_finish(TRUE);
    expect(radio_done==4&&radio_done_before==TD_QUESTS-1&&td.done==TD_QUESTS,"the last contract reaches the radio with the full count");
    reset_case();td.job=4;radio_done=65535;radio_said=255;td_finish(FALSE);
    expect(radio_done==65535&&radio_said>=TD_RADIO_FAIL,"a failure is not a delivery");
    /* Taking a job plays its briefing; dismissing the title of a fresh game welcomes the courier. */
    /* The board browses offers in the active contract's copy: closing it
     * reloads the contract. */
    reset_case();authored_content=1;td.job=3;td_get_job(3,&td_job);td.mode=TD_BOARD;td.menu=10;td_get_job(10,&td_offer);
    joy=0;world_tick(J_B,2);authored_content=0;
    {td_job_t three;td_authored_get_job(3,&three);
     expect(td.mode==TD_ROAM&&!memcmp(&td_job,&three,sizeof(three)),"closing the board restores the active contract");}
    reset_case();td.job=TD_NONE;td.mode=TD_BOARD;td.menu=0;td_get_job(0,&td_offer);td.onfoot=0;radio_contract=65535;
    joy=joy_pressed=J_A;sys_time+=2;toronto_update();
    expect(td.job==0&&radio_contract==0,"accepting a contract plays its briefing call");
    reset_case();td.job=TD_NONE;td.done=0;td.mode=TD_HELP;td_resume_mode=TD_ROAM;radio_said=255;
    joy=joy_pressed=J_A;sys_time+=2;toronto_update();
    expect(td.mode==TD_ROAM&&radio_said==TD_RADIO_INTRO,"leaving the title on a fresh shift starts the welcome call");
    reset_case();td_message(5);expect(audio_cue==TD_AUDIO_IMPACT,"vehicle impacts queue their audio feedback");
}

static void test_bounded_corner_assist(void) {
    for(unsigned shift=1;shift<=7;shift++) {
        reset_case();geometry=SOUTHWEST_CORNER;td.u=(406-shift)*16;td.v=393*16;
        td.heading=4;td.speed=16;td_vx=0;td_vy=256;
        UWORD old_u=td.u,old_v=td.v;driving_tick(J_A);
        if(shift<=6) {
            expect(td.speed==16&&td.heading==4&&td.v>old_v,"small corner clearance retains heading, throttle and dominant travel");
            expect((unsigned)(td.u-old_u)==shift*16&&lf_drive(td.u>>4,td.v>>4),"corner assist chooses the nearest collision-valid lateral clearance");
        }else expect(td.speed<=0&&td.u==old_u&&td.v==old_v,"seven-pixel blocked corner exceeds assistance budget and remains solid");
    }
    const UBYTE prohibited[]={0,J_A|J_B,J_B};
    for(unsigned input=0;input<3;input++) {
        reset_case();geometry=SOUTHWEST_CORNER;td.u=405*16;td.v=393*16;td.speed=16;td_vy=256;joy=prohibited[input];
        expect(!lf_corner_slide(td.u,394*16),"coasting and braking do not invoke throttle corner assistance");
    }
    reset_case();geometry=SOUTHWEST_CORNER;td.u=405*16;td.v=393*16;td.speed=-6;td_vy=256;joy=J_A;
    expect(!lf_corner_slide(td.u,394*16),"reverse does not invoke forward corner assistance");
    td.speed=16;td_vx=td_vy=256;
    expect(!lf_corner_slide(td.u,394*16),"equal diagonal velocity has no arbitrary assistance axis");
    td_vx=0;td_vy=256;td_corner_used=1;
    expect(!lf_corner_slide(td.u,394*16),"catch-up steps cannot apply multiple lateral assists in one rendered update");
    reset_case();joy=J_A;td.speed=16;td_vy=256;td.v=968*16;
    expect(!lf_corner_slide(td.u,td.v+16),"corner assistance cannot push the car beyond the southern map bound");
}

static void test_street_life(void) {
    UBYTE found_bad=0;
    /* Car theft: A beside a road vehicle drags its driver out and takes it. */
    reset_case();td.onfoot=1;td.u=300*16;td.v=292*16;td.park_u=100*16;td.park_v=100*16;
    td_traffic_u[1]=300*16;td_traffic_v[1]=280*16;td_traffic_bases[1]=TD_FRAME_PLAYER_VAN;actors[3].frame=TD_FRAME_PLAYER_VAN;
    td_input_edge=1;driving_tick(J_A);
    expect(td_entry_timer&&td.park_u==300*16&&td.park_v==280*16&&td.vehicle==1,"A beside a van steals it and keeps its position");
    expect((td_tr_ctrl&2)&&tr_mode[1]==TR_GONE&&td_fx_kind==FX_RUNNER,"the stolen slot leaves traffic and its driver runs off");
    for(unsigned i=0;i<16;i++)driving_tick(0);
    expect(!td.onfoot&&td.u==300*16&&td.v==280*16,"the courier ends up driving the stolen vehicle");

    /* A punch knocks a walker down; a pistol shot uses a round. */
    reset_case();td.onfoot=1;td.u=300*16;td.v=300*16;td_walk_dir=0;td.ammo=3;
    actors[9].pos.x=307*32;actors[9].pos.y=300*32;actors[9].flags=0;td_ped_route[0]=0;
    td_life_foot_a();
    expect((td_ped_ovr&1)&&pk_mode[0]==PK_FLY,"a punch throws the walker in front of the courier");
    for(unsigned i=0;i<40;i++)td_life_tick();
    expect(pk_mode[0]==PK_DOWN,"a punched walker lands and stays down for a while");
    for(unsigned i=0;i<20;i++)td_life_tick();
    td_life_foot_b();
    expect(td.ammo==2&&td_fx_kind==FX_BULLET,"B on foot fires one round");
    td.ammo=0;for(unsigned i=0;i<20;i++)td_life_tick();td_life_foot_b();
    expect(td.msg==TD_MSG_NO_AMMO,"an empty pistol only reports no ammunition");

    /* Aiming: the pistol locks on to a walker roughly along the aim and the
     * tracer round travels eight pixels a tick until it hits. */
    reset_case();td.onfoot=1;td.mode=TD_ROAM;td.u=300*16;td.v=300*16;td.ammo=5;td_aim_dir=0;geometry=CLEAR_GROUND;
    for(UBYTE k=0;k<TD_PEDS;k++)actors[TD_ACTOR_PEDS+k].flags=ACTOR_FLAG_HIDDEN;
    actors[10].pos.x=340*32;actors[10].pos.y=310*32;actors[10].flags=0;td_ped_route[1]=0;
    actors[11].pos.x=300*32;actors[11].pos.y=250*32;actors[11].flags=0;td_ped_route[2]=0;
    td_life_aim();
    expect(td_aim_target==1,"aiming east locks on to the walker ahead, not the one behind the courier's shoulder");
    td_life_present();
    expect((actors[TD_ACTOR_RETICLE].flags&ACTOR_FLAG_ACTIVE)&&actors[TD_ACTOR_RETICLE].frame==TD_FRAME_RETICLE&&
           actors[TD_ACTOR_RETICLE].pos.x==actors[10].pos.x,"the lock-on marker sits on the target");
    td_aim_dir=6;td_life_aim();
    expect(td_aim_target==2,"turning to aim north switches the lock to the walker in that direction");
    td_aim_dir=4;td_life_aim();
    expect(td_aim_target==TD_NONE,"nothing to the west means no lock");
    td_aim_dir=0;td_life_foot_b();
    expect(td_fx_kind==FX_BULLET&&td.ammo==4&&fx_du>0&&fx_dv>0&&td_walk_dir==0,
           "a locked shot heads for the target and the courier turns to face it");
    actors[TD_ACTOR_FX].flags|=ACTOR_FLAG_DISABLED;td_life_present();
    expect(!(actors[TD_ACTOR_FX].flags&(ACTOR_FLAG_DISABLED|ACTOR_FLAG_HIDDEN))&&actors[TD_ACTOR_FX].frame_start>=TD_FRAME_TRACER&&
           actors[TD_ACTOR_FX].frame_start<TD_FRAME_TRACER+8,"the tracer shows at once even if GBVM had flagged the actor off screen");
    for(unsigned i=0;i<8&&td_fx_kind==FX_BULLET;i++)td_life_tick();
    expect((td_ped_ovr&2)&&pk_mode[1]==PK_FLY,"the tracer reaches the locked walker within a few ticks");
    /* Holding B keeps a lock while strafing; releasing it drops a lock
     * outside the aim cone. The cone reaches about 60 degrees. */
    td_aim_target=TD_NONE;td_aim_dir=6;td_life_aim();
    expect(td_aim_target==2,"the north walker is locked before strafing");
    td_aim_hold=1;td_aim_dir=0;td_life_aim();
    expect(td_aim_target==2,"holding B keeps the lock while the courier strafes away from the target");
    td_aim_hold=0;td_life_aim();
    expect(td_aim_target==TD_NONE,"releasing B drops a lock that is outside the aim cone");
    actors[11].pos.x=330*32;actors[11].pos.y=360*32;td_aim_dir=0;td_life_aim();
    expect(td_aim_target==2,"a walker about 60 degrees off the aim is within the lock cone");
    td.onfoot=0;td_life_aim();
    expect(td_aim_target==TD_NONE,"no lock-on while driving");

    /* Road vehicles get a random design and colour; the yellow van keeps its
     * slot downtown and a taxi is always yellow. */
    reset_case();
    for(UBYTE seed=0;seed<64;seed++){
        td.district=1;td_lf_new_look(2,seed);
        if(td_traffic_bases[2]==TD_FRAME_TRAFFIC_TAXI&&actors[4].move_speed!=TD_PAL_YELLOW)found_bad=1;
        if(actors[4].move_speed>7)found_bad=1;
    }
    expect(!found_bad,"traffic looks always use a valid palette offset and taxis stay yellow");
    td.district=0;td_lf_new_look(5,9);
    expect(td_traffic_bases[5]==TD_FRAME_PLAYER_VAN&&actors[7].move_speed==TD_PAL_YELLOW,"downtown keeps the yellow delivery van in slot 5");

    /* A moving car throws a struck walker; it loses a quarter of its speed. */
    reset_case();td.speed=20;td_vx=320;td.mode=TD_ROAM;td_ped_route[2]=0;
    actors[11].pos.x=(td.u>>4)*32;actors[11].pos.y=(td.v>>4)*32;actors[11].flags=0;
    td_life_peds(4);
    expect((td_ped_ovr&4)&&pk_mode[2]==PK_FLY&&pk_vu[2]>0&&td.speed==15,"a car strike throws the walker forward and slows the car");
    expect(td.msg==TD_MSG_PED,"a car strike reports the pedestrian");

    /* Officers seeing a crime raise attention; unseen chaos builds up slowly. */
    reset_case();td_lf_crime(CR_GUN);
    expect(td.wanted==0,"one unseen shot does not summon the police at once");
    for(unsigned i=0;i<10;i++)td_lf_crime(CR_GUN);
    expect(td.wanted==0,"eleven unseen shots are still not enough for a star");
    for(unsigned i=0;i<5;i++)td_lf_crime(CR_GUN);
    expect(td.wanted==1&&td.heat==TD_HEAT_SECONDS,"repeated unseen chaos eventually draws a star");
    reset_case();td_lf_crime(CR_COP);
    expect(td.wanted==1,"assaulting an officer adds one star");
    td_lf_crime(CR_COP_KILL);
    expect(td.wanted==3,"killing an officer brings at least three stars");
    reset_case();td.wanted=3;for(unsigned i=0;i<64;i++)td_lf_crime(CR_KILL);
    expect(td.wanted==4,"unseen chaos alone never raises attention past four stars");
    reset_case();td.wanted=1;actors[9].pos.x=(td.u>>4)*32;actors[9].pos.y=(td.v>>4)*32;actors[9].flags=0;td_ped_route[0]=5;
    td_lf_crime(CR_KILL);td_lf_crime(CR_KILL);td_lf_crime(CR_KILL);
    expect(td.wanted==3,"a witnessed killing raises attention one star at a time up to three");

    /* Slot 4 is an ordinary car until a pursuit needs it, and only turns into
     * the patrol car out of view; it arrives from beyond the screen edge. */
    reset_case();native_case();td.mode=TD_ROAM;td.wanted=1;td.heat=TD_HEAT_SECONDS;
    scroll_x=(td.u>>4)-80;scroll_y=(td.v>>4)-72;
    td_traffic_u[TD_POLICE_SLOT]=td.u+320;td_traffic_v[TD_POLICE_SLOT]=td.v;
    lf_recruit();
    expect(!lf_patrol&&!(td_tr_ctrl&(1<<TD_POLICE_SLOT)),"a civilian car in view never turns into the patrol car");
    td_traffic_u[TD_POLICE_SLOT]=td.u+300*16;lf_recruit();
    expect(lf_patrol&&tr_mode[TD_POLICE_SLOT]==TR_CHASE&&tr_timer[TD_POLICE_SLOT]==60&&
           !lf_on_screen(td_traffic_u[TD_POLICE_SLOT]>>4,td_traffic_v[TD_POLICE_SLOT]>>4),
           "out of view, slot 4 becomes the patrol car, waits a second and starts outside the screen");
    td_tick=0;td_life_present();
    expect(actors[2+TD_POLICE_SLOT].frame_start>=TD_FRAME_POLICE&&actors[2+TD_POLICE_SLOT].frame_start<TD_FRAME_POLICE+8,
           "the pursuing car shows the patrol livery");
    td_tick=8;td_life_present();
    expect(actors[2+TD_POLICE_SLOT].frame_start>=TD_FRAME_POLICE&&actors[2+TD_POLICE_SLOT].frame_start<TD_FRAME_POLICE+8&&
           TD_PALETTE(&actors[2+TD_POLICE_SLOT])==TD_PAL_RED,
           "a pursuing patrol car flashes its light bar by switching to the red palette");
    td_tick=0;td_life_present();
    expect(TD_PALETTE(&actors[2+TD_POLICE_SLOT])==TD_PAL_BLUE,"the flash returns to the blue patrol palette");
    td.wanted=0;lf_cars_tick();
    expect(tr_mode[TD_POLICE_SLOT]==TR_PARK&&lf_patrol,"a pursuit ending leaves the patrol car parked until it is out of view");
    lf_cars_tick();
    expect(!lf_patrol&&!(td_tr_ctrl&(1<<TD_POLICE_SLOT)),"out of view the patrol car rejoins traffic as an ordinary car");

    /* The patrol car is slower than the courier's car at full speed. */
    reset_case();td.wanted=5;td_lf_own_car(TD_POLICE_SLOT,TR_CHASE);lf_patrol=1;tr_timer[TD_POLICE_SLOT]=0;
    td_traffic_u[TD_POLICE_SLOT]=td.u-200*16;td_traffic_v[TD_POLICE_SLOT]=td.v;lf_axis=0;
    { UWORD u0=td_traffic_u[TD_POLICE_SLOT];for(unsigned i=0;i<60;i++){td_tick++;lf_cars_tick();}
      expect(td_traffic_u[TD_POLICE_SLOT]-u0<=60*16,"at five stars the patrol car covers under a pixel per update"); }

    /* Officers on foot are slower than a walking courier below four stars,
     * and an arrest needs sustained contact. */
    reset_case();td.onfoot=1;td.wanted=2;td_ped_ovr=1;pk_mode[0]=PK_CHASE;pk_look[0]=LF_LOOK_OFFICER;
    pk_u[0]=td.u-60*16;pk_v[0]=td.v;
    { UWORD u0=pk_u[0];for(unsigned i=0;i<60;i++){td_tick++;lf_peds_tick();}
      expect(pk_u[0]-u0<60*8,"a two-star officer is slower than a walking courier"); }
    reset_case();td.onfoot=1;td.wanted=1;td.mode=TD_ROAM;td_ped_ovr=1;pk_mode[0]=PK_CHASE;pk_look[0]=LF_LOOK_OFFICER;
    pk_u[0]=td.u;pk_v[0]=td.v;
    for(unsigned i=0;i<20;i++){td_tick++;td_life_tick();}
    expect(!lf_arrest&&td.mode==TD_ROAM,"brief contact with an officer is not an arrest");
    for(unsigned i=0;i<100;i++){td_tick++;td_life_tick();}
    expect(lf_arrest,"an officer holding the courier for about a second and a half makes an arrest");

    /* Shooting starts at four stars. */
    reset_case();td.onfoot=1;td.wanted=3;td.mode=TD_ROAM;td_ped_ovr=1;pk_mode[0]=PK_CHASE;pk_look[0]=LF_LOOK_OFFICER;
    pk_u[0]=td.u-40*16;pk_v[0]=td.v;geometry=CLEAR_GROUND;
    for(unsigned i=0;i<300;i++)lf_police_fire();
    expect(td.vitality==100,"three-star officers do not shoot");
    td.wanted=4;for(unsigned i=0;i<300;i++)lf_police_fire();
    expect(td.vitality<100&&td.vitality>=100-4*4,"four-star officers fire about every one and a half seconds for four damage");

    /* Attention cools when no officer is near, one star per twelve seconds. */
    reset_case();td.wanted=1;td.heat=2;td_traffic_u[TD_POLICE_SLOT]=30000;
    td_life_second();td_life_second();
    expect(td.wanted==0&&td.heat==0,"unseen courier loses the last star");

    /* Arrest: fine scales with stars, attention clears, half the ammo goes. */
    reset_case();td.wanted=2;td.cash=500;td.ammo=10;td.job=0;
    td_life_busted();
    expect(td.cash==450&&td.wanted==0&&td.ammo==5&&td.job==TD_NONE&&td_life_fine==50,"arrest charges $25 a star and clears attention");
    reset_case();td.wanted=3;td.cash=40;td_life_busted();
    expect(td.cash==0&&td_life_fine==40,"an arrest fine never takes cash below zero");

    /* Hospital: full vitality, bill capped at cash, core forecourt exit. */
    reset_case();td.vitality=0;td.cash=60;td.wanted=4;UWORD hu,hv;td_life_hospital(&hu,&hv);
    expect(td.vitality==100&&td.cash==0&&td.wanted==0&&td_life_fine==60,"hospital restores vitality and bills at most the cash held");
    reset_case();td.cash=200;td_life_hospital(&hu,&hv);
    expect(td.cash==140&&td_life_fine==60,"the hospital bill is at most sixty dollars");
    expect(lf_dist(hu>>4,TD_HOSPITAL_U)<=16&&lf_dist(hv>>4,TD_HOSPITAL_V)<=16,"recovery starts at the hospital forecourt");

    /* Supplies: twelve rounds and first aid for twenty dollars. */
    reset_case();td.cash=25;td.ammo=95;td.vitality=40;
    expect(td_life_buy()&&td.cash==5&&td.ammo==TD_AMMO_MAX&&td.vitality==100,"supplies refill ammunition up to the cap and heal");
    expect(!td_life_buy()&&td.cash==5&&td.msg==TD_MSG_NO_CASH,"supplies need twenty dollars");
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
    expect(td_save_address(td_save_slot)[2]==7,"legacy migration writes a current dual-slot record");

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
    td.seconds=2;world_tick(J_A,1);
    expect(td.mode==TD_WAIT&&td.transit_target==12,"fresh trip defaults to a different served destination than its origin");
    td.seconds=17;td.subsecond=59;world_tick(0,1);
    expect(td.mode==TD_RIDE&&td.cash==27,"fresh free-roaming trip boards and charges exactly one fare");
    world_tick(0,60);
    expect(td.mode==TD_ROAM&&td.job==TD_NONE&&td.u==900*16&&td.v==900*16&&td.cash==27,
           "past contract failure cannot turn a fresh free-roaming arrival into another failure result");

    reset_case();stop0_here=1;td.onfoot=1;td.job=0;td.left=2;td_job.kind=0;
    world_tick(J_B,1);
    for(unsigned i=0;i<5;i++){world_tick(J_RIGHT,1);world_tick(0,1);}
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
    td.u=td.park_u=td.safe_u=576*16;td.v=td.park_v=td.safe_v=740*16;
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
            /* Drive to the contract's own pickup (Union, or the museum for the passenger). */
            td_stop_t pickup;td_get_stop(td_job.route[0],&pickup);
            td.u=td.safe_u=td.park_u=pickup.u*16;td.v=td.safe_v=td.park_v=pickup.v*16;
            world_tick(0,0);world_tick(J_SELECT,1);
            expect(td.mode==TD_ROAM&&td.job==jobs[kind]&&td.stage==1&&td.health==100,
                   "an actual stopped pickup at the contract's first stop advances the accepted authored job into carrying");
        }
        if(lifecycle==2){td.job=TD_NONE;td.stage=9;td.health=37;}
        td.cooldown=td_turn_tick=td_tick=0;td.speed=24;td.heading=0;
        td_vx=384;td_vy=0;joy=joy_pressed=0;
        td.u=400*16;td.v=450*16;td.safe_u=td.u;td.safe_v=td.v;
        UBYTE damage=0;
        if(hazard==0) {
            /* Rear-ending an eastbound car of equal mass (restitution 3/4)
               shoves it ahead and keeps the courier behind it. */
            geometry=CLEAR_GROUND;td_traffic_u[0]=td.u+176;td_traffic_v[0]=td.v;td_traffic_bases[0]=TD_FRAME_PLAYER_CAR;actors[2].frame=td_traffic_bases[0];
            UWORD before_u=td.u;driving_tick(0);damage=12;
            expect(td.speed==10&&td_vx==160&&td.cooldown==30&&td.msg==5&&td.u==before_u&&(td_tr_ctrl&1)&&tr_pu[0]==22,
                   "a traffic impact shoves the other car, slows and warns an empty, carrying or retired vehicle");
        }else if(hazard==1) {
            geometry=EAST_WALL;td.u=393*16;td.safe_u=td.u;
            driving_tick(0);damage=kind==1?20:8;
            expect(td.u==393*16&&td.speed==-6&&td_vx==-96&&!td_vy&&td.cooldown==45&&td.msg==5,
                   "a broad wall still stops the vehicle with a rebound and applies collision cooldown before and after pickup");
        }else if(hazard==2) {
            geometry=EAST_WALL;td.u=393*16;td.safe_u=td.u;td.heading=1;td_vy=128;
            driving_tick(0);damage=kind==1?4:1;
            expect(td.u==393*16&&td.v>450*16&&td.speed==24&&!td_vx&&td_vy>0&&td.cooldown==30&&td.msg==5,
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
        expect(td_target.district==0&&td_target.u==576&&td_target.v==740,
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
        td_save();td_state_t saved=td;saved.mode=TD_ROAM;memset(&td,0,sizeof(td));
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
               "invalid approach-condition fixture is a genuine committed CRC-valid newer version6 record");
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
        expect(td.mode==TD_ROAM&&td.msg==4&&td.cash==transit_cases[service].fare-1&&td.ride_left==0,
               "a closed window checks the fare on confirmation instead of leaving the courier waiting for a refusal");
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
    old_word(old,40,state->safe_u);old_word(old,42,state->safe_v);old_word(old,44,1400);old_word(old,46,2200);
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
    /* Version 7 street fields replace v5/v6 atlas words with fresh defaults. */
    state.vitality=100;state.ammo=TD_AMMO_START;state.wanted=0;state.heat=0;state.onfoot=mode!=TD_ROAM;
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
        expect(td_save_address(td_save_slot)[2]==7&&td_save_address(td_save_slot)[3]==58,
               "first save after migration writes the actual58-byte v6 record to the other slot");
        memset(&td,0,sizeof(td));expect(td_restore()&&memcmp(&td,&legacy,sizeof(td))==0,
               "upgraded current record restores all preserved legacy fields");
    }
    native_case();td_state_t stable=legacy_work(TD_RIDE);write_v5(0,&stable,88);
    expect(td_restore(),"interruption fixture starts from a genuine decoded v5 snapshot");
    UBYTE stable_image[sizeof(td_test_sram)];memcpy(stable_image,td_test_sram,sizeof(stable_image));
    td_state_t candidate=stable;candidate.cash+=111;candidate.complete[9]=1;candidate.done++;
    candidate.stage=2;candidate.left=119;candidate.ride_left=2;td=candidate;sram_writes=0;td_save();
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

static void test_station_reach(void) {
    /* Every boarding stop of the scene is cached, including the last of
     * the core's seventeen; reach covers the sidewalk sign beside a stop. */
    for(UBYTE id=0;id<TD_STOPS;id++) {
        td_stop_t stop;native_case();td_get_stop(id,&stop);
        if(!stop.transit||!td_transit_can_origin(id))continue;
        test_current_district=td.district=stop.district;td.onfoot=1;
        td.u=stop.u*16;td.v=(stop.v+16)*16;
        char name[90];snprintf(name,sizeof(name),"station %u is offered from its sidewalk sign",id);
        expect(td_origin()==id,name);
    }
    native_case();td.u=300*16;td.v=300*16;expect(td_origin()==TD_NONE,"away from stations there is no TTC prompt");
}

static void test_district_semantic_fallback(void) {
    native_case();td.cash=111;td_save();td.cash=222;td_save();
    UBYTE newest=td_save_slot,image[sizeof(td_test_sram)];memcpy(image,td_test_sram,sizeof(image));
    expect(td_district_drivable(0,576,740)&&!td_district_drivable(1,576,740),
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

static void apply_queued_scene(void) {
    expect(test_queued_district<TD_DISTRICT_COUNT,"scene fixture applies a genuinely queued district");
    test_current_district=test_queued_district;test_queued_district=TD_DISTRICT_NONE;
    memset(actors,0,sizeof(actors));actors_inactive_head=NULL;toronto_init();
}

static void test_safe_transit_alighting(void) {
    /* Bloor->Union reproduces the real return onto the parked car. Add a
       second obstruction case to force the opposite safe walking side. */
    for(UBYTE traffic=0;traffic<2;traffic++) {
        transit_menu_case(6,12);td.subsecond=0;
        td_stop_t depot;td_get_stop(0,&depot);
        if(traffic) {
            td.park_u=640*16;td.park_v=640*16;
            td_traffic_u[0]=576*16;td_traffic_v[0]=740*16;
            td_traffic_u[1]=588*16;td_traffic_v[1]=740*16;
        }
        world_tick(J_A,1);world_tick(0,240);
        UWORD expected_u=(traffic?564:588)*16;
        expect(td.mode==TD_ROAM&&td.onfoot&&td.cash==27&&td.u==expected_u&&td.v==740*16&&
               td.safe_u==td.u&&td.safe_v==td.v&&td_foot_free(td.u,td.v)&&td_near(&depot),
               "Union returns alight beside the actual parked car or loaded traffic at a clear reachable stop-side point");
        td_state_t arrived=td;memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&arrived,sizeof(td)),
               "safe Union alighting commits its actual clear foot position and single paid fare");
        world_tick(traffic?J_LEFT:J_RIGHT,1);
        expect(td.u==expected_u+(traffic?-8:8)&&td.v==740*16&&td.onfoot&&td.cash==27,
               "the first ordinary walking step escapes the alighting obstacle instead of trapping the courier inside it");
    }

    /* Four close traffic centres reject all12px points but leave the18px
       fallback usable. The courier can walk back into handoff range. */
    transit_menu_case(6,12);td.mode=TD_RIDE;td.cash=27;td.ride_left=1;
    td_traffic_u[0]=579*16;td_traffic_v[0]=740*16;
    td_traffic_u[1]=573*16;td_traffic_v[1]=740*16;
    td_traffic_u[2]=576*16;td_traffic_v[2]=743*16;
    td_traffic_u[3]=576*16;td_traffic_v[3]=737*16;
    td_second();
    expect(td.mode==TD_ROAM&&td.u==594*16&&td.v==740*16&&td_foot_free(td.u,td.v),
           "blocked12px landings fall back to an actual clear18px position rather than the vehicle centre");
    for(unsigned i=0;i<8;i++)driving_tick(J_LEFT);
    td_stop_t depot;td_get_stop(0,&depot);
    expect(td.u==590*16&&td_near(&depot)&&td_foot_free(td.u,td.v),
           "the18px fallback can approach the real stop into interaction range without entering the parked-car or traffic boxes");

    /* A temporary crowd covers the centre and every12/18px endpoint. The
       paid trip remains saved at Bloor while its parcel genuinely expires. */
    transit_menu_case(6,12);td.mode=TD_RIDE;td.cash=27;td.ride_left=1;
    td.park_u=640*16;td.park_v=640*16;
    td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=2;td_set_target();
    const WORD blocked_u[5]={576,594,558,576,576},blocked_v[5]={740,740,740,758,722};
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
           td.u==origin_u&&td.v==origin_v&&td_target.district==0&&td_target.u==576&&td_target.v==740,
           "deadline expiry while alighting is obstructed clears the job and objective while preserving the paid retry");
    retry=td;memset(&td,0,sizeof(td));
    expect(td_restore()&&!memcmp(&td,&retry,sizeof(td)),
           "the expired paid retry restores with failure condition instead of losing its ride or charging another fare");
    for(unsigned i=0;i<6;i++){td_traffic_u[i]=30000;td_traffic_v[i]=30000;}
    td_second();
    expect(td.mode==TD_RESULT&&td.job==TD_NONE&&td.health==0&&td.cash==27&&td.u==576*16&&td.v==740*16&&
           td.safe_u==td.u&&td.safe_v==td.v&&td_near(&depot),
           "removing the obstruction completes the expired paid journey once at its clear destination before showing failure");
    td_state_t failed=td;memset(&td,0,sizeof(td));
    expect(td_restore()&&td.mode==TD_ROAM&&td.job==TD_NONE&&td.health==0&&td.cash==27&&
           td.u==failed.u&&td.v==failed.v,
           "a failed but safely alighted destination remains a valid persistent free-roam state");
    td=failed;world_tick(J_B,1);world_tick(J_RIGHT,1);
    expect(td.mode==TD_ROAM&&td.u==576*16+8&&td.v==740*16&&td.cash==27,
           "dismissed expired-trip results allow the first walking step from the clear destination");

    /* Keep the registered grid except one synthetic solid tile between
       Union and the east18px endpoint. Clear endpoints cannot bypass it. */
    transit_menu_case(6,12);geometry=ALIGHT_BARRIER;td.mode=TD_RIDE;td.cash=27;td.ride_left=1;
    td_traffic_u[0]=558*16;td_traffic_v[0]=740*16;
    td_traffic_u[1]=576*16;td_traffic_v[1]=758*16;
    td_traffic_u[2]=576*16;td_traffic_v[2]=722*16;
    expect(td_district_walkable(0,594,740)&&!td_district_walkable(0,588,740),
           "alighting barrier fixture has a clear distant endpoint with a real intervening collision tile");
    td_second();
    expect(td.mode==TD_RIDE&&td.ride_left==1&&td.cash==27&&td.u!=594*16,
           "alighting rejects a clear18px endpoint when its swept walking path crosses solid collision");
    geometry=NATIVE_GRID;td_second();
    expect(td.mode==TD_ROAM&&td.u==588*16&&td.v==740*16&&td.cash==27,
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
        UBYTE phase=target_id>origin_id?index*4:(7-index)*4;
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
        if(waiting)world_tick(0,30*60-td.subsecond);
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
        world_tick(0,td.ride_left*60-td.subsecond);
        expect(td.mode==TD_RIDE&&td.ride_left==1&&td.cash==27&&td.district==origin.district&&
               td.u==origin.u*16&&td.v==origin.v*16&&!td_transition_pending&&test_queue_calls==1&&
               test_queued_district==TD_DISTRICT_NONE,
               "failed Queen scene allocation retains the paid rider at the origin with one retry second");
        td_state_t retry=td;memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&retry,sizeof(td)),
               "failed Queen arrival commits a recoverable origin ride rather than an invalid remote paid state");
        actor_t old_actors[TD_ACTORS];UWORD old_traffic_u[6],old_traffic_v[6];
        memcpy(old_actors,actors,sizeof(actors));memcpy(old_traffic_u,td_traffic_u,sizeof(td_traffic_u));
        memcpy(old_traffic_v,td_traffic_v,sizeof(td_traffic_v));
        UBYTE old_tick=td_tick;UWORD old_second=td.seconds;
        test_queue_fail=0;world_tick(J_RIGHT,180);
        expect(td_transition_pending==1&&test_queue_calls==2&&test_queued_district==destination.district&&
               td.district==destination.district&&td.u==destination.u*16&&td.v==destination.v*16&&
               td.safe_u==td.u&&td.safe_v==td.v&&td.mode==(failing?TD_RESULT:TD_ROAM)&&td.cash==27,
               "Queen arrival retries the queue once and commits the selected destination with success or failure result");
        expect(td.seconds==old_second+1&&td_tick==old_tick&&!memcmp(actors,old_actors,sizeof(actors))&&
               !memcmp(td_traffic_u,old_traffic_u,sizeof(td_traffic_u))&&
               !memcmp(td_traffic_v,old_traffic_v,sizeof(td_traffic_v)),
               "queued Queen arrival stops subsequent clock and motion catchup before moving old-scene actors or traffic");
        td_state_t arrived=td;memset(&td,0,sizeof(td));
        expect(td_restore()&&td.mode==TD_ROAM&&td.district==destination.district&&td.cash==27&&
               td.u==arrived.u&&td.v==arrived.v&&td.health==arrived.health&&td.job==arrived.job,
               "alighted Queen saves validate at the actual destination, including a retired failed parcel");
        td=arrived;apply_queued_scene();
        expect(!td_transition_pending&&test_current_district==destination.district&&
               td.mode==(failing?TD_RESULT:TD_ROAM)&&td.cash==27&&td.onfoot&&
               PLAYER.pos.x==destination.u*32&&PLAYER.pos.y==destination.v*32,
               "loaded Queen destination presents the courier on foot and retains its intended arrival mode");
        expect(td.park_district==0&&td.park_u==576*16&&td.park_v==740*16,
               "cross-district Queen travel never teleports the car parked at Union");
    }
}

static void test_reciprocal_portals(void) {
    for(unsigned index=0;index<TD_PORTALS;index++)for(unsigned foot=0;foot<2;foot++) {
        const td_portal_t *portal=&td_portals[index];native_case();
        test_current_district=td.district=portal->from;td_session_live=1;td.onfoot=foot;
        td.u=portal->u*16;td.v=portal->v*16;
        td.park_district=foot?(portal->from?0:2):portal->from;
        td.park_u=foot?(td.park_district?736:576)*16:td.u;
        td.park_v=foot?(td.park_district?640:740)*16:td.v;
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
    expect(td_door_path(td.u,td.v,td.park_u,td.park_v)&&lf_drive(24,640),
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
    const UWORD locations[TD_DISTRICT_COUNT][2]={{576,740},{800,64},{736,640},{224,528}};
    for(unsigned district=0;district<TD_DISTRICT_COUNT;district++) {
        native_case();td_session_live=1;test_current_district=td.district=td.park_district=district;
        td.u=(locations[district][0]+18)*16;td.v=locations[district][1]*16;
        td.park_u=locations[district][0]*16;td.park_v=td.v;td.onfoot=1;toronto_init();
        expect(actors_len==TD_ACTORS&&PLAYER.pos.x==(td.u>>4)*32&&PLAYER.pos.y==(td.v>>4)*32,
               "each registered district initializes all actors and courier coordinates before its first update");
        expect(actors[8].pos.x==(td.park_u>>4)*32&&actors[8].pos.y==(td.park_v>>4)*32&&!(actors[8].flags&ACTOR_FLAG_HIDDEN),
               "first scene frame shows the locally parked vehicle at its real saved position");
        int traffic=1;unsigned visible=0;
        for(unsigned i=0;i<6;i++)
            if(actors[i+2].pos.x!=(td_traffic_u[i]>>4)*32||actors[i+2].pos.y!=(td_traffic_v[i]>>4)*32||
               !lf_drive(td_traffic_u[i]>>4,td_traffic_v[i]>>4))traffic=0;
        for(unsigned i=0;i<TD_PEDS;i++) {
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
    /* Story order: contract 9 (the neighbourhoods) waits for contract 7 to end
     * the first chapter's story, however many deliveries are in. */
    native_case();td_get_job(8,&td_offer);
    expect(td_offer.min_done==6&&td_offer.after==6,"the second chapter opens after six deliveries and contract 7");
    td.done=6;td.complete[0]=0x3F;td_ready_offer();expect(td.menu!=8,"dispatch does not offer a chapter its story has not reached");
    td.mode=TD_BOARD;td.menu=8;td_get_job(8,&td_offer);td.msg=0;joy=0;world_tick(J_A,2);
    expect(td.job==TD_NONE&&td.msg==3,"a contract the story has not reached is locked");
    native_case();td.done=6;td.complete[0]=0x7F;td.complete[0]&=~0x20;td.mode=TD_BOARD;td.menu=8;td_get_job(8,&td_offer);joy=0;world_tick(J_A,2);
    expect(td.job==8,"finishing contract 7 opens the next chapter");
    native_case();td.done=3;td.complete[0]=7;td_ready_offer();
    expect(td.menu==4&&td_offer.vehicle==TD_NONE,"dispatch after three car completions skips truck-only job04 for compatible relay05");
    td.vehicle=1;td_ready_offer();expect(td.menu==3&&td_offer.vehicle==1,"truck dispatch still offers the first unlocked truck contract");
    td.onfoot=1;td_ready_offer();expect(td.menu==4&&td_offer.vehicle==TD_NONE,"walking dispatch skips vehicle-required offers without hiding compatible packages");

    native_case();test_current_district=td.district=2;td.job=77;td_get_job(td.job,&td_job);td.stage=2;td.left=180;
    td_set_target();td.u=736*16;td.v=640*16;td.park_district=2;td.park_u=td.u;td.park_v=td.v;
    expect(td_target.reserved&TD_STOP_FOOT,"authored Lodge fixture carries its actual native foot-only delivery flag");
    expect(lf_drive(736,640)&&td_near(&td_target),"Lodge approach fixture is a genuine driveable parking point inside the interaction radius");
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
        expect(lf_drive(anchors[i][0],anchors[i][1])&&td_near(&td_target)&&
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
        td.vitality=77;td.ammo=33;td_set_target();
        camera_x=12000+district*16;camera_y=8000+district*16;camera_settings=0x5A;
        UWORD original_camera_x=camera_x,original_camera_y=camera_y;
        world_tick(J_START,300);
        expect(td.mode==TD_PAUSE&&td_resume_mode==TD_ROAM&&td.seconds==100&&td.left==199,
               "opening pause for the atlas preserves the active cross-district contract clock");
        td.menu=1;world_tick(J_A,300);
        expect(td.mode==TD_MAP&&test_map_opens==1&&test_map_active&&camera_settings==0,
               "actual pause choice opens the dedicated atlas UI exactly once");
        expect(td.vitality==77&&td.ammo==33,
               "atlas browsing does not reuse serialized courier fields");
        td_state_t frozen=td;td_job_t frozen_job=td_job;
        td_stop_t frozen_target=td_target,frozen_cursor=td_cursor;
        actor_t frozen_actors[TD_ACTORS];memcpy(frozen_actors,actors,sizeof(actors));
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


static void test_sidewalk_pickups(void) {
    /* Pickups come from the generated district table; the scene keeps two slots. */
    UBYTE start=td_pickup_start[0],slot=255,kind=td_pickup_kind[start];UWORD pu=td_pickup_u[start],pv=td_pickup_v[start];
    native_case();td.mode=TD_ROAM;td.onfoot=1;td.district=0;td_street_reset(1);
    td.u=pu*16;td.v=(pv+40)*16;td.vitality=50;td.ammo=10;td.cash=30;
    for(unsigned i=0;i<16;i++)td_pickups_present();
    for(UBYTE s=0;s<TD_PICKUP_SLOTS;s++)if(td_pickup_slot[s]==0)slot=s;
    expect(slot!=255&&td_pickup_su[slot]==pu&&td_pickup_sv[slot]==pv,"a nearby pickup takes an actor slot");
    expect(!(actors[TD_ACTOR_PICKUPS+slot].flags&ACTOR_FLAG_HIDDEN)&&
           actors[TD_ACTOR_PICKUPS+slot].frame_start==TD_FRAME_PICKUP_CASH+kind&&
           actors[TD_ACTOR_PICKUPS+slot].pos.x==pu*32,"a pickup shows its item frame at its pavement position");
    audio_cue=255;td.u=pu*16;td.v=pv*16;td_pickups_present();
    expect(td_pickup_slot[slot]==255&&audio_cue==TD_AUDIO_PICKUP&&
           (kind==0?td.cash==30+TD_PICKUP_CASH:kind==1?td.vitality==50+TD_PICKUP_FIRST_AID:td.ammo==10+TD_PICKUP_AMMO),
           "walking over a pickup collects it");
    expect(td.msg==(kind==0?TD_MSG_CASH:kind==1?TD_MSG_FIRST_AID:TD_MSG_AMMO),"collection reports the item");
    td_pickups_present();
    expect(actors[TD_ACTOR_PICKUPS+slot].flags&ACTOR_FLAG_HIDDEN,"a collected pickup disappears");
    td.u=(pu+300)*16;for(unsigned i=0;i<16;i++)td_pickups_present();
    td.u=pu*16;td.v=(pv+40)*16;for(unsigned i=0;i<16;i++)td_pickups_present();
    for(UBYTE s=0;s<TD_PICKUP_SLOTS;s++)expect(td_pickup_slot[s]!=0,"a recently collected pickup does not come back");
    /* First aid is left for later at full vitality, ammunition when full. */
    for(UBYTE i=0;i<td_pickup_count[0];i++){
        UBYTE k=td_pickup_kind[start+i];if(k==0||k==TD_PICKUP_PARCEL)continue;
        native_case();td.mode=TD_ROAM;td.onfoot=0;td.district=0;td_street_reset(1);
        td.u=td_pickup_u[start+i]*16;td.v=(td_pickup_v[start+i]+40)*16;td.vitality=100;td.ammo=TD_AMMO_MAX;
        for(unsigned n=0;n<16;n++)td_pickups_present();
        td.v=td_pickup_v[start+i]*16;td_pickups_present();
        slot=255;for(UBYTE s=0;s<TD_PICKUP_SLOTS;s++)if(td_pickup_slot[s]==i)slot=s;
        expect(slot!=255&&td.vitality==100&&td.ammo==TD_AMMO_MAX,"a full courier leaves first aid and ammunition in place");
        td.vitality=60;td.ammo=0;td_pickups_present();
        expect(k==1?td.vitality==100:td.ammo==TD_PICKUP_AMMO,"a car driving over a pickup collects it");
    }
    expect(geometry==NATIVE_GRID&&td.mode==TD_ROAM,"pickups never change mode or collision");
    /* Every table entry lies on pavement and its coarse coordinates match. */
    for(UBYTE d=0;d<TD_DISTRICT_COUNT;d++){
        expect(td_pickup_count[d]>=12&&td_pickup_count[d]<=TD_PICKUPS_PER_DISTRICT,"each district has a modest number of pickups");
        for(UBYTE i=0;i<td_pickup_count[d];i++){
            UWORD u=td_pickup_u[td_pickup_start[d]+i],v=td_pickup_v[td_pickup_start[d]+i];
            expect(td_pickup_uv8[2*(td_pickup_start[d]+i)]==(u>>3)&&td_pickup_uv8[2*(td_pickup_start[d]+i)+1]==(v>>3),"coarse pickup coordinates match");
            expect(native_collision[d][(v>>3)*TD_DISTRICT_TILE_WIDTH+(u>>3)]==16,"pickups lie on pavement");
        }
    }
}

/* Lost parcels, the spray bay and crowd panic (GTA-style street life). */
static void test_parcels_spray_panic(void) {
    UBYTE start=td_pickup_start[0],p=255,slot=255,i;
    for(i=0;i<td_pickup_count[0];i++)if(td_pickup_kind[start+i]==TD_PICKUP_PARCEL){p=i;break;}
    expect(p!=255&&td_parcel_of[start+p]==0,"the first district holds the first lost parcel");
    {UBYTE n[TD_DISTRICT_COUNT]={0};for(i=0;i<td_pickup_start[3]+td_pickup_count[3];i++)if(td_pickup_kind[i]==TD_PICKUP_PARCEL)n[td_parcel_of[i]/5]++;
     expect(n[0]==5&&n[1]==5&&n[2]==5&&n[3]==5,"five lost parcels hide in each district");}
    native_case();td.mode=TD_ROAM;td.onfoot=1;td.district=0;td_street_reset(1);td.cash=30;
    td.u=td_pickup_u[start+p]*16;td.v=(td_pickup_v[start+p]+40)*16;
    for(unsigned n=0;n<16;n++)td_pickups_present();
    for(UBYTE s=0;s<TD_PICKUP_SLOTS;s++)if(td_pickup_slot[s]==p)slot=s;
    expect(slot!=255&&actors[TD_ACTOR_PICKUPS+slot].frame_start==TD_FRAME_PARCEL,"a lost parcel shows as a parcel");
    radio_said=255;td.v=td_pickup_v[start+p]*16;td_pickups_present();
    expect(TD_DONE(TD_PARCEL_BIT)&&td_parcels_found()==1&&td.cash==30+TD_PARCEL_REWARD&&td.msg==TD_MSG_PARCEL&&radio_said==255,
           "finding a lost parcel records it, pays and shows the count");
    td.u=(td_pickup_u[start+p]+400)*16;for(unsigned n=0;n<64;n++)td_pickups_present();
    td_street_reset(1);td.u=td_pickup_u[start+p]*16;td.v=(td_pickup_v[start+p]+40)*16;for(unsigned n=0;n<32;n++)td_pickups_present();
    for(UBYTE s=0;s<TD_PICKUP_SLOTS;s++)expect(td_pickup_slot[s]!=p,"a found parcel never comes back, even after a reset");
    td.complete[TD_PARCEL_BIT>>3]=0x1E;td_street_reset(1);td.v=(td_pickup_v[start+p]+40)*16;
    for(unsigned n=0;n<16;n++)td_pickups_present();
    radio_said=255;td.v=td_pickup_v[start+p]*16;td_pickups_present();
    expect(td_parcels_found()==5&&radio_said==TD_RADIO_PARCELS,"every fifth parcel brings Rosa's count");
    memset(td.complete+(TD_PARCEL_BIT>>3),0xFF,2);td.complete[(TD_PARCEL_BIT>>3)+2]=0x0F;td.complete[TD_PARCEL_BIT>>3]&=~1;
    td.cash=0;td_street_reset(1);td.v=(td_pickup_v[start+p]+40)*16;for(unsigned n=0;n<16;n++)td_pickups_present();
    radio_said=255;td.v=td_pickup_v[start+p]*16;td_pickups_present();
    expect(td_parcels_found()==TD_PARCELS&&td.cash==TD_PARCEL_REWARD+TD_PARCEL_BONUS&&radio_said==TD_RADIO_PARCELS+3,
           "the last parcel pays the bonus");
    /* Found parcels survive a save; anything else above the contracts is corrupt. */
    {td_state_t saved=td;td.mode=TD_ROAM;td.job=TD_NONE;td_save();memset(&td,0,sizeof(td));
     expect(td_restore()&&td_parcels_found()==TD_PARCELS&&td.done==saved.done,"found parcels are saved with the contracts");
     td.complete[11]=1;expect(!td_valid_state(&td),"bits between the contracts and the parcels make a save invalid");
     td.complete[11]=0;td.complete[14]|=0x10;expect(!td_valid_state(&td),"bits past the last parcel make a save invalid");
     td.complete[14]&=0x0F;expect(td_valid_state(&td),"parcel bits alone keep a save valid");}
    /* The spray bay on King St West. */
#define TD_SPRAY_U td_spray_at[0][0]
#define TD_SPRAY_V td_spray_at[0][1]
    native_case();td.mode=TD_ROAM;td.district=0;td.onfoot=0;td.speed=0;td.cash=100;td.wanted=3;td.heat=20;td_car_damage=70;
    td.u=TD_SPRAY_U*16;td.v=TD_SPRAY_V*16;td_spray_state=0;radio_said=255;td_spray_check();
    expect(!td.wanted&&!td.heat&&td.cash==100-TD_SPRAY_PRICE&&!td_car_damage&&td.msg==TD_MSG_SPRAY&&radio_said==TD_RADIO_SPRAY,
           "stopping in the spray bay loses the police and repairs the car");
    td.cash=100;td_spray_check();expect(td.cash==100,"sitting in the bay pays once");
    td_car_colour=TD_PAL_YELLOW;td.u=(TD_SPRAY_U+300)*16;td_spray_check();td.u=TD_SPRAY_U*16;td_spray_check();
    expect(td_car_colour!=TD_PAL_YELLOW&&td_car_colour!=TD_PAL_NAVY&&td_car_colour!=TD_PAL_COURIER,"a stolen car comes out another colour");
    td.u=(TD_SPRAY_U+300)*16;td_spray_check();td.cash=10;td.wanted=2;td.u=TD_SPRAY_U*16;td_spray_check();
    expect(td.wanted==2&&td.msg==TD_MSG_NO_CASH,"no money, no paint");
    td.u=(TD_SPRAY_U+300)*16;td_spray_check();td.cash=100;td.speed=12;td.u=TD_SPRAY_U*16;td_spray_check();
    expect(td.wanted==2,"driving through the bay does nothing");
    td.speed=0;td.u=(TD_SPRAY_U+300)*16;td_spray_check();td.msg=0;td.u=(TD_SPRAY_U+80)*16;td_spray_check();
    expect(td.msg==TD_MSG_SPRAY_NEAR,"with stars on, the bay announces itself nearby");
    td.msg=0;td_spray_check();expect(!td.msg,"once");
    td.district=1;td.u=TD_SPRAY_U*16;td_spray_check();expect(td.wanted==2,"other scenes have no bay there");
    /* Every scene has its own bay. */
    for(UBYTE scene=1;scene<TD_SPRAY_BAYS;scene++){
        td.district=scene;td.cash=100;td.wanted=2;td.speed=0;td_spray_state=0;
        td.u=(td_spray_at[scene][0]+300)*16;td.v=td_spray_at[scene][1]*16;td_spray_check();
        td.u=td_spray_at[scene][0]*16;td_spray_check();
        expect(!td.wanted&&td.cash==100-TD_SPRAY_PRICE,"each scene's spray bay works");
    }
    td.district=TD_SPRAY_BAYS;td.wanted=2;td_spray_check();expect(td.wanted==2,"no bay outside the four scenes");
#undef TD_SPRAY_U
#undef TD_SPRAY_V
    /* Crowd panic: walkers in view near trouble run, officers stand. */
    native_case();td.mode=TD_ROAM;td.district=0;
    for(i=0;i<TD_PEDS;i++){td_ped_route[i]=i==2?5:8+i;actors[TD_ACTOR_PEDS+i].flags=0;actors[TD_ACTOR_PEDS+i].pos.x=(400+i*8)*32;actors[TD_ACTOR_PEDS+i].pos.y=400*32;}
    actors[TD_ACTOR_PEDS+7].pos.x=700*32;td_ped_ovr=0;
    td_lf_panic(400,400);
    expect((td_ped_ovr&1)&&pk_mode[0]==PK_FLEE&&!(td_ped_ovr&(1<<2))&&!(td_ped_ovr&(1<<7)),
           "gunfire sends nearby walkers running, not officers or walkers far away");
    for(i=0;i<TD_PEDS;i++){td_ped_ovr&=~(1<<i);pk_mode[i]=0;}
}

static void test_visible_transit(void) {
    UWORD u,v,seconds;UBYTE found=0;actor_t *bus=&actors[TD_ACTOR_TRANSIT];
    native_case();td.district=0;td.onfoot=1;td.mode=TD_WAIT;td.transit_origin=45;td.transit_target=46;
    expect(td_street_berth(45,0,&u,&v)==1&&!td_street_berth(45,1,&u,&v)&&!td_street_berth(13,0,&u,&v),
           "street berths belong to their district; Line 1 has none");
    td_street_berth(45,0,&u,&v);
    for(seconds=0;seconds<64&&!found;seconds++){td.seconds=seconds;if(td_transit_departure(45,46,seconds)==2)found=1;}
    expect(found,"fixture finds a departure two seconds away");
    td.subsecond=0;td_transit_present(0);
    expect(!(bus->flags&ACTOR_FLAG_HIDDEN)&&bus->frame_start==TD_FRAME_STREETCAR_E&&
           bus->pos.x==(u-((120u*120u)>>8))*32&&bus->pos.y==(v+8)*32,
           "an eastbound streetcar approaches its berth in the eastbound lane on schedule");
    td.seconds+=2;td_transit_present(0);
    expect(td_transit_departure(45,46,td.seconds)==0&&bus->pos.x==u*32,"the streetcar is at its berth when the boarding window opens");
    td.seconds-=6;td_transit_present(0);
    expect(bus->flags&ACTOR_FLAG_HIDDEN,"no vehicle is shown while the next departure is far away");
    td.mode=TD_RIDE;td_tv_begin(1,45);td_transit_present(16);
    expect(PLAYER.flags&ACTOR_FLAG_HIDDEN,"the courier is aboard during the ride");
    expect(!(bus->flags&ACTOR_FLAG_HIDDEN)&&bus->pos.x==(u+1)*32,"the streetcar pulls away east with the courier");
    td_transit_present(240);
    expect((bus->flags&ACTOR_FLAG_HIDDEN)&&!td_tv_phase,"the departed streetcar leaves the view");
    td.mode=TD_ROAM;td_transit_present(1);
    expect(!(PLAYER.flags&ACTOR_FLAG_HIDDEN),"the courier reappears after alighting");
    td_tv_begin(2,46);td_street_berth(46,0,&u,&v);td_transit_present(10);
    expect(!(bus->flags&ACTOR_FLAG_HIDDEN)&&bus->pos.x==u*32,"the vehicle dwells at the destination berth after arrival");
    td.mode=TD_WAIT;td.transit_origin=10;td.transit_target=21;
    for(seconds=0,found=0;seconds<64&&!found;seconds++){td.seconds=seconds;if(!td_transit_departure(10,21,seconds))found=1;}
    td_transit_present(0);td_street_berth(10,0,&u,&v);
    expect(found&&bus->frame_start==TD_FRAME_FERRY_S&&bus->pos.x==u*32&&bus->pos.y==(v+TD_ANCHOR_FERRY_S_DY)*32,
           "the island ferry waits off the terminal dock");
    td.mode=TD_WAIT;td.transit_origin=12;td.transit_target=17;td_transit_present(0);
    expect(bus->flags&ACTOR_FLAG_HIDDEN,"the subway is never drawn on the street");
    expect(td_transit_heading(45,46)==TD_HEADING_EAST&&td_transit_heading(46,45)==TD_HEADING_WEST&&
           td_transit_heading(10,21)==TD_HEADING_SOUTH&&td_transit_heading(21,10)==TD_HEADING_NORTH&&
           td_transit_heading(80,19)==TD_HEADING_EAST&&td_transit_heading(45,45)==TD_TRANSIT_NONE,
           "journey headings follow route order and ferry direction");
}


static void test_ambient_traffic(void) {
    /* A vehicle far out of view comes back onto its own loop just beyond the
     * screen, heading toward the courier; one in view is left alone. */
    for(UBYTE district=1;district<TD_DISTRICT_COUNT;district++){
        UWORD u,v;UBYTE leg,moved=0;td_traffic_sample_t sample;
        native_case();td.district=test_current_district=district;td.mode=TD_ROAM;
        td_world_traffic_init(district,td_traffic_u,td_traffic_v,td_traffic_leg,td_traffic_samples);
        for(UBYTE i=0;i<6;i++){
            const UWORD (*path)[2]=td_west_traffic[district-1][i];
            td.u=path[0][0]*16;td.v=path[0][1]*16;scroll_x=path[0][0]-80;scroll_y=path[0][1]-72;
            if(!td_world_traffic_recycle(district,i,td.u>>4,td.v>>4,112,96,&u,&v,&leg,&sample))continue;
            moved++;
            expect((lf_dist(u>>4,td.u>>4)>=112||lf_dist(v>>4,td.v>>4)>=96)&&lf_dist(u>>4,td.u>>4)+lf_dist(v>>4,td.v>>4)<=280,
                   "a recycled vehicle starts just outside the view");
            expect(leg<td_west_traffic_counts[district-1][i]&&sample.u==path[leg][0]*16&&sample.v==path[leg][1]*16,
                   "a recycled vehicle continues along its own loop");
            expect(lf_drive(u>>4,v>>4),"a recycled vehicle starts on drivable ground");
        }
        expect(moved>=3,"most loops pass close enough to bring vehicles back");
    }
    native_case();td.district=0;td.mode=TD_ROAM;scroll_x=(td.u>>4)-80;scroll_y=(td.v>>4)-72;
    for(UBYTE i=0;i<6;i++){td_traffic_u[i]=(td.u>>4)>500?40*16:1000*16;td_traffic_v[i]=td_core_lane_v[i&3];}
    td_traffic_u[1]=td.u+20*16;td_traffic_v[1]=td.v;lf_amb=1;lf_ambient();
    expect(td_traffic_u[1]==td.u+20*16,"a vehicle in view is never moved");
    lf_amb=3;lf_ambient();
    expect(lf_dist(td_traffic_v[3]>>4,640)<=8&&!lf_on_screen(td_traffic_u[3]>>4,td_traffic_v[3]>>4)&&
           lf_dist(td_traffic_u[3]>>4,td.u>>4)<=280,"a far core car rejoins its avenue just outside the view");
    td_traffic_u[0]=40*16;lf_amb=0;lf_ambient();
    expect(td_traffic_u[0]==40*16,"an avenue far from the courier keeps its car where it is");
    td_tr_ctrl=4;td_traffic_u[2]=1000*16;lf_amb=2;lf_ambient();
    expect(td_traffic_u[2]==1000*16,"an owned vehicle is left to street life");
}

static unsigned hour_seconds(unsigned minutes) {
    /* Play seconds (mod one game day) that show the given time of day. */
    unsigned phase=(minutes*32+44)/45;return (phase+TD_DN_DAY_SECONDS-TD_DN_START)%TD_DN_DAY_SECONDS;
}
/* Animated scenery: water and screen tiles found in a scene's tilesets get
 * the next frame a few times a second. */
static void test_scenery(void) {
    UBYTE other[16];
    expect(td_scenery_vram_tile(5,163)==5&&td_scenery_vram_tile(128,163)==157&&td_scenery_vram_tile(162,163)==191&&
           td_scenery_vram_tile(130,200)==130,"scenery tiles are placed where GB Studio loads tileset entries");
    td_scenery_find();expect(!td_scenery_found[0]&&!td_scenery_found[1]&&!td_scenery_found[2],"a scene without tilesets has no scenery tiles");
    memset(other,0x55,sizeof(other));td_scenery_match(other,7,163,0);
    expect(!td_scenery_found[0],"other tiles are not scenery");
    td_scenery_match(td_scenery_frames[0][3],130,163,1);td_scenery_match(td_scenery_frames[0][TD_SCENERY_SCREEN+2],12,163,0);
    expect(td_scenery_found[0]==(1u<<3)&&td_scenery_found[(TD_SCENERY_SCREEN+2)>>3]==(1u<<((TD_SCENERY_SCREEN+2)&7))&&
           td_scenery_bank1[0]==(1u<<3)&&td_scenery_slot[3]==159&&td_scenery_slot[TD_SCENERY_SCREEN+2]==12,
           "the scene's water and screen tiles are recorded with their VRAM tile and bank");
    /* A new frame starts with the clock (every 16 frames); each update arms
     * eight tiles that the vertical-blank handler copies: three for all. */
    bkg_uploads=0;sys_time=16;td.mode=TD_ROAM;td_scenery_tick();
    expect(bkg_uploads==0,"an update only arms the copy");
    td_scenery_vbl();
    expect(bkg_uploads==1&&bkg_last_first==159&&bkg_last_bank==1&&bkg_last_data==td_scenery_frames[1][3]&&!VBK_REG,
           "the vertical blank draws the first eight tiles into their VRAM tile and bank and restores the VRAM bank");
    td_scenery_vbl();expect(bkg_uploads==1,"an unarmed vertical blank does nothing");
    td_scenery_tick();td_scenery_vbl();expect(bkg_uploads==1,"the next chunk follows");
    td_scenery_tick();td_scenery_vbl();
    expect(bkg_uploads==2&&bkg_last_first==12&&bkg_last_bank==0&&bkg_last_data==td_scenery_frames[1][TD_SCENERY_SCREEN+2],
           "the third reaches the screen");
    td_scenery_tick();td_scenery_vbl();expect(bkg_uploads==2,"then nothing until the next frame");
    VBK_REG=1;sys_time+=16;td_scenery_tick();td_scenery_vbl();expect(VBK_REG==1,"the handler keeps the interrupted VRAM bank");
    VBK_REG=0;sys_time+=16;td_scenery_tick();td.mode=TD_MAP;td_scenery_vbl();td.mode=TD_ROAM;
    expect(bkg_uploads==3,"nothing is drawn while the map owns the tiles");
    sys_time+=16;for(unsigned k=0;k<3;k++){td_scenery_tick();td_scenery_vbl();}
    expect(bkg_last_data==td_scenery_frames[0][TD_SCENERY_SCREEN+2],"the four frames repeat");
    reset_case();td.mode=TD_ROAM;td_scenery_find();td_scenery_match(td_scenery_frames[0][2],12,163,0);
    bkg_uploads=0;sys_time=48;toronto_update();td_scenery_vbl();
    expect(bkg_uploads==1,"the city redraws its scenery at the next vertical blank");
    sys_time+=4;toronto_update();td_scenery_vbl();expect(bkg_uploads==1,"and not on every update");
    td.mode=TD_PAUSE;sys_time+=16;toronto_update();td_scenery_vbl();expect(bkg_uploads==1,"menus hold it still (the map owns those tiles)");
    td_scenery_find();
}
static void test_day_night(void) {
    reset_case();td.seconds=0;
    expect(td_daynight_apply(TD_DN_FORCE)&&td_daynight_set==TD_DN_DAY_SET&&!td_daynight_lights,"a new game starts in daylight");
    expect(td_daynight_minutes()==8*60,"play second 0 reads 08:00");
    expect(!td_daynight_apply(0),"an unchanged step leaves the palettes alone");
    expect(td_daynight_apply(TD_DN_FORCE),"a forced apply always copies (scene init and title exit)");
    td.seconds=(UWORD)hour_seconds(22*60);td_daynight_apply(0);
    expect(td_daynight_set==TD_DN_NIGHT_SET&&td_daynight_lights,"22:00 is night with headlamps on");
    td.seconds=(UWORD)hour_seconds(3*60);td_daynight_apply(0);
    expect(td_daynight_set==TD_DN_NIGHT_SET,"night lasts past midnight");
    td.seconds=(UWORD)hour_seconds(13*60);td_daynight_apply(0);
    expect(td_daynight_set==TD_DN_DAY_SET&&!td_daynight_lights,"midday uses the registered scene palettes");
    unsigned changes=0;UBYTE last=255,distinct[TD_DN_SETS]={0};
    for(unsigned s=0;s<TD_DN_DAY_SECONDS;s++){
        td.seconds=(UWORD)s;td_daynight_apply(0);
        if(td_daynight_set!=last){changes++;last=td_daynight_set;}
        distinct[td_daynight_set]=1;
        expect(td_daynight_minutes()<24*60,"the clock stays within one day");
    }
    unsigned used=0;for(unsigned i=0;i<TD_DN_SETS;i++)used+=distinct[i];
    expect(used==TD_DN_SETS&&changes<=TD_DN_STEPS,"every palette set is reached and steps change at most every 16 seconds");
    td.seconds=TD_DN_DAY_SECONDS-1;td_daynight_apply(0);last=td_daynight_set;td.seconds=TD_DN_DAY_SECONDS;td_daynight_apply(0);
    expect(td_daynight_minutes()==8*60,"the day repeats every 1024 play seconds");
    /* Scene init forces the time of day; a second tick re-applies it. */
    native_case();td.seconds=(UWORD)hour_seconds(22*60);td_session_live=1;toronto_init();
    expect(td_daynight_set==TD_DN_NIGHT_SET,"a scene opens in the current time of day");
}
static void anim_present_ticks(unsigned n) {for(unsigned i=0;i<n;i++){td_tick++;td_anim_update();}}
static void test_animation(void) {
    native_case();td_session_live=1;td.seconds=0;toronto_init();td.mode=TD_ROAM;
    actor_t *p0=&actors[TD_ACTOR_PARTS],*p1=&actors[TD_ACTOR_PARTS+1];
    expect(!(p0->flags&ACTOR_FLAG_ACTIVE)&&!(p1->flags&ACTOR_FLAG_ACTIVE),"idle effect actors stay out of the active actor list");
    /* A collection pop rises 16 px and blinks out, then frees its actor. */
    td_anim_spawn(TD_PART_POP,TD_FRAME_PARCEL,500,600);
    anim_present_ticks(1);
    expect((p0->flags&ACTOR_FLAG_ACTIVE)&&!(p0->flags&ACTOR_FLAG_HIDDEN)&&p0->frame_start==TD_FRAME_PARCEL&&p0->frame==TD_FRAME_PARCEL,"a pop shows its icon, not the idle frame activation resets to");
    td_anim_spawn(TD_PART_SMOKE,0,510,600);td_anim_spawn(TD_PART_SMOKE,0,520,600);
    expect(td_anim_parts[0].kind==TD_PART_POP,"smoke never replaces a collection pop");
    anim_present_ticks(TD_PART_POP_TICKS);
    expect(td_anim_parts[0].y==(600-16)*32,"a pop rises 16 px");
    expect(!(p0->flags&ACTOR_FLAG_ACTIVE)&&!(p1->flags&ACTOR_FLAG_ACTIVE),"expired particles leave the active list");
    /* Braking hard at speed leaves tyre smoke behind the car. */
    native_case();td_session_live=1;td.seconds=0;toronto_init();td.mode=TD_ROAM;td.onfoot=0;geometry=CLEAR_GROUND;
    td.u=400*16;td.v=450*16;
    for(unsigned i=0;i<120;i++){driving_tick(J_A);td_anim_update();}
    expect(td.speed>10,"the car reaches speed");
    memset(td_anim_parts,0,sizeof(td_anim_parts));
    for(unsigned i=0;i<2;i++){driving_tick(J_B);td_anim_update();}
    expect(td_anim_parts[0].time&&td_anim_parts[0].kind==TD_PART_SMOKE&&td_anim_parts[0].x<(td.u>>4)*32,"hard braking leaves smoke behind an eastbound car");
    /* A launch from rest puffs exhaust; walking makes no smoke. */
    native_case();td_session_live=1;td.seconds=0;toronto_init();td.mode=TD_ROAM;td.onfoot=0;geometry=CLEAR_GROUND;
    td.u=400*16;td.v=450*16;memset(td_anim_parts,0,sizeof(td_anim_parts));
    for(unsigned i=0;i<8;i++){driving_tick(J_A);td_anim_update();}
    expect(td_anim_parts[0].kind==TD_PART_PUFF||td_anim_parts[1].kind==TD_PART_PUFF,"a launch from rest puffs exhaust");
    td.onfoot=1;td.speed=0;memset(td_anim_parts,0,sizeof(td_anim_parts));
    for(unsigned i=0;i<40;i++){driving_tick(J_RIGHT);td_anim_update();}
    expect(!td_anim_parts[0].time&&!td_anim_parts[1].time,"walking makes no smoke");
    /* Action poses replace the walking frame for a few ticks. */
    td_anim_pose(TD_FRAME_COURIER_PUNCH,10);td_walk_dir=2;driving_tick(J_DOWN);
    expect(PLAYER.frame_start==TD_FRAME_COURIER_PUNCH+2,"a punch shows the courier's punch pose");
    for(unsigned i=0;i<12;i++)driving_tick(0);
    expect(PLAYER.frame_start==TD_FRAME_COURIER_WALK+4,"the courier returns to the walking frames");
    /* Headlamps only at night in a vehicle; the camera looks ahead. */
    native_case();td_session_live=1;td.seconds=0;toronto_init();td.mode=TD_ROAM;td.onfoot=0;geometry=CLEAR_GROUND;
    td.u=400*16;td.v=450*16;
    for(unsigned i=0;i<150;i++){driving_tick(J_A);td_anim_update();}
    expect(PLAYER.frame_start==TD_FRAME_PLAYER_CAR,"no headlamps by day");
    expect(camera_offset_x<=-20&&camera_offset_y==0,"the camera looks ahead of an eastbound car");
    td.seconds=(UWORD)hour_seconds(22*60);td_daynight_apply(0);driving_tick(J_A);
    expect(PLAYER.frame_start==TD_FRAME_PLAYER_CAR_LIT,"night headlamps light the road ahead of the car");
    expect(actors[8].frame_start<TD_FRAME_PLAYER_CAR_LIT,"a parked car keeps its lamps off");
    td.vehicle=1;td.heading=4;driving_tick(J_A);
    expect(PLAYER.frame_start==TD_FRAME_PLAYER_CAR_LIT+8+2,"lit frames follow the vehicle and heading");
    td.vehicle=0;td.onfoot=1;td.speed=0;driving_tick(0);
    expect(PLAYER.frame_start>=TD_FRAME_COURIER_WALK&&PLAYER.frame_start<TD_FRAME_COURIER_WALK+8,"headlamps go out on foot");
    for(unsigned i=0;i<80;i++){td_tick++;td_anim_update();}
    expect(camera_offset_x==0&&camera_offset_y==0,"the camera recentres on foot");
}


static void test_car_damage(void) {
    /* A hard wall strike wears the car by its speed; vans take half. */
    reset_case();prime_car();td.u=393*16;td.v=450*16;geometry=EAST_WALL;
    UBYTE speed=(UBYTE)td.speed;driving_tick(J_A);
    expect(td_car_damage==speed,"a wall strike at speed damages the car by that speed");
    reset_case();td.vehicle=1;prime_car();td.u=393*16;td.v=450*16;geometry=EAST_WALL;
    speed=(UBYTE)td.speed;driving_tick(J_A);
    expect(td_car_damage==(speed+1)/2,"a van shrugs off half the damage");
    /* Past seventy the engine fails: a warning, smoke and a lower top speed. */
    reset_case();td_car_damage=60;prime_car();td.u=393*16;td.v=450*16;geometry=EAST_WALL;driving_tick(J_A);
    expect(td_car_damage>=TD_DAMAGE_FAIL&&td.msg==TD_MSG_SMOKING,"crossing the failing threshold warns of a smoking engine");
    geometry=CLEAR_GROUND;td.cooldown=0;td.speed=0;td_vx=td_vy=0;
    for(unsigned i=0;i<400;i++)driving_tick(J_A);
    expect(td.speed==18,"a failing car tops out at three quarters of its speed");
    native_case();td_session_live=1;td.seconds=0;toronto_init();td.mode=TD_ROAM;td.onfoot=0;geometry=CLEAR_GROUND;
    td_car_damage=TD_DAMAGE_FAIL;memset(td_anim_parts,0,sizeof(td_anim_parts));
    for(unsigned i=0;i<40;i++){td_tick++;td_anim_update();}
    expect(td_anim_parts[0].time&&td_anim_parts[0].kind==TD_PART_SMOKE,"a failing engine smokes from the bonnet");
    /* At a hundred the car is wrecked and only crawls until repaired. */
    reset_case();td_car_damage=95;prime_car();td.u=393*16;td.v=450*16;geometry=EAST_WALL;driving_tick(J_A);
    expect(td_car_damage==TD_DAMAGE_WRECK&&td.msg==TD_MSG_WRECKED,"a wrecked car says so");
    geometry=CLEAR_GROUND;td.speed=0;td_vx=td_vy=0;for(unsigned i=0;i<200;i++)driving_tick(J_A);
    expect(td.speed==6,"a wreck crawls");
    td.cash=40;expect(td_life_buy()&&td_car_damage==0,"supplies repair the car");
    /* A stolen car starts fresh in its own paint. */
    reset_case();td_car_damage=80;td.onfoot=1;td.u=300*16;td.v=292*16;td.park_u=100*16;td.park_v=100*16;
    td_traffic_u[1]=300*16;td_traffic_v[1]=280*16;td_traffic_bases[1]=TD_FRAME_PLAYER_CAR;actors[3].move_speed=TD_PAL_TEAL;
    td_input_edge=1;driving_tick(J_A);
    expect(td_car_damage==0&&td_car_colour==TD_PAL_TEAL,"a stolen car is undamaged and keeps its colour");
}

/* Navigation names from the generated tables (scripts/create_places.py). */
static void place_names(UBYTE district,UWORD u,UWORD v,UBYTE area,char *a,char *m,char *j) {
    UBYTE ids[3]={area,255,255};td.district=district;td_get_places(u,v,ids);
    a[0]=m[0]=j[0]=0;
    if(ids[0]!=TD_PLACE_NONE)td_get_place_name(TD_PLACE_AREA,ids[0],a);
    if(ids[1]!=TD_PLACE_NONE)td_get_place_name(TD_PLACE_MARK,ids[1],m);
    if(ids[2]!=TD_PLACE_NONE)td_get_place_name(TD_PLACE_JUNCTION,ids[2],j);
}
static void test_places(void) {
    char a[21],m[21],j[21],s[19];UBYTE ids[3]={255,255,255},kensington;
    place_names(0,384,400,255,a,m,j);
    expect(!strcmp(j,"SPADINA & DUNDAS"),"Spadina and Dundas is named as Torontonians say it");
    place_names(0,370,350,255,a,m,j);expect(!strcmp(a,"KENSINGTON MARKET"),"Kensington Market lies west of Spadina above Dundas");
    td.district=0;td_get_places(370,350,ids);kensington=ids[0];ids[1]=ids[2]=255;
    td_get_places(392,350,ids);expect(ids[0]==kensington,"crossing Spadina's centre line keeps the neighbourhood for a moment");
    td_get_places(420,350,ids);td_get_place_name(TD_PLACE_AREA,ids[0],a);expect(!strcmp(a,"CHINATOWN"),"then Chinatown");
    place_names(0,712,800,255,a,m,j);expect(!strcmp(m,"ST LAWRENCE MARKET"),"St Lawrence Market is announced from Front St");
    place_names(0,512,224,255,a,m,j);expect(!strcmp(a,"QUEENS PARK")&&!strcmp(m,"ONTARIO LEGISLATURE"),"Queen's Park and the Legislature");
    place_names(0,700,920,255,a,m,j);expect(!strcmp(a,"CENTRE ISLAND"),"the Islands are named one by one");
    place_names(3,544,64,255,a,m,j);expect(!strcmp(a,"GREEKTOWN")&&!strcmp(j,"PAPE & DANFORTH"),"Pape and Danforth in Greektown");
    place_names(1,864,600,255,a,m,j);expect(!strcmp(a,"PARKDALE"),"Parkdale lies south of Queen");
    place_names(2,700,450,255,a,m,j);expect(!strcmp(a,"HIGH PARK"),"High Park");
    td.district=0;td_get_street_name(td_authored_get_street(600,900),s);expect(!strcmp(s,"TORONTO ISLANDS"),"the Islands have no street");
    td_get_street_name(td_authored_get_street(384,600),s);expect(!strcmp(s,"SPADINA AVE"),"the nearest street is still named");
}
int main(void) {
    expect(sizeof(td_state_t)==58&&offsetof(td_state_t,district)==56,"host fixture retains the current serialized state layout");
    test_acceleration_and_turning();test_glancing_contact();test_wall_and_brake();
    test_momentum_and_coasting();test_pressed_edge_once();test_clock();test_street_life();
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
    test_v5_migration_and_interrupted_upgrade();test_district_semantic_fallback();test_station_reach();
    test_reciprocal_portals();test_queue_failure_and_remote_boot();test_car_entry_at_portal();test_first_frame_actors();
    test_walk_pace_dispatch_and_foot_delivery();
    test_park_delivery_guidance();
    test_atlas_driver_handoff_and_freeze();
    test_sidewalk_pickups();test_parcels_spray_panic();test_visible_transit();test_ambient_traffic();
    test_day_night();test_animation();test_car_damage();test_scenery();test_places();
    printf("Host engine regressions: %u checks, %u failures. Hardware/emulator evidence remains separate.\n",checks,failures);
    return failures?1:0;
}
