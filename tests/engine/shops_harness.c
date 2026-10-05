/* Execute the actual native shop C against registered collision-map oracles. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host_shops.h"
#include "td_game.h"
#include "td_district.h"
#include "shop_grid_oracle.h"

actor_t actors[22];
UBYTE actors_len,VBK_REG,joy,joy_pressed,camera_settings;
WORD camera_x,camera_y,camera_offset_x,camera_offset_y,camera_deadzone_x,camera_deadzone_y;
UWORD sys_time;
far_ptr_t current_scene;
td_state_t td;
const UBYTE scene_toronto_shop_grocery=0,scene_toronto_shop_corner_store=1,scene_toronto_shop_repair_shop=2;
static const UBYTE *grid;
static UBYTE script_allowed,return_allowed,clock_allowed,loaded_district,window_y;
static unsigned checks,script_calls,save_calls,return_calls,clock_frames,font_calls;
static UBYTE script_copy[9];
static void require(int ok,const char *why){checks++;if(!ok){fprintf(stderr,"FAIL after %u checks: %s\n",checks,why);exit(1);}}
UBYTE tile_at(UBYTE x,UBYTE y){return x<20&&y<18?grid[y*20+x]:15;}
void actor_set_frames(actor_t *actor,UBYTE start,UBYTE end){actor->frame_start=start;actor->frame_end=end;}
void set_win_tiles(UBYTE x,UBYTE y,UBYTE width,UBYTE height,const UBYTE *tiles){
    require(!x&&y<3&&width==20&&height==1,"Shop text escaped its three window rows");
    for(UBYTE i=0;i<20;i++)require(tiles[i]>=192&&tiles[i]<=230,"Shop text used unreserved font tiles");
}
void ui_set_pos(UBYTE x,UBYTE y){require(x==0&&(y==120||y==144),"Unexpected shop overlay position");window_y=y;}
void *script_execute(UBYTE bank,const UBYTE *script,void *context,UBYTE count){
    require(bank==1&&context==NULL&&!count,"Room change must execute owned WRAM script");
    script_calls++;memcpy(script_copy,script,9);return script_allowed?&script_copy:NULL;
}
void td_save(void){save_calls++;}
void td_ui_init(void){font_calls++;}
UBYTE td_district_current(void){return loaded_district;}
UBYTE td_district_queue(UBYTE district){require(district==td.district,"Shop returned to the wrong outdoor district");return_calls++;return return_allowed;}
UBYTE td_shop_world_seconds(UWORD elapsed){clock_frames+=elapsed;return clock_allowed;}
#include "shop_under_test.c"

static void setup(UBYTE room){
    memset(&td,0x5a,sizeof(td));memset(actors,0,sizeof(actors));
    td.mode=TD_ROAM;td.onfoot=1;td.district=td_shops[room].district;
    td.u=td_shops[room].u*16;td.v=td_shops[room].v*16;td.speed=0;
    script_allowed=return_allowed=clock_allowed=1;loaded_district=td.district;
    current_scene=td_shops[room].scene;grid=room==0?room_grid_grocery:room==1?room_grid_corner_store:room_grid_repair_shop;
    script_calls=save_calls=return_calls=clock_frames=font_calls=0;sys_time=0;joy=joy_pressed=0;actors_len=2;
    actors[1].pos.x=(room==0?80:room==1?32:120)*32;actors[1].pos.y=(room==0?32:room==1?88:64)*32;
    td_shops_reset();
}
static void tick(UBYTE held,UBYTE pressed,UWORD frames){joy=held;joy_pressed=pressed;sys_time+=frames;td_shop_update();}
static void activate(UBYTE room){
    setup(room);require(td_shops_interact(),"Marked door did not enter room");loaded_district=TD_DISTRICT_NONE;td_shop_init();
    require(!td_shops_pending()&&td_shop_active==room,"Actual room resource did not bind its native state");
}
static void transition_checks(void){
    const UBYTE prefix[]={0x25,0x57,0x01,0x27,3,EXCEPTION_CHANGE_SCENE};
    for(UBYTE room=0;room<3;room++){
        setup(room);td_state_t original=td;script_allowed=0;
        require(!td_shops_interact()&&!td_shops_pending()&&!save_calls&&!memcmp(&td,&original,sizeof(td)),"Rejected entry changed outdoor state/save/pending flag");
        script_allowed=1;require(td_shops_interact()&&td_shops_pending()&&save_calls==1,"Accepted entry did not freeze and save once");
        require(!memcmp(script_copy,prefix,6)&&script_copy[6]==42,"Entry lost locked fade/change-scene payload");
        UWORD address=(UWORD)(uintptr_t)td_shops[room].scene.ptr;
        require(script_copy[7]==(UBYTE)address&&script_copy[8]==(UBYTE)(address>>8),"Entry targeted wrong compiled room pointer");
        require(!td_shops_interact()&&save_calls==1,"Pending room load requeued or resaved entry");
        tick(J_DOWN,0,3);require(!clock_frames&&!return_calls,"World/movement advanced while entry was queued");
        loaded_district=TD_DISTRICT_NONE;joy=joy_pressed=J_A;td_shop_init();
        require(font_calls==1&&PLAYER.pos.x==80*32&&PLAYER.pos.y==120*32&&PLAYER.frame_start==32,"Shop did not initialize separate courier actor");
        require(!camera_settings&&!camera_x&&!camera_y&&!camera_offset_x&&!camera_offset_y&&!camera_deadzone_x&&!camera_deadzone_y,"Full-screen room retained outdoor camera offsets");
        require(!memcmp(&td,&original,sizeof(td)),"Room initialization overwrote persistent outdoor position/state");
        PLAYER.pos.y=128*32;tick(J_A,J_A,1);require(!return_calls,"Held entry A immediately exited shop");
        tick(0,0,1);tick(J_A,J_A,1);require(return_calls==1&&td_shops_pending(),"Fresh A at doorway did not queue return");
        require(!memcmp(&td,&original,sizeof(td)),"Shop return altered saved outdoor courier/car/mission state");
        unsigned frames=clock_frames;tick(J_RIGHT,0,3);require(clock_frames==frames,"Queued return continued advancing clock");
        loaded_district=td.district;require(!td_shops_pending(),"Completed outdoor return retained pending freeze");
        td_shops_reset();require(!td_shops_pending()&&td_shop_active==TD_NONE,"Cold/soft reset retained interior transient state");
    }
    setup(0);td.onfoot=0;require(!td_shops_interact(),"Driving courier entered shop");
    td.onfoot=1;td.mode=TD_RESULT;require(!td_shops_interact(),"Result screen entered shop");
    td.mode=TD_ROAM;td.u+=13*16;require(!td_shops_interact(),"Distant courier activated shop doorway");
}
static int body_clear(unsigned x,unsigned y){
    if(x<10||x>149||y<34||y>133)return 0;
    for(unsigned cy=(y-2)/8;cy<=(y+2)/8;cy++)for(unsigned cx=(x-2)/8;cx<=(x+2)/8;cx++)if(grid[cy*20+cx]&15)return 0;
    return 1;
}
static void room_checks(void){
    const UBYTE directions[]={J_LEFT,J_RIGHT,J_UP,J_DOWN};
    const UBYTE first_frames[]={34,32,38,36};
    for(UBYTE room=0;room<3;room++){
        activate(room);td_state_t original=td;
        /* Exhaust each clear room position and all movement directions against
         * the independently decoded full-body collision map. */
        for(unsigned y=34;y<=128;y+=2)for(unsigned x=10;x<=149;x+=2){
            if(!body_clear(x,y))continue;
            for(UBYTE d=0;d<4;d++){
                PLAYER.pos.x=x*32;PLAYER.pos.y=y*32;
                unsigned nx=x+(d==0?-1:d==1?1:0),ny=y+(d==2?-1:d==3?1:0);
                int accepted=body_clear(nx,ny);
                tick(directions[d],0,1);
                require(PLAYER.pos.x==(accepted?nx:x)*32&&PLAYER.pos.y==(accepted?ny:y)*32,"Native courier crossed a shelf/wall or rejected clear floor");
                require((PLAYER.frame_start&~1)==first_frames[d],"Shop direction disagrees with approved outdoor courier pose meanings");
            }
        }
        require(!memcmp(&td,&original,sizeof(td)),"Interior walking mutated persistent outdoor state");
        PLAYER.pos.x=80*32;PLAYER.pos.y=120*32;tick(J_UP,0,120);
        require(PLAYER.pos.y==116*32&&window_y==144,"Long frame catchup exceeded four-pixel budget or left text obscuring room");
        PLAYER.pos.x=actors[1].pos.x;PLAYER.pos.y=actors[1].pos.y;tick(J_A,J_A,1);
        require(window_y==120&&td_shop_text_time==120,"Nearby shopkeeper did not show original greeting");
        return_allowed=0;tick(J_B,J_B,1);require(td_shop_active==room&&!td_shops_pending(),"Rejected exit stranded room in pending state");
        tick(0,0,1);return_allowed=1;tick(J_B,J_B,1);
        require(td_shops_pending()&&td_shop_active==TD_NONE,"Fresh B did not leave room from anywhere");
        activate(room);PLAYER.pos.x=80*32;PLAYER.pos.y=128*32;tick(J_DOWN,0,4);
        require(td_shops_pending()&&return_calls==1,"Marked door walk-out failed");
        activate(room);td.mode=TD_RESULT;clock_allowed=0;tick(J_RIGHT,0,1);
        require(td_shops_pending()&&return_calls==1&&td.mode==TD_RESULT,"Expired mission did not return outdoors preserving result state");
    }
}
int main(void){transition_checks();room_checks();printf("Native shop regressions passed: %u checks across three registered rooms, allocation failures, full-body collisions, entry/exit, clock and saved-state preservation.\n",checks);return 0;}
