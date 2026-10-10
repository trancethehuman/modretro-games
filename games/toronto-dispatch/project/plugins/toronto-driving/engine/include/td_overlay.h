#ifndef TD_OVERLAY_H
#define TD_OVERLAY_H
#include <gbdk/platform.h>
/* Overlay sprites drawn straight into OAM after the actors (actor.c.patch):
 * the police helicopter, sharks, rain, a drifting cloud and its shadow, sun
 * rays and gulls. Their tiles are bank-0 sprite tiles 236..255
 * (td_overlay_data.h); being drawn last, they are the first sprites the
 * hardware drops on a crowded line. */

/* Weather from the play clock: 0 clear, 1 cloudy, 2 rain. */
#define TD_WEATHER_CLEAR 0
#define TD_WEATHER_CLOUDY 1
#define TD_WEATHER_RAIN 2
extern UBYTE td_weather;

/* Scene start: load the tiles, place the weather, clear the helicopter and
 * shark. Overlay drawing stays off until the scene is live. */
void td_overlay_init(void) BANKED;
/* Hide everything (map, title, transitions) or show it again. */
void td_overlay_show(UBYTE on) BANKED;
/* Once a frame of play: helicopter, shark, rain, cloud, rays and gulls. */
void td_overlay_tick(void) BANKED;
/* Once a play second: the weather for the clock; TRUE when it changed. */
UBYTE td_overlay_second(void) BANKED;
/* Called by the patched actors_render. */
void td_overlay_render(void) BANKED;
/* The helicopter is overhead within (ru, rv) whole pixels of the courier. */
UBYTE td_overlay_heli_near(UBYTE ru,UBYTE rv) BANKED;
#endif
