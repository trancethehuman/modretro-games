#pragma bank 255
/* The pause menu, the dispatch board, the TTC destination menu and the
 * cards and result screens' buttons (moved out of TORONTO.c to give the
 * scene's own bank room). Runs while td.mode is a menu mode. */
#include <string.h>
#include "td_game.h"
#include "td_menu.h"
#include "td_audio.h"
#include "td_transit.h"
#include "td_life.h"
#include "td_special.h"
#include "td_interior.h"
#include "td_radio_data.h"
#include "input.h"

/* Enough deliveries in, and the story has reached it. */
static UBYTE td_offer_open(void){
    return td.done>=td_offer.min_done&&(td_offer.after==TD_NONE||TD_DONE(td_offer.after));
}

void td_ready_offer(void) BANKED {
    UBYTE i;
    for(i=0;i<TD_QUESTS;i++){
        td_get_job(i,&td_offer);
        if(td_offer_open()&&!TD_DONE(i)&&(td_offer.vehicle==TD_NONE||(!td.onfoot&&td.vehicle==td_offer.vehicle))){td.menu=i;return;}
    }
    td.menu=0;td_get_job(0,&td_offer);
}

static void td_transit_step(BYTE delta){
    UBYTE count=td_transit_count(td.transit_origin),n;
    for(n=0;n<count;n++){
        td.menu=(td.menu+count+delta)%count;td.transit_target=td_transit_stop(td.transit_origin,td.menu);
        if(td.transit_target!=(td.transit_origin&63))break;
    }
    td_get_stop(td.transit_target,&td_cursor);
}

static void td_pause_choose(void){
    if((td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)&&td.menu>TD_MENU_MAP&&td.menu!=TD_MENU_SOUND){td_message(2);return;}
    switch(td.menu){
        case TD_MENU_RESUME:td.mode=td_resume_mode;break;
        case TD_MENU_MAP:td.mode=TD_MAP;td_map_open();break;
        case TD_MENU_JOBS:td.mode=TD_BOARD;if(td.job==TD_NONE)td_ready_offer();else{td.menu=td.job;td_get_job(td.menu,&td_offer);}break;
        case TD_MENU_VEHICLE:
            if(td.job!=TD_NONE||td.speed>2||td.speed<-2||td.onfoot){td_message(2);return;}
            td.vehicle=(td.vehicle+1)&3;td_player_special=0;td_car_damage=0;td_car_colour=TD_PAL_COURIER;td_save();break;
        case TD_MENU_SUPPLIES:if(td_life_buy())td_save();td.mode=TD_ROAM;break;
        case TD_MENU_CANCEL:
            if(td.job==TD_NONE){td_message(2);return;}
            td.job=TD_NONE;td.speed=0;td.mode=TD_ROAM;td_set_target();td_save();break;
        case TD_MENU_SOUND:td_audio_set_mode((td_audio_get_mode()+1)%TD_AUDIO_MODES);break;
        case TD_MENU_GALLERY:if(!td_gallery_open()){td_message(2);return;}td.mode=TD_ROAM;break;
    }
    td_audio_play(TD_AUDIO_MENU);
    td_ui_draw();
}

void td_menu_update(void) BANKED {
    td_ui_tick();
    if(td.mode==TD_HELP){
        if(INPUT_A_PRESSED||INPUT_B_PRESSED){
            /* A fresh shift starts with Rosa's welcome. */
            if(!td.done&&td.job==TD_NONE)td_radio_say(TD_RADIO_INTRO);
            td.mode=td_resume_mode;td_ui_draw();
        }
        return;
    }
    if(td.mode==TD_BUSTED||td.mode==TD_WASTED){if(INPUT_A_PRESSED||INPUT_B_PRESSED){td.mode=TD_ROAM;td_resume_mode=TD_ROAM;td_ui_draw();}return;}
    if(td.mode==TD_MAP){
        if(INPUT_B_PRESSED||INPUT_START_PRESSED){td_map_close();td.mode=TD_PAUSE;td.menu=TD_MENU_MAP;td_ui_draw();}
        else td_map_update(joy,joy_pressed);
        return;
    }
    if(INPUT_B_PRESSED||INPUT_START_PRESSED){
        /* Browsing offers used the active contract's copy: reload it. */
        if(td.mode==TD_BOARD&&td.job!=TD_NONE)td_get_job(td.job,&td_job);
        td.mode=td.mode==TD_PAUSE?td_resume_mode:TD_ROAM;td_ui_draw();return;
    }
    if(td.mode==TD_PAUSE){
        if(INPUT_DOWN_PRESSED)td.menu=(td.menu+1)%TD_MENU_ITEMS;
        if(INPUT_UP_PRESSED)td.menu=(td.menu+TD_MENU_ITEMS-1)%TD_MENU_ITEMS;
        if(INPUT_A_PRESSED){td_pause_choose();return;}
    }else if(td.mode==TD_BOARD){
        if(INPUT_RIGHT_PRESSED){td.menu=(td.menu+1)%TD_QUESTS;td_get_job(td.menu,&td_offer);}
        if(INPUT_LEFT_PRESSED){td.menu=(td.menu+TD_QUESTS-1)%TD_QUESTS;td_get_job(td.menu,&td_offer);}
        if(INPUT_A_PRESSED){
            if(td.job!=TD_NONE){td_message(2);return;}
            if(!td_offer_open()){td_message(3);return;}
            if(td_offer.vehicle!=TD_NONE&&(td.onfoot||td.vehicle!=td_offer.vehicle)){td_message(2);return;}
            td.job=td.menu;td.stage=0;td.health=100;td.left=td_job.seconds;td.mode=TD_ROAM;td_audio_play(TD_AUDIO_MENU);
            td_radio_contract(td.job,0);td_set_target();td_save();td_ui_draw();return;
        }
    }else if(td.mode==TD_TRANSIT){
        if((INPUT_UP_PRESSED||INPUT_DOWN_PRESSED)&&(td.transit_origin&63)==16){
            td.transit_origin^=64;td.menu=0;td.transit_target=td_transit_stop(td.transit_origin,0);td_get_stop(td.transit_target,&td_cursor);
        }
        if(INPUT_RIGHT_PRESSED&&td_transit_count(td.transit_origin))td_transit_step(1);
        if(INPUT_LEFT_PRESSED&&td_transit_count(td.transit_origin))td_transit_step(-1);
        if(INPUT_A_PRESSED){
            if(!td_transit_valid(td.transit_origin,td.transit_target))return;
            /* Short of the fare: say so before waiting for a departure. */
            if(td.cash<td_transit_fare(td.transit_origin)){td.mode=TD_ROAM;td_save();td_message(4);return;}
            td.mode=TD_WAIT;td.speed=0;
            /* The displayed two-second window includes the current second;
               confirmation must not wait for another clock tick to board. */
            if(!td_board_current_window())td_save();
            td_ui_draw();return;
        }
    }else if(td.mode==TD_RESULT&&INPUT_A_PRESSED){td.mode=TD_BOARD;td_ready_offer();}
    if(INPUT_A_PRESSED||INPUT_UP_PRESSED||INPUT_DOWN_PRESSED||INPUT_LEFT_PRESSED||INPUT_RIGHT_PRESSED)td_ui_draw();
}
