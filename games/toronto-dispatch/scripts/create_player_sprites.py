#!/usr/bin/env python3
"""Author the original native courier/vehicle sheet without repainting the city.

Frame IDs 0..44 retain their gameplay meanings. Cars use two hardware objects;
walking courier poses use one centred 8x16 object, matching smaller civilians.
"""
from pathlib import Path
import argparse
import hashlib
import json
import uuid
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]
PROJECT=ROOT/'project'
COLOURS=['#071821','#306850','#86c06c','#e0f8cf']
TRANSPARENT='#65ff00'

def ident(name):
    return str(uuid.uuid5(uuid.NAMESPACE_URL,'toronto-dispatch/topdown/'+name))

def car_east():
    image=Image.new('RGB',(16,16),TRANSPARENT);d=ImageDraw.Draw(image)
    d.rectangle((3,3,11,13),fill=COLOURS[0])
    d.polygon(((1,5),(3,4),(12,4),(15,6),(15,10),(12,12),(3,12),(1,11)),fill=COLOURS[0])
    d.rectangle((2,5,13,11),fill=COLOURS[2])
    d.rectangle((5,4,10,12),fill=COLOURS[2])
    d.rectangle((4,5,5,11),fill=COLOURS[0])
    d.rectangle((10,5,11,11),fill=COLOURS[0])
    d.line((6,5,9,5),fill=COLOURS[3]);d.line((6,11,9,11),fill=COLOURS[3])
    d.line((14,6,14,10),fill=COLOURS[3])
    d.point((2,5),fill=COLOURS[3]);d.point((2,11),fill=COLOURS[3])
    # Tires stay exposed beside a narrower cabin; pale hood lights and a dark
    # windshield distinguish the front even at the native16px canvas.
    d.rectangle((3,3,5,4),fill=COLOURS[0]);d.rectangle((10,3,12,4),fill=COLOURS[0])
    d.rectangle((3,12,5,13),fill=COLOURS[0]);d.rectangle((10,12,12,13),fill=COLOURS[0])
    d.line((12,6,12,10),fill=COLOURS[2]);d.point((14,6),fill=COLOURS[3]);d.point((14,10),fill=COLOURS[3])
    d.point((8,4),fill=COLOURS[3]);d.point((8,12),fill=COLOURS[0])
    return image

def vehicle_east(kind):
    image=Image.new('RGB',(16,16),TRANSPARENT);d=ImageDraw.Draw(image)
    if kind==1:
        # Broad parcel box, cab glass, bumper and two visible axle pairs.
        d.rectangle((1,3,10,12),fill=COLOURS[0]);d.rectangle((2,4,9,11),fill=COLOURS[2])
        d.rectangle((10,5,14,10),fill=COLOURS[0]);d.rectangle((11,5,13,10),fill=COLOURS[3])
        d.line((12,6,12,9),fill=COLOURS[0]);d.line((5,4,5,11),fill=COLOURS[3])
        for x in (3,11):
            d.rectangle((x,2,x+2,3),fill=COLOURS[0]);d.rectangle((x,12,x+2,13),fill=COLOURS[0])
    else:
        # Two exposed tires, a saddle and human rider separate two-wheelers
        # from narrow painted rectangles. Scooter has a distinct step-through.
        d.rectangle((1,7,3,9),fill=COLOURS[0]);d.rectangle((12,7,14,9),fill=COLOURS[0])
        d.line((3,8,12,8),fill=COLOURS[2],width=3)
        d.rectangle((6,5,9,10),fill=COLOURS[0]);d.rectangle((7,5,9,7),fill=COLOURS[3])
        d.rectangle((5,7,8,9),fill=COLOURS[2]);d.line((10,5,10,11),fill=COLOURS[0])
        d.point((12,6),fill=COLOURS[3]);d.point((7,10),fill=COLOURS[0])
        if kind==3:
            d.line((10,6,10,10),fill=COLOURS[3]);d.rectangle((4,7,5,9),fill=COLOURS[3])
            d.point((12,6),fill=COLOURS[2])
    return image

def courier(direction,step):
    image=Image.new('RGB',(16,16),TRANSPARENT);d=ImageDraw.Draw(image)
    d.rectangle((6,3,9,5),fill=COLOURS[0])
    d.rectangle((6,4,9,5),fill=COLOURS[3])
    if direction==3:d.line((6,4,9,4),fill=COLOURS[0])
    elif direction==2:d.point((6,4),fill=COLOURS[0])
    else:d.point((9,4),fill=COLOURS[0])
    d.rectangle((6,6,9,9),fill=COLOURS[2])
    d.point((5,7+step),fill=COLOURS[3]);d.point((10,8-step),fill=COLOURS[3])
    d.line((6,10,6+step,12),fill=COLOURS[0])
    d.line((9,10,9-step,12),fill=COLOURS[0])
    return image

def artwork():
    sheet=Image.new('RGB',(256,48),TRANSPARENT);sd=ImageDraw.Draw(sheet)
    for frame in range(45):
        ox=(frame%16)*16;oy=(frame//16)*16;veh=frame//8;heading=frame%8
        if frame<8 or frame==44:
            pose=car_east().rotate(-45*(heading if frame<8 else 0),resample=Image.Resampling.NEAREST,fillcolor=TRANSPARENT)
            if frame==44:
                d=ImageDraw.Draw(pose);d.line((8,12,11,15),fill=COLOURS[2],width=2)
            sheet.paste(pose,(ox,oy))
        elif frame<32:
            sheet.paste(vehicle_east(veh).rotate(-45*heading,resample=Image.Resampling.NEAREST,fillcolor=TRANSPARENT),(ox,oy))
        elif frame<40:sheet.paste(courier((frame-32)//2,frame&1),(ox,oy))
        else:
            sd.polygon([(ox+8,oy+1),(ox+14,oy+7),(ox+8,oy+14),(ox+2,oy+7)],fill=COLOURS[3],outline=COLOURS[0]);sd.rectangle((ox+7,oy+4,ox+9,oy+9),fill=COLOURS[2])
    frames=[]
    for n in range(45):
        frames.append({'id':ident(f'frame-{n}'),'tiles':[{'id':ident(f'tile-{n}-{x}'),'x':x,'y':0,'sliceX':(n%16)*16+x,'sliceY':(n//16)*16,'flipX':False,'flipY':False,'palette':0,'paletteIndex':0,'objPalette':'OBP0','priority':False} for x in ((4,) if 32<=n<40 else (0,8))]})
    states=[]
    for name,subset in [('vehicles',frames[:32]),('courier',frames[32:])]:
        animations=[{'id':ident(name+'-animation'),'frames':subset}]+[{'id':ident(f'{name}-empty-{n}'),'frames':[{'id':ident(f'{name}-emptyframe-{n}'),'tiles':[]}]} for n in range(7)]
        states.append({'id':ident(name+'-state'),'name':'' if name=='vehicles' else 'Courier and beacon','animationType':'fixed','flipLeft':False,'animations':animations})
    meta={'_resourceType':'sprite','id':ident('vehicles'),'name':'Top-down vehicles and courier','symbol':'sprite_dispatch_topdown','states':states,'width':256,'height':48,'canvasOriginX':8,'canvasOriginY':8,'canvasWidth':16,'canvasHeight':16,'boundsX':2,'boundsY':2,'boundsWidth':12,'boundsHeight':12,'animSpeed':255,'numTiles':0,'filename':'dispatch_topdown.png'}
    return sheet,meta

def write():
    sheet,meta=artwork();png=PROJECT/'original-art/dispatch_topdown.png'
    sheet.save(png);meta['checksum']=hashlib.sha1(png.read_bytes()).hexdigest()
    (PROJECT/'dispatch_topdown.metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
    sheet.resize((1024,192),Image.Resampling.NEAREST).save(PROJECT/'original-art/dispatch_topdown_preview.png')

def normalized(value):
    if isinstance(value,dict):return {k:normalized(v) for k,v in value.items() if k not in ('id','name','symbol')}
    if isinstance(value,list):return [normalized(v) for v in value]
    return value

def check():
    sheet,meta=artwork();png=PROJECT/'original-art/dispatch_topdown.png'
    with Image.open(png) as image:assert image.convert('RGB').tobytes()==sheet.tobytes()
    assert hashlib.sha256(sheet.crop((0,32,128,48)).tobytes()).hexdigest()=='e0dd0b7e8406cabb36a510e8767dda3a8e057f56d6e4936f5035cdbfebe65569','Approved courier pixels changed'
    assert hashlib.sha256(sheet.crop((128,32,192,48)).tobytes()).hexdigest()=='92e251f5697a1670309048b4ff2bccd7008aaec63340e3017b64acd83ebacc3b','Approved objective beacon pixels changed'
    assert (PROJECT/'assets/sprites/dispatch_topdown.png').read_bytes()==png.read_bytes()
    meta['checksum']=hashlib.sha1(png.read_bytes()).hexdigest()
    assert json.loads((PROJECT/'dispatch_topdown.metadata.json').read_text())==meta
    native=json.loads((PROJECT/'assets/sprites/dispatch_topdown.png.gbsres').read_text())
    assert normalized(native)==normalized(meta)
    frames=meta['states'][1]['animations'][0]['frames']
    assert [len(f['tiles']) for f in frames[:8]]==[1]*8
    for frame in range(32,40):
        crop=sheet.crop(((frame%16)*16,32,(frame%16+1)*16,48))
        occupied=[(x,y) for y in range(16) for x in range(16) if crop.getpixel((x,y))!=(101,255,0)]
        assert max(x for x,y in occupied)-min(x for x,y in occupied)+1==6
        assert max(y for x,y in occupied)-min(y for x,y in occupied)+1==10
    assert {sheet.getpixel((x,y)) for x in range(256) for y in range(48)}<=set(((101,255,0),(7,24,33),(134,192,108),(224,248,207)))
    with Image.open(PROJECT/'original-art/dispatch_topdown_preview.png') as preview:
        assert preview.convert('RGB').tobytes()==sheet.resize((1024,192),Image.Resampling.NEAREST).tobytes()
    print('Courier/car source and native pose checks passed: 45 preserved frame meanings, 6x10 one-OBJ walker, original larger car pixels')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true')
    args=parser.parse_args();check() if args.check else write()
