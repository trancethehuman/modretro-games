"""Derive original fixed sidewalk routes from each scene's native collision grid."""
import json
import sys
from pathlib import Path
import core2x
import world2x

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "project/plugins/toronto-driving/engine/include/td_world_routes.h"
# Per-scene identity pool (the runtime keeps one bit per identity).
ROUTE_IDS = 128


def district_lines(old):
    """Walking lines (y, x_left, x_right) of a district in world pixels: the
    middle of both sidewalks of every east-west street, and footpaths."""
    if old == 0:
        lines = [(s['at'] + side * (s['half'] + s['walk'] // 2), s['a'], s['b']) for s in core2x.STREETS
                 if s['axis'] == 'h' and s['walk'] for side in (-1, 1)]
        # The Islands' paths (plan pixels).
        m = world2x.district_map(0)
        for y, a, b in ((920, 336, 600), (936, 336, 600), (912, 640, 784), (912, 800, 928)):
            lines.append((m.fy.map_int(y), m.fx.map_int(a), m.fx.map_int(b)))
        return lines
    metadata = json.loads((ROOT / f'content/districts/{world2x.OLD_NAMES[old]}_art.json').read_text())
    walk = metadata['road_half_width'] + (metadata['walk_half_width'] - metadata['road_half_width']) // 2
    lines = []
    for kind, offsets in (('roads', (-walk, walk)), ('footpaths', (0,))):
        for route in metadata['world'][kind]:
            for a, b in zip(route['points'], route['points'][1:]):
                if a[1] == b[1]:
                    lines += [(a[1] + off, min(a[0], b[0]), max(a[0], b[0])) for off in offsets]
    return lines


def source():
    district_routes = []
    for district in range(16):
        grid = world2x.scene_grid(district)
        ox, oy = world2x.scene_origin(district)

        def free(x, y):
            return 8 <= x < 1016 and 8 <= y < 968 and not grid[(y // 8) * 128 + x // 8] & 15
        routes = set()
        for y, left, right in district_lines(district >> 2):
            y -= oy
            # Overlapping paces 48 pixels apart for busier pavements.
            for start in range(max(left - ox, 8) // 8 * 8 + 8, min(right - ox, 1016) - 64, 48):
                if all(free(x, y) for x in range(start, start + 64)):
                    routes.add((start, y))
        routes = sorted(routes, key=lambda p: (p[1], p[0]))
        assert len(routes) >= 8, f'{world2x.scene_slug(district)}: insufficient usable sidewalk routes'
        # Spread a bounded identity pool across the whole scene, north to south.
        if len(routes) > ROUTE_IDS:
            routes = [routes[i * len(routes) // ROUTE_IDS] for i in range(ROUTE_IDS)]
        district_routes.append(routes)
    core_routes = district_routes[0]
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
