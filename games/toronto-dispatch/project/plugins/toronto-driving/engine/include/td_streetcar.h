#ifndef TD_STREETCAR_H
#define TD_STREETCAR_H
#include <gbdk/platform.h>

#define TD_STREETCAR_PERIOD_TICKS 15360
#define TD_STREETCAR_CLEAR 0
#define TD_STREETCAR_HIT 1
#define TD_STREETCAR_INVALID 255

/* Local positions and inclusive box edges use native Q4 units. Headings are
 * 0 east, 4 south, 8 west, 12 north. Original sprite poses are 0E/1S/2W/3N,
 * plus four for open doors. Stop is 43..50 while dwelling, otherwise 255.
 * Direction is 0 eastbound or 1 westbound, including endpoint turnarounds. */
typedef struct {
    UWORD u,v;
    UBYTE district,heading,frame,stop,direction,doors;
} td_streetcar_pose_t;
typedef struct {
    UWORD left,top,right,bottom;
    UBYTE district;
} td_streetcar_box_t;

/* Pure BANKED queries; no persistent WRAM, scene loads, fares or save writes.
 * Output buffers must be in WRAM. Invalid/null queries leave them unchanged.
 * Subsecond is 0..59 VBlanks; seconds is the persisted unsigned world clock.
 * Its 65,536-second rollover preserves this exactly 256-second timetable. */
UBYTE td_streetcar_pose(UWORD seconds,UBYTE subsecond,
                       td_streetcar_pose_t *pose) BANKED;
/* A booked direction follows its remaining-time phase: elapsed whole seconds
 * are trip duration minus ride_left, with the four boarding-offset seconds.
 * Outside that interval, remaining time 1 pins a retrying/restored rider to
 * the destination doors. Remaining time must fit the actual trip duration;
 * other inconsistent phases fail, so callers can retain their existing view.
 * A persisted arrival-hold flag must use destination() instead: after whole
 * timetable cycles, phase alone cannot distinguish retry from final approach. */
UBYTE td_streetcar_ride(UBYTE origin,UBYTE target,UBYTE ride_left,
                       UWORD seconds,UBYTE subsecond,
                       td_streetcar_pose_t *pose) BANKED;
/* Directional target doors, independent of clock. Runtime/save code owns the
 * decision to latch an actual alighting/scene-queue failure; this module does
 * not invent serialized flags or mutate the58-byte game state. */
UBYTE td_streetcar_destination(UBYTE origin,UBYTE target,
                              td_streetcar_pose_t *pose) BANKED;
/* Exact 28x12 horizontal or 12x28 vertical body. Non-spatial pose fields are
 * ignored. Bounds can be converted to tile spans for collision-resource
 * validation before a scene load; no nested banked tile queries are made. */
UBYTE td_streetcar_bounds(const td_streetcar_pose_t *pose,
                         td_streetcar_box_t *box) BANKED;
/* Test an inclusive caller box in its own district against the complete body
 * swept from elapsed VBlanks ago to the supplied current clock, endpoints
 * included. Checks each cardinal path separately, never a rectangle covering
 * the empty East-bend interior or a discontinuity between local scene seams.
 * Work is bounded by one 256-second cycle even for elapsed >=15360; that query
 * also identifies the entire occupied rail corridor for parking/traffic.
 * A static box can be a parked car, desired walking footprint, or a conservative
 * player-motion union. This does not resolve collisions or simulate impulses. */
UBYTE td_streetcar_sweep(UWORD seconds,UBYTE subsecond,UWORD elapsed,
                        const td_streetcar_box_t *box) BANKED;
#endif
