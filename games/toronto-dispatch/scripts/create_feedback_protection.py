"""Explicitly capture accepted feedback proofs, retaining old geometry/job pins.

This is a maintainer operation, never part of automatic make check. It refuses
unknown JSON scope changes and authenticates every old file against the frozen
traffic fixture before recording the current approved visual artifact hashes.
"""
import argparse,base64,copy,hashlib,io,json,subprocess,zlib
from pathlib import Path
from feedback_protection import ROOT,REPO,PATH,canonical,digest,get_path,AIRCRAFT_FILES,AIRCRAFT_META,AIRCRAFT_ART,aircraft_extension_check,aircraft_extension_negatives
OLD='8ea5ec413c3650cd3441ee588ba15f87a98889fa'

def old_bytes(relative):return subprocess.check_output(['git','show',OLD+':games/toronto-dispatch/'+relative],cwd=REPO)
def strip(value,pin):
 value=copy.deepcopy(value)
 for key in pin.get('omit_top_keys',[]):value.pop(key,None)
 for key in pin.get('omit_geography_keys',[]):value['geography_metadata'].pop(key)
 if pin.get('geography_sha256_sources'):
  for row in value['geography_metadata']:row.pop('sha256')
 return value

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--capture-approved',action='store_true');args=parser.parse_args();assert args.capture_approved,'Explicit maintainer capture flag required'
 traffic=json.loads((REPO/'tests/fixtures/traffic_lanes.json').read_text());raw={};scopes={}
 for relative,expected in traffic['protected_files'].items():
  before=old_bytes(relative);assert hashlib.sha256(before).hexdigest()==expected,('Historical raw pin mismatch',relative)
  current=(ROOT/relative).read_bytes();after=hashlib.sha256(current).hexdigest()
  if after!=expected:raw[relative]={'before_sha256':expected,'after_sha256':after}
 # Only original city/vehicle/boat artwork, the six named aircraft artifacts
 # with four appended jets, empty loaders and accepted actor/save notices fit.
 allowed={'content/campaign.json','project/plugins/toronto-driving/engine/src/td_content.c','project/plugins/toronto-driving/engine/include/td_game.h','project/plugins/toronto-driving/engine/src/td_save.c'}
 for relative in raw:
  assert relative in allowed or relative in AIRCRAFT_FILES or relative.endswith(('ambient_boat_loader.gbsres','city_fleet_loader.gbsres')) or (relative.startswith('project/assets/backgrounds/toronto_') and relative.endswith('.png')) or (relative.startswith(('project/assets/sprites/','project/original-art/')) and any(name in relative for name in ('ambient_boat','city_fleet','dispatch_topdown','toronto_east.png','toronto_islands.png','toronto_north.png','toronto_port_lands.png'))),('Unauthorised protected raw change',relative)
 for relative,pin in traffic['protected_json'].items():
  old=strip(json.loads(old_bytes(relative)),pin);new=strip(json.loads((ROOT/relative).read_text()),pin)
  assert digest(old)==pin['sha256'],('Historical geometry/job pin mismatch',relative)
  if old==new:continue
  paths=[]
  if relative.endswith('_art.json'):
   paths=[['scenery'],['background_sha256'],['background_pixel_sha256'],['validation','raw_unique_tiles'],['validation','flip_canonical_unique_tiles']]
   if relative.endswith('island_art.json'):paths += [['palette_proposal','slot6_colors_light_to_dark'],['palette_proposal','native_palette_registered']]
  elif relative=='content/districts/west_jobs.json':paths=[['quests',4,'stages',2,'objective']]
  selected=[];restored=copy.deepcopy(new)
  for path in paths:
   try:a=get_path(old,path);b=get_path(new,path)
   except KeyError:continue
   if a==b:continue
   selected.append({'path':path,'before_sha256':digest(a),'after_sha256':digest(b),'before_json_zlib':base64.b64encode(zlib.compress(canonical(a),9)).decode()})
   get_path(restored,path[:-1])[path[-1]]=a
  assert restored==old,('Change outside accepted visual/name scope',relative)
  scopes[relative]=selected
 identity={};checksum={};loaders=[];fleet_loaders=[]
 for relative in raw:
  if relative.endswith('ambient_boat_loader.gbsres'):loaders.append(relative)
  if relative.endswith('city_fleet_loader.gbsres'):fleet_loaders.append(relative)
  if relative.endswith('.gbsres') and any(name in relative for name in ('dispatch_topdown.png','city_fleet.png')):checksum[relative]=json.loads(old_bytes(relative))['checksum']
  if relative.endswith('ambient_boat.png.gbsres'):
   before=json.loads(old_bytes(relative));identity[relative]={k:before[k] for k in ('id','name','symbol','filename','animSpeed')}
 palettes={}
 for file in (ROOT/'project/project/palettes').glob('toronto_*.gbsres'):
  d=json.loads(file.read_text());palettes[file.name]={k:d[k] for k in ['id','colors','defaultColors']}
 stop=json.loads((ROOT/'content/campaign.json').read_text())['stops'][18]
 from PIL import Image
 with Image.open(io.BytesIO(old_bytes('project/assets/sprites/ambient_aircraft.png'))) as image:
  assert image.size==(416,32),'Unexpected historical aircraft dimensions'
  original_rgb=hashlib.sha256(image.convert('RGB').tobytes()).hexdigest()
 with Image.open(ROOT/'project/assets/sprites/ambient_aircraft.png') as image:
  jet_rgb=hashlib.sha256(image.convert('RGB').crop((448,0,576,32)).tobytes()).hexdigest()
 aircraft={'original_frame_indices':list(range(14)),'jet_frame_indices':[14,15,16,17],
  'original_rgb_sha256':original_rgb,'jet_rgb_sha256':jet_rgb,'metadata':{},
  'original_art_json_zlib':base64.b64encode(zlib.compress(old_bytes(AIRCRAFT_ART),9)).decode()}
 for relative in AIRCRAFT_META:
  before=json.loads(old_bytes(relative));current=json.loads((ROOT/relative).read_text())
  aircraft['metadata'][relative]={'original_checksum':before['checksum'],
   'original_frames_sha256':digest(before['states'][0]['animations'][0]['frames']),
   'jet_frames_sha256':digest(current['states'][0]['animations'][0]['frames'][14:])}
 proof={'schema':1,'purpose':'Accepted original visual improvements, appended original taxi poses and four jet shadows, source-correct stop18 alias and native22-actor/save-notice capacity. Historical traffic fixture remains immutable; all terrain/client/job fields, previous fleet metadata and aircraft frames0..13/pixels/identity/defaults are reconstructed against its original pins.','predecessor_commit':OLD,'historical_fixture_sha256':hashlib.sha256((REPO/'tests/fixtures/traffic_lanes.json').read_bytes()).hexdigest(),'raw_scopes':raw,'json_scopes':scopes,'stop18_location_notice':stop['location_notice'],'checksum_only_sprites':checksum,'sprite_identity_scopes':identity,'boat_loaders':loaders,'fleet_loaders':fleet_loaders,'world_palettes':palettes,'aircraft_extension':aircraft}
 aircraft_extension_check(traffic,proof);aircraft_extension_negatives(traffic,proof)
 PATH.write_text(json.dumps(proof,indent=2)+'\n');print(f'Captured {len(raw)} explicit raw feedback changes, {sum(map(len,scopes.values()))} visual/name fields; immutable geometry/client/job digests retained')
if __name__=='__main__':main()
