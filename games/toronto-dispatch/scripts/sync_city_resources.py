"""Synchronize authored CGB priority/palette attributes using native GB Studio RLE."""
from pathlib import Path
import json
from city_layout import WIDTH,HEIGHT,road,walkable
ROOT=Path(__file__).resolve().parents[1]
def compress(values):
    output=[];last=None;count=0
    for value in values+[None]:
        if value!=last:
            if count:output.append(f'{last:02x}'+('!' if count==1 else f'{count:x}+'))
            last=value;count=0
        count+=1
    return ''.join(output)
def main():
    project=ROOT/'project';path=project/'assets/backgrounds/toronto_city.png.gbsres'
    data=json.loads(path.read_text());attrs=json.loads((project/'original-art/city_attributes.json').read_text())
    assert len(attrs)==WIDTH//8*(HEIGHT//8)
    data.update(width=WIDTH//8,height=HEIGHT//8,imageWidth=WIDTH,imageHeight=HEIGHT,tileColors=compress(attrs))
    path.write_text(json.dumps(data,indent=2)+'\n')
    city=json.loads((ROOT/'content/city_art.json').read_text())
    collisions=[]
    for y in range(4,HEIGHT,8):
        for x in range(4,WIDTH,8):
            blocked=any(b['x']<=x<b['x']+b['width'] and b['y']<=y<b['y']+b['depth'] for b in city['blocks'])
            if 32<=x<=560 and 752<=y<760:blocked=True
            if 376<=x<408 and 664<=y<696:blocked=True
            collisions.append(15 if blocked or not walkable(x,y) else 0 if road(x,y) else 16)
    scene=project/'project/scenes/toronto_city/scene.gbsres';data=json.loads(scene.read_text());data.update(width=WIDTH//8,height=HEIGHT//8,collisions=compress(collisions));scene.write_text(json.dumps(data,indent=2)+'\n')
    print('Native CGB attributes synchronized; roof/canopy tiles carry background priority.')
if __name__=='__main__':main()
