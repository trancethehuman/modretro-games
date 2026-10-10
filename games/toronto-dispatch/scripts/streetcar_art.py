"""Original seven-pixel curb signs; no TTC logo or imported imagery."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def paint_streetcar_stops(draw, district, colors, mapper=None):
    """Signs for the platforms of one old district (content/streetcar.json,
    1x layout); mapper places them at the new scale."""
    data = json.loads((ROOT / 'content/streetcar.json').read_text())
    for stop in data['stops']:
        plan = stop.get('plan', stop)
        if plan['district'] != district:
            continue
        u, v = plan['u'], plan['v']
        if mapper:
            u, v = mapper(u, v)
        draw.rectangle((u - 3, v - 3, u + 3, v + 3), fill=colors[0])
        draw.rectangle((u - 2, v - 2, u + 2, v + 2), fill=colors[3])
        draw.line((u - 1, v - 1, u + 1, v - 1), fill=colors[0])
        draw.line((u, v - 1, u, v + 1), fill=colors[0])
