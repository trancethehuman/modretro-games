#pragma bank 255
/* The radio: the story told in calls. A call is a card above the HUD (drawn
 * by td_ui.c) whose three lines type out a character a frame, hold about two
 * seconds and move on to the next page; each page names its speaker (Rosa,
 * Margo or a client patched through). Play continues underneath and menus
 * suspend a call. Calls come from the city's state (stars, night, quiet
 * stretches), three per contract (the briefing, the client at the first
 * pickup and the delivery) and from the story a first delivery moves on:
 * the beat that follows that contract, a chapter's opening once it unlocks
 * and the finale. Story calls wait their turn in a short queue. The text
 * lives in td_radio_text*.c, generated from content/radio.json and
 * content/story.json by scripts/create_radio.py. */
#include "td_game.h"
#include "td_life.h"
#include "td_audio.h"
#include "td_daynight.h"
#define TD_RADIO_DATA
#include "td_radio_data.h"

#define TD_RADIO_CHATTER_SECONDS 90
/* Playing and queued calls: a script id (TD_RADIO_CONTRACT for contract
 * calls) with its page range. */
UBYTE td_radio_script=TD_NONE,td_radio_next=TD_NONE,td_radio_pos,td_radio_hold;
UWORD td_radio_page;
static UWORD td_radio_end,td_radio_next_first,td_radio_next_end;
/* Story calls waiting for the radio to fall quiet, oldest first. */
#define TD_RADIO_STORY_SLOTS 4
static UBYTE td_radio_story[TD_RADIO_STORY_SLOTS],td_radio_stories;
/* Story-beat trackers: stars last seen, day or night, quiet seconds. */
UBYTE td_radio_wanted;
static UBYTE td_radio_night=255,td_radio_quiet,td_radio_fresh;

/* Page text from whichever bank holds it. */
static void td_radio_text(UWORD page,UBYTE from,UBYTE n,char *dest){
#if TD_RADIO_TEXT_BANKS>2
#error "add a text bank to td_radio_text"
#endif
    if(page<TD_RADIO_BANK_PAGES)td_radio_text0(page,from,n,dest);
#if TD_RADIO_TEXT_BANKS>1
    else td_radio_text1(page,from,n,dest);
#endif
}
static UBYTE td_radio_page_speaker(UWORD page){
    if(page<TD_RADIO_BANK_PAGES)return td_radio_speaker0(page);
#if TD_RADIO_TEXT_BANKS>1
    return td_radio_speaker1(page);
#else
    return 0;
#endif
}
static void td_radio_begin(UBYTE script,UWORD first,UWORD end){
    td_radio_script=script;td_radio_page=first;td_radio_end=end;
    td_radio_pos=0;td_radio_hold=0;td_radio_quiet=0;td_radio_fresh=1;
    td_audio_play(TD_AUDIO_MENU);
}
/* Starts a call, or queues it behind the one playing. Ambient calls
 * (chatter, night and morning) give way at once; a repeated call is not
 * queued twice. */
static void td_radio_queue(UBYTE script,UWORD first,UWORD end){
    if(first==end)return;
    if(td_radio_script==TD_NONE||(td_radio_script!=TD_RADIO_CONTRACT&&TD_RADIO_INTERRUPTIBLE(td_radio_script)))
        td_radio_begin(script,first,end);
    else if(td_radio_script!=script||script==TD_RADIO_CONTRACT){
        td_radio_next=script;td_radio_next_first=first;td_radio_next_end=end;
    }
}
void td_radio_say(UBYTE script) BANKED {
    /* An arrest or the hospital clears the stars without a "lost them". */
    if(script==TD_RADIO_BUSTED||script==TD_RADIO_WASTED)td_radio_wanted=0;
    td_radio_queue(script,td_radio_first[script],td_radio_first[script+1]);
}
/* A contract's briefing (part 0), first pickup (1) or delivery (2). A
 * delivery cuts short older talk about the contract and drops a queued
 * pickup line; a pickup waits for its briefing to finish. */
void td_radio_contract(UBYTE job,UBYTE part) BANKED {
    UWORD k;
    if(job>=TD_QUESTS||part>2)return;
    k=(UWORD)job*3+part;
    if(part==2){
        if(td_radio_next==TD_RADIO_CONTRACT)td_radio_next=TD_NONE;
        if(td_radio_script==TD_RADIO_CONTRACT){td_radio_begin(TD_RADIO_CONTRACT,td_contract_first[k],td_contract_first[k+1]);return;}
    }
    td_radio_queue(TD_RADIO_CONTRACT,td_contract_first[k],td_contract_first[k+1]);
}
static void td_radio_story_add(UBYTE script){
    if(td_radio_stories<TD_RADIO_STORY_SLOTS)td_radio_story[td_radio_stories++]=script;
}
/* Starts the oldest waiting story call; FALSE when none waits. */
static UBYTE td_radio_story_next(void){
    UBYTE i,script;
    if(!td_radio_stories)return FALSE;
    script=td_radio_story[0];td_radio_stories--;
    for(i=0;i<td_radio_stories;i++)td_radio_story[i]=td_radio_story[i+1];
    td_radio_say(script);return TRUE;
}
/* Chapter k is open once enough deliveries are in and the contract that
 * ends the previous chapter's story is done. */
UWORD td_radio_open(void) BANKED {
    UBYTE k;UWORD open=1;
    for(k=1;k<TD_STORY_CHAPTERS;k++)
        if(td.done>=td_chapter_min[k]&&TD_DONE(td_chapter_after[k]))open|=1<<k;
    return open;
}
void td_radio_done(UBYTE job,UBYTE done_before,UWORD open_before) BANKED {
    UBYTE i;UWORD open;
    td_radio_contract(job,2);
    if(td.done==done_before)return;
    for(i=0;i<TD_RADIO_FOLLOWS;i++)if(td_radio_follow_job[i]==job)td_radio_story_add(td_radio_follow_script[i]);
    for(i=0;i<TD_RADIO_BEATS;i++)if(td_radio_beat_at[i]==td.done)td_radio_story_add(td_radio_beat_script[i]);
    open=td_radio_open()&~open_before;
    for(i=1;i<TD_STORY_CHAPTERS;i++)if(open&(1<<i))td_radio_story_add(td_chapter_opening[i]);
    if(td.done==TD_QUESTS)td_radio_story_add(TD_RADIO_MASTER);
}
UBYTE td_radio_playing(void) BANKED {return td_radio_script;}
void td_radio_line(UBYTE line,char *dest) BANKED {
    UBYTE n,k=line*TD_RADIO_COLS;
    td_radio_text(td_radio_page,k,TD_RADIO_COLS,dest);
    for(n=0;n<TD_RADIO_COLS;n++)if(k+n>=td_radio_pos)dest[n]=' ';
    dest[TD_RADIO_COLS]=0;
}
/* The current page's speaker card: TRUE when it is Rosa (her portrait). */
UBYTE td_radio_speaker(char *dest) BANKED {
    UBYTE s=td_radio_page_speaker(td_radio_page);const char *c=td_radio_cards[s<TD_RADIO_SPEAKERS?s:0];
    while(*c)*dest++=*c++;
    *dest=0;
    return td_radio_rosa[s<TD_RADIO_SPEAKERS?s:0];
}
void td_chapter_name(UBYTE chapter,char *dest) BANKED {
    const char *s=td_chapter_names[chapter<TD_CHAPTERS?chapter:TD_CHAPTERS-1];
    while(*s)*dest++=*s++;
    *dest=0;
}
/* Once per rendered update in the HUD modes. */
void td_radio_tick(void) BANKED {
    UWORD minutes;UBYTE night,c,line;char page[TD_RADIO_PAGE];
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
    if(td_radio_script==TD_NONE&&td_radio_next==TD_NONE)td_radio_story_next();
    if(td_radio_script==TD_NONE||!td_ui_radio_ready())return;
    /* Every call starts on a freshly painted card. */
    if(td_radio_fresh){td_radio_fresh=0;td_ui_draw_radio();}
    if(td_radio_pos<TD_RADIO_PAGE){
        td_radio_text(td_radio_page,td_radio_pos,TD_RADIO_PAGE-td_radio_pos,page+td_radio_pos);
        /* Spaces are already blank: type through them to the next letter. */
        while(td_radio_pos<TD_RADIO_PAGE&&page[td_radio_pos]==' ')td_radio_pos++;
        if(td_radio_pos<TD_RADIO_PAGE){
            for(c=td_radio_pos,line=0;c>=TD_RADIO_COLS;c-=TD_RADIO_COLS)line++;
            td_ui_radio_put(c,line,page[td_radio_pos]);td_radio_pos++;
        }
        return;
    }
    if(++td_radio_hold<TD_RADIO_HOLD)return;
    td_radio_hold=0;td_radio_pos=0;
    if(++td_radio_page<td_radio_end){td_ui_draw_radio();return;}
    td_radio_script=TD_NONE;
    /* A queued call, then a waiting story call, follows on a freshly
     * painted card. */
    if(td_radio_next!=TD_NONE){
        c=td_radio_next;td_radio_next=TD_NONE;
        td_radio_begin(c,td_radio_next_first,td_radio_next_end);return;
    }
    if(td_radio_story_next())return;
    td_ui_draw();
}
