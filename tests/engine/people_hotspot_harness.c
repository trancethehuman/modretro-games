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
actor_t actors[21];
UBYTE td_streetcar_ride_view,td_streetcar_view_district,td_resume_mode;
UWORD td_streetcar_focus_u,td_streetcar_focus_v;
static UWORD td_streetcar_elapsed;
static UBYTE fixture_routes[6],poses[6],variants[6];
static UBYTE fixture_authored_routes;
static unsigned long checks;
static uint32_t random_state=0x479cd013;
static void require(int condition,const char *message){
    checks++;
    if(!condition){fprintf(stderr,"FAIL %lu: %s\n",checks,message);exit(1);}
}
void td_refresh_routes(UBYTE *routes,UWORD (*nearby)[2]){
    UBYTE district=td_streetcar_ride_view?td_streetcar_view_district:td.district;
    for(UBYTE i=0;i<6;i++){
        routes[i]=fixture_routes[i];
        if(fixture_authored_routes&&routes[i]!=TD_NONE){
            require(district<TD_DISTRICT_COUNT&&routes[i]<td_route_counts[district],"Authored fixture selects an actual loaded route");
            nearby[i][0]=td_district_routes[district][routes[i]][0];
            nearby[i][1]=td_district_routes[district][routes[i]][1];
        }else{nearby[i][0]=100+i*90;nearby[i][1]=100+i*80;}
    }
}
void td_save(void){}
#include "rail_queries_under_test.c"
void td_civilian_present(actor_t *actor,UBYTE variant,UBYTE pose){
    ptrdiff_t index=actor-&actors[9];require(index>=0&&index<6,"Civilian actor slot remains bounded");
    variants[index]=variant;poses[index]=pose;
}
#include "people_under_test.c"
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
    static const UBYTE extents[6]={5,6,5,7,6,7};
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    td.mode=TD_ROAM;td.onfoot=1;td.park_u=560*16;td.park_v=720*16;
    td_streetcar_view_district=TD_DISTRICT_CITY;td_streetcar_ride_view=0;td_streetcar_elapsed=0;
    for(unsigned kind=0;kind<6;kind++){
        unsigned violations=0;
        for(UBYTE i=2;i<8;i++)actors[i].flags=ACTOR_FLAG_HIDDEN;
        actors[kind+2].flags=0;
        for(unsigned fraction=0;fraction<2;fraction++){
            UWORD cu=500*16+fraction*8,cv=400*16+fraction*8;
            actors[kind+2].pos.x=cu*2;actors[kind+2].pos.y=cv*2;
            td_state_t saved=td;actor_t saved_actor=actors[kind+2];
            for(int dx=-11;dx<=11;dx++)for(int dy=-11;dy<=11;dy++){
                UWORD u=(UWORD)(500+dx),v=(UWORD)(400+dy);
                UBYTE clear=td_person_road_clear(u,v);
                int body_clear=reference_fleet_clear(u,v,cu,cv,extents[kind]);
                if(clear&&!body_clear)violations++;
                fleet_check(!clear||body_clear,"A permitted human position never overlaps the actual fleet body");
                if(dx<=-11||dx>=11||dy<=-11||dy>=11)
                    fleet_check(clear,"Humans remain free beyond the fleet exclusion boundary");
            }
            fleet_check(!memcmp(&td,&saved,sizeof(td))&&!memcmp(&actors[kind+2],&saved_actor,sizeof(saved_actor)),
                        "Fleet clearance queries preserve saved state and the actual actor body");
        }
        if(violations)fprintf(stderr,"Fleet kind%u half%u: %u unsafe permitted centres\n",kind,extents[kind],violations);
        actors[kind+2].flags=ACTOR_FLAG_HIDDEN;
        fleet_check(td_person_road_clear(500,400),"Hidden fleet bodies remain absent from pedestrian occupancy");
    }
    /* A coherent actual Core bus leg: from(816,176) north to(816,64),
     * with its next eight-pixel advance160->152. Route19 is the real
     * horizontal foot crossing(784..847,148), phase22->23 at7:15->7:20. */
    UWORD fleet_u[6]={80*16,200*16,320*16,440*16,824*16,816*16};
    UWORD fleet_v[6]={280*16,392*16,168*16,632*16,240*16,160*16};
    td_traffic_context_t context={fleet_u,fleet_v,&actors[9],extents,extents,560*16,720*16,0,0};
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    td.u=816*16;td.v=112*16;td.onfoot=1;td.mode=TD_ROAM;
    td.park_u=560*16;td.park_v=720*16;td.seconds=7;td.subsecond=15;
    td_streetcar_view_district=TD_DISTRICT_CITY;td_streetcar_ride_view=0;
    fixture_authored_routes=1;memset(fixture_routes,TD_NONE,sizeof(fixture_routes));fixture_routes[0]=19;
    for(UBYTE i=2;i<8;i++)actors[i].flags=ACTOR_FLAG_HIDDEN;
    actors[7].flags=0;actors[7].pos.x=816*32;actors[7].pos.y=160*32;
    require(td_district_routes[0][19][0]==784&&td_district_routes[0][19][1]==148,
            "Bus crossing fixture uses the unchanged authored Core route19");
    td_people_reset();require(td_people_present(24)==0&&td_people[0].phase==22&&
            actors[9].pos.x==806*32&&actors[9].pos.y==148*32,
            "Actual world7:15 presents the preceding safe eastbound human phase22");
    require(td_traffic_admit(&context,0,7,5,816*16,160*16,816*16,152*16,0),
            "Actual bus admission accepts its coherent next sweep while the human is safely at806");
    fleet_v[5]=152*16;actors[7].pos.y=152*32;td.subsecond=20;
    td_state_t saved=td;fleet_check(td_people_present(24)==0,"The crossing guard does not invent an impact");
    fleet_check(td_people[0].phase==22&&td_people[0].lag==1&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&
                actors[9].pos.x==806*32&&actors[9].pos.y==148*32,
                "Attempted actual phase23 waits visibly at806 rather than entering the bus body at807");
    fleet_check(reference_fleet_clear(actors[9].pos.x>>5,actors[9].pos.y>>5,816*16,152*16,7),
                "The actual displayed human remains outside the full bus body");
    for(unsigned advance=0;advance<2;advance++){
        UWORD next=fleet_v[5]-8*16;
        UBYTE admitted=td_traffic_admit(&context,0,7,5,fleet_u[5],fleet_v[5],fleet_u[5],next,0);
        fleet_check(admitted,"The bus can clear the crossing instead of becoming mutually blocked");
        if(admitted){fleet_v[5]=next;actors[7].pos.y=next*2;}
        td.subsecond=25+advance*5;fleet_check(td_people_present(24)==0,"Waiting/resuming foot motion invents no impact");
    }
    fleet_check(actors[9].pos.x==807*32&&td_people[0].phase==23&&td_people[0].lag==2&&
                !(actors[9].flags&ACTOR_FLAG_HIDDEN),"The same phase23 resumes once the bus clears, without teleporting or hiding");
    saved.subsecond=td.subsecond;fleet_check(!memcmp(&td,&saved,sizeof(td)),
                "Crossing admission, waiting and resumption preserve all58 gameplay bytes");
    fixture_authored_routes=0;
    require(!unused_traffic_callbacks,"Pure actual traffic body/admission checks never consult the fail-closed hardware stubs");
    if(fleet_failures)fprintf(stderr,"Fleet extent regression: %u failures\n",fleet_failures);
    require(!fleet_failures,"Fleet extent and authored crossing safety regressions pass");
}

static uint32_t random_u32(void){random_state=random_state*1664525u+1013904223u;return random_state;}
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
        if(distance16(x,target_u)<128&&distance16(y,target_v)<128)return 1;
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
    for(UBYTE i=2;i<8;i++)actors[i].flags=ACTOR_FLAG_HIDDEN;
    td_people_reset();
    for(unsigned s=0;s<sizeof(seconds)/sizeof(seconds[0]);s++)for(UBYTE sub=0;sub<60;sub++)
        for(unsigned group=0;group<43;group++){
            td.seconds=seconds[s];td.subsecond=sub;
            for(UBYTE i=0;i<6;i++)fixture_routes[i]=(UBYTE)(group*6+i);
            td_ped_refresh=1;
            require(td_people_present(24)==0,"Phase-only stationary walker cannot invent impact");
            for(UBYTE i=0;i<6;i++){
                UBYTE route=fixture_routes[i];
                if(route==TD_NONE){require(actors[9+i].flags&ACTOR_FLAG_HIDDEN,"Missing route stays hidden");continue;}
                unsigned expected=(unsigned)(((uint64_t)td.seconds*12+sub/5+(uint64_t)route*37)%128);
                require(td_people[i].phase==expected,"Hoisted clock matches full old phase across seconds wrap");
                require(actors[9+i].pos.x==(100+i*90+(expected<64?expected:127-expected))*32&&
                        actors[9+i].pos.y==(100+i*80)*32,"Phase preserves exact authored-path presentation");
                require(variants[i]==route%2&&poses[i]==(expected<64?0:2)+1,"Direction, step and outfit unchanged");
            }
        }
}

static void loaded_rails(void){
    td_streetcar_pose_t pose;td_state_t before;
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    for(UBYTE i=2;i<8;i++)actors[i].flags=ACTOR_FLAG_HIDDEN;
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
    for(UBYTE i=2;i<8;i++)actors[i].flags=ACTOR_FLAG_HIDDEN;
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
    for(UBYTE i=2;i<8;i++)actors[i].flags=ACTOR_FLAG_HIDDEN;
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
    td.seconds=59;td.subsecond=25;td.park_u=944*16;td.park_v=548*16;
    fixture_routes[0]=70;
    require(td_district_routes[TD_DISTRICT_WEST][70][0]==928&&
            td_district_routes[TD_DISTRICT_WEST][70][1]==556,"Ordinary paid fixture uses the registered West route70");
    td_people_reset();
    for(UBYTE sample=0;sample<7;sample++){
        td.subsecond=25+sample*5;
        require(td_streetcar_ride(47,43,1,59,td.subsecond,&pose)&&pose.district==TD_DISTRICT_WEST,
                "Every ordinary approach sample is an actual paid West pose");
        td_streetcar_focus_u=pose.u;td_streetcar_focus_v=pose.v;before=td;
        require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&
                actors[9].pos.x==(952-sample)*32&&actors[9].pos.y==556*32,
                "Ordinary paid view permits the moving West human through the origin-car exclusion");
        require(td_people[0].phase==103+sample&&td_people[0].lag==0&&poses[0]==3&&variants[0]==0,
                "Ordinary paid view retains every walk step without false yielding or stumble");
        require(!memcmp(&td,&before,sizeof(td)),"Ordinary paid-view presentation preserves every saved byte");
    }

    /* A car parked earlier in Core must remain occupancy while a West-origin
     * paid ride shows Core. This is a real moving pose, not a synthetic view. */
    td.reserved=0;td.district=TD_DISTRICT_WEST;td.u=836*16;td.v=556*16;
    td.transit_origin=43;td.transit_target=47;td.ride_left=1;td.seconds=15;td.subsecond=0;
    td.park_district=TD_DISTRICT_CITY;td.park_u=800*16;td.park_v=556*16;
    require(td_streetcar_ride(43,47,1,15,0,&pose)&&pose.district==TD_DISTRICT_CITY,
            "The actual paid Queen path supplies a logical-West/loaded-Core pose");
    td_streetcar_view_district=pose.district;td_streetcar_focus_u=pose.u;td_streetcar_focus_v=pose.v;
    fixture_routes[0]=68;
    require(td_district_routes[TD_DISTRICT_CITY][68][0]==784&&
            td_district_routes[TD_DISTRICT_CITY][68][1]==556,"Core fixture uses the registered sidewalk route");
    td_people_reset();before=td;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "A view-local parked car still blocks a Core pedestrian despite the saved West origin");
    require(td_people[0].phase==8,"Blocked presentation retains the actual Core route phase");
    require(!memcmp(&td,&before,sizeof(td)),"View-local blocked presentation preserves paid state");
    td.park_district=TD_DISTRICT_WEST;td.subsecond=5;before=td;
    require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&
            actors[9].pos.x==793*32&&actors[9].pos.y==556*32,
            "A cleared view-local occupancy resumes the same Core pedestrian without relocation");
    require(!memcmp(&td,&before,sizeof(td)),"Resumed Core presentation preserves paid state");
    fixture_authored_routes=0;
}

int main(void){fleet_extent_guards();contacts();phases();loaded_rails();parked_district_queries();paid_parked_presentation();printf("People hotspot harness: %lu checks, 0 failures\n",checks);return 0;}
