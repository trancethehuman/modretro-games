"""Original larger red/green signal heads on verified non-priority curb cells.

The original road-phase/stop coordinates remain unchanged. Separate8px heads
show EW and NS directions; existing background palette3 coral and6 park green
supply colors independently of the new navy/mint/gold UI palette.
"""
import argparse,json,re
from pathlib import Path
from PIL import Image
from create_traffic_signals import model,decode
ROOT=Path(__file__).resolve().parents[1]
ENGINE=ROOT/'project/plugins/toronto-driving/engine'
SOURCE=ENGINE/'src/td_traffic_lights.c'
PNG=ROOT/'project/original-art/traffic_signals.png'
ROWS=(
 ("03333330","03111330","03111330","03333330","03333330","00033000","00033000","00033000"),
 ("03333330","03333330","03333330","03111330","03111330","00033000","00033000","00033000"),
)
SLUGS=['city','west','high_park','east','port_lands','islands','north']
ARTS=['city_art.json']+[f'districts/{s}_art.json' for s in ['west','high_park','east','port_lands','island','north']]

def patterns():
 return [v for pose in ROWS for row in pose for v in (sum((int(c)&1)<<(7-x) for x,c in enumerate(row)),sum(((int(c)>>1)&1)<<(7-x) for x,c in enumerate(row)))]

def graphics():
 output=[]
 for d,signals in enumerate(model()):
  scene=json.loads((ROOT/f'project/project/scenes/toronto_{SLUGS[d]}/scene.gbsres').read_text());grid=decode(scene['collisions'])
  bg=json.loads((ROOT/f'project/assets/backgrounds/toronto_{SLUGS[d]}.png.gbsres').read_text());attrs=decode(bg['tileColors'])
  art=json.loads((ROOT/'content'/ARTS[d]).read_text());props={(p['x']//8,p['y']//8) for p in art['scenery']['destructible_furniture']}
  occupied={(p[3],p[4]) for p in signals}
  for u,v,arms,x,y in signals:
   options=[(x+1,y),(x-1,y),(x,y+1),(x,y-1)]
   free=[p for p in options if 0<=p[0]<128 and 0<=p[1]<122 and not grid[p[1]*128+p[0]]&15 and not attrs[p[1]*128+p[0]]&128 and p not in occupied and p not in props]
   assert free,('No second safe signal head',d,u,v)
   a,b=free[0];occupied.add((a,b));output.append({'district':d,'junction':[u,v],'arms':arms,'ew_cell':[x,y],'ns_cell':[a,b]})
 return output

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
 output=patterns();heads=graphics();old=SOURCE.read_text();expected=re.sub(r'static const UBYTE td_light_patterns\[32\]=\{[^}]+\};','static const UBYTE td_light_patterns[32]={'+','.join(map(str,output))+'};',old)
 code='/* Original second signal heads; phases and junction positions unchanged. */\nstatic const UBYTE td_signal_extra[][2]={\n'+''.join('    {'+','.join(map(str,p['ns_cell']))+'},\n' for p in heads)+'};\n'
 image=Image.new('RGB',(16,8))
 for n,(pose,palette_file) in enumerate(zip(ROWS,['toronto_architecture_3.gbsres','toronto_islands_park.gbsres'])):
  colors=[tuple(bytes.fromhex(c)) for c in json.loads((ROOT/'project/project/palettes'/palette_file).read_text())['colors']]
  for y,row in enumerate(pose):
   for x,c in enumerate(row):image.putpixel((n*8+x,y),colors[int(c)])
 files={SOURCE:expected.encode(),ENGINE/'include/td_signal_graphics.h':code.encode(),ROOT/'content/traffic_signal_art.json':(json.dumps({'schema':1,'source':'create_signal_art.py','heads':heads,'reserved_tiles':[47,48],'red_palette':3,'green_palette':6,'phase_seconds':12,'ew_green_seconds':7,'new_oam_objects':0},indent=2)+'\n').encode()}
 if args.check:
  for p,data in files.items():assert p.read_bytes()==data,('Stale signal source',p)
  with Image.open(PNG) as actual:assert actual.size==image.size and actual.convert('RGB').tobytes()==image.tobytes(),'Signal source pixels stale'
 else:
  for p,data in files.items():p.write_bytes(data)
  image.save(PNG)
 print(f'{len(heads)} original dual8px signal heads:32 bitmap bytes,tiles47/48,palettes3/6; traffic geometry/phases unchanged')
if __name__=='__main__':main()
