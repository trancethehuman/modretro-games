"""Original tile-aligned top-down pixel art for the core (central Toronto).

Streets come from city_layout.py (real street order and spacing from the City
of Toronto Centreline, compressed). Each block is filled by the neighbourhood
it lies in, looked up from real downtown-grid coordinates, so Kensington,
Chinatown, the Financial District, St. Lawrence and the rest sit where they
are in Toronto. Writes the background, attributes, collision grid and
content/city_art.json. Requires Pillow; no downloaded art or map imagery.
"""
from pathlib import Path
import json, sys
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
    ('THE ANNEX', -2075, -1438, 1040, 2400, 'houses'),
    ('DISCOVERY DISTRICT', -592, 0, 460, 1040, 'civic'),
    ('CITY HALL', -592, 0, 0, 460, 'city_hall'),
    ('FINANCIAL DISTRICT', -592, 0, -688, 0, 'towers'),
    ('UNION STATION', -592, 0, -1000, -688, 'union'),
    ('ENTERTAINMENT DISTRICT', -1438, -592, -688, 0, 'towers_civic'),
    ('CN TOWER / ROGERS CENTRE', -1438, -592, -1000, -688, 'cn_rogers'),
    ('CITYPLACE', -2075, -1438, -1000, -688, 'towers'),
    ('KING WEST', -2075, -1438, -688, 0, 'warehouse'),
    ('GRANGE PARK', -1438, -592, 0, 460, 'ago'),
    ('CHINATOWN', -1438, -592, 460, 1040, 'shops'),
    ('KENSINGTON MARKET', -2075, -1438, 460, 1040, 'market'),
    ('ALEXANDRA PARK', -2075, -1438, 0, 460, 'apartments'),
    ('LIBERTY VILLAGE / FORT YORK', -4300, -2075, -1000, -382, 'liberty'),
    ('WEST QUEEN WEST', -4300, -2075, -382, 0, 'shops'),
    ('TRINITY BELLWOODS', -3297, -2075, 0, 460, 'park'),
    ('LITTLE PORTUGAL', -4300, -3297, 0, 1040, 'houses'),
    ('LITTLE ITALY', -3297, -2075, 460, 1040, 'shops'),
    ('DUFFERIN GROVE', -4300, -3297, 1040, 2400, 'dufferin_grove'),
    ('SEATON VILLAGE', -3297, -2075, 1040, 2400, 'houses'),
]


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
    blocks, canopies, solids, reserved, districts, open_ground = [], [], [], [], [], []

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

    # Water everywhere first; land, roads and islands on top.
    box(0, 0, WIDTH, HEIGHT, 2)
    for py in range(8, HEIGHT, 16):
        for px in range((py // 16 % 2) * 16, WIDTH, 32):
            box(px, py, 8, 1, 3)
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
    for py in range(28, my1, 12):
        box(L.RIVER[0] + 8 + (py // 12 % 2) * 12, py, 8, 1, 3)
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

    # ------------------------------------------------------------- buildings
    def free(x, y, w, h, gap=0):
        if x < mx0 or y < my0 + 8 or x + w > mx1 or y + h > my1:
            return False
        for _, _, i in cells(x - gap, y - gap, w + 2 * gap, h + 2 * gap):
            if collisions[i] != 16 or attrs[i] != 6:
                return False
        return not any(rx < x + w and x < rx + rw and ry < y + h and y < ry + rh for rx, ry, rw, rh in reserved)

    def building(x, y, w, h, style, kind=None, name=None, lip=True):
        tall = style in (3, 4) and h >= 40
        roof = 16 if tall else 8
        if h < 24:
            style = 0
        # Shadow onto open ground only.
        for py in range(y + 4, y + h + 4):
            for px in range(x + 4, x + w + 4):
                i = (py // 8) * TW + px // 8
                if 0 <= px < WIDTH and 0 <= py < HEIGHT and collisions[i] == 16 and attrs[i] == 6 and \
                   not (x <= px < x + w and y <= py < y + h):
                    d.point((px, py), fill=COLORS[0])
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
            city_kit.roof_details(d, box, x, y, w, h, roof, city_kit.seed_of('core', x, y), COLORS)
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
            box(x + 2, y + h - 10, w - 4, 8, 3)
            for cx in range(x + 4, x + w - 3, 4):
                box(cx, y + h - 10, 2, 8, 0)
            box(x + w // 2 - 6, y + 4, 12, 6, 3)
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
        elif kind == 'crystal':      # ROM crystal on heritage stone
            d.polygon([(x + 4, y + h - 6), (x + 10, y + 4), (x + 20, y + 8), (x + 16, y + h - 8)], fill=COLORS[3], outline=COLORS[0])
        elif kind == 'legislature':  # Queen's Park: Romanesque front and towers
            box(x + 2, y + 2, 6, 6, 3); box(x + w - 8, y + 2, 6, 6, 3); box(x + w // 2 - 4, y + 2, 8, 8, 3)
        elif kind == 'palm_house':   # Allan Gardens glass dome
            d.ellipse((x + w // 2 - 8, y + 2, x + w // 2 + 7, y + 17), fill=COLORS[3], outline=COLORS[0])
            d.line((x + w // 2, y + 3, x + w // 2, y + 16), fill=COLORS[0])
        elif kind == 'warehouse_chimney':  # Distillery brick works
            box(x + w - 8, y + 2, 4, 12, 0); box(x + w - 7, y + 1, 2, 2, 3)
            for cx in range(x + 4, x + w - 12, 8):
                box(cx, y + 6, 4, 4, 3)
        elif kind == 'armoury':
            for cx in range(x + 4, x + w - 4, 6):
                box(cx, y + 2, 3, 3, 0)
        elif kind == 'hall':
            box(x + 4, y + 4, w - 8, 6, 3); box(x + w // 2 - 2, y + 4, 4, 6, 0)

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
        open_ground.append((x, y, w, h))

    def plaza(x, y, w, h):
        reserved.append((x, y, w, h)); open_ground.append((x, y, w, h))
        city_kit.plaza(d, box, x, y, w, h, COLORS)
        for _, _, i in cells(x, y, w, h):
            attrs[i] = 0

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
            park(x0, y0, W, H, region_of(x0, y0)[0]); return
        if kind == 'houses':
            stacked(x0, x1, y0, H, [24, 16, 24][s:] + [24], [2]); return
        if kind == 'cabbagetown':
            if H > 100:   # Riverdale Farm on the valley edge
                park(x0, y0 + 64, W, 56, 'RIVERDALE FARM', paths=False)
                stacked(x0, x1, y0, 56, [24, 16], [2]); stacked(x0, x1, y0 + 128, H - 128, [16, 24], [2])
            else:
                stacked(x0, x1, y0, H, [24, 16], [2])
            return
        if kind == 'shops':
            stacked(x0, x1, y0, H, [24, 32, 24][s:] + [24], [0, 1]); return
        if kind == 'market':      # Kensington: small stalls and shopfronts
            stacked(x0, x1, y0, H, [16, 24, 16], [0, 1, 0]); return
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
    d.ellipse((418, 761, 449, 791), fill=COLORS[3], outline=COLORS[0])
    for k in range(3):
        d.arc((422 + k * 4, 765 + k * 4, 445 - k * 4, 787 - k * 4), 180, 360, fill=COLORS[1])
    solid(424, 768, 16, 16, 'Rogers Centre'); attr(416, 760, 32, 32, 2, True)
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
    park(416, 464, 64, 32, 'GRANGE PARK', paths=False)
    # Royal Ontario Museum at Bloor and Queen's Park; the Legislature in Queen's Park.
    lm(448, 96, 32, 40, 3, 'crystal', 'Royal Ontario Museum')
    lm(552, 224, 48, 32, 3, 'legislature', 'Ontario Legislative Building')
    park(544, 200, 64, 16, 'QUEENS PARK', paths=False)
    park(544, 96, 64, 56, 'QUEENS PARK NORTH', paths=True)
    # St Lawrence Market and the Gooderham Flatiron on Front St E.
    lm(696, 760, 48, 32, 1, 'market', 'St Lawrence Market')
    lm(672, 680, 32, 32, 1, 'flatiron', 'Gooderham Flatiron')
    # Allan Gardens palm house, Moss Park and its armoury, Massey Hall.
    park(704, 320, 48, 48, 'ALLAN GARDENS', paths=False)
    reserved.pop()
    lm(716, 328, 24, 24, 3, 'palm_house', 'Allan Gardens Palm House')
    park(704, 464, 48, 32, 'MOSS PARK', paths=False)
    lm(704, 432, 48, 24, 5, 'armoury', 'Moss Park Armoury')
    lm(672, 464, 32, 32, 3, 'hall', 'Massey Hall')
    # Distillery District south of Mill St.
    lm(816, 760, 32, 32, 5, 'warehouse_chimney', 'Distillery District')
    # Fort York beside the rail corridor.
    reserved.append((176, 736, 48, 56))
    d.polygon([(184, 744), (216, 744), (222, 764), (200, 788), (178, 764)], fill=COLORS[3], outline=COLORS[0])
    box(192, 756, 16, 12, 1); d.rectangle((192, 756, 207, 767), outline=COLORS[0])
    solid(176, 736, 48, 56, 'Fort York')
    attr(176, 736, 48, 56, 6)
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
    for b in sorted(found, key=lambda r: (r[1], r[0])):
        x0, y0, x1, y1 = b
        if y1 - y0 < 24 or x1 - x0 < 24:
            continue
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

    # Private lots and yards inside blocks are closed; sidewalks, parks and
    # squares stay open to walkers (cars keep to the asphalt).
    for ty in range(my0 // 8, my1 // 8):
        for tx in range(mx0 // 8, mx1 // 8):
            i = ty * TW + tx; x, y = tx * 8 + 4, ty * 8 + 4
            if collisions[i] == 16 and not L.road(x, y, L.WALK_HALF) and \
               not any(rx <= x < rx + rw and ry <= y < ry + rh for rx, ry, rw, rh in open_ground):
                collisions[i] = 15
    patterns = set(); raw = set()
    for ty in range(TH):
        for tx in range(TW):
            tile = img.crop((tx * 8, ty * 8, tx * 8 + 8, ty * 8 + 8))
            raw.add(tile.tobytes())
            variants = [tile, tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT), tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM), tile.transpose(Image.Transpose.ROTATE_180)]
            patterns.add(min(v.tobytes() for v in variants))
    assert len(patterns) <= 384, ('core background patterns', len(patterns))
    assert all((a & 7) <= 6 and not (a & 128) for a, c in zip(attrs, collisions) if c == 0)
    content = {'projection': 'compressed north-up; street order and spacing from the City of Toronto Centreline (see content/districts/core-research.json)',
               'dimensions': [WIDTH, HEIGHT], 'rows': L.ROWS, 'columns': L.COLS,
               'streets': L.STREETS, 'river': L.RIVER, 'bridges': L.BRIDGES, 'rail': L.RAIL,
               'mainland': L.MAINLAND, 'islands': L.ISLANDS, 'blocks': blocks, 'canopies': canopies,
               'solids': solids, 'districts': districts, 'collisions': collisions,
               'collision_rules': {'road': 0, 'foot_only': 16, 'solid': 15},
               'validation': {'raw_unique_tiles': len(raw), 'flip_canonical_unique_tiles': len(patterns)},
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
