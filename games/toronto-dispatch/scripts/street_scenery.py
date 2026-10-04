"""Original reusable street details painted into existing background tiles.

These are decorative game pixels, not surveyed Toronto furniture. Registered
roads, sidewalks, public paths, service points, building footprints, palette
bytes and collision bytes remain authoritative. No objects, VRAM animation,
new collision or runtime state is added by this layer.
"""
from collections import Counter
from PIL import Image, ImageDraw

COLORS = ("#071821", "#306850", "#86c06c", "#e0f8cf")
RGB = tuple(tuple(bytes.fromhex(c[1:])) for c in COLORS)

# Whole 8px cells repeat exactly, keeping decorative detail inexpensive in ROM.
ROOFS = (
    ("22222222", "20000002", "20333302", "20300302", "20333302", "20000002", "22222222", "22222222"),
    ("22222222", "20000002", "20303002", "20000002", "20303002", "20000002", "22222222", "22222222"),
    ("21222222", "22122222", "22212222", "22221222", "22222122", "22222212", "22222221", "12222222"),
    ("22222222", "20000002", "20111102", "20133102", "20133102", "20111102", "20000002", "22222222"),
    ("23232322", "20000002", "20330302", "20000002", "20330302", "20000002", "23232322", "22222222"),
    ("22222222", "20000002", "20333302", "20311302", "20311302", "20333302", "20000002", "22222222"),
)
PROPS = {
    "street_sign": ("22222222", "20000002", "20333002", "20000002", "22202222", "22202222", "22202222", "22202222"),
    "park_sign": ("22222222", "20000002", "20323002", "20232002", "20000002", "22202222", "22202222", "22202222"),
    "bin": ("22222222", "22000022", "20333302", "20111102", "20101102", "20101102", "20000002", "22222222"),
    "bollard": ("22222222", "22233222", "22200222", "22233222", "22200222", "22200222", "22200222", "22000022"),
    "flowers": ("22222222", "23323322", "21121122", "23223222", "20000002", "20111102", "20000002", "22222222"),
    "bench": ("22222222", "20000002", "23333332", "20000002", "23333332", "20222202", "20222202", "22222222"),
    "yard_crate": ("22222222", "20000002", "20333302", "20000002", "20333302", "20000002", "20333302", "20000002"),
}


def _tile(rows):
    tile = Image.new("RGB", (8, 8))
    tile.putdata([RGB[int(c)] for row in rows for c in row])
    return tile


def _route_mask(size, spec):
    """Protect full authored corridors, rather than only route centrelines."""
    mask = Image.new("1", size)
    draw = ImageDraw.Draw(mask)
    for group, default in (("roads", 32), ("footpaths", 16)):
        for route in spec.get(group, []):
            half = 32 if group == "roads" else route.get("half_width", default)
            points = route.get("points", [])
            for (x, y), (u, v) in zip(points, points[1:]):
                assert x == u or y == v
                draw.rectangle((min(x,u)-half, min(y,v)-half,
                                max(x,u)+half-1, max(y,v)+half-1), fill=1)
    for key in ("ports", "conditional_ports", "stop_candidates"):
        for point in spec.get(key, []):
            x, y = point["x"], point["y"]
            draw.rectangle((x-24,y-24,x+23,y+23), fill=1)
    for block in spec.get("blocks", []):
        x,y,w,h = (block[k] for k in ("x","y","width","depth"))
        draw.rectangle((x-8,y-16,x+w+15,y+h+15), fill=1)
    return mask


def decorate(image, collisions, attrs, blocks, canopies, slug, spec=None):
    """Add repeatable architecture/furniture without editing terrain or attributes."""
    spec = dict(spec or {})
    width, height = image.size
    tw = width // 8
    assert len(collisions) == len(attrs) == width//8 * (height//8)
    terrain_before, attrs_before = tuple(collisions), tuple(attrs)
    counts = Counter()
    placements = []

    def stamp(kind, x, y, tile):
        assert x % 8 == y % 8 == 0 and 0 <= x <= width-8 and 0 <= y <= height-8
        image.paste(tile, (x,y))
        counts[kind] += 1
        placements.append({"kind":kind,"x":x,"y":y})

    roofs = tuple(_tile(r) for r in ROOFS)
    landmarks=[(b['x']-8,b['y']-16,b['x']+b['width']+8,b['y']+b['depth']+8)
               for b in blocks if b.get('landmark') or b.get('kind')]
    # Preserve the landmark silhouettes and add modules only inside solid roofs.
    # Existing north lips, window rows, doorways and all footprint edges remain.
    for block in blocks:
        if block.get("landmark") or block.get("kind"):
            continue
        x,y,w,h = (block[k] for k in ("x","y","width","depth"))
        if w < 24 or h < 24:
            continue
        authored_style=block["style"]
        style=(authored_style if isinstance(authored_style,int)
               else {"cottage":2,"pavilion":3,"lighthouse":4}.get(authored_style,0)) % len(roofs)
        for px in range(x+8,x+w-8,16):
            py = y+8
            if any(px<right and left<px+8 and py<bottom and top<py+8
                   for left,top,right,bottom in landmarks):
                continue
            if all(collisions[ty*tw+tx] == 15
                   for ty in range(py//8,(py+7)//8+1)
                   for tx in range(px//8,(px+7)//8+1)):
                stamp("roof_module", px, py, roofs[style])

    protected = _route_mask(image.size, {**spec,"blocks":blocks})
    props = {name:_tile(rows) for name,rows in PROPS.items()}
    # Ground under trees already has a solid trunk. A small bench shares that
    # solid cell and leaves the crown, trunk and collision silhouette intact.
    for n,(x,y,w,h) in enumerate(canopies):
        if n % 4:
            continue
        px,py = (x//8*8), ((y+h)//8*8)
        if not (0 <= px < width and 0 <= py < height):
            continue
        i=py//8*tw+px//8
        if collisions[i] == 15 and not attrs[i]&128:
            stamp("bench",px,py,props["bench"])

    families = {
        "city": ("street_sign","bin","flowers","bench","bollard","park_sign"),
        "west": ("street_sign","flowers","bin","park_sign"),
        "high_park": ("park_sign","flowers","bench","bin"),
        "east": ("flowers","street_sign","bin","bench"),
        "port_lands": ("yard_crate","bollard","park_sign","bin"),
        "islands": ("park_sign","flowers","bench","bollard"),
        "north": ("flowers","park_sign","street_sign","bin"),
    }
    kinds=families[slug]
    # Only untouched grass outside full roads/paths/clients accepts new props.
    # Neighbouring pavement or park foliage keeps them near places players see.
    for py in range(40,height-32,32):
        for px in range(40,width-32,32):
            i=py//8*tw+px//8
            # A source shade is not a terrain type: Core uses shade2/palette0
            # for Don/harbour water. Only actual public grass (foot16/green6)
            # may receive these cosmetic garden props; solid tree bases above
            # are the explicit bench exception.
            if collisions[i]!=16 or (attrs[i]&7)!=6 or attrs[i]&128 or protected.crop((px,py,px+8,py+8)).getbbox():
                continue
            if set(image.crop((px,py,px+8,py+8)).getdata()) != {RGB[2]}:
                continue
            near_pavement = any(
                collisions[(py+dy)//8*tw+(px+dx)//8]==0
                or image.getpixel((px+dx,py+dy))==RGB[3]
                for dx,dy in ((-32,0),(32,0),(0,-32),(0,32))
                if 0 <= px+dx < width and 0 <= py+dy < height)
            if not near_pavement:
                continue
            kind=kinds[(px//32+py//32)%len(kinds)]
            stamp(kind,px,py,props[kind])

    assert tuple(collisions)==terrain_before and tuple(attrs)==attrs_before
    return {"source":"scripts/street_scenery.py","original_pixels":True,
            "decorative_game_placements_not_surveyed":True,"counts":dict(sorted(counts.items())),
            "placements":placements,"new_oam_objects":0,"new_runtime_state_bytes":0,
            "collision_bytes_changed":0,"palette_priority_bytes_changed":0,
            "building_footprints_changed":0,"formal_corridors_protected":True}
