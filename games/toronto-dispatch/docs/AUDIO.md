# Original native audio

Toronto Dispatch's original **City Shift** score uses a quiet pulse melody and a rounded wave accompaniment. Six event cues distinguish an impact, pickup, completed delivery, transit boarding/arrival, failure and menu action. Braking adds a short noise effect; car, truck, motorcycle and scooter engine pitches follow four movement-speed ranges. All notes, wave samples and effects were authored for this game under the repository MIT licence. No melody or recorded sample was copied.

## Editable source and generator

- `project/original-audio/city_shift.json`: 16 authored bars, instruments, original 32-sample wave, vehicle pitch ranges and effect segments. `.` continues a note/envelope; `!` cuts that channel's volume. Natural notes use `E4` in JSON, converted to GBDK's `E_4` names.
- `scripts/create_audio.py`: standard-library generator. It validates note/instrument/byte ranges and writes only the marked immutable data block in `project/plugins/toronto-driving/engine/src/td_audio.c`.
- `engine/include/td_audio.h`: the scene integration API. The score is compiled native `hUGESong_t` data; this milestone does not claim a registered `.uge` music-editor asset.

From the repository root:

```sh
python3 games/toronto-dispatch/scripts/create_audio.py
```

The generated score has four 64-row orders per musical channel, plus a shared silent pattern for the effect channels. Tempo is nine hUGE ticks per row. All descriptor pointers, patterns, wave data and SFX streams live in the same automatically assigned ROM bank. Compile/link inspection must confirm that allocation in the final ROM. Effect duration parameters follow the pinned GB Studio convention of four native SFX ticks per nominal frame. Musical timing, cue distinction and intelligibility remain manual listening checks.

## Channel sharing and modes

| Hardware channel | Ownership |
| --- | --- |
| CH1 pulse | Movement engine and short event tones |
| CH2 pulse | Original melody |
| CH3 wave | Original accompaniment |
| CH4 noise | Braking and impacts |

GBVM's existing timer interrupt, hUGEDriver and finite SFX player perform playback. The scene does not install another IRQ or write an independent audio-register driver. Music reserves CH1/CH4 through the driver's mute mask. SFX keep CH2/CH3 available, explicitly cut every effect channel before their terminator, and use the upstream priority mechanism. Completion/impact/failure priority 8 interrupts engine/braking; pickup/transit priority 4 interrupts ambience; menu priority 2 is quieter. Low-priority ambience waits for an active event sound to finish. Engine refreshes are bounded by the real `sys_time` clock.

Modes exposed to the pause menu are **music + effects**, **effects only**, and **silent**. The preference defaults to music + effects on each boot; it is not stored in the game save. Silent cuts active effects and the tune. On foot, engine ambience stops. Holding the brake at forward speed produces a hiss; reversing does not continuously squeal.

## Scene integration

- Call `td_audio_init()` during city initialization after the engine music manager is ready.
- Call `td_audio_update(speed, vehicle, onfoot, braking, world_active)` once per rendered update, including menu frames, after simulation/events. Active means the world clock advances: roaming, waiting or riding. Use the actual held brake input.
- Call `td_audio_play(TD_AUDIO_PICKUP/COMPLETE/IMPACT/TRANSIT/FAIL/MENU)` once at the corresponding event. Calls queue a cue; the next update flushes it after pause transitions, so the result screen does not immediately cut a new completion/failure jingle. Higher-priority queued cues win when events share a frame.
- Cycle `td_audio_set_mode()` from the pause menu; `td_audio_get_mode()` supplies its label.

Map, dispatch, help and pause screens stop the tune and engine. Short queued interface/result cues may still sound. Leaving a paused screen resumes the existing score position instead of restarting the composition; effects-only keeps the score paused. GBVM's music pause cuts sound, so resumption becomes audible with the next authored note.

The pinned GBVM `music_pause()` lacks a BANKED/NONBANKED annotation and resides in `music_manager.c`'s assigned bank. A small HOME/NONBANKED wrapper gets that bank from exported linker symbol `b_music_init_driver`, switches for the near call and restores `CURRENT_BANK` before returning. The getter uses an immediate linker relocation, rather than equating an external symbol during assembly. This adapts the existing GBVM function without modifying upstream code. The final build must verify that `music_pause` and `music_init_driver` share a bank and that the wrapper is in HOME.

## Evidence and acceptance

The 2026-10-02 milestone has an official native ROM build, 471 passing host engine checks, plugin visual tests and supplemental real emulator PCM capture. The exact 262,144-byte CGB ROM has SHA-256:

```text
a2f00db4ef834112a3491e50cec832653023a0456f0d9cbca6d2386be7322a59
```

The retained `native-a2f00db4-modes/manifest.json` and WAV files are local in ignored `project/build/audio-evidence/`. The manifest matches that ROM, uses public PyBoy 2.7.0 in CGB mode and records stereo 48,000 Hz PCM converted from signed 8-bit samples to 16-bit WAV. Ordinary buttons drove the run; no progress/memory injection or save loading/writing was used. All eight active/silent sample checks passed.

Historical note: the earlier pre-transition-fix ROM `038f1561a52b9abaa943111ba4fc215c400bac19d0e5894f921aa23663e17381` had the same interval lengths, peaks and WAV hashes in `native-038f1561-modes`. The table below comes from a separate fresh capture of the verified new ROM bytes, not an attribution of the earlier recording to this binary.

| Captured interval | Duration | Peak absolute PCM16 sample |
| --- | --- | --- |
| City music while stationary | 10 seconds | 2304 |
| Held acceleration, music + effects | 2 seconds | 3072 |
| Braking, music + effects | 0.8 seconds | 2560 |
| Paused in effects-only mode, after menu cue ended | 2 seconds | 0 |
| Held acceleration in effects-only mode | 2 seconds | 768 |
| Silent menu | 2 seconds | 0 |
| Silent driving | 2 seconds | 0 |
| City music after returning to music + effects | 10 seconds | 4352 |

These samples establish actual native emulated sound output, effects-only output and zero samples in the measured silent/pause intervals. Peak values are measurements of the combined captured output, not isolated-channel loudness or a listening assessment. They do not establish every event cue, the entire score, audio quality or physical speaker/headphone behaviour. No human listening or physical sound result is claimed.

## Reproduce the supplemental PCM check

Use a Python interpreter with the optional public **PyBoy 2.7.0** dependency. The plugin's `toolchain_doctor` identifies its configured emulator runtime; substitute that interpreter for `python3` if PyBoy is unavailable in your default Python. Do not modify the plugin cache or managed toolchain. This check supplements the plugin's native visual playtests.

From the repository root, choose a new output directory. This example targets the tested native ROM. The retained run already uses the named directory; choose a new name such as `native-a2f00db4-recapture` when repeating it:

```sh
python3 games/toronto-dispatch/scripts/capture_audio.py \
  --rom games/toronto-dispatch/project/build/toronto-dispatch.gbc \
  --expected-sha256 a2f00db4ef834112a3491e50cec832653023a0456f0d9cbca6d2386be7322a59 \
  --output games/toronto-dispatch/project/build/audio-evidence/native-a2f00db4-modes
```

The script refuses a different ROM hash, another PyBoy version, an existing output directory or an output outside this game's ignored build tree. It boots verified ROM bytes through `io.BytesIO`, avoiding adjacent cartridge saves/RTC files, and always calls `stop(save=False)`. The fixed button sequence belongs to this milestone's boot and pause-menu flow; review it before using the script on a later gameplay revision.

Every native frame advances with `tick(1, render=True, sound=True)`. The script copies the documented public `sound.raw_buffer[:sound.raw_buffer_head]`, checks signed-byte format and an even byte count, preserves stereo order and scales each signed sample by 256 for PCM16. Pinned PyBoy 2.7.0's `ndarray` slices stereo rows using a byte head, which can retain stale tail samples in short frames; the public raw-buffer path uses only the valid bytes. No emulator-private worker or IO/APU memory access is required. See the [public Sound API](https://docs.pyboy.dk/api/sound.html), [PyBoy constructor/tick/stop API](https://docs.pyboy.dk/) and [pinned 2.7.0 source](https://github.com/Baekalfen/PyBoy/blob/v2.7.0/pyboy/api/sound.py).

The script writes a manifest and WAV hashes before checking nonzero active samples and zero measured silent samples. If an assertion fails, preserve that output for diagnosis. A passed sample check does not replace listening.

Review the tune at normal handheld volume, event distinction, completion cues on result screens, menu/world pause, foot/vehicle transitions and every mute mode. Check that repeated impacts cannot permanently mute the music or leave an effect channel sounding. Associate any retained actual audio sample with the exact ROM digest and emulator/hardware identity.

References reviewed 2026-10-02: the pinned GBVM `music_manager.h/.c`, `sfx_player.h/.c`, `hUGEDriver.h` and timer ISR; GB Studio CLI 4.3.2's `ugeHelper.ts` and basic-SFX compiler; public PyBoy 2.7.0 Sound API/source; [Pan Docs audio registers](https://github.com/gbdev/pandocs/blob/master/src/Audio_Registers.md). Runtime distribution notices remain in [DISTRIBUTION.md](DISTRIBUTION.md); the native build identity and remaining gates are in [BUILD.md](BUILD.md) and [TESTING.md](../TESTING.md).
