/* Includes production gameplay so these fixtures exercise its static routines. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "gbvm_stubs.h"
#define TD_WORLD_ROUTE_DATA
#define TD_WORLD_DATA
#include "engine_under_test.c"

/* Host-only isolated private-call compatibility; no production NULL path. */
static void td_test_drive(void){td_terrain_cache_t cache={0};td_drive(&cache);}
static UBYTE td_test_drivable(UWORD u,UWORD v){td_terrain_cache_t cache;cache.valid=0;return td_terrain_drivable(u,v,&cache);}
#define td_drive() td_test_drive()
#define td_drivable(u,v) td_test_drivable(u,v)

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

actor_t actors[22];
actor_t *actors_inactive_head;
UBYTE actors_len;
UWORD camera_x,camera_y,image_width=1024,image_height=976,sys_time;
WORD draw_scroll_x,draw_scroll_y;
far_ptr_t current_scene;
UBYTE camera_settings,joy,joy_pressed;
BYTE camera_offset_x,camera_offset_y,camera_deadzone_x,camera_deadzone_y;
UBYTE td_test_sram[8192];

static unsigned failures,checks,stop_reads,ui_draws;
static UBYTE ui_last_draw_onfoot;
static unsigned audio_updates,audio_inits,audio_impacts;
/* Native compositing has its own actual-source VRAM/OAM harness. */
static UBYTE test_aircraft_exposed,test_boat_cover;
UBYTE td_aircraft_render_exposed(UWORD u,UWORD v){(void)u;(void)v;return test_aircraft_exposed;}
UBYTE td_boats_under_cover(void){return test_boat_cover;}
void td_aircraft_render_reset(void) {}
void td_aircraft_render_bind(void) {}
void td_aircraft_render_restore(void) {}
void td_city_sprites_bind(void) {}
void td_player_sprite_restore(void) {}
static UBYTE test_boat_shore_active,test_boat_active,test_shop_pending;
static UBYTE test_boat_interact_calls,test_boat_drive_calls,test_boat_drive_keys,test_boat_exit_allowed;
static UWORD test_boat_shore_u,test_boat_shore_v;
static unsigned sandbox_render_calls;
static unsigned sandbox_pose_calls;
UBYTE td_boats_restore_shore(UWORD *u,UWORD *v){if(!test_boat_shore_active)return FALSE;*u=test_boat_shore_u;*v=test_boat_shore_v;return TRUE;}
static unsigned test_boat_controlled_calls;
UBYTE td_boats_controlled(void){test_boat_controlled_calls++;return test_boat_active;}
UBYTE td_boats_interact(UWORD *u,UWORD *v){
    test_boat_interact_calls++;
    if(test_boat_active&&test_boat_exit_allowed){test_boat_active=0;*u=560*16;*v=800*16;return 2;}
    return 0;
}
void td_boats_drive(UBYTE keys,UWORD elapsed,UWORD *u,UWORD *v){
    test_boat_drive_calls++;test_boat_drive_keys=keys;(void)elapsed;(void)u;(void)v;
}
UBYTE td_shops_interact(void){return FALSE;}
UBYTE td_shops_pending(void){return test_shop_pending;}
void td_shops_reset(void){test_shop_pending=0;}
void td_actor_render_actor(actor_t *a){(void)a;sandbox_render_calls++;}
void td_vehicle_present(actor_t *a,UBYTE vehicle,UBYTE heading){UBYTE f=vehicle*8+((heading+1)&15)/2;sandbox_pose_calls++;actor_set_frames(a,f,f+1);}
void td_boats_bind(void){}
void td_boats_reset(void){}
void td_boats_update(UWORD elapsed){(void)elapsed;}
void td_traffic_lights_reset(void){}
void td_fleet_present(actor_t *a,UBYTE kind,UBYTE direction){UBYTE frame=kind<2?kind*8+direction*2:(kind-2)*4+direction;sandbox_pose_calls++;actor_set_frames(a,frame,frame+1);}
void td_civilian_present(actor_t *a,UBYTE variant,UBYTE pose){(void)variant;actor_set_frames(a,32+pose,33+pose);}
static UBYTE audio_mode,audio_active,audio_cue,audio_braking;
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
static enum { CLEAR_GROUND,EAST_WALL,SOUTH_CURB,NATIVE_GRID,HIDDEN_NPC_TILE,SOUTHWEST_CORNER,ALIGHT_BARRIER,ISLAND_CHECKPOINT_EDGE,TRAM_CORNER } geometry;
static UBYTE test_corner_tile_x,test_corner_tile_y;
static unsigned long test_road_tile_reads;
static void test_refresh_scene(void){
    current_scene.bank=geometry+1;
    current_scene.ptr=test_current_district<TD_DISTRICT_COUNT?&test_native_scenes[test_current_district]:NULL;
}
static jmp_buf interrupted_save;
static unsigned sram_writes,sram_interrupt_after;
static int sram_interrupt_enabled;
static unsigned sram_offsets[256];
static UBYTE sram_values[256];

static void expect(int condition,const char *name) {
    checks++;
    if (!condition) { failures++; fprintf(stderr,"FAIL %s\n",name);if(getenv("TD_ENGINE_DIAGNOSTICS"))fprintf(stderr,"  state d%u mode%u foot%u pos%u/%u speed%d heading%u health%u cash%u cool%u msg%u heat%u tick%u entry%u release%u contact%u\n",td.district,td.mode,td.onfoot,td.u,td.v,td.speed,td.heading,td.health,td.cash,td.cooldown,td.msg,td.wanted,td_tick,td_entry_timer,td_result_b_release,td_contact_episode); }
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
    if (geometry==TRAM_CORNER && y>=test_corner_tile_y && x<test_corner_tile_x) return 15;
    if (geometry==ALIGHT_BARRIER&&x==71&&y==90) return 15;
    if (geometry==ISLAND_CHECKPOINT_EDGE&&district==TD_DISTRICT_ISLANDS&&x==39&&y==35)return 15;
    if (geometry==NATIVE_GRID||geometry==ALIGHT_BARRIER||geometry==ISLAND_CHECKPOINT_EDGE) return x<native_widths[district]&&y<native_heights[district]?native_collision[district][y*native_widths[district]+x]:15;
    if (geometry==HIDDEN_NPC_TILE && x==48 && y==53) return 15;
    return 0;
}
UBYTE tile_at(UBYTE x,UBYTE y) {test_road_tile_reads++;return district_tile(test_current_district,x,y);}
UBYTE tile_hit_x,tile_hit_y;
UBYTE tile_col_test_range_x(UBYTE mask,UBYTE row,UBYTE first,UBYTE last){
    tile_hit_y=row;
    for(unsigned x=first;x<=last;x++){tile_hit_x=x;UBYTE tile=tile_at(x,row);if(tile&mask)return tile;}
    return 0;
}
UBYTE tile_col_test_range_y(UBYTE mask,UBYTE column,UBYTE first,UBYTE last){
    tile_hit_x=column;
    for(unsigned y=first;y<=last;y++){tile_hit_y=y;UBYTE tile=tile_at(column,y);if(tile&mask)return tile;}
    return 0;
}

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
/* The actual UI initializer's transient-page reset is independently exercised
 * by the production UI harness. Retain that contract in this hardware stub. */
void td_ui_init(void) { td_board_route=0;ui_draws++; }
void td_ui_draw(void) { ui_draws++;ui_last_draw_onfoot=td.onfoot; }
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
void td_audio_init(void) {audio_inits++;audio_mode=TD_AUDIO_FULL;audio_active=audio_braking=0;audio_cue=255;}
void td_audio_update(WORD speed,UBYTE vehicle,UBYTE onfoot,UBYTE braking,UBYTE active) {
    (void)speed;(void)vehicle;(void)onfoot;audio_updates++;audio_active=active;audio_braking=braking;
}
void td_audio_play(UBYTE cue) {if(audio_mode!=TD_AUDIO_SILENT){audio_cue=cue;if(cue==TD_AUDIO_IMPACT)audio_impacts++;}}
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
UBYTE td_district_current(void) {test_refresh_scene();return test_current_district;}
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
            if(district_tile(district,x,y)&15)return FALSE;
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
    td_tick=td_notice_timer=td_red_cooldown=td_turn_tick=td_entry_timer=td_result_b_release=0;
    td_entry_target=td_walk_dir=td_input_edge=0;
    td_session_live=td_transition_pending=test_current_district=test_queue_fail=0;test_queued_district=TD_DISTRICT_NONE;
    td_motion_reset();td_menu_reset();td_vx=td_vy=0;td_last_frame=0;td_resume_mode=TD_ROAM;td_board_route=0;
    td_route_district=TD_DISTRICT_NONE;memset(td_traffic_samples,0,sizeof(td_traffic_samples));
    td_corner_used=td_contact_episode=td_traffic_retreat_mask=td_vehicle_contact_mask=0;td_traffic_advance=8;td_police_waypoint.valid=td_police_stuck=td_traffic_elapsed=td_police_elapsed=0;td_police_advance=0;
    memset(td_nearby_routes,0,sizeof(td_nearby_routes));
    joy=joy_pressed=0;sys_time=0;stop_reads=ui_draws=0;
    draw_scroll_x=draw_scroll_y=0;sandbox_pose_calls=0;
    test_boat_interact_calls=test_boat_drive_calls=test_boat_drive_keys=test_boat_exit_allowed=0;test_boat_controlled_calls=0;
    ui_last_draw_onfoot=0;
    stop0_here=authored_content=0;test_queue_calls=test_reset_calls=0;
    test_map_opens=test_map_updates=test_map_closes=0;
    test_map_active=test_map_buttons=test_map_pressed=test_map_camera_settings=0;
    test_map_camera_x=test_map_camera_y=0;
    td_save_slot=TD_NONE;td_save_seq=0;actors_inactive_head=test_actors_active_head=NULL;actors_len=0;
    sram_writes=sram_interrupt_after=0;sram_interrupt_enabled=0;
    geometry=CLEAR_GROUND;test_boat_shore_active=test_boat_active=test_shop_pending=test_boat_cover=0;test_aircraft_exposed=1;test_boat_shore_u=test_boat_shore_v=0;sandbox_render_calls=0;td_inside_shop=0;td_sandbox_reset();td_scenery_reset();td_ramming_reset();test_refresh_scene();test_road_tile_reads=0;
    td_audio_init();audio_updates=audio_inits=audio_impacts=0;
    td_world_traffic_init(0,td_traffic_u,td_traffic_v,td_traffic_leg,td_traffic_samples);
    for(unsigned i=0;i<8;i++) { td_traffic_u[i]=(i<6?800+i*32:704+(i-6)*32)*16;td_traffic_v[i]=928*16;td_ped_route[i]=TD_NONE; }
    td_sandbox_u=td_traffic_u;td_sandbox_v=td_traffic_v;td_sandbox_district=0;
    td_ped_refresh=1;td_ped_anchor_u=td.u>>4;td_ped_anchor_v=td.v>>4;
    td_streetcar_runtime_reset();
    td_people_reset();
    for(unsigned i=9;i<17;i++)actors[i].flags=ACTOR_FLAG_HIDDEN;
    td_aircraft_reset(0x9D27);
}

static void driving_tick(UBYTE held) {
    joy_pressed=held & ~joy;joy=held;td_tick++;sys_time++;td_corner_used=0;td_drive();
}
static void world_tick(UBYTE held,UWORD elapsed) {
    test_refresh_scene();joy_pressed=held & ~joy;joy=held;sys_time+=elapsed;toronto_update();
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
    td.u=300*16;td.v=392*16;td.heading=1;geometry=SOUTH_CURB;
    UWORD start=td.u;WORD minimum=td.speed;
    for(unsigned i=0;i<80;i++) {
        driving_tick(J_A);if(td.speed<minimum)minimum=td.speed;
        expect(td.v>>4<393,"glancing contact remains outside curb footprint");
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
    expect(td_road_walkable(td.u>>4,td.v>>4),"rail fixture courier endpoint is walkable");
    UWORD before_u=td.u,before_v=td.v;td_enter_exit();
    expect(td.onfoot&&td_entry_timer==0,"entry cannot animate through native rail collision");
    expect(td.u==before_u&&td.v==before_v,"rejected entry keeps the courier position");

    reset_case();td.park_u=500*16;td.park_v=450*16;td.u=518*16;td.v=450*16;td.onfoot=1;
    td_enter_exit();expect(td_entry_timer==12&&ui_last_draw_onfoot,"clear door approach begins an entry animation with its still-walking HUD");
    unsigned draws=ui_draws,stores=sram_writes;
    for(unsigned i=0;i<11;i++)driving_tick(0);
    expect(td.onfoot&&td_entry_timer==1&&ui_draws==draws&&sram_writes==stores,
           "unfinished car entry retains walking state without repeated redraws or saves");
    driving_tick(0);
    expect(!td.onfoot&&!td_entry_timer&&td.u==td.park_u&&td.v==td.park_v&&
           ui_draws==draws+1&&!ui_last_draw_onfoot&&sram_writes>stores,
           "the final entry step requests its own HUD repaint only after snapping to the occupied parked car");
    stores=sram_writes;
    for(unsigned i=0;i<12;i++)driving_tick(0);
    expect(ui_draws==draws+1&&sram_writes==stores,
           "ordinary stopped-car updates do not repeat the completion repaint or save");
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
static void native_case(void);
static void test_pedestrian_phase_reuse(void) {
    /* Recorded triangle positions/directions, including exact turnaround,
       clock rollover and the highest route identity in each district. */
    const struct {UWORD seconds;UBYTE subsecond,route,offset,direction;} cases[]={
        {0,0,0,0,0},{0,0,1,37,0},{0,0,2,53,2},{0,0,3,16,2},
        {1,59,0,23,0},{31,59,0,0,2},{32,0,0,0,0},
        {65535,59,0,0,2},{65535,59,101,24,0},{65535,59,127,37,2}
    };
    for(UBYTE district=0;district<TD_DISTRICT_COUNT;district++)for(unsigned point=0;point<sizeof(cases)/sizeof(cases[0]);point++){
        UBYTE route=cases[point].route;if(route>=td_route_counts[district])continue;
        native_case();td.onfoot=1;td.district=test_current_district=district;
        td.seconds=cases[point].seconds;td.subsecond=cases[point].subsecond;td_tick=24;
        td_ped_refresh=2;td_ped_route[0]=route;
        td_ped_anchor_u=td.u>>4;td_ped_anchor_v=td.v>>4;
        td_nearby_routes[0][0]=td_district_routes[district][route][0];
        td_nearby_routes[0][1]=td_district_routes[district][route][1];
        td_state_t before=td;td_pedestrians();
        UWORD u=td_nearby_routes[0][0]+cases[point].offset,v=td_nearby_routes[0][1];
        expect(actors[9].pos.x==u*32&&actors[9].pos.y==v*32&&
               actors[9].frame_start==33+cases[point].direction&&td_ped_route[0]==route,
               "reused phase preserves recorded native pedestrian position, turnaround direction and animation");
        expect(!!(actors[9].flags&ACTOR_FLAG_HIDDEN)==!(td_distance(td.u>>4,u)<176&&td_distance(td.v>>4,v)<144)&&
               !memcmp(&td,&before,58),"pedestrian phase reuse preserves visibility and every saved field");
    }
}

static void test_signal_and_autonomous_traffic(void) {
    reset_case();td.onfoot=1;td.seconds=8;
    td_traffic_leg[0]=3;td_world_traffic_samples(0,td_traffic_leg,td_traffic_samples);
    td_traffic_u[0]=184*16;td_traffic_v[0]=296*16;UWORD before=td_traffic_u[0];
    td_traffic_step();expect(td_traffic_u[0]==before,"red traffic stops before the Bathurst intersection in world units");
    td.seconds=0;td_traffic_step();expect(td_traffic_u[0]>before,"green traffic leaves the stop line");

    td.seconds=8;td_traffic_u[0]=182*16+6;before=td_traffic_u[0];
    td_traffic_step();expect(td_traffic_u[0]>before,"red traffic does not stop at arbitrary eight-pixel intervals");

    td.seconds=0;td_traffic_u[4]=824*16;td_traffic_v[4]=424*16;before=td_traffic_v[4];
    td_traffic_step();expect(td_traffic_v[4]==before,"vertical red traffic stops before Dundas");
    td.seconds=8;td_traffic_step();expect(td_traffic_v[4]<before,"northbound green traffic resumes in Parliament's east lane");

    reset_case();td.mode=TD_WAIT;td.onfoot=1;td_traffic_u[0]=100*16;td_traffic_v[0]=280*16;before=td_traffic_u[0];sys_time=4;
    toronto_update();expect(td_traffic_u[0]==before,"four elapsed VBlanks accrue without premature autonomous movement");
    sys_time=8;toronto_update();expect(td_traffic_u[0]==before,"eight elapsed VBlanks remain below the ordinary road quantum");
    sys_time=16;toronto_update();expect(td_traffic_u[0]==before-128,"a sixteen-VBlank autonomous quantum advances eight fully swept pixels west during transit waiting");
    expect(actors[9].pos.x||actors[9].pos.y,"pedestrians update during unpaused transit waiting");

    reset_case();td.mode=TD_WAIT;td.onfoot=1;td_traffic_u[0]=100*16;td_traffic_v[0]=280*16;
    before=td_traffic_u[0];sys_time=30;toronto_update();
    expect(td_traffic_u[0]==before-128,"large elapsed gaps cap one autonomous sweep at eight pixels instead of repeating it for courier substeps");
    reset_case();td.seconds=8;td_traffic_advance=128;
    td_traffic_leg[0]=3;td_world_traffic_samples(0,td_traffic_leg,td_traffic_samples);
    td_traffic_u[0]=182*16;td_traffic_v[0]=296*16;before=td_traffic_u[0];
    td_traffic_step();expect(td_traffic_u[0]==before,"elapsed traffic cannot sweep across a red stop line with clear endpoints");
}

static void test_parked_visibility_context(void) {
    static const UBYTE districts[]={0,1,3},modes[]={TD_ROAM,TD_WAIT,TD_RIDE};
    static const UBYTE flags[]={0x30,0x32,0x70,0x72};
    for(unsigned logical=0;logical<3;logical++)for(unsigned view=0;view<3;view++)
        for(unsigned park=0;park<TD_DISTRICT_COUNT;park++)for(unsigned foot=0;foot<2;foot++)
        for(unsigned mode=0;mode<3;mode++)for(unsigned pattern=0;pattern<4;pattern++){
            reset_case();td.district=districts[logical];td.park_district=park;
            td.onfoot=foot;td.mode=modes[mode];td.vehicle=pattern;td.heading=pattern*4;
            td.park_u=952*16+6;td.park_v=548*16+12;
            td_streetcar_view_district=districts[view];td_streetcar_ride_view=td.mode==TD_RIDE;
            td_streetcar_focus_u=910*16+11;td_streetcar_focus_v=520*16+3;
            td_streetcar_elapsed=23;td_streetcar_bound=td_streetcar_valid=1;
            td_streetcar_cue=7;td_streetcar_was_ride=td_streetcar_ride_view;
            td_streetcar_pose(59,25,&td_streetcar_display);
            actors[8].flags=flags[pattern];
            td_state_t before=td;td_streetcar_pose_t display=td_streetcar_display;
            UBYTE hidden=foot&&park==districts[view]?0:ACTOR_FLAG_HIDDEN;
            td_traffic_present();
            expect(actors[8].flags==((flags[pattern]&~ACTOR_FLAG_HIDDEN)|hidden),
                   "parked visibility uses loaded view and preserves every other actor flag");
            expect(td_streetcar_view_district==districts[view]&&td_streetcar_ride_view==(td.mode==TD_RIDE)&&
                   td_streetcar_focus_u==910*16+11&&td_streetcar_focus_v==520*16+3,
                   "parked actor presentation never overwrites the ride district or focus");
            expect(!memcmp(&td_streetcar_display,&display,sizeof(display))&&td_streetcar_elapsed==23&&
                   td_streetcar_bound==1&&td_streetcar_valid==1&&td_streetcar_cue==7&&
                   td_streetcar_was_ride==(td.mode==TD_RIDE),
                   "fleet presentation preserves prepared tram pose and transient timing");
            expect(!memcmp(&td,&before,58)&&actors[8].pos.x==952*32&&actors[8].pos.y==548*32,
                   "loaded parked visibility retains serialized state and floored car position");
        }
}

static void test_fractional_fleet_presentation(void) {
    const UWORD positions[6][2]={{80,280},{200,392},{320,168},{440,632},{824,240},{808,138}};
    /* Each offset remains on a genuine straight Core route segment. The
       bus is southbound leg2, immediately above the real route19 crossing. */
    for(unsigned fraction=0;fraction<16;fraction++) {
        native_case();td.onfoot=1;td.u=816*16;td.v=112*16;
        td.park_u=560*16;td.park_v=720*16;td.seconds=7;td.subsecond=20;
        for(unsigned i=0;i<6;i++) {
            td_traffic_u[i]=positions[i][0]*16+(i<4?fraction:0);
            td_traffic_v[i]=positions[i][1]*16+(i>=4?fraction:0);
            td_traffic_leg[i]=i==5?2:1;
        }
        td_world_traffic_samples(0,td_traffic_leg,td_traffic_samples);
        td_state_t before=td;
        UBYTE view=td_streetcar_view_district,ride=td_streetcar_ride_view;
        td_traffic_present();
        for(unsigned i=0;i<6;i++)expect(actors[i+2].pos.x==td_traffic_u[i]*2&&
            actors[i+2].pos.y==td_traffic_v[i]*2&&!(actors[i+2].flags&ACTOR_FLAG_HIDDEN),
            "actual fleet publication retains every Q4 fraction in native Q5 actor coordinates");
        expect(!memcmp(&td,&before,58)&&view==td_streetcar_view_district&&ride==td_streetcar_ride_view,
               "fractional fleet publication preserves all paid/save and prepared view state");
        expect(td_road_body(808,138,7)&&td_district_routes[0][19][0]==784&&
               td_district_routes[0][19][1]==148,
               "fractional bus fixture remains on the registered road beside the actual route19");
        expect(td_person_road_clear(799,148)==(fraction==0),
               "real cache-to-actor fraction forbids route19 entering the bus at9px by9.xpx separation");
    }
}

static void test_route_selection_invalid_context(void) {
    for(unsigned ride=0;ride<2;ride++)for(unsigned invalid=TD_DISTRICT_COUNT;invalid<=255;invalid++){
        struct {UBYTE before[3],ids[8],after[3];} slots={{0x41,0x82,0xC3},{0,1,70,255,127,128,4,5},{0xD4,0xA5,0x76}};
        struct {UWORD before,points[8][2],after;} points={0xA15C,{{123,456},{789,12},{345,678},{901,234},{567,890},{1234,5678},{321,654},{987,210}},0xE29D};
        UWORD cached[8][2];memcpy(cached,points.points,sizeof(cached));
        reset_case();td.district=ride?0:invalid;td.mode=ride?TD_RIDE:TD_ROAM;
        td_streetcar_view_district=ride?invalid:1;td_streetcar_ride_view=ride;
        td_streetcar_focus_u=910*16+15;td_streetcar_focus_v=520*16+7;
        td_state_t before=td;
        td_refresh_routes(slots.ids,points.points);
        for(unsigned slot=0;slot<8;slot++)expect(slots.ids[slot]==TD_NONE,
                "invalid chosen district clears every pedestrian identity before ROM indexing");
        expect(!memcmp(points.points,cached,sizeof(cached))&&points.before==0xA15C&&points.after==0xE29D&&
               slots.before[0]==0x41&&slots.before[1]==0x82&&slots.before[2]==0xC3&&
               slots.after[0]==0xD4&&slots.after[1]==0xA5&&slots.after[2]==0x76,
               "invalid route context preserves all coordinate bytes and surrounding caller buffers");
        expect(!memcmp(&td,&before,58)&&td_streetcar_view_district==(ride?invalid:1)&&
               td_streetcar_ride_view==ride&&td_streetcar_focus_u==910*16+15&&td_streetcar_focus_v==520*16+7,
               "invalid route selection changes no serialized or presentation context");
    }
    /* Only the selected context is guarded: a stale unused view must not
       suppress legitimate logical-scene pedestrians while walking. */
    reset_case();td.district=1;td.u=910*16;td.v=520*16;
    td_streetcar_view_district=1;td_streetcar_ride_view=0;
    UBYTE expected[8];UWORD expected_points[8][2];memset(expected,TD_NONE,sizeof(expected));
    memset(expected_points,0xA5,sizeof(expected_points));td_refresh_routes(expected,expected_points);
    expect(expected[0]!=TD_NONE,"registered West comparison selects actual nearby pedestrian routes");
    for(unsigned invalid=TD_DISTRICT_COUNT;invalid<=255;invalid++){
        UBYTE selected[8];UWORD points[8][2];memset(selected,TD_NONE,sizeof(selected));memset(points,0xA5,sizeof(points));
        td_streetcar_view_district=invalid;td_refresh_routes(selected,points);
        expect(!memcmp(selected,expected,sizeof(selected))&&!memcmp(points,expected_points,sizeof(points)),
               "unused invalid view cannot alter valid logical-district route selection");
    }
    for(unsigned district=0;district<TD_DISTRICT_COUNT;district++)for(unsigned ride=0;ride<2;ride++){
        UBYTE selected[8];UWORD points[8][2];memset(selected,TD_NONE,sizeof(selected));memset(points,0,sizeof(points));
        reset_case();td.district=ride?0:district;td_streetcar_view_district=district;td_streetcar_ride_view=ride;
        td.u=td_streetcar_focus_u=(td_district_routes[district][0][0]+32)*16;
        td.v=td_streetcar_focus_v=td_district_routes[district][0][1]*16;
        td_refresh_routes(selected,points);
        expect(selected[0]!=TD_NONE,"every registered chosen district retains positive route selection");
        for(unsigned slot=0;slot<8;slot++)expect(selected[slot]==TD_NONE||
            (selected[slot]<td_route_counts[district]&&points[slot][0]==td_district_routes[district][selected[slot]][0]&&
             points[slot][1]==td_district_routes[district][selected[slot]][1]),
             "selected route identities and cached coordinates remain authored and in range");
    }
}

static void test_core_bus_lane_joint_loop(void) {
    const UWORD loop[6][2]={{640,72},{808,72},{808,168},{640,168},{216,168},{216,72}};
    const UBYTE facing[6]={0,0,1,2,2,3};
    native_case();
    UBYTE legs[6]={1,1,1,1,1,4};UWORD from_u=0,from_v=0;
    UBYTE separated=td_streetcar_runtime_traffic_segment(0,5,6,legs,
        600*16,168*16,216*16,168*16,&from_u,&from_v);
    expect(separated&&from_u==640*16&&from_v==168*16,
           "the Core bus follows the westbound right-hand Wellesley lane");
    /* Inspect every full7px body along the six independently specified
       cardinal lane segments, including each shared corner and closing leg. */
    for(unsigned leg=0;leg<6;leg++) {
        unsigned prior=leg?leg-1:5;
        int x=loop[prior][0],y=loop[prior][1];
        int tx=loop[leg][0],ty=loop[leg][1];
        legs[5]=leg;
        td_traffic_leg[5]=leg;td_traffic_u[5]=x*16;td_traffic_v[5]=y*16;
        td_world_traffic_samples(0,td_traffic_leg,td_traffic_samples);td_traffic_present();
        expect(actors[7].frame_start==12+facing[leg],"each actual bus leg presents its cardinal direction including the westward split");
        expect((x==tx)!=(y==ty),"each proposed bus leg is nonzero and cardinal");
        for(;;) {
            expect(td_road_body(x,y,7),"full bus footprint stays on registered collision at every segment/corner pixel");
            expect(td_streetcar_runtime_traffic_segment(0,5,6,legs,x*16,y*16,
                tx*16,ty*16,&from_u,&from_v)&&from_u==loop[prior][0]*16&&from_v==loop[prior][1]*16,
                "actual rare-retreat segment metadata matches every complete bus lane segment");
            if(x==tx&&y==ty)break;
            if(x<tx)x++;else if(x>tx)x--;else if(y<ty)y++;else y--;
        }
    }
    /* Independent lawful-following fixture: the bus trails police by16px
       in their shared westbound lane. No fixture writes occur during the
       actual600-VBlank update sequence below. Old passing records stay old. */
    native_case();td.mode=TD_ROAM;td.onfoot=1;td.u=840*16;td.v=147*16;
    td.park_u=560*16;td.park_v=720*16;td.seconds=70;
    const UWORD positions[6][2]={{80,280},{200,392},{696,168},{440,632},{824,240},{712,168}};
    for(unsigned i=0;i<6;i++) {
        td_traffic_u[i]=positions[i][0]*16;td_traffic_v[i]=positions[i][1]*16;
        td_traffic_leg[i]=i==5?3:1;
    }
    td_world_traffic_samples(0,td_traffic_leg,td_traffic_samples);
    td_traffic_present();
    UWORD bus_u=td_traffic_u[5],police_u=td_traffic_u[2];
    UBYTE peak_overlap=0,bus_passed=0,police_passed=0;
    for(unsigned frame=0;frame<600;frame++) {
        world_tick(0,1);
        /* Independent half-open square bodies: bus7px + police5px. */
        int32_t bl=(int32_t)td_traffic_u[5]-112,br=(int32_t)td_traffic_u[5]+112;
        int32_t bt=(int32_t)td_traffic_v[5]-112,bb=(int32_t)td_traffic_v[5]+112;
        int32_t pl=(int32_t)td_traffic_u[2]-80,pr=(int32_t)td_traffic_u[2]+80;
        int32_t pt=(int32_t)td_traffic_v[2]-80,pb=(int32_t)td_traffic_v[2]+80;
        if(bl<pr&&pl<br&&bt<pb&&pt<bb)peak_overlap=1;
        if(td_traffic_u[5]<bus_u&&td_traffic_v[5]==168*16)bus_passed=1;
        if(td_traffic_u[2]<police_u&&td_traffic_v[2]==168*16)police_passed=1;
    }
    expect(td.seconds==80&&td.cash==30&&td.health==100&&td.mode==TD_ROAM,
           "the coherent bus/police replay advances ten real game seconds without fines or invented impacts");
    expect(td_traffic_u[5]!=bus_u&&td_traffic_u[2]!=police_u,
           "the native bus/police pair both progress rather than remaining mutually stopped for600VBlanks");
    expect(bus_passed&&police_passed,"both lawful same-direction vehicles make progress over the stated interval");
    expect(!peak_overlap,"following bus and police never overlap their complete bodies during the joint-loop replay");
}

static void test_city_routes_and_walking(void) {
    reset_case();geometry=NATIVE_GRID;
    expect(!td_drivable(76,756),"car footprint rejects a narrow solid rail under its centre");
    reset_case();geometry=NATIVE_GRID;td.u=793*16+2;td.v=730*16+12;
    td.heading=4;td.speed=21;td_vx=0;td_vy=336;
    for(unsigned i=0;i<4;i++)driving_tick(J_A);
    expect(td.speed>=21&&td.v>734*16,"held throttle clears the small quantised terrain corner before reaching actual street furniture");
    expect(td_drivable(td.u>>4,td.v>>4),"corner slide retains a collision-valid car footprint");

    reset_case();geometry=EAST_WALL;td.u=394*16;td.v=450*16;td.heading=0;td.speed=24;td_vx=384;
    driving_tick(J_A);expect(td.speed==0&&td.u==394*16,"corner assist cannot bypass a broad head-on wall");

    reset_case();geometry=NATIVE_GRID;toronto_init();td.mode=TD_ROAM;
    td.u=560*16;td.v=720*16;td.onfoot=0;
    expect(td_traffic_u[5]==216*16&&td_traffic_v[5]==72*16&&td_traffic_leg[5]==0,
           "fresh Core initialization starts the bus toward Bloor's right-hand eastbound lane");
    int usable=1,continuous=1;unsigned bus_circuits=0;
    UBYTE initial_bus_leg=td_traffic_leg[5],previous_bus_leg=initial_bus_leg;
    for(unsigned step=0;step<12000;step++) {
        UWORD old_u=td_traffic_u[5],old_v=td_traffic_v[5];
        td.seconds=step/60;td.subsecond=step%60;td_traffic_step();
        if(td_traffic_leg[5]==initial_bus_leg&&previous_bus_leg!=initial_bus_leg)bus_circuits++;
        previous_bus_leg=td_traffic_leg[5];
        if(td_distance(old_u,td_traffic_u[5])+td_distance(old_v,td_traffic_v[5])>8)continuous=0;
        for(unsigned i=0;i<6;i++)if(!td_drivable(td_traffic_u[i]>>4,td_traffic_v[i]>>4))usable=0;
    }
    expect(continuous,"autonomous bus has continuous movement through clock changes and route loops");
    expect(usable,"all six vehicles follow usable native road footprints through a long route run");
    expect(bus_circuits>0,"the bus completes its actual full route with all six fleet loops active");

    reset_case();geometry=NATIVE_GRID;int sidewalk=1;
    for(unsigned route=0;route<TD_PEDESTRIAN_ROUTES;route++)
        for(unsigned offset=0;offset<64;offset++)
            if(!td_road_walkable(td_district_routes[td.district][route][0]+offset,td_district_routes[td.district][route][1]))sidewalk=0;
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
    td_traffic_u[0]=197*16;td_traffic_v[0]=280*16;player_u=td_traffic_u[0];
    td_traffic_step();expect(td_traffic_u[0]<player_u,"moving traffic approaches the on-foot courier without an invincible13px exclusion");
    td.v+=40*16;td_traffic_step();expect(td_traffic_u[0]<player_u,"yielding westbound traffic continues once the courier clears");
    td.v=280*16;td.mode=TD_WAIT;player_u=td_traffic_u[0];td_traffic_step();
    expect(td_traffic_u[0]<player_u,"waiting for transit does not grant immunity from approaching traffic");

    reset_case();td_traffic_u[0]=400*16;td_traffic_v[0]=468*16;
    td_traffic_u[1]=418*16;td_traffic_v[1]=450*16;td_enter_exit();
    expect(td.onfoot&&td.u==400*16&&td.v==432*16,"car exit skips doors occupied by moving vehicles");

    reset_case();td_traffic_u[0]=300*16;td_traffic_v[0]=280*16;
    td_traffic_step();expect(!actors[2].pos.x&&!actors[2].pos.y,"motion substeps do not perform duplicate actor presentation");
    td_traffic_present();expect(actors[2].pos.x==td_traffic_u[0]*2,"actor presentation reflects the final integrated position");
}

static void test_audio_event_integration(void) {
    reset_case();sys_time=2;toronto_update();
    expect(audio_updates==1&&audio_active,"roaming flushes audio once per rendered update");
    joy=joy_pressed=J_START;sys_time+=2;toronto_update();
    expect(td.mode==TD_PAUSE&&!audio_active&&audio_updates==2,"entering pause stops world audio on the same update");
    joy=joy_pressed=0;sys_time+=240;toronto_update();
    expect(!audio_active&&audio_updates==3,"paused frames continue to service audio without advancing the world");
    td.menu=8;td_resume_mode=TD_RIDE;
    joy=joy_pressed=J_A;sys_time+=2;toronto_update();
    expect(td.menu==TD_SETTINGS_SOUND&&audio_mode==0&&td.mode==TD_PAUSE,
           "the paused transit Settings entry opens Sound without silently changing the audio preference");
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

static void settings_press(UBYTE buttons){world_tick(0,1);world_tick(buttons,1);}
static void test_settings_controller(void){
    const UBYTE resumes[]={TD_ROAM,TD_WAIT,TD_RIDE};
    for(unsigned index=0;index<sizeof(resumes)/sizeof(resumes[0]);index++){
        native_case();td.mode=resumes[index];td.onfoot=index!=0;
        td.job=0;td_get_job(td.job,&td_job);td.stage=1;td.health=67;td.left=90;
        td.seconds=7;td.subsecond=13;td.transit_origin=0;td.transit_target=12;td.ride_left=9;
        td.wanted=2;td.wanted_left=17;td.cooldown=43;
        td_state_t frozen=td;td_aircraft_state_t aircraft=td_aircraft;
        UWORD fleet_u[8],fleet_v[8];UBYTE legs[8];actor_t people[22];
        memcpy(fleet_u,td_traffic_u,sizeof(fleet_u));memcpy(fleet_v,td_traffic_v,sizeof(fleet_v));
        memcpy(legs,td_traffic_leg,sizeof(legs));memcpy(people,(const void *)actors,sizeof(people));
        unsigned stores=sram_writes;UBYTE tick=td_tick,traffic_elapsed=td_traffic_elapsed,police_elapsed=td_police_elapsed;
        world_tick(J_START,1);
        expect(td.mode==TD_PAUSE&&td.menu==0&&td_resume_mode==resumes[index],
               "ordinary Start preserves ROAM, WAIT or paid RIDE as the Settings resume mode");
        settings_press(J_UP);
        expect(td.menu==8,"main Pause Up wraps from Resume to Settings without entering its submenu");
        settings_press(J_A);
        expect(td.mode==TD_PAUSE&&td.menu==TD_SETTINGS_SOUND&&audio_mode==0,
               "main Settings opens its Sound row in every resumable world mode");
        for(UBYTE sound=1;sound<=TD_AUDIO_MODES;sound++){
            settings_press(J_A);
            expect(audio_mode==sound%TD_AUDIO_MODES&&td.menu==TD_SETTINGS_SOUND&&td.mode==TD_PAUSE,
                   "Sound cycles Music+Effects, Effects Only and All Sound Off without leaving Settings");
            world_tick(J_A,240);
            expect(audio_mode==sound%TD_AUDIO_MODES,
                   "holding the Sound confirmation cannot repeatedly cycle the preference");
        }
        settings_press(J_UP);expect(td.menu==TD_SETTINGS_BACK,"Settings Up wraps Sound to its own Back row");
        settings_press(J_DOWN);expect(td.menu==TD_SETTINGS_SOUND,"Settings Down wraps Back to Sound, never the main Resume row");
        settings_press(J_DOWN);expect(td.menu==TD_SETTINGS_CONTROLS,"Settings Down selects its visible Control Guide");
        for(UBYTE dismissal=0;dismissal<2;dismissal++){
            settings_press(J_A);
            expect(td.mode==TD_HELP&&td.menu==TD_SETTINGS_CONTROLS&&td_resume_mode==resumes[index],
                   "Control Guide opens ordinary HELP with its Settings return marker and original resume mode");
            world_tick(0,240);settings_press(dismissal?J_A:J_B);
            expect(td.mode==TD_PAUSE&&td.menu==TD_SETTINGS_CONTROLS,
                   "both A and B close the Control Guide to its own Settings row");
            world_tick(dismissal?J_A:J_B,240);
            expect(td.mode==TD_PAUSE&&td.menu==TD_SETTINGS_CONTROLS,
                   "a held guide dismissal cannot choose another setting or resume the world");
        }
        settings_press(J_DOWN);settings_press(J_A);
        expect(td.mode==TD_PAUSE&&td.menu==8,"the Settings Back row returns to main Settings without resuming");
        settings_press(J_A);settings_press(J_DOWN);settings_press(J_DOWN);settings_press(J_START);
        expect(td.mode==TD_PAUSE&&td.menu==8,"Start closes the Settings submenu to main Settings");
        world_tick(J_START,240);
        expect(td.mode==TD_PAUSE&&td.menu==8,"held submenu Start cannot leak a second resume action");
        settings_press(J_A);settings_press(J_B);
        expect(td.mode==TD_PAUSE&&td.menu==8,"B closes the Settings submenu to main Settings");
        world_tick(J_B,240);
        expect(td.mode==TD_PAUSE&&td.menu==8,"held submenu B cannot resume or cancel a paid or waiting journey");
        frozen.mode=TD_PAUSE;frozen.menu=8;
        expect(!memcmp(&td,&frozen,sizeof(td))&&td_resume_mode==resumes[index]&&sram_writes==stores,
               "all Settings and guide actions preserve every58-byte world, mission, fare and saved field");
        expect(!memcmp(fleet_u,td_traffic_u,sizeof(fleet_u))&&!memcmp(fleet_v,td_traffic_v,sizeof(fleet_v))&&
               !memcmp(legs,td_traffic_leg,sizeof(legs))&&!memcmp(people,(const void *)actors,sizeof(people))&&
               !memcmp(&aircraft,&td_aircraft,sizeof(aircraft))&&td_tick==tick&&
               td_traffic_elapsed==traffic_elapsed&&td_police_elapsed==police_elapsed,
               "long Settings waits freeze fleet, pedestrian actors, aircraft and autonomous timing accumulators");
        settings_press(J_B);
        expect(td.mode==resumes[index]&&td.speed==0&&!audio_braking&&(td_result_b_release&J_B),
               "a fresh main Pause B resumes its original world mode while consuming that brake input");
        for(UBYTE update=0;update<8;update++)world_tick(J_B,4);
        expect(td.mode==resumes[index]&&td.speed==0&&td.left==90&&td.ride_left==9&&td.cash==frozen.cash&&!audio_braking,
               "held Settings-close B neither reverses a car, cancels WAIT nor alights from a paid RIDE");
        if(!index){
            world_tick(0,1);for(UBYTE update=0;update<24;update++)world_tick(J_B,4);
            expect(td.speed<0&&audio_braking,"physical release then a fresh B restores braking and reverse after Settings");
        }
    }
    reset_case();td.mode=TD_HELP;td.menu=0;td_resume_mode=TD_ROAM;world_tick(J_A,1);
    expect(td.mode==TD_ROAM&&td.menu==0,"cold Welcome HELP retains its existing A/B Start Game destination");
}

static void test_bounded_corner_assist(void) {
    for(unsigned shift=1;shift<=7;shift++) {
        reset_case();geometry=SOUTHWEST_CORNER;td.u=(407-shift)*16;td.v=392*16;
        td.heading=4;td.speed=16;td_vx=0;td_vy=256;
        UWORD old_u=td.u,old_v=td.v;driving_tick(J_A);
        if(shift<=6) {
            expect(td.speed==16&&td.heading==4&&td.v>old_v,"small corner clearance retains heading, throttle and dominant travel");
            expect((unsigned)(td.u-old_u)==shift*16&&td_drivable(td.u>>4,td.v>>4),"corner assist chooses the nearest collision-valid lateral clearance");
        }else expect(td.speed==0&&td.u==old_u&&td.v==old_v,"seven-pixel blocked corner exceeds assistance budget and remains solid");
    }
    const UBYTE prohibited[]={0,J_A|J_B,J_B};
    for(unsigned input=0;input<3;input++) {
        reset_case();geometry=SOUTHWEST_CORNER;td.u=406*16;td.v=392*16;td.speed=16;td_vy=256;joy=prohibited[input];
        expect(!td_road_corner(&td,&td_vx,&td_vy,&td_corner_used,td.u,393*16),"coasting and braking do not invoke throttle corner assistance");
    }
    reset_case();geometry=SOUTHWEST_CORNER;td.u=406*16;td.v=392*16;td.speed=-6;td_vy=256;joy=J_A;
    expect(!td_road_corner(&td,&td_vx,&td_vy,&td_corner_used,td.u,393*16),"reverse does not invoke forward corner assistance");
    td.speed=16;td_vx=td_vy=256;
    expect(!td_road_corner(&td,&td_vx,&td_vy,&td_corner_used,td.u,393*16),"equal diagonal velocity has no arbitrary assistance axis");
    td_vx=0;td_vy=256;td_corner_used=1;
    expect(!td_road_corner(&td,&td_vx,&td_vy,&td_corner_used,td.u,393*16),"catch-up steps cannot apply multiple lateral assists in one rendered update");
    reset_case();joy=J_A;td.speed=16;td_vy=256;td.v=968*16;
    expect(!td_road_corner(&td,&td_vx,&td_vy,&td_corner_used,td.u,td.v+16),"corner assistance cannot push the car beyond the southern map bound");
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

static void check_clock_interval(UWORD frames,UBYTE fraction,UWORD start_frame,UWORD start_second){
    reset_case();td.seconds=start_second;td.subsecond=fraction;
    td_last_frame=start_frame;sys_time=(UWORD)((unsigned)start_frame+frames);
    /* One wide absolute phase calculation is independent of production's
     * bounded remainder/carry branches and world-second iteration. */
    unsigned total=(unsigned)start_second*60+fraction+frames;
    toronto_update();
    expect(td.seconds==(UWORD)(total/60)&&td.subsecond==total%60&&td_last_frame==sys_time,
           "actual active update preserves absolute60Hz phase across fast/full catch-up, fractional carry and both16-bit rollovers");
}
static void test_clock_boundaries(void){
    /* Every valid fractional clock state crosses zero, the full common
     * elapsed range, and both sides of the59/60/61-frame branch boundary. */
    for(unsigned fraction=0;fraction<60;fraction++)for(unsigned frames=0;frames<=61;frames++)
        check_clock_interval(frames,fraction,(frames+fraction)&1?65530:0,frames&1?65535:17);
    static const UWORD long_gaps[]={62,119,120,121,255,256,257,599,600,601,3839,3840,3841,65534,65535};
    static const UBYTE fractions[]={0,1,58,59};
    for(unsigned i=0;i<sizeof(long_gaps)/sizeof(long_gaps[0]);i++)for(unsigned j=0;j<4;j++)
        check_clock_interval(long_gaps[i],fractions[j],65530,65535);
    static const UBYTE frozen_modes[]={TD_PAUSE,TD_MAP,TD_BOARD,TD_HELP};
    static const UWORD gaps[]={0,1,59,60,61,65535};
    for(unsigned mode=0;mode<4;mode++)for(unsigned gap=0;gap<6;gap++){
        reset_case();td.mode=frozen_modes[mode];td.seconds=65535;td.subsecond=59;
        td_state_t before=td;td_last_frame=65530;sys_time=(UWORD)(65530u+gaps[gap]);
        toronto_update();
        expect(!memcmp(&before,&td,58)&&td_last_frame==sys_time,
               "menu/pause updates freeze the entire game state and discard each fast/full elapsed gap through video wrap");
    }
    for(unsigned pending=1;pending<=2;pending++)for(unsigned gap=0;gap<6;gap++){
        reset_case();td.seconds=65535;td.subsecond=59;td_transition_pending=pending;test_queue_fail=1;
        td_state_t before=td;td_last_frame=65530;sys_time=(UWORD)(65530u+gaps[gap]);
        toronto_update();
        expect(!memcmp(&before,&td,58)&&td_last_frame==sys_time&&td_transition_pending==pending,
               "pending scene/load retry freezes clock and game bytes before the elapsed fast-path decision");
    }
    reset_case();td.seconds=65535;td.subsecond=59;td_last_frame=65530;sys_time=54;joy=joy_pressed=J_START;
    toronto_update();
    expect(td.mode==TD_PAUSE&&td.seconds==65535&&td.subsecond==59,
           "opening pause on a60-frame gap still wins before clock catch-up");
    joy=joy_pressed=0;sys_time=114;toronto_update();
    joy=joy_pressed=J_B;sys_time=174;toronto_update();
    expect(td.mode==TD_ROAM&&td.seconds==65535&&td.subsecond==59&&td_last_frame==174,
           "resuming pause discards frozen elapsed gaps without consuming a clock second");
    joy=joy_pressed=0;sys_time=175;toronto_update();
    expect(td.seconds==0&&td.subsecond==0,
           "first active frame after resume carries exactly once across world-second rollover");
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
            expect(td.speed==12&&td_vx==-192&&td_vehicle_recoil==6&&td.cooldown==60&&td.msg==5,
                   "traffic reverses half the actual momentum for bounded rebound and warns each cargo lifecycle once");
        }else if(hazard==1) {
            geometry=EAST_WALL;td.u=394*16;td.safe_u=td.u;
            driving_tick(0);damage=kind==1?20:8;
            expect(td.u==394*16&&td.speed==0&&!td_vx&&!td_vy&&td.cooldown==45&&td.msg==5,
                   "a broad wall still stops the vehicle and applies collision cooldown before and after pickup");
        }else if(hazard==2) {
            geometry=EAST_WALL;td.u=392*16;td.safe_u=td.u;td.heading=1;td_vy=128;
            driving_tick(0);damage=kind==1?4:1;
            expect(td.u==392*16&&td.v>450*16&&td.speed==24&&!td_vx&&td_vy>0&&td.cooldown==30&&td.msg==5,
                   "glancing curb recovery keeps forward speed and its warning independently of cargo occupancy");
        }else {
            geometry=CLEAR_GROUND;td_turn_tick=17;driving_tick(J_RIGHT);
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
        UBYTE sequence,source_version;td_state_t raw;
        expect(td_read_slot(td_save_slot,&raw,&sequence,&source_version)&&raw.job==1&&raw.stage==0&&raw.health==invalid_health[fault],
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

/* Clear terrain and distant fleet bodies isolate the input transition. The
   actual finish, menu, clock, driving, audio handoff and save code still run. */
static void result_input_case(UBYTE success,UBYTE onfoot) {
    reset_case();td.onfoot=onfoot;stop0_here=onfoot;
    td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=99;
    td_finish(success);
    expect(td.mode==TD_RESULT&&td.job==TD_NONE&&td.speed==0,
           "RESULT release fixture uses an actual retired contract and stopped courier");
}

static void test_result_b_release(void) {
    for(UBYTE success=0;success<2;success++) {
        result_input_case(success,0);
        UWORD u=td.u,v=td.v,cash=td.cash;UBYTE done=td.done,health=td.health;
        world_tick(J_B,1);
        expect(td.mode==TD_ROAM&&!audio_braking,
               "RESULT B dismisses the screen without handing its brake press to audio");
        for(unsigned update=0;update<30;update++) {
            world_tick(J_B,4);
            expect(td.mode==TD_ROAM&&td.speed==0&&td_vx==0&&td_vy==0&&td.u==u&&td.v==v,
                   "repeated held RESULT B updates keep the stopped vehicle stationary");
            expect(audio_active&&!audio_braking,
                   "consumed RESULT B leaves world audio active without brake audio");
        }
        expect(td.seconds==2&&td.subsecond==0&&td.cash==cash&&td.done==done&&td.health==health,
               "consumed RESULT B advances the real world clock without changing the completed outcome");
        world_tick(0,1);
        for(unsigned update=0;update<8;update++)world_tick(J_B,4);
        expect(td.mode==TD_ROAM&&td.speed<0&&td.u<u&&td.v==v&&audio_braking,
               "sampling release restores ordinary held-B reverse movement and brake audio");
    }

    result_input_case(TRUE,0);
    world_tick(J_B,1);
    for(unsigned update=0;update<4;update++)world_tick(J_B,4);
    world_tick(J_B|J_START,1);
    UWORD paused_seconds=td.seconds;UBYTE paused_fraction=td.subsecond,old_vehicle=td.vehicle;
    expect(td.mode==TD_PAUSE&&td.speed==0&&!audio_active&&!audio_braking,
           "held RESULT B followed by START pauses a stopped vehicle instead of inherited reverse");
    for(unsigned row=0;row<4;row++) {
        world_tick(J_B|J_DOWN,1);world_tick(J_B,1);
    }
    expect(td.menu==4,"ordinary pause navigation reaches the vehicle selector while B remains held");
    world_tick(J_B|J_A,1);
    expect(td.mode==TD_PAUSE&&td.vehicle==(UBYTE)((old_vehicle+1)&3)&&td.speed==0,
           "the actual vehicle selector accepts a stopped courier after RESULT B dismissal");
    expect(td.seconds==paused_seconds&&td.subsecond==paused_fraction,
           "vehicle-selection inputs preserve the paused world clock");

    /* B is released while a frozen menu is open, before its B exit handler. */
    world_tick(0,1);world_tick(J_B,1);
    expect(td.mode==TD_ROAM&&td.speed==0&&!audio_braking,
           "a fresh PAUSE B exit consumes its own held brake input after the earlier RESULT release");
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.speed==0&&!audio_braking,"ordinary PAUSE B stays consumed until its physical release");
    world_tick(0,1);
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.speed<0&&audio_braking,"a fresh brake press after PAUSE B release restores reverse control");

    result_input_case(FALSE,0);
    world_tick(J_B,1);world_tick(J_B|J_START,1);
    world_tick(J_B|J_DOWN,1);world_tick(J_B,1);world_tick(J_B|J_A,1);
    expect(td.mode==TD_MAP&&test_map_opens==1,
           "a RESULT-consumed held B can remain held through ordinary pause-to-map input");
    td_state_t map_state=td;world_tick(0,60);
    expect(td.mode==TD_MAP&&!memcmp(&td,&map_state,sizeof(td)),
           "release sampled on the map does not change any of the 58 frozen gameplay bytes");
    world_tick(J_B,1);world_tick(J_B|J_START,1);
    expect(td.mode==TD_ROAM,"fresh map B and pause START preserve the existing two-stage map exit");
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.speed==0&&!audio_braking,"the map-close B remains consumed through the subsequent PAUSE START exit");
    world_tick(0,1);
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.speed<0&&audio_braking,"a newly pressed brake after map and pause exits still reverses normally");

    result_input_case(TRUE,0);world_tick(J_START,1);
    expect(td.mode==TD_ROAM,"RESULT START retains its ordinary free-roam exit");
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.speed<0&&audio_braking,"RESULT START does not suppress a subsequent fresh brake press");

    result_input_case(TRUE,0);world_tick(J_A,1);
    expect(td.mode==TD_BOARD&&td.job==TD_NONE,"RESULT A still opens dispatch without automatically accepting work");
    world_tick(J_B,1);
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.mode==TD_ROAM&&td.speed==0&&!audio_braking,
           "ordinary dispatch B exit consumes its own held brake input");
    world_tick(0,1);
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.speed<0&&audio_braking,"a released and newly pressed dispatch brake still restores reverse");

    result_input_case(TRUE,1);world_tick(J_B,1);
    UWORD foot_u=td.u,foot_v=td.v;
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.mode==TD_ROAM&&td.onfoot&&td.u==foot_u&&td.v==foot_v&&!audio_braking,
           "held RESULT B preserves the walking courier and cannot re-open transit without a new press");
    world_tick(0,1);world_tick(J_B,1);
    expect(td.mode==TD_TRANSIT&&td.onfoot&&td.transit_origin==0,
           "a fresh on-foot B after release still opens the nearby service selector");
}

static void test_result_release_initialization_and_save(void) {
    result_input_case(TRUE,0);world_tick(J_B,1);
    td_state_t guarded,unguarded;UBYTE sequence,version;
    td_save();
    expect(td_read_slot(td_save_slot,&guarded,&sequence,&version)&&version==TD_SAVE_VERSION&&
           td_save_address(td_save_slot)[3]==58&&guarded.mode==TD_ROAM,
           "saving after consumed RESULT B produces a real current-version 58-byte roam record");
    td_state_t before_release=td;world_tick(0,0);td_save();
    expect(!memcmp(&td,&before_release,sizeof(td))&&
           td_read_slot(td_save_slot,&unguarded,&sequence,&version)&&
           !memcmp(&guarded,&unguarded,sizeof(guarded)),
           "releasing RESULT B changes no saved gameplay field or actual serialized payload");

    result_input_case(TRUE,0);world_tick(J_B,1);
    td_session_live=1;toronto_init();
    expect(td.mode==TD_ROAM,"warm scene initialization keeps the current free-roam mode");
    UWORD warm_u=td.u,warm_v=td.v;
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.speed==0&&td.u==warm_u&&td.v==warm_v&&!audio_braking,
           "warm initialization preserves a still-held consumed RESULT B until release");
    world_tick(0,1);
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.speed<0&&audio_braking,"warm initialization still permits reverse after the physical release");

    result_input_case(TRUE,0);world_tick(J_B,1);
    UWORD saved_cash=td.cash;UBYTE saved_done=td.done;
    memset(&td,0,sizeof(td));td_session_live=0;actors_inactive_head=NULL;toronto_init();
    expect(td.mode==TD_HELP&&td.cash==saved_cash&&td.done==saved_done&&td.job==TD_NONE&&td.speed==0,
           "cold initialization restores the committed result outcome behind ordinary help");
    /* Cold initialization clears transient input state; the new HELP exit
       must consume its own A/B chord rather than reconstructing it from SRAM. */
    world_tick(J_A|J_B,1);
    expect(td.mode==TD_ROAM,"ordinary help A dismisses the cold-start screen with B still held");
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.speed==0&&!audio_braking&&td.cash==saved_cash&&td.done==saved_done,
           "cold HELP consumes its own dismissal chord without losing saved earnings or completion");
    world_tick(0,1);
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.speed<0&&audio_braking,"a fresh brake after cold HELP release still restores reverse");
}

static void test_menu_held_drive_buttons(void) {
    const struct { UBYTE mode,menu,buttons,active; } cases[]={
        {TD_PAUSE,0,J_B,1},{TD_PAUSE,0,J_A,1},
        {TD_PAUSE,0,J_A|J_B,1},{TD_PAUSE,0,J_B|J_START,1},
        {TD_PAUSE,0,J_A|J_START,1},{TD_PAUSE,6,J_A,1},
        {TD_PAUSE,7,J_A,1},{TD_BOARD,0,J_B,1},
        {TD_BOARD,0,J_A,1},{TD_BOARD,0,J_A,0},
        {TD_HELP,0,J_A,1},{TD_HELP,0,J_B,1},{TD_HELP,0,J_A|J_B,1}
    };
    /* Actual menu/driver/clock/traffic/audio functions on clear ground with
       distant bodies isolate input leakage from collision consequences. */
    for(unsigned index=0;index<sizeof(cases)/sizeof(cases[0]);index++) {
        reset_case();td.mode=cases[index].mode;td.menu=cases[index].menu;
        td_resume_mode=TD_ROAM;td_get_job(0,&td_job);td_offer=td_job;
        if(cases[index].active){td.job=0;td.stage=1;td.left=180;td.health=68;}
        td_traffic_u[0]=100*16;td_traffic_v[0]=280*16;
        UWORD u=td.u,v=td.v,cash=td.cash,fleet_u=td_traffic_u[0];
        world_tick(cases[index].buttons,1);
        expect(td.mode==TD_ROAM&&td.speed==0&&!audio_braking,
               "menu exit returns to stopped ROAM without treating its held button as a brake");
        UBYTE job=td.job,stage=td.stage,health=td.health;UWORD deadline=td.left;
        if(cases[index].mode==TD_BOARD&&!cases[index].active)
            expect(job==0&&stage==0&&health==100&&deadline==120,
                   "held-A dispatch fixture accepts a real fresh contract through the menu handler");
        for(unsigned update=0;update<30;update++) {
            world_tick(cases[index].buttons&(J_A|J_B),4);
            expect(td.mode==TD_ROAM&&td.u==u&&td.v==v&&td.speed==0&&td_vx==0&&td_vy==0,
                   "held menu A/B never accelerates or reverses a stopped courier after returning to ROAM");
            expect(audio_active&&!audio_braking,
                   "consumed menu inputs retain active world audio without brake audio");
        }
        expect(td.seconds==2&&td.subsecond==0&&td_traffic_u[0]<fleet_u&&
               td.cash==cash&&td.job==job&&td.stage==stage&&td.health==health&&
               (job==TD_NONE||td.left==deadline-2),
               "consumed menu input leaves traffic and the deadline live without changing cargo or money");
        world_tick(0,1);
        for(unsigned update=0;update<8;update++)world_tick(J_A,4);
        expect(td.speed>0&&td.u>u&&!audio_braking,
               "release followed by fresh A restores acceleration after every menu exit");
        world_tick(0,1);
        for(unsigned update=0;update<24;update++)world_tick(J_B,4);
        expect(td.speed<0&&audio_braking,
               "fresh B brakes through rest and restores reverse after every menu exit");
    }

    /* Reproduce the native North rejection/close inputs and exact pose;
       registered job data is real, while clear terrain/distant fleet keeps
       this regression about the input contract rather than native rendering. */
    native_case();geometry=CLEAR_GROUND;test_current_district=td.district=TD_DISTRICT_NORTH;
    td.u=td.safe_u=406*16+9;td.v=td.safe_v=637*16+8;td.heading=4;
    td.job=97;td_get_job(td.job,&td_job);td.stage=1;td.health=68;td.left=150;
    td.mode=TD_PAUSE;td.menu=4;td_resume_mode=TD_ROAM;
    UWORD north_u=td.u,north_v=td.v;UBYTE vehicle=td.vehicle;
    world_tick(J_A,8);
    expect(td.mode==TD_PAUSE&&td.msg==2&&td.vehicle==vehicle&&td.health==68,
           "active North fragile work rejects the actual vehicle selector without altering its condition");
    world_tick(0,8);world_tick(J_B,8);world_tick(0,8);
    expect(td.mode==TD_ROAM&&td.u==north_u&&td.v==north_v&&td.speed==0&&td.health==68,
           "North vehicle rejection followed by B8 and neutral8 never produces inherited reverse");

    reset_case();td.mode=TD_HELP;td_resume_mode=TD_ROAM;
    world_tick(J_A|J_B,1);world_tick(J_B,4);
    for(unsigned update=0;update<8;update++)world_tick(J_A|J_B,4);
    expect(td.speed>0&&!audio_braking,
           "fresh A works while only the unreleased menu B remains consumed");
    reset_case();td.mode=TD_HELP;td_resume_mode=TD_ROAM;
    world_tick(J_A|J_B,1);world_tick(J_A,4);
    for(unsigned update=0;update<8;update++)world_tick(J_A|J_B,4);
    expect(td.speed<0&&audio_braking,
           "fresh B works while only the unreleased menu A remains consumed");

    reset_case();td.speed=24;td_vx=384;UWORD moving_u=td.u;
    world_tick(J_START,1);world_tick(0,1);world_tick(J_A|J_RIGHT,1);
    for(unsigned update=0;update<8;update++)world_tick(J_A|J_RIGHT,4);
    expect(td.mode==TD_ROAM&&td.u>moving_u&&td.speed>0&&td.speed<24&&td.heading!=0&&td.subsecond==32,
           "consumed resume throttle preserves coasting, steering and the active world clock");

    reset_case();td.onfoot=1;td.park_u=td.u;td.park_v=td.v;
    world_tick(J_A,4);
    for(unsigned update=0;update<12;update++)world_tick(J_A,4);
    expect(!td.onfoot&&!td_entry_timer&&!td.speed&&(td_result_b_release&J_A),
           "vehicle entry consumes its interaction A until physical release, avoiding accidental acceleration");
    world_tick(0,1);for(unsigned update=0;update<8;update++)world_tick(J_A,4);
    expect(td.speed>0,"fresh A after vehicle-entry release accelerates normally");

    reset_case();geometry=SOUTHWEST_CORNER;td.u=406*16;td.v=392*16;
    td.heading=4;td.speed=16;td_vy=256;td.mode=TD_PAUSE;td.menu=0;
    world_tick(J_A,1);world_tick(J_A,1);
    expect(!td_corner_used&&td.u==406*16&&td.v==392*16&&td.speed==0&&joy==J_A,
           "consumed menu A cannot trigger throttle-only corner assistance while the car coasts into a wall");
    reset_case();geometry=SOUTHWEST_CORNER;td.u=406*16;td.v=392*16;
    td.heading=4;td.speed=16;td_vy=256;td.mode=TD_PAUSE;td.menu=0;
    world_tick(J_B,1);world_tick(J_A|J_B,1);
    expect(td_corner_used&&td.u==407*16&&td.v>392*16&&td.speed==16&&joy==(J_A|J_B),
           "fresh A retains corner assistance despite a consumed menu B and restores the sampled hardware input");

    reset_case();stop0_here=1;td.onfoot=1;td.mode=TD_TRANSIT;
    td.transit_origin=0;td.transit_target=12;
    world_tick(J_B,1);
    for(unsigned update=0;update<8;update++)world_tick(J_B,4);
    expect(td.mode==TD_ROAM&&td.onfoot&&td.cash==30,
           "held transit-menu B returns to walking without reopening the menu or charging a fare");
    world_tick(0,1);world_tick(J_B,1);
    expect(td.mode==TD_TRANSIT,"fresh B after transit-menu cancellation still opens a nearby service");
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
        /* An active job has no return assistance; idle low-cash Island
           returns are exercised separately by the fare boundary fixture. */
        if(td.district==TD_DISTRICT_ISLANDS){td.job=0;td_get_job(0,&td_job);td.left=600;}
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
        if(td.district==TD_DISTRICT_ISLANDS){td.job=0;td_get_job(0,&td_job);td.left=600;}
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
    state.onfoot=mode!=TD_ROAM;
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
    native_case();td.u=td.safe_u=312*16;td.v=td.safe_v=16*16;td.park_u=td.u;td.park_v=td.v;
    td.cash=111;td_save();td.cash=222;td_save();
    UBYTE newest=td_save_slot,image[sizeof(td_test_sram)];memcpy(image,td_test_sram,sizeof(image));
    expect(td_district_drivable(0,312,16)&&!td_district_drivable(1,312,16),
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
        world_tick(J_A,1);
        /* Isolate the actual second/alighting commit from the subsequent
           live fleet quantum: road traffic can now strike an alighted walker,
           covered by the separate swept moving-body regression. */
        UBYTE remaining=td.ride_left;while(remaining--)td_second();
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
    for(unsigned i=0;i<6;i++){td_traffic_u[i]=(800+i*32)*16;td_traffic_v[i]=928*16;}
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
    td_traffic_u[0]=800*16;td_traffic_v[0]=928*16;td_second();
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
        UBYTE phase=target_id>origin_id?index*16:128+(7-index)*16;
        UBYTE duration=(target_id>origin_id?target_id-origin_id:origin_id-target_id)*16;
        UBYTE waiting=scenario==1,failing=scenario==2;
        td_stop_t origin,destination,depot;
        native_case();td_get_stop(origin_id,&origin);td_get_stop(target_id,&destination);td_get_stop(0,&depot);
        test_current_district=td.district=origin.district;td_session_live=1;td.onfoot=1;
        td.u=td.safe_u=origin.u*16;td.v=td.safe_v=origin.v*16;
        td.mode=TD_TRANSIT;td.transit_origin=origin_id;td.transit_target=target_id;
        td_cursor=destination;td.seconds=256*2+phase+(waiting?4:0);td.subsecond=13;
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
        if(waiting)world_tick(0,252*60-td.subsecond);
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
        actor_t old_actors[22];UWORD old_traffic_u[8],old_traffic_v[8];
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
    /* Port Lands: Leslie's clear road at128, with authored sidewalk route
       (864,92) near the courier at(930,128), rather than the zero-filled
       origin of an omitted fifth initializer. */
    /* North Casa Loma parking approach336,576 and nearby authored
       horizontal sidewalks at548/604 give a real first-frame neighborhood. */
    const UWORD locations[][2]={{560,720},{800,64},{736,640},{224,528},{912,128},{320,280},{336,576}};
    _Static_assert(sizeof(locations)/sizeof(locations[0])==TD_DISTRICT_COUNT,
                   "Every registered district needs an explicit first-frame neighborhood fixture");
    for(unsigned district=0;district<TD_DISTRICT_COUNT;district++) {
        native_case();td_session_live=1;test_current_district=td.district=td.park_district=district;
        td.u=(locations[district][0]+18)*16;td.v=locations[district][1]*16;
        td.park_u=locations[district][0]*16;td.park_v=td.v;td.onfoot=1;
        if(district==TD_DISTRICT_ISLANDS){td.u=320*16;td.park_district=TD_DISTRICT_CITY;td.park_u=560*16;td.park_v=720*16;}
        toronto_init();
        expect((district==TD_DISTRICT_ISLANDS?td.park_district==TD_DISTRICT_CITY:
                td_drivable(locations[district][0],locations[district][1]))&&td_road_walkable(td.u>>4,td.v>>4),
               "each first-frame neighborhood uses actual walking ground and preserves the Island road exclusion");
        expect(actors_len==TD_ACTORS&&PLAYER.pos.x==(td.u>>4)*32&&PLAYER.pos.y==(td.v>>4)*32,
               "each registered district initializes all actors and courier coordinates before its first update");
        if(district==TD_DISTRICT_ISLANDS)
            expect(actors[8].flags&ACTOR_FLAG_HIDDEN,"the first real Island scene frame cannot display a parked mainland vehicle");
        else expect(actors[8].pos.x==(td.park_u>>4)*32&&actors[8].pos.y==(td.park_v>>4)*32&&!(actors[8].flags&ACTOR_FLAG_HIDDEN),
                    "first scene frame shows the locally parked vehicle at its real saved position");
        int traffic=1;unsigned visible=0;
        for(unsigned i=0;i<6;i++) {
            if(district==TD_DISTRICT_ISLANDS){if(!(actors[i+2].flags&ACTOR_FLAG_HIDDEN))traffic=0;}
            else if(actors[i+2].pos.x!=td_traffic_u[i]*2||actors[i+2].pos.y!=td_traffic_v[i]*2||
                    !td_drivable(td_traffic_u[i]>>4,td_traffic_v[i]>>4))traffic=0;
            if(!(actors[i+9].flags&ACTOR_FLAG_HIDDEN)) {
                visible++;
                expect(td_ped_route[i]<td_route_counts[td.district]&&td_road_walkable(actors[i+9].pos.x/32,actors[i+9].pos.y/32),
                       "first-frame visible pedestrian has a valid identity on actual scene walking terrain");
            }
        }
        expect(traffic,"first scene frame places mainland traffic on clear roads and hides all fleet on the actual foot-only Island");
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
        int expected=stop==34||stop==36||stop==41||stop==52||stop==53||stop==56||stop==58||stop==61||stop==63;
        expect(found==expected&&(expected||(u==1234&&v==5678)),
               "native parking getter leaves ordinary stops and invalid IDs unchanged");
        if(stop==61||stop==63)
            expect(found&&u==(stop==61?336:640)&&v==(stop==61?576:280)&&td_district_drivable(TD_DISTRICT_NORTH,u,v),
                   "the two appended North parking anchors retain exact authored coordinates and real native full-body road clearance");
    }
    UWORD u=1234,v=5678;
    expect(!td_get_parking(36,NULL,&v)&&v==5678&&!td_get_parking(36,&u,NULL)&&u==1234,
           "native parking lookup rejects missing outputs without a partial write");
    native_case();td.job=84;td_get_job(td.job,&td_job);td.stage=3;td.district=test_current_district=1;
    td.u=736*16;td.v=640*16;td_set_target();
    expect(td_route_district==0,"remote eastern parking destination guides a western driver through the core graph branch");
}

static void test_port_content_and_ordered_handoffs(void) {
    /* Campaign JSON is separately pinned to the pre-expansion51/88 prefix.
       Compare its independently encoded fixture with the actual native getters
       field by field: host struct padding is not part of the native contract. */
    native_case();
    for(UBYTE i=0;i<51;i++) {
        td_stop_t stop;td_get_stop(i,&stop);const td_stop_t *expected=&td_fixture_stops[i];
        expect(stop.u==expected->u&&stop.v==expected->v&&!strcmp(stop.name,expected->name)&&
               stop.transit==expected->transit&&stop.district==expected->district&&stop.reserved==expected->reserved,
               "every existing stop ordinal still decodes its pinned native coordinates, identity and restrictions");
    }
    for(UBYTE i=0;i<88;i++) {
        td_job_t job;td_get_job(i,&job);const td_job_t *expected=&td_fixture_jobs[i];
        expect(!strcmp(job.title,expected->title)&&job.kind==expected->kind&&job.count==expected->count&&
               job.vehicle==expected->vehicle&&job.min_done==expected->min_done&&job.seconds==expected->seconds&&
               job.reward==expected->reward&&!memcmp(job.route,expected->route,12),
               "every existing saved job ordinal still decodes its pinned native requirements, order and reward");
    }
    expect(TD_QUESTS>=96&&TD_STOPS>=59,"the actual campaign contains all eight appended Port contracts and clients");
    if(TD_QUESTS<96||TD_STOPS<59)return;
    static const UBYTE routes[8][4]={{0,51,255,255},{37,52,255,255},{0,53,255,255},{51,55,54,255},
                                   {39,55,255,255},{52,58,56,255},{57,1,255,255},{54,51,39,54}};
    static const UBYTE counts[8]={2,2,2,3,2,3,2,4};
    static const UBYTE vehicles[8]={1,255,255,1,0,255,2,0};
    static const UBYTE kinds[8]={3,0,4,3,1,0,2,6};
    static const UBYTE gates[8]={8,3,8,8,6,12,6,12};
    for(UBYTE row=0;row<8;row++) {
        native_case();memset(td.complete,255,11);td.done=88;
        td.vehicle=vehicles[row]==TD_NONE?0:vehicles[row];td.mode=TD_BOARD;td.menu=88+row;
        td_get_job(td.menu,&td_offer);
        expect(td_offer.count==counts[row]&&td_offer.vehicle==vehicles[row]&&td_offer.kind==kinds[row]&&
               td_offer.min_done==gates[row]&&!memcmp(td_offer.route,routes[row],counts[row]),
               "each appended Port offer has the independently specified job kind, vehicle, gate and ordered route");
        world_tick(J_A,1);
        expect(td.mode==TD_ROAM&&td.job==88+row&&!td.stage&&td.health==100&&td.left==td_offer.seconds,
               "the real board handler accepts each compatible unlocked Port offer without altering prior completion credit");
        for(UBYTE stage=0;stage<counts[row];stage++) {
            td_stop_t client,other;td_get_stop(routes[row][stage],&client);
            td.onfoot=vehicles[row]==TD_NONE;td_set_target();
            expect(td_target.u==client.u&&td_target.v==client.v&&td_target.district==client.district&&
                   !strcmp(td_target.name,client.name),
                   "each ordered Port objective resolves the true client in its authored district");
            td.district=test_current_district=(client.district+1)%TD_DISTRICT_COUNT;
            td.u=client.u*16;td.v=client.v*16;
            UWORD cash=td.cash,left=td.left;unsigned stores=sram_writes;
            td_interact();
            expect(td.stage==stage&&td.job==88+row&&td.msg==6&&td.cash==cash&&td.left==left&&
                   td.done==88&&sram_writes==stores,
                   "equal client coordinates in another loaded district cannot advance, pay or save a Port handoff");
            UBYTE other_stage=0;
            while(routes[row][other_stage]==routes[row][stage])other_stage++;
            td_get_stop(routes[row][other_stage],&other);
            td.district=test_current_district=other.district;td.u=other.u*16;td.v=other.v*16;
            td_interact();
            expect(td.stage==stage&&td.job==88+row&&td.msg==6&&td.cash==cash&&td.left==left&&
                   td.done==88&&sram_writes==stores,
                   "an otherwise real pickup, future handoff or return endpoint cannot skip the current ordered Port stage");
            td.district=test_current_district=client.district;td.u=client.u*16;td.v=client.v*16;
            td.speed=0;td_interact();
            expect(td.stage==stage+1&&td.job==(stage+1==counts[row]?TD_NONE:88+row)&&
                   td.mode==(stage+1==counts[row]?TD_RESULT:TD_ROAM)&&
                   td.done==(stage+1==counts[row]?89:88),
                   "only the current eligible client advances one actual stage and only the final handoff credits completion");
        }
    }
}

/* Fixed pre-Port version6/7/8 disk layout. These ROMs had88 jobs and51 stops,
   but already stored58 bytes and a16-byte bitmap. Encode offsets independently
   instead of memcpying today's struct into an alleged historical snapshot. */
static void write_pre_port_record(UBYTE version,const td_state_t *state) {
    UBYTE bytes[58]={0};volatile UBYTE *record=td_save_address(0);UWORD crc=0xFFFF;
    old_word(bytes,0,state->u);old_word(bytes,2,state->v);old_word(bytes,4,state->park_u);old_word(bytes,6,state->park_v);
    old_word(bytes,8,state->cash);old_word(bytes,10,state->seconds);old_word(bytes,12,state->left);old_word(bytes,14,state->speed);
    bytes[16]=state->heading;bytes[17]=state->vehicle;bytes[18]=state->onfoot;bytes[19]=state->mode;
    bytes[20]=state->menu;bytes[21]=state->job;bytes[22]=state->stage;bytes[23]=state->health;
    bytes[24]=state->done;bytes[25]=state->subsecond;memcpy(bytes+26,state->complete,11);
    bytes[42]=state->transit_origin;bytes[43]=state->transit_target;bytes[44]=state->ride_left;
    bytes[45]=state->cooldown;bytes[46]=state->msg;bytes[47]=state->reserved;
    old_word(bytes,48,state->safe_u);old_word(bytes,50,state->safe_v);
    old_word(bytes,52,version<8?1400:state->wanted);old_word(bytes,54,version<8?2200:state->wanted_left);
    bytes[56]=state->district;bytes[57]=state->park_district;
    record[0]=0x54;record[1]=0xD7;record[2]=version;record[3]=58;record[4]=37;record[7]=0;
    for(unsigned i=2;i<=4;i++)crc=td_crc_byte(crc,record[i]);
    for(unsigned i=0;i<58;i++){record[8+i]=bytes[i];crc=td_crc_byte(crc,bytes[i]);}
    record[5]=crc;record[6]=crc>>8;
}

static void test_port_save_and_final_credit(void) {
    for(UBYTE version=6;version<=8;version++) {
        native_case();td.cash=5678;td.seconds=65535;td.subsecond=59;td.left=203;
        td.vehicle=3;td.heading=9;td.job=87;td.stage=8;td.health=73;
        memset(td.complete,255,11);td.done=88;td.cooldown=7;td.msg=12;
        if(version==8){td.wanted=2;td.wanted_left=17;}
        td_state_t historical=td;write_pre_port_record(version,&historical);memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&historical,58),
               "an independently encoded pre-Port58-byte v6/v7/v8 snapshot preserves active job87, all88 completions and prior fields");
        int empty=1;for(unsigned i=11;i<16;i++)if(td.complete[i])empty=0;
        expect(empty&&td.done==88&&td.job==87&&td.stage==8&&td.complete[10]==255,
               "expanding the campaign leaves every new quest bit unset without retiring an existing carried contract");
        td.job=TD_NONE;td_ready_offer();
        expect(td.menu==89&&td_offer.vehicle==TD_NONE,
               "all88 old completions unlock the next compatible appended offer while skipping truck-only work for the saved scooter");
    }
    native_case();td.district=4;td.onfoot=1;td.u=td.safe_u=352*16;td.v=td.safe_v=592*16;
    td.job=93;td.stage=1;td_get_job(td.job,&td_job);td.left=161;td.health=67;
    td.cash=2417;td.seconds=65535;td.subsecond=59;td.wanted=2;td.wanted_left=17;
    td.complete[0]=255;td.complete[1]=15;td.done=12;td.transit_origin=49;td.transit_target=43;
    td_state_t saved=td;td_save();
    expect(td_save_address(td_save_slot)[2]==TD_SAVE_VERSION&&td_save_address(td_save_slot)[3]==58,
           "a new carried Port contract writes the current version with unchanged58-byte payload");
    memset(&td,0,sizeof(td));expect(td_restore()&&!memcmp(&td,&saved,58),
           "active Port job93 at Riverbank restores its exact deadline, condition, bitmap, heat, foot position and remote Core car");
    memset(&td,0,sizeof(td));td_session_live=0;load_authored_scene_fixture();toronto_init();
    expect(td.mode==TD_HELP&&td_resume_mode==TD_ROAM&&td_transition_pending==1&&test_queued_district==4,
           "cold recovery queues the real Port scene before exposing its saved active foot contract");
    td_state_t waiting=td;world_tick(0,600);
    expect(!memcmp(&td,&waiting,58),"pending Port recovery freezes every saved byte through a long video-frame gap");
    apply_queued_scene();waiting.mode=TD_HELP;
    expect(!memcmp(&td,&waiting,58)&&!td_transition_pending&&test_current_district==4&&
           td_job.route[td.stage]==58&&td_target.u==352&&td_target.v==592&&td_target.district==4,
           "the warm Port load restores the same carried stage, true foot objective and remote car without replaying a handoff");
    world_tick(J_A,1);waiting.mode=TD_ROAM;
    expect(!memcmp(&td,&waiting,58),"leaving recovery help resumes the saved Port contract without charging or consuming its clock");

    native_case();memset(td.complete,255,11);td.complete[11]=127;td.done=95;
    td.mode=TD_BOARD;td.menu=95;td_get_job(95,&td_offer);world_tick(J_A,1);
    expect(td.job==95&&!td.stage&&td.done==95,"the last appended job accepts after95 unique completions");
    static const UBYTE last_route[4]={54,51,39,54};
    UWORD reward=(UWORD)((unsigned)td_fixture_jobs[95].reward*67/100+99/5);
    for(unsigned repeat=0;repeat<2;repeat++) {
        if(repeat){td.mode=TD_BOARD;td.menu=95;td_get_job(95,&td_offer);world_tick(0,0);world_tick(J_A,1);}
        for(UBYTE stage=0;stage<4;stage++) {
            td_stop_t stop;td_get_stop(last_route[stage],&stop);
            td.district=test_current_district=stop.district;td.u=stop.u*16;td.v=stop.v*16;
            td.left=99;td.health=67;td.speed=0;td_set_target();td_interact();
        }
        expect(td.job==TD_NONE&&td.mode==TD_RESULT&&td.stage==4&&td.done==96&&td.complete[11]==255&&
               td.cash==30+(repeat+1)*reward,
               "job95 records bit95 and reaches96 exactly once; a full replay retains unique credit and earns the normal condition/time reward");
        td_state_t finished=td;finished.mode=TD_ROAM;memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&finished,58),
               "the final96-completion record and bounded repeat reward survive the actual save/restore path");
    }
}

static void test_port_freight_transit_and_riverbank_walk(void) {
    native_case();memset(td.complete,255,11);td.done=88;td.mode=TD_BOARD;td.menu=88;td_get_job(88,&td_offer);
    world_tick(J_A,1);
    expect(td.job==TD_NONE&&td.mode==TD_BOARD&&td.msg==2&&td.cash==30,
           "the actual Port stock contract rejects acceptance with the courier's car instead of its required truck");
    td.vehicle=1;world_tick(0,0);world_tick(J_A,1);
    expect(td.job==88&&td.vehicle==1&&td_job.kind==3,"the same Port freight offer accepts with its required truck");
    for(UBYTE stage=0;stage<2;stage++) {
        td.onfoot=1;td.stage=stage;td.u=574*16;td.v=720*16;
        td.transit_origin=43;td.transit_target=49;td.ride_left=0;
        td_state_t before=td;unsigned stores=sram_writes;td_transit_open();
        before.msg=8;
        expect(!memcmp(&td,&before,58)&&td.mode==TD_ROAM&&sram_writes==stores,
               "walking beside real Union transit cannot turn empty or carried Port truck freight into a paid waiting/ride state");
    }
    native_case();test_current_district=td.district=td.park_district=4;
    td.u=td.park_u=td.safe_u=192*16;td.v=td.park_v=td.safe_v=552*16;
    td.job=93;td.stage=1;td.left=180;td_get_job(td.job,&td_job);td_set_target();
    td_stop_t client;td_get_stop(58,&client);
    int full_foot=1;
    for(unsigned y=(client.v-2)>>3;y<=(unsigned)(client.v+2)>>3;y++)
        for(unsigned x=(client.u-2)>>3;x<=(unsigned)(client.u+2)>>3;x++)
            if(native_collision[4][y*native_widths[4]+x]&15)full_foot=0;
    expect(client.u==352&&client.v==592&&client.district==4&&(client.reserved&TD_STOP_FOOT)&&!client.transit&&
           full_foot&&td_drivable(192,552),
           "Riverbank retains authored foot delivery even though sidewalk driving is legal; its road parking anchor and full human body remain clear");
    expect(td_target.u==192&&td_target.v==552&&td_target.district==4,
           "driving job93 stage1 marks Riverbank's real legal road approach");
    td_interact();expect(td.stage==1&&td.msg==16&&td.job==93,"the Riverbank parking beacon cannot deliver from inside the car");
    td_enter_exit();
    expect(td.onfoot&&td_entry_timer==12&&td.park_u==192*16&&td.park_v==552*16&&
           td.u==192*16&&td.v==570*16&&td_target.u==352&&td_target.v==592,
           "actual Riverbank car exit chooses a connected visible door and switches the objective to the foot client");
    for(unsigned i=0;i<12;i++)driving_tick(0);
    td_interact();expect(td.stage==1&&td.msg==6,"the actual walking door is still too far from the Riverbank parcel");
    /* Use ordinary walking through a fixed authored clear L-shaped approach.
       No fixture placement or unchecked teleport connects parking and client. */
    for(unsigned i=0;i<320;i++)driving_tick(J_RIGHT);
    for(unsigned i=0;i<44;i++)driving_tick(J_DOWN);
    expect(td.u==352*16&&td.v==592*16&&td.onfoot&&td_foot_free(td.u,td.v),
           "the courier walks the full182px door-to-Riverbank route on the actual Port collision map");
    td_interact();expect(td.stage==2&&td.job==93&&td.mode==TD_ROAM&&td_target.u==432&&td_target.v==888,
           "the true Riverbank handoff advances exactly once to the next Beach foot parcel");
    td_interact();expect(td.stage==2&&td.msg==6,"Select again at Riverbank cannot also deliver the distant next parcel");
    for(unsigned i=0;i<44;i++)driving_tick(J_UP);
    for(unsigned i=0;i<320;i++)driving_tick(J_LEFT);
    expect(td.u==192*16&&td.v==570*16&&td_near_car()&&td.park_district==4,
           "the courier walks back to the same saved car door without moving the parked vehicle");
    td_enter_exit();for(unsigned i=0;i<12;i++)driving_tick(0);
    expect(!td.onfoot&&td.u==192*16&&td.v==552*16&&td.job==93&&td.stage==2&&
           td_target.u==352&&td_target.v==856&&td_target.district==4,
           "ordinary car re-entry retains the completed Riverbank stage and changes the next Beach objective to its legal road approach");
}

static void test_atlas_driver_handoff_and_freeze(void) {
    for(UBYTE district=0;district<TD_DISTRICT_COUNT;district++) {
        native_case();test_current_district=td.district=district;
        td.onfoot=district&1;td.park_district=(district+1)%TD_DISTRICT_COUNT;
        td.park_u=400*16;td.park_v=528*16;td.speed=0;
        td.job=80;td_get_job(td.job,&td_job);td.stage=1;td.left=199;td.seconds=100;
        td.wanted=2;td.wanted_left=21;td_set_target();
        camera_x=12000+district*16;camera_y=8000+district*16;camera_settings=0x5A;
        UWORD original_camera_x=camera_x,original_camera_y=camera_y;
        world_tick(J_START,300);
        expect(td.mode==TD_PAUSE&&td_resume_mode==TD_ROAM&&td.seconds==100&&td.left==199,
               "opening pause for the atlas preserves the active cross-district contract clock");
        td.menu=1;world_tick(J_A,300);
        expect(td.mode==TD_MAP&&test_map_opens==1&&test_map_active&&camera_settings==0,
               "actual pause choice opens the dedicated atlas UI exactly once");
        expect(td.wanted==2&&td.wanted_left==21,
               "atlas browsing does not reuse serialized police attention fields");
        td_state_t frozen=td;td_job_t frozen_job=td_job;
        td_stop_t frozen_target=td_target,frozen_cursor=td_cursor;
        actor_t frozen_actors[22];memcpy(frozen_actors,actors,sizeof(actors));
        UWORD frozen_traffic_u[8],frozen_traffic_v[8];UBYTE frozen_traffic_leg[8],frozen_ped_route[8];
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
    td.seconds=48+(closed?4:0);td.subsecond=0;
    load_authored_scene_fixture();toronto_init();
    td.mode=TD_TRANSIT;td.transit_origin=46;td.transit_target=48;td_get_stop(48,&td_cursor);
    td_streetcar_runtime_reset();td_streetcar_runtime_prepare(0);
    world_tick(J_A,1);
    expect(td.mode==(closed?TD_WAIT:TD_RIDE)&&td.cash==(closed?30:27)&&td.ride_left==(closed?0:32),
           "visual Queen fixture books the real Yonge-to-Saulter trip with its exact fare and 32 ride seconds");
}

static void advance_queen_to_east_view(void) {
    for(unsigned second=0;second<32&&!td_transition_pending;second++)world_tick(0,60);
    expect(td_transition_pending==1&&test_queued_district==3&&td.mode==TD_RIDE&&td.ride_left,
           "ordinary paid Queen updates queue the actual East scene before alighting");
}

static void test_streetcar_loader_binding(void) {
    for(UBYTE district=0;district<TD_DISTRICT_COUNT;district++)for(UBYTE active=0;active<2;active++){
        UBYTE has_tram=district==0||district==1||district==3;
        native_case();test_current_district=district;load_authored_scene_fixture();
        if(active&&has_tram)activate_actor(&actors[1]);
        actors[1].script.bank=7;actors[1].script_update.bank=8;
        actors[1].hscript_hit=111;actors[1].hscript_update=222;
        UBYTE bound=td_streetcar_runtime_bind();actor_t *tram=&actors[TD_STREETCAR_ACTOR];
        expect(bound==has_tram&&tram->sprite.ptr==(has_tram?&test_tram_sprite:&test_player_sprite)&&
               tram->base_tile==(has_tram?96:4),
               "binding captures the authored sheet only in the three registered Queen districts");
        expect((tram->flags&(ACTOR_FLAG_ACTIVE|ACTOR_FLAG_PERSISTENT|ACTOR_FLAG_HIDDEN))==
               (ACTOR_FLAG_ACTIVE|ACTOR_FLAG_PERSISTENT|ACTOR_FLAG_HIDDEN)&&
               !tram->collision_group&&!tram->script.bank&&!tram->script_update.bank&&
               !tram->hscript_hit&&!tram->hscript_update&&(!has_tram||tram->frame==8),
               "bound tram is initially hidden and linked once without the loader's scripts, collision or animation handles");
        expect(test_actors_active_head==tram&&!tram->prev&&!tram->next&&
               (!has_tram||(!actors[1].prev&&!actors[1].next))&&actors_inactive_head==NULL,
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
    expect(actors[TD_STREETCAR_ACTOR].sprite.ptr==&test_tram_sprite&&actors[TD_STREETCAR_ACTOR].base_tile==96&&
           !(actors[TD_STREETCAR_ACTOR].flags&ACTOR_FLAG_HIDDEN)&&(PLAYER.flags&ACTOR_FLAG_HIDDEN)&&
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
           (PLAYER.flags&ACTOR_FLAG_HIDDEN)&&!(actors[TD_STREETCAR_ACTOR].flags&ACTOR_FLAG_HIDDEN),
           "warm visual entry does not restore twice and presents the recovered paid rider on the correct native tram");
}

static void test_queen_prior_timetable_paid_save(void) {
    /* This is the exact old Yonge eastbound departure: second12, eight
       seconds left to Saulter. The payload remains v10/58 after the
       timetable change, so preserve its already-paid remaining duration. */
    queen_runtime_case(0);td.seconds=12;td.subsecond=1;td.ride_left=8;td_save();
    td_state_t paid=td;unsigned writes=sram_writes;
    memset(&td,0,sizeof(td));td_session_live=0;test_current_district=0;
    load_authored_scene_fixture();toronto_init();
    expect(td.mode==TD_HELP&&td_resume_mode==TD_RIDE&&td.ride_left==8&&td.cash==27&&
           td.seconds==12&&td.subsecond==1&&td.transit_origin==46&&td.transit_target==48&&
           td.park_district==paid.park_district&&td.park_u==paid.park_u&&td.park_v==paid.park_v&&
           sram_writes==writes&&!td_streetcar_ride_view,
           "a prior-timetable paid58-byte save keeps its clock, remaining ride, fare and car despite lacking a current visual pose");
    world_tick(0,1);world_tick(J_A,1);
    for(unsigned update=0;update<20&&td.mode==TD_RIDE;update++){
        if(td_transition_pending)apply_queued_scene();else world_tick(0,60);
    }
    expect(td.mode==TD_ROAM&&td.onfoot&&td.district==3&&td.cash==27&&!td.ride_left&&
           td.park_district==paid.park_district&&td.park_u==paid.park_u&&td.park_v==paid.park_v,
           "the old paid duration safely reaches its booked native destination without a second fare or loss of the remotely parked car");
}

static void test_queen_visual_queue_retry_clock(void) {
    queen_runtime_case(0);test_queue_fail=1;
    for(unsigned second=0;second<32&&!td_transition_pending;second++)world_tick(0,60);
    expect(td_transition_pending==2&&td.mode==TD_RIDE&&td.district==0&&td.cash==27&&
           td_streetcar_view_district==3&&test_queued_district==TD_DISTRICT_NONE,
           "failed visual scene allocation preserves the paid origin trip and records the derived East retry");
    td_state_t frozen=td;unsigned writes=sram_writes;actor_t old_actors[22];memcpy(old_actors,actors,sizeof(actors));
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
        td_state_t running=td;actor_t tram=actors[TD_STREETCAR_ACTOR];
        UWORD focus_u=td_streetcar_focus_u,focus_v=td_streetcar_focus_v;
        UBYTE district=td_streetcar_view_district;
        world_tick(J_START,600);
        expect(td.mode==TD_PAUSE&&td.seconds==running.seconds&&td.subsecond==running.subsecond&&
               !memcmp(&tram,&actors[TD_STREETCAR_ACTOR],sizeof(tram)),
               "opening pause freezes the actual autonomous tram pose on the same paid or waiting update");
        td.menu=1;world_tick(J_A,600);td_state_t frozen=td;
        world_tick(J_RIGHT|J_DOWN,600);world_tick(J_A,600);world_tick(0,600);
        expect(td.mode==TD_MAP&&!memcmp(&td,&frozen,58)&&!memcmp(&tram,&actors[TD_STREETCAR_ACTOR],sizeof(tram))&&
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
    expect(td_save_address(td_save_slot)[2]==TD_SAVE_VERSION,"successful old rail-car recovery commits the current save version");
    td_state_t recovered=td;memset(&td,0,sizeof(td));
    expect(td_restore()&&td.mode==TD_ROAM&&td.park_u==recovered.park_u&&td.park_v==recovered.park_v&&td.cash==141,
           "the recovered car location is durable and reloads without discarding the valid old earnings");
}

static void test_streetcar_contacts_and_future_traffic_yield(void) {
    for(UBYTE foot=0;foot<2;foot++)for(UBYTE carrying=0;carrying<2;carrying++){
        native_case();td.job=0;td.left=120;td_get_job(0,&td_job);td_set_target();
        if(carrying){td_interact();expect(td.stage==1,"the tram cargo-contact fixture actually collects its Union parcel before testing carrying damage");}
        td.onfoot=foot;td.u=td.safe_u=676*16;td.v=td.safe_v=536*16;td.seconds=48;
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
    native_case();td.seconds=52;td.subsecond=0;td_streetcar_runtime_prepare(0);
    td_streetcar_box_t ahead={699*16,531*16,709*16,541*16,0};
    expect(td_streetcar_sweep(td.seconds,td.subsecond,0,&ahead)==TD_STREETCAR_CLEAR&&
           !td_streetcar_runtime_traffic_clear(0,704*16,536*16),
           "traffic yields to the upcoming one-second Queen body sweep before an actual present overlap");
    td_traffic_u[0]=704*16;td_traffic_v[0]=536*16;td_traffic_leg[0]=0;
    UWORD u=td_traffic_u[0],v=td_traffic_v[0];td_traffic_step();
    expect(td_traffic_u[0]==u&&td_traffic_v[0]==v,"the production autonomous traffic step respects future tram occupancy instead of entering its path");
    expect(td_streetcar_runtime_traffic_clear(0,704*16,520*16),
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
    td_state_t held=td;expect(td_save_address(td_save_slot)[2]==TD_SAVE_VERSION,"real held alighting writes the current semantic discriminator");
    td_session_live=0;memset(&td,0,sizeof(td));test_current_district=0;load_authored_scene_fixture();toronto_init();
    expect(td.mode==TD_HELP&&td_resume_mode==TD_RIDE&&td.reserved==TD_STREETCAR_HOLD&&
           td.u==held.u&&td.v==held.v&&td.cash==held.cash&&td.seconds==held.seconds&&
           td_streetcar_view_district==3&&test_queued_district==3,
           "cold CRC-valid held recovery derives its booked target doors while preserving paid origin bytes and cash");
    apply_queued_scene();world_tick(J_A,1);world_tick(0,1);
    for(unsigned i=0;i<6;i++){td_traffic_u[i]=(800+i*32)*16;td_traffic_v[i]=928*16;}
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
    td.u=td.safe_u=690*16;td.v=td.safe_v=542*16;td.seconds=48;
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
    expect(td.mode==TD_RIDE&&td.cash==booked.cash-3&&td.ride_left==32,
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
        expect(td_streetcar_pose(48,0,&td_streetcar_display),"cue fixture samples the actual Queen Yonge doors");
        td_streetcar_focus_u=td_streetcar_display.u;td_streetcar_focus_v=td_streetcar_display.v;
        td_state_t saved=td;td_streetcar_runtime_present();
        long x=td_streetcar_display.u,y=td_streetcar_display.v+14*16;
        x+=((long)td.u-x)*cue/12;y+=((long)td.v-y)*cue/12;
        expect(PLAYER.pos.x==x*2&&PLAYER.pos.y==y*2&&!(PLAYER.flags&ACTOR_FLAG_HIDDEN)&&
               !memcmp(&td,&saved,58),
               "actual courier presentation matches independent wide-arithmetic endpoints without mutating the saved ride");
    }
    native_case();td.onfoot=1;td.mode=TD_WAIT;td.transit_origin=46;td.transit_target=48;
    td.u=td.safe_u=676*16;td.v=td.safe_v=536*16;td.seconds=48;
    actors[2].pos.x=676*32;actors[2].pos.y=536*32;
    td_state_t trapped=td;
    expect(td_streetcar_runtime_recover_contact(1)==TD_STREETCAR_PARK_BLOCKED&&
           !memcmp(&td,&trapped,58),
           "fully obstructed contact reports BLOCKED and preserves state instead of inventing an unvalidated escape");
}

static void contact_blocked_case(UBYTE foot) {
    native_case();td.onfoot=foot;td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=120;
    td.u=td.safe_u=676*16;td.v=td.safe_v=536*16;td.seconds=48;
    td_traffic_u[0]=td.u;td_traffic_v[0]=td.v;td_traffic_leg[0]=0;
    td_traffic_present();
}
static void test_contact_episode_and_invalid(void) {
    for(UBYTE foot=0;foot<2;foot++){
        contact_blocked_case(foot);td_state_t before=td;
        for(unsigned frame=0;frame<180;frame++){
            /* Hold the body pose fixed to isolate a contact episode from
               timetable departure; real update/motion/cooldown still run. */
            td.seconds=48;td.subsecond=0;world_tick(foot?J_DOWN:J_A,1);
        }
        expect(td.u==before.u&&td.v==before.v&&td.safe_u==before.safe_u&&td.safe_v==before.safe_v&&
               td.cash==before.cash&&td.left==before.left&&td.stage==before.stage,
               "persistent genuine BLOCKED contact never teleports or changes fare, deadline or cargo stage");
        expect(td.health==(foot?100:88)&&audio_impacts==(foot?0:1)&&td_contact_episode,
               "persistent blocked tram contact gives occupied cargo one impact episode despite expired cooldown");
        expect(td.msg==(foot?18:5)&&td_traffic_u[0]==before.u&&td_traffic_v[0]==before.v,
               "blocked cues remain visible and an off-route Queen NPC cannot invent a retreat segment");
        expect(td.park_u==before.park_u&&td.park_v==before.park_v,
               "a failed contact separation cannot relocate the parked vehicle");
        td.u=600*16;td.v=556*16;world_tick(0,1);
        expect(!td_contact_episode,"only a valid clear contact result releases the impact episode latch");
        td.u=before.u;td.v=before.v;td.seconds=48;td.subsecond=0;td.cooldown=0;world_tick(0,1);
        expect(td.health==(foot?100:76)&&audio_impacts==(foot?0:2),
               "a later independently reacquired blocked contact starts exactly one fresh impact episode");
    }
    contact_blocked_case(0);td.u=1;td.safe_u=1;td_traffic_u[0]=1;
    td_state_t invalid=td;unsigned writes=sram_writes;world_tick(J_A,1);
    expect(td.health==invalid.health&&!audio_impacts&&sram_writes==writes&&
           td.u==invalid.u&&td.v==invalid.v&&td_traffic_u[0]==1&&!td_contact_episode,
           "invalid contact geometry fails closed without inventing cargo damage, save writes or traffic movement");
    expect(td_streetcar_runtime_recover_contact(2)==TD_STREETCAR_CONTACT_INVALID&&
           td_streetcar_runtime_recover_contact(1)==TD_STREETCAR_CONTACT_INVALID,
           "invalid or mismatched contact arguments are distinct from a proven obstructed collision");
    contact_blocked_case(1);td.mode=TD_WAIT;td.transit_origin=46;td.transit_target=48;
    td.u=td.safe_u=676*16;td.v=td.safe_v=543*16;td_traffic_v[0]=td.v;td_traffic_present();
    td_state_t waiting=td;world_tick(0,1);
    expect(td.mode==TD_WAIT&&td.msg==18&&td.cash==waiting.cash&&td.u==waiting.u&&td.v==waiting.v&&
           td.transit_origin==waiting.transit_origin&&td.transit_target==waiting.transit_target,
           "a genuine blocked contact at a valid waiting platform shows its cue without moving, charging or changing the booking");
    world_tick(J_B,1);
    expect(td.mode==TD_ROAM&&td.cash==waiting.cash&&!td.ride_left,
           "a blocked contact cue never prevents deliberate uncharged WAIT cancellation");
}

static void reverse_retreat_case(void) {
    native_case();td.onfoot=1;td.u=824*16;td.v=537*16;td.seconds=57;td.subsecond=48;
    td_traffic_u[4]=824*16;td_traffic_v[4]=536*16;td_traffic_leg[4]=1;
    td_world_traffic_samples(0,td_traffic_leg,td_traffic_samples);
    td_streetcar_runtime_prepare(0);td_traffic_present();
}
static void test_banked_traffic_segments(void) {
    const UWORD core[6][6][2]={
        {{840,280},{48,280},{48,296},{840,296}},
        {{840,392},{48,392},{48,408},{840,408}},
        {{840,168},{48,168},{48,184},{840,184}},
        {{832,632},{48,632},{48,648},{832,648}},
        {{824,792},{824,48},{808,48},{808,792}},
        {{640,72},{808,72},{808,168},{640,168},{216,168},{216,72}}
    };
    native_case();UBYTE legs[6]={0};UWORD from_u,from_v;
    for(UBYTE slot=0;slot<6;slot++)for(UBYTE leg=0;leg<(slot==5?6:4);leg++){
        UBYTE count=slot==5?6:4,previous=leg?leg-1:count-1;legs[slot]=leg;
        UWORD u=core[slot][previous][0]*16,v=core[slot][previous][1]*16;
        UWORD target_u=core[slot][leg][0]*16,target_v=core[slot][leg][1]*16;
        UBYTE before[6];memcpy(before,legs,6);td_state_t saved=td;
        expect(td_streetcar_runtime_traffic_segment(0,slot,count,legs,u,v,target_u,target_v,&from_u,&from_v)&&
               from_u==u&&from_v==v,
               "the banked query preserves every authored Core vehicle and bus segment endpoint");
        expect(td_streetcar_runtime_traffic_segment(0,slot,count,legs,(u+target_u)/2,(v+target_v)/2,
               target_u,target_v,&from_u,&from_v),"the banked segment query accepts bounded interior route points");
        expect(!td_streetcar_runtime_traffic_segment(0,slot,count,legs,(u+target_u)/2+16,(v+target_v)/2+16,
               target_u,target_v,&from_u,&from_v),"a diagonal off-route offset cannot acquire a reverse segment");
        expect(!memcmp(before,legs,6)&&!memcmp(&td,&saved,58),"banked Core segment queries preserve all caller legs and saved bytes");
    }
    for(UBYTE district=1;district<TD_DISTRICT_COUNT;district++){
        UWORD start_u[6],start_v[6];td_traffic_sample_t target[6],prior[6];
        if(district==TD_DISTRICT_ISLANDS){
            memset(start_u,0x37,sizeof(start_u));memset(start_v,0x37,sizeof(start_v));
            memset(target,0x37,sizeof(target));memset(legs,0x37,sizeof(legs));
            UWORD before_u[6],before_v[6];td_traffic_sample_t before_target[6];UBYTE before_legs[6];
            memcpy(before_u,start_u,sizeof(start_u));memcpy(before_v,start_v,sizeof(start_v));
            memcpy(before_target,target,sizeof(target));memcpy(before_legs,legs,sizeof(legs));
            expect(!td_world_traffic_init(district,start_u,start_v,legs,target)&&
                   !memcmp(before_u,start_u,sizeof(start_u))&&!memcmp(before_v,start_v,sizeof(start_v))&&
                   !memcmp(before_target,target,sizeof(target))&&!memcmp(before_legs,legs,sizeof(legs)),
                   "the actual foot-only Island has no authored traffic route and preserves every rejected output");
            continue;
        }
        expect(td_world_traffic_init(district,start_u,start_v,legs,target),"segment fixture reads real registered district traffic");
        for(UBYTE slot=0;slot<6;slot++){
            UBYTE count=target[slot].count;
            for(UBYTE leg=0;leg<count;leg++){
                legs[slot]=leg;expect(td_world_traffic_samples(district,legs,target),"segment target comes from actual district route metadata");
                UBYTE previous_legs[6];memcpy(previous_legs,legs,6);previous_legs[slot]=leg?leg-1:count-1;
                expect(td_world_traffic_samples(district,previous_legs,prior),"segment predecessor comes from actual route metadata");
                UBYTE before[6];memcpy(before,legs,6);td_state_t saved=td;
                expect(td_streetcar_runtime_traffic_segment(district,slot,count,legs,prior[slot].u,prior[slot].v,
                       target[slot].u,target[slot].v,&from_u,&from_v)&&from_u==prior[slot].u&&from_v==prior[slot].v,
                       "banked route validation retains actual West, High Park and East predecessor endpoints");
                expect(!memcmp(before,legs,6)&&!memcmp(&td,&saved,58),"banked district route queries preserve WRAM inputs and saved state");
            }
        }
    }
    memset(legs,0,6);
    expect(!td_streetcar_runtime_traffic_segment(TD_DISTRICT_COUNT,0,4,legs,48*16,280*16,840*16,280*16,&from_u,&from_v)&&
           !td_streetcar_runtime_traffic_segment(0,6,4,legs,48*16,280*16,840*16,280*16,&from_u,&from_v)&&
           !td_streetcar_runtime_traffic_segment(0,0,2,legs,48*16,280*16,840*16,280*16,&from_u,&from_v),
           "unknown banked route districts, slots and incompatible Core counts fail closed");
    expect(!td_streetcar_runtime_traffic_segment(0,0,4,NULL,48*16,280*16,840*16,280*16,&from_u,&from_v)&&
           !td_streetcar_runtime_traffic_segment(0,0,4,legs,48*16,280*16,840*16,280*16,NULL,&from_v)&&
           !td_streetcar_runtime_traffic_segment(0,0,4,legs,48*16,280*16,840*16,280*16,&from_u,NULL),
           "null banked route input/output pointers fail closed");
}
static void test_connected_traffic_retreat(void) {
    /* Native ordinary save/reset reproduced this exact valid cold slot4
       obstruction without debugger writes: courier824,238; NPC824,240. */
    native_case();td.onfoot=1;td.u=824*16;td.v=238*16;td.seconds=34;td.subsecond=30;
    td_traffic_u[4]=824*16;td_traffic_v[4]=240*16;td_traffic_leg[4]=1;
    td_world_traffic_samples(0,td_traffic_leg,td_traffic_samples);td_traffic_present();
    td_state_t reset_overlap=td;
    for(unsigned frame=0;frame<120;frame++)world_tick(J_DOWN,1);
    expect(td.v>reset_overlap.v&&td.u==reset_overlap.u&&td.cash==reset_overlap.cash&&td.health==100&&
           td_traffic_v[4]>240*16&&td_traffic_leg[4]==1,
           "the right-hand cold-pose obstruction separates through connected backup while held walking resumes");

    reverse_retreat_case();td_state_t courier=td;UWORD start=td_traffic_v[4];
    td_streetcar_pose_t pose;td_streetcar_box_t tram;
    expect(td_streetcar_pose(td.seconds,td.subsecond,&pose)&&td_streetcar_bounds(&pose,&tram)&&
           tram.district==0&&tram.left<=824*16+80&&tram.right>=824*16-80,
           "coherent Core slot4 northbound leg1 meets the actual moving Queen tram at57 seconds48 ticks");
    expect(!td_streetcar_runtime_traffic_clear(0,824*16,536*16-8),
           "ordinary future yield alone forbids a still-overlapping half-pixel retreat");
    td_traffic_step();
    expect(td_traffic_v[4]==start-8&&td_traffic_u[4]==824*16&&td_traffic_leg[4]==1,
           "a courier behind a real northbound NPC permits a bounded connected separating step");
    UWORD previous=td_traffic_v[4];
    for(unsigned step=0;step<40;step++){
        td_traffic_step();
        expect(td_traffic_v[4]<=previous&&previous-td_traffic_v[4]<=8,
               "a connected retreat never accelerates, oscillates, or leaves its authored segment");
        previous=td_traffic_v[4];
    }
    td_traffic_present();
    expect(!td_traffic_retreat_mask&&td_traffic_free(td.u,td.v)&&td_distance(td.v,actors[6].pos.y>>1)>=180&&
           td_traffic_leg[4]==1&&actors[6].frame_start==11&&actors[6].pos.x==824*32,
           "retreat clears both cached walking and rounded actor recovery margins without changing route leg or facing");
    expect(td_streetcar_runtime_recover_contact(1)==TD_STREETCAR_PARK_MOVED&&
           td_streetcar_runtime_foot_clear(td.u,td.v)&&td.cash==courier.cash&&td.health==courier.health,
           "after the coherent NPC retreats the real connected contact solver releases the walker safely");

    native_case();td.onfoot=1;td.u=409*16;td.v=280*16;
    td_traffic_u[0]=400*16;td_traffic_v[0]=280*16;td_traffic_leg[0]=1;
    td_world_traffic_samples(0,td_traffic_leg,td_traffic_samples);
    expect(td_distance(td.u,td_traffic_u[0])==144&&!td_traffic_free(td.u,td.v),
           "a9px courier offset exceeds an8.5px visible-body margin but remains inside the actual10.5px walking exclusion");
    for(unsigned step=0;step<40;step++)td_traffic_step();
    expect(td_traffic_u[0]<400*16&&td_traffic_free(td.u,td.v)&&!td_traffic_retreat_mask,
           "a9px existing obstruction retreats until usable walking space clears instead of requiring visual overlap");
    UWORD walker=td.u;driving_tick(J_RIGHT);
    expect(td.u>walker,"walking resumes after an ordinary connected NPC retreat");

    for(UBYTE obstacle=0;obstacle<4;obstacle++){
        reverse_retreat_case();start=td_traffic_v[4];
        if(obstacle==0){td.park_u=824*16;td.park_v=535*16;}
        if(obstacle==1){td_traffic_u[0]=824*16;td_traffic_v[0]=535*16;}
        if(obstacle==2){actors[9].pos.x=824*32;actors[9].pos.y=535*32;actors[9].flags&=~ACTOR_FLAG_HIDDEN;}
        if(obstacle==3)geometry=EAST_WALL;
        td_traffic_step();
        expect(td_traffic_v[4]==start,"retreat retains parked-car, other-vehicle, visible pedestrian and full-road guards");
    }
    native_case();td.onfoot=1;td.u=57*16;td.v=280*16;
    td_traffic_u[0]=840*16;td_traffic_v[0]=280*16;td.u=831*16;td_traffic_step();
    expect(td_traffic_u[0]==840*16,"separation cannot back up beyond the authored right-hand segment start");
    native_case();td.onfoot=1;td.u=545*16;td.v=280*16;td.seconds=7;
    td_traffic_u[0]=536*16;td_traffic_v[0]=280*16;td_traffic_step();
    expect(td_traffic_u[0]==536*16-8,"a validated overlapping retreat moves away from the courier and the red entry line without entering the junction");
    reverse_retreat_case();td.u=824*16;td.v=549*16;start=td_traffic_v[4];td_traffic_step();
    expect(td_traffic_v[4]==start&&!td_traffic_retreat_mask,
           "incoming traffic inside13px but outside the actual walking exclusion still yields without an escape exception");
    expect(!td_streetcar_runtime_traffic_retreat(0,824*16,536*16,824*16+8,536*16+8)&&
           !td_streetcar_runtime_traffic_retreat(0,824*16,536*16,824*16,536*16-9)&&
           !td_streetcar_runtime_traffic_retreat(0,65535,536*16,65527,536*16),
           "diagonal, oversized and invalid-geometry traffic retreat requests fail closed");
}

static int retreat_future_oracle(UBYTE district,UWORD old_u,UWORD old_v,UWORD u,UWORD v) {
    for(unsigned ticks=0;ticks<=60;ticks++){
        unsigned phase=td.subsecond+ticks;td_streetcar_pose_t pose;
        if(!td_streetcar_pose((UWORD)(td.seconds+phase/60),phase%60,&pose))return 0;
        if(pose.district!=district)continue;
        if((u>old_u&&old_u<pose.u)||(u<old_u&&old_u>pose.u)||
           (v>old_v&&old_v<pose.v)||(v<old_v&&old_v>pose.v))return 0;
        long old_distance=labs((long)old_u-pose.u)+labs((long)old_v-pose.v);
        long new_distance=labs((long)u-pose.u)+labs((long)v-pose.v);
        if(new_distance<=old_distance)return 0;
        long hx=(pose.heading&4?6:14)*16,hy=(pose.heading&4?14:6)*16;
        int old_hit=(long)old_u+80>=pose.u-hx&&(long)old_u-80<=pose.u+hx-1&&
                    (long)old_v+80>=pose.v-hy&&(long)old_v-80<=pose.v+hy-1;
        int new_hit=(long)u+80>=pose.u-hx&&(long)u-80<=pose.u+hx-1&&
                    (long)v+80>=pose.v-hy&&(long)v-80<=pose.v+hy-1;
        if(new_hit&&!old_hit)return 0;
    }
    return 1;
}
static void test_retreat_future_truth(void) {
    const WORD offsets[3][2]={{0,0},{64,-64},{-64,64}};
    native_case();unsigned allowed=0,rejected=0;
    for(unsigned phase=0;phase<15360;phase++){
        td.seconds=(UWORD)(65280+phase/60);td.subsecond=phase%60;td_streetcar_pose_t current;
        expect(td_streetcar_pose(td.seconds,td.subsecond,&current),"retreat oracle spans every native route pose through rollover");
        for(unsigned offset=0;offset<3;offset++)for(unsigned direction=0;direction<4;direction++){
            UWORD old_u=current.u+offsets[offset][0],old_v=current.v+offsets[offset][1];
            UWORD u=old_u+(direction==0?8:direction==1?-8:0),v=old_v+(direction==2?8:direction==3?-8:0);
            td_state_t before=td;int expected=retreat_future_oracle(current.district,old_u,old_v,u,v);
            UBYTE actual=td_streetcar_runtime_traffic_retreat(current.district,old_u,old_v,u,v);
            expect(actual==expected,"optimized band proof and bend fallback match independent wide-arithmetic future-body retreat truth");
            expect(!memcmp(&td,&before,58),"rare traffic depenetration queries do not modify saved gameplay state");
            if(actual)allowed++;else rejected++;
        }
    }
    expect(allowed&&rejected,"future retreat oracle exercises accepted separation and refused approaching directions");
}

static int landing_body_oracle(UBYTE district,UWORD u,UWORD v,const td_streetcar_pose_t *pose) {
    if(district>=TD_DISTRICT_COUNT||u<48||v<48||u+48>=1024*16||v+48>=976*16)return 0;
    for(unsigned y=((v>>4)-3)>>3;y<=((v>>4)+3)>>3;y++)
        for(unsigned x=((u>>4)-3)>>3;x<=((u>>4)+3)>>3;x++)if(district_tile(district,x,y)&15)return 0;
    if(district!=pose->district)return 1;
    unsigned hx=(pose->heading&4?6:14)*16,hy=(pose->heading&4?14:6)*16;
    return u+48<pose->u-hx||u-48>pose->u+hx-1||v+48<pose->v-hy||v-48>pose->v+hy-1;
}
static void test_booked_landing_and_held_occupancy(void) {
    const WORD du[9]={0,192,-192,0,0,288,-288,0,0};
    const WORD dv[9]={0,0,0,192,-192,0,0,288,-288};
    for(UBYTE target=43;target<51;target++)for(UBYTE direction=0;direction<2;direction++){
        if((target==43&&!direction)||(target==50&&direction))continue;
        native_case();td.onfoot=1;td.mode=TD_RIDE;td.transit_origin=direction?50:43;td.transit_target=target;
        td.ride_left=0;td.cash=27;td_get_stop(target,&td_cursor);td_streetcar_pose_t booked;
        expect(td_streetcar_destination(td.transit_origin,target,&booked),"every legal endpoint direction derives explicit booked doors");
        for(unsigned point=0;point<9;point++){
            UWORD u=td_cursor.u*16+du[point],v=td_cursor.v*16+dv[point];td_state_t before=td;
            expect(td_streetcar_runtime_landing_clear(td_cursor.district,u,v)==landing_body_oracle(td_cursor.district,u,v,&booked),
                   "first-arrival landing tests full native foot terrain and explicit directional tram body before HOLD/prepare");
            expect(!memcmp(&td,&before,58),"booked landing queries leave every paid-origin and save byte unchanged");
        }
        td.reserved=TD_STREETCAR_HOLD;td.ride_left=1;
        for(unsigned phase=0;phase<15360;phase++){
            td.seconds=(UWORD)(65280+phase/60);td.subsecond=phase%60;
            td_state_t before=td;
            expect(!td_streetcar_runtime_traffic_clear(booked.district,booked.u,booked.v),
                   "a held paid tram occupies exactly its booked destination body throughout timetable and16-bit clock rollover");
            expect(td_streetcar_runtime_traffic_clear(booked.district,booked.u,booked.v+20*16),
                   "held traffic is clear20px beside booked doors without a phantom autonomous tram");
            td_streetcar_pose_t autonomous;
            expect(td_streetcar_pose(td.seconds,td.subsecond,&autonomous),"held occupancy fixture retains the independent autonomous clock");
            int overlaps_booked=autonomous.district==booked.district&&autonomous.u+80>=booked.u-224&&
                autonomous.u-80<=booked.u+223&&autonomous.v+80>=booked.v-96&&autonomous.v-80<=booked.v+95;
            expect(td_streetcar_runtime_traffic_clear(autonomous.district,autonomous.u,autonomous.v)==!overlaps_booked,
                   "the autonomous phase never adds a second phantom traffic body during a booked destination hold");
            expect(!memcmp(&td,&before,58),"held occupancy queries preserve fare, origin, clock, cargo and serialized layout");
        }
        for(UBYTE menu=0;menu<3;menu++){
            td.mode=menu==0?TD_HELP:menu==1?TD_PAUSE:TD_MAP;td_resume_mode=TD_RIDE;
            expect(!td_streetcar_runtime_traffic_clear(booked.district,booked.u,booked.v),
                   "restored HELP, PAUSE and MAP held rides retain booked body occupancy through effective resumeRIDE");
        }
    }
    queen_runtime_case(0);advance_queen_to_east_view();apply_queued_scene();td.ride_left=1;
    const UWORD blockers[4][2]={{128,556},{140,556},{116,556},{128,568}};
    for(unsigned i=0;i<4;i++){td_traffic_u[i]=blockers[i][0]*16;td_traffic_v[i]=blockers[i][1]*16;}
    td_state_t paid=td;td_second();
    expect(td.mode==TD_RIDE&&td.ride_left==1&&td.reserved==TD_STREETCAR_HOLD&&td.cash==paid.cash&&
           td.u==paid.u&&td.v==paid.v&&td.district==paid.district,
           "when other endpoints are occupied the north12/18px fallback cannot put the courier inside booked Saulter tram doors");
    for(unsigned i=0;i<6;i++){td_traffic_u[i]=(800+i*32)*16;td_traffic_v[i]=928*16;}
    td_second();
    expect(td.mode==TD_ROAM&&td.district==3&&td.u==128*16&&td.v==556*16&&!td.reserved&&td.cash==paid.cash,
           "a blocked first arrival commits one safe landing after clearance without charging the paid fare again");
}

static void test_streetcar_native_q5_bounds(void) {
    native_case();td_streetcar_bound=td_streetcar_valid=1;
    unsigned bad_pose=0,bad_size=0,bad_scaled=0,bad_state=0;UBYTE headings=0;
    for(unsigned phase=0;phase<256*60;phase++){
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
    expect(!bad_pose&&headings==15,"one actual256-second streetcar cycle exercises all four native actor orientations");
    expect(!bad_size,"every native tram presentation uses exact28x12 or12x28 inclusive Q5 actor bounds");
    expect(!bad_scaled,"every Q5 actor endpoint agrees with the actual Q4 physical body including the last half-subpixel");
    expect(!bad_state,"all autonomous Q5-bound presentations preserve the serialized game state");
}
static void test_contact_corridor_coverage(void) {
    const UWORD clear_points[][3][2]={
        {{560,720},{824,240},{500,128}},{{800,480},{512,536},{840,720}},
        {{560,528},{824,240},{500,128}},{{944,504},{400,720},{400,128}},
        /* Clear Leslie/Lake Shore road, service drive and Cherry South
           bridge. Queen-like local y coordinates must not invent service. */
        {{912,128},{672,536},{192,536}},
        /* Actual public ferry checkpoints, not synthetic Island roads. */
        {{320,280},{512,448},{920,280}},
        /* Actual North parking approach, St Clair and Summerhill curbs;
           these full native road bodies have no Queen contact corridor. */
        {{336,576},{688,176},{696,416}}
    };
    _Static_assert(sizeof(clear_points)/sizeof(clear_points[0])==TD_DISTRICT_COUNT,
                   "Every registered district needs explicit off-corridor contact fixtures");
    native_case();
    const td_streetcar_box_t port_lands={0,0,1024*16-1,976*16-1,TD_DISTRICT_PORT_LANDS};
    expect(!td_streetcar_runtime_near(&port_lands),
           "the entire Port Lands district has no Queen contact corridor despite overlap with other districts' local coordinates");
    for(UBYTE point=0;point<3;point++){
        expect(td_district_drivable(TD_DISTRICT_PORT_LANDS,clear_points[TD_DISTRICT_PORT_LANDS][point][0],
                                   clear_points[TD_DISTRICT_PORT_LANDS][point][1]),
               "Port Lands off-corridor fixtures use actual clear eleven-pixel native road footprints");
    }
    for(UBYTE point=0;point<3;point++)
        expect(td_district_drivable(TD_DISTRICT_NORTH,clear_points[TD_DISTRICT_NORTH][point][0],
                                   clear_points[TD_DISTRICT_NORTH][point][1]),
               "North off-corridor fixtures use actual clear eleven-pixel native road footprints");
    for(unsigned phase=0;phase<15360;phase++){
        td.seconds=(UWORD)(65280+phase/60);td.subsecond=phase%60;
        td_streetcar_pose_t pose;td_streetcar_box_t tram;
        expect(td_streetcar_pose(td.seconds,td.subsecond,&pose)&&td_streetcar_bounds(&pose,&tram),
               "corridor coverage samples all current bodies at bends, seams and turnarounds through clock rollover");
        expect(pose.district!=TD_DISTRICT_PORT_LANDS,
               "the full autonomous Queen cycle never produces a Port Lands tram body");
        for(UBYTE corner=0;corner<4;corner++){
            UWORD x=corner&1?tram.right:tram.left,y=corner&2?tram.bottom:tram.top;
            td_streetcar_box_t point={x,y,x,y,tram.district};
            expect(td_streetcar_runtime_near(&point),
                   "every exact tram rectangle corner lies inside its district's conservative contact corridor");
            for(UBYTE foot=0;foot<2;foot++){
                UWORD half=foot?48:80;td_streetcar_box_t player;
                UWORD u=corner&1?x+half:x-half,v=corner&2?y+half:y-half;
                expect(td_streetcar_runtime_box(u,v,half,half,tram.district,&player)&&
                       td_streetcar_runtime_overlap(&tram,&player)&&td_streetcar_runtime_near(&player),
                       "a valid foot/car box touching just one outer tram corner cannot be rejected by the coarse early-out");
            }
        }
        for(UBYTE district=0;district<TD_DISTRICT_COUNT;district++)for(UBYTE foot=0;foot<2;foot++){
            td.district=district;td.onfoot=foot;td.mode=foot?TD_WAIT:TD_ROAM;
            for(UBYTE point=0;point<3;point++){
                td.u=clear_points[district][point][0]*16;td.v=clear_points[district][point][1]*16;
                td_streetcar_box_t player;td_state_t before=td;
                expect(td_streetcar_runtime_box(td.u,td.v,foot?48:80,foot?48:80,district,&player)&&
                       !td_streetcar_runtime_near(&player)&&!td_streetcar_runtime_overlap(&tram,&player)&&
                       td_streetcar_runtime_recover_contact(foot)==TD_STREETCAR_PARK_UNCHANGED&&
                       !memcmp(&td,&before,58),
                       "every valid off-corridor ROAM/WAIT box remains unchanged for the entire autonomous timetable");
                if(district==TD_DISTRICT_PORT_LANDS)
                    expect(td_streetcar_runtime_foot_clear(td.u,td.v)&&
                           td_streetcar_runtime_car_clear(td.u,td.v,td.u,td.v)&&
                           td_streetcar_runtime_traffic_clear(district,td.u,td.v)&&!memcmp(&td,&before,58),
                           "Port Lands foot, car and future traffic queries add no phantom Queen occupancy across clock rollover");
            }
        }
    }
    for(UBYTE foot=0;foot<2;foot++){
        native_case();td.onfoot=foot;td.u=560*16;td.v=720*16;td.subsecond=60;
        expect(td_streetcar_runtime_recover_contact(foot)==TD_STREETCAR_CONTACT_INVALID,
               "off-corridor optimization cannot bypass invalid subsecond validation");
        td.subsecond=0;td.u=foot?47:79;
        expect(td_streetcar_runtime_recover_contact(foot)==TD_STREETCAR_CONTACT_INVALID,
               "off-corridor optimization cannot bypass the actual player footprint boundary");
        td.u=560*16;td.district=TD_DISTRICT_COUNT;
        expect(td_streetcar_runtime_recover_contact(foot)==TD_STREETCAR_CONTACT_INVALID,
               "unknown district player geometry remains invalid before the coarse early-out");
        td.mode=TD_PAUSE;td.subsecond=60;td.u=65535;
        expect(td_streetcar_runtime_recover_contact(foot)==TD_STREETCAR_PARK_UNCHANGED,
               "valid menu-mode calls retain their previous no-contact-query behavior before geometry validation");
        expect(td_streetcar_runtime_recover_contact(!foot)==TD_STREETCAR_CONTACT_INVALID,
               "even menu-mode calls preserve the original on-foot argument mismatch validation");
    }
}

/* Deliberately include the East bend's empty interior, each seam, the
   turnaround lane and ordinary off-corridor traffic; none are generated
   from the production path table. */
static const UWORD traffic_probe_points[][6][2]={
    {{740*16,536*16},{740*16,520*16},{24*16,536*16},{1000*16,520*16},{640*16,528*16},{560*16,720*16}},
    {{836*16,536*16},{836*16,520*16},{912*16,528*16},{1000*16,536*16},{800*16,480*16},{512*16,536*16}},
    {{740*16,536*16},{740*16,520*16},{24*16,536*16},{1000*16,520*16},{640*16,528*16},{560*16,720*16}},
    {{680*16,536*16},{680*16,504*16},{664*16,488*16},{664*16,520*16},{400*16,504*16},{24*16,536*16}},
    {{192*16,536*16},{672*16,536*16},{912*16,536*16},{912*16,128*16},{352*16,824*16},{736*16,128*16}},
    /* Pure body-query probes at actual foot docks/clients; no Island fleet
       is fabricated or made drivable by these Queen exclusion checks. */
    {{320*16,280*16},{512*16,448*16},{920*16,280*16},{160*16,600*16},{512*16,744*16},{904*16,440*16}},
    /* Six actual North fleet loop origins; one distinct authored route per
       slot, independent of the production Queen timetable/path table. */
    {{200*16,568*16},{272*16,760*16},{328*16,568*16},{472*16,152*16},{200*16,152*16},{632*16,328*16}}
};
_Static_assert(sizeof(traffic_probe_points)/sizeof(traffic_probe_points[0])==TD_DISTRICT_COUNT,
               "Every registered district needs explicit traffic lookahead probes");
static void traffic_probe_boxes(UBYTE district,td_streetcar_box_t boxes[6]) {
    for(unsigned i=0;i<6;i++){
        UWORD u=traffic_probe_points[district][i][0],v=traffic_probe_points[district][i][1];
        boxes[i].left=u-80;boxes[i].right=u+80;boxes[i].top=v-80;boxes[i].bottom=v+80;
        boxes[i].district=district;
    }
}
static void test_traffic_lookahead_truth(void) {
    native_case();
    for(UBYTE district=0;district<TD_DISTRICT_COUNT;district++){
        td_streetcar_box_t boxes[6];traffic_probe_boxes(district,boxes);
        for(unsigned phase=0;phase<256*60;phase++){
            td.seconds=65280+phase/60;td.subsecond=phase%60;td_state_t before=td;
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

/* Retained old64sec/half5 digest identity: 496959e1 /6ca5fcfe /b8588dc5 /3321d730.
 * The new16sec timetable gets a same-source full16-section reference rather
 * than copying new implementation output into an alleged independent golden. */
static UBYTE full_section_reference(UWORD first,UWORD last,const td_streetcar_box_t *box){
    td_streetcar_pose_t pose;td_streetcar_box_t body;UWORD a,b;
    for(UBYTE section=0;section<16;section++){
        UWORD base=section*960;
        if(first<=base+239&&last>=base){
            td_streetcar_dwell(section>=8?15-section:section,section>=8,&pose);
            td_streetcar_bounds_local(&pose,&body);
            if(td_streetcar_boxes_hit(&body,box))return TRUE;
        }
        if(first<=base+960&&last>=base+240){
            a=first>base+240?first-base-240:0;b=last<base+960?last-base-240:720;
            if(td_streetcar_travel_hit(td_streetcar_route(section),a,b,box))return TRUE;
        }
    }
    return FALSE;
}
static UBYTE full_sweep_reference(UWORD seconds,UBYTE subsecond,UWORD elapsed,const td_streetcar_box_t *box){
    UWORD phase=(seconds%256)*60+subsecond,first;
    td_streetcar_pose_t pose;td_streetcar_box_t body;
    if(!elapsed){td_streetcar_pose_local(phase,&pose);td_streetcar_bounds_local(&pose,&body);return td_streetcar_boxes_hit(&body,box);}
    if(elapsed>=15360)return full_section_reference(0,15360,box);
    first=(phase+15360-elapsed)%15360;
    return first<=phase?full_section_reference(first,phase,box):
        full_section_reference(first,15360,box)||full_section_reference(0,phase,box);
}
static void test_sweep_section_scan_equivalence(void) {
    const UWORD intervals[]={0,1,59,60,61,239,240,241,719,720,721,959,960,961,15359,15360,65535};
    unsigned mismatches=0;
    for(UBYTE district=0;district<4;district++){
        td_streetcar_box_t boxes[6];traffic_probe_boxes(district,boxes);
        for(unsigned phase=0;phase<256*60;phase++)for(unsigned n=0;n<sizeof(intervals)/sizeof(intervals[0]);n++)
            for(unsigned i=0;i<6;i++){
                UWORD clock=(UWORD)(65280+phase/60);UBYTE sub=phase%60;
                if(td_streetcar_sweep(clock,sub,intervals[n],&boxes[i])!=full_sweep_reference(clock,sub,intervals[n],&boxes[i]))mismatches++;
            }
    }
    expect(!mismatches,"new timetable optimized section admission agrees with a complete16-section reference through rollover");
}

static void test_aircraft_world_freezing(void) {
    native_case();td_aircraft_reset(1);
    UWORD wait=td_aircraft.wait;unsigned writes=sram_writes;
    world_tick(0,wait);
    expect(td_aircraft.active&&td_aircraft.ticks==0,
           "real world elapsed VBlanks start a cosmetic flight without consuming its first tick");
    unsigned flight_writes=sram_writes-writes;
    td_state_t flight_world=td;td_aircraft_state_t flying=td_aircraft;
    native_case();td_aircraft_reset(1);td_aircraft.wait=1000;writes=sram_writes;
    world_tick(0,wait);
    expect(sram_writes-writes==flight_writes&&!memcmp(&td,&flight_world,58),
           "a due cosmetic flight adds no save writes or persistent gameplay changes beyond ordinary clock updates");
    td_aircraft=flying;world_tick(J_START,60);
    expect(td.mode==TD_PAUSE&&!memcmp(&td_aircraft,&flying,sizeof(flying)),
           "opening pause freezes aircraft on the same input update");
    td.menu=1;world_tick(J_A,600);world_tick(J_RIGHT|J_DOWN,600);world_tick(0,600);
    expect(td.mode==TD_MAP&&!memcmp(&td_aircraft,&flying,sizeof(flying)),
           "opening and panning the city atlas preserves flight position, rotor phase and cooldown");
    world_tick(J_B,600);world_tick(0,1);world_tick(J_B,600);
    expect(td.mode==TD_ROAM&&!memcmp(&td_aircraft,&flying,sizeof(flying)),
           "closing atlas and pause applies no aircraft catch-up");
    world_tick(0,8);
    expect(td_aircraft.ticks==8,"resumed flight advances only the new active-world VBlanks");
    flying=td_aircraft;td_transition_pending=2;test_queue_fail=1;world_tick(0,600);
    expect(!memcmp(&td_aircraft,&flying,sizeof(flying)),
           "pending district allocation freezes the cosmetic aircraft simulation");
    queen_runtime_case(0);td_aircraft_reset(2);
    /* A due flight during a paid ride exercises integration without crossing
       the scene seam during a single deliberately oversized fixture step. */
    td_aircraft.wait=1;world_tick(0,1);
    expect(td.mode==TD_RIDE&&td_aircraft.active,
           "paid transit permits cosmetic flybys using its derived camera view");
}


static void test_human_impacts_and_police(void){
    UBYTE found=0,route=0;UWORD u=0,v=0,clock=0;
    native_case();
    for(UBYTE r=0;r<td_route_counts[0]&&!found;r++)for(UWORD t=0;t<12&&!found;t++){
        UBYTE phase=(t*12+r*37)&127;
        UWORD x=td_district_routes[0][r][0]+(phase<64?phase:127-phase),y=td_district_routes[0][r][1];
        if(x>20&&td_drivable(x-11,y)&&td_drivable(x+11,y)){
            found=1;route=r;u=x;v=y;clock=t;
        }
    }
    expect(found,"impact fixture uses a real registered pedestrian route through a usable road footprint");
    if(!found)return;
    td.u=(u-11)*16;td.v=v*16;td.seconds=clock;td.speed=24;td_vx=384;
    td_people_reset();td_ped_refresh=16;td_ped_route[0]=route;
    td_nearby_routes[0][0]=td_district_routes[0][route][0];td_nearby_routes[0][1]=v;
    td_pedestrians();
    expect(!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun&&td.cash==30&&!td.wanted,
           "the real route is presented at the clear eleven-pixel edge before the courier crosses it");
    td.u=(u+11)*16;td_pedestrians();
    expect(td_people[0].stun==6&&actors[9].frame_start==36,
           "swept movement knocks down the actual human even when the final car endpoint clears the hit box");
    expect(td.cash==10&&td.wanted==1&&td.wanted_left==30&&td.speed==12&&td.msg==19,
           "one human impact slows momentum and applies one20-dollar fine plus persistent attention");
    td_state_t charged=td;td_pedestrians();
    expect(td.cash==charged.cash&&td.wanted==charged.wanted&&td_people[0].stun==6,
           "holding contact against a recovering person cannot repeat the penalty");
    UWORD impact_u=actors[9].pos.x,impact_v=actors[9].pos.y;
    td_tick+=24;td_pedestrians();
    UWORD human_u=actors[9].pos.x,human_v=actors[9].pos.y;
    expect(td_people[0].stun==6&&td_people[0].recover==24&&human_u>=impact_u&&
           human_u-impact_u<=12*32&&human_v==impact_v&&actors[9].frame_start==37,
           "a struck human makes a bounded twelve-pixel flight then becomes a prone non-graphic body");
    for(unsigned second=0;second<6;second++){td.seconds++;td_people_second();}
    td.speed=0;td_pedestrians();
    expect(td_people[0].stun==6&&actors[9].pos.x==human_u&&actors[9].pos.y==human_v,
           "six active-world seconds leave the struck person down rather than reviving them");
    td.seconds++;td_pedestrians();
    expect(actors[9].pos.x==human_u&&actors[9].pos.y==human_v&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&
           td.cash==charged.cash&&td.wanted==charged.wanted,
           "a visible prone body cannot repeat the collision fine or resume walking");
    td.u=(u-11)*16;td_pedestrians();td.seconds++;td_pedestrians();
    expect(actors[9].pos.x==human_u&&actors[9].pos.y==human_v&&td_people[0].stun==6,
           "clearing the courier leaves the same district-local corpse in place");
    td.wanted=3;td.wanted_left=30;td.cash=500;td_save();
    memset(&td,0,sizeof(td));expect(td_restore()&&td.wanted==3&&td.wanted_left==30&&td.cash==500,
           "version8 dual-slot restore retains actual fines and active police attention");
    td.mode=TD_ROAM;
    expect(!td_people_police(td.u+600,td.v),"a patrol outside its stop range cannot fine the player");
    expect(td_people_police(td.u+383,td.v)&&td.cash==275&&!td.wanted&&!td.wanted_left,
           "a nearby patrol charges the accepted quadratic25-dollar attention penalty then resolves the episode");
    expect(!td_people_police(td.u,td.v)&&td.cash==275,"resolved attention cannot generate repeated patrol fines");
    td.wanted=2;td.wanted_left=2;td_people_second();
    expect(td.wanted==2&&td.wanted_left==1,"attention counts active simulation seconds");
    td_people_second();expect(td.wanted==1&&td.wanted_left==30,"quiet time decays one level and starts the next interval");
    td.wanted_left=1;td_people_second();expect(!td.wanted&&!td.wanted_left,"the last attention level ends completely");
    /* Genuine older wire data contains arbitrary cursor words, not heat. */
    td.cash=123;td_save();volatile UBYTE *old=td_save_address(td_save_slot);
    old[2]=7;old[8+52]=0xff;old[8+53]=0xff;old[8+54]=0xff;old[8+55]=0xff;refresh_record_crc(old);
    memset(&td,0,sizeof(td));expect(td_restore()&&td.cash==123&&!td.wanted&&!td.wanted_left,
           "a real CRC-valid version7 record migrates obsolete cursors without inventing police attention");
}

static UBYTE route83_slot(void){
    for(UBYTE i=0;i<TD_PEOPLE_COUNT;i++)if(td_people[i].route==83)return i;
    return TD_NONE;
}
static void test_visible_human_reverse_start(void){
    static const UWORD cu[6]={80,200,320,440,824,216};
    static const UWORD cv[6]={280,392,168,632,240,72};
    native_case();td.u=td.safe_u=566*16;td.v=td.safe_v=695*16+3;
    td.heading=12;td.cash=138;td.done=1;td.complete[0]=1;td.stage=2;td.left=111;
    td.seconds=24;td.subsecond=22;
    /* Use the native endpoint's odd initial motion parity and actual input
     * updates, rather than assigning a reverse displacement. */
    td_tick=1;
    for(UBYTE i=0;i<6;i++){
        td_traffic_u[i]=cu[i]*16;td_traffic_v[i]=cv[i]*16;td_traffic_leg[i]=i==5?2:0;
    }
    td_people_reset();world_tick(0,0);
    /* Isolate the historical reverse-start safety regression from the new
       independently tested social detour: keep all eight visible identities. */
    for(unsigned i=0;i<TD_PEOPLE_COUNT;i++){td_people[i].state&=TD_PERSON_ADMITTED;td_people[i].timer=255;td_people[i].ox=td_people[i].oy=0;}
    UBYTE slot=route83_slot();
    expect(slot!=TD_NONE,"the actual loaded selector registers native Bay route83 near the courier");
    if(slot==TD_NONE)return;
    expect(td_people[slot].phase==35&&actors[9+slot].pos.x==555*32&&!(actors[9+slot].flags&ACTOR_FLAG_HIDDEN),
           "the native24:22 route83 approach is visible before any reverse-start input");
    world_tick(0,3);
    for(unsigned frame=0;frame<95;frame++)world_tick(0,1);
    slot=route83_slot();expect(slot!=TD_NONE,"ordinary refreshes retain the physically visible route83 identity");
    if(slot==TD_NONE)return;
    expect(td.seconds==26&&!td.subsecond&&td.u==566*16&&td.v==695*16+3&&!td.speed&&
           td_people[slot].phase==35&&td_people[slot].lag==20&&actors[9+slot].pos.x==555*32,
           "actual world updates reproduce new26:00 phase35/lag20 waiting beside the stationary car");
    for(unsigned update=0;update<4;update++)world_tick(J_B,4);
    slot=route83_slot();expect(slot!=TD_NONE,"ordinary16-VBlank reversing does not unload the existing human");
    if(slot==TD_NONE)return;
    if(td.cash!=138||td_people[slot].stun)fprintf(stderr,
      "engine reverse diagnostic: car=%u/%u speed=%d clock=%u:%u human=%u/%u phase=%u lag=%u stun=%u cash=%u H=%u\n",
      td.u,td.v,td.speed,td.seconds,td.subsecond,actors[9+slot].pos.x>>5,actors[9+slot].pos.y>>5,
      td_people[slot].phase,td_people[slot].lag,td_people[slot].stun,td.cash,td.wanted);
    expect(td.u==566*16&&td.v>695*16+3&&td.v<698*16&&td.speed==-4&&td.seconds==26&&td.subsecond==16,
           "four actual held-B updates produce the slower bounded reverse-start displacement and speed minus4");
    expect(td_people[slot].phase==35&&actors[9+slot].pos.x==555*32&&!td_people[slot].stun&&
           !(actors[9+slot].flags&ACTOR_FLAG_HIDDEN)&&td.cash==138&&!td.wanted&&!td.cooldown&&td.msg!=19,
           "a human stays visible at its safe old position instead of advancing into the reversing car and charging20dollars");
    for(unsigned update=0;update<11;update++)world_tick(J_B,4);
    slot=route83_slot();expect(slot!=TD_NONE,"continued ordinary reverse travel retains the same visible human route");
    if(slot==TD_NONE)return;
    expect(td.v>=702*16&&td_people[slot].phase>35&&actors[9+slot].pos.x>555*32&&
           !(actors[9+slot].flags&ACTOR_FLAG_HIDDEN)&&!td_people[slot].stun&&
           td.cash==138&&!td.wanted&&!td.cooldown,
           "the walker resumes its ordinary route after actual reverse motion clears the full occupied body");
}

static void test_road_police_pursuit(void){
    reset_case();geometry=NATIVE_GRID;toronto_init();td.mode=TD_ROAM;
    td.u=560*16;td.v=720*16;td.wanted=1;td.wanted_left=30;td.cash=100;
    td_traffic_u[2]=336*16;td_traffic_v[2]=280*16;
    UWORD start_u=td_traffic_u[2],start_v=td_traffic_v[2];unsigned turns=0;
    UBYTE heading=0;int safe=1;
    for(unsigned step=0;step<12000&&td.wanted;step++){
        td.seconds=step/60;td.subsecond=step%60;
        UWORD old_u=td_traffic_u[2],old_v=td_traffic_v[2];td_traffic_step();
        if(!td_road_sweep(old_u>>4,old_v>>4,td_traffic_u[2]>>4,td_traffic_v[2]>>4,5))safe=0;
        if(td_police_waypoint.valid&&td_police_waypoint.heading!=heading){turns++;heading=td_police_waypoint.heading;}
    }
    expect(safe&&turns>0&&(td_traffic_u[2]!=start_u||td_traffic_v[2]!=start_v),
           "wanted patrol drives and turns on actual connected roads without teleporting through terrain");
    if(td.wanted||td.cash!=75||td.msg!=20)fprintf(stderr,"pursuit diagnostic: police=%u,%u wanted=%u cash=%u target=%u,%u direction=%u stuck=%u\n",td_traffic_u[2],td_traffic_v[2],td.wanted,td.cash,td_police_waypoint.u,td_police_waypoint.v,td_police_waypoint.heading,td_police_stuck);
    expect(!td.wanted&&td.cash==75&&td.msg==20,"a road pursuit reaches the courier and charges the level-one capture fine once");
    td_state_t captured=td;for(unsigned step=0;step<120;step++)td_traffic_step();
    expect(td.cash==captured.cash&&!td.wanted,"capture clears attention without repeated fines while the police returns to patrol");

    reset_case();td.wanted=2;td.wanted_left=1;td.cash=500;td_traffic_u[2]=td.u+32*16;td_traffic_v[2]=td.v;
    expect(!td_people_police(td.u+32*16,td.v)&&td.wanted_left==30,
           "nearby pursuit refreshes attention without charging outside capture range");
    td_people_second();expect(td.wanted==2&&td.wanted_left==30,"an actually nearby observed road chase refreshes its escape clock instead of decaying a level");
    expect(td_people_police(td.u,td.v)&&td.cash==400&&!td.wanted,"level-two capture charges a tougher quadratic penalty");
    reset_case();td.mode=TD_PAUSE;td.wanted=3;td.wanted_left=1;sys_time=120;
    toronto_update();expect(td.wanted==3&&td.wanted_left==1,"pause freezes pursuit attention with the rest of the world");

    for(UBYTE heat=1;heat<=3;heat++){
        reset_case();td.wanted=heat;td.wanted_left=30;td.u=500*16;td.v=450*16;
        td_traffic_u[2]=400*16;td_traffic_v[2]=392*16;
        UWORD other_u=td_traffic_u[0];sys_time=4;toronto_update();
        expect(td_traffic_u[2]==400*16-(8+4*heat)*4&&td_traffic_v[2]==392*16,
               "four-VBlank pursuit motion escalates on the legal westbound Dundas lane across all three attention levels");
        expect(td_traffic_u[0]==other_u,"police cadence does not accelerate ordinary traffic between its sixteen-VBlank quanta");
    }
    reset_case();td.job=0;td.stage=1;td.health=100;td_traffic_u[0]=td.u+9*16;td_traffic_v[0]=td.v;
    sys_time=1;toronto_update();
    expect(td.health==88&&td.msg==5,"vehicle contact penalties are checked before the next autonomous motion quantum");
}

/* Real input edges without advancing physical scenery between fixture client
 * placements. Long frozen updates below separately verify clock behaviour. */
static void dispatch_edge(UBYTE button) {
    world_tick(0,0);world_tick(button,0);
}

static void dispatch_offer(UBYTE job) {
    native_case();td.mode=TD_BOARD;td.menu=job;td_get_job(job,&td_offer);
    /* A valid, fully unlocked campaign with precisely this job uncompleted. */
    memset(td.complete,255,TD_QUESTS/8);td.complete[job>>3]&=~(1<<(job&7));td.done=TD_QUESTS-1;
}

static void test_dispatch_itinerary_inputs(void) {
    unsigned ordered_stops=0,repeated_stops=0;
    for(UBYTE job=0;job<TD_QUESTS;job++) {
        dispatch_offer(job);td.seconds=65535;td.subsecond=59;td.left=217;
        td_set_target();td_state_t paused=td;td_stop_t objective=td_target;
        unsigned stores=sram_writes;const td_job_t *expected=&td_fixture_jobs[job];
        expect(expected->count&&td_offer.count==expected->count,"every itinerary fixture uses the campaign JSON's actual ordered stop count");
        for(UBYTE page=0;page<expected->count;page++) {
            expect(td_board_route==page&&td_offer.route[td_board_route]==expected->route[page],
                   "Down reaches each campaign stop in its original order, including repeated endpoints");
            ordered_stops++;
            for(UBYTE prior=0;prior<page;prior++)if(expected->route[prior]==expected->route[page]){repeated_stops++;break;}
            world_tick(0,255);
            expect(!memcmp(&td,&paused,58)&&!memcmp(&td_target,&objective,sizeof(objective))&&sram_writes==stores,
                   "itinerary browsing freezes all saved fields, the delivery objective and SRAM across long updates");
            dispatch_edge(J_DOWN);
        }
        expect(td_board_route==0,"Down wraps from the last ordered stop to pickup");
        for(UBYTE remaining=expected->count;remaining;remaining--) {
            dispatch_edge(J_UP);
            expect(td_board_route==remaining-1&&td_offer.route[td_board_route]==expected->route[remaining-1],
                   "Up visits every ordered stop backwards and wraps from pickup to the final stop");
        }
        expect(td_board_route==0,"a complete upward itinerary cycle returns to pickup");
        dispatch_edge(J_DOWN);UBYTE held_page=td_board_route;
        world_tick(J_DOWN,255);
        expect(td_board_route==(held_page+1)%expected->count&&!memcmp(&td,&paused,58),"holding Down repeats one itinerary step after the delay without consuming the paused deadline");
        UBYTE prior_page=td_board_route;
        dispatch_edge(J_UP|J_DOWN);
        expect(td_board_route==(prior_page?prior_page-1:expected->count-1),"fresh simultaneous Up and Down retains the existing Up-first menu policy");
        td_board_route=expected->count-1;dispatch_edge(J_RIGHT);
        UBYTE next=(job+1)%TD_QUESTS;
        expect(td.menu==next&&!td_board_route&&td_offer.reward==td_fixture_jobs[next].reward&&
               !memcmp(td_offer.route,td_fixture_jobs[next].route,12),
               "Right changes to the next actual offer, wraps at the campaign end and resets the preview to pickup");
        td_board_route=td_offer.count-1;dispatch_edge(J_LEFT);
        expect(td.menu==job&&!td_board_route&&td_offer.reward==expected->reward&&
               !memcmp(td_offer.route,expected->route,12),
               "Left returns to the previous actual offer, wraps at the campaign start and resets the preview to pickup");
        expect(td.stage==paused.stage&&td.job==paused.job&&td.seconds==paused.seconds&&td.left==paused.left&&sram_writes==stores,
               "contract selection and stop navigation cannot collect cargo, pay money or save progress");
    }
    expect(ordered_stops>TD_QUESTS*2&&repeated_stops>0,"navigation coverage includes longer itineraries and repeated return endpoints rather than only two-stop deliveries");

    /* Recovery from a stale transient index happens before processing input. */
    dispatch_offer(95);td_board_route=255;dispatch_edge(J_DOWN);
    expect(td_board_route==1,"an invalid transient page is normalized before Down selects the second ordered stop");
    dispatch_offer(95);td_board_route=255;dispatch_edge(J_UP);
    expect(td_board_route==td_fixture_jobs[95].count-1,"an invalid transient page is normalized before Up wraps to the actual return");
}

/* Explicit chapter-start oracles, independent of the production bitmask. */
static const UBYTE dispatch_next_chapters[13]={8,16,24,32,40,48,56,64,72,80,88,96,0};
static const UBYTE dispatch_previous_chapters[13]={96,0,8,16,24,32,40,48,56,64,72,80,88};
_Static_assert(sizeof(dispatch_next_chapters)==TD_QUESTS/8&&sizeof(dispatch_previous_chapters)==TD_QUESTS/8,
               "every actual eight-offer chapter needs an explicit independent forward/backward oracle");
static int dispatch_offer_matches(UBYTE job) {
    const td_job_t *expected=&td_fixture_jobs[job];
    return !strcmp(td_offer.title,expected->title)&&td_offer.kind==expected->kind&&
        td_offer.count==expected->count&&td_offer.vehicle==expected->vehicle&&
        td_offer.min_done==expected->min_done&&td_offer.seconds==expected->seconds&&
        td_offer.reward==expected->reward&&!memcmp(td_offer.route,expected->route,12);
}
static void dispatch_chapter_offer(UBYTE job) {
    dispatch_offer(dispatch_previous_chapters[job/8]);
    dispatch_edge(J_SELECT);
    for(UBYTE i=0;i<job%8;i++)dispatch_edge(J_RIGHT);
    expect(td.mode==TD_BOARD&&td.menu==job&&!td_board_route&&dispatch_offer_matches(job),
           "a chapter jump followed by ordinary Right edges reaches the actual requested offer");
}
static void test_dispatch_chapter_inputs(void) {
    static const UBYTE chords[]={0,J_A,J_LEFT,J_RIGHT,J_UP,J_DOWN,J_A|J_LEFT|J_RIGHT|J_UP|J_DOWN};
    static const UBYTE exits[]={J_B,J_START,J_B|J_START};
    expect(TD_QUESTS==104,"the chapter shortcut fixture covers all104 offers including the appended North chapter");
    for(UBYTE job=0;job<TD_QUESTS;job++) {
        UBYTE next=dispatch_next_chapters[job/8];
        const td_job_t *next_offer=&td_fixture_jobs[next];
        for(unsigned chord=0;chord<sizeof(chords)/sizeof(chords[0]);chord++) {
            dispatch_offer(job);td.vehicle=next_offer->vehicle==TD_NONE?0:next_offer->vehicle;
            td_board_route=td_offer.count-1;td.seconds=65535;td.subsecond=59;
            td.left=217;td.health=37;td.cash=913;td_set_target();
            td_state_t expected=td;expected.menu=next;
            td_stop_t objective=td_target;td_job_t carried=td_job;
            unsigned stores=sram_writes,draws=ui_draws;
            dispatch_edge(J_SELECT|chords[chord]);
            expect(td.menu==next&&!td_board_route&&dispatch_offer_matches(next),
                   "Select from every actual offer reaches the explicit next chapter start and resets itinerary");
            expect(!memcmp(&td,&expected,58)&&!memcmp(&td_job,&carried,sizeof(carried))&&
                   !memcmp(&td_target,&objective,sizeof(objective))&&sram_writes==stores,
                   "Select plus accept/navigation consumes only the chapter change without altering cash, time, completion or job");
            expect(ui_draws==draws+1,"the chapter change requests its own immediate board redraw");
            world_tick(J_SELECT|chords[chord],255);world_tick(J_SELECT|chords[chord],1025);
            expect(!memcmp(&td,&expected,58)&&!td_board_route&&dispatch_offer_matches(next)&&
                   !memcmp(&td_target,&objective,sizeof(objective))&&sram_writes==stores&&ui_draws==draws+1,
                   "held Select/chords do not repeat navigation, accept work, redraw or advance the paused world");
            dispatch_edge(J_SELECT);expected.menu=dispatch_next_chapters[next/8];
            expect(!memcmp(&td,&expected,58)&&!td_board_route&&dispatch_offer_matches(expected.menu)&&sram_writes==stores,
                   "a released then fresh Select edge advances exactly one more chapter");
        }
        for(unsigned exit=0;exit<sizeof(exits)/sizeof(exits[0]);exit++) {
            dispatch_offer(job);td_board_route=td_offer.count-1;
            td_state_t expected=td;expected.mode=TD_ROAM;
            unsigned stores=sram_writes;UBYTE page=td_board_route;
            dispatch_edge(J_SELECT|J_A|J_LEFT|J_RIGHT|J_UP|J_DOWN|exits[exit]);
            expect(!memcmp(&td,&expected,58)&&td_board_route==page&&dispatch_offer_matches(job)&&sram_writes==stores,
                   "B and Start retain board-exit priority over chapter jumps and all simultaneous accept/navigation edges");
        }
    }
    dispatch_offer(61);td_board_route=255;dispatch_edge(J_SELECT);
    expect(td.menu==64&&!td_board_route&&dispatch_offer_matches(64),
           "a chapter jump discards a stale itinerary page before previewing the next actual pickup");
}
static void test_dispatch_chapter_eligibility(void) {
    for(UBYTE job=0;job<TD_QUESTS;job++) {
        const td_job_t *expected=&td_fixture_jobs[job];
        dispatch_chapter_offer(job);td.vehicle=expected->vehicle==TD_NONE?0:expected->vehicle;
        td.health=37;td.cash=913;td_board_route=expected->count-1;
        dispatch_edge(J_A);
        expect(td.mode==TD_ROAM&&td.job==job&&!td.stage&&td.health==100&&td.left==expected->seconds&&td.cash==913&&
               td_job.reward==expected->reward&&!memcmp(td_job.route,expected->route,12),
               "a fresh A after chapter browsing accepts the compatible actual offer at pickup under its original rules");
        if(expected->min_done) {
            dispatch_chapter_offer(job);memset(td.complete,0,sizeof(td.complete));td.done=0;
            td.vehicle=expected->vehicle==TD_NONE?0:expected->vehicle;td_board_route=expected->count-1;
            td_state_t before=td;before.msg=3;unsigned stores=sram_writes;
            dispatch_edge(J_A);
            expect(!memcmp(&td,&before,58)&&sram_writes==stores,
                   "chapter browsing cannot bypass any actual offer's individual completion lock");
        }
        if(expected->vehicle!=TD_NONE)for(UBYTE foot=0;foot<2;foot++) {
            dispatch_chapter_offer(job);td.vehicle=foot?expected->vehicle:(expected->vehicle+1)&3;td.onfoot=foot;
            td_board_route=expected->count-1;td_state_t before=td;before.msg=2;
            unsigned stores=sram_writes;dispatch_edge(J_A);
            expect(!memcmp(&td,&before,58)&&sram_writes==stores,
                   "chapter browsing retains wrong-vehicle and on-foot denial for every vehicle-required offer");
        }
        if(expected->vehicle==TD_NONE) {
            dispatch_chapter_offer(job);td.onfoot=1;dispatch_edge(J_A);
            expect(td.mode==TD_ROAM&&td.job==job&&!td.stage&&td.onfoot&&td.left==expected->seconds,
                   "vehicle-independent offers remain available to walkers after chapter browsing");
        }
    }
    /* Region groups have individual unlocks, not one shared chapter tier. */
    dispatch_offer(80);memset(td.complete,0,sizeof(td.complete));td.complete[0]=7;td.done=3;
    td.vehicle=1;dispatch_edge(J_SELECT);
    expect(td.menu==88&&td_offer.min_done==8,"Select previews the actual locked first Port contract without skipping it");
    unsigned stores=sram_writes;dispatch_edge(J_A);
    expect(td.mode==TD_BOARD&&td.job==TD_NONE&&td.msg==3&&td.done==3&&td.cash==30&&sram_writes==stores,
           "the first Port chapter offer keeps its own eight-completion lock");
    dispatch_edge(J_RIGHT);
    expect(td.menu==89&&td_offer.min_done==3,"Right still reaches the neighboring three-completion Port offer");
    dispatch_edge(J_A);
    expect(td.mode==TD_ROAM&&td.job==89&&!td.stage&&td.done==3&&td.complete[0]==7&&td.cash==30,
           "an earlier-unlocked neighbor can be accepted without inventing chapter-wide progression");

    dispatch_offer(95);td.vehicle=0;dispatch_edge(J_A);td.stage=2;td.left=73;td.health=67;td_set_target();
    dispatch_edge(J_START);dispatch_edge(J_DOWN);dispatch_edge(J_DOWN);dispatch_edge(J_A);
    td_state_t before=td;td_job_t carried=td_job;td_stop_t target=td_target;
    stores=sram_writes;td_board_route=3;dispatch_edge(J_SELECT);before.menu=96;
    expect(!memcmp(&td,&before,58)&&!td_board_route&&!memcmp(&td_job,&carried,sizeof(carried))&&
           !memcmp(&td_target,&target,sizeof(target))&&sram_writes==stores,
           "chapter browsing while carrying work preserves the active ordered job and its current handoff");
    dispatch_edge(J_A);before.mode=TD_ROAM;
    expect(!memcmp(&td,&before,58)&&!memcmp(&td_job,&carried,sizeof(carried))&&
           !memcmp(&td_target,&target,sizeof(target))&&sram_writes==stores,
           "A after an active-job chapter jump resumes without replacing the carried contract or adding an error");
}

static void test_dispatch_acceptance_and_reentry(void) {
    for(UBYTE job=0;job<TD_QUESTS;job++) {
        const td_job_t *expected=&td_fixture_jobs[job];
        dispatch_offer(job);td.vehicle=expected->vehicle==TD_NONE?0:expected->vehicle;
        td_board_route=expected->count-1;td.health=37;td.cash=913;
        dispatch_edge(J_A);
        expect(td.mode==TD_ROAM&&td.job==job&&!td.stage&&td.health==100&&td.left==expected->seconds&&td.cash==913&&
               td_job.reward==expected->reward&&!memcmp(td_job.route,expected->route,12),
               "A accepts the actual compatible offer from any preview page but begins the job at pickup without charging or paying");
        td_state_t accepted=td;unsigned stores=sram_writes;
        world_tick(J_A,0);
        expect(!memcmp(&td,&accepted,58)&&sram_writes==stores,"a held acceptance edge cannot accept or save the contract twice");

        dispatch_offer(job);td_board_route=expected->count-1;td_state_t before=td;stores=sram_writes;
        dispatch_edge(J_B);before.mode=TD_ROAM;
        expect(!memcmp(&td,&before,58)&&sram_writes==stores,"B leaves an unaccepted preview for roaming without committing, paying or changing its itinerary");
        dispatch_edge(J_SELECT);
        expect(td.mode==TD_BOARD&&!td_board_route&&td_offer.reward==td_fixture_jobs[td.menu].reward,
               "Select reopens dispatch after B with a rebuilt offer and pickup preview");

        if(expected->min_done) {
            dispatch_offer(job);memset(td.complete,0,sizeof(td.complete));td.done=0;
            td.vehicle=expected->vehicle==TD_NONE?0:expected->vehicle;td_board_route=expected->count-1;
            stores=sram_writes;dispatch_edge(J_A);
            expect(td.mode==TD_BOARD&&td.job==TD_NONE&&td.msg==3&&td_board_route==expected->count-1&&
                   td.cash==30&&!td.stage&&sram_writes==stores,
                   "previewing the destination cannot bypass the actual unique-completion lock");
        }
        if(expected->vehicle!=TD_NONE)for(UBYTE foot=0;foot<2;foot++) {
            dispatch_offer(job);td.vehicle=foot?expected->vehicle:(expected->vehicle+1)&3;td.onfoot=foot;
            td_board_route=expected->count-1;stores=sram_writes;dispatch_edge(J_A);
            expect(td.mode==TD_BOARD&&td.job==TD_NONE&&td.msg==2&&td_board_route==expected->count-1&&
                   td.cash==30&&!td.stage&&sram_writes==stores,
                   "a wrong vehicle or walking courier cannot accept a vehicle-required job from its final preview page");
        }
        if(expected->vehicle==TD_NONE) {
            dispatch_offer(job);td.onfoot=1;td_board_route=expected->count-1;dispatch_edge(J_A);
            expect(td.mode==TD_ROAM&&td.job==job&&!td.stage&&td.onfoot&&td.left==expected->seconds,
                   "a walking courier can still accept every vehicle-independent package offer from any itinerary page");
        }
        dispatch_offer(job);td.vehicle=expected->vehicle==TD_NONE?0:expected->vehicle;
        td.complete[job>>3]|=1<<(job&7);td.done=TD_QUESTS;dispatch_edge(J_A);
        expect(td.job==job&&!td.stage&&td.done==TD_QUESTS&&td.complete[job>>3]&(1<<(job&7)),
               "completed offers remain replayable through A without removing or awarding unique completion credit");
    }

    dispatch_offer(95);td.vehicle=0;dispatch_edge(J_A);td.stage=2;td.left=73;td.health=67;td_set_target();
    td_state_t active=td;td_stop_t target=td_target;unsigned stores=sram_writes;
    td_board_route=3;dispatch_edge(J_START);dispatch_edge(J_DOWN);dispatch_edge(J_DOWN);dispatch_edge(J_A);
    expect(td.mode==TD_BOARD&&td.menu==95&&!td_board_route&&td_offer.reward==td_fixture_jobs[95].reward&&
           !memcmp(td_offer.route,td_job.route,12),"opening dispatch from pause during work rebuilds the active job's complete itinerary at pickup");
    dispatch_edge(J_UP);dispatch_edge(J_RIGHT);dispatch_edge(J_DOWN);dispatch_edge(J_A);
    expect(td.mode==TD_ROAM&&td.msg==active.msg&&td.job==95&&td.stage==active.stage&&td.left==active.left&&
           td.health==active.health&&td.cash==active.cash&&sram_writes==stores&&
           !memcmp(&td_target,&target,sizeof(target)),
           "A on another contract's preview resumes the carried job without replacing its current handoff or adding an error");
    dispatch_edge(J_START);dispatch_edge(J_DOWN);dispatch_edge(J_DOWN);dispatch_edge(J_A);
    dispatch_edge(J_B);
    expect(td.mode==TD_ROAM&&td.job==95&&td.stage==2&&td.left==73,"B returns from an active-job preview to the unchanged carried job");
    td_board_route=3;dispatch_edge(J_START);dispatch_edge(J_DOWN);dispatch_edge(J_DOWN);dispatch_edge(J_A);
    expect(td.mode==TD_BOARD&&td.menu==95&&!td_board_route,"re-entering the active-job board always returns its preview to pickup");
    dispatch_edge(J_START);
    expect(td.mode==TD_ROAM&&td.job==95&&td.stage==2,"Start retains its board-back behaviour without cancelling the carried job");
}

static void test_dispatch_active_preview_resume(void) {
    const UBYTE jobs[]={0,6,95};const UBYTE stages[]={1,2,2};
    for(UBYTE i=0;i<sizeof(jobs);i++){
        dispatch_offer(jobs[i]);td.vehicle=td_offer.vehicle==TD_NONE?0:td_offer.vehicle;dispatch_edge(J_A);
        td.stage=stages[i];td.left=73;td.health=67;td.msg=6;td.onfoot=i==0;td_set_target();
        dispatch_edge(J_START);dispatch_edge(J_DOWN);dispatch_edge(J_DOWN);dispatch_edge(J_A);
        td_board_route=td_job.count-1;
        td_state_t before=td;td_job_t carried=td_job;td_stop_t target=td_target;unsigned stores=sram_writes;
        world_tick(0,360);
        expect(!memcmp(&td,&before,58)&&!memcmp(&td_job,&carried,sizeof(carried))&&
               !memcmp(&td_target,&target,sizeof(target))&&sram_writes==stores,
               "active parcel, return and fixed-car previews freeze the carried deadline and all saved fields across six seconds");
        world_tick(J_A,0);before.mode=TD_ROAM;
        expect(!memcmp(&td,&before,58)&&!memcmp(&td_job,&carried,sizeof(carried))&&
               !memcmp(&td_target,&target,sizeof(target))&&sram_writes==stores&&td_board_route==td_job.count-1,
               "the active-board A edge only resumes and preserves the walking/vehicle state, preview, message, target and ordered job");
    }
}

static void test_dispatch_credit_cache_and_order(void) {
    for(UBYTE job=0;job<TD_QUESTS;job++) {
        const td_job_t *expected=&td_fixture_jobs[job];
        dispatch_offer(job);td.vehicle=expected->vehicle==TD_NONE?0:expected->vehicle;
        td_board_route=expected->count-1;dispatch_edge(J_A);
        UWORD cash_before=td.cash;unsigned gross=(unsigned)expected->reward*67/100+99/5;
        for(UBYTE page=0;page<expected->count;page++) {
            td_stop_t client;td_get_stop(expected->route[page],&client);
            td.district=test_current_district=client.district;td.onfoot=expected->vehicle==TD_NONE;
            td.u=client.u*16;td.v=client.v*16;td.speed=0;td.left=99;td.health=67;td_set_target();
            if(page+1<expected->count) {
                /* The preview index deliberately remains on the last stop. */
                UWORD cached=td_offer.reward;dispatch_edge(J_SELECT);
                expect(td.job==job&&td.stage==page+1&&td.mode==TD_ROAM&&td.cash==cash_before&&td_offer.reward==cached,
                       "Select at each actual ordered nonfinal stop advances one delivery stage without replacing the payout cache or paying early");
            }else {
                dispatch_edge(J_SELECT);
                expect(td.mode==TD_RESULT&&td.job==TD_NONE&&td.stage==expected->count&&td.cash==cash_before+gross&&
                       td_offer.reward==cash_before&&td.done==TD_QUESTS&&td.complete[job>>3]&(1<<(job&7)),
                       "only the actual final ordered handoff completes once and caches the true pre-payment cash independently of the preview page");
            }
        }
        unsigned stores=sram_writes;td_state_t result=td;world_tick(J_SELECT,240);
        expect(!memcmp(&td,&result,58)&&td_offer.reward==cash_before&&sram_writes==stores,
               "holding Select on a delivered result neither repeats payment nor overwrites its balance cache");
        td_board_route=255;dispatch_edge(J_A);
        expect(td.mode==TD_BOARD&&!td_board_route&&td_offer.reward==td_fixture_jobs[td.menu].reward&&
               td.cash==cash_before+gross&&td.done==TD_QUESTS,
               "A leaves the result with a fresh real offer instead of accepting the cached pre-payment balance as a reward");
    }

    const UWORD balances[]={30,59900,59999,60000,65535};
    const UBYTE conditions[]={1,67,100};
    for(unsigned b=0;b<sizeof(balances)/sizeof(balances[0]);b++)for(unsigned h=0;h<sizeof(conditions);h++) {
        dispatch_offer(0);dispatch_edge(J_A);td.stage=1;td.left=99;td.health=conditions[h];td.cash=balances[b];
        td_set_target();td.u=td_target.u*16;td.v=td_target.v*16;td.speed=0;
        unsigned gross=(unsigned)td_fixture_jobs[0].reward*conditions[h]/100+99/5;
        unsigned wide=balances[b]+gross,credited=wide>60000?60000:wide;
        dispatch_edge(J_SELECT);
        expect(td.mode==TD_RESULT&&td.cash==credited&&td_offer.reward==balances[b],
               "actual final interaction retains the full pre-payment balance for normal, partial, zero-cap and above-cap legacy credits");
        td_state_t result=td;result.mode=TD_ROAM;td_board_route=11;td_offer.reward=balances[b];
        memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&result,58),"capped payment and its unique completion restore through the unchanged real58-byte save record");
        unsigned inits=ui_draws;td_session_live=0;load_authored_scene_fixture();toronto_init();
        expect(td.mode==TD_HELP&&td_resume_mode==TD_ROAM&&!td_board_route&&ui_draws>inits&&td.cash==credited&&td.job==TD_NONE,
               "a genuine cold initialization invokes UI reset and restores a delivered result as roaming behind help without another credit");
        dispatch_edge(J_A);dispatch_edge(J_SELECT);
        expect(td.mode==TD_BOARD&&!td_board_route&&td_offer.reward==td_fixture_jobs[td.menu].reward&&td.cash==credited,
               "dispatch after result reset rebuilds its reward and pickup page rather than showing the discarded balance cache");
    }

    /* A closing return shares pickup coordinates, but is still a later stage. */
    dispatch_offer(95);td.vehicle=0;dispatch_edge(J_A);td.left=99;td.health=67;
    td_stop_t yard;td_get_stop(54,&yard);td.district=test_current_district=yard.district;td.u=yard.u*16;td.v=yard.v*16;td_set_target();
    dispatch_edge(J_SELECT);
    expect(td.job==95&&td.stage==1&&td.cash==30&&td_offer.reward==td_fixture_jobs[95].reward,
           "Select at the return yard initially performs pickup rather than prematurely awarding the displayed closing return");
    dispatch_edge(J_SELECT);
    expect(td.job==95&&td.stage==1&&td.cash==30&&td.msg==6,"return-yard repeated interaction cannot skip the intermediate studio and works handoffs");
    td.mode=TD_PAUSE;td_resume_mode=TD_ROAM;td.menu=7;td_board_route=3;dispatch_edge(J_A);
    expect(td.mode==TD_ROAM&&td.job==TD_NONE&&td.cash==30&&td.done==TD_QUESTS-1,
           "cancelling a partially collected ordered return gives no payout or unique completion");

    for(UBYTE stage=0;stage<2;stage++) {
        dispatch_offer(0);dispatch_edge(J_A);td.stage=stage;td.left=1;td.health=67;td.cash=59999;
        td.subsecond=59;td_set_target();td_offer.reward=43210;world_tick(0,1);
        expect(td.mode==TD_RESULT&&td.job==TD_NONE&&!td.health&&!td.left&&td.cash==59999&&td_offer.reward==59999&&td.done==TD_QUESTS-1,
               "the real deadline failure before or after pickup caches current cash but pays nothing and leaves unique completion unset");
        td_board_route=11;dispatch_edge(J_A);
        expect(td.mode==TD_BOARD&&!td_board_route&&td_offer.reward==td_fixture_jobs[td.menu].reward&&td.cash==59999,
               "retry dispatch after failure clears a stale balance cache and itinerary page without paying the failed job");
    }

    /* Paid expiry retires its parcel before arrival and intentionally bypasses
       td_finish. Its stale board buffer must stay harmless and be rebuilt. */
    dispatch_offer(0);dispatch_edge(J_A);td.onfoot=1;td.mode=TD_TRANSIT;
    td.transit_origin=0;td.transit_target=17;td_get_stop(17,&td_cursor);td.seconds=18;
    td.left=1;td_offer.reward=43210;dispatch_edge(J_A);
    expect(td.mode==TD_RIDE&&td.cash==27&&td.job==0,"result-cache fixture pays the actual authored subway fare before its parcel expires");
    world_tick(0,60);
    expect(td.mode==TD_RIDE&&td.job==TD_NONE&&!td.health&&td.cash==27,
           "expiry on a paid trip retires the parcel without cancelling or recharging the accepted ride");
    for(unsigned retry=0;retry<16&&td.mode==TD_RIDE;retry++)world_tick(0,60);
    expect(td.mode==TD_RESULT&&td.job==TD_NONE&&!td.health&&td.cash==27&&td.done==TD_QUESTS-1&&td_offer.reward==43210,
           "actual paid failed arrival pays nothing even though the retired parcel never overwrote the stale offer buffer");
    td_board_route=11;dispatch_edge(J_A);
    expect(td.mode==TD_BOARD&&!td_board_route&&td_offer.reward==td_fixture_jobs[td.menu].reward&&td.cash==27,
           "A after paid failure replaces the stale offer buffer and preview page without charging another fare");

    dispatch_offer(0);dispatch_edge(J_A);td.stage=1;td.left=99;td.health=67;td_set_target();
    td.u=td_target.u*16;td.v=td_target.v*16;dispatch_edge(J_SELECT);UWORD once=td.cash;
    dispatch_edge(J_A);td.menu=0;td_get_job(0,&td_offer);dispatch_edge(J_A);
    for(UBYTE stage=0;stage<2;stage++) {
        td_stop_t stop;td_get_stop(td_fixture_jobs[0].route[stage],&stop);td.u=stop.u*16;td.v=stop.v*16;
        td.left=99;td.health=67;td_set_target();dispatch_edge(J_SELECT);
    }
    expect(td.mode==TD_RESULT&&td.cash==once+(unsigned)td_fixture_jobs[0].reward*67/100+99/5&&td_offer.reward==once&&td.done==TD_QUESTS,
           "a full actual replay earns again and caches its new previous balance while preserving the first unique completion");
}

static void test_dispatch_transient_save_contract(void) {
    dispatch_offer(95);td.vehicle=0;dispatch_edge(J_A);td.stage=2;td.left=73;td.health=67;td_set_target();
    td.mode=TD_BOARD;td.menu=95;td_board_route=3;td_offer.reward=65535;td_save();
    UBYTE payload[58];memcpy(payload,(const void *)(td_save_address(td_save_slot)+8),sizeof(payload));
    td_board_route=1;td_offer.reward=1;td_save();
    expect(td_save_address(td_save_slot)[2]==TD_SAVE_VERSION&&td_save_address(td_save_slot)[3]==58&&
           !memcmp(payload,(const void *)(td_save_address(td_save_slot)+8),sizeof(payload)),
           "preview page and offer balance cache never enter or grow the current58-byte SRAM payload");
    td_state_t saved=td;saved.mode=TD_ROAM;td_board_route=11;td_offer.reward=43210;memset(&td,0,sizeof(td));
    expect(td_restore()&&!memcmp(&td,&saved,58)&&td_board_route==11&&td_offer.reward==43210,
           "restoring the real payload retains all carried-job fields and does not pretend that transient UI values were serialized");
    td_session_live=0;load_authored_scene_fixture();toronto_init();
    expect(td.mode==TD_HELP&&td_resume_mode==TD_ROAM&&!td_board_route&&td.job==95&&td.stage==2&&td.left==73&&td.health==67,
           "cold carried-job recovery calls the UI reset hook while preserving its actual ordered stage, remaining deadline and condition");
    dispatch_edge(J_A);dispatch_edge(J_START);dispatch_edge(J_DOWN);dispatch_edge(J_DOWN);dispatch_edge(J_A);
    expect(td.mode==TD_BOARD&&td.menu==95&&!td_board_route&&td_offer.reward==td_fixture_jobs[95].reward,
           "recovered active-job dispatch starts at pickup with the authored reward rather than stale transient cache data");
}

static void test_reserved_islands_traffic_gates(void) {
    /* Deliberately stale mainland caches in the loaded Island view cannot
       create road traffic; actual scene initialization is tested separately. */
    expect(TD_DISTRICT_ISLANDS==5,
           "the Islands traffic capability retains its explicit district identity");
    for(UBYTE mode=TD_ROAM;mode<=TD_RIDE;mode++){
        if(mode!=TD_ROAM&&mode!=TD_WAIT&&mode!=TD_RIDE)continue;
        for(UBYTE foot=0;foot<2;foot++){
            reset_case();td.mode=mode;td.onfoot=foot;td.job=0;td.stage=1;
            td.wanted=3;td.wanted_left=1;td.cash=913;td.health=73;
            td.speed=28;td_vx=448;td_vy=-64;
            for(unsigned i=0;i<8;i++){
                td_traffic_u[i]=td.u;td_traffic_v[i]=td.v;td_traffic_leg[i]=255;
                memset(&td_traffic_samples[i],255,sizeof(td_traffic_samples[i]));
                actors[i<6?i+2:i+11].pos.x=12000+i;actors[i<6?i+2:i+11].pos.y=13000+i;
                actors[i<6?i+2:i+11].frame_start=231+i;actors[i<6?i+2:i+11].frame_end=232+i;
                actors[i<6?i+2:i+11].flags=ACTOR_FLAG_ACTIVE|ACTOR_FLAG_PERSISTENT;
            }
            td_traffic_retreat_mask=255;td_vehicle_contact_mask=255;td_police_waypoint.valid=1;
            td_police_waypoint.u=td.u;td_police_waypoint.v=td.v;td_police_stuck=61;
            actors[8].flags=ACTOR_FLAG_ACTIVE;
            for(unsigned i=9;i<17;i++)actors[i].flags=ACTOR_FLAG_ACTIVE|((i&1)?ACTOR_FLAG_HIDDEN:0);
            actor_t before_actors[TD_ACTORS];memcpy(before_actors,actors,sizeof(before_actors));
            td_state_t before=td;WORD vx=td_vx,vy=td_vy;unsigned stores=sram_writes,draws=ui_draws,impacts=audio_impacts;
            UWORD before_u[8],before_v[8];UBYTE before_leg[8];td_traffic_sample_t before_samples[8];
            memcpy(before_u,td_traffic_u,sizeof(before_u));memcpy(before_v,td_traffic_v,sizeof(before_v));
            memcpy(before_leg,td_traffic_leg,sizeof(before_leg));memcpy(before_samples,td_traffic_samples,sizeof(before_samples));
            td_streetcar_view_district=TD_DISTRICT_ISLANDS;
            expect(td_traffic_free(td.u,td.v)&&td_traffic_free(td.u+167,td.v+167),
                   "stale fleet coordinates cannot occupy the reserved foot-only view");
            td_traffic_contacts();td_traffic_step();td_traffic_motion(255);
            expect(!memcmp(&td,&before,58)&&td_vx==vx&&td_vy==vy&&sram_writes==stores&&
                   ui_draws==draws&&audio_impacts==impacts,
                   "disabled fleet cannot damage cargo, capture, refresh attention, charge or write SRAM");
            expect(!memcmp(before_u,td_traffic_u,sizeof(before_u))&&!memcmp(before_v,td_traffic_v,sizeof(before_v))&&
                   !memcmp(before_leg,td_traffic_leg,sizeof(before_leg))&&!memcmp(before_samples,td_traffic_samples,sizeof(before_samples))&&
                   td_traffic_retreat_mask==255&&td_vehicle_contact_mask==255&&td_police_waypoint.valid&&td_police_stuck==61,
                   "disabled motion rejects even malformed stale routes without advancing caches or pursuit");
            td_traffic_present();
            for(unsigned i=2;i<9;i++){
                actor_t hidden=before_actors[i];hidden.flags|=ACTOR_FLAG_HIDDEN;
                expect(!memcmp(&actors[i],&hidden,sizeof(hidden)),
                       "reserved view hides all six fleet actors and parked car without positioning stale data");
            }
            for(unsigned i=17;i<19;i++){actor_t hidden=before_actors[i];hidden.flags|=ACTOR_FLAG_HIDDEN;expect(!memcmp(&actors[i],&hidden,sizeof(hidden)),"Islands suppress both additional road cars without affecting other actors");}
            for(unsigned i=0;i<TD_ACTORS;i++)if((i<2||i>=9)&&i!=17&&i!=18)
                expect(!memcmp(&actors[i],&before_actors[i],sizeof(actors[i])),
                       "fleet suppression preserves player, beacon, civilian and aircraft actor state");
            expect(!memcmp(&td,&before,58)&&sram_writes==stores,"fleet presentation changes no saved fields or booked state");
        }
    }
    /* Restore the actual registered mainland view and demonstrate that cached
       road users regain both occupancy and visible presentation. */
    reset_case();td.onfoot=1;
    td_streetcar_view_district=TD_DISTRICT_ISLANDS;td_traffic_present();
    td_streetcar_view_district=TD_DISTRICT_CITY;
    expect(!td_traffic_free(td_traffic_u[0],td_traffic_v[0]),"mainland cached traffic occupancy resumes after an Island view");
    td_state_t before=td;td_traffic_present();
    for(unsigned i=0;i<8;i++)expect(!(actors[i<6?i+2:i+11].flags&ACTOR_FLAG_HIDDEN)&&
        actors[i<6?i+2:i+11].pos.x==td_traffic_u[i]*2&&actors[i<6?i+2:i+11].pos.y==td_traffic_v[i]*2,
        "ordinary mainland presentation restores each fleet actor at its unchanged route coordinate");
    expect(!(actors[8].flags&ACTOR_FLAG_HIDDEN)&&!memcmp(&td,&before,58),
           "the visible mainland parked car and saved state retain their original presentation rules");

    reset_case();td.u=td_traffic_u[2];td.v=td_traffic_v[2];td.onfoot=1;td.wanted=2;td.wanted_left=1;td.cash=913;
    td_traffic_contacts();expect(td.wanted==0&&td.cash==813&&td.msg==20,
                               "mainland police capture and its escalating fine remain active");
}

static void test_current_ferry_fare_boundary(void) {
    for(UBYTE dock=20;dock<=22;dock++)for(UBYTE cash=0;cash<=4;cash++){
        reset_case();authored_content=1;td.onfoot=1;td.mode=TD_WAIT;td.cash=cash;
        td.transit_origin=dock;td.transit_target=10;td_get_stop(dock,&td_cursor);
        td.u=td_cursor.u*16;td.v=td_cursor.v*16;td.district=td_cursor.district;td.seconds=(dock-19)*7;
        UWORD park_u=td.park_u,park_v=td.park_v;
        expect(td_board_current_window(),"the existing ferry evaluates its actual open boarding window");
        expect(TD_DISTRICT_COUNT>5?
               (td.mode==TD_RIDE&&td.cash==cash-(cash<4?0:4)&&td.ride_left==8):
               (cash<4?(td.mode==TD_ROAM&&td.cash==cash&&td.msg==4):(td.mode==TD_RIDE&&td.cash==0&&td.ride_left==8)),
               "registered Island returns use assistance only below four dollars; ordinary fare and duration remain intact");
        expect(td.park_u==park_u&&td.park_v==park_v&&td.park_district==0,
               "ordinary ferry fare checks never relocate the mainland parked car");
    }
}

#include "island_save_harness.h"
#include "north_save_harness.h"
#include "island_objective_harness.h"
static void test_hardware_motion_feedback(void){
    static const UBYTE limits[4]={14,12,16,10};
    for(UBYTE vehicle=0;vehicle<4;vehicle++){
        reset_case();td.vehicle=vehicle;UWORD origin=td.u;
        for(unsigned tick=0;tick<180;tick++)driving_tick(J_A);
        expect(td.speed==limits[vehicle]&&td.u>origin,"new vehicle pacing keeps four distinct slower reachable cruise limits");
        td_vy=71;td.heading=0;
        for(unsigned tick=0;tick<30;tick++)driving_tick(J_A);
        UWORD northing=td.v;driving_tick(J_A);
        expect(td_vy==0&&td.v==northing&&td.u>origin,"straight east heading settles lateral momentum exactly without vertical drift");
        td.heading=4;driving_tick(J_A);
        expect(td_vx>0&&td_vy>0&&td.speed==limits[vehicle],"a turn retains momentum and acceleration rather than abruptly stopping");
        for(unsigned tick=0;tick<30;tick++)driving_tick(J_A);
        expect(td_vx==0&&td_vy>0,"south heading settles old eastward slip exactly");
        for(unsigned tick=0;tick<180;tick++)driving_tick(J_B);
        expect(td.speed==-4,"held brake stops before reaching a slower bounded reverse");
    }
    reset_case();td.onfoot=1;td.park_u=600*16;td.park_v=600*16;
    td.job=0;td.stage=1;td.health=100;td.left=300;
    expect(!td_motion_player_hit(390*16,450*16,390*16,450*16,5),"a parked car cannot injure the on-foot courier");
    expect(td_motion_player_hit(390*16,450*16,410*16,450*16,5),"a moving swept road body hits the courier even with clear separated endpoints");
    expect(td.health==85&&td.onfoot&&td.mode==TD_ROAM&&td_player_hurt==72,"a moving hit damages only carried cargo and starts bounded recoverable foot injury");
    expect(!td_motion_player_hit(390*16,450*16,410*16,450*16,5)&&td.health==85,"one vehicle impact cannot repeatedly drain cargo during recovery");
    UWORD origin=td.u;for(unsigned tick=0;tick<72;tick++)driving_tick(J_LEFT);
    expect(!td_player_hurt&&td.onfoot&&td.u==origin+24*16&&td.health==85,"input is held during24px knockback and72tick fall/get-up recovery");
    UWORD recovered=td.u;driving_tick(J_RIGHT);
    expect(td.u>recovered,"on-foot controls resume immediately after getting up");
    reset_case();td.onfoot=1;td.park_u=600*16;td.park_v=600*16;geometry=EAST_WALL;td.u=392*16;
    expect(td_motion_player_knockback(1,0),"on-foot body starts knockback beside a solid wall");
    for(unsigned tick=0;tick<72;tick++)driving_tick(J_RIGHT);
    expect(td.u<400*16&&!td_player_hurt,"knockback clips against solid ground and still recovers without crossing a building");
    reset_case();td.onfoot=1;td.mode=TD_PAUSE;
    expect(!td_motion_player_knockback(1,0),"paused menus cannot trigger a hidden player injury");
    reset_case();td.onfoot=1;td.park_u=600*16;td.park_v=600*16;
    td_traffic_u[0]=td.u-176;td_traffic_v[0]=td.v;
    td_traffic_samples[0].u=600*16;td_traffic_samples[0].v=td.v;
    td_traffic_advance=128;UWORD moving_from=td_traffic_u[0];
    td_traffic_motion(1);
    expect(td_traffic_u[0]==moving_from&&!td_player_hurt&&!td.cooldown&&td.onfoot,
           "ordinary admitted traffic now yields before crossing a newly approached foot courier; direct and pursuing contacts remain recoverable above");
}

#include "sandbox_save_harness.h"
#include "sandbox_harness.h"
#include "boat_traffic_harness.h"
#include "police_aircraft_harness.h"
#include "ramming_harness.h"
#include "traffic_courier_harness.h"

int main(void) {
    if(getenv("TD_COURIER_YIELD_ONLY")){test_ordinary_courier_yielding();printf("Ordinary courier yielding actual-C: %u checks, %u failures\n",checks,failures);return failures?1:0;}
    if(getenv("TD_SETTINGS_ONLY")){test_settings_controller();test_audio_event_integration();printf("Settings controller actual-C: %u checks, %u failures\n",checks,failures);return failures?1:0;}
    if(getenv("TD_BOAT_TRAFFIC_ONLY")){test_boat_traffic_exclusions();printf("Boat passenger/road traffic actual-C: %u checks, %u failures\n",checks,failures);return failures?1:0;}
    if(getenv("TD_POLICE_AIRCRAFT_ONLY")){test_police_aircraft_attention();printf("Police aircraft/attention actual-C: %u checks, %u failures\n",checks,failures);return failures?1:0;}
    if(getenv("TD_RAMMING_ONLY")){test_two_body_ramming();test_corner_physical_admission();printf("Two-body ramming actual-C: %u checks, %u failures\n",checks,failures);return failures?1:0;}
    if(getenv("TD_SANDBOX_ONLY")){test_sandbox_actual();printf("Road sandbox actual-C regressions: %u checks, %u failures\n",checks,failures);return failures?1:0;}
    if(getenv("TD_HARDWARE_MOTION_ONLY")){test_hardware_motion_feedback();printf("Hardware motion actual-C regressions: %u checks, %u failures\n",checks,failures);return failures?1:0;}
    test_ordinary_courier_yielding();test_hardware_motion_feedback();test_sandbox_entry_and_boat_save();test_boat_traffic_exclusions();test_police_aircraft_attention();test_two_body_ramming();test_corner_physical_admission();
    expect(sizeof(td_state_t)==58&&offsetof(td_state_t,district)==56,"host fixture retains the current serialized state layout");
    test_acceleration_and_turning();test_glancing_contact();test_wall_and_brake();
    test_momentum_and_coasting();test_pressed_edge_once();test_clock();test_clock_boundaries();
    test_passenger_comfort();test_entry_collision();test_hidden_pedestrian();test_pedestrian_phase_reuse();
    test_signal_and_autonomous_traffic();
    test_fractional_fleet_presentation();test_parked_visibility_context();test_route_selection_invalid_context();
    test_core_bus_lane_joint_loop();test_city_routes_and_walking();
    test_audio_event_integration();test_settings_controller();
    test_bounded_corner_assist();
    test_atomic_saves();test_valid_crc_invalid_states();test_legacy_and_transit_recovery();
    test_wait_cancellation();
    test_entry_transit_exclusion();test_fresh_transit_after_failure();
    test_pickup_damage_lifecycle();
    test_finished_job_target();
    test_result_b_release();test_result_release_initialization_and_save();
    test_menu_held_drive_buttons();
    test_current_transit_window();test_transit_funds_pause_and_deadline();test_immediate_transit_interrupted_save();
    test_safe_transit_alighting();
    test_cross_district_streetcar();
    test_v5_migration_and_interrupted_upgrade();test_district_semantic_fallback();
    test_reciprocal_portals();test_queue_failure_and_remote_boot();test_car_entry_at_portal();test_first_frame_actors();
    test_walk_pace_dispatch_and_foot_delivery();
    test_park_delivery_guidance();
    test_port_content_and_ordered_handoffs();test_port_save_and_final_credit();
    test_port_freight_transit_and_riverbank_walk();
    test_atlas_driver_handoff_and_freeze();
    test_streetcar_loader_binding();test_queen_visual_save_follow_and_loaded_arrival();
    test_queen_cold_derived_view();test_queen_prior_timetable_paid_save();test_queen_visual_queue_retry_clock();test_streetcar_pause_and_map_pose();
    test_streetcar_rail_parking_and_v6_recovery();test_streetcar_contacts_and_future_traffic_yield();
    test_queen_hold_v7_recovery_and_invalid_flags();
    test_displaced_streetcar_wait();
    test_streetcar_cue_and_blocked_contact();
    test_contact_episode_and_invalid();test_banked_traffic_segments();test_connected_traffic_retreat();test_retreat_future_truth();test_booked_landing_and_held_occupancy();
    test_streetcar_native_q5_bounds();test_contact_corridor_coverage();
    test_traffic_lookahead_truth();test_sweep_section_scan_equivalence();
    test_aircraft_world_freezing();
    test_human_impacts_and_police();test_visible_human_reverse_start();
    test_road_police_pursuit();
    test_dispatch_itinerary_inputs();test_dispatch_chapter_inputs();test_dispatch_chapter_eligibility();test_dispatch_acceptance_and_reentry();
    test_dispatch_active_preview_resume();test_dispatch_credit_cache_and_order();test_dispatch_transient_save_contract();
    test_reserved_islands_traffic_gates();test_current_ferry_fare_boundary();
    test_island_save_migration();test_north_save_migration();test_island_objective_guidance();
    printf("Host engine regressions: %u checks, %u failures. Hardware/emulator evidence remains separate.\n",checks,failures);
    return failures?1:0;
}
