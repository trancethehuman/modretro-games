"""Core district (central Toronto) layout in native world pixels, north up.

Street positions follow the City of Toronto Centreline (content/districts/
core-research.json): each street's real position along the downtown grid
(metres east of Yonge / north of Queen, rotated 16.6 degrees to the street
grid) is mapped to pixels by a piecewise-linear compression, so streets keep
their real order and relative spacing. Minor streets that would leave blocks
narrower than about 40 px at this scale are omitted (Jarvis, Church, Bay,
Richmond, Adelaide and others). North-south streets are snapped to 64 px
where possible so the atlas map stays inside its tile budget. Seam rows shared with the West and East scenes are unchanged.

Road classes (asphalt half-width / sidewalk): arterial 24/8, local 16/8,
lane 8/0. Everything below is original compressed game geometry.
"""
WIDTH, HEIGHT = 1024, 976
ARTERIAL, LOCAL, LANE = (24, 8), (16, 8), (8, 0)
CRESCENT = (12, 8)                     # Queen's Park Crescent: one lane each way
ROAD_HALF, WALK_HALF = 24, 32          # arterial values, kept for callers
WEST_PORTS = [64, 288, 400, 528, 640]  # Bloor, College, Dundas, Queen, King
EAST_PORTS = [64, 400, 528]            # Bloor/Danforth, Dundas and Queen
MAINLAND = [24, 24, 992, 848]          # land bounds; the harbour lies below
RIVER = [848, 888]                     # Don River channel (x range)
ISLANDS = [[336, 912, 600, 952], [640, 896, 784, 952], [800, 880, 928, 928]]
RAIL = [24, 792, 848, 800]             # Union Station rail corridor (x0,y0,x1,y1)

# Real positions (metres along the rotated downtown grid) -> pixels.
U_ANCHORS = [(-4121, 64), (-3297, 160), (-2075, 256), (-1438, 384), (-592, 512),
             (0, 640), (1247, 784), (2120, 868), (2478, 928)]
W_ANCHORS = [(2079, 64), (1550, 176), (1040, 288), (460, 400), (0, 528),
             (-382, 640), (-688, 736), (-820, 776), (-1100, 824), (-1300, 848)]


def _piecewise(anchors, value):
    pts = sorted(anchors)
    for (a, x0), (b, x1) in zip(pts, pts[1:]):
        if value <= b:
            return x0 + (value - a) * (x1 - x0) / (b - a)
    return pts[-1][1]


def map_u(metres):
    return round(_piecewise(U_ANCHORS, metres))


def map_w(metres):
    return round(_piecewise(W_ANCHORS, metres))


def street(name, axis, at, cls, a, b, names=None):
    """axis 'h': y=at from x=a..b; axis 'v': x=at from y=a..b. names: optional
    [(from, to, label)] along the street for segments with other names."""
    half, walk = cls
    return {'name': name, 'axis': axis, 'at': at, 'half': half, 'walk': walk,
            'a': a, 'b': b, 'names': names or []}


STREETS = [
    # East-west
    street('BLOOR ST', 'h', 64, ARTERIAL, 0, 1024, [(888, 1024, 'DANFORTH AVE')]),
    # Harbord (Hoskin Ave east of Spadina) and Wellesley end at Queen's Park
    # Crescent, as on the Centreline; the park lies between them.
    street('HARBORD ST', 'h', 176, LOCAL, 160, 460, [(384, 460, 'HOSKIN AVE')]),
    street('WELLESLEY ST', 'h', 176, LOCAL, 564, 784),
    street('COLLEGE ST', 'h', 288, ARTERIAL, 0, 992, [(640, 784, 'CARLTON ST'), (784, 992, 'GERRARD ST E')]),
    street('DUNDAS ST', 'h', 400, ARTERIAL, 0, 1024),
    street('QUEEN ST', 'h', 528, ARTERIAL, 0, 1024),
    street('KING ST', 'h', 640, ARTERIAL, 0, 848),
    street('FRONT ST', 'h', 736, LOCAL, 256, 848, [(784, 848, 'MILL ST')]),
    street('QUEENS QUAY', 'h', 824, LOCAL, 24, 992),
    # North-south
    street('DUFFERIN ST', 'v', 64, ARTERIAL, 24, 840),
    street('OSSINGTON AVE', 'v', 160, LOCAL, 24, 528),
    street('BATHURST ST', 'v', 256, ARTERIAL, 24, 840),
    street('SPADINA AVE', 'v', 384, ARTERIAL, 24, 840),
    street('UNIVERSITY AVE', 'v', 512, ARTERIAL, 288, 736),
    street('QUEENS PARK', 'v', 512, ARTERIAL, 24, 128),
    # Straight arms of Queen's Park Crescent; QP_DIAGONALS join them to
    # University Ave at College and to Queen's Park at the north end.
    street('QUEENS PARK CRES', 'v', 460, CRESCENT, 168, 248),
    street('QUEENS PARK CRES', 'v', 564, CRESCENT, 168, 248),
    street('YONGE ST', 'v', 640, ARTERIAL, 24, 840),
    street('PARLIAMENT ST', 'v', 784, ARTERIAL, 24, 840),
    street('BROADVIEW AVE', 'v', 928, LOCAL, 24, 528),
]
# Queen's Park Crescent around the Legislature and the park (Centreline:
# the crescents leave University Ave just north of College, Hoskin meets the
# west arm and Wellesley the east arm, and they rejoin as Queen's Park, which
# runs on to Bloor). The real oval is about 36 px wide at this compression;
# the game widens it to about 104 px so the park reads at street level.
# 45-degree centreline segments (x0, y0, x1, y1); asphalt and sidewalk keep
# CRESCENT widths measured square to the road.
QP_DIAGONALS = [(476, 264, 460, 248), (548, 264, 564, 248),   # from College
                (460, 168, 500, 128), (564, 168, 524, 128)]   # to Queen's Park
# The park inside the crescent: Legislature, lawns and the statue.
QP_INTERIOR = (480, 144, 544, 256)
QP_ZONE = (416, 96, 608, 256)          # the crescent's blocks, drawn as one design
QP_LEGISLATURE = (480, 208, 64, 32)   # spans the park between the arms, as it does
QP_STATUE = (512, 176)      # on a tile corner: the plaza and paths are flips of one design

# Don crossings: Bloor (Prince Edward Viaduct), Gerrard, Dundas, Queen and the
# waterfront (Lake Shore at the Keating Channel, folded into Queens Quay).
BRIDGES = [64, 288, 400, 528, 824]
ROWS = sorted({s['at'] for s in STREETS if s['axis'] == 'h'})
# Grid columns; the crescent's arms are not part of the street grid.
COLS = sorted({s['at'] for s in STREETS if s['axis'] == 'v' and (s['half'], s['walk']) != CRESCENT})


def _rects(walk):
    out = []
    for s in STREETS:
        half = s['half'] + (s['walk'] if walk else 0)
        a, b = s['a'], s['b']
        if s['axis'] == 'h':
            out.append((a, s['at'] - half, b, s['at'] + half))
        else:
            out.append((s['at'] - half, a, s['at'] + half, b))
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
    hit = _in(_WALK_RECTS if half == WALK_HALF else _ROAD_RECTS, u, v)
    if not hit:
        return False
    if u < 24 and not any(abs(v - r) < (32 if half == WALK_HALF else 24) for r in WEST_PORTS):
        return False
    if u >= 992 and not any(abs(v - r) < (32 if half == WALK_HALF else 24) for r in EAST_PORTS):
        return False
    if in_river(u, v) and not any(abs(v - r) < (32 if half == WALK_HALF else 24) for r in BRIDGES):
        return False
    return True


def walkable(u, v):
    return road(u, v, WALK_HALF) or any(x <= u <= r and y <= v <= b for x, y, r, b in ISLANDS)


def diagonal_band(u, v, x0, y0, x1, y1):
    """Distance to a 45-degree centreline segment (round ends join the arms)."""
    sx, sy = (1 if x1 > x0 else -1), (1 if y1 > y0 else -1)
    t = min(max(((u - x0) * sx + (v - y0) * sy) / 2, 0), abs(x1 - x0))
    return ((u - x0 - sx * t) ** 2 + (v - y0 - sy * t) ** 2) ** 0.5


def crescent(u, v, walk=False):
    """On Queen's Park Crescent's 45-degree parts (asphalt, or with walk the
    sidewalk too)."""
    half = CRESCENT[0] + (CRESCENT[1] * 1.42 if walk else 0)
    return any(diagonal_band(u, v, *seg) <= half for seg in QP_DIAGONALS)


def street_spans():
    """(x0, y0, x1, y1, label) centre-line segments for the HUD street name."""
    out = [(min(x0, x1), min(y0, y1), max(x0, x1), max(y0, y1), 'QUEENS PARK CRES') for x0, y0, x1, y1 in QP_DIAGONALS]
    for s in STREETS:
        cuts = sorted({s['a'], s['b']} | {p for f, t, _ in s['names'] for p in (f, t)})
        for lo, hi in zip(cuts, cuts[1:]):
            if hi <= s['a'] or lo >= s['b']:
                continue
            label = next((n for f, t, n in s['names'] if f <= lo and hi <= t), s['name'])
            if s['axis'] == 'h':
                out.append((lo, s['at'], hi, s['at'], label))
            else:
                out.append((s['at'], lo, s['at'], hi, label))
    return out


# Core delivery clients and stations: (u, v, name, transit, sign). A stop sits
# in the curb lane in front of its place so it can be served by car or on
# foot; stations also get a sign on the sidewalk beside the stop. Positions
# follow the real places: Line 1 stations on Yonge, the 94 Wellesley bus
# between Ossington and Castle Frank, the ferry docks at the foot of Bay.
CORE_STOPS = [
    (576, 740, 'UNION DEPOT', 1, (576, 756)),      # Union Station, Front St W
    (712, 740, 'ST LAWRENCE', 0, None),            # St Lawrence Market, Front St E
    (820, 740, 'DISTILLERY', 0, None),             # Mill St
    (576, 516, 'CITY HALL', 0, None),              # Queen St W at Bay
    (204, 516, 'QUEEN WEST', 0, None),             # Queen St W at Trinity Bellwoods
    (448, 412, 'AGO / GRANGE', 0, None),           # Dundas St W at McCaul
    (464, 76, 'ROM / BLOOR', 0, None),             # Bloor St W at Queen's Park
    (372, 344, 'KENSINGTON', 0, None),             # Spadina Ave at Baldwin
    (76, 476, 'DUFFERIN', 0, None),                # Dufferin St at Queen
    (968, 540, 'RIVERSIDE', 0, None),              # Queen St E at Broadview
    (584, 828, 'FERRY TERMINAL', 3, (584, 844)),   # Jack Layton Ferry Terminal
    (724, 828, 'EAST BAYFRONT', 0, None),          # Queens Quay E
    (652, 688, 'KING STATION', 1, (668, 688)),
    (652, 580, 'QUEEN STATION', 1, (668, 580)),
    (652, 444, 'DUNDAS STATION', 1, (668, 444)),
    (652, 332, 'COLLEGE STATION', 1, (668, 332)),
    (652, 212, 'WELLESLEY', 1, (668, 212)),
    (652, 108, 'BLOOR-YONGE', 1, (668, 108)),
    (164, 108, 'OSSINGTON BUS', 2, (180, 108)),    # Ossington station, Bloor St W
    (828, 76, 'CASTLE FRANK', 2, (828, 92)),       # Castle Frank station, Bloor St E
    (444, 928, 'HANLANS POINT', 3, None),
    (720, 920, 'CENTRE ISLAND', 3, None),
    (848, 896, 'WARDS ISLAND', 3, None),
    (464, 740, 'CN TOWER', 0, None),               # Front St W at John
]
