#ifndef TD_AUDIO_H
#define TD_AUDIO_H
#include <gbdk/platform.h>

#define TD_AUDIO_FULL 0
#define TD_AUDIO_EFFECTS 1
#define TD_AUDIO_SILENT 2
#define TD_AUDIO_MODES 3

#define TD_AUDIO_IMPACT 0
#define TD_AUDIO_PICKUP 1
#define TD_AUDIO_COMPLETE 2
#define TD_AUDIO_TRANSIT 3
#define TD_AUDIO_FAIL 4
#define TD_AUDIO_MENU 5
#define TD_AUDIO_CUES 6

/* Original hUGE music + GBVM SFX; preference resets to FULL on each cold boot. */
void td_audio_init(void) BANKED;
/* Call once per rendered update, including menus, after simulation/events.
   world_active means the simulation clock advances (ROAM/WAIT/RIDE).
   Pause stops the tune/engine; queued UI/completion cues may still sound. */
void td_audio_update(WORD speed, UBYTE vehicle, UBYTE onfoot,
                     UBYTE braking, UBYTE world_active) BANKED;
/* Queue a cue for the next update; higher priority wins within one frame. */
void td_audio_play(UBYTE cue) BANKED;
void td_audio_set_mode(UBYTE mode) BANKED;
UBYTE td_audio_get_mode(void) BANKED;
#endif
