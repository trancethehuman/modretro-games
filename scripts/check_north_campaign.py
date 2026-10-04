"""Independent North append/prefix/source invariants; no playtime acceptance."""
import copy
import json
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT/'games/toronto-dispatch'
sys.path.insert(0,str(GAME/'scripts'))
from create_north_jobs import author, preserved_prefix, NorthBodyRoutes
from create_district_jobs import RouteModel

EXPECTED=[
    (96,0,255,3,[0,61],220,180),
    (97,1,0,6,[5,62],205,172),
    (98,2,2,6,[62,3],170,156),
    (99,3,1,8,[0,62,60],220,202),
    (100,4,255,12,[0,59,62,60,63,60,0],330,252),
    (101,5,0,8,[0,62,60,17,0],255,230),
    (102,6,255,12,[62,61,63,62],310,212),
    (103,0,255,12,[61,62,63],260,195),
]
CHECKS=0


def require(value,message):
    global CHECKS
    CHECKS+=1
    assert value,message


def rejects(action,message):
    try:action()
    except AssertionError:require(True,message)
    else:require(False,message)


def check():
    campaign=json.loads((GAME/'content/campaign.json').read_text())
    require((len(campaign['stops']),len(campaign['quests']))==(64,104),'Only the bounded64/104 append is allowed')
    preserved_prefix(campaign)
    for index,kind,vehicle,gate,route,deadline,reward in EXPECTED:
        q=campaign['quests'][index]
        require((q['id'],q['kind_id'],q['required_vehicle'],q['min_completed'],q['route'],q['time_limit_seconds'],q['reward'])==
                (f'contract-{index+1:02d}',kind,vehicle,gate,route,deadline,reward),'New route/rules differ from the reviewed proposal')
    require(campaign['quests'][103]['title']=='RIDGE PARCEL ROUND','Original title must match the corrected bounded geography')
    for index,stop in enumerate(campaign['stops'][:59]):
        for field in ('u','v','transit','district','reserved'):
            bad=copy.deepcopy(campaign);bad['stops'][index][field]+=1
            rejects(lambda bad=bad:preserved_prefix(bad),'North broadened old stop geometry/identity exceptions')
    for index in range(96):
        for field in ('time_limit_seconds','reward','min_completed'):
            bad=copy.deepcopy(campaign);bad['quests'][index][field]+=1
            rejects(lambda bad=bad:preserved_prefix(bad),'North silently changed an old native deadline/reward/unlock')
    require([s['id'] for s in campaign['stops'][59:] if s['transit']]==[59,60],'Non-transit clients must never enter the six-bit service codec')
    service=campaign['transit']['line1']
    require(service['stops']==[0,12,13,14,15,16,17,59,60] and service['fare']==3 and service['period_seconds']==18,'Old fictional Line1 prefix/rules changed')
    require([i*2 for i in range(7,9)]==[14,16],'Only two new phases fit the unchanged18-second period')
    require(len(campaign['chapters'])==13 and len({q['chapter'] for q in campaign['quests'][96:]})==1,'North is one eight-offer authored group')
    expected=author();actual=json.loads((GAME/'content/districts/north_jobs.json').read_text())
    require(actual==expected and campaign['stops'][59:64]==actual['stops'] and campaign['quests'][96:104]==actual['quests'],'Registered source/body paths or fused content are stale')
    world=json.loads((GAME/'content/districts/world.json').read_text())
    model=RouteModel(world,campaign['stops'])
    ports=[p for p in world['portals'] if 6 in (p['from']['district'],p['to']['district'])]
    paths=NorthBodyRoutes(model,campaign['stops'],ports)
    # Two actual foot clients retain meaningful alternative continuous walks;
    # their native flags do not require a car return between handoffs.
    for a,b in ((59,61),(61,62),(61,63),(59,62),(62,60),(60,63)):
        distance,seams=paths.shortest(('stop',a),('stop',b),False)
        require(distance>0 and not seams,'North public last-mile path is not connected locally')
    require(not model.usable((6,688//8,176//8),True),'Document the valid StClair curb versus legacy3x3 limitation explicitly')
    require(actual['duration_verified'] is False and all(q['timing_design']['planning_only'] and q['timing_design']['measured_duration_seconds'] is None for q in actual['quests']),'Source models must not masquerade as measured native/human play')
    print(f'North source: {CHECKS} independent reviewed-route, old-prefix-negative, strict-ID, fullhalf5 source paths and provenance checks passed; native/human/hardware acceptance separate')


if __name__=='__main__':
    check()
