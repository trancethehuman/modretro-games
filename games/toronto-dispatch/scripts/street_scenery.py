"""Original reusable street details painted into existing background tiles.

These are decorative game pixels, not surveyed Toronto furniture. Registered
roads, sidewalks, public paths, service points, building footprints, palette
bytes and collision bytes remain authoritative. No objects, VRAM animation,
raw collision bytes are added by this layer. Reachable furniture is registered
for the bounded native destruction overlay; structural terrain stays solid.
"""
from collections import Counter
import json
from pathlib import Path
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
    "fence": ("22222222","20202022","20000002","20303032","20000002","20202022","20202022","22222222"),
    "light_pole": ("22000022","20333302","22000022","22200222","22200222","22200222","22200222","22000022"),
    "street_sign": ("22222222", "20000002", "20333002", "20000002", "22202222", "22202222", "22202222", "22202222"),
    "park_sign": ("22222222", "20000002", "20323002", "20232002", "20000002", "22202222", "22202222", "22202222"),
    "bin": ("22222222", "22000022", "20333302", "20111102", "20101102", "20101102", "20000002", "22222222"),
    "bollard": ("22222222", "22233222", "22200222", "22233222", "22200222", "22200222", "22200222", "22000022"),
    "flowers": ("22222222", "23323322", "21121122", "23223222", "20000002", "20111102", "20000002", "22222222"),
    "bench": ("22222222", "20000002", "23333332", "20000002", "23333332", "20222202", "20222202", "22222222"),
    "yard_crate": ("22222222", "20000002", "20333302", "20000002", "20333302", "20000002", "20333302", "20000002"),
    "bus_stop": ("33333333", "30000003", "30333003", "30303003", "30000003", "33303333", "33303333", "33000333"),
    "crossing_sign": ("33333333", "33000333", "30333033", "30303033", "30000033", "33303333", "33303333", "33000333"),
}
FACADES = (
    ("00000000","33313331","33313331","00000000","11111111","10111011","11111111","00000000"),
    ("01010101","33303330","33303330","11111111","11011101","11101110","11111111","00000000"),
    ("11111111","33033303","33033303","00000000","11111111","11111111","11111111","00000000"),
    ("33333333","30133013","30133013","30133013","33333333","11111111","10111011","00000000"),
    ("30303030","32323232","30303030","32323232","30303030","32323232","30303030","00000000"),
    ("11111111","03300330","03300330","03300330","00000000","11111111","01010101","00000000"),
)
# One reusable wave tile plus an offset variant. These stay ordinary background
# pixels; integration can animate this exact pattern without adding scene actors.
WATER_WAVES = (
    ("22222222","22222222","23332222","22233322","22222222","22222222","22112222","22221122"),
    ("22222222","22222222","22223332","23332222","22222222","22222222","22221122","22112222"),
)
SHOP_ENTRANCES = {
    "city": {"key":"grocery","district":0,"x":124,"y":152,"marker_x":120,"marker_y":144,
             "building":[112,96,24,40],"name":"HARBOUR GROCER"},
    "west": {"key":"corner_store","district":1,"x":64,"y":180,"marker_x":56,"marker_y":176,
             "building":[48,144,32,24],"name":"WEST END CORNER"},
    "east": {"key":"repair_shop","district":3,"x":768,"y":148,"marker_x":760,"marker_y":144,
             "building":[752,112,32,24],"name":"EAST END REPAIRS"},
}
FIRE_GARAGE={"district":0,"building":[240,464,24,24],"sign":[248,472,16,8],
             "name":"Fictional Queen West fire garage",
             "notice":"Original fictional service building; not a surveyed Toronto Fire Services station",
             "pedestrian_route_id":57,"worker_variant":1,"route_start":[240,500],"route_length":63}


def _junctions(spec, road_at):
    """Intersections of actual authored road segments, including T junctions."""
    segments = [tuple(a+b) for route in spec.get("roads", [])
                for a,b in zip(route["points"],route["points"][1:])]
    found = set()
    for x,y,u,v in segments:
        for a,b,c,d in segments:
            if y == v and a == c and min(x,u) <= a <= max(x,u) and min(b,d) <= y <= max(b,d):
                found.add((a,y))
    result=[]
    for x,y in sorted(found,key=lambda p:(p[1],p[0])):
        if x%8 or y%8 or not road_at(x,y):continue
        arms=[name for name,dx,dy in (("north",0,-48),("east",48,0),("south",0,48),("west",-48,0))
              if road_at(x+dx,y+dy)]
        if len(arms)>=3:result.append({"x":x,"y":y,"arms":arms})
    return result


def _water_mask(size, slug, spec):
    mask=Image.new("1",size);draw=ImageDraw.Draw(mask)
    if slug=="city":
        draw.rectangle((872,24,911,815),fill=1)
        draw.rectangle((0,816,size[0]-1,size[1]-1),fill=1)
    elif slug=="islands":
        draw.rectangle((0,0,size[0]-1,size[1]-1),fill=1)
        for land in spec.get("landforms",[]):draw.polygon([tuple(p) for p in land["polygon"]],fill=0)
    else:
        for water in spec.get("water",[]):
            x,y,w,h=water["rect"] if isinstance(water,dict) else water
            draw.rectangle((x,y,x+w-1,y+h-1),fill=1)
        if "pond" in spec:draw.polygon([tuple(p) for p in spec["pond"]],fill=1)
    return mask


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
    destructible = {}

    def stamp(kind, x, y, tile):
        assert x % 8 == y % 8 == 0 and 0 <= x <= width-8 and 0 <= y <= height-8
        i=y//8*tw+x//8
        if kind in PROPS or kind == "streetcar_stop":
            actual_kind="bus_stop" if kind == "streetcar_stop" else kind
            if not collisions[i]&15 and not attrs[i]&128:
                underlay=image.crop((x,y,x+8,y+8))
                rows=["".join(str(RGB.index(underlay.getpixel((px,py)))) for px in range(8)) for py in range(8)]
                destructible[(x,y)]={"kind":actual_kind,"x":x,"y":y,"ground_rows":rows,"palette":attrs[i]&7}
        image.paste(tile, (x,y))
        counts[kind] += 1
        placements.append({"kind":kind,"x":x,"y":y})

    roofs = tuple(_tile(r) for r in ROOFS)
    facades = tuple(_tile(r) for r in FACADES)
    landmarks=[(b['x']-8,b['y']-16,b['x']+b['width']+8,b['y']+b['depth']+8)
               for b in blocks if b.get('landmark') or b.get('kind')]
    # Preserve the landmark silhouettes and add modules only inside solid roofs.
    # Existing north lips, window rows, doorways and all footprint edges remain.
    for n,block in enumerate(blocks):
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
                stamp("roof_module", px, py, roofs[(style+n%2)%len(roofs)])
        # Existing dimensions and north lips retain depth/collision. Distinct
        # store windows, brick bays, terraces, glass grids and loading doors
        # fill repeated whole cells; leave the original central doorway clear.
        py=y+h-8
        for px in range(x+8,x+w-8,8):
            if px<=x+w//2<px+8:continue
            if all(collisions[ty*tw+tx]==15
                   for ty in range(py//8,(py+7)//8+1)
                   for tx in range(px//8,(px+7)//8+1)):
                stamp("facade_module",px,py,facades[style])

    crew=[]
    if slug=='city':
        # A real authored sidewalk identity already uses the approved helmet
        # worker sheet. Give its ordinary walking activity a clear service
        # context without creating an NPC, changing a landmark or moving land.
        x,y,w,h=FIRE_GARAGE['building']
        assert any([b[k] for k in ('x','y','width','depth')]==[x,y,w,h]
                   and not b.get('landmark') for b in blocks)
        sx,sy,sw,sh=FIRE_GARAGE['sign']
        assert all(collisions[ty*tw+tx]==15 for ty in range(sy//8,(sy+sh-1)//8+1)
                   for tx in range(sx//8,(sx+sw-1)//8+1))
        letters={'F':('111','100','110','100','100'),
                 'I':('111','010','010','010','111'),
                 'R':('110','101','110','101','101'),
                 'E':('111','100','110','100','111')}
        label=Image.new('RGB',(sw,sh),RGB[0]);ink=ImageDraw.Draw(label)
        for n,char in enumerate('FIRE'):
            for py,row in enumerate(letters[char]):
                for px,pixel in enumerate(row):
                    if pixel=='1':ink.point((n*4+px,py+1),fill=RGB[3])
        stamp('fire_garage_sign',sx,sy,label)
        crew.append(dict(FIRE_GARAGE))

    protected = _route_mask(image.size, {**spec,"blocks":blocks})
    props = {name:_tile(rows) for name,rows in PROPS.items()}
    def clear(x,y,half=0,road_only=False):
        if not(0<=x-half and x+half<width and 0<=y-half and y+half<height):return False
        return all(collisions[ty*tw+tx]==0 if road_only else not(collisions[ty*tw+tx]&15)
                   for ty in range((y-half)//8,(y+half)//8+1)
                   for tx in range((x-half)//8,(x+half)//8+1))

    junctions=_junctions(spec,lambda x,y:clear(x,y,2,True))
    crossing=_tile(("13333111",)*8)
    crossing_vertical=crossing.transpose(Image.Transpose.ROTATE_90)
    for junction in junctions:
        x,y=junction["x"],junction["y"]
        for arm in junction["arms"]:
            horizontal=arm in ("north","south")
            # Runtime stops a centre24px from the junction. A5–7px vehicle
            # nose stays behind the17–18px stop bar and8–16px zebra stripes.
            offset=-16 if arm in ("north","west") else 8
            for across in range(-24,24,8):
                px,py=(x+across,y+offset) if horizontal else (x+offset,y+across)
                if clear(px+4,py+4,3,True):stamp("crosswalk",px,py,crossing if horizontal else crossing_vertical)
            # Short solid stop lines only in the entering right-hand lane.
            line_offset=-24 if arm in ("north","west") else 16
            rows=(("11111111",)*6+("33333333",)*2 if line_offset<0
                  else ("11111111",)+( "33333333",)*2+("11111111",)*5)
            stopline=_tile(rows)
            for across in (0,8,16) if arm in ("south","west") else (-24,-16,-8):
                px,py=(x+across,y+line_offset) if horizontal else (x+line_offset,y+across)
                tile=stopline if horizontal else stopline.transpose(Image.Transpose.TRANSPOSE)
                if clear(px+4,py+4,3,True):stamp("stop_line",px,py,tile)
        # Sign sprites are background cells on the curb, independent of the
        # existing runtime two-head traffic lights. No stationary OBJ is added.
        for dx,dy in ((-32,-32),(24,24)):
            px,py=x+dx,y+dy
            if clear(px+4,py+4,3) and not attrs[py//8*tw+px//8]&128:
                stamp("crossing_sign",px,py,props["crossing_sign"])

    if slug=="city":
        for n,junction in enumerate(junctions):
            px,py=junction["x"]-32,junction["y"]+24
            if clear(px+4,py+4,3) and not attrs[py//8*tw+px//8]&128:
                kind="light_pole" if n%2 else "fence"
                stamp(kind,px,py,props[kind])

    waves=tuple(_tile(rows) for rows in WATER_WAVES)
    water=_water_mask(image.size,slug,spec)
    for py in range(0,height,8):
        for px in range(0,width,8):
            i=py//8*tw+px//8
            if collisions[i]==15 and attrs[i]==0 and water.crop((px,py,px+8,py+8)).getextrema()==(1,1):
                stamp("water_wave",px,py,waves[(px//32+py//24)&1])

    # Fictional curbside bays are deliberately separated from junctions and
    # client/portal handoffs. Positions are candidates for integration's parked
    # vehicle state, not decorative cars pretending to be enterable actors.
    parking=[]
    segments=[(a,b) for route in spec.get("roads",[]) for a,b in zip(route["points"],route["points"][1:])]
    for a,b in segments:
        if len(parking)>=8:break
        before=len(parking)
        x,y=a;u,v=b
        horizontal=y==v
        distance=abs(u-x)+abs(v-y)
        if distance<128:continue
        for along in range(64,distance-47,32):
            cx=x+(along if u>x else -along if u<x else 0)
            cy=y+(along if v>y else -along if v<y else 0)
            if any(abs(cx-j["x"])<48 and abs(cy-j["y"])<48 for j in junctions):continue
            for side in (-24,24):
                bx,by=(cx,cy+side) if horizontal else (cx+side,cy)
                if not clear(bx,by,8) or any(abs(bx-p["x"])<48 and abs(by-p["y"])<48 for p in parking):continue
                if any(abs(bx-p["x"])<32 and abs(by-p["y"])<32
                       for group in ("ports","conditional_ports","stop_candidates") for p in spec.get(group,[])):continue
                parking.append({"x":bx,"y":by,"heading":0 if horizontal else 4,"kind":"curb_bay"})
                # Align bay markings to native tile cells; 32x16 east/west,
                # 16x32 north/south. Keep lane centres unobscured.
                px=(bx//8)*8-(16 if horizontal else 8);py=(by//8)*8-(8 if horizontal else 16)
                w,h=(32,16) if horizontal else (16,32)
                draw=ImageDraw.Draw(image)
                draw.rectangle((px,py,px+w-1,py+h-1),outline=RGB[3])
                counts["parking_bay"]+=1
                placements.append({"kind":"parking_bay","x":px,"y":py,"width":w,"height":h})
                if len(parking)>=8 or len(parking)-before>=2:break
            if len(parking)>=8 or len(parking)-before>=2:break

    lots=[]
    # A small original parking court on otherwise unused public ground. The
    # same grass palette has the shared asphalt shade, so no attribute changes
    # are needed. Courts join an existing sidewalk rather than creating roads.
    if slug not in ("city","islands"):
        for py in range(48,height-80,16):
            if lots:break
            for px in range(48,width-112,16):
                cells=[(yy//8)*tw+xx//8 for yy in range(py,py+32,8) for xx in range(px,px+64,8)]
                if not all(collisions[i]==16 and attrs[i]==6 for i in cells):continue
                if protected.crop((px,py,px+64,py+32)).getbbox():continue
                if not any(clear(x,y,2,True) for x,y in ((px-16,py+16),(px+80,py+16),(px+32,py-16),(px+32,py+48))):continue
                draw=ImageDraw.Draw(image);draw.rectangle((px,py,px+63,py+31),fill=RGB[1],outline=RGB[3])
                for sx in range(px+16,px+64,16):draw.line((sx,py+1,sx,py+15),fill=RGB[3])
                lots.append({"x":px,"y":py,"width":64,"height":32,"fictional":True})
                counts["parking_lot"]+=1
                placements.append({"kind":"parking_lot",**lots[-1]})
                break

    # Clearly identified bus stop poles and nearby benches at authored service
    # handoffs; the native stop and timetable remain the gameplay authority.
    district={"city":0,"west":1,"high_park":2,"east":3,"port_lands":4,"islands":5,"north":6}[slug]
    campaign=json.loads((Path(__file__).resolve().parents[1]/'content/campaign.json').read_text())
    service_points=[{'x':s['u'],'y':s['v'],'transit':s['transit']} for s in campaign['stops']
                    if s['district']==district and s['transit'] in (2,4)]
    for point in service_points:
        for dx,dy in ((24,-32),(-32,-32),(24,24),(-32,24),(16,16),(-24,16),(16,-24)):
            px=((point["x"]+dx)//8)*8;py=((point["y"]+dy)//8)*8
            if clear(px+4,py+4,3) and not attrs[py//8*tw+px//8]&128 and collisions[py//8*tw+px//8]==16:
                stamp("bus_stop" if point['transit']==2 else "streetcar_stop",px,py,props["bus_stop"])
                break
    entrances=[]
    if slug in SHOP_ENTRANCES:
        entrance=SHOP_ENTRANCES[slug]
        assert clear(entrance["x"],entrance["y"],5),("Blocked shop approach",slug)
        marker=_tile(("33333333","33000333","33030333","33030333","33000333","33303333","33000333","33303333"))
        stamp("shop_entrance",entrance["marker_x"],entrance["marker_y"],marker)
        entrances.append(dict(entrance))
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
        "west": ("street_sign","flowers","bin","park_sign","fence","light_pole"),
        "high_park": ("park_sign","flowers","bench","bin","fence","light_pole"),
        "east": ("flowers","street_sign","bin","bench","fence","light_pole"),
        "port_lands": ("yard_crate","bollard","park_sign","bin","fence","light_pole"),
        "islands": ("park_sign","flowers","bench","bollard","fence","light_pole"),
        "north": ("flowers","park_sign","street_sign","bin","fence","light_pole"),
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

    # A later doorway/marking wins over an earlier prop. Register only final
    # visible furniture, never solid tree-base benches or roof-priority cells.
    destroyables=[p for (x,y),p in sorted(destructible.items(),key=lambda item:(item[0][1],item[0][0]))
                  if image.crop((x,y,x+8,y+8)).tobytes()==props[p["kind"]].tobytes()]
    assert len(destroyables)<=256,(slug,len(destroyables))
    for index,p in enumerate(destroyables):p["id"]=index
    assert tuple(collisions)==terrain_before and tuple(attrs)==attrs_before
    return {"source":"scripts/street_scenery.py","original_pixels":True,
            "decorative_game_placements_not_surveyed":True,"counts":dict(sorted(counts.items())),
            "placements":placements,"new_oam_objects":0,"new_runtime_state_bytes":0,
            "destructible_furniture":destroyables,"destruction_runtime":"td_scenery.c shared bounded transient state; buildings/trees/shoreline remain solid",
            "collision_bytes_changed":0,"palette_priority_bytes_changed":0,
            "building_footprints_changed":0,"formal_corridors_protected":True,
            "junctions":junctions,"parked_vehicle_candidates":parking,"parking_lots":lots,"shop_entrances":entrances,
            **({"ambient_fire_crew":crew} if crew else {}),
            "water_wave_frames":[list(rows) for rows in WATER_WAVES],
            "water_animation":"Static background waves; moving boat wake belongs to native boat module"}
