/* Differential exact contact/phase oracles for the actual production module. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "td_game.h"
#include "td_district.h"
#include "td_streetcar.h"
#include "td_streetcar_runtime.h"
#include "actor.h"
#define TD_WORLD_ROUTE_DATA
#include "td_world_routes.h"
td_state_t td;
static UBYTE police_observed;
UBYTE td_police_observed(void){return police_observed;}
actor_t actors[22];
UBYTE td_streetcar_ride_view,td_streetcar_view_district,td_resume_mode;
UWORD td_streetcar_focus_u,td_streetcar_focus_v;
static UWORD td_streetcar_elapsed;
static UBYTE fixture_routes[8],poses[8],variants[8];
static UBYTE fixture_authored_routes;
static UBYTE fixture_custom_routes;
static UWORD fixture_points[8][2];
static UBYTE fixture_terrain_active,fixture_terrain_district;
static unsigned fixture_tile_reads;
UBYTE tile_hit_x,tile_hit_y;
#include "people_raw_terrain.h"
static unsigned long checks,host_people_word_pairs,host_route_candidates;
static uint32_t random_state=0x479cd013;
static void require(int condition,const char *message){
    checks++;
    if(!condition){fprintf(stderr,"FAIL %lu: %s\n",checks,message);exit(1);}
}
void td_refresh_routes(UBYTE *routes,UWORD (*nearby)[2]){
    UBYTE district=td_streetcar_ride_view?td_streetcar_view_district:td.district;
    for(UBYTE i=0;i<8;i++){
        routes[i]=fixture_routes[i];
        if(fixture_custom_routes){
            nearby[i][0]=fixture_points[i][0];nearby[i][1]=fixture_points[i][1];
        }else if(fixture_authored_routes&&routes[i]!=TD_NONE){
            require(district<TD_DISTRICT_COUNT&&routes[i]<td_route_counts[district],"Authored fixture selects an actual loaded route");
            nearby[i][0]=td_district_routes[district][routes[i]][0];
            nearby[i][1]=td_district_routes[district][routes[i]][1];
        }else{nearby[i][0]=100+i*90;nearby[i][1]=100+i*80;}
    }
}
void td_save(void){}
UBYTE tile_at(UBYTE x,UBYTE y){
    fixture_tile_reads++;
    if(x>=128||y>=122)return 15;
    return fixture_terrain_active?people_raw_terrain[fixture_terrain_district][(unsigned)y*128+x]:0;
}
UBYTE tile_col_test_range_x(UBYTE mask,UBYTE row,UBYTE first,UBYTE last){
    tile_hit_y=row;for(unsigned x=first;x<=last;x++){tile_hit_x=x;UBYTE value=tile_at(x,row);if(value&mask)return value;}return 0;
}
UBYTE tile_col_test_range_y(UBYTE mask,UBYTE column,UBYTE first,UBYTE last){
    tile_hit_x=column;for(unsigned y=first;y<=last;y++){tile_hit_y=y;UBYTE value=tile_at(column,y);if(value&mask)return value;}return 0;
}
#include "walkable_query_under_test.c"
#include "rail_queries_under_test.c"
void td_civilian_present(actor_t *actor,UBYTE variant,UBYTE pose){
    ptrdiff_t index=actor-&actors[9];require(index>=0&&index<8,"Civilian actor slot remains bounded");
    variants[index]=variant;poses[index]=pose;
}
#include "people_under_test.c"
#include "route_queries_under_test.c"
static void social_body_terrain_differential(void){
    static const UWORD malformed[]={0,1,2,1021,1023,1024,15615,32767,65534,65535};
    fixture_terrain_active=1;
    for(unsigned district=0;district<TD_DISTRICT_COUNT;district++){
        fixture_terrain_district=district;
        for(unsigned v=0;v<976;v++)for(unsigned u=0;u<1024;u++){
            unsigned expected=u>=3&&v>=3&&u<=1020&&v<=972;
            if(expected){
                /* Independent four point reads from original registered
                   raw tiles; no range helper or unique-rectangle formula. */
                const UBYTE *raw=people_raw_terrain[district];
                expected=!(raw[((v-3)/8)*128+(u-3)/8]&15)&&
                    !(raw[((v-3)/8)*128+(u+3)/8]&15)&&
                    !(raw[((v+3)/8)*128+(u-3)/8]&15)&&
                    !(raw[((v+3)/8)*128+(u+3)/8]&15);
            }
            tile_hit_x=83;tile_hit_y=109;fixture_tile_reads=0;
            require(td_person_social_body_clear(u,v)==expected,"Unique social body query matches independent four corners at every pixel of every registered district");
            require(tile_hit_x==83&&tile_hit_y==109,"Social full-body terrain query restores native collision hit globals");
            unsigned unique=(u>=3&&v>=3&&u<=1020&&v<=972)?
                (((u+3)/8-(u-3)/8)+1)*(((v+3)/8-(v-3)/8)+1):0;
            require(fixture_tile_reads<=unique,"Social terrain reads never exceed unique original corner tiles");
        }
        for(unsigned i=0;i<sizeof(malformed)/sizeof(malformed[0]);i++)
            for(unsigned j=0;j<sizeof(malformed)/sizeof(malformed[0]);j++){
                UWORD u=malformed[i],v=malformed[j];unsigned expected=u>=3&&u<=1020&&v>=3&&v<=972;
                if(expected)expected=td_road_walkable(u-3,v-3)&&td_road_walkable(u+3,v-3)&&
                    td_road_walkable(u-3,v+3)&&td_road_walkable(u+3,v+3);
                tile_hit_x=83;tile_hit_y=109;fixture_tile_reads=0;
                require(td_person_social_body_clear(u,v)==expected&&tile_hit_x==83&&tile_hit_y==109,
                        "Malformed and map-edge social body coordinates retain original bounds and hit state without wrapping");
                if(u>1020||u<3||v>972||v<3)require(!fixture_tile_reads,"Out-of-world social body input performs no terrain read");
            }
    }
    fixture_terrain_active=0;fixture_tile_reads=0;
}
/* Exercise the actual fleet body/signal admission code alongside people.
 * The focused cases use these pure queries, not the separate terrain/tram
 * epoch wrapper. Its unused hardware callbacks fail closed and are counted. */
#include "../../games/toronto-dispatch/project/plugins/toronto-driving/engine/src/td_traffic.c"
static unsigned unused_traffic_callbacks;
UBYTE td_road_sweep(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half){
    (void)old_u;(void)old_v;(void)u;(void)v;(void)half;unused_traffic_callbacks++;return FALSE;
}

UBYTE td_streetcar_runtime_traffic_sweep_clear(UBYTE district,UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half){
    (void)district;(void)old_u;(void)old_v;(void)u;(void)v;(void)half;unused_traffic_callbacks++;return FALSE;
}
UBYTE td_traffic_signal_stop(UBYTE district,UWORD seconds,UWORD old_u,UWORD old_v,UWORD u,UWORD v){
    (void)district;(void)seconds;(void)old_u;(void)old_v;(void)u;(void)v;unused_traffic_callbacks++;return TRUE;
}

/* Independent actor-layout fixture: six original service vehicles remain
 * at2..7; added cars use17/18 so the eight people at9..16 never overlap. */
static const UBYTE fleet_actor_slots[TD_TRAFFIC_SLOTS]={2,3,4,5,6,7,17,18};
static void hide_fleet_fixture(void){
    for(unsigned i=0;i<TD_TRAFFIC_SLOTS;i++)actors[fleet_actor_slots[i]].flags=ACTOR_FLAG_HIDDEN;
}

static unsigned fleet_failures;
static void fleet_check(int condition,const char *message){
    checks++;
    if(!condition){
        fleet_failures++;
        if(fleet_failures<=12)fprintf(stderr,"FAIL %lu: %s\n",checks,message);
    }
}
static int reference_fleet_clear(UWORD u,UWORD v,UWORD car_u,UWORD car_v,unsigned half){
    /* Independent signed half-open rectangles in Q4; retain fractional fleet
     * centres instead of rounding the bus/fire body to a whole pixel. */
    int32_t pl=(int32_t)u*16-48,pr=(int32_t)u*16+48;
    int32_t pt=(int32_t)v*16-48,pb=(int32_t)v*16+48;
    int32_t cl=(int32_t)car_u-(int32_t)half*16,cr=(int32_t)car_u+(int32_t)half*16;
    int32_t ct=(int32_t)car_v-(int32_t)half*16,cb=(int32_t)car_v+(int32_t)half*16;
    return !(pl<cr&&cl<pr&&pt<cb&&ct<pb);
}
static void fleet_extent_guards(void){
    static const UBYTE extents[TD_TRAFFIC_SLOTS]={5,6,5,7,6,7,5,5};
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    td.mode=TD_ROAM;td.onfoot=1;td.park_u=560*16;td.park_v=720*16;
    td_streetcar_view_district=TD_DISTRICT_CITY;td_streetcar_ride_view=0;td_streetcar_elapsed=0;
    for(unsigned kind=0;kind<TD_TRAFFIC_SLOTS;kind++){
        UBYTE actor_index=fleet_actor_slots[kind];
        unsigned violations=0;
        hide_fleet_fixture();
        actors[actor_index].flags=0;
        for(unsigned fraction=0;fraction<2;fraction++){
            UWORD cu=500*16+fraction*8,cv=400*16+fraction*8;
            actors[actor_index].pos.x=cu*2;actors[actor_index].pos.y=cv*2;
            td_state_t saved=td;actor_t saved_actor=actors[actor_index];
            for(int dx=-11;dx<=11;dx++)for(int dy=-11;dy<=11;dy++){
                UWORD u=(UWORD)(500+dx),v=(UWORD)(400+dy);
                UBYTE clear=td_person_road_clear(u,v);
                int body_clear=reference_fleet_clear(u,v,cu,cv,extents[kind]);
                if(clear&&!body_clear)violations++;
                fleet_check(!clear||body_clear,"A permitted human position never overlaps the actual fleet body");
                if(dx<=-11||dx>=11||dy<=-11||dy>=11)
                    fleet_check(clear,"Humans remain free beyond the fleet exclusion boundary");
            }
            fleet_check(!memcmp(&td,&saved,sizeof(td))&&!memcmp(&actors[actor_index],&saved_actor,sizeof(saved_actor)),
                        "Fleet clearance queries preserve saved state and the actual actor body");
        }
        if(violations)fprintf(stderr,"Fleet kind%u half%u: %u unsafe permitted centres\n",kind,extents[kind],violations);
        actors[actor_index].flags=ACTOR_FLAG_HIDDEN;
        fleet_check(td_person_road_clear(500,400),"Hidden fleet bodies remain absent from pedestrian occupancy");
    }
    /* A coherent actual Core bus leg: from(808,168) north to(808,72),
     * with its next eight-pixel advance160->152. Route19 is the real
     * horizontal foot crossing(784..847,148), phase14->15 at6:35->6:40. */
    UWORD fleet_u[TD_TRAFFIC_SLOTS]={80*16,200*16,320*16,440*16,824*16,808*16,960*16,960*16};
    UWORD fleet_v[TD_TRAFFIC_SLOTS]={280*16,392*16,168*16,632*16,240*16,160*16,880*16,940*16};
    td_traffic_context_t context={fleet_u,fleet_v,&actors[9],extents,extents,560*16,720*16,0,0};
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    td.u=808*16;td.v=112*16;td.onfoot=1;td.mode=TD_ROAM;
    td.park_u=560*16;td.park_v=720*16;td.seconds=6;td.subsecond=35;
    td_streetcar_view_district=TD_DISTRICT_CITY;td_streetcar_ride_view=0;
    fixture_authored_routes=1;memset(fixture_routes,TD_NONE,sizeof(fixture_routes));fixture_routes[0]=19;
    hide_fleet_fixture();
    actors[7].flags=0;actors[7].pos.x=808*32;actors[7].pos.y=160*32;
    require(td_district_routes[0][19][0]==784&&td_district_routes[0][19][1]==148,
            "Bus crossing fixture uses the unchanged authored Core route19");
    td_people_reset();require(td_people_present(24)==0&&td_people[0].phase==14&&
            actors[9].pos.x==798*32&&actors[9].pos.y==148*32,
            "Actual world6:35 presents the preceding safe eastbound human phase14");
    require(td_traffic_admit(&context,0,6,5,808*16,160*16,808*16,152*16,0),
            "Actual bus admission accepts its coherent next sweep while the human is safely at798");
    fleet_v[5]=152*16;actors[7].pos.y=152*32;td.subsecond=40;
    td_state_t saved=td;fleet_check(td_people_present(24)==0,"The crossing guard does not invent an impact");
    fleet_check(td_people[0].phase==14&&td_people[0].lag==1&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&
                actors[9].pos.x==798*32&&actors[9].pos.y==148*32,
                "Attempted actual phase15 waits visibly at798 rather than entering the bus body at799");
    fleet_check(reference_fleet_clear(actors[9].pos.x>>5,actors[9].pos.y>>5,808*16,152*16,7),
                "The actual displayed human remains outside the full bus body");
    for(unsigned advance=0;advance<2;advance++){
        UWORD next=fleet_v[5]-8*16;
        UBYTE admitted=td_traffic_admit(&context,0,6,5,fleet_u[5],fleet_v[5],fleet_u[5],next,0);
        fleet_check(admitted,"The bus can clear the crossing instead of becoming mutually blocked");
        if(admitted){fleet_v[5]=next;actors[7].pos.y=next*2;}
        td.subsecond=45+advance*5;fleet_check(td_people_present(24)==0,"Waiting/resuming foot motion invents no impact");
    }
    fleet_check(actors[9].pos.x==799*32&&td_people[0].phase==15&&td_people[0].lag==2&&
                !(actors[9].flags&ACTOR_FLAG_HIDDEN),"The same phase15 resumes once the bus clears, without teleporting or hiding");
    saved.subsecond=td.subsecond;fleet_check(!memcmp(&td,&saved,sizeof(td)),
                "Crossing admission, waiting and resumption preserve all58 gameplay bytes");
    fixture_authored_routes=0;
    require(!unused_traffic_callbacks,"Pure actual traffic body/admission checks never consult the fail-closed hardware stubs");
    if(fleet_failures)fprintf(stderr,"Fleet extent regression: %u failures\n",fleet_failures);
    require(!fleet_failures,"Fleet extent and authored crossing safety regressions pass");
}

static uint32_t random_u32(void){random_state=random_state*1664525u+1013904223u;return random_state;}
static void fleet_bucket_differential(void){
    static const unsigned radii[8]={144,144,144,160,144,160,144,144};
    memset(&td,0,sizeof(td));td.mode=TD_ROAM;td_streetcar_view_district=0;
    td_streetcar_ride_view=0;td_streetcar_elapsed=0;
    for(unsigned n=0;n<400000;n++){
        UWORD u=(UWORD)(random_u32()>>8),v=(UWORD)(random_u32()>>8);
        if(n&1){u%=1024;v%=976;}
        uint16_t pu=(uint16_t)((uint32_t)u*16),pv=(uint16_t)((uint32_t)v*16);
        unsigned expected=1;
        for(unsigned i=0;i<8;i++){
            unsigned idx=fleet_actor_slots[i];
            actors[idx].pos.x=(UWORD)(random_u32()>>8);actors[idx].pos.y=(UWORD)(random_u32()>>8);
            actors[idx].flags=(n>>(i+1))&1?ACTOR_FLAG_HIDDEN:0;
            if(!(n&3)&&i==0){actors[idx].pos.x=(UWORD)((pu+(int)(n%513)-256)*2);actors[idx].pos.y=(UWORD)((pv+(int)((n*3)%513)-256)*2);}
            uint16_t cu=actors[idx].pos.x/2,cv=actors[idx].pos.y/2;
            int32_t dx=(int32_t)pu-cu,dy=(int32_t)pv-cv;
            if(!(actors[idx].flags&ACTOR_FLAG_HIDDEN)&&abs(dx)<(int)radii[i]&&abs(dy)<(int)radii[i])expected=0;
        }
        if(expected)expected=td_streetcar_runtime_pedestrian_clear(0,pu,pv);
        require(td_person_road_clear(u,v)==expected,
                "byte fleet broad phase matches independent signed Q4 distances for fractional actors, hidden bodies, malformed wrapped points and every radius");
    }
}
static unsigned distance16(uint16_t a,uint16_t b){return a>b?a-b:b-a;}
static int reference_contact(UWORD u,UWORD v){
    /* Wider host arithmetic computes the retained evenly sampled path without
     * using the new rectangle early-out or production distance helper. */
    int32_t du=(int16_t)(uint16_t)(td.u-td_people_last_u);
    int32_t dv=(int16_t)(uint16_t)(td.v-td_people_last_v);
    unsigned span=(unsigned)abs(du),other=(unsigned)abs(dv),steps;
    uint16_t target_u=(uint16_t)((uint32_t)u*16),target_v=(uint16_t)((uint32_t)v*16);
    if(other>span)span=other;
    if(!span||span>512)return 0;
    steps=(span+15)/16;
    for(unsigned n=0;n<=steps;n++){
        uint16_t x=(uint16_t)((int32_t)td_people_last_u+du*(int32_t)n/(int32_t)steps);
        uint16_t y=(uint16_t)((int32_t)td_people_last_v+dv*(int32_t)n/(int32_t)steps);
        if(distance16(x,target_u)<160&&distance16(y,target_v)<160)return 1;
    }
    return 0;
}

static void contacts(void){
    const int directions[8][2]={{1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,1},{1,-1},{-1,-1}};
    for(unsigned d=0;d<8;d++)for(int span=0;span<=513;span++){
        td_people_last_u=8192;td_people_last_v=7168;
        td.u=(UWORD)(8192+span*directions[d][0]);td.v=(UWORD)(7168+span*directions[d][1]);
        for(int ox=-9;ox<=9;ox++)for(int oy=-9;oy<=9;oy++){
            UWORD u=(UWORD)(512+ox),v=(UWORD)(448+oy);
            require(td_person_contact(u,v)==reference_contact(u,v),"Contact endpoints, exact radius and diagonal sample equivalence");
        }
    }
    for(unsigned n=0;n<100000;n++){
        int dx=(int)(random_u32()%1101)-550,dy=(int)(random_u32()%1101)-550;
        td_people_last_u=(UWORD)(1024+random_u32()%13000);
        td_people_last_v=(UWORD)(1024+random_u32()%12000);
        td.u=(UWORD)(td_people_last_u+dx);td.v=(UWORD)(td_people_last_v+dy);
        UWORD u=(UWORD)((td_people_last_u>>4)+(int)(random_u32()%101)-50);
        UWORD v=(UWORD)((td_people_last_v>>4)+(int)(random_u32()%101)-50);
        require(td_person_contact(u,v)==reference_contact(u,v),"Random fractional movement contact equivalence");
    }
    /* Public state validation prevents these in gameplay. Retain the original
     * helper behaviour rather than letting unsigned wrap create false rejects. */
    for(unsigned n=0;n<20000;n++){
        td_people_last_u=(UWORD)random_u32();td_people_last_v=(UWORD)random_u32();
        td.u=(UWORD)(td_people_last_u+(int)(random_u32()%1025)-512);
        td.v=(UWORD)(td_people_last_v+(int)(random_u32()%1025)-512);
        UWORD u=(UWORD)random_u32(),v=(UWORD)random_u32();
        require(td_person_contact(u,v)==reference_contact(u,v),"Malformed wrapping endpoints retain old contact oracle");
    }
    td_people_last_u=65100;td_people_last_v=2000;td.u=2;td.v=2000;
    require(td_person_contact(4095,125)==reference_contact(4095,125),"Short modulo wrap retains intermediate high coordinate contact");
}

static void phases(void){
    const UWORD seconds[]={0,1,2,3,7,15,31,32,10921,54611,65534,65535};
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    td.u=td.v=4096;td.onfoot=1;td.mode=TD_ROAM;td.park_district=4;
    hide_fleet_fixture();
    td_people_reset();
    for(unsigned s=0;s<sizeof(seconds)/sizeof(seconds[0]);s++)for(UBYTE sub=0;sub<60;sub++)
        for(unsigned group=0;group<32;group++){
            td.seconds=seconds[s];td.subsecond=sub;
            for(UBYTE i=0;i<8;i++)fixture_routes[i]=(UBYTE)(group*8+i);
            td_ped_refresh=1;
            /* This mathematical oracle samples fresh scene occupants. The
             * separate natural-arrival cases exercise subsequent admission. */
            td_people_boot=1;
            require(td_people_present(24)==0,"Phase-only stationary walker cannot invent impact");
            for(UBYTE i=0;i<8;i++){
                UBYTE route=fixture_routes[i];
                if(route==TD_NONE){require(actors[9+i].flags&ACTOR_FLAG_HIDDEN,"Missing route stays hidden");continue;}
                unsigned expected=(unsigned)(((uint64_t)td.seconds*12+sub/5+(uint64_t)route*37)%128);
                require(td_people[i].phase==expected,"Hoisted clock matches full old phase across seconds wrap");
                require(actors[9+i].pos.x==(100+i*90+(expected<64?expected:127-expected))*32&&
                        actors[9+i].pos.y==(100+i*80)*32,"Phase preserves exact authored-path presentation");
                require(variants[i]==route%4&&poses[i]==(expected<64?0:2)+1,"Direction, step and outfit unchanged");
            }
        }
}

static void loaded_rails(void){
    td_streetcar_pose_t pose;td_state_t before;
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    hide_fleet_fixture();
    td.mode=TD_RIDE;td_streetcar_ride_view=1;td_streetcar_elapsed=0;
    for(UWORD tick=0;tick<TD_STREETCAR_PERIOD_TICKS;tick++){
        td.seconds=tick/60;td.subsecond=tick%60;
        require(td_streetcar_pose(td.seconds,td.subsecond,&pose),"Real timetable supplies every autonomous rail sample");
        td.district=pose.district==0?1:0;td_streetcar_view_district=pose.district;
        before=td;
        require(!td_streetcar_runtime_pedestrian_clear(pose.district,pose.u,pose.v),"Loaded district rejects actual autonomous tram centre");
        require(!td_person_road_clear(pose.u>>4,pose.v>>4),"People pass actual loaded district rather than saved origin");
        require(td_streetcar_runtime_foot_clear(pose.u,pose.v),"Player query retains saved-origin semantics");
        require(!memcmp(&td,&before,sizeof(td)),"Rail queries never commit paid origin/save state");
    }
    /* A booked HOLD remains at directional destination doors even after
     * unrelated autonomous phases, and while help/pause/map show the ride. */
    for(UBYTE origin=43;origin<51;origin++)for(UBYTE target=43;target<51;target++){
        if(origin==target)continue;
        require(td_streetcar_destination(origin,target,&pose),"Directional booked destination is authored");
        td.transit_origin=origin;td.transit_target=target;td.reserved=TD_STREETCAR_HOLD;
        td.ride_left=1;td.district=pose.district==0?1:0;td_streetcar_view_district=pose.district;
        const UWORD clocks[]={0,14,31,63,65535};
        const UBYTE modes[]={TD_RIDE,TD_HELP,TD_PAUSE,TD_MAP};
        for(unsigned clock=0;clock<sizeof(clocks)/sizeof(clocks[0]);clock++)
            for(unsigned mode=0;mode<sizeof(modes)/sizeof(modes[0]);mode++){
                td.seconds=clocks[clock];td.subsecond=59;td.mode=modes[mode];td_resume_mode=TD_RIDE;before=td;
                require(!td_streetcar_runtime_pedestrian_clear(pose.district,pose.u,pose.v),"Held loaded destination stays solid through phase/menu/rollover");
                require(!td_person_road_clear(pose.u>>4,pose.v>>4),"People respect the visibly booked HOLD body");
                require(td_streetcar_runtime_pedestrian_clear(pose.district,pose.u+32*16,pose.v+32*16),"Foot centre outside held body remains clear");
                require(!memcmp(&td,&before,sizeof(td)),"Held query leaves origin/fare/58-byte record untouched");
            }
    }
    td.mode=TD_RIDE;td.reserved=0;td.subsecond=0;
    require(td_streetcar_runtime_pedestrian_clear(2,500*16,528*16)&&
            td_streetcar_runtime_pedestrian_clear(4,500*16,528*16),"No Queen body exists in HighPark/PortLands");
    require(!td_streetcar_runtime_pedestrian_clear(TD_DISTRICT_COUNT,500*16,528*16),"Invalid district fails closed");
    require(!td_streetcar_runtime_pedestrian_clear(0,47,500*16)&&
            !td_streetcar_runtime_pedestrian_clear(0,500*16,47)&&
            !td_streetcar_runtime_pedestrian_clear(0,1021*16,500*16),"Full6px foot bounds fail closed at map edge");
    td.subsecond=60;require(!td_streetcar_runtime_pedestrian_clear(0,500*16,400*16),"Invalid clock fails closed even away from rails");
}

static void parked_district_queries(void){
    const UBYTE queen_districts[]={TD_DISTRICT_CITY,TD_DISTRICT_WEST,TD_DISTRICT_EAST};
    const int offsets[][2]={{0,0},{8,8},{9,0},{0,-9}};
    td_state_t before;
    require(sizeof(td)==58,"The pedestrian correction retains the58-byte save state layout");
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    hide_fleet_fixture();
    td.mode=TD_RIDE;td_streetcar_ride_view=1;td_streetcar_elapsed=0;
    td.cash=237;td.health=81;td.wanted=2;td.wanted_left=21;
    for(unsigned fraction=0;fraction<16;fraction++)
    for(unsigned logical=0;logical<3;logical++)for(unsigned view=0;view<3;view++)
        for(UBYTE parked=0;parked<TD_DISTRICT_COUNT;parked++)for(UBYTE foot=0;foot<2;foot++)
            for(unsigned point=0;point<sizeof(offsets)/sizeof(offsets[0]);point++){
                UWORD u=(UWORD)(824+offsets[point][0]),v=(UWORD)(400+offsets[point][1]);
                td.district=queen_districts[logical];td_streetcar_view_district=queen_districts[view];
                td.park_district=parked;td.onfoot=foot;
                td.park_u=824*16+fraction;td.park_v=400*16+fraction;before=td;
                /* A conservative6px parked body plus3px human footprint.
                 * Positive9px is blocked at fractions1..15; negative9px
                 * remains clear. The reference uses signed body rectangles. */
                int occupied=foot&&parked==queen_districts[view]&&
                    !reference_fleet_clear(u,v,td.park_u,td.park_v,6);
                require(td_person_road_clear(u,v)==!occupied,
                        "Park occupancy uses loaded district and actual fractional Q4 boundaries");
                require(!memcmp(&td,&before,sizeof(td)),"Park query preserves every paid-trip/save byte");
            }
    td.onfoot=1;td.park_district=td_streetcar_view_district=TD_DISTRICT_CITY;
    actors[2].flags=0;actors[2].pos.x=824*32;actors[2].pos.y=400*32;
    td.park_district=TD_DISTRICT_WEST;
    require(!td_person_road_clear(824,400),"Ignoring a remote parked car does not bypass a visible fleet body");
    actors[2].flags=ACTOR_FLAG_HIDDEN;
}

static void paid_parked_presentation(void){
    td_streetcar_pose_t pose;td_state_t before;
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    hide_fleet_fixture();
    memset(fixture_routes,TD_NONE,sizeof(fixture_routes));fixture_authored_routes=1;
    td.mode=TD_RIDE;td.onfoot=1;td.district=TD_DISTRICT_CITY;
    td.u=116*16;td.v=556*16;td.park_district=TD_DISTRICT_CITY;
    td.park_u=824*16;td.park_v=556*16;
    td.transit_origin=44;td.transit_target=43;td.ride_left=1;td.reserved=TD_STREETCAR_HOLD;
    td.seconds=9;td.subsecond=10;td.cash=237;td.health=81;td.wanted=2;td.wanted_left=21;
    require(td_streetcar_destination(44,43,&pose)&&pose.district==TD_DISTRICT_WEST,
            "A real booked Queen HOLD supplies the remote West view");
    td_streetcar_ride_view=1;td_streetcar_view_district=pose.district;
    td_streetcar_focus_u=pose.u;td_streetcar_focus_v=pose.v;td_streetcar_elapsed=0;
    fixture_routes[0]=69;
    require(td_district_routes[TD_DISTRICT_WEST][69][0]==800&&
            td_district_routes[TD_DISTRICT_WEST][69][1]==556,"West fixture uses the registered sidewalk route");
    td_people_reset();before=td;
    require(td_people_present(24)==0,"Paid remote pedestrian presentation cannot charge an impact");
    require(!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&actors[9].pos.x==824*32&&actors[9].pos.y==556*32,
            "Core parked coordinates do not hide the real West pedestrian at that local point");
    require(td_people[0].phase==103&&td_people[0].lag==0&&poses[0]==3&&variants[0]==1,
            "Ignoring an origin car preserves West walk phase, step and outfit");
    require(!memcmp(&td,&before,sizeof(td)),"Remote visible pedestrian leaves all58 paid state bytes unchanged");
    td.subsecond=15;before=td;
    require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&actors[9].pos.x==823*32,
            "Remote pedestrian continues moving rather than accumulating false parked-car lag");
    require(!memcmp(&td,&before,sizeof(td)),"Remote continued movement preserves paid state");

    /* The ordinary47->43 approach provides this same mismatch without a
     * held arrival. Core Broadview parking944,548 is clear of the rails;
     * West's selected route70 walks at y556 within the old8px exclusion. */
    td.reserved=0;td.u=980*16;td.v=556*16;td.transit_origin=47;td.transit_target=43;
    td.seconds=239;td.subsecond=25;td.park_u=850*16;td.park_v=548*16;
    fixture_routes[0]=69;
    require(td_district_routes[TD_DISTRICT_WEST][69][0]==800&&
            td_district_routes[TD_DISTRICT_WEST][69][1]==556,"Ordinary paid fixture uses the registered West route70");
    td_people_reset();
    for(UBYTE sample=0;sample<7;sample++){
        td.subsecond=25+sample*5;
        require(td_streetcar_ride(47,43,1,239,td.subsecond,&pose)&&pose.district==TD_DISTRICT_WEST,
                "Every ordinary approach sample is an actual paid West pose");
        td_streetcar_focus_u=pose.u;td_streetcar_focus_v=pose.v;before=td;
        require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&
                actors[9].pos.x==(850+sample)*32&&actors[9].pos.y==556*32,
                "Ordinary paid view permits the moving West human through the origin-car exclusion");
        require(td_people[0].phase==50+sample&&td_people[0].lag==0&&poses[0]==1&&variants[0]==1,
                "Ordinary paid view retains every walk step without false yielding or stumble");
        require(!memcmp(&td,&before,sizeof(td)),"Ordinary paid-view presentation preserves every saved byte");
    }

    /* A car parked earlier in Core must remain occupancy while a West-origin
     * paid ride shows Core. This is a real moving pose, not a synthetic view. */
    td.reserved=0;td.district=TD_DISTRICT_WEST;td.u=836*16;td.v=556*16;
    td.transit_origin=43;td.transit_target=47;td.ride_left=8;td.seconds=56;td.subsecond=0;
    td.park_district=TD_DISTRICT_CITY;td.park_u=800*16;td.park_v=556*16;
    require(td_streetcar_ride(43,47,8,56,0,&pose)&&pose.district==TD_DISTRICT_CITY,
            "The actual paid Queen path supplies a logical-West/loaded-Core pose");
    td_streetcar_view_district=pose.district;td_streetcar_focus_u=pose.u;td_streetcar_focus_v=pose.v;
    fixture_routes[0]=68;
    require(td_district_routes[TD_DISTRICT_CITY][68][0]==784&&
            td_district_routes[TD_DISTRICT_CITY][68][1]==556,"Core fixture uses the registered sidewalk route");
    td_people_reset();before=td;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "A view-local parked car still blocks a Core pedestrian despite the saved West origin");
    require(td_people[0].phase==116,"Blocked presentation retains the actual Core route phase");
    require(!memcmp(&td,&before,sizeof(td)),"View-local blocked presentation preserves paid state");
    td.park_district=TD_DISTRICT_WEST;td.subsecond=5;before=td;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "Clearing view-local occupancy cannot rematerialise a hidden human in the viewport");
    require(!memcmp(&td,&before,sizeof(td)),"Resumed Core presentation preserves paid state");
    fixture_authored_routes=0;
}

static void appearance_case(UWORD u,UWORD v){
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    memset(fixture_routes,TD_NONE,sizeof(fixture_routes));fixture_routes[0]=19;
    fixture_authored_routes=1;td.mode=TD_ROAM;td.job=TD_NONE;
    td.u=u;td.v=v;td.speed=3;td.seconds=8;td.subsecond=10;
    td.cash=237;td.health=81;td.wanted=2;td.wanted_left=21;
    td_streetcar_ride_view=0;td_streetcar_view_district=TD_DISTRICT_CITY;
    td_streetcar_elapsed=0;
    hide_fleet_fixture();
    actors[9].flags=ACTOR_FLAG_HIDDEN|0x40;
    td_people_reset();
}

static void appearance_admission(void){
    td_state_t before;
    require(td_district_routes[0][19][0]==784&&td_district_routes[0][19][1]==148,
            "Appearance fixtures use actual Core crossing route19");
    appearance_case(828*16,148*16);before=td;
    require(!td_people_present(24)&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&actors[9].pos.x==817*32,
            "Scene initialization seeds an already-present clear resident, with no empty city wait");
    require(td_people_route_visible(19)&&!td_people_route_visible(26),
            "Actual visible identity is pinned while unrelated identities remain recyclable");
    require(!memcmp(&td,&before,sizeof(td)),"Initial residents leave every saved gameplay byte unchanged");

    /* A hidden identity must not pop back into an already painted view simply
     * because road/courier occupancy cleared. Arm it only after the camera
     * leaves its whole silhouette, then pan back over a continuous world path. */
    appearance_case(824*16,147*16+7);
    actors[7].flags=0;actors[7].pos.x=808*32;actors[7].pos.y=152*32;
    require(!td_people_present(24)&&(actors[9].flags&ACTOR_FLAG_HIDDEN),"A bus-occupied initial person stays hidden");
    actors[7].flags=ACTOR_FLAG_HIDDEN;td.u=833*16;td.subsecond=15;
    require(!td_people_present(25)&&(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun,
            "Clearing a blocker cannot create a fresh identity inside the viewport");
    before=td;td.u=600*16;
    require(!td_people_present(26)&&(td_people[0].state&TD_PERSON_ADMITTED)&&
            (actors[9].flags&ACTOR_FLAG_HIDDEN),"The same route arms only once its complete sprite is offscreen");
    td.u=770*16;
    require(!td_people_present(27)&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&actors[9].pos.x==818*32,
            "Camera travel reveals the armed pedestrian on its unchanged world path");
    before.u=td.u;require(!memcmp(&td,&before,sizeof(td)),"Natural reveal preserves all58 saved bytes");

    appearance_case(828*16,148*16);td_people_present(24);
    fixture_routes[0]=26;td_ped_refresh=1;td.v=204*16;before=td;
    require(!td_people_present(25)&&(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun,
            "Recycled route cannot inherit the old resident's visible admission");
    require(!memcmp(&td,&before,sizeof(td)),"Route recycling creates no retrospective collision or saved consequences");

    /* Boundary oracle remains independent signed rectangles, including every
     * Q4 fraction around11px and every occupied road-vehicle selection. */
    for(UBYTE vehicle=0;vehicle<4;vehicle++)for(UBYTE axis=0;axis<2;axis++)
        for(int sign=-1;sign<=1;sign+=2)for(int gap=144;gap<176;gap++){
            UWORD cu=(UWORD)(817*16+(axis?0:sign*gap));
            UWORD cv=(UWORD)(148*16+(axis?sign*gap:0));
            appearance_case(cu,cv);td.vehicle=vehicle;before=td;
            int clear=reference_fleet_clear(817,148,cu,cv,8);
            require(!td_people_present(24),"Fractional initial admission never retroactively charges a hit");
            require(!!(actors[9].flags&ACTOR_FLAG_HIDDEN)==!clear,
                    "Initial occupied-body admission retains exact independent Q4 rectangles");
            require(!td_people[0].stun&&!memcmp(&td,&before,sizeof(td))&&(actors[9].flags&0x40),
                    "Occupied-body admission preserves death state, all58 saved bytes and unrelated actor bits");
        }
    /* The existing independent sweep oracle above covers contact at160Q4.
     * This directly samples the separate176Q4 first-appearance edge. */
    for(UBYTE axis=0;axis<2;axis++)for(int sign=-1;sign<=1;sign+=2)
        for(int gap=175;gap<=177;gap++){
            UWORD su=(UWORD)(817*16+(axis?sign*gap:-12*16));
            UWORD sv=(UWORD)(148*16+(axis?-12*16:sign*gap));
            UWORD eu=(UWORD)(817*16+(axis?sign*gap:12*16));
            UWORD ev=(UWORD)(148*16+(axis?12*16:sign*gap));
            appearance_case(su,sv);td.u=eu;td.v=ev;
            require(td_person_sweep(817,148,176)==(gap<176),
                    "Independent tangential176Q4 appearance sweep retains both axes and signs");
        }

    appearance_case(828*16,148*16);td_people_present(24);td.u=825*16;
    require(td_people_present(25)==1&&td_people[0].stun==6&&poses[0]==TD_CIVILIAN_HIT,
            "A genuinely established resident still receives one real swept impact");
    for(UBYTE tick=26;tick<=49;tick++)require(!td_people_present(tick),"Established impact never repeats a fine");
    require(poses[0]==TD_CIVILIAN_PRONE&&td_people[0].recover==24,
            "Natural identity admission preserves the existing airborne then prone sequence");
    fixture_authored_routes=0;
}

static void idle_courier_case(void){
    /* Preserve the six source-derived Core cold poses. The added cars use
     * representative valid far positions; these are fixture geometry, not
     * claims about their authored route starts. Route19 stays clear of all8. */
    static const UWORD cu[TD_TRAFFIC_SLOTS]={80,200,320,440,824,216,960,960};
    static const UWORD cv[TD_TRAFFIC_SLOTS]={280,392,168,632,240,72,880,940};
    appearance_case(828*16,147*16+7);
    td.speed=0;td.park_u=560*16;td.park_v=720*16;
    for(UBYTE i=0;i<TD_TRAFFIC_SLOTS;i++){
        UBYTE actor_index=fleet_actor_slots[i];
        actors[actor_index].flags=0;actors[actor_index].pos.x=cu[i]*32;actors[actor_index].pos.y=cv[i]*32;
    }
}
static void idle_courier_boundaries(void){
    static const int speeds[]={-6,-3,-2,0,2,3,24};
    for(UBYTE vehicle=0;vehicle<4;vehicle++)for(UBYTE axis=0;axis<2;axis++)
        for(int sign=-1;sign<=1;sign+=2)for(int gap=168;gap<=184;gap++)
            for(unsigned n=0;n<sizeof(speeds)/sizeof(speeds[0]);n++){
            idle_courier_case();td.vehicle=vehicle;td.speed=speeds[n];
            td.u=(UWORD)(817*16+(axis?0:sign*gap));
            td.v=(UWORD)(148*16+(axis?sign*gap:0));
            td_state_t before=td;
            int clear=reference_fleet_clear(817,148,td.u,td.v,8);
            require(td_person_courier_blocks(817,148)==!clear,
                    "The proposed-step guard matches independent fractional maximum-body rectangles at every tested speed");
            require(!memcmp(&td,&before,sizeof(td)),"Courier yielding guard preserves all58 saved bytes");
        }
}
static void idle_courier_wait_and_resume(void){
    for(int speed=-2;speed<=2;speed++){
        idle_courier_case();td.speed=speed;
        require(td_people_present(24)==0&&actors[9].pos.x==817*32&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
                "The actual Core route19 starts visible at its strict10px clear edge");
        for(unsigned step=1;step<=128;step++){
            unsigned clock=8*60+10+5*step;td.seconds=clock/60;td.subsecond=clock%60;
            td_state_t before=td;
            require(td_people_present(24)==0&&actors[9].pos.x==817*32&&
                    !(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun,
                    "Visible pedestrians wait through a full raw-phase wrap instead of entering the slow courier");
            require(reference_fleet_clear(817,148,td.u,td.v,7)&&(actors[9].flags&0x40),
                    "Waiting keeps the full body clear and unrelated actor bits intact");
            require(!memcmp(&td,&before,sizeof(td)),"Waiting invents no cash/cargo/heat/save mutation");
        }
        td.u=839*16;td.subsecond=55;
        require(td_people_present(24)==0&&actors[9].pos.x==818*32&&
                !(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun,
                "The same human immediately resumes one ordinary route step after body clearance");
    }
}
static void idle_courier_impacts_and_recovery(void){
    static const int speeds[]={-6,-3,3,12,24};
    for(unsigned n=0;n<sizeof(speeds)/sizeof(speeds[0]);n++){
        idle_courier_case();td.speed=speeds[n];td_people_present(24);
        td.u=825*16;td.v=148*16+3;
        require(td_people_present(24)==1&&td_people[0].stun==6&&poses[0]==TD_CIVILIAN_HIT,
                "Moving car sweeps start one non-graphic flying pose");
        td.speed=0;
        for(UBYTE tick=25;tick<=48;tick++)require(!td_people_present(tick),"The animation does not repeat a penalty");
        require(poses[0]==TD_CIVILIAN_PRONE&&td_people[0].stun==6,"The person stays down after the short flight");
        require(actors[9].pos.x==805*32&&actors[9].pos.y==160*32,
                "Actual movement direction displaces the body twelve pixels per axis");
        UWORD x=actors[9].pos.x,y=actors[9].pos.y;
        for(unsigned second=0;second<60;second++){td.seconds++;td_people_second();}
        td_people_present(49);
        require(actors[9].pos.x==x&&actors[9].pos.y==y&&poses[0]==TD_CIVILIAN_PRONE,
                "A corpse never resumes its walking route within the loaded scene");
        fixture_routes[0]=20;td_ped_refresh=1;td_people_present(50);
        fixture_routes[0]=19;td_ped_refresh=1;td_people_present(51);
        require(actors[9].flags&ACTOR_FLAG_HIDDEN,"Replacing a nearby slot cannot respawn the dead route identity");
        td_people_reset();td.u=850*16;
        require(!td_people_present(0)&&!td_people[0].stun,"District reset discards only transient route death state");
    }
}

static void idle_courier_context_controls(void){
    for(UBYTE context=0;context<3;context++){
        idle_courier_case();td_people_present(24);
        if(context==0)td.onfoot=1;
        else if(context==1){td_streetcar_ride_view=1;td_streetcar_focus_u=td.u;td_streetcar_focus_v=td.v;td.mode=TD_RIDE;}
        else td_streetcar_view_district=TD_DISTRICT_WEST;
        td.subsecond=25;td_state_t before=td;
        require(td_people_present(24)==0&&actors[9].pos.x==820*32&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
                "Walking, remote paid focus and foreign loaded views retain their original human motion");
        require(!memcmp(&td,&before,sizeof(td)),"Nonlocal/foot controls preserve complete saved state");
    }
    idle_courier_case();td.u=829*16;td_people_present(24);td.subsecond=15;
    require(td_people_present(24)==0&&actors[9].pos.x==818*32&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "An exact11px ordinary step is admitted immediately without a grace timer");
}
static void idle_courier_resume_regression(void){
    static const int held_speeds[]={-2,0,2};
    for(unsigned n=0;n<sizeof(held_speeds)/sizeof(held_speeds[0]);n++){
        idle_courier_case();td.speed=held_speeds[n];
        require(td_people_present(24)==0&&actors[9].pos.x==817*32&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
                "The stationary/slow resume regression begins with a genuinely visible human");
        td.subsecond=25;td_state_t before=td;
        require(td_people_present(24)==0&&actors[9].pos.x==817*32&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&
                reference_fleet_clear(817,148,td.u,td.v,7),
                "A15-VBlank idle interval cannot let a visible walker enter the stopped/slow car body");
        require(!memcmp(&td,&before,sizeof(td)),"An idle walker creates no gameplay penalty");
        td.v+=12;td.speed=3;before=td;
        require(td_people_present(24)==0&&!td_people[0].stun&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
                "The first0.75px resume cannot charge the driver's prior stationary wait as a swept human impact");
        require(!memcmp(&td,&before,sizeof(td)),"A clear resumption leaves all58 saved bytes unchanged");
    }
    /* Original fallback still respects a real bus hull. Do not keep a human
     * visible inside road traffic merely because the courier blocked its
     * proposed step. Both bus poses are on Core's actual northbound lane. */
    idle_courier_case();actors[7].pos.x=808*32;actors[7].pos.y=160*32;
    require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "A coherent northbound bus starts clear of the visible human");
    actors[7].pos.y=152*32;td.subsecond=15;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun,
            "Road-occupied old positions retain the original hidden fallback without an invented impact");
}

static void reversing_courier_route83(void){
    idle_courier_case();fixture_routes[0]=83;
    td.u=566*16;td.v=695*16+3;td.heading=12;
    td.cash=138;td.health=100;td.wanted=td.wanted_left=0;
    td.seconds=24;td.subsecond=22;td_people_reset();
    require(td_district_routes[0][83][0]==520&&td_district_routes[0][83][1]==692,
            "The reverse-start fixture uses the exact registered Bay route83");
    require(td_people_present(24)==0&&td_people[0].phase==35&&actors[9].pos.x==555*32&&
            !(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "Route83 is genuinely visible at the native24:22 approach phase35");
    for(unsigned clock=24*60+25;clock<=26*60;clock+=5){
        td.seconds=clock/60;td.subsecond=clock%60;
        require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
                "The native approach clock advances while the visible human waits beside the occupied car");
    }
    require(td_people[0].phase==35&&td_people[0].lag==20&&actors[9].pos.x==555*32,
            "At26:00 the actual route/clock preserves native phase35 and accumulated lag19");
    td.v=697*16+12;td.speed=-6;td.subsecond=16;
    require(!reference_contact(555,692)&&reference_contact(559,692),
            "The independent8px sweep clears the original human but intersects the newly advanced phase39 human");
    td_state_t before=td;UBYTE hits=td_people_present(24);
    if(hits||td_people[0].phase!=35)fprintf(stderr,
      "route83 reverse diagnostic: phase=%u lag=%u human=%u,%u stun=%u hits=%u\n",
      td_people[0].phase,td_people[0].lag,actors[9].pos.x>>5,actors[9].pos.y>>5,td_people[0].stun,hits);
    require(!hits&&td_people[0].phase==35&&actors[9].pos.x==555*32&&
            !td_people[0].stun&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "A visible phase35 human cannot walk into the still-occupied reverse-start body at phase39");
    require(!memcmp(&td,&before,sizeof(td)),
            "A clear reverse start cannot invent a cash/cargo/attention penalty");
}

static void camera_admission_bounds(void){
    const unsigned focus[]={0,79,80,81,87,88,89,500,919,920,921,943,944,945,1024};
    memset(&td,0,sizeof(td));td_streetcar_ride_view=0;
    for(unsigned x=0;x<sizeof(focus)/sizeof(*focus);x++)for(unsigned y=0;y<sizeof(focus)/sizeof(*focus);y++){
        td.u=focus[x]*16;td.v=focus[y]*16;
        int left=focus[x]<80?0:focus[x]>944?864:(int)focus[x]-80;
        int top=focus[y]<88?0:focus[y]>920?832:(int)focus[y]-88;
        const int dx[]={-14,-13,-12,0,80,160,172,173,174};
        const int dy[]={-14,-13,-12,0,72,144,164,165,166};
        for(unsigned a=0;a<sizeof(dx)/sizeof(*dx);a++)for(unsigned b=0;b<sizeof(dy)/sizeof(*dy);b++){
            int u=left+dx[a],v=top+dy[b];
            if(u<0||v<0||u>1024||v>976)continue;
            /* Independent inclusive silhouette rectangles, accounting for
             * the entire small sheet and the camera's eight-pixel deadzone. */
            int intersects=!(u+12<left||u-12>left+160||v+12<top||v-20>top+144);
            td_state_t before=td;
            require(td_person_in_view((UWORD)u,(UWORD)v)==intersects,
                    "Admission intersects whole silhouette rectangles at every clamped camera edge");
            require(!memcmp(&td,&before,sizeof(td)),"Admission-camera query never changes the58-byte save state");
        }
    }
}
static void social_pairs(void){
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    memset(fixture_routes,TD_NONE,sizeof(fixture_routes));fixture_routes[0]=0;fixture_routes[1]=1;
    fixture_custom_routes=1;fixture_authored_routes=0;
    fixture_points[0][0]=100;fixture_points[1][0]=105;
    fixture_points[0][1]=fixture_points[1][1]=100;
    td.mode=TD_ROAM;td.onfoot=1;td.job=TD_NONE;td.u=120*16;td.v=120*16;td.park_district=4;
    td_streetcar_ride_view=0;td_streetcar_view_district=0;td_streetcar_elapsed=0;
    hide_fleet_fixture();
    td_people_reset();td_state_t saved=td;
    require(!td_people_present(0)&&
            (td_people[0].state&TD_PERSON_SOCIAL)==TD_PERSON_APPROACH&&
            (td_people[1].state&TD_PERSON_SOCIAL)==TD_PERSON_APPROACH,
            "Two real displayed identities establish a reciprocal approach without creating another actor");
    UWORD last_u[2]={actors[9].pos.x>>5,actors[10].pos.x>>5};
    unsigned talked=0,gestured=0,returned=0;
    UWORD talking_u[2]={0,0};
    for(unsigned tick=5;tick<=540;tick+=5){
        td.seconds=tick/60;td.subsecond=tick%60;
        require(!td_people_present((UBYTE)tick),"Ordinary social motion invents no impact or police consequence");
        for(unsigned i=0;i<2;i++){
            UWORD u=actors[9+i].pos.x>>5,v=actors[9+i].pos.y>>5;
            require(td_people_distance(last_u[i],u)<=1&&v==100&&!(actors[9+i].flags&ACTOR_FLAG_HIDDEN),
                    "Approach and rejoin use visible one-pixel land motion rather than route teleports");
            last_u[i]=u;
        }
        if((td_people[0].state&TD_PERSON_SOCIAL)==TD_PERSON_TALK&&
           (td_people[1].state&TD_PERSON_SOCIAL)==TD_PERSON_TALK){
            if(!talked){talking_u[0]=last_u[0];talking_u[1]=last_u[1];}
            talked++;
            require(last_u[0]==talking_u[0]&&last_u[1]==talking_u[1],
                    "Conversation visibly pauses both participants at their meeting place");
            require(poses[0]<2&&poses[1]>=2&&poses[1]<4,
                    "Conversation poses face the actual partner while preserving original civilian frames");
            gestured|=1u<<(poses[0]&1);
        }
        if(talked&&!(td_people[0].state&TD_PERSON_SOCIAL)&&!(td_people[1].state&TD_PERSON_SOCIAL)){
            returned=1;break;
        }
        saved.seconds=td.seconds;saved.subsecond=td.subsecond;
        require(!memcmp(&td,&saved,sizeof(td)),"Pair approach/talk/rejoin changes no saved gameplay byte");
    }
    require(talked>=20&&gestured==3&&returned,"Pairs approach, stop, alternate gestures and visibly rejoin their original paths");
    require(!td_people[0].ox&&!td_people[0].oy&&!td_people[1].ox&&!td_people[1].oy,
            "Completed conversation clears its transient path offsets");
    fixture_custom_routes=0;
}

static void eight_visible_people(void){
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));fixture_custom_routes=1;fixture_authored_routes=0;
    td.mode=TD_ROAM;td.onfoot=1;td.job=TD_NONE;td.u=160*16;td.v=120*16;td.park_district=4;
    td_streetcar_ride_view=0;td_streetcar_view_district=0;td_streetcar_elapsed=0;
    for(unsigned i=0;i<TD_PEOPLE_COUNT;i++){
        unsigned phase=(i*37)&127;
        fixture_routes[i]=i;fixture_points[i][0]=100+i*12-(phase<64?phase:127-phase);
        fixture_points[i][1]=100+i*6;
    }
    hide_fleet_fixture();
    td_people_reset();td_state_t before=td;
    require(!td_people_present(0),"Eight clear starting residents create no artificial impact");
    for(unsigned i=0;i<TD_PEOPLE_COUNT;i++)require(!(actors[9+i].flags&ACTOR_FLAG_HIDDEN)&&
        actors[9+i].pos.x==(100+i*12)*32&&actors[9+i].pos.y==(100+i*6)*32,
        "All eight actual pedestrian actors publish their own continuous world position");
    require(!memcmp(&td,&before,sizeof(td)),"Expanded resident presentation leaves all58 serialized bytes intact");
    UWORD fleet_u[8]={184*16,300*16,400*16,500*16,600*16,700*16,800*16,900*16};
    UWORD fleet_v[8]={150*16,800*16,800*16,800*16,800*16,800*16,800*16,800*16};
    UBYTE halves[8]={5,5,5,5,5,5,5,5};
    td_traffic_context_t context={fleet_u,fleet_v,&actors[9],halves,halves,0,0,0,0};
    for(unsigned i=0;i<TD_PEOPLE_COUNT-1;i++)actors[9+i].flags|=ACTOR_FLAG_HIDDEN;
    require(!td_traffic_motion_clear(&context,0,184*16,150*16,184*16,150*16-8,0),
            "Actual traffic clearance includes eighth pedestrian slot instead of driving through it");
    fixture_custom_routes=0;
}

static void actual_route_retention(void){
    UBYTE identities[TD_PEOPLE_COUNT];UWORD points[TD_PEOPLE_COUNT][2];
    appearance_case(438*16,692*16);td.onfoot=1;td.seconds=0;td.subsecond=5;fixture_routes[0]=83;
    td_people_reset();require(!td_people_present(0)&&actors[9].pos.x==520*32&&
        !(actors[9].flags&ACTOR_FLAG_HIDDEN),"Actual Core route83 begins at a partly visible eastern screen edge");
    memset(identities,TD_NONE,sizeof(identities));memset(points,0,sizeof(points));
    identities[0]=83;points[0][0]=520;points[0][1]=692;
    td.seconds=5;td.subsecond=20;
    require(td_route_position(83,520,(td.seconds*12+td.subsecond/5)&127)==583&&distance16(438,583)>144,
            "The fixture genuinely crosses the old nearest-route recycling threshold");
    td_state_t before=td;td_test_refresh_routes(identities,points);
    require(identities[0]==83&&points[0][0]==520&&points[0][1]==692,
            "Production nearest-route selection pins the actual visible lagged identity despite raw-clock distance");
    for(unsigned i=1;i<TD_PEOPLE_COUNT;i++)require(identities[i]!=83,
            "Pinned route stays unique among all eight candidate slots");
    require(!memcmp(&td,&before,sizeof(td)),"Actual selection preserves every saved byte");
    actors[9].flags|=ACTOR_FLAG_HIDDEN;td.u=300*16;td_test_refresh_routes(identities,points);
    require(identities[0]!=83,"An actually hidden distant route becomes eligible for normal offscreen replacement");
    fixture_authored_routes=0;
}

/* Independent complete scan: wide arithmetic for route-clock position,
 * original identity order and strict tie replacement, plus actual visible
 * identity pinning. No production binary bound or distance helper is used. */
static unsigned long reference_route_candidates;
static UWORD reference_route_u(UBYTE identity,UWORD start){
    unsigned phase=((uint32_t)td.seconds*12+td.subsecond/5+(unsigned)identity*37)%128;
    return (UWORD)(start+(phase<64?phase:127-phase));
}
static void reference_route_selector(UBYTE *ids,UWORD (*points)[2]){
    unsigned pu=(td_streetcar_ride_view?td_streetcar_focus_u:td.u)/16;
    unsigned pv=(td_streetcar_ride_view?td_streetcar_focus_v:td.v)/16;
    UBYTE district=td_streetcar_ride_view?td_streetcar_view_district:td.district;
    if(district>=TD_DISTRICT_COUNT){for(unsigned i=0;i<TD_PEOPLE_COUNT;i++){ ids[i]=TD_NONE; }return;}
    for(unsigned i=0;i<TD_PEOPLE_COUNT;i++){
        unsigned id=ids[i];
        if(id<td_route_counts[district]&&(td_people_route_visible(id)||
           (distance16(pu,reference_route_u(id,points[i][0]))<144&&distance16(pv,points[i][1])<112)))continue;
        ids[i]=TD_NONE;unsigned best=65535;
        for(unsigned j=0;j<td_route_counts[district];j++){
            reference_route_candidates++;
            unsigned x=td_district_routes[district][j][0],y=td_district_routes[district][j][1];
            if(distance16(pu,x+32)>176||distance16(pv,y)>112)continue;
            unsigned k;for(k=0;k<TD_PEOPLE_COUNT;k++)if(ids[k]==j)break;
            if(k<TD_PEOPLE_COUNT)continue;
            unsigned u=reference_route_u(j,x),score=distance16(pu,u)+distance16(pv,y);
            if(score<best){best=score;ids[i]=j;points[i][0]=x;points[i][1]=y;}
        }
    }
}
static void route_selector_case(UWORD pu,UWORD pv,UBYTE district,UWORD seconds,UBYTE sub,UBYTE view,UBYTE pattern){
    UBYTE actual[TD_PEOPLE_COUNT],expected[TD_PEOPLE_COUNT];UWORD a[TD_PEOPLE_COUNT][2],b[TD_PEOPLE_COUNT][2];
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));memset(td_people,0,sizeof(td_people));
    td.u=pu;td.v=pv;td.district=district;td.seconds=seconds;td.subsecond=sub;
    td_streetcar_ride_view=view;td_streetcar_view_district=district;
    td_streetcar_focus_u=pu;td_streetcar_focus_v=pv;
    for(unsigned i=0;i<TD_PEOPLE_COUNT;i++){
        unsigned count=district<TD_DISTRICT_COUNT?td_route_counts[district]:128;
        actual[i]=pattern==0?TD_NONE:pattern==1?(i*17+seconds)%count:pattern==2?0:(i&1?254:(i*37)%count);
        a[i][0]=pattern==3?(UWORD)(65535-i*37):actual[i]<count&&district<TD_DISTRICT_COUNT?td_district_routes[district][actual[i]][0]:0;
        a[i][1]=pattern==3?(UWORD)(32768+i*127):actual[i]<count&&district<TD_DISTRICT_COUNT?td_district_routes[district][actual[i]][1]:0;
        td_people[i].route=pattern==1?actual[i]:TD_NONE;
        actors[9+i].flags=pattern==1&&!(i&1)?0:ACTOR_FLAG_HIDDEN;
        actors[9+i].pos.x=(UWORD)((pu/16+(int)i*4)*32);actors[9+i].pos.y=(UWORD)((pv/16)*32);
    }
    memcpy(expected,actual,sizeof(actual));memcpy(b,a,sizeof(a));
    td_state_t saved=td;actor_t saved_actors[22];td_person_t saved_people[TD_PEOPLE_COUNT];
    memcpy(saved_actors,actors,sizeof(actors));memcpy(saved_people,td_people,sizeof(td_people));
    host_route_candidates=reference_route_candidates=0;
    reference_route_selector(expected,b);td_test_refresh_routes(actual,a);
    require(!memcmp(actual,expected,sizeof(actual))&&!memcmp(a,b,sizeof(a)),
            "Sorted-Y and hoisted-clock selector preserves all eight IDs and coordinates of the independent complete scan, including ties and duplicate reservation order");
    require(!memcmp(&td,&saved,sizeof(td))&&!memcmp(actors,saved_actors,sizeof(actors))&&!memcmp(td_people,saved_people,sizeof(td_people)),
            "Route bounds and clock reuse do not change any saved byte, visible actor or pedestrian transient state");
    require(host_route_candidates<=reference_route_candidates,
            "Bounded selector never scans more identities than the original whole-district search");
}
static void route_selector_differential(void){
    static const unsigned clocks[]={0,1,5,10,59,60,65535};
    for(unsigned d=0;d<TD_DISTRICT_COUNT;d++){
        for(unsigned i=1;i<td_route_counts[d];i++)require(td_district_routes[d][i-1][1]<=td_district_routes[d][i][1],
                                                        "Every actual registered route prefix is sorted by Y before binary bounds are permitted");
        for(unsigned y=0;y<976;y+=16)for(unsigned x=0;x<1024;x+=32)
        for(unsigned c=0;c<sizeof(clocks)/sizeof(clocks[0]);c++)for(unsigned pattern=0;pattern<4;pattern++)
            route_selector_case(x*16,y*16,d,clocks[c],(x+y+c)%60,c&1,pattern);
        /* Every exact inclusive Y-window boundary and all127 possible folded
         * clock phases check absent rows, first/last rows and equal scores. */
        for(unsigned r=0;r<td_route_counts[d];r++)for(int edge=-113;edge<=113;edge+=1)
        if(edge<=-111||edge>=111){
            int y=(int)td_district_routes[d][r][1]+edge;
            for(unsigned phase=0;phase<128;phase+=7)
                route_selector_case((td_district_routes[d][r][0]+32)*16,(UWORD)(y*16),d,phase/12,(phase%12)*5,phase&1,r%4);
        }
    }
    static const UWORD invalid[]={0,1,127,128,16383,16384,32767,32768,65534,65535};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++)for(unsigned j=0;j<sizeof(invalid)/sizeof(invalid[0]);j++)
    for(unsigned d=0;d<9;d++)for(unsigned pattern=0;pattern<4;pattern++)
        route_selector_case(invalid[i],invalid[j],d<8?d:255,65535,255,i&1,pattern);
    route_selector_case(560*16,720*16,0,0,0,0,0);
    require(host_route_candidates<reference_route_candidates/2,
            "Union cold selection demonstrably skips more than half the full-district identity visits without reducing people");
    printf("Route candidate visits at Union: %lu -> %lu, exact IDs/coordinates unchanged\n",reference_route_candidates,host_route_candidates);
}
static void byte_first_word_work(void){
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));td.mode=TD_ROAM;
    td_streetcar_view_district=5;td_streetcar_ride_view=0;td_streetcar_elapsed=0;
    for(unsigned i=0;i<TD_TRAFFIC_SLOTS;i++){
        actor_t *car=&actors[fleet_actor_slots[i]];car->flags=0;car->pos.x=(1400+i*48)*32;car->pos.y=900*32;
    }
    host_people_word_pairs=0;
    require(td_person_road_clear(500,400)&&!host_people_word_pairs,
            "Eight distant visible cars require no full Q4 coordinate pair materialisation");
    actors[2].pos.x=500*32+1;actors[2].pos.y=400*32+1;host_people_word_pairs=0;
    require(!td_person_road_clear(500,400)&&host_people_word_pairs==1,
            "The same nearest fractional body still materialises and checks its exact Q4 coordinates once");
    actors[2].flags=ACTOR_FLAG_HIDDEN;host_people_word_pairs=0;
    require(td_person_road_clear(500,400)&&!host_people_word_pairs,
            "Hidden nearby actors and distant visible actors retain exact flag semantics without word work");
}
static void attention_observation_checks(void){
    td_people_reset();td.wanted=3;td.wanted_left=1;td.cash=121;td.job=4;td.health=82;
    police_observed=1;
    for(unsigned second=0;second<90;second++)
        require(!td_people_second()&&td.wanted==3&&td.wanted_left==30,
                "Actual observed attention cannot expire on an independent magic timer");
    police_observed=0;
    for(unsigned star=3;star;star--){
        for(unsigned second=0;second<29;second++)
            require(!td_people_second()&&td.wanted==star&&td.wanted_left==29-second,
                    "Unobserved attention counts thirty active seconds without prematurely clearing a star");
        require(td_people_second()&&td.wanted==star-1&&td.wanted_left==(star>1?30:0),
                "Genuine evasion cools exactly one persistent star after the complete interval");
    }
    require(td.cash==121&&td.job==4&&td.health==82,"Attention countdown changes no cash/cargo/active-job state");
}
int main(void){social_body_terrain_differential();fleet_bucket_differential();attention_observation_checks();fleet_extent_guards();contacts();phases();loaded_rails();parked_district_queries();paid_parked_presentation();appearance_admission();idle_courier_boundaries();idle_courier_wait_and_resume();idle_courier_impacts_and_recovery();idle_courier_context_controls();idle_courier_resume_regression();reversing_courier_route83();camera_admission_bounds();social_pairs();eight_visible_people();actual_route_retention();route_selector_differential();byte_first_word_work();printf("People hotspot harness: %lu checks, 0 failures\n",checks);return 0;}
