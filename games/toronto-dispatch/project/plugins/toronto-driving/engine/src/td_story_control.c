#pragma bank 255
#include "td_game.h"
#include "td_story.h"
#include "td_story_control.h"
#include "td_menu_actions.h"
#include "td_motion.h"
#include "td_audio.h"
#include "input.h"

UBYTE td_story_maybe_begin(void) BANKED {
    UBYTE scene;
    if(td.job!=TD_NONE||td.mode==TD_WAIT||td.mode==TD_RIDE)return FALSE;
    scene=td_story_due();if(scene==TD_NONE)return FALSE;
    td_ui_story_frame();td_story_begin(scene);td_story_draw();
    td_result_b_release|=joy&(J_A|J_B);return TRUE;
}
void td_story_control_update(UBYTE pressed) BANKED {
    if(td.mode!=TD_DIALOG)return;
    if(td_story_input(pressed)){
        td.mode=TD_ROAM;td_save();
        /* A chapter follows the result and then returns to the same job board.
           Intro returns to the street to learn navigation without an offer. */
        if(td.done){td.mode=TD_BOARD;td_ready_offer();}
        td_result_b_release|=joy&(J_A|J_B);
        td_ui_story_close();td_audio_play(TD_AUDIO_MENU);
    }else if(pressed&J_A)td_story_draw();
}
