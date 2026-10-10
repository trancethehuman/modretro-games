"""Compile original, authored courier contracts into the native game.

Shortest collision-grid routes inform time allowances. These are design estimates,
not measured campaign duration, and do not prove the two-hour release target.
"""
from collections import deque
from functools import cache
from pathlib import Path
import json
import math
from core2x import CORE_STOPS, MAINLAND, street_spans
import world2x
# The Islands lie south of the mainland shore, in the core's southern scenes.
L_MAINLAND_BOTTOM = MAINLAND[3] - world2x.scene_origin(2)[1]

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'project/plugins/toronto-driving/engine'
KINDS = ['PARCEL ROUND', 'FRAGILE ART', 'EXPRESS FILES', 'HEAVY FREIGHT',
         'TRANSIT RELAY', 'PASSENGER RUN', 'RETURN PAPERS', 'ISLAND POST']
STORY = json.loads((ROOT / 'content/story.json').read_text())
CHAPTERS = [c['name'] for c in STORY['chapters']]


def order_drops(pickup, drops, back, distances):
    """Shortest visiting order for a contract's drop-offs (exact, Held-Karp),
    from the pickup and, for returns, back to it: deliveries follow the
    streets instead of zigzagging across the city."""
    def cost(a, b):
        d = distances.get((True, a, b))
        return d if d is not None else distances[False, a, b]
    n = len(drops)
    best = {(1 << k, k): (cost(pickup, drops[k]), [drops[k]]) for k in range(n)}
    for mask in range(1, 1 << n):
        for last in range(n):
            if (mask, last) not in best:
                continue
            c, path = best[mask, last]
            for nxt in range(n):
                if mask & (1 << nxt):
                    continue
                key = (mask | 1 << nxt, nxt)
                cand = (c + cost(drops[last], drops[nxt]), path + [drops[nxt]])
                if key not in best or cand[0] < best[key][0]:
                    best[key] = cand
    full = (1 << n) - 1
    total, path = min(((c + (cost(path[-1], pickup) if back else 0), path)
                       for (mask, _), (c, path) in best.items() if mask == full), key=lambda t: (t[0], t[1]))
    return path


def contract_route(contract, kind, distances):
    pickup, drops = contract['pickup'], list(contract['drops'])
    back = contract.get('return', False)
    if contract.get('order') != 'fixed' and len(drops) > 1:
        assert kind not in (4, 7), 'transit relays and Island post keep their authored order'
        drops = order_drops(pickup, drops, back, distances)
    return [pickup] + drops + ([pickup] if back else [])


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
    """Conservative tile-centre distances on the core's collision grid (its
    four scenes stitched, district-world pixels)."""
    width, height = world2x.WORLD_TW, world2x.WORLD_TH
    grid = world2x.world_grid(0)
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
    # Conservative slow road vehicle is 15/16 pixels per video frame (the
    # scooter's top speed). Truck-only jobs use 17/16. Handling allows braking,
    # turns and interaction; it is a tuning hypothesis, not idle time the game
    # imposes on the player.
    road_speed = 63.75 if kind == 3 else 56.25
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


def story_order(quests):
    """The story's order: a contract waits for the one before it in the plot
    (`after`). Chapter k opens with the previous chapter's spine contract
    done, so its opening call always plays before its contracts; a contract
    may also wait for one in its own chapter."""
    spines = [None] + [c['spine'] - 1 for c in STORY['chapters'][:-1]]
    for index, (quest, contract) in enumerate(zip(quests, STORY['contracts'])):
        chapter = index // 8
        if 'after' in contract:
            after = contract['after'] - 1
            assert after // 8 == chapter and after != index, (quest['id'], 'waits within its chapter')
        else:
            after = spines[chapter]
        quest['after'] = None if after is None else quests[after]['id']
    ids = [q['id'] for q in quests]

    def closure(index):
        seen = []
        while quests[index]['after'] is not None:
            index = ids.index(quests[index]['after'])
            assert index not in seen, 'story order loops'
            seen.append(index)
        return seen
    for chapter in range(1, len(STORY['chapters'])):
        first = quests[chapter * 8]
        spine = spines[chapter]
        assert spine // 8 == chapter - 1, f'chapter {chapter} spine is in the previous chapter'
        # A chapter opens when its first contract can be taken; every other
        # contract of the chapter needs at least as much.
        assert first['after'] == ids[spine] and first['min_completed'] == chapter * 6, first['id']
        for index in range(chapter * 8, chapter * 8 + 8):
            assert quests[index]['min_completed'] >= first['min_completed'] and spine in closure(index), ids[index]
        assert quests[spine]['required_vehicle'] in (255, 0), f'chapter {chapter} spine needs a special vehicle'
    #Check that the story can always be finished by distinct completions.
    done = set()
    while len(done) < len(quests):
        open_now = {i for i, q in enumerate(quests) if q['min_completed'] <= len(done) and
                    (q['after'] is None or ids.index(q['after']) in done)} - done
        assert open_now, f'Unlock deadlock after {len(done)} completions'
        done.add(min(open_now))


def main():
    # Core stops in district-world pixels (core2x maps the 1x plan).
    stops = [(u, v, name, transit) for u, v, name, transit, _ in CORE_STOPS]
    #Fictional service entrances on existing Island walkable land, not claims
    #about surveyed public access or exact real-world building entrances.
    m = world2x.district_map(0)
    stops += [(*m.point(u, v), name, 0) for u, v, name in
              ((560, 928, 'HANLAN SERVICE'), (760, 944, 'CENTRE PARK POST'), (912, 912, 'WARD COTTAGE POST'))]
    distances = shortest_routes(stops)
    for i in range(len(stops)):
        assert distances.get((False, i, i)) == 0, f'Blocked stop: {stops[i][2]}'
    quests = []
    contracts = STORY['contracts']
    assert len(contracts) == 72 and len(CHAPTERS) == 9
    for index, contract in enumerate(contracts):
        chapter, kind = divmod(index, 8)
        title, brief = contract['title'], contract['brief']
        route = contract_route(contract, kind, distances)
        assert len(title) <= 18 and len(brief) == 2 and all(len(line) <= 18 for line in brief), title
        assert 2 <= len(route) <= 12 and all(a != b for a, b in zip(route, route[1:])), (title, route)
        assert all(0 <= i < 27 for i in route), title
        estimate, seconds = route_estimate(route, kind, distances)
        if index < 3:
            seconds = 120  #Tutorials preserve time to learn the controls.
        base_unlock = 3 if kind in (3, 4) else 8 if kind == 5 else 12 if kind == 7 else 0
        reward = (70 + math.ceil(estimate['vehicle_route_pixels'] / 80)
                  + (len(route) - 1) * 10 + estimate['fictional_ferry_fares']
                  + (25 if kind in (1, 3, 5) else 15 if kind == 2 else 0)
                  + chapter * 12)
        quests.append({
            'id': f'contract-{index + 1:02d}', 'title': title, 'brief': list(brief),
            'chapter': CHAPTERS[chapter], 'kind': KINDS[kind], 'kind_id': kind,
            'required_vehicle': 1 if kind == 3 else 0 if kind == 5 else 255,
            'min_completed': max(base_unlock, chapter * 6, contract.get('unlock', 0)), 'route': route,
            'time_limit_seconds': seconds, 'reward': reward, 'timing_design': estimate,
        })
    assert len(quests) == 72 and len({tuple(q['route']) for q in quests}) == 72
    assert len({q['title'] for q in quests}) == 72
    story_order(quests)
    content = {
        'status': 'engine-integrated',
        'scope': 'Compressed central Toronto prototype; broad Old Toronto map accuracy and full campaign duration remain release checks',
        'duration_target_minutes': 120, 'duration_verified': False,
        'duration_notice': 'Authored contract counts, shortest-path models and deadlines do not verify duration or enjoyment. Measure representative jobs and a complete campaign.',
        'quest_types': KINDS, 'chapters': CHAPTERS,
        'stops': [{'id': i, **dict(zip(('district', 'u', 'v'), world2x.scene_of(0, s[0], s[1]))), 'name': s[2], 'transit': s[3],
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
        stop.update(reserved=0)
    content['stops'].extend(extra['stops'])
    quests.extend(extra['quests'])
    assert len(quests) == 80 and len(content['stops']) == 35
    eastern = json.loads((ROOT / 'content/districts/east_jobs.json').read_text())
    assert [s['id'] for s in eastern['stops']] == list(range(35, 43))
    assert [q['id'] for q in eastern['quests']] == [f'contract-{i:02d}' for i in range(81, 89)]
    content['stops'].extend(eastern['stops'])
    quests.extend(eastern['quests'])
    content['scope'] = 'Four linked original compressed districts, each drawn at double scale in four native scenes: central Toronto, western neighbourhoods, High Park/Junction and eastern Riverdale/Leslieville. Full Old Toronto and measured duration remain release checks.'
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
        after = quest.get('after')
        after = 255 if after is None else [q['id'] for q in quests].index(after)
        code.append('  {"%s",%d,%d,%d,%d,%d,%d,%d,{%s}},' % (
            quest['title'], quest['kind_id'], len(quest['route']), quest['required_vehicle'],
            quest['min_completed'], after, quest['time_limit_seconds'], quest['reward'], ','.join(map(str, route))))
    code += ['};', 'static const char td_briefs[TD_QUESTS][37] = {']
    for quest in quests:
        brief = ''.join(line.ljust(18) for line in quest['brief'])
        code.append(f'  "{brief}",')
    code += ['};']
    code += parking_code(content['stops'])
    # Street names live in their own bank: the HUD lookup tables are large.
    streets = ['//Generated by scripts/create_campaign.py; original authored content.',
               '#pragma bank 255', '#include <string.h>', '#include "td_game.h"']
    street_names, street_segments = [], []
    aliases = {'Colborne Lodge Drive south approach':'COLBORNE LODGE DR',
               'Martin Goodman waterfront path':'MARTIN GOODMAN TRL',
               'High Park formal spine':'HIGH PARK WALK',
               'Colborne fictional service entrance':'COLBORNE WALK',
               'Beaty pedestrian bridge':'BEATY FOOTBRIDGE',
               'Sorauren park path':'SORAUREN PARK',
               'High Park Boulevard park walk':'HIGH PARK BLVD',
               'Spring Road park walk':'SPRING RD',
               'High Park Loop platform':'HIGH PARK LOOP',
               'Pape Avenue north fragment':'PAPE AVE',
               'Pape Avenue south fragment':'PAPE AVE',
               'Pape pedestrian rail crossing':'PAPE RAIL CROSSING',
               'Withrow park walk':'WITHROW PARK',
               'Greenwood park walk':'GREENWOOD PARK',
               'Chester station approach':'CHESTER STATION',
               'Sunnyside pavilion approach':'SUNNYSIDE PAVILION'}
    world = json.loads((ROOT / 'content/districts/world.json').read_text())
    # Street centrelines in district-world pixels: the core's from the
    # Centreline-derived layout, the outer districts' from their art plans.
    spans = {0: list(street_spans())}
    for old in (1, 2, 3):
        metadata = json.loads((ROOT / f'content/districts/{world2x.OLD_NAMES[old]}_art.json').read_text())
        spans[old] = []
        for road in metadata['world']['roads'] + metadata['world']['footpaths']:
            name = aliases.get(road['name'], road['name'].upper().replace(' STREET WEST',' ST W').replace(' STREET EAST',' ST E').replace(' STREET',' ST').replace(' AVENUE',' AVE').replace(' BOULEVARD WEST',' BLVD W').replace(' BOULEVARD',' BLVD').replace(' ROAD',' RD').replace(' DRIVE',' DR'))[:18]
            assert len(name) <= 18, ('HUD street name', name)
            for a, b in zip(road['points'], road['points'][1:]):
                spans[old].append((min(a[0], b[0]), min(a[1], b[1]), max(a[0], b[0]), max(a[1], b[1]), name))
    # Each scene lists the parts of its district's streets that cross it.
    for district in range(len(world['districts'])):
        ox, oy = world2x.scene_origin(district)
        for x1, y1, x2, y2, label in spans[district >> 2]:
            if x2 < ox or x1 >= ox + world2x.SCENE_W or y2 < oy or y1 >= oy + world2x.SCENE_H:
                continue
            if label not in street_names:
                street_names.append(label)
            street_segments.append((max(x1, ox) - ox, max(y1, oy) - oy, min(x2, ox + world2x.SCENE_W - 1) - ox,
                                    min(y2, oy + world2x.SCENE_H - 1) - oy, district, street_names.index(label)))
    street_names.append('TORONTO ISLANDS')
    assert len(street_names) < 256
    streets += [f'static const char td_west_street_names[{len(street_names)}][19]={{']
    streets += [f'  "{name}",' for name in street_names]
    streets += ['};','typedef struct { UWORD x1,y1,x2,y2; UBYTE district,name; } td_street_t;',
             f'static const td_street_t td_west_streets[{len(street_segments)}]={{']
    streets += ['  {' + ','.join(map(str, segment)) + '},' for segment in street_segments]
    # Segments are appended scene by scene; index each scene's run so the
    # HUD lookup visits only its own streets in the same order.
    districts = len(world['districts'])
    assert [segment[4] for segment in street_segments] == sorted(segment[4] for segment in street_segments)
    starts = [next((i for i, segment in enumerate(street_segments) if segment[4] >= district), len(street_segments))
              for district in range(districts + 1)]
    assert all(b - a < 256 for a, b in zip(starts, starts[1:]))
    streets += ['};',
             f'static const UWORD td_west_street_start[{districts + 1}]={{' + ','.join(map(str, starts)) + '};']
    # Exact candidate lists per 128-pixel region: a segment is kept only if
    # its least distance to the region does not exceed the best worst-case
    # distance of any segment there, so every possible first minimum stays
    # in the list, in its original order. Entries count from the scene's
    # first segment.
    def box_score(seg, u, v):
        x1, y1, x2, y2 = seg[:4]
        return (x1 - u if u < x1 else u - x2 if u > x2 else 0) + (y1 - v if v < y1 else v - y2 if v > y2 else 0)
    region_start, region_list = [], []
    for district in range(districts):
        segs = list(enumerate(street_segments))[starts[district]:starts[district + 1]]
        for ry in range(8):
            for rx in range(8):
                X1, X2, Y1, Y2 = rx * 128, min(rx * 128 + 127, 1023), ry * 128, min(ry * 128 + 127, 975)
                region_start.append(len(region_list))
                if X1 > X2 or Y1 > Y2 or not segs:
                    continue
                lows = [max(0, seg[0] - X2, X1 - seg[2]) + max(0, seg[1] - Y2, Y1 - seg[3]) for _, seg in segs]
                worst = min(max(box_score(seg, u, v) for u in (X1, X2) for v in (Y1, Y2)) for _, seg in segs)
                region_list += [index - starts[district] for (index, _), low in zip(segs, lows) if low <= worst]
    region_start.append(len(region_list))
    assert len(region_start) == districts * 64 + 1 and len(region_list) < 65536
    streets += [f'static const UWORD td_west_region_start[{len(region_start)}]={{' + ','.join(map(str, region_start)) + '};',
             f'static const UBYTE td_west_region_list[{len(region_list)}]={{' + ','.join(map(str, region_list)) + '};',
             '/* First segment with the least Manhattan distance from (u,v) to its box.',
             '   On the map, only the 128-pixel region\'s exact candidate list is scanned;',
             '   a segment whose x distance alone reaches the best cannot win, and nothing',
             '   beats zero. Positions off the map scan the whole scene. */',
             'static UBYTE td_get_west_street(UBYTE district,UWORD u,UWORD v) {',
             ' UBYTE name=0;UWORD k=0,stop=0,base=0,dx,dy,lo,hi,best=65535;const td_street_t *s;const UBYTE *list=0;',
             f' if(district>={districts})return 0;',
             ' base=td_west_street_start[district];',
             ' if(u<1024&&v<976){',
             '  k=(UWORD)district*64+((v>>7)<<3)+(u>>7);stop=td_west_region_start[k+1];k=td_west_region_start[k];list=td_west_region_list;',
             ' }else{k=0;stop=td_west_street_start[district+1]-base;}',
             ' for(;k<stop;k++){',
             '  s=&td_west_streets[base+(list?list[k]:k)];',
             '  lo=s->x1;hi=s->x2;dx=u<lo?lo-u:u>hi?u-hi:0;if(dx>=best)continue;',
             '  lo=s->y1;hi=s->y2;dy=v<lo?lo-v:v>hi?v-hi:0;dx+=dy;',
             '  if(dx<best){best=dx;name=s->name;if(!best)break;}',
             ' }return name;',
             '}',
             ]
    code += [
             'void td_get_stop(UBYTE i,td_stop_t *d) BANKED { if(i<TD_STOPS) memcpy(d,&td_stops[i],sizeof(td_stop_t)); }',
             'void td_get_job(UBYTE i,td_job_t *d) BANKED { if(i<TD_QUESTS) memcpy(d,&td_jobs[i],sizeof(td_job_t)); }',
             'void td_get_brief(UBYTE i,char *d) BANKED { if(i<TD_QUESTS) memcpy(d,td_briefs[i],37); else d[0]=0; }']
    streets += [
             '/* Name id of the street nearest (u,v) in the current district (the',
             '   Islands below the core\'s shore); td_get_street_name spells it. */',
             'UBYTE td_get_street(UWORD u,UWORD v) BANKED {',
             '  if((td.district==2||td.district==3)&&v>=%d)return %d;' % (L_MAINLAND_BOTTOM, len(street_names) - 1),
             '  return td_get_west_street(td.district,u,v);',
             '}',
             'void td_get_street_name(UBYTE id,char *d) BANKED {memcpy(d,td_west_street_names[id],19);}']
    (ENGINE / 'src').mkdir(parents=True, exist_ok=True)
    (ENGINE / 'src/td_content.c').write_text('\n'.join(code) + '\n')
    (ENGINE / 'src/td_street_names.c').write_text('\n'.join(streets) + '\n')
    # The UI font is original and generated by create_ui_art.py.
    print(f'Compiled {len(quests)} unique authored contracts, {len(content["stops"])} stops, native briefs, and train/bus/ferry services. Duration remains unverified.')


if __name__ == '__main__':
    main()
