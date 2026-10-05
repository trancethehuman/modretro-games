#ifndef TD_AIRCRAFT_RENDER_H
#define TD_AIRCRAFT_RENDER_H
#include <gbdk/platform.h>

/* Reset after graphics load; bind before authored loaders are recycled. */
void td_aircraft_render_reset(void) BANKED;
void td_aircraft_render_bind(void) BANKED;
/* Restore before UI/scroll mutations, and at the stock renderer's entry. */
void td_aircraft_render_restore(void) BANKED;
/* Called after ground actors have populated the current shadow OAM. */
void td_aircraft_render(void) BANKED;
/* Full authored world attribute map, independent of scrolling/overlayVRAM. */
UBYTE td_aircraft_render_exposed(UWORD u,UWORD v) BANKED;
#endif
