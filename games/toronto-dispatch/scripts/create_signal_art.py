"""Exact original two-head signal pixels for existing bank-1 tiles 47 and 48.

The left head governs east/west; the right head governs north/south. The light
palette remains existing UI slot7: pale ground, green, red, dark. Only these
32 bitmap bytes change; phase timing, junction data and VRAM allocation do not.
"""
import argparse
import json
from pathlib import Path
import re
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'project/plugins/toronto-driving/engine/src/td_traffic_lights.c'
PNG=ROOT/'project/original-art/traffic_signals.png'
ROWS=(
    ("33333333","33333223","33333333","31133333","33333333","03333330","00033000","00033000"),
    ("33333333","32233333","33333333","33333113","33333333","03333330","00033000","00033000"),
)


def patterns():
    output=[]
    for pose in ROWS:
        for row in pose:
            output.extend((sum((int(c)&1)<<(7-x) for x,c in enumerate(row)),
                           sum(((int(c)>>1)&1)<<(7-x) for x,c in enumerate(row))))
    assert len(output)==32
    return output


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    output=patterns()
    old=SOURCE.read_text()
    expected=re.sub(r'static const UBYTE td_light_patterns\[32\]=\{[^}]+\};',
                    'static const UBYTE td_light_patterns[32]={'+','.join(map(str,output))+'};',old)
    assert expected!=old or list(map(int,re.search(r'td_light_patterns\[32\]=\{([^}]+)\}',old)[1].split(',')))==output
    palette=json.loads((ROOT/'project/project/palettes/default_ui.gbsres').read_text())['colors']
    colors=[tuple(bytes.fromhex(c)) for c in palette]
    image=Image.new('RGB',(16,8))
    for n,pose in enumerate(ROWS):
        for y,row in enumerate(pose):
            for x,c in enumerate(row):image.putpixel((n*8+x,y),colors[int(c)])
    if args.check:
        assert SOURCE.read_text()==expected,'Runtime signal pixels are stale'
        with Image.open(PNG) as actual:assert actual.size==image.size and actual.convert('RGB').tobytes()==image.tobytes(),'Signal source PNG is stale'
    else:
        SOURCE.write_text(expected);image.save(PNG)
    print('Two original signal-head/pole poses match 32 bytes; existing tiles47/48 and palette7 unchanged')


if __name__=='__main__':main()
