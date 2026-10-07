"""Check the story's coherence against the compiled campaign.

- Every contract has a briefing, a pickup and a delivery call; every speaker
  is defined.
- Places: a client who speaks at the pickup is at the pickup stop; a client
  who speaks on delivery is on the route or is the one who called it in.
- Order: the story is told in the order the game guarantees. Before a
  contract's briefing the player has heard the welcome, every count beat its
  unlock implies, the opening of its chapter and the chapters before it, and
  every contract (with the beat that follows it) on its `after` chain. A
  fact (Dev, Dev at the depot, Vance, Lakelight, the barge, the storm, the
  leaked bid, Margo's van, the vote) may only be mentioned once a call that
  establishes it has been heard; `needs` names further calls.
- Routes: multi-drop contracts visit their drop-offs in the shortest order
  (order_drops in create_campaign.py) unless authored as fixed.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# Facts and the calls that establish them (scripts or contract ids).
FACTS = {
    'DEV': ('BEAT_RIVAL',),          # Dev of Rushly first appears
    'DEVD': ('CHAPTER_7',),          # Dev quits Rushly and joins the depot
    'VANCE': ('CHAPTER_5',),         # Dev names his boss
    'LAKELIGHT': ('CHAPTER_2',),     # the city tenders the festival
    'FESTIVAL': ('CHAPTER_2',),
    'BARGE': ('CHAPTER_4',),
    'STORM': ('contract-35',),
    'LEAK': ('BEAT_COPY',),
    "MARGO'S VAN": ('contract-27',),  # it dies on the viaduct
    'VOTED': ('BEAT_AWARD',),
}
# Calls that can play at any time: they establish nothing and assume nothing.
AMBIENT = ('FAIL_', 'CHATTER_', 'WANTED', 'LOST', 'BUSTED', 'WASTED', 'NIGHT', 'MORNING')


def speaker_of(text, speakers, current):
    head, sep, _ = text.partition(': ')
    return head if sep and head in speakers else current


def main():
    story = json.loads((ROOT / 'content/story.json').read_text())
    radio = json.loads((ROOT / 'content/radio.json').read_text())
    campaign = json.loads((ROOT / 'content/campaign.json').read_text())
    speakers = story['speakers']
    quests = campaign['quests']
    calls = list(story['contracts']) + [story['district_contracts'][q['id']] for q in quests[72:]]
    assert len(calls) == len(quests)
    for name, texts in radio['scripts'].items():
        for text in texts:
            head, sep, _ = text.partition(': ')
            assert not sep or head in speakers or not head.isalpha(), (name, head)
    ids = [q['id'] for q in quests]
    chapter_count = len(story['chapters'])
    # The compiled campaign carries the story order authored here.
    for index, contract in enumerate(story['contracts']):
        after = contract.get('after', story['chapters'][index // 8 - 1].get('spine') if index >= 8 else None)
        assert quests[index].get('after') == (None if after is None else ids[after - 1]), \
            f"{ids[index]}: campaign.json is stale; run scripts/create_campaign.py"
    assert all(q.get('after') is None for q in quests[72:])
    counted = {b['at']: b['script'] for b in radio['beats'] if 'at' in b}
    follows = {ids[b['after'] - 1]: b['script'] for b in radio['beats'] if 'after' in b}
    spines = {k + 1: ids[c['spine'] - 1] for k, c in enumerate(story['chapters'][:-1])}
    by_id = dict(zip(ids, calls))
    checked = 0
    for quest, contract in zip(quests, calls):
        label = f"{quest['id']} {quest['title']}"
        parts = {}
        for part in ('call', 'pick', 'done'):
            assert contract.get(part), f'{label}: missing {part} call'
            current, who = radio['default_speaker'], []
            for text in contract[part]:
                head, sep, _ = text.partition(': ')
                assert not sep or head in speakers or not head.replace(' ', '').isalpha(), (label, head)
                current = speaker_of(text, speakers, current)
                who.append(current)
            parts[part] = who
        route = quest['route']
        caller = parts['call'][0]
        pick = parts['pick'][0]
        if 'stop' in speakers[pick]:
            assert speakers[pick]['stop'] == route[0], f'{label}: {pick} speaks at a pickup that is not theirs'
        for who in parts['done']:
            if 'stop' in speakers[who]:
                assert speakers[who]['stop'] in route or who == caller, f'{label}: {who} is not on the route'
        checked += 1

    def chain(qid):
        out = []
        while (after := quests[ids.index(qid)].get('after')) is not None:
            out.append(after)
            qid = after
        return out

    def heard_before(qid):
        """Calls certainly heard before this contract's briefing."""
        quest = quests[ids.index(qid)]
        heard = {'INTRO'} | {s for at, s in counted.items() if at <= quest['min_completed']}
        index = ids.index(qid)
        if index < 72:
            heard |= {f'CHAPTER_{k}' for k in range(1, index // 8 + 1)}
        for done in chain(qid):
            heard |= {done} | ({follows[done]} if done in follows else set())
        return heard

    def text_of(unit):
        if unit in radio['scripts']:
            lines, who = radio['scripts'][unit], set()
        else:
            c = by_id[unit]
            lines = c['call'] + c['pick'] + c['done'] + c.get('brief', [])
            who = set()
        current = radio['default_speaker']
        for line in lines:
            current = speaker_of(line, speakers, current)
            who.add(current)
        return ' '.join(lines).replace('|', ' '), who

    def assumes(unit):
        text, who = text_of(unit)
        found = {fact for fact in FACTS if re.search(r'\b' + fact + r'\b', text) or fact in who}
        if 'DEVD' in found:
            found.add('DEV')
        return found

    def check(unit, heard):
        for fact in assumes(unit):
            assert unit in FACTS[fact] or heard & set(FACTS[fact]), \
                f'{unit} mentions {fact} before {" or ".join(FACTS[fact])} can have been heard'
        if unit in by_id:
            for need in by_id[unit].get('needs', []):
                assert need in heard, f'{unit} needs {need} first'

    for qid in ids:
        check(qid, heard_before(qid))
    for qid, script in follows.items():
        check(script, heard_before(qid) | {qid})
    for k, spine in spines.items():
        # A chapter opens at the earliest with the delivery of its spine.
        heard = heard_before(spine) | {spine, f'CHAPTER_{k - 1}'} | ({follows[spine]} if spine in follows else set())
        check(f'CHAPTER_{k}', heard)
        assert quests[k * 8]['after'] == spine
    for at, script in counted.items():
        check(script, {'INTRO'} | {s for a, s in counted.items() if a < at})
    for name in radio['scripts']:
        if name.startswith(AMBIENT):
            check(name, {'INTRO'})
    told = set(counted.values()) | set(follows.values()) | {f'CHAPTER_{k}' for k in range(1, chapter_count)}
    unused = [n for n in radio['scripts'] if n.startswith(('BEAT_', 'CHAPTER_')) and n not in told]
    assert not unused, f'story calls never played: {unused}'
    # Shortest drop order for automatic routes.
    import create_campaign as C
    distances = C.shortest_routes([(s['u'], s['v'], s['name'], s['transit']) for s in campaign['stops'][:27]])
    for index, contract in enumerate(story['contracts']):
        if contract.get('order') == 'fixed' or len(contract['drops']) < 2:
            continue
        kind = index % 8
        assert quests[index]['route'] == C.contract_route(contract, kind, distances), quests[index]['id']
    # Dialogue that names stops in order matches the route order.
    names = {'MARKET': 1, 'MILL ST': 2, 'CITY HALL': 3, 'QUEEN WEST': 4, 'KENSINGTON': 7, 'DUFFERIN': 8,
             'BAYFRONT': 11, 'TOWER': 23}
    for index, contract in enumerate(story['contracts']):
        route = quests[index]['route']
        for text in contract['call'] + contract['pick']:
            flat = text.replace('|', ' ')
            if flat.count(',') < 2:
                continue
            seen = [names[n] for n in sorted(names, key=lambda n: flat.find(n)) if flat.find(n) >= 0]
            positions = [route.index(s, 1) for s in seen if s in route[1:]]
            assert positions == sorted(positions), f"{quests[index]['id']}: '{flat}' lists stops out of route order {route}"
    print(f'Story coherence: {checked} contracts have briefing, pickup and delivery calls; clients, story order and route order checked.')


if __name__ == '__main__':
    main()
