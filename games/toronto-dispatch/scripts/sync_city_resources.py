"""Synchronize authored CGB priority/palette attributes using native GB Studio RLE."""
from pathlib import Path
import json
from city_layout import WIDTH,HEIGHT
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
    collisions=city['collisions']
    assert len(collisions)==WIDTH//8*(HEIGHT//8)
    scene=project/'project/scenes/toronto_city/scene.gbsres';data=json.loads(scene.read_text());data.update(width=WIDTH//8,height=HEIGHT//8,collisions=compress(collisions));scene.write_text(json.dumps(data,indent=2)+'\n')
    # District backgrounds: palette/priority bytes come from each generator's
    # attributes file; their collisions are authored by the generators.
    for slug in ('west','high_park','east'):
        path=project/f'assets/backgrounds/toronto_{slug}.png.gbsres'
        data=json.loads(path.read_text());attrs=json.loads((project/f'original-art/{slug}_attributes.json').read_text())
        assert len(attrs)==data['width']*data['height']
        data['tileColors']=compress(attrs);path.write_text(json.dumps(data,indent=2)+'\n')
    print('Native CGB attributes synchronized; roof/canopy tiles carry background priority.')
if __name__=='__main__':main()
