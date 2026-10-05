"""Narrow immutable R6->empty-scooter extension proof.

All original49 frame IDs, pixels, defaults and empty animation metadata are
checked before reconstructing R6 for the existing historical courier guard.
The two appended cells are exact approved original art, not an omission scope.
"""
from pathlib import Path
import base64
import copy
import hashlib
import io
import json
import zlib
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
FIXTURE=ROOT.parents[1]/'tests/fixtures/scooter_extension.json'
FIXTURE_SHA256='56e0ff9d99594703ed2a30f4cdba147814586b6474266ada9cd3308fdf904133'
META_FILES={'project/dispatch_topdown.metadata.json','project/assets/sprites/dispatch_topdown.png.gbsres'}
PNG_FILES={'project/original-art/dispatch_topdown.png','project/assets/sprites/dispatch_topdown.png','project/original-art/dispatch_topdown_preview.png'}
PARKED_FRAMES=(49,50)
TRANSPARENT=(101,255,0)

def digest(value):
    return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()

def scope():
    payload=FIXTURE.read_bytes()
    assert hashlib.sha256(payload).hexdigest()==FIXTURE_SHA256,'Immutable R6 scooter proof changed'
    proof=json.loads(payload)
    assert proof['schema']==1 and proof['predecessor_commit']=='fb589599f6411d517fe8e2af9d8c43dbf6d89144'
    assert set(proof['metadata'])==META_FILES and set(proof['pngs'])==PNG_FILES
    return proof

def frames(meta):
    return [f for state in meta['states'] for f in state['animations'][0]['frames']]

def meta_history(relative,meta):
    proof=scope();assert relative in META_FILES,'Unexpected scooter metadata scope'
    pin=proof['metadata'][relative];meta=copy.deepcopy(meta);poses=frames(meta)
    assert (meta['width'],meta['height'],meta['numTiles'],len(poses))==(256,64,0,51)
    assert len(meta['states'])==2 and len(meta['states'][0]['animations'][0]['frames'])==32
    assert digest(poses[:49])==pin['old_frames_sha256'],'An original R6 frame ID/cell changed'
    assert poses[49:]==pin['empty_frames'],'An approved empty scooter frame changed'
    assert meta['checksum']==pin['new_checksum'],'Metadata checksum differs from approved scooter PNG'
    for n,pose in enumerate(poses[49:],49):
        assert len(pose['tiles'])==1,'An empty scooter must use one OBJ'
        tile=pose['tiles'][0]
        assert (tile['x'],tile['y'],tile['sliceX'],tile['sliceY'])==(4,0,(n%16)*16+4,(n//16)*16)
        assert (tile['palette'],tile['paletteIndex'],tile['objPalette'])==(0,0,'OBP0')
        assert not tile['flipX'] and not tile['flipY'] and not tile['priority']
    meta['states'][1]['animations'][0]['frames']=meta['states'][1]['animations'][0]['frames'][:17]
    meta['checksum']=pin['old_checksum']
    assert digest(meta)==pin['old_canonical_sha256'],'Original R6 defaults/state/metadata changed'
    return meta

def pixels_history(image):
    proof=scope();image=image.convert('RGB')
    assert image.size==(256,64),'Scooter art changed original sheet dimensions'
    original=[];empty=[]
    for n in range(64):
        rect=((n%16)*16,(n//16)*16,(n%16+1)*16,(n//16+1)*16);crop=image.crop(rect)
        if n<49:original.append(crop.tobytes())
        elif n<51:
            empty.append(crop.tobytes())
            assert all(4<=x<=11 for y in range(16) for x in range(16) if crop.getpixel((x,y))!=TRANSPARENT),'Scooter pixels exceed one OBJ'
        else:assert crop.getcolors()==[(256,TRANSPARENT)],'Scooter paint escaped the two appended cells'
    assert hashlib.sha256(b''.join(original)).hexdigest()==proof['old49_rgb_sha256'],'Original49 R6 pose pixels changed'
    assert hashlib.sha256(b''.join(empty)).hexdigest()==proof['empty_rgb_sha256'],'Approved two vacant scooter pixel poses changed'
    previous=image.copy()
    for n in PARKED_FRAMES:previous.paste(TRANSPARENT,((n%16)*16,(n//16)*16,(n%16+1)*16,(n//16+1)*16))
    assert hashlib.sha256(previous.tobytes()).hexdigest()==proof['old_rgb_sha256'],'Original R6 unused canvas/default pixels changed'
    return previous

def png_history(relative,payload):
    proof=scope();assert relative in PNG_FILES,'Unexpected scooter PNG scope'
    pin=proof['pngs'][relative]
    assert hashlib.sha256(payload).hexdigest()==pin['new_sha256'],'Approved scooter PNG bytes changed'
    with Image.open(io.BytesIO(payload)) as image:
        assert image.mode=='RGB' and image.format=='PNG'
        if relative.endswith('_preview.png'):
            assert image.size==(1024,256)
            reduced=image.resize((256,64),Image.Resampling.NEAREST)
            assert image.tobytes()==reduced.resize((1024,256),Image.Resampling.NEAREST).tobytes()
            pixels_history(reduced)
        else:pixels_history(image)
    previous=zlib.decompress(base64.b64decode(pin['old_png_zlib']))
    assert hashlib.sha256(previous).hexdigest()==pin['old_sha256'],'Corrupt immutable R6 asset bytes'
    return previous

def check():
    proof=scope();native=(ROOT/'project/assets/sprites/dispatch_topdown.png').read_bytes()
    assert native==(ROOT/'project/original-art/dispatch_topdown.png').read_bytes()
    for relative in META_FILES:
        meta=json.loads((ROOT/relative).read_text())
        assert meta['checksum']==hashlib.sha1(native).hexdigest()
        meta_history(relative,meta)
    for relative in PNG_FILES:png_history(relative,(ROOT/relative).read_bytes())
    # The installed pose helper can only select49/50; confirm every heading.
    source=(ROOT/'project/plugins/toronto-driving/engine/src/td_city_sprites.c').read_text()
    assert 'td_city_pose(actor,&td_city_player,49+((((heading+1)&15)/4)&1));' in source
    assert proof['old49_rgb_sha256']=='30e4f1b4a629bbe4088d8e3cf9d37352226fa9fd11ba1b81e7164ba94e5234fb'
    return proof

if __name__=='__main__':
    check();print('Exact scooter art extension passed:49 R6 poses preserved, two original one-OBJ empty poses appended')
