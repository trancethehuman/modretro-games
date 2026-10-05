#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gbvm_stubs.h"
#include "td_game.h"
#include "td_district.h"
#include "td_world.h"
#include "td_scenery.h"
#include "td_streetcar.h"
#include "td_streetcar_runtime.h"
#include "td_scooter.h"
#include "td_traffic.h"
#include "native_fixture.h"

td_state_t td;td_job_t td_job,td_offer;td_stop_t td_target,td_cursor;
actor_t actors[22],*actors_inactive_head;UBYTE actors_len=22;
UWORD td_traffic_u[8],td_traffic_v[8];td_traffic_sample_t td_traffic_samples[8];
UWORD td_streetcar_focus_u,td_streetcar_focus_v;UBYTE td_streetcar_view_district,td_streetcar_ride_view;
WORD td_vx,td_vy,draw_scroll_x,draw_scroll_y;
UBYTE td_entry_timer,td_entry_target,tile_hit_x,tile_hit_y,joy,joy_pressed;
static unsigned checks,failures,saves,transfers,render_calls,parking_calls,terrain_calls;
static UBYTE current_district,use_raw,rail_clear=1,prop_clear=1,boat,rebounding;
static int wall_x=-1;static UBYTE rendered[8];
static void expect(int value,const char *message){checks++;if(!value){failures++;if(failures<15)fprintf(stderr,"FAIL: %s\n",message);}}
UBYTE td_district_current(void){return current_district;}
UBYTE td_boats_controlled(void){return boat;}
UBYTE td_motion_rebounding(void){return rebounding;}
void td_motion_transfer(UBYTE keep){transfers++;td.speed=td.speed<0?-(WORD)keep:keep;}
void td_save(void){saves++;}
void td_set_target(void){}
UBYTE tile_at(UBYTE x,UBYTE y){
    if(x>=128||y>=122)return 15;
    if(wall_x>=0&&x==wall_x)return 15;
    return use_raw?native_collision[current_district][y*128+x]:0;
}
UBYTE tile_col_test_range_x(UBYTE mask,UBYTE row,UBYTE first,UBYTE last){
    terrain_calls++;for(unsigned x=first;x<=last;x++)if(tile_at(x,row)&mask){tile_hit_x=x;tile_hit_y=row;return TRUE;}return FALSE;
}
UBYTE tile_col_test_range_y(UBYTE mask,UBYTE column,UBYTE first,UBYTE last){
    for(unsigned y=first;y<=last;y++)if(tile_at(column,y)&mask){tile_hit_x=column;tile_hit_y=y;return TRUE;}return FALSE;
}
UBYTE td_road_walkable(UWORD u,UWORD v){return u<1024&&v<976&&!(tile_at(u>>3,v>>3)&15);}
UBYTE td_streetcar_runtime_parking_allowed(UBYTE district,UWORD u,UWORD v){(void)district;(void)u;(void)v;parking_calls++;return rail_clear;}
UBYTE td_streetcar_runtime_traffic_clear_extent(UBYTE district,UWORD u,UWORD v,UBYTE half){(void)district;(void)u;(void)v;(void)half;return rail_clear;}
UBYTE td_streetcar_runtime_traffic_sweep_clear(UBYTE d,UWORD ou,UWORD ov,UWORD u,UWORD v,UBYTE half){(void)d;(void)ou;(void)ov;(void)u;(void)v;(void)half;return rail_clear;}
UBYTE td_scenery_contact(UWORD ou,UWORD ov,UWORD u,UWORD v,UBYTE half,BYTE speed){(void)ou;(void)ov;(void)u;(void)v;(void)half;(void)speed;return prop_clear?TD_SCENERY_CLEAR:TD_SCENERY_BLOCK;}
void td_player_sprite_restore(void){}
void td_fleet_present(actor_t *a,UBYTE kind,UBYTE direction){a->frame=kind*8+direction*2;}
void td_vehicle_present(actor_t *a,UBYTE vehicle,UBYTE heading){a->frame=vehicle*8+((heading+1)&15)/2;}
void td_scooter_parked_present(actor_t *a,UBYTE heading){a->frame=49+((((heading+1)&15)/4)&1);}
void td_civilian_present(actor_t *a,UBYTE variant,UBYTE pose){a->frame=100+variant*6+pose;}
void td_actor_render_actor(actor_t *a){if(render_calls<8)rendered[render_calls]=a->frame;render_calls++;}

#include "scooter_actual.inc"

static void reset_case(UBYTE district){
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    td.district=td.park_district=current_district=td_streetcar_view_district=district;
    td.mode=TD_ROAM;td.u=560*16;td.v=720*16;td.park_u=td.u;td.park_v=td.v;
    td.health=td.vitality=100;td.job=TD_NONE;td.speed=12;td_entry_timer=td_entry_target=0;
    td_streetcar_ride_view=boat=rebounding=use_raw=0;rail_clear=prop_clear=1;wall_x=-1;
    td_vx=192;td_vy=0;draw_scroll_x=480;draw_scroll_y=632;
    for(unsigned i=0;i<8;i++){td_traffic_u[i]=(48+i*48)*16;td_traffic_v[i]=48*16;actors[9+i].flags=ACTOR_FLAG_HIDDEN;}
    saves=transfers=render_calls=parking_calls=terrain_calls=0;
    td_scooter_reset();td_sandbox_reset();td_sandbox_bind(td_traffic_u,td_traffic_v,district);td_scooter_bind(district);
    memset(td_sandbox_parked,0,sizeof(td_sandbox_parked));
}
static int raw_body(UBYTE d,UWORD u,UWORD v,unsigned half){
    if(u<half||v<half||u+half>=1024||v+half>=976)return 0;
    for(unsigned y=(v-half)/8;y<=(v+half)/8;y++)for(unsigned x=(u-half)/8;x<=(u+half)/8;x++)
        if(native_collision[d][y*128+x]&15)return 0;
    return 1;
}
static void test_routes(void){
    for(unsigned d=0;d<7;d++)if(d!=5)for(unsigned r=0;r<2;r++){
        unsigned road=0,sidewalk=0;
        for(unsigned leg=0;leg<4;leg++){
            int x=td_scooter_routes[d][r][leg][0],y=td_scooter_routes[d][r][leg][1];
            int tx=td_scooter_routes[d][r][(leg+1)&3][0],ty=td_scooter_routes[d][r][(leg+1)&3][1];
            expect(x==tx||y==ty,"every authored scooter leg is cardinal");
            for(;;){
                expect(raw_body(d,x,y,7),"every scooter route pixel retains its full mounted footprint on registered solid/water collision");
                unsigned t=native_collision[d][(y/8)*128+x/8];road+=t==0;sidewalk+=!!(t&16);
                if(x==tx&&y==ty)break;x+=x<tx?1:x>tx?-1:0;y+=y<ty?1:y>ty?-1:0;
            }
        }
        expect(road&&sidewalk,"every independent rider uses both road and sidewalk geometry");
    }
}
static void test_admission_and_freeze(void){
    reset_case(0);expect(td_scooter_update(8),"Core riders admit at actual offscreen route points");
    for(unsigned i=0;i<2;i++)expect((td_scooter_riders[i].flags&1)&&!td_scooter_view(td_scooter_riders[i].u,td_scooter_riders[i].v),"whole rider pose and camera margin are outside the screen at initial admission");
    td_rider_t before[2];memcpy(before,td_scooter_riders,sizeof(before));
    td_scooter_bind(0);expect(!memcmp(before,td_scooter_riders,sizeof(before)),"same-district interior return preserves all visible rider identities");
    for(unsigned mode=0;mode<=TD_DIALOG;mode++)if(mode!=TD_ROAM&&mode!=TD_WAIT&&mode!=TD_RIDE){
        td.mode=mode;UBYTE elapsed=td_scooter_elapsed;expect(!td_scooter_update(600)&&!memcmp(before,td_scooter_riders,sizeof(before))&&elapsed==td_scooter_elapsed,"menus, dialogue, results and transit selection freeze movement, admission, hit arcs and accumulator");
    }
    td.mode=TD_ROAM;current_district=TD_DISTRICT_NONE;expect(!td_scooter_update(64)&&!memcmp(before,td_scooter_riders,sizeof(before)),"an actual shop/interior cannot move stale outdoor riders");
    current_district=0;td_streetcar_view_district=1;expect(!td_scooter_update(64)&&!memcmp(before,td_scooter_riders,sizeof(before)),"queued scene mismatch freezes old district riders");
    td_streetcar_view_district=0;td_scooter_update(64000);
    for(unsigned i=0;i<2;i++)expect(abs((int)td_scooter_riders[i].u-before[i].u)+abs((int)td_scooter_riders[i].v-before[i].v)<=64,"large elapsed time cannot exceed eight half-pixel rider quanta");
    td_scooter_bind(5);expect(!td_scooter_update(64)&&!td_scooter_riders[0].flags&&!td_scooter_riders[1].flags,"Islands never acquire fictional road scooters");
}
static void test_native_parking(void){
    for(unsigned d=0;d<7;d++){
        reset_case(d);use_raw=1;td_sandbox_ready=0;td_sandbox_bind(td_traffic_u,td_traffic_v,d);
        if(d==5){expect(!td_sandbox_parked[0].active&&!td_sandbox_parked[1].active,"island parking remains empty");continue;}
        expect(td_sandbox_parked[0].active&&td_sandbox_parked[0].vehicle<2,"the first seeded original parked car is retained");
        expect(td_sandbox_parked[1].active&&td_sandbox_parked[1].vehicle==3&&td_sandbox_parked[1].skin==TD_NONE,"every mainland supplies a real parked scooter without replacing any of eight road cars");
        expect(raw_body(d,td_sandbox_parked[1].u>>4,td_sandbox_parked[1].v>>4,8)&&parking_calls,"new parked scooter checks whole native geometry and full rail schedule admission");
        if(!d)expect(td_sandbox_parked[1].u==584*16&&td_sandbox_parked[1].v==744*16,"first Core scooter is deterministic beside Union");
    }
    reset_case(0);rail_clear=0;td_sandbox_ready=0;td_sandbox_bind(td_traffic_u,td_traffic_v,0);expect(!td_sandbox_parked[1].active,"blocked rail admission cannot invent an unsafe scooter");
    reset_case(0);prop_clear=0;td_sandbox_ready=0;td_sandbox_bind(td_traffic_u,td_traffic_v,0);expect(!td_sandbox_parked[1].active,"intact street furniture can reject every scooter bay without being destroyed by seeding");
}
static void test_borrowing(void){
    reset_case(0);td.onfoot=1;td.u=568*16;td.v=744*16;td.park_u=560*16;td.park_v=720*16;
    td_sandbox_parked[1]=(td_parked_t){584*16,744*16,0,3,TD_NONE,0,1,0};
    expect(td_sandbox_interact()==1&&td.vehicle==3&&td_sandbox_owner==9&&td_entry_timer==12,"either existing contextual button can borrow a vacant parked scooter through actual twelve-tick entry");
    expect(td_sandbox_parked[2].active&&td_sandbox_parked[2].protected&&td_sandbox_parked[2].u==560*16&&td_sandbox_parked[2].v==720*16,"the prior owned vehicle is preserved before scooter assignment");
    expect(saves==1&&!td_sandbox_extracting(),"a vacant scooter has no invented driver extraction and commits entry once");
    td.onfoot=0;td.heading=4;td.u=600*16;td.v=744*16;td_sandbox_sync();
    expect(td_sandbox_parked[1].u==td.u&&td_sandbox_parked[1].protected,"driven scooter retains its protected physical identity");
    td.onfoot=1;td.park_u=td.u;td.park_v=td.v;td_sandbox_present();expect(actors[8].frame==50,"leaving the scooter displays the appended empty vertical pose");
    td_sandbox_bind(td_traffic_u,td_traffic_v,0);expect(td_sandbox_owner==9&&td_sandbox_parked[1].u==600*16,"shop return cannot replace the owned scooter");
    reset_case(0);td.onfoot=1;td.u=400*16;td.v=450*16;td.park_u=80*16;td.park_v=64*16;
    td_sandbox_parked[1]=(td_parked_t){410*16,450*16,0,3,TD_NONE,0,1,0};
    for(unsigned i=2;i<4;i++)td_sandbox_parked[i]=(td_parked_t){(200+i*32)*16,600*16,0,0,0,0,1,1};
    td_state_t before=td;expect(td_sandbox_interact()==3&&!memcmp(&before,&td,sizeof(td))&&td_sandbox_owner==TD_NONE,"full preservation capacity refuses theft instead of silently deleting an abandoned car");
}
static void test_ramming(void){
    for(unsigned type=0;type<4;type++)for(unsigned speed=3;speed<=18;speed++)for(unsigned direction=0;direction<4;direction++){
        reset_case(0);td.vehicle=type;td.speed=speed;td.u=400*16;td.v=450*16;td.park_u=80*16;td.park_v=64*16;
        td_scooter_riders[0]=(td_rider_t){td.u+td_scooter_dx[direction]*14*16,td.v+td_scooter_dy[direction]*14*16,1,0,1};
        UWORD u=td.u+td_scooter_dx[direction]*8,v=td.v+td_scooter_dy[direction]*8;
        expect(td_scooter_ram(td.u,td.v,u,v)&&(td_scooter_riders[0].flags&2),"actual cardinal faster vehicle impact transfers momentum and ejects the occupied scooter rider");
        expect(td.speed>0&&td.speed<=(WORD)speed&&transfers==1&&td_scooter_take_hits()==1&&!td_scooter_take_hits(),"mass-dependent transfer remains positive and produces one consumable occupied-rider consequence");
        UWORD ou=td_scooter_riders[0].u,ov=td_scooter_riders[0].v;
        td.u=200*16;td.v=500*16;td_scooter_update(64);expect(td_scooter_riders[0].hit==64&&abs((int)td_scooter_riders[0].u-ou)+abs((int)td_scooter_riders[0].v-ov)<=42,"speed-bounded impulse uses checked terrain and a finite non-graphic airborne phase");
    }
    reset_case(0);td.u=400*16;td.v=450*16;td_scooter_riders[0]=(td_rider_t){414*16,450*16,1,0,1};td.speed=2;
    expect(!td_scooter_ram(td.u,td.v,td.u+8,td.v)&&td_scooter_riders[0].flags==1&&!transfers,"slow contact does not destroy or teleport the rider");
    td.speed=12;prop_clear=0;expect(!td_scooter_ram(td.u,td.v,td.u+8,td.v)&&td_scooter_riders[0].flags==1&&!transfers,"blocked first shove preserves rider state and momentum rather than moving through a prop");
    prop_clear=1;rail_clear=0;expect(!td_scooter_ram(td.u,td.v,td.u+8,td.v)&&td_scooter_riders[0].flags==1,"tram guard rejects a scooter impulse");
    reset_case(0);td.u=400*16;td.v=450*16;td_sandbox_parked[1]=(td_parked_t){414*16,450*16,0,3,TD_NONE,0,1,0};
    expect(td_scooter_ram(td.u,td.v,td.u+8,td.v)&&(td_sandbox_parked[1].active&2)&&transfers==1,"vacant parked scooter rams through its actual stored body and packed impulse state");
    UWORD pu=td_sandbox_parked[1].u;td_scooter_update(56);
    expect(td_sandbox_parked[1].u>pu&&td_sandbox_parked[1].u-pu<=98&&!(td_sandbox_parked[1].active>>5)&&!td_scooter_take_hits(),"vacant scooter has seven bounded checked shove ticks and no invented human collision");
    td.onfoot=1;td.u=td_sandbox_parked[1].u-16*16;td.v=td_sandbox_parked[1].v;td.park_u=80*16;td.park_v=64*16;
    expect(!td_sandbox_interact(),"a wrecked scooter cannot be borrowed as an intact vehicle");
    reset_case(0);td.u=400*16;td.v=450*16;td_sandbox_owner=9;td_sandbox_parked[1]=(td_parked_t){414*16,450*16,0,3,TD_NONE,0,1,1};
    expect(!td_sandbox_scooter_ram(td.u,td.v,td.u+8,td.v)&&td_sandbox_parked[1].active==1,"current owned scooter never collides with its parked alias");
    for(unsigned speed=3;speed<=4;speed++){
        reset_case(0);td.u=400*16;td.v=450*16;td.speed=-(WORD)speed;
        td_scooter_riders[0]=(td_rider_t){386*16,450*16,1,0,1};
        UWORD fleet_u[8],fleet_v[8];td_parked_t parked[4];
        memcpy(fleet_u,td_traffic_u,sizeof(fleet_u));memcpy(fleet_v,td_traffic_v,sizeof(fleet_v));memcpy(parked,td_sandbox_parked,sizeof(parked));
        expect(td_scooter_ram(td.u,td.v,td.u-8,td.v)&&td.speed<0&&td_scooter_take_hits()==1,"small reverse impact retains signed reverse momentum and one occupied-rider consequence");
        expect(!memcmp(fleet_u,td_traffic_u,sizeof(fleet_u))&&!memcmp(fleet_v,td_traffic_v,sizeof(fleet_v))&&!memcmp(parked,td_sandbox_parked,sizeof(parked)),"reverse rider ramming never mutates the original fleet or unrelated parked identities");
    }
}
static void test_elapsed_partitions(void){
    td_parked_t expected;td_rider_t riders[2];UBYTE residual;
    for(unsigned partition=0;partition<3;partition++){
        reset_case(0);td.u=400*16;td.v=450*16;
        td_sandbox_parked[1]=(td_parked_t){414*16,450*16,0,3,TD_NONE,0,1,0};
        expect(td_scooter_ram(td.u,td.v,td.u+8,td.v),"partition fixture executes actual vacant scooter impulse");
        if(partition==0)td_scooter_update(56);
        else for(unsigned i=0;i<(partition==1?7:56);i++)td_scooter_update(partition==1?8:1);
        if(!partition){expected=td_sandbox_parked[1];memcpy(riders,td_scooter_riders,sizeof(riders));residual=td_scooter_elapsed;}
        else expect(!memcmp(&expected,&td_sandbox_parked[1],sizeof(expected))&&!memcmp(riders,td_scooter_riders,sizeof(riders))&&residual==td_scooter_elapsed,"1x56,7x8,56x1 actual VBlanks have identical vacant shove, rider motion and residual timing");
    }
    reset_case(0);td.u=400*16;td.v=450*16;
    td_sandbox_parked[1]=(td_parked_t){414*16,450*16,0,3,TD_NONE,0,1,0};td_scooter_ram(td.u,td.v,td.u+8,td.v);
    expected=td_sandbox_parked[1];td.mode=TD_PAUSE;
    for(unsigned i=0;i<72;i++)td_scooter_update(8);
    expect(!memcmp(&expected,&td_sandbox_parked[1],sizeof(expected)),"pause freezes the actual packed vacant scooter impulse for arbitrary wall-clock time");
    td.mode=TD_ROAM;for(unsigned i=0;i<12;i++)td_sandbox_tick();
    expect(!memcmp(&expected,&td_sandbox_parked[1],sizeof(expected)),"driver extraction ticks cannot accelerate or expire a scooter impulse");
    td_scooter_update(56);expect(!(td_sandbox_parked[1].active>>5),"resumed active time expires the seven actual scooter quanta");
}
static void test_cover_render_edges(void){
    reset_case(0);td_scooter_riders[0]=(td_rider_t){400*16,450*16,1,0,1};
    expect(!td_sandbox_clear(380*16,450*16,420*16,450*16,7,TD_NONE),"long player sweep cannot tunnel through an independent rider");
    expect(!td_scooter_foot_clear(400*16,450*16)&&td_scooter_foot_clear(420*16,450*16),"normal pedestrians and door probes yield to the actual rider body");
    expect(td_scooter_clear(400*16,450*16,401*16,450*16,7,240),"an autonomous rider skips only its own body");
    expect(td_scooter_clear(8*16,8*16,9*16,8*16,7,TD_NONE)&&!td_scooter_clear(65535,0,0,0,7,TD_NONE),"opposite map edges remain clear without wrapped coordinate admission");
    tile_hit_x=41;tile_hit_y=92;wall_x=50;expect(!td_scooter_terrain(399*16,450*16,400*16,450*16,7,NULL)&&tile_hit_x==41&&tile_hit_y==92,"whole-body wall rejection restores collision hit ownership");
    wall_x=-1;draw_scroll_x=320;draw_scroll_y=362;td_scooter_render();expect(render_calls==1&&rendered[0]>=24&&rendered[0]<=31,"mounted rider renders original scooter art through the existing safe renderer");
    render_calls=0;td_scooter_riders[0].flags=3;td_scooter_riders[0].hit=0;td_scooter_render();expect(render_calls==2&&rendered[0]==49&&rendered[1]==100+6+TD_CIVILIAN_HIT,"crash renders one empty scooter OBJ plus one original airborne civilian");
    render_calls=0;td_scooter_riders[0].hit=96;td_scooter_render();expect(render_calls==2&&rendered[1]==100+6+TD_CIVILIAN_PRONE,"grounded wreck and prone rider retain the same two-object rendering budget");
    td.u=400*16;td.v=450*16;td_scooter_update(8);expect(td_scooter_riders[0].flags&1,"whole visible wreck identity is never retired or replaced");
    td.u=800*16;td.v=800*16;td_scooter_update(8);expect(!td_scooter_riders[0].flags,"wreck retires only after its complete pose and camera guard leave the screen");
}
static void test_external_traffic(void){
    reset_case(0);UBYTE extents[8]={5,6,5,7,6,7,5,5};td_traffic_context_t ctx={td_traffic_u,td_traffic_v,&actors[9],extents,extents,0,0,0,0};
    /* Use the production generated first four-arm Core signal. */
    unsigned index=0;while(index<td_signal_offsets[1]&&td_signals_h[index].arms!=15)index++;
    expect(index<td_signal_offsets[1],"Core fixture contains an actual generated four-arm junction");
    UWORD u=td_signals_h[index].u*16-24*16,v=td_signals_h[index].v*16+8*16;
    for(unsigned i=0;i<8;i++){td_traffic_u[i]=(700+i*24)*16;td_traffic_v[i]=900*16;}
    expect(!td_traffic_external_admit(&ctx,0,8,u,v,u+8,v,7)&&td_traffic_external_admit(&ctx,0,0,u,v,u+8,v,7),"independent rider obeys the same real authored red/green stop line");
    expect(!td_traffic_external_admit(&ctx,0,8,u,v+16*16,u+8,v+16*16,7)&&td_traffic_external_admit(&ctx,0,0,u,v+16*16,u+8,v+16*16,7),"sidewalk rider cannot bypass red via the same junction's24px curb lane");
    td_traffic_u[7]=td_signals_h[index].u*16;td_traffic_v[7]=td_signals_h[index].v*16;
    expect(!td_traffic_external_admit(&ctx,0,0,u,v,u+8,v,7),"eighth original car retains junction ownership; no slot is substituted for the rider");
    td_traffic_u[7]=900*16;td_traffic_v[7]=900*16;actors[9].flags=0;actors[9].pos.x=(u+12*16)*2;actors[9].pos.y=v*2;
    expect(!td_traffic_external_admit(&ctx,0,0,u,v,u+8,v,7),"independent rider yields to the existing live pedestrian body");
    actors[9].flags=ACTOR_FLAG_HIDDEN;
    for(unsigned slot=0;slot<8;slot++){
        UWORD old=td_traffic_u[slot];td_traffic_u[slot]=1;
        expect(!td_traffic_external_admit(&ctx,0,0,400*16,450*16,400*16+8,450*16,7),"malformed distant road body fails closed even for independent rider admission");td_traffic_u[slot]=old;
    }
    expect(!td_traffic_external_admit(NULL,0,0,u,v,u+8,v,7)&&!td_traffic_external_admit(&ctx,0,0,u,v,u+16,v,7)&&!td_traffic_external_admit(&ctx,0,0,u,v,u+8,v+8,7),"external motion rejects null context, oversized and diagonal proposals");
    UWORD fleet_u[8],fleet_v[8];memcpy(fleet_u,td_traffic_u,sizeof(fleet_u));memcpy(fleet_v,td_traffic_v,sizeof(fleet_v));
    for(unsigned i=0;i<100;i++)td_traffic_external_admit(&ctx,0,i%12,u,v,u+8,v,7);
    td_scooter_update(64);
    expect(!memcmp(fleet_u,td_traffic_u,sizeof(fleet_u))&&!memcmp(fleet_v,td_traffic_v,sizeof(fleet_v)),"external admissions and independent rider updates are read-only toward every original road vehicle cache");
}
/* Independent linear signal oracle uses the immutable old fleet's body and
 * entry helpers. It checks the split unit's private copies and binary lane
 * search against every generated signal, phase, direction and curb offset. */
static UBYTE reference_external(const td_traffic_context_t *ctx,UBYTE d,UWORD seconds,
    UWORD ou,UWORD ov,UWORD u,UWORD v,UBYTE half){
    td_traffic_motion_t m;const td_signal_t *signals;UBYTE horizontal,arm;
    UWORD pos,next,lateral,line;
    if(!ctx||!ctx->u||!ctx->v||ctx->parked_active>1||!half||half>8||d>=7||
       !td_traffic_step_limit(ou,ov,u,v,8)||!td_traffic_body_valid(ou,ov,half,half)||
       !td_traffic_body_valid(u,v,half,half))return FALSE;
    m.old_u=ou;m.old_v=ov;m.u=u;m.v=v;m.hu=m.hv=half;
    m.left=(ou<u?ou:u)-half*16;m.right=(ou>u?ou:u)+half*16;
    m.top=(ov<v?ov:v)-half*16;m.bottom=(ov>v?ov:v)+half*16;
    if(!td_traffic_bodies(ctx,8,&m,1))return FALSE;
    if(ou==u&&ov==v)return TRUE;
    horizontal=ou!=u;pos=horizontal?ou:ov;next=horizontal?u:v;lateral=horizontal?v:u;
    arm=horizontal?(u>ou?8:2):(v>ov?1:4);signals=horizontal?td_signals_h:td_signals_v;
    for(unsigned i=td_signal_offsets[d];i<td_signal_offsets[d+1];i++){
        UWORD lat=(horizontal?signals[i].v:signals[i].u)*16;
        if(td_scooter_distance(lat,lateral)>32*16||!(signals[i].arms&arm))continue;
        UWORD centre=(horizontal?signals[i].u:signals[i].v)*16;
        line=next>pos?centre-24*16:centre+24*16;
        if(next>pos?(pos<=line&&next>line):(pos>=line&&next<line))
            if((horizontal?seconds%12>=7:seconds%12<7)||!td_traffic_entry_clear(ctx,8,signals[i].u*16,signals[i].v*16))return FALSE;
    }
    return TRUE;
}
static uint32_t rider_oracle_seed=0x39247ac1;
static uint32_t rider_oracle_random(void){rider_oracle_seed=rider_oracle_seed*1664525u+1013904223u;return rider_oracle_seed;}
static void test_split_differential(void){
    reset_case(0);UBYTE extents[8]={5,6,5,7,6,7,5,5};td_traffic_context_t ctx={td_traffic_u,td_traffic_v,&actors[9],extents,extents,0,0,0,0};
    td_rider_batch_t batch;
    for(unsigned i=0;i<8;i++){td_traffic_u[i]=(800+i*20)*16;td_traffic_v[i]=920*16;}
    expect(td_traffic_external_begin(&ctx,&batch),"all authored signal samples borrow one validated immutable fleet context");
    for(unsigned d=0;d<7;d++)for(unsigned i=td_signal_offsets[d];i<td_signal_offsets[d+1];i++)
        for(unsigned direction=0;direction<4;direction++)for(int offset=-33;offset<=33;offset++)for(unsigned phase=0;phase<12;phase++){
            int x=td_signals_h[i].u*16,y=td_signals_h[i].v*16;
            int dx=td_scooter_dx[direction],dy=td_scooter_dy[direction];
            UWORD u=x-dx*24*16+(dy?offset*16:0),v=y-dy*24*16+(dx?offset*16:0);
            expect(td_traffic_external_admit(&ctx,d,phase,u,v,u+dx*8,v+dy*8,7)==reference_external(&ctx,d,phase,u,v,u+dx*8,v+dy*8,7),
                   "split rider bank matches immutable old-body/linear-signal oracle at every light, direction, phase and sidewalk boundary");
            expect(td_traffic_external_batch_admit(&batch,d,phase,u,v,u+dx*8,v+dy*8,7)==reference_external(&ctx,d,phase,u,v,u+dx*8,v+dy*8,7),
                   "borrowed rider batch matches independent oracle at every authored signal, direction, phase and sidewalk boundary");
        }
    for(unsigned n=0;n<100000;n++){
        UBYTE half=1+n%8,d=n%7,dir=(n/7)%4;
        UWORD u=(16+rider_oracle_random()%992)*16,v=(16+rider_oracle_random()%944)*16;
        UWORD next_u=u+td_scooter_dx[dir]*(n%9),next_v=v+td_scooter_dy[dir]*(n%9);
        for(unsigned i=0;i<8;i++){
            td_traffic_u[i]=(16+rider_oracle_random()%992)*16;td_traffic_v[i]=(16+rider_oracle_random()%944)*16;
            extents[i]=1+rider_oracle_random()%16;
        }
        if(!(n&7)){td_traffic_u[n%8]=u;td_traffic_v[n%8]=v;}
        if(!(n&31))td_traffic_u[n%8]=65535;
        ctx.priority_mask=rider_oracle_random();ctx.parked_active=n%17==0?2:n%3==0;
        ctx.park_u=(16+rider_oracle_random()%992)*16;ctx.park_v=(16+rider_oracle_random()%944)*16;
        actors[9].flags=n&1?ACTOR_FLAG_HIDDEN:0;actors[9].pos.x=(u+96)*2;actors[9].pos.y=v*2;
        UWORD seconds=rider_oracle_random();
        expect(td_traffic_external_admit(&ctx,d,seconds,u,v,next_u,next_v,half)==reference_external(&ctx,d,seconds,u,v,next_u,next_v,half),
               "split private helpers retain exact full bodies, priority, old overlap escape, malformed far actors and parked/person admission");
    }
}
static void test_rider_batch_differential(void){
    UBYTE extents[8]={5,6,5,7,6,7,5,5};td_rider_batch_t batch;
    td_traffic_context_t ctx={td_traffic_u,td_traffic_v,&actors[9],extents,extents,0,0,0,0};
    reset_case(0);
    expect(!td_traffic_external_begin(NULL,&batch)&&!batch.valid&&!batch.ctx,
           "missing context invalidates a borrowed batch rather than retaining an earlier proof");
    expect(!td_traffic_external_begin(&ctx,NULL)&&
           !td_traffic_external_batch_admit(NULL,0,0,400*16,450*16,400*16+8,450*16,7),
           "null borrowed batch fails closed without changing context");
    expect(!td_traffic_external_batch_admit(&batch,0,0,400*16,450*16,400*16+8,450*16,7),
           "failed begin cannot authorize any rider admission");
    for(unsigned n=0;n<30000;n++){
        reset_case(n%7);
        for(unsigned i=0;i<8;i++){
            td_traffic_u[i]=(16+rider_oracle_random()%992)*16+(rider_oracle_random()&15);
            td_traffic_v[i]=(16+rider_oracle_random()%944)*16+(rider_oracle_random()&15);
            extents[i]=1+rider_oracle_random()%16;
            actors[9+i].flags=(rider_oracle_random()&3)?ACTOR_FLAG_HIDDEN:0;
            actors[9+i].pos.x=(16+rider_oracle_random()%992)*32;
            actors[9+i].pos.y=(16+rider_oracle_random()%944)*32;
        }
        ctx.priority_mask=rider_oracle_random();ctx.parked_active=n%13==0?2:n%3==0;
        ctx.park_u=(16+rider_oracle_random()%992)*16;ctx.park_v=(16+rider_oracle_random()%944)*16;
        if(!(n&7))td_traffic_u[n%8]=65535;
        if(!(n&15)){actors[9+n%8].flags=0;actors[9+n%8].pos.x=0;}
        if(n%29==0)extents[n%8]=0;
        td_traffic_context_t original=ctx;actor_t people[8];UWORD fleet_u[8],fleet_v[8];
        memcpy(people,&actors[9],sizeof(people));memcpy(fleet_u,td_traffic_u,sizeof(fleet_u));memcpy(fleet_v,td_traffic_v,sizeof(fleet_v));
        UBYTE valid=td_traffic_external_begin(&ctx,&batch);
        expect(valid==(batch.valid==TD_RIDER_BATCH_VALID),"batch proof is published only after whole immutable context validation");
        UWORD seconds=rider_oracle_random();
        /* Multiple arbitrary riders reuse one proof; no candidate or blocked
           move can change the fleet/pedestrian/park data it borrows. */
        for(unsigned j=0;j<12;j++){
            UBYTE half=1+rider_oracle_random()%8,d=rider_oracle_random()%9,dir=rider_oracle_random()%4;
            UWORD u=(16+rider_oracle_random()%992)*16+(rider_oracle_random()&15),v=(16+rider_oracle_random()%944)*16+(rider_oracle_random()&15);
            UWORD nu=u+td_scooter_dx[dir]*(rider_oracle_random()%10),nv=v+td_scooter_dy[dir]*(rider_oracle_random()%10);
            if(j==3)nu=nv=65535;
            if(j==4){nu=u;nv=v;}
            if(j==5){nu=u+8;nv=v+8;}
            expect(td_traffic_external_batch_admit(&batch,d,seconds,u,v,nu,nv,half)==
                   reference_external(&ctx,d,seconds,u,v,nu,nv,half),
                   "reused rider validation matches immutable old-body/linear-signal oracle across full bodies, malformed geometry, priority, people, park, diagonals and zero movement");
        }
        expect(!memcmp(&ctx,&original,sizeof(ctx))&&!memcmp(people,&actors[9],sizeof(people))&&
               !memcmp(fleet_u,td_traffic_u,sizeof(fleet_u))&&!memcmp(fleet_v,td_traffic_v,sizeof(fleet_v)),
               "borrowed rider batch never mutates context, authored fleet or people");
    }
}
static UBYTE reference_scooter_terrain(UWORD ou,UWORD ov,UWORD u,UWORD v,UBYTE half){
    if(!half||half>8||ou<half*16||u<half*16||ov<half*16||v<half*16||
       ou>(1024-half)*16||u>(1024-half)*16||ov>(976-half)*16||v>(976-half)*16||
       (ou!=u&&ov!=v))return FALSE;
    unsigned l=((ou<u?ou:u)-half*16)/128,r=((ou>u?ou:u)+half*16)/128;
    unsigned t=((ov<v?ov:v)-half*16)/128,b=((ov>v?ov:v)+half*16)/128;
    for(unsigned y=t;y<=b;y++)for(unsigned x=l;x<=r;x++)if(tile_at(x,y)&15)return FALSE;
    return TRUE;
}
static void test_scooter_terrain_reuse(void){
    td_scooter_terrain_cache_t cache={0};
    reset_case(0);UBYTE expected;
    for(unsigned blocked=0;blocked<2;blocked++){
        wall_x=blocked?50:-1;cache.valid=0;
        for(unsigned i=0;i<8;i++){
            UWORD ou=405*16+i,u=ou+1,v=450*16;
            unsigned before=terrain_calls;tile_hit_x=41;tile_hit_y=92;
            expected=reference_scooter_terrain(ou,v,u,v,7);
            expect(td_scooter_terrain(ou,v,u,v,7,&cache)==expected&&tile_hit_x==41&&tile_hit_y==92,
                   "identical validated terrain union reuses both clear and blocked outcomes without changing collision hit ownership");
            expect(i?terrain_calls==before:terrain_calls>before,
                   "a raw-collision rectangle is queried once then reused across fractional steps in the same update");
        }
    }
    static const UWORD malformed[]={0,1,111,127,128,16383,16384,15615,15616,32767,65535};
    for(unsigned d=0;d<7;d++){
        reset_case(d);use_raw=1;cache.valid=0;
        for(unsigned n=0;n<65000;n++){
            UBYTE half=n%11,dir=rider_oracle_random()%4;
            UWORD ou=rider_oracle_random()%16500,ov=rider_oracle_random()%16000;
            if(!(n&7))ou=malformed[n%11];
            if(!(n&15))ov=malformed[(n/11)%11];
            UWORD u=ou+td_scooter_dx[dir]*(rider_oracle_random()%130),v=ov+td_scooter_dy[dir]*(rider_oracle_random()%130);
            if(n%19==0){u=ou+1;v=ov+1;}
            tile_hit_x=41;tile_hit_y=92;
            expected=reference_scooter_terrain(ou,ov,u,v,half);
            expect(td_scooter_terrain(ou,ov,u,v,half,&cache)==expected&&tile_hit_x==41&&tile_hit_y==92,
                   "local terrain reuse matches independent raw tile union across every district, half extent, fraction, malformed/cardinal/diagonal input and map edge");
        }
    }
    use_raw=0;wall_x=-1;
}
int main(void){
    test_routes();test_admission_and_freeze();test_native_parking();test_borrowing();test_ramming();test_elapsed_partitions();test_cover_render_edges();test_external_traffic();test_split_differential();test_rider_batch_differential();test_scooter_terrain_reuse();
    printf("Actual scooter/sandbox/external-traffic checks: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
