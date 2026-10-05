/* Execute the actual native shop C against registered collision-map oracles. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host_shops.h"
#include "td_game.h"
#include "td_audio.h"
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
static unsigned checks,script_calls,save_calls,return_calls,clock_frames,font_calls,audio_calls;
static UBYTE script_copy[9];
static UBYTE displayed_rows[3][20],last_audio;
static td_state_t saved_state;
static void require(int ok,const char *why){checks++;if(!ok){fprintf(stderr,"FAIL after %u checks: %s\n",checks,why);exit(1);}}
UBYTE tile_at(UBYTE x,UBYTE y){return x<20&&y<18?grid[y*20+x]:15;}
void actor_set_frames(actor_t *actor,UBYTE start,UBYTE end){actor->frame_start=start;actor->frame_end=end;}
void set_win_tiles(UBYTE x,UBYTE y,UBYTE width,UBYTE height,const UBYTE *tiles){
    require(!x&&y<3&&width==20&&height==1,"Shop text escaped its three window rows");
    for(UBYTE i=0;i<20;i++)require(tiles[i]>=192&&tiles[i]<=237,"Shop text used unreserved font tiles");
    memcpy(displayed_rows[y],tiles,20);
}
void ui_set_pos(UBYTE x,UBYTE y){require(x==0&&(y==120||y==144),"Unexpected shop overlay position");window_y=y;}
void *script_execute(UBYTE bank,const UBYTE *script,void *context,UBYTE count){
    require(bank==1&&context==NULL&&!count,"Room change must execute owned WRAM script");
    script_calls++;memcpy(script_copy,script,9);return script_allowed?&script_copy:NULL;
}
void td_save(void){save_calls++;saved_state=td;}
void td_audio_play(UBYTE cue){audio_calls++;last_audio=cue;}
void td_ui_init(void){font_calls++;}
UBYTE td_district_current(void){return loaded_district;}
UBYTE td_district_queue(UBYTE district){require(district==td.district,"Shop returned to the wrong outdoor district");return_calls++;return return_allowed;}
UBYTE td_shop_world_seconds(UWORD elapsed){clock_frames+=elapsed;return clock_allowed;}
#include "shop_under_test.c"

static void setup(UBYTE room){
    memset(&td,0x5a,sizeof(td));memset(actors,0,sizeof(actors));
    td.vitality=100;td.ammo=12;td.mode=TD_ROAM;td.onfoot=1;td.district=td_shops[room].district;
    td.u=td_shops[room].u*16;td.v=td_shops[room].v*16;td.speed=0;
    script_allowed=return_allowed=clock_allowed=1;loaded_district=td.district;
    current_scene=td_shops[room].scene;grid=room==0?room_grid_grocery:room==1?room_grid_corner_store:room_grid_repair_shop;
    script_calls=save_calls=return_calls=clock_frames=font_calls=audio_calls=0;sys_time=0;joy=joy_pressed=0;actors_len=2;
    memset(displayed_rows,0,sizeof(displayed_rows));memset(&saved_state,0,sizeof(saved_state));last_audio=0;
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
/* Decode rendered font bytes against independent, literal player-facing labels.
 * The production glyph function is not reused as an expected-value oracle. */
static char rendered_char(UBYTE tile){
    if(tile==192)return ' ';
    if(tile>=193&&tile<=218)return 'A'+tile-193;
    if(tile>=219&&tile<=228)return '0'+tile-219;
    if(tile==230)return '/';
    if(tile==233)return '+';
    if(tile==237)return '$';
    return '?';
}
static void require_text(UBYTE row,const char *expected,const char *why){
    char rendered[21];size_t length=strlen(expected);
    require(length<=20,"Expected shop text exceeded hardware row");
    for(UBYTE i=0;i<20;i++)rendered[i]=rendered_char(displayed_rows[row][i]);
    rendered[20]=0;
    require(!strncmp(rendered,expected,length),why);
    for(size_t i=length;i<20;i++)require(rendered[i]==' ',"Shop label retained stale trailing glyphs");
}
static void start_menu_hint_checks(void){
    for(UBYTE room=0;room<3;room++){
        activate(room);td_state_t original=td;actor_t keeper=actors[1];
        unsigned old_saves=save_calls,old_returns=return_calls,old_audio=audio_calls,old_clock=clock_frames;
        PLAYER.pos.x=80*32;PLAYER.pos.y=120*32;
        tick(J_START|J_UP,J_START,1);
        require_text(0,td_shops[room].name,"Start hint lost the original shop title");
        require_text(1,"B EXIT TO START MENU","Fresh indoor Start did not show its full beginner menu/exit hint");
        require_text(2,"A TALK / B LEAVE","Start hint removed the existing shop interaction or exit controls");
        require(window_y==120&&td_shop_text_time==120,"Fresh Start did not show a bounded temporary three-row hint");
        require(clock_frames==old_clock+1&&PLAYER.pos.y==119*32&&PLAYER.pos.x==80*32,"Start hint paused the live interior clock or ordinary walking");
        require(!memcmp(&td,&original,sizeof(td))&&!memcmp(&actors[1],&keeper,sizeof(keeper))&&
                save_calls==old_saves&&return_calls==old_returns&&audio_calls==old_audio,
                "Start hint changed persistent game/keeper state, saved, exited or played an action cue");
        tick(J_START,0,120);
        require(!td_shop_text_time&&window_y==144,"Held Start repeatedly reopened the expired hint");
        require(clock_frames==old_clock+121&&!memcmp(&td,&original,sizeof(td))&&save_calls==old_saves&&!return_calls,
                "Held Start altered the indoor clock, persistent state, save count or scene");
        tick(0,0,1);tick(J_START,J_START,1);
        require_text(1,"B EXIT TO START MENU","A fresh Start after release could not redisplay the hint");
        tick(0,0,1);tick(J_B,J_B,1);
        require(td_shops_pending()&&return_calls==old_returns+1,"B after the Start hint failed to leave by the original supported route");
        /* A Start held through scene entry is consumed just like entry A/B;
           the initial welcome stays until release and a new button edge. */
        setup(room);require(td_shops_interact(),"Held-Start fixture could not enter the shop");
        loaded_district=TD_DISTRICT_NONE;joy=J_START;td_shop_init();
        tick(J_START,J_START,1);require_text(1,"WELCOME COURIER","Held scene-entry Start immediately overwrote the welcome");
        tick(0,0,1);tick(J_START,J_START,1);require_text(1,"B EXIT TO START MENU","Released entry Start did not permit the first deliberate hint");
    }
}
static void supplies_activate(UBYTE room,UBYTE health,UBYTE ammo,UWORD cash,UBYTE entry_held){
    setup(room);td.vitality=health;td.ammo=ammo;td.cash=cash;
    require(td_shops_interact(),"Supplies fixture could not enter marked shop");
    require(save_calls==1&&!memcmp(&saved_state,&td,sizeof(td)),"Shop entry did not save unchanged outdoor health/ammo/money");
    loaded_district=TD_DISTRICT_NONE;joy=entry_held;td_shop_init();
    PLAYER.pos.x=actors[1].pos.x;PLAYER.pos.y=actors[1].pos.y;
    require_text(1,health<100||ammo<12?"SUPPLIES $10 A TALK":"WELCOME COURIER","Entry omitted advertised $10 supplies price or replaced funded welcome");
    require_text(2,"A TALK / B LEAVE","Shop did not advertise beginner interaction and exit controls");
    require(td.cash==cash&&td.vitality==health&&td.ammo==ammo&&save_calls==1&&!audio_calls,"Advertising supplies silently purchased them");
}
static void supplies_checks(void){
    for(UBYTE room=0;room<3;room++){
        /* Every insufficient balance refuses both healing and replenishment,
         * including an empty wallet and the amount immediately below the fee. */
        for(UWORD cash=0;cash<10;cash++){
            supplies_activate(room,40,3,cash,0);td_state_t original=td;
            tick(J_A,J_A,1);
            require_text(1,"NEED $10 / SUPPLIES","Underfunded supplies failed to explain exact price in a complete hardware-width label");
            require(!memcmp(&td,&original,sizeof(td))&&save_calls==1&&!audio_calls,"Underfunded purchase charged, healed, replenished, saved or played success audio");
            tick(J_A,0,120);
            require(!memcmp(&td,&original,sizeof(td))&&save_calls==1&&!audio_calls,"Held underfunded A changed supplies state");
        }
        const UBYTE health_cases[]={0,1,25,74,75,76,99,100};
        const UBYTE ammo_cases[]={0,1,11,12,13,24};
        for(UBYTE h=0;h<sizeof(health_cases);h++)for(UBYTE a=0;a<sizeof(ammo_cases);a++){
            UBYTE health=health_cases[h],ammo=ammo_cases[a];
            supplies_activate(room,health,ammo,10,0);td_state_t expected=td;
            UBYTE eligible=health<100||ammo<12;
            if(eligible){expected.cash=0;expected.vitality=health>75?100:health+25;expected.ammo=ammo<12?12:ammo;}
            tick(J_A,J_A,1);
            require(!memcmp(&td,&expected,sizeof(td)),"Exact-$10 supply purchase changed unrelated state, failed +25 health cap or reduced carried ammunition");
            require(save_calls==(eligible?2u:1u)&&audio_calls==(eligible?1u:0u),"Supplies saved/played success more than once or saved a fully supplied greeting");
            if(eligible){
                require(!memcmp(&saved_state,&expected,sizeof(td))&&last_audio==TD_AUDIO_PICKUP,"Supply save/audio did not contain the purchased state");
                require_text(1,"HEALTH +25 / AMMO 12","Purchase omitted healing/ammunition receipt");
            }else require_text(1,td_shops[room].greeting,"Fully supplied courier lost original shopkeeper greeting");
        }
        /* A held through entry cannot buy. A real release and subsequent press
         * permits one purchase; long held periods cannot drain the wallet. */
        supplies_activate(room,0,24,100,J_A);td_state_t expected=td;
        tick(J_A,J_A,1);tick(J_A,0,4);tick(J_A,0,120);
        require(!memcmp(&td,&expected,sizeof(td))&&save_calls==1&&!audio_calls,"Held entry button immediately purchased supplies");
        tick(0,0,1);tick(J_A,J_A,1);expected.cash=90;expected.vitality=25;
        require(!memcmp(&td,&expected,sizeof(td))&&save_calls==2&&audio_calls==1,"First fresh purchase did not charge exactly $10 or retained-ammo healing failed");
        const UWORD held_frames[]={1,2,4,120,65500};
        for(UBYTE i=0;i<sizeof(held_frames)/sizeof(*held_frames);i++){
            tick(J_A,0,held_frames[i]);
            require(!memcmp(&td,&expected,sizeof(td))&&save_calls==2&&audio_calls==1,"Held A repeatedly deducted the supplies fee");
        }
        tick(0,0,1);tick(J_A,J_A,1);expected.cash=80;expected.vitality=50;
        require(!memcmp(&td,&expected,sizeof(td))&&save_calls==3&&audio_calls==2&&!memcmp(&saved_state,&expected,sizeof(td)),"Released-and-repressed A failed to save one further deliberate purchase");
        /* Proximity still matters: neither distance nor absent keeper can buy. */
        supplies_activate(room,0,0,100,0);expected=td;
        PLAYER.pos.x=(room==1?149:10)*32;PLAYER.pos.y=(room==1?34:120)*32;tick(J_A,J_A,1);
        require(!memcmp(&td,&expected,sizeof(td))&&save_calls==1&&!audio_calls,"Distant courier purchased from an unreachable shopkeeper");
        PLAYER.pos.x=actors[1].pos.x;PLAYER.pos.y=actors[1].pos.y;actors_len=1;
        tick(0,0,1);tick(J_A,J_A,1);
        require(!memcmp(&td,&expected,sizeof(td))&&save_calls==1&&!audio_calls,"Missing shopkeeper sold supplies");
    }
    /* Exhaust all valid health/ammo combinations in one actual room. Every
     * other room shares the same native routine and is covered above. */
    for(unsigned health=0;health<=100;health++)for(unsigned ammo=0;ammo<=24;ammo++){
        supplies_activate(0,health,ammo,100,0);td_state_t expected=td;
        UBYTE eligible=health<100||ammo<12;
        if(eligible){expected.cash=90;expected.vitality=health>75?100:health+25;expected.ammo=ammo<12?12:ammo;}
        tick(J_A,J_A,1);
        require(!memcmp(&td,&expected,sizeof(td)),"Valid health/ammo combination violated price, capped healing, ammo floor or unrelated-state preservation");
        require(save_calls==(eligible?2u:1u)&&audio_calls==(eligible?1u:0u),"Valid health/ammo combination had wrong purchase save/audio count");
        if(eligible)require(!memcmp(&saved_state,&expected,sizeof(td)),"Saved supplies differ from actual purchased state");
    }
}
int main(void){transition_checks();room_checks();start_menu_hint_checks();supplies_checks();printf("Native shop regressions passed: %u checks across three registered rooms, allocation failures, full-body collisions, entry/exit, clock, saved-state preservation, Start hints and $10 supplies/held-input cases.\n",checks);return 0;}
