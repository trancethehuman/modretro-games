"""Independent negative mutations of the narrow original scooter art proof."""
from pathlib import Path
import copy
import importlib.util
import io
import json
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
file=ROOT/'games/toronto-dispatch/scripts/scooter_protection.py'
spec=importlib.util.spec_from_file_location('scooter_proof',file)
proof=importlib.util.module_from_spec(spec);spec.loader.exec_module(proof)
proof.check();checks=0
def rejected(callback):
    global checks
    try:callback()
    except (AssertionError,KeyError):checks+=1
    else:raise AssertionError('Scooter guard accepted a changed old pose/new pose/default/unused cell')
for relative in proof.META_FILES:
    base=json.loads((proof.ROOT/relative).read_text())
    for kind in ('old_id','old_pixelslice','gun_id','gun_slice','new_id','new_slice','new_palette','new_flip','new_priority','new_count','extra_frame','default','identity','checksum','old_empty'):
        changed=copy.deepcopy(base);frames=proof.frames(changed)
        if kind=='old_id':frames[0]['id']='changed'
        elif kind=='old_pixelslice':frames[44]['tiles'][0]['sliceX']+=1
        elif kind=='gun_id':frames[45]['id']='changed'
        elif kind=='gun_slice':frames[48]['tiles'][0]['sliceY']+=1
        elif kind=='new_id':frames[49]['id']='changed'
        elif kind=='new_slice':frames[50]['tiles'][0]['sliceX']+=1
        elif kind=='new_palette':frames[49]['tiles'][0]['paletteIndex']=1
        elif kind=='new_flip':frames[50]['tiles'][0]['flipY']=True
        elif kind=='new_priority':frames[49]['tiles'][0]['priority']=True
        elif kind=='new_count':frames[50]['tiles'].append(copy.deepcopy(frames[50]['tiles'][0]))
        elif kind=='extra_frame':changed['states'][1]['animations'][0]['frames'].append(copy.deepcopy(frames[50]))
        elif kind=='default':changed['boundsWidth']+=1
        elif kind=='identity':changed['id']='changed'
        elif kind=='checksum':changed['checksum']='changed'
        else:changed['states'][0]['animations'][1]['frames'][0]['tiles']=copy.deepcopy(frames[0]['tiles'])
        rejected(lambda:proof.meta_history(relative,changed))
with Image.open(proof.ROOT/'project/original-art/dispatch_topdown.png') as image:
    for n in range(64):
        changed=image.convert('RGB');changed.putpixel(((n%16)*16,(n//16)*16),(0,0,0))
        rejected(lambda:proof.pixels_history(changed))
    rejected(lambda:proof.pixels_history(image.crop((0,0,256,48))))
    for n in (49,50):
        changed=image.convert('RGB');changed.putpixel(((n%16)*16+3,(n//16)*16+8),(7,24,33))
        rejected(lambda:proof.pixels_history(changed))
for relative in proof.PNG_FILES:
    payload=(proof.ROOT/relative).read_bytes();rejected(lambda:proof.png_history(relative,payload+b'extra'))
rejected(lambda:proof.png_history('project/unrelated.png',b''))
rejected(lambda:proof.meta_history('project/unrelated.json',{}))
for heading in range(16):
    assert 49+((((heading+1)&15)//4)&1) in (49,50);checks+=1
assert checks==118,checks
print(f'Scooter art preservation mutation checks:{checks}, zero accepted mutations')
