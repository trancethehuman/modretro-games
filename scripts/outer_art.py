"""Original manual-palette art for the outer districts (West, High Park and
East) at double scale, split into four native scenes each.

The authored layouts (west_layout.py, east_layout.py) stay in their compressed
1x plan coordinates; world2x.py maps every point onto the 2000 x 1904 district
world (asphalt keeps its width, sidewalks and blocks grow). This module paints
that world, checks it, and writes one background, attribute list and
collision grid per scene. No map imagery, photos, official logos or
downloaded geometry are used as pixels.
"""
import hashlib
import json
from collections import deque
from pathlib import Path
from PIL import Image, ImageDraw
import city_kit
import greenery
import world2x
from streetcar_art import paint_streetcar_stops

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "project"
COLORS = ["#071821", "#306850", "#86c06c", "#e0f8cf"]
WIDTH, HEIGHT = world2x.WORLD_W, world2x.WORLD_H
TW, TH = WIDTH // 8, HEIGHT // 8
ROAD_HALF, WALK_HALF = 24, 40
PATH_HALF = 12


def mapper(old_id):
    m = world2x.district_map(old_id)

    def X(x):
        if x <= 0:
            return 0
        if x >= world2x.OLD_W:
            return WIDTH
        if x == 24:
            return 24
        if x == 1000:
            return WIDTH - 24
        return m.fx.map_int(x)

    def Y(y):
        if y <= 0:
            return 0
        if y >= world2x.OLD_H:
            return HEIGHT
        if y == 24:
            return 24
        if y == 952:
            return HEIGHT - 24
        return m.fy.map_int(y)

    def rect(r):
        x, y, w, h = r
        x0, y0 = X(x) // 8 * 8, Y(y) // 8 * 8
        x1, y1 = (X(x + w) + 7) // 8 * 8, (Y(y + h) + 7) // 8 * 8
        return [x0, y0, max(8, x1 - x0), max(8, y1 - y0)]
    return X, Y, rect


def scale_spec(spec, areas):
    """The authored layout in district-world pixels (a copy; plan
    coordinates stay in the original under 'plan')."""
    X, Y, rect = mapper(spec["id"])

    def pts(points):
        return [[X(x), Y(y)] for x, y in points]

    def route(r):
        return {**r, "points": pts(r["points"])}
    out = {k: v for k, v in spec.items()}
    out["roads"] = [route(r) for r in spec["roads"]]
    out["footpaths"] = [route(r) for r in spec.get("footpaths", [])]
    out["ports"] = [{**p, "x": X(p["x"]), "y": Y(p["y"])} for p in spec["ports"]]
    out["conditional_ports"] = [{**p, "x": X(p["x"]), "y": Y(p["y"])} for p in spec.get("conditional_ports", [])]
    parks = []
    for p in spec["parks"]:
        q = {**p, "rect": rect(p["rect"])}
        q["features"] = [{**f, "rect": rect(f["rect"])} for f in p.get("features", [])]
        if p.get("fieldhouse"):
            q["fieldhouse"] = rect(p["fieldhouse"])
        parks.append(q)
    out["parks"] = parks
    out["water"] = [rect(w) for w in spec.get("water", [])]
    if "pond" in spec:
        out["pond"] = [(X(x), Y(y)) for x, y in spec["pond"]]
    out["rails"] = [route(r) for r in spec.get("rails", [])]
    if "gardiner" in spec:
        g = rect(spec["gardiner"])
        out["gardiner"] = [0, g[1], WIDTH, 16]
    out["closed_frontiers"] = [{**f, "rect": rect(f["rect"])} for f in spec.get("closed_frontiers", [])]
    out["landmarks"] = []
    for l in spec["landmarks"]:
        x, y, w, h = rect([l["x"], l["y"], l["width"], l["depth"]])
        out["landmarks"].append({**l, "x": x, "y": y, "width": w, "depth": h})
    # The spray bay keeps its place in the lane of its street; the garage
    # door moves out with the wider sidewalk.
    bay = spec["spray_bay"]
    centre = min((y1 for r in spec["roads"] for (x1, y1), (x2, y2) in zip(r["points"], r["points"][1:])
                  if y1 == y2 and min(x1, x2) <= bay["x"] <= max(x1, x2)), key=lambda c: abs(c - bay["y"]))
    by = Y(centre) + bay["y"] - centre
    out["spray_bay"] = {**bay, "x": X(bay["x"]) // 8 * 8, "y": by,
                        "door_bottom": Y(centre) - (centre - bay["door_bottom"]) - 8}
    out["stop_candidates"] = []
    for s in spec["stop_candidates"]:
        t = {**s, "x": X(s["x"]), "y": Y(s["y"])}
        if s.get("parking_anchor"):
            t["parking_anchor"] = [X(s["parking_anchor"][0]), Y(s["parking_anchor"][1])]
        out["stop_candidates"].append(t)
    out["traffic_loops"] = [pts(loop) for loop in spec["traffic_loops"]]
    out["areas"] = [(name, (X(x0), Y(y0), X(x1), Y(y1))) for name, (x0, y0, x1, y1) in areas]
    return out


def extended_points(route, spec, foot=False):
    """Extend only registered seams through the image border."""
    points = [p[:] for p in route["points"]]
    for port in spec["ports"]:
        if bool(port["foot_only"]) != foot:
            continue
        end = [port["x"], port["y"]]
        outside = [0 if port["edge"] == "west" else WIDTH, port["y"]]
        if points[0] == end:
            points.insert(0, outside)
        elif points[-1] == end:
            points.append(outside)
    return points


def paint_path(draw, points, half, fill):
    """Square-ended, tile-aligned cardinal paths with exact exclusive far edges."""
    for (x1, y1), (x2, y2) in zip(points, points[1:]):
        assert x1 == x2 or y1 == y2, (x1, y1, x2, y2)
        draw.rectangle((min(x1, x2) - half, min(y1, y2) - half, max(x1, x2) + half - 1, max(y1, y2) + half - 1), fill=fill)


def rail_stairs(points):
    """An 8 px stepped north-up interpretation of a diagonal rail curve."""
    result = [points[0][:]]
    for a, b in zip(points, points[1:]):
        steps = max(abs(b[0] - a[0]), abs(b[1] - a[1])) // 8
        for step in range(1, steps + 1):
            x = round((a[0] + (b[0] - a[0]) * step / steps) / 8) * 8
            y = round((a[1] + (b[1] - a[1]) * step / steps) / 8) * 8
            if result[-1][0] != x:
                result.append([x, result[-1][1]])
            if result[-1][1] != y:
                result.append([x, y])
    return result


def render(plan, areas, kinds=None):
    """Paint one outer district. Returns (image, attrs, collisions, metadata)."""
    spec = scale_spec(plan, areas)
    areas = spec["areas"]
    slug = spec["slug"]
    city_kit.WALK_HALF = WALK_HALF
    img = Image.new("RGB", (WIDTH, HEIGHT), COLORS[2])
    d = ImageDraw.Draw(img)
    road_mask = Image.new("1", (WIDTH, HEIGHT), 0)
    walk_mask = Image.new("1", (WIDTH, HEIGHT), 0)
    rd, wd = ImageDraw.Draw(road_mask), ImageDraw.Draw(walk_mask)
    attrs = [6] * (TW * TH)
    collisions = [16] * (TW * TH)
    blocks, canopies = [], []

    def box(x, y, w, h, color):
        d.rectangle((x, y, x + w - 1, y + h - 1), fill=COLORS[color])

    def cells(x, y, w, h):
        for ty in range(max(0, y // 8), min(TH, (y + h + 7) // 8)):
            for tx in range(max(0, x // 8), min(TW, (x + w + 7) // 8)):
                yield tx, ty, ty * TW + tx

    def attr(x, y, w, h, slot, priority=False):
        for tx, ty, i in cells(x, y, w, h):
            attrs[i] = slot | (128 if priority and not road_mask.getpixel((tx * 8 + 4, ty * 8 + 4)) else 0)

    def solid(x, y, w, h):
        for _, _, i in cells(x, y, w, h):
            collisions[i] = 15

    def mask_solid(mask, slot=None):
        for ty in range(TH):
            for tx in range(TW):
                if mask.crop((tx * 8, ty * 8, tx * 8 + 8, ty * 8 + 8)).getbbox():
                    collisions[ty * TW + tx] = 15
                    if slot is not None:
                        attrs[ty * TW + tx] = slot

    # Visible closed district edges; only named portals are reopened.
    for x, y, w, h in [(0, 0, WIDTH, 16), (0, HEIGHT - 16, WIDTH, 16), (0, 0, 16, HEIGHT), (WIDTH - 16, 0, 16, HEIGHT)]:
        box(x, y, w, h, 1)
        solid(x, y, w, h)
    for frontier in spec["closed_frontiers"]:
        # The edge of the map, drawn as a fenced rail yard: closed ground.
        x, y, w, h = frontier["rect"]
        box(x, y, w, h, 2)
        for ty in range(y + 16, y + h - 8, 24):
            box(x, ty, w, 8, 0)
            box(x, ty + 3, w, 1, 3)
            for px in range(x, x + w, 8):
                box(px + 3, ty, 2, 8, 1)
        for px in range(x, x + w, 8):
            box(px, y + 2, 8, 1, 0)
            box(px + 3, y, 1, 4, 0)
        solid(x, y, w, h)
        attr(x, y, w, h, 0)
    for x, y, w, h in spec["water"]:
        box(x, y, w, h, 2)
        solid(x, y, w, h)
        attr(x, y, w, h, 0)
    for park in spec["parks"]:
        x, y, w, h = park["rect"]
        box(x, y, w, h, 2)
        attr(x, y, w, h, 6)
    pond_mask = None
    if "pond" in spec:
        d.polygon(spec["pond"], fill=COLORS[2], outline=COLORS[0])
        pond_mask = Image.new("1", (WIDTH, HEIGHT))
        ImageDraw.Draw(pond_mask).polygon(spec["pond"], fill=1)
        mask_solid(pond_mask, 0)
    # Rail and expressway are barriers; authored crossings go over them.
    for rail in spec["rails"]:
        points = rail_stairs(rail["points"])
        rail_mask = Image.new("1", (WIDTH, HEIGHT))
        paint_path(ImageDraw.Draw(rail_mask), points, 8, 1)
        paint_path(d, points, 8, COLORS[0])
        paint_path(d, points, 1, COLORS[3])
        mask_solid(rail_mask, 0)
    if "gardiner" in spec:
        x, y, w, h = spec["gardiner"]
        box(x, y, w, h, 0)
        solid(x, y, w, h)
        for px in range(0, WIDTH, 32):
            box(px, y + 7, 8, 2, 3)
    for route in spec["roads"]:
        points = extended_points(route, spec)
        paint_path(wd, points, WALK_HALF, 1)
        paint_path(rd, points, ROAD_HALF, 1)
    for route in spec["footpaths"]:
        paint_path(wd, extended_points(route, spec, foot=True), PATH_HALF, 1)
    # Tile-level permission agrees with the authored asphalt/sidewalk edges.
    for ty in range(TH):
        for tx in range(TW):
            i = ty * TW + tx
            x, y = tx * 8, ty * 8
            if walk_mask.getpixel((x + 4, y + 4)):
                box(x, y, 8, 8, 3)
                collisions[i] = 16
                attrs[i] = 0
            if road_mask.getpixel((x + 4, y + 4)):
                box(x, y, 8, 8, 1)
                collisions[i] = 0
                attrs[i] = 0
    # Lane dashes stop short of junctions, which carry zebra crossings.
    centres = city_kit.intersections_from_routes([extended_points(r, spec) for r in spec["roads"]])
    for route in spec["roads"]:
        for (x1, y1), (x2, y2) in zip(route["points"], route["points"][1:]):
            if y1 == y2:
                for px in range((min(x1, x2) // 32 + 1) * 32, max(x1, x2), 32):
                    if not city_kit.near_crossing(px + 4, y1, centres):
                        box(px, y1, 8, 1, 3)
            else:
                for py in range((min(y1, y2) // 32 + 1) * 32, max(y1, y2), 32):
                    if not city_kit.near_crossing(x1, py + 4, centres):
                        box(x1, py, 1, 8, 3)

    def on_road(x, y):
        return 0 <= x < WIDTH and 0 <= y < HEIGHT and road_mask.getpixel((x, y))

    def on_walk(x, y):
        return 0 <= x < WIDTH and 0 <= y < HEIGHT and walk_mask.getpixel((x, y)) and not road_mask.getpixel((x, y))
    city_kit.paint_crosswalks(box, on_road, on_walk, centres)
    # A conditional connector stays visibly closed at the district edge.
    for port in spec["conditional_ports"]:
        x, y, w, h = 0, port["y"] - WALK_HALF, 16, WALK_HALF * 2
        box(x, y, w, h, 0)
        solid(x, y, w, h)
        attr(x, y, w, h, 0)
        for py in range(y + 8, y + h, 16):
            box(4, py, 8, 2, 3)

    reserved = [(l["x"] - 8, l["y"] - 16, l["width"] + 24, l["depth"] + 32) for l in spec["landmarks"]]
    reserved += [tuple(p["rect"]) for p in spec["parks"]]
    if "pond" in spec:
        px = [p[0] for p in spec["pond"]]
        py = [p[1] for p in spec["pond"]]
        reserved.append((min(px) - 8, min(py) - 8, max(px) - min(px) + 16, max(py) - min(py) + 16))
    south = min([w[1] for w in spec["water"]] + [f["rect"][1] for f in spec["closed_frontiers"]] + [HEIGHT]) - 16

    def overlap(a, b):
        x, y, w, h = a
        xx, yy, ww, hh = b
        return x < xx + ww and xx < x + w and y < yy + hh and yy < y + h

    def may_build(x, y, w, h, landmark=False):
        if x < 32 or y < 40 or x + w > WIDTH - 32 or y + h > south:
            return False
        footprint = (x - 8, y - 8, w + 16, h + 16)
        if not landmark and any(overlap(footprint, r) for r in reserved):
            return False
        for tx, ty, i in cells(*footprint):
            if walk_mask.getpixel((tx * 8 + 4, ty * 8 + 4)) or collisions[i] == 15:
                return False
        return True

    def crown(x, y, w, h, style):
        # Towers rise over open ground to their north: the upper floors take
        # priority, so traffic and walkers pass behind them.
        if style not in (3, 4) or h < 40 or y < 48:
            return 8
        if any(collisions[i] == 15 for _, _, i in cells(x, y - 24, w, 16)):
            return 8
        city_kit.tower_crown(d, box, x, y, w, 24, style, COLORS)
        for _, _, i in cells(x, y - 24, w, 16):
            attrs[i] = city_kit.STYLE_SLOT[style] | 128
        return 24

    def building(x, y, w, h, style, kind=None, name=None):
        assert may_build(x, y, w, h, landmark=bool(name)), (slug, name, x, y, w, h)
        roof = 16 if style in (3, 4) and h >= 40 else 8
        box(x + 4, y + 4, w, h, 0)
        box(x, y, w, h, 1)
        d.rectangle((x, y, x + w - 1, y + h - 1), outline=COLORS[0])
        box(x + 2, y + 2, w - 4, max(4, h - roof - 2), 2)
        d.rectangle((x + 4, y + 4, x + w - 5, y + h - roof - 3), outline=COLORS[0])
        box(x, y - 8, w, 8, 2)
        d.line((x, y - 8, x + w - 1, y - 8), fill=COLORS[0])
        for wx in range(x + 4, x + w - 4, 8):
            box(wx, y + h - roof + 3, 4, 3, 3)
        box(x + w // 2 - 2, y + h - 5, 4, 4, 0)
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
        elif style == 4:
            for wx in range(x + 8, x + w - 8, 8):
                d.line((wx, y + 4, wx, y + h - roof - 4), fill=COLORS[3])
            box(x + 8, y + 8, 8, 8, 1)
        else:
            for wx in range(x + 8, x + w - 8, 16):
                box(wx, y + 8, 8, 8, 3)
                box(wx, y + h - 6, 8, 4, 0)
        city_kit.roof_details(d, box, x, y, w, h, roof, city_kit.seed_of(slug, x, y), COLORS, aligned=True, kinds=6)
        if kind == "carhouse":
            for wx in range(x + 8, x + w - 8, 24):
                box(wx, y + h - 16, 16, 12, 0)
                box(wx + 4, y + h - 14, 8, 8, 2)
        elif kind == "regency":
            box(x - 4, y + h - 8, w + 8, 8, 3)
            for wx in range(x, x + w, 8):
                box(wx, y + h - 8, 2, 8, 0)
            box(x + 8, y + 4, 4, 8, 0)
            box(x + w - 12, y + 4, 4, 8, 0)
        elif kind == "pavilion":
            for wx in range(x + 8, x + w - 8, 16):
                d.rectangle((wx, y + 8, wx + 7, y + h - 3), outline=COLORS[0])
            box(x + w // 2 - 8, y - 8, 16, 8, 1)
        elif kind == "junction":
            for wx in range(x + 8, x + w - 8, 16):
                box(wx, y + 2, 8, 3, 3)
        elif kind == "music_hall":
            box(x + 16, y + 16, w - 32, 8, 1)
            for wx in range(x + 8, x + w - 8, 16):
                box(wx, y + h - 12, 8, 8, 0)
        elif kind == "theatre":
            box(x + 8, y + 8, w - 16, 4, 3)
            for wx in range(x + 8, x + w - 8, 16):
                d.arc((wx, y + 12, wx + 8, y + 24), 180, 360, fill=COLORS[0], width=2)
        elif kind == "heritage_house":
            box(x + 8, y + 8, 4, 8, 0)
            box(x + w - 12, y + 8, 4, 8, 0)
            for wx in range(x + 8, x + w - 8, 16):
                box(wx, y + h - 12, 8, 6, 3)
        elif kind == "factory":
            for wx in range(x + 8, x + w - 8, 16):
                box(wx, y + 24, 8, 8, 1)
            box(x + w - 16, y - 8, 8, 16, 0)
        solid(x, y, w, h)
        # Slots 5 and 6 are the gardens and parks palettes (city_kit.STYLE_SLOT).
        attr(x, y - 8, w + 8, h + 16, city_kit.STYLE_SLOT[style], True)
        lip = crown(x, y, w, h, style)
        blocks.append({"x": x, "y": y, "width": w, "depth": h, "height": roof, "style": style,
                       "landmark": name, "kind": kind, "overhang": lip})

    def dress_building(x, y, w, h, style, k):
        look = city_kit.area_look(areas, x + w // 2, y + h // 2, k)
        if not look:
            building(x, y, w, h, style)
            return
        style, slot, awnings, signs = look
        building(x, y, w, h, style)
        lip = blocks[-1]["overhang"]
        attr(x, y - 8, w + 8, h + 16, slot, True)
        if lip > 8:
            for _, _, i in cells(x, y - lip, w, lip - 8):
                attrs[i] = slot | 128
        if awnings and style in (0, 1):
            city_kit.paint_awnings(d, box, x, y, w, h, COLORS, signs)

    for landmark in spec["landmarks"]:
        building(landmark["x"], landmark["y"], landmark["width"], landmark["depth"], landmark["style"],
                 landmark["kind"], landmark["name"])
    # The district's body shop: a spray bay in the road lane and the roll-up
    # door of the garage behind it (gameplay table: td_district_world.h).
    bay = spec["spray_bay"]
    shop = (bay["x"] - 16, bay["door_bottom"] - 40, city_kit.SPRAY_BAY_W + 32, 40)
    building(*shop, 5, "body_shop", "Body shop")
    # Park features from the layout, drawn before the generic buildings and
    # trees so neither covers them.
    no_trees = []
    for park in spec["parks"]:
        if park.get("fieldhouse"):
            fx, fy, fw, fh = park["fieldhouse"]
            building(fx, fy, fw, fh, 5, None, park["name"] + " fieldhouse")
        for feature in park.get("features", []):
            fx, fy, fw, fh = feature["rect"]
            # A feature that grew onto a widened path shrinks back clear of it.
            while fw > 24 and fh > 24 and any(walk_mask.getpixel((px, py)) for px in range(fx, fx + fw)
                                               for py in range(fy, fy + fh)):
                fx, fy, fw, fh = fx + 8, fy + 8, fw - 16, fh - 16
            feature["rect"] = [fx, fy, fw, fh]
            assert not any(walk_mask.getpixel((px, py)) for px in range(fx, fx + fw) for py in range(fy, fy + fh)), \
                (slug, feature, "park feature on a road or path")
            solid_rects, slots = city_kit.paint_park_feature(d, box, COLORS, feature["kind"], fx, fy, fw, fh)
            for r in solid_rects:
                solid(*r)
            for r, s in slots:
                attr(*r, s)
            reserved.append((fx - 8, fy - 8, fw + 16, fh + 16))
            no_trees.append((fx, fy, fw, fh))
    # Low brick terraces, pitched homes, tall glass blocks and wide work
    # sheds on a fixed 64 px grid, with smaller infill where a big one does
    # not fit; each neighbourhood builds on it in its own way.
    sizes = {0: (48, 40), 1: (56, 48), 2: (40, 40), 3: (48, 64), 4: (56, 56), 5: (64, 48)}
    for j, yy in enumerate(range(48, south, 64)):
        for i, xx in enumerate(range(40, WIDTH - 40, 64)):
            style = (i * 3 + j) % 6
            w, h = sizes[style]
            k = i + j
            if may_build(xx, yy, w, h):
                dress_building(xx, yy, w, h, style, k)
            elif may_build(xx, yy, 40, 32):
                dress_building(xx, yy, 40, 32, style if style != 3 else 0, k)
            elif may_build(xx + 8, yy + 16, 32, 24):
                dress_building(xx + 8, yy + 16, 32, 24, style if style not in (3, 4) else 2, k)
    # Broad harbour buildings and beach kiosks stop clear of the walkway.
    if spec["water"]:
        hy = (spec["water"][0][1] - 80) // 8 * 8
        for xx in range(96, WIDTH - 160, 176):
            if may_build(xx, hy, 80, 40):
                building(xx, hy, 80, 40, 0)
    assert all(road_mask.getpixel((x, y)) for x in (bay["x"], bay["x"] + city_kit.SPRAY_BAY_W - 1)
               for y in (bay["y"], bay["y"] + city_kit.SPRAY_BAY_H - 1)), (slug, "spray bay off the asphalt", bay)
    city_kit.paint_spray_bay(d, box, COLORS, bay["x"], bay["y"], bay["door_bottom"], 14)
    # Park trees on a staggered grid; they never cover paths or features.
    for park in spec["parks"]:
        x, y, w, h = park["rect"]
        for j, yy in enumerate(range((y + 7) // 8 * 8, y + h - 23, 32)):
            for xx in range((x + 7) // 8 * 8 + (16 if j % 2 else 0), x + w - 15, 32):
                if any(walk_mask.getpixel((px, py)) for py in range(yy, yy + 24) for px in range(xx, xx + 16)):
                    continue
                if any(collisions[i] == 15 for _, _, i in cells(xx, yy, 16, 24)):
                    continue
                if any(overlap((xx, yy, 16, 24), r) for r in no_trees):
                    continue
                box(xx + 6, yy + 16, 3, 8, 0)
                d.ellipse((xx, yy, xx + 15, yy + 15), fill=COLORS[1], outline=COLORS[0])
                box(xx + 4, yy + 4, 8, 8, 2)
                solid(xx, yy + 16, 16, 8)
                attr(xx, yy, 16, 16, 6, True)
                canopies.append([xx, yy, 16, 16])

    def clear(x, y, half=2, car=False):
        if not (0 <= x - half and x + half < WIDTH and 0 <= y - half and y + half < HEIGHT):
            return False
        return all(c == 0 if car else not c & 15 for _, _, i in cells(x - half, y - half, half * 2 + 1, half * 2 + 1)
                   for c in [collisions[i]])

    def sweep(points, half=8, car=True, closed=False):
        pairs = zip(points, points[1:] + points[:1] if closed else points[1:])
        for (x1, y1), (x2, y2) in pairs:
            assert x1 == x2 or y1 == y2, points
            for step in range(abs(x2 - x1) + abs(y2 - y1) + 1):
                x = x1 + (step if x2 > x1 else -step if x2 < x1 else 0)
                y = y1 + (step if y2 > y1 else -step if y2 < y1 else 0)
                assert clear(x, y, half, car), (slug, "swept clearance", x, y, half, car)

    for loop in spec["traffic_loops"]:
        assert 4 <= len(loop) <= 16
        sweep(loop, closed=True)
    for port in spec["ports"]:
        for x in range(port["x"] - 4, port["x"] + 5):
            if port["foot_only"]:
                for offset in range(-8, 9):
                    assert clear(x, port["y"] + offset), (slug, "foot seam", port, offset)
            else:
                for offset in range(-18, 19):
                    assert clear(x, port["y"] + offset, 5, True), (slug, "car seam", port, offset)
                for offset in range(-36, 37):
                    assert clear(x, port["y"] + offset), (slug, "foot seam", port, offset)
    for stop in spec["stop_candidates"]:
        assert clear(stop["x"], stop["y"], 2 if stop["foot_only"] else 8, not stop["foot_only"]), (slug, stop)
        if stop.get("parking_anchor"):
            assert clear(*stop["parking_anchor"], 8, True), (slug, stop)
    X, Y, _ = mapper(spec["id"])
    for check in (kinds or {}).get("foot_only_points", []):
        x, y = X(check[0]), Y(check[1])
        assert collisions[(y // 8) * TW + x // 8] == 16 and not clear(x, y, 5, True), (slug, "foot-only crossing", check)

    # Every port and client is connected on foot; closed edges stay solid.
    start = spec["stop_candidates"][0]
    visited = {(start["x"] // 8, start["y"] // 8)}
    queue = deque(visited)
    while queue:
        tx, ty = queue.popleft()
        for nx, ny in ((tx - 1, ty), (tx + 1, ty), (tx, ty - 1), (tx, ty + 1)):
            if 0 <= nx < TW and 0 <= ny < TH and (nx, ny) not in visited and not collisions[ny * TW + nx] & 15:
                visited.add((nx, ny))
                queue.append((nx, ny))
    for point in spec["ports"] + spec["stop_candidates"]:
        assert (point["x"] // 8, point["y"] // 8) in visited, (slug, "foot unreachable", point)

    # Lawns, canopy trees, parking and plazas on untouched walkable ground.
    ground = bytes.fromhex(COLORS[2][1:]) * 64
    reserved_lots = [(l["x"] - 8, l["y"] - 16, l["width"] + 24, l["depth"] + 32) for l in spec["landmarks"]] + no_trees

    def lot(tx, ty):
        i = ty * TW + tx
        x, y = tx * 8, ty * 8
        if collisions[i] != 16 or attrs[i] != 6 or walk_mask.getpixel((x + 4, y + 4)):
            return False
        if any(rx <= x + 4 < rx + rw and ry <= y + 4 < ry + rh for rx, ry, rw, rh in reserved_lots):
            return False
        return img.crop((x, y, x + 8, y + 8)).tobytes() == ground

    def set_attr(tx, ty, value):
        attrs[ty * TW + tx] = value
    before = list(collisions)
    canopies += city_kit.dress_lots(d, box, TW, TH, lot, set_attr, COLORS, slug, [tuple(p["rect"]) for p in spec["parks"]])
    assert collisions == before, "lot decoration must not change collision"
    paint_streetcar_stops(d, spec["id"], COLORS, lambda u, v: (X(u), Y(v)))
    # Sidewalk slabs (curb, joints) and street furniture, last so only plain
    # sidewalk is touched; not beside the expressway or rail embankments.
    barriers = [(spec["gardiner"][1], spec["gardiner"][3])] if "gardiner" in spec else []
    slabs = city_kit.detail_sidewalks(img, d, TW, TH, collisions, attrs, COLORS,
                                      lambda tx, ty: not any(ry - 16 <= ty * 8 < ry + rh + 16 for ry, rh in barriers))
    slab_tile = {k: img.crop((k[0] * 8, k[1] * 8, k[0] * 8 + 8, k[1] * 8 + 8)).tobytes() for k in slabs}

    def busy(x, y):
        return city_kit.AREA_LOOKS.get(city_kit.area_at(areas, x, y), {}).get("awnings", False)
    city_kit.place_furniture(img, d, box, slabs, slab_tile, attrs, TW, canopies, busy, COLORS)

    def is_water(x, y):
        return any(wx <= x < wx + ww and wy <= y < wy + wh for wx, wy, ww, wh in spec["water"]) or \
            bool(pond_mask and pond_mask.getpixel((x, y)))
    wet_tiles = set()
    water_tiles = city_kit.texture_water(img, attrs, TW, is_water, COLORS, wet_tiles)

    # Tree species, hedges, fences, bushes and lawn patches (greenery.py).
    def park_at(x, y):
        return next((p["name"] for p in spec["parks"]
                     if p["rect"][0] <= x < p["rect"][0] + p["rect"][2] and p["rect"][1] <= y < p["rect"][1] + p["rect"][3]), None)

    def kind_at(x, y):
        if park_at(x, y):
            return "park"
        look = city_kit.AREA_LOOKS.get(city_kit.area_at(areas, x, y))
        if not look:
            return None
        styles = set(look["styles"])
        return "shops" if look.get("awnings") else "houses" if 2 in styles else \
            "towers" if styles & {3, 4} else "works" if 5 in styles else None
    greens = greenery.dress(img, attrs, collisions, TW, canopies, COLORS, slug, spec["id"], kind_at, park_at, is_water)
    assert set(img.get_flattened_data()) <= set(tuple(bytes.fromhex(c[1:])) for c in COLORS)
    for block in blocks:
        assert all(collisions[i] == 15 for _, _, i in cells(block["x"], block["y"], block["width"], block["depth"])), block
    city_kit.mark_water(collisions, wet_tiles)
    assert len(collisions) == TW * TH and set(collisions) <= {0, 16, 15, city_kit.WATER_TILE}
    assert all((a & 7) <= 6 for a in attrs)
    assert all((a & 7) <= 6 and not (a & 128) for a, c in zip(attrs, collisions) if c == 0)
    scenes = city_kit.split_scenes(img, attrs, collisions, TW, spec["id"])
    metadata = {
        "id": spec["id"], "slug": slug, "name": spec["name"],
        "projection": "Original compressed north-up plan (1x, see 'plan') drawn at double scale by world2x.py; not GIS coordinates",
        "dimensions": [WIDTH, HEIGHT], "tile_dimensions": [TW, TH],
        "road_half_width": ROAD_HALF, "walk_half_width": WALK_HALF, "path_half_width": PATH_HALF,
        "plan": plan, "world": {k: spec[k] for k in ("roads", "footpaths", "ports", "parks", "landmarks",
                                                       "spray_bay", "stop_candidates", "traffic_loops")},
        "scenes": [{k: sc[k] for k in ("slug", "district", "origin", "tiles")} for sc in scenes],
        "blocks": blocks, "canopies": canopies,
        "collision_rules": {"road": 0, "foot_only": 16, "solid": 15},
        "greenery": greens,
        "validation": {"animated_water_tiles": water_tiles, "traffic_loops": len(spec["traffic_loops"]),
                       "all_foot_clients_and_ports_connected": True},
    }
    return img, attrs, collisions, scenes, metadata


def write(plan, areas, check, extra=None, kinds=None):
    """Render one district and write (or check) its scenes and metadata."""
    img, attrs, collisions, scenes, metadata = render(plan, areas, kinds)
    if extra:
        metadata.update(extra)
    for sc in scenes:
        assert sc["tiles"] <= city_kit.SCENE_TILE_BUDGET, (sc["slug"], "background patterns", sc["tiles"])
    texts = {ROOT / "content/districts" / f"{plan['slug']}_art.json": json.dumps(metadata, indent=1) + "\n"}
    images = {}
    for sc in scenes:
        texts[PROJECT / f"original-art/{sc['slug']}_attributes.json"] = json.dumps(sc["attrs"]) + "\n"
        texts[ROOT / f"content/scenes/{sc['slug']}.json"] = json.dumps({"collisions": world2x.encode_grid(sc["collisions"])}) + "\n"
        images[PROJECT / f"assets/backgrounds/{sc['slug']}.png"] = sc["image"]
    if check:
        for path, im in images.items():
            with Image.open(path) as current:
                assert current.convert("RGB").tobytes() == im.tobytes(), f"Stale art: {path.name}"
        for path, text in texts.items():
            assert path.read_text() == text, f"Stale art output: {path.relative_to(ROOT)}"
        print(f"{plan['slug']}: matches its generator (scene tiles {[sc['tiles'] for sc in scenes]})")
        return metadata
    for path, im in images.items():
        im.save(path)
    for path, text in texts.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
    print(f"{plan['slug']}: {len(metadata['blocks'])} buildings at {WIDTH}x{HEIGHT}, scene tiles {[sc['tiles'] for sc in scenes]}")
    return metadata


def preview(plan, areas, out):
    img, attrs, collisions, scenes, metadata = render(plan, areas)
    img.save(out)
    Path(out + ".attrs.json").write_text(json.dumps(attrs))
    print(plan["slug"], "scene tiles", [sc["tiles"] for sc in scenes], "buildings", len(metadata["blocks"]))
    return scenes


def png_sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()
