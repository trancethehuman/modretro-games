"""Generate/check original, car-free Toronto Islands source art.

This is an original north-up compression of researched public relationships,
not a raster/geometry conversion of a City map. Source assets and the manifest
are written; an already registered background receives the same source PNG.
Native registration, gameplay, save migration,
compiled budgets and hardware remain separate gates owned by integration.
"""
import argparse
import hashlib
import io
import json
from sync_city_resources import compress
from collections import deque
from pathlib import Path

from PIL import Image, ImageDraw
from street_scenery import decorate

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "project/original-art"
WIDTH, HEIGHT = 1024, 976
TW, TH = WIDTH // 8, HEIGHT // 8
COLORS = ["#071821", "#306850", "#86c06c", "#e0f8cf"]
FOOT_HALF = 5
LANDING_HALF = 3


def path(name, points, half=16, **extra):
    return {"name": name, "points": points, "half_width": half, **extra}


RESEARCH = {
    "reviewed_on": "2026-10-03",
    "source_plan": "docs/ISLAND_DISTRICT_PLAN.md",
    "map_era": "Representative established public topology, not an adopted 2026 game era, live access guide or ferry timetable.",
    "source_registry_ids": [
        "islands-wayfinding-202609-reviewed20261003",
        "islands-existing-context-202211-reviewed20261003",
        "islands-existing-versus-proposed-bridges-20261003",
        "islands-public-access-20261003",
        "islands-landmark-path-guide-20261003",
        "islands-ferry-service-20261003",
        "islands-project-status-20261003",
    ],
    "references": [
        "https://www.toronto.ca/wp-content/uploads/2026/09/8ff1-Toronto-Island-Park-mapcompressed.pdf",
        "https://www.toronto.ca/wp-content/uploads/2022/11/91bd-toronto-island-park-master-plan-open-house-info-panel-nov-30-2022-03.pdf",
        "https://www.toronto.ca/legdocs/mmis/2024/ie/bgrd/backgroundfile-246911.pdf",
        "https://www.toronto.ca/explore-enjoy/toronto-island-ferries/getting-around/",
        "https://www.toronto.ca/explore-enjoy/toronto-island-ferries/things-to-do-on-toronto-island/",
    ],
    "attribution": "Public place names and relative established path/dock/bridge relationships researched from City of Toronto sources. Original artwork and game compression; no endorsement.",
    "rights": "The City wayfinding map is copyrighted and includes OpenStreetMap credits. No source map, photograph, logo, public artwork, raw geometry or altered map is included in these assets.",
    "limitations": [
        "The September 2026 map's printed overview is south-up; this original scene follows its north arrow instead.",
        "All polygons, building footprints, widths, fictional courier handoffs and distances are original compressed design, not surveyed entrances or parcel boundaries.",
        "Airport/tunnel, water-treatment grounds, private yacht-club islands and marked protected dunes stay inaccessible.",
        "Only established Manitou, Algonquin and Snake footbridge relationships are represented; Olympic is omitted and proposed Lagoon Loop/Gibraltar bridges are not drawn.",
        "No mainland land connection or ordinary road vehicles; source art does not implement ferry migration or native scene access.",
    ],
}

ISLAND = {
    "id": 5, "slug": "islands", "name": "Toronto Islands public paths",
    "landforms": [
        {"name": "Hanlan park and Gibraltar western shore", "polygon": [[64,248],[280,248],[280,312],[336,312],[336,408],[280,408],[280,544],[336,544],[336,704],[392,704],[392,800],[280,800],[280,856],[176,856],[176,800],[112,800],[112,656],[80,656],[80,424],[64,424]]},
        {"name": "Centre landing and attractions", "polygon": [[400,416],[608,416],[608,448],[632,448],[632,528],[608,528],[608,560],[416,560],[416,528],[400,528]]},
        {"name": "Centre southern park and beach", "polygon": [[328,632],[632,632],[632,648],[712,648],[712,680],[768,680],[768,816],[600,816],[600,864],[464,864],[464,840],[344,840],[344,800],[304,800],[304,736],[328,736]]},
        {"name": "Cibola eastern strip", "polygon": [[632,592],[744,592],[744,544],[816,544],[816,488],[896,488],[896,520],[968,520],[968,600],[896,600],[896,648],[824,648],[824,720],[768,720],[768,776],[712,776],[712,680],[632,680]]},
        {"name": "Ward settlement", "polygon": [[872,288],[960,288],[960,320],[992,320],[992,528],[968,528],[968,568],[880,568],[880,520],[856,520],[856,384],[872,384]]},
        {"name": "Algonquin Island", "polygon": [[752,328],[840,328],[840,352],[864,352],[864,408],[840,408],[840,448],[744,448],[744,408],[728,408],[728,360],[752,360]]},
        {"name": "Snake Island", "polygon": [[648,448],[720,448],[720,472],[744,472],[744,512],[720,512],[720,536],[648,536]]},
    ],
    "footpaths": [
        path("Hanlan landing approach", [[320,280],[320,344],[208,344]]),
        path("Lakeshore Avenue", [[208,344],[208,592],[272,592],[272,760],[384,760],[384,704],[688,704]]),
        path("Cibola Avenue", [[688,704],[688,624],[776,624],[776,584],[848,584],[848,528],[928,528],[928,312],[920,312],[920,280]]),
        path("Avenue of the Islands", [[512,448],[512,832]]),
        path("Centre landing garden walk", [[448,448],[560,448],[560,496],[512,496]]),
        path("Centre pavilion approach", [[512,744],[536,744]], half=8),
        path("Hanlan pavilion approach", [[208,592],[160,592],[160,600]], half=8),
        path("Hanlan beach access", [[160,600],[128,600],[128,640]], half=8),
        path("Gibraltar lighthouse walk", [[272,760],[272,728]], half=8),
        path("Gibraltar beach access", [[272,800],[224,800],[224,816]], half=8),
        path("Centre beach approach", [[512,800],[560,800]]),
        path("Centre to Ward coastal boardwalk", [[560,800],[736,800],[736,704],[808,704],[808,624],[880,624],[880,552],[944,552],[944,496],[928,496]], half=8,
             note="Existing southern-shore relationship compressed into original cardinal segments, not a new crossing of open harbour."),
        path("Ward cottage post public approach", [[928,440],[904,440]], half=8),
        path("Ward village lane", [[888,344],[928,344],[928,392],[968,392],[968,440],[928,440]], half=8),
        path("Ward beach access", [[928,528],[928,552]], half=8),
        path("Algonquin public bridge and lane", [[800,584],[800,368],[824,368]], half=16),
        path("Snake Island public bridge spur", [[688,704],[688,480]], half=16),
    ],
    "bridges": [
        {"name": "Manitou Bridge", "points": [[512,544],[512,648]], "half_width": 16, "water": "Long Pond", "palette": 2,
         "existing_relationship": "Centre landing/attractions to southern Centre via Avenue of the Islands"},
        {"name": "Algonquin Island Bridge", "points": [[800,432],[800,568]], "half_width": 16, "water": "Eastern lagoon", "palette": 1,
         "existing_relationship": "Northern Algonquin spur from the eastern Cibola strip"},
        {"name": "Snake Island footbridge", "points": [[688,520],[688,648]], "half_width": 16, "water": "Snug Harbour lagoon", "palette": 4,
         "existing_relationship": "Separate public Snake Island spur, not a private yacht-club connection"},
    ],
    "ferry_docks": [
        {"name": "Hanlan's Point ferry landing", "points": [[320,256],[320,344]], "half_width": 16, "stop_id": 20},
        {"name": "Centre Island ferry landing", "points": [[512,392],[512,448]], "half_width": 16, "stop_id": 21},
        {"name": "Ward's Island ferry landing", "points": [[920,248],[920,312]], "half_width": 16, "stop_id": 22},
        {"name": "Centre Island pier", "points": [[512,832],[512,912]], "half_width": 16,
         "note": "Public lookout only; not an added ferry stop or mainland route."},
    ],
    "stop_candidates": [
        {"id": 20, "key": "hanlan_dock", "name": "HANLANS POINT", "x": 320, "y": 280, "transit": 3, "reserved": 0, "access": "district-level ferry/foot only"},
        {"id": 24, "key": "hanlan_service", "name": "HANLAN SERVICE", "x": 160, "y": 600, "transit": 0, "reserved": 0, "fictional_service_point": True, "access": "district-level ferry/foot only"},
        {"id": 21, "key": "centre_dock", "name": "CENTRE ISLAND", "x": 512, "y": 448, "transit": 3, "reserved": 0, "access": "district-level ferry/foot only"},
        {"id": 25, "key": "centre_post", "name": "CENTRE PARK POST", "x": 512, "y": 744, "transit": 0, "reserved": 0, "fictional_service_point": True, "access": "district-level ferry/foot only"},
        {"id": 22, "key": "ward_dock", "name": "WARDS ISLAND", "x": 920, "y": 280, "transit": 3, "reserved": 0, "access": "district-level ferry/foot only"},
        {"id": 26, "key": "ward_post", "name": "WARD COTTAGE POST", "x": 904, "y": 440, "transit": 0, "reserved": 0, "fictional_service_point": True, "access": "district-level ferry/foot only"},
    ],
    "restricted_areas": [
        {"name": "Billy Bishop airport and terminal boundary", "rect": [32,32,248,192], "kind": "airport"},
        {"name": "Water treatment grounds", "rect": [352,648,80,48], "kind": "water_plant"},
        {"name": "Private yacht-club island placeholder", "rect": [328,360,48,64], "kind": "private_island"},
        {"name": "Hanlan protected dune", "rect": [112,704,32,80], "kind": "dune"},
        {"name": "Gibraltar protected dune", "rect": [192,824,80,24], "kind": "dune"},
    ],
    "beaches": [
        {"name": "Hanlan's Point Beach", "rect": [96,600,48,96]},
        {"name": "Gibraltar Point Beach", "rect": [192,800,144,32]},
        {"name": "Centre Island Beach", "rect": [472,808,128,48]},
        {"name": "Ward's Island Beach", "rect": [904,528,64,40]},
    ],
    "landmarks": [
        {"name": "Hanlan service pavilion", "x": 128, "y": 560, "width": 24, "depth": 24, "kind": "pavilion", "palette": 4, "fictional": True},
        {"name": "Centre public pavilion", "x": 544, "y": 728, "width": 56, "depth": 40, "kind": "pavilion", "palette": 4, "fictional_handoff": True},
        {"name": "Gibraltar Point Lighthouse", "x": 224, "y": 712, "width": 24, "depth": 32, "kind": "lighthouse", "palette": 4},
        {"name": "Ward cottage post shelter", "x": 872, "y": 424, "width": 24, "depth": 24, "kind": "cottage", "palette": 1, "fictional": True},
    ],
    "alternate_foot_routes": {
        "from": [560,800], "to": [928,528],
        "inland": [[560,800],[512,800],[512,704],[688,704],[688,624],[776,624],[776,584],[848,584],[848,528],[928,528]],
        "coastal": [[560,800],[736,800],[736,704],[808,704],[808,624],[880,624],[880,552],[944,552],[944,528],[928,528]],
        "scope": "Two established eastern public-route relationships, with original cardinal distances and scenery. Not two invented lagoon bridges.",
    },
    "ports": [], "roads": [], "traffic_loops": [], "rails": [],
}


def paint_path(draw, points, half, fill):
    for (x1, y1), (x2, y2) in zip(points, points[1:]):
        assert x1 == x2 or y1 == y2, ("noncardinal public path", points)
        draw.rectangle((min(x1,x2)-half, min(y1,y2)-half,
                        max(x1,x2)+half-1, max(y1,y2)+half-1), fill=fill)


def render():
    image = Image.new("RGB", (WIDTH, HEIGHT), COLORS[0])
    draw = ImageDraw.Draw(image)
    land = Image.new("1", (WIDTH, HEIGHT)); land_draw = ImageDraw.Draw(land)
    walk = Image.new("1", (WIDTH, HEIGHT)); walk_draw = ImageDraw.Draw(walk)
    deck = Image.new("1", (WIDTH, HEIGHT)); deck_draw = ImageDraw.Draw(deck)
    attrs = [0] * (TW*TH); collisions = [15] * (TW*TH)
    blocks, canopies, cottage_yards, water_cells = [], [], [], []

    def box(x, y, w, h, color):
        assert w > 0 and h > 0
        draw.rectangle((x,y,x+w-1,y+h-1), fill=COLORS[color])

    def cells(x, y, w, h):
        for ty in range(max(0,y//8), min(TH,(y+h+7)//8)):
            for tx in range(max(0,x//8), min(TW,(x+w+7)//8)):
                yield tx, ty, ty*TW+tx

    def attributes(x, y, w, h, slot, priority=False):
        assert 0 <= slot <= 6
        for _, _, index in cells(x,y,w,h):
            attrs[index] = slot | (128 if priority else 0)

    def solid(x, y, w, h):
        for _, _, index in cells(x,y,w,h):
            collisions[index] = 15

    for shape in ISLAND["landforms"]:
        assert all(x%8 == y%8 == 0 for x,y in shape["polygon"])
        land_draw.polygon(shape["polygon"], fill=1)
    for area in ISLAND["restricted_areas"]:
        if area["kind"] in ("airport","private_island"):
            x,y,w,h=area["rect"]
            land_draw.rectangle((x,y,x+w-1,y+h-1),fill=1)
    for route in ISLAND["footpaths"]:
        paint_path(walk_draw,route["points"],route["half_width"],1)
    for bridge in ISLAND["bridges"] + ISLAND["ferry_docks"]:
        paint_path(deck_draw,bridge["points"],bridge["half_width"],1)

    for ty in range(TH):
        for tx in range(TW):
            x,y,index = tx*8,ty*8,ty*TW+tx
            terrain = land.getpixel((x+4,y+4))
            supported = deck.getpixel((x+4,y+4))
            if not terrain:
                water_cells.append(index)
            if terrain or supported:
                box(x,y,8,8,2 if terrain else 3)
                collisions[index] = 16; attrs[index] = 6 if terrain else 4
            if walk.getpixel((x+4,y+4)):
                assert terrain or supported, ("unsupported path over water",x+4,y+4)
                box(x,y,8,8,3); attrs[index] = 4
    # Original wave modules only on water, with no source map pixels used.
    for y in range(8,HEIGHT-8,32):
        for x in range(8,WIDTH-8,32):
            if collisions[(y//8)*TW+x//8] == 15 and not land.getpixel((x+4,y+4)):
                box(x,y+3,8,1,1)
    for beach in ISLAND["beaches"]:
        for tx,ty,index in cells(*beach["rect"]):
            if collisions[index] == 16 and land.getpixel((tx*8+4,ty*8+4)):
                box(tx*8,ty*8,8,8,3); box(tx*8+2,ty*8+2,1,1,2); attrs[index] = 4
    # Public bridge decks use original repeating planks and edge rails.
    for bridge in ISLAND["bridges"] + ISLAND["ferry_docks"]:
        x1,y1 = bridge["points"][0]; x2,y2 = bridge["points"][-1]
        half = bridge["half_width"]
        assert x1 == x2
        top,bottom = min(y1,y2)-half,max(y1,y2)+half
        for y in range(top,bottom,8):
            box(x1-half,y,half*2,1,1)
        for offset in (-half+1,half-2):
            box(x1+offset,top,1,bottom-top,0)
        attributes(x1-half,top,half*2,bottom-top,bridge.get("palette",4))
    # The boardwalk's planks distinguish the longer coastal choice.
    boardwalk = next(p for p in ISLAND["footpaths"] if "coastal boardwalk" in p["name"])
    for (x1,y1),(x2,y2) in zip(boardwalk["points"],boardwalk["points"][1:]):
        if y1 == y2:
            for x in range(min(x1,x2),max(x1,x2),8):
                box(x,y1-6,1,12,1)
        else:
            for y in range(min(y1,y2),max(y1,y2),8):
                box(x1-6,y,12,1,1)

    # Restriction symbols are original fencing; neither airport runway nor
    # yacht-club access is an ordinary courier path or a mainland connection.
    for area in ISLAND["restricted_areas"]:
        x,y,w,h = area["rect"]
        assert x%8 == y%8 == w%8 == h%8 == 0
        solid(x,y,w,h); box(x,y,w,h,2); attributes(x,y,w,h,6)
        draw.rectangle((x,y,x+w-1,y+h-1), outline=COLORS[0], width=2)
        for px in range(x+4,x+w-4,8):
            box(px,y,2,4,0); box(px,y+h-4,2,4,0)
        if area["kind"] == "airport":
            box(x+16,y+48,w-32,32,1)
            for px in range(x+32,x+w-24,32):
                box(px,y+62,16,2,3)
            box(x+144,y+112,64,32,1); box(x+152,y+120,48,8,3)
            box(x+208,y+88,16,48,0); box(x+210,y+88,12,10,3)
        elif area["kind"] == "water_plant":
            box(x+8,y+8,w-16,h-16,1)
            for px in range(x+16,x+w-16,24):
                draw.ellipse((px,y+12,px+15,y+27),fill=COLORS[3],outline=COLORS[0])
        elif area["kind"] == "private_island":
            box(x+8,y+16,w-16,24,1); box(x+12,y+20,w-24,8,3)
        else:
            for py in range(y+8,y+h-8,16):
                for px in range(x+8,x+w-8,16):
                    box(px,py,2,8,1)

    def may_build(x,y,w,h):
        if x < 16 or y < 16 or x+w >= WIDTH-16 or y+h >= HEIGHT-16:
            return False
        return all(collisions[index] == 16 and land.getpixel((tx*8+4,ty*8+4)) and
                   not walk.getpixel((tx*8+4,ty*8+4)) and not deck.getpixel((tx*8+4,ty*8+4))
                   for tx,ty,index in cells(x,y,w,h))

    def building(x,y,w,h,kind,palette,name=None):
        assert may_build(x,y,w,h), ("building on public corridor/water",name,x,y,w,h)
        roof = 8
        box(x,y,w,h,1); draw.rectangle((x,y,x+w-1,y+h-1),outline=COLORS[0])
        box(x+2,y+2,w-4,h-roof-2,2)
        draw.rectangle((x+4,y+4,x+w-5,y+h-roof-3),outline=COLORS[0])
        box(x,y-8,w,8,2); box(x,y-8,w,1,0)
        if kind == "pavilion":
            for px in range(x+4,x+w-4,8):
                box(px,y+h-roof,2,7,3)
                box(px+2,y+h-roof+2,4,4,0)
            box(x+w//2-2,y+h-5,4,4,0)
        elif kind == "lighthouse":
            # The raised lantern/tower projects north from its solid base.
            cx=x+w//2
            box(cx-6,y-40,12,56,3); box(cx-6,y-40,1,56,0); box(cx+5,y-40,1,56,0)
            box(cx-10,y-44,20,8,1); box(cx-8,y-42,16,4,3)
            box(cx-2,y-52,4,8,0); box(cx-4,y-16,8,3,1)
            # Only cells actually touched by the raised tower/lantern/spire
            # receive priority. A rectangular halo would hide walkers on grass.
            attributes(cx-6,y-40,12,56,palette,True)
            attributes(cx-10,y-44,20,8,palette,True)
            attributes(cx-2,y-52,4,8,palette,True)
        else:
            box(x+w//2-1,y+4,2,h-roof-8,0)
            for px in range(x+4,x+w-4,8):
                box(px,y+h-6,4,3,3)
            box(x+w//2-2,y+h-5,4,4,0)
        solid(x,y,w,h); attributes(x,y-8,w,h+8,palette,True)
        blocks.append({"x":x,"y":y,"width":w,"depth":h,"height":roof,
                       "style":kind,"landmark":name,"palette":palette})

    for landmark in ISLAND["landmarks"]:
        building(landmark["x"],landmark["y"],landmark["width"],landmark["depth"],
                 landmark["kind"],landmark["palette"],landmark["name"])
    # Modular cottages represent neighbourhood character, not copied houses.
    for zone in ((872,304,112,192),(744,344,112,88)):
        zx,zy,zw,zh = zone
        for y in range(zy,zy+zh-23,40):
            for x in range(zx+8,zx+zw-23,40):
                # Original closed cottage lots: public paths stay outside the
                # entire fenced yard, rather than allowing private shortcuts.
                yard=(x-8,y-8,40,40)
                if may_build(*yard):
                    building(x,y,24,24,"cottage",1+(x//8+y//8)%4)
                    solid(*yard);attributes(*yard,6)
                    draw.rectangle((yard[0],yard[1],yard[0]+39,yard[1]+39),outline=COLORS[1])
                    for px in range(yard[0]+4,yard[0]+36,8):
                        box(px,yard[1],1,3,0);box(px,yard[1]+37,1,3,0)
                    attributes(x,y-8,24,32,1+(x//8+y//8)%4,True)
                    cottage_yards.append({"rect":list(yard),"access":"closed original private yard; public lanes retained outside","surveyed_property":False})
    # Trees can occlude the courier beneath their canopies. Keep their solid
    # trunk tiles off authored paths, client pads, bridges and restricted areas.
    pads = [(s["x"]-16,s["y"]-16,32,32) for s in ISLAND["stop_candidates"]]
    def intersects(a,b):
        x,y,w,h=a; xx,yy,ww,hh=b
        return x<xx+ww and xx<x+w and y<yy+hh and yy<y+h
    for y in range(264,HEIGHT-32,32):
        for x in range(88,WIDTH-32,32):
            if (x//32+3*y//32)%3 or any(intersects((x,y,16,24),p) for p in pads):
                continue
            if not may_build(x,y,16,24):
                continue
            box(x+6,y+16,3,8,0)
            draw.ellipse((x,y,x+15,y+15),fill=COLORS[1],outline=COLORS[0])
            box(x+4,y+4,8,8,2); solid(x,y+16,16,8)
            attributes(x,y,16,16,6,True); canopies.append([x,y,16,16])
    # Path-side benches and beds are repeated modules outside full-foot routes.
    for x,y in ((448,536),(576,536),(352,784),(592,784),(856,472),(888,360)):
        if may_build(x,y,16,8):
            box(x,y,16,3,1); box(x+2,y+3,2,5,0); box(x+12,y+3,2,5,0)
            solid(x,y,16,8); attributes(x,y,16,8,4)
    for stop in ISLAND["stop_candidates"]:
        x,y=stop["x"],stop["y"]
        draw.rectangle((x-4,y-4,x+3,y+3),fill=COLORS[3],outline=COLORS[0])
        box(x-2,y-1,4,1,0)

    def clear(x,y,half=FOOT_HALF):
        if x-half<0 or y-half<0 or x+half>=WIDTH or y+half>=HEIGHT:
            return False
        return all(not(collisions[index]&15) for _,_,index in cells(x-half,y-half,half*2+1,half*2+1))

    swept_samples=0
    def sweep(points):
        nonlocal swept_samples
        for (x1,y1),(x2,y2) in zip(points,points[1:]):
            assert x1==x2 or y1==y2
            for offset in range(abs(x2-x1)+abs(y2-y1)+1):
                x=x1+(offset if x2>x1 else -offset if x2<x1 else 0)
                y=y1+(offset if y2>y1 else -offset if y2<y1 else 0)
                assert clear(x,y), ("full-foot swept body",x,y,points)
                swept_samples+=1
    for route in ISLAND["footpaths"]+ISLAND["bridges"]+ISLAND["ferry_docks"]:
        sweep(route["points"])
    for alternative in ("inland","coastal"):
        sweep(ISLAND["alternate_foot_routes"][alternative])
    for stop in ISLAND["stop_candidates"]:
        assert clear(stop["x"],stop["y"]), ("full-foot service pad",stop)
        assert clear(stop["x"],stop["y"],LANDING_HALF), ("landing body service pad",stop)
        assert stop["reserved"] == 0 and not any("parking" in key for key in stop)
    for index in water_cells:
        x,y=index%TW*8+4,index//TW*8+4
        assert deck.getpixel((x,y)) or collisions[index] == 15, ("accidental water crossing",x,y)
    for area in ISLAND["restricted_areas"]:
        assert all(collisions[index]==15 for _,_,index in cells(*area["rect"])), area
    for yard in cottage_yards:
        assert all(collisions[index]==15 for _,_,index in cells(*yard["rect"])), yard
    assert set(collisions)=={15,16} and len(collisions)==15616
    assert all(collisions[i]==15 for i in list(range(TW))+list(range((TH-1)*TW,TH*TW)))
    assert all(collisions[y*TW]==collisions[y*TW+TW-1]==15 for y in range(TH))
    bridge_lateral_offsets=list(range(-10,11))
    for bridge in ISLAND["bridges"]:
        for lateral in bridge_lateral_offsets:
            sweep([[x+lateral,y] for x,y in bridge["points"]])

    # Export nonoverlapping tile-aligned water rectangles for collection tools.
    # Named water relationships stay separate from the ordinary rectangle list.
    active_water={};water_rectangles=[]
    wet=set(water_cells)
    for ty in range(TH):
        runs=[];tx=0
        while tx<TW:
            if ty*TW+tx not in wet:
                tx+=1;continue
            start=tx
            while tx<TW and ty*TW+tx in wet:
                tx+=1
            runs.append((start*8,(tx-start)*8))
        for key in list(active_water):
            if key not in runs:
                water_rectangles.append(active_water.pop(key))
        for x,w in runs:
            if (x,w) in active_water:
                active_water[(x,w)][3]+=8
            else:
                active_water[(x,w)]=[x,ty*8,w,8]
    water_rectangles.extend(active_water.values())
    exported_water=set()
    for rectangle in water_rectangles:
        for _,_,index in cells(*rectangle):
            assert index not in exported_water
            exported_water.add(index)
    assert exported_water==wet

    # Conservative 4px-grid connected body paths; every neighbouring pair's
    # overlapping body union also covers all intermediate cardinal positions.
    usable = {(x,y) for y in range(4,HEIGHT-4,4) for x in range(4,WIDTH-4,4) if clear(x,y)}
    anchors = [(s["x"],s["y"]) for s in ISLAND["stop_candidates"]]
    distances=[]
    reachable_count=0
    for origin in anchors:
        queue=deque([origin]); distance={origin:0}
        while queue:
            x,y=queue.popleft()
            for point in ((x-4,y),(x+4,y),(x,y-4),(x,y+4)):
                if point in usable and point not in distance:
                    distance[point]=distance[(x,y)]+4; queue.append(point)
        assert all(target in distance for target in anchors), ("disconnected ferry/client graph",origin)
        distances.append([distance[target] for target in anchors]); reachable_count=len(distance)
    assert len(usable)==reachable_count, ("unintended accessible disconnected island",len(usable),reachable_count)

    peds=set()
    for route in ISLAND["footpaths"]:
        for (x1,y1),(x2,y2) in zip(route["points"],route["points"][1:]):
            if y1 != y2:
                continue
            left,right=sorted((x1,x2))
            for offset in (-8,0,8) if route["half_width"]>=16 else (0,):
                for x in range((left+7)//8*8,right-62,64):
                    if all(clear(px,y1+offset) for px in range(x,x+64)):
                        peds.add((x,y1+offset))
    peds=sorted(peds,key=lambda p:(p[1],p[0])); assert len(peds)>=24
    if len(peds)>128:
        peds=[peds[i*len(peds)//128] for i in range(128)]
    for x,y in peds:
        sweep([[x,y],[x+63,y]])

    scenery=decorate(image,collisions,attrs,blocks,canopies,'islands',ISLAND)
    raw_patterns, patterns = set(), set()
    for ty in range(TH):
        for tx in range(TW):
            tile=image.crop((tx*8,ty*8,tx*8+8,ty*8+8)); raw_patterns.add(tile.tobytes())
            if attrs[ty*TW+tx]&128 and collisions[ty*TW+tx]==16:
                assert set(tile.get_flattened_data())!={tuple(bytes.fromhex(COLORS[2][1:]))}, ("priority on undecorated public grass",tx,ty)
            variants=(tile,tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT),
                      tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM),tile.transpose(Image.Transpose.ROTATE_180))
            patterns.add(min(item.tobytes() for item in variants))
    assert len(raw_patterns)<=256 and len(patterns)<=256, ("Island source tile target",len(raw_patterns),len(patterns))
    assert set(image.get_flattened_data()) <= {tuple(bytes.fromhex(c[1:])) for c in COLORS}
    assert all((a&7)<=6 and not(a&0x78) for a in attrs)
    for block in blocks:
        assert all(collisions[index]==15 for _,_,index in cells(block["x"],block["y"],block["width"],block["depth"])), block
    buffer=io.BytesIO(); image.save(buffer,format="PNG"); png=buffer.getvalue()
    alternatives=ISLAND["alternate_foot_routes"]
    def length(points):
        return sum(abs(a[0]-b[0])+abs(a[1]-b[1]) for a,b in zip(points,points[1:]))
    metadata={**ISLAND,
        "status": "Generated original source proposal only; native registration/build/play/save-migration/hardware unverified.",
        "projection": "Orthogonal north-up original game compression; not transformed GIS or copied map artwork.",
        "dimensions": [WIDTH,HEIGHT], "tile_dimensions": [TW,TH],
        "background_filename": "toronto_islands.png", "source_png": "project/original-art/toronto_islands.png",
        "background_sha256": hashlib.sha256(png).hexdigest(),
        "source_attributes": "project/original-art/island_attributes.json",
        "source_research": "docs/ISLAND_DISTRICT_PLAN.md", "research": RESEARCH,
        "collision_rules": {"foot_only":16,"solid":15,"ordinary_car_terrain_cells":0},
        "blocks": blocks, "canopies": canopies, "cottage_yards":cottage_yards, "collisions": collisions,
        "water":water_rectangles,
        "water_bodies": {"representation":"All cells outside original land polygons and isolated restricted land. Supported decks preserve underlying water.",
                  "named_relationships":["Inner Harbour north","Lake Ontario south","Blockhouse Bay east of Hanlan","Long Pond beneath Manitou","Eastern Algonquin lagoon","Snug Harbour and Snake spur"]},
        "palette_proposal": {"source_shades":COLORS,"slots0_to5":"Reuse scene stone/water, cottage and sand families.",
                             "slot6":"Island park greens", "slot6_colors_light_to_dark":["FFF7CC","80D856","4A9356","284E38"],
                             "slot7":"Existing UI/font reserved; no asset consumes it.","native_palette_registered":True},
        "pedestrian_routes": [{"x":x,"y":y,"axis":"horizontal","length_pixels":63} for x,y in peds],
        "full_foot_route_graph": {"anchor_order":[s["id"] for s in ISLAND["stop_candidates"]],
                                  "conservative_grid_step_pixels":4,"body_half_pixels":FOOT_HALF,
                                  "shortest_anchor_distances_pixels":distances,
                                  "connected_grid_positions":reachable_count,
                                  "inland_alternative_pixels":length(alternatives["inland"]),
                                  "coastal_alternative_pixels":length(alternatives["coastal"]),
                                  "walking_speed_or_waits_changed":False},
        "validation": {"raw_unique_tiles":len(raw_patterns),"flip_canonical_unique_tiles":len(patterns),
                       "source_tile_target":256,"existing_collection_source_ceiling":320,
                       "full_foot_half_pixels":FOOT_HALF,"landing_body_half_pixels":LANDING_HALF,
                       "all_six_half3_landing_bodies_clear":True,"swept_pixel_samples":swept_samples,
                       "public_terrain_all_collision16":True,"ordinary_car_terrain_cells":0,
                       "water_outside_authored_decks_solid":True,"restricted_areas_solid":True,
                       "private_cottage_yards_closed":True,"closed_cottage_yards":len(cottage_yards),
                       "no_priority_on_undecorated_public_grass":True,
                       "bridge_lateral_offsets_verified":bridge_lateral_offsets,
                       "water_rectangles_exactly_cover_source_water_cells":True,
                       "supported_public_footbridges":3,"public_foot_docks_and_pier":4,
                       "all_six_docks_and_services_connected":True,"no_mainland_seam":True,
                       "no_proposed_lagoon_or_gibraltar_bridge":True,"fixed_pedestrian_routes":len(peds),
                       "compiled_background_bank1_limit":32,"compiled_background_bank1_gate_verified":False,
                       "new_sprite_sheets":0,"new_persistent_runtime_bytes":0,
                       "native_registration_verified":False,"native_build_verified":False,
                       "native_foot_travel_verified":False,"save_migration_implemented":False,
                       "physical_execution_verified":False,"measured_gameplay_duration_verified":False}}
    metadata['scenery']=scenery
    return png,attrs,metadata


def same_png_artwork(expected,actual):
    with Image.open(io.BytesIO(expected)) as a, Image.open(io.BytesIO(actual)) as b:
        return (a.format==b.format=="PNG" and a.mode==b.mode and a.size==b.size and
                a.getpalette()==b.getpalette() and a.info==b.info and a.tobytes()==b.tobytes())


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    modes=parser.add_mutually_exclusive_group()
    modes.add_argument("--check",action="store_true")
    modes.add_argument("--dry-run",action="store_true")
    args=parser.parse_args(); png,attrs,metadata=render()
    if args.check:
        canonical=(ART/"toronto_islands.png").read_bytes()
        assert same_png_artwork(png,canonical), "Island source pixels/palette differ"
        png=canonical; metadata["background_sha256"]=hashlib.sha256(png).hexdigest()
    if not args.dry_run:
        files={ART/"toronto_islands.png":png,
               ART/"island_attributes.json":(json.dumps(attrs)+"\n").encode(),
               ROOT/"content/districts/island_art.json":(json.dumps(metadata,indent=2)+"\n").encode()}
        # Registration belongs to the official authoring API. Once it exists,
        # synchronize only original pixels, palette/priority RLE and collision
        # RLE; preserve IDs, loaders, palettes, scripts and every other field.
        native=ROOT/"project/assets/backgrounds/toronto_islands.png"
        sidecar=native.with_suffix(".png.gbsres")
        scene_path=ROOT/"project/project/scenes/toronto_islands/scene.gbsres"
        if sidecar.exists() or scene_path.exists():
            assert sidecar.exists() and scene_path.exists(), "Incomplete native Island registration"
            background=json.loads(sidecar.read_text());scene=json.loads(scene_path.read_text())
            assert background["filename"]==native.name and background["autoColor"] is False
            assert scene["type"]=="TORONTO" and scene["symbol"]=="scene_toronto_islands"
            assert scene["backgroundId"]==background["id"] and (scene["width"],scene["height"])==(TW,TH)
            background.update(width=TW,height=TH,imageWidth=WIDTH,imageHeight=HEIGHT,tileColors=compress(attrs))
            scene["collisions"]=compress(metadata["collisions"])
            files[native]=png
            files[sidecar]=(json.dumps(background,indent=2)+"\n").encode()
            files[scene_path]=(json.dumps(scene,indent=2)+"\n").encode()
        for filename,data in files.items():
            if args.check:
                assert filename.read_bytes()==data, f"Island source differs: {filename}"
            else:
                filename.parent.mkdir(parents=True,exist_ok=True); filename.write_bytes(data)
    v=metadata["validation"]; state="validated in memory" if args.dry_run else "matches" if args.check else "generated"
    print(f"Islands source {state}: {len(metadata['blocks'])} original footprints, {v['raw_unique_tiles']} raw/{v['flip_canonical_unique_tiles']} flipped tiles, {v['supported_public_footbridges']} public footbridges, {v['fixed_pedestrian_routes']} full-foot routes, {v['swept_pixel_samples']} swept samples. Native integration unverified.")


if __name__ == "__main__":
    main()
