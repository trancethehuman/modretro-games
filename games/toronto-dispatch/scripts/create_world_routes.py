"""Derive original fixed sidewalk routes from the registered native collision grid."""
import json
import sys
from pathlib import Path
from check_campaign import decode
from city_layout import ROAD_HALF, STREETS

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "project/plugins/toronto-driving/engine/include/td_world_routes.h"
# Per-district identity pool (the runtime keeps one bit per identity).
ROUTE_IDS = 160


def source():
    scene = json.loads((ROOT / "project/project/scenes/toronto_city/scene.gbsres").read_text())
    grid = decode(scene["collisions"])
    width, height = scene["width"] * 8, scene["height"] * 8

    def free(x, y):
        return 0 <= x < width and 0 <= y < height and not grid[(y // 8) * scene["width"] + x // 8] & 15

    routes = []
    sidewalk_centre = ROAD_HALF + 4
    # Core: both sidewalks of every east-west street, paced 48 pixels apart,
    # then the Island paths.
    lines = [(s['at'] + side * (s['half'] + 4), s['a'], s['b']) for s in STREETS
             if s['axis'] == 'h' and s['walk'] for side in (-1, 1)]
    lines += [(920, 336, 600), (936, 336, 600), (912, 640, 784), (912, 800, 928)]
    for y, left, right in sorted(lines):
        for start in range((left + 7) // 8 * 8 + 8, right - 64, 48):
            if all(free(x, y) for x in range(start, start + 64)) and (start, y) not in routes:
                routes.append((start, y))
    if len(routes) > ROUTE_IDS:
        routes = [routes[i * len(routes) // ROUTE_IDS] for i in range(ROUTE_IDS)]
    assert 24 <= len(routes) <= ROUTE_IDS
    core_routes = routes
    district_routes = [core_routes]
    world = json.loads((ROOT / 'content/districts/world.json').read_text())
    assert [d['id'] for d in world['districts']] == list(range(len(world['districts'])))
    for district in world['districts'][1:]:
        assert district['scene'].startswith('toronto_')
        slug = district['scene'].removeprefix('toronto_')
        metadata = json.loads((ROOT / f'content/districts/{slug}_art.json').read_text())
        scene = json.loads((ROOT / f'project/project/scenes/toronto_{slug}/scene.gbsres').read_text())
        grid = decode(scene['collisions'])
        routes = set()
        for route in metadata['roads'] + metadata['footpaths']:
            points = route['points']
            for a, b in zip(points, points[1:]):
                if a[1] != b[1]:
                    continue
                left, right = sorted((a[0], b[0]))
                offsets = (-sidewalk_centre, sidewalk_centre) if route in metadata['roads'] else (0,)
                for offset in offsets:
                    y = a[1] + offset
                    # Overlapping paces 48 pixels apart for busier pavements.
                    for start in range((left + 7) // 8 * 8, right - 62, 48):
                        if all(free(x, y) for x in range(start, start + 64)):
                            routes.add((start, y))
        routes = sorted(routes, key=lambda p: (p[1], p[0]))
        assert len(routes) >= 24, f'{slug}: insufficient usable sidewalk routes'
        # Spread a bounded identity pool across the whole district, north to south.
        if len(routes) > ROUTE_IDS:
            routes = [routes[i * len(routes) // ROUTE_IDS] for i in range(ROUTE_IDS)]
        district_routes.append(routes)
    # Runtime refresh index: identities ordered by y, and for each 32-pixel band
    # the first ordered position at or below that band. A window scan then visits
    # only nearby rows; identities themselves keep their authored order.
    orders, bands = [], []
    for routes in district_routes:
        assert all(y < 1024 for _, y in routes)
        order = sorted(range(len(routes)), key=lambda i: (routes[i][1], i))
        orders.append(order)
        bands.append([next((k for k, i in enumerate(order) if routes[i][1] >= band * 32), len(order))
                      for band in range(33)])
    # Each route is63 pixels long; per-district identities fit a byte.
    return ("/* Generated from native collision by create_world_routes.py. Original route placement. */\n"
            "#ifndef TD_WORLD_ROUTES_H\n#define TD_WORLD_ROUTES_H\n"
            f"#define TD_PEDESTRIAN_ROUTES {len(core_routes)}\n"
            f"#define TD_ROUTE_IDS {ROUTE_IDS}\n"
            "#ifdef TD_WORLD_ROUTE_DATA\n"
            f"static const UBYTE td_route_counts[{len(district_routes)}]={{" + ','.join(str(len(r)) for r in district_routes) + "};\n"
            f"static const UWORD td_district_routes[{len(district_routes)}][{ROUTE_IDS}][2]={{\n" +
            ''.join('  {\n' + ''.join(f'    {{{x},{y}}},\n' for x,y in routes) + '  },\n' for routes in district_routes) +
            "};\n"
            f"static const UBYTE td_route_order[{len(district_routes)}][{ROUTE_IDS}]={{\n" +
            ''.join('  {' + ','.join(map(str, order)) + '},\n' for order in orders) +
            "};\n"
            f"static const UBYTE td_route_band[{len(district_routes)}][33]={{\n" +
            ''.join('  {' + ','.join(map(str, band)) + '},\n' for band in bands) +
            "};\n#endif\n#endif\n")


if __name__ == "__main__":
    generated = source()
    if "--check" in sys.argv:
        assert HEADER.read_text() == generated, "Pedestrian routes differ from registered collision; regenerate them"
        print("Fixed pedestrian routes match the native collision grid")
    else:
        HEADER.write_text(generated)
        print("Generated fixed, collision-validated pedestrian routes")
