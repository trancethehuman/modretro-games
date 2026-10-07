"""Original tile-aligned top-down pixel art for the core (central Toronto).

Streets come from city_layout.py (real street order and spacing from the City
of Toronto Centreline, compressed). Each block is filled by the neighbourhood
it lies in, looked up from real downtown-grid coordinates, so Kensington,
Chinatown, the Financial District, St. Lawrence and the rest sit where they
are in Toronto. Writes the background, attributes, collision grid and
content/city_art.json. Requires Pillow; no downloaded art or map imagery.
"""
from pathlib import Path
import json, os, sys
from PIL import Image, ImageDraw
import city_layout as L
from streetcar_art import paint_streetcar_stops
import city_kit

ROOT = Path(__file__).resolve().parents[1]; PROJECT = ROOT / 'project'
WIDTH, HEIGHT = L.WIDTH, L.HEIGHT
TW, TH = WIDTH // 8, HEIGHT // 8
COLORS = ['#071821', '#306850', '#86c06c', '#e0f8cf']

# Neighbourhoods by real position (metres east of Yonge, north of Queen on the
# rotated downtown grid). First match wins. Extents are approximate: they
# place each area's character, not an official boundary.
REGIONS = [
    ('RIVERDALE PARK', 2120, 2330, -300, 2400, 'park'),
    ('RIVERDALE', 2330, 2700, -300, 2400, 'houses'),
    ('RIVERSIDE / PORT LANDS', 2120, 2700, -1400, -300, 'warehouse'),
    ('DISTILLERY DISTRICT', 1247, 2120, -1000, -500, 'distillery'),
    ('CORKTOWN', 1247, 2120, -500, 0, 'brick'),
    ('REGENT PARK', 1247, 2120, 0, 1040, 'regent'),
    ('CABBAGETOWN', 1247, 2120, 1040, 2400, 'cabbagetown'),
    ('ST LAWRENCE', 0, 1247, -1000, -688, 'st_lawrence'),
    ('OLD TOWN', 0, 1247, -688, 0, 'old_town'),
    ('MOSS PARK', 420, 1247, 0, 460, 'moss_park'),
    ('GARDEN DISTRICT', 0, 420, 0, 460, 'shops'),
    ('ALLAN GARDENS', 420, 1247, 460, 1040, 'allan_gardens'),
    ('YONGE-DUNDAS', 0, 420, 460, 1040, 'shops'),
    ('CHURCH-WELLESLEY', 0, 1247, 1040, 1800, 'apartments'),
    ('BLOOR-YONGE', -592, 1247, 1800, 2400, 'towers'),
    ('QUEENS PARK', -592, 0, 1040, 1800, 'queens_park'),
    ('UNIVERSITY OF TORONTO', -1438, -592, 1040, 2400, 'campus'),
    ('HARBORD VILLAGE', -2075, -1438, 1040, 2400, 'houses'),
    ('DISCOVERY DISTRICT', -592, 0, 460, 1040, 'discovery'),
    ('CITY HALL', -592, 0, 0, 460, 'city_hall'),
    ('FINANCIAL DISTRICT', -592, 0, -688, 0, 'financial'),
    ('UNION STATION', -592, 0, -1000, -688, 'union'),
    ('ENTERTAINMENT DISTRICT', -1438, -592, -688, 0, 'entertainment'),
    ('CN TOWER / ROGERS CENTRE', -1438, -592, -1000, -688, 'cn_rogers'),
    ('CITYPLACE', -2075, -1438, -1000, -688, 'cityplace'),
    ('KING WEST', -2075, -1438, -688, 0, 'warehouse'),
    ('GRANGE PARK', -1438, -592, 0, 460, 'ago'),
    ('CHINATOWN', -1438, -592, 460, 1040, 'chinatown'),
    ('KENSINGTON MARKET', -2075, -1438, 460, 1040, 'market'),
    ('ALEXANDRA PARK', -2075, -1438, 0, 460, 'apartments'),
    ('LIBERTY VILLAGE / FORT YORK', -4300, -2075, -1000, -382, 'liberty'),
    ('WEST QUEEN WEST', -4300, -2075, -382, 0, 'shops'),
    ('TRINITY BELLWOODS', -3297, -2075, 0, 460, 'park'),
    ('LITTLE PORTUGAL', -4300, -3297, 0, 1040, 'houses'),
    ('LITTLE ITALY', -3297, -2075, 460, 1040, 'shops'),
    ('DUFFERIN GROVE', -4300, -3297, 1040, 2400, 'dufferin_grove'),
    ('PALMERSTON', -3297, -2075, 1040, 2400, 'houses'),
]


# Colour identities by neighbourhood (palette slots; no tiles of their own):
# houses, and shop rows whose awnings carry the colours.
HOUSE_SLOT = {'LITTLE PORTUGAL': 2, 'PALMERSTON': 1, 'RIVERDALE': 1}
SHOP_SLOTS = {'LITTLE ITALY': [6, 1], 'WEST QUEEN WEST': [5, 3, 4, 1, 2]}


def inverse(anchors, px):
    pts = sorted((x, m) for m, x in anchors)
    for (x0, m0), (x1, m1) in zip(pts, pts[1:]):
        if px <= x1:
            return m0 + (px - x0) * (m1 - m0) / (x1 - x0)
    return pts[-1][1]


def region_of(x, y):
    u, w = inverse(L.U_ANCHORS, x), inverse(L.W_ANCHORS, y)
    for name, u0, u1, w0, w1, kind in REGIONS:
        if u0 <= u < u1 and w0 <= w < w1:
            return name, kind
    return 'DOWNTOWN', 'shops'


def main(check=False):
    img = Image.new('RGB', (WIDTH, HEIGHT), COLORS[2]); d = ImageDraw.Draw(img)
    attrs = [6] * (TW * TH)
    collisions = [16] * (TW * TH)
    blocks, canopies, solids, reserved, districts = [], [], [], [], []

    def box(x, y, w, h, c):
        d.rectangle((x, y, x + w - 1, y + h - 1), fill=COLORS[c])

    def cells(x, y, w, h):
        for ty in range(max(0, y // 8), min(TH, (y + h + 7) // 8)):
            for tx in range(max(0, x // 8), min(TW, (x + w + 7) // 8)):
                yield tx, ty, ty * TW + tx

    def is_road_tile(i):
        return collisions[i] == 0

    def attr(x, y, w, h, slot, priority=False):
        for tx, ty, i in cells(x, y, w, h):
            attrs[i] = slot | (128 if priority and not is_road_tile(i) else 0)

    def solid(x, y, w, h, record=None):
        for _, _, i in cells(x, y, w, h):
            collisions[i] = 15
        if record:
            solids.append({'name': record, 'rect': [x, y, w, h]})

    # Water everywhere first (textured at the end); land, roads and islands on top.
    box(0, 0, WIDTH, HEIGHT, 2)
    for i in range(TW * TH):
        collisions[i] = 15; attrs[i] = 0
    mx0, my0, mx1, my1 = L.MAINLAND
    # The city continues past the scene edges: ground there, closed by collision.
    for _, _, i in cells(0, 0, WIDTH, my1):
        attrs[i] = 6
    box(0, 0, WIDTH, my1, 2)
    for _, _, i in cells(mx0, my0, mx1 - mx0, my1 - my0):
        collisions[i] = 16; attrs[i] = 6
    for x, y, r, b in L.ISLANDS:
        box(x, y, r - x, b - y, 2); box(x, y, r - x, 2, 3)
        for _, _, i in cells(x, y, r - x, b - y):
            collisions[i] = 16; attrs[i] = 6
    # Don River channel with wooded banks.
    box(L.RIVER[0], 24, L.RIVER[1] - L.RIVER[0], my1 - 24, 2)
    d.line((L.RIVER[0], 24, L.RIVER[0], my1 - 1), fill=COLORS[0]); d.line((L.RIVER[1] - 1, 24, L.RIVER[1] - 1, my1 - 1), fill=COLORS[0])
    solid(L.RIVER[0], 24, L.RIVER[1] - L.RIVER[0], my1 - 24)
    for _, _, i in cells(L.RIVER[0], 24, L.RIVER[1] - L.RIVER[0], my1 - 24):
        attrs[i] = 0
    # Union rail corridor: two tracks on ballast, a barrier except at underpasses.
    rx0, ry0, rx1, ry1 = L.RAIL
    box(rx0, ry0, rx1 - rx0, ry1 - ry0, 0)
    for x in range(rx0, rx1, 4):
        box(x, ry0 + 2, 2, 4, 1)
    d.line((rx0, ry0 + 1, rx1 - 1, ry0 + 1), fill=COLORS[3]); d.line((rx0, ry0 + 6, rx1 - 1, ry0 + 6), fill=COLORS[3])
    solid(rx0, ry0, rx1 - rx0, ry1 - ry0, 'Union Station rail corridor')
    for _, _, i in cells(rx0, ry0, rx1 - rx0, ry1 - ry0):
        attrs[i] = 0
    # Streets: sidewalk then asphalt, tile by tile from the layout.
    for ty in range(TH):
        for tx in range(TW):
            x, y, i = tx * 8, ty * 8, ty * TW + tx
            if L.road(x + 4, y + 4, L.WALK_HALF):
                box(x, y, 8, 8, 3); collisions[i] = 16; attrs[i] = 0
            if L.road(x + 4, y + 4):
                box(x, y, 8, 8, 1); collisions[i] = 0; attrs[i] = 0
    # Rail bridges over the streets that pass under the corridor.
    for s in L.STREETS:
        if s['axis'] == 'v' and s['a'] < ry0 and s['b'] > ry1:
            half = s['half'] + s['walk']
            d.line((s['at'] - half, ry0, s['at'] + half - 1, ry0), fill=COLORS[0])
            d.line((s['at'] - half, ry1 - 1, s['at'] + half - 1, ry1 - 1), fill=COLORS[0])
    # River bridges get railings.
    for row in L.BRIDGES:
        for s in L.STREETS:
            if s['axis'] == 'h' and s['at'] == row:
                half = s['half'] + s['walk']
                d.line((L.RIVER[0], row - half, L.RIVER[1] - 1, row - half), fill=COLORS[0])
                d.line((L.RIVER[0], row + half - 1, L.RIVER[1] - 1, row + half - 1), fill=COLORS[0])

    # Junctions, lane dashes, crosswalks and signals.
    hs = [s for s in L.STREETS if s['axis'] == 'h']; vs = [s for s in L.STREETS if s['axis'] == 'v']
    junctions = []
    for h in hs:
        for v in vs:
            if h['a'] <= v['at'] <= h['b'] and v['a'] <= h['at'] <= v['b']:
                junctions.append((v, h))

    def near_junction(x, y, margin=8):
        return any(abs(x - v['at']) < v['half'] + v['walk'] + margin and abs(y - h['at']) < h['half'] + h['walk'] + margin
                   for v, h in junctions)
    for s in L.STREETS:
        if s['half'] < 16:
            continue
        for p in range((s['a'] // 32 + 1) * 32, s['b'], 32):
            x, y = (p, s['at']) if s['axis'] == 'h' else (s['at'], p)
            if not L.road(x + 4, y + 4) or near_junction(x + 4, y + 4):
                continue
            if s['axis'] == 'h':
                box(p, s['at'], 8, 1, 3)
            else:
                box(s['at'], p, 1, 8, 3)
    for v, h in junctions:
        cx, cy = v['at'], h['at']
        # Zebra bars across an arm where the cross street's sidewalk continues.
        if h['walk']:
            for arm, top in (('n', cy - h['half'] - h['walk']), ('s', cy + h['half'])):
                beyond = top - 4 if arm == 'n' else top + h['walk'] + 4
                if L.road(cx, top + h['walk'] // 2) and L.road(cx, beyond) and \
                   L.road(cx - v['half'] - 4, top + 4, L.WALK_HALF) and L.road(cx + v['half'] + 4, top + 4, L.WALK_HALF):
                    for x in range(cx - v['half'] + 2, cx + v['half'] - 1, 8):
                        box(x, top + 1, 4, h['walk'] - 2, 3)
        if v['walk']:
            for arm, left in (('w', cx - v['half'] - v['walk']), ('e', cx + v['half'])):
                beyond = left - 4 if arm == 'w' else left + v['walk'] + 4
                if L.road(left + v['walk'] // 2, cy) and L.road(beyond, cy) and \
                   L.road(left + 4, cy - h['half'] - 4, L.WALK_HALF) and L.road(left + 4, cy + h['half'] + 4, L.WALK_HALF):
                    for y in range(cy - h['half'] + 2, cy + h['half'] - 1, 8):
                        box(left + 1, y, v['walk'] - 2, 4, 3)
        if h['half'] == 24 and v['half'] == 24:
            box(cx - 30, cy - 30, 3, 3, 0); box(cx + 27, cy + 27, 3, 3, 0)

    # Queen's Park Crescent's 45-degree parts, drawn to the pixel so the ring
    # reads as a curve; tiles keep one palette each (sidewalk separates
    # asphalt from lawn by more than a tile's diagonal).
    qx0 = min(min(seg[0], seg[2]) for seg in L.QP_DIAGONALS) - 32
    qx1 = max(max(seg[0], seg[2]) for seg in L.QP_DIAGONALS) + 32
    qy0 = min(min(seg[1], seg[3]) for seg in L.QP_DIAGONALS) - 32
    qy1 = max(max(seg[1], seg[3]) for seg in L.QP_DIAGONALS) + 32
    for py in range(qy0, qy1):
        for px in range(qx0, qx1):
            if L.crescent(px + 0.5, py + 0.5):
                d.point((px, py), fill=COLORS[1])
            elif L.crescent(px + 0.5, py + 0.5, walk=True) and not L.road(px, py):
                d.point((px, py), fill=COLORS[3])
    for ty in range(qy0 // 8, qy1 // 8):
        for tx in range(qx0 // 8, qx1 // 8):
            x, y, i = tx * 8, ty * 8, ty * TW + tx
            if L.road(x + 4, y + 4, L.WALK_HALF):
                continue
            if L.crescent(x + 4, y + 4):
                collisions[i] = 0; attrs[i] = 0
            elif any(L.crescent(x + a + 0.5, y + b + 0.5, walk=True) for a in range(8) for b in range(8)):
                ground_left = any(img.getpixel((x + a, y + b)) == tuple(bytes.fromhex(COLORS[2][1:])) for a in range(8) for b in range(8))
                attrs[i] = 6 if ground_left else 0

    # ------------------------------------------------------------- buildings
    def free(x, y, w, h, gap=0):
        if x < mx0 or y < my0 + 8 or x + w > mx1 or y + h > my1:
            return False
        for _, _, i in cells(x - gap, y - gap, w + 2 * gap, h + 2 * gap):
            if collisions[i] != 16 or attrs[i] != 6:
                return False
        return not any(rx < x + w and x < rx + rw and ry < y + h and y < ry + rh for rx, ry, rw, rh in reserved)

    green_px = bytes.fromhex(COLORS[2][1:]) * 64

    def cast_shadow(x, y, w, h):
        """Shadow onto open ground only, and only on tiles that are still
        plain ground, so shadow tiles repeat."""
        plain = {}
        for py in range(y + 4, y + h + 4):
            for px in range(x + 4, x + w + 4):
                i = (py // 8) * TW + px // 8
                if not (0 <= px < WIDTH and 0 <= py < HEIGHT) or (x <= px < x + w and y <= py < y + h):
                    continue
                if i not in plain:
                    tx, ty = px // 8 * 8, py // 8 * 8
                    plain[i] = collisions[i] == 16 and attrs[i] == 6 and img.crop((tx, ty, tx + 8, ty + 8)).tobytes() == green_px
                if plain[i]:
                    d.point((px, py), fill=COLORS[0])

    def lip_free(x, y, w, n):
        return y - n >= my0 and all(collisions[i] != 15 for _, _, i in cells(x, y - n, w, n))

    def building(x, y, w, h, style, kind=None, name=None, lip=True):
        tall = style in (3, 4) and h >= 40
        roof = 16 if tall else 8
        if h < 24:
            style = 0
        cast_shadow(x, y, w, h)
        box(x, y, w, h, 1); d.rectangle((x, y, x + w - 1, y + h - 1), outline=COLORS[0])
        box(x + 2, y + 2, w - 4, max(4, h - roof - 2), 2)
        d.rectangle((x + 4, y + 4, x + w - 5, y + h - roof - 3), outline=COLORS[0])
        for wx in range(x + 4, x + w - 4, 8):
            box(wx, y + h - roof + 3, 4, 3, 3)
        box(x + w // 2 - 2, y + h - 5, 4, 4, 0)
        if w < 24 and style not in (0, 5):
            style = 2
        if style == 0:
            for wx in range(x + 2, x + w - 2, 8):
                box(wx, y + h - 8, 4, 3, 3)
            box(x + 8, y + 8, 8, 8, 1)
        elif style == 1:
            box(x + 8, y + 8, max(8, w - 16), 8, 1)
            for wx in range(x + 8, x + w - 8, 8):
                box(wx, y + 12, 4, 2, 3)
        elif style == 2:
            city_kit.gable_house(d, box, x, y, w, h, roof, COLORS)
        elif style == 3:
            d.rectangle((x + 8, y + 8, x + w - 9, y + h - roof - 7), outline=COLORS[3])
            box(x + w // 2 - 4, y + 8, 8, 8, 1)
        elif style == 4:
            for wx in range(x + 8, x + w - 8, 8):
                d.line((wx, y + 4, wx, y + h - roof - 4), fill=COLORS[3])
            box(x + 8, y + 8, 8, 8, 1)
        else:
            for wx in range(x + 8, x + w - 8, 16):
                box(wx, y + 8, 8, 8, 3); box(wx, y + h - 6, 8, 4, 0)
        if kind:
            landmark_detail(kind, x, y, w, h)
        else:
            city_kit.roof_details(d, box, x, y, w, h, roof, city_kit.seed_of('core', x, y), COLORS, aligned=True)
        solid(x, y, w, h)
        slot = 1 if style == 5 else style + 1
        attr(x, y, w, h, slot, True)
        overhang = 0
        if lip and y - 8 >= my0:
            north = [collisions[i] for _, _, i in cells(x, y - 8, w, 8)]
            if all(c != 15 for c in north):
                overhang = 8
                if tall and y - 24 >= my0 and all(c != 15 for _, _, i in cells(x, y - 24, w, 24) for c in [collisions[i]]):
                    overhang = 24
                    city_kit.tower_crown(d, box, x, y, w, 24, style, COLORS)
                else:
                    box(x, y - 8, w, 8, 2); d.line((x, y - 8, x + w - 1, y - 8), fill=COLORS[0])
                attr(x, y - overhang, w, overhang, slot, True)
        blocks.append({'x': x, 'y': y, 'width': w, 'depth': h, 'height': roof, 'style': style,
                       'landmark': name, 'kind': kind, 'overhang': overhang})

    def landmark_detail(kind, x, y, w, h):
        if kind == 'colonnade':      # Union Station: long stone front, column rhythm
            box(x + 2, y + h - 8, w - 4, 7, 3)
            for cx in range(x + 4, x + w - 3, 4):
                box(cx, y + h - 8, 2, 7, 0)
            box(x + w // 2 - 8, y + 8, 16, 8, 3); d.rectangle((x + w // 2 - 8, y + 8, x + w // 2 + 7, y + 15), outline=COLORS[0])
        elif kind == 'market':       # St Lawrence Market: long hall, clerestory
            box(x + 4, y + 4, w - 8, 4, 3)
            for cx in range(x + 6, x + w - 6, 6):
                box(cx, y + 5, 2, 2, 0)
        elif kind == 'flatiron':     # Gooderham wedge: angled west end and turret
            for k in range(min(h, 12)):
                box(x, y + k, max(0, 12 - k), 1, 2)
            d.ellipse((x + 1, y + h - 9, x + 8, y + h - 2), fill=COLORS[1], outline=COLORS[0])
        elif kind == 'clocktower':   # Old City Hall clock tower
            box(x + w // 2 - 4, y + 2, 8, 10, 3); d.rectangle((x + w // 2 - 4, y + 2, x + w // 2 + 3, y + 11), outline=COLORS[0])
            d.ellipse((x + w // 2 - 2, y + 4, x + w // 2 + 1, y + 7), fill=COLORS[0])
        elif kind == 'twin_towers':  # City Hall: two curved towers around the chamber
            d.pieslice((x + 2, y + 2, x + w // 2 + 4, y + h - 4), 90, 270, fill=COLORS[3], outline=COLORS[0])
            d.pieslice((x + w // 2 - 4, y + 6, x + w - 3, y + h - 4), 270, 90, fill=COLORS[3], outline=COLORS[0])
            d.ellipse((x + w // 2 - 4, y + h // 2 - 3, x + w // 2 + 3, y + h // 2 + 4), fill=COLORS[2], outline=COLORS[0])
        elif kind == 'galleria':     # Eaton Centre glass arcade
            box(x + 4, y + 4, w - 8, h - 12, 3)
            for cy in range(y + 6, y + h - 8, 4):
                d.line((x + 5, cy, x + w - 6, cy), fill=COLORS[0])
        elif kind == 'bowed_glass':  # AGO: long bowed glass front
            d.arc((x + 2, y + h - 14, x + w - 3, y + h + 6), 180, 360, fill=COLORS[3], width=2)
            box(x + w - 12, y + 4, 8, 8, 3)
        elif kind == 'crystal':      # ROM: the Crystal's prism on the Bloor St front of the stone wings
            ccx, ccy = x + 24, y + 8    # a tile corner, so the prism's quarters share tiles
            for py in range(ccy - 12, ccy + 12):
                for px in range(ccx - 12, ccx + 12):
                    a, b = abs(px + 0.5 - ccx), abs(py + 0.5 - ccy)
                    if a + b < 12:
                        c = 0 if a + b >= 11 else 1 if (a < 1 or b < 1 or abs(a - b) < 1) else 3
                        d.point((px, py), fill=COLORS[c])
        elif kind == 'warehouse_chimney':  # Distillery brick works
            box(x + w - 8, y + 2, 4, 12, 0); box(x + w - 7, y + 1, 2, 2, 3)
            for cx in range(x + 4, x + w - 12, 8):
                box(cx, y + 6, 4, 4, 3)
        elif kind == 'armoury':
            for cx in range(x + 4, x + w - 4, 6):
                box(cx, y + 2, 3, 3, 0)
        elif kind == 'hall':
            box(x + 4, y + 4, w - 8, 6, 3); box(x + w // 2 - 2, y + 4, 4, 6, 0)
        elif kind == 'marquee':      # King St theatres: fly tower, lit marquee over the doors
            box(x + 8, y + 8, w - 16, 8, 1); d.rectangle((x + 8, y + 8, x + w - 9, y + 15), outline=COLORS[0])
            box(x + 1, y + h - 7, w - 2, 4, 0)
            for bx in range(x + 1, x + w - 1, 2):
                d.point((bx, y + h - 7), fill=COLORS[3]); d.point((bx, y + h - 4), fill=COLORS[3])
        elif kind == 'mars':         # MaRS: the 1913 hospital wing on College, its portico in the middle
            box(x + 1, y + 1, w - 2, 6, 2); d.line((x + 1, y + 4, x + w - 2, y + 4), fill=COLORS[1])
            box(x + 1, y + 8, w - 2, h - 9, 2)
            for wx in range(x + 3, x + w - 3, 4):
                box(wx, y + 10, 2, 3, 3)
            box(x + w // 2 - 8, y + 8, 16, h - 9, 3)
            for cx in range(x + w // 2 - 7, x + w // 2 + 8, 2):
                d.line((cx, y + 9, cx, y + h - 3), fill=COLORS[1])
            d.line((x + w // 2 - 8, y + 8, x + w // 2 + 7, y + 8), fill=COLORS[0])

    def park(x, y, w, h, name, paths=True):
        reserved.append((x, y, w, h))
        city_kit.lawn_texture(d, x, y, w, h, COLORS)
        if paths and w >= 32 and h >= 32:
            box(x + w // 2 - 2, y, 4, h, 3); box(x, y + h // 2 - 2, w, 4, 3)
        for ty in range(y // 8, (y + h) // 8, 3):
            for tx in range(x // 8, (x + w) // 8 - 1, 3):
                tx8, ty8 = tx * 8, ty * 8
                if ty8 + 16 > y + h or tx8 + 16 > x + w:
                    continue
                if paths and (abs(tx8 + 8 - (x + w // 2)) < 10 or abs(ty8 + 8 - (y + h // 2)) < 10):
                    continue
                city_kit.tree(d, tx8, ty8, COLORS, (tx + ty) & 1)
                for _, _, i in cells(tx8, ty8, 16, 16):
                    attrs[i] = 6 | 128
                canopies.append([tx8, ty8, 16, 16])
        districts.append({'name': name, 'kind': 'park', 'rect': [x, y, w, h]})

    green_rgb = tuple(bytes.fromhex(COLORS[2][1:]))

    def lawn_only(x0, y0, x1, y1):
        """Pixels of untouched lawn in a rectangle (designed parks)."""
        return lambda px, py: x0 <= px < x1 and y0 <= py < y1 and img.getpixel((px, py)) == green_rgb

    def plant_trees(x0, y0, x1, y1, ok, keep_out=()):
        """Tile-aligned trees wherever a 16x16 square is plain lawn."""
        taken = []
        for ty in range(y0 // 8 * 8, y1 - 15, 8):
            for tx in range(x0 // 8 * 8, x1 - 15, 8):
                if any(abs(tx - ox) < 16 and abs(ty - oy) < 16 for ox, oy in taken):
                    continue
                if any(kx - 16 < tx < kx + kw and ky - 16 < ty < ky + kh for kx, ky, kw, kh in keep_out):
                    continue
                if all(ok(tx + a, ty + b) for a in range(16) for b in range(16)):
                    city_kit.tree(d, tx, ty, COLORS, (tx // 8 + ty // 8) & 1)
                    for _, _, i in cells(tx, ty, 16, 16):
                        attrs[i] = 6 | 128
                    canopies.append([tx, ty, 16, 16]); taken.append((tx, ty))

    def tufts(x0, y0, x1, y1):
        """Grass tufts only on whole lawn tiles, so mixed tiles stay shared."""
        for ty in range(y0 // 8, y1 // 8):
            for tx in range(x0 // 8, x1 // 8):
                if all(img.getpixel((tx * 8 + a, ty * 8 + b)) == green_rgb for a in range(8) for b in range(8)):
                    d.point((tx * 8 + 2, ty * 8 + 3), fill=COLORS[1]); d.point((tx * 8 + 3, ty * 8 + 2), fill=COLORS[1])

    def flower_beds(x0, y0, x1, y1, every=1):
        """Whole lawn tiles in a rectangle become flower beds: one bed tile,
        coloured by palette (pink, gold and red beds side by side)."""
        n = 0
        for ty in range(y0 // 8, y1 // 8):
            for tx in range(x0 // 8, x1 // 8):
                X, Y = tx * 8, ty * 8
                if (tx + ty) % every or img.crop((X, Y, X + 8, Y + 8)).tobytes() != green_px:
                    continue
                box(X, Y, 8, 8, 2); d.rectangle((X, Y, X + 7, Y + 7), outline=COLORS[1])
                for py in range(Y + 1, Y + 7):
                    for px in range(X + 1, X + 7):
                        a, b = px % 8, py % 8
                        if (a + 2 * b) % 5 == 0:
                            d.point((px, py), fill=COLORS[3])
                        elif (2 * a + b) % 7 == 3:
                            d.point((px, py), fill=COLORS[1])
                attrs[ty * TW + tx] = (3, 4, 1)[n % 3]; n += 1
        return n

    def trinity_bellwoods(x0, y0, x1, y1):
        """The whole block from Queen to Dundas: the stone gates on Queen St,
        the sunken dog bowl, the walk between them and old trees."""
        reserved.append((x0, y0, x1 - x0, y1 - y0))
        cx = (x0 + x1) // 2
        ok = lawn_only(x0, y0, x1, y1)
        # The walk from the gates up through the park to Dundas.
        box(cx - 2, y0, 4, y1 - y0, 3)
        # The dog bowl: a hollow in the centre of the park, its north slope
        # in shade.
        cy = (y0 + y1) // 2
        bx0, by0, bx1, by1 = x0 + 6, cy - 10, x1 - 7, cy + 9
        d.ellipse((bx0, by0, bx1, by1), fill=COLORS[2], outline=COLORS[1])
        d.arc((bx0 + 1, by0 + 1, bx1 - 1, by1 - 1), 200, 340, fill=COLORS[1], width=2)
        # The gates: two stone piers and the iron arch over the walk.
        for px0 in (cx - 9, cx + 5):
            box(px0, y1 - 8, 4, 7, 3); d.rectangle((px0, y1 - 8, px0 + 3, y1 - 2), outline=COLORS[0])
        d.line((cx - 5, y1 - 7, cx + 4, y1 - 7), fill=COLORS[0]); d.point((cx - 1, y1 - 8), fill=COLORS[0])
        solid(cx - 9, y1 - 8, 4, 7); solid(cx + 5, y1 - 8, 4, 7)
        plant_trees(x0, y0, x1, y1, ok, keep_out=[(bx0, by0, bx1 - bx0, by1 - by0)])
        tufts(x0, y0, x1, y1)
        districts.append({'name': 'TRINITY BELLWOODS', 'kind': 'park', 'rect': [x0, y0, x1 - x0, y1 - y0]})

    def allan_gardens(x0, y0, x1, y1):
        """The whole block: the Palm House's glass dome and its greenhouse
        wings on their paved forecourt at the north, paths from Dundas and
        the corners, and trees."""
        reserved.append((x0, y0, x1 - x0, y1 - y0))
        cx = (x0 + x1) // 2
        hx0, hy0, hx1, hy1 = cx - 24, y0, cx + 24, y0 + 24
        box(hx0, hy0, hx1 - hx0, hy1 - hy0, 3)
        for wx0 in (hx0 + 2, cx + 9):
            box(wx0, hy0 + 6, 13, 12, 2); d.rectangle((wx0, hy0 + 6, wx0 + 12, hy0 + 17), outline=COLORS[0])
            for mx in range(wx0 + 3, wx0 + 12, 3):
                d.line((mx, hy0 + 7, mx, hy0 + 16), fill=COLORS[3])
        d.ellipse((cx - 10, hy0 + 2, cx + 9, hy0 + 21), fill=COLORS[2], outline=COLORS[0])
        d.line((cx - 1, hy0 + 3, cx - 1, hy0 + 20), fill=COLORS[3]); d.line((cx - 9, hy0 + 11, cx + 8, hy0 + 11), fill=COLORS[3])
        d.ellipse((cx - 5, hy0 + 7, cx + 4, hy0 + 16), outline=COLORS[3])
        solid(hx0 + 2, hy0 + 6, hx1 - hx0 - 4, 12)
        attr(hx0, hy0, hx1 - hx0, hy1 - hy0, 2)
        districts.append({'name': 'Allan Gardens Palm House', 'kind': 'landmark', 'rect': [hx0, hy0, hx1 - hx0, hy1 - hy0]})
        # Paths: south from the Palm House doors to Dundas, and diagonals
        # from the south corners.
        ok = lawn_only(x0, y0, x1, y1)
        for py in range(hy1, y1):
            for px in range(x0, x1):
                if ok(px, py) and (abs(px - cx + 0.5) < 2 or abs(abs(px - cx + 0.5) - (py - hy1 + 0.5)) < 1.5):
                    d.point((px, py), fill=COLORS[3])
        flower_beds(x0, hy1, x1, hy1 + 8, 2)
        plant_trees(x0, y0, x1, y1, ok)
        tufts(x0, y0, x1, y1)
        districts.append({'name': 'ALLAN GARDENS', 'kind': 'park', 'rect': [x0, y0, x1 - x0, y1 - y0]})

    def grange_park(x0, y0, x1, y1):
        """South of the AGO: The Grange, the 1817 brick house the gallery
        grew from, facing its park, with the walk to Queen St and trees."""
        reserved.append((x0, y0, x1 - x0, y1 - y0))
        cx = (x0 + x1) // 2
        lm(cx - 16, y0, 32, 16, 0, None, 'The Grange', lip=False)
        ok = lawn_only(x0, y0, x1, y1)
        for py in range(y0 + 20, y1):
            for px in range(cx - 2, cx + 2):
                if ok(px, py):
                    d.point((px, py), fill=COLORS[3])
        plant_trees(x0, y0, x1, y1, ok)
        tufts(x0, y0, x1, y1)
        districts.append({'name': 'GRANGE PARK', 'kind': 'park', 'rect': [x0, y0, x1 - x0, y1 - y0]})

    def queens_park():
        """Queen's Park inside the crescent: the Legislative Building across
        the south end facing University Ave and College, its front lawn and
        flagpoles, and the park to the north with paths radiating from the
        King Edward VII statue under old trees. Around the crescent: the ROM
        and Victoria College at Bloor, University College and the Whitney
        Block on either side."""
        zx0, zy0, zx1, zy1 = L.QP_ZONE
        ix0, iy0, ix1, iy1 = L.QP_INTERIOR
        lx, ly, lw, lh = L.QP_LEGISLATURE
        sx, sy = L.QP_STATUE
        green = tuple(bytes.fromhex(COLORS[2][1:])); cream = tuple(bytes.fromhex(COLORS[3][1:]))
        reserved.append((zx0, zy0, zx1 - zx0, zy1 - zy0))

        def ground(px, py, paths=False):
            """Untouched ground (or, with paths, park paths) in the zone."""
            return zx0 <= px < zx1 and zy0 <= py < zy1 and not L.crescent(px + 0.5, py + 0.5, walk=True) and \
                not L.road(px, py, L.WALK_HALF) and img.getpixel((px, py)) in ((green, cream) if paths else (green,))

        def park_px(px, py):
            return ix0 <= px < ix1 and iy0 <= py < iy1 and ground(px, py)
        # Paths radiating from the statue as on the real trail network: north
        # to the crescent's tip, across between Hoskin and Wellesley, south to
        # the Legislature and on the two northern diagonals (the southern
        # lawns keep their trees).
        for py in range(iy0, ly):
            for px in range(ix0, ix1):
                ax, ay = px - sx + 0.5, py - sy + 0.5
                if park_px(px, py) and (abs(ax) < 2 or abs(ay) < 2 or (ay < 0 and abs(abs(ax) - abs(ay)) < 1.5)):
                    d.point((px, py), fill=COLORS[3])
        d.ellipse((sx - 8, sy - 8, sx + 7, sy + 7), fill=COLORS[3], outline=COLORS[0])
        # King Edward VII on horseback, on a stone plinth.
        d.rectangle((sx - 4, sy - 3, sx + 3, sy + 3), fill=COLORS[1], outline=COLORS[0])
        box(sx - 3, sy - 1, 6, 2, 0); box(sx + 1, sy - 3, 2, 2, 0)
        solid(sx - 8, sy - 4, 16, 8, 'King Edward VII statue')
        # The Legislative Building: pink sandstone, slate roofs, end pavilions
        # and the central block with its tower over the porte-cochere.
        for py in range(ly + 4, ly + lh + 4):
            for px in range(lx + 4, lx + lw + 4):
                if not (lx <= px < lx + lw and ly <= py < ly + lh) and ground(px, py):
                    d.point((px, py), fill=COLORS[0])
        box(lx, ly + 6, lw, lh - 14, 1); d.rectangle((lx, ly + 6, lx + lw - 1, ly + lh - 9), outline=COLORS[0])
        d.line((lx + 12, ly + 14, lx + lw - 13, ly + 14), fill=COLORS[0])
        for px0 in (lx, lx + lw - 12):
            box(px0, ly + 2, 12, lh - 10, 2); d.rectangle((px0, ly + 2, px0 + 11, ly + lh - 9), outline=COLORS[0])
            d.line((px0, ly + 2, px0 + 5, ly + 8), fill=COLORS[0]); d.line((px0 + 11, ly + 2, px0 + 6, ly + 8), fill=COLORS[0])
        cx0 = lx + lw // 2 - 12
        box(cx0, ly, 24, lh - 6, 2); d.rectangle((cx0, ly, cx0 + 23, ly + lh - 7), outline=COLORS[0])
        d.line((cx0, ly, cx0 + 7, ly + 7), fill=COLORS[0]); d.line((cx0 + 23, ly, cx0 + 16, ly + 7), fill=COLORS[0])
        box(cx0 + 7, ly + 5, 10, 10, 1); d.rectangle((cx0 + 7, ly + 5, cx0 + 16, ly + 14), outline=COLORS[0])
        box(cx0 + 11, ly + 9, 2, 2, 3)
        box(lx, ly + lh - 8, lw, 8, 2); d.line((lx, ly + lh - 8, lx + lw - 1, ly + lh - 8), fill=COLORS[0])
        d.rectangle((lx, ly, lx + lw - 1, ly + lh - 1), outline=COLORS[0])
        for wx in range(lx + 3, lx + lw - 3, 4):
            if not cx0 <= wx < cx0 + 24:
                box(wx, ly + lh - 6, 2, 3, 3)
        for ax in range(cx0 + 3, cx0 + 21, 7):
            box(ax, ly + lh - 6, 5, 6, 3); d.line((ax, ly + lh - 6, ax + 4, ly + lh - 6), fill=COLORS[0])
        solid(lx, ly, lw, lh)
        attr(lx, ly, lw, lh, 3, True)
        blocks.append({'x': lx, 'y': ly, 'width': lw, 'depth': lh, 'height': 8, 'style': 2,
                       'landmark': 'Ontario Legislative Building', 'kind': 'legislature', 'overhang': 0})
        districts.append({'name': 'Ontario Legislative Building', 'kind': 'landmark', 'rect': [lx, ly, lw, lh]})
        # Front lawn: the walk from College to the doors and two flagpoles.
        for py in range(ly + lh, iy1):
            for px in range(sx - 4, sx + 4):
                if ground(px, py):
                    d.point((px, py), fill=COLORS[3])
        for fx in (sx - 12, sx + 11):
            box(fx, ly + lh + 3, 1, 6, 0); d.point((fx, ly + lh + 2), fill=COLORS[3])
        # Museums at Bloor and Queen's Park: the ROM on the west side and the
        # Gardiner Museum across the road; Victoria College south of the
        # Gardiner and Convocation Hall's dome south of Hoskin, for the
        # University of Toronto. The other campus lawns are lawn and trees.
        lm(416, 104, 40, 32, 3, 'crystal', 'Royal Ontario Museum')
        solid(416, 96, 40, 8)    # under its roof lip, as before the lip moved onto the tile grid

        def clear_of_crescent(x, y, w, h):
            return not any(L.crescent(px + 0.5, py + 0.5, walk=True) or L.road(px, py, L.WALK_HALF)
                           for py in range(y, y + h) for px in range(x, x + w))
        # The Gardiner: a pale limestone box, its glass top floor and terrace
        # looking west over Queen's Park.
        gx, gy, gw, gh = 552, 96, 32, 24
        assert clear_of_crescent(gx, gy, gw, gh)
        reserved.append((gx, gy, gw, gh)); cast_shadow(gx, gy, gw, gh)
        box(gx, gy, gw, gh, 3); d.rectangle((gx, gy, gx + gw - 1, gy + gh - 1), outline=COLORS[0])
        box(gx + 1, gy + 1, 8, gh - 10, 2)
        for py in range(gy + 3, gy + gh - 9, 4):
            d.line((gx + 2, py, gx + 7, py), fill=COLORS[1])
        d.line((gx, gy + gh - 8, gx + gw - 1, gy + gh - 8), fill=COLORS[0])
        box(gx + 2, gy + gh - 6, gw - 4, 3, 1)
        solid(gx, gy, gw, gh); attr(gx, gy, gw, gh, 0, True)
        blocks.append({'x': gx, 'y': gy, 'width': gw, 'depth': gh, 'height': 8, 'style': 3,
                       'landmark': 'Gardiner Museum', 'kind': 'museum', 'overhang': 0})
        districts.append({'name': 'Gardiner Museum', 'kind': 'landmark', 'rect': [gx, gy, gw, gh]})
        # Victoria College: red sandstone, its tower over the doors.
        assert clear_of_crescent(584, 128, 24, 24)
        lm(584, 128, 24, 24, 0, 'clocktower', 'Victoria College')
        # Convocation Hall: the domed rotunda, its copper dome ribbed round
        # the oculus; centred on a tile corner (vertically) and a tile's middle.
        ccx, ccy, cr = 428, 224, 11.5
        assert clear_of_crescent(416, 212, 24, 24)
        for py in range(212, 236):
            for px in range(416, 440):
                dx, dy = abs(px + 0.5 - ccx), abs(py + 0.5 - ccy)
                r = (dx * dx + dy * dy) ** 0.5
                if r >= cr:
                    continue
                c = 0 if r >= cr - 1 else 3 if (r >= cr - 2.5 or r < 2) else 0 if (dx < 0.6 or dy < 0.6 or abs(dx - dy) < 0.8) else 1
                d.point((px, py), fill=COLORS[c])
        reserved.append((416, 212, 24, 24)); solid(420, 216, 16, 16, 'Convocation Hall')
        districts.append({'name': 'Convocation Hall', 'kind': 'landmark', 'rect': [416, 212, 24, 24]})
        # Old trees on plain lawn, tile-aligned so every one reuses the same
        # tiles (a canopy over a path edge would make one-off tiles).
        taken = []
        for ty in range(zy0, zy1 - 15, 8):
            for tx in range(zx0, zx1 - 15, 8):
                if any(abs(tx - ox) < 16 and abs(ty - oy) < 16 for ox, oy in taken):
                    continue
                if (tx - sx + 8) ** 2 + (ty - sy + 8) ** 2 < 24 ** 2:
                    continue
                if all(ground(tx + a, ty + b) for a in range(16) for b in range(16)):
                    city_kit.tree(d, tx, ty, COLORS, (tx // 8 + ty // 8) & 1)
                    for _, _, i in cells(tx, ty, 16, 16):
                        attrs[i] = 6 | 128
                    canopies.append([tx, ty, 16, 16]); taken.append((tx, ty))
        # Grass tufts only on whole lawn tiles (a tuft beside a path or kerb
        # would make a one-off tile).
        for ty in range(zy0 // 8, zy1 // 8):
            for tx in range(zx0 // 8, zx1 // 8):
                if all(img.getpixel((tx * 8 + a, ty * 8 + b)) == green for a in range(8) for b in range(8)):
                    d.point((tx * 8 + 2, ty * 8 + 3), fill=COLORS[1]); d.point((tx * 8 + 3, ty * 8 + 2), fill=COLORS[1])
        districts.append({'name': 'QUEENS PARK', 'kind': 'park', 'rect': [ix0, iy0, ix1 - ix0, iy1 - iy0]})

    def plaza(x, y, w, h):
        reserved.append((x, y, w, h))
        city_kit.plaza(d, box, x, y, w, h, COLORS)
        for _, _, i in cells(x, y, w, h):
            attrs[i] = 0

    # ------------------------------------------------- district building kits
    TOWER_SLOT = {'black': 2, 'gold': 4, 'granite': 1, 'glass': 5, 'condo': 2}

    def tower(x, y, w, h, look, name=None):
        """High-rises with a recognisable top: 'black' steel and bronze glass
        (the Financial District's Mies towers), 'gold' glass with faceted
        walls, red 'granite' with a notched crown, 'glass' curtain walls and
        slim 'condo' towers. Each draws its upper floors over the street to
        the north like other tall buildings."""
        slot = TOWER_SLOT[look]
        body = 0 if look == 'black' else 2
        cast_shadow(x, y, w, h)
        box(x, y, w, h, body); d.rectangle((x, y, x + w - 1, y + h - 1), outline=COLORS[0])
        fy = y + h - 8                      # the south face below the roof
        d.line((x, fy, x + w - 1, fy), fill=COLORS[0])
        for xx in range(x + 2, x + w - 1, 2):
            d.line((xx, fy + 1, xx, y + h - 2), fill=COLORS[1])
        box(x + w // 2 - 2, y + h - 4, 4, 3, 0)
        cx = x + w // 2
        if look == 'black':                 # black steel: the same mullion grid all over
            for xx in range(x + 2, x + w - 1, 4):
                d.line((xx, y + 1, xx, fy - 1), fill=COLORS[1])
        elif look == 'gold':                # faceted glass: diagonal folds
            for py in range(y + 1, fy):
                for px in range(x + 1, x + w - 1):
                    if (px - py) % 8 == 0:
                        d.point((px, py), fill=COLORS[1])
        elif look == 'granite':             # piers (the notch is in the crown)
            for xx in range(x + 4, x + w - 1, 4):
                d.line((xx, y + 1, xx, fy - 1), fill=COLORS[1])
        else:                               # glass grid
            for py in range(y + 4, fy, 4):
                d.line((x + 1, py, x + w - 2, py), fill=COLORS[1])
            for xx in range(x + 4, x + w - 1, 4):
                d.line((xx, y + 1, xx, fy - 1), fill=COLORS[1])
        solid(x, y, w, h); attr(x, y, w, h, slot, True)
        lip = 24 if lip_free(x, y, w, 24) else 8 if lip_free(x, y, w, 8) else 0
        if lip:
            top = y - lip
            box(x, top, w, lip, body)
            d.line((x, top, x + w - 1, top), fill=COLORS[0])
            d.line((x, top, x, y - 1), fill=COLORS[0]); d.line((x + w - 1, top, x + w - 1, y - 1), fill=COLORS[0])
            for xx in range(x + 2 if look == 'black' else x + 4, x + w - 1, 4):
                d.line((xx, top + 1, xx, y - 1), fill=COLORS[1])
            if look in ('glass', 'condo'):
                for yy in range(top + 4, y, 4):
                    d.line((x + 1, yy, x + w - 2, yy), fill=COLORS[1])
            if look == 'granite':
                for k in range(6):
                    d.line((cx - 6 + k, top + k, cx + 5 - k, top + k), fill=COLORS[0])
            attr(x, top, w, lip, slot, True)
        blocks.append({'x': x, 'y': y, 'width': w, 'depth': h, 'height': 16, 'style': 4 if look in ('glass', 'condo') else 3,
                       'landmark': name, 'kind': look, 'overhang': lip})
        if name:
            districts.append({'name': name, 'kind': 'landmark', 'rect': [x, y, w, h]})

    def pavilion(x, y, w, h):
        """A one-storey black steel and glass pavilion between towers."""
        cast_shadow(x, y, w, h)
        box(x, y, w, h, 0); d.rectangle((x, y, x + w - 1, y + h - 1), outline=COLORS[0])
        for xx in range(x + 2, x + w - 1, 4):
            d.line((xx, y + 1, xx, y + h - 2), fill=COLORS[1])
        solid(x, y, w, h); attr(x, y, w, h, 2, True)
        blocks.append({'x': x, 'y': y, 'width': w, 'depth': h, 'height': 8, 'style': 1,
                       'landmark': None, 'kind': 'pavilion', 'overhang': 0})

    def round_hall(x, y, w, h, name):
        """Roy Thomson Hall: a round glass hall under its diamond-paned
        canopy, on a paved forecourt; centred on a tile corner so its four
        quarters share tiles."""
        reserved.append((x, y, w, h))
        box(x, y, w, h, 3)
        cx, cy = x + w // 2, y + 16
        for py in range(cy - 16, cy + 16):
            for px in range(cx - 16, cx + 16):
                dx, dy = abs(px + 0.5 - cx), abs(py + 0.5 - cy)
                r = (dx * dx + dy * dy) ** 0.5
                if r >= 15.5:
                    continue
                if r >= 14.3:
                    c = 0
                elif 10 <= r < 11:
                    c = 1
                elif (dx + dy) % 4 == 1 or abs(dx - dy) % 4 == 0:
                    c = 1
                else:
                    c = 2
                d.point((px, py), fill=COLORS[c])
        solid(x, y, w, h); attr(x, y, w, h, 2)
        districts.append({'name': name, 'kind': 'landmark', 'rect': [x, y, w, h]})
        blocks.append({'x': x, 'y': y, 'width': w, 'depth': h, 'height': 8, 'style': 3,
                       'landmark': name, 'kind': 'round_hall', 'overhang': 0})

    def hospital(x, y, w, h, name, pad):
        """A hospital on University Avenue: pale stone, rows of windows on
        the south face and a rooftop helipad (centred on a tile corner, so
        it is one tile four ways)."""
        cast_shadow(x, y, w, h)
        box(x, y, w, h, 3); d.rectangle((x, y, x + w - 1, y + h - 1), outline=COLORS[0])
        fy = y + h - 8
        d.line((x, fy, x + w - 1, fy), fill=COLORS[0])
        for wx in range(x + 2, x + w - 2, 4):
            box(wx, fy + 2, 2, 3, 1)
        box(x + w // 2 - 2, y + h - 4, 4, 3, 0)
        px0, py0 = pad
        for py in range(py0 - 8, py0 + 8):
            for px in range(px0 - 8, px0 + 8):
                dx, dy = abs(px + 0.5 - px0), abs(py + 0.5 - py0)
                r = (dx * dx + dy * dy) ** 0.5
                if r < 7.5:
                    h_mark = dy < 4 and (2 <= dx < 4 or (dx < 2 and dy < 1))
                    d.point((px, py), fill=COLORS[3 if h_mark or 6 <= r else 1])
        solid(x, y, w, h); attr(x, y, w, h, 0, True)
        if lip_free(x, y, w, 8):
            box(x, y - 8, w, 8, 1); d.line((x, y - 8, x + w - 1, y - 8), fill=COLORS[0])
            for wx in range(x + 2, x + w - 2, 4):
                d.line((wx, y - 6, wx, y - 2), fill=COLORS[0])
            attr(x, y - 8, w, 8, 0, True)
        blocks.append({'x': x, 'y': y, 'width': w, 'depth': h, 'height': 8, 'style': 3, 'landmark': name,
                       'kind': 'hospital', 'overhang': 8})
        districts.append({'name': name, 'kind': 'landmark', 'rect': [x, y, w, h]})

    def shopfronts(x0, x1, y, depth, slots, signs=False, lip=True):
        """Narrow shops and houses, each in its own colour, with striped
        awnings over the doors (and Chinatown's vertical signboards)."""
        x, k = x0, 0
        while x + 16 <= x1:
            w = x1 - x if x1 - x < 32 else 16
            if free(x, y, w, depth):
                building(x, y, w, depth, 0, lip=lip)
                slot = slots[k % len(slots)]
                attr(x, y - 8 if blocks[-1]['overhang'] else y, w, depth + (8 if blocks[-1]['overhang'] else 0), slot, True)
                city_kit.paint_awnings(d, box, x, y, w, depth, COLORS, signs)
            x += w; k += 1

    def st_james(x0, y0, x1, y1):
        """St James Cathedral at King and Church: a cross-shaped slate roof,
        the spire over the King St doors, and St James Park beside it."""
        cx = 704
        reserved.append((x0, y0, x1 - x0, y1 - y0))
        nave = (cx - 8, y0, 16, y1 - y0); tr = (cx - 16, y0 + 8, 32, 8)
        cast_shadow(*nave); cast_shadow(*tr)
        for bx, by, bw, bh in (nave, tr):
            box(bx, by, bw, bh, 1); d.rectangle((bx, by, bx + bw - 1, by + bh - 1), outline=COLORS[0])
        box(cx - 7, y0 + 9, 14, 6, 1)
        d.line((cx - 1, y0 + 1, cx - 1, y1 - 10), fill=COLORS[0]); d.line((cx, y0 + 1, cx, y1 - 10), fill=COLORS[0])
        d.line((cx - 15, y0 + 11, cx + 14, y0 + 11), fill=COLORS[0]); d.line((cx - 15, y0 + 12, cx + 14, y0 + 12), fill=COLORS[0])
        # Tower and spire: a pyramid seen from above, over the King St doors.
        sy = y1 - 8
        box(cx - 4, sy, 8, 8, 2); d.rectangle((cx - 4, sy, cx + 3, sy + 7), outline=COLORS[0])
        d.line((cx - 3, sy + 1, cx + 2, sy + 6), fill=COLORS[0]); d.line((cx + 2, sy + 1, cx - 3, sy + 6), fill=COLORS[0])
        for bx, by, bw, bh in (nave, tr):
            solid(bx, by, bw, bh); attr(bx, by, bw, bh, 4, True)
        blocks.append({'x': nave[0], 'y': nave[1], 'width': nave[2], 'depth': nave[3], 'height': 8, 'style': 1,
                       'landmark': 'St James Cathedral', 'kind': 'cathedral', 'overhang': 0})
        districts.append({'name': 'St James Cathedral', 'kind': 'landmark', 'rect': [cx - 16, y0, 32, y1 - y0]})
        # St James Park: the Victorian garden east of the cathedral, a
        # fountain in its round bed and paths to King St.
        gx0 = cx + 24
        ok = lawn_only(gx0, y0, x1, y1)
        gcx, gcy = (gx0 + x1) // 2, (y0 + y1) // 2
        for py in range(y0, y1):
            for px in range(gx0, x1):
                if ok(px, py) and (abs(px - gcx + 0.5) < 2 or abs(py - gcy + 0.5) < 2):
                    d.point((px, py), fill=COLORS[3])
        d.ellipse((gcx - 6, gcy - 6, gcx + 5, gcy + 5), fill=COLORS[3], outline=COLORS[1])
        d.ellipse((gcx - 3, gcy - 3, gcx + 2, gcy + 2), fill=COLORS[2], outline=COLORS[0])
        solid(gcx - 4, gcy - 4, 8, 8)
        flower_beds(gx0, y0, x1, y1, 2)
        tufts(gx0, y0, x1, y1)
        districts.append({'name': 'St James Park', 'kind': 'park', 'rect': [gx0, y0, x1 - gx0, y1 - y0]})

    def row(x0, x1, y, depth, widths, styles, gap=0, lip=True):
        """Buildings left to right along a frontage; returns the count."""
        x, k, n = x0, 0, 0
        while x < x1:
            w = widths[k % len(widths)]
            if x + w > x1 or x1 - (x + w) < 16:
                w = x1 - x
            if w >= 16 and free(x, y, w, depth):
                building(x, y, w, depth, styles[k % len(styles)], lip=lip)
                n += 1
            x += w + gap; k += 1
        return n

    def stacked(x0, x1, y0, H, widths, styles, d=24):
        """Rows of buildings filling a block from front to back: an 8 px
        yard between rows takes the next row's roof lip; a short remainder
        becomes a flush row of backyard sheds or shops."""
        if H <= 32:
            row(x0, x1, y0, H, widths, styles); return
        n = (H + 8) // (d + 8); y = y0
        for k in range(n):
            row(x0, x1, y, d, widths[k % 2:] + widths[:k % 2], styles); y += d + 8
        rest = y0 + H - (y - 8)
        if rest >= 16:
            row(x0, x1, y - 8, rest, widths[::-1], styles, lip=False)

    def fill(b, kind, seed):
        x0, y0, x1, y1 = b; W, H = x1 - x0, y1 - y0
        s = seed % 3
        if kind in ('park',):
            if region_of(x0, y0)[0] == 'TRINITY BELLWOODS':
                trinity_bellwoods(x0, y0, x1, y1); return
            park(x0, y0, W, H, region_of(x0, y0)[0]); return
        if kind == 'houses':
            first = len(blocks)
            stacked(x0, x1, y0, H, [24, 16, 24][s:] + [24], [2])
            # Each neighbourhood's houses in its own colour: Little Portugal's
            # azulejo blue, red brick in Palmerston and Riverdale.
            slot = HOUSE_SLOT.get(region_of(x0 + W // 2, y0 + H // 2)[0])
            if slot is not None:
                for b in blocks[first:]:
                    attr(b['x'], b['y'] - b['overhang'], b['width'], b['depth'] + b['overhang'], slot, True)
            return
        if kind == 'cabbagetown':
            if H > 100:   # Riverdale Farm on the valley edge
                park(x0, y0 + 64, W, 56, 'RIVERDALE FARM', paths=False)
                stacked(x0, x1, y0, 56, [24, 16], [2]); stacked(x0, x1, y0 + 128, H - 128, [16, 24], [2])
            else:
                stacked(x0, x1, y0, H, [24, 16], [2])
            return
        if kind == 'shops':
            name = region_of(x0 + W // 2, y0 + H // 2)[0]
            if name in SHOP_SLOTS:    # main streets of small shops under awnings
                shopfronts(x0, x1, y0, 24, SHOP_SLOTS[name]); shopfronts(x0, x1, y0 + 24, H - 24, SHOP_SLOTS[name][::-1], lip=False)
                return
            stacked(x0, x1, y0, H, [24, 32, 24][s:] + [24], [0, 1]); return
        if kind == 'market':      # Kensington: narrow houses painted every colour, awnings
            shopfronts(x0, x1, y0, 24, [1, 4, 5, 3, 2]); shopfronts(x0, x1, y0 + 24, H - 24, [3, 2, 1, 5, 4], lip=False); return
        if kind == 'chinatown':   # Spadina's shops, red and gold, signboards; a hospital on University
            shopfronts(x0, x0 + 32, y0, 24, [1, 4], True); shopfronts(x0, x0 + 32, y0 + 24, H - 24, [4, 1], True, lip=False)
            hospital(x0 + 32, y0, W - 32, H, 'Mount Sinai Hospital', (x0 + 48, y0 + 16)); return
        if kind == 'financial':
            if H >= 48:           # Queen to King: red granite tower beside a glass one
                tower(x0, y0, 32, H, 'granite'); tower(x0 + 32, y0, W - 32, H, 'glass')
            else:                 # King to Front: black towers and their low glass pavilion, gold towers by Union
                tower(x0, y0, 24, H, 'black'); pavilion(x0 + 24, y0, 16, H); tower(x0 + 40, y0, W - 40, H, 'gold')
            return
        if kind == 'entertainment':
            if H >= 48:           # north side of King: the two theatres and their marquees
                lm(x0, y0, 32, 40, 0, 'marquee', 'Princess of Wales Theatre')
                lm(x0 + 32, y0, 32, 40, 0, 'marquee', 'Royal Alexandra Theatre')
                attr(x0, y0 - 8, 32, 48, 2, True)
            else:                 # south side: a glass tower and Roy Thomson Hall at Simcoe
                tower(x0, y0, 32, H, 'glass'); round_hall(x0 + 32, y0, W - 32, H, 'Roy Thomson Hall')
            return
        if kind == 'discovery':   # MaRS on College; its atrium tower and the hospital behind
            lm(x0, y0, W, 16, 0, 'mars', 'MaRS Centre')
            tower(x0, y0 + 16, 32, 24, 'glass')
            hospital(x0 + 32, y0 + 16, W - 32, 24, 'Toronto General Hospital', (x0 + 48, y0 + 24)); return
        if kind == 'cityplace':   # slim glass condo towers on the old railway lands
            x = x0
            while x + 16 <= x1:
                tower(x, y0, 16, H, 'condo')
                if x + 24 <= x1:      # the podium between towers, its green roof
                    box(x + 16, y0, 8, H, 1); d.rectangle((x + 16, y0, x + 23, y0 + H - 1), outline=COLORS[0])
                    for py in range(y0 + 3, y0 + H - 2, 4):
                        d.point((x + 19, py), fill=COLORS[2]); d.point((x + 20, py + 1), fill=COLORS[2])
                    solid(x + 16, y0, 8, H); attr(x + 16, y0, 8, H, 6, True)
                x += 24
            return
        if kind == 'old_town' and H >= 48:   # shops on Queen; St James and its park on King
            row(x0, x1, y0, 16, [24, 16], [1, 0])
            st_james(x0 + 16, y0 + 16, x1, y1)
            row(x0, x0 + 16, y0 + 16, H - 16, [16], [1], lip=False); return
        if kind in ('apartments', 'regent'):
            if kind == 'regent' and H >= 64:
                park(x0, y1 - 24, W, 24, 'REGENT PARK', paths=False)
                row(x0, x1, y0, 32, [W], [1]); return
            stacked(x0, x1, y0, H, [32, 24], [1], 32 if H >= 72 else 24); return
        if kind in ('brick', 'old_town'):
            stacked(x0, x1, y0, H, [24, 16], [1, 0]); return
        if kind == 'towers':
            row(x0, x1, y0, min(48, H), [24, 24] if W < 56 else [32, 24], [4, 3] if s else [4])
            return
        if kind == 'towers_civic':
            row(x0, x1, y0, min(40, H), [32, 24], [3, 4])
            return
        if kind == 'civic':
            row(x0, x1, y0, 40 if H >= 48 else H, [W // 2 // 8 * 8 or 24], [3])
            return
        if kind == 'warehouse':
            row(x0, x1, y0, 32 if H >= 40 else H, [48, 40], [5])
            if H >= 72:
                row(x0, x1, y1 - 32, 32, [40, 48], [5])
            return
        if kind == 'campus':
            row(x0, x1, y0, 24, [W // 8 * 8], [3])
            if H >= 56:
                park(x0, y0 + 32, W, H - 32, 'UNIVERSITY OF TORONTO', paths=False)
            return
        if kind == 'dufferin_grove':
            park(x0, y0, W, 72, 'DUFFERIN GROVE PARK', paths=False)
            row(x0, x1, y0 + 80, 24, [24], [2]); row(x0, x1, y1 - 24, 24, [24], [2])
            return
        # Specific landmark blocks fall through to their own handling below.
        row(x0, x1, y0, 24, [24, 32], [0, 1])

    # ------------------------------------------------------------- landmarks
    def lm(x, y, w, h, style, kind, name, lip=True):
        reserved.append((x, y, w, h))
        building(x, y, w, h, style, kind, name, lip=lip)
        districts.append({'name': name, 'kind': 'landmark', 'rect': [x, y, w, h]})
    # Union Station (Front St W between York and Bay), CN Tower and Rogers Centre.
    lm(552, 760, 48, 32, 3, 'colonnade', 'Union Station')
    reserved.append((416, 760, 64, 32))
    # The Rogers Centre's roof from above: a white dome, its ring and the
    # seams of the sliding panels, centred on a tile corner so its four
    # quarters share tiles.
    rcx, rcy = 432, 776
    for py in range(rcy - 16, rcy + 16):
        for px in range(rcx - 16, rcx + 16):
            dx, dy = px + 0.5 - rcx, py + 0.5 - rcy
            r = (dx * dx + dy * dy) ** 0.5
            if r >= 15.5:
                continue
            c = 3
            if r >= 14.3:
                c = 0
            elif 10 <= r < 11 or (r < 10 and 5 <= abs(dy) < 6):
                c = 1
            d.point((px, py), fill=COLORS[c])
    solid(424, 768, 16, 16, 'Rogers Centre'); attr(416, 760, 32, 32, 6, True)
    d.ellipse((456, 768, 471, 783), fill=COLORS[1], outline=COLORS[0])
    box(462, 752, 4, 22, 0); box(460, 772, 8, 6, 3)
    solid(456, 768, 16, 16, 'CN Tower'); attr(456, 752, 16, 32, 4, True)
    districts.append({'name': 'CN Tower', 'kind': 'landmark', 'rect': [456, 752, 16, 32]})
    districts.append({'name': 'Rogers Centre', 'kind': 'landmark', 'rect': [416, 760, 32, 32]})
    # City Hall and Nathan Phillips Square, Old City Hall, the Eaton Centre.
    lm(544, 432, 32, 24, 3, 'twin_towers', 'City Hall')
    plaza(544, 464, 32, 32)
    lm(576, 432, 32, 24, 0, 'galleria', 'Eaton Centre')
    lm(576, 464, 32, 32, 3, 'clocktower', 'Old City Hall')
    # AGO and Grange Park on Dundas St W at McCaul.
    lm(424, 432, 48, 24, 3, 'bowed_glass', 'Art Gallery of Ontario')
    grange_park(416, 464, 480, 496)
    # Royal Ontario Museum at the north-west corner of Bloor and Queen's Park,
    # Victoria College across the street.
    queens_park()
    # St Lawrence Market and the Gooderham Flatiron on Front St E.
    lm(696, 760, 48, 32, 1, 'market', 'St Lawrence Market')
    lm(672, 680, 32, 32, 1, 'flatiron', 'Gooderham Flatiron')
    # Allan Gardens palm house, Moss Park and its armoury, Massey Hall.
    allan_gardens(672, 320, 752, 368)
    park(704, 464, 48, 32, 'MOSS PARK', paths=False)
    lm(704, 432, 48, 24, 5, 'armoury', 'Moss Park Armoury')
    lm(672, 464, 32, 32, 3, 'hall', 'Massey Hall')
    # Distillery District south of Mill St.
    lm(816, 760, 32, 32, 5, 'warehouse_chimney', 'Distillery District')
    # Fort York beside the rail corridor: grassy ramparts with cut corners
    # (bastions at 45 degrees, so each corner repeats one tile) round the
    # parade ground, and two brick barracks inside.
    reserved.append((176, 736, 48, 56))
    fx0, fy0, fx1, fy1, cut = 176, 744, 224, 792, 8
    for py in range(fy0, fy1):
        for px in range(fx0, fx1):
            a, b = min(px - fx0, fx1 - 1 - px), min(py - fy0, fy1 - 1 - py)
            if a + b < cut:
                continue
            edge = min(a, b, a + b - cut)
            d.point((px, py), fill=COLORS[0 if edge < 1 else 1 if edge < 4 else 2])
    for bx0 in (184, 200):
        box(bx0, 752, 16, 8, 2); d.rectangle((bx0, 752, bx0 + 15, 759), outline=COLORS[0])
        d.line((bx0 + 2, 755, bx0 + 13, 755), fill=COLORS[1])
    box(192, 772, 16, 8, 2); d.rectangle((192, 772, 207, 779), outline=COLORS[0]); d.line((194, 775, 205, 775), fill=COLORS[1])
    solid(176, 736, 48, 56, 'Fort York')
    attr(176, 736, 48, 56, 6)
    attr(184, 752, 32, 8, 1); attr(192, 772, 16, 8, 1)
    districts.append({'name': 'Fort York', 'kind': 'landmark', 'rect': [176, 736, 48, 56]})
    # Islands: Hanlan's, Centre Island and Ward's buildings.
    building(480, 928, 32, 16, 3, 'hall', 'Hanlans service pavilion')
    building(664, 904, 32, 24, 0, None, 'Centre Island pavilion')
    building(872, 888, 24, 24, 2, None, 'Wards Island cottages')

    # ------------------------------------------------------------- blocks
    land = [[collisions[ty * TW + tx] == 16 and attrs[ty * TW + tx] == 6 and
             mx0 <= tx * 8 < mx1 and my0 <= ty * 8 < my1 for tx in range(TW)] for ty in range(TH)]
    seen, found = set(), []
    for ty in range(TH):
        for tx in range(TW):
            if land[ty][tx] and (tx, ty) not in seen and (ty + 1) * 8 <= my1:
                stack = [(tx, ty)]; seen.add((tx, ty)); group = []
                while stack:
                    a, b = stack.pop(); group.append((a, b))
                    for c, e in ((a + 1, b), (a - 1, b), (a, b + 1), (a, b - 1)):
                        if 0 <= c < TW and 0 <= e < TH and land[e][c] and (c, e) not in seen:
                            seen.add((c, e)); stack.append((c, e))
                xs = [g[0] for g in group]; ys = [g[1] for g in group]
                found.append((min(xs) * 8, min(ys) * 8, (max(xs) + 1) * 8, (max(ys) + 1) * 8))
    zx0, zy0, zx1, zy1 = L.QP_ZONE
    for b in sorted(found, key=lambda r: (r[1], r[0])):
        x0, y0, x1, y1 = b
        if y1 - y0 < 24 or x1 - x0 < 24:
            continue
        if x0 < zx1 and zx0 < x1 and y0 < zy1 and zy0 < y1:
            continue   # Queen's Park and its neighbours are drawn by queens_park()
        name, kind = region_of((x0 + x1) // 2, (y0 + y1) // 2)
        districts.append({'name': name, 'kind': kind, 'rect': [x0, y0, x1 - x0, y1 - y0]})
        fill(b, kind, city_kit.seed_of('core-block', x0, y0))

    # ------------------------------------------------------------- dressing
    for u in range(32, 992, 32):
        box(u, my1 - 4, 4, 4, 0)
    ground = bytes.fromhex(COLORS[2][1:]) * 64
    def lot(tx, ty):
        i = ty * TW + tx; x = tx * 8; y = ty * 8
        if collisions[i] != 16 or attrs[i] != 6 or not (mx0 <= x < mx1 and my0 <= y < my1):
            return False
        if any(rx <= x + 4 < rx + rw and ry <= y + 4 < ry + rh for rx, ry, rw, rh in reserved):
            return False
        return img.crop((x, y, x + 8, y + 8)).tobytes() == ground
    def set_attr(tx, ty, value):
        attrs[ty * TW + tx] = value
    canopies += city_kit.dress_lots(d, box, TW, TH, lot, set_attr, COLORS, 'core', [])
    # Transit station signs on the sidewalk beside each stop (campaign stops).
    for _, _, _, transit, sign in L.CORE_STOPS:
        if sign:
            sx, sy = sign
            d.rectangle((sx - 4, sy - 4, sx + 3, sy + 3), fill=COLORS[3], outline=COLORS[0])
            d.line((sx - 2, sy - 2, sx + 1, sy - 2), fill=COLORS[0]); d.line((sx - 1, sy - 2, sx - 1, sy + 1), fill=COLORS[0])
    paint_streetcar_stops(d, 0, COLORS)
    # The Water's Edge Promenade: Queens Quay's south sidewalk is a wooden
    # boardwalk along the harbour (planks across the walk, bollards kept).
    qq = next(st for st in L.STREETS if st['name'] == 'QUEENS QUAY')
    wy = qq['at'] + qq['half']
    for x in range(qq['a'], qq['b'], 8):
        tile = img.crop((x, wy, x + 8, wy + 8)).tobytes()
        if collisions[(wy // 8) * TW + x // 8] != 16:
            continue
        for px in range(x, x + 8):
            for py in range(wy, wy + 8):
                if img.getpixel((px, py)) == tuple(bytes.fromhex(COLORS[3][1:])):
                    d.point((px, py), fill=COLORS[1 if px % 4 == 3 else 2])
        attrs[(wy // 8) * TW + x // 8] = 1
    # Sidewalk slabs, last so only plain sidewalk is touched: the curb's
    # gutter line and concrete joints (not round Queen's Park's crescent or
    # under the rail bridges).
    zx0, zy0, zx1, zy1 = L.QP_ZONE

    def slab_wanted(tx, ty):
        x, y = tx * 8, ty * 8
        return not (zx0 - 32 <= x < zx1 + 32 and zy0 - 32 <= y < zy1 + 32) and not (L.RAIL[1] - 16 <= y < L.RAIL[3] + 16)
    slabs = city_kit.detail_sidewalks(img, d, TW, TH, collisions, attrs, COLORS, slab_wanted)
    slab_tile = {k: img.crop((k[0] * 8, k[1] * 8, k[0] * 8 + 8, k[1] * 8 + 8)).tobytes() for k in slabs}
    # Street furniture on sidewalk slabs away from the corners: lamps and
    # street trees everywhere, benches and bike rings where there are shops.
    quiet = ('houses', 'cabbagetown', 'apartments', 'regent', 'dufferin_grove', 'park', 'brick', 'liberty', 'warehouse')
    city_kit.place_furniture(img, d, box, slabs, slab_tile, attrs, TW, canopies,
                             lambda x, y: region_of(x, y)[1] not in quiet, COLORS)

    # Everything flat stays open (user direction, 2026-10-07): lawns, lots,
    # plazas and yards take cars and walkers alike, and both pass under tree
    # canopies. Buildings, monuments, water, rails and walls block.
    # The spray bay (gameplay: TORONTO.c td_spray_check): hazard-striped bay in
    # the north lane of King St West between Ossington and Bathurst, with the
    # body shop's roll-up door on the house behind it.
    bay = (192, L.ROWS[5] - 24, 32, 16)          # on the tile grid, so its stripes repeat
    bx, by, bw, bh = bay
    city_kit.paint_spray_bay(d, box, COLORS, bx, by, by - 8, door_h=16)
    engine = (PROJECT / 'plugins/toronto-driving/engine/src/states/TORONTO.c').read_text()
    assert city_kit.spray_bay_registered(engine, 0, bx, by), 'spray bay moved: update td_spray_at in TORONTO.c'
    districts.append({'name': 'SPRAY BAY', 'rect': list(bay), 'kind': 'gameplay'})
    # The video screen on the roof at the south-east corner of Yonge and
    # Dundas (the square), animated by the engine like the water.
    sx, sy = L.COLS[5] + 40, L.ROWS[3] + 32
    assert all(attrs[ty * TW + tx] & 7 in (1, 2, 3, 4, 5) for ty in range(sy // 8, sy // 8 + 2) for tx in range(sx // 8, sx // 8 + 4)), \
        'the screen sits on a roof'
    city_kit.paint_screen(img, sx, sy, COLORS)
    districts.append({'name': 'YONGE-DUNDAS SCREEN', 'rect': [sx, sy, city_kit.SCREEN_W, city_kit.SCREEN_H], 'kind': 'scenery'})
    # Open water takes the shared animated texture, with foam along the shore.
    def is_water(x, y):
        return (L.RIVER[0] <= x < L.RIVER[1] and 24 <= y < my1) or y >= my1
    water_tiles = city_kit.texture_water(img, attrs, TW, is_water, COLORS)
    patterns = set(); raw = set()
    for ty in range(TH):
        for tx in range(TW):
            tile = img.crop((tx * 8, ty * 8, tx * 8 + 8, ty * 8 + 8))
            raw.add(tile.tobytes())
            variants = [tile, tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT), tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM), tile.transpose(Image.Transpose.ROTATE_180)]
            patterns.add(min(v.tobytes() for v in variants))
    # TD_ART_PREVIEW=path saves the art even when it is over the tile budget,
    # for tile-budget work.
    if os.environ.get('TD_ART_PREVIEW'):
        img.save(os.environ['TD_ART_PREVIEW'])
        Path(os.environ['TD_ART_PREVIEW'] + '.attrs.json').write_text(json.dumps(attrs))
    assert len(patterns) <= 384, ('core background patterns', len(patterns))
    bad_road = [(i % TW * 8, i // TW * 8) for i, (a, c) in enumerate(zip(attrs, collisions)) if c == 0 and ((a & 7) > 6 or a & 128)]
    assert not bad_road, ('road tiles with priority or a bad palette', bad_road[:8])
    content = {'projection': 'compressed north-up; street order and spacing from the City of Toronto Centreline (see content/districts/core-research.json)',
               'dimensions': [WIDTH, HEIGHT], 'rows': L.ROWS, 'columns': L.COLS,
               'streets': L.STREETS, 'river': L.RIVER, 'bridges': L.BRIDGES, 'rail': L.RAIL,
               'mainland': L.MAINLAND, 'islands': L.ISLANDS, 'blocks': blocks, 'canopies': canopies,
               'solids': solids, 'districts': districts, 'collisions': collisions,
               'collision_rules': {'road': 0, 'foot_only': 16, 'solid': 15},
               'validation': {'raw_unique_tiles': len(raw), 'flip_canonical_unique_tiles': len(patterns),
                              'animated_water_tiles': water_tiles},
               'scope': 'Compressed central Toronto (Dufferin to Broadview, Bloor to the harbour) and the Islands'}
    texts = {ROOT / 'content/city_art.json': json.dumps(content, indent=1) + '\n',
             PROJECT / 'original-art/city_attributes.json': json.dumps(attrs) + '\n'}
    png = PROJECT / 'assets/backgrounds/toronto_city.png'
    if check:
        with Image.open(png) as current:
            assert current.convert('RGB').tobytes() == img.tobytes(), 'Core background pixels are stale'
        for path, text in texts.items():
            assert path.read_text() == text, f'Stale core art output: {path.relative_to(ROOT)}'
        print(f'Core background matches its generator: {len(blocks)} buildings, {len(patterns)} flip-canonical tiles.')
        return
    img.save(png)
    for path, text in texts.items():
        path.write_text(text)
    print(f'Authored {WIDTH}x{HEIGHT} core: {len(blocks)} buildings, {len(districts)} districts/landmarks, '
          f'{len(patterns)} flip-canonical tiles ({len(raw)} raw).')


if __name__ == '__main__':
    main(check='--check' in sys.argv)
