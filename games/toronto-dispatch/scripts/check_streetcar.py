"""Check original Queen platforms against registered native ground and routes.

This checks authored geography/compression and walking access, not moving
streetcar artwork, human route enjoyment, runtime scene loading or hardware.
"""
import json
from pathlib import Path

from check_campaign import TOTAL_STOPS, decode
from create_district_jobs import RouteModel, point

ROOT = Path(__file__).resolve().parents[1]


def check():
    authored = json.loads((ROOT / 'content/streetcar.json').read_text())
    campaign = json.loads((ROOT / 'content/campaign.json').read_text())
    world = json.loads((ROOT / 'content/districts/world.json').read_text())
    stops = authored['stops']
    assert [stop['id'] for stop in stops] == list(range(43, 51))
    assert stops == campaign['stops'][43:51] and len(campaign['stops']) == TOTAL_STOPS
    service = authored['service']
    assert service['stops'] == list(range(43, 51))
    assert service['fare'] == 3 and service['period_seconds'] == 64
    assert service['boarding_window_seconds'] == 2 and service['segment_seconds'] == 4
    assert service['eastbound_phases'] == list(range(0, 32, 4))
    assert service['westbound_phases'] == list(range(60, 28, -4))
    assert campaign['transit']['streetcar501'] == service
    assert [stop['district'] for stop in stops] == [1, 0, 0, 0, 0, 3, 3, 3]
    grids = {}
    for entry in world['districts']:
        scene = json.loads((ROOT / 'project/project/scenes' / entry['scene'] / 'scene.gbsres').read_text())
        assert (scene['width'], scene['height']) == (128, 122)
        grids[entry['id']] = decode(scene['collisions'])
    model = RouteModel(world, campaign['stops'])
    home = point(0, campaign['stops'][0]['u'], campaign['stops'][0]['v'])
    for stop in stops:
        assert stop['transit'] == 4 and stop['reserved'] == 0
        assert len(stop['name']) <= 18 and stop['name'].isascii()
        assert type(stop['u']) is int and type(stop['v']) is int
        assert 8 <= stop['u'] < 1016 and 8 <= stop['v'] < 968
        grid = grids[stop['district']]
        for dx in range(-4, 5):
            for dy in range(-3, 4):
                assert grid[((stop['v'] + dy) // 8) * 128 + (stop['u'] + dx) // 8] == 16, f"Platform clearance: {stop['id']}"
        # Walking must reach the actual sidewalk through the registered seams;
        # a platform in a disconnected block cannot become a paid dead end.
        model.shortest(home, point(stop['district'], stop['u'], stop['v']), False)
    for a in stops:
        for b in stops:
            if a['id'] != b['id'] and a['district'] == b['district']:
                assert abs(a['u'] - b['u']) >= 30 or abs(a['v'] - b['v']) >= 30, 'Overlapping native stop interaction rectangles'
    print('Queen streetcar: eight distinct sidewalk platforms, clearance and registered walking access passed; directional fictional schedule matches content. Native ride and scene tests are separate.')


if __name__ == '__main__':
    check()
