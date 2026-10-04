"""Generate/check original Port Lands PNG, attributes and collision metadata.

Only explicit invocation writes source/background assets. This never registers
a GB Studio scene, edits engine files or builds a ROM. Official images/maps
are not inputs. --dry-run validates in memory without writing assets.
"""
import argparse
import hashlib
import io
import json
from collections import deque
from pathlib import Path
from PIL import Image, ImageDraw
from port_lands_layout import PORT_LANDS, RESEARCH, WIDTH, HEIGHT, ROAD_HALF, WALK_HALF, extended_points

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "project/original-art"
COLORS = ["#071821", "#306850", "#86c06c", "#e0f8cf"]
TW, TH = WIDTH // 8, HEIGHT // 8


def paint_path(draw, points, half, fill):
    for (x1,y1),(x2,y2) in zip(points,points[1:]):
        assert x1 == x2 or y1 == y2, ("noncardinal path",x1,y1,x2,y2)
        draw.rectangle((min(x1,x2)-half,min(y1,y2)-half,max(x1,x2)+half-1,max(y1,y2)+half-1),fill=fill)


def overlap(a,b):
    x,y,w,h=a;xx,yy,ww,hh=b
    return x<xx+ww and xx<x+w and y<yy+hh and yy<y+h


def render():
    image=Image.new("RGB",(WIDTH,HEIGHT),COLORS[2]);draw=ImageDraw.Draw(image)
    road=Image.new("1",(WIDTH,HEIGHT));walk=Image.new("1",(WIDTH,HEIGHT))
    water=Image.new("1",(WIDTH,HEIGHT));deck=Image.new("1",(WIDTH,HEIGHT))
    water_draw=ImageDraw.Draw(water);deck_draw=ImageDraw.Draw(deck)
    road_draw,walk_draw=ImageDraw.Draw(road),ImageDraw.Draw(walk)
    attrs=[6]*(TW*TH);collisions=[16]*(TW*TH);blocks=[];canopies=[]

    def box(x,y,w,h,color):
        assert w>0 and h>0,("invalid box",x,y,w,h)
        draw.rectangle((x,y,x+w-1,y+h-1),fill=COLORS[color])

    def cells(x,y,w,h):
        for ty in range(max(0,y//8),min(TH,(y+h+7)//8)):
            for tx in range(max(0,x//8),min(TW,(x+w+7)//8)):
                yield tx,ty,ty*TW+tx

    def solid(x,y,w,h):
        for _,_,i in cells(x,y,w,h):collisions[i]=15

    def attr(x,y,w,h,slot,priority=False):
        assert 0<=slot<=6
        for tx,ty,i in cells(x,y,w,h):
            attrs[i]=slot|(128 if priority and not road.getpixel((tx*8+4,ty*8+4)) else 0)

    for item in PORT_LANDS["water"]+PORT_LANDS["wetlands"]:
        x,y,w,h=item["rect"]
        assert x%8==y%8==w%8==h%8==0,item
        water_draw.rectangle((x,y,x+w-1,y+h-1),fill=1)
    for bridge in PORT_LANDS["bridges"]:paint_path(deck_draw,bridge["points"],WALK_HALF,1)
    for route in PORT_LANDS["roads"]:
        points=extended_points(route)
        paint_path(road_draw,points,ROAD_HALF,1);paint_path(walk_draw,points,WALK_HALF,1)
    for route in PORT_LANDS["footpaths"]:paint_path(walk_draw,route["points"],8,1)
    for apron in PORT_LANDS["aprons"]:
        x,y,w,h=apron["rect"]
        road_draw.rectangle((x,y,x+w-1,y+h-1),fill=1)
        walk_draw.rectangle((x,y,x+w-1,y+h-1),fill=1)

    # Streets cannot paint an accidental ford: clear every water/wetland tile
    # outside a separately authored supported bridge deck.
    water_cells=[]
    for ty in range(TH):
        for tx in range(TW):
            i=ty*TW+tx;x,y=tx*8,ty*8
            wet=water.getpixel((x+4,y+4));bridge=deck.getpixel((x+4,y+4))
            if wet:
                water_cells.append(i);box(x,y,8,8,0);collisions[i]=15;attrs[i]=0
                if not bridge:
                    road_draw.rectangle((x,y,x+7,y+7),fill=0)
                    walk_draw.rectangle((x,y,x+7,y+7),fill=0)
            if walk.getpixel((x+4,y+4)):box(x,y,8,8,3);collisions[i]=16;attrs[i]=0
            if road.getpixel((x+4,y+4)):box(x,y,8,8,1);collisions[i]=0;attrs[i]=0
    # Minimal original wave module, never applied over a bridge or road.
    for y in range(152,HEIGHT-8,32):
        for x in range(8,WIDTH-8,32):
            if water.getpixel((x+4,y+4)) and not deck.getpixel((x+4,y+4)):
                box(x,y,8,1,2)
    for wetland in PORT_LANDS["wetlands"]:
        x,y,w,h=wetland["rect"]
        box(x,y,w,h,1);attr(x,y,w,h,6)
        for py in range(y+8,y+h-8,16):
            for px in range(x+8,x+w-8,16):box(px,py,2,8,2)
    for park in PORT_LANDS["parks"]:
        x,y,w,h=park["rect"]
        for tx,ty,i in cells(x,y,w,h):
            if collisions[i]==16 and not walk.getpixel((tx*8+4,ty*8+4)):
                box(tx*8,ty*8,8,8,2);attrs[i]=6
    # Sand remains pedestrian terrain; a repeated eight-pixel grain module.
    for y in range(880,928,8):
        for x in range(112,704,8):
            if not water.getpixel((x+4,y+4)) and not road.getpixel((x+4,y+4)):
                box(x,y,8,8,3);box(x+2,y+2,1,1,2);attrs[(y//8)*TW+x//8]=6
    for route in PORT_LANDS["roads"]:
        for (x1,y1),(x2,y2) in zip(route["points"],route["points"][1:]):
            if y1==y2:
                for x in range((min(x1,x2)//32+1)*32,max(x1,x2),32):
                    if road.getpixel((x,y1)):box(x,y1,8,1,3)
            else:
                for y in range((min(y1,y2)//32+1)*32,max(y1,y2),32):
                    if road.getpixel((x1,y)):box(x1,y,1,8,3)
    for bridge in PORT_LANDS["bridges"]:
        (x1,y1),(x2,y2)=bridge["points"]
        x,y=min(x1,x2),min(y1,y2);w,h=abs(x2-x1),abs(y2-y1)
        if x1==x2:
            for offset in (-WALK_HALF+2,WALK_HALF-3):draw.line((x+offset,y,x+offset,y+h),fill=COLORS[0],width=2)
            attr(x-WALK_HALF,y,WALK_HALF*2,h,bridge["palette"])
        else:
            for offset in (-WALK_HALF+2,WALK_HALF-3):draw.line((x,y+offset,x+w,y+offset),fill=COLORS[0],width=2)
            attr(x,y-WALK_HALF,w,WALK_HALF*2,bridge["palette"])

    # Solid outer cutlines are game frontiers, not municipal borders or closures.
    # Only Leslie is open. Withheld Core/Carlaw roads end at safe inset points.
    for x,y,w,h in [(0,0,WIDTH,16),(0,HEIGHT-16,WIDTH,16),(0,0,16,HEIGHT),(WIDTH-16,0,16,HEIGHT)]:
        box(x,y,w,h,1);solid(x,y,w,h);attr(x,y,w,h,0)
    for port in PORT_LANDS["ports"]:
        assert port["edge"]=="north"
        x=port["x"]
        box(x-WALK_HALF,0,WALK_HALF*2,16,3)
        box(x-ROAD_HALF,0,ROAD_HALF*2,16,1)
        for _,_,i in cells(x-WALK_HALF,0,WALK_HALF*2,16):collisions[i]=16;attrs[i]=0
        for _,_,i in cells(x-ROAD_HALF,0,ROAD_HALF*2,16):collisions[i]=0

    reserved=[tuple(p["rect"]) for p in PORT_LANDS["parks"]]
    reserved += [(p["x"]-8,p["y"]-8,p["width"]+16,p["depth"]+16) for p in PORT_LANDS["landmarks"]]
    for stop in PORT_LANDS["stop_candidates"]:
        reserved.append((stop["x"]-16,stop["y"]-16,32,32))
        if stop.get("parking_anchor"):
            x,y=stop["parking_anchor"];reserved.append((x-16,y-16,32,32))

    def may_build(x,y,w,h,landmark=False):
        if x<24 or y<40 or x+w>WIDTH-24 or y+h>HEIGHT-40:return False
        footprint=(x,y,w,h) if landmark else (x-8,y-8,w+16,h+16)
        if not landmark and any(overlap(footprint,r) for r in reserved):return False
        return all(not walk.getpixel((tx*8+4,ty*8+4)) and collisions[i]!=15 for tx,ty,i in cells(*footprint))

    def building(x,y,w,h,style,kind=None,name=None):
        assert may_build(x,y,w,h,landmark=bool(name)),("building footprint",name,x,y,w,h)
        roof=16 if style in (3,4) and h>=40 else 8
        # Decorative shadows clip at pavement/water instead of covering lanes.
        for py in range(y+4,y+h+4):
            for px in range(x+4,x+w+4):
                if not road.getpixel((px,py)) and not water.getpixel((px,py)):draw.point((px,py),fill=COLORS[0])
        box(x,y,w,h,1);draw.rectangle((x,y,x+w-1,y+h-1),outline=COLORS[0])
        box(x+2,y+2,w-4,max(4,h-roof-2),2)
        draw.rectangle((x+4,y+4,x+w-5,y+h-roof-3),outline=COLORS[0])
        box(x,y-8,w,8,2);draw.line((x,y-8,x+w-1,y-8),fill=COLORS[0])
        for xx in range(x+4,x+w-4,8):box(xx,y+h-roof+3,4,3,3)
        box(x+w//2-2,y+h-5,4,4,0)
        if style==0:  # workshop with awning and utility roof
            for xx in range(x+2,x+w-2,8):box(xx,y+h-8,4,3,3)
            box(x+8,y+8,8,8,1)
        elif style==1:  # brick warehouse bays
            for yy in range(y+8,y+h-roof,8):
                for xx in range(x+8,x+w-8,16):box(xx,yy,8,2,1)
        elif style==2:  # pitched small service structure
            draw.line((x+w//2,y+4,x+w//2,y+h-roof-4),fill=COLORS[0],width=2)
            box(x+4,y+8,4,4,1)
        elif style==3:  # stepped civic/industrial volume
            draw.rectangle((x+8,y+8,x+w-9,y+h-roof-7),outline=COLORS[3])
        elif style==4:  # restrained modern office roof, no future towers
            for xx in range(x+8,x+w-8,8):draw.line((xx,y+4,xx,y+h-roof-4),fill=COLORS[3])
            box(x+8,y+8,8,8,1)
        else:  # broad corrugated shed / loading doors
            for xx in range(x+8,x+w-8,16):box(xx,y+8,8,8,3);box(xx,y+h-6,8,4,0)
        if kind in ("soundstage","sawtooth"):
            for yy in range(y+16,y+h-roof-8,16):
                draw.line((x+8,yy,x+w-9,yy),fill=COLORS[0],width=2)
            for xx in range(x+8,x+w-8,16):box(xx,y+h-12,8,8,0)
        elif kind=="fire_hall":
            box(x+4,y+4,8,8,0);box(x+6,y+6,4,4,3)
            for xx in range(x+4,x+w-4,12):box(xx,y+h-12,8,8,0)
        elif kind=="tank_shed":
            for xx in range(x+8,x+w-16,24):
                draw.ellipse((xx,y+8,xx+15,y+23),fill=COLORS[3],outline=COLORS[0])
                box(xx+6,y+12,4,8,1)
        solid(x,y,w,h);attr(x,y-8,w+8,h+16,style+1,True)
        if kind=="hearn":
            # Original tall stack projects north from a base inside the hall.
            sx=x+w-16;box(sx,y-64,8,80,0);box(sx+2,y-62,4,76,3)
            attr(sx,y-64,8,80,3,True)
            for xx in range(x+16,x+w-16,24):box(xx,y+24,16,24,1)
        elif kind=="crane":
            sx=x+4;box(sx,y-40,4,48,0);box(x-8,y-40,32,4,1)
            draw.line((x-8,y-36,x+12,y-16),fill=COLORS[0],width=2)
            draw.line((x+20,y-36,x+4,y-16),fill=COLORS[0],width=2)
            attr(x-8,y-48,32,64,5,True)
        blocks.append({"x":x,"y":y,"width":w,"depth":h,"height":roof,"style":style,"landmark":name,"kind":kind})

    for landmark in PORT_LANDS["landmarks"]:
        building(landmark["x"],landmark["y"],landmark["width"],landmark["depth"],landmark["style"],landmark["kind"],landmark["name"])
    # Varied industrial massing is limited to explicitly authored existing-use
    # zones. Ookwemin's future residential plan is not filled with invented towers.
    for yy in range(48,HEIGHT-56,32):
        for xx in range(48,WIDTH-48,32):
            if not any(zx<=xx<zx+zw and zy<=yy<zy+zh for zx,zy,zw,zh in PORT_LANDS["industrial_zones"]):continue
            style=((xx//32)*3+yy//32)%6
            w,h=(96,48) if style==5 else (64,40) if style==1 else (40,48) if style==3 else (32,32)
            kind="tank_shed" if style==3 else None
            if may_build(xx,yy,w,h):building(xx,yy,w,h,style,kind)
            elif may_build(xx,yy,32,24):building(xx,yy,32,24,style)
    for park in PORT_LANDS["parks"]:
        x,y,w,h=park["rect"]
        for yy in range((y+7)//8*8,y+h-23,32):
            for xx in range((x+7)//8*8,x+w-15,32):
                if any(walk.getpixel((px,py)) or water.getpixel((px,py)) for py in range(yy,yy+24) for px in range(xx,xx+16)):continue
                if any(collisions[i]==15 for _,_,i in cells(xx,yy,16,24)):continue
                box(xx+6,yy+16,3,8,0)
                draw.ellipse((xx,yy,xx+15,yy+15),fill=COLORS[1],outline=COLORS[0])
                box(xx+4,yy+4,8,8,2);solid(xx,yy+16,16,8)
                attr(xx,yy,16,16,6,True);canopies.append([xx,yy,16,16])

    def clear(x,y,half=2,car=False):
        if not (0<=x-half and x+half<WIDTH and 0<=y-half and y+half<HEIGHT):return False
        return all(c==0 if car else not c&15 for _,_,i in cells(x-half,y-half,half*2+1,half*2+1) for c in [collisions[i]])

    def sweep(points,half=8,car=True,closed=False):
        pairs=zip(points,points[1:]+points[:1] if closed else points[1:])
        for (x1,y1),(x2,y2) in pairs:
            assert x1==x2 or y1==y2,("noncardinal sweep",points)
            for step in range(abs(x2-x1)+abs(y2-y1)+1):
                x=x1+(step if x2>x1 else -step if x2<x1 else 0)
                y=y1+(step if y2>y1 else -step if y2<y1 else 0)
                assert clear(x,y,half,car),("swept footprint",x,y,half,car)

    for route in PORT_LANDS["roads"]:sweep(route["points"])
    for bridge in PORT_LANDS["bridges"]:
        sweep(bridge["points"])
        (x1,y1),(x2,y2)=bridge["points"]
        offsets=range(-18,19)
        for offset in offsets:
            points=[[x+offset,y] if x1==x2 else [x,y+offset] for x,y in bridge["points"]]
            sweep(points,half=5,car=True)
    for route in PORT_LANDS["footpaths"]:sweep(route["points"],half=2,car=False)
    for loop in PORT_LANDS["traffic_loops"]:
        assert 4<=len(loop)<=16;sweep(loop,closed=True)
    for port in PORT_LANDS["ports"]+PORT_LANDS["conditional_ports"]:
        ys=range(20,29) if port in PORT_LANDS["ports"] else (24,)
        for y in ys:
            for offset in range(-18,19):assert clear(port["x"]+offset,y,5,True),("car approach",port,offset)
            for offset in range(-28,29):assert clear(port["x"]+offset,y),("foot approach",port,offset)
        if port in PORT_LANDS["conditional_ports"]:
            assert collisions[port["x"]//8]==15,("withheld edge must be closed",port)
    for stop in PORT_LANDS["stop_candidates"]:
        assert clear(stop["x"],stop["y"],2 if stop["foot_only"] else 8,not stop["foot_only"]),("client",stop)
        if stop.get("parking_anchor"):assert clear(*stop["parking_anchor"],8,True),("parking anchor",stop)
        if stop["foot_only"]:assert not clear(stop["x"],stop["y"],5,True),("foot-only handoff",stop)
    for name,x,y in [("Keating",320,192),("Don",464,224),("mouth",320,496),("Ship Channel",544,672),("Turning Basin",808,640),("circulation channel",808,880),("wetland",464,592),("no Villiers ford",464,240)]:
        assert not clear(x,y,2) and not clear(x,y,8,True),("blocked water barrier",name)
    for i in water_cells:
        x,y=(i%TW)*8+4,(i//TW)*8+4
        assert deck.getpixel((x,y)) or collisions[i]==15,("accidental water crossing",i)

    def connected(car):
        start=PORT_LANDS["ports"][0]
        origin=(start["x"]//8,start["y"]//8);visited={origin};queue=deque([origin]);half=8 if car else 2
        assert clear(origin[0]*8+4,origin[1]*8+4,half,car)
        while queue:
            tx,ty=queue.popleft()
            for nx,ny in ((tx-1,ty),(tx+1,ty),(tx,ty-1),(tx,ty+1)):
                if (nx,ny) in visited or not clear(nx*8+4,ny*8+4,half,car):continue
                visited.add((nx,ny));queue.append((nx,ny))
        return visited

    foot_visited,car_visited=connected(False),connected(True)
    for point in PORT_LANDS["ports"]+PORT_LANDS["conditional_ports"]+PORT_LANDS["stop_candidates"]:
        key=(point["x"]//8,point["y"]//8)
        assert key in foot_visited,("foot connectivity",point)
        if not point["foot_only"]:assert key in car_visited,("full-car connectivity",point)
        if point.get("parking_anchor"):
            x,y=point["parking_anchor"];assert (x//8,y//8) in car_visited,("parking connectivity",point)
    peds=set()
    for route in PORT_LANDS["roads"]+PORT_LANDS["footpaths"]:
        for a,b in zip(route["points"],route["points"][1:]):
            if a[1]!=b[1]:continue
            left,right=sorted((a[0],b[0]));offsets=(-ROAD_HALF-4,ROAD_HALF+4) if route in PORT_LANDS["roads"] else (0,)
            for offset in offsets:
                y=a[1]+offset
                for x in range((left+7)//8*8,right-62,64):
                    if all(clear(px,y) for px in range(x,x+64)):peds.add((x,y))
    peds=sorted(peds,key=lambda p:(p[1],p[0]));assert len(peds)>=24,("pedestrian route count",len(peds))
    if len(peds)>128:peds=[peds[i*len(peds)//128] for i in range(128)]
    for x,y in peds:sweep([[x,y],[x+63,y]],half=2,car=False)

    raw_patterns,patterns=set(),set()
    for ty in range(TH):
        for tx in range(TW):
            tile=image.crop((tx*8,ty*8,tx*8+8,ty*8+8));raw_patterns.add(tile.tobytes())
            variants=[tile,tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT),tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM),tile.transpose(Image.Transpose.ROTATE_180)]
            patterns.add(min(t.tobytes() for t in variants))
    assert len(raw_patterns)<=320,("Port Lands raw tile budget",len(raw_patterns))
    assert len(patterns)<=320
    assert set(image.get_flattened_data())<=set(tuple(bytes.fromhex(c[1:])) for c in COLORS)
    assert len(collisions)==15616 and set(collisions)<={0,16,15}
    for block in blocks:
        assert all(collisions[i]==15 for _,_,i in cells(block["x"],block["y"],block["width"],block["depth"])),block
    assert all((a&7)<=6 for a in attrs)
    assert all(not(a&128) for a,c in zip(attrs,collisions) if c==0)
    buffer=io.BytesIO();image.save(buffer,format="PNG");png=buffer.getvalue()
    metadata={**PORT_LANDS,
        "status":"Generated original source asset; native registration/build/play/hardware remain separate gates.",
        "projection":"Original compressed orthogonal north-up; not projected GIS geometry or a complete municipality clip.",
        "dimensions":[WIDTH,HEIGHT],"tile_dimensions":[TW,TH],"road_half_width":ROAD_HALF,"walk_half_width":WALK_HALF,
        "water_bodies":PORT_LANDS["water"],"water":[w["rect"] for w in PORT_LANDS["water"]],"rails":[],
        "blocks":blocks,"canopies":canopies,"collisions":collisions,
        "collision_rules":{"road":0,"foot_only":16,"solid":15},
        "background_filename":"toronto_port_lands.png","source_png":"project/original-art/toronto_port_lands.png",
        "background_sha256":hashlib.sha256(png).hexdigest(),"source_research":"docs/PORT_LANDS_PLAN.md","research":RESEARCH,
        "pedestrian_routes":[{"x":x,"y":y,"axis":"horizontal","length_pixels":63} for x,y in peds],
        "validation":{"raw_unique_tiles":len(raw_patterns),"flip_canonical_unique_tiles":len(patterns),"tile_budget":320,
            "traffic_loops":6,"traffic_footprint_half_pixels":8,"traffic_swept_all_overlapped_tiles_clear":True,
            "supported_bridge_decks":6,"all_road_and_bridge_centreline_footprints_swept_clear":True,
            "water_outside_authored_decks_solid":True,"no_added_villiers_car_bridge":True,
            "portal_native_car_half_pixels":5,"portal_car_offsets_verified":list(range(-18,19)),
            "portal_foot_offsets_verified":list(range(-28,29)),"north_portal_swept_y_verified":[20,28],
            "conditional_ports_arrival_only_y_verified":24,"all_foot_clients_and_approaches_connected":True,
            "all_car_clients_and_park_anchors_conservatively_connected":True,"fixed_pedestrian_routes":len(peds),
            "compiled_background_bank1_limit":32,"compiled_background_bank1_gate_verified":False,
            "native_registration_verified":False,"native_build_verified":False,"measured_gameplay_duration_verified":False}}
    return png,attrs,metadata


def same_png_artwork(expected,actual):
    with Image.open(io.BytesIO(expected)) as a,Image.open(io.BytesIO(actual)) as b:
        return (a.format==b.format=="PNG" and a.mode==b.mode and a.size==b.size and
                a.getpalette()==b.getpalette() and a.info==b.info and a.tobytes()==b.tobytes())


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    modes=parser.add_mutually_exclusive_group()
    modes.add_argument("--check",action="store_true")
    modes.add_argument("--dry-run",action="store_true")
    args=parser.parse_args();png,attrs,metadata=render()
    if args.check:
        canonical=(ART/"toronto_port_lands.png").read_bytes()
        assert same_png_artwork(png,canonical),"Port Lands source pixels/palette differ"
        png=canonical;metadata["background_sha256"]=hashlib.sha256(png).hexdigest()
    if not args.dry_run:
        files={ART/"toronto_port_lands.png":png,ROOT/"project/assets/backgrounds/toronto_port_lands.png":png,
               ART/"port_lands_attributes.json":(json.dumps(attrs)+"\n").encode(),
               ROOT/"content/districts/port_lands_art.json":(json.dumps(metadata,indent=2)+"\n").encode()}
        for filename,data in files.items():
            if args.check:assert filename.read_bytes()==data,f"Port Lands source differs: {filename}"
            else:filename.parent.mkdir(parents=True,exist_ok=True);filename.write_bytes(data)
    v=metadata["validation"];state="validated in memory" if args.dry_run else "matches" if args.check else "generated"
    print(f"Port Lands source {state}: {len(metadata['blocks'])} buildings, {v['raw_unique_tiles']} raw/{v['flip_canonical_unique_tiles']} flipped tiles, 6 swept-clear traffic loops, {v['fixed_pedestrian_routes']} foot-safe routes. Native build unverified.")


if __name__=="__main__":main()
