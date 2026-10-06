"""Compile original, authored courier contracts into the native game.

Shortest collision-grid routes inform time allowances. These are design estimates,
not measured campaign duration, and do not prove the two-hour release target.
"""
from collections import deque
from functools import cache
from pathlib import Path
import json
import math
from city_layout import location

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'project/plugins/toronto-driving/engine'
KINDS = ['PARCEL ROUND', 'FRAGILE ART', 'EXPRESS FILES', 'HEAVY FREIGHT',
         'TRANSIT RELAY', 'PASSENGER RUN', 'RETURN PAPERS', 'ISLAND POST']
CHAPTERS = ['First shift', 'Neighbourhood connections', 'City events',
            'Crossing the city', 'Waterfront work', 'Arts and audiences',
            'Evening dispatch', 'Across the network', 'Master courier']

# Titles and brief lines fit the native handheld UI. Every route is authored,
# rather than rotating an unrelated pool of landmarks to inflate the job count.
# Each row is one chapter, with the eight native rule types in KINDS order.
CONTRACTS = [
    [
        ('MARKET START', ('MARKET PARCEL', 'UNION TO MARKET'), [0, 1]),
        ('FIRST ART CRATE', ('ART CRATE FOR AGO', 'BRAKE BEFORE TURNS'), [0, 5]),
        ('DISTILLERY FILES', ('SEALED OFFICE FILE', 'BEAT THE DEADLINE'), [0, 2]),
        ('SHORE SUPPLIES', ('TRUCK LOADS ONLY', 'MARKET TO HARBOUR'), [0, 1, 2, 11]),
        ('FIRST CONNECTION', ('POCKET MAIL RELAY', 'TRY TRAIN AND BUS'), [0, 17, 16, 18, 16, 19, 16, 0]),
        ('STATION PICKUPS', ('PASSENGERS BY CAR', 'SMOOTH CITY RIDES'), [0, 1, 2, 9, 17, 6, 23, 0]),
        ('SIGNED AND SEALED', ('COLLECT SIGNATURES', 'RETURN TO UNION'), [0, 3, 1, 0]),
        ('CENTRE LETTERS', ('PARK THEN FERRY', 'WALK THE LAST LEG'), [10, 21, 25, 21, 10]),
    ],
    [
        ('SHOPFRONT ROUND', ('SMALL SHOP PARCELS', 'LINK LOCAL STOPS'), [0, 1, 2, 11, 4, 7, 8, 0]),
        ('GALLERY LOAN', ('FRAMED PRINTS', 'DELIVER INTACT'), [5, 6, 3, 4, 7, 5]),
        ('CROSSING DEADLINE', ('CIVIC FILE RUN', 'CHOOSE YOUR BRIDGE'), [0, 3, 12, 2, 9, 19, 17, 0]),
        ('STOCK THE STALLS', ('MARKET STOCK TRUCK', 'WEST TO OLD TOWN'), [11, 2, 1, 0, 23, 8, 4, 0]),
        ('BUS CONNECTIONS', ('STATION LETTERS', 'BUS AT WELLESLEY'), [17, 15, 0, 16, 19, 16, 18, 16, 17]),
        ('MUSEUM OUTING', ('MUSEUM VISIT RIDES', 'KEEP THE CAR CALM'), [0, 6, 17, 3, 1, 11, 23, 0]),
        ('HARBOUR RECEIPTS', ('SIGNED CARGO SLIPS', 'BACK TO THE DEPOT'), [0, 11, 2, 1, 3, 0]),
        ('HANLAN POST', ('HANLAN LETTERS', 'FERRY THEN FOOT'), [10, 20, 24, 20, 10]),
    ],
    [
        ('MARKET TO MARKET', ('NEIGHBOURHOOD MAIL', 'WEST TO EAST ROUND'), [8, 7, 5, 4, 3, 1, 2, 11, 0]),
        ('OPENING NIGHT ART', ('EXHIBITION CRATES', 'NO HARD IMPACTS'), [6, 5, 7, 4, 3, 1, 23, 6]),
        ('BRIDGE DOCUMENTS', ('URGENT CITY MAIL', 'BRIDGE TO BRIDGE'), [7, 3, 14, 16, 19, 9, 2, 0]),
        ('EVENT EQUIPMENT', ('TRUCK EVENT LOADS', 'RETURN EMPTY CASES'), [2, 11, 23, 8, 7, 4, 1, 0]),
        ('CAMPUS ENVELOPES', ('LIGHT CAMPUS MAIL', 'COMPARE THE ROUTES'), [18, 16, 0, 12, 15, 17, 16, 19, 16, 0]),
        ('AUDIENCE ARRIVALS', ('EVENT PASSENGERS', 'CAR TO EACH DOOR'), [17, 6, 5, 4, 23, 11, 2, 1, 0]),
        ('PERMIT LOOP', ('SIGNED EVENT FORMS', 'RETURN TO START'), [0, 3, 5, 6, 17, 19, 9, 2, 0]),
        ('WARD COTTAGE MAIL', ('WARD LETTER ROUND', 'LEAVE CAR MAINLAND'), [10, 22, 26, 22, 10]),
    ],
    [
        ('WEST END THREAD', ('SHOP REPAIR KITS', 'STOPS IN ORDER'), [0, 23, 4, 8, 7, 5, 6, 3, 1, 0]),
        ('GLASS ACROSS TOWN', ('GLASS NEEDS CARE', 'PLAN WIDE TURNS'), [1, 2, 9, 19, 17, 6, 5, 4, 23, 1]),
        ('NORTH SOUTH FILES', ('OFFICE CUTOFF RUN', 'STOP AT THE BEACON'), [0, 17, 19, 9, 2, 11, 23, 3, 0]),
        ('CITY WORK CREW', ('WORK CREW TOOLS', 'TRUCK EACH STOP'), [8, 7, 4, 23, 0, 1, 2, 11, 9, 8]),
        ('YONGE CONNECTION', ('STATION MAIL', 'KEEP FARE MONEY'), [0, 14, 17, 15, 12, 16, 18, 16, 19, 16, 0]),
        ('CITY TOUR RIDES', ('VISITOR RIDES', 'SMOOTH CITY TOUR'), [0, 23, 5, 6, 17, 19, 9, 2, 1, 0]),
        ('CROSS CITY INK', ('SIGN AT EACH STOP', 'UNION GETS COPIES'), [0, 8, 7, 5, 6, 17, 19, 9, 2, 1, 0]),
        ('WEST CENTRE POST', ('TWO ISLAND ROUNDS', 'TRANSFER MAINLAND'), [10, 20, 24, 20, 10, 21, 25, 21, 10]),
    ],
    [
        ('HARBOUR SHOP MAIL', ('WATERFRONT PARCELS', 'SHORE TO CITY'), [11, 2, 1, 0, 23, 4, 7, 8, 3, 11]),
        ('SHORE DISPLAY', ('FRAGILE DISPLAY', 'TAKE CARE AT CURBS'), [5, 4, 23, 0, 11, 2, 1, 3, 6, 5]),
        ('DOCK OFFICE DASH', ('URGENT SHORE FILES', 'BEAT THE CLOCK'), [0, 11, 2, 9, 19, 17, 15, 3, 23, 0]),
        ('HARBOUR LOADS', ('BULKY SHORE CARGO', 'WIDE TRUCK TURNS'), [0, 23, 11, 2, 1, 9, 8, 4, 3, 0]),
        ('SHORE CONNECTION', ('SMALL SHORE KIT', 'TRAIN THEN WALK'), [11, 0, 17, 16, 19, 16, 18, 16, 12, 10]),
        ('FERRY CONNECTIONS', ('RIDES TO THE SHORE', 'PARK AT DROP OFFS'), [0, 6, 17, 19, 9, 2, 11, 10, 23, 0]),
        ('CARGO SIGNOFF', ('SIGNED DOCK PAPERS', 'BRING COPIES HOME'), [0, 11, 2, 9, 19, 17, 6, 3, 1, 0]),
        ('CENTRE WARD POST', ('TWO ISLAND PARCELS', 'FOOTPATHS TO DOORS'), [10, 21, 25, 21, 10, 22, 26, 22, 10]),
    ],
    [
        ('CULTURE PARCELS', ('BOOKS AND PROGRAMS', 'MUSEUM TO MARKET'), [6, 17, 3, 1, 2, 11, 23, 4, 7, 5, 6]),
        ('ART EXCHANGE', ('FRAMED ART LOANS', 'KEEP CRATES WHOLE'), [5, 7, 8, 4, 23, 0, 1, 2, 3, 6, 5]),
        ('PRINT SHOP CUTOFF', ('PRINT SHOP FILES', 'TIME THE CROSSINGS'), [7, 5, 6, 17, 19, 9, 2, 11, 3, 0]),
        ('STAGE CASES', ('HEAVY EVENT CASES', 'TRUCK LOADING RUN'), [23, 0, 1, 2, 11, 9, 19, 6, 5, 4, 23]),
        ('ART VIA TRANSIT', ('GALLERY MAIL', 'SMART TRANSFERS'), [5, 14, 17, 16, 18, 16, 19, 16, 12, 0, 5]),
        ('GALLERY NIGHT', ('GALLERY CAR RIDES', 'PROTECT PASSENGERS'), [0, 5, 7, 8, 4, 23, 11, 2, 9, 6, 0]),
        ('PROGRAM APPROVAL', ('COLLECT APPROVALS', 'RETURN ORIGINAL'), [0, 5, 7, 8, 4, 23, 11, 2, 9, 3, 0]),
        ('WARD HANLAN POST', ('EAST AND WEST POST', 'FERRY FOOT FERRY'), [10, 22, 26, 22, 10, 20, 24, 20, 10]),
    ],
    [
        ('LATE SHOP ROUND', ('LAST SHOP PARCELS', 'CITY STILL MOVES'), [0, 1, 2, 9, 19, 17, 6, 5, 7, 8, 4, 0]),
        ('EVENING EXHIBIT', ('FINAL EXHIBIT LOAD', 'SLOW IN TURNS'), [6, 3, 1, 2, 11, 23, 4, 8, 7, 5, 6]),
        ('LAST FILE RUN', ('URGENT FINAL FILES', 'PLAN THEN DRIVE'), [0, 8, 7, 5, 6, 17, 19, 9, 2, 3, 0]),
        ('CLOSING STOCK', ('SHOP STOCK RETURNS', 'WIDE TURNS LOADED'), [1, 2, 11, 23, 4, 8, 7, 5, 3, 0, 1]),
        ('NIGHT NETWORK', ('DISPATCH LETTERS', 'CATCH DEPARTURE'), [19, 16, 12, 0, 17, 15, 16, 18, 16, 14, 0]),
        ('EVENING RIDES', ('LATE CITY RIDES', 'SAFE CAR JOURNEYS'), [0, 1, 2, 11, 23, 4, 8, 7, 5, 6, 17, 0]),
        ('CLOSING LEDGER', ('SIGNED SHOP LEDGER', 'RETURN IT TO DEPOT'), [0, 23, 4, 8, 7, 5, 6, 17, 19, 9, 1, 0]),
        ('THREE ISLAND POST', ('ALL THREE ISLANDS', 'WALK EACH DELIVERY'), [10, 20, 24, 20, 10, 21, 25, 21, 10, 22, 26]),
    ],
    [
        ('CITY LINK PARCELS', ('CITY PARCEL ROUND', 'MANY ROUTE CHOICES'), [8, 4, 23, 11, 2, 9, 19, 17, 6, 5, 7, 0]),
        ('FRAGILE CITY LINK', ('CITY ART HANDOFFS', 'CONDITION MATTERS'), [5, 6, 17, 19, 9, 2, 1, 11, 23, 4, 8, 5]),
        ('NETWORK DEADLINE', ('EXPRESS FILE LINK', 'SAVE TIME LEGALLY'), [0, 23, 8, 7, 6, 17, 19, 9, 2, 11, 3, 0]),
        ('NETWORK FREIGHT', ('LARGE CITY LOADS', 'TRUCK STOP CONTROL'), [11, 2, 1, 0, 23, 4, 8, 7, 6, 19, 9, 11]),
        ('CITY TRANSFERS', ('POCKET CITY PAPERS', 'BUS TRAIN AND FOOT'), [18, 16, 19, 16, 17, 15, 0, 12, 14, 5, 1, 0]),
        ('NEIGHBOUR RIDES', ('CITY RIDES', 'SMOOTH DRIVING'), [8, 7, 5, 6, 17, 19, 9, 2, 11, 1, 23, 0]),
        ('SIGNATURE CHAIN', ('SIGNATURE CHAIN', 'FINAL STOP UNION'), [0, 1, 11, 2, 9, 19, 17, 6, 5, 7, 8, 0]),
        ('ISLAND HANDOFFS', ('CENTRE WARD HANLAN', 'MAINLAND TRANSFERS'), [10, 21, 25, 21, 10, 22, 26, 22, 10, 20, 24]),
    ],
    [
        ('MASTER PARCELS', ('FULL CITY PARCELS', 'EVERY STOP COUNTS'), [0, 8, 7, 5, 6, 17, 19, 9, 2, 11, 1, 23]),
        ('MASTER ART ROUND', ('ART CARE ROUND', 'ARRIVE WITH CARE'), [6, 5, 8, 7, 4, 23, 0, 11, 2, 9, 19, 6]),
        ('MASTER DEADLINE', ('FINAL EXPRESS RUN', 'CONTROL BEATS RUSH'), [0, 1, 2, 9, 19, 17, 6, 5, 8, 4, 23, 0]),
        ('MASTER FREIGHT', ('FULL TRUCK CIRCUIT', 'BRAKE EARLY LOADED'), [0, 8, 7, 4, 23, 11, 2, 9, 19, 6, 3, 0]),
        ('MASTER TRANSIT', ('NETWORK MAIL RELAY', 'TIME EACH TRANSFER'), [0, 17, 15, 16, 18, 16, 19, 16, 14, 5, 1, 0]),
        ('MASTER RIDES', ('FULL CITY SHIFT', 'SMOOTH TO THE END'), [0, 23, 11, 2, 9, 19, 17, 6, 5, 7, 8, 1]),
        ('MASTER RETURNS', ('FULL SIGNOFF RUN', 'ORIGINAL TO UNION'), [0, 8, 4, 23, 11, 2, 9, 19, 17, 6, 3, 0]),
        ('MASTER ISLAND POST', ('WARD HANLAN CENTRE', 'FINAL FOOT ROUND'), [10, 22, 26, 22, 10, 20, 24, 20, 10, 21, 25]),
    ],
]


def parking_rows(stops):
    """Auxiliary whole-pixel guidance; never expand native stop/save records."""
    ids = [stop['id'] for stop in stops]
    assert all(type(index) is int and 0 <= index < 255 for index in ids) and len(set(ids)) == len(ids), 'Parking source stop IDs must be unique native IDs'
    rows = []
    for stop in stops:
        foot = bool(stop.get('reserved', 0) & 1)
        anchor = stop.get('parking_anchor')
        assert foot == bool(stop.get('foot_only', False)), f"Native/source foot-only flag differs: {stop['id']}"
        assert (anchor is not None) == foot, f"Every native foot-only client requires exactly one parking anchor: {stop['id']}"
        if anchor is None:
            continue
        assert isinstance(anchor, dict) and set(anchor) == {'u', 'v'}, f"Invalid parking coordinates: {stop['id']}"
        u, v = anchor['u'], anchor['v']
        assert type(u) is int and type(v) is int and 8 <= u <= 1016 and 8 <= v <= 968, f"Parking anchor outside native vehicle bounds: {stop['id']}"
        assert (u, v) != (stop['u'], stop['v']), f"Parking anchor must differ from foot-only client: {stop['id']}"
        rows.append((stop['id'], u, v))
    return sorted(rows)


def parking_code(stops):
    rows = parking_rows(stops)
    assert rows, 'Native parking guidance requires authored foot-only clients'
    code = ['/* Auxiliary parking cues; client records remain unchanged. */',
            'typedef struct { UWORD u,v; UBYTE stop; } td_parking_t;',
            f'static const td_parking_t td_parking[{len(rows)}]={{']
    code += [f'  {{{u},{v},{stop}}},' for stop, u, v in rows]
    code += ['};',
             'UBYTE td_get_parking(UBYTE stop,UWORD *u,UWORD *v) BANKED {',
             ' UBYTE i;if(!u||!v)return FALSE;',
             f' for(i=0;i<{len(rows)};i++)if(td_parking[i].stop==stop){{*u=td_parking[i].u;*v=td_parking[i].v;return TRUE;}}',
             ' return FALSE;', '}']
    return code


def decode_grid(text):
    result, pos = [], 0
    while pos < len(text):
        value = int(text[pos:pos + 2], 16)
        pos += 2
        if text[pos] == '!':
            count, pos = 1, pos + 1
        else:
            end = text.index('+', pos)
            count, pos = int(text[pos:end], 16), end + 1
        result.extend([value] * count)
    return result


def shortest_routes(stops):
    """Conservative tile-centre distances using the actual scene collision grid."""
    scene = json.loads((ROOT / 'project/project/scenes/toronto_city/scene.gbsres').read_text())
    width, height = scene['width'], scene['height']
    grid = decode_grid(scene['collisions'])
    assert len(grid) == width * height
    points = [(s[0] // 8, s[1] // 8) for s in stops]

    @cache
    def usable(x, y, car):
        if not (1 <= x < width - 1 and 1 <= y < height - 1):
            return False
        if car:
            return all(grid[(y + dy) * width + x + dx] == 0
                       for dx in (-1, 0, 1) for dy in (-1, 0, 1))
        return not (grid[y * width + x] & 15)

    distances = {}
    for car in (False, True):
        for origin, start in enumerate(points):
            if not usable(*start, car):
                continue
            queue, visited = deque([start]), {start: 0}
            while queue:
                x, y = queue.popleft()
                for point in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                    if point not in visited and usable(*point, car):
                        visited[point] = visited[x, y] + 8
                        queue.append(point)
            for dest, point in enumerate(points):
                distances[car, origin, dest] = visited.get(point)
    return distances


def route_estimate(route, kind, distances):
    """Model generous controlling/handling allowances; never measure gameplay here."""
    road_pixels = foot_pixels = ferry_legs = 0
    for origin, dest in zip(route, route[1:]):
        walk = distances.get((False, origin, dest))
        road = distances.get((True, origin, dest))
        if walk is None:
            assert origin in (10, 20, 21, 22) and dest in (10, 20, 21, 22)
            assert origin == 10 or dest == 10, 'Public ferry route needs mainland transfer'
            ferry_legs += 1
        elif road is None:
            assert kind == 7, 'Only Island post uses inaccessible vehicle stops'
            foot_pixels += walk
        else:
            road_pixels += road
    # Conservative slow road vehicle is 18/16 pixels per video frame (scooter).
    # Truck-only jobs use20/16. Handling allows braking, turns and interaction;
    # it is a tuning hypothesis, not idle time the game imposes on the player.
    road_speed = 75 if kind == 3 else 67.5
    moving = road_pixels / road_speed + foot_pixels / 30
    handling = 5 * max(0, len(route) - 1)
    if kind == 7:
        #28s worst phase wait plus8s ride on the fictional30s ferry schedule.
        budget = 90 + (moving + handling) * 1.35 + ferry_legs * 36
    elif kind == 2:
        budget = 60 + (moving + handling) * 1.35
    elif kind == 4:
        #All non-Island relay routes remain feasible by road. Transit offers an
        #alternative; choosing it does not turn waiting into a required objective.
        budget = 90 + (moving + handling) * 1.7
    else:
        budget = 90 + (moving + handling) * 2.0
    return {
        'vehicle_route_pixels': road_pixels,
        'island_walking_pixels': foot_pixels,
        'required_ferry_legs': ferry_legs,
        'fictional_ferry_fares': ferry_legs * 4,
        'modeled_road_and_foot_seconds': round(moving, 1),
        'modeled_control_allowance_seconds': handling,
        'basis': 'Collision-grid shortest paths and native maximum speeds; not measured playtime',
    }, math.ceil(budget / 5) * 5


def main():
    original_stops = [
        (288, 368, 'UNION DEPOT', 1), (384, 368, 'ST LAWRENCE', 0), (416, 368, 'DISTILLERY', 0),
        (288, 288, 'CITY HALL', 0), (192, 288, 'QUEEN WEST', 0), (192, 224, 'AGO / GRANGE', 0),
        (256, 64, 'ROM / BLOOR', 0), (128, 224, 'KENSINGTON', 0), (64, 288, 'DUFFERIN', 0),
        (480, 288, 'RIVERSIDE', 0), (320, 400, 'FERRY TERMINAL', 3), (416, 400, 'EAST BAYFRONT', 0),
        (320, 336, 'KING STATION', 1), (320, 288, 'QUEEN STATION', 1), (320, 224, 'DUNDAS STATION', 1),
        (320, 160, 'COLLEGE STATION', 1), (320, 112, 'WELLESLEY', 1), (320, 64, 'BLOOR-YONGE', 1),
        (96, 64, 'OSSINGTON BUS', 2), (416, 64, 'CASTLE FRANK', 2),
        (240, 520, 'HANLANS POINT', 3), (352, 512, 'CENTRE ISLAND', 3), (432, 496, 'WARDS ISLAND', 3),
        (192, 368, 'CN TOWER', 0),
    ]
    stops = [(*location(u, v), name, transit) for u, v, name, transit in original_stops]
    #Fictional service entrances on existing Island walkable land, not claims
    #about surveyed public access or exact real-world building entrances.
    stops += [(560, 928, 'HANLAN SERVICE', 0), (760, 944, 'CENTRE PARK POST', 0),
              (912, 912, 'WARD COTTAGE POST', 0)]
    distances = shortest_routes(stops)
    for i in range(len(stops)):
        assert distances.get((False, i, i)) == 0, f'Blocked stop: {stops[i][2]}'
    quests = []
    for chapter, rows in enumerate(CONTRACTS):
        assert len(rows) == len(KINDS)
        for kind, (title, brief, route) in enumerate(rows):
            index = len(quests)
            assert len(title) <= 18 and all(len(line) <= 18 for line in brief), title
            assert 2 <= len(route) <= 12 and all(a != b for a, b in zip(route, route[1:]))
            estimate, seconds = route_estimate(route, kind, distances)
            if index < 3:
                seconds = 120  #Tutorials preserve time to learn the controls.
            base_unlock = 3 if kind in (3, 4) else 8 if kind == 5 else 12 if kind == 7 else 0
            reward = (70 + math.ceil(estimate['vehicle_route_pixels'] / 40)
                      + (len(route) - 1) * 10 + estimate['fictional_ferry_fares']
                      + (25 if kind in (1, 3, 5) else 15 if kind == 2 else 0)
                      + chapter * 12)
            quests.append({
                'id': f'contract-{index + 1:02d}', 'title': title, 'brief': list(brief),
                'chapter': CHAPTERS[chapter], 'kind': KINDS[kind], 'kind_id': kind,
                'required_vehicle': 1 if kind == 3 else 0 if kind == 5 else 255,
                'min_completed': max(base_unlock, chapter * 6), 'route': route,
                'time_limit_seconds': seconds, 'reward': reward, 'timing_design': estimate,
            })
    assert len(quests) == 72 and len({tuple(q['route']) for q in quests}) == 72
    assert len({q['title'] for q in quests}) == 72
    #Check that gated chapters can always be reached by distinct completions.
    completed = 0
    while completed < len(quests):
        available = sum(q['min_completed'] <= completed for q in quests)
        assert available > completed, f'Unlock deadlock after {completed} completions'
        completed = available
    content = {
        'status': 'engine-integrated',
        'scope': 'Compressed central Toronto prototype; broad Old Toronto map accuracy and full campaign duration remain release checks',
        'duration_target_minutes': 120, 'duration_verified': False,
        'duration_notice': 'Authored contract counts, shortest-path models and deadlines do not verify duration or enjoyment. Measure representative jobs and a complete campaign.',
        'quest_types': KINDS, 'chapters': CHAPTERS,
        'stops': [{'id': i, 'u': s[0], 'v': s[1], 'name': s[2], 'transit': s[3],
                   **({'location_notice': 'Original fictional delivery entrance on compressed Island terrain'} if i >= 24 else {})}
                  for i, s in enumerate(stops)],
        'quests': quests,
        'transit': {
            'line1': {'stops': [0, 12, 13, 14, 15, 16, 17], 'period_seconds': 18, 'fare': 3,
                      'source': 'https://www.ttc.ca/routes-and-schedules/1/0/15657'},
            'bus94': {'stops': [18, 16, 19], 'period_seconds': 24, 'fare': 2,
                      'source': 'https://www.ttc.ca/routes-and-schedules/94/1/8008'},
            'ferry': {'stops': [10, 20, 21, 22], 'period_seconds': 30, 'fare': 4,
                      'topology': 'Mainland terminal to each Island dock; transfer via mainland between Islands',
                      'source': 'https://www.toronto.ca/explore-enjoy/toronto-island-ferries/getting-around/'},
            'notice': 'Fictional game timetables, fares and compression. Not a TTC trip planner.',
        },
    }
    extra = json.loads((ROOT / 'content/districts/west_jobs.json').read_text())
    assert [s['id'] for s in extra['stops']] == list(range(27, 35))
    assert [q['id'] for q in extra['quests']] == [f'contract-{i:02d}' for i in range(73, 81)]
    for stop in content['stops']:
        stop.update(district=0, reserved=0)
    content['stops'].extend(extra['stops'])
    quests.extend(extra['quests'])
    assert len(quests) == 80 and len(content['stops']) == 35
    eastern = json.loads((ROOT / 'content/districts/east_jobs.json').read_text())
    assert [s['id'] for s in eastern['stops']] == list(range(35, 43))
    assert [q['id'] for q in eastern['quests']] == [f'contract-{i:02d}' for i in range(81, 89)]
    content['stops'].extend(eastern['stops'])
    quests.extend(eastern['quests'])
    content['scope'] = 'Four linked original compressed scenes: central Toronto, western neighbourhoods, High Park/Junction and eastern Riverdale/Leslieville. Full Old Toronto and measured duration remain release checks.'
    assert len(quests) == 88 and len(content['stops']) == 43
    streetcar = json.loads((ROOT / 'content/streetcar.json').read_text())
    assert [s['id'] for s in streetcar['stops']] == list(range(43, 51))
    content['stops'].extend(streetcar['stops'])
    content['transit']['streetcar501'] = streetcar['service']
    assert len(content['stops']) == 51
    (ROOT / 'content/campaign.json').write_text(json.dumps(content, indent=2) + '\n')
    code = ['//Generated by scripts/create_campaign.py; original authored content.',
            '#pragma bank 255', '#include <string.h>', '#include "td_game.h"',
            'static const td_stop_t td_stops[TD_STOPS] = {']
    for stop in content['stops']:
        code.append('  {%d,%d,"%s",%d,%d,%d},' % (stop['u'], stop['v'], stop['name'], stop['transit'], stop['district'], stop['reserved']))
    code += ['};', 'static const td_job_t td_jobs[TD_QUESTS] = {']
    for quest in quests:
        route = quest['route'] + [255] * (12 - len(quest['route']))
        code.append('  {"%s",%d,%d,%d,%d,%d,%d,{%s}},' % (
            quest['title'], quest['kind_id'], len(quest['route']), quest['required_vehicle'],
            quest['min_completed'], quest['time_limit_seconds'], quest['reward'], ','.join(map(str, route))))
    code += ['};', 'static const char td_briefs[TD_QUESTS][37] = {']
    for quest in quests:
        brief = ''.join(line.ljust(18) for line in quest['brief'])
        code.append(f'  "{brief}",')
    code += ['};']
    code += parking_code(content['stops'])
    street_names, street_segments = [], []
    aliases = {'Colborne Lodge Drive south approach':'COLBORNE LODGE DR',
               'Martin Goodman waterfront path':'MARTIN GOODMAN TRL',
               'High Park formal spine':'HIGH PARK WALK',
               'Colborne fictional service entrance':'COLBORNE WALK'}
    world = json.loads((ROOT / 'content/districts/world.json').read_text())
    for entry in world['districts'][1:]:
        district = entry['id']
        slug = entry['scene'].removeprefix('toronto_')
        metadata = json.loads((ROOT / f'content/districts/{slug}_art.json').read_text())
        for road in metadata['roads'] + metadata['footpaths']:
            name = aliases.get(road['name'], road['name'].upper().replace(' STREET WEST',' ST W').replace(' STREET',' ST').replace(' AVENUE',' AVE').replace(' BOULEVARD WEST',' BLVD W').replace(' BOULEVARD',' BLVD').replace(' ROAD',' RD').replace(' DRIVE',' DR'))[:18]
            if name not in street_names:
                street_names.append(name)
            index = street_names.index(name)
            for a, b in zip(road['points'], road['points'][1:]):
                street_segments.append((min(a[0], b[0]), min(a[1], b[1]), max(a[0], b[0]), max(a[1], b[1]), district, index))
    code += [f'static const char td_west_street_names[{len(street_names)}][19]={{']
    code += [f'  "{name}",' for name in street_names]
    code += ['};','typedef struct { UWORD x1,y1,x2,y2; UBYTE district,name; } td_street_t;',
             f'static const td_street_t td_west_streets[{len(street_segments)}]={{']
    code += ['  {' + ','.join(map(str, segment)) + '},' for segment in street_segments]
    # Segments are appended district by district; index each district's run so
    # the HUD lookup visits only its own streets in the same order.
    districts = len(world['districts'])
    assert [segment[4] for segment in street_segments] == sorted(segment[4] for segment in street_segments)
    starts = [next((i for i, segment in enumerate(street_segments) if segment[4] >= district), len(street_segments))
              for district in range(districts + 1)]
    starts[0] = starts[1]
    assert len(street_segments) < 256
    code += ['};',
             f'static const UBYTE td_west_street_start[{districts + 1}]={{' + ','.join(map(str, starts)) + '};',
             'void td_get_west_street(UBYTE district,UWORD u,UWORD v,char *d) BANKED {',
             ' UBYTE name=0,i=0,end=0;UWORD score,best=65535;const td_street_t *s;',
             f' if(district<{districts}){{i=td_west_street_start[district];end=td_west_street_start[district+1];}}',
             ' for(s=&td_west_streets[i];i<end;i++,s++){',
             ' score=(u<s->x1?s->x1-u:u>s->x2?u-s->x2:0)+(v<s->y1?s->y1-v:v>s->y2?v-s->y2:0);',
             ' if(score<best){best=score;name=s->name;}',
             ' }memcpy(d,td_west_street_names[name],19);',
             '}',
             'void td_get_stop(UBYTE i,td_stop_t *d) BANKED { if(i<TD_STOPS) memcpy(d,&td_stops[i],sizeof(td_stop_t)); }',
             'void td_get_job(UBYTE i,td_job_t *d) BANKED { if(i<TD_QUESTS) memcpy(d,&td_jobs[i],sizeof(td_job_t)); }',
             'void td_get_brief(UBYTE i,char *d) BANKED { if(i<TD_QUESTS) memcpy(d,td_briefs[i],37); else d[0]=0; }',
             'void td_get_street(UWORD u,UWORD v,char *d) BANKED {',
             '  if(td.district){td_get_west_street(td.district,u,v,d);return;}',
             '  const char *name="TORONTO";',
             '  if(v>816) name="TORONTO ISLANDS";',
             '  else if(v>768) name="QUEENS QUAY";',
             '  else if(v>688) name="FRONT STREET";',
             '  else if(v>608) name="KING STREET";',
             '  else if(v>496) name="QUEEN STREET";',
             '  else if(v>368) name="DUNDAS STREET";',
             '  else if(v>256) name="COLLEGE / CARLTON";',
             '  else if(v>144) name="WELLESLEY / HARBORD";',
             '  else name=u>912?"DANFORTH AVENUE":"BLOOR STREET";',
             '  if(v>256&&v<320&&u>=816)name="GERRARD ST EAST";',
             '  { static const UWORD columns[]={80,208,336,480,560,640,720,816,944};',
             '    static const char * const roads[]={"DUFFERIN STREET","BATHURST STREET","SPADINA AVENUE","UNIVERSITY AVENUE","BAY STREET","YONGE STREET","JARVIS STREET","PARLIAMENT STREET","BROADVIEW AVENUE"};',
             '    static const UWORD rows[]={64,176,288,400,528,640,720,784};',
             '    UWORD nearest_x=65535,nearest_y=65535,delta; UBYTE i,best=0;',
             '    for(i=0;i<9;i++){delta=u>columns[i]?u-columns[i]:columns[i]-u;if(delta<nearest_x){nearest_x=delta;best=i;}}',
             '    for(i=0;i<8;i++){delta=v>rows[i]?v-rows[i]:rows[i]-v;if(delta<nearest_y)nearest_y=delta;}',
             '    if(v<816 && nearest_x<nearest_y)name=roads[best];',
             '  } strcpy(d,name);', '}']
    (ENGINE / 'src').mkdir(parents=True, exist_ok=True)
    (ENGINE / 'src/td_content.c').write_text('\n'.join(code) + '\n')
    #Fixed-size UI cells reuse the MIT starter font, retaining its asset licence.
    font = json.loads((ROOT / 'project/original-art/native-cells.json').read_text())['font']['cells']
    chars = ' ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/.-+?<>$%#='
    glyphs = {cell['character']: cell['rows'] for cell in font}
    data = []
    for char in chars:
        for row in glyphs[char]:
            bits = sum(1 << (7 - i) for i, pixel in enumerate(row) if pixel == '0')
            data.extend((bits, bits))
    (ENGINE / 'include/td_font.h').write_text(
        '// Derived from MIT Bench Mono glyphs; see project/ASSET_LICENSE.\n'
        'static const char td_chars[]="' + chars + '";\n'
        'static const UBYTE td_font[]={' + ','.join(map(str, data)) + '};\n')
    print(f'Compiled {len(quests)} unique authored contracts, {len(content["stops"])} stops, native briefs, and train/bus/ferry services. Duration remains unverified.')


if __name__ == '__main__':
    main()
