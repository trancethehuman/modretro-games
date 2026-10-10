"""The core (central Toronto) layout at double scale, for the art and content
generators. Same names as city_layout.py, in district-world pixels
(world2x.WORLD_W x WORLD_H; the scene split is world2x.scene_of).

Street centrelines and every authored position go through world2x's street-
aware map: asphalt keeps its width, sidewalks widen, blocks grow. Queen's
Park Crescent is rebuilt at the new scale so its corners stay at 45 degrees.
"""
import city_layout as O
import world2x as W

M = W.district_map(0)
FX, FY = M.fx, M.fy
WIDTH, HEIGHT = W.WORLD_W, W.WORLD_H
ARTERIAL = (24, W.new_walk(*O.ARTERIAL))
LOCAL = (16, W.new_walk(*O.LOCAL))
CRESCENT = (12, W.new_walk(*O.CRESCENT))
ROAD_HALF = 24
WALK_HALF = ARTERIAL[0] + ARTERIAL[1]
_CLASS = {O.ARTERIAL: ARTERIAL, O.LOCAL: LOCAL, O.CRESCENT: CRESCENT, O.LANE: (8, 0)}


def fx(x):
    return FX.map_int(x)


def fy(y):
    return FY.map_int(y)


def point(u, v):
    return fx(u), fy(v)


def rect(x, y, w, h, align=8):
    return M.rect(x, y, w, h, align)


def _inverse(knots, value):
    """Old pixels for a new pixel (for the real-world region lookup)."""
    pts = [(b, a) for a, b in knots]
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        if value <= x1:
            return y0 + (value - x0) * (y1 - y0) / max(1e-9, x1 - x0)
    return pts[-1][1]


def old_u(x):
    return _inverse(FX.knots, x)


def old_v(y):
    return _inverse(FY.knots, y)


def _street(s):
    half, walk = _CLASS[(s['half'], s['walk'])]
    if s['axis'] == 'h':
        at, a, b = fy(s['at']), fx(s['a']), fx(s['b'])
    else:
        at, a, b = fx(s['at']), fy(s['a']), fy(s['b'])
    # A street that ran to the old scene edge still reaches the new one.
    if s['a'] <= 0:
        a = 0
    if s['axis'] == 'h' and s['b'] >= O.WIDTH:
        b = WIDTH
    names = [((fx if s['axis'] == 'h' else fy)(f), (fx if s['axis'] == 'h' else fy)(t), n) for f, t, n in s['names']]
    return {'name': s['name'], 'axis': s['axis'], 'at': at, 'half': half, 'walk': walk, 'a': a, 'b': b, 'names': names}


# The crescent's straight arms are rebuilt with the diagonals below.
STREETS = [_street(s) for s in O.STREETS if s['name'] != 'QUEENS PARK CRES']
WEST_PORTS = [fy(r) for r in O.WEST_PORTS]
EAST_PORTS = [fy(r) for r in O.EAST_PORTS]
MAINLAND = [24, 24, WIDTH - 32, fy(O.MAINLAND[3])]
RIVER = [fx(O.RIVER[0]), fx(O.RIVER[1])]
ISLANDS = [list(rect(x0, y0, x1 - x0, y1 - y0)) for x0, y0, x1, y1 in O.ISLANDS]
ISLANDS = [[x, y, x + w, y + h] for x, y, w, h in ISLANDS]
_rx, _ry, _rw, _rh = rect(O.RAIL[0], O.RAIL[1], O.RAIL[2] - O.RAIL[0], 8)
RAIL = [24, _ry, RIVER[0], _ry + 16]
BRIDGES = [fy(r) for r in O.BRIDGES]

# Queen's Park Crescent at the new scale: University Ave ends at College; the
# crescent's arms leave it just north of College on 45-degree corners, run
# north either side of the park, and rejoin as Queen's Park before Bloor.
_U = fx(512) // 16 * 16                   # University's centre on a 16 px grid: the park is symmetric on tiles
_C, _H, _B = fy(288), fy(176), fy(64)
_ARM = 88                                  # arms either side of University's centre
_AW, _AE = _U - _ARM, _U + _ARM
_south = _C - 24 - 8                       # where the corners leave University
_arm_bot = _south - (_ARM - 16)
_north = fy(128)                           # Queen's Park begins
_arm_top = _north + (_ARM - 12)
STREETS += [
    {'name': 'QUEENS PARK CRES', 'axis': 'v', 'at': _AW, 'half': CRESCENT[0], 'walk': CRESCENT[1],
     'a': _arm_top, 'b': _arm_bot, 'names': []},
    {'name': 'QUEENS PARK CRES', 'axis': 'v', 'at': _AE, 'half': CRESCENT[0], 'walk': CRESCENT[1],
     'a': _arm_top, 'b': _arm_bot, 'names': []},
]
for s in STREETS:
    if s['name'] == 'UNIVERSITY AVE':
        s['a'] = _C                         # ends at College
    if s['name'] == 'QUEENS PARK':
        s['b'] = _north
    if s['name'] in ('HARBORD ST',):
        s['b'] = min(s['b'], _AW)
    if s['name'] == 'WELLESLEY ST':
        s['a'] = max(s['a'], _AE)
QP_DIAGONALS = [(_U - 16, _south, _AW, _arm_bot), (_U + 16, _south, _AE, _arm_bot),
                (_AW, _arm_top, _U - 12, _north), (_AE, _arm_top, _U + 12, _north)]
QP_INTERIOR = (_AW + 20, _arm_top - 8, _AE - 20 - (_AW + 20), _south - (_arm_top - 8))
QP_INTERIOR = (QP_INTERIOR[0], QP_INTERIOR[1], QP_INTERIOR[0] + QP_INTERIOR[2], QP_INTERIOR[1] + QP_INTERIOR[3])
QP_ZONE = (_AW - 96, _B + 40, _AE + 96, _C - 24)
QP_LEGISLATURE = (_U - 64, _south - 72, 128, 56)
QP_STATUE = (_U, (_arm_top + _arm_bot) // 2 - 24)
ROWS = sorted({s['at'] for s in STREETS if s['axis'] == 'h'})
COLS = sorted({s['at'] for s in STREETS if s['axis'] == 'v' and s['name'] != 'QUEENS PARK CRES'})
U_ANCHORS, W_ANCHORS = O.U_ANCHORS, O.W_ANCHORS


def diagonal_band(u, v, x0, y0, x1, y1):
    return O.diagonal_band(u, v, x0, y0, x1, y1)


def crescent(u, v, walk=False):
    half = CRESCENT[0] + (CRESCENT[1] * 1.42 if walk else 0)
    return any(diagonal_band(u, v, *seg) <= half for seg in QP_DIAGONALS)


def _rects(walk):
    out = []
    for s in STREETS:
        half = s['half'] + (s['walk'] if walk else 0)
        if s['axis'] == 'h':
            out.append((s['a'], s['at'] - half, s['b'], s['at'] + half))
        else:
            out.append((s['at'] - half, s['a'], s['at'] + half, s['b']))
    return out


_ROAD_RECTS, _WALK_RECTS = _rects(False), _rects(True)


def _in(rects, u, v):
    return any(x0 <= u < x1 and y0 <= v < y1 for x0, y0, x1, y1 in rects)


def in_river(u, v):
    return RIVER[0] <= u < RIVER[1] and 24 <= v < MAINLAND[3]


def road(u, v, half=ROAD_HALF):
    """Asphalt (or, with half=WALK_HALF, asphalt or sidewalk) at (u,v)."""
    if not (0 <= u < WIDTH and 0 <= v < HEIGHT):
        return False
    walk = half == WALK_HALF
    if not _in(_WALK_RECTS if walk else _ROAD_RECTS, u, v):
        return False
    edge = WALK_HALF if walk else ROAD_HALF
    if u < 24 and not any(abs(v - r) < edge for r in WEST_PORTS):
        return False
    if u >= WIDTH - 24 and not any(abs(v - r) < edge for r in EAST_PORTS):
        return False
    if in_river(u, v) and not any(abs(v - r) < edge for r in BRIDGES):
        return False
    return True


def walkable(u, v):
    return road(u, v, WALK_HALF) or any(x <= u <= r and y <= v <= b for x, y, r, b in ISLANDS)


def street_spans():
    out = [(min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1), 'QUEENS PARK CRES') for x0, y0, x1, y1 in QP_DIAGONALS]
    for s in STREETS:
        cuts = sorted({s['a'], s['b']} | {p for f, t, _ in s['names'] for p in (f, t)})
        for lo, hi in zip(cuts, cuts[1:]):
            if hi <= s['a'] or lo >= s['b']:
                continue
            label = next((n for f, t, n in s['names'] if f <= lo and hi <= t), s['name'])
            out.append((lo, s['at'], hi, s['at'], label) if s['axis'] == 'h' else (s['at'], lo, s['at'], hi, label))
    return out


def _stop(u, v, name, transit, sign):
    """A stop keeps its place: on a street it stays in the same lane, a
    sign on the sidewalk moves out with the wider sidewalk."""
    return (fx(u), fy(v), name, transit, None if sign is None else (fx(sign[0]), fy(sign[1])))


CORE_STOPS = [_stop(*s) for s in O.CORE_STOPS]
