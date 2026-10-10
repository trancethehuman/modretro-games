"""Author the indoor contracts: deliveries that end (or start) at a desk
inside a building, including the CN Tower's LookOut, reached by elevator.

Each indoor stop is a native stop record whose position is the sidewalk at
the building's door (content/doors.json) and whose reserved byte carries
the foot-only flag (bit 0) and the interior's index + 1 (bits 1..6): the
courier parks at the stop's parking anchor, walks in, and hands over at
the interior's desk (td_interior.c). Writes content/districts/interior_jobs.json;
create_campaign.py appends it after the eastern contracts and the 501
platforms. `--check` verifies without writing.
"""
from pathlib import Path
import json
import math
import sys
import world2x

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'content/districts/interior_jobs.json'
FIRST_STOP, FIRST_QUEST = 51, 89
CHAPTER = 'Inside the city'
# (stop name, interior key, door's interior key, door name)
STOPS = (
    ('CN LOOKOUT DESK', 'cn_lookout', 'cn_base', 'CN TOWER'),
    ('AGO FRONT DESK', 'ago', 'ago', 'ART GALLERY (AGO)'),
    ('ROM RECEIVING', 'rom', 'rom', 'ROYAL ONT. MUSEUM'),
    ('EATON CENTRE KIOSK', 'eaton', 'eaton', 'EATON CENTRE'),
    ('DRAGON CITY STALL', 'dragon_city', 'dragon_city', 'DRAGON CITY MALL'),
    ('BYTE BARN DESK', 'electronics', 'electronics', 'BYTE BARN'),
    ('UNION GREAT HALL', 'union', 'union', 'UNION STATION'),
)
# (title, brief, kind id, min completed, route of stop ids, stage objectives)
CONTRACTS = (
    ('SKY HIGH LUNCH', ('LUNCH FOR THE TOP', 'TAKE THE ELEVATOR'), 0, 3, [1, 51],
     ['Collect the market lunch order', 'Sky Desk handoff, 346 m up']),
    ('GALLERY LOAN', ('A FRAMED LOAN', 'CARRY IT INSIDE'), 1, 4, [4, 52],
     ['Collect the framed loan', 'Gallery front desk handoff']),
    ('FOSSIL CAST', ('PLASTER FOSSIL', 'TO THE DINO HALL'), 1, 6, [7, 53],
     ['Collect the fossil cast', 'Museum receiving handoff']),
    ('MALL RESTOCK', ('UNION TO EATON', 'TWO INDOOR DESKS'), 0, 8, [57, 54],
     ['Collect kiosk stock in the great hall', 'Mall kiosk handoff']),
    ('DIM SUM ORDER', ('BAMBOO BASKETS', 'DRAGON CITY STALL'), 2, 10, [7, 55],
     ['Collect the steamer baskets', 'Dragon City stall handoff']),
    ('CONSOLE RUSH', ('LAUNCH-DAY BOX', 'BYTE BARN TO WEST'), 2, 12, [56, 8],
     ['Collect the launch-day console', 'Dufferin handoff']),
)
KINDS = ('PARCEL ROUND', 'FRAGILE ART', 'EXPRESS FILES')
CAR_SPEED = 67.5  # px/s, the courier's car (td_drive.c lf_top)


def doors():
    data = json.loads((ROOT / 'content/doors.json').read_text())
    out = {}
    for d in range(16):
        for door in data['scenes'][world2x.scene_slug(d)]:
            out[(door['interior'], door['name'])] = (d, door['u'], door['v'])
    return out


def parking(district, u, v):
    """Nearest point whose 17 x 17 car footprint is all asphalt (the
    district world check's parking rule)."""
    grid = world2x.scene_grid(district)
    tw = world2x.SCENE_W // 8
    best = None
    for pv in range(max(8, v - 200), min(968, v + 201), 2):
        for pu in range(max(8, u - 200), min(1016, u + 201), 2):
            if all(grid[y * tw + x] == 0 for y in range((pv - 8) // 8, (pv + 8) // 8 + 1)
                   for x in range((pu - 8) // 8, (pu + 8) // 8 + 1)):
                d = abs(pu - u) + abs(pv - v)
                if best is None or d < best[0]:
                    best = (d, pu, pv)
    assert best, ('no parking near', district, u, v)
    return {'u': best[1], 'v': best[2]}


def world(stop):
    ox, oy = world2x.scene_origin(stop['district'])
    return stop['district'] >> 2, ox + stop['u'], oy + stop['v']


def author():
    keys = [i['key'] for i in json.loads((ROOT / 'content/interiors.json').read_text())['interiors']]
    campaign = json.loads((ROOT / 'content/campaign.json').read_text())
    base = campaign['stops'][:FIRST_STOP]
    door = doors()
    stops = []
    for k, (name, interior, door_key, door_name) in enumerate(STOPS):
        district, u, v = door[(door_key, door_name)]
        reserved = ((keys.index(interior) + 1) << 1) | 1
        assert reserved < 256 and len(name) <= 18
        stops.append({'id': FIRST_STOP + k, 'u': u, 'v': v, 'name': name, 'transit': 0, 'district': district,
                      'reserved': reserved, 'foot_only': True, 'interior': interior, 'door': door_name,
                      'parking_anchor': parking(district, u, v),
                      'location_notice': 'Fictional delivery desk inside an original interpretation of the building'})
    every = base + stops
    quests = []
    for n, (title, brief, kind, min_done, route, objectives) in enumerate(CONTRACTS):
        points = [world(every[s]) for s in route]
        dist = sum(abs(a[1] - b[1]) + abs(a[2] - b[2]) for a, b in zip(points, points[1:]))
        inside = sum(1 for s in route if s >= FIRST_STOP)
        lift = sum(1 for s in route if s >= FIRST_STOP and every[s]['interior'] == 'cn_lookout')
        seconds = int(math.ceil((30 + dist / (CAR_SPEED * 0.55) + 45 * inside + 40 * lift) / 10) * 10)
        reward = 90 + dist // 60 + 40 * inside + (25 if kind == 1 else 15 if kind == 2 else 0)
        quests.append({'id': f'contract-{FIRST_QUEST + n:02d}', 'title': title, 'brief': list(brief), 'chapter': CHAPTER,
                       'kind': KINDS[kind], 'kind_id': kind, 'required_vehicle': 255, 'min_completed': min_done,
                       'route': route, 'time_limit_seconds': seconds, 'reward': reward,
                       'stages': [{'stop_id': s, 'district': every[s]['district'], 'objective': o} for s, o in zip(route, objectives)],
                       'objective_notice': 'Park, walk in at the door and hand over at the desk inside; the LookOut is reached by the elevator'})
    return {'status': 'engine-integrated', 'chapter': CHAPTER,
            'notice': 'Indoor desks are fictional; the buildings are original interpretations of real Toronto landmarks.',
            'stops': stops, 'quests': quests}


def main():
    text = json.dumps(author(), indent=2) + '\n'
    if '--check' in sys.argv:
        assert OUTPUT.read_text() == text, 'indoor jobs are stale; run scripts/create_interior_jobs.py'
        print(f'Indoor contracts match their doors: {len(CONTRACTS)} contracts, {len(STOPS)} desks.')
        return
    OUTPUT.write_text(text)
    print(f'Wrote {OUTPUT.relative_to(ROOT)}: {len(CONTRACTS)} contracts, {len(STOPS)} indoor desks.')


if __name__ == '__main__':
    main()
