"""Author original manual-palette western native backgrounds and collision grids.

No map imagery, photos, official logos or downloaded geometry are used as pixels.
The parent native workflow registers these source assets and performs ROM tests.
"""
import hashlib
import sys
import json
from collections import deque
from pathlib import Path
from PIL import Image, ImageDraw
from west_layout import DISTRICTS, WIDTH, HEIGHT, ROAD_HALF, WALK_HALF, extended_points
from streetcar_art import paint_streetcar_stops
import city_kit

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "project"
COLORS = ["#071821", "#306850", "#86c06c", "#e0f8cf"]
TW, TH = WIDTH // 8, HEIGHT // 8


def paint_path(draw, points, half, fill):
    """Square-ended, tile-aligned cardinal paths with exact exclusive far edges."""
    for (x1,y1),(x2,y2) in zip(points, points[1:]):
        assert x1 == x2 or y1 == y2, (x1,y1,x2,y2)
        draw.rectangle((min(x1,x2)-half,min(y1,y2)-half,max(x1,x2)+half-1,max(y1,y2)+half-1), fill=fill)


def generate(spec,check=False):
    img = Image.new("RGB", (WIDTH,HEIGHT), COLORS[2])
    d = ImageDraw.Draw(img)
    road_mask = Image.new("1", (WIDTH,HEIGHT), 0)
    walk_mask = Image.new("1", (WIDTH,HEIGHT), 0)
    rd, wd = ImageDraw.Draw(road_mask), ImageDraw.Draw(walk_mask)
    attrs = [6] * (TW*TH)
    collisions = [16] * (TW*TH)
    blocks, canopies = [], []

    def box(x,y,w,h,color):
        d.rectangle((x,y,x+w-1,y+h-1), fill=COLORS[color])

    def cells(x,y,w,h):
        for ty in range(max(0,y//8),min(TH,(y+h+7)//8)):
            for tx in range(max(0,x//8),min(TW,(x+w+7)//8)):
                yield tx,ty,ty*TW+tx

    def attr(x,y,w,h,slot,priority=False):
        for tx,ty,i in cells(x,y,w,h):
            attrs[i] = slot | (128 if priority and not road_mask.getpixel((tx*8+4,ty*8+4)) else 0)

    def solid(x,y,w,h):
        for _,_,i in cells(x,y,w,h):
            collisions[i] = 15

    # Visible closed scene edges; only named portals are subsequently reopened.
    for x,y,w,h in [(0,0,WIDTH,16),(0,HEIGHT-16,WIDTH,16),(0,0,16,HEIGHT),(WIDTH-16,0,16,HEIGHT)]:
        box(x,y,w,h,1); solid(x,y,w,h)
    for x,y,w,h in spec["water"]:
        box(x,y,w,h,2); solid(x,y,w,h); attr(x,y,w,h,0)
    for park in spec["parks"]:
        x,y,w,h=park["rect"]
        box(x,y,w,h,2);attr(x,y,w,h,6)
    if "pond" in spec:
        d.polygon(spec["pond"],fill=COLORS[2],outline=COLORS[0])
        pond_mask=Image.new("1",(WIDTH,HEIGHT));ImageDraw.Draw(pond_mask).polygon(spec["pond"],fill=1)
        for ty in range(TH):
            for tx in range(TW):
                if pond_mask.crop((tx*8,ty*8,tx*8+8,ty*8+8)).getbbox():
                    collisions[ty*TW+tx]=15;attrs[ty*TW+tx]=0
    # Rail and expressway are barriers; explicitly authored crossings go over them.
    for rail in spec["rails"]:
        pts=rail["points"]
        rail_mask=Image.new("1",(WIDTH,HEIGHT));rmd=ImageDraw.Draw(rail_mask)
        if all(a[0]==b[0] or a[1]==b[1] for a,b in zip(pts,pts[1:])):
            paint_path(rmd,pts,8,1);paint_path(d,pts,8,COLORS[0])
        else:
            rmd.line([tuple(p) for p in pts],fill=1,width=16)
            d.line([tuple(p) for p in pts],fill=COLORS[0],width=16)
        d.line([tuple(p) for p in pts],fill=COLORS[3],width=2)
        for ty in range(TH):
            for tx in range(TW):
                if rail_mask.crop((tx*8,ty*8,tx*8+8,ty*8+8)).getbbox(): collisions[ty*TW+tx]=15
    x,y,w,h=spec["gardiner"]
    box(x,y,w,h,0);solid(x,y,w,h)
    for px in range(0,WIDTH,32):box(px,y+7,8,2,3)
    for route in spec["roads"]:
        points=extended_points(route,spec)
        paint_path(wd,points,WALK_HALF,1)
        paint_path(rd,points,ROAD_HALF,1)
    for route in spec["footpaths"]:
        paint_path(wd,extended_points(route,spec,foot=True),8,1)
    # Tile-level permission agrees with the authored asphalt/sidewalk boundaries.
    for ty in range(TH):
        for tx in range(TW):
            i=ty*TW+tx;x=tx*8;y=ty*8
            if walk_mask.getpixel((x+4,y+4)):
                box(x,y,8,8,3);collisions[i]=16;attrs[i]=0
            if road_mask.getpixel((x+4,y+4)):
                box(x,y,8,8,1);collisions[i]=0;attrs[i]=0
    # Repeated lane dashes do not introduce per-road pattern variants; they
    # stop short of junctions, which carry zebra crossings on every arm.
    centres=city_kit.intersections_from_routes([extended_points(r,spec) for r in spec["roads"]])
    for route in spec["roads"]:
        for (x1,y1),(x2,y2) in zip(route["points"],route["points"][1:]):
            if y1==y2:
                for px in range((min(x1,x2)//32+1)*32,max(x1,x2),32):
                    if not city_kit.near_crossing(px+4,y1,centres):box(px,y1,8,1,3)
            else:
                for py in range((min(y1,y2)//32+1)*32,max(y1,y2),32):
                    if not city_kit.near_crossing(x1,py+4,centres):box(x1,py,1,8,3)
    def on_road(x,y):return 0<=x<WIDTH and 0<=y<HEIGHT and road_mask.getpixel((x,y))
    def on_walk(x,y):return 0<=x<WIDTH and 0<=y<HEIGHT and walk_mask.getpixel((x,y)) and not road_mask.getpixel((x,y))
    city_kit.paint_crosswalks(box,on_road,on_walk,centres)

    reserved=[]
    for landmark in spec["landmarks"]:
        reserved.append((landmark["x"]-8,landmark["y"]-16,landmark["width"]+24,landmark["depth"]+32))
    for park in spec["parks"]:reserved.append(tuple(park["rect"]))
    if "pond" in spec:
        px=[point[0] for point in spec["pond"]];py=[point[1] for point in spec["pond"]]
        reserved.append((min(px)-8,min(py)-8,max(px)-min(px)+16,max(py)-min(py)+16))

    def overlap(a,b):
        x,y,w,h=a;xx,yy,ww,hh=b
        return x<xx+ww and xx<x+w and y<yy+hh and yy<y+h

    def may_build(x,y,w,h,landmark=False):
        if x<32 or y<40 or x+w>WIDTH-32 or y+h>spec["water"][0][1]-8:return False
        footprint=(x-8,y-8,w+16,h+16)
        if not landmark and any(overlap(footprint,r) for r in reserved):return False
        for tx,ty,i in cells(*footprint):
            if walk_mask.getpixel((tx*8+4,ty*8+4)) or collisions[i]==15:return False
        return True

    def crown(x,y,w,h,style):
        # Towers rise over open street to their north: the upper floors take
        # priority over the road too, so traffic and walkers pass behind them.
        if style not in (3,4) or h<32 or y<40:return 8
        if any(collisions[i]==15 for _,_,i in cells(x,y-24,w,16)):return 8
        city_kit.tower_crown(d,box,x,y,w,24,style,COLORS)
        for _,_,i in cells(x,y-24,w,16):attrs[i]=(1 if style==5 else style+1)|128
        return 24

    def building(x,y,w,h,style,kind=None,name=None):
        assert may_build(x,y,w,h,landmark=bool(name)), (spec["slug"],name,x,y,w,h)
        roof=16 if style in (3,4) and h>=40 else 8
        # Ground footprints are explicit; lips/canopies can occlude foot traffic.
        box(x+4,y+4,w,h,0)
        box(x,y,w,h,1);d.rectangle((x,y,x+w-1,y+h-1),outline=COLORS[0])
        box(x+2,y+2,w-4,max(4,h-roof-2),2)
        d.rectangle((x+4,y+4,x+w-5,y+h-roof-3),outline=COLORS[0])
        box(x,y-8,w,8,2);d.line((x,y-8,x+w-1,y-8),fill=COLORS[0])
        for wx in range(x+4,x+w-4,8):box(wx,y+h-roof+3,4,3,3)
        box(x+w//2-2,y+h-5,4,4,0)
        if style==0:
            for wx in range(x+2,x+w-2,8):box(wx,y+h-8,4,3,3)
            box(x+8,y+8,8,8,1)
        elif style==1:
            box(x+8,y+8,max(8,w-16),8,1)
            for wx in range(x+8,x+w-8,8):box(wx,y+12,4,2,3)
        elif style==2:city_kit.gable_house(d,box,x,y,w,h,roof,COLORS)
        elif style==3:
            d.rectangle((x+8,y+8,x+w-9,y+h-roof-7),outline=COLORS[3])
        elif style==4:
            for wx in range(x+8,x+w-8,8):d.line((wx,y+4,wx,y+h-roof-4),fill=COLORS[3])
            box(x+8,y+8,8,8,1)
        else:
            for wx in range(x+8,x+w-8,16):box(wx,y+8,8,8,3);box(wx,y+h-6,8,4,0)
        city_kit.roof_details(d,box,x,y,w,h,roof,city_kit.seed_of(spec["slug"],x,y),COLORS)
        if kind=="carhouse":
            for wx in range(x+8,x+w-8,24):box(wx,y+h-16,16,12,0);box(wx+4,y+h-14,8,8,2)
        elif kind=="regency":
            box(x-4,y+h-8,w+8,8,3)
            for wx in range(x,x+w,8):box(wx,y+h-8,2,8,0)
            box(x+8,y+4,4,8,0);box(x+w-12,y+4,4,8,0)
        elif kind=="pavilion":
            for wx in range(x+8,x+w-8,16):d.rectangle((wx,y+8,wx+7,y+h-3),outline=COLORS[0])
            box(x+w//2-8,y-8,16,8,1)
        elif kind=="junction":
            for wx in range(x+8,x+w-8,16):box(wx,y+2,8,3,3)
        solid(x,y,w,h)
        # Slot 6 is the vegetation palette; wide work sheds use brick terracotta.
        attr(x,y-8,w+8,h+16,1 if style==5 else style+1,True)
        lip=crown(x,y,w,h,style)
        blocks.append({"x":x,"y":y,"width":w,"depth":h,"height":roof,"style":style,"landmark":name,"kind":kind,"overhang":lip})

    for landmark in spec["landmarks"]:
        building(landmark["x"],landmark["y"],landmark["width"],landmark["depth"],landmark["style"],landmark["kind"],landmark["name"])
    # Park features from the layout (pitch, fieldhouse, paddocks, open lawn),
    # drawn before the generic buildings and trees so neither covers them.
    no_trees=[]
    for park in spec["parks"]:
        if park.get("fieldhouse"):
            fx,fy,fw,fh=park["fieldhouse"];building(fx,fy,fw,fh,5,None,park["name"]+" fieldhouse")
        for feature in park.get("features",[]):
            fx,fy,fw,fh=feature["rect"]
            assert not any(walk_mask.getpixel((px,py)) for px in range(fx,fx+fw) for py in range(fy,fy+fh)),(spec["slug"],feature,"park feature on a road or path")
            solid_rects,slots=city_kit.paint_park_feature(d,box,COLORS,feature["kind"],fx,fy,fw,fh)
            for rect in solid_rects:solid(*rect)
            for rect,slot in slots:attr(*rect,slot)
            reserved.append((fx-8,fy-8,fw+16,fh+16));no_trees.append((fx,fy,fw,fh))
    # Six repeating designs, different dimensions; no copied facades or signage.
    for yy in range(48,728,48):
        for xx in range(48,976,48):
            style=((xx//48)*3+yy//48)%6
            w=32 if style not in (3,5) else 48
            h=24 if style not in (1,4) else 32
            if may_build(xx,yy,w,h):building(xx,yy,w,h,style)
    # Broad harbour buildings and beach kiosks stop clear of the walkway.
    for xx in range(64,944,96):
        if may_build(xx,856,48,24):building(xx,856,48,24,0)
    # The scene's body shop: a spray bay in the road lane, its door on the
    # building behind (gameplay: TORONTO.c td_spray_at).
    bay=spec["spray_bay"]
    assert all(road_mask.getpixel((x,y)) for x in (bay["x"],bay["x"]+city_kit.SPRAY_BAY_W-1) for y in (bay["y"],bay["y"]+city_kit.SPRAY_BAY_H-1)),(spec["slug"],"spray bay off the asphalt")
    assert any(b["x"]<=bay["x"] and bay["x"]+city_kit.SPRAY_BAY_W<=b["x"]+b["width"] and b["y"]+b["depth"]==bay["door_bottom"] for b in blocks),(spec["slug"],"spray bay door has no building")
    city_kit.paint_spray_bay(d,box,COLORS,bay["x"],bay["y"],bay["door_bottom"],14)
    assert city_kit.spray_bay_registered((PROJECT/"plugins/toronto-driving/engine/src/states/TORONTO.c").read_text(),spec["id"],bay["x"],bay["y"]),(spec["slug"],"spray bay moved: update td_spray_at in TORONTO.c")
    # Trees reuse exactly aligned patterns and never cover asphalt or formal paths.
    for yy in range(336,624,32):
        for xx in range(624 if spec["id"]==2 else 592,944 if spec["id"]==2 else 704,32):
            if any(walk_mask.getpixel((px,py)) for py in range(yy,yy+24) for px in range(xx,xx+16)):continue
            if any(collisions[i]==15 for _,_,i in cells(xx,yy,16,24)):continue
            if any(overlap((xx,yy,16,24),r) for r in no_trees):continue
            box(xx+6,yy+16,3,8,0);d.ellipse((xx,yy,xx+15,yy+15),fill=COLORS[1],outline=COLORS[0]);box(xx+4,yy+4,8,8,2)
            solid(xx,yy+16,16,8);attr(xx,yy,16,16,6,True);canopies.append([xx,yy,16,16])

    def car_clear(x,y,half=8):
        return 0<=x-half and x+half<WIDTH and 0<=y-half and y+half<HEIGHT and all(collisions[i]==0 for _,_,i in cells(x-half,y-half,half*2+1,half*2+1))

    def foot_clear(x,y):
        return all(not collisions[i]&15 for _,_,i in cells(x-2,y-2,5,5))

    def route_clear(points):
        for (x1,y1),(x2,y2) in zip(points,points[1:]+points[:1]):
            assert x1==x2 or y1==y2,points
            for step in range(abs(x2-x1)+abs(y2-y1)+1):
                x=x1+(step if x2>x1 else -step if x2<x1 else 0)
                y=y1+(step if y2>y1 else -step if y2<y1 else 0)
                assert car_clear(x,y), (spec["slug"],"traffic footprint",x,y)

    for loop in spec["traffic_loops"]:
        assert 4<=len(loop)<=16
        route_clear(loop)
    for port in spec["ports"]:
        x,y=port["x"],port["y"]
        assert foot_clear(x,y)
        if not port["foot_only"]:
            for offset in (-12,0,12):assert car_clear(x,y+offset), (spec["slug"],"portal",x,y,offset)
    for stop in spec["stop_candidates"]:
        assert (foot_clear if stop["foot_only"] else car_clear)(stop["x"],stop["y"]),stop
        if stop.get("parking_anchor"):assert car_clear(*stop["parking_anchor"])

    # Every foot portal/client must be connected; closed municipal edges remain solid.
    start=spec["stop_candidates"][0]
    visited={(start["x"]//8,start["y"]//8)};queue=deque(visited)
    while queue:
        tx,ty=queue.popleft()
        for nx,ny in ((tx-1,ty),(tx+1,ty),(tx,ty-1),(tx,ty+1)):
            if 0<=nx<TW and 0<=ny<TH and (nx,ny) not in visited and not collisions[ny*TW+nx]&15:
                visited.add((nx,ny));queue.append((nx,ny))
    for point in spec["ports"]+spec["stop_candidates"]:
        assert (point["x"]//8,point["y"]//8) in visited,(spec["slug"],"foot unreachable",point)

    # Lawns, canopy trees, parking and plazas on untouched walkable ground.
    ground=bytes.fromhex(COLORS[2][1:])*64
    reserved_lots=[(l["x"]-8,l["y"]-16,l["width"]+24,l["depth"]+32) for l in spec["landmarks"]]+no_trees
    def lot(tx,ty):
        i=ty*TW+tx;x=tx*8;y=ty*8
        if collisions[i]!=16 or attrs[i]!=6 or walk_mask.getpixel((x+4,y+4)):return False
        if any(rx<=x+4<rx+rw and ry<=y+4<ry+rh for rx,ry,rw,rh in reserved_lots):return False
        return img.crop((x,y,x+8,y+8)).tobytes()==ground
    def set_attr(tx,ty,value):attrs[ty*TW+tx]=value
    before=list(collisions)
    canopies+=city_kit.dress_lots(d,box,TW,TH,lot,set_attr,COLORS,spec["slug"],[tuple(p["rect"]) for p in spec["parks"]])
    assert collisions==before,"lot decoration must not change collision"
    paint_streetcar_stops(d,spec['id'],COLORS)
    # Open water takes the shared animated texture, with foam along the shore.
    pond=None
    if "pond" in spec:
        pond=Image.new("1",(WIDTH,HEIGHT));ImageDraw.Draw(pond).polygon(spec["pond"],fill=1)
    def is_water(x,y):
        return any(wx<=x<wx+ww and wy<=y<wy+wh for wx,wy,ww,wh in spec["water"]) or bool(pond and pond.getpixel((x,y)))
    water_tiles=city_kit.texture_water(img,attrs,TW,is_water,COLORS)
    patterns=set();raw_patterns=set()
    for ty in range(TH):
        for tx in range(TW):
            tile=img.crop((tx*8,ty*8,tx*8+8,ty*8+8))
            raw_patterns.add(tile.tobytes())
            variants=[tile,tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT),tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM),tile.transpose(Image.Transpose.ROTATE_180)]
            patterns.add(min(v.tobytes() for v in variants))
    assert len(patterns)<=320,(spec["slug"],"128 bank-0 + 192 bank-1 background tiles",len(patterns))
    assert set(img.get_flattened_data())<=set(tuple(bytes.fromhex(c[1:])) for c in COLORS)
    assert len(collisions)==15616 and set(collisions)<={0,16,15}
    assert all((a&7)<=6 and not(a&128) for a,c in zip(attrs,collisions) if c==0)

    filename=f"toronto_{spec['slug']}.png"
    out=PROJECT/"assets/backgrounds"/filename
    if check:
        # PNG encoders differ between platforms: compare decoded pixels and
        # keep the committed file's bytes as the hashed identity.
        with Image.open(out) as current:
            assert current.convert("RGB").tobytes()==img.convert("RGB").tobytes(),(spec["slug"],"background pixels are stale")
    else:
        img.save(out)
    metadata={**spec,"projection":"original compressed orthogonal north-up; not GIS coordinates","dimensions":[WIDTH,HEIGHT],"tile_dimensions":[TW,TH],"road_half_width":ROAD_HALF,"walk_half_width":WALK_HALF,"blocks":blocks,"canopies":canopies,"collisions":collisions,"collision_rules":{"road":0,"foot_only":16,"solid":15},"background_filename":filename,"background_sha256":hashlib.sha256(out.read_bytes()).hexdigest(),"source_research":"content/districts/west-research.json","validation":{"raw_unique_tiles":len(raw_patterns),"flip_canonical_unique_tiles":len(patterns),"animated_water_tiles":water_tiles,"traffic_loops":len(spec["traffic_loops"]),"traffic_footprint_half_pixels":8,"traffic_swept_all_overlapped_tiles_clear":True,"portal_car_offsets_verified":[-12,0,12],"all_foot_clients_and_ports_connected":True,"native_build_verified":False,"measured_gameplay_duration_verified":False}}
    # Canonical stable candidate keys consumed by campaign tooling.
    keys=["dufferin_college","lansdowne_bloor","parkdale_queen","roncy_howard_park","sorauren"] if spec["id"]==1 else ["bloor_park_gate","parkside_south","colborne_service"]
    metadata["stop_candidates"]=[{"key":key,**stop} for key,stop in zip(keys,spec["stop_candidates"])]
    texts={ROOT/"content/districts"/f"{spec['slug']}_art.json":json.dumps(metadata,indent=2)+"\n",
           PROJECT/"original-art"/f"{spec['slug']}_attributes.json":json.dumps(attrs)+"\n"}
    for path,text in texts.items():
        if check:assert path.read_text()==text,(spec["slug"],"stale output",str(path.relative_to(ROOT)))
        else:path.write_text(text)
    if check:
        print(f"{filename}: matches its generator ({len(patterns)} flip-canonical tiles)")
        return
    print(f"{filename}: {len(blocks)} buildings, {len(patterns)} flip-canonical tiles ({len(raw_patterns)} raw), 6 swept-clear traffic loops")


if __name__=="__main__":
    (ROOT/"content/districts").mkdir(parents=True,exist_ok=True)
    for district in DISTRICTS:generate(district,check="--check" in sys.argv)
