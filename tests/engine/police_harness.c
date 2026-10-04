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
static void load(UBYTE district){loaded=district;memcpy(grid,oracle_grids[district],sizeof(grid));queries=0;tile_hit_x=31;tile_hit_y=76;}
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
static int road_side_piece(unsigned u,unsigned v,const td_police_plan_t *plan){
    unsigned direction=plan->heading;
    for(unsigned i=0;i<sizeof(oracle_roads)/sizeof(oracle_roads[0]);i++){
        const UWORD *r=oracle_roads[i];if(r[0]!=loaded)continue;
        if(!(direction&1)&&r[2]==r[4]){
            unsigned lane=(r[2]+(direction==0?8:-8))*16;
            unsigned low=(r[1]<r[3]?r[1]:r[3])*16,high=(r[1]>r[3]?r[1]:r[3])*16;
            if(v==lane&&plan->v==lane&&u>=low&&u<=high&&plan->u>=low&&plan->u<=high)return 1;
        }else if((direction&1)&&r[1]==r[3]){
            unsigned lane=(r[1]+(direction==3?8:-8))*16;
            unsigned low=(r[2]<r[4]?r[2]:r[4])*16,high=(r[2]>r[4]?r[2]:r[4])*16;
            if(u==lane&&plan->u==lane&&v>=low&&v<=high&&plan->v>=low&&plan->v<=high)return 1;
        }
    }
    for(unsigned i=0;i<sizeof(oracle_junctions)/sizeof(oracle_junctions[0]);i++){
        const UWORD *p=oracle_junctions[i];if(p[0]!=loaded)continue;
        unsigned x=p[1]*16,y=p[2]*16;
        if(distance(u,x)>128||distance(v,y)>128||distance(plan->u,x)>128||distance(plan->v,y)>128)continue;
        if(direction==0&&v==y+128)return 1;
        if(direction==1&&u==x-128)return 1;
        if(direction==2&&v==y-128)return 1;
        if(direction==3&&u==x+128)return 1;
    }
    for(unsigned i=0;i<sizeof(oracle_endcaps)/sizeof(oracle_endcaps[0]);i++){
        const UWORD *p=oracle_endcaps[i];if(p[0]!=loaded)continue;
        unsigned x=p[1]*16,y=p[2]*16,a=p[3]*16,b=p[4]*16;
        unsigned d=x!=a?(a>x?0:2):(b>y?1:3);
        if(direction!=d)continue;
        if(x==a&&u==x&&plan->u==x&&v>=(y<b?y:b)&&v<=(y>b?y:b)&&
           plan->v>=(y<b?y:b)&&plan->v<=(y>b?y:b))return 1;
        if(y==b&&v==y&&plan->v==y&&u>=(x<a?x:a)&&u<=(x>a?x:a)&&
           plan->u>=(x<a?x:a)&&plan->u<=(x>a?x:a))return 1;
    }
    return 0;
}
static int road_side(unsigned u,unsigned v,const td_police_plan_t *plan){
    unsigned amount=distance(u,plan->u)+distance(v,plan->v);
    td_police_plan_t point=*plan;
    for(unsigned step=0;;step+=16){
        if(step>amount)step=amount;
        unsigned x=plan->heading==0?u+step:plan->heading==2?u-step:u;
        unsigned y=plan->heading==1?v+step:plan->heading==3?v-step:v;
        point.u=x;point.v=y;
        if(!road_side_piece(x,y,&point))return 0;
        if(step==amount)return 1;
    }
}
static void valid_plan(unsigned u,unsigned v,const td_police_plan_t *plan){
    expect(tile_hit_x==31&&tile_hit_y==76,"successful planner queries preserve native collision hit globals");
    expect(plan->valid==1&&plan->heading<4,"valid waypoint has one bounded native heading");
    expect((plan->u==u)!=(plan->v==v),"waypoint is one nonzero cardinal segment without position snapping");
    expect((plan->heading==0?plan->u>u&&plan->v==v:
            plan->heading==1?plan->v>v&&plan->u==u:
            plan->heading==2?plan->u<u&&plan->v==v:plan->v<v&&plan->u==u),
           "heading matches the actual emitted movement, independent of old patrol legs");
    expect(road_side(u,v,plan),"each emitted piece uses the right-hand named street or an explicit bounded junction/endcap");
    expect(oracle_segment(u>>4,v>>4,plan->u>>4,plan->v>>4),
           "every touched eleven-pixel body stays on registered collision ground");
}
static int navigate(UBYTE district,unsigned u,unsigned v,unsigned goal_u,unsigned goal_v,
                    UBYTE heading,unsigned limit,UBYTE wanted){
    load(district);td_police_plan_t plan={0,0,0,0};
    for(unsigned step=0;step<limit;step++){
        if(wanted?(distance(u,goal_u)<14*16&&distance(v,goal_v)<14*16):(u==goal_u&&v==goal_v))return 1;
        if(!td_police_plan(district,wanted,u,v,goal_u,goal_v,heading,&plan))return 0;
        valid_plan(u,v,&plan);
        heading=plan.heading;u=plan.u;v=plan.v;
    }
    return wanted?(distance(u,goal_u)<14*16&&distance(v,goal_v)<14*16):(u==goal_u&&v==goal_v);
}
static void test_cardinal_turns(void){
    /* Independent right-hand corner/outgoing-lane oracle at Spadina/College.
     * E travels y+8, S x-8, W y-8, N x+8. A left turn or U-turn must
     * traverse the square's actual first edge rather than snap laterally. */
    const UWORD starts[4][2]={{200,296},{200,280},{216,280},{216,296}};
    const UWORD goals[4][2]={{328,296},{200,392},{88,280},{216,184}};
    const UBYTE first[4][4]={{0,1,0,0},{1,1,2,1},{2,2,2,3},{0,3,3,3}};
    load(0);
    for(unsigned incoming=0;incoming<4;incoming++)for(unsigned outgoing=0;outgoing<4;outgoing++){
        td_police_plan_t plan={123,456,77,99};
        unsigned u=starts[incoming][0]*16,v=starts[incoming][1]*16;
        expect(td_police_plan(0,3,u,v,goals[outgoing][0]*16,goals[outgoing][1]*16,incoming,&plan),
               "all sixteen lawful incoming/outgoing intersection choices produce a connected first edge");
        valid_plan(u,v,&plan);
        expect(plan.heading==first[incoming][outgoing],"turn heading describes the real connector, not the future street");
        expect(navigate(0,u,v,goals[outgoing][0]*16,goals[outgoing][1]*16,incoming,24,3),
               "turn connectors make progress to the intended right-hand outgoing street without oscillating");
    }
    td_police_plan_t plan={123,456,77,99},before=plan;
    expect(!td_police_plan(0,3,200*16,304*16,216*16,304*16,0,&plan)&&!memcmp(&plan,&before,sizeof(plan)),
           "a clear nearby parallel coordinate cannot invent an off-lane lateral shortcut");
    expect(!td_police_plan(0,0,200*16,296*16,200*16,312*16,0,&plan)&&!memcmp(&plan,&before,sizeof(plan)),
           "heat-zero accepts only an exact authored patrol target, not an arbitrary nearby road point");
}
static void test_registered_waypoints(void){
    unsigned successes[TD_DISTRICT_COUNT]={0};
    for(unsigned n=0;n<sizeof(oracle_vertices)/sizeof(oracle_vertices[0]);n++){
        UBYTE district=oracle_vertices[n][0];unsigned x=oracle_vertices[n][1]*16,y=oracle_vertices[n][2]*16;
        load(district);
        for(unsigned heading=0;heading<4;heading++)for(unsigned wanted=1;wanted<=3;wanted++){
            unsigned goal_u=heading==0?1000*16:heading==2?24*16:x;
            unsigned goal_v=heading==1?944*16:heading==3?24*16:y;
            td_police_plan_t plan={123,456,77,99},before=plan;
            UBYTE result=td_police_plan(district,wanted,x,y,goal_u,goal_v,heading,&plan);
            if(result){successes[district]++;valid_plan(x,y,&plan);}
            else expect(!memcmp(&plan,&before,sizeof(plan)),"an inaccessible directed road choice preserves the cached plan");
        }
        for(unsigned goal=0;goal<oracle_patrol_counts[district];goal++)
            expect(navigate(district,x,y,oracle_patrol[district][goal][0]*16,
                            oracle_patrol[district][goal][1]*16,0,512,0),
                   "every supported lane vertex returns to every exact authored role2 target with finite continuous movement");
    }
    for(unsigned district=0;district<TD_DISTRICT_COUNT;district++)
        expect(oracle_traffic_enabled[district]?successes[district]>0:!successes[district],
               "actual road districts support police choices while the foot-only Island has no invented pursuit");
    /* Source-authenticated six-district pursuit/capture-distance and exact
     * return cases. These are not arbitrary empty-grid junction examples. */
    const UWORD cases[6][8]={
        {0,320,168,2,640,528,840,168}, {1,904,400,1,976,64,920,400},
        {2,440,216,2,432,112,168,216}, {3,232,296,0,816,496,376,296},
        {4,536,160,1,192,816,552,160}, {6,328,568,1,640,952,416,568}};
    for(unsigned c=0;c<6;c++){
        const UWORD *a=cases[c];
        int caught=navigate(a[0],a[1]*16,a[2]*16,a[4]*16,a[5]*16,a[3],96,3);
        if(!caught)fprintf(stderr,"Pursuit fixture failed district %u\n",a[0]);
        expect(caught,"six real districts have representative connected pursuits reaching capture distance");
        expect(navigate(a[0],a[1]*16,a[2]*16,a[6]*16,a[7]*16,a[3],512,0),
               "all six district patrols rejoin their exact next target without teleporting");
    }
    for(unsigned n=0;n<sizeof(oracle_edges)/sizeof(oracle_edges[0]);n++){
        const UWORD *edge=oracle_edges[n];
        unsigned heading=edge[1]!=edge[3]?(edge[3]>edge[1]?0:2):(edge[4]>edge[2]?1:3);
        for(unsigned fraction=1;fraction<16;fraction+=7){
            unsigned u=edge[1]*16,v=edge[2]*16;
            if(heading==0)u+=fraction;else if(heading==1)v+=fraction;
            else if(heading==2)u-=fraction;else v-=fraction;
            td_police_plan_t plan={123,456,77,99};load(edge[0]);
            expect(td_police_plan(edge[0],0,u,v,oracle_patrol[edge[0]][0][0]*16,
                                  oracle_patrol[edge[0]][0][1]*16,heading,&plan),
                   "fractional Q4 movement along real directed edges retains its lane until a registered vertex");
            valid_plan(u,v,&plan);
            expect(heading&1?plan.u==u:plan.v==v,"fractional continuation never changes the lateral coordinate");
        }
    }
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
    load(1);
    unsigned signal=0;
    for(unsigned i=0;i<sizeof(oracle_nodes)/sizeof(oracle_nodes[0]);i++)
        if(oracle_nodes[i][0]==1&&oracle_nodes[i][1]==816&&oracle_nodes[i][2]==112)signal=1;
    expect(!signal,"the real Bloor bend is unsignalled rather than a fabricated traffic-light junction");
    /* West Bloor's real 816/112 bend has two arms, no invented traffic light.
     * Its eastward lane must turn north via the two valid lane corners. */
    expect(td_police_plan(1,2,808*16,120*16,824*16,72*16,0,&plan),
           "a registered unsignalled Bloor bend has a real connected lane turn");
    valid_plan(808*16,120*16,&plan);
    expect(navigate(1,808*16,120*16,824*16,72*16,0,24,2),
           "the registered unsignalled bend reaches its outgoing right-hand lane");
    load(0);
    for(unsigned y=0;y<122;y++)grid[y*128+38]=255;
    before=plan;
    expect(!td_police_plan(0,2,320*16,168*16,184*16,168*16,2,&plan)&&
           !memcmp(&plan,&before,sizeof(plan)),
           "an intermediate blocked strip invalidates the entire generated road edge and preserves output");
    load(0);
    for(unsigned x=24;x<=992;x+=8)for(unsigned heading=0;heading<4;heading++){
        td_police_plan_t fractional={123,456,77,99},saved=fractional;
        if(td_police_plan(0,3,x*16+7,168*16+7,640*16,528*16,heading,&fractional))
            valid_plan(x*16+7,168*16+7,&fractional);
        else expect(!memcmp(&fractional,&saved,sizeof(saved)),"off-lane fractions fail without rewriting the output");
    }
}
static void test_caller_admission(void){
    load(0);td_police_plan_t plan={0,0,0,0};UWORD us[6],vs[6];
    actor_t people[6];memset(people,0,sizeof(people));
    for(unsigned i=0;i<6;i++){us[i]=(24+i*40)*16;vs[i]=720*16;people[i].flags=ACTOR_FLAG_HIDDEN;}
    us[2]=184*16;vs[2]=72*16;
    td_traffic_context_t ctx={us,vs,people,NULL,NULL,0,0,0,4};
    expect(td_police_plan(0,3,us[2],vs[2],480*16,72*16,0,&plan)&&plan.heading==0,
           "high-attention police plans toward the courier along the real road");
    expect(!td_traffic_admit(&ctx,0,7,2,us[2],vs[2],us[2]+128,vs[2],1),
           "a chase waypoint grants no exception to the same red light as civilian traffic");
    expect(td_traffic_admit(&ctx,0,0,2,us[2],vs[2],us[2]+128,vs[2],1),
           "the chase can advance safely on green with the full bounded sweep");
    us[0]=198*16;vs[0]=72*16;
    expect(!td_traffic_admit(&ctx,0,0,2,us[2],vs[2],us[2]+128,vs[2],1),
           "pursuing police still queues behind another complete vehicle body");
    us[0]=24*16;vs[0]=720*16;people[0].flags=0;people[0].pos.x=198*32;people[0].pos.y=72*32;
    expect(!td_traffic_admit(&ctx,0,0,2,us[2],vs[2],us[2]+128,vs[2],1),
           "pursuing police still respects a visible pedestrian footprint");
}
int main(void){
    expect(sizeof(td_police_plan_t)==6,"the caller-owned public waypoint remains exactly six bytes");
    test_cardinal_turns();test_registered_waypoints();test_actual_island_exclusion();test_invalid_and_obstructions();test_caller_admission();
    printf("Police planner: %u checks, %u failures\n",checks,failures);
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
