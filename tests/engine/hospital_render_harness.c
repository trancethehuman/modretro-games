#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "host_hospital.h"
#include "td_game.h"
td_state_t td;
UBYTE VBK_REG,WX_REG,WY_REG,win_pos_x,win_pos_y,win_dest_pos_x,win_dest_pos_y;
WORD draw_scroll_x,draw_scroll_y;
static UBYTE maps[2][2][1024],badge[16],page,district;
static unsigned checks,reads,writes,uploads,queries;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Hospital renderer check failed at %d: %s\n",__LINE__,#x);exit(1);}}while(0)
UBYTE td_district_current(void){queries++;return district;}
UBYTE *GetBkgAddr(void){return (UBYTE *)(uintptr_t)(page?0x9c00:0x9800);}
static unsigned address(UBYTE *pointer){uintptr_t p=(uintptr_t)pointer;CHECK(p>=0x9800&&p<0xa000);return p-0x9800;}
UBYTE get_vram_byte(UBYTE *pointer){unsigned i=address(pointer);CHECK(VBK_REG<2);reads++;return maps[i>>10][VBK_REG][i&1023];}
void set_vram_byte(UBYTE *pointer,UBYTE value){unsigned i=address(pointer);CHECK(VBK_REG<2);writes++;maps[i>>10][VBK_REG][i&1023]=value;}
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *source){CHECK(first==254&&count==1&&VBK_REG==1);memcpy(badge,source,16);uploads++;}
#include "hospital_under_test.c"
static unsigned cell(void){return (((344>>3)&31)<<5)|((512>>3)&31);}
static void reset(void){
    memset(&td,0,sizeof(td));memset(maps,0,sizeof(maps));
    td.mode=TD_ROAM;td.district=district=TD_HOSPITAL_DISTRICT;
    page=VBK_REG=WX_REG=WY_REG=0;draw_scroll_x=448;draw_scroll_y=280;
    win_pos_x=win_dest_pos_x=160;win_pos_y=win_dest_pos_y=144;
    reads=writes=uploads=queries=0;
}
static void pixels(void){
    static const char *rows[]={"03333330","30011003","30011003","31111113","31111113","30011003","30011003","03333330"};
    for(unsigned bank=0;bank<2;bank++){
        reset();VBK_REG=bank;td_hospital_init();CHECK(VBK_REG==bank&&uploads==1&&!writes&&!reads);
        for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
            unsigned bit=1u<<(7-x),value=((badge[y*2]&bit)?1:0)|((badge[y*2+1]&bit)?2:0);
            CHECK(value==(unsigned)(rows[y][x]-'0'));
        }
    }
}
static void attributes(void){
    for(unsigned attr=0;attr<256;attr++)for(unsigned p=0;p<2;p++)for(unsigned bank=0;bank<2;bank++){
        reset();page=p;VBK_REG=bank;unsigned c=cell();maps[page][0][c]=19;maps[page][1][c]=attr;
        maps[!page][0][c]=43;maps[!page][1][c]=65;
        td_hospital_render();CHECK(VBK_REG==bank&&writes==2&&reads==1+((attr&127)==15)&&!uploads&&queries==1);
        CHECK(maps[page][0][c]==254&&maps[page][1][c]==((attr&128)|15));
        CHECK(maps[!page][0][c]==43&&maps[!page][1][c]==65);
        unsigned before_writes=writes,before_reads=reads;td_hospital_render();
        CHECK(VBK_REG==bank&&writes==before_writes&&reads==before_reads+2&&!uploads);
        /* Ordinary scrolling may reload the original facade. The annotation
         * is redrawn into that same world cell without an old patch owner. */
        maps[page][0][c]=73;maps[page][1][c]=attr;td_hospital_render();
        CHECK(maps[page][0][c]==254&&maps[page][1][c]==((attr&128)|15)&&!uploads);
    }
}
static void admission(void){
    for(unsigned mode=0;mode<256;mode++){
        reset();td.mode=mode;td_hospital_render();
        CHECK((writes==2)==(mode==TD_ROAM||mode==TD_WAIT||mode==TD_RIDE));
        CHECK(!uploads&&queries==((mode==TD_ROAM||mode==TD_WAIT||mode==TD_RIDE)?1u:0u));
    }
    for(unsigned d=1;d<8;d++){
        reset();district=d;td_hospital_render();CHECK(!reads&&!writes&&!uploads);
        reset();td.district=d;td_hospital_render();CHECK(!reads&&!writes&&!uploads&&!queries);
    }
    static const WORD xs[]={-1,0,1,151,152,153},ys[]={-1,0,1,135,136,137};
    for(unsigned x=0;x<sizeof(xs)/sizeof(xs[0]);x++)for(unsigned y=0;y<sizeof(ys)/sizeof(ys[0]);y++){
        reset();draw_scroll_x=512-xs[x];draw_scroll_y=344-ys[y];td_hospital_render();
        CHECK((writes==2)==(xs[x]>=0&&xs[x]<=152&&ys[y]>=0&&ys[y]<=136));
        CHECK(queries==((xs[x]>=0&&xs[x]<=152&&ys[y]>=0&&ys[y]<=136)?1u:0u));
    }
    reset();win_pos_x=0;win_pos_y=71;td_hospital_render();CHECK(!reads&&!writes&&!queries);
    reset();win_dest_pos_x=0;win_dest_pos_y=71;td_hospital_render();CHECK(!reads&&!writes&&!queries);
    reset();WX_REG=7;WY_REG=71;td_hospital_render();CHECK(!reads&&!writes&&!queries);
    reset();win_pos_x=0;win_pos_y=72;td_hospital_render();CHECK(writes==2);
    reset();win_dest_pos_x=0;win_dest_pos_y=72;td_hospital_render();CHECK(writes==2);
    reset();WX_REG=7;WY_REG=72;td_hospital_render();CHECK(writes==2);
    reset();draw_scroll_x=-32768;draw_scroll_y=-32768;td_hospital_render();CHECK(!reads&&!writes);
    reset();draw_scroll_x=32767;draw_scroll_y=32767;td_hospital_render();CHECK(!reads&&!writes);
}
int main(void){pixels();attributes();admission();printf("Hospital marker: %u checks passed (actual C, independent pixels/VRAM)\n",checks);return 0;}
