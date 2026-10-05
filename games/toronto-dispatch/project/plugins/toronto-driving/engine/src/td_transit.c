#pragma bank 255
#include <string.h>
#include "td_transit.h"
#include "td_game.h"
#include "td_district.h"

typedef char td_transit_queen_ids_fit_origin[
    (TD_TRANSIT_QUEEN_FIRST + TD_TRANSIT_QUEEN_COUNT <= 64) ? 1 : -1];
typedef char td_transit_queen_timetable_matches_eight_stops[
    (TD_TRANSIT_QUEEN_COUNT == 8 && TD_TRANSIT_QUEEN_PERIOD == 256 &&
     TD_TRANSIT_QUEEN_HOP_SECONDS == 16) ? 1 : -1];

/* Preserve all seven historical positions/phases; append the northbound
 * Summerhill and St Clair endpoints to the fictional18-second service. */
static const UBYTE td_transit_train_stops[] = {0, 12, 13, 14, 15, 16, 17, 59, 60};
#define TD_TRANSIT_TRAIN_COUNT 9
typedef char td_transit_train_phase_fits_period[
    (sizeof(td_transit_train_stops)==TD_TRANSIT_TRAIN_COUNT &&
     (TD_TRANSIT_TRAIN_COUNT-1)*2<18)?1:-1];
static const UBYTE td_transit_bus_stops[] = {18, 16, 19};
static const UBYTE td_transit_ferry_stops[] = {10, 20, 21, 22};
static const char td_transit_labels[4][19] = {
    "LINE 1 TRAIN", "94 WELLESLEY BUS", "ISLAND FERRY", "501 QUEEN"
};

/* Internal calls stay in this bank: public BANKED wrappers do not recursively
 * invoke one another or retain pointers into their private ROM tables. */
static UBYTE td_transit_service_local(UBYTE origin) {
    UBYTE i;
    if (origin & 128) return TD_TRANSIT_INVALID;
    if (origin & 64) return origin == 80 ? TD_TRANSIT_BUS : TD_TRANSIT_INVALID;
    for (i = 0; i < TD_TRANSIT_TRAIN_COUNT; i++) {
        if (td_transit_train_stops[i] == origin) return TD_TRANSIT_TRAIN;
    }
    if (origin == 18 || origin == 19) return TD_TRANSIT_BUS;
    if (origin == 10 || (origin >= 20 && origin <= 22)) return TD_TRANSIT_FERRY;
    if (origin >= TD_TRANSIT_QUEEN_FIRST &&
        origin - TD_TRANSIT_QUEEN_FIRST < TD_TRANSIT_QUEEN_COUNT) return TD_TRANSIT_STREETCAR;
    return TD_TRANSIT_INVALID;
}

static UBYTE td_transit_index_local(UBYTE service, UBYTE stop) {
    const UBYTE *route;
    UBYTE i, count;
    if (service == TD_TRANSIT_STREETCAR) {
        return stop >= TD_TRANSIT_QUEEN_FIRST &&
            stop - TD_TRANSIT_QUEEN_FIRST < TD_TRANSIT_QUEEN_COUNT ?
            stop - TD_TRANSIT_QUEEN_FIRST : TD_TRANSIT_NONE;
    }
    if (service == TD_TRANSIT_TRAIN) {
        route = td_transit_train_stops;
        count = TD_TRANSIT_TRAIN_COUNT;
    } else if (service == TD_TRANSIT_BUS) {
        route = td_transit_bus_stops;
        count = 3;
    } else if (service == TD_TRANSIT_FERRY) {
        route = td_transit_ferry_stops;
        count = 4;
    } else return TD_TRANSIT_NONE;
    for (i = 0; i < count; i++) {
        if (route[i] == stop) return i;
    }
    return TD_TRANSIT_NONE;
}

static UBYTE td_transit_route_target_local(UBYTE origin, UBYTE target, UBYTE service) {
    UBYTE source = origin & 63;
    if (!service || target >= 64 || td_transit_index_local(service, target) == TD_TRANSIT_NONE) return FALSE;
    if (service == TD_TRANSIT_FERRY) {
        return source == 10 ? target >= 20 && target <= 22 : target == 10;
    }
    return TRUE;
}

UBYTE td_transit_service(UBYTE origin) BANKED {
    return td_transit_service_local(origin);
}

UBYTE td_transit_can_origin(UBYTE index) BANKED {
    return index < 64 && td_transit_service_local(index) != TD_TRANSIT_INVALID;
}

UBYTE td_transit_valid(UBYTE origin, UBYTE target) BANKED {
    return target != (origin & 63) &&
        td_transit_route_target_local(origin, target, td_transit_service_local(origin));
}

UBYTE td_transit_count(UBYTE origin) BANKED {
    UBYTE service = td_transit_service_local(origin);
    if (service == TD_TRANSIT_TRAIN) return TD_TRANSIT_TRAIN_COUNT;
    if (service == TD_TRANSIT_BUS) return 3;
    if (service == TD_TRANSIT_FERRY) return (origin & 63) == 10 ? 3 : 1;
    if (service == TD_TRANSIT_STREETCAR) return TD_TRANSIT_QUEEN_COUNT;
    return 0;
}

UBYTE td_transit_stop(UBYTE origin, UBYTE selection) BANKED {
    UBYTE service = td_transit_service_local(origin);
    if (service == TD_TRANSIT_TRAIN && selection < TD_TRANSIT_TRAIN_COUNT) return td_transit_train_stops[selection];
    if (service == TD_TRANSIT_BUS && selection < 3) return td_transit_bus_stops[selection];
    if (service == TD_TRANSIT_FERRY) {
        if ((origin & 63) == 10 && selection < 3) return td_transit_ferry_stops[selection + 1];
        if ((origin & 63) != 10 && selection == 0) return 10;
    }
    if (service == TD_TRANSIT_STREETCAR && selection < TD_TRANSIT_QUEEN_COUNT) {
        return TD_TRANSIT_QUEEN_FIRST + selection;
    }
    return TD_TRANSIT_NONE;
}

UBYTE td_transit_departure(UBYTE origin, UBYTE target, UWORD seconds) BANKED {
    UBYTE service = td_transit_service_local(origin), source_index, target_index;
    UWORD period,phase,elapsed;
    /* A self target asks only for this origin's timetable. It never permits
     * boarding, including at a ferry island with mainland-only service. */
    if (!(service && target == (origin & 63)) &&
        !td_transit_route_target_local(origin, target, service)) return TD_TRANSIT_NONE;
    source_index = td_transit_index_local(service, origin & 63);
    if (service == TD_TRANSIT_TRAIN) {
        period = 18;
        phase = source_index * 2;
    } else if (service == TD_TRANSIT_BUS) {
        period = 24;
        phase = source_index * 4;
    } else if (service == TD_TRANSIT_FERRY) {
        period = 30;
        phase = source_index * 7;
    } else {
        period = TD_TRANSIT_QUEEN_PERIOD;
        target_index = td_transit_index_local(service, target);
        phase = target_index >= source_index ? source_index * 16 : 128 + (7 - source_index) * 16;
    }
    /* Queen period256 requires WORD temporaries; valid closed countdowns
     * are at most252, distinct from the255 invalid sentinel. */
    elapsed = (seconds % period + period - phase) % period;
    return elapsed < (service==TD_TRANSIT_STREETCAR?4:2) ? 0 : period - elapsed;
}

static UBYTE td_transit_fare_local(UBYTE origin) {
    UBYTE service = td_transit_service_local(origin);
    if (service == TD_TRANSIT_TRAIN || service == TD_TRANSIT_STREETCAR) return 3;
    if (service == TD_TRANSIT_BUS) return 2;
    if (service == TD_TRANSIT_FERRY) return 4;
    return 0;
}

UBYTE td_transit_fare(UBYTE origin) BANKED {
    return td_transit_fare_local(origin);
}

UBYTE td_transit_booking_fare(UBYTE origin,UBYTE target,UBYTE job,UWORD cash,UBYTE district) BANKED {
    if(district==TD_DISTRICT_ISLANDS&&origin>=20&&origin<=22&&
       target==10&&job==TD_NONE&&cash<4)return 0;
    return td_transit_fare_local(origin);
}

UBYTE td_transit_duration(UBYTE origin, UBYTE target) BANKED {
    UBYTE service = td_transit_service_local(origin), source_index, target_index, distance;
    if (target == (origin & 63) || !td_transit_route_target_local(origin, target, service)) return 0;
    if (service == TD_TRANSIT_FERRY) return 8;
    source_index = td_transit_index_local(service, origin & 63);
    target_index = td_transit_index_local(service, target);
    distance = source_index > target_index ? source_index - target_index : target_index - source_index;
    if (service == TD_TRANSIT_TRAIN) return 1 + distance / 2;
    if (service == TD_TRANSIT_BUS) return 2 + distance * 2;
    return distance * TD_TRANSIT_QUEEN_HOP_SECONDS;
}

UBYTE td_transit_label(UBYTE origin, char *dest19) BANKED {
    UBYTE service = td_transit_service_local(origin);
    if (!dest19 || !service) return FALSE;
    memcpy(dest19, td_transit_labels[service - 1], 19);
    return TRUE;
}
