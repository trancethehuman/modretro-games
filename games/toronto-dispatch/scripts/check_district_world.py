"""Validate the registered sixteen-scene world and its compiled data.

This is source/resource validation, not emulator, hardware, full-city coverage,
performance or measured two-hour campaign evidence. No files are regenerated.
"""
import json
import re
from pathlib import Path
from PIL import Image
from check_campaign import ROOT, decode, TOTAL_STOPS, TOTAL_QUESTS
from create_district_jobs import RouteModel, point
import city_kit
import create_district_world
import create_world_routes
import world2x

SOURCE_COLORS = {tuple(bytes.fromhex(value)) for value in ('071821', '306850', '86c06c', 'e0f8cf')}


def read(path):
    return json.loads(path.read_text())


def check():
    world = read(ROOT / 'content/districts/world.json')
    campaign = read(ROOT / 'content/campaign.json')
    settings = read(ROOT / 'project/project/settings.gbsres')
    district_count = int(re.search(r'#define TD_DISTRICT_COUNT (\d+)', (ROOT / 'project/plugins/toronto-driving/engine/include/td_district.h').read_text()).group(1))
    assert district_count == len(world['districts']) == 16 and [d['id'] for d in world['districts']] == list(range(16))
    assert len(campaign['stops']) == TOTAL_STOPS and len(campaign['quests']) == TOTAL_QUESTS
    assert settings['colorMode'] == 'color', '384-tile budget requires color-only project'
    flips = settings.get('autoTileFlipEnabled', False)
    resources, budgets = {}, []
    for district in world['districts']:
        slug = district['scene']
        assert slug == world2x.scene_slug(district['id'])
        scene = read(ROOT / 'project/project/scenes' / slug / 'scene.gbsres')
        background_path = ROOT / 'project/assets/backgrounds' / f'{slug}.png'
        bg = read(background_path.with_suffix('.png.gbsres'))
        attrs, grid = decode(bg['tileColors']), decode(scene['collisions'])
        width, height = scene['width'], scene['height']
        assert scene['type'] == 'TORONTO' and scene['symbol'] == district['symbol']
        assert (width, height) == (128, 122)
        assert len(grid) == len(attrs) == 15616 < 16384, 'Native tile map must fit its bank'
        assert bg['id'] == scene['backgroundId'] and bg['filename'] == f'{slug}.png'
        assert (bg['width'], bg['height'], bg['imageWidth'], bg['imageHeight']) == (128, 122, 1024, 976)
        assert bg['autoColor'] is False, 'Manual source palette expected'
        assert len(scene['paletteIds']) == 7
        assert attrs == read(ROOT / 'project/original-art' / f'{slug}_attributes.json'), f'{slug}: registered palette/priority bytes differ from source'
        assert grid == world2x.scene_grid(district['id']), f'{slug}: registered collisions differ from the art'
        assert set(grid) <= {0, 16, 15}, f'{slug}: unexpected native collision flags'
        assert all((a & 7) <= 6 and not (a & 0x78) for a in attrs), f'{slug}: palette/attribute flags out of authored range'
        assert all(not (a & 128) for a, c in zip(attrs, grid) if c == 0), f'{slug}: raised roof/canopy priority covers asphalt'
        assert any(a & 128 for a in attrs), f'{slug}: missing actual native roof/canopy priority'
        with Image.open(background_path) as opened:
            img = opened.convert('RGB')
        assert img.size == (1024, 976)
        pixels = img.tobytes()
        assert {tuple(pixels[i:i + 3]) for i in range(0, len(pixels), 3)} <= SOURCE_COLORS, f'{slug}: non-native source shade'
        raw = len({img.crop((tx * 8, ty * 8, tx * 8 + 8, ty * 8 + 8)).tobytes() for ty in range(height) for tx in range(width)})
        flipped = city_kit.flip_canonical_count(img)
        consumed = flipped if flips and scene.get('autoTileFlipEnabled', True) else raw
        # Within this budget GB Studio leaves 144 tiles per bank for sprites.
        assert consumed <= city_kit.SCENE_TILE_BUDGET, f'{slug}: background patterns exceed the scene budget ({consumed})'
        budgets.append((slug, raw, flipped))
        resources[district['id']] = grid

    def tile(district, x, y):
        grid = resources[district]
        return grid[y * 128 + x] if 0 <= x < 128 and 0 <= y < 122 else 15

    def footprint(district, u, v, half=5):
        # Exact native bounds and every overlapped tile, not four corners.
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

    # Building footprints are solid on every district's stitched grid.
    for old in range(4):
        grid = world2x.world_grid(old)
        name = 'content/city_art.json' if old == 0 else f'content/districts/{world2x.OLD_NAMES[old]}_art.json'
        for block in read(ROOT / name)['blocks']:
            left, top = block['x'] // 8, block['y'] // 8
            right, bottom = (block['x'] + block['width'] - 1) // 8, (block['y'] + block['depth'] - 1) // 8
            assert all(grid[y * world2x.WORLD_TW + x] == 15 for y in range(top, bottom + 1) for x in range(left, right + 1)), \
                f'{name}: building footprint is passable {block}'
        if old:
            meta = read(ROOT / name)
            assert meta['dimensions'] == [world2x.WORLD_W, world2x.WORLD_H]
            assert (ROOT / meta['source_research']).exists()

            def world_footprint(x, y, half):
                return all(grid[ty * world2x.WORLD_TW + tx] == 0 for ty in range((y - half) // 8, (y + half) // 8 + 1)
                           for tx in range((x - half) // 8, (x + half) // 8 + 1))
            for route in meta['world']['roads']:
                assert all(world_footprint(x, y, 8) for x, y in cardinal_points(route['points'])), f"Blocked researched road: {route['name']}"
            for route in meta['world']['footpaths']:
                pts = list(cardinal_points(route['points']))
                assert all(not grid[(y // 8) * world2x.WORLD_TW + x // 8] & 15 for x, y in pts), f"Blocked footpath: {route['name']}"
                assert any(grid[(y // 8) * world2x.WORLD_TW + x // 8] == 16 for x, y in pts), f"Footpath lacks foot-only terrain: {route['name']}"

    directed, unordered = set(), set()
    outer = 0
    for portal in world['portals']:
        first, last = portal['from'], portal['to']
        assert first['district'] != last['district'] and set(portal['access']) <= {'foot', 'vehicle'} and 'foot' in portal['access']
        horizontal = first['u'] in (24, 1000)
        if horizontal:
            assert last['u'] == 1024 - first['u']
        else:
            assert first['v'] in (24, 952) and last['v'] == 976 - first['v']
        assert isinstance(portal['modeled_crossing_pixels'], int) and portal['modeled_crossing_pixels'] > 0
        endpoints = tuple(sorted((tuple(first[k] for k in ('district', 'u', 'v')), tuple(last[k] for k in ('district', 'u', 'v')))))
        assert endpoints not in unordered, 'Duplicate seam pair'
        unordered.add(endpoints)
        vehicle = int('vehicle' in portal['access'])
        inner = first['district'] >> 2 == last['district'] >> 2
        outer += not inner
        for origin, destination in ((first, last), (last, first)):
            directed.add((origin['district'], origin['u'], origin['v'], destination['district'], destination['u'], destination['v'], vehicle))
            assert walkable(origin['district'], origin['u'], origin['v']), f"Blocked seam: {portal['name']}"
            if inner:
                if vehicle:
                    assert footprint(origin['district'], origin['u'], origin['v'], 8), f"Blocked inner car seam: {portal['name']}"
                continue
            # District seams: every accepted lateral offset of the approach.
            foot_us = range(20, 29) if origin['u'] == 24 else range(996, 1005)
            for u in foot_us:
                for offset in range(-36 if vehicle else -8, 37 if vehicle else 9):
                    assert walkable(origin['district'], u, origin['v'] + offset), f"Blocked foot seam lane: {portal['name']} {offset}"
            if vehicle:
                car_us = range(8, 25) if origin['u'] == 24 else range(1000, 1017)
                for u in car_us:
                    for offset in range(-18, 19):
                        assert footprint(origin['district'], u, origin['v'] + offset), f"Blocked native car seam lane: {portal['name']}"
            else:
                assert not footprint(origin['district'], origin['u'], origin['v']), 'Foot-only seam admits a car'
    assert outer >= 11
    for entry in world['traffic']:
        for loop in entry['loops']:
            assert 4 <= len(loop) <= 8
            assert all(footprint(entry['district'], u, v, 7) for u, v in cardinal_points(loop, closed=True)), \
                f"{entry['district']}: traffic swept footprint blocked"

    # Shortest paths use the registered grids, with reciprocal scene edges.
    model = RouteModel(world, campaign['stops'])
    first = campaign['stops'][0]
    origin = point(first['district'], first['u'], first['v'])
    # The sixteen authored expansion clients need car/park-hand-off access.
    # Queen streetcar platforms are checked by check_streetcar.py.
    for stop in campaign['stops'][27:43]:
        district, u, v = stop['district'], stop['u'], stop['v']
        foot_only = bool(stop.get('foot_only', False))
        assert district in resources and district >> 2 > 0 and stop['transit'] == 0
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
        for a, b in zip(quest['route'], quest['route'][1:]):
            model.stop_leg(campaign['stops'][a], campaign['stops'][b])
        assert quest['timing_design']['planning_only'] and quest['timing_design']['measured_duration_seconds'] is None

    header, generated_world = create_district_world.build()
    assert create_district_world.HEADER.read_text() == header, 'Compiled seams/traffic differ'
    ped_header = create_world_routes.HEADER.read_text()
    assert ped_header == create_world_routes.source(), 'Compiled pedestrian routes differ from registered grids'
    counts = list(map(int, re.search(rf'td_route_counts\[{district_count}\]=\{{([\d,]+)\}}', ped_header).group(1).split(',')))
    ids = create_world_routes.ROUTE_IDS
    ped_table = re.search(rf'td_district_routes\[{district_count}\]\[{ids}\]\[2\]=\{{(.*?)\n\}};', ped_header, re.S).group(1)
    groups = re.findall(r'^  \{\n(.*?)^  \},', ped_table, re.S | re.M)
    assert len(groups) == district_count and all(8 <= count <= ids for count in counts)
    for district, (group, count) in enumerate(zip(groups, counts)):
        rows = [tuple(map(int, pair)) for pair in re.findall(r'\{(\d+),(\d+)\}', group)]
        assert len(rows) == count and len(set(rows)) == count
        assert all(walkable(district, u + offset, v) for u, v in rows for offset in range(64)), 'Compiled NPC path crosses solid terrain'
    report = ', '.join(f'{slug}:{flipped}' for slug, raw, flipped in budgets)
    print(f'Native district resources: {district_count} scenes, {len(world["portals"])} seam pairs ({outer} between districts), '
          f'16 expansion clients, {6 * district_count} swept-clear traffic loops and {sum(counts)} fixed pedestrian routes passed; '
          f'flip-canonical tiles {report}. Build, gameplay duration, full-city and hardware evidence remain separate.')


if __name__ == '__main__':
    check()
