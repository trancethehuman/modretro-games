#!/usr/bin/env python3
"""Compile Toronto Dispatch's original soundtrack into GBVM hUGE/SFX C data.

Sources (project/original-audio/):
- soundtrack.json: instrument palette, wave shapes, drum kit, event cues,
  braking, engine pitches and the list of songs.
- songs/<slug>.json: one score per song, written in four-bar sections.

Outputs (project/plugins/toronto-driving/engine/src/):
- td_song_<slug>.c: one complete hUGE song per file. Each file is autobanked
  (#pragma bank 255), so a song's patterns, orders, instruments and waves
  share one ROM bank, which GBVM's music interrupt switches to itself.
- the marked data block in td_audio.c: the song bank/address table, event
  cues, the engine drone (effects-only mode) and engine revs (music mode).

`--check` verifies every output is current and that no stale song file is
left. Runtime integration and upstream engine files are never changed here.
"""
from pathlib import Path
import json
import math
import re
import sys

GAME = Path(__file__).resolve().parents[1]
AUDIO = GAME / "project/original-audio"
SOUNDTRACK = AUDIO / "soundtrack.json"
ENGINE_SRC = GAME / "project/plugins/toronto-driving/engine/src"
TARGET = ENGINE_SRC / "td_audio.c"
BEGIN = "/* BEGIN GENERATED AUDIO: scripts/create_audio.py */"
END = "/* END GENERATED AUDIO */"
CUES = ("impact", "pickup", "complete", "transit", "fail", "menu")
VEHICLES = ("car", "truck", "motorcycle", "scooter")
# td_audio.c selects these songs by game state; every one must exist.
ROLES = ("title", "day", "night", "chase")
# Score channel -> (instrument type, hUGE order/hardware channel). The lead
# owns CH2, which no effect uses; the counter-line shares CH1 with event cues
# and engine revs, and the drums share CH4 with braking and impacts.
CHANNELS = (("counter", "duty"), ("lead", "duty"), ("bass", "wave"), ("drums", "noise"))
ROWS_PER_BAR = 16
BARS_PER_SECTION = 4
PATTERN_ROWS = ROWS_PER_BAR * BARS_PER_SECTION  # hUGE patterns are 64 rows
NO_NOTE = 90
MAX_SONG_BYTES = 15 * 1024  # one 16 KiB bank, minus a margin for the bank stub
SEMITONES = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
GBDK_NAMES = ("C_", "Cs", "D_", "Ds", "E_", "F_", "Fs", "G_", "Gs", "A_", "As", "B_")
TOKEN = re.compile(r"(?P<body>\.|!|[A-G][sb]?[3-8]|[a-z])"
                   r"(?::(?P<fx>[0-9A-F]{3})(?P<hold>\+)?)?(?:/(?P<rows>[1-9][0-9]*))?")
DIRECTIVE = re.compile(r"<([a-z][a-z0-9_]*)>")
# Effects a score may use. Master volume (5), panning (8) and routines (6)
# would fight GBVM's own NR50/NR51 use or need routines; position jump (B) and
# pattern break (D) are placed by this generator only.
EFFECTS = {0x0: "arpeggio", 0x1: "portamento up", 0x2: "portamento down",
           0x3: "tone portamento", 0x4: "vibrato", 0x7: "note delay",
           0x9: "duty", 0xA: "volume slide", 0xC: "volume", 0xE: "note cut"}


def note_index(note):
    """hUGE note number: C3 is 0. Pulse channels sound an octave lower than
    the name (C5 is middle C) and the wave channel two octaves lower."""
    match = re.fullmatch(r"([A-G])([sb]?)([3-8])", note)
    if not match:
        raise ValueError(f"Invalid hUGE note: {note}")
    index = (int(match[3]) - 3) * 12 + SEMITONES[match[1]]
    index += {"": 0, "s": 1, "b": -1}[match[2]]
    if not 0 <= index < 72:
        raise ValueError(f"Note outside hUGE's C3..B8 table: {note}")
    return index


def gbdk_note(index):
    return "___" if index == NO_NOTE else f"{GBDK_NAMES[index % 12]}{index // 12 + 3}"


def nibble(value, name, low=0, high=15):
    if not isinstance(value, int) or not low <= value <= high:
        raise ValueError(f"{name} must be an integer {low}..{high}")
    return value


# ----------------------------------------------------------------- effects

def pulse_period(step):
    if "note" in step:
        # Effect pitches keep the original convention: C3 is 130.81 Hz.
        hz = 130.81278265 * math.pow(2, note_index(step["note"]) / 12)
    else:
        hz = step["hz"]
    if not 64 <= hz <= 8000:
        raise ValueError("Pulse frequency is outside authored bounds")
    return min(2047, max(0, round(2048 - 131072 / hz)))


def instruction(channel, step, fade):
    volume = nibble(step["volume"], "Envelope volume")
    fade = nibble(step.get("fade", fade), "Envelope fade", 0, 7)
    envelope = (volume << 4) | fade
    if channel == "pulse1":
        period = pulse_period(step)
        return [0xF8, 0x00, 0x40, envelope, period & 0xFF, 0x80 | (period >> 8)]
    polynomial = step["polynomial"]
    if not 0 <= polynomial <= 255:
        raise ValueError("Noise polynomial must be a byte")
    return [0x7B, 0x00, envelope, polynomial, 0x80]


def sfx_bytes(effect, fade=1):
    """GBVM finite SFX stream; `fade` is the default envelope pace."""
    channels = [channel for channel in ("pulse1", "noise4") if channel in effect]
    if not channels:
        raise ValueError("An effect must have an authored channel")
    counts = {len(effect[channel]) for channel in channels}
    if len(counts) != 1:
        raise ValueError("Combined-channel effects must have aligned segments")
    result = []
    for index in range(counts.pop()):
        durations = {effect[channel][index]["frames"] for channel in channels}
        if len(durations) != 1:
            raise ValueError("Combined-channel segment durations must match")
        frames = durations.pop()
        if not isinstance(frames, int) or not 1 <= frames <= 120:
            raise ValueError("SFX duration must be 1..120 video frames")
        ticks = frames * 4  # Same tick conversion as pinned GB Studio basic SFX.
        first = True
        while ticks:
            duration = min(ticks, 16)
            result.append(((duration - 1) << 4) | (len(channels) if first else 0))
            if first:
                for channel in channels:
                    result.extend(instruction(channel, effect[channel][index], fade))
            first = False
            ticks -= duration
    # Explicitly silence every channel touched before the native terminator.
    for channel in channels:
        result.extend([0x01, 0x28 if channel == "pulse1" else 0x2B, 0x00, 0xC0])
    result.extend([0x01, 0x07])
    return result


def c_bytes(name, data):
    text = f"static const UBYTE {name}[] = {{\n"
    for start in range(0, len(data), 12):
        text += "    " + ", ".join(f"0x{byte:02X}" for byte in data[start:start + 12]) + ",\n"
    return text + "};\n"


def engine_effects(engine):
    """Finite engine samples: a steady drone for effects-only mode and a short
    rising rev for music mode (td_audio.c plays it on launches/upshifts)."""
    out = ""
    for kind in ("drone", "rev"):
        for vehicle in VEHICLES:
            pitches = engine["hz"][vehicle]
            if len(pitches) != 4:
                raise ValueError("Every road vehicle needs four engine speed stages")
            trim = engine.get("volume_trim", {}).get(vehicle, 0)
            for stage, hz in enumerate(pitches):
                if kind == "drone":
                    drone = engine["drone"]
                    steps = [{"hz": hz, "frames": drone["frames"],
                              "volume": max(1, drone["volume"] + trim), "fade": 0}]
                else:
                    steps = [{"hz": round(hz * part["scale"]), "frames": part["frames"],
                              "volume": max(1, part["volume"] + trim), "fade": part["fade"]}
                             for part in engine["rev"]]
                out += c_bytes(f"td_audio_{kind}_{vehicle}_{stage}", sfx_bytes({"pulse1": steps}, 0))
        out += f"static const UBYTE * const td_audio_{kind}s[4][4] = {{\n"
        out += "".join("    {" + ", ".join(f"td_audio_{kind}_{vehicle}_{stage}" for stage in range(4)) + "},\n"
                       for vehicle in VEHICLES) + "};\n"
    return out


# ------------------------------------------------------------------- songs

def duty_instrument(spec):
    duty = nibble(spec["duty"], "Duty", 0, 3)
    length = spec.get("length")
    len_duty = duty << 6
    if length is not None:
        len_duty |= (64 - nibble(length, "Pulse length", 1, 64)) & 0x3F
    envelope = (nibble(spec["volume"], "Volume") << 4) | (8 if spec.get("rise") else 0)
    envelope |= nibble(spec.get("fade", 0), "Fade", 0, 7)
    sweep = nibble(spec.get("sweep", 0), "Sweep", 0, 0x7F)
    return [sweep, len_duty, envelope, "NULL", 0x80 | (0x40 if length is not None else 0)]


def wave_instrument(spec, wave_index):
    length = spec.get("length")
    level = nibble(spec["level"], "Wave level (1 full, 2 half, 3 quarter)", 1, 3)
    raw_length = 0 if length is None else (256 - nibble(length, "Wave length", 1, 256)) & 0xFF
    return [raw_length, level << 5, wave_index, "NULL", 0x80 | (0x40 if length is not None else 0)]


def noise_instrument(spec, subpattern):
    envelope = (nibble(spec["volume"], "Volume") << 4) | (8 if spec.get("rise") else 0)
    envelope |= nibble(spec.get("fade", 0), "Fade", 0, 7)
    length = spec.get("length")
    highmask = 0 if length is None else ((64 - nibble(length, "Noise length", 1, 64)) & 0x3F) | 0x40
    if spec.get("periodic"):
        highmask |= 0x80  # 7-bit LFSR: metallic, pitched noise
    return [envelope, subpattern or "NULL", highmask, 0, 0]


def macro_rows(macro):
    """Noise pitch macro as a hUGE subpattern: row 0 is the struck note, rows
    1..n offset its pitch on the following ticks and the last row holds."""
    if not 1 <= len(macro) <= 6:
        raise ValueError("Noise macros have 1..6 tick offsets")
    rows = [(NO_NOTE, 0, 0)]
    for tick, offset in enumerate(macro, 1):
        if not -36 <= offset <= 35:
            raise ValueError("Noise macro offsets must be -36..35 semitones")
        jump = tick + 1 if tick == len(macro) else 0  # stay on the last row
        rows.append((offset + 36, jump, 0))
    return rows + [(NO_NOTE, 0, 0)] * (32 - len(rows))


class Song:
    def __init__(self, slug, score, soundtrack):
        self.slug, self.score, self.palette = slug, score, soundtrack
        self.tempo = score["ticks_per_row"]
        if not isinstance(self.tempo, int) or not 4 <= self.tempo <= 16:
            raise ValueError(f"{slug}: ticks_per_row must be 4..16")
        self.kit = dict(soundtrack["drums"])
        self.kit.update(score.get("drums", {}))
        self.used = {"duty": [], "wave": [], "noise": []}
        self.parsed = {}
        self.order = score["order"]
        unknown = set(self.order) - set(score["sections"])
        unused = set(score["sections"]) - set(self.order)
        if unknown or unused or not self.order:
            raise ValueError(f"{slug}: order uses {sorted(unknown)}; unused sections {sorted(unused)}")
        self.loop = score.get("loop", 0)
        if not isinstance(self.loop, int) or not 0 <= self.loop < len(self.order):
            raise ValueError(f"{slug}: loop must index the order")

    def instrument(self, kind, name):
        if name not in self.palette[kind]:
            raise ValueError(f"{self.slug}: unknown {kind} instrument {name}")
        if name not in self.used[kind]:
            self.used[kind].append(name)
            if len(self.used[kind]) > 15:
                raise ValueError(f"{self.slug}: more than 15 {kind} instruments")
        return self.used[kind].index(name) + 1

    def channel(self, section, name, kind, stack=()):
        key = (section, name)
        if key in self.parsed:
            return self.parsed[key]
        if key in stack:
            raise ValueError(f"{self.slug}: circular reference at {section}.{name}")
        spec = self.score["sections"][section].get(name)
        if isinstance(spec, str):
            if not spec.startswith("@") or spec[1:] not in self.score["sections"]:
                raise ValueError(f"{self.slug}: {section}.{name} must be bars or @section")
            cells = self.channel(spec[1:], name, kind, stack + (key,))
        else:
            cells = self.parse_bars(section, name, kind, spec or ["./16"])
        self.parsed[key] = cells
        return cells

    def parse_bars(self, section, name, kind, bars):
        where = f"{self.slug}: {section}.{name}"
        if not isinstance(bars, list) or BARS_PER_SECTION % len(bars):
            raise ValueError(f"{where}: give 1, 2 or 4 bars")
        current = None if kind == "noise" else self.score["defaults"][name]
        cells, held = [], None
        for bar_number, bar in enumerate(bars * (BARS_PER_SECTION // len(bars)), 1):
            start = len(cells)
            for token in bar.split():
                directive = DIRECTIVE.fullmatch(token)
                if directive:
                    if kind == "noise":
                        raise ValueError(f"{where}: drums choose instruments by letter")
                    current = directive[1]
                    continue
                match = TOKEN.fullmatch(token)
                if not match:
                    raise ValueError(f"{where} bar {bar_number}: bad token {token!r}")
                cells.extend(self.cells(where, kind, current, match, held))
                body, fx, hold = match["body"], match["fx"], match["hold"]
                if body != "." or fx:
                    held = int(fx, 16) if hold else None
            if len(cells) - start != ROWS_PER_BAR:
                raise ValueError(f"{where} bar {bar_number}: {len(cells) - start} rows, not 16")
        return cells

    def cells(self, where, kind, current, match, held):
        body, rows = match["body"], int(match["rows"] or 1)
        fx = int(match["fx"], 16) if match["fx"] else None
        if fx is not None:
            self.check_effect(where, kind, fx, body)
        if body == "!":
            if fx is not None:
                raise ValueError(f"{where}: a cut cannot take another effect")
            first = (NO_NOTE, 0, 0xE00)
        elif body == ".":
            first = (NO_NOTE, 0, fx if fx is not None else (held or 0))
        elif kind == "noise":
            if body not in self.kit:
                raise ValueError(f"{where}: unknown drum {body!r}")
            instrument, note = self.kit[body]
            index = note_index(note)
            spec = self.palette["noise"][instrument]
            if not all(0 <= index + offset < 64 for offset in [0] + spec.get("macro", [])):
                raise ValueError(f"{where}: drum {body!r} leaves the noise range")
            first = (index, self.instrument("noise", instrument), fx or 0)
        else:
            if body.islower():
                raise ValueError(f"{where}: drum letter {body!r} on a tonal channel")
            first = (note_index(body), self.instrument(kind, current), fx or 0)
        follow = fx if match["hold"] else (held if body == "." and fx is None else 0)
        return [first] + [(NO_NOTE, 0, follow or 0)] * (rows - 1)

    def check_effect(self, where, kind, fx, body):
        code, param = fx >> 8, fx & 0xFF
        if code not in EFFECTS or fx == 0:
            raise ValueError(f"{where}: effect {fx:03X} is not allowed")
        if code == 0x9 and kind != "duty":
            raise ValueError(f"{where}: duty effects belong to pulse channels")
        if code == 0x3 and (body in ".!" or kind == "noise"):
            raise ValueError(f"{where}: tone portamento needs a target note")
        if code == 0xC and param > 15:
            raise ValueError(f"{where}: volume is 0..15")
        if code in (0x7, 0xE) and param >= self.tempo:
            raise ValueError(f"{where}: delay/cut tick must be within the row")

    def build(self):
        grid = []  # [order][channel] -> 64 cells
        for section in self.order:
            grid.append([list(self.channel(section, name, kind)) for name, kind in CHANNELS])
        if self.loop:
            # Global position jump on the final row; hUGE's parameter is order + 1.
            last = grid[-1]
            free = [cells for cells in last if cells[-1][2] == 0]
            if not free:
                raise ValueError(f"{self.slug}: no free effect on the final row for the loop")
            note, instrument, _ = free[0][-1]
            free[0][-1] = (note, instrument, 0xB00 | (self.loop + 1))
        self.patterns, self.orders = [], [[] for _ in CHANNELS]
        for row in grid:
            for channel, cells in enumerate(row):
                key = tuple(cells)
                if key not in self.patterns:
                    self.patterns.append(key)
                self.orders[channel].append(self.patterns.index(key))
        self.subpatterns = {}
        for name in self.used["noise"]:
            macro = self.palette["noise"][name].get("macro")
            if macro:
                self.subpatterns[name] = macro_rows(macro)
        self.waves = []
        for name in self.used["wave"]:
            wave = self.palette["wave"][name]["wave"]
            if wave not in self.palette["waves"]:
                raise ValueError(f"{self.slug}: unknown wave {wave}")
            if wave not in self.waves:
                self.waves.append(wave)
        self.size = (len(self.patterns) * PATTERN_ROWS * 3 + len(self.subpatterns) * 96 +
                     len(self.order) * 4 * 2 + 6 * sum(map(len, self.used.values())) +
                     16 * len(self.waves) + 22)
        if self.size > MAX_SONG_BYTES:
            raise ValueError(f"{self.slug}: {self.size} bytes exceeds one ROM bank")

    def bpm(self):
        return round(64 * 60 / (self.tempo * 4))  # GBVM ticks hUGE at 64 Hz

    def source(self, json_name):
        score = self.score
        bars = len(self.order) * BARS_PER_SECTION
        out = (f"/* Generated by scripts/create_audio.py from project/original-audio/{json_name}; do not edit.\n"
               f"   \"{score['title']}\" ({self.slug}): {score['mood']}\n"
               f"   {bars} bars in {len(self.order)} four-bar orders, {self.tempo} ticks per row "
               f"(about {self.bpm()} BPM), loops to order {self.loop}.\n"
               f"   Original music for Toronto Dispatch, MIT licence, copyright {self.palette['copyright']}. */\n")
        out += "#pragma bank 255\n#include <gbdk/platform.h>\n#include <stddef.h>\n#include \"hUGEDriver.h\"\n\n"
        out += f"BANKREF(td_song_{self.slug})\n\n"
        out += f"static const unsigned char order_cnt = {len(self.order) * 2};\n"
        for index, cells in enumerate(self.patterns):
            out += f"static const unsigned char pattern_{index}[] = {{\n"
            for row in range(0, PATTERN_ROWS, 4):
                out += "    " + " ".join(f"DN({gbdk_note(n)}, {i}, 0x{f:03X})," for n, i, f in cells[row:row + 4]) + "\n"
            out += "};\n"
        for name, rows in self.subpatterns.items():
            out += f"static const unsigned char macro_{name}[] = {{\n"
            for row in range(0, 32, 4):
                out += "    " + " ".join(f"DN({gbdk_note(n)}, {j}, 0x{f:03X})," for n, j, f in rows[row:row + 4]) + "\n"
            out += "};\n"
        for channel, (name, _) in enumerate(CHANNELS):
            pointers = ", ".join(f"pattern_{index}" for index in self.orders[channel])
            out += f"static const unsigned char * const order{channel + 1}[] = {{{pointers}}}; /* {name} */\n"

        def table(ctype, name, rows):
            text = f"static const {ctype} {name}[] = {{\n"
            for label, fields in rows:
                values = ", ".join(field if isinstance(field, str) else f"0x{field:02X}" for field in fields)
                text += f"    {{{values}}}, /* {label} */\n"
            return text + "};\n"

        out += table("hUGEDutyInstr_t", "duty_instruments",
                     [(name, duty_instrument(self.palette["duty"][name])) for name in self.used["duty"]])
        out += table("hUGEWaveInstr_t", "wave_instruments",
                     [(name, wave_instrument(self.palette["wave"][name], self.waves.index(self.palette["wave"][name]["wave"])))
                      for name in self.used["wave"]])
        out += table("hUGENoiseInstr_t", "noise_instruments",
                     [(name, noise_instrument(self.palette["noise"][name],
                                              f"macro_{name}" if name in self.subpatterns else None))
                      for name in self.used["noise"]])
        out += "static const unsigned char waves[] = {\n"
        for wave in self.waves:
            samples = self.palette["waves"][wave]
            packed = [(samples[i] << 4) | samples[i + 1] for i in range(0, 32, 2)]
            out += "    " + ", ".join(f"0x{byte:02X}" for byte in packed) + f", /* {wave} */\n"
        out += "};\n"
        out += f"const hUGESong_t td_song_{self.slug} = {{\n"
        out += f"    {self.tempo}, &order_cnt,\n"
        out += "    (const unsigned char **)order1, (const unsigned char **)order2,\n"
        out += "    (const unsigned char **)order3, (const unsigned char **)order4,\n"
        out += "    duty_instruments, wave_instruments, noise_instruments,\n    NULL, waves\n};\n"
        return out


def check_palette(soundtrack):
    for name, samples in soundtrack["waves"].items():
        if len(samples) != 32 or any(not isinstance(s, int) or not 0 <= s <= 15 for s in samples):
            raise ValueError(f"Wave {name} must have 32 four-bit samples")
    for kind in ("duty", "wave", "noise"):
        for name in soundtrack[kind]:
            if not re.fullmatch(r"[a-z][a-z0-9_]*", name):
                raise ValueError(f"Instrument name {name} must be a C identifier part")
    for letter, (instrument, note) in soundtrack["drums"].items():
        if not re.fullmatch(r"[a-z]", letter) or instrument not in soundtrack["noise"]:
            raise ValueError(f"Drum {letter} must be one letter naming a noise instrument")
        note_index(note)


def generate(soundtrack):
    """Returns {path: text} for every generated song file and the td_audio.c block."""
    check_palette(soundtrack)
    songs, outputs = [], {}
    slugs = [entry["slug"] for entry in soundtrack["songs"]]
    if sorted(slugs) != sorted(ROLES):
        raise ValueError(f"The soundtrack must define exactly the songs {ROLES}")
    for entry in soundtrack["songs"]:
        slug = entry["slug"]
        json_name = f"songs/{slug}.json"
        score = json.loads((AUDIO / json_name).read_text())
        if score.get("slug") != slug:
            raise ValueError(f"{json_name} must declare slug {slug}")
        song = Song(slug, score, soundtrack)
        song.build()
        songs.append(song)
        outputs[ENGINE_SRC / f"td_song_{slug}.c"] = song.source(json_name)
    out = "/* Original soundtrack and effects. Edit project/original-audio/ and run\n"
    out += "   scripts/create_audio.py; each song is its own autobanked td_song_*.c. */\n"
    for index, song in enumerate(songs):
        out += f"#define TD_SONG_{song.slug.upper()} {index} /* {song.score['title']}: {len(song.order) * 4} bars, about {song.bpm()} BPM, {song.size} bytes */\n"
    out += f"#define TD_SONGS {len(songs)}\n"
    out += "".join(f"BANKREF_EXTERN(td_song_{song.slug})\nextern const hUGESong_t td_song_{song.slug};\n" for song in songs)
    # SDCC accepts a bank reference as a constant only inside a far pointer.
    out += "static const far_ptr_t td_audio_songs[TD_SONGS] = {" + ", ".join(f"TO_FAR_PTR_T(td_song_{song.slug})" for song in songs) + "};\n"
    effects = soundtrack["effects"]
    for cue in (*CUES, "brake"):
        effect = effects[cue]
        if effect["priority"] not in range(9):
            raise ValueError("Native effect priorities must be 0..8")
        out += c_bytes(f"td_audio_{cue}", sfx_bytes(effect))
    out += "static const UBYTE * const td_audio_cues[] = {" + ", ".join(f"td_audio_{cue}" for cue in CUES) + "};\n"
    out += "static const UBYTE td_audio_cue_priorities[] = {" + ", ".join(str(effects[cue]["priority"]) for cue in CUES) + "};\n"
    out += "static const UBYTE td_audio_cue_masks[] = {" + ", ".join(str((1 if "pulse1" in effects[cue] else 0) | (8 if "noise4" in effects[cue] else 0)) for cue in CUES) + "};\n"
    out += engine_effects(soundtrack["engine"])
    return songs, outputs, out


def main():
    soundtrack = json.loads(SOUNDTRACK.read_text())
    songs, outputs, block = generate(soundtrack)
    runtime = TARGET.read_text()
    if runtime.count(BEGIN) != 1 or runtime.count(END) != 1:
        raise ValueError("Runtime must contain exactly one marked audio data block")
    prefix, remainder = runtime.split(BEGIN)
    _, suffix = remainder.split(END)
    outputs[TARGET] = prefix + BEGIN + "\n" + block + END + suffix
    stale = sorted(set(ENGINE_SRC.glob("td_song_*.c")) - set(outputs))
    summary = ", ".join(f"{song.slug} {len(song.order) * 4} bars/{song.size} B" for song in songs)
    if "--check" in sys.argv:
        changed = [path.name for path, text in outputs.items() if not path.exists() or path.read_text() != text]
        if changed or stale:
            raise SystemExit("Original audio differs from the editable scores; run create_audio.py "
                             f"(changed: {changed}, stale: {[path.name for path in stale]})")
        print(f"Original audio matches editable scores ({summary}) and native hUGE/SFX formats.")
    else:
        for path, text in outputs.items():
            if not path.exists() or path.read_text() != text:
                path.write_text(text)
        for path in stale:
            path.unlink()
        print(f"Original audio: {summary}; 7 cues, 16 engine drones and 16 engine revs generated.")


if __name__ == "__main__":
    main()
