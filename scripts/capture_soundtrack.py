#!/usr/bin/env python3
"""Capture and analyse a native ROM's soundtrack in public PyBoy 2.7.

Supplements capture_audio.py (which never touches memory). To reach the chase
and night themes quickly this run writes game state (police stars and heat,
the play clock) and, to hear one channel at a time, the APU's NR51 panning
register; the manifest lists every write. It boots from in-memory ROM bytes,
loads and writes no save, and reads only documented symbols from the build's
NOI file. Captured PCM shows emulated output, which channels sound and how the
songs differ; it does not certify hardware audio or replace listening.
Requires PyBoy 2.7.0 and numpy.
"""
import argparse
from array import array
import hashlib
import io
from importlib.metadata import version
import json
from pathlib import Path
import re
import wave

import numpy as np
from pyboy import PyBoy

GAME = Path(__file__).resolve().parents[1]
RATE = 48000
SONGS = ("title", "day", "night", "chase")
# td_state_t offsets (td_game.h; SDCC packs structs without padding).
TD_SECONDS, TD_MODE, TD_VITALITY, TD_WANTED, TD_HEAT = 10, 19, 52, 54, 55
TD_HELP, TD_ROAM = 8, 0
NR51 = 0xFF25
CHANNEL_PAN = {"ch1": 0x11, "ch2": 0x22, "ch3": 0x44, "ch4": 0x88}
# hUGE ticks at 64 Hz in GBVM: beat = 4 rows.
SONG_BPM = {"title": 64 * 60 / (8 * 4), "day": 64 * 60 / (7 * 4),
            "night": 64 * 60 / (11 * 4), "chase": 64 * 60 / (6 * 4)}


def read_symbols(path):
    symbols = {}
    for line in path.read_text().splitlines():
        match = re.fullmatch(r"DEF (\S+) 0x([0-9A-Fa-f]+)", line.strip())
        if match:
            symbols[match[1]] = int(match[2], 16)
    return symbols


def analyse(pcm):
    """Loudness, activity, spectrum, pitch-class profile and beat period."""
    stereo = np.frombuffer(pcm, dtype=np.int16).reshape(-1, 2).astype(np.float64)
    mono = stereo.mean(axis=1)
    result = {"seconds": round(len(mono) / RATE, 3), "peak": int(np.abs(stereo).max(initial=0)),
              "rms": round(float(np.sqrt(np.mean(mono ** 2))) if len(mono) else 0.0, 1)}
    if len(mono) < RATE // 2 or result["peak"] == 0:
        return result
    window = RATE // 10
    frames = mono[:len(mono) // window * window].reshape(-1, window)
    frame_rms = np.sqrt((frames ** 2).mean(axis=1))
    result["active_fraction"] = round(float((frame_rms > 64).mean()), 3)
    segment = 8192
    hann = np.hanning(segment)
    power = np.zeros(segment // 2 + 1)
    count = 0
    for start in range(0, len(mono) - segment, segment // 2):
        power += np.abs(np.fft.rfft((mono[start:start + segment] - mono[start:start + segment].mean()) * hann)) ** 2
        count += 1
    power /= max(count, 1)
    freqs = np.fft.rfftfreq(segment, 1 / RATE)
    band = (freqs >= 200) & (freqs <= 8000)
    result["spectral_flatness"] = round(float(np.exp(np.mean(np.log(power[band] + 1e-9))) / (np.mean(power[band]) + 1e-9)), 4)
    tonal = (freqs >= 40) & (freqs <= 2500)
    picks = np.argsort(power * tonal)[::-1]
    peaks = []
    for index in picks:
        if len(peaks) == 5:
            break
        if all(abs(freqs[index] - f) > 12 for f in peaks):
            peaks.append(float(freqs[index]))
    result["peak_hz"] = [round(f, 1) for f in peaks]
    chroma = np.zeros(12)
    for f, p in zip(freqs[tonal], power[tonal]):
        chroma[int(round(12 * np.log2(f / 261.6256))) % 12] += p
    result["chroma"] = [round(float(v), 4) for v in chroma / chroma.sum()]
    names = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"]
    result["strongest_pitch_classes"] = [names[i] for i in np.argsort(chroma)[::-1][:4]]
    # Beat period from the autocorrelation of the onset envelope (10 ms hop).
    hop = RATE // 100
    energy = np.sqrt((mono[:len(mono) // hop * hop].reshape(-1, hop) ** 2).mean(axis=1))
    onset = np.maximum(np.diff(energy), 0)
    onset -= onset.mean()
    corr = np.correlate(onset, onset, "full")[len(onset) - 1:]
    lags = np.arange(len(corr)) * hop / RATE
    choice = (lags >= 0.3) & (lags <= 0.8)
    if choice.any():
        lag = lags[choice][np.argmax(corr[choice])]
        result["beat_seconds"] = round(float(lag), 3)
        result["beat_bpm"] = round(60 / float(lag), 1)
    return result


def cosine(a, b):
    a, b = np.array(a), np.array(b)
    return round(float(a @ b / (np.linalg.norm(a) * np.linalg.norm(b))), 3)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True, type=Path)
    parser.add_argument("--symbols", required=True, type=Path, help="the build's .noi file")
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    if version("pyboy") != "2.7.0":
        raise SystemExit("This capture is written against public PyBoy 2.7.0")
    rom = args.rom.read_bytes()
    digest = hashlib.sha256(rom).hexdigest()
    output = args.output.resolve()
    if not output.is_relative_to((GAME / "project/build").resolve()) or output.exists():
        raise SystemExit("Choose a new evidence directory inside project/build")
    symbols = read_symbols(args.symbols)
    td = symbols["_td"]
    song_address = {symbols[f"_td_song_{slug}"] & 0xFFFF: slug for slug in SONGS}
    output.mkdir(parents=True)
    emulator = PyBoy(io.BytesIO(rom), window="null", cgb=True,
                     sound_emulated=True, sound_sample_rate=RATE, sound_volume=100)
    emulator.set_emulation_speed(0)
    memory = emulator.memory
    frame, held, writes, captures = 0, set(), [], []

    def word(address):
        return memory[address] | (memory[address + 1] << 8)

    def song():
        return song_address.get(word(symbols["_music_current_track"]), "none")

    def write(address, value, why):
        memory[address] = value
        writes.append({"frame": frame, "address": f"0x{address:04X}", "value": value, "why": why})

    def tick(buttons, count, pcm=None, pan=None, log=None):
        nonlocal frame, held
        requested = set(buttons)
        for button in held - requested:
            emulator.button_release(button)
        for button in requested - held:
            emulator.button_press(button)
        held = requested
        for _ in range(count):
            if pan is not None:
                memory[NR51] = pan  # an effect start rewrites NR51 = 0xFF
            emulator.tick(1, render=False, sound=True)
            frame += 1
            if log is not None:
                log.append((song(), memory[symbols["_music_play_isr_pause"]],
                            memory[symbols["_sfx_play_bank"]] != 0xFF, memory[symbols["_music_mute_mask"]]))
            if pcm is not None:
                raw = emulator.sound.raw_buffer
                head = emulator.sound.raw_buffer_head
                if emulator.sound.raw_buffer_format != "b" or head < 0 or head > len(raw) or head % 2:
                    raise RuntimeError("Unexpected public signed stereo PCM buffer")
                pcm.extend(sample << 8 for sample in raw[:head])

    def capture(name, buttons, count, pan=None, note=""):
        pcm, log, start = array("h"), [], frame
        tick(buttons, count, pcm, pan, log)
        path = output / f"{name}.wav"
        with wave.open(str(path), "wb") as wav:
            wav.setnchannels(2)
            wav.setsampwidth(2)
            wav.setframerate(RATE)
            wav.writeframes(pcm.tobytes())
        songs = sorted({entry[0] for entry in log})
        report = {"name": name, "note": note, "start_frame": start, "end_frame": frame,
                  "mode_at_end": memory[td + TD_MODE], "wanted_at_end": memory[td + TD_WANTED],
                  "songs_loaded": songs, "music_paused_frames": sum(entry[1] for entry in log),
                  "effect_frames": sum(entry[2] for entry in log),
                  "effect_channel_masks": sorted({entry[3] for entry in log if entry[2]}),
                  "pan": None if pan is None else f"0x{pan:02X}",
                  "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
        report.update(analyse(pcm.tobytes()))
        captures.append(report)
        return report

    def isolate(prefix, seconds):
        for channel, pan in CHANNEL_PAN.items():
            capture(f"{prefix}-{channel}", [], int(60 * seconds), pan, f"{prefix} with only {channel} routed to the outputs")

    def menu_audio_step():  # START, wrap the cursor up to the sound item, A
        tick(["start"], 4); tick([], 4); tick(["up"], 4); tick([], 4); tick(["a"], 4)

    try:
        tick([], 240)
        if memory[td + TD_MODE] != TD_HELP or memory[td + TD_VITALITY] != 100:
            raise SystemExit("Unexpected boot state; check the td_state_t offsets")
        capture("title", [], 480, note="title screen (td.mode == TD_HELP)")
        isolate("title", 3)
        tick(["a"], 4)
        tick([], 120)
        capture("day", [], 600, note="free roam, new game at 08:00, standing still")
        isolate("day", 3)
        capture("drive-full", ["a"], 90, note="music + effects, pulling away: engine revs on launch and upshift only")
        capture("brake-full", ["b"], 30, note="braking from speed: hiss on CH4 over the music")
        tick([], 60)
        capture("drive-full-ch1", ["a"], 240, CHANNEL_PAN["ch1"], "driving again, CH1 only: counter-line plus brief revs")
        tick(["b"], 48); tick([], 90)
        write(td + TD_WANTED, 3, "three police stars")
        write(td + TD_HEAT, 12, "full heat so the stars hold")
        tick([], 150)
        capture("chase", [], 480, note="td.wanted = 3")
        write(td + TD_HEAT, 12, "keep the stars")
        isolate("chase", 2.5)
        write(td + TD_WANTED, 0, "clear the stars")
        write(td + TD_HEAT, 0, "clear the heat")
        tick([], 240)
        capture("after-chase", [], 120, note="stars cleared; the chase winds down to the day theme")
        write(td + TD_SECONDS, 600 & 0xFF, "play clock to second 600 (about 22:30)")
        write(td + TD_SECONDS + 1, 600 >> 8, "play clock high byte")
        tick([], 180)
        capture("night", [], 600, note="night (td_daynight_lights), standing still")
        isolate("night", 3)
        tick(["start"], 4)
        capture("menu-cue", [], 30, note="pause menu opened: menu cue, music paused")
        tick([], 4); tick(["up"], 4); tick([], 4); tick(["a"], 4)  # FULL -> EFFECTS
        tick([], 120)
        capture("effects-only-paused", [], 120, note="pause menu, effects-only, after the cue")
        tick(["b"], 4); tick([], 30)
        capture("effects-only-idle", [], 120, note="effects-only, standing still")
        capture("effects-only-driving", ["a"], 120, note="effects-only, holding A: steady engine drone")
        tick(["b"], 48); tick([], 60)
        menu_audio_step()  # EFFECTS -> SILENT
        tick([], 120)
        capture("silent-menu", [], 120, note="pause menu, silent")
        tick(["b"], 4); tick([], 4)
        capture("silent-driving", ["a"], 120, note="silent, holding A")
        tick(["b"], 48); tick([], 60)
        menu_audio_step()  # SILENT -> FULL
        tick([], 4); tick(["b"], 4); tick([], 60)
        capture("resumed", [], 360, note="back to music + effects at night")
    finally:
        emulator.stop(save=False)

    songs = {name: next(c for c in captures if c["name"] == name) for name in ("title", "day", "night", "chase")}
    comparison = {f"{a}~{b}": {"chroma_cosine": cosine(songs[a]["chroma"], songs[b]["chroma"])}
                  for i, a in enumerate(songs) for b in list(songs)[i + 1:]}
    result = {"rom_sha256": digest, "emulator": f"PyBoy {version('pyboy')} public Sound API (CGB)",
              "sample_rate": RATE, "memory_writes": writes, "save_writes": False,
              "expected_bpm": {k: round(v, 1) for k, v in SONG_BPM.items()},
              "captures": captures, "song_comparison": comparison}
    (output / "manifest.json").write_text(json.dumps(result, indent=2) + "\n")
    by_name = {c["name"]: c for c in captures}
    failures = []
    for name in SONGS:
        if by_name[name]["songs_loaded"] != [name]:
            failures.append(f"{name}: loaded {by_name[name]['songs_loaded']}")
        for channel in CHANNEL_PAN:
            if by_name[f"{name}-{channel}"]["rms"] <= 0:
                failures.append(f"{name}: {channel} silent")
    for name in ("title", "day", "night", "chase", "menu-cue", "brake-full", "effects-only-driving", "resumed"):
        if by_name[name]["peak"] == 0:
            failures.append(f"{name}: no sound")
    for name in ("effects-only-paused", "silent-menu", "silent-driving"):
        if by_name[name]["peak"] != 0:
            failures.append(f"{name}: not silent")
    if 8 not in by_name["brake-full"]["effect_channel_masks"]:
        failures.append("brake-full: no braking hiss on CH4")
    if 1 not in by_name["drive-full"]["effect_channel_masks"]:
        failures.append("drive-full: no engine rev")
    if 1 not in by_name["effects-only-driving"]["effect_channel_masks"]:
        failures.append("effects-only-driving: no engine drone")
    ch1 = by_name["drive-full-ch1"]
    if ch1["effect_frames"] * 2 > ch1["end_frame"] - ch1["start_frame"]:
        failures.append("drive-full-ch1: effects held CH1 for most of the drive")
    summary = {c["name"]: {k: c.get(k) for k in ("songs_loaded", "peak", "rms", "active_fraction",
                                                  "spectral_flatness", "beat_bpm", "strongest_pitch_classes",
                                                  "effect_frames", "effect_channel_masks", "mode_at_end")}
               for c in captures}
    print(json.dumps({"rom_sha256": digest, "summary": summary, "song_comparison": comparison,
                      "failures": failures}, indent=1))
    if failures:
        raise SystemExit("Soundtrack checks failed; evidence retained for diagnosis")


if __name__ == "__main__":
    main()
