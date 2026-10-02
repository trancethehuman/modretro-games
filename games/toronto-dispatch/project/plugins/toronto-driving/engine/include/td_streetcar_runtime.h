#ifndef TD_STREETCAR_RUNTIME_H
#define TD_STREETCAR_RUNTIME_H
#include <gbdk/platform.h>

#define TD_STREETCAR_ACTOR 15
#define TD_STREETCAR_PARK_UNCHANGED 0
#define TD_STREETCAR_PARK_MOVED 1
#define TD_STREETCAR_PARK_BLOCKED 2

/* Presentation-only Q4 coordinates. A paid Queen ride leaves all 58 saved
 * bytes at its origin until the caller commits safe alighting. The view can
 * load a different actual district without changing td.district/park fields. */
extern UWORD td_streetcar_focus_u,td_streetcar_focus_v;
extern UBYTE td_streetcar_view_district,td_streetcar_ride_view;

/* Cold/session reset after restore and resume-mode selection. Scene binding
 * must run before the caller overwrites authored actors[1]. Core/West/East
 * load the original tram sprite there; High Park has no authored tram.
 * bind copies/unlinks that loader and activates persistent custom actor15.
 * The caller recreates only actors1..14 and sets actors_len to16. */
void td_streetcar_runtime_reset(void) BANKED;
UBYTE td_streetcar_runtime_bind(void) BANKED;
/* Call after clock/deadline updates, before queuing a presentation scene or
 * doing motion. Elapsed is actual VBlanks, including bounded motion catch-up.
 * Calling with0 only rebuilds focus after restore/scene init/menu changes. */
void td_streetcar_runtime_prepare(UWORD elapsed) BANKED;
/* Call after ordinary PLAYER/traffic presentation; it overrides only the
 * courier's position/hidden bit during an effective Queen RIDE, including
 * HELP/PAUSE/MAP with resumeRIDE. Do not call it while the atlas is open. */
void td_streetcar_runtime_present(void) BANKED;

/* Q4 caller coordinates; TRUE means clear. Sweep checks fail closed on bad
 * geometry. Driving tests a conservative old/new11px-body union, walking a
 *6px body, and traffic an11px body, against the sampled tram motion. */
UBYTE td_streetcar_runtime_foot_clear(UWORD u,UWORD v) BANKED;
UBYTE td_streetcar_runtime_car_clear(UWORD old_u,UWORD old_v,
                                   UWORD u,UWORD v) BANKED;
UBYTE td_streetcar_runtime_traffic_clear(UBYTE district,UWORD u,UWORD v) BANKED;
UBYTE td_streetcar_runtime_parking_allowed(UBYTE district,UWORD u,UWORD v) BANKED;
/* Cold old-save recovery for an on-foot courier's parked car only. A bounded
 * authored cross-street search verifies the entire11px car path/footprint,
 * rail exclusion and loaded traffic before changing only park_u/park_v.
 * MOVED requires the caller to save; BLOCKED leaves all saved fields intact. */
UBYTE td_streetcar_runtime_recover_park(void) BANKED;
/* Resolve a current body overlap while ROAM/WAIT by the nearest connected
 *4px-grid separation within48px, then an authored cross-street approach
 * within256px. A successful move preserves cash/job/deadline; caller owns
 * crash impulse/damage/checkpoint. BLOCKED leaves position unchanged. */
UBYTE td_streetcar_runtime_recover_contact(UBYTE onfoot) BANKED;
#endif
