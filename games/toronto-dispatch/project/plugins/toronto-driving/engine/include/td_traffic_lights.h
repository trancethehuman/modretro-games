#ifndef TD_TRAFFIC_LIGHTS_H
#define TD_TRAFFIC_LIGHTS_H
#include <gbdk/platform.h>
/* Original 8px corner signal markers, bank-1 BKG47/48, palette7.
 * Palette colour1 is green,2 red; root UI owns its0/3 colours.
 * Active gameplay only; map owns these IDs while paused. One cached byte.
 * Reset after every scene graphics load and on atlas open/close, including
 * when no actor rendering occurred while the atlas overwrote its patterns. */
void td_traffic_lights_reset(void) BANKED;
void td_traffic_lights_render(void) BANKED;
#endif
