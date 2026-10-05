#pragma bank 255
#include "td_game.h"
#include "td_menu.h"
#include "td_menu_actions.h"
#include "td_story_control.h"
#include "td_combat.h"
#include "td_motion.h"
#include "td_audio.h"
#include "td_transit.h"
#include "input.h"
#include "compat.h"

void td_ready_offer(void) BANKED {
    UBYTE i;td_board_route=0;
    for(i=0;i<TD_QUESTS;i++){
        td_get_job(i,&td_offer);
        if(td.done>=td_offer.min_done&&!(td.complete[i>>3]&(1<<(i&7)))&&(td_offer.vehicle==TD_NONE||(!td.onfoot&&td.vehicle==td_offer.vehicle))){td.menu=i;return;}
    }
    td.menu=0;td_get_job(0,&td_offer);
}
static void td_pause_choose(void){
    if((td.menu==3||td.menu==4||td.menu==5)&&(!td.vitality||td_combat_locked())){td_motion_notice(24);return;}
    if((td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)&&td.menu>1&&
       (td.menu<8||td.menu>TD_SETTINGS_BACK)){td_motion_notice(2);return;}
    switch(td.menu){
        case 0:td.mode=td_resume_mode;break;
        case 1:td.mode=TD_MAP;td_map_open();break;
        case 2:td.mode=TD_BOARD;td_board_route=0;if(td.job==TD_NONE)td_ready_offer();else{td.menu=td.job;td_get_job(td.menu,&td_offer);}break;
        case 3:td_motion_enter_exit();return;
        case 4:
            if(td.job!=TD_NONE||td.speed>2||td.speed<-2||td.onfoot){td_motion_notice(2);return;}
            td.vehicle=(td.vehicle+1)&3;td_save();break;
        case 5:td_motion_transit_open();return;
        case 6:td_save();td.mode=TD_ROAM;td_motion_notice(12);break;
        case 7:td.job=TD_NONE;td.speed=0;td.mode=TD_ROAM;td_set_target();td_save();break;
        case 8:td.menu=TD_SETTINGS_SOUND;break;
        case TD_SETTINGS_SOUND:td_audio_set_mode((td_audio_get_mode()+1)%TD_AUDIO_MODES);break;
        case TD_SETTINGS_CONTROLS:td.mode=TD_HELP;break;
        case TD_SETTINGS_BACK:td.menu=8;break;
    }
    td_audio_play(TD_AUDIO_MENU);
    td_ui_draw();
}
static void td_menu_update_inner(void){
    if(td.mode==TD_DIALOG){td_story_control_update(joy_pressed);return;}
    if(td.mode==TD_HELP){if(INPUT_A_PRESSED||INPUT_B_PRESSED){
        td.mode=td.menu==TD_SETTINGS_CONTROLS?TD_PAUSE:td_resume_mode;
        if(td.mode==TD_ROAM&&td.menu!=TD_SETTINGS_CONTROLS&&td_story_maybe_begin())return;
        td_ui_draw();}return;}
    if(td.mode==TD_MAP){
        if(INPUT_B_PRESSED||INPUT_START_PRESSED){td_map_close();td.mode=TD_PAUSE;td.menu=1;td_ui_draw();}
        else td_map_update(joy,joy_pressed);
        return;
    }
    if(INPUT_B_PRESSED||INPUT_START_PRESSED){
        if(td.mode==TD_PAUSE&&td.menu>=TD_SETTINGS_SOUND&&td.menu<=TD_SETTINGS_BACK)td.menu=8;
        else td.mode=td.mode==TD_PAUSE?td_resume_mode:TD_ROAM;
        td_ui_draw();return;
    }
    if(td.mode==TD_PAUSE){
        if(INPUT_SELECT_PRESSED){td.mode=TD_MAP;td_map_open();return;}
        if(td.menu>=TD_SETTINGS_SOUND&&td.menu<=TD_SETTINGS_BACK){
            if(INPUT_DOWN_PRESSED)td.menu=td.menu<TD_SETTINGS_BACK?td.menu+1:TD_SETTINGS_SOUND;
            if(INPUT_UP_PRESSED)td.menu=td.menu>TD_SETTINGS_SOUND?td.menu-1:TD_SETTINGS_BACK;
        }else{
            if(INPUT_DOWN_PRESSED)td.menu=(td.menu+1)%9;
            if(INPUT_UP_PRESSED)td.menu=(td.menu+8)%9;
        }
        if(INPUT_A_PRESSED){td_pause_choose();return;}
    }else if(td.mode==TD_BOARD){
        if(INPUT_SELECT_PRESSED){
            td.menu=(td.menu&0xf8)+8;
            if(td.menu>=TD_QUESTS)td.menu=0;
            td_get_job(td.menu,&td_offer);td_board_route=0;td_ui_draw();return;
        }
        if(INPUT_RIGHT_PRESSED){td.menu=(td.menu+1)%TD_QUESTS;td_get_job(td.menu,&td_offer);td_board_route=0;}
        if(INPUT_LEFT_PRESSED){td.menu=(td.menu+TD_QUESTS-1)%TD_QUESTS;td_get_job(td.menu,&td_offer);td_board_route=0;}
        if(td_offer.count){
            if(td_board_route>=td_offer.count)td_board_route=0;
            if(INPUT_UP_PRESSED)td_board_route=td_board_route?td_board_route-1:td_offer.count-1;
            else if(INPUT_DOWN_PRESSED)td_board_route=td_board_route+1<td_offer.count?td_board_route+1:0;
        }
        if(INPUT_A_PRESSED){
            if(td.job!=TD_NONE){td.mode=TD_ROAM;td_ui_draw();return;}
            if(!td.vitality||td_combat_locked()){td_motion_notice(24);return;}
            if(td.done<td_offer.min_done){td_motion_notice(3);return;}
            if(td_offer.vehicle!=TD_NONE&&(td.onfoot||td.vehicle!=td_offer.vehicle)){td_motion_notice(2);return;}
            td.job=td.menu;td_job=td_offer;td.stage=0;td.health=100;td.left=td_job.seconds;td.mode=TD_ROAM;td_audio_play(TD_AUDIO_MENU);td_set_target();td_save();td_ui_draw();return;
        }
    }else if(td.mode==TD_TRANSIT){
        if((INPUT_UP_PRESSED||INPUT_DOWN_PRESSED)&&(td.transit_origin&63)==16){
            td.transit_origin^=64;td.menu=0;td.transit_target=td_transit_stop(td.transit_origin,0);td_get_stop(td.transit_target,&td_cursor);
        }
        if(INPUT_RIGHT_PRESSED){UBYTE count=td_transit_count(td.transit_origin);if(count){td.menu=(td.menu+1)%count;td.transit_target=td_transit_stop(td.transit_origin,td.menu);td_get_stop(td.transit_target,&td_cursor);}}
        if(INPUT_LEFT_PRESSED){UBYTE count=td_transit_count(td.transit_origin);if(count){td.menu=(td.menu+count-1)%count;td.transit_target=td_transit_stop(td.transit_origin,td.menu);td_get_stop(td.transit_target,&td_cursor);}}
        if(INPUT_A_PRESSED){
            if(!td_transit_valid(td.transit_origin,td.transit_target))return;
            td.mode=TD_WAIT;td.speed=0;
            /* The displayed two-second window includes the current second;
               confirmation must not wait for another clock tick to board. */
            if(!td_board_current_window())td_save();
            td_ui_draw();return;
        }
    }else if(td.mode==TD_RESULT&&INPUT_A_PRESSED){if(td_story_maybe_begin())return;td.mode=TD_BOARD;td_ready_offer();}
    if(INPUT_A_PRESSED||INPUT_UP_PRESSED||INPUT_DOWN_PRESSED||INPUT_LEFT_PRESSED||INPUT_RIGHT_PRESSED)td_ui_draw();
}
void td_menu_update(UWORD elapsed) BANKED {
    UBYTE pressed=joy_pressed,mode=td.mode;
    if(mode==TD_PAUSE||mode==TD_BOARD||mode==TD_TRANSIT)joy_pressed|=td_menu_repeat(joy,elapsed);
    else td_menu_reset();
    td_menu_update_inner();joy_pressed=pressed;
    if(td.mode!=mode)td_menu_reset();
}
