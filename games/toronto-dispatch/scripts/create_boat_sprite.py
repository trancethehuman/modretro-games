#!/usr/bin/env python3
"""Original tiny boat and independent OBJ scratch pair; no native registration.

One8x16 hull object, one nonblank reserve frame, one empty startup. The renderer
copies pristine compiled hull pixels into the dedicated scratch pair. --check
and --dry-run do not write assets; compilation/native playback stay separate.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import uuid
from PIL import Image, ImageDraw
from create_atlas import decode_grid

ROOT=Path(__file__).resolve().parents[1]
ART=ROOT/'project/original-art'
TRANSPARENT=(101,255,0)
COLOURS=(TRANSPARENT,(224,248,207),(134,192,108),(7,24,33))
def ident(key):return str(uuid.uuid5(uuid.NAMESPACE_URL,'toronto-dispatch.boats.'+key))

def same_png(left,right):
    # PNG compression varies across Pillow/zlib builds. Preserve native colour
    # indices and transparency instead of normalizing either image to RGB.
    with Image.open(io.BytesIO(left)) as a,Image.open(io.BytesIO(right)) as b:
        return a.format==b.format=='PNG' and a.mode==b.mode and a.size==b.size and \
            a.tobytes()==b.tobytes() and a.getpalette()==b.getpalette() and \
            (a.palette.mode if a.palette else None)==(b.palette.mode if b.palette else None) and \
            a.info.get('transparency')==b.info.get('transparency')

def model(png_checksum=None):
    image=Image.new('RGB',(32,16),TRANSPARENT);draw=ImageDraw.Draw(image)
    # Original civilian launch: pointed bow, bright gunwale, compact cabin,
    # dark stern. Southbound reverses this source in the renderer.
    draw.polygon([(3,0),(5,2),(6,6),(6,13),(5,14),(2,14),(1,13),(1,6),(2,2)],fill=COLOURS[3])
    draw.polygon([(3,2),(4,3),(5,6),(5,12),(2,12),(2,6)],fill=COLOURS[1])
    draw.rectangle((2,6,5,9),fill=COLOURS[2]);draw.line((2,7,5,7),fill=COLOURS[3])
    draw.point((3,13),fill=COLOURS[2])
    # Distinct real pixels reserve one independent pair through the optimizer.
    # This frame is never displayed; the startup frame is completely empty.
    for y in range(16):
        for x in range(8):image.putpixel((8+x,y),COLOURS[1+((x*3+y+y//4)%3)])
    frames=[]
    for f in range(2):
        frames.append({'id':ident(f'frame-{f}'),'tiles':[{'id':ident(f'tile-{f}'),'x':0,'y':0,
            'sliceX':f*8,'sliceY':0,'flipX':False,'flipY':False,'palette':0,'paletteIndex':0,
            'objPalette':'OBP0','priority':False}]})
    frames.append({'id':ident('empty-frame'),'tiles':[]})
    animations=[{'id':ident('poses'),'frames':frames}]
    animations += [{'id':ident(f'empty-animation-{i}'),'frames':[{'id':ident(f'empty-{i}'),'tiles':[]}]} for i in range(1,8)]
    png=io.BytesIO();image.save(png,format='PNG');blob=png.getvalue()
    meta={'_resourceType':'sprite','id':ident('editable-source-sprite'),'name':'Ambient boat',
        'symbol':'sprite_ambient_boat','filename':'ambient_boat.png','width':32,'height':16,
        'checksum':hashlib.sha1(blob).hexdigest() if png_checksum is None else png_checksum,
        'numTiles':4,'canvasOriginX':8,'canvasOriginY':8,
        'canvasWidth':16,'canvasHeight':16,'boundsX':0,'boundsY':0,'boundsWidth':8,'boundsHeight':16,
        'animSpeed':255,'states':[{'id':ident('state'),'name':'','animationType':'fixed',
        'flipLeft':False,'animations':animations}]}
    manifest={'status':'Original source; compiled scratch isolation, scene allocation and native play remain separate gates',
        'native_frame_order':['north_hull','independent_scratch','empty_startup'],
        'source_colours':[list(c) for c in COLOURS],'hull_objects':1,'scratch_tile_pairs':1,
        'compiled_reserved_tiles_per_bank_limit':6,'sprite_palette':7,'loader_frame':2,
        'persistent_native_state_target_bytes':13,'maximum_persistent_state_bytes':20,
        'fictional_routes':[{'district':0,'u':560,'top':840,'bottom':896},
                             {'district':4,'u':464,'top':64,'bottom':432}],
        'port_deck_clip_rectangles':[[416,96,96,64],[416,256,96,64]],
        'player_control':False,'passenger_service':False,'save_fields':False,
        'water_full_hull_checks':validate_routes()}
    assert {image.getpixel((x,y)) for y in range(image.height) for x in range(image.width)}<=set(COLOURS)
    assert image.crop((0,0,8,16)).tobytes()!=image.crop((8,0,16,16)).tobytes()
    exact={image.crop((x,y,x+8,y+8)).tobytes() for x in (0,8) for y in (0,8)}
    assert len(exact)<=6
    manifest['source_raw_8x8_patterns']=len(exact)
    return {'ambient_boat.png':blob,'ambient_boat.metadata.json':(json.dumps(meta,indent=2)+'\n').encode(),
            'ambient_boat_art.json':(json.dumps(manifest,indent=2)+'\n').encode()}

def validate_routes():
    core=json.loads((ROOT/'content/city_art.json').read_text())
    port=json.loads((ROOT/'content/districts/port_lands_art.json').read_text())
    count=0
    for district,u,top,bottom,name in ((0,560,840,896,'toronto_city'),(4,464,64,432,'toronto_port_lands')):
        scene=json.loads((ROOT/f'project/project/scenes/{name}/scene.gbsres').read_text())
        grid=decode_grid(scene['collisions'],128*122)
        for v in range(top,bottom+1):
            for y in range(v-8,v+8):
                for x in range(u-4,u+4):
                    if district==0:
                        water=y>=core['mainland'][3] and not any(a<=x<c and b<=y<d for a,b,c,d in core['islands'])
                        deck=False
                    else:
                        water=any(a<=x<a+w and b<=y<b+h for a,b,w,h in port['water'])
                        deck=416<=x<512 and (96<=y<160 or 256<=y<320)
                    assert 0<=x<1024 and 0<=y<976 and water,'Hull leaves actual authored water'
                    assert grid[(y//8)*128+x//8]==15 or deck,'Hull crosses unsupported ground'
                    count+=1
    return count

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true');parser.add_argument('--dry-run',action='store_true')
    args=parser.parse_args()
    # Exact metadata still identifies the committed source file, independent
    # of how this platform compresses the equivalent regenerated PNG.
    checksum=hashlib.sha1((ART/'ambient_boat.png').read_bytes()).hexdigest() if args.check else None
    files=model(checksum)
    if args.check:
        for name,value in files.items():
            existing=(ART/name).read_bytes()
            assert same_png(existing,value) if name.endswith('.png') else existing==value, \
                f'Stale boat source: {name}'
    elif not args.dry_run:
        for name,value in files.items():(ART/name).write_bytes(value)
    print('Original boat: one8x16 object,4 raw tiles,3 frames;54,528 full-hull water/deck samples; native unverified.')
if __name__=='__main__':main()
