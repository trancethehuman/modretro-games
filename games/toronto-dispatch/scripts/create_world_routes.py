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
    # Each route is 63 pixels long; the triangle clock needs only a seven-bit mask.
    return ("/* Generated from native collision by create_world_routes.py. Original route placement. */\n"
            "#ifndef TD_WORLD_ROUTES_H\n#define TD_WORLD_ROUTES_H\n"
            f"#define TD_PEDESTRIAN_ROUTES {len(routes)}\n"
            "static const UWORD td_sidewalk_routes[TD_PEDESTRIAN_ROUTES][2]={\n" +
            "".join(f"    {{{x},{y}}},\n" for x, y in routes) + "};\n#endif\n")


if __name__ == "__main__":
    generated = source()
    if "--check" in sys.argv:
        assert HEADER.read_text() == generated, "Pedestrian routes differ from registered collision; regenerate them"
        print("Fixed pedestrian routes match the native collision grid")
    else:
        HEADER.write_text(generated)
        print("Generated fixed, collision-validated pedestrian routes")
