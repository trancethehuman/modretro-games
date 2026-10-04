#include <stdio.h>
#include <string.h>
#include "td_traffic_lights.h"
#include "td_game.h"
#include "td_district.h"
#define TD_TRAFFIC_RENDER_FIXTURE
#include "traffic_fixture.h"

td_state_t td;
WORD draw_scroll_x,draw_scroll_y;
UBYTE VBK_REG;
static UBYTE district,map[2][1024],patterns[2][256][16];
static unsigned uploads,writes,reads,checks,failures;
UBYTE td_district_current(void){return district;}
UBYTE *GetBkgAddr(void){return map[0];}
UBYTE get_vram_byte(UBYTE *address){
    size_t offset=(size_t)(address-map[0]);
    if(offset>=1024){fprintf(stderr,"Invalid signal map read\n");failures++;return 0;}
    reads++;return map[VBK_REG&1][offset];
}
void set_vram_byte(UBYTE *address,UBYTE value){
    size_t offset=(size_t)(address-map[0]);
    if(offset>=1024){fprintf(stderr,"Invalid signal map address\n");failures++;return;}
    map[VBK_REG&1][offset]=value;writes++;
}
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *data){
    if(first!=47||count!=2||VBK_REG!=1){fprintf(stderr,"Invalid signal VRAM allocation\n");failures++;}
    memcpy(patterns[VBK_REG&1][first],data,count*16);uploads++;
}
static void expect(int ok,const char *message){checks++;if(!ok){if(failures<15)fprintf(stderr,"FAIL: %s\n",message);failures++;}}
static void check_view(void){
    UBYTE expected[2][1024],old_patterns[2][256][16];unsigned visible=0;
    memset(map[0],77,1024);memset(map[1],128,1024);memset(patterns,59,sizeof(patterns));
    td_traffic_lights_reset();
    memcpy(expected,map,sizeof(map));memcpy(old_patterns,patterns,sizeof(patterns));
    uploads=writes=reads=0;td_state_t before=td;UBYTE vbk=VBK_REG;
    int active=(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE)&&district<TD_DISTRICT_COUNT&&
        draw_scroll_x<1024&&draw_scroll_y<976&&draw_scroll_x>=-160&&draw_scroll_y>=-144;
    if(active)for(unsigned i=oracle_offsets[district];i<oracle_offsets[district+1];i++){
        const unsigned *node=oracle_lights[i];int x=node[3]*8,y=node[4]*8;
        if(x+8<=draw_scroll_x||x>=draw_scroll_x+160||y+8<=draw_scroll_y||y>=draw_scroll_y+144)continue;
        unsigned offset=(node[4]%32)*32+node[3]%32;
        expected[0][offset]=td.seconds%12<7?47:48;expected[1][offset]=15;visible++;
    }
    td_traffic_lights_render();
    expect(!memcmp(map,expected,sizeof(map)),"actual signal cells/phase/attributes match independent viewport and32tile ring oracle");
    expect(uploads==(visible?1u:0u)&&writes==visible*2,"signal renderer uploads two reserved patterns once only when a corner is visible");
    expect(reads==visible*2,"renderer checks actual bank0 tile and bank1 attribute for every visible corner");
    expect(VBK_REG==vbk&&!memcmp(&td,&before,58),"signal render preserves VRAM bank and every saved gameplay field");
    if(visible){
        for(unsigned phase=0;phase<2;phase++){
            const UBYTE *tile=patterns[1][47+phase];
            unsigned horizontal=((tile[2]>>6)&1)|(((tile[3]>>6)&1)<<1);
            unsigned vertical=((tile[2]>>2)&1)|(((tile[3]>>2)&1)<<1);
            expect(horizontal==(phase?2u:1u)&&vertical==(phase?1u:2u),"original horizontal and vertical lamps show opposing green/red indices");
        }
        memset(patterns[1][47],59,32);
    }
    expect(!memcmp(patterns,old_patterns,sizeof(patterns)),"signal render touches no OBJ, aircraft scratch, font or other background patterns");
}
static unsigned visible_cells(UBYTE *cells){
    unsigned count=0;memset(cells,0,1024);
    for(unsigned i=oracle_offsets[district];i<oracle_offsets[district+1];i++){
        const unsigned *node=oracle_lights[i];int x=node[3]*8,y=node[4]*8;
        if(x+8<=draw_scroll_x||x>=draw_scroll_x+160||y+8<=draw_scroll_y||y>=draw_scroll_y+144)continue;
        unsigned offset=(node[4]%32)*32+node[3]%32;cells[offset]=1;count++;
    }
    return count;
}
static void lifecycle(void){
    UBYTE cells[1024],before_map[2][1024],good_patterns[32];unsigned count,first=0;
    district=0;td.mode=TD_ROAM;td.seconds=6;VBK_REG=1;
    draw_scroll_x=(int)oracle_lights[0][3]*8-80;draw_scroll_y=(int)oracle_lights[0][4]*8-72;
    memset(map[0],77,1024);memset(map[1],128,1024);memset(patterns,59,sizeof(patterns));
    count=visible_cells(cells);expect(count>0,"Lifecycle viewport has actual registered corners");
    while(first<1024&&!cells[first])first++;
    if(first==1024)return;
    td_traffic_lights_reset();uploads=writes=reads=0;
    td_state_t before=td;td_traffic_lights_render();
    expect(uploads==1&&writes==count*2,"Initial visible frame uploads patterns and repairs both fields");
    memcpy(before_map,map,sizeof(map));memcpy(good_patterns,patterns[1][47],32);
    uploads=writes=reads=0;
    for(unsigned repeat=0;repeat<8;repeat++)td_traffic_lights_render();
    expect(!uploads&&!writes&&reads==count*2*8,"Stationary frames read actual VRAM and perform zero redundant writes/uploads");
    expect(!memcmp(map,before_map,sizeof(map))&&!memcmp(patterns[1][47],good_patterns,32),"Unchanged visible cells and patterns survive cached frames");
    expect(VBK_REG==1&&!memcmp(&td,&before,sizeof(td)),"Cached rendering preserves caller bank and complete gameplay state");
    td.seconds=7;uploads=writes=reads=0;td_traffic_lights_render();
    expect(!uploads&&writes==count&&reads==count*2,"Phase boundary updates only bank0 corner tile IDs");
    expect(!memcmp(map[1],before_map[1],1024),"Phase-only change preserves all bank1 attributes");
    for(unsigned offset=0;offset<1024;offset++)
        expect(map[0][offset]==(cells[offset]?48:before_map[0][offset]),"Phase flip preserves every unrelated ring cell");
    map[1][first]=140;uploads=writes=reads=0;td_traffic_lights_render();
    expect(!uploads&&writes==1&&map[1][first]==15&&map[0][first]==48,"Scroll/repaint of only an attribute is repaired exactly once");
    map[0][first]=91;uploads=writes=reads=0;td_traffic_lights_render();
    expect(!uploads&&writes==1&&map[0][first]==48&&map[1][first]==15,"Scroll/repaint of only a tile is repaired exactly once");
    memcpy(before_map,map,sizeof(map));
    /* MAP may suppress actor rendering completely. Atlas lifecycle explicitly
     * resets the pattern cache even with no inactive-render callback. */
    td.mode=TD_MAP;memset(patterns[1][47],153,32);
    uploads=writes=reads=0;td_traffic_lights_reset();
    expect(!uploads&&!writes&&!reads,"Explicit atlas reset itself does not mutate VRAM");
    td.mode=TD_ROAM;td_traffic_lights_render();
    expect(uploads==1&&!writes&&!memcmp(patterns[1][47],good_patterns,32),"Atlas overwrite with no actor frames is restored after explicit lifecycle reset");
    expect(!memcmp(map,before_map,sizeof(map))&&VBK_REG==1,"Atlas return keeps correct existing cells and restores VBK");
    td.mode=TD_MAP;memset(patterns[1][47],29,32);uploads=writes=reads=0;td_traffic_lights_render();
    expect(!uploads&&!writes&&!reads&&patterns[1][47][0]==29,"Inactive render leaves atlas VRAM untouched and invalidates cache");
    td.mode=TD_WAIT;td_traffic_lights_render();
    expect(uploads==1&&!writes&&!memcmp(patterns[1][47],good_patterns,32),"Waiting resume restores patterns after inactive rendering");
    /* A graphics reload can leave exactly the same scene/district identity.
     * The explicit reset, not identity comparison, authorizes cache reuse. */
    memset(patterns[1][47],67,32);uploads=writes=reads=0;td_traffic_lights_reset();td_traffic_lights_render();
    expect(uploads==1&&!writes&&!memcmp(patterns[1][47],good_patterns,32),"Same-scene graphics reload restores patterns after reset");
    district=1;draw_scroll_x=(int)oracle_lights[oracle_offsets[1]][3]*8-80;
    draw_scroll_y=(int)oracle_lights[oracle_offsets[1]][4]*8-72;count=visible_cells(cells);
    memset(map[0],77,1024);memset(map[1],128,1024);memset(patterns[1][47],41,32);
    uploads=writes=reads=0;td_traffic_lights_reset();td.mode=TD_RIDE;td_traffic_lights_render();
    expect(count>0&&uploads==1&&writes==count*2&&reads==count*2,"District graphics load repairs only its current registered visible corners");
    district=0;draw_scroll_x=0;draw_scroll_y=840;td_traffic_lights_reset();uploads=writes=reads=0;
    td_traffic_lights_render();expect(!uploads&&!writes&&!reads,"No visible corner defers all pattern and map work");
    draw_scroll_x=(int)oracle_lights[0][3]*8-80;draw_scroll_y=(int)oracle_lights[0][4]*8-72;
    td_traffic_lights_render();expect(uploads==1,"Deferred cache loads when its first corner becomes visible");
}
int main(void){
    memset(&td,0,sizeof(td));td.cash=139;td.health=100;td.park_u=560*16;td.park_v=720*16;
    const int positions[][2]={{0,0},{159,143},{-7,-7},{160,144},{80,72},{32,-7}};
    for(district=0;district<TD_DISTRICT_COUNT;district++)
    for(unsigned i=oracle_offsets[district];i<oracle_offsets[district+1];i++)
    for(unsigned edge=0;edge<sizeof(positions)/sizeof(positions[0]);edge++)for(unsigned phase=0;phase<2;phase++){
        draw_scroll_x=oracle_lights[i][3]*8-positions[edge][0];draw_scroll_y=oracle_lights[i][4]*8-positions[edge][1];
        td.seconds=phase?7:6;td.mode=TD_ROAM;VBK_REG=phase;check_view();
    }
    district=0;draw_scroll_x=draw_scroll_y=0;
    for(td.mode=0;td.mode<=TD_HELP;td.mode++){VBK_REG=1;check_view();}
    td.mode=TD_ROAM;district=TD_DISTRICT_COUNT;check_view();district=0;
    draw_scroll_x=1024;check_view();draw_scroll_x=0;draw_scroll_y=976;check_view();
    draw_scroll_y=-145;check_view();draw_scroll_y=0;draw_scroll_x=-161;check_view();
    lifecycle();
    printf("Traffic lights: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
