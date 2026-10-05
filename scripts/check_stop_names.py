"""Reject broadened historical normalization of the reviewed stop18 label."""
from pathlib import Path
import json
import sys

ROOT=Path(__file__).resolve().parents[1]
GAME=ROOT/'games/toronto-dispatch'
sys.path.insert(0,str(GAME/'scripts'))
from island_campaign import frozen_campaign, historical_stop, validate_preserved_campaign
from stop_names import current_name, historical_name, validate_current, NATIVE_FIELDS


def rejected(call):
    try:call()
    except (AssertionError,KeyError):return
    raise AssertionError('Reviewed stop18 exception accepted an unrelated change')


def main():
    original=frozen_campaign()['stops'][18]
    assert original['name']=='OSSINGTON BUS'
    campaign=json.loads((GAME/'content/campaign.json').read_text());current=campaign['stops'][18]
    validate_current(current);validate_preserved_campaign(campaign)
    assert historical_name(current)['name']==original['name']
    assert all(historical_name(current)[key]==original[key] for key in original)
    assert current_name(original)==current and historical_stop(current)['name']==original['name']
    for field in NATIVE_FIELDS[1:]:
        bad=dict(current);bad[field]+=1;rejected(lambda bad=bad:historical_name(bad))
    for name in ('OSSINGTON','ANNEX BUS','BATHURST BUS','UNKNOWN BUS'):
        rejected(lambda name=name:historical_name(dict(current,name=name)))
    for field in ('location_notice','geography_source','geography_reviewed'):
        rejected(lambda field=field:validate_current({**current,field:'unreviewed'}))
    rejected(lambda:validate_current({**current,'name':'OSSINGTON BUS'}))
    # A different ID cannot take the display exception: normalization must
    # leave it untouched and the immutable full-prefix checksum must reject it.
    other=dict(current,id=19)
    assert historical_name(other)==other
    wrong=json.loads(json.dumps(campaign));wrong['stops'][18]['id']=19
    rejected(lambda:validate_preserved_campaign(wrong))
    native=(GAME/'project/plugins/toronto-driving/engine/src/td_content.c').read_text()
    assert native.count('{144,64,"BLOORCOURT BUS",2,0,0}')==1 and '"OSSINGTON BUS"' not in native
    print('Reviewed stop18 label checks passed: immutable old hash retained; only exact regional display name may change.')


if __name__=='__main__':main()
