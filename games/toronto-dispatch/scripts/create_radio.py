"""Generate the radio calls from content/radio.json and content/story.json.

Every call is a run of pages; every page is one authored line that fills at
most three 17-column lines on the card (word-wrapped here, '|' forces a break)
and is said by one speaker. A page may start with
"NAME: " to change speaker; otherwise the previous page's speaker carries on
(scripts start with the default speaker).

Writes:
  engine/include/td_radio_data.h      script ids, speaker cards, call tables
  engine/src/td_radio_text<N>.c       page text and speakers, one ROM bank each
The engine plays three calls per contract (story.json): the briefing on
accepting it, the client at the first pickup and the delivery. A first
delivery may bring more: the story beat that follows that contract
(radio.json "beats" with "after"), a delivery-count beat ("at"), the opening
call of each chapter it unlocks (CHAPTER_<k>) and, after the last contract,
MASTER. `--check` verifies outputs without writing.
"""
import json
import sys
from pathlib import Path
import ui_art as A

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / 'project/plugins/toronto-driving/engine'
COLS, LINES = 17, 3
PAGES_PER_BANK = 290          # 290 x 52 bytes = 15.1 KB of each 16 KB bank
# Scripts the engine indexes as a group, in engine order.
GROUPS = {'FAIL': 3, 'CHATTER': 16}
# Calls a new call may cut short (ambient, not story).
INTERRUPTIBLE = ('NIGHT', 'MORNING')
CONTRACT_PARTS = ('call', 'pick', 'done')


def _greedy(parts):
    lines = []
    for part in parts:
        line = ''
        for word in part.split():
            assert len(word) <= COLS, word
            candidate = f'{line} {word}' if line else word
            if len(candidate) <= COLS:
                line = candidate
            else:
                lines.append(line)
                line = word
        lines.append(line)
    return lines


def wrap(text):
    """One card: three 17-column lines. The authored '|' breaks are kept when
    they fit; otherwise the words are packed greedily."""
    lines = _greedy(text.split('|'))
    if len(lines) > LINES:
        lines = _greedy([text.replace('|', ' ')])
    assert len(lines) <= LINES, (text, 'does not fit one card')
    return ''.join(l.ljust(COLS) for l in lines).ljust(COLS * LINES)


def c_string(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def build():
    data = json.loads((ROOT / 'content/radio.json').read_text())
    story = json.loads((ROOT / 'content/story.json').read_text())
    campaign = json.loads((ROOT / 'content/campaign.json').read_text())
    allowed = set(A.FONT_CHARS)
    speakers = list(story['speakers'])
    for key, s in story['speakers'].items():
        assert len(s['card']) <= COLS and set(s['card']) <= allowed, key
        assert s['portrait'] in ('rosa', 'caller'), key
    default = data['default_speaker']

    pages, page_speaker = [], []

    def add_script(name, texts):
        assert texts, name
        speaker = default
        for text in texts:
            head, sep, rest = text.partition(': ')
            if sep and head in story['speakers']:
                speaker, text = head, rest
            assert set(text) - {'|'} <= allowed, (name, set(text) - allowed)
            pages.append(wrap(text))
            page_speaker.append(speakers.index(speaker))

    # Chapter short names follow the campaign's chapters of eight contracts.
    chapters = []
    for quest in campaign['quests']:
        if quest['chapter'] not in chapters:
            chapters.append(quest['chapter'])
    short = data['chapter_short_names']
    assert len(short) == len(chapters) == (len(campaign['quests']) + 7) // 8, (len(short), len(chapters))
    for i, quest in enumerate(campaign['quests']):
        assert chapters.index(quest['chapter']) == i // 8, 'chapters are groups of eight contracts'
    for name in short:
        assert len(name) <= 16 and set(name) <= allowed, name

    scripts = data['scripts']
    grouped = {f'{g}_{k}' for g, n in GROUPS.items() for k in range(n)}
    assert grouped <= set(scripts), sorted(grouped - set(scripts))
    order = [n for n in scripts if n not in grouped]
    for group, count in GROUPS.items():
        order += [f'{group}_{k}' for k in range(count)]
    assert sorted(order) == sorted(scripts)
    first = []
    for name in order:
        first.append(len(pages))
        add_script(name, scripts[name])
    first.append(len(pages))
    # One pseudo script plays contract calls, with its own page range.
    contract_id = len(order)
    assert contract_id + 1 < 255, 'script ids are bytes; 255 means none'

    # Contract calls in campaign order: core contracts from story.json, the
    # district contracts by id.
    quests = campaign['quests']
    core = story['contracts']
    assert len(core) == 72
    contract_first = []
    for index, quest in enumerate(quests):
        source = core[index] if index < 72 else story['district_contracts'][quest['id']]
        if index < 72:
            assert source['title'] == quest['title'], (index, source['title'], quest['title'])
        for part in CONTRACT_PARTS:
            contract_first.append(len(pages))
            add_script(f"{quest['id']}-{part}", source[part])
    contract_first.append(len(pages))
    assert len(contract_first) == 3 * len(quests) + 1

    beats = data['beats']
    counted = [b for b in beats if 'at' in b]
    following = [b for b in beats if 'after' in b]
    assert len(counted) + len(following) == len(beats)
    assert [b['at'] for b in counted] == sorted({b['at'] for b in counted}), 'count beats in order, one per count'
    assert all(0 < b['at'] < len(quests) for b in counted)
    assert len({b['after'] for b in following}) == len(following), 'one beat per contract'
    assert all(1 <= b['after'] <= 72 for b in following)
    assert all(b['script'] in order and b['script'] not in grouped for b in beats)
    # Chapter openings: chapter k opens when its first contract can be taken
    # (create_campaign.py checks that the rest of the chapter needs as much).
    ids = [q['id'] for q in quests]
    story_chapters = len(story['chapters'])
    gates = [(0, 255)] + [(quests[k * 8]['min_completed'], ids.index(quests[k * 8]['after']))
                          for k in range(1, story_chapters)]
    openings = [255] + [order.index(f'CHAPTER_{k}') for k in range(1, story_chapters)]
    assert story_chapters <= 16, 'chapters are a 16-bit mask'
    scripted = {b['script'] for b in beats} | {f'CHAPTER_{k}' for k in range(1, story_chapters)}
    story_scripts = {n for n in order if n.startswith(('BEAT_', 'CHAPTER_'))}
    assert story_scripts <= scripted, sorted(story_scripts - scripted)

    assert len(pages) < 65535
    banks = [(start, min(start + PAGES_PER_BANK, len(pages))) for start in range(0, len(pages), PAGES_PER_BANK)]
    files = {}
    lines = ['/* Generated by scripts/create_radio.py from content/radio.json and',
             ' * content/story.json. Original dialogue. */',
             '#ifndef TD_RADIO_DATA_H', '#define TD_RADIO_DATA_H',
             f'#define TD_RADIO_COLS {COLS}', f'#define TD_RADIO_PAGE {COLS * LINES}',
             f'#define TD_RADIO_ROWS {LINES + 1}  /* the card: speaker row and text rows */',
             f'#define TD_RADIO_SCRIPTS {len(order)}', f'#define TD_RADIO_PAGES {len(pages)}',
             f'#define TD_CHAPTERS {len(short)}', f'#define TD_RADIO_SPEAKERS {len(speakers)}',
             f'#define TD_RADIO_CONTRACT {contract_id}', f'#define TD_RADIO_BEATS {len(counted)}',
             f'#define TD_RADIO_FOLLOWS {len(following)}', f'#define TD_STORY_CHAPTERS {story_chapters}',
             f'#define TD_RADIO_TEXT_BANKS {len(banks)}', f'#define TD_RADIO_BANK_PAGES {PAGES_PER_BANK}']
    for k, name in enumerate(order):
        if name not in grouped:
            lines.append(f'#define TD_RADIO_{name} {k}')
    for group, count in GROUPS.items():
        lines.append(f'#define TD_RADIO_{group} {order.index(f"{group}_0")}')
        lines.append(f'#define TD_RADIO_{group}_COUNT {count}')
    lines.append('#define TD_RADIO_INTERRUPTIBLE(s) (' + '||'.join(
        [f'(s)=={order.index(n)}' for n in INTERRUPTIBLE] +
        [f'((s)>={order.index("CHATTER_0")}&&(s)<{order.index("CHATTER_0") + GROUPS["CHATTER"]})',
         f'(s)=={contract_id}']) + ')')
    for b in range(len(banks)):
        lines.append(f'void td_radio_text{b}(UWORD page,UBYTE from,UBYTE n,char *dest) BANKED;')
        lines.append(f'UBYTE td_radio_speaker{b}(UWORD page) BANKED;')
    lines += ['#ifdef TD_RADIO_DATA',
              f'static const UWORD td_radio_first[{len(first)}]={{' + ','.join(map(str, first)) + '};',
              f'static const UWORD td_contract_first[{len(contract_first)}]={{' + ','.join(map(str, contract_first)) + '};',
              '/* Beats at a delivery count, beats that follow a contract (job index), and',
              ' * each chapter\'s gate (deliveries and the contract before it) and opening. */',
              f'static const UBYTE td_radio_beat_at[{len(counted)}]={{' + ','.join(str(b['at']) for b in counted) + '};',
              f'static const UBYTE td_radio_beat_script[{len(counted)}]={{' + ','.join(str(order.index(b['script'])) for b in counted) + '};',
              f'static const UBYTE td_radio_follow_job[{len(following)}]={{' + ','.join(str(b['after'] - 1) for b in following) + '};',
              f'static const UBYTE td_radio_follow_script[{len(following)}]={{' + ','.join(str(order.index(b['script'])) for b in following) + '};',
              f'static const UBYTE td_chapter_min[{story_chapters}]={{' + ','.join(str(g[0]) for g in gates) + '};',
              f'static const UBYTE td_chapter_after[{story_chapters}]={{' + ','.join(str(g[1]) for g in gates) + '};',
              f'static const UBYTE td_chapter_opening[{story_chapters}]={{' + ','.join(map(str, openings)) + '};',
              f'static const char td_radio_cards[{len(speakers)}][{COLS + 1}]={{' +
              ','.join(c_string(story['speakers'][k]['card']) for k in speakers) + '};',
              f'static const UBYTE td_radio_rosa[{len(speakers)}]={{' +
              ','.join('1' if story['speakers'][k]['portrait'] == 'rosa' else '0' for k in speakers) + '};',
              f'static const char td_chapter_names[{len(short)}][17]={{' + ','.join(c_string(s) for s in short) + '};',
              '#endif', '#endif', '']
    files[ENGINE / 'include/td_radio_data.h'] = '\n'.join(lines)
    for b, (start, end) in enumerate(banks):
        text = ['/* Generated by scripts/create_radio.py: radio page text, one ROM bank. */',
                '#pragma bank 255', '#include <gbdk/platform.h>',
                f'static const UBYTE td_rs{b}[{end - start}]={{' + ','.join(map(str, page_speaker[start:end])) + '};',
                f'/* Exactly {COLS * LINES} characters each: no terminating NUL is stored. */',
                f'static const char td_rt{b}[{end - start}][{COLS * LINES}]={{']
        text += [f'    {c_string(p)},' for p in pages[start:end]]
        text += ['};',
                 f'void td_radio_text{b}(UWORD page,UBYTE from,UBYTE n,char *dest) BANKED {{',
                 f'    const char *s=td_rt{b}[page-{start}]+from;while(n--)*dest++=*s++;',
                 '}',
                 f'UBYTE td_radio_speaker{b}(UWORD page) BANKED {{return td_rs{b}[page-{start}];}}', '']
        files[ENGINE / f'src/td_radio_text{b}.c'] = '\n'.join(text)
    return files, len(order), len(pages), len(banks)


def main():
    files, scripts, pages, banks = build()
    stale = [p for p, text in files.items() if not p.exists() or p.read_text() != text]
    extra = [p for p in (ENGINE / 'src').glob('td_radio_text*.c') if p not in files]
    if '--check' in sys.argv:
        assert not stale and not extra, 'radio lines are stale; run scripts/create_radio.py'
        print(f'Radio lines match content: {scripts} scripts, {pages} pages in {banks} text banks.')
        return
    for path in stale:
        path.write_text(files[path])
    for path in extra:
        path.unlink()
    print(f'Wrote {len(stale)} radio output(s): {scripts} scripts, {pages} pages in {banks} text banks.')


if __name__ == '__main__':
    main()
