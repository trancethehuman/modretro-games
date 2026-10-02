"""Supplement plugin visual tests with actual PCM from public PyBoy 2.7.

No adjacent saves are loaded or written, no game memory is modified, and every
sample comes from an individually ticked native frame. This does not certify
hardware audio or human listening. Requires the optional PyBoy dependency.
"""
import argparse
from array import array
import hashlib
import io
from importlib.metadata import version
import json
from pathlib import Path
import sys
import wave
from pyboy import PyBoy

GAME = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True, type=Path)
    parser.add_argument("--expected-sha256", required=True)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    sdk_version = version("pyboy")
    if sdk_version != "2.7.0":
        raise SystemExit("This capture contract is verified against public PyBoy2.7.0")
    rom = args.rom.read_bytes()
    digest = hashlib.sha256(rom).hexdigest()
    if digest != args.expected_sha256:
        raise SystemExit("ROM digest mismatch")
    output = args.output.resolve()
    if not output.is_relative_to((GAME / "project/build").resolve()) or output.exists():
        raise SystemExit("Choose a new evidence directory inside project/build")
    output.mkdir(parents=True)
    emulator = PyBoy(io.BytesIO(rom), window="null", cgb=True,
                     sound_emulated=True, sound_sample_rate=48000, sound_volume=100)
    emulator.set_emulation_speed(0)
    frame = 0
    held = set()
    reports = []

    def tick(buttons, count, pcm=None):
        nonlocal frame, held
        requested = set(buttons)
        for button in held - requested:
            emulator.button_release(button)
        for button in requested - held:
            emulator.button_press(button)
        held = requested
        for _ in range(count):
            emulator.tick(1, render=True, sound=True)
            frame += 1
            if pcm is not None:
                raw = emulator.sound.raw_buffer
                head = emulator.sound.raw_buffer_head
                if emulator.sound.raw_buffer_format != "b" or head < 0 or head > len(raw) or head % 2:
                    raise RuntimeError("Unexpected public signed stereo PCM buffer")
                # Head is a BYTE count in pinned2.7. Its ndarray row slicing
                # can include stale tail data, so copy only these genuine bytes.
                pcm.extend(sample << 8 for sample in raw[:head])

    def capture(name, buttons, count):
        pcm = array("h")
        start = frame
        tick(buttons, count, pcm)
        peak = max(map(abs, pcm), default=0)
        if sys.byteorder != "little":
            pcm.byteswap()
        path = output / f"{name}.wav"
        with wave.open(str(path), "wb") as wav:
            wav.setnchannels(2)
            wav.setsampwidth(2)
            wav.setframerate(48000)
            wav.writeframes(pcm.tobytes())
        reports.append({"name": name, "start_frame": start, "end_frame": frame,
                        "stereo_samples": len(pcm) // 2, "peak": peak,
                        "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})

    try:
        tick([], 180)
        tick(["a"], 4)
        tick([], 60)
        capture("city-music", [], 600)
        capture("engine-and-brake", ["a"], 120)
        capture("braking", ["b"], 48)
        tick([], 60)
        tick(["start"], 4)
        tick([], 4)
        tick(["up"], 4)  # Wrap pause cursor0 to audio8.
        tick([], 4)
        tick(["a"], 4)  # FULL → EFFECTS.
        tick([], 120)   # Let finite menu cue end before measuring pause silence.
        capture("paused-effects-only", [], 120)
        tick(["b"], 4)
        tick([], 4)
        capture("effects-only-engine", ["a"], 120)
        tick(["b"], 48)
        tick([], 60)
        tick(["start"], 4)
        tick([], 4)
        tick(["up"], 4)
        tick([], 4)
        tick(["a"], 4)  # EFFECTS → SILENT.
        tick([], 120)
        capture("silent-menu", [], 120)
        tick(["b"], 4)
        tick([], 4)
        capture("silent-driving", ["a"], 120)
        tick(["b"], 48)
        tick([], 60)
        tick(["start"], 4)
        tick([], 4)
        tick(["up"], 4)
        tick([], 4)
        tick(["a"], 4)  # SILENT → FULL.
        tick([], 4)
        tick(["b"], 4)
        tick([], 60)
        capture("resumed-city-music", [], 600)
    finally:
        emulator.stop(save=False)
    result = {"rom_sha256": digest, "emulator": f"PyBoy{sdk_version} public Sound API",
              "sample_rate": 48000, "channels": 2, "sample_bits": 16,
              "memory_writes": False, "save_writes": False, "captures": reports}
    (output / "manifest.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    peaks = {item["name"]: item["peak"] for item in reports}
    if not all(peaks[name] > 0 for name in ("city-music", "engine-and-brake", "braking", "effects-only-engine", "resumed-city-music")):
        raise SystemExit("Expected active native PCM was absent; retain evidence for diagnosis")
    if not all(peaks[name] == 0 for name in ("paused-effects-only", "silent-menu", "silent-driving")):
        raise SystemExit("Expected silent native PCM contained samples; retain evidence for diagnosis")


if __name__ == "__main__":
    main()
