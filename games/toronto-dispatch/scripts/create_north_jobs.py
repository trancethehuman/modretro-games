"""Append eight authored North jobs after registered terrain, preserving old96.

New-only full-body planning remains distinct from historical route oracles and
exact-ROM campaign, balance, duration and hardware acceptance.
"""
import argparse
from collections import deque
import difflib
import heapq
import json
from pathlib import Path
import re
import sys
from create_district_jobs import (BASE_QUEST_FIELDS, FOOT_SPEED, SPEEDS,
                                  RouteModel, canonical, decode_grid, point,
                                  read_json, sha)

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'content/districts/north_jobs.json'
PREFIX_STOPS, PREFIX_QUESTS = 59, 96
STOP_FIELDS = ('id', 'u', 'v', 'name', 'transit', 'district', 'reserved')
PREFIX_STOPS_SHA256 = 'de021d3932a406dc75772cf61e330ddb6cc7d56b6956de227ff6aa1f0f349c90'
PREFIX_QUESTS_SHA256 = '64c3f534bd12c03dd50c825691e555974cc9143384199ea8fb5f0f0edb6bde5e'
GEOMETRY_FIELDS = ('dimensions', 'roads', 'footpaths', 'rails', 'rail_crossings', 'landmarks', 'parks', 'water', 'blocked_ravines', 'private_ground', 'forced_foot_masks', 'closed_frontiers', 'ports', 'core_throat_proposal', 'stop_candidates', 'traffic_loops', 'road_half_width', 'walk_half_width', 'footpath_half_width', 'rail_half_width')
# Reviewed traffic-only handedness change; mission/stop fields and terrain are unchanged.
GEOMETRY_SHA256 = 'cd2d68b830f87ca906519b65e1d7d260f47d00a3c055eb8207c613d430e89022'
CHAPTER = 'Northern hills and station rounds'
STOP_POINTS = ((696,416,'SUMMERHILL',1,0),
               (688,176,'ST CLAIR',1,0),
               (296,464,'CASA LOMA PARCEL',0,1),
               (704,552,'SUMMERHILL STOCK',0,0),
               (752,256,'ROSEHILL POST',0,1))
PARKING = {61:(336,576), 63:(640,280)}
CONTRACTS = (
    ('BALDWIN BOOK BOX',('UNION BOOK PARCEL','STAIRS OR UPPER RD'),0,255,3,[0,61],220,180,
     ['Collect the book box at Union','Walk to the public Casa Loma Parcel handoff']),
    ('SUMMERHILL GLASS',('FRAGILE GLASS KIT','BRAKE UNDER RAIL'),1,0,6,[5,62],205,172,
     ['Collect the glass kit at AGO / Grange','Deliver intact at Summerhill Stock']),
    ('HILLTOP FILES',('URGENT NORTH FILE','BIKE TO CITY HALL'),2,2,6,[62,3],170,156,
     ['Collect the sealed files at Summerhill Stock','Reach City Hall before the cutoff']),
    ('ST CLAIR STOCK',('UNION STOCK LOAD','TRUCK TO ST CLAIR'),3,1,8,[0,62,60],220,202,
     ['Load the stock at Union','Handoff the first stock at Summerhill Stock','Deliver the remaining stock at St Clair']),
    ('NORTH CONNECTION',('TRAIN OR YONGE RD','PARK EDGE WALK'),4,255,12,[0,59,62,60,63,60,0],330,252,
     ['Collect the pocket relay at Union','Handoff at Summerhill','Collect the heritage-side packet',
      'Handoff at St Clair','Walk to Rosehill Post','Return to St Clair','Deliver the relay at Union']),
    ('NORTH PICKUPS',('NORTH PASSENGERS','KEEP CAR READY'),5,0,8,[0,62,60,17,0],255,230,
     ['Start the car run at Union','Handoff at Summerhill Stock','Stop at St Clair','Stop at Bloor-Yonge','Finish at Union']),
    ('ROSEHILL SIGNED',('SIGNED PARK PAPERS','RETURN TO DESK'),6,255,12,[62,61,63,62],310,212,
     ['Collect the papers at Summerhill Stock','Walk to the Casa Loma Parcel handoff',
      'Walk to Rosehill Post','Return the original papers to Summerhill Stock']),
    ('RIDGE PARCEL ROUND',('SMALL PARCEL ROUND','TWO FOOT HANDOFFS'),0,255,12,[61,62,63],260,195,
     ['Collect the parcel at the public Casa Loma edge','Handoff at Summerhill Stock','Walk to Rosehill Post']),
)


def table(code, name):
    found = re.search(r'\b' + re.escape(name) + r'\s*\[[^;=]+\]\s*=\s*\{(.*?)\n\};', code, re.S)
    assert found, f'Missing current native prefix: {name}'
    return found.group(1)


def preserved_prefix(campaign):
    """Pin current v9 geometry too; North does not relocate historical Islands."""
    stops, jobs = campaign['stops'][:59], campaign['quests'][:96]
    assert len(stops) == 59 and len(jobs) == 96
    assert [s['id'] for s in stops] == list(range(59))
    assert [q['id'] for q in jobs] == [f'contract-{i:02d}' for i in range(1,97)]
    native_stops = [{key:s.get(key,0) for key in STOP_FIELDS} for s in stops]
    native_jobs = [{key:q[key] for key in BASE_QUEST_FIELDS} for q in jobs]
    assert sha(canonical(native_stops)) == PREFIX_STOPS_SHA256, 'Existing current59 native stop geometries/identities changed'
    assert sha(canonical(native_jobs)) == PREFIX_QUESTS_SHA256, 'Existing96 native job fields/briefs/ordinals changed'
    code = (ROOT / 'project/plugins/toronto-driving/engine/src/td_content.c').read_text()
    rows = re.findall(r'\{(\d+),(\d+),"([^"]+)",(\d+),(\d+),(\d+)\}',table(code,'td_stops'))
    assert len(rows) >= 59
    for stop,row in zip(stops,rows[:59]):
        u,v,name,service,district,flags=row
        assert (int(u),int(v),name,int(service),int(district),int(flags)) == tuple(stop.get(k,0) for k in STOP_FIELDS[1:])
    rows = re.findall(r'\{"([^"]+)",(\d+),(\d+),(\d+),(\d+),(\d+),(\d+),\{([\d,]+)\}\}',table(code,'td_jobs'))
    assert len(rows) >= 96
    for job,row in zip(jobs,rows[:96]):
        title,*numbers,route=row
        assert title == job['title']
        assert list(map(int,numbers)) == [job['kind_id'],len(job['route']),job['required_vehicle'],job['min_completed'],job['time_limit_seconds'],job['reward']]
        assert list(map(int,route.split(','))) == job['route'] + [255]*(12-len(job['route']))
    briefs = re.findall(r'"([^"]*)"',table(code,'td_briefs'))
    assert briefs[:96] == [''.join(s.ljust(18) for s in job['brief']) for job in jobs]
    return stops,jobs


def full_body(scene,u,v,half,vehicle=False):
    grid=scene['grid'];width=scene['width'];height=scene['height']
    left,right,top,bottom=(u-half)//8,(u+half)//8,(v-half)//8,(v+half)//8
    return (0<=left<=right<width and 0<=top<=bottom<height and
            all(not (grid[y*width+x] if vehicle else grid[y*width+x]&15)
                for y in range(top,bottom+1) for x in range(left,right+1)))


class NorthBodyRoutes:
    """Four-pixel fullhalf5 graph, explicitly confined to Core and North.

    The legacy3x3 tile-centre model rejects the valid StClair176 curb. Do not
    alter historical models or move that landing. This independent graph uses
    exact points and retains every touched terrain tile, with one4px cardinal
    edge's swept hull covered by the union of its endpoint hulls. It excludes
    dynamic occupancy and optional detours into other districts.
    """
    def __init__(self,model,stops,portals):
        self.scenes={i:dict(width=w,height=h,grid=g) for i,(w,h,g) in model.grids.items() if i in (0,6)}
        self.nodes={}
        for stop in stops:
            if stop['id'] in (0,3,5,17,59,60,61,62,63):
                self.nodes[('stop',stop['id'])]=(stop['district'],stop['u'],stop['v'])
                if stop.get('foot_only'):
                    self.nodes[('park',stop['id'])]=(stop['district'],stop['parking_anchor']['u'],stop['parking_anchor']['v'])
        self.crossings=[]
        for index,p in enumerate(portals):
            keys=[]
            for side in ('from','to'):
                e=p[side];key=('seam',index,side)
                self.nodes[key]=(e['district'],e['u'],e['v']);keys.append(key)
            self.crossings.append((*keys,p['modeled_crossing_pixels'],p['name']))
        self.available={};self.local_cache={};self.route_cache={}

    def local(self,first,vehicle):
        if (first,vehicle) in self.local_cache:return self.local_cache[first,vehicle]
        district,u,v=first;scene=self.scenes[district];columns,rows=scene['width']*2,scene['height']*2
        assert u%4==v%4==0
        if (district,vehicle) not in self.available:
            self.available[district,vehicle]=bytearray(full_body(scene,x*4,y*4,5,vehicle) for y in range(rows) for x in range(columns))
        available=self.available[district,vehicle];start=(v//4)*columns+u//4
        assert available[start],f'Blocked fullhalf5 start:{first}/{vehicle}'
        pending=deque([start]);distance={start:0}
        while pending:
            i=pending.popleft();x,y=i%columns,i//columns
            for nx,ny in ((x-1,y),(x+1,y),(x,y-1),(x,y+1)):
                if not(0<=nx<columns and 0<=ny<rows):continue
                target=ny*columns+nx
                if available[target] and target not in distance:
                    distance[target]=distance[i]+4;pending.append(target)
        self.local_cache[first,vehicle]=(distance,columns)
        return distance,columns

    def shortest(self,start,end,vehicle):
        key=start,end,vehicle
        if key in self.route_cache:return self.route_cache[key]
        graph={k:[] for k in self.nodes}
        for a,p in self.nodes.items():
            if not full_body(self.scenes[p[0]],p[1],p[2],5,vehicle):continue
            local,columns=self.local(p,vehicle)
            for b,q in self.nodes.items():
                index=(q[2]//4)*columns+q[1]//4
                if a!=b and p[0]==q[0] and index in local:graph[a].append((b,local[index],None))
        for a,b,weight,name in self.crossings:
            graph[a].append((b,weight,name));graph[b].append((a,weight,name))
        pending=[(0,start)];distance={start:0};parents={}
        while pending:
            cost,node=heapq.heappop(pending)
            if cost!=distance[node]:continue
            if node==end:break
            for target,weight,name in graph[node]:
                trial=cost+weight
                if trial<distance.get(target,float('inf')):
                    distance[target]=trial;parents[target]=(node,name);heapq.heappush(pending,(trial,target))
        assert end in distance,f'No fullhalf5 Core/North route:{start}->{end}/{vehicle}'
        seams=[];node=end
        while node!=start:
            node,name=parents[node]
            if name:seams.append(name)
        result=distance[end],list(reversed(seams));self.route_cache[key]=result
        return result

    def stop_leg(self,first,last):
        a=('park' if first.get('foot_only') else 'stop',first['id'])
        b=('park' if last.get('foot_only') else 'stop',last['id'])
        return self.shortest(a,b,True)


def author():
    campaign=read_json(ROOT/'content/campaign.json')
    old_stops,old_jobs=preserved_prefix(campaign)
    world=read_json(ROOT/'content/districts/world.json')
    from district_sources import read_district_art
    assert len(world['districts'])==7 and world['districts'][6]['scene']=='toronto_north', 'Register actual seventh scene before authoring North content'
    metadata=read_district_art(world['districts'][6],root=ROOT)
    assert metadata['id']==6 and metadata['dimensions']==[1024,976]
    assert sha(canonical({key:metadata[key] for key in GEOMETRY_FIELDS}))==GEOMETRY_SHA256, 'Review changed geometry before reauthoring'
    native=read_json(ROOT/'project/project/scenes/toronto_north/scene.gbsres')
    assert native['_resourceType']=='scene' and native['type']=='TORONTO' and (native['width'],native['height'])==(128,122)
    scene=dict(width=native['width'],height=native['height'],grid=decode_grid(native['collisions']))
    assert scene['grid']==metadata['collisions'], 'North actual scene/art collision differs'
    portals=[p for p in world['portals'] if 6 in (p['from']['district'],p['to']['district'])]
    assert len(portals)==2 and {p['from']['u'] for p in portals}=={336,640}
    for p in portals:
        x=p['from']['u']
        assert p['from']==dict(district=0,u=x,v=24) and p['to']==dict(district=6,u=x,v=952)
        assert set(p['access'])=={'foot','vehicle'} and p['modeled_crossing_pixels']==48
    candidates=metadata['stop_candidates']
    assert [s['id_proposal'] for s in candidates]==list(range(59,64))
    stops=[]
    for index,(candidate,expected) in enumerate(zip(candidates,STOP_POINTS),59):
        u,v,name,service,flags=expected
        assert (candidate['x'],candidate['y'],candidate['name'],candidate['transit'],int(candidate['foot_only']))==expected
        assert candidate['fictional_service_point'] is True
        stop=dict(id=index,u=u,v=v,name=name,transit=service,district=6,reserved=flags,
                  foot_only=bool(flags),location_notice='Original compressed public courier point; not a surveyed door or station parking')
        assert full_body(scene,u,v,5),f'Blocked North fullfoot5 client:{index}'
        if flags:
            pu,pv=PARKING[index]
            assert candidate['parking_anchor']==[pu,pv]
            stop['parking_anchor']=dict(u=pu,v=pv)
            assert full_body(scene,pu,pv,7,True) and not full_body(scene,u,v,5,True)
        else:
            assert full_body(scene,u,v,7,True),f'Blocked North conservative parking/handoff:{index}'
        stops.append(stop)
    all_stops=old_stops+stops
    model=RouteModel(world,all_stops)
    body_model=NorthBodyRoutes(model,all_stops,portals)
    anchor_walks={}
    for stop in stops:
        model.full_foot_shortest(all_stops[59],stop,5)
        if stop['foot_only']:
            anchor={**stop,**stop['parking_anchor']}
            distance=model.full_foot_shortest(anchor,stop,5)
            assert 0<distance<=256
            anchor_walks[stop['id']]=distance
        else:
            assert full_body(scene,stop['u'],stop['v'],5,True)
    jobs=[]
    for index,(title,brief,kind,vehicle,gate,route,deadline,reward,objectives) in enumerate(CONTRACTS,96):
        assert title.isascii() and len(title)<=18 and all(s.isascii() and len(s)<=18 for s in brief)
        assert 2<=len(route)<=12 and len(objectives)==len(route)
        assert all(0<=s<64 for s in route) and all(a!=b for a,b in zip(route,route[1:]))
        assert any(s>=59 for s in route)
        assert vehicle==255 or not any(all_stops[s].get('foot_only') for s in route)
        assert kind!=3 or vehicle==1
        assert kind!=5 or vehicle==0
        assert kind!=6 or route[0]==route[-1]
        legs=[];road=walking=0
        for first,last in zip(route,route[1:]):
            a,b=all_stops[first],all_stops[last]
            driven,seams=body_model.stop_leg(a,b)
            walked=sum(anchor_walks.get(s,0) for s in (first,last))
            road+=driven;walking+=walked
            legs.append(dict(from_stop=first,to_stop=last,from_district=a['district'],to_district=b['district'],
                             full_half5_vehicle_pixels=driven,full_foot5_parking_walk_pixels=walked,modeled_seams=seams))
        fastest,slowest=(SPEEDS[vehicle],SPEEDS[vehicle]) if vehicle!=255 else (max(SPEEDS.values()),min(SPEEDS.values()))
        fast=road/fastest+walking/FOOT_SPEED;slow=road/slowest+walking/FOOT_SPEED
        handling=5*(len(route)-1)
        job=dict(id=f'contract-{index+1:02d}',title=title,brief=list(brief),chapter=CHAPTER,
                 kind=campaign['quest_types'][kind],kind_id=kind,required_vehicle=vehicle,min_completed=gate,
                 route=route,time_limit_seconds=deadline,reward=reward,
                 stages=[dict(stop_id=s,district=all_stops[s]['district'],objective=obj) for s,obj in zip(route,objectives)],
                 objective_notice='Existing ordered handoffs, condition, vehicle and return rules; no new loading/signature/passenger UI or forced train requirement',
                 timing_design=dict(planning_only=True,measured_duration_seconds=None,deadline_status='Provisional authored limit; ordinary-input tuning pending',
                                    full_half5_vehicle_route_pixels=road,required_full_foot5_parking_walk_pixels=walking,
                                    normalized_moving_seconds_fast=round(fast,1),normalized_moving_seconds_slow=round(slow,1),
                                    assumed_control_and_handoff_seconds=handling,modeled_nominal_slack_seconds=round(deadline-slow-handling,1),
                                    legs=legs,basis='Registered collision grids after integration; proposed fixture beforehand. Exact vehicle/foot half5 bodies on four-pixel Core+North graph; endpoints cover each cardinal edge sweep. Legacy3x3 models unchanged. Excludes optional outside-district detours, dynamic occupancy, acceleration, turns, prepositioning, scene/load/entry waits and observed CPU pacing. Parking-per-leg can overcount native continuous walking. Not measured time.'))
        assert job['timing_design']['modeled_nominal_slack_seconds']>0, f'Explicit deadline review needed:{index}'
        jobs.append(job)
    merged=old_jobs+jobs
    assert len(merged)==104 and len({q['title'] for q in merged})==104 and len({tuple(q['route']) for q in merged})==104
    assert {s for q in jobs for s in q['route'] if s>=59}==set(range(59,64))
    completed=0
    while completed<104:
        unlocked=sum(q['min_completed']<=completed for q in merged)
        assert unlocked>completed
        completed=unlocked
    return dict(schema_version=1,status='Authored source after registered geometry; exact-ROM campaign/native/hardware pending',
                scope='Eight North contracts and five original public clients/station landings appended after unchanged59/96 prefix',
                base_stop_count=59,base_quest_count=96,result_stop_count=64,result_quest_count=104,
                preserved_base_stops_sha256=PREFIX_STOPS_SHA256,preserved_base_quests_sha256=PREFIX_QUESTS_SHA256,
                preserved_base_stop_fields=list(STOP_FIELDS),preserved_base_quest_fields=list(BASE_QUEST_FIELDS),
                world_sha256=sha((ROOT/'content/districts/world.json').read_bytes()),collision_resources=model.resources,
                geography_metadata=dict(path='content/districts/north_art.json',source_layout_sha256=metadata['source_layout_sha256'],geometry_sha256=GEOMETRY_SHA256,
                                         notice='Source facts and original compressed art; no surveyed service doors'),
                line1_append=[59,60],preserved_line1_prefix=[0,12,13,14,15,16,17],new_line1_phases=[14,16],
                train_period_seconds=18,train_fare=3,forced_transit=False,
                foot_parking_walks=[dict(stop_id=i,full_half5_walking_pixels=n) for i,n in sorted(anchor_walks.items())],stops=stops,quests=jobs,
                duration_target_minutes=120,duration_verified=False,
                duration_notice='Counts, route distances, deadlines and waiting do not establish two enjoyable human hours.',
                compatibility_notice='Requires seven scenes/savev10 same58 bytes with explicit oldv9 caps/import; preserve16 completion bytes and all old IDs. IDs59/60 only enter Line1; nontransit61/62/63 never alias service IDs.')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true')
    parser.add_argument('--diff',action='store_true')
    args=parser.parse_args()
    expected=json.dumps(author(),indent=2)+'\n'
    actual=OUTPUT.read_text() if OUTPUT.exists() else ''
    if args.diff:
        sys.stdout.writelines(difflib.unified_diff(actual.splitlines(True),expected.splitlines(True),fromfile='north_jobs.json',tofile='authored north_jobs.json'))
    if args.check or args.diff:
        return int(actual!=expected)
    OUTPUT.parent.mkdir(parents=True,exist_ok=True)
    OUTPUT.write_text(expected)
    return 0


if __name__=='__main__':
    raise SystemExit(main())
