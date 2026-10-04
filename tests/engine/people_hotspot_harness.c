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
UBYTE td_road_walkable(UWORD u,UWORD v){return u>=3&&u<=1020&&v>=3&&v<=972;}
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
    /* A coherent actual Core bus leg: from(808,168) north to(808,72),
     * with its next eight-pixel advance160->152. Route19 is the real
     * horizontal foot crossing(784..847,148), phase14->15 at6:35->6:40. */
    UWORD fleet_u[6]={80*16,200*16,320*16,440*16,824*16,808*16};
    UWORD fleet_v[6]={280*16,392*16,168*16,632*16,240*16,160*16};
    td_traffic_context_t context={fleet_u,fleet_v,&actors[9],extents,extents,560*16,720*16,0,0};
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    td.u=808*16;td.v=112*16;td.onfoot=1;td.mode=TD_ROAM;
    td.park_u=560*16;td.park_v=720*16;td.seconds=6;td.subsecond=35;
    td_streetcar_view_district=TD_DISTRICT_CITY;td_streetcar_ride_view=0;
    fixture_authored_routes=1;memset(fixture_routes,TD_NONE,sizeof(fixture_routes));fixture_routes[0]=19;
    for(UBYTE i=2;i<8;i++)actors[i].flags=ACTOR_FLAG_HIDDEN;
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
                require(variants[i]==route%4&&poses[i]==(expected<64?0:2)+1,"Direction, step and outfit unchanged");
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
    require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&
            actors[9].pos.x==794*32&&actors[9].pos.y==556*32,
            "A cleared view-local occupancy resumes the same Core pedestrian without relocation");
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
    for(UBYTE i=2;i<8;i++)actors[i].flags=ACTOR_FLAG_HIDDEN;
    actors[9].flags=ACTOR_FLAG_HIDDEN|0x40;
    td_people_reset();
}

static void appearance_admission(void){
    td_state_t before;
    require(td_district_routes[0][19][0]==784&&td_district_routes[0][19][1]==148,
            "Appearance fixtures use actual Core crossing route19, not a synthetic human location");
    /* The bus is on its real northbound leg808,168->808,72 and may retain
     * this pose over the five-VBlank interval between traffic batches. */
    appearance_case(824*16,147*16+7);
    actors[7].flags=0;actors[7].pos.x=808*32;actors[7].pos.y=152*32;
    before=td;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN)&&td_people[0].phase==33,
            "At8:10 the bus hides authored phase33 human817,148 before any courier contact");
    require(!memcmp(&td,&before,sizeof(td)),"Hidden bus occupancy preserves all58 gameplay bytes");
    td.subsecond=15;td.v=148*16+3;before=td;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun,
            "At8:15 a newly clear phase34 human818,148 cannot appear inside the advancing occupied courier");
    require(!memcmp(&td,&before,sizeof(td))&&(actors[9].flags&0x40),
            "Denied appearance adds no penalty and preserves unrelated actor flags");
    td.u=833*16;td.subsecond=20;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "A clear final endpoint still defers appearance inside the preceding courier sweep");
    td.subsecond=25;before=td;
    require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN)&&
            actors[9].pos.x==819*32&&actors[9].pos.y==148*32&&!td_people[0].stun,
            "The same authored human appears promptly once both body and preceding sweep are clear");
    require(!memcmp(&td,&before,sizeof(td)),"Admitted appearance preserves cash, cargo, attention and save fields");

    appearance_case(824*16,148*16);
    actors[7].flags=0;actors[7].pos.x=808*32;actors[7].pos.y=152*32;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN),"Stationary control starts with the same bus-hidden human");
    td.subsecond=15;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun,
            "A stationary occupied vehicle does not admit an overlapping appearance or invent an impact");
    td.u=833*16;td.subsecond=20;td_people_present(24);
    require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "After separation a stationary clear update needs no extra grace countdown");

    /* These signed rectangle bounds are an independent conservative maximum
     * occupied vehicle half7 plus human half3. Exercise every Q4 fraction
     * around strict10px, all vehicle selections and both coordinate axes. */
    for(UBYTE vehicle=0;vehicle<4;vehicle++)for(UBYTE axis=0;axis<2;axis++)
        for(int sign=-1;sign<=1;sign+=2)for(int gap=144;gap<176;gap++){
            UWORD cu=(UWORD)(817*16+(axis?0:sign*gap));
            UWORD cv=(UWORD)(148*16+(axis?sign*gap:0));
            appearance_case(cu,cv);td.vehicle=vehicle;before=td;
            int clear=reference_fleet_clear(817,148,cu,cv,8);
            require(td_people_present(24)==0,"Stationary fractional-boundary admission never charges a hit");
            require(!!(actors[9].flags&ACTOR_FLAG_HIDDEN)==!clear,
                    "Appearance visibility follows independent exact Q4 maximum-body rectangles");
            require(!td_people[0].stun&&!memcmp(&td,&before,sizeof(td))&&(actors[9].flags&0x40),
                    "Boundary admission preserves recovery, every saved byte and unrelated actor flags");
        }

    /* A new route must not inherit an old route's visible history. */
    appearance_case(828*16,148*16);
    require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "Exactly11px separated first appearance is admitted without a delay");
    require(td_district_routes[0][26][0]==784&&td_district_routes[0][26][1]==204,
            "Replacement case uses the registered Core route26");
    fixture_routes[0]=26;td_ped_refresh=1;td.u=826*16;td.v=204*16;before=td;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun,
            "A replacement route cannot inherit stale visible flags and materialize inside the courier");
    require(!memcmp(&td,&before,sizeof(td)),"Replacing a nearby identity creates no saved impact consequences");

    appearance_case(805*16,148*16);td.u=829*16;before=td;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun,
            "A first appearance beyond both endpoints cannot retroactively collide with the24px crossing sweep");
    require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "A now-clear stationary sweep admits that person immediately on the next update");
    require(!memcmp(&td,&before,sizeof(td)),"Retrospective admission suppression adds no gameplay penalty");

    /* Endpoint rectangles are independently clear. The middle point of the
     * cardinal sweep tests fractional tangency of the same full bodies. */
    for(UBYTE axis=0;axis<2;axis++)for(int sign=-1;sign<=1;sign+=2)
        for(int gap=175;gap<=177;gap++){
            UWORD start_u=(UWORD)(817*16+(axis?sign*gap:-12*16));
            UWORD start_v=(UWORD)(148*16+(axis?-12*16:sign*gap));
            UWORD end_u=(UWORD)(817*16+(axis?sign*gap:12*16));
            UWORD end_v=(UWORD)(148*16+(axis?12*16:sign*gap));
            UWORD middle_u=(UWORD)(817*16+(axis?sign*gap:0));
            UWORD middle_v=(UWORD)(148*16+(axis?0:sign*gap));
            appearance_case(start_u,start_v);td.u=end_u;td.v=end_v;
            require(reference_fleet_clear(817,148,start_u,start_v,8)&&
                    reference_fleet_clear(817,148,end_u,end_v,8),
                    "Tangential prior-sweep fixture has independently clear start and end bodies");
            int clear=reference_fleet_clear(817,148,middle_u,middle_v,8);
            require(td_people_present(24)==0&&!!(actors[9].flags&ACTOR_FLAG_HIDDEN)==!clear,
                    "Prior-sweep admission retains strict175/176/177Q4 tangency on both axes and signs");
            require(!td_people[0].stun,"Tangential first appearances never produce a retrospective stumble");
        }
    appearance_case(805*16,136*16);td.u=829*16;td.v=160*16;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun,
            "A diagonal first-appearance sweep through the human also defers admission despite clear endpoints");

    appearance_case(781*16,148*16);td.u=853*16;
    require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "A remote discontinuity beyond32px retains the existing rejected-sweep semantics");
    appearance_case(781*16,148*16);td.u=817*16;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "A rejected remote sweep still cannot bypass current occupied-body admission");

    /* A genuine continuous ten-pixel sweep marks exactly one dead route. */
    appearance_case(828*16,148*16);
    require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),"Continuous control begins outside the eleven-pixel appearance edge");
    td.u=825*16;before=td;
    require(td_people_present(24)==1&&td_people[0].stun==6&&poses[0]==TD_CIVILIAN_HIT,
            "A real continuously visible swept contact begins a single knockback");
    require(!memcmp(&td,&before,sizeof(td)),"People returns a hit without independently rewriting higher-level penalties");
    require(td_people_present(24)==0,"The same struck identity cannot be charged twice");
    for(UBYTE frame=25;frame<=48;frame++)require(td_people_present(frame)==0,"Knockback never repeats the impact");
    require(td_people[0].stun==6&&td_people[0].recover==24&&poses[0]==TD_CIVILIAN_PRONE,
            "After twenty-four active motion ticks the person remains prone");
    UWORD corpse_u=actors[9].pos.x,corpse_v=actors[9].pos.y;
    for(unsigned second=0;second<60;second++){td.seconds++;td_people_second();}
    td_people_present(49);
    require(td_people[0].stun==6&&poses[0]==TD_CIVILIAN_PRONE&&actors[9].pos.x==corpse_u&&actors[9].pos.y==corpse_v,
            "Active seconds do not revive the corpse or move its ground pose");
    td.seconds=8;td.subsecond=10;td.u=817*16;td.v=148*16;
    td_people_reset();before=td;
    require(td_people_present(24)==0&&(actors[9].flags&ACTOR_FLAG_HIDDEN)&&!td_people[0].stun,
            "Scene reset discards stale visible continuity before admitting a person overlapping the vehicle");
    require(!memcmp(&td,&before,sizeof(td)),"Scene-reset admission leaves the58-byte game record unchanged");

    appearance_case(817*16,148*16);td.mode=TD_RIDE;td.district=TD_DISTRICT_WEST;
    td_streetcar_ride_view=1;td_streetcar_focus_u=817*16;td_streetcar_focus_v=148*16;before=td;
    require(td_people_present(24)==0&&!(actors[9].flags&ACTOR_FLAG_HIDDEN),
            "A remote paid view does not treat logical-origin occupied coordinates as a visible local car");
    require(!memcmp(&td,&before,sizeof(td)),"Remote appearance preserves paid fields and logical district");
    fixture_authored_routes=0;
}

static void idle_courier_case(void){
    /* Actual Core cold fleet poses; the original visible route19 case is
     * clear of all six bodies, rather than relying on hidden traffic. */
    static const UWORD cu[6]={80,200,320,440,824,216};
    static const UWORD cv[6]={280,392,168,632,240,72};
    appearance_case(828*16,147*16+7);
    td.speed=0;td.park_u=560*16;td.park_v=720*16;
    for(UBYTE i=0;i<6;i++){
        actors[2+i].flags=0;actors[2+i].pos.x=cu[i]*32;actors[2+i].pos.y=cv[i]*32;
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

int main(void){fleet_extent_guards();contacts();phases();loaded_rails();parked_district_queries();paid_parked_presentation();appearance_admission();idle_courier_boundaries();idle_courier_wait_and_resume();idle_courier_impacts_and_recovery();idle_courier_context_controls();idle_courier_resume_regression();reversing_courier_route83();printf("People hotspot harness: %lu checks, 0 failures\n",checks);return 0;}
