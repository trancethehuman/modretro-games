#pragma bank 255
/* Rosa on dispatch: radio calls that tell the story of the shift. A call is
 * a card above the HUD (drawn by td_ui.c) whose two lines type out a
 * character a frame, hold about two and a half seconds and move on to the
 * next page. Play continues underneath and menus suspend a call. Lines are
 * generated from content/radio.json by scripts/create_radio.py. */
#include "td_game.h"
#include "td_life.h"
#include "td_audio.h"
#include "td_daynight.h"
#define TD_RADIO_DATA
#include "td_radio_data.h"

#define TD_RADIO_CHATTER_SECONDS 90
UBYTE td_radio_script=TD_NONE,td_radio_next=TD_NONE,td_radio_page,td_radio_pos,td_radio_hold;
/* Story-beat trackers: stars last seen, day or night, quiet seconds. */
UBYTE td_radio_wanted;
static UBYTE td_radio_night=255,td_radio_quiet,td_radio_fresh;

static void td_radio_start(UBYTE script){
    td_radio_script=script;td_radio_page=td_radio_first[script];td_radio_pos=0;td_radio_hold=0;td_radio_quiet=0;td_radio_fresh=1;
    td_audio_play(TD_AUDIO_MENU);
}
void td_radio_say(UBYTE script) BANKED {
    /* An arrest or the hospital clears the stars without a "lost them". */
    if(script==TD_RADIO_BUSTED||script==TD_RADIO_WASTED)td_radio_wanted=0;
    if(td_radio_script==TD_NONE)td_radio_start(script);
    else if(td_radio_script!=script)td_radio_next=script;
}
UBYTE td_radio_playing(void) BANKED {return td_radio_script;}
void td_radio_line(UBYTE line,char *dest) BANKED {
    UBYTE n,k=line?TD_RADIO_COLS:0;const char *page=td_radio_pages[td_radio_page];
    for(n=0;n<TD_RADIO_COLS;n++)dest[n]=k+n<td_radio_pos?page[k+n]:' ';
    dest[TD_RADIO_COLS]=0;
}
void td_chapter_name(UBYTE chapter,char *dest) BANKED {
    const char *s=td_chapter_names[chapter<TD_CHAPTERS?chapter:TD_CHAPTERS-1];
    while(*s)*dest++=*s++;
    *dest=0;
}
/* Once per rendered update in the HUD modes. */
void td_radio_tick(void) BANKED {
    UWORD minutes;UBYTE night,c;const char *page;
    /* Story beats from the city's state. */
    if(td.wanted&&!td_radio_wanted)td_radio_say(TD_RADIO_WANTED);
    else if(!td.wanted&&td_radio_wanted&&td.mode==TD_ROAM)td_radio_say(TD_RADIO_LOST);
    td_radio_wanted=td.wanted;
    if(!(td_tick&63)){
        minutes=td_daynight_minutes();night=minutes>=19*60||minutes<7*60;
        if(night!=td_radio_night){if(td_radio_night!=255)td_radio_say(night?TD_RADIO_NIGHT:TD_RADIO_MORNING);td_radio_night=night;}
        /* Chatter on quiet stretches of free roam. */
        if(td.job!=TD_NONE||td.wanted||td.mode!=TD_ROAM)td_radio_quiet=0;
        else if(td_radio_script==TD_NONE&&++td_radio_quiet>=TD_RADIO_CHATTER_SECONDS)
            td_radio_say(TD_RADIO_CHATTER+((UBYTE)(td.seconds+td.done)&(TD_RADIO_CHATTER_COUNT-1)));
    }
    if(td_radio_script==TD_NONE||!td_ui_radio_ready())return;
    /* Every call starts on a freshly painted card. */
    if(td_radio_fresh){td_radio_fresh=0;td_ui_draw_radio();}
    page=td_radio_pages[td_radio_page];
    if(td_radio_pos<TD_RADIO_PAGE){
        /* Spaces are already blank: type through them to the next letter. */
        while(td_radio_pos<TD_RADIO_PAGE&&page[td_radio_pos]==' ')td_radio_pos++;
        if(td_radio_pos<TD_RADIO_PAGE){
            c=td_radio_pos<TD_RADIO_COLS?td_radio_pos:td_radio_pos-TD_RADIO_COLS;
            td_ui_radio_put(c,td_radio_pos>=TD_RADIO_COLS,page[td_radio_pos]);td_radio_pos++;
        }
        return;
    }
    if(++td_radio_hold<TD_RADIO_HOLD)return;
    td_radio_hold=0;td_radio_pos=0;
    if(++td_radio_page<td_radio_first[td_radio_script+1]){td_ui_draw_radio();return;}
    td_radio_script=TD_NONE;
    /* A queued call follows on a freshly painted card. */
    if(td_radio_next!=TD_NONE){c=td_radio_next;td_radio_next=TD_NONE;td_radio_start(c);return;}
    td_ui_draw();
}
