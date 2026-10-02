/* Include unchanged production code; synthetic graphs exercise its real
 * private search/geometry helpers without replacing an algorithm or table. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world_under_test.c"

static unsigned checks,failures;
static void expect(int condition,const char *name){
    checks++;if(!condition){failures++;fprintf(stderr,"FAIL: %s\n",name);}
}
static void test_interpolation_arithmetic(void){
    const UWORD values[]={1,2,17,255,4095,8192,16383};
    for(unsigned i=0;i<7;i++)for(unsigned j=0;j<7;j++){
        UWORD a=values[i],c=values[j],cuts[]={0,1,c/2,c-1,c};
        for(unsigned k=0;k<5;k++)expect(td_world_muldiv(a,cuts[k],c)==(uint32_t)a*cuts[k]/c,
                                       "bounded arithmetic agrees with independent32-bit oracle at limits");
    }
    uint32_t seed=0x527A31;
    for(unsigned i=0;i<10000;i++){
        seed=seed*1664525u+1013904223u;UWORD a=seed&16383;
        seed=seed*1664525u+1013904223u;UWORD c=seed%16383+1;
        seed=seed*1664525u+1013904223u;UWORD b=seed%(c+1);
        expect(td_world_muldiv(a,b,c)==(uint32_t)a*b/c,
               "bounded arithmetic preserves exact truncation over deterministic independent inputs");
    }
}
static void test_graphs(void){
    /* Nonlinear IDs, a cycle, a downstream foot-only link, and node5 isolated.
     * The near0→1 corridor cannot carry a car all the way to2. */
    const td_portal_t graph[]={
        {0,1,24,64,1000,64,1},{1,0,1000,64,24,64,1},
        {1,2,24,64,1000,64,0},{0,4,1000,600,24,600,1},
        {4,2,24,400,1000,400,1},{2,4,1000,400,24,400,1},
        {4,0,24,600,1000,600,1},{0,2,24,750,1000,750,0},
        {0,4,1000,200,24,200,1},{4,3,1000,288,24,288,1},
        {0,7,24,64,1000,64,1}, /* invalid endpoint must not index scratch */
    };
    td_portal_t out,prior;memset(&out,0xA5,sizeof(out));prior=out;
    expect(td_world_route_from(graph,11,6,0,2,0,500,64,500,64,&out)&&out.to==4,
           "car routing follows the real nonlinear branch and excludes downstream foot-only edge");
    expect(td_world_route_from(graph,11,6,0,2,1,500,64,500,64,&out)&&out.to==2,
           "walking selects the direct foot-only shortcut before longer feasible roads");
    expect(td_world_route_from(graph,11,6,0,2,0,500,600,500,600,&out)&&out.v==600,
           "equal-hop entrances choose the closest gate instead of the first generated record");
    expect(td_world_route_from(graph,11,6,2,0,0,500,400,500,600,&out)&&out.to==4,
           "cycle search terminates and directed return follows existing edges");
    out=prior;
    expect(!td_world_route_from(graph,11,6,0,5,1,500,64,500,64,&out)&&!memcmp(&out,&prior,sizeof(out)),
           "disconnected target leaves caller output unchanged");
    expect(!td_world_route_from(graph,11,6,0,0,1,500,64,500,64,&out),"same district needs no portal");
    expect(!td_world_route_from(graph,11,6,6,2,1,500,64,500,64,&out),"unknown origin is rejected");
    expect(!td_world_route_from(graph,11,6,0,6,1,500,64,500,64,&out),"unknown target is rejected");
    expect(!td_world_route_from(graph,11,33,0,2,1,500,64,500,64,&out),"oversized node pool is rejected");
    expect(!td_world_route_from(graph,513,6,0,2,1,500,64,500,64,&out),"oversized edge table is rejected before reading");
    expect(!td_world_route_from(NULL,11,6,0,2,1,500,64,500,64,&out),"missing metadata is rejected");
    expect(!td_world_route_from(graph,11,6,0,2,1,500,64,500,64,NULL),"null route output is rejected");
    expect(!td_world_route_from(graph,11,6,0,2,2,500,64,500,64,&out),"unknown movement mode is rejected");
    expect(!td_world_route_from(graph,11,6,0,2,1,1024,64,500,64,&out),"off-map source coordinates are rejected");
    expect(!td_world_route_from(graph,11,6,0,2,1,500,64,500,976,&out),"off-map target coordinates are rejected");
    td_portal_t chain[31];
    for(unsigned i=0;i<31;i++)chain[i]=(td_portal_t){i,i+1,24,128,1000,128,1};
    expect(td_world_route_from(chain,31,32,0,31,0,500,128,500,128,&out)&&out.to==1,
           "maximum32-node chain uses every bounded queue slot without overflow");
    expect(!td_world_route_from(chain,31,32,31,0,0,500,128,500,128,&out),
           "directed edges are not invented in reverse");
}

static void test_current_route_compatibility(void){
    td_portal_t out;
    for(unsigned from=0;from<3;from++)for(unsigned to=0;to<3;to++)if(from!=to)
        for(unsigned foot=0;foot<2;foot++)for(unsigned v=32;v<944;v+=37){
            unsigned next=from<to?from+1:from-1,best=65535;const td_portal_t *expected=NULL;
            /* Independent legacy behavior: adjacent ID and nearest street row. */
            for(unsigned i=0;i<TD_PORTALS;i++){
                const td_portal_t *p=&td_portals[i];
                if(p->from!=from||p->to!=next||(!foot&&!p->vehicle))continue;
                unsigned score=abs((int)v-p->v)+abs(608-(int)p->arrival_v);
                if(score<best){best=score;expected=p;}
            }
            expect(expected&&td_world_route(from,to,foot,500,v,784,608,&out)&&
                   !memcmp(&out,expected,sizeof(out)),"current three-district portal choice preserves legacy nearest-row behavior");
        }
}

static void test_registered_graph(void){
    td_portal_t portal;
    /* East's ID follows High Park numerically but its actual neighbor is core.
     * These registered routes catch a return to the old +/-1 ID assumption. */
    for(UBYTE foot=0;foot<2;foot++){
        expect(td_world_route(1,3,foot,500,528,500,528,&portal)&&portal.to==0,
               "West-to-East registered route first returns to Central Toronto");
        expect(td_world_route(3,2,foot,500,400,500,608,&portal)&&portal.to==0,
               "East-to-High-Park registered route follows its actual Central neighbor");
        expect(td_world_route(2,3,foot,500,240,500,400,&portal)&&portal.to==1,
               "High-Park-to-East registered route first uses West End");
        expect(td_world_route(0,3,foot,800,400,500,400,&portal)&&portal.to==3&&portal.u==1000,
               "Central-to-East registered route uses the east-facing boundary");
    }
    for(UWORD i=0;i<TD_PORTALS;i++){
        const td_portal_t *p=&td_portals[i];UBYTE horizontal=p->u==24||p->u==1000;
        UWORD u=p->u*16,v=p->v*16;
        UWORD old_u=u+(horizontal?(p->u==24?16:-16):0);
        UWORD old_v=v+(!horizontal?(p->v==24?16:-16):0);
        for(UBYTE foot=0;foot<2;foot++){
            td_crossing_t out,prior;memset(&out,0xA5,sizeof(out));prior=out;
            UBYTE permitted=foot||p->vehicle;
            UBYTE crossed=td_world_crossing(p->from,foot,old_u,old_v,u,v,&out);
            expect(crossed==permitted,"each registered directed seam honors its authored travel modes");
            if(permitted)expect(out.district==p->to&&out.u==p->arrival_u*16&&out.v==p->arrival_v*16,
                                "each registered directed seam reaches its exact authored destination");
            else expect(!memcmp(&out,&prior,sizeof(out)),"registered foot-only seam rejection preserves output");
        }
    }
    char name[19];
    expect(td_world_name(3,name)&&strcmp(name,"TORONTO EAST END")==0,
           "registered East district name is available through the banked metadata API");
}

static void test_crossings(void){
    const td_portal_t edges[]={
        {0,1,24,400,1000,400,1},{0,1,1000,400,24,400,1},
        {0,1,480,24,480,952,1},{0,1,480,952,480,24,1},
    };
    for(unsigned edge=0;edge<4;edge++)for(unsigned foot=0;foot<2;foot++){
        const td_portal_t *p=&edges[edge];int limit=(foot?28:18)*16;
        int horizontal=edge<2,negative=edge==0||edge==2;
        for(int offset=-limit;offset<=limit;offset++){
            UWORD u=p->u*16+(horizontal?0:offset),v=p->v*16+(horizontal?offset:0);
            UWORD old_u=u+(horizontal?(negative?16:-16):0),old_v=v+(!horizontal?(negative?16:-16):0);
            td_crossing_t result;
            expect(td_world_crossing_from(p,1,2,0,foot,old_u,old_v,u,v,&result)&&result.district==1&&
                   result.u==p->arrival_u*16+(horizontal?0:offset)&&result.v==p->arrival_v*16+(horizontal?offset:0),
                   "each Q4 lateral lane accepts all four outbound sides and preserves its offset");
        }
        td_crossing_t out,prior;memset(&out,0xA5,sizeof(out));prior=out;
        UWORD u=p->u*16+(horizontal?0:limit+1),v=p->v*16+(horizontal?limit+1:0);
        expect(!td_world_crossing_from(p,1,2,0,foot,u+(horizontal?(negative?16:-16):0),
                     v+(!horizontal?(negative?16:-16):0),u,v,&out)&&!memcmp(&out,&prior,sizeof(out)),
               "one Q4 unit outside lateral allowance fails without changing output");
        u=p->u*16;v=p->v*16;
        expect(!td_world_crossing_from(p,1,2,0,foot,u,v,u,v,&out),"stationary border position does not transition");
        expect(!td_world_crossing_from(p,1,2,0,foot,u+(horizontal?(negative?-16:16):0),
                     v+(!horizontal?(negative?-16:16):0),u,v,&out),"inbound movement never activates the outbound seam");
        expect(!td_world_crossing_from(p,1,2,0,foot,u+(horizontal?(negative?32:-32):0),
                     v+(!horizontal?(negative?32:-32):0),u+(horizontal?(negative?16:-16):0),
                     v+(!horizontal?(negative?16:-16):0),&out),"stopping before the trigger remains in source scene");
        expect(td_world_crossing_from(p,1,2,0,foot,u+(horizontal?(negative?-16:16):0),
                     v+(!horizontal?(negative?-16:16):0),u+(horizontal?(negative?-32:32):0),
                     v+(!horizontal?(negative?-32:32):0),&out),"already-outbound movement can retry a failed allocation");
    }
    td_portal_t p={0,1,24,200,1000,200,1};td_crossing_t out,prior;memset(&out,0xA5,sizeof(out));prior=out;
    expect(!td_world_crossing_from(&p,1,2,0,0,40*16,320*16,20*16,200*16,&out)&&!memcmp(&out,&prior,sizeof(out)),
           "diagonal sweep outside the portal at intersection cannot enter by ending near its row");
    expect(td_world_crossing_from(&p,1,2,0,1,40*16,320*16,20*16,200*16,&out),
           "same diagonal intersection inside wider walking lane remains permitted");
    p.v=p.arrival_v=920;
    expect(td_world_crossing_from(&p,1,2,0,0,1000*16,10*16,0,930*16,&out)&&out.v==930*16,
           "large bounded sweeps retain correct intersection without overflowing native16-bit intermediates");
    p.vehicle=0;out=prior;
    expect(!td_world_crossing_from(&p,1,2,0,0,25*16,920*16,24*16,920*16,&out)&&!memcmp(&out,&prior,sizeof(out)),
           "foot-only edge cannot carry a vehicle");
    expect(td_world_crossing_from(&p,1,2,0,1,25*16,920*16,24*16,920*16,&out),"walking uses foot-only crossing");
    expect(!td_world_crossing_from(&p,1,2,2,1,25*16,920*16,24*16,920*16,&out),"unknown crossing district rejected");
    expect(!td_world_crossing_from(&p,1,2,0,1,65535,920*16,24*16,920*16,&out),"large coordinates cannot wrap into a border");
    expect(!td_world_crossing_from(&p,1,2,0,1,25*16,920*16,24*16,920*16,NULL),"null crossing output rejected");
    p.arrival_u=24;out=prior;
    expect(!td_world_crossing_from(&p,1,2,0,1,25*16,920*16,24*16,920*16,&out)&&!memcmp(&out,&prior,sizeof(out)),
           "same-side seam metadata cannot silently reverse heading");
    p=(td_portal_t){0,1,480,24,480,952,1};
    expect(td_world_route_from(&p,1,2,0,1,0,480,200,480,600,&(td_portal_t){0}),
           "north-south metadata participates in the unchanged graph search");
    expect(td_world_crossing(1,1,25*16,896*16,24*16,896*16,&out)&&out.district==2,
           "registered waterfront foot-only portal uses public metadata API");
}

static void test_traffic_and_names(void){
    UWORD u[6],v[6];UBYTE legs[6];td_traffic_sample_t samples[6],prior[6];
    for(unsigned district=1;district<TD_DISTRICT_COUNT;district++){
        expect(td_world_traffic_init(district,u,v,legs,samples),"all six western actors initialize in one query");
        for(unsigned i=0;i<6;i++){
            expect(u[i]==td_west_traffic[district-1][i][0][0]*16&&v[i]==td_west_traffic[district-1][i][0][1]*16&&
                   legs[i]==1&&samples[i].count==td_west_traffic_counts[district-1][i],"initial pose and route count preserve generated traffic");
        }
        for(unsigned step=0;step<16;step++){
            for(unsigned i=0;i<6;i++)legs[i]=step%td_west_traffic_counts[district-1][i];
            expect(td_world_traffic_samples(district,legs,samples),"batched traffic query accepts all valid leg combinations");
            for(unsigned i=0;i<6;i++)expect(samples[i].u==td_west_traffic[district-1][i][legs[i]][0]*16&&
                  samples[i].v==td_west_traffic[district-1][i][legs[i]][1]*16,"batched targets cover closed-loop wrap and exact Q4 coordinates");
        }
    }
    memset(samples,0xA5,sizeof(samples));memcpy(prior,samples,sizeof(samples));memset(legs,0,sizeof(legs));legs[5]=255;
    expect(!td_world_traffic_samples(1,legs,samples)&&!memcmp(samples,prior,sizeof(samples)),
           "invalid final actor leg cannot partially replace earlier cached samples");
    expect(!td_world_traffic_samples(0,legs,samples),"core signal-specific loops stay with the driver");
    expect(!td_world_traffic_samples(TD_DISTRICT_COUNT,legs,samples),"unknown traffic district rejected");
    expect(!td_world_traffic_samples(1,NULL,samples)&&!td_world_traffic_samples(1,legs,NULL),"missing traffic query buffers rejected");
    expect(!td_world_traffic_init(1,NULL,v,legs,samples)&&!td_world_traffic_init(1,u,NULL,legs,samples)&&
           !td_world_traffic_init(1,u,v,NULL,samples)&&!td_world_traffic_init(1,u,v,legs,NULL),"missing traffic init buffers rejected");
    td_world_traffic_init(1,u,v,legs,samples);
    expect(u[0]==800*16&&v[0]==64*16&&samples[0].u==912*16&&samples[0].v==64*16&&samples[0].frame==0,
           "known west loop starts east with original vehicle frame");
    legs[0]=0;td_world_traffic_samples(1,legs,samples);
    expect(samples[0].frame==6,"wrapped loop leg0 faces north toward its starting point");
    char name[21];memset(name,0xA5,sizeof(name));
    expect(td_world_name(2,name)&&strcmp(name,"HIGH PARK/JUNCTION")==0&&(unsigned char)name[19]==0xA5,
           "district name copies exactly19 bytes with terminator and intact canary");
    char saved[21];memcpy(saved,name,sizeof(name));
    expect(!td_world_name(TD_DISTRICT_COUNT,name)&&!memcmp(name,saved,sizeof(name)),"unknown district preserves name buffer");
    expect(!td_world_name(0,NULL),"null name buffer rejected");
}
int main(void){
    test_interpolation_arithmetic();test_graphs();test_current_route_compatibility();test_registered_graph();test_crossings();test_traffic_and_names();
    printf("World navigation host regressions: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
