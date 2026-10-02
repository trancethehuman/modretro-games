#!/usr/bin/env python3
"""Compile the original City Shift JSON score into GBVM hUGE/SFX C data.

Only the marked data block in td_audio.c is regenerated. Runtime integration,
upstream tools and engine resources are not changed by this generator.
"""
from pathlib import Path
import json
import math
import re
import sys

GAME = Path(__file__).resolve().parents[1]
SOURCE = GAME / "project/original-audio/city_shift.json"
TARGET = GAME / "project/plugins/toronto-driving/engine/src/td_audio.c"
BEGIN = "/* BEGIN GENERATED AUDIO: scripts/create_audio.py */"
END = "/* END GENERATED AUDIO */"
CUES = ("impact", "pickup", "complete", "transit", "fail", "menu")
VEHICLES = ("car", "truck", "motorcycle", "scooter")
SEMITONES = {"C": 0, "Cs": 1, "D": 2, "Ds": 3, "E": 4, "F": 5,
             "Fs": 6, "G": 7, "Gs": 8, "A": 9, "As": 10, "B": 11}


def note_index(note):
    match = re.fullmatch(r"([A-G]s?)([3-8])", note)
    if not match or match[1] not in SEMITONES:
        raise ValueError(f"Invalid hUGE note: {note}")
    return (int(match[2]) - 3) * 12 + SEMITONES[match[1]]


def tracker_cell(token):
    if token == ".":
        return "DN(___, 0, 0x000)"
    if token == "!":
        return "DN(___, 0, 0xC00)"
    note_index(token)
    # GBDK's natural-note labels use E_4; sharp labels use Fs4.
    label = f"{token[0]}_{token[-1]}" if len(token) == 2 else token
    return f"DN({label}, 1, 0x000)"


def pulse_period(step):
    if "note" in step:
        index = note_index(step["note"])
        hz = 130.81278265 * math.pow(2, index / 12)
    else:
        hz = step["hz"]
    if not 64 <= hz <= 8000:
        raise ValueError("Pulse frequency is outside authored bounds")
    return min(2047, max(0, round(2048 - 131072 / hz)))


def instruction(channel, step, decay=True):
    volume = step["volume"]
    if not isinstance(volume, int) or not 0 <= volume <= 15:
        raise ValueError("Envelope volume must fit one nibble")
    envelope = (volume << 4) | int(decay)
    if channel == "pulse1":
        period = pulse_period(step)
        return [0xF8, 0x00, 0x40, envelope, period & 0xFF, 0x80 | (period >> 8)]
    polynomial = step["polynomial"]
    if not 0 <= polynomial <= 255:
        raise ValueError("Noise polynomial must be a byte")
    return [0x7B, 0x00, envelope, polynomial, 0x80]


def sfx_bytes(effect, decay=True):
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
                    result.extend(instruction(channel, effect[channel][index], decay))
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


def generate(score):
    bars = score["bars"]
    if score["schema"] != 1 or score["rows_per_bar"] != 16 or len(bars) % 4:
        raise ValueError("Score must have full four-bar hUGE patterns")
    if len(bars) > 64 or not 1 <= score["ticks_per_row"] <= 32:
        raise ValueError("Score length/tempo exceeds authored bounds")
    for bar in bars:
        if len(bar["pulse2"]) != 16 or len(bar["wave3"]) != 16:
            raise ValueError("Every authored bar must have 16 rows per channel")
    patterns = len(bars) // 4
    out = "/* Original City Shift. Edit project/original-audio/city_shift.json. */\n"
    out += f"static const UBYTE td_audio_order_count = {patterns * 2};\n"
    out += "static const UBYTE td_audio_rest[] = {\n" + "    DN(___, 0, 0x000),\n" * 64 + "};\n"
    for channel in ("pulse2", "wave3"):
        for pattern in range(patterns):
            rows = [token for bar in bars[pattern * 4:(pattern + 1) * 4] for token in bar[channel]]
            out += f"static const UBYTE td_audio_{channel}_{pattern}[] = {{\n"
            out += "".join(f"    {tracker_cell(token)},\n" for token in rows) + "};\n"
    for index, channel in enumerate(("rest", "pulse2", "wave3", "rest"), 1):
        pointers = ["td_audio_rest" if channel == "rest" else f"td_audio_{channel}_{p}" for p in range(patterns)]
        out += f"static const UBYTE * const td_audio_order{index}[] = {{{', '.join(pointers)}}};\n"
    pulse = score["pulse2"]
    wave = score["wave3"]
    if pulse["duty"] not in range(4) or pulse["envelope"] not in range(256) or pulse["highmask"] != 0x80:
        raise ValueError("Duty instrument is outside authored bounds")
    if len(wave["samples"]) != 32 or any(sample not in range(16) for sample in wave["samples"]) or wave["volume"] not in (1, 2, 3):
        raise ValueError("Wave must have 32 four-bit samples and a supported volume")
    out += f"static const hUGEDutyInstr_t td_audio_duty[] = {{{{0, {pulse['duty'] << 6}, {pulse['envelope']}, NULL, 0x80}}}};\n"
    out += f"static const hUGEWaveInstr_t td_audio_wave_instruments[] = {{{{0, {wave['volume'] << 5}, 0, NULL, 0x80}}}};\n"
    out += "static const hUGENoiseInstr_t td_audio_noise_instruments[] = {{0, NULL, 0, 0, 0}};\n"
    samples = wave["samples"]
    out += c_bytes("td_audio_wave", [(samples[i] << 4) | samples[i+1] for i in range(0, 32, 2)])
    out += "static const hUGESong_t td_city_shift_song = {\n"
    out += f"    {score['ticks_per_row']}, &td_audio_order_count,\n"
    out += "    (const UBYTE **)td_audio_order1, (const UBYTE **)td_audio_order2,\n"
    out += "    (const UBYTE **)td_audio_order3, (const UBYTE **)td_audio_order4,\n"
    out += "    td_audio_duty, td_audio_wave_instruments, td_audio_noise_instruments,\n    NULL, td_audio_wave\n};\n"
    for cue in (*CUES, "brake"):
        effect = score["effects"][cue]
        if effect["priority"] not in range(9):
            raise ValueError("Native effect priorities must be 0..8")
        out += c_bytes(f"td_audio_{cue}", sfx_bytes(effect))
    out += "static const UBYTE * const td_audio_cues[] = {" + ", ".join(f"td_audio_{cue}" for cue in CUES) + "};\n"
    out += "static const UBYTE td_audio_cue_priorities[] = {" + ", ".join(str(score["effects"][cue]["priority"]) for cue in CUES) + "};\n"
    out += "static const UBYTE td_audio_cue_masks[] = {" + ", ".join(str((1 if "pulse1" in score["effects"][cue] else 0) | (8 if "noise4" in score["effects"][cue] else 0)) for cue in CUES) + "};\n"
    for vehicle in VEHICLES:
        pitches = score["engine_hz"][vehicle]
        if len(pitches) != 4:
            raise ValueError("Every road vehicle needs four engine speed stages")
        for stage, hz in enumerate(pitches):
            effect = {"pulse1": [{"hz": hz, "frames": 12, "volume": 2 if vehicle == "motorcycle" else 3}]}
            out += c_bytes(f"td_audio_engine_{vehicle}_{stage}", sfx_bytes(effect, decay=False))
    out += "static const UBYTE * const td_audio_engines[4][4] = {\n"
    out += "".join("    {" + ", ".join(f"td_audio_engine_{vehicle}_{stage}" for stage in range(4)) + "},\n" for vehicle in VEHICLES) + "};\n"
    return out


def main():
    generated = generate(json.loads(SOURCE.read_text()))
    runtime = TARGET.read_text()
    if runtime.count(BEGIN) != 1 or runtime.count(END) != 1:
        raise ValueError("Runtime must contain exactly one marked audio data block")
    prefix, remainder = runtime.split(BEGIN)
    _, suffix = remainder.split(END)
    expected = prefix + BEGIN + "\n" + generated + END + suffix
    if "--check" in sys.argv:
        if runtime != expected:
            raise SystemExit("Original audio differs from editable score; regenerate create_audio.py")
        print("Original audio matches editable score and native hUGE/SFX formats.")
    else:
        TARGET.write_text(expected)
        print("Original audio: 16-bar hUGE loop, 7 cues and 16 finite vehicle engine samples generated.")


if __name__ == "__main__":
    main()
