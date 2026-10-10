"""The world at double scale (user direction 2026-10-10: blocks, parks and
sidewalks were too small next to the courier).

Each of the four compressed districts (old ids: 0 core, 1 west, 2 High Park,
3 east) keeps its authored layout in the old 1024 x 976 units. Here it is
drawn at about twice the scale, 2000 x 1904, and split into four native
scenes of 1024 x 976 that overlap by 48 px at their seams (north-west,
north-east, south-west, south-east): new district id = old id * 4 + quadrant.

Points map with a per-axis, street-aware function rather than a plain x2:
a street's centreline doubles, asphalt keeps its width (cars are the same
size), a sidewalk widens (8 -> 16 px on arterials) and block interiors take
the rest. Streets are kept at least SEAM_CLEAR px from a scene seam, so
driving along one never flips between scenes.
"""
from bisect import bisect_right

OLD_W, OLD_H = 1024, 976
SCENE_W, SCENE_H = 1024, 976
# Seam portals sit 24 px inside each scene edge and arrive 24 px inside the
# neighbour's: neighbouring scenes overlap by 48 px, so a district drawn as
# 2 x 2 scenes is 2000 x 1904 and its seams are the lines x = 1000, y = 952.
OVERLAP = 48
WORLD_W, WORLD_H = 2 * SCENE_W - OVERLAP, 2 * SCENE_H - OVERLAP
SEAM_X, SEAM_Y = SCENE_W - 24, SCENE_H - 24
# A street's centre stays this far from a seam line: a car in its lane and a
# walker on its sidewalk never touch the seam while following it.
SEAM_CLEAR = 48
OLD_NAMES = ('core', 'west', 'high_park', 'east')
QUADRANTS = ('nw', 'ne', 'sw', 'se')
# New sidewalk widths by old (asphalt half, sidewalk) class.
WIDER_WALK = {(24, 8): 16, (16, 8): 16, (12, 8): 12, (8, 0): 0}


def new_walk(half, walk):
    return WIDER_WALK.get((half, walk), walk * 2)


def _linear(knots, x):
    xs = [k[0] for k in knots]
    if x <= xs[0]:
        return knots[0][1] + (x - xs[0])
    if x >= xs[-1]:
        return knots[-1][1] + (x - xs[-1])
    k = bisect_right(xs, x) - 1
    (x0, y0), (x1, y1) = knots[k], knots[k + 1]
    return y0 + (x - x0) * (y1 - y0) / (x1 - x0)


class AxisMap:
    """Monotone piecewise-linear map of one axis from old to new pixels.

    breaks: (old centre, old asphalt half, old sidewalk) for each street on
    this axis. Centres scale by about two, warped so that the widest gap near
    the seam takes the seam; within a street's band the asphalt keeps its
    width and the sidewalk widens; block interiors take the rest."""

    def __init__(self, breaks, old_size, new_size, seam, prefer=None):
        merged = {}
        for c, half, walk in breaks:
            if c not in merged or half + walk > sum(merged[c]):
                merged[c] = (half, walk)
        self.old_size, self.new_size, self.seam = old_size, new_size, seam
        centres = sorted(merged)
        scale = new_size / old_size
        base = [c * scale for c in centres]
        # The gap (between consecutive centres, or a scene edge) whose middle
        # becomes the seam: wide enough and as close to the seam as possible.
        edges = [0.0] + base + [float(new_size)]
        best = None
        for a, b in zip(edges, edges[1:]):
            if b - a < 2 * SEAM_CLEAR + 16:
                continue
            mid = (a + b) / 2
            cost = abs(mid - seam)
            if best is None or cost < best[0]:
                best = (cost, mid)
        # An authored seam position (old pixels) overrides the gap search.
        if prefer is not None:
            best = (0, prefer * scale)
        assert best is not None, 'no gap for the seam'
        warp = [(0.0, 0.0), (best[1], float(seam)), (float(new_size), float(new_size))]
        self.centres = centres
        # Centres on the tile grid: road edges, rails and lane marks repeat.
        self.targets = [int(round(_linear(warp, t) / 8)) * 8 for t in base]
        assert all(abs(t - seam) >= SEAM_CLEAR for t in self.targets), (self.targets, seam)
        self.bands = [merged[c] for c in centres]
        # Knots by priority: centres, then asphalt edges (the first and last
        # asphalt pixel keep their offsets), then sidewalk edges. A knot that
        # would break monotonicity with those already placed is dropped, so
        # two streets whose 1x bands touched get a jump between them, not a
        # squeezed road.
        cands = []
        for c, t, (half, walk) in zip(centres, self.targets, self.bands):
            nw = new_walk(half, walk)
            cands.append((0, c, t))
            if half:
                cands += [(1, c - half, t - half), (1, c + half - 1, t + half - 1)]
            if walk:
                cands += [(2, c - half - walk, t - half - nw), (2, c + half + walk - 1, t + half + nw - 1)]
        knots = {0: 0, old_size: new_size}
        for _, x, y in sorted(cands, key=lambda k: k[0]):
            if not 0 < x < old_size or x in knots:
                continue
            lo = max(k for k in knots if k < x)
            hi = min(k for k in knots if k > x)
            if knots[lo] < y < knots[hi]:
                knots[x] = y
        self.knots = sorted(knots.items())

    def __call__(self, x):
        return _linear(self.knots, x)

    def map_int(self, x, align=1):
        return int(round(self(x) / align)) * align


class DistrictMap:
    def __init__(self, old_id, xbreaks, ybreaks, prefer_x=None, prefer_y=None):
        self.old_id = old_id
        self.fx = AxisMap(xbreaks, OLD_W, WORLD_W, SEAM_X, prefer_x)
        self.fy = AxisMap(ybreaks, OLD_H, WORLD_H, SEAM_Y, prefer_y)

    def point(self, u, v, align=1):
        return self.fx.map_int(u, align), self.fy.map_int(v, align)

    def rect(self, x, y, w, h, align=8):
        """Map a rectangle by its corners, on the tile grid."""
        x0, y0 = self.point(x, y, align)
        x1, y1 = self.point(x + w, y + h, align)
        return x0, y0, max(align, x1 - x0), max(align, y1 - y0)


def scene_of(old_id, X, Y):
    """(new district id, local u, local v) for a district-world point: the
    scene on its side of each seam line."""
    q = (1 if X >= SEAM_X else 0) + (2 if Y >= SEAM_Y else 0)
    ox, oy = scene_origin(q)
    return old_id * 4 + q, X - ox, Y - oy


def scene_origin(new_id):
    """District-world position of a scene's top-left pixel."""
    q = new_id & 3
    return (SCENE_W - OVERLAP if q & 1 else 0), (SCENE_H - OVERLAP if q & 2 else 0)


def scene_slug(new_id):
    return f'toronto_{OLD_NAMES[new_id >> 2]}_{QUADRANTS[new_id & 3]}'


# Names shown when the HUD points to the next area and on the city map.
SCENE_NAMES = [
    'KENSINGTON/U OF T', 'YONGE & CHURCH', 'QUEEN & KING WEST', 'FINANCIAL DISTRICT',
    'JUNCTION TRIANGLE', 'BROCKTON VILLAGE', 'RONCESVALLES', 'PARKDALE',
    'BLOOR WEST VILLAGE', 'THE JUNCTION', 'SWANSEA', 'HIGH PARK',
    'RIVERDALE', 'GREENWOOD', 'RIVERSIDE', 'LESLIEVILLE',
]


def _core_breaks():
    import city_layout as L
    xb, yb = [], []
    for s in L.STREETS:
        (yb if s['axis'] == 'h' else xb).append((s['at'], s['half'], s['walk']))
    return xb, yb


def _path_breaks(spec, road_half, walk):
    xb, yb = [], []
    for route in spec['roads']:
        for (x1, y1), (x2, y2) in zip(route['points'], route['points'][1:]):
            if x1 == x2:
                xb.append((x1, road_half, walk))
            if y1 == y2:
                yb.append((y1, road_half, walk))
    for route in spec.get('footpaths', []):
        for (x1, y1), (x2, y2) in zip(route['points'], route['points'][1:]):
            if x1 == x2:
                xb.append((x1, 8, 0))
            if y1 == y2:
                yb.append((y1, 8, 0))
    return xb, yb


_MAPS = {}


def district_map(old_id):
    if old_id in _MAPS:
        return _MAPS[old_id]
    prefer = {}
    if old_id == 0:
        xb, yb = _core_breaks()
        # Downtown's landmark band (City Hall to Massey Hall, south of
        # Dundas) goes to the southern scenes: the seam runs just south of
        # Dundas, which keeps the warp gentle.
        prefer['prefer_y'] = 426
    elif old_id in (1, 2):
        import west_layout as W
        spec = W.WEST if old_id == 1 else W.HIGH_PARK
        xb, yb = _path_breaks(spec, W.ROAD_HALF, W.WALK_HALF - W.ROAD_HALF)
    else:
        import east_layout as E
        xb, yb = _path_breaks(E.EAST, E.ROAD_HALF, E.WALK_HALF - E.ROAD_HALF)
    m = DistrictMap(old_id, xb, yb, **prefer)
    _MAPS[old_id] = m
    return m


def map_point(old_id, u, v):
    """Old (district, u, v) -> (new district, local u, local v)."""
    X, Y = district_map(old_id).point(u, v)
    return scene_of(old_id, X, Y)


# ------------------------------------------------------------ native grids
def _root():
    from pathlib import Path
    return Path(__file__).resolve().parents[1]


_GRIDS = {}


def encode_grid(values):
    """GB Studio's run-length text ('0f102+00!...') for a tile grid."""
    out, last, count = [], None, 0
    for value in list(values) + [None]:
        if value != last:
            if count:
                out.append(f'{last:02x}' + ('!' if count == 1 else f'{count:x}+'))
            last, count = value, 0
        count += 1
    return ''.join(out)


def decode_grid(text):
    out, pos = [], 0
    while pos < len(text):
        value = int(text[pos:pos + 2], 16)
        pos += 2
        if text[pos] == '!':
            count, pos = 1, pos + 1
        else:
            end = text.index('+', pos)
            count, pos = int(text[pos:end], 16), end + 1
        out.extend([value] * count)
    return out


def scene_grid(new_id):
    """Collision values of one scene (128 x 122 tiles, row-major), as the
    art generators wrote them (content/scenes/<slug>.json, run-length)."""
    if new_id not in _GRIDS:
        import json
        path = _root() / f'content/scenes/{scene_slug(new_id)}.json'
        _GRIDS[new_id] = decode_grid(json.loads(path.read_text())['collisions'])
    return _GRIDS[new_id]


WORLD_TW, WORLD_TH = WORLD_W // 8, WORLD_H // 8
SCENE_TW, SCENE_TH = SCENE_W // 8, SCENE_H // 8


def world_grid(old_id):
    """A district's collision grid in district-world tiles, stitched from
    its four scenes (the overlaps agree)."""
    key = ('world', old_id)
    if key not in _GRIDS:
        grid = [None] * (WORLD_TW * WORLD_TH)
        for q in range(4):
            new_id = old_id * 4 + q
            ox, oy = scene_origin(new_id)
            cells = scene_grid(new_id)
            for ty in range(SCENE_TH):
                row = (oy // 8 + ty) * WORLD_TW + ox // 8
                for tx in range(SCENE_TW):
                    value = cells[ty * SCENE_TW + tx]
                    assert grid[row + tx] in (None, value), ('scene overlap differs', old_id, q, tx, ty)
                    grid[row + tx] = value
        _GRIDS[key] = grid
    return _GRIDS[key]


def map_stop(old_id, u, v, anchor=None):
    """A plan stop (and its parking anchor, which must land in the same
    scene) -> (new district, u, v, anchor or None)."""
    d, nu, nv = map_point(old_id, u, v)
    if anchor is None:
        return d, nu, nv, None
    ad, au, av = map_point(old_id, *anchor)
    if ad != d:
        # Express the anchor in the stop's scene (scenes overlap by 48 px).
        X, Y = district_map(old_id).point(*anchor)
        ox, oy = scene_origin(d)
        au, av = X - ox, Y - oy
        assert 8 <= au <= 1016 and 8 <= av <= 968, ('parking anchor outside the stop scene', old_id, u, v, anchor)
    return d, nu, nv, (au, av)
