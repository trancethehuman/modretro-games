"""Original Queen rails and seven-pixel curb signs; no imported imagery.

Decorative pixels follow the original gameplay paths, without changing road
collision or background priority. They are compressed game design, not surveyed
track geometry. Native geometry tests independently check the full moving body.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def paint_streetcar_rails(draw, district, colors):
    paths = {
        0: [[[0, 536], [1023, 536]], [[1023, 520], [0, 520]]],
        1: [[[836, 536], [1023, 536]], [[1023, 520], [836, 520], [836, 536]]],
        3: [[[0, 536], [680, 536], [680, 504], [880, 504], [880, 488]],
            [[880, 488], [664, 488], [664, 520], [0, 520]]],
    }
    for points in paths.get(district, []):
        normals = []
        for a, b in zip(points, points[1:]):
            dx, dy = b[0] - a[0], b[1] - a[1]
            assert bool(dx) != bool(dy), 'Rail paths must stay cardinal'
            normals.append((0 if not dy else (1 if dy > 0 else -1),
                            0 if not dx else (-1 if dx > 0 else 1)))
        for side in (-4, 4):
            shifted = []
            for i, (u, v) in enumerate(points):
                before = normals[max(0, i - 1)]
                after = normals[min(i, len(normals) - 1)]
                n = before if before == after else (before[0] + after[0], before[1] + after[1])
                shifted.append((u + n[0] * side, v + n[1] * side))
            draw.line(shifted, fill=colors[0], width=1)


def paint_streetcar_stops(draw, district, colors):
    paint_streetcar_rails(draw, district, colors)
    data = json.loads((ROOT / 'content/streetcar.json').read_text())
    for stop in data['stops']:
        if stop['district'] != district:
            continue
        u, v = stop['u'], stop['v']
        draw.rectangle((u - 3, v - 3, u + 3, v + 3), fill=colors[0])
        draw.rectangle((u - 2, v - 2, u + 2, v + 2), fill=colors[3])
        draw.line((u - 1, v - 1, u + 1, v - 1), fill=colors[0])
        draw.line((u, v - 1, u, v + 1), fill=colors[0])
