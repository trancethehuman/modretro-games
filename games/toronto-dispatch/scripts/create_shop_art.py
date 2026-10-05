"""Author three original full-screen native shop backgrounds and collision maps.

Only source PNGs and their manifest are written. Scene registration and native
entry/exit behaviour belong to the integration workflow. No downloaded art,
brand logos, new palette resources or actor objects are used.
"""
import argparse
import hashlib
import io
import json
from collections import deque
from pathlib import Path

from PIL import Image, ImageDraw
from street_scenery import COLORS, RGB, SHOP_ENTRANCES

ROOT=Path(__file__).resolve().parents[1]
ART=ROOT/'project/original-art'
WIDTH,HEIGHT=160,144
TW,TH=20,18


def render(kind):
    image=Image.new('RGB',(WIDTH,HEIGHT),RGB[3]);draw=ImageDraw.Draw(image)
    collisions=[16]*(TW*TH);attrs=[0]*(TW*TH)
    def rect(x,y,w,h,shade,solid=False,slot=0):
        draw.rectangle((x,y,x+w-1,y+h-1),fill=RGB[shade])
        for ty in range(y//8,(y+h+7)//8):
            for tx in range(x//8,(x+w+7)//8):
                if solid:collisions[ty*TW+tx]=15
                attrs[ty*TW+tx]=slot
    # The front room has a clear central aisle, a visible door and full walls.
    rect(0,0,WIDTH,32,1,True,1)
    rect(0,0,WIDTH,8,0,True,1)
    rect(0,32,8,112,1,True,1);rect(152,32,8,112,1,True,1)
    rect(0,136,WIDTH,8,1,True,1)
    for y in range(32,136,8):
        for x in range(8,152,8):
            draw.point((x,y),fill=RGB[2])
    for x in range(16,144,32):
        rect(x,16,24,8,3,True,2)
        draw.line((x+11,16,x+11,23),fill=RGB[0])
    def counter(x,y,w,h,slot=1):
        rect(x,y,w,h,1,True,slot)
        draw.rectangle((x,y,x+w-1,y+h-1),outline=RGB[0])
        draw.line((x+1,y+1,x+w-2,y+1),fill=RGB[3])
    def stock(x,y,w,h,produce=False):
        counter(x,y,w,h,6 if produce else 1)
        for yy in range(y+4,y+h-2,8):
            for xx in range(x+4,x+w-3,8):
                if produce:
                    draw.ellipse((xx,yy,xx+3,yy+3),fill=RGB[2],outline=RGB[0])
                    draw.point((xx+2,yy),fill=RGB[3])
                else:
                    draw.rectangle((xx,yy,xx+3,yy+4),fill=RGB[3],outline=RGB[0])
    if kind=='grocery':
        stock(16,40,40,24,True);stock(104,40,40,24,True)
        stock(24,80,32,32);stock(104,80,32,32)
        counter(64,40,32,16)
        rect(84,43,8,8,0,True,1);rect(86,44,4,4,3,True,1)
    elif kind=='corner_store':
        stock(16,40,40,16);stock(104,40,40,16)
        stock(16,72,40,16);stock(104,72,40,16)
        counter(16,104,40,16)
        rect(42,107,8,8,0,True,1);rect(44,108,4,4,3,True,1)
        # Bright refrigerator doors: three whole repeated cells at the rear.
        for x in (64,80,96):
            rect(x,32,8,24,2,True,2);draw.rectangle((x,32,x+7,55),outline=RGB[0])
            draw.line((x+2,36,x+2,48),fill=RGB[3])
    else:
        counter(16,40,40,16,5);counter(104,40,40,16,5)
        counter(16,80,32,32,5);counter(112,80,32,32,5)
        # Workbenches, a tire stack and original tool silhouettes.
        for x in (24,40,112,128):
            draw.line((x,43,x+6,49),fill=RGB[3],width=2)
            draw.point((x+6,43),fill=RGB[0])
        for y in (82,90,98):
            draw.ellipse((20,y,27,y+7),fill=RGB[0]);draw.ellipse((22,y+2,25,y+5),fill=RGB[3])
        rect(120,88,16,16,2,True,5);draw.rectangle((120,88,135,103),outline=RGB[0])
    # A clear, distinctive door and inward arrow remain inside the foot area.
    rect(64,136,32,8,0,True,1)
    draw.rectangle((72,137,87,143),fill=RGB[3])
    draw.line((80,131,80,135),fill=RGB[0]);draw.line((77,133,80,136,83,133),fill=RGB[0])
    spawn=(80,120);exit=(80,128)
    def clear(x,y):
        return all(not collisions[ty*TW+tx]&15 for ty in range((y-2)//8,(y+2)//8+1)
                   for tx in range((x-2)//8,(x+2)//8+1))
    assert clear(*spawn) and clear(*exit)
    reachable={spawn};queue=deque([spawn])
    while queue:
        x,y=queue.popleft()
        for p in ((x-8,y),(x+8,y),(x,y-8),(x,y+8)):
            if p not in reachable and 8<=p[0]<152 and 32<=p[1]<136 and clear(*p):
                reachable.add(p);queue.append(p)
    assert exit in reachable
    patterns={image.crop((x,y,x+8,y+8)).tobytes() for y in range(0,HEIGHT,8) for x in range(0,WIDTH,8)}
    assert len(patterns)<192 and set(image.get_flattened_data())<=set(RGB)
    stream=io.BytesIO();image.save(stream,format='PNG',compress_level=9);png=stream.getvalue()
    return png,{'key':kind,'filename':'shop_'+kind+'.png','dimensions':[WIDTH,HEIGHT],
        'tile_dimensions':[TW,TH],'spawn':list(spawn),'exit':list(exit),
        'attributes':attrs,'collisions':collisions,'raw_unique_tiles':len(patterns),
        'background_sha256':hashlib.sha256(png).hexdigest(),'original_art':True,
        'new_oam_objects':0,'native_registration_verified':False,'native_gameplay_verified':False}


def same_png_artwork(expected,actual):
    """Platform zlib encodings may differ; native artwork must remain exact."""
    with Image.open(io.BytesIO(expected)) as a,Image.open(io.BytesIO(actual)) as b:
        return (a.format==b.format=='PNG' and a.mode==b.mode and a.size==b.size and
                a.getpalette()==b.getpalette() and a.info==b.info and a.tobytes()==b.tobytes())


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    rooms=[]
    for kind in ('grocery','corner_store','repair_shop'):
        png,room=render(kind);rooms.append(room);path=ART/room['filename']
        if args.check:
            actual=path.read_bytes()
            assert same_png_artwork(png,actual),('Stale original shop pixels/mode/dimensions',kind)
            # Preserve the committed asset's binary identity after checking its
            # artwork; the manifest and registered copy still pin those bytes.
            room['background_sha256']=hashlib.sha256(actual).hexdigest()
        else:path.write_bytes(png)
        print(f"{kind}:160x144,{room['raw_unique_tiles']} raw tiles, original full-screen room with reachable exit")
    metadata={'schema_version':1,'license':'MIT','source':'scripts/create_shop_art.py',
        'notice':'Original fictional shops inside researched compressed Toronto districts; not surveyed commercial entrances',
        'entrances':list(SHOP_ENTRANCES.values()),'rooms':rooms}
    encoded=(json.dumps(metadata,indent=2)+'\n').encode();path=ROOT/'content/shop_interiors.json'
    if args.check:assert path.read_bytes()==encoded,'Stale room manifest'
    else:path.write_bytes(encoded)


if __name__=='__main__':main()
