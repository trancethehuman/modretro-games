"""Check actual native collision connectivity and campaign progression, not elapsed playtime."""
import json,re
from pathlib import Path
from collections import deque
ROOT=Path(__file__).resolve().parents[1]
def decode(text):
    result=[];pos=0
    while pos<len(text):
        value=int(text[pos:pos+2],16);pos+=2
        if text[pos]=='!':count=1;pos+=1
        else:end=text.index('+',pos);count=int(text[pos:end],16);pos=end+1
        result.extend([value]*count)
    return result

def check():
    campaign=json.loads((ROOT/'content/campaign.json').read_text());city=json.loads((ROOT/'content/city_art.json').read_text())
    scene=json.loads((ROOT/'project/project/scenes/toronto_city/scene.gbsres').read_text());background=json.loads((ROOT/'project/assets/backgrounds/toronto_city.png.gbsres').read_text())
    width,height=scene['width'],scene['height'];grid=decode(scene['collisions']);attrs=decode(background['tileColors'])
    assert len(grid)==len(attrs)==width*height,'Native grid dimensions disagree'
    assert city['dimensions']==[width*8,height*8]
    assert width*height<=16384,'Tile map must fit one ROM bank'
    assert len(city['blocks'])>=50,'City should retain architectural variety'
    assert len(set(b['style'] for b in city['blocks']))==6
    assert any(a&128 for a in attrs),'Missing actual CGB roof priority flags'
    def blocked(x,y,car=False):
        if not (0<=x<width and 0<=y<height):return True
        return grid[y*width+x]!=0 if car else bool(grid[y*width+x]&15)
    def usable(x,y,car):
        return not blocked(x,y,car) and (not car or all(not blocked(x+dx,y+dy,True) for dx in [-1,0,1] for dy in [-1,0,1]))
    stops=campaign['stops'];locations=[(s['u']//8,s['v']//8) for s in stops]
    for s,(x,y) in zip(stops,locations):assert usable(x,y,False),f"Unreachable stop footprint: {s['name']}"
    def flood(origin,car=False):
        queue=deque([origin]);seen={origin}
        while queue:
            x,y=queue.popleft()
            for nx,ny in [(x-1,y),(x+1,y),(x,y-1),(x,y+1)]:
                if (nx,ny) not in seen and usable(nx,ny,car):seen.add((nx,ny));queue.append((nx,ny))
        return seen
    walk=flood(locations[0]);car=flood(locations[0],True)
    ferry=campaign['transit']['ferry']['stops']
    assert locations[ferry[0]] in walk
    for island in ferry[1:]:walk.update(flood(locations[island]))
    assert all(p in walk for p in locations),'Disconnected pedestrian/ferry endpoint'
    assert all(locations[s['id']] in car for s in stops if s['id'] not in ferry[1:]),'Disconnected vehicle endpoint'
    quests=campaign['quests'];assert len(quests)==72 and len({q['id'] for q in quests})==72
    assert campaign['status']=='engine-integrated' and campaign['duration_target_minutes']>=120
    assert len({q['kind_id'] for q in quests})==8
    for q in quests:
        assert 2<=len(q['route'])<=12 and all(0<=i<len(stops) for i in q['route'])
        assert q['min_completed']<len(quests) and q['time_limit_seconds']>0 and q['reward']>0
        if q['required_vehicle']!=255:assert all(locations[i] in car for i in q['route'])
    assert sum(q['min_completed']==0 for q in quests)>=12,'Unlock progression can deadlock'
    # Verify data compilation deterministically without regenerating files during CI.
    c=(ROOT/'project/plugins/toronto-driving/engine/src/td_content.c').read_text()
    for s in stops:assert f'{{{s["u"]},{s["v"]},"{s["name"]}",{s["transit"]}}}' in c
    for q in quests:
        route=q['route']+[255]*(12-len(q['route']))
        row='{'+f'"{q["title"][:18].upper()}",{q["kind_id"]},{len(q["route"])},{q["required_vehicle"]},{q["min_completed"]},{q["time_limit_seconds"]},{q["reward"]},'+'{'+','.join(map(str,route))+'}},'
        assert row in c,f'Stale native contract: {q["id"]}'
    print(f'Native campaign: {len(quests)} contracts, {len(stops)} reachable stops, {len(city["blocks"])} buildings, vehicle and pedestrian/ferry connectivity passed. Duration requires playtesting.')
if __name__=='__main__':check()
