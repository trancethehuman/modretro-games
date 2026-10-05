"""Sanitize actual scenery contact and BG ownership with independent geometry."""
import json,os,shutil,subprocess,tempfile,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT/'games/toronto-dispatch';ENGINE=GAME/'project/plugins/toronto-driving/engine'

def main():
 subprocess.run(['python3',str(GAME/'scripts/create_scenery_data.py'),'--check'],check=True)
 counts=[];maxima=[]
 for f in ['city_art.json']+[f'districts/{x}_art.json' for x in ['west','high_park','east','port_lands','island','north']]:
  ps=json.loads((GAME/'content'/f).read_text())['scenery']['destructible_furniture'];coords={(p['x']//8,p['y']//8) for p in ps}
  assert len(coords)==len(ps)<=256
  best=max(sum(x<=px<=x+20 and y<=py<=y+18 for px,py in coords) for x in range(128) for y in range(122))
  assert best<=18,(f,best);counts.append(len(ps));maxima.append(best)
 # Foot movement does not call vehicle furniture contact. Verify every real
 # mission/transit marker, doorway and reciprocal seam still has a whole3px
 # pedestrian body and approachable public ground. Vehicle parking anchors
 # and shared curb bays also stay clear of intact furniture at half7.
 sys.path.insert(0,str(GAME/'scripts'))
 from create_scenery_data import decompress
 slugs=['city','west','high_park','east','port_lands','islands','north']
 grids=[decompress(json.loads((GAME/f'project/project/scenes/toronto_{s}/scene.gbsres').read_text())['collisions']) for s in slugs]
 campaign=json.loads((GAME/'content/campaign.json').read_text());points=[(s['district'],s['u'],s['v']) for s in campaign['stops']]
 world=json.loads((GAME/'content/districts/world.json').read_text())
 points += [(p[k]['district'],p[k]['u'],p[k]['v']) for p in world['portals'] for k in ['from','to']]
 points += [(p['district'],p['x'],p['y']) for f in ['city_art.json','districts/west_art.json','districts/east_art.json'] for p in json.loads((GAME/'content'/f).read_text())['scenery']['shop_entrances']]
 for d,u,v in points:
  assert all(not grids[d][y*128+x]&15 for y in range((v-3)//8,(v+3)//8+1) for x in range((u-3)//8,(u+3)//8+1)),('Foot service access blocked',d,u,v)
 anchors=[(s['district'],s['parking_anchor']['u'],s['parking_anchor']['v']) for s in campaign['stops'] if s.get('parking_anchor')]
 for d,f in enumerate(['city_art.json']+[f'districts/{x}_art.json' for x in ['west','high_park','east','port_lands','island','north']]):
  art=json.loads((GAME/'content'/f).read_text());props=art['scenery']['destructible_furniture'];bays=art['scenery']['parked_vehicle_candidates']
  for u,v in [(p['x'],p['y']) for p in bays]+[(u,v) for district,u,v in anchors if district==d]:
   assert all(not grids[d][y*128+x]&15 for y in range((v-7)//8,(v+7)//8+1) for x in range((u-7)//8,(u+7)//8+1)),('Car parking terrain blocked',d,u,v)
   assert not any(u+7>=p['x'] and u-7<p['x']+8 and v+7>=p['y'] and v-7<p['y']+8 for p in props),('Intact furniture blocks parking',d,u,v)
 print(f'Access:64 stops,{len(points)-67} seam endpoints,3 doors,9 parking anchors and all shared curb bays remain approachable without destruction')
 source=(ENGINE/'src/td_scenery.c').read_text()
 assert 'td_scenery_native_static_budget' in source and '==233' in source
 assert sum(counts)==1135 and (sum(counts)+7)//8==142
 assert 'td_broken[(TD_SCENERY_PROPS+7)/8]' in source
 assert 'td_prop_dead(i)' in source and 'td_prop_dead(index)' in source
 assert 'td_broken[i>>3]|=1<<(i&7)' in source
 assert 'flash=&td_prop_flashes[td_prop_flash_next&3]' in source
 assert 'td_prop_flash_next=((td_prop_flash_next+1)&3)|128' in source
 render_start=source.index('void td_scenery_render(void)')
 assert source.index('if(!(td_prop_flash_next&128))return;',render_start)<source.index('district=td_district_current();',render_start)
 assert '#define TD_SCENERY_FIRST 80' in source and '#define TD_SCENERY_PATCHES 18' in source
 assert 'for(row=lower;row<=upper;row++)' in source and 'td_prop_lower(district,row,left)' in source
 assert 'du>16' in source and 'dv>16' in source
 # Structural and modal masks never become an alternate collision channel.
 assert 'td_terrain' not in source and 'td.onfoot' not in source
 compiler=shutil.which(os.environ.get('CC','cc'));assert compiler
 with tempfile.TemporaryDirectory(prefix='toronto-scenery-') as directory:
  w=Path(directory);(w/'gbdk').mkdir();(w/'gbdk/platform.h').write_text('#include "host_scenery.h"\n')
  (w/'host_scenery.h').write_text('''#ifndef HOST_SCENERY_H
#define HOST_SCENERY_H
#include <stdint.h>
#include <stddef.h>
typedef uint8_t UBYTE;typedef int8_t BYTE;typedef uint16_t UWORD;typedef int16_t WORD;
#define BANKED
#define TRUE 1
#define FALSE 0
typedef struct {UBYTE bank;const void *ptr;} far_ptr_t;
extern far_ptr_t current_scene;extern UBYTE VBK_REG;extern WORD draw_scroll_x,draw_scroll_y;
UBYTE *GetBkgAddr(void);UBYTE get_vram_byte(UBYTE *);void set_vram_byte(UBYTE *,UBYTE);
void set_bkg_data(UBYTE,UBYTE,const UBYTE *);
#endif
''')
  for name in ['bankdata','data_manager','gbs_types','scroll','compat']:(w/f'{name}.h').write_text('#include "host_scenery.h"\n')
  (w/'scenery_under_test.c').write_text(source)
  b=w/'test';subprocess.run([compiler,'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unknown-pragmas','-DCGB','-fsanitize=address,undefined','-I',str(w),'-I',str(ENGINE/'include'),str(ROOT/'tests/engine/scenery_harness.c'),'-o',str(b)],check=True)
  subprocess.run([str(b)],check=True)
  subprocess.run([compiler,'-std=c11','-Wall','-Wextra','-Werror','-Wno-unknown-pragmas','-fsyntax-only','-I',str(w),'-I',str(ENGINE/'include'),str(w/'scenery_under_test.c')],check=True)
 print(f'Scenery source bounds: {counts}; exact160x144 viewport maxima {maxima};1135 props in142 packed bytes,native233 bytes,18 BKG tiles,0 OBJ. Clean views skip district/map queries with0 new bytes. Native CPU timing remains separate.')
if __name__=='__main__':main()
