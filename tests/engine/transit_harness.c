/* Compatibility expectations are authored here independently of the module's
 * private tables. The actual source is included unchanged by the host runner. */
#include <stdio.h>
#include <string.h>
#include "transit_under_test.c"

static unsigned long checks, failures;

static void expect_value(unsigned actual, unsigned expected, const char *operation,
                         unsigned origin, unsigned target, unsigned clock) {
    checks++;
    if (actual == expected) return;
    if (failures++ < 12) {
        fprintf(stderr, "%s origin=%u target=%u clock=%u: got%u expected%u\n",
                operation, origin, target, clock, actual, expected);
    }
}

static unsigned oracle_service(unsigned origin) {
    if (origin == 80) return 2;
    if (origin >= 64) return 0;
    if (origin == 0 || (origin >= 12 && origin <= 17)) return 1;
    if (origin == 18 || origin == 19) return 2;
    if (origin == 10 || (origin >= 20 && origin <= 22)) return 3;
    if (origin >= 43 && origin <= 50) return 4;
    return 0;
}

static int oracle_index(unsigned service, unsigned stop) {
    if (service == 1) {
        if (stop == 0) return 0;
        if (stop >= 12 && stop <= 17) return (int)stop - 11;
    } else if (service == 2) {
        if (stop == 18) return 0;
        if (stop == 16) return 1;
        if (stop == 19) return 2;
    } else if (service == 3) {
        if (stop == 10) return 0;
        if (stop >= 20 && stop <= 22) return (int)stop - 19;
    } else if (service == 4 && stop >= 43 && stop <= 50) return (int)stop - 43;
    return -1;
}

static unsigned oracle_target(unsigned origin, unsigned target) {
    unsigned service = oracle_service(origin), source = origin & 63;
    if (!service || target >= 64 || oracle_index(service, target) < 0) return 0;
    if (service == 3) return source == 10 ? target >= 20 && target <= 22 : target == 10;
    return 1;
}

static unsigned oracle_valid(unsigned origin, unsigned target) {
    return target != (origin & 63) && oracle_target(origin, target);
}

static unsigned oracle_count(unsigned origin) {
    switch (oracle_service(origin)) {
        case 1: return 7;
        case 2: return 3;
        case 3: return (origin & 63) == 10 ? 3 : 1;
        case 4: return 8;
        default: return 0;
    }
}

static unsigned oracle_stop(unsigned origin, unsigned selection) {
    static const unsigned train[] = {0, 12, 13, 14, 15, 16, 17};
    static const unsigned bus[] = {18, 16, 19};
    if (selection >= oracle_count(origin)) return 255;
    switch (oracle_service(origin)) {
        case 1: return train[selection];
        case 2: return bus[selection];
        case 3: return (origin & 63) == 10 ? 20 + selection : 10;
        case 4: return 43 + selection;
        default: return 255;
    }
}

static unsigned oracle_duration(unsigned origin, unsigned target) {
    unsigned service = oracle_service(origin);
    int distance;
    if (!oracle_valid(origin, target)) return 0;
    if (service == 3) return 8;
    distance = oracle_index(service, origin & 63) - oracle_index(service, target);
    if (distance < 0) distance = -distance;
    if (service == 1) return 1 + (unsigned)distance / 2;
    if (service == 2) return 2 + 2 * (unsigned)distance;
    return 4 * (unsigned)distance;
}

static unsigned oracle_departure(unsigned origin, unsigned target, unsigned clock) {
    unsigned service = oracle_service(origin), source = origin & 63;
    int period, phase, elapsed, index = oracle_index(service, source);
    if (!service || (target != source && !oracle_target(origin, target))) return 255;
    if (service == 1) { period = 18; phase = index * 2; }
    else if (service == 2) { period = 24; phase = index * 4; }
    else if (service == 3) { period = 30; phase = index * 7; }
    else {
        period = 64;
        phase = oracle_index(service, target) >= index ? index * 4 : 32 + 4 * (7 - index);
    }
    /* Signed remainder is deliberately unlike the implementation's unsigned
     * period-offset expression, including clocks before a stop's phase. */
    elapsed = ((int)clock - phase) % period;
    if (elapsed < 0) elapsed += period;
    return elapsed < 2 ? 0 : (unsigned)(period - elapsed);
}

static void test_encodings_and_routes(void) {
    unsigned origin, target, selection;
    static const unsigned fares[] = {0, 3, 2, 4, 3};
    static const char *labels[] = {NULL, "LINE 1 TRAIN", "94 WELLESLEY BUS", "ISLAND FERRY", "501 QUEEN"};
    for (origin = 0; origin < 256; origin++) {
        unsigned service = oracle_service(origin);
        unsigned char buffer[21], prior[21];
        expect_value(td_transit_service((UBYTE)origin), service, "service", origin, 0, 0);
        expect_value(td_transit_can_origin((UBYTE)origin), origin < 64 && service != 0,
                     "plain-origin", origin, 0, 0);
        expect_value(td_transit_count((UBYTE)origin), oracle_count(origin), "count", origin, 0, 0);
        expect_value(td_transit_fare((UBYTE)origin), fares[service], "fare", origin, 0, 0);
        memset(buffer, 0xA5, sizeof(buffer));
        memcpy(prior, buffer, sizeof(buffer));
        expect_value(td_transit_label((UBYTE)origin, (char *)&buffer[1]), service != 0,
                     "label-valid", origin, 0, 0);
        expect_value(buffer[0], 0xA5, "label-left-guard", origin, 0, 0);
        expect_value(buffer[20], 0xA5, "label-right-guard", origin, 0, 0);
        if (service) {
            expect_value(strcmp((char *)&buffer[1], labels[service]), 0, "label-text", origin, 0, 0);
            expect_value(buffer[19], 0, "label-terminated", origin, 0, 0);
        } else expect_value(memcmp(buffer, prior, sizeof(buffer)), 0, "label-failure-unchanged", origin, 0, 0);
        expect_value(td_transit_label((UBYTE)origin, NULL), 0, "label-null", origin, 0, 0);
        for (selection = 0; selection < 256; selection++) {
            expect_value(td_transit_stop((UBYTE)origin, (UBYTE)selection), oracle_stop(origin, selection),
                         "menu-stop", origin, selection, 0);
        }
        for (target = 0; target < 256; target++) {
            expect_value(td_transit_valid((UBYTE)origin, (UBYTE)target), oracle_valid(origin, target),
                         "valid", origin, target, 0);
            expect_value(td_transit_duration((UBYTE)origin, (UBYTE)target), oracle_duration(origin, target),
                         "duration", origin, target, 0);
            expect_value(td_transit_departure((UBYTE)origin, (UBYTE)target, 65535),
                         oracle_departure(origin, target, 65535), "departure-encoding", origin, target, 65535);
        }
    }
}

static void test_every_route_and_phase(void) {
    unsigned origin, target, clock;
    for (origin = 0; origin < 256; origin++) {
        if (!oracle_service(origin)) continue;
        for (target = 0; target < 64; target++) {
            if (target != (origin & 63) && !oracle_target(origin, target)) continue;
            for (clock = 0; clock < 128; clock++) {
                expect_value(td_transit_departure((UBYTE)origin, (UBYTE)target, (UWORD)clock),
                             oracle_departure(origin, target, clock), "route-phase", origin, target, clock);
            }
            for (clock = 65533; clock < 65536; clock++) {
                expect_value(td_transit_departure((UBYTE)origin, (UBYTE)target, (UWORD)clock),
                             oracle_departure(origin, target, clock), "route-clock-edge", origin, target, clock);
            }
        }
    }
}

static void test_complete_uword_clock(void) {
    static const unsigned pairs[][2] = {{0,17}, {80,19}, {22,10}, {43,50}, {50,43}};
    unsigned pair, clock;
    for (pair = 0; pair < sizeof(pairs) / sizeof(pairs[0]); pair++) {
        unsigned origin = pairs[pair][0], target = pairs[pair][1];
        for (clock = 0; clock < 65536; clock++) {
            expect_value(td_transit_departure((UBYTE)origin, (UBYTE)target, (UWORD)clock),
                         oracle_departure(origin, target, clock), "full-uword-clock", origin, target, clock);
        }
    }
    /* Encoded Wellesley remains bus while plain16 is train. Ferry self only
     * queries timetable; it never joins island-to-island or self journeys. */
    expect_value(td_transit_departure(80, 19, 4), 0, "bus-window-start", 80, 19, 4);
    expect_value(td_transit_departure(80, 19, 5), 0, "bus-window-last", 80, 19, 5);
    expect_value(td_transit_departure(80, 19, 6), 22, "bus-window-closed", 80, 19, 6);
    expect_value(td_transit_departure(20, 20, 7), 0, "ferry-self-timetable", 20, 20, 7);
    expect_value(td_transit_valid(20, 20), 0, "ferry-self-unboardable", 20, 20, 7);
    expect_value(td_transit_valid(20, 21), 0, "ferry-no-island-shortcut", 20, 21, 0);
    expect_value(td_transit_departure(50, 43, 32), 0, "queen-west-window", 50, 43, 32);
    expect_value(td_transit_departure(43, 50, 0), 0, "queen-east-window", 43, 50, 0);
    expect_value(td_transit_duration(43, 50), 28, "queen-end-to-end", 43, 50, 0);
    expect_value(td_transit_departure(43, 50, (UWORD)(65535U + 1U)), 0,
                 "uword-modulo-reset", 43, 50, 0);
}

int main(void) {
    test_encodings_and_routes();
    test_every_route_and_phase();
    test_complete_uword_clock();
    printf("Transit production-source regressions: %lu checks, %lu failures.\n", checks, failures);
    return failures ? 1 : 0;
}
