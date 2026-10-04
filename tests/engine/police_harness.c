#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "td_police.h"
#include "td_roads.h"
#include "td_district.h"
#include "td_traffic.h"
#include "td_streetcar_runtime.h"
#include "police_fixture.h"

UBYTE joy,joy_pressed,tile_hit_x,tile_hit_y;
static UBYTE loaded,grid[128*122];
static unsigned checks,failures,queries;
static void expect(int truth,const char *message){
    checks++;
    if(!truth){failures++;if(failures<12)fprintf(stderr,"FAIL: %s\n",message);}
}
/* This planner harness uses the strict traffic admission query, never the
 * fused epoch mover. Supply its new link dependency without pretending to
 * model live/future trams: unexpected invocation fails the harness closed.
 * Real fused guard delegation is covered by traffic_harness, and the native
 * tram implementation by runtime_harness. */
UBYTE td_streetcar_runtime_traffic_sweep_clear(UBYTE district,UWORD old_u,UWORD old_v,
    UWORD u,UWORD v,UBYTE half) BANKED {
    (void)district;(void)old_u;(void)old_v;(void)u;(void)v;(void)half;
    expect(0,"planner-only fixture unexpectedly invoked the fused future-tram runtime guard");
    return FALSE;
}
UBYTE tile_at(UBYTE x,UBYTE y){queries++;return x<128&&y<122?grid[y*128+x]:15;}
/* Only the hardware range adapter changes: actual road/police logic remains
 * linked below. Inclusive/reverse ranges expose native hit globals; the
 * production road query must preserve them around its bulk scans. */
static UBYTE range(UBYTE horizontal,UBYTE mask,UBYTE fixed,UBYTE start,UBYTE end){
    int position=start,step=start<end?1:-1;
    for(;;position+=step){
        tile_hit_x=horizontal?position:fixed;tile_hit_y=horizontal?fixed:position;
        UBYTE value=tile_at(tile_hit_x,tile_hit_y);
        if(value&mask)return value;
        if(position==end)return 0;
    }
}
UBYTE tile_col_test_range_x(UBYTE mask,UBYTE y,UBYTE left,UBYTE right){return range(1,mask,y,left,right);}
UBYTE tile_col_test_range_y(UBYTE mask,UBYTE x,UBYTE top,UBYTE bottom){return range(0,mask,x,top,bottom);}
UBYTE td_district_current(void){return loaded;}
static void load(UBYTE district){loaded=district;memcpy(grid,oracle_grids[district],sizeof(grid));queries=0;}
static unsigned distance(unsigned a,unsigned b){return a>b?a-b:b-a;}

/* Independent pixel-space footprint oracle, sampling every eight pixels or
 * less: adjacent eleven-pixel bodies overlap, proving the entire segment. */
static int oracle_body(unsigned u,unsigned v){
    if(u<8||v<8||u>1016||v>968)return 0;
    for(int y=-5;y<=5;y++)for(int x=-5;x<=5;x++)
        if(grid[((v+y)/8)*128+(u+x)/8])return 0;
    return 1;
}
static int oracle_segment(unsigned old_u,unsigned old_v,unsigned u,unsigned v){
    if(old_u!=u&&old_v!=v)return 0;
    unsigned amount=distance(old_u,u)+distance(old_v,v);
    for(unsigned step=0;step<amount;step+=8){
        unsigned x=old_u==u?u:u>old_u?old_u+step:old_u-step;
        unsigned y=old_v==v?v:v>old_v?old_v+step:old_v-step;
        if(!oracle_body(x,y))return 0;
    }
    return oracle_body(u,v);
}
static void valid_plan(unsigned u,unsigned v,const td_police_plan_t *plan){
    expect(plan->valid==1&&plan->heading<4,"valid waypoint has one bounded native heading");
    expect((plan->u==u)!=(plan->v==v),"waypoint is one nonzero cardinal segment without position snapping");
    expect((plan->heading==0?plan->u>u&&plan->v==v:
            plan->heading==1?plan->v>v&&plan->u==u:
            plan->heading==2?plan->u<u&&plan->v==v:plan->v<v&&plan->u==u),
           "heading matches the actual emitted movement, independent of old patrol legs");
    expect(oracle_segment(u>>4,v>>4,plan->u>>4,plan->v>>4),
           "every touched eleven-pixel body stays on registered collision ground");
}
static int navigate(UBYTE district,unsigned u,unsigned v,unsigned goal_u,unsigned goal_v,
                    UBYTE heading,unsigned limit){
    load(district);td_police_plan_t plan={0,0,0,0};
    for(unsigned step=0;step<limit;step++){
        if(u==goal_u&&v==goal_v)return 1;
        if(!td_police_plan(district,0,u,v,goal_u,goal_v,heading,&plan))return 0;
        valid_plan(u,v,&plan);
        heading=plan.heading;u=plan.u;v=plan.v;
    }
    return u==goal_u&&v==goal_v;
}
static void test_registered_waypoints(void){
    unsigned successes[TD_DISTRICT_COUNT]={0};
    for(unsigned n=0;n<sizeof(oracle_nodes)/sizeof(oracle_nodes[0]);n++){
        UBYTE district=oracle_nodes[n][0];unsigned x=oracle_nodes[n][1]*16,y=oracle_nodes[n][2]*16;
        load(district);
        for(unsigned heading=0;heading<4;heading++)for(unsigned wanted=0;wanted<=3;wanted++){
            unsigned goal_u=heading==0?1000*16:heading==2?24*16:x;
            unsigned goal_v=heading==1?944*16:heading==3?24*16:y;
            td_police_plan_t plan={123,456,77,99},before=plan;
            UBYTE result=td_police_plan(district,wanted,x,y,goal_u,goal_v,heading,&plan);
            if(result){successes[district]++;valid_plan(x,y,&plan);}
            else expect(!memcmp(&plan,&before,sizeof(plan)),"an inaccessible authored arm preserves the cached plan");
        }
    }
    for(unsigned district=0;district<TD_DISTRICT_COUNT;district++)
        expect(oracle_traffic_enabled[district]?successes[district]>0:!successes[district],
               "actual road districts support police choices while the foot-only Island has no invented junction or pursuit");
    expect(navigate(0,48*16,168*16,640*16,528*16,0,24),
           "police leaves its original College loop and actually reaches a distant perpendicular courier road");
    expect(navigate(0,640*16,528*16,840*16,168*16,3,24),
           "heat-zero patrol return reaches the exact saved lane target continuously without teleporting");
    expect(navigate(1,904*16,400*16,912*16,288*16,0,24),
           "West police navigates from its short industrial loop onto a different collision-backed road");
    expect(navigate(3,384*16,288*16,816*16,496*16,0,24),
           "East police actually traverses road junctions toward a distant courier target");
}
static void test_actual_island_exclusion(void){
    const UWORD anchors[6][2]={{320,280},{512,448},{920,280},{160,600},{512,744},{904,440}};
    unsigned foot_tiles=0;
    load(TD_DISTRICT_ISLANDS);
    for(unsigned y=0;y<122;y++)for(unsigned x=0;x<128;x++){
        expect(grid[y*128+x]==15||grid[y*128+x]==16,"actual Island collision consists of blocked water/buildings and public foot-only ground");
        if(grid[y*128+x]!=16)continue;
        foot_tiles++;td_police_plan_t plan={123,456,77,99},before=plan;
        expect(!td_police_plan(TD_DISTRICT_ISLANDS,3,(x*8+4)*16,(y*8+4)*16,
                              512*16,448*16,0,&plan)&&!memcmp(&plan,&before,sizeof(plan)),
               "every actual Island foot tile rejects a vehicle pursuit without replacing its cached plan");
    }
    expect(foot_tiles>100,"Island police exclusion is tested on a substantial real walkable scene rather than an empty mocked map");
    for(unsigned anchor=0;anchor<6;anchor++)for(unsigned heading=0;heading<4;heading++)
        for(unsigned heat=0;heat<=3;heat++)for(unsigned fraction=0;fraction<16;fraction++){
            td_police_plan_t plan={123,456,77,99},before=plan;
            unsigned u=anchors[anchor][0]*16+fraction,v=anchors[anchor][1]*16+15-fraction;
            expect(grid[(v/16/8)*128+u/16/8]==16,"each preserved public ferry/client checkpoint uses actual foot ground");
            expect(!td_police_plan(TD_DISTRICT_ISLANDS,heat,u,v,560*16,720*16,heading,&plan)&&
                   !memcmp(&plan,&before,sizeof(plan)),
                   "all attention levels, headings and Q4 phases preserve Island checkpoint pursuit rejection");
        }
}
static void test_invalid_and_obstructions(void){
    load(0);td_police_plan_t plan={123,456,77,99},before=plan;
    expect(!td_police_plan(0,1,208*16,64*16,480*16,176*16,0,NULL),"NULL output fails closed");
    const UWORD bad[][7]={{TD_DISTRICT_COUNT,1,3328,1024,7680,2816,0},
        {1,1,3328,1024,7680,2816,0},{0,4,3328,1024,7680,2816,0},
        {0,1,127,1024,7680,2816,0},{0,1,65535,1024,7680,2816,0},
        {0,1,3328,127,7680,2816,0},{0,1,3328,65535,7680,2816,0},
        {0,1,3328,1024,16384,2816,0},{0,1,3328,1024,7680,15616,0},
        {0,1,3328,1024,7680,2816,4},{0,1,3328,1024,3328,1024,0}};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){
        const UWORD *a=bad[i];
        expect(!td_police_plan(a[0],a[1],a[2],a[3],a[4],a[5],a[6],&plan)&&
               !memcmp(&plan,&before,sizeof(plan)),"bad district/loaded scene/heat/bounds/heading/arrived input preserves output");
    }
    memset(grid,255,sizeof(grid));
    expect(!td_police_plan(0,3,208*16,64*16,480*16,176*16,0,&plan)&&
           !memcmp(&plan,&before,sizeof(plan)),"building or water under the police fails closed without a hidden move");
    /* A bend with no signal-node centre must still turn on actual connected
     * terrain, while a blocked forward road cannot be jumped or retired. */
    memset(grid,255,sizeof(grid));
    for(unsigned y=35;y<=41;y++)for(unsigned x=48;x<=64;x++)grid[y*128+x]=0;
    for(unsigned y=35;y<=51;y++)for(unsigned x=58;x<=64;x++)grid[y*128+x]=0;
    expect(td_police_plan(0,2,488*16,304*16,488*16,400*16,0,&plan),"an unsignalled bend yields a real connected turn");
    valid_plan(488*16,304*16,&plan);
    for(unsigned y=35;y<=41;y++)grid[y*128+63]=255;
    if(td_police_plan(0,2,488*16,304*16,700*16,304*16,0,&plan)){
        valid_plan(488*16,304*16,&plan);
        expect(plan.u<63*8*16,"intermediate water/rail strip is never skipped even when the remote endpoint is clear");
    }
    load(0);
    for(unsigned x=8;x<=1016;x+=8)for(unsigned heading=0;heading<4;heading++){
        if(td_police_plan(0,3,x*16+7,168*16+7,640*16,528*16,heading,&plan))
            valid_plan(x*16+7,168*16+7,&plan);
    }
}
static void test_caller_admission(void){
    load(0);td_police_plan_t plan={0,0,0,0};UWORD us[6],vs[6];
    actor_t people[6];memset(people,0,sizeof(people));
    for(unsigned i=0;i<6;i++){us[i]=(24+i*40)*16;vs[i]=720*16;people[i].flags=ACTOR_FLAG_HIDDEN;}
    us[2]=184*16;vs[2]=64*16;
    td_traffic_context_t ctx={us,vs,people,NULL,NULL,0,0,0,4};
    expect(td_police_plan(0,3,us[2],vs[2],480*16,64*16,0,&plan)&&plan.heading==0,
           "high-attention police plans toward the courier along the real road");
    expect(!td_traffic_admit(&ctx,0,7,2,us[2],vs[2],us[2]+128,vs[2],1),
           "a chase waypoint grants no exception to the same red light as civilian traffic");
    expect(td_traffic_admit(&ctx,0,0,2,us[2],vs[2],us[2]+128,vs[2],1),
           "the chase can advance safely on green with the full bounded sweep");
    us[0]=198*16;vs[0]=64*16;
    expect(!td_traffic_admit(&ctx,0,0,2,us[2],vs[2],us[2]+128,vs[2],1),
           "pursuing police still queues behind another complete vehicle body");
    us[0]=24*16;vs[0]=720*16;people[0].flags=0;people[0].pos.x=198*32;people[0].pos.y=64*32;
    expect(!td_traffic_admit(&ctx,0,0,2,us[2],vs[2],us[2]+128,vs[2],1),
           "pursuing police still respects a visible pedestrian footprint");
}
int main(void){
    test_registered_waypoints();test_actual_island_exclusion();test_invalid_and_obstructions();test_caller_admission();
    printf("Police planner: %u checks, %u failures\n",checks,failures);
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
