#include <stdio.h>
#include <string.h>
#include "td_traffic_lights.h"
#include "td_game.h"
#include "td_district.h"
#define TD_TRAFFIC_RENDER_FIXTURE
#include "traffic_fixture.h"
td_state_t td;WORD draw_scroll_x,draw_scroll_y;UBYTE VBK_REG;
static UBYTE district,map[2][1024],patterns[2][256][16];
static unsigned uploads,writes,reads,checks,failures;
/* Host-only injection counts actual production head-loop work, never native
 * state. The reference counts the previous broad48/192 junction Y band. */
unsigned host_signal_head_steps;
static unsigned long legacy_head_steps,optimized_head_steps;
UBYTE td_district_current(void){return district;}
UBYTE *GetBkgAddr(void){return map[0];}
static void expect(int ok,const char *msg){checks++;if(!ok){if(failures<12){ fprintf(stderr,"FAIL %s\n",msg); }failures++;}}
UBYTE get_vram_byte(UBYTE *p){size_t off=p-map[0];expect(off<1024,"signal read bounded");reads++;return off<1024?map[VBK_REG&1][off]:0;}
void set_vram_byte(UBYTE *p,UBYTE value){size_t off=p-map[0];expect(off<1024,"signal write bounded");if(off<1024){ map[VBK_REG&1][off]=value; }writes++;}
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *data){expect(first==47&&count==2&&VBK_REG==1,"exact reserved two-pattern signal allocation");memcpy(patterns[1][47],data,32);uploads++;}
static unsigned pixel(unsigned tile,unsigned x,unsigned y){return ((patterns[1][tile][y*2]>>(7-x))&1)|(((patterns[1][tile][y*2+1]>>(7-x))&1)<<1);}
static unsigned expected(UBYTE out[2][1024],UBYTE cells[1024]){
 unsigned count=0;memset(cells,0,1024);
 int active=(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE)&&district<TD_DISTRICT_COUNT&&draw_scroll_x<1024&&draw_scroll_y<976&&draw_scroll_x>=-160&&draw_scroll_y>=-144;
 if(active)for(unsigned i=oracle_offsets[district];i<oracle_offsets[district+1];i++)for(unsigned h=0;h<2;h++){
  int x=(h?oracle_extra[i][0]:oracle_lights[i][3])*8,y=(h?oracle_extra[i][1]:oracle_lights[i][4])*8;
  if(x+8<=draw_scroll_x||x>=draw_scroll_x+160||y+8<=draw_scroll_y||y>=draw_scroll_y+144)continue;
  unsigned off=((y/8)%32)*32+(x/8)%32,green=(td.seconds%12<7)^h;
  out[0][off]=green?48:47;out[1][off]=green?14:11;cells[off]=1;count++;
 }
 return count;
}
static unsigned legacy_work(void){
 unsigned count=0;
 if((td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE)||district>=TD_DISTRICT_COUNT||
    draw_scroll_x>=1024||draw_scroll_y>=976||draw_scroll_x<-160||draw_scroll_y<-144)return 0;
 int lower=draw_scroll_y>48?draw_scroll_y-48:0,upper=draw_scroll_y+192;
 for(unsigned i=oracle_offsets[district];i<oracle_offsets[district+1];i++)
  if((int)oracle_lights[i][1]>=lower&&(int)oracle_lights[i][1]<=upper)count+=2;
 return count;
}
static void view(void){
 UBYTE want[2][1024],cells[1024],original[2][256][16],bank=VBK_REG;td_state_t state=td;
 memset(map[0],77,1024);memset(map[1],128,1024);memset(patterns,59,sizeof(patterns));memcpy(want,map,sizeof(map));memcpy(original,patterns,sizeof(patterns));
 unsigned visible=expected(want,cells),prior_work=legacy_work();td_traffic_lights_reset();uploads=writes=reads=host_signal_head_steps=0;td_traffic_lights_render();
 expect(host_signal_head_steps<=prior_work,"exact coarse cull never increases legacy head-decoding work");
 legacy_head_steps+=prior_work;optimized_head_steps+=host_signal_head_steps;
 expect(!memcmp(want,map,sizeof(map)),"each EW/NS pole matches independent full viewport, phase, palette and ring-cell oracle");
 expect(uploads==(visible?1u:0u)&&writes==visible*2&&reads==visible*2,"heads upload two original patterns once and check exactly visible cells");
 expect(VBK_REG==bank&&!memcmp(&td,&state,sizeof(td)),"signal render preserves bank and every saved field");
 if(visible){
  for(unsigned t=47;t<=48;t++){
   unsigned y=t==47?1:3;for(unsigned dy=0;dy<2;dy++)for(unsigned x=2;x<=4;x++)expect(pixel(t,x,y+dy)==1,"large six-pixel lamp uses clear red/green palette shade");
   expect(pixel(t,3,7)==3&&pixel(t,4,7)==3&&pixel(t,0,7)==0,"original upright dark pole has light pavement surround");
  }
  memcpy(patterns[1][47],original[1][47],32);
 }
 expect(!memcmp(patterns,original,sizeof(patterns)),"signals never touch OBJ, aircraft, props, fonts or other patterns");
}
static void lifecycle(void){
 UBYTE want[2][1024],cells[1024],good[32],before[2][1024];district=0;td.mode=TD_ROAM;td.seconds=6;draw_scroll_x=0;draw_scroll_y=0;VBK_REG=1;
 memset(map[0],77,1024);memset(map[1],128,1024);memset(patterns,59,sizeof(patterns));memcpy(want,map,sizeof(map));unsigned count=expected(want,cells);
 td_traffic_lights_reset();uploads=writes=reads=0;td_traffic_lights_render();memcpy(good,patterns[1][47],32);memcpy(before,map,sizeof(map));
 expect(uploads==1&&writes==count*2&&count>0,"initial viewport has larger actual heads");uploads=writes=reads=0;
 for(unsigned n=0;n<8;n++){ td_traffic_lights_render(); }expect(!uploads&&!writes&&reads==count*16,"unchanged frame performs no redundant uploads or writes");
 td.seconds=7;memcpy(want,map,sizeof(map));expected(want,cells);uploads=writes=reads=0;td_traffic_lights_render();
 expect(!memcmp(want,map,sizeof(map))&&!uploads&&writes==count*2&&reads==count*2,"phase boundary changes both lamp position and red/green palette exactly");
 unsigned first=0;while(first<1024&&!cells[first]){ first++; }expect(first<1024,"visible head retained");
 map[1][first]=140;uploads=writes=reads=0;td_traffic_lights_render();expect(writes==1&&map[1][first]==want[1][first],"attribute-only repaint repaired once");
 map[0][first]=91;uploads=writes=reads=0;td_traffic_lights_render();expect(writes==1&&map[0][first]==want[0][first],"tile-only repaint repaired once");
 memcpy(before,map,sizeof(map));td.mode=TD_MAP;memset(patterns[1][47],153,32);uploads=writes=reads=0;td_traffic_lights_reset();td_traffic_lights_render();
 expect(!uploads&&!writes&&!reads&&patterns[1][47][0]==153,"atlas owns VRAM while signal render suppressed");
 td.mode=TD_WAIT;td_traffic_lights_render();expect(uploads==1&&!writes&&!memcmp(patterns[1][47],good,32)&&!memcmp(map,before,sizeof(map)),"modal return repairs source patterns without changing correct cells");
 memset(patterns[1][47],67,32);uploads=writes=reads=0;td_traffic_lights_reset();td_traffic_lights_render();expect(uploads==1&&!writes&&!memcmp(patterns[1][47],good,32),"same-scene graphics reload requires explicit cache reset");
 district=1;draw_scroll_x=760;draw_scroll_y=0;memset(map[0],77,1024);memset(map[1],128,1024);memcpy(want,map,sizeof(map));count=expected(want,cells);
 td.mode=TD_RIDE;td_traffic_lights_reset();uploads=writes=reads=0;td_traffic_lights_render();expect(count>0&&uploads==1&&writes==count*2&&!memcmp(want,map,sizeof(map)),"new district draws only its actual heads");
}
int main(void){
 memset(&td,0,sizeof(td));td.cash=139;td.health=100;
 const int edges[][2]={{0,0},{159,143},{-7,-7},{160,144},{80,72},{32,-7}};
 for(district=0;district<TD_DISTRICT_COUNT;district++)for(unsigned i=oracle_offsets[district];i<oracle_offsets[district+1];i++)for(unsigned h=0;h<2;h++)for(unsigned e=0;e<6;e++)for(unsigned phase=0;phase<12;phase++){
  draw_scroll_x=(h?oracle_extra[i][0]:oracle_lights[i][3])*8-edges[e][0];draw_scroll_y=(h?oracle_extra[i][1]:oracle_lights[i][4])*8-edges[e][1];td.seconds=phase;td.mode=TD_ROAM;VBK_REG=phase&1;view();
 }
 district=0;draw_scroll_x=draw_scroll_y=0;for(td.mode=0;td.mode<=TD_HELP;td.mode++){ view(); }td.mode=TD_ROAM;district=TD_DISTRICT_COUNT;view();district=0;
 draw_scroll_x=1024;view();draw_scroll_x=0;draw_scroll_y=976;view();draw_scroll_y=-145;view();draw_scroll_y=0;draw_scroll_x=-161;view();lifecycle();
 district=0;td.mode=TD_ROAM;td.seconds=0;draw_scroll_x=480;draw_scroll_y=648;
 unsigned union_before=legacy_work();view();
 expect(host_signal_head_steps<union_before,"stationary Union camera skips offscreen columns without removing visible heads");
 printf("Signal head work: Union%u->%u; exhaustive view sum%lu->%lu, with exact visible VRAM work/pixels unchanged\n",
        union_before,host_signal_head_steps,legacy_head_steps,optimized_head_steps);
 printf("Larger traffic heads actual C: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
