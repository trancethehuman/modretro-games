#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "td_game.h"
#include "scenery_under_test.c"
td_state_t td;far_ptr_t current_scene;UBYTE VBK_REG;WORD draw_scroll_x,draw_scroll_y;
static UBYTE district,map[2][1024],patterns[2][256][16];
static unsigned checks,failures,uploads,writes,district_reads,map_lookups,reads;
UBYTE td_district_current(void){district_reads++;return district;}
UBYTE *GetBkgAddr(void){map_lookups++;return map[0];}
static void expect(int ok,const char *message){checks++;if(!ok){if(failures<12)fprintf(stderr,"FAIL %s\n",message);failures++;}}
UBYTE get_vram_byte(UBYTE *p){size_t off=p-map[0];reads++;expect(off<1024,"map read bounded");return off<1024?map[VBK_REG&1][off]:0;}
void set_vram_byte(UBYTE *p,UBYTE value){size_t off=p-map[0];expect(off<1024,"map write bounded");if(off<1024)map[VBK_REG&1][off]=value;writes++;}
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *data){expect(first>=80&&first<=97&&count==1&&VBK_REG==1,"only18 reserved signed bank1 patterns");memcpy(patterns[VBK_REG&1][first],data,16);uploads++;}
static unsigned random_state=7171;
static unsigned rng(void){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
static int slab(double old,double delta,double low,double high,double *a,double *b){
 if(delta==0)return old>=low&&old<=high;
 double first=(low-old)/delta,last=(high-old)/delta,t;if(first>last){t=first;first=last;last=t;}
 if(first>*a)*a=first;if(last<*b)*b=last;return *a<=*b;
}
static int oracle(unsigned ou,unsigned ov,unsigned u,unsigned v,unsigned half){
 double a,b;int pad=half*16;unsigned start=td_prop_offsets[district],end=td_prop_offsets[district+1];
 for(unsigned i=start;i<end;i++){
  if((td_broken[i/8]>>(i%8))&1)continue;
  int x=td_props[i].x*128,y=td_props[i].y*128;
  int inside=(int)ou>=x-pad&&(int)ou<=x+127+pad&&(int)ov>=y-pad&&(int)ov<=y+127+pad;
  int before=abs((int)ou-x-64)+abs((int)ov-y-64),after=abs((int)u-x-64)+abs((int)v-y-64);
  if(inside&&after>=before)continue;
  a=0;b=1;
  if(slab(ou,(int)u-(int)ou,x-pad,x+127+pad,&a,&b)&&slab(ov,(int)v-(int)ov,y-pad,y+127+pad,&a,&b))return TD_SCENERY_BLOCK;
 }
 return TD_SCENERY_CLEAR;
}
static void contact(void){
 for(district=0;district<7;district++){
  unsigned start=td_prop_offsets[district],end=td_prop_offsets[district+1];
  for(unsigned i=start;i<end;i++)for(unsigned side=0;side<4;side++){
   unsigned x=td_props[i].x*128,y=td_props[i].y*128;
   unsigned ou=x+64,ov=y+64,u=ou,v=ov;
   if(side==0){ou=x-113;u=ou+1;}if(side==1){ou=x+240;u=ou-1;}
   if(side==2){ov=y-113;v=ov+1;}if(side==3){ov=y+240;v=ov-1;}
   memset(td_broken,255,sizeof(td_broken));td_broken[i/8]&=~(1<<(i%8));td_prop_flash_next|=128;
   expect(td_scenery_contact(ou,ov,u,v,7,2)==TD_SCENERY_BLOCK,"every authored item blocks weak whole-body contact from four sides");
   expect(!td_prop_dead(i),"weak contact cannot silently destroy prop");
   expect(td_scenery_contact(ou,ov,u,v,7,side&1?-3:3)==TD_SCENERY_BROKE,"forward/reverse threshold breaks exact whole-body contact");
   expect(td_prop_dead(i),"target global bit persists in correct district and ID");
   expect(td_scenery_contact(ou,ov,u,v,7,2)==TD_SCENERY_CLEAR,"broken item is traversable without repeated recoil");
  }
  td_scenery_reset();
  for(unsigned n=0;n<12000;n++){
   unsigned i=start+rng()%(end-start),x=td_props[i].x*128+64,y=td_props[i].y*128+64;
   int ou=(int)x+(int)(rng()%641)-320,ov=(int)y+(int)(rng()%641)-320;
   int u=ou+(int)(rng()%33)-16,v=ov+(int)(rng()%33)-16;
   if(ou<0||ov<0||u<0||v<0||ou>16383||u>16383||ov>15615||v>15615)continue;
   int expected=ou==u&&ov==v?0:oracle(ou,ov,u,v,7);
   expect(td_scenery_contact(ou,ov,u,v,7,2)==expected,"actual binary broadphase and diagonal Q4 body sweep match independent slab oracle");
  }
 }
 district=7;expect(td_scenery_contact(1000,1000,1001,1000,7,3)==TD_SCENERY_BLOCK,"invalid district fails closed");district=0;
 expect(td_scenery_contact(1000,1000,1033,1000,7,3)==TD_SCENERY_BLOCK,"unbounded teleport fails closed");
 td_scenery_reset();expect(!memcmp(td_broken,(UBYTE[142]){0},142),"cold reset clears all packed district bits");
}
static void packing(void){
 static int packed_scene;
 expect(TD_SCENERY_PROPS==1135&&sizeof(td_broken)==142,"all1135 authored props occupy exact142-byte packed storage");
 expect(td_prop_offsets[7]==TD_SCENERY_PROPS,"district offsets cover exactly the global bitmap");
 /* Every index must be distinguishable, including adjacent district ends
  * sharing a byte. Compare the actual getter to an independent single-bit
  * identity oracle rather than another district/offset calculation. */
 for(unsigned target=0;target<1135;target++){
  td_scenery_reset();td_broken[target/8]=1u<<(target%8);td_prop_flash_next|=128;
  for(unsigned index=0;index<1135;index++)
   expect(td_prop_dead(index)==(index==target),"one global bit never aliases another prop/district");
  UBYTE before[142];memcpy(before,td_broken,sizeof(before));
  for(unsigned d=0;d<7;d++){district=d;td_scenery_render_reset();
   expect(!memcmp(td_broken,before,sizeof(before)),"scene/shop reset retains every district's exact packed destruction state");}
  district=0;while(target>=td_prop_offsets[district+1])district++;
  current_scene=(far_ptr_t){1,&packed_scene};td.mode=TD_ROAM;
  draw_scroll_x=td_props[target].x*8-80;draw_scroll_y=td_props[target].y*8-72;
  memset(map,0,sizeof(map));uploads=writes=0;td_scenery_render();
  unsigned off=(td_props[target].y%32)*32+td_props[target].x%32;
  expect(uploads==1&&td_prop_patch_count==1&&map[0][off]==80&&map[1][off]==8,
         "single global debug-painted bit renders only its exact prop in its own district");
  td_scenery_restore();expect(map[0][off]==0&&map[1][off]==0,
         "single global debug-painted prop restores the exact untouched map cell");
 }
 td_prop_flashes[0]=(td_prop_flash_t){0,0,24};td_prop_flashes[1]=(td_prop_flash_t){1,0,16};
 expect(td_prop_phase(0,0)==2&&td_prop_phase(1,0)==1,"equal local IDs in different districts retain independent flash phases");
 td_prop_flash_next=131;td_scenery_reset();
 expect(!td_prop_flash_next&&!memcmp(td_prop_flashes,(td_prop_flash_t[4]){{0}},sizeof(td_prop_flashes)),"cold reset also clears flash identities and replacement cursor");
 expect(!memcmp(td_broken,(UBYTE[142]){0},142),"cold reset clears all1135 destruction bits after exhaustive paint");
}
static void fastpath(void){
 static int outside,shop;
 UBYTE before_map[2][1024],before_patterns[2][256][16];
 const UBYTE modes[]={TD_ROAM,TD_WAIT,TD_RIDE,TD_MAP};
 td_scenery_reset();memset(map,59,sizeof(map));memset(patterns,117,sizeof(patterns));
 memcpy(before_map,map,sizeof(map));memcpy(before_patterns,patterns,sizeof(patterns));
 for(district=0;district<7;district++)for(unsigned mode=0;mode<4;mode++)for(unsigned bank=0;bank<2;bank++){
  current_scene=(far_ptr_t){1,&outside};draw_scroll_x=480;draw_scroll_y=648;td.mode=modes[mode];VBK_REG=bank;
  uploads=writes=reads=district_reads=map_lookups=0;td_scenery_render();
  expect(!district_reads&&!map_lookups&&!reads&&!writes&&!uploads,
         "pristine session skips district bank lookup and all map/pattern traffic");
  expect(!td_prop_flash_next&&!td_prop_patch_count&&VBK_REG==bank,
         "pristine fast path leaves cursor, patches and VRAM bank unchanged");
 }
 expect(!memcmp(before_map,map,sizeof(map))&&!memcmp(before_patterns,patterns,sizeof(patterns)),
        "clean-view shortcut preserves all graphics bytes");
 /* Debug paint must explicitly mark session damage. A raw unmarked bit is
  * deliberately ignored, proving the shortcut does not secretly scan142B. */
 district=0;td.mode=TD_ROAM;td_broken[0]=1;draw_scroll_x=td_props[0].x*8-80;draw_scroll_y=td_props[0].y*8-72;
 uploads=writes=reads=district_reads=map_lookups=0;td_scenery_render();
 expect(!district_reads&&!map_lookups&&!reads&&!writes&&!uploads,
        "an unmarked debug bit cannot bypass the clean session flag");
 /* Every authored prop must set the flag via the real public contact API,
  * without injected neighboring destruction. Half0 isolates its exact cell;
  * existing four-sided half7 tests separately prove whole-car impacts. */
 for(unsigned target=0;target<1135;target++){
  td_scenery_reset();district=0;while(target>=td_prop_offsets[district+1])district++;
  unsigned owner=district,x=td_props[target].x*128,y=td_props[target].y*128+64;
  expect(x>0,"authored impact probe is inside the valid native coordinate range");
  expect(td_scenery_contact(x-1,y,x,y,0,2)==TD_SCENERY_BLOCK&&!td_prop_flash_next,
         "weak API impact keeps the pristine session flag clear");
  expect(td_scenery_contact(x-1,y,x,y,0,3)==TD_SCENERY_BROKE&&td_prop_flash_next==129,
         "every first public API break sets dirty bit and advances low2 cursor");
  UBYTE expected[142]={0};expected[target/8]=1u<<(target%8);
  expect(!memcmp(td_broken,expected,sizeof(expected)),"public API break marks exactly its own global bitmap bit");
  expect(td_prop_flashes[0].district==owner&&td_prop_flashes[0].id==target-td_prop_offsets[owner]&&td_prop_flashes[0].ticks==24,
         "first dirty flag retains exact district-local flash identity");
  current_scene=(far_ptr_t){1,&outside};td.mode=TD_ROAM;
  draw_scroll_x=td_props[target].x*8-80;draw_scroll_y=td_props[target].y*8-72;
  memset(map,0,sizeof(map));uploads=writes=reads=district_reads=map_lookups=0;td_scenery_render();
  unsigned off=(td_props[target].y%32)*32+td_props[target].x%32;
  expect(district_reads==1&&map_lookups==1&&uploads==1&&map[0][off]==80&&map[1][off]==8,
         "first real impact enables the existing exact single-prop renderer");
  expect(td_prop_flashes[0].ticks==23,"first dirty presentation still advances the original impact animation");
  td_scenery_restore();expect(map[0][off]==0&&map[1][off]==0,"first API break restores original ground bytes");
  td.mode=TD_MAP;uploads=writes=reads=district_reads=map_lookups=0;td_scenery_render();
  expect(!district_reads&&!map_lookups&&!reads&&!writes&&!uploads&&td_prop_flash_next==129,
         "map modal owns no scenery work and preserves the session flag");
  current_scene=(far_ptr_t){2,&shop};td_scenery_render_reset();td_scenery_restore();
  expect(td_prop_flash_next==129&&!memcmp(td_broken,expected,sizeof(expected)),
         "shop render reset and return preserve persistent damage and packed cursor");
  td.mode=TD_ROAM;current_scene=(far_ptr_t){1,&outside};
  for(district=0;district<7;district++){
   td_scenery_render_reset();uploads=0;td_scenery_render();
   expect(uploads==(district==owner)&&td_prop_flash_next==129&&!memcmp(td_broken,expected,sizeof(expected)),
          "district transitions retain session flag and render only the exact owning district");
   td_scenery_restore();
  }
  district=owner;td_scenery_reset();uploads=writes=reads=district_reads=map_lookups=0;td_scenery_render();
  expect(!district_reads&&!map_lookups&&!reads&&!writes&&!uploads&&!td_prop_flash_next&&!memcmp(td_broken,(UBYTE[142]){0},142),
         "cold reset after each real API impact restores the zero-work pristine path");
 }
 /* Eight real breaks wrap the four-entry ring twice while bit7 stays set. */
 td_scenery_reset();district=0;
 for(unsigned target=0;target<8;target++){
  unsigned x=td_props[target].x*128,y=td_props[target].y*128+64;
  expect(td_scenery_contact(x-1,y,x,y,0,3)==TD_SCENERY_BROKE,"separate intact API targets break without bitmap injection");
  expect(td_prop_flash_next==(128|((target+1)&3)),"dirty high bit never enters four-entry flash array index");
  expect(td_prop_flashes[target&3].district==0&&td_prop_flashes[target&3].id==target&&td_prop_flashes[target&3].ticks==24,
         "low2 replacement cursor retains correct identities through two wraps");
 }
 draw_scroll_x=td_props[0].x*8-80;draw_scroll_y=td_props[0].y*8-72;memset(map,0,sizeof(map));td_scenery_render();
 expect(td_prop_patch_count>0,"dirty test has active patches before independent restore");
 td_prop_flash_next&=3;td_scenery_restore();
 expect(!memcmp(map,(UBYTE[2][1024]){{0}},sizeof(map)),"restore remains independent of the fast-path flag");
 td_scenery_reset();
}
static void render(void){
 static int scene_a,scene_b;UBYTE before_map[2][1024],before_patterns[2][256][16];
 for(district=0;district<7;district++)for(unsigned cy=0;cy<976;cy+=32)for(unsigned cx=0;cx<1024;cx+=32){
  td_scenery_render_reset();td.mode=TD_ROAM;current_scene=(far_ptr_t){1,&scene_a};draw_scroll_x=cx;draw_scroll_y=cy;VBK_REG=(cx/32)&1;
  memset(td_broken,255,sizeof(td_broken));memset(td_prop_flashes,0,sizeof(td_prop_flashes));td_prop_flash_next|=128;
  for(unsigned i=0;i<1024;i++){map[0][i]=(i*13)%80;map[1][i]=(i%7)|((i&1)?8:0)|((i&2)?32:0)|((i&4)?64:0);}
  memset(patterns,71,sizeof(patterns));memcpy(before_map,map,sizeof(map));memcpy(before_patterns,patterns,sizeof(patterns));uploads=writes=0;
  td_state_t state=td;UBYTE bank=VBK_REG;td_scenery_render();
  unsigned visible=0;for(unsigned i=td_prop_offsets[district];i<td_prop_offsets[district+1];i++){
   int x=td_props[i].x*8,y=td_props[i].y*8;if(x>draw_scroll_x+159||x+7<draw_scroll_x||y>draw_scroll_y+143||y+7<draw_scroll_y)continue;
   unsigned off=(td_props[i].y%32)*32+td_props[i].x%32;
   expect(map[0][off]==80+visible&&map[1][off]==((before_map[1][off]&7)|8),"every visible broken item receives ground palette with no roof/flip flags");
   UBYTE expected[16];memcpy(expected,td_prop_ground[td_props[i].ground],16);
   unsigned variant=td_props[i].kind&1,shade=td_props[i].kind==1?2:3;
   const unsigned rows[2][8]={{0,0,0,0,0,16,66,36},{0,0,0,0,0,8,36,66}};
   for(unsigned r=0;r<8;r++)for(unsigned bit=0;bit<8;bit++)if(rows[variant][r]&(1<<bit)){
    expected[2*r]=(expected[2*r]&~(1<<bit))|((shade&1)?1<<bit:0);expected[2*r+1]=(expected[2*r+1]&~(1<<bit))|((shade&2)?1<<bit:0);
   }
   expect(!memcmp(expected,patterns[1][80+visible],16),"rubble bits and exact pre-furniture ground match independent pixel oracle");visible++;
  }
  expect(visible<=18&&uploads==visible&&td_prop_patch_count==visible,"source geometry never exceeds18 patch admission");
  expect(VBK_REG==bank&&!memcmp(&td,&state,sizeof(td)),"renderer preserves VRAM bank and gameplay/save state");
  td_scenery_restore();expect(!memcmp(map,before_map,sizeof(map)),"restore preserves original bank, palette and H/V flip bytes exactly");
  memcpy(patterns[1][80],before_patterns[1][80],18*16);expect(!memcmp(patterns,before_patterns,sizeof(patterns)),"renderer never changes OBJ, aircraft, light or UI patterns");
  uploads=writes=0;td_scenery_restore();expect(!writes&&!uploads,"repeat restore owns no cells");
 }
 district=1;td_scenery_render_reset();draw_scroll_x=353;draw_scroll_y=289;td_scenery_render();
 expect(td_prop_patch_count==18,"worst West viewport admits all18 damaged props");
 td_prop_patch_t p=td_prop_patches[0];map[0][p.offset]=119;map[1][p.offset]=131;
 td_scenery_restore();expect(map[0][p.offset]==119&&map[1][p.offset]==131,"scroll repaint retains new ownership");
 td_scenery_render();memcpy(before_map,map,sizeof(map));current_scene=(far_ptr_t){2,&scene_b};td_scenery_restore();
 expect(!memcmp(map,before_map,sizeof(map)),"old outdoor patches never restore into another scene/shop");
 td_scenery_render_reset();uploads=writes=0;td.mode=TD_MAP;td_scenery_render();expect(!writes&&!uploads,"map/menu leaves modal scratch untouched");
 td.mode=TD_ROAM;current_scene=(far_ptr_t){1,&scene_a};memset(map,0,sizeof(map));td_scenery_render();
 expect(uploads==18,"return from same-scene modal overwrites cached scratch with persistent damage");td_scenery_restore();
 unsigned i=td_prop_offsets[district];td_prop_flashes[0]=(td_prop_flash_t){district,0,24};
 draw_scroll_x=td_props[i].x*8-80;draw_scroll_y=td_props[i].y*8-72;
 for(unsigned frame=0;frame<25;frame++){unsigned expected=frame<8?2:frame<16?1:0;expect(td_prop_phase(district,0)==expected,"impact flash falls into rubble in24 visible frames");td_scenery_render();td_scenery_restore();}
 unsigned prior=td_prop_flash_next;td_scenery_render_reset();expect(td_prop_dead(td_prop_offsets[district])&&prior==td_prop_flash_next,"scene reload preserves destruction and animation identities");
}
int main(void){packing();contact();fastpath();render();printf("Scenery actual C: %u checks, %u failures\n",checks,failures);return failures?1:0;}
