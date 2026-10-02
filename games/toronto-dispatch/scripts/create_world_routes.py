"""Derive original fixed sidewalk routes from the registered native collision grid."""
import json
import sys
from pathlib import Path
from check_campaign import decode
from city_layout import ROAD_HALF, ROWS

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "project/plugins/toronto-driving/engine/include/td_world_routes.h"


def source():
    scene = json.loads((ROOT / "project/project/scenes/toronto_city/scene.gbsres").read_text())
    grid = decode(scene["collisions"])
    width, height = scene["width"] * 8, scene["height"] * 8

    def free(x, y):
        return 0 <= x < width and 0 <= y < height and not grid[(y // 8) * scene["width"] + x // 8] & 15

    routes = []
    sidewalk_centre = ROAD_HALF + 4
    for y in [row + side for row in ROWS for side in (-sidewalk_centre, sidewalk_centre)] + [920, 936, 912]:
        for centre in [128, 272, 408, 552, 688, 816, 952]:
            start, end = centre - 32, centre + 31
            if all(free(x, y) for x in range(start, end + 1)):
                routes.append((start, y))
    assert 24 <= len(routes) < 255
    core_routes = routes
    district_routes = [core_routes]
    for slug in ('west', 'high_park'):
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
                    for start in range((left + 7) // 8 * 8, right - 62, 64):
                        if all(free(x, y) for x in range(start, start + 64)):
                            routes.add((start, y))
        routes = sorted(routes, key=lambda p: (p[1], p[0]))
        assert len(routes) >= 24, f'{slug}: insufficient usable sidewalk routes'
        # Spread a bounded identity pool across the whole district, north to south.
        if len(routes) > 128:
            routes = [routes[i * len(routes) // 128] for i in range(128)]
        district_routes.append(routes)
    # Each route is63 pixels long; per-district identities fit a byte.
    return ("/* Generated from native collision by create_world_routes.py. Original route placement. */\n"
            "#ifndef TD_WORLD_ROUTES_H\n#define TD_WORLD_ROUTES_H\n"
            f"#define TD_PEDESTRIAN_ROUTES {len(core_routes)}\n"
            "#ifdef TD_WORLD_ROUTE_DATA\n"
            "static const UBYTE td_route_counts[3]={" + ','.join(str(len(r)) for r in district_routes) + "};\n"
            "static const UWORD td_district_routes[3][128][2]={\n" +
            ''.join('  {\n' + ''.join(f'    {{{x},{y}}},\n' for x,y in routes) + '  },\n' for routes in district_routes) +
            "};\n#endif\n#endif\n")


if __name__ == "__main__":
    generated = source()
    if "--check" in sys.argv:
        assert HEADER.read_text() == generated, "Pedestrian routes differ from registered collision; regenerate them"
        print("Fixed pedestrian routes match the native collision grid")
    else:
        HEADER.write_text(generated)
        print("Generated fixed, collision-validated pedestrian routes")
