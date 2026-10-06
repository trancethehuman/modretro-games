#ifndef TD_TRANSIT_H
#define TD_TRANSIT_H
#include <gbdk/platform.h>

#define TD_TRANSIT_INVALID 0
#define TD_TRANSIT_TRAIN 1
#define TD_TRANSIT_BUS 2
#define TD_TRANSIT_FERRY 3
#define TD_TRANSIT_STREETCAR 4
#define TD_TRANSIT_NONE 255
#define TD_TRANSIT_QUEEN_FIRST 43
#define TD_TRANSIT_QUEEN_COUNT 8
#define TD_TRANSIT_QUEEN_PERIOD 64
#define TD_TRANSIT_QUEEN_HOP_SECONDS 4

/* Pure timetable/route queries. Origins are plain six-bit stop IDs except
 * 80 (16|64), the preserved Wellesley bus selector. Bit7 and other bit6
 * encodings are invalid. Targets and menu selections are never encoded.
 * Frequencies, journey durations and fares are original gameplay values. */
UBYTE td_transit_service(UBYTE origin) BANKED;
UBYTE td_transit_can_origin(UBYTE index) BANKED;
UBYTE td_transit_valid(UBYTE origin, UBYTE target) BANKED;
UBYTE td_transit_count(UBYTE origin) BANKED;
/* Invalid origin/selection returns TD_TRANSIT_NONE; no automatic wrapping. */
UBYTE td_transit_stop(UBYTE origin, UBYTE selection) BANKED;
/* Returns seconds until the two-second boarding window, or NONE on failure.
 * A self selection can display any valid service origin's timetable;
 * valid() still rejects boarding it. Queen self selection defaults east.
 * This samples a UWORD clock modulo its period; it stores no absolute time. */
UBYTE td_transit_departure(UBYTE origin, UBYTE target, UWORD seconds) BANKED;
/* Invalid queries return zero. A valid service fare does not require a target. */
UBYTE td_transit_fare(UBYTE origin) BANKED;
UBYTE td_transit_duration(UBYTE origin, UBYTE target) BANKED;
/* Travel heading of a valid journey: 0 east, 1 west, 2 south, 3 north;
 * NONE for an invalid pair. Street services run east-west in this map. */
#define TD_HEADING_EAST 0
#define TD_HEADING_WEST 1
#define TD_HEADING_SOUTH 2
#define TD_HEADING_NORTH 3
UBYTE td_transit_heading(UBYTE origin, UBYTE target) BANKED;
/* Caller supplies nineteen WRAM bytes. FALSE leaves the buffer unchanged. */
UBYTE td_transit_label(UBYTE origin, char *dest19) BANKED;

#endif
