"""Original northern district layout and cardinal geometry helpers.

Read the attributed original compressed layout without changing native resources.
"""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LAYOUT_PATH = ROOT / 'content/districts/north_layout.json'
WIDTH, HEIGHT = 1024, 976
ROAD_HALF, WALK_HALF, FOOT_HALF, RAIL_HALF = 24, 32, 12, 8


def read_layout():
    raw = LAYOUT_PATH.read_bytes()
    data = json.loads(raw)
    assert data['id'] == 6 and data['dimensions'] == [WIDTH, HEIGHT]
    assert data['road_half_width'] == ROAD_HALF and data['walk_half_width'] == WALK_HALF
    assert data['footpath_half_width'] == FOOT_HALF and data['rail_half_width'] == RAIL_HALF
    return data, hashlib.sha256(raw).hexdigest()


def pairs(points, closed=False):
    assert len(points) >= 2
    for a, b in zip(points, points[1:] + points[:1] if closed else points[1:]):
        assert len(a) == len(b) == 2 and (a[0] == b[0]) != (a[1] == b[1]), ('cardinal section', a, b)
        yield a, b


def pixel_points(points, closed=False):
    for (x, y), (u, v) in pairs(points, closed):
        for step in range(abs(u-x) + abs(v-y) + 1):
            yield x + (step if u > x else -step if u < x else 0), y + (step if v > y else -step if v < y else 0)
