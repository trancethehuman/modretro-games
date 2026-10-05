"""Validate native campaign compilation and preserved core/Island connectivity.

Western registered scene/connectivity checks live in check_district_world.py.
Neither content counts nor shortest paths establish elapsed gameplay duration.
"""
import hashlib
import json
import re
from collections import deque
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CORE_STOPS, CORE_QUESTS = 27, 72
TOTAL_STOPS, TOTAL_QUESTS = 64, 104


def decode(text):
    result, pos = [], 0
    while pos < len(text):
        assert pos + 2 < len(text), 'Truncated native byte run'
        value = int(text[pos:pos + 2], 16)
        pos += 2
        if text[pos] == '!':
            count, pos = 1, pos + 1
        else:
            end = text.index('+', pos)
            count, pos = int(text[pos:end], 16), end + 1
        assert count > 0, 'Empty native byte run'
        result.extend([value] * count)
    return result


def canonical_sha(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def table(code, name):
    found = re.search(r'\b' + re.escape(name) + r'\s*\[[^;=]+\]\s*=\s*\{(.*?)\n\};', code, re.S)
    assert found, f'Missing native table: {name}'
    return found.group(1)


def check_parking(stops, code):
    from create_campaign import parking_code, parking_rows

    expected = parking_rows(stops)
    rows = [tuple(map(int, row)) for row in re.findall(r'\{(\d+),(\d+),(\d+)\}', table(code, 'td_parking'))]
    native = [(stop, u, v) for u, v, stop in rows]
    assert len({stop for stop, _, _ in native}) == len(native), 'Repeated native parking stop ID'
    assert native == expected, 'Native parking IDs/coordinates differ from source anchors'
    foot_ids = {stop['id'] for stop in stops if stop.get('reserved', 0) & 1}
    assert {stop for stop, _, _ in native} == foot_ids, 'Every native foot-only client requires exactly one auxiliary parking row'
    assert '\n'.join(parking_code(stops)) in code, 'Auxiliary parking table/getter differs from deterministic generation'
    assert code.count('UBYTE td_get_parking(') == 1, 'Native parking getter must have one definition'
    world = json.loads((ROOT / 'content/districts/world.json').read_text())
    resources = {}
    for index, u, v in native:
        stop = stops[index]
        district = stop.get('district', 0)
        assert 0 <= district < len(world['districts']) and world['districts'][district]['id'] == district
        if district not in resources:
            name = world['districts'][district]['scene']
            scene = json.loads((ROOT / 'project/project/scenes' / name / 'scene.gbsres').read_text())
            grid = decode(scene['collisions'])
            assert len(grid) == scene['width'] * scene['height'], 'Parking collision resource dimensions disagree'
            resources[district] = (scene['width'], scene['height'], grid)
        width, height, grid = resources[district]
        assert all(grid[y * width + x] == 0 for y in range((v - 5) // 8, (v + 5) // 8 + 1)
                   for x in range((u - 5) // 8, (u + 5) // 8 + 1)), f'Blocked native car parking footprint: {index}'
        client = (stop['u'] // 8, stop['v'] // 8)
        assert not grid[client[1] * width + client[0]] & 15, f'Blocked walking client: {index}'
        parked = (u // 8, v // 8)
        queue, distance = deque([parked]), {parked: 0}
        while queue and client not in distance:
            x, y = queue.popleft()
            for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                if 0 <= nx < width and 0 <= ny < height and (nx, ny) not in distance and not grid[ny * width + nx] & 15:
                    distance[nx, ny] = distance[x, y] + 8
                    queue.append((nx, ny))
        assert client in distance and 0 < distance[client] <= 900, f'Parking anchor needs a short connected walking approach: {index}'
    return len(native)


def check():
    # The native host harness imports decode() by file path. Keep sibling
    # campaign-authoring dependencies local to full campaign validation.
    from create_district_jobs import BASE_STOPS_SHA256, BASE_QUESTS_SHA256, BASE_QUEST_FIELDS
    from create_district_jobs import RouteModel, point
    from district_sources import read_district_art
    from island_campaign import (historical_stop, ISLAND_IDS, ISLAND_DISTRICT,
                                 validate_preserved_campaign, validate_relocated_stops)

    campaign = json.loads((ROOT / 'content/campaign.json').read_text())
    from stop_names import validate_current
    validate_current(campaign['stops'][18])
    city = json.loads((ROOT / 'content/city_art.json').read_text())
    scene = json.loads((ROOT / 'project/project/scenes/toronto_city/scene.gbsres').read_text())
    background = json.loads((ROOT / 'project/assets/backgrounds/toronto_city.png.gbsres').read_text())
    width, height = scene['width'], scene['height']
    grid, attrs = decode(scene['collisions']), decode(background['tileColors'])
    assert len(grid) == len(attrs) == width * height, 'Native grid dimensions disagree'
    assert city['dimensions'] == [width * 8, height * 8]
    assert width * height <= 16384, 'Tile map must fit one ROM bank'
    assert len(city['blocks']) >= 50, 'Core must retain architectural variety'
    assert len({block['style'] for block in city['blocks']}) == 6
    assert any(attr & 128 for attr in attrs), 'Missing actual CGB roof/canopy priority'

    def blocked(x, y, car=False):
        if not (0 <= x < width and 0 <= y < height):
            return True
        return grid[y * width + x] != 0 if car else bool(grid[y * width + x] & 15)

    def usable(x, y, car):
        return not blocked(x, y, car) and (not car or all(not blocked(x + dx, y + dy, True)
                      for dx in (-1, 0, 1) for dy in (-1, 0, 1)))

    def flood(origin, car=False):
        queue, seen = deque([origin]), {origin}
        while queue:
            x, y = queue.popleft()
            for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                if (nx, ny) not in seen and usable(nx, ny, car):
                    seen.add((nx, ny))
                    queue.append((nx, ny))
        return seen

    stops, quests = campaign['stops'], campaign['quests']
    assert len(stops) == TOTAL_STOPS and [s['id'] for s in stops] == list(range(TOTAL_STOPS))
    assert len(quests) == TOTAL_QUESTS and len({q['id'] for q in quests}) == TOTAL_QUESTS
    assert [q['id'] for q in quests] == [f'contract-{i:02d}' for i in range(1, TOTAL_QUESTS + 1)]
    validate_preserved_campaign(campaign)
    assert all(s.get('district', 0) == (ISLAND_DISTRICT if s['id'] in ISLAND_IDS else 0)
               and s.get('reserved', 0) == 0 for s in stops[:CORE_STOPS])
    assert all(all(stop < CORE_STOPS for stop in q['route']) for q in quests[:CORE_QUESTS])
    locations = [(s['u'] // 8, s['v'] // 8) for s in stops[:CORE_STOPS]]
    for stop, (x, y) in zip(stops[:CORE_STOPS], locations):
        if stop['id'] in ISLAND_IDS:
            continue
        assert usable(x, y, False), f"Blocked core stop: {stop['name']}"
    walk, car = flood(locations[0]), flood(locations[0], True)
    ferry = campaign['transit']['ferry']['stops']
    assert ferry == [10, 20, 21, 22], 'Preserve mainland-spoke Island ferry topology'
    assert locations[ferry[0]] in walk
    assert all(locations[s['id']] in walk and locations[s['id']] in car
               for s in stops[:CORE_STOPS] if s['id'] not in ISLAND_IDS), 'Disconnected mainland core endpoint'
    world = json.loads((ROOT / 'content/districts/world.json').read_text())
    assert len(world['districts']) == 7 and world['districts'][ISLAND_DISTRICT]['scene'] == 'toronto_islands' and world['districts'][6]['scene'] == 'toronto_north'
    island = read_district_art(world['districts'][ISLAND_DISTRICT])
    validate_relocated_stops(stops, island)
    model = RouteModel(world, stops)
    assert not any(portal[side]['district'] == ISLAND_DISTRICT for portal in world['portals'] for side in ('from', 'to')), 'Islands must not acquire an ordinary mainland seam'
    assert set(model.grids[ISLAND_DISTRICT][2]) <= {15, 16}, 'Public Islands must contain no drivable terrain'
    for index in ISLAND_IDS:
        stop = stops[index]
        model.full_foot_shortest(stops[20], stop)
        assert not model.usable(point(stop['district'], stop['u'], stop['v']), True), 'Island stop admits a vehicle'
    for dock in ferry[1:]:
        for origin, target in ((10, dock), (dock, 10)):
            leg = model.service_leg(stops[origin], stops[target])
            assert leg == dict(mode='ferry', from_stop=origin, to_stop=target,
                               walking_pixels=0, worst_wait_seconds=28, ride_seconds=8, fare=4)
        try:
            model.shortest(point(0, stops[10]['u'], stops[10]['v']),
                           point(ISLAND_DISTRICT, stops[dock]['u'], stops[dock]['v']), False)
        except AssertionError as error:
            assert 'No foot route' in str(error)
        else:
            raise AssertionError('Ordinary foot graph crosses the harbour')
    island_jobs = [(index, quest) for index, quest in enumerate(quests) if quest['kind_id'] == 7]
    assert [index for index, _ in island_jobs] == [7, 15, 23, 31, 39, 47, 55, 63, 71]
    for index, quest in island_jobs:
        from create_campaign import route_estimate
        estimate, _ = route_estimate(quest['route'], 7, {}, model, stops)
        actual = quest['timing_design']
        assert all(actual[key] == value for key, value in estimate.items()), f'Stale Island full-body/ferry model: {index}'
        assert actual['preserved_deadline_seconds'] == quest['time_limit_seconds']
        assert actual['modeled_slack_seconds'] == round(quest['time_limit_seconds'] - actual['modeled_movement_wait_and_handling_seconds'], 1) > 0
    for quest in quests[:CORE_QUESTS]:
        if quest['required_vehicle'] != 255:
            assert all(locations[i] in car for i in quest['route'])

    west = json.loads((ROOT / 'content/districts/west_jobs.json').read_text())
    preserved_fields = west['preserved_base_quest_fields']
    assert tuple(preserved_fields) == BASE_QUEST_FIELDS
    assert west['preserved_base_stops_sha256'] == BASE_STOPS_SHA256
    assert west['preserved_base_quests_sha256'] == BASE_QUESTS_SHA256
    normalized_stops = [{field: historical_stop(s)[field] for field in ('id', 'u', 'v', 'name', 'transit')} for s in stops[:CORE_STOPS]]
    normalized_quests = [{field: q[field] for field in preserved_fields} for q in quests[:CORE_QUESTS]]
    assert canonical_sha(normalized_stops) == west['preserved_base_stops_sha256'], 'Original core stops changed'
    assert canonical_sha(normalized_quests) == west['preserved_base_quests_sha256'], 'Original 72 native contract fields changed'
    assert stops[CORE_STOPS:35] == west['stops'], 'Western stop fusion is stale'
    assert quests[CORE_QUESTS:80] == west['quests'], 'Western contract fusion is stale'
    east = json.loads((ROOT / 'content/districts/east_jobs.json').read_text())
    assert stops[35:43] == east['stops'] and quests[80:88] == east['quests'], 'Eastern content fusion is stale'
    streetcar = json.loads((ROOT / 'content/streetcar.json').read_text())
    assert stops[43:51] == streetcar['stops'], 'Queen streetcar platform fusion is stale'
    assert campaign['transit']['streetcar501'] == streetcar['service'], 'Queen streetcar schedule fusion is stale'
    from create_east_jobs import preserved_prefix
    preserved_prefix(campaign)
    from create_port_jobs import author as authored_port, preserved_prefix as preserved_port_prefix
    preserved_port_prefix(campaign)
    port = json.loads((ROOT / 'content/districts/port_lands_jobs.json').read_text())
    assert port == authored_port(), 'Port Lands content provenance/route estimates are stale'
    assert stops[51:59] == port['stops'] and quests[88:96] == port['quests'], 'Port Lands content fusion is stale'
    assert {i for q in quests[88:96] for i in q['route'] if i >= 51} == set(range(51,59)), 'Every original Port Lands client must serve a Port contract'
    from create_north_jobs import author as authored_north, preserved_prefix as preserved_north_prefix
    preserved_north_prefix(campaign)
    north = json.loads((ROOT / 'content/districts/north_jobs.json').read_text())
    assert north == authored_north(), 'North source/body-route provenance is stale'
    assert stops[59:64] == north['stops'] and quests[96:104] == north['quests'], 'North content fusion is stale'
    assert {i for q in quests[96:104] for i in q['route'] if i >=59} == set(range(59,64)), 'Every North endpoint must serve a contract'
    assert campaign['transit']['line1']['stops'] == [0,12,13,14,15,16,17,59,60]
    assert campaign['transit']['line1']['period_seconds'] == 18 and campaign['transit']['line1']['fare'] == 3
    assert [s['id'] for s in stops[59:] if s['transit']] == [59,60], 'North clients must not become masked transit aliases'
    assert len(campaign['chapters']) == 13 and campaign['chapters'] == [quests[i]['chapter'] for i in range(0,104,8)]
    assert campaign['status'] == 'engine-integrated' and campaign['duration_target_minutes'] >= 120
    assert campaign['duration_verified'] is False, 'Elapsed campaign duration requires measured play evidence'
    assert len({q['kind_id'] for q in quests}) == 8
    for quest in quests:
        assert 2 <= len(quest['route']) <= 12 and all(0 <= i < len(stops) for i in quest['route'])
        assert quest['min_completed'] < len(quests) and quest['time_limit_seconds'] > 0 and quest['reward'] > 0
        assert quest['required_vehicle'] in (0, 1, 2, 3, 255)
        assert len(quest['title']) <= 18 and quest['title'].isascii()
        assert len(quest['brief']) == 2 and all(len(line) <= 18 and line.isascii() for line in quest['brief'])
        if quest['required_vehicle'] != 255:
            assert all(not stops[i].get('foot_only', False) for i in quest['route'])
    assert len({q['title'] for q in quests}) == TOTAL_QUESTS, 'Repeated contract titles'
    assert len({tuple(q['route']) for q in quests}) == TOTAL_QUESTS, 'Repeated contract routes'
    completed = set()
    while True:
        available = {q['id'] for q in quests if q['min_completed'] <= len(completed)}
        if available <= completed:
            break
        completed |= available
    assert len(completed) == TOTAL_QUESTS, 'Unique-completion progression can deadlock'

    code = (ROOT / 'project/plugins/toronto-driving/engine/src/td_content.c').read_text()
    # Optional trailing fields on preserved legacy rows use C's zero initialization.
    native_stops = re.findall(r'\{(\d+),(\d+),"([^"]+)",(\d+)(?:,(\d+),(\d+))?\}', table(code, 'td_stops'))
    assert len(native_stops) == TOTAL_STOPS, 'Native stop row count disagrees'
    for stop, row in zip(stops, native_stops):
        u, v, name, transit, district, reserved = row
        expected = (stop['u'], stop['v'], stop['name'], stop['transit'], stop.get('district', 0), stop.get('reserved', 0))
        assert (int(u), int(v), name, int(transit), int(district or 0), int(reserved or 0)) == expected, f"Stale native stop: {stop['id']}"
    native_jobs = re.findall(r'\{"([^"]+)",(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),\{([\d,]+)\}\}', table(code, 'td_jobs'))
    assert len(native_jobs) == TOTAL_QUESTS, 'Native contract row count disagrees'
    for quest, row in zip(quests, native_jobs):
        title, *numbers, route = row
        expected_numbers = [quest['kind_id'], len(quest['route']), quest['required_vehicle'], quest['min_completed'], quest['time_limit_seconds'], quest['reward']]
        assert title == quest['title'][:18].upper() and list(map(int, numbers)) == expected_numbers, f"Stale native contract: {quest['id']}"
        assert list(map(int, route.split(','))) == quest['route'] + [255] * (12 - len(quest['route']))
    native_briefs = re.findall(r'"([^"]*)"', table(code, 'td_briefs'))
    assert native_briefs == [''.join(line.ljust(18) for line in q['brief']) for q in quests], 'Native briefs differ'
    parking_count = check_parking(stops, code)
    header = (ROOT / 'project/plugins/toronto-driving/engine/include/td_game.h').read_text()
    for macro, count in [('TD_STOPS', TOTAL_STOPS), ('TD_QUESTS', TOTAL_QUESTS)]:
        assert re.search(r'#define\s+' + macro + r'\s+' + str(count) + r'\b', header), f'{macro} differs from campaign'
    completed_bytes = int(re.search(r'#define\s+TD_COMPLETE_BYTES\s+(\d+)', header).group(1))
    assert completed_bytes * 8 >= TOTAL_QUESTS
    assert re.search(r'UBYTE\s+td_get_parking\s*\(\s*UBYTE\s+stop\s*,\s*UWORD\s*\*\s*u\s*,\s*UWORD\s*\*\s*v\s*\)\s+BANKED\s*;', header), 'Parking getter must retain its banked whole-pixel API'
    print(f'Native campaign source: {TOTAL_QUESTS} contracts/{TOTAL_STOPS} stops and briefs match C; {parking_count} parking anchors match clear bodies/footpaths; all 96 native job fields and 59 stop ordinals preserved with six declared Island geometries relocated and one exact Bloorcourt display correction. Typed ferry spokes, conservative full-body Island routes/deadline models, ordinary harbour disconnection, Port routes and unlock closure passed. Native play and measured duration require separate evidence.')


if __name__ == '__main__':
    check()
