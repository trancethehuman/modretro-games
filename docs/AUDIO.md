# Original native audio

Toronto Dispatch has four original songs that follow the game state, six event cues (impact, pickup, completed delivery, transit boarding/arrival, failure, menu action), a braking hiss and vehicle engine sounds. Every note, drum pattern, instrument, wave shape and effect was written for this game under the repository MIT licence. No existing song, jingle, TTC chime or recording was copied or paraphrased; the Toronto flavour comes from general genre idioms, not from any particular work.

## Soundtrack

| Song | Plays | Character | Tempo | Length | Data |
| --- | --- | --- | --- | --- | --- |
| **Skyline Call** (`title`) | Title screen (`td.mode == TD_HELP`) | D major indie-rock anthem: a rising four-note call over strummed arpeggio chords, driving eighth-note bass, rock beat; verse, resolving verse, half-time bridge with pads, tag | 8 ticks/row, about 120 BPM | 32 bars; loops past its 4-bar intro | 6,108 B |
| **King Street Iron** (`day`) | Free roam by day | F major soca groove (Caribana feel): steelpan-style bell lead with rolls, offbeat chord skanks, a bouncing bass with cuts, kick/snare and the "iron" on periodic noise; intro, verse, chorus, breakdown, then verse and chorus again with pads and 3-3-2 strums | 7 ticks/row, about 137 BPM | 40 bars; loops past its intro | 5,746 B |
| **Six After Dark** (`night`) | Free roam from 19:00 to 06:30 (headlamps on) | C minor late-night R&B: broken electric-piano chords, a breathy lead with vibrato, sliding 808-style bass, half-time beat with claps and rims; verse, bridge, verse with pads, tag | 11 ticks/row, about 87 BPM | 32 bars; loops past its intro | 3,990 B |
| **Gardiner Heat** (`chase`) | Police attention of two or more stars | Tense E minor with Phrygian turns: galloping bass, stabbing chords, brassy lead, 16th-note hats, a half-time breakdown and a snare-roll build | 6 ticks/row, about 160 BPM | 32 bars; loops whole | 5,148 B |

Menus (pause, map, dispatch board, transit, result, busted/wasted) keep music quiet: they pause the current song in place and leave room for the menu, completion and failure cues; leaving the menu resumes the song where it stopped, unless the state changed behind the menu (see below). GBVM ticks hUGEDriver at 64 Hz, so a row of N ticks lasts N/64 s and a 16-row bar is four beats.

## Channels

| Hardware channel | Music | Shared with |
| --- | --- | --- |
| CH1 pulse | Counter-line: strummed or skanked chords (arpeggio effect), broken chords, pads, runs | Event cues and short engine revs, only while they play |
| CH2 pulse | Lead melody | Nothing; no effect uses CH2 |
| CH3 wave | Bass, with `warm` (fundamental plus second and third harmonics) and `bright` (rounded saw) waves for small speakers | Nothing |
| CH4 noise | Drums: kick and snare with pitch macros, clap, closed/open hats, crash, and periodic-noise iron and rim | Braking hiss and impacts, only while they play |

GBVM's existing timer interrupt, hUGEDriver and finite SFX player perform playback; the scene installs no interrupt and writes no audio registers of its own. In music + effects mode the music's global mute mask is empty. An effect mutes only the channels it uses (`music_mute_mask`), and GBVM's end-of-effect path hands them back; every music note row loads its instrument, so a channel resumes cleanly on its next note. Effects explicitly cut each channel they touch before their terminator. Completion/impact/failure priority 8 interrupts revs and braking; pickup/transit priority 4 interrupts them; menu priority 2 is quieter; revs and braking wait for an active event sound to finish.

## Engine and braking

- **Music + effects:** the continuous engine drone is gone. A quiet 11-frame rev (three rising steps at 95%, 100% and 112% of the vehicle's stage pitch, fading) plays when the vehicle pulls away and when it reaches a faster speed stage, at most once per 30 frames. Slowing by two stages re-arms the next rev. The counter-line on CH1 therefore keeps playing while driving.
- **Effects only:** nothing else uses CH1, so the original steady drone remains: 12-frame samples at the vehicle's four stage pitches, refreshed every 10 frames.
- **Braking:** holding the brake above speed 6 repeats the short CH4 hiss every 10 frames in both modes; reversing does not squeal. On foot or stopped, engine sounds stop.

Car, truck, motorcycle and scooter keep their own four stage pitches (`engine.hz`); the motorcycle is one volume step quieter.

## Song selection and switching

`td_audio_update()` decides the wanted song every update:

1. `td.mode == TD_HELP`: title.
2. Otherwise chase when `td.wanted >= 2`, night when `td_daynight_lights` is set (19:00 to 06:30, the headlamp rule), else day.

Menus pause the tune but the wanted song keeps following the world, so a change made behind a menu (an arrest clears the stars) has already waited out its hold when play resumes. A different song starts only at a safe moment: it must have been wanted for 60 frames (180 frames when leaving the chase while one star remains, so the chase winds down instead of flickering), the tune must be audible, and no effect may be playing or queued. Leaving the title switches at once. `music_load()` passes the song's bank and address; GBVM's music interrupt cuts the old notes and runs `hUGE_init` for the new song on its next tick, which starts it from the top and clears hUGE's mute mask. Until that has happened, queued cues and engine sounds wait, so the restart cannot swallow them; the next update restores the mute mask.

## Editable source and generator

- `project/original-audio/soundtrack.json`: the shared instrument palette (pulse duty/volume/fade, wave level and shape, noise envelopes, pitch macros and periodic mode), the wave shapes, the drum-letter kit, event cues, braking, engine pitches/drone/rev and the song list. Its `notation` block documents the score format.
- `project/original-audio/songs/<slug>.json`: one score per song: title, mood, ticks per row, default instruments per channel, four-bar sections and the order (with `loop`, the order to return to).
- `scripts/create_audio.py`: standard-library generator. It writes each song as a complete generated `engine/src/td_song_<slug>.c` and regenerates only the marked data block in `engine/src/td_audio.c` (the song table, cues, drones and revs). `--check`, run by `make check`, fails if any output is stale or a `td_song_*.c` file is left over. It validates notes, rows per bar, instruments (at most 15 per type), effects, drum ranges and each song's size.

Score notation, in short: each bar is 16 rows of space-separated tokens. A token is a note (`C5`, `Fs4`, `Bb3`; hUGE names, so pulse channels sound an octave lower and the wave channel two octaves lower), a drum letter on the drum channel (`k` kick, `s` snare, `c` clap, `h` hat, `o` open hat, `x` crash, `i` iron, `r` rim), `.` to continue or `!` to cut. `:XYZ` adds a hUGE effect (for example `:047` arpeggio, `:330` slide, `:432` vibrato, `:C08` volume), `+` holds that effect over the token's rows and following bare `.` rows, and `/n` fills n rows. `<name>` switches a channel's instrument. A section gives each channel 1, 2 or 4 bars (repeated to four), `"@section"` to reuse another section's channel, or nothing for silence. Master volume, panning, routines, pattern break and position jump are not available to scores; the generator adds the loop's position jump itself.

From the repository root:

```sh
python3 scripts/create_audio.py
```

## Banks and memory

Each song file is `#pragma bank 255` with `BANKREF(td_song_<slug>)`, so the autobanker places a whole song (patterns, orders, instruments, noise-macro subpatterns, waves and the `hUGESong_t`) in one ROM bank. `td_audio.c` stores `TO_FAR_PTR_T` bank/address pairs (SDCC accepts a bank reference as a constant only in that form) and never reads song data; GBVM's music interrupt switches to the song's bank before calling hUGEDriver. Event cues and engine samples stay in `td_audio.c`'s own bank (`td_audio_assets`), which GBVM's SFX player switches to.

The pinned GBVM `music_pause()` lacks a BANKED/NONBANKED annotation and resides in `music_manager.c`'s assigned bank. A small HOME/NONBANKED wrapper gets that bank from exported linker symbol `b_music_init_driver`, switches for the near call and restores `CURRENT_BANK` before returning. The getter uses an immediate linker relocation, rather than equating an external symbol during assembly. A build must keep `music_pause` and `music_init_driver` in one bank and the wrapper in HOME.

In the 2026-10-10 build the songs landed in banks 19 (day), 21 (title), 23 (chase) and 24 (night), `td_audio` in bank 22, and `music_pause`/`music_init_driver` share bank 12 with the wrapper in `_HOME`. ROM use rose from 433,513 to 453,613 bytes (banks 1-27 used, four of 32 still free); the ROM stays 524,288 bytes. The runtime adds seven bytes of WRAM.

## Scene integration

The public API in `engine/include/td_audio.h` is unchanged.

- Call `td_audio_init()` during city initialization after the engine music manager is ready. It loads the song for the current state (the title at boot).
- Call `td_audio_update(speed, vehicle, onfoot, braking, world_active)` once per rendered update, including menu frames, after simulation/events. Active means the world clock advances: roaming, waiting or riding. Use the actual held brake input. The function also reads `td.mode`, `td.wanted` and `td_daynight_lights`.
- Call `td_audio_play(TD_AUDIO_PICKUP/COMPLETE/IMPACT/TRANSIT/FAIL/MENU)` once at the corresponding event. Calls queue a cue; the next update flushes it after pause transitions, so the result screen does not immediately cut a new completion/failure jingle. Higher-priority queued cues win when events share a frame.
- Cycle `td_audio_set_mode()` from the pause menu; `td_audio_get_mode()` supplies its label. Modes are **music + effects**, **effects only** and **silent**; the preference resets to music + effects on each boot and is not saved. Silent cuts active effects and the tune; effects only keeps the tune paused.

## Evidence, 2026-10-10

ROM: GB Studio CLI `make:rom` of this source, 524,288 bytes, SHA-256 `086d36d266e08b6367ff5ac4c0219030251d6c6f7eaa7471b3b70e57ab67a541`. `make check` passes (including `create_audio.py --check`); the memory guard passes with heap `D54B`, stack `DF00` and 2,485 bytes of reserve (guard 1,024; the source before this change had 2,492).

`scripts/capture_soundtrack.py` ran that ROM in public PyBoy 2.7.0 (CGB, 48 kHz stereo PCM from the public raw sound buffer). It boots from ROM bytes, loads and writes no save, and lists its memory writes in the manifest: `td.wanted`/`td.heat` to reach the chase, `td.seconds` to reach night, and NR51 to route one channel at a time to the output. It also reads which `hUGESong_t` GBVM has loaded. Results (`project/build/audio-evidence/soundtrack-086d36d2/`, git-ignored, with WAVs per capture, `manifest.json`, screenshots of each song's game state and a spectrogram sheet):

| Capture | Loaded song | Mix RMS | CH1 / CH2 / CH3 / CH4 RMS | Beat from onsets |
| --- | --- | --- | --- | --- |
| Title screen, 8 s | title | 3,424 | 710 / 1,201 / 2,240 / 881 | 120.0 BPM |
| Day roam, 10 s | day | 3,279 | 369 / 1,437 / 1,934 / 1,210 | 136.4 BPM on CH1 and CH4 |
| Chase (three stars), 8 s | chase | 3,971 | 615 / 1,289 / 2,215 / 1,167 | 80.0 BPM (half of 160) |
| Night (play second 600), 10 s | night | 2,558 | 757 / 808 / 2,240 / 766 | 87.0 BPM |

- All four channels sound in every song. Spectral flatness separates the noise drums (0.31-0.75) from the pulse channels (0.02-0.11); the strongest pitch classes match each key (title C#/D/B/A, day F/C/Bb/A, chase E/G/B/F#). Pitch-class profiles of the four mixes have cosine similarity 0.59-0.86 and their tempos differ.
- The loaded song matched the state in every capture. A separate probe on the same ROM timed the changes: title to day 9 frames after A (the radio's menu cue finishes first); day to chase 61 frames after two stars; chase to day 181 frames after dropping to one star; day to night 112 frames after moving the clock to 19:00 and night to day 119 frames after 06:29 (the palette and headlamp flag update on the next play second, then the 60-frame hold); after an arrest cleared three stars behind the busted screen, the day theme started 1 frame after pressing A. After the title theme's 64-second pass, its sound envelope best matches the first pass 8.25 s after the song was loaded (r = 0.93; order 1 begins 8 s into the song) and not its start (r = 0.13): the loop skips the intro.
- Pulling away for 90 frames: 21 frames of CH1 revs (launch and one upshift). Braking from speed 17 for 30 frames: 12 frames of CH4 hiss. Driving 240 more frames with only CH1 routed: effects held CH1 for 45 frames (revs and one impact) and CH1 sounded in 68% of 100 ms windows (50% when standing still, as the day counter-line is offbeat skanks).
- Opening the pause menu: the menu cue sounded (peak 768) with the song paused. Effects only: zero samples in the pause menu and standing still, the engine drone while driving (peak 4,096). Silent: zero samples in the menu and while driving. Back to music + effects, the night theme resumed (peak 10,752).
- Pacing (PyBoy, scene updates per second counted at `toronto_update`, attention held at zero), source before -> this build: title 60.00 -> 60.00, parked 59.67 -> 59.60, driving 57.80 -> 57.63, driving and turning 58.17 -> 58.23.

These samples establish emulated sound output, channel use, song selection and mode behaviour for this ROM. They do not establish audio quality, mix balance, hardware timing or physical speaker/headphone behaviour; no human listening is claimed. Listen at normal handheld volume for the mix of the four channels, the rev level, cue distinction against the busier music, song changes at dusk/dawn and when stars appear or clear, and every mute mode.

Earlier evidence (the City Shift score with CH1/CH4 reserved for effects, ROM `a2f00db4…`, `native-a2f00db4-modes`) applies only to that ROM; `scripts/capture_audio.py` remains pinned to it.

## Reproduce

Use the plugin's PyBoy 2.7.0 interpreter (it includes numpy) and the `.noi` symbol file from the same build (`build/rom/toronto-dispatch.noi` in the GB Studio build folder). Choose a new output directory inside `project/build`:

```sh
python scripts/capture_soundtrack.py \
  --rom project/build/<rom>.gbc \
  --symbols <build folder>/build/rom/toronto-dispatch.noi \
  --output project/build/audio-evidence/<new-name>
```

The script fails if a song is not the one loaded in its state, a channel is silent in any song, a sound check is silent, a silent check has samples, the brake hiss or a rev is missing, or effects hold CH1 for most of a drive. Its button sequence belongs to the 2026-10 boot and pause-menu flow; review it after gameplay changes.

References reviewed 2026-10-10: the pinned GBVM `music_manager.h/.c`, `sfx_player.h/.c`, `bankdata.h` and hUGEDriver (`hUGEDriver.h`, `hUGEDriver.asm`), GB Studio CLI 4.3's `ugeHelper.ts` song export and `buildMakeScript.ts`; public PyBoy 2.7.0 Sound API; [Pan Docs audio registers](https://github.com/gbdev/pandocs/blob/master/src/Audio_Registers.md). Runtime distribution notices remain in [DISTRIBUTION.md](DISTRIBUTION.md); the native build identity and remaining gates are in [BUILD.md](BUILD.md) and [TESTING.md](../TESTING.md).
