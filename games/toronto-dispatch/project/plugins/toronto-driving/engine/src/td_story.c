#pragma bank 255
#include "td_game.h"
#include "td_story.h"
#include "td_story_data.h"
#ifdef CGB
#include "data_manager.h"
#include "gbs_types.h"
#include "compat.h"
/* This existing GBVM engine service is defined in src/core/data_manager.c.
 * It preserves the engine's exact signed-tile allocation, including the
 * ALLOC_BKG_TILES_TOWARDS_SPR tail layout, without reloading the scene. */
void load_bkg_tileset(const tileset_t *tiles,UBYTE bank) BANKED;
#endif

typedef char td_story_tiles_leave_font[(16+TD_STORY_TILE_COUNT<=188)?1:-1];
typedef char td_story_menu_page_fits[(TD_STORY_PAGE_COUNT<255)?1:-1];
typedef char td_story_flags_outside_jobs[(TD_QUESTS==104&&TD_COMPLETE_BYTES==16&&TD_STORY_SAVE_BYTE==13)?1:-1];

/* The UI callback lives in another ROM bank. Its argument must be a WRAM
 * string, never a near pointer into this bank's dialogue or literal pool.
 * This21-byte automatic buffer exists only in the paused cutscene path. */
static void td_story_row(UBYTE row,const char *source){
    char line[21];UBYTE length=0;
    while(length<20&&*source)line[length++]=*source++;
    line[length]=0;td_ui_story_text(row,line);
}

UBYTE td_story_due(void) BANKED {
    UBYTE scene;
    for(scene=0;scene<TD_STORY_SCENES;scene++)
        if(td.done>=td_story_thresholds[scene]&&!(td.complete[TD_STORY_SAVE_BYTE]&(1<<scene)))return scene;
    return TD_NONE;
}

UBYTE td_story_begin(UBYTE scene) BANKED {
    if(scene>=TD_STORY_SCENES)return FALSE;
    td.menu=td_story_firsts[scene];td.mode=TD_DIALOG;
    return TRUE;
}

void td_story_draw(void) BANKED {
    const td_story_page_t *page;UBYTE row;
    if(td.mode!=TD_DIALOG||td.menu>=TD_STORY_PAGE_COUNT)return;
    page=&td_story_pages[td.menu];
    /* Every portrait shares the same immutable pool. First-page upload reclaims
     * the paused atlas scratch space; subsequent dialogue pages only repaint
     * their map/text and never overwrite protected font or guidance tiles. */
    if(td.menu==td_story_firsts[page->scene]){
        VBK_REG=1;set_bkg_data(16,TD_STORY_TILE_COUNT,td_story_tiles);
    }
    VBK_REG=0;set_win_tiles(0,1,20,10,td_story_maps[page->portrait]);
    td_story_row(0,td_story_titles[page->scene]);
    td_story_row(11,page->speaker);
    for(row=0;row<4;row++)td_story_row(12+row,page->lines[row]);
    td_story_row(16,"A NEXT / B SKIP");
    td_story_row(17,td.menu+1==td_story_firsts[page->scene+1]?"END OF CHAPTER":"STORY / WORLD PAUSED");
}

UBYTE td_story_input(UBYTE pressed) BANKED {
    UBYTE scene;
    if(td.mode!=TD_DIALOG||td.menu>=TD_STORY_PAGE_COUNT)return FALSE;
    scene=td_story_pages[td.menu].scene;
    if(pressed&(J_B|J_START)){
        td.complete[TD_STORY_SAVE_BYTE]|=1<<scene;return TRUE;
    }
    if(!(pressed&J_A))return FALSE;
    if(td.menu+1==td_story_firsts[scene+1]){
        td.complete[TD_STORY_SAVE_BYTE]|=1<<scene;return TRUE;
    }
    ++td.menu;return FALSE;
}

void td_story_restore_background(void) BANKED {
#ifdef CGB
    far_ptr_t reference;UWORD count;UBYTE prior_bank;
    if(!current_scene.bank||!current_scene.ptr)return;
    MemcpyBanked(&reference,&((const scene_t*)current_scene.ptr)->background,sizeof(reference),current_scene.bank);
    if(!reference.bank||!reference.ptr)return;
    MemcpyBanked(&reference,&((const background_t*)reference.ptr)->cgb_tileset,sizeof(reference),reference.bank);
    if(!reference.bank||!reference.ptr)return;
    MemcpyBanked(&count,&((const tileset_t*)reference.ptr)->n_tiles,sizeof(count),reference.bank);
    /* The compiled scene guard keeps current backgrounds below47 tiles.
     * The broader192 limit also protects the font192..252 if assets change. */
    if(count>192)return;
    prior_bank=VBK_REG&1;VBK_REG=1;
    load_bkg_tileset(reference.ptr,reference.bank);
    VBK_REG=prior_bank;
#endif
}
