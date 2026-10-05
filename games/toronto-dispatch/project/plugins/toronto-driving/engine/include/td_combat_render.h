#ifndef TD_COMBAT_RENDER_H
#define TD_COMBAT_RENDER_H
#include <gbdk/platform.h>
/* One original eight-pixel muzzle burst composed over the exact ground tile.
 * Four transient bytes; bank1 BKG253, no new OAM object or authored-art edit.
 * Restore before aircraft/scroll/UI, render before aerial roof patches, reset
 * whenever scene graphics are replaced. Roof-priority/window cells are skipped. */
void td_combat_render_reset(void) BANKED;
void td_combat_render_restore(void) BANKED;
void td_combat_render(void) BANKED;
#endif
