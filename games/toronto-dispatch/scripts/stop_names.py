"""One reviewed display-name correction, without changing saved stop identity.

The immutable campaign remains an independent historical oracle. Only stop18
may normalize an old/new display name for old hashes; every native geometry,
service and flag field is pinned. Current source/native checks require the new
name separately, so historical normalization cannot hide a reverted game label.
"""
STOP_ID=18
OLD_NAME='OSSINGTON BUS'
CURRENT_NAME='BLOORCOURT BUS'
NATIVE_FIELDS=('id','u','v','transit','district','reserved')
EXACT_FIELDS=(18,144,64,2,0,0)
NOTICE=('Fictional compressed Bloorcourt bus handoff, not a surveyed TTC stop; '
        'regional label corrected from Ossington without moving stop18 or its saved identity')
SOURCE='https://www.toronto.ca/business-economy/business-operation-growth/business-improvement-areas/bia-list/bia-list-a-e/'
REVIEWED='2026-10-04'


def historical_name(stop):
    result=dict(stop)
    if stop['id']==STOP_ID:
        assert tuple(stop.get(k,0) for k in NATIVE_FIELDS)==EXACT_FIELDS,'Stop18 correction must not change geometry, service or flags'
        assert stop['name'] in (OLD_NAME,CURRENT_NAME),'Unapproved stop18 display name'
        result['name']=OLD_NAME
    return result


def current_name(stop):
    result=historical_name(stop)
    if stop['id']==STOP_ID:
        result.update(name=CURRENT_NAME,location_notice=NOTICE,
                      geography_source=SOURCE,geography_reviewed=REVIEWED)
    return result


def validate_current(stop):
    historical_name(stop)
    assert stop['id']==STOP_ID and stop['name']==CURRENT_NAME,'Current game must use reviewed Bloorcourt regional label'
    assert (stop.get('location_notice'),stop.get('geography_source'),stop.get('geography_reviewed'))==(NOTICE,SOURCE,REVIEWED),'Missing exact source/fictional-stop notice'
