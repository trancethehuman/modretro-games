/* Actual compiled quest-navigation/ground-overlay C. Full grids, heap and
 * shortest-path scratch below belong only to this independent host oracle. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "navigation_host.h"
#include "td_game.h"
#include "td_navigation_data.h"
#include "navigation_fixture.h"
#define td_navigation_next td_navigation_uncached_next
#include "navigation_under_test.c"
#undef td_navigation_next
#include "guidance_under_test.c"

#define CELLS (128*122)
static unsigned checks,failures;
td_state_t td;
td_job_t td_job,td_offer;
td_stop_t td_target,td_cursor;
actor_t actors[22];
UBYTE VBK_REG;
WORD draw_scroll_x,draw_scroll_y;
static UBYTE loaded_district,boat_control;
static UBYTE bg_map[2][1024],bg_map_alt[2][1024],bg_page;
static unsigned navigation_queries,vram_writes;
UBYTE td_navigation_next(UBYTE district,UBYTE onfoot,UWORD goal_u,UWORD goal_v,
    UWORD u,UWORD v,td_navigation_waypoint_t *out){
    navigation_queries++;
    return td_navigation_uncached_next(district,onfoot,goal_u,goal_v,u,v,out);
}
UBYTE td_district_current(void){return loaded_district;}
UBYTE td_boats_controlled(void){return boat_control;}
UBYTE *GetBkgAddr(void){return (UBYTE*)(uintptr_t)(bg_page?0x9c00:0x9800);}
static UBYTE *vram_byte(UBYTE *p){
    uintptr_t address=(uintptr_t)p;
    if(address>=0x9800&&address<0x9c00)return &bg_map[VBK_REG&1][address-0x9800];
    if(address>=0x9c00&&address<0xa000)return &bg_map_alt[VBK_REG&1][address-0x9c00];
    fprintf(stderr,"Native overlay accessed an invalid VRAM map address\n");exit(2);
}
UBYTE get_vram_byte(UBYTE *p){return *vram_byte(p);}
void set_vram_byte(UBYTE *p,UBYTE v){vram_writes++;*vram_byte(p)=v;}
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *data){(void)first;(void)count;(void)data;}

static void expect(int condition,const char *name){
    checks++;
    if(!condition){failures++;if(failures<25)fprintf(stderr,"FAIL %s\n",name);}
}
static unsigned distance(unsigned a,unsigned b){return a>b?a-b:b-a;}
static int body(unsigned district,unsigned u,unsigned v,unsigned half){
    if(district>=7||u<8||u>1016||v<8||v>968)return 0;
    for(unsigned y=(v-half)/8;y<=(v+half)/8;y++)
        for(unsigned x=(u-half)/8;x<=(u+half)/8;x++)
            if(oracle_grids[district][y*128+x]&15)return 0;
    return 1;
}
static int neighbour(unsigned index,unsigned direction){
    unsigned x=index%128,y=index/128;
    if(direction==1)return y?(int)index-128:-1;
    if(direction==2)return x<127?(int)index+1:-1;
    if(direction==3)return y<121?(int)index+128:-1;
    if(direction==4)return x?(int)index-1:-1;
    return -1;
}
typedef struct {unsigned position,cost;} heap_entry_t;
static heap_entry_t heap[CELLS*4];
static unsigned heap_count;
static void push(unsigned position,unsigned cost){
    if(heap_count>=CELLS*4){fprintf(stderr,"Oracle heap capacity exceeded\n");exit(2);}
    unsigned child=heap_count++;
    while(child){unsigned parent=(child-1)/2;if(heap[parent].cost<=cost)break;
        heap[child]=heap[parent];child=parent;}
    heap[child]=(heap_entry_t){position,cost};
}
static heap_entry_t pop(void){
    heap_entry_t first=heap[0],last=heap[--heap_count];unsigned parent=0;
    while(parent*2+1<heap_count){unsigned child=parent*2+1;
        if(child+1<heap_count&&heap[child+1].cost<heap[child].cost)child++;
        if(heap[child].cost>=last.cost)break;
        heap[parent]=heap[child];parent=child;}
    if(heap_count)heap[parent]=last;
    return first;
}
static UBYTE legal[CELLS],directions[CELLS];
static unsigned costs[CELLS];

static void verify_ordinals(void){
    /* This running count uses only raw collision grids and the full body,
       never generated masks, prefixes, row runs or navigation directions. */
    for(unsigned district=0;district<7;district++)for(unsigned onfoot=0;onfoot<2;onfoot++)
        for(unsigned y=0;y<122;y++){
            unsigned ordinal=0;
            for(unsigned x=0;x<128;x++){
                int usable=body(district,x*8+4,y*8+4,onfoot?3:7);
                expect(td_navigation_ordinal(district,onfoot,x,y)==(usable?ordinal:255),
                       "Every exact ROM ordinal equals the independent full-body running count");
                ordinal+=usable;
            }
        }
}

static void verify_field(unsigned goal,unsigned onfoot){
    const td_navigation_goal_t *g=&td_navigation_goals[goal];
    unsigned terminal=CELLS,terminals=0,half=onfoot?3:7;
    for(unsigned i=0;i<CELLS;i++){
        unsigned u=(i%128)*8+4,v=(i/128)*8+4;
        legal[i]=body(g->district,u,v,half);
        directions[i]=td_navigation_cell(goal,onfoot,i%128,i/128);
        expect(directions[i]<=5,"ROM direction is bounded before any neighbour arithmetic");
        expect(!directions[i]||legal[i],"Every road/foot flow cell retains its complete collision body");
        if(directions[i]==5){terminal=i;terminals++;
            expect(distance(u,g->u)<15&&distance(v,g->v)<15,
                   "Terminal fits the unchanged native stop interaction radius");}
        costs[i]=65535;
        if(i%97==0){
            expect(td_navigation_direction(g->district,onfoot,g->u,g->v,u,v)==directions[i],
                   "Public whole-pixel goal lookup matches exact decoded ROM direction");
            td_navigation_waypoint_t out={65000,64000,255};
            unsigned okay=td_navigation_next(g->district,onfoot,g->u,g->v,u,v,&out);
            expect(okay==!!directions[i],"Bounded lookahead succeeds exactly for reachable flow cells");
            if(okay){
                unsigned end=out.v/8*128+out.u/8;
                expect(out.u%8==4&&out.v%8==4&&body(g->district,out.u,out.v,half),
                       "Ground indicator is a usable native tile centre with the correct full body");
                expect(distance(u,out.u)+distance(v,out.v)<=32,
                       "Indicator lookahead stays within four exact eight-pixel flow steps");
                expect(out.direction==td_navigation_cell(goal,onfoot,out.u/8,out.v/8),
                       "Indicator orientation shows the actual upcoming turn at its own ground tile");
                expect(end<CELLS,"Waypoint never wraps outside the registered scene");
            }else expect(out.u==65000&&out.v==64000&&out.direction==255,
                        "Failed guidance preserves caller output completely");
        }
    }
    expect(terminals<=1,"Exactly one terminal policy cell exists when this body can reach the handoff");
    unsigned candidates=0;
    for(unsigned i=0;i<CELLS;i++)if(legal[i]){
        unsigned u=(i%128)*8+4,v=(i/128)*8+4;
        if(distance(u,g->u)>=15||distance(v,g->v)>=15)continue;
        int connected=1;
        for(unsigned y=(v<g->v?v:g->v)/8;y<=(v>g->v?v:g->v)/8;y++)
            for(unsigned x=(u<g->u?u:g->u)/8;x<=(u>g->u?u:g->u)/8;x++)
                if(oracle_grids[g->district][y*128+x]&15)connected=0;
        candidates+=connected;
    }
    expect(terminals==!!candidates,"Every body-feasible registered handoff has a terminal, rather than an empty generated field");
    if(!terminals){for(unsigned i=0;i<CELLS;i++)expect(!directions[i],"An unusable handoff creates no phantom route");return;}
    /* Independent full-grid Dijkstra from the real compiled terminal, with
     * the declared car asphalt preference. It does not decode generated
     * potential data or reuse the Python field algorithm. */
    heap_count=0;costs[terminal]=0;push(terminal,0);
    while(heap_count){
        heap_entry_t current=pop();if(current.cost!=costs[current.position])continue;
        unsigned step=onfoot||oracle_grids[g->district][current.position]==0?1:8;
        for(unsigned direction=1;direction<=4;direction++){
            int other=neighbour(current.position,direction);if(other<0||!legal[other])continue;
            unsigned candidate=current.cost+step;
            if(candidate<costs[other]){costs[other]=candidate;push(other,candidate);}
        }
    }
    for(unsigned i=0;i<CELLS;i++){
        expect(!!directions[i]==(costs[i]!=65535),"Every connected usable cell has guidance, every disconnected cell is withheld");
        if(directions[i]>=1&&directions[i]<=4){
            int other=neighbour(i,directions[i]);
            expect(other>=0,"Cardinal routing never wraps a row or scene edge");
            if(other>=0){unsigned step=onfoot||oracle_grids[g->district][other]==0?1:8;
                expect(costs[other]+step==costs[i],"Every actual ROM direction follows a minimum-cost collision-backed path");
                unsigned x=i%128,y=i/128,nx=other%128,ny=other/128;
                for(unsigned pixel=0;pixel<=8;pixel++){
                    unsigned u=x*8+4+(int)(nx-x)*(int)pixel;
                    unsigned v=y*8+4+(int)(ny-y)*(int)pixel;
                    expect(body(g->district,u,v,half),"Every intervening pixel of the cardinal full-body route clears buildings/water/rails");
                }
            }
        }
    }
}

static void verify_invalid(void){
    td_navigation_waypoint_t out={65000,64000,255};
    expect(!td_navigation_next(255,0,0,0,0,0,&out)&&out.u==65000&&out.v==64000&&out.direction==255,
           "Unknown districts preserve output");
    expect(!td_navigation_next(0,0,1,1,320,640,&out),"Unknown goal cannot silently become another quest");
    expect(!td_navigation_next(0,2,320,640,320,640,&out),"Invalid travel mode is withheld");
    expect(!td_navigation_next(0,0,320,640,65535,65535,&out),"Extreme coordinates are withheld without overflow");
    expect(!td_navigation_next(0,0,320,640,320,640,NULL),"Null output is rejected");
    expect(!td_navigation_cell(65535,0,0,0)&&!td_navigation_cell(0,255,0,0)&&
           !td_navigation_cell(0,0,255,0)&&!td_navigation_cell(0,0,0,255),
           "Private compiled-field boundary checks precede every ROM read");
    verify_private_decoder_bounds();
}

static void verify_overlay(void){
    /* The actual first courier route starts on a road and turns around
     * registered building geometry, without replacing the final beacon. */
    const td_navigation_goal_t *g=NULL;unsigned start=0;
    for(unsigned goal=0;goal<TD_NAVIGATION_GOALS&&!g;goal++)if(td_navigation_goals[goal].district==0)
        for(unsigned i=0;i<CELLS;i++){
            UBYTE direction=td_navigation_cell(goal,0,i%128,i/128);
            if(direction>=1&&direction<=4&&i%128>=12&&i/128>=12){g=&td_navigation_goals[goal];start=i;break;}
        }
    expect(g!=NULL,"Overlay uses a genuine compiled road route");if(!g)return;
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    td.mode=TD_ROAM;td.job=0;td.district=loaded_district=0;
    td.u=(start%128*8+4)*16;td.v=(start/128*8+4)*16;
    draw_scroll_x=(td.u>>4)-80;draw_scroll_y=(td.v>>4)-64;
    actors[1].pos.x=g->u*32;actors[1].pos.y=(g->v-12)*32;
    actor_t original=actors[1];td_navigation_waypoint_t waypoint;
    expect(td_navigation_next(0,0,g->u,g->v,td.u>>4,td.v>>4,&waypoint),"Overlay's road lookahead exists");
    unsigned offset=((waypoint.v/8&31)<<5)|(waypoint.u/8&31);
    memset(bg_map[0],42,sizeof(bg_map[0]));memset(bg_map[1],0,sizeof(bg_map[1]));
    td_guidance_road_reset();VBK_REG=1;td_guidance_road_render();
    expect(td_road_arrow_active&&bg_map[0][offset]==240+waypoint.direction&&bg_map[1][offset]==15,
           "A real active quest paints the original oriented ground arrow ahead of the courier");
    expect(VBK_REG==1&&!memcmp(&actors[1],&original,sizeof(original)),"Overlay retains ROM bank selection and exact destination beacon");
    td_guidance_road_restore();
    expect(bg_map[0][offset]==42&&bg_map[1][offset]==0&&!td_road_arrow_active&&VBK_REG==1,
           "Ground arrow restores both original underlay bytes and caller VRAM bank");
    td_guidance_road_render();bg_map[0][offset]=80;bg_map[1][offset]=8;
    td_guidance_road_restore();
    expect(bg_map[0][offset]==80&&bg_map[1][offset]==8,"A later prop/aircraft/scroll overlay retains ownership");
    for(unsigned attr=0;attr<256;attr++)if(attr&136){
        bg_map[0][offset]=42;bg_map[1][offset]=attr;td_guidance_road_render();
        expect(!td_road_arrow_active&&bg_map[0][offset]==42&&bg_map[1][offset]==attr,
               "Every roof-priority or bank1 owner blocks guidance without any tile write");
    }
    bg_map[1][offset]=0;
    for(unsigned mode=1;mode<=8;mode++){
        td.mode=mode;td_guidance_road_render();expect(!td_road_arrow_active,"Menus, transit and map withhold the active road arrow");
    }
    td.mode=0;td.job=TD_NONE;td_guidance_road_render();expect(!td_road_arrow_active,"Free roaming has no stale quest arrow");
    td.job=0;boat_control=1;td_guidance_road_render();expect(!td_road_arrow_active,"Controllable watercraft do not receive a land-road arrow");
    boat_control=0;loaded_district=1;td_guidance_road_render();expect(!td_road_arrow_active,"An unloaded destination/scene cannot paint a road arrow");
    loaded_district=0;actors[1].flags=ACTOR_FLAG_HIDDEN;td_guidance_road_render();expect(!td_road_arrow_active,"Unreachable hidden beacon cannot produce guidance");
    actors[1].flags=0;td_guidance_road_render();td_guidance_road_reset();bg_map[0][offset]=77;td_guidance_road_restore();
    expect(bg_map[0][offset]==77,"Scene initialization discards prior VRAM ownership before restoration");
    expect(sizeof(td_road_arrow_offset)+sizeof(td_road_arrow_tile)+sizeof(td_road_arrow_attr)+sizeof(td_road_arrow_active)==5,
           "Ground overlay owns exactly five transient bytes and no actor/save expansion");
    for(unsigned page=0;page<2;page++){
        memset(bg_map[0],42,sizeof(bg_map[0]));memset(bg_map[1],0,sizeof(bg_map[1]));
        memset(bg_map_alt[0],53,sizeof(bg_map_alt[0]));memset(bg_map_alt[1],0,sizeof(bg_map_alt[1]));
        bg_page=page;td_guidance_road_reset();td_guidance_road_render();
        expect(td_road_arrow_active&&!!(td_road_arrow_offset&1024)==page,
               "Ground patch captures its original LCDC page in the existing offset word");
        UBYTE (*other)[1024]=page?bg_map:bg_map_alt;
        other[0][offset]=240+waypoint.direction;other[1][offset]=15;
        bg_page=!page;td_guidance_road_restore();
        UBYTE (*original_map)[1024]=page?bg_map_alt:bg_map;
        expect(original_map[0][offset]==(page?53:42)&&original_map[1][offset]==0,
               "Page changes restore the captured ground map, rather than the currently displayed page");
        expect(other[0][offset]==240+waypoint.direction&&other[1][offset]==15,
               "Identical arrow-looking content on the other page never grants restoration ownership");
        bg_page=page;td_guidance_road_restore();
        expect(original_map[0][offset]==(page?53:42),"Returning to the original page cannot replay an already consumed patch");
    }
    bg_page=0;
}

static void verify_route_cache(void){
    memset(&td,0,sizeof(td));td.district=loaded_district=0;td.mode=TD_ROAM;td.job=0;
    td.u=583*16;td.v=720*16;
    td_navigation_waypoint_t expected;
    expect(td_navigation_uncached_next(0,0,768,720,583,720,&expected),"Cache fixture uses the real first delivery road route");
    td_guidance_road_reset();unsigned before=navigation_queries;
    expect(td_guidance_road_plan(0,768,720)&&navigation_queries==before+1,
           "The first cache key computes exactly one actual ROM waypoint");
    expect(td_road_route_waypoint.u==expected.u&&td_road_route_waypoint.v==expected.v&&
           td_road_route_waypoint.direction==expected.direction,"Cached waypoint is byte-equivalent in every meaningful field");
    unsigned base_u=td.u&~127u,base_v=td.v&~127u;
    for(unsigned u=0;u<128;u++)for(unsigned v=0;v<128;v++){
        td.u=base_u+u;td.v=base_v+v;
        expect(td_guidance_road_plan(0,768,720)&&navigation_queries==before+1,
               "Every Q4 fraction and whole-pixel deviation inside one eight-pixel cell reuses the exact route");
        expect(td_road_route_waypoint.u==expected.u&&td_road_route_waypoint.v==expected.v&&
               td_road_route_waypoint.direction==expected.direction,"Fractional movement cannot alter cached lookahead output");
    }
    td.u=base_u+128;expect(td_guidance_road_plan(0,768,720)&&navigation_queries==before+2,
                         "Crossing the horizontal eight-pixel boundary computes fresh road advice");
    td.v=base_v+128;expect(td_guidance_road_plan(0,768,720)&&navigation_queries==before+3,
                         "Crossing the vertical eight-pixel boundary computes fresh road advice");
    td.onfoot=1;td_guidance_road_plan(0,768,720);expect(navigation_queries==before+4,
                                                    "Walking mode replaces car full-body route advice immediately");
    td_guidance_road_plan(0,769,720);expect(navigation_queries==before+5&&!td_road_route_waypoint.direction,
                                         "Exact whole-pixel goal change caches failure instead of retaining the previous route");
    td_guidance_road_plan(0,769,720);expect(navigation_queries==before+5,"Repeated unknown goals reuse a cached false result");
    td_guidance_road_plan(0,769,721);expect(navigation_queries==before+6,"Goal vertical coordinate participates in the exact cache key");
    td_guidance_road_plan(1,769,721);expect(navigation_queries==before+7,"District changes invalidate otherwise identical local coordinates");
    td_guidance_road_reset();td_guidance_road_plan(1,769,721);expect(navigation_queries==before+8,
                                                                "Scene reset discards successful and failed cached queries");
    td.u=65000;td.v=65000;expect(!td_guidance_road_plan(1,769,721)&&navigation_queries==before+8,
                               "Malformed Q4 coordinates are rejected before truncated cell keys could alias a valid location");
    td.u=583*16;td.v=720*16;td.onfoot=0;
    td_guidance_road_plan(0,768,720);expect(navigation_queries==before+9,"Returning to a genuine valid road key refreshes after an invalid world position");
    td.u=760*16;td.v=712*16;td_guidance_road_plan(0,768,720);
    unsigned arrived=navigation_queries;
    for(unsigned y=710;y<731;y++)for(unsigned x=758;x<779;x++){
        if(td_navigation_uncached_next(0,0,768,720,x,y,&expected)&&expected.direction==5){
            td.u=x*16;td.v=y*16;td_guidance_road_plan(0,768,720);arrived=navigation_queries;
            expect(td_road_route_waypoint.direction==5&&!td_guidance_road_plan(0,768,720)&&navigation_queries==arrived,
                   "Arrival is cached exactly and withholds a misleading directional road arrow");
            x=779;y=731;
        }
    }
    expect(sizeof(td_road_route_key)==8&&sizeof(td_road_route_valid)==1,
           "Exact route key and validity byte match their native memory contract");
}

static unsigned retention_fixture(void){
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    memset(bg_map[0],52,sizeof(bg_map[0]));memset(bg_map[1],2,sizeof(bg_map[1]));
    memset(bg_map_alt[0],63,sizeof(bg_map_alt[0]));memset(bg_map_alt[1],6,sizeof(bg_map_alt[1]));
    td.district=loaded_district=0;td.mode=TD_ROAM;td.job=0;boat_control=bg_page=0;
    td.u=583*16;td.v=720*16;
    draw_scroll_x=503;draw_scroll_y=656;
    actors[1].pos.x=768*32;actors[1].pos.y=(720-12)*32;
    td_guidance_road_reset();VBK_REG=1;td_guidance_road_render();
    expect(td_road_arrow_active==1,"Retention fixture captures the exact active first-job tag");
    expect(td_road_arrow_tile==52&&td_road_arrow_attr==2,"Retention fixture keeps the original ground underlay");
    return td_road_arrow_offset&1023;
}

static void verify_patch_retention(void){
    unsigned offset=retention_fixture(),before=navigation_queries,writes=vram_writes;
    td_state_t state=td;actor_t marker=actors[1];
    for(unsigned frame=0;frame<500;frame++){
        td_guidance_road_prepare();td_guidance_road_render();
        expect(td_road_arrow_active&&bg_map[0][offset]==240+td_road_route_waypoint.direction&&bg_map[1][offset]==15,
               "Every unchanged ordinary frame retains its complete road arrow");
        expect(td_road_arrow_tile==52&&td_road_arrow_attr==2&&VBK_REG==1,
               "Retained rendering never captures an arrow as its own underlay or changes the caller bank");
        expect(navigation_queries==before&&vram_writes==writes,
               "Unchanged prepare/render performs no ROM navigation recomputation or VRAM rewrite");
    }
    expect(!memcmp(&td,&state,sizeof(td))&&!memcmp(&actors[1],&marker,sizeof(marker)),
           "Road retention preserves every gameplay and marker field");
    unsigned base_u=td.u&~127u,base_v=td.v&~127u;
    for(unsigned u=0;u<128;u++)for(unsigned v=0;v<128;v++){
        td.u=base_u+u;td.v=base_v+v;td_guidance_road_prepare();td_guidance_road_render();
        expect(td_road_arrow_active&&navigation_queries==before&&vram_writes==writes&&
               td_road_arrow_tile==52&&td_road_arrow_attr==2,
               "Every fractional/whole-pixel position in the unchanged cell retains exact ground without any rewrite");
    }
    td_guidance_road_restore();
    expect(!td_road_arrow_active&&bg_map[0][offset]==52&&bg_map[1][offset]==2,
           "Explicit modal restoration always releases a retained patch to its original ground");
    for(unsigned mode=1;mode<=TD_DIALOG;mode++){
        offset=retention_fixture();td.mode=mode;before=navigation_queries;td_guidance_road_prepare();
        expect(!td_road_arrow_active&&bg_map[0][offset]==52&&bg_map[1][offset]==2&&navigation_queries==before,
               "Every non-roaming mode force-releases retained guidance without a ROM query");
    }

    /* Every validity condition independently releases the prior patch before
       modal/transition/changed-route reuse, without querying the ROM route. */
    for(unsigned change=0;change<17;change++){
        offset=retention_fixture();before=navigation_queries;
        switch(change){
            case 0:td.mode=TD_PAUSE;break;
            case 1:td.job=TD_NONE;break;
            case 2:td.job=1;break;
            case 3:actors[1].flags|=ACTOR_FLAG_HIDDEN;break;
            case 4:boat_control=1;break;
            case 5:td_road_route_valid=0;break;
            case 6:td.u=1024*16;break;
            case 7:td.v=976*16;break;
            case 8:actors[1].pos.x+=32;break;
            case 9:actors[1].pos.y+=32;break;
            case 10:td.onfoot=1;break;
            case 11:td.u+=128;break;
            case 12:td.v+=128;break;
            case 13:td_road_route_waypoint.direction=5;break;
            case 14:loaded_district=1;break;
            case 15:td.district=1;break;
            case 16:td_road_route_key.district=1;break;
        }
        td_guidance_road_prepare();
        expect(!td_road_arrow_active&&bg_map[0][offset]==52&&bg_map[1][offset]==2,
               "Every changed context/key independently releases the exact old patch");
        expect(navigation_queries==before&&VBK_REG==1,
               "Preparation never recomputes ROM directions and retains the native caller bank");
    }

    offset=retention_fixture();td.job=1;td_guidance_road_prepare();td_guidance_road_render();
    expect(td_road_arrow_active==2&&td_road_arrow_tile==52,
           "A new job with the same goal captures its own tag without another persistent byte");
    td_guidance_road_restore();

    offset=retention_fixture();before=navigation_queries;
    bg_page=1;bg_map_alt[0][offset]=240+td_road_route_waypoint.direction;bg_map_alt[1][offset]=15;
    td_guidance_road_prepare();
    expect(!td_road_arrow_active&&bg_map[0][offset]==52&&bg_map[1][offset]==2&&
           bg_map_alt[0][offset]==240+td_road_route_waypoint.direction&&bg_map_alt[1][offset]==15,
           "A changed map page restores only the old owned patch, preserving identical-looking other-page content");
    expect(navigation_queries==before,"Page ownership checks do not search navigation ROM");

    offset=retention_fixture();bg_map[0][offset]=61;bg_map[1][offset]=3;
    writes=vram_writes;td_guidance_road_prepare();
    expect(!td_road_arrow_active&&vram_writes==writes&&bg_map[0][offset]==61&&bg_map[1][offset]==3,
           "A fresh scroll underlay wins without any replay of the old captured ground");
    td_guidance_road_render();
    expect(td_road_arrow_active&&td_road_arrow_tile==61&&td_road_arrow_attr==3,
           "The next render captures fresh scroll ground instead of the previous arrow or stale underlay");
    td_guidance_road_restore();
    expect(bg_map[0][offset]==61&&bg_map[1][offset]==3,"Fresh scroll ground survives subsequent forced restoration");

    for(unsigned attr=0;attr<256;attr++)if(attr&136){
        offset=retention_fixture();bg_map[0][offset]=80;bg_map[1][offset]=attr;
        writes=vram_writes;td_guidance_road_prepare();td_guidance_road_render();
        expect(!td_road_arrow_active&&vram_writes==writes&&bg_map[0][offset]==80&&bg_map[1][offset]==attr,
               "Every later roof or bank1 overlay displaces retention without any destructive rewrite");
    }

    offset=retention_fixture();draw_scroll_x=2000;td_guidance_road_render();
    expect(!td_road_arrow_active&&bg_map[0][offset]==52&&bg_map[1][offset]==2,
           "Camera scrolling outside the unchanged waypoint's visible range withdraws its old patch");
    offset=retention_fixture();td_guidance_road_restore();
    /* Other dynamic writers force release before replacing a retained tile.
       A destruction overlay therefore records ground, never an arrow glyph. */
    UBYTE prop_underlay=bg_map[0][offset],prop_attr=bg_map[1][offset];
    bg_map[0][offset]=80;bg_map[1][offset]=10;td_guidance_road_render();
    expect(prop_underlay==52&&prop_attr==2&&!td_road_arrow_active,
           "A newly broken prop can capture only the released original ground, never a retained arrow");
    bg_map[0][offset]=prop_underlay;bg_map[1][offset]=prop_attr;
    expect(bg_map[0][offset]==52&&bg_map[1][offset]==2,"Destruction-overlay restoration cannot orphan an arrow");
}

int main(void){
    verify_invalid();
    verify_ordinals();
    for(unsigned goal=0;goal<TD_NAVIGATION_GOALS;goal++)for(unsigned onfoot=0;onfoot<2;onfoot++)verify_field(goal,onfoot);
    verify_overlay();
    verify_route_cache();
    verify_patch_retention();
    printf("Quest navigation: %u assertions, %u failures; all107 goals, both body modes, native C flow/overlay\n",checks,failures);
    return failures?1:0;
}
