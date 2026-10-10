# World runtime and frame pacing audit

Reviewed 2026-10-02 against the native source before the persistent-world integration. This is a source audit and profiling plan, not a new emulator or hardware test. No runtime, build, browser or emulator state was changed by this audit.

Baseline files:

| Artifact | SHA-256 |
| --- | --- |
| Native ROM `project/build/toronto-dispatch.gbc` | `4db8413ab8ad8f7c20e9f1030632a0abcd323b9d512ddfcd29b77bb1be52e61f` |
| `TORONTO.c` | `f102562cc6da59b59805700389085a6a3de71cd1719c2505e2888c4aa915ff2a` |
| `td_ui.c` | `a64015bf0b1b5f83ca6c194bda23ce41c6d1b30daae96f1e85e7536b6d59fdd8` |

The [testing record](../TESTING.md) reports 59 engine updates during 120 video frames in a **pre-cleanup** ride sample, approximately 29.5 updates per second. The current file's ROM identity is recorded above; source inspection does not repeat that runtime measurement. Re-measure after changing the world runtime. A two-second sample does not certify whole-city pacing, and host emulator execution speed does not measure cartridge CPU performance.

## Findings

The full game clock now follows every VBlank while movement catch-up is bounded. This preserves deadlines during missed render frames; it does not make missed frames disappear. Maintain that separation while reducing the recurring work.

### Repeated world presentation inside motion catch-up

Baseline `TORONTO.c` lines 406–423 call `td_traffic()` inside each motion substep. At two VBlanks per update, that normally writes the six traffic actors and parked-car pose/frame twice before the engine renders once. Pedestrians then add six further pose/frame writes, and the player position is written once. This is 21 actor-position writes and 20 frame-setting calls per rendered update, although most traffic frames and the parked pose are unchanged.

`td_signal_stop()` scans up to nine horizontal junctions or eight vertical junctions for every relevant traffic vehicle. Red horizontal traffic can inspect 36 junction candidates per substep, or 72 with two catch-up steps; the vertical-red phase has a smaller eight-candidate scan. The green short-circuit means these maxima do not occur together. These are source work counts, not measured cycle costs.

Split traffic simulation from presentation. Advance only lane progress, speed, stop state and direction in each bounded motion substep. Present final positions once per render. Initialize fixed-heading traffic frames once and change frames only when a route turns or animation changes. A next-junction cursor can replace repeated full-list stop-line scans.

### Pedestrian identity changes with the player

Baseline lines 311–322 select the nearest road row from the player's `v` and an aligned 128-pixel segment from the player's `u`. Consequently crossing `u=127→128` shifts every generated pedestrian segment by 128 pixels. Crossing the midpoint `v=464→465` switches the selected road from Dundas row 400 to Queen row 528, also moving the whole group by 128 pixels. World time can remain unchanged while those identities relocate.

The comment about deterministic routes continuing off-screen is therefore broader than the implementation. The actors are a moving decoration pool attached to the player. The formula also recomputes six 16-bit phase remainders and checks a banked collision tile for every pedestrian every render. Raw `seconds*12` can wrap a 16-bit intermediate during a long session; a two-hour game should avoid phase discontinuities caused by that product.

Use fixed route identities, endpoints and phase. Keep six visible sprite slots if needed, backed by more world-route descriptors. Assign a slot only while its previous occupant is outside an expanded viewport and hidden; never relocate a visible occupant. Re-entry into a region should recover the route's world-time position rather than restarting at its endpoint. Advance a bounded route phase or compute a bounded phase from a clock remainder, avoiding large clock products. Pre-validate every route against actual native walking collision so normal presentation does not require a tile lookup for every person.

### The visible bus jumps on whole seconds

Baseline line 300 sets the bus's position to `80 + (seconds % 24)*32`, with a constant `v=176`. It stays at one coordinate between second boundaries, jumps 32 pixels each second, and jumps from the end back to the beginning when the period wraps. It is neither a continuous vehicle path nor the full researched 94 route. The current paid-transit state machine is a separate system and does not mean the visible bus has carried the player.

Use a collision-validated road polyline, current segment, bounded fractional progress, direction and dwell state. Carry any movement remainder across waypoint boundaries. Reverse or follow an authored return loop at terminals; never reset a visible bus to the beginning. Keep the fictional timetable as the service clock. Author any missing street link before drawing a bus through buildings or silently using an inaccurate connector. Record compressed route geometry separately from real TTC facts.

### The walker passes through moving traffic

Baseline line 340 checks walking tiles and the parked car only. Traffic reactions at line 303 and pedestrian/car reactions at line 322 expressly exclude `onfoot`. Dynamic road vehicles therefore do not block or react to the courier on foot.

Check proposed walking movement against the nearby traffic footprints, using the same ground coordinates and current substep positions as vehicle simulation. Use a small walking footprint matched to the courier art, vehicle-specific dimensions, and an early broad-phase distance check. Reject or slide a blocked component instead of teleporting or trapping the courier. If a moving vehicle reaches a stationary walker, brake it or apply a bounded, collision-checked displacement with a cooldown. Preserve the parked-car entry exception only during its explicit entry animation. These checks must cover bus turns and route boundaries too.

### Engine work continues for persistent actors

The scene copies 14 additional actors and marks them persistent (baseline lines 394–399). GBVM `actors_update()` still visits persistent off-screen actors; it sets their disabled flag rather than removing them from the active list. `actors_render()` skips disabled/hidden sprites but still traverses the actor list. Each visible actor switches to its sprite bank and presents its metasprite. A sprite pool bounds hardware usage, but hidden sprites do not make all scene/engine work free.

The current 15 total actors are within the 21-element engine array. Optimize repeated scene work first. If list maintenance is changed later, preserve the native active/inactive linked-list contract and verify activation when entering/leaving a viewport. Do not reduce the release's living world merely to obtain a passing benchmark.

### Checkpoint/UI work can cause spikes

`td_second()` saves at its end and also saves on some boarding, arrival and expiry branches. A normal second serializes 48 state bytes and runs CRC16 over the record; a transition can perform this twice. `td_ui_draw()` formats the active HUD each second or notice. These periodic costs can cause a spike, but source inspection cannot attribute a steady 30 Hz rate to them.

The UI already caches rows, uploads window attributes once, and avoids unchanged tile uploads. Do not describe it as redrawing the entire screen every frame. Coalesce duplicate saves in the same second while preserving immediate fare/cancellation/checkpoint durability. Preserve the dual-slot commit invariant. A table or nibble CRC implementation is a possible later optimization only if profiling identifies CRC as significant and the existing interruption/known-vector tests remain green.

## Engine evidence and limits

The pinned GBVM engine is `bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3`, compatibility `4.3.0-e1`. Relevant inspected sources:

- [`core.c`](https://github.com/chrismaltby/gbvm/blob/bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3/src/core/core.c), lines 61–99: state update, camera, scroll, actors, UI and collision work precede `game_time++` and `wait_vbl_done()`.
- The same file, lines 198–207: CGB detection calls `cpu_fast()`. This project is CGB-only. Missing double-speed initialization is not the source explanation for the sampled pacing.
- [`actor.c`](https://github.com/chrismaltby/gbvm/blob/bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3/src/core/actor.c), lines 84–165 and 201–253: actor-list traversal, persistent off-screen disabling and metasprite rendering.
- [`collision.h`](https://github.com/chrismaltby/gbvm/blob/bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3/include/collision.h), lines 64–67, and [`bankdata.h`](https://github.com/chrismaltby/gbvm/blob/bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3/include/bankdata.h), lines 112–114: tile reads compute the scene-array index and use a banked read. `ReadBankedUWORD` changes/restores the ROM bank in `src/core/bankdata.c` lines 134–155.
- [`scroll.c`](https://github.com/chrismaltby/gbvm/blob/bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3/src/core/scroll.c), lines 69–179: stationary camera work differs from tile-boundary crossing or large camera jumps. A stationary ride sample cannot establish driving/scrolling cost.
- [`game_time.h`](https://github.com/chrismaltby/gbvm/blob/bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3/include/game_time.h): `game_time` is **UINT8**, not a 16-bit clock. Native `sys_time` and the game clock are separate counters.

The linked ROM has software integer division/modulo/multiplication helpers, but their presence does not identify a hot path or its cycle share. Engine rendering, VM work, scrolling and interrupt/audio-driver work may also contribute. No function-level cycle profile was collected in this audit.

The baseline ride path skips `td_drive()`. Its four-corner vehicle collision reads, slide retries and walking checks therefore cannot explain a sustained stationary ride slowdown. Car-entry line checks are interaction costs rather than recurring ride work. Profile the shared world presentation, engine actor/VM work and periodic checkpoint/UI work first for that case, then profile driving separately.

## Safe native profiling plan

1. Build the intended project with the official plugin and fresh debug artifacts. Identify the exact ROM and matching symbols; offsets can move after each build. Start a separate owned native recording. Preserve the unresolved browser session and any existing user play.
2. Use ordinary input to reach comparable standing-car, walking, waiting, riding, clear-road driving, continuous-turn and curb-contact cases. Do not inject progress or write emulator memory. Begin/end samples with genuine observation; preserve every button set and frame count.
3. For bounded windows of at most 240 video frames, read one byte of `game_time` and calculate its delta modulo 256. This avoids ambiguity from its wrap. Compare against `sys_time`/video-frame delta. A longer sample needs a wider project-local update counter or shorter consecutive windows. Label each interval with its actual mode; arrival during the interval changes the workload.
4. If attribution is needed, add a development-only WRAM metrics structure in the scene extension: update count, elapsed-frame histogram, traffic simulation/presentation counts, tile reads, changed actor frames, saves and UI-row uploads. Keep it outside the serialized player state. Read it with bounded inert `emulator_inspect` calls. Counts identify work and missed deadlines; they are not instruction-cycle measurements.
5. For stronger attribution, use separate explicitly labelled diagnostic builds that disable one work group at a time while keeping the same input and starting condition. Restore all world features in the candidate release and rerun the native cases. A faster diagnostic build with fewer pedestrians is not evidence that the full game is optimized.
6. Do not repurpose the hardware timers, reset the divider, disable interrupts, patch the installed plugin/toolchain or edit emulator internals. The public plugin diagnostics do not expose a general CPU-cycle profiler. Coarse `sys_time` readings around a work group can detect VBlank crossings but cannot prove a sub-frame duration. Use the supported counters and repeatable before/after native windows first.
7. Inspect genuine consecutive frames and OAM in crowded cases, checking 40-object total and 10-object scanline limits. Check motion distance and game-time equivalence as well as render-update counts; a higher count obtained by slowing the simulation is a regression.
8. Repeat on the final full-feature ROM and, once hardware is connected, the cartridge. Preserve handling, deadlines, pause, fare/cancellation persistence, world-route identity and sprite visibility tests. A native emulator improvement remains separate from physical frame pacing.

## Acceptance checks for the world integration

- A visible pedestrian's position changes only by its bounded route movement, even when the player crosses a 128-pixel boundary or a nearest-street midpoint.
- A route exits and re-enters the viewport at the position implied by the continuing world clock; pool reassignment happens only off-screen.
- Bus travel is continuous through authored turns and dwell periods; no whole-second jumps or visible terminal reset.
- The courier cannot pass through a moving car, bus or parked vehicle; collision/entry recovery cannot place the courier inside a building or water.
- World actors continue during waiting/riding and freeze during deliberate pause/map. Sidewalk/crossing routes and road lane footprints agree with native collision.
- Final actor poses are presented once per rendered update, with unchanged frames cached, while simulation/deadlines still account for every elapsed VBlank.
- Native pacing samples include quiet and crowded districts, walking, driving, transit and save transitions. Record unresolved hotspots rather than declaring whole-city performance from a single sample.

## Follow-up source review, 2026-10-02

The integrated runtime now separates traffic simulation from presentation, caches unchanged actor frames, follows continuous road waypoints, and assigns fixed pedestrian routes without replacing visible identities. Walking checks occupied traffic footprints; road users yield to roaming and waiting couriers. Exit placement rejects occupied doors. The car collision checker reads every overlapped tile, including narrow rails that four corner samples could miss. The swept corner assist preserves heading and dominant momentum, rejects brake/reverse and broad walls, checks intermediate lateral footprints, and is limited by a flag reset once per rendered update rather than a wrapping VBlank timestamp.

Two additional transition defects were reproduced with the real C engine: simultaneous entry/transit input or a paused transit selection could suspend entry until after arrival at a distant stop, and a previous failed contract could make a new free-roaming transit arrival display another failure result. New regression fixtures produced **4 failures in 471 checks** before correction. Entry now excludes transit until its animation finishes; a newly boarded trip without an active contract clears the old failure condition. An already-paid trip whose own parcel expires retains its failure until arrival. Root's subsequent `make check` reported **471 checks and zero failures**. This is host logic evidence, separate from native input, timing and cartridge behavior.

Read-only ROM/symbol inspection of `038f1561a52b9abaa943111ba4fc215c400bac19d0e5894f921aa23663e17381` confirmed the audio HOME wrapper resolves music bank 8, calls the near `music_pause` at `8:6C96`, and restores the previous bank. Audio assets use bank 7. No ABI or channel-priority hazard was found. The two later transition corrections are not native verification of that older ROM; use the new build's identity and tests when packaging it. The [testing record](../TESTING.md) separately records actual emulated PCM and silence checks; human listening and physical audio remain separate gates.

Performance remains open. The same testing record reports 59 engine updates in 120 video frames in the bounded final waterfront sample, approximately 29.5 updates per second. The shared world work is more coherent, but that observation does not demonstrate higher frame pacing. Signal-list scans, periodic route-slot searches, save/CRC work, engine actor traversal, scrolling and music interrupts still need representative native attribution. Coalesce redundant checkpoints only while retaining fare/cancellation durability and per-second paid-trip recovery.

No further concrete steering or audio blocker was identified in the reviewed source. Handheld handling, crowded-scene pacing, physical SRAM/audio, complete campaign duration, all eight job types, full Old Toronto coverage, moving streetcars and visible transit boarding/riding remain acceptance or scope work; this review does not certify the requested full release.
