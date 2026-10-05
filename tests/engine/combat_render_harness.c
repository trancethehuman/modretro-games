#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "host_combat_render.h"
#include "td_game.h"
#include "td_combat.h"
UBYTE VBK_REG,WX_REG,WY_REG,win_pos_x,win_pos_y,win_dest_pos_x,win_dest_pos_y;
WORD draw_scroll_x,draw_scroll_y;
static UBYTE maps[2][2][1024],data[2][256][16],page,flash,dir;
static UWORD origin_u,origin_v;
static unsigned checks,writes,reads,tile_writes;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Muzzle renderer check failed at %d: %s\n",__LINE__,#x);exit(1);}}while(0)
UBYTE *GetBkgAddr(void){return (UBYTE*)(uintptr_t)(page?0x9c00:0x9800);}
static unsigned address(UBYTE *pointer){uintptr_t p=(uintptr_t)pointer;CHECK(p>=0x9800&&p<0xa000);return p-0x9800;}
UBYTE get_vram_byte(UBYTE *pointer){unsigned i=address(pointer);CHECK(VBK_REG<2);reads++;return maps[i>>10][VBK_REG][i&1023];}
void set_vram_byte(UBYTE *pointer,UBYTE value){unsigned i=address(pointer);CHECK(VBK_REG<2);writes++;maps[i>>10][VBK_REG][i&1023]=value;}
void get_bkg_data(UBYTE first,UBYTE count,UBYTE *dest){CHECK(count==1&&VBK_REG<2);memcpy(dest,data[VBK_REG][first],16);}
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *source){CHECK(count==1&&VBK_REG==1&&first==253);memcpy(data[VBK_REG][first],source,16);tile_writes++;}
UBYTE td_combat_flash(UWORD *u,UWORD *v,UBYTE *direction){if(!flash)return FALSE;*u=origin_u;*v=origin_v;*direction=dir;return TRUE;}
#include "renderer_under_test.c"
static void reset(void){
    td_combat_render_reset();memset(maps,0,sizeof(maps));
    for(unsigned bank=0;bank<2;bank++)for(unsigned tile=0;tile<256;tile++)for(unsigned byte=0;byte<16;byte++)data[bank][tile][byte]=(tile*3+byte*17+bank*89)&255;
    page=VBK_REG=0;flash=1;dir=0;origin_u=origin_v=320;draw_scroll_x=256;draw_scroll_y=256;
    win_pos_x=win_dest_pos_x=160;win_pos_y=win_dest_pos_y=144;WX_REG=WY_REG=0;
    reads=writes=tile_writes=0;
}
static unsigned cell(void){unsigned x=origin_u+(dir==0?12:dir==1?-12:0),y=origin_v+(dir==2?12:dir==3?-12:0);return (((y>>3)&31)<<5)|((x>>3)&31);}
static void pixels(void){
    for(unsigned attr=0;attr<256;attr++)for(unsigned bank=0;bank<2;bank++)for(unsigned direction=0;direction<4;direction++){
        reset();page=bank;dir=direction;unsigned c=cell();maps[page][0][c]=29;maps[page][1][c]=attr;
        UBYTE before[16],original[2][2][1024];memcpy(before,data[(attr>>3)&1][29],16);memcpy(original,maps,sizeof(maps));VBK_REG=bank;
        td_combat_render();CHECK(VBK_REG==bank);
        if(attr&128){CHECK(!writes&&!tile_writes);CHECK(!memcmp(maps,original,sizeof(maps)));continue;}
        CHECK(tile_writes==1&&writes==2);CHECK(maps[page][0][c]==253&&maps[page][1][c]==((attr&247)|8));
        for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
            unsigned mark=(y==1||y==6)?(x==3||x==4):(y==2||y==5)?(x>=2&&x<=5):(y==3||y==4)?(x>=1&&x<=6):0;
            unsigned bright=(y==2||y==5)?(x==3||x==4):(y==3||y==4)?(x>=2&&x<=5):0;
            unsigned bit=1u<<(7-x);
            unsigned value=((data[1][253][y*2]&bit)?1:0)|((data[1][253][y*2+1]&bit)?2:0);
            unsigned prior=((before[y*2]&bit)?1:0)|((before[y*2+1]&bit)?2:0);
            CHECK(value==(bright?0:mark?3:prior));
        }
        td_combat_render();CHECK(tile_writes==1);td_combat_render_restore();CHECK(VBK_REG==bank);CHECK(!memcmp(maps,original,sizeof(maps)));
        unsigned after=writes;td_combat_render_restore();CHECK(writes==after);
    }
}
static void ownership(void){
    reset();unsigned c=cell();maps[0][0][c]=23;maps[0][1][c]=5;td_combat_render();page=1;maps[1][0][c]=99;maps[1][1][c]=7;
    td_combat_render_restore();CHECK(maps[0][0][c]==23&&maps[0][1][c]==5&&maps[1][0][c]==99&&maps[1][1][c]==7);
    reset();c=cell();maps[0][0][c]=23;maps[0][1][c]=5;td_combat_render();maps[0][0][c]=88;td_combat_render_restore();CHECK(maps[0][0][c]==88&&maps[0][1][c]==13);
    reset();c=cell();maps[0][0][c]=23;maps[0][1][c]=5;td_combat_render();maps[0][1][c]=2;td_combat_render_restore();CHECK(maps[0][0][c]==253&&maps[0][1][c]==2);
    reset();c=cell();td_combat_render();td_combat_render_reset();maps[0][0][c]=17;unsigned count=writes;td_combat_render_restore();CHECK(writes==count&&maps[0][0][c]==17);
}
static void admission(void){
    reset();flash=0;td_combat_render();CHECK(!reads&&!writes);
    reset();dir=4;td_combat_render();CHECK(!reads&&!writes);
    reset();origin_u=0;dir=1;td_combat_render();CHECK(!reads&&!writes);
    reset();origin_u=1023;td_combat_render();CHECK(!reads&&!writes);
    reset();draw_scroll_x=400;td_combat_render();CHECK(!reads&&!writes);
    reset();win_pos_x=0;win_pos_y=68;td_combat_render();CHECK(!reads&&!writes);
    reset();win_dest_pos_x=0;win_dest_pos_y=68;td_combat_render();CHECK(!reads&&!writes);
    reset();WX_REG=7;WY_REG=68;td_combat_render();CHECK(!reads&&!writes);
    reset();origin_u=1011;origin_v=964;dir=2;draw_scroll_x=864;draw_scroll_y=832;td_combat_render();CHECK(!reads&&!writes);
}
int main(void){CHECK(sizeof(td_muzzle_x)+sizeof(td_muzzle_y)+sizeof(td_muzzle_tile)+sizeof(td_muzzle_attr)==4);pixels();ownership();admission();printf("Muzzle compositor: %u checks passed (actual C, independent pixels/VRAM)\n",checks);return 0;}
