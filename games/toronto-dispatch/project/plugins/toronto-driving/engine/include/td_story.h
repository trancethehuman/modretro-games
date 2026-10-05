#ifndef TD_STORY_H
#define TD_STORY_H
#include <gbdk/platform.h>

#define TD_STORY_SCENES 8
#define TD_STORY_SAVE_BYTE 13

/* One increasing, completion-triggered original act at a time. TD_NONE means
 * no scene is due; the eight seen flags use spare completion bits104..111.
 * Quest-completion bits0..103 and every other saved field are untouched. */
UBYTE td_story_due(void) BANKED;
/* FALSE leaves state untouched for invalid scene IDs. The parent snapshots
 * actors/UI, then consumes menu-used buttons and restores/resaves on exit. */
UBYTE td_story_begin(UBYTE scene) BANKED;
/* Draw only on begin or a page advance. Full-screen native art uses bank1
 * tiles16..145/palette7; font192..252, scene palettes and OAM are preserved. */
void td_story_draw(void) BANKED;
/* Restore the current scene's complete original CGB background tileset after
 * a modal portrait/atlas reclaimed bank1 tiles. No scene reload, palette,
 * actor, map/collision or courier-state mutation; caller then restores UI. */
void td_story_restore_background(void) BANKED;
/* TRUE closes. A advances/finishes; B or Start skips and marks the act seen.
 * Only the scene bit is persisted by this API; parent performs the save after
 * restoring the normal mode. Other buttons and held input do not advance. */
UBYTE td_story_input(UBYTE pressed) BANKED;

/* UI owns its existing packed text/hidden-actor caches; no new story buffers.
 * Frame opens full-window presentation, close restores original actor flags. */
void td_ui_story_frame(void) BANKED;
/* Text must be WRAM, because the callback switches away from the story bank. */
void td_ui_story_text(UBYTE row,const char *text) BANKED;
void td_ui_story_close(void) BANKED;
#endif
