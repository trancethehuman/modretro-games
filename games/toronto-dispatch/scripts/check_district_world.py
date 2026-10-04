"""Validate actual registered district resources and compiled world data.

This is source/resource validation, not emulator, hardware, full-city coverage,
performance or measured two-hour campaign evidence. No files are regenerated.
"""
import hashlib
import json
import re
from pathlib import Path
from PIL import Image
from check_campaign import ROOT, decode, TOTAL_STOPS, TOTAL_QUESTS
from create_district_jobs import RouteModel, point
from district_sources import read_district_art
from island_campaign import ISLAND_DISTRICT, ISLAND_IDS
import create_district_world
import create_world_routes
import check_traffic_lanes

SOURCE_COLORS = {tuple(bytes.fromhex(value)) for value in ('071821', '306850', '86c06c', 'e0f8cf')}


def read(path):
    return json.loads(path.read_text())


def native_tile(resources, district, x, y):
    width, height, grid, _ = resources[district]
    return grid[y * width + x] if 0 <= x < width and 0 <= y < height else 15


def native_footprint(resources, district, u, v, half=5):
    # Match td_district_drivable's centre bounds and every overlapped tile.
    if u < 8 or v < 8 or u > 1016 or v > 968:
        return False
    return all(native_tile(resources, district, x, y) == 0
               for y in range((v - half) // 8, (v + half) // 8 + 1)
               for x in range((u - half) // 8, (u + half) // 8 + 1))


def native_walkable(resources, district, u, v):
    return not (native_tile(resources, district, u // 8, v // 8) & 15)


def check_seams(world, resources):
    """Check reciprocal native inset lanes and adjacent atlas rectangles.

    Compression may give the two ends different lateral coordinates. Native
    crossings preserve their offset from each end's own centre, not a global
    lateral coordinate. N/S uses v as its crossing axis; E/W uses u.
    """
    districts = {district['id']: district for district in world['districts']}

    def endpoint(value):
        assert all(type(value[key]) is int for key in ('district', 'u', 'v')), 'Non-integer seam endpoint'
        assert value['district'] in districts and value['district'] in resources, 'Unregistered seam district'
        district = districts[value['district']]
        assert (district['width_pixels'], district['height_pixels']) == (1024, 976)
        u, v = value['u'], value['v']
        if u in (24, 1000):
            assert 32 <= v < 944, 'Horizontal seam lateral coordinate outside native bounds'
            return 'horizontal'
        assert v in (24, 952) and 32 <= u < 992, 'Vertical seam outside native inset/lateral bounds'
        return 'vertical'

    directed, unordered = set(), set()
    for portal in world['portals']:
        first, last = portal['from'], portal['to']
        assert first['district'] != last['district'] and set(portal['access']) <= {'foot', 'vehicle'}
        assert 'foot' in portal['access']
        axis = endpoint(first)
        assert endpoint(last) == axis, 'Mixed seam axes'
        horizontal = axis == 'horizontal'
        normal, lateral = ('u', 'v') if horizontal else ('v', 'u')
        extent = 1024 if horizontal else 976
        assert last[normal] == extent - first[normal], 'Seam insets must be opposite'
        a, b = districts[first['district']], districts[last['district']]
        atlas_normal, atlas_lateral = ('atlas_x', 'atlas_y') if horizontal else ('atlas_y', 'atlas_x')
        lateral_extent = 976 if horizontal else 1024
        delta = extent if first[normal] != 24 else -extent
        assert b[atlas_normal] == a[atlas_normal] + delta, 'Seam atlas edges are not adjacent'
        assert max(a[atlas_lateral], b[atlas_lateral]) < min(a[atlas_lateral], b[atlas_lateral]) + lateral_extent, 'Seam atlas edges do not overlap'
        assert type(portal['modeled_crossing_pixels']) is int and portal['modeled_crossing_pixels'] > 0
        endpoints = tuple(sorted((tuple(first[k] for k in ('district', 'u', 'v')), tuple(last[k] for k in ('district', 'u', 'v')))))
        assert endpoints not in unordered, 'Duplicate seam pair'
        unordered.add(endpoints)
        vehicle = int('vehicle' in portal['access'])
        for origin, destination in ((first, last), (last, first)):
            directed.add((origin['district'], origin['u'], origin['v'], destination['district'], destination['u'], destination['v'], vehicle))

            def position(coordinate, offset):
                return (coordinate, origin[lateral] + offset) if horizontal else (origin[lateral] + offset, coordinate)

            # Walking queues at the inset trigger, before the outer fence.
            # Four pixels of overshoot also covers the road-motion approach.
            inset = origin[normal]
            for coordinate in range(inset - 4, inset + 5):
                for offset in range(-28, 29):
                    assert native_walkable(resources, origin['district'], *position(coordinate, offset)), f"Blocked foot seam lane: {portal['name']}"
            if vehicle:
                approach = range(8, 25) if inset == 24 else range(extent - 24, extent - 7)
                for coordinate in approach:
                    for offset in range(-18, 19):
                        assert native_footprint(resources, origin['district'], *position(coordinate, offset)), f"Blocked native car seam lane: {portal['name']}"
                for offset in (-12, 0, 12):
                    assert native_footprint(resources, origin['district'], *position(inset, offset), half=8), f"Blocked centred car seam footprint: {portal['name']}"
            else:
                assert not native_footprint(resources, origin['district'], origin['u'], origin['v']), 'Foot-only seam admits a car'
    return directed


def check():
    world = read(ROOT / 'content/districts/world.json')
    campaign = read(ROOT / 'content/campaign.json')
    settings = read(ROOT / 'project/project/settings.gbsres')
    western_jobs = read(ROOT / 'content/districts/west_jobs.json')
    district_count = int(re.search(r'#define TD_DISTRICT_COUNT (\d+)', (ROOT / 'project/plugins/toronto-driving/engine/include/td_district.h').read_text()).group(1))
    assert district_count == len(world['districts']) and [d['id'] for d in world['districts']] == list(range(district_count))
    assert len(campaign['stops']) == TOTAL_STOPS and len(campaign['quests']) == TOTAL_QUESTS
    assert settings['colorMode'] == 'color', '384-tile budget requires color-only project'
    resources, budgets, metadata = {}, [], {}
    for district in world['districts']:
        assert district['scene'].startswith('toronto_')
        slug = district['scene'].removeprefix('toronto_')
        scene = read(ROOT / 'project/project/scenes' / district['scene'] / 'scene.gbsres')
        filename = f'toronto_{slug}.png'
        background_path = ROOT / 'project/assets/backgrounds' / filename
        bg = read(background_path.with_suffix('.png.gbsres'))
        attrs = decode(bg['tileColors'])
        grid = decode(scene['collisions'])
        meta = read_district_art(district)
        original_attrs = read(ROOT / meta.get('source_attributes', f'project/original-art/{slug}_attributes.json'))
        width, height = scene['width'], scene['height']
        assert scene['type'] == 'TORONTO' and scene['symbol'] == district['symbol']
        assert (width, height) == (128, 122)
        assert len(grid) == len(attrs) == 15616 < 16384, 'Native tile map must fit its bank'
        assert bg['id'] == scene['backgroundId'] and bg['filename'] == filename
        assert (bg['width'], bg['height'], bg['imageWidth'], bg['imageHeight']) == (128, 122, 1024, 976)
        assert bg['autoColor'] is False, 'Manual source palette expected'
        assert len(scene['paletteIds']) == 7
        assert attrs == original_attrs, f'{slug}: registered palette/priority bytes differ from source'
        assert set(grid) <= {0, 16, 15}, f'{slug}: unexpected native collision flags'
        assert all((a & 7) <= 6 and not(a & 0x78) for a in attrs), f'{slug}: palette/attribute flags out of authored range'
        assert any(a & 128 for a in attrs), f'{slug}: missing actual native roof/canopy priority'
        with Image.open(background_path) as opened:
            img = opened.convert('RGB')
        assert img.size == (1024, 976)
        pixels = img.tobytes()
        assert {tuple(pixels[i:i + 3]) for i in range(0, len(pixels), 3)} <= SOURCE_COLORS, f'{slug}: non-native source shade'
        raw_patterns, flipped_patterns = set(), set()
        for ty in range(height):
            for tx in range(width):
                tile = img.crop((tx * 8, ty * 8, tx * 8 + 8, ty * 8 + 8))
                raw_patterns.add(tile.tobytes())
                variants = [tile, tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT), tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM), tile.transpose(Image.Transpose.ROTATE_180)]
                flipped_patterns.add(min(v.tobytes() for v in variants))
        consumed = len(flipped_patterns) if settings.get('autoTileFlipEnabled', False) and scene.get('autoTileFlipEnabled', True) else len(raw_patterns)
        assert consumed <= 384, f'{slug}: actual source pixel patterns exceed color-only import budget'
        budgets.append((slug, len(raw_patterns), len(flipped_patterns)))
        resources[district['id']] = (width, height, grid, attrs)
        captured = next(item for item in western_jobs['collision_resources'] if item['district'] == district['id'])
        assert captured['collision_sha256'] == hashlib.sha256(scene['collisions'].encode()).hexdigest(), f'{slug}: western job model uses stale native collisions'
        if district['id']:
            metadata[district['id']] = meta
            assert meta['dimensions'] == [1024, 976] and meta['tile_dimensions'] == [128, 122]
            assert meta['collisions'] == grid, f'{slug}: registered collision differs from original metadata'
            assert meta['background_filename'] == filename
            assert meta['background_sha256'] == hashlib.sha256(background_path.read_bytes()).hexdigest()
            assert meta['validation']['raw_unique_tiles'] == len(raw_patterns)
            assert meta['validation']['flip_canonical_unique_tiles'] == len(flipped_patterns)
            if district.get('traffic_enabled', True):
                assert meta['road_half_width'] == 24 and meta['walk_half_width'] == 32
            else:
                assert district['id'] == ISLAND_DISTRICT and not meta['roads'] and not meta['traffic_loops'], 'Only public walking Islands disable road traffic'
                assert set(grid) <= {15, 16}, 'Foot-only district admits road vehicles'
            assert all(not (a & 128) for a, c in zip(attrs, grid) if c == 0), f'{slug}: raised roof/canopy priority covers asphalt'
            assert (ROOT / meta['source_research']).exists()
        # Ground building footprints must be blocked even while decorative lips
        # and tree canopies can intentionally occlude a passable ground plane.
        blocks = metadata[district['id']]['blocks'] if district['id'] else read(ROOT / 'content/city_art.json')['blocks']
        for block in blocks:
            left, top = block['x'] // 8, block['y'] // 8
            right = (block['x'] + block['width'] - 1) // 8
            bottom = (block['y'] + block['depth'] - 1) // 8
            assert all(grid[y * width + x] == 15 for y in range(top, bottom + 1) for x in range(left, right + 1)), f'{slug}: building footprint is passable'

    def tile(district, x, y):
        return native_tile(resources, district, x, y)

    def footprint(district, u, v, half=5):
        return native_footprint(resources, district, u, v, half)

    def walkable(district, u, v):
        return native_walkable(resources, district, u, v)

    def cardinal_points(points, closed=False):
        segments = zip(points, points[1:] + points[:1] if closed else points[1:])
        for a, b in segments:
            assert (a[0] == b[0]) != (a[1] == b[1]), f'Non-cardinal or zero segment: {a}->{b}'
            distance = abs(b[0] - a[0]) + abs(b[1] - a[1])
            for step in range(distance + 1):
                yield a[0] + (step if b[0] > a[0] else -step if b[0] < a[0] else 0), a[1] + (step if b[1] > a[1] else -step if b[1] < a[1] else 0)

    assert len(world['portals']) >= 11
    directed = check_seams(world, resources)
    for district, meta in metadata.items():
        # The researched North manifest names its sole registered destination.
        # Retain historical numeric-port expectations everywhere else.
        if district == 6:
            assert all(p['target'] == 'toronto_city' for p in meta['ports']), 'North may only connect to the two Core gateways'
            expected = {(district, p['x'], p['y'], 0, p['target_x'], p['target_y'], int(not p['foot_only'])) for p in meta['ports']}
        else:
            expected = {(district, p['x'], p['y'], p['target'], p['target_x'], p['target_y'], int(not p['foot_only'])) for p in meta['ports']}
        assert expected == {p for p in directed if p[0] == district}, f'{district}: native seams differ from original ports'
        for route in meta['roads']:
            assert route['name'] and len(route['points']) >= 2
            points = list(cardinal_points(route['points']))
            if district == 6:
                # Two authored south throats paint to the image edge. A
                # conservative half8 centre ends at967; native half5/half7
                # reach968 and crossing uses inset952, checked separately.
                # Admit only these exact paint-only tails, never blocked body
                # paths or another out-of-bounds road.
                outside = {(u, v) for u, v in points if v >= 968}
                if outside:
                    column = {'SPADINA ROAD SOUTH': 336, 'YONGE STREET': 640}.get(route['name'])
                    assert column is not None and outside == {(column, v) for v in range(968, 977)}, 'Unexpected North boundary-road extension'
                    assert footprint(district, column, 968, 7), 'North boundary fails separate full half7 body'
                    points = [(u, v) for u, v in points if v < 968]
            assert all(footprint(district, u, v, 8) for u, v in points), f"Blocked researched road: {route['name']}"
        for route in meta['footpaths']:
            points = list(cardinal_points(route['points']))
            assert all(walkable(district, u, v) for u, v in points), f"Blocked footpath: {route['name']}"
            shared_entrances = {
                'SUMMERHILL ENTRANCE': [[696, 432], [696, 416]],
                'ST CLAIR ENTRANCE': [[688, 160], [688, 176]],
            }
            if district == 6 and route['name'] in shared_entrances:
                # These exact curb approaches share Shaftesbury/St Clair
                # asphalt; unlike the stairs/park paths they do not forbid
                # vehicles. Check every touched full half5 walking tile.
                assert route['points'] == shared_entrances[route['name']], 'Changed shared North station approach'
                assert all(tile(district, u // 8, v // 8) == 0 for u, v in points), 'Shared station approach no longer connects its road curb'
                assert all(not (tile(district, x, y) & 15)
                           for u, v in points
                           for y in range((v - 5) // 8, (v + 5) // 8 + 1)
                           for x in range((u - 5) // 8, (u + 5) // 8 + 1)), 'North station approach blocks full foot body'
            else:
                assert any(tile(district, u // 8, v // 8) == 16 for u, v in points), f"Footpath lacks foot-only terrain: {route['name']}"
        assert len(meta['traffic_loops']) == (0 if district == ISLAND_DISTRICT else 6)
        for loop in meta['traffic_loops']:
            assert 4 <= len(loop) <= 16
            assert all(footprint(district, u, v, 8) for u, v in cardinal_points(loop, closed=True)), f'{district}: traffic swept footprint blocked'

    # Shortest paths use the registered grids, with reciprocal scene edges.
    model = RouteModel(world, campaign['stops'])
    origin = point(0, campaign['stops'][0]['u'], campaign['stops'][0]['v'])
    # All appended courier clients need car/park-hand-off access.
    # Supplemental Queen transit platforms are walking-only boarding points
    # checked independently by check_streetcar.py, including core platforms.
    expansion_clients = [stop for stop in campaign['stops'][27:59] if stop['transit'] == 0]
    for stop in expansion_clients:
        district, u, v = stop['district'], stop['u'], stop['v']
        foot_only = bool(stop.get('foot_only', False))
        assert district in resources and district > 0 and stop['transit'] == 0
        assert bool(stop.get('reserved', 0) & 1) == foot_only, 'Native foot-only flag differs'
        assert walkable(district, u, v)
        model.shortest(origin, point(district, u, v), False)
        if foot_only:
            assert not footprint(district, u, v)
            anchor = stop['parking_anchor']
            assert footprint(district, anchor['u'], anchor['v'], 8)
            model.shortest(origin, point(district, anchor['u'], anchor['v']), True)
            distance, _ = model.shortest(point(district, anchor['u'], anchor['v']), point(district, u, v), False)
            assert 0 < distance <= 900, 'Foot-only client needs a short reachable parking approach'
        else:
            assert footprint(district, u, v, 8)
            model.shortest(origin, point(district, u, v), True)
    # Keep all legacy expansion estimates on their unchanged strict3x3
    # model. North's valid StClair curb needs its explicit fullhalf5 model.
    for quest in campaign['quests'][72:96]:
        for first, last in zip(quest['route'], quest['route'][1:]):
            model.stop_leg(campaign['stops'][first], campaign['stops'][last])
        assert quest['timing_design']['planning_only'] and quest['timing_design']['measured_duration_seconds'] is None

    from create_north_jobs import author as authored_north
    north = authored_north()
    assert campaign['stops'][59:64] == north['stops'] and campaign['quests'][96:104] == north['quests'], 'North explicit full-body route/content differs'

    # A ferry changes scenes; it is not a navigable mainland edge. Every public
    # Island endpoint uses the same conservative full-foot component, and no
    # Island target is rewritten as a parking client or road destination.
    for index in ISLAND_IDS:
        stop = campaign['stops'][index]
        assert stop['district'] == ISLAND_DISTRICT and stop.get('reserved', 0) == 0
        assert not stop.get('foot_only', False) and 'parking_anchor' not in stop
        model.full_foot_shortest(campaign['stops'][20], stop)
        assert not model.usable(point(ISLAND_DISTRICT, stop['u'], stop['v']), True)
    assert not any(portal[side]['district'] == ISLAND_DISTRICT for portal in world['portals'] for side in ('from', 'to'))
    for quest in campaign['quests']:
        if quest['kind_id'] == 7:
            for first, last in zip(quest['route'], quest['route'][1:]):
                model.service_leg(campaign['stops'][first], campaign['stops'][last])

    assert create_district_world.HEADER.read_text() == create_district_world.source(), 'Compiled reciprocal seams/traffic differ'
    # Core now shares the authored banked patrol model; include its lanes and
    # full service footprints rather than validating only expansion metadata.
    check_traffic_lanes.check()
    ped_header = create_world_routes.HEADER.read_text()
    assert ped_header == create_world_routes.source(), 'Compiled pedestrian routes differ from registered grids'
    counts = list(map(int, re.search(rf'td_route_counts\[{district_count}\]=\{{([\d,]+)\}}', ped_header).group(1).split(',')))
    ped_table = re.search(rf'td_district_routes\[{district_count}\]\[128\]\[2\]=\{{(.*?)\n\}};', ped_header, re.S).group(1)
    groups = re.findall(r'^  \{\n(.*?)^  \},', ped_table, re.S | re.M)
    assert len(groups) == district_count and all(24 <= count <= 128 for count in counts)
    for district, (group, count) in enumerate(zip(groups, counts)):
        rows = [tuple(map(int, pair)) for pair in re.findall(r'\{(\d+),(\d+)\}', group)]
        assert len(rows) == count and len(set(rows)) == count
        assert all(walkable(district, u + offset, v) for u, v in rows for offset in range(64)), 'Compiled NPC path crosses solid terrain'
    report = ', '.join(f'{slug}:{raw} raw/{flipped} flipped tiles' for slug, raw, flipped in budgets)
    traffic_loops = len(read(ROOT / 'content/core_traffic.json')['traffic_loops']) + sum(len(meta['traffic_loops']) for meta in metadata.values())
    print(f'Native district resource source: {district_count} scenes, {len(world["portals"])} reciprocal seam pairs, {len(expansion_clients)} mainland expansion clients, {traffic_loops} swept-clear traffic loops, ferry-only Island access and {sum(counts)} fixed pedestrian routes passed; {report}. Build, gameplay duration, full-city and hardware evidence remain separate.')


if __name__ == '__main__':
    check()
