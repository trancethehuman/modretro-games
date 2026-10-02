"""Generate/check original eastern source PNG, attributes and collision metadata.

This never registers a GB Studio scene, edits runtime source, or builds a ROM.
Pillow paints exact manual-palette pixels; official maps/images are not inputs.
"""
import argparse
import hashlib
import io
import json
from collections import deque
from pathlib import Path
from PIL import Image, ImageDraw
from east_layout import EAST, RESEARCH, WIDTH, HEIGHT, ROAD_HALF, WALK_HALF, extended_points

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "project/original-art"
COLORS = ["#071821", "#306850", "#86c06c", "#e0f8cf"]
TW, TH = WIDTH // 8, HEIGHT // 8


def paint_path(draw, points, half, fill):
    for (x1,y1),(x2,y2) in zip(points,points[1:]):
        assert x1 == x2 or y1 == y2, (x1,y1,x2,y2)
        draw.rectangle((min(x1,x2)-half,min(y1,y2)-half,max(x1,x2)+half-1,max(y1,y2)+half-1),fill=fill)


def rail_stairs(points):
    """Original 8px stepped interpretation reduces rail tile variety and stays north-up."""
    result=[points[0][:]]
    for a,b in zip(points,points[1:]):
        steps=max(abs(b[0]-a[0]),abs(b[1]-a[1]))//8
        for step in range(1,steps+1):
            x=round((a[0]+(b[0]-a[0])*step/steps)/8)*8
            y=round((a[1]+(b[1]-a[1])*step/steps)/8)*8
            if result[-1][0]!=x:result.append([x,result[-1][1]])
            if result[-1][1]!=y:result.append([x,y])
    return result


def render():
    img=Image.new("RGB",(WIDTH,HEIGHT),COLORS[2]);d=ImageDraw.Draw(img)
    road=Image.new("1",(WIDTH,HEIGHT));walk=Image.new("1",(WIDTH,HEIGHT))
    rd,wd=ImageDraw.Draw(road),ImageDraw.Draw(walk)
    attrs=[6]*(TW*TH);collisions=[16]*(TW*TH);blocks=[];canopies=[]

    def box(x,y,w,h,color):
        d.rectangle((x,y,x+w-1,y+h-1),fill=COLORS[color])

    def cells(x,y,w,h):
        for ty in range(max(0,y//8),min(TH,(y+h+7)//8)):
            for tx in range(max(0,x//8),min(TW,(x+w+7)//8)):
                yield tx,ty,ty*TW+tx

    def solid(x,y,w,h):
        for _,_,i in cells(x,y,w,h):collisions[i]=15

    def attr(x,y,w,h,slot,priority=False):
        for tx,ty,i in cells(x,y,w,h):
            attrs[i]=slot|(128 if priority and not road.getpixel((tx*8+4,ty*8+4)) else 0)

    for x,y,w,h in [(0,0,WIDTH,16),(0,HEIGHT-16,WIDTH,16),(0,0,16,HEIGHT),(WIDTH-16,0,16,HEIGHT)]:
        box(x,y,w,h,1);solid(x,y,w,h)
    for frontier in EAST["closed_frontiers"]:
        x,y,w,h=frontier["rect"];box(x,y,w,h,1);solid(x,y,w,h);attr(x,y,w,h,0)
        for px in range(24,1000,32):box(px,y,8,2,3)
    for park in EAST["parks"]:
        x,y,w,h=park["rect"];box(x,y,w,h,2);attr(x,y,w,h,6)
    rail_masks=[]
    for rail in EAST["rails"]:
        points=rail_stairs(rail["points"])
        mask=Image.new("1",(WIDTH,HEIGHT));paint_path(ImageDraw.Draw(mask),points,8,1)
        paint_path(d,points,8,COLORS[0]);paint_path(d,points,1,COLORS[3]);rail_masks.append(mask)
        for ty in range(TH):
            for tx in range(TW):
                if mask.crop((tx*8,ty*8,tx*8+8,ty*8+8)).getbbox():collisions[ty*TW+tx]=15;attrs[ty*TW+tx]=0
    for route in EAST["roads"]:
        points=extended_points(route);paint_path(wd,points,WALK_HALF,1);paint_path(rd,points,ROAD_HALF,1)
    for route in EAST["footpaths"]:paint_path(wd,route["points"],8,1)
    for ty in range(TH):
        for tx in range(TW):
            i=ty*TW+tx;x=tx*8;y=ty*8
            if walk.getpixel((x+4,y+4)):box(x,y,8,8,3);collisions[i]=16;attrs[i]=0
            if road.getpixel((x+4,y+4)):box(x,y,8,8,1);collisions[i]=0;attrs[i]=0
    for route in EAST["roads"]:
        for (x1,y1),(x2,y2) in zip(route["points"],route["points"][1:]):
            if y1==y2:
                for px in range((min(x1,x2)//32+1)*32,max(x1,x2),32):box(px,y1,8,1,3)
            else:
                for py in range((min(y1,y2)//32+1)*32,max(y1,y2),32):box(x1,py,1,8,3)
    # The conditional Gerrard connector remains visibly closed at the viewport edge.
    for port in EAST["conditional_ports"]:
        x,y,w,h=0,port["y"]-WALK_HALF,16,WALK_HALF*2
        box(x,y,w,h,0);solid(x,y,w,h);attr(x,y,w,h,0)
        for py in range(y+8,y+h,16):box(4,py,8,2,3)

    reserved=[tuple(p["rect"]) for p in EAST["parks"]]
    reserved += [(p["x"]-8,p["y"]-16,p["width"]+24,p["depth"]+32) for p in EAST["landmarks"]]

    def overlap(a,b):
        x,y,w,h=a;xx,yy,ww,hh=b
        return x<xx+ww and xx<x+w and y<yy+hh and yy<y+h

    def may_build(x,y,w,h,landmark=False):
        if x<32 or y<40 or x+w>WIDTH-32 or y+h>800:return False
        footprint=(x-8,y-8,w+16,h+16)
        if not landmark and any(overlap(footprint,r) for r in reserved):return False
        return all(not walk.getpixel((tx*8+4,ty*8+4)) and collisions[i]!=15 for tx,ty,i in cells(*footprint))

    def building(x,y,w,h,style,kind=None,name=None):
        assert may_build(x,y,w,h,landmark=bool(name)),(name,x,y,w,h)
        roof=16 if style in (3,4) and h>=40 else 8
        box(x+4,y+4,w,h,0);box(x,y,w,h,1)
        d.rectangle((x,y,x+w-1,y+h-1),outline=COLORS[0])
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
        elif style==2:
            d.line((x+w//2,y+4,x+w//2,y+h-roof-4),fill=COLORS[0],width=2);box(x+4,y+8,4,4,1)
        elif style==3:d.rectangle((x+8,y+8,x+w-9,y+h-roof-7),outline=COLORS[3])
        elif style==4:
            for wx in range(x+8,x+w-8,8):d.line((wx,y+4,wx,y+h-roof-4),fill=COLORS[3])
            box(x+8,y+8,8,8,1)
        else:
            for wx in range(x+8,x+w-8,16):box(wx,y+8,8,8,3);box(wx,y+h-6,8,4,0)
        if kind=="music_hall":
            box(x+16,y+16,w-32,8,1)
            for wx in range(x+8,x+w-8,16):box(wx,y+h-12,8,8,0)
        elif kind=="theatre":
            box(x+8,y+8,w-16,4,3)
            for wx in range(x+8,x+w-8,16):d.arc((wx,y+12,wx+8,y+24),180,360,fill=COLORS[0],width=2)
        elif kind=="heritage_house":
            box(x+8,y+8,4,8,0);box(x+w-12,y+8,4,8,0)
            for wx in range(x+8,x+w-8,16):box(wx,y+h-12,8,6,3)
        elif kind=="factory":
            for wx in range(x+8,x+w-8,16):box(wx,y+24,8,8,1)
            box(x+w-16,y-8,8,16,0)
        solid(x,y,w,h);attr(x,y-8,w+8,h+16,style+1,True)
        blocks.append({"x":x,"y":y,"width":w,"depth":h,"height":roof,"style":style,"landmark":name,"kind":kind})

    for landmark in EAST["landmarks"]:
        building(landmark["x"],landmark["y"],landmark["width"],landmark["depth"],landmark["style"],landmark["kind"],landmark["name"])
    # Low brick terraces, pitched homes, tall glass blocks and wide work sheds.
    for yy in range(48,800,32):
        for xx in range(48,976,32):
            style=((xx//32)*3+yy//32)%6
            w=40 if style==3 else 48 if style==5 else 32
            h=48 if style==4 else 40 if style==3 else 32 if style==1 else 24
            if may_build(xx,yy,w,h):building(xx,yy,w,h,style)
            elif (w,h)!=(32,24) and may_build(xx,yy,32,24):
                building(xx,yy,32,24,style)
    for park in EAST["parks"]:
        x,y,w,h=park["rect"]
        for yy in range((y+7)//8*8,y+h-23,32):
            for xx in range((x+7)//8*8,x+w-15,32):
                if any(walk.getpixel((px,py)) for py in range(yy,yy+24) for px in range(xx,xx+16)):continue
                if any(collisions[i]==15 for _,_,i in cells(xx,yy,16,24)):continue
                box(xx+6,yy+16,3,8,0);d.ellipse((xx,yy,xx+15,yy+15),fill=COLORS[1],outline=COLORS[0]);box(xx+4,yy+4,8,8,2)
                solid(xx,yy+16,16,8);attr(xx,yy,16,16,6,True);canopies.append([xx,yy,16,16])

    def clear(x,y,half=2,car=False):
        if not (0<=x-half and x+half<WIDTH and 0<=y-half and y+half<HEIGHT):return False
        return all(c==0 if car else not c&15 for _,_,i in cells(x-half,y-half,half*2+1,half*2+1) for c in [collisions[i]])

    def sweep(points,half=8,car=True,closed=False):
        pairs=zip(points,points[1:]+points[:1] if closed else points[1:])
        for (x1,y1),(x2,y2) in pairs:
            assert x1==x2 or y1==y2,points
            for step in range(abs(x2-x1)+abs(y2-y1)+1):
                x=x1+(step if x2>x1 else -step if x2<x1 else 0);y=y1+(step if y2>y1 else -step if y2<y1 else 0)
                assert clear(x,y,half,car), ("swept clearance",x,y,half,car)

    for loop in EAST["traffic_loops"]:
        assert 4<=len(loop)<=16;sweep(loop,closed=True)
    for port in EAST["ports"]+EAST["conditional_ports"]:
        # A closed conditional connector has no swept portal approach yet.
        approach=range(20,29) if port in EAST["ports"] else (24,)
        for x in approach:
            for offset in range(-18,19):assert clear(x,port["y"]+offset,5,True),("native car seam",port,offset)
            for offset in range(-28,29):assert clear(x,port["y"]+offset),("native foot seam",port,offset)
    for stop in EAST["stop_candidates"]:
        assert clear(stop["x"],stop["y"],2 if stop["foot_only"] else 8,not stop["foot_only"]),stop
        if stop.get("parking_anchor"):assert clear(*stop["parking_anchor"],8,True),stop
    for route in EAST["footpaths"]:sweep(route["points"],half=2,car=False)
    assert collisions[(248//8)*TW+544//8]==16,"Pape's rail connection must remain foot-only"
    assert not clear(544,248,5,True),"Pape must not acquire an invented car bridge"

    start=EAST["stop_candidates"][0];visited={(start["x"]//8,start["y"]//8)};queue=deque(visited)
    while queue:
        tx,ty=queue.popleft()
        for nx,ny in ((tx-1,ty),(tx+1,ty),(tx,ty-1),(tx,ty+1)):
            if 0<=nx<TW and 0<=ny<TH and (nx,ny) not in visited and not collisions[ny*TW+nx]&15:
                visited.add((nx,ny));queue.append((nx,ny))
    for point in EAST["ports"]+EAST["conditional_ports"]+EAST["stop_candidates"]:
        assert (point["x"]//8,point["y"]//8) in visited,("foot connectivity",point)
    # Conservative full-car tile-centre graph joins all road clients/park anchors.
    origin=(start["x"]//8,start["y"]//8);car_visited={origin};queue=deque([origin])
    assert clear(origin[0]*8+4,origin[1]*8+4,8,True)
    while queue:
        tx,ty=queue.popleft()
        for nx,ny in ((tx-1,ty),(tx+1,ty),(tx,ty-1),(tx,ty+1)):
            if (nx,ny) in car_visited or not clear(nx*8+4,ny*8+4,8,True):continue
            car_visited.add((nx,ny));queue.append((nx,ny))
    car_points=[p for p in EAST["ports"]+EAST["stop_candidates"] if not p["foot_only"]]
    car_points += [{"x":p["parking_anchor"][0],"y":p["parking_anchor"][1]} for p in EAST["stop_candidates"] if p.get("parking_anchor")]
    for point in car_points:assert (point["x"]//8,point["y"]//8) in car_visited,("conservative car connectivity",point)
    # Fixed horizontal 63px walks use actual footprint-safe sidewalk tiles.
    peds=set()
    for route in EAST["roads"]+EAST["footpaths"]:
        for a,b in zip(route["points"],route["points"][1:]):
            if a[1]!=b[1]:continue
            left,right=sorted((a[0],b[0]));offsets=(-ROAD_HALF-4,ROAD_HALF+4) if route in EAST["roads"] else (0,)
            for offset in offsets:
                y=a[1]+offset
                for x in range((left+7)//8*8,right-62,64):
                    if all(clear(px,y) for px in range(x,x+64)):peds.add((x,y))
    peds=sorted(peds,key=lambda p:(p[1],p[0]))
    assert len(peds)>=24
    if len(peds)>128:peds=[peds[i*len(peds)//128] for i in range(128)]
    for x,y in peds:sweep([[x,y],[x+63,y]],half=2,car=False)

    patterns=set();raw_patterns=set()
    for ty in range(TH):
        for tx in range(TW):
            tile=img.crop((tx*8,ty*8,tx*8+8,ty*8+8));raw_patterns.add(tile.tobytes())
            patterns.add(min(t.tobytes() for t in [tile,tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT),tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM),tile.transpose(Image.Transpose.ROTATE_180)]))
    assert len(raw_patterns)<=320,("eastern raw tile budget",len(raw_patterns))
    assert len(patterns)<=320
    assert set(img.get_flattened_data())<=set(tuple(bytes.fromhex(c[1:])) for c in COLORS)
    assert len(collisions)==15616 and set(collisions)<={0,16,15}
    for block in blocks:
        assert all(collisions[i]==15 for _,_,i in cells(block["x"],block["y"],block["width"],block["depth"])),block
    assert all((a&7)<=6 for a in attrs)
    assert all((a&7)<=6 and not(a&128) for a,c in zip(attrs,collisions) if c==0)
    buffer=io.BytesIO();img.save(buffer,format="PNG");png=buffer.getvalue()
    metadata={**EAST,"projection":"Original compressed orthogonal north-up; not projected GIS coordinates or a complete municipal map.","dimensions":[WIDTH,HEIGHT],"tile_dimensions":[TW,TH],"road_half_width":ROAD_HALF,"walk_half_width":WALK_HALF,"blocks":blocks,"canopies":canopies,"collisions":collisions,"collision_rules":{"road":0,"foot_only":16,"solid":15},"background_filename":"toronto_east.png","source_png":"project/original-art/toronto_east.png","background_sha256":hashlib.sha256(png).hexdigest(),"source_research":"docs/EAST_DISTRICT.md","source_research_details":"Reviewed facts, query hashes and licence in the research object of this metadata","research":RESEARCH,"pedestrian_routes":[{"x":x,"y":y,"axis":"horizontal","length_pixels":63} for x,y in peds],"validation":{"raw_unique_tiles":len(raw_patterns),"flip_canonical_unique_tiles":len(patterns),"tile_budget":320,"traffic_loops":6,"traffic_footprint_half_pixels":8,"traffic_swept_all_overlapped_tiles_clear":True,"portal_native_car_half_pixels":5,"portal_car_offsets_verified":list(range(-18,19)),"portal_foot_offsets_verified":list(range(-28,29)),"portal_swept_approach_x_verified":[20,28],"conditional_port_arrival_only_x_verified":24,"all_foot_clients_and_ports_connected":True,"all_car_clients_and_park_anchors_conservatively_connected":True,"fixed_pedestrian_routes":len(peds),"pape_rail_crossing_foot_only":True,"native_registration_verified":False,"native_build_verified":False,"measured_gameplay_duration_verified":False}}
    return png,attrs,metadata


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument("--check",action="store_true");args=parser.parse_args()
    png,attrs,metadata=render()
    files={ART/"toronto_east.png":png,ROOT/"project/assets/backgrounds/toronto_east.png":png,ART/"east_attributes.json":(json.dumps(attrs)+"\n").encode(),ROOT/"content/districts/east_art.json":(json.dumps(metadata,indent=2)+"\n").encode()}
    for filename,data in files.items():
        if args.check:assert filename.read_bytes()==data,f"Eastern source differs: {filename}"
        else:filename.parent.mkdir(parents=True,exist_ok=True);filename.write_bytes(data)
    v=metadata["validation"]
    print(f"Eastern source {'matches' if args.check else 'generated'}: {len(metadata['blocks'])} buildings, {v['raw_unique_tiles']} raw/{v['flip_canonical_unique_tiles']} flipped tiles, 6 swept-clear traffic loops, {v['fixed_pedestrian_routes']} foot-safe routes. Native build unverified.")


if __name__=="__main__":main()
