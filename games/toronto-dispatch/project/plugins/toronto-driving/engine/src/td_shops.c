#pragma bank 255
#include <stdint.h>
#include "td_shops.h"
#include "td_game.h"
#include "td_audio.h"
#include "td_district.h"
#include "actor.h"
#include "camera.h"
#include "collision.h"
#include "data_manager.h"
#include "input.h"
#include "scroll.h"
#include "system.h"
#include "ui.h"
#include "vm.h"
#include "vm_exceptions.h"
#include "data/scene_toronto_shop_grocery.h"
#include "data/scene_toronto_shop_corner_store.h"
#include "data/scene_toronto_shop_repair_shop.h"

typedef struct {UWORD u,v;UBYTE district;far_ptr_t scene;const char *name;const char *greeting;} td_shop_t;
static const td_shop_t td_shops[]={
    {124,152,0,TO_FAR_PTR_T(scene_toronto_shop_grocery),"HARBOUR GROCER","FRESH FOOD DAILY"},
    {64,180,1,TO_FAR_PTR_T(scene_toronto_shop_corner_store),"WEST END CORNER","GOOD LUCK COURIER"},
    {768,148,3,TO_FAR_PTR_T(scene_toronto_shop_repair_shop),"EAST END REPAIRS","DRIVE SAFE OUT THERE"}
};
/* Same process-owned WRAM VM_LOCK/FADE/RAISE sequence as td_district.c. */
static UBYTE td_shop_script[9]={0x25,0x57,0x01,0x27,3,EXCEPTION_CHANGE_SCENE,0,0,0};
static UBYTE td_shop_active=TD_NONE,td_shop_loading,td_shop_direction,td_shop_step,td_shop_text_time,td_shop_buttons;
static UWORD td_shop_last_frame;
static UBYTE td_shop_text[20];

static UWORD distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
void td_shops_reset(void) BANKED {td_shop_active=TD_NONE;td_shop_loading=0;}
UBYTE td_shops_pending(void) BANKED {
    /* A completed return has replaced the room. Entry keeps a valid shop ID,
     * so the outdoor caller still freezes immediately after queuing it. */
    if(td_shop_loading&&td_shop_active==TD_NONE&&td_district_current()!=TD_DISTRICT_NONE)td_shop_loading=0;
    return td_shop_loading;
}

UBYTE td_shops_interact(void) BANKED {
    UBYTE i;UWORD address;
    if(td_shop_loading||td_shop_active!=TD_NONE||!td.onfoot||td.mode!=TD_ROAM)return FALSE;
    for(i=0;i<3;i++){
        if(td.district!=td_shops[i].district||distance(td.u>>4,td_shops[i].u)>12||distance(td.v>>4,td_shops[i].v)>12)continue;
        address=(UWORD)(uintptr_t)td_shops[i].scene.ptr;
        td_shop_script[6]=td_shops[i].scene.bank;
        td_shop_script[7]=(UBYTE)address;td_shop_script[8]=(UBYTE)(address>>8);
        if(script_execute(1,td_shop_script,NULL,0)==NULL)return FALSE;
        td_shop_active=i;td_shop_loading=1;td_save();return TRUE;
    }
    return FALSE;
}

static UBYTE glyph(char c){
    if(c>='A'&&c<='Z')return 193+c-'A';
    if(c>='0'&&c<='9')return 219+c-'0';
    return c=='/'?230:c=='$'?237:c=='+'?233:192;
}
static void row(UBYTE y,const char *s){
    UBYTE i;
    for(i=0;i<20;i++){td_shop_text[i]=glyph(*s);if(*s)s++;}
    VBK_REG=0;set_win_tiles(0,y,20,1,td_shop_text);
}
static void show_text(const char *message){
    row(0,td_shops[td_shop_active].name);row(1,message);row(2,"A TALK / B LEAVE");
    ui_set_pos(0,120);td_shop_text_time=120;
}
static UBYTE leave(void){
    if(!td_district_queue(td.district))return FALSE;
    td_shop_active=TD_NONE;td_shop_loading=1;return TRUE;
}
static UBYTE clear(UWORD x,UWORD y){
    UBYTE tx,ty;
    if(x<10||x>149||y<34||y>133)return FALSE;
    for(ty=(y-2)>>3;ty<=((y+2)>>3);ty++)
        for(tx=(x-2)>>3;tx<=((x+2)>>3);tx++)if(tile_at(tx,ty)&COLLISION_ALL)return FALSE;
    return TRUE;
}

void td_shop_init(void) BANKED {
    UBYTE i;
    /* Cold/soft bootstrap normally starts in Core. A direct editor Run Scene
     * also resolves this actual room rather than trusting stale transient ID. */
    td_shop_active=TD_NONE;
    for(i=0;i<3;i++)if(current_scene.bank==td_shops[i].scene.bank&&current_scene.ptr==td_shops[i].scene.ptr){td_shop_active=i;break;}
    td_shop_loading=0;td_shop_last_frame=sys_time;td_shop_direction=td_shop_step=0;td_shop_buttons=joy;
    PLAYER.pos.x=80*32;PLAYER.pos.y=120*32;
    actor_set_frames(&PLAYER,32,33);PLAYER.anim_tick=255;
    /* Full-screen rooms have no outdoor crowd, aircraft, camera scroll or
     * vehicle physics. Authored keeper uses the existing civilian sheet. */
    camera_settings=0;camera_x=camera_y=0;
    camera_offset_x=camera_offset_y=camera_deadzone_x=camera_deadzone_y=0;
    td_ui_init();
    if(td_shop_active<3)show_text(td.vitality<100||td.ammo<12?"SUPPLIES $10 A TALK":"WELCOME COURIER");else ui_set_pos(0,144);
}

void td_shop_update(void) BANKED {
    UWORD now,elapsed,x,y,nx,ny;UBYTE n,moving=0,pressed;
    if(td_shop_loading)return;
    if(td_shop_active>=3){leave();return;}
    now=sys_time;elapsed=now-td_shop_last_frame;td_shop_last_frame=now;
    if(!td_shop_world_seconds(elapsed)){leave();return;}
    pressed=joy_pressed&~td_shop_buttons;td_shop_buttons&=joy;
    if(td_shop_text_time){
        td_shop_text_time=elapsed>=td_shop_text_time?0:td_shop_text_time-elapsed;
        if(!td_shop_text_time)ui_set_pos(0,144);
    }else ui_set_pos(0,144);
    /* Interiors retain their live clock and simple exit controls. Start only
     * explains how to reach the outdoor menu; it never pauses or saves. */
    if(pressed&J_START)show_text("B EXIT TO START MENU");
    /* Fresh B is a simple, discoverable exit from anywhere inside. A near
     * the marked bottom doorway also returns; held entry A cannot exit. */
    x=PLAYER.pos.x>>5;y=PLAYER.pos.y>>5;
    if((pressed&J_B)||((pressed&J_A)&&distance(x,80)<=16&&y>=124)){leave();return;}
    if((pressed&J_A)&&actors_len>1&&distance(x,actors[1].pos.x>>5)<40&&distance(y,actors[1].pos.y>>5)<40){
        if(td.vitality<100||td.ammo<12){
            if(td.cash<10)show_text("NEED $10 / SUPPLIES");
            else{td.cash-=10;td.vitality=td.vitality>75?100:td.vitality+25;if(td.ammo<12)td.ammo=12;td_save();td_audio_play(TD_AUDIO_PICKUP);show_text("HEALTH +25 / AMMO 12");}
        }else show_text(td_shops[td_shop_active].greeting);
    }
    for(n=0;n<(elapsed>4?4:elapsed);n++){
        nx=x;ny=y;
        if(joy&J_LEFT){if(nx)nx--;td_shop_direction=1;moving=1;}
        else if(joy&J_RIGHT){nx++;td_shop_direction=0;moving=1;}
        else if(joy&J_UP){if(ny)ny--;td_shop_direction=3;moving=1;}
        else if(joy&J_DOWN){ny++;td_shop_direction=2;moving=1;}
        if(clear(nx,ny)){x=nx;y=ny;}
    }
    PLAYER.pos.x=x*32;PLAYER.pos.y=y*32;
    if(moving)td_shop_step+=elapsed;
    n=32+td_shop_direction*2+(moving?((td_shop_step>>3)&1):0);
    actor_set_frames(&PLAYER,n,n+1);PLAYER.anim_tick=255;
    /* The bottom interior door is a walk-out threshold after entry movement.
     * Courier saved coordinates remain outdoors even while this actor walks. */
    if(distance(x,80)<=12&&y>=132)leave();
}
