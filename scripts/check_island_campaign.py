"""Independent source regressions for six Island relocations and one label.

No native runtime, ROM, save or elapsed-gameplay acceptance is implied. Pin the
public pre-migration 59/96 fields independently, reject broadened exceptions,
and keep ferry service separate from ordinary navigation and vehicle paths.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / 'games/toronto-dispatch'
sys.path.insert(0, str(GAME / 'scripts'))
from island_campaign import historical_stop, validate_preserved_campaign
from create_district_jobs import RouteModel, point

CHECKS = 0
OLD = {20: (444, 928, 0), 21: (720, 920, 0), 22: (848, 896, 0),
       24: (560, 928, 0), 25: (760, 944, 0), 26: (912, 912, 0)}
NEW = {20: (320, 280, 5), 21: (512, 448, 5), 22: (920, 280, 5),
       24: (160, 600, 5), 25: (512, 744, 5), 26: (904, 440, 5)}
STOP_FIELDS = ('id', 'u', 'v', 'name', 'transit', 'district', 'reserved')
JOB_FIELDS = ('id', 'title', 'brief', 'kind_id', 'required_vehicle',
              'min_completed', 'route', 'time_limit_seconds', 'reward')


def require(value, message):
    global CHECKS
    CHECKS += 1
    assert value, message


def rejects(action, message):
    try:
        action()
    except AssertionError:
        require(True, message)
    else:
        require(False, message)


def check():
    raw = (GAME / 'content/districts/island_campaign_v8.json').read_bytes()
    require(hashlib.sha256(raw).hexdigest() == 'ed3354c8f0e726f369c86f8f36f4373427a56d2d920bbca078252bed8890053b', 'Immutable 59/96 source oracle changed')
    old = json.loads(raw)
    campaign = json.loads((GAME / 'content/campaign.json').read_text())
    require((len(campaign['stops']), len(campaign['quests'])) == (64,104), 'Only the bounded North append is accepted')
    from create_north_jobs import preserved_prefix as preserved_north_prefix
    preserved_north_prefix(campaign)
    for stop, expected in zip(campaign['stops'][:59], old['stops']):
        fields = {key: stop.get(key, 0) for key in STOP_FIELDS}
        if stop['id'] in NEW:
            require((fields['u'], fields['v'], fields['district']) == NEW[stop['id']], 'Current Island geometry differs from independent expected triple')
            fields.update(zip(('u', 'v', 'district'), OLD[stop['id']]))
        if stop['id']==18:
            require(tuple(fields[k] for k in STOP_FIELDS)==(18,144,64,'BLOORCOURT BUS',2,0,0),
                    'Stop18 display correction changed current native geometry/service/flags or label')
            fields['name']='OSSINGTON BUS'
        require(fields == expected, 'A field other than six declared geometries or the exact stop18 label changed')
    require([{key: quest[key] for key in JOB_FIELDS} for quest in campaign['quests'][:96]] == old['quests'], 'A native job/brief/reward/deadline/completion ordinal changed')
    validate_preserved_campaign(campaign)
    for field,value in (('u',145),('v',65),('district',1),('transit',1),('reserved',1),('name','ANNEX BUS')):
        bad=copy.deepcopy(campaign);bad['stops'][18][field]=value
        rejects(lambda bad=bad:validate_preserved_campaign(bad),'Stop18 display correction broadened geometry/service/flag/name exceptions')
    for index in OLD:
        for geometry in (OLD[index], NEW[index]):
            stop = dict(campaign['stops'][index])
            stop.update(zip(('u', 'v', 'district'), geometry))
            require({key: historical_stop(stop).get(key, 0) for key in STOP_FIELDS} == old['stops'][index], 'Historical normalization does more than the permitted geometry')
        stop = dict(campaign['stops'][index])
        for field, value in (('u', stop['u'] + 1), ('v', stop['v'] + 1), ('district', 0),
                             ('name', stop['name'] + 'X'), ('transit', 7), ('reserved', 1),
                             ('foot_only', True), ('parking_anchor', {'u': 320, 'v': 280})):
            bad = {**stop, field: value}
            rejects(lambda bad=bad: historical_stop(bad), f'Island exception accepted altered {index}/{field}')
    for index, stop in enumerate(campaign['stops'][:59]):
        if index not in OLD:
            bad = copy.deepcopy(campaign)
            bad['stops'][index]['u'] += 1
            rejects(lambda bad=bad: validate_preserved_campaign(bad), 'Non-Island geometry exception broadened')
    for index in range(96):
        for field in ('time_limit_seconds', 'reward', 'min_completed'):
            bad = copy.deepcopy(campaign)
            bad['quests'][index][field] += 1
            rejects(lambda bad=bad: validate_preserved_campaign(bad), 'Saved native contract fields were silently recalculated')
    world = json.loads((GAME / 'content/districts/world.json').read_text())
    model = RouteModel(world, campaign['stops'])
    require(not any(portal[side]['district'] == 5 for portal in world['portals'] for side in ('from', 'to')), 'Ordinary Island seam exists')
    require(set(model.grids[5][2]) == {15, 16}, 'Public Island has car terrain')
    for dock, client, pixels in ((20, 24, 480), (21, 25, 296), (22, 26, 176)):
        a, b = campaign['stops'][dock], campaign['stops'][client]
        for first, last in ((a, b), (b, a)):
            require(model.full_foot_shortest(first, last) == pixels, 'Conservative full-body client route changed')
            require(model.service_leg(first, last)['mode'] == 'foot', 'Client approach was relabeled as paid travel')
        for origin, target in ((10, dock), (dock, 10)):
            require(model.service_leg(campaign['stops'][origin], campaign['stops'][target]) ==
                    dict(mode='ferry', from_stop=origin, to_stop=target, walking_pixels=0,
                         worst_wait_seconds=28, ride_seconds=8, fare=4), 'Ferry spoke/rule changed')
        rejects(lambda a=a: model.shortest(point(0, campaign['stops'][10]['u'], campaign['stops'][10]['v']), point(5, a['u'], a['v']), False), 'Ferry became ordinary walking')
        rejects(lambda b=b: model.service_leg(campaign['stops'][10], b), 'Ferry lands directly at a client')
        rejects(lambda a=a, b=b: model.stop_leg(a, b), 'Island became a road/parking mission')
    for index in OLD:
        stop = campaign['stops'][index]
        require(not model.usable(point(5, stop['u'], stop['v']), True), 'Vehicle reaches Island endpoint')
    for first, last in ((20, 21), (21, 22), (22, 20)):
        require(model.service_leg(campaign['stops'][first], campaign['stops'][last])['mode'] == 'foot', 'Connected public Island paths require invented inter-Island ferry')
    jobs = [(index, job) for index, job in enumerate(campaign['quests']) if job['kind_id'] == 7]
    require([index for index, _ in jobs] == [7, 15, 23, 31, 39, 47, 55, 63, 71], 'Island completion ordinals moved')
    require([job['time_limit_seconds'] for _, job in jobs] == [195, 205, 200, 310, 305, 310, 360, 360, 360], 'A named Island deadline changed without tuning evidence')
    require(all(job['timing_design']['planning_only'] and job['timing_design']['measured_duration_seconds'] is None and job['timing_design']['modeled_slack_seconds'] > 0 for _, job in jobs), 'Model is mislabeled as measured play or exceeds preserved limit')
    print(f'Island campaign source: {CHECKS} independent prefix, geometry-negative, full-body foot, typed ferry/disconnection and deadline regressions passed; native/human duration remains separate')


if __name__ == '__main__':
    check()
