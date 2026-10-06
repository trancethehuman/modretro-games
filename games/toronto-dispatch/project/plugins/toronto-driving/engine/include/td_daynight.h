#ifndef TD_DAYNIGHT_H
#define TD_DAYNIGHT_H
#include <gbdk/platform.h>
/* Day/night cycle on the play clock (td.seconds): one game day lasts 1024
 * play seconds and a new game starts at 08:00. Background palettes 0..6 and
 * all sprite palettes follow 64 precomputed steps (scripts/create_daynight.py);
 * the UI palette 7 is never tinted. */
#define TD_DN_FORCE 1 /* copy even when the step's palette set is unchanged */
#define TD_DN_HW 2    /* also write the hardware palettes */
/* Copies the current palette set into BkgPalette/SprPalette. Scene init
 * passes TD_DN_FORCE only (the fade-in applies them); live updates add
 * TD_DN_HW. Returns TRUE when the set changed. */
UBYTE td_daynight_apply(UBYTE flags) BANKED;
/* Minutes since midnight, 0..1439. */
UWORD td_daynight_minutes(void) BANKED;
/* Current palette set (0..TD_DN_SETS-1) and whether vehicle headlamps are
 * on (19:00..06:30); both refresh on every td_daynight_apply. */
extern UBYTE td_daynight_set,td_daynight_lights;
#endif
