#ifndef TD_COMBAT_H
#define TD_COMBAT_H
#include <gbdk/platform.h>
#define TD_COMBAT_CHANGED 1
#define TD_COMBAT_DOWN 2
#define TD_COMBAT_RECOVERED 4
#define TD_COMBAT_BLOCKED 0
#define TD_COMBAT_EMPTY 1
#define TD_COMBAT_SHOT 2
/* Seven transient bytes. Saved vitality/ammo are independent of cargo condition.
 * The city caller owns menu/room/boat/input-context arbitration and saves once
 * for ordinary CHANGED results. RECOVERED already commits the final hospital
 * record; its caller then queues that district. Timers use active VBlanks. */
void td_combat_reset(void) BANKED;
/* Only after a genuine saved downed courier is restored; zeroed host fixtures
 * do not implicitly become a recovery episode. No additional transient RAM. */
void td_combat_resume_downed(void) BANKED;
/* Capture holds the normal courier for60 active-city VBlanks and clears weapon
 * presentation. Health/ammo, parked vehicle and serialized layout stay unchanged. */
void td_combat_arrested(void) BANKED;
UBYTE td_combat_fire(void) BANKED;
UBYTE td_combat_update(UWORD elapsed) BANKED;
UBYTE td_combat_damage(UBYTE damage,BYTE dx,BYTE dy) BANKED;
UBYTE td_combat_locked(void) BANKED;
UBYTE td_combat_police_disabled(void) BANKED;
/* Call after ordinary player presentation, only in the active city. The city
 * restores its canonical player sheet before resuming ordinary driving. */
void td_combat_present(void) BANKED;
/* Original six-VBlank muzzle effect. Outputs whole-pixel origin plus the
 * existing walking directions east0/west1/south2/north3. No retained pointers. */
UBYTE td_combat_flash(UWORD *u,UWORD *v,UBYTE *direction) BANKED;
#endif
