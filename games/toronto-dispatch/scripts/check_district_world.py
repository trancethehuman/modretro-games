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
import create_district_world
import create_world_routes

SOURCE_COLORS = {tuple(bytes.fromhex(value)) for value in ('071821', '306850', '86c06c', 'e0f8cf')}


def read(path):
    return json.loads(path.read_text())


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
        original_attrs = read(ROOT / 'project/original-art' / f'{slug}_attributes.json')
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
            meta = read(ROOT / 'content/districts' / f'{slug}_art.json')
            metadata[district['id']] = meta
            assert meta['dimensions'] == [1024, 976] and meta['tile_dimensions'] == [128, 122]
            assert meta['collisions'] == grid, f'{slug}: registered collision differs from original metadata'
            assert meta['background_filename'] == filename
            assert meta['background_sha256'] == hashlib.sha256(background_path.read_bytes()).hexdigest()
            assert meta['validation']['raw_unique_tiles'] == len(raw_patterns)
            assert meta['validation']['flip_canonical_unique_tiles'] == len(flipped_patterns)
            assert meta['road_half_width'] == 24 and meta['walk_half_width'] == 32
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
        width, height, grid, _ = resources[district]
        return grid[y * width + x] if 0 <= x < width and 0 <= y < height else 15

    def footprint(district, u, v, half=5):
        # Exact native half5 bounds and every overlapped tile, not four corners.
        if u < 8 or v < 8 or u > 1016 or v > 968:
            return False
        return all(tile(district, x, y) == 0
                   for y in range((v - half) // 8, (v + half) // 8 + 1)
                   for x in range((u - half) // 8, (u + half) // 8 + 1))

    def walkable(district, u, v):
        return not (tile(district, u // 8, v // 8) & 15)

    def cardinal_points(points, closed=False):
        segments = zip(points, points[1:] + points[:1] if closed else points[1:])
        for a, b in segments:
            assert (a[0] == b[0]) != (a[1] == b[1]), f'Non-cardinal or zero segment: {a}->{b}'
            distance = abs(b[0] - a[0]) + abs(b[1] - a[1])
            for step in range(distance + 1):
                yield a[0] + (step if b[0] > a[0] else -step if b[0] < a[0] else 0), a[1] + (step if b[1] > a[1] else -step if b[1] < a[1] else 0)

    assert len(world['portals']) >= 11
    directed, unordered = set(), set()
    for portal in world['portals']:
        first, last = portal['from'], portal['to']
        assert first['district'] != last['district'] and set(portal['access']) <= {'foot', 'vehicle'}
        assert 'foot' in portal['access']
        assert first['u'] in (24, 1000) and last['u'] == 1024 - first['u']
        assert isinstance(portal['modeled_crossing_pixels'], int) and portal['modeled_crossing_pixels'] > 0
        endpoints = tuple(sorted((tuple(first[k] for k in ('district', 'u', 'v')), tuple(last[k] for k in ('district', 'u', 'v')))))
        assert endpoints not in unordered, 'Duplicate seam pair'
        unordered.add(endpoints)
        vehicle = int('vehicle' in portal['access'])
        for origin, destination in ((first, last), (last, first)):
            directed.add((origin['district'], origin['u'], origin['v'], destination['district'], destination['u'], destination['v'], vehicle))
            # Validate every accepted native lateral offset, and the whole
            # pre-transition approach. Arrival side preserves the offset.
            # Walking queues at the inset trigger, before the outer fence;
            # a half-pixel step cannot visit x8/1016 after crossing x24/1000.
            # Four pixels of overshoot is also conservative for road motion.
            foot_us = range(20, 29) if origin['u'] == 24 else range(996, 1005)
            for u in foot_us:
                for offset in range(-28, 29):
                    assert walkable(origin['district'], u, origin['v'] + offset), f"Blocked foot seam lane: {portal['name']}"
            if vehicle:
                car_us = range(8, 25) if origin['u'] == 24 else range(1000, 1017)
                for u in car_us:
                    for offset in range(-18, 19):
                        assert footprint(origin['district'], u, origin['v'] + offset), f"Blocked native car seam lane: {portal['name']}"
            if vehicle:
                for offset in (-12, 0, 12):
                    assert footprint(origin['district'], origin['u'], origin['v'] + offset, 8)
            else:
                assert not footprint(origin['district'], origin['u'], origin['v']), 'Foot-only seam admits a car'
    for district, meta in metadata.items():
        expected = {(district, p['x'], p['y'], p['target'], p['target_x'], p['target_y'], int(not p['foot_only'])) for p in meta['ports']}
        assert expected == {p for p in directed if p[0] == district}, f'{district}: native seams differ from original ports'
        for route in meta['roads']:
            assert route['name'] and len(route['points']) >= 2
            assert all(footprint(district, u, v, 8) for u, v in cardinal_points(route['points'])), f"Blocked researched road: {route['name']}"
        for route in meta['footpaths']:
            points = list(cardinal_points(route['points']))
            assert all(walkable(district, u, v) for u, v in points), f"Blocked footpath: {route['name']}"
            assert any(tile(district, u // 8, v // 8) == 16 for u, v in points), f"Footpath lacks foot-only terrain: {route['name']}"
        assert len(meta['traffic_loops']) == 6
        for loop in meta['traffic_loops']:
            assert 4 <= len(loop) <= 16
            assert all(footprint(district, u, v, 8) for u, v in cardinal_points(loop, closed=True)), f'{district}: traffic swept footprint blocked'

    # Shortest paths use the registered grids, with reciprocal scene edges.
    model = RouteModel(world, campaign['stops'])
    origin = point(0, campaign['stops'][0]['u'], campaign['stops'][0]['v'])
    for stop in campaign['stops'][27:]:
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
    for quest in campaign['quests'][72:]:
        for first, last in zip(quest['route'], quest['route'][1:]):
            model.stop_leg(campaign['stops'][first], campaign['stops'][last])
        assert quest['timing_design']['planning_only'] and quest['timing_design']['measured_duration_seconds'] is None

    assert create_district_world.HEADER.read_text() == create_district_world.source(), 'Compiled reciprocal seams/traffic differ'
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
    print(f'Native district resources: {district_count} scenes, {len(world["portals"])} reciprocal seam pairs, {len(campaign["stops"])-27} expansion clients, {6*(district_count-1)} swept-clear traffic loops and {sum(counts)} fixed pedestrian routes passed; {report}. Build, gameplay duration, full-city and hardware evidence remain separate.')


if __name__ == '__main__':
    check()
