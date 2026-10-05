"""Audit the native actor pool extension without substituting a host ABI.

The body must reconstruct the exact installed GB Studio4.3.2 stock header by
reversing one define. Every type, declaration and inline helper stays unchanged;
a sanitizer mock's actor capacity cannot satisfy this native source gate.
"""
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'games/toronto-dispatch/project/plugins/toronto-driving/engine'
STOCK_SHA256 = 'aaed624fc6755e3200326bb5a1f636cd46432d0ab92c7181586b9451255f4db6'


def check_header(source):
    assert source.count('#ifndef ACTOR_H') == 1, 'Native actor include guard changed'
    body = source[source.index('#ifndef ACTOR_H'):]
    assert body.count('#define MAX_ACTORS            22') == 1, 'Native pool must contain22 actors'
    assert '#define MAX_ACTORS_ACTIVE     12' in body, 'Stock active-actor cap changed'
    original = body.replace('#define MAX_ACTORS            22', '#define MAX_ACTORS            21')
    assert hashlib.sha256(original.encode()).hexdigest() == STOCK_SHA256, 'Native actor ABI changed beyond the single pool define'
    assert STOCK_SHA256 in source and 'bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3' in source, 'Missing native header provenance'
    assert 'Copyright (c) 2020 Toxa' in source and 'THE SOFTWARE IS PROVIDED' in source, 'Missing upstream MIT notice'


def main():
    source = (ENGINE / 'include/actor.h').read_text()
    check_header(source)
    manifest = json.loads((ENGINE / 'engine.json').read_text())
    files = next(s['files'] for s in manifest['sceneTypes'] if s['key'] == 'TORONTO')
    assert files.count('include/actor.h') == 1, 'Native pool header is not registered exactly once'
    game = (ENGINE / 'include/td_game.h').read_text()
    tram = (ENGINE / 'include/td_streetcar_runtime.h').read_text()
    assert re.search(r'^#define TD_ACTORS 22$', game, re.M)
    assert re.search(r'^#define TD_STREETCAR_ACTOR 21$', tram, re.M)
    for before, after in (
        ('#define MAX_ACTORS            22', '#define MAX_ACTORS            21'),
        ('#define MAX_ACTORS_ACTIVE     12', '#define MAX_ACTORS_ACTIVE     13'),
        ('extern actor_t actors[MAX_ACTORS];', 'extern actor_t actors[21];'),
        ('void player_init(void) BANKED;', 'void player_init(void) NONBANKED;'),
        ('Copyright (c) 2020 Toxa', 'Copyright removed'),
    ):
        assert source.count(before) == 1
        try:
            check_header(source.replace(before, after))
        except AssertionError:
            continue
        raise AssertionError('Actor header mutation admitted: ' + before)
    print('Native actor header: exact stock ABI retained, pool22/active12; five negative mutations rejected.')


if __name__ == '__main__':
    main()
