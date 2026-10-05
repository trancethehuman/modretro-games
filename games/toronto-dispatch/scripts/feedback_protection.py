"""Narrow accepted visual/name changes layered over immutable traffic protection.

Historical fixture hashes remain authoritative. Current visual values are
separately pinned, then replaced with retained historical values for the old
geometry/client/job digest. This is not a blanket omit for scenery or missions.
"""
import base64,copy,hashlib,json,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];REPO=ROOT.parents[1]
PATH=REPO/'tests/fixtures/city_feedback_protected.json'
AIRCRAFT_FILES={
 'project/assets/sprites/ambient_aircraft.png',
 'project/assets/sprites/ambient_aircraft.png.gbsres',
 'project/original-art/ambient_aircraft.metadata.json',
 'project/original-art/ambient_aircraft.png',
 'project/original-art/ambient_aircraft_art.json',
 'project/original-art/ambient_aircraft_preview.png',
}
AIRCRAFT_META=('project/assets/sprites/ambient_aircraft.png.gbsres',
 'project/original-art/ambient_aircraft.metadata.json')
AIRCRAFT_ART='project/original-art/ambient_aircraft_art.json'
AIRCRAFT_CHANGED_ART_FIELDS=('png_sha256','metadata_sha256','decoded_rgb_sha256',
 'unique_patterns_8x8','unique_patterns_8x16','runtime')
AIRCRAFT_ADDED_ART_FIELDS=('unique_flipped_patterns_8x16','jet_additional_flipped_patterns_8x16',
 'jet_shadow_frames','original_decoded_rgb_sha256')

def canonical(value):return json.dumps(value,sort_keys=True,separators=(',',':')).encode()
def digest(value):return hashlib.sha256(canonical(value)).hexdigest()
def fixture():return json.loads(PATH.read_text())
def get_path(value,path):
 for key in path:value=value[key]
 return value

def historical_json(relative,value,proof=None):
 proof=fixture() if proof is None else proof;value=copy.deepcopy(value)
 for patch in proof['json_scopes'].get(relative,[]):
  current=get_path(value,patch['path']);assert digest(current)==patch['after_sha256'],('Unapproved feedback field',relative,patch['path'])
  old=json.loads(zlib.decompress(base64.b64decode(patch['before_json_zlib'])))
  assert digest(old)==patch['before_sha256'],'Corrupt retained historical field'
  parent=get_path(value,patch['path'][:-1]);parent[patch['path'][-1]]=old
 return value

def raw_sha(relative,payload,old_expected,proof=None):
 proof=fixture() if proof is None else proof;scope=proof['raw_scopes'].get(relative)
 if scope is None:return hashlib.sha256(payload).hexdigest()==old_expected
 assert scope['before_sha256']==old_expected,('Predecessor pin changed',relative)
 return hashlib.sha256(payload).hexdigest()==scope['after_sha256']

def aircraft_meta_history(relative,meta,scope,traffic):
 meta=copy.deepcopy(meta);pin=scope['metadata'][relative]
 frames=meta['states'][0]['animations'][0]['frames']
 assert (meta['width'],meta['height'],meta['numTiles'],len(frames))==(576,32,62,18),'Aircraft append dimensions/count changed'
 assert not frames[13]['tiles'],'Original hidden frame13 is no longer empty'
 assert digest(frames[:14])==pin['original_frames_sha256'],'Original aircraft frames0..13 changed'
 assert digest(frames[14:])==pin['jet_frames_sha256'],'Approved four jet frames changed'
 for n,frame in enumerate(frames[14:]):
  tiles=frame['tiles'];ys=(16,16,0,0) if n%2==0 else (0,0,16,16)
  assert len(tiles)==4 and [(t['x'],t['y']) for t in tiles]==list(zip((-8,0,8,16),ys)),'Jet cardinal four-cell geometry changed'
  assert all(t['sliceX']==448+n*32+i*8 and t['sliceY']==16-t['y'] and
   t['paletteIndex']==0 and not t['priority'] and not t['flipX'] and not t['flipY'] for i,t in enumerate(tiles)),'Jet source cells/palette/flips changed'
 meta['width']=416;meta['numTiles']=50;meta['checksum']=pin['original_checksum']
 meta['states'][0]['animations'][0]['frames']=frames[:14]
 assert hashlib.sha256((json.dumps(meta,indent=2)+'\n').encode()).hexdigest()==traffic['protected_files'][relative],('Aircraft identity/defaults/old poses changed',relative)
 return meta

def aircraft_pixels_check(image,scope):
 image=image.convert('RGB')
 assert image.size==(576,32),'Aircraft sheet changed beyond the appended jet strip'
 assert hashlib.sha256(image.crop((0,0,416,32)).tobytes()).hexdigest()==scope['original_rgb_sha256'],'Original plane/helicopter/ellipse pixels changed'
 assert image.crop((416,0,448,32)).getcolors()==[(32*32,(101,255,0))],'Hidden frame13 source gap must remain transparent'
 assert hashlib.sha256(image.crop((448,0,576,32)).tobytes()).hexdigest()==scope['jet_rgb_sha256'],'Approved jet shadow pixels changed'

def aircraft_art_history(meta,scope,traffic):
 meta=copy.deepcopy(meta)
 before=json.loads(zlib.decompress(base64.b64decode(scope['original_art_json_zlib'])))
 assert hashlib.sha256((json.dumps(before,indent=2)+'\n').encode()).hexdigest()==traffic['protected_files'][AIRCRAFT_ART],'Corrupt historical aircraft-art document'
 assert len(meta['poses'])==18 and meta['poses'][:13]==before['poses'],'Original authored aircraft poses changed'
 hidden={'frame':13,'kind':'hidden','direction':None,'rotor_phase':None,'source_rect':[416,0,32,32],
  'opaque_bounds':[],'oam_objects':0,'maximum_objects_per_scanline':0}
 assert meta['poses'][13]==hidden and meta['hidden_frame']==13 and meta['shadow_frame']==12,'Original empty/default/ellipse registration changed'
 for n,direction in enumerate(('east','south','west','north')):
  assert meta['poses'][14+n]=={'frame':14+n,'kind':'jet_shadow','direction':direction,'rotor_phase':None,
   'source_rect':[448+n*32,0,32,32],'opaque_bounds':[0,0,32,32],'oam_objects':4,'maximum_objects_per_scanline':2},'Jet source pose scope changed'
 assert [meta[k] for k in AIRCRAFT_ADDED_ART_FIELDS]==[31,6,[14,15,16,17],scope['original_rgb_sha256']],'Jet extension statistics/frame registration changed'
 for key in AIRCRAFT_ADDED_ART_FIELDS:meta.pop(key)
 for key in AIRCRAFT_CHANGED_ART_FIELDS:meta[key]=before[key]
 meta['poses']=meta['poses'][:13]
 assert meta==before,'Aircraft-art document changed outside four appended jet descriptions/hashes/statistics'
 return meta

def aircraft_extension_check(traffic,proof):
 from PIL import Image
 scope=proof['aircraft_extension']
 assert scope['original_frame_indices']==list(range(14)) and scope['jet_frame_indices']==[14,15,16,17]
 assert set(scope['metadata'])==set(AIRCRAFT_META)
 for relative in AIRCRAFT_META:aircraft_meta_history(relative,json.loads((ROOT/relative).read_text()),scope,traffic)
 for relative in ('project/assets/sprites/ambient_aircraft.png','project/original-art/ambient_aircraft.png'):
  with Image.open(ROOT/relative) as image:aircraft_pixels_check(image,scope)
 aircraft_art_history(json.loads((ROOT/AIRCRAFT_ART).read_text()),scope,traffic)

def aircraft_extension_negatives(traffic,proof):
 from PIL import Image
 scope=proof['aircraft_extension'];checks=0
 def rejected(callback):
  nonlocal checks
  try:callback()
  except AssertionError:checks+=1
  else:raise AssertionError('Aircraft extension accepted a protected-prefix/default/jet mutation')
 for relative in AIRCRAFT_META:
  base=json.loads((ROOT/relative).read_text())
  for kind in ('identity','old_cell','empty13','extra_frame','jet_palette','jet_slice'):
   changed=copy.deepcopy(base);frames=changed['states'][0]['animations'][0]['frames']
   if kind=='identity':changed['id']='changed'
   elif kind=='old_cell':frames[0]['tiles'][0]['x']+=1
   elif kind=='empty13':frames[13]['tiles']=copy.deepcopy(frames[12]['tiles'])
   elif kind=='extra_frame':frames.append(copy.deepcopy(frames[17]))
   elif kind=='jet_palette':frames[14]['tiles'][0]['paletteIndex']=1
   elif kind=='jet_slice':frames[14]['tiles'][0]['sliceX']+=1
   rejected(lambda:aircraft_meta_history(relative,changed,scope,traffic))
 base=json.loads((ROOT/AIRCRAFT_ART).read_text())
 for kind in ('old_pose','hidden','added_scope','unrelated'):
  changed=copy.deepcopy(base)
  if kind=='old_pose':changed['poses'][0]['direction']='changed'
  elif kind=='hidden':changed['hidden_frame']=17
  elif kind=='added_scope':changed['poses'][14]['oam_objects']=5
  else:changed['licence']='changed'
  rejected(lambda:aircraft_art_history(changed,scope,traffic))
 with Image.open(ROOT/'project/assets/sprites/ambient_aircraft.png') as image:
  for coordinate in ((0,0),(415,31),(416,0),(575,31)):
   changed=image.convert('RGB');changed.putpixel(coordinate,(0,0,0))
   rejected(lambda:aircraft_pixels_check(changed,scope))
 assert checks==20,'Aircraft extension mutation coverage changed'

def semantic_check(traffic,proof=None):
 from PIL import Image
 proof=fixture() if proof is None else proof
 assert proof['predecessor_commit']=='8ea5ec413c3650cd3441ee588ba15f87a98889fa'
 assert proof['historical_fixture_sha256']==hashlib.sha256((REPO/'tests/fixtures/traffic_lanes.json').read_bytes()).hexdigest()
 aircraft_extension_check(traffic,proof);aircraft_extension_negatives(traffic,proof)
 # Reconstruct only the accepted label/provenance correction. Every cash,
 # reward, deadline, job ordinal, route, stop coordinate and save ID stays old.
 campaign=json.loads((ROOT/'content/campaign.json').read_text());stop=campaign['stops'][18]
 assert stop['name']=='BLOORCOURT BUS' and (stop['u'],stop['v'],stop['district'],stop['transit'],stop['reserved'])==(144,64,0,2,0)
 assert stop['geography_source']=='https://www.toronto.ca/business-economy/business-operation-growth/business-improvement-areas/bia-list/bia-list-a-e/' and stop['geography_reviewed']=='2026-10-04'
 assert stop['location_notice']==proof['stop18_location_notice']
 stop['name']='OSSINGTON BUS'
 for key in ('geography_source','geography_reviewed','location_notice'):stop.pop(key)
 assert campaign['quests'][76]['stages'][2]['objective']=='Bloorcourt relay handoff'
 campaign['quests'][76]['stages'][2]['objective']='Ossington relay handoff'
 assert hashlib.sha256((json.dumps(campaign,indent=2)+'\n').encode()).hexdigest()==traffic['protected_files']['content/campaign.json'],'Historical campaign changed beyond accepted label'
 native=(ROOT/'project/plugins/toronto-driving/engine/src/td_content.c').read_text()
 assert native.count('BLOORCOURT BUS')==1
 assert hashlib.sha256(native.replace('BLOORCOURT BUS','OSSINGTON BUS').encode()).hexdigest()==traffic['protected_files']['project/plugins/toronto-driving/engine/src/td_content.c']
 for relative,checks in proof['sprite_identity_scopes'].items():
  meta=json.loads((ROOT/relative).read_text())
  for key,value in checks.items():assert meta[key]==value,('Sprite identity/layout changed',relative,key)
 for relative,old_checksum in proof['checksum_only_sprites'].items():
  meta=json.loads((ROOT/relative).read_text());meta['checksum']=old_checksum
  if relative.endswith('city_fleet.png.gbsres'):
   frames=meta['states'][0]['animations'][0]['frames']
   assert meta['width']==160 and meta['numTiles']==20 and len(frames)==21 and not frames[20]['tiles']
   assert hashlib.sha256(json.dumps(frames[:16],sort_keys=True,separators=(',',':')).encode()).hexdigest()=='ba0d3f4f4882686f86afc30d71009a2789f6b946b9e0720051f52e989032343d'
   assert all(len(f['tiles'])==2 and all(t['paletteIndex']==6 for t in f['tiles']) for f in frames[16:20])
   meta['width']=128;meta['numTiles']=16
   meta['states'][0]['animations'][0]['frames']=frames[:16]+frames[20:]
  assert hashlib.sha256((json.dumps(meta,indent=2)+'\n').encode()).hexdigest()==traffic['protected_files'][relative],('Sprite poses changed',relative)
 for relative in proof['fleet_loaders']:
  meta=json.loads((ROOT/relative).read_text());assert meta['frame']==20;meta['frame']=16
  assert hashlib.sha256((json.dumps(meta,indent=2)+'\n').encode()).hexdigest()==traffic['protected_files'][relative],('Fleet loader changed beyond appended taxi empty frame',relative)
 for relative in proof['boat_loaders']:
  meta=json.loads((ROOT/relative).read_text());assert meta['frame']==3;meta['frame']=2
  assert hashlib.sha256((json.dumps(meta,indent=2)+'\n').encode()).hexdigest()==traffic['protected_files'][relative],('Boat loader changed beyond empty frame',relative)
 with Image.open(ROOT/'project/assets/sprites/dispatch_topdown.png') as image:
  assert hashlib.sha256(image.convert('RGB').crop((0,32,128,48)).tobytes()).hexdigest()=='e0dd0b7e8406cabb36a510e8767dda3a8e057f56d6e4936f5035cdbfebe65569','Approved courier pixels changed'
  assert hashlib.sha256(image.convert('RGB').crop((128,32,192,48)).tobytes()).hexdigest()=='92e251f5697a1670309048b4ff2bccd7008aaec63340e3017b64acd83ebacc3b','Approved beacons changed'
 # Source/default colors are four ordered entries; updates preserve IDs/defaults.
 for filename,pin in proof['world_palettes'].items():
  meta=json.loads((ROOT/'project/project/palettes'/filename).read_text())
  assert meta['id']==pin['id'] and meta['colors']==pin['colors'] and meta['defaultColors']==pin['defaultColors']
 game=(ROOT/'project/plugins/toronto-driving/engine/include/td_game.h').read_text()
 assert '#define TD_ACTORS 22' in game and '#define TD_SAVE_VERSION 10' in game
 assert '#define TD_STOPS 64' in game and '#define TD_QUESTS 104' in game
 assert 'msg>21' in (ROOT/'project/plugins/toronto-driving/engine/src/td_save.c').read_text()
