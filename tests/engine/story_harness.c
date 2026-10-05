/* Actual story controller through bounded VRAM/window adapters. These checks
 * establish source behavior and pixel identity, not native banking or timing. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "td_game.h"
#include "td_story.h"
#include "gbs_types.h"
#include "story_oracle.h"

td_state_t td;
far_ptr_t current_scene;
UBYTE VBK_REG;
static UBYTE vram[2][256][16],window_tiles[18][20];
static char text_rows[18][21];
static unsigned long checks,uploads,map_writes,text_writes;
static unsigned long scene_reads,background_restores;
static story_scene_t host_scene;
static background_t host_background;
static tileset_t host_tileset;

#define REQUIRE(condition) do { ++checks; if(!(condition)){ \
    fprintf(stderr,"Story regression line%u: %s\n",__LINE__,#condition);exit(1); } } while(0)

void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *tiles){
    UWORD i;
    REQUIRE(VBK_REG==1);REQUIRE(first==16);REQUIRE(count==ORACLE_TILE_COUNT);
    REQUIRE((UWORD)first+count<=188);++uploads;
    for(i=0;i<count;i++)memcpy(vram[VBK_REG][first+i],tiles+i*16,16);
}
void set_win_tiles(UBYTE x,UBYTE y,UBYTE width,UBYTE height,const UBYTE *tiles){
    UBYTE row;
    REQUIRE(VBK_REG==0);REQUIRE(x==0&&y==1&&width==20&&height==10);++map_writes;
    for(row=0;row<height;row++)memcpy(&window_tiles[y+row][x],tiles+row*width,width);
}
void td_ui_story_text(UBYTE row,const char *text){
    REQUIRE(row<18);REQUIRE(strlen(text)<=20);++text_writes;
    memset(text_rows[row],0,21);strcpy(text_rows[row],text);
}
void MemcpyBanked(void *destination,const void *source,size_t length,UBYTE bank){
    REQUIRE(bank==7||bank==19||bank==37);++scene_reads;
    memcpy(destination,source,length);
}
/* The inspected existing GBVM engine service handles its signed allocation.
 * This bounded adapter records the invocation and models the exact two-chunk
 * layout for <=192 background tiles; native linking must separately prove it. */
void load_bkg_tileset(const tileset_t *tiles,UBYTE bank){
    UWORD count=tiles->n_tiles,head=count<128?count:128,tail=count-head;
    REQUIRE(VBK_REG==1&&tiles==&host_tileset&&bank==37);++background_restores;
    REQUIRE(count<=192);
    if(head)memcpy(vram[1][0],tiles->tiles,head*16);
    if(tail)memcpy(vram[1][192-tail],tiles->tiles+128*16,tail*16);
}

#include "story_under_test.c"

static void initialize(void){
    memset(&td,0x5a,sizeof(td));td.mode=TD_ROAM;td.menu=0;
    td.done=0;td.complete[TD_STORY_SAVE_BYTE]=0;
    memset(vram,0x6d,sizeof(vram));memset(window_tiles,0x7c,sizeof(window_tiles));
    memset(text_rows,0,sizeof(text_rows));uploads=map_writes=text_writes=0;VBK_REG=0;
    current_scene=(far_ptr_t){0,NULL};scene_reads=background_restores=0;
}
static void unchanged_except_ui(const td_state_t *before,UBYTE storyflags){
    td_state_t after=td;
    after.mode=before->mode;after.menu=before->menu;
    after.complete[TD_STORY_SAVE_BYTE]=storyflags;
    REQUIRE(!memcmp(before,&after,sizeof(after)));
}
static void pixel_match(UBYTE portrait){
    UWORD x,y;UBYTE tile,pixel;
    for(y=0;y<80;y++)for(x=0;x<160;x++){
        tile=window_tiles[(y>>3)+1][x>>3];
        REQUIRE(tile>=16&&tile<16+ORACLE_TILE_COUNT);
        pixel=((vram[1][tile][(y&7)*2]>>(7-(x&7)))&1)|
              (((vram[1][tile][(y&7)*2+1]>>(7-(x&7)))&1)<<1);
        REQUIRE(pixel==oracle_pixels[portrait][y*160+x]);
    }
    for(tile=188;tile<255;tile++)for(x=0;x<16;x++)REQUIRE(vram[1][tile][x]==0x6d);
    for(tile=0;tile<16;tile++)for(x=0;x<16;x++)REQUIRE(vram[1][tile][x]==0x6d);
    for(y=0;y<256;y++)for(x=0;x<16;x++)REQUIRE(vram[0][y][x]==0x6d);
}
static void due_boundaries(void){
    UWORD done,seen;UBYTE scene,expected;
    for(done=0;done<=104;done++)for(seen=0;seen<256;seen++){
        initialize();td.done=done;td.complete[13]=seen;expected=TD_NONE;
        for(scene=0;scene<8;scene++)if(done>=oracle_thresholds[scene]&&!(seen&(1<<scene))){expected=scene;break;}
        REQUIRE(td_story_due()==expected);
    }
    initialize();td.done=255;td.complete[13]=255;REQUIRE(td_story_due()==TD_NONE);
}
static void full_scenes(void){
    UBYTE scene,page,row,flags;td_state_t before;
    for(scene=0;scene<8;scene++){
        initialize();td.done=oracle_thresholds[scene];flags=(1<<scene)-1;
        td.complete[13]=flags;before=td;
        REQUIRE(td_story_due()==scene);REQUIRE(td_story_begin(scene));
        REQUIRE(td.mode==TD_DIALOG&&td.menu==oracle_firsts[scene]);
        for(page=oracle_firsts[scene];page<oracle_firsts[scene+1];page++){
            REQUIRE(td.menu==page);td_story_draw();pixel_match(oracle_pages[page].portrait);
            REQUIRE(!strcmp(text_rows[0],oracle_titles[scene]));
            REQUIRE(!strcmp(text_rows[11],oracle_pages[page].speaker));
            for(row=0;row<4;row++)REQUIRE(!strcmp(text_rows[12+row],oracle_pages[page].lines[row]));
            REQUIRE(!strcmp(text_rows[16],"A NEXT / B SKIP"));
            REQUIRE(!strcmp(text_rows[17],page+1==oracle_firsts[scene+1]?"END OF CHAPTER":"STORY / WORLD PAUSED"));
            REQUIRE(uploads==1);REQUIRE(map_writes==page-oracle_firsts[scene]+1u);
            REQUIRE(!td_story_input(0));REQUIRE(td.menu==page);
            REQUIRE(!td_story_input(J_UP|J_DOWN|J_LEFT|J_RIGHT|J_SELECT));REQUIRE(td.menu==page);
            unchanged_except_ui(&before,flags);
            if(page+1==oracle_firsts[scene+1]){
                REQUIRE(td_story_input(J_A));REQUIRE(td.complete[13]==(UBYTE)(flags|(1<<scene)));
                unchanged_except_ui(&before,flags);
            }else{
                REQUIRE(!td_story_input(J_A));REQUIRE(td.menu==page+1);
            }
        }
    }
}
static void skip_and_invalid(void){
    UWORD value;UBYTE scene,button;td_state_t before;
    for(scene=0;scene<8;scene++)for(value=0;value<256;value++)for(button=0;button<2;button++){
        initialize();td.complete[13]=value;before=td;REQUIRE(td_story_begin(scene));
        REQUIRE(td_story_input(button?J_START:J_B));
        REQUIRE(td.complete[13]==(UBYTE)(value|(1<<scene)));
        unchanged_except_ui(&before,value);
    }
    for(value=8;value<256;value++){
        initialize();before=td;REQUIRE(!td_story_begin(value));
        REQUIRE(!memcmp(&before,&td,sizeof(td)));td_story_draw();
        REQUIRE(!uploads&&!map_writes&&!text_writes);
    }
    for(value=ORACLE_PAGE_COUNT;value<256;value++){
        initialize();td.mode=TD_DIALOG;td.menu=value;before=td;
        td_story_draw();REQUIRE(!td_story_input(J_A|J_B|J_START));
        REQUIRE(!memcmp(&before,&td,sizeof(td)));REQUIRE(!uploads&&!map_writes&&!text_writes);
    }
    initialize();before=td;REQUIRE(!td_story_input(J_A|J_B|J_START));
    td_story_draw();REQUIRE(!memcmp(&before,&td,sizeof(td)));REQUIRE(!uploads&&!map_writes&&!text_writes);
}
static void restore_original_background(void){
    UWORD count,tile,index;UBYTE bank,expected;td_state_t before;
    for(count=0;count<=192;count++)for(bank=0;bank<2;bank++){
        initialize();before=td;host_scene.background=(far_ptr_t){19,&host_background};
        host_background.cgb_tileset=(far_ptr_t){37,&host_tileset};
        current_scene=(far_ptr_t){7,&host_scene};host_tileset.n_tiles=count;
        for(index=0;index<sizeof(host_tileset.tiles);index++)host_tileset.tiles[index]=(UBYTE)(index*13+count);
        VBK_REG=bank;td_story_restore_background();
        REQUIRE(VBK_REG==bank&&background_restores==1&&scene_reads==3);
        REQUIRE(!memcmp(&before,&td,sizeof(td)));
        REQUIRE(host_scene.background.ptr==&host_background&&host_background.cgb_tileset.ptr==&host_tileset);
        for(tile=0;tile<256;tile++)for(index=0;index<16;index++){
            expected=0x6d;
            if(tile<count&&tile<128)expected=host_tileset.tiles[tile*16+index];
            else if(count>128&&tile>=192-(count-128)&&tile<192)
                expected=host_tileset.tiles[(128+tile-(192-(count-128)))*16+index];
            REQUIRE(vram[1][tile][index]==expected);REQUIRE(vram[0][tile][index]==0x6d);
        }
        REQUIRE(!uploads&&!map_writes&&!text_writes);
    }
    /* Missing far references, and a tileset overlapping protected UI, must
     * not trigger a scene reload or any native VRAM write. */
    for(index=0;index<7;index++){
        initialize();host_scene.background=(far_ptr_t){19,&host_background};
        host_background.cgb_tileset=(far_ptr_t){37,&host_tileset};
        current_scene=(far_ptr_t){7,&host_scene};host_tileset.n_tiles=193;
        switch(index){
            case 0:current_scene.bank=0;break;
            case 1:current_scene.ptr=NULL;break;
            case 2:host_scene.background.bank=0;break;
            case 3:host_scene.background.ptr=NULL;break;
            case 4:host_background.cgb_tileset.bank=0;break;
            case 5:host_background.cgb_tileset.ptr=NULL;break;
        }
        td_story_restore_background();REQUIRE(!background_restores);
        for(tile=0;tile<256;tile++)for(count=0;count<16;count++)REQUIRE(vram[1][tile][count]==0x6d);
    }
}
int main(void){
    due_boundaries();full_scenes();skip_and_invalid();restore_original_background();
    printf("Story source/controller/portrait identity:%lu checks,8 full acts/%u pages; host scope only\n",checks,ORACLE_PAGE_COUNT);
    return 0;
}
