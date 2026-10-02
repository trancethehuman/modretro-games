# Testing record

## Scaffold — 2026-10-01

Repository/content checks passed on initial setup and were rerun after research updates. The validator checks structure, source references, unique content identifiers, endpoint references, vehicle compatibility, and sensible mission values. It does not verify geographic placement, map reachability, handling, or engine integration.

## Game and hardware evidence

| Check | Status | Evidence |
| --- | --- | --- |
| Valid GB Studio project created | Passed 2026-10-02 | Plugin-created native distributed project; health inspection reported zero errors/warnings before starter boot text |
| Dependency/toolchain versions | Passed 2026-10-02 | Official preparation and doctor checks; versions in [BUILD.md](docs/BUILD.md) |
| ROM build and digest | Passed 2026-10-02 | GB Studio CLI `make:rom`, 64 KiB CGB-only ROM; digest below |
| Emulator boot and controls | Passed for setup room 2026-10-02 | PyBoy boot, two text pages, movement, basic room boundary and escape; detailed steps below |
| Browser emulator preview | Passed for setup room 2026-10-02 | Official `make:web` export; workshop visibly running in Codex's built-in browser, retained for user play |
| Vehicle physics and collision | Pending | Design only |
| Complete/failed/retried missions | Pending | Content definitions only |
| Live Chromatic streaming | Pending | Hardware readiness unverified |
| Cartridge write and read-back | Pending | No write attempted |
| Physical cartridge boot/gameplay/audio | Pending | Requires connected supported cartridge and user observation |

Record date, commit/build digest, relevant tool versions, steps, observed outcome, and limitations for each real test. Never replace a pending result with an inference from source code or scaffold CI.

## Setup ROM — 2026-10-02

Project: `project/project.gbsproj`, single `Development boot` scene. Final script revision after adding the boot text: `5c5745bf5c2ba784e480b95f5798028288a660f602d1466b3f7c0f7ea6572f0f`. Built during bootstrap; native source and asset notices were committed as `20d1e08`. A final project health inspection after documentation/provenance additions also reported zero errors and warnings.

Native ROM: `project/build/toronto-dispatch.gbc`, 65,536 bytes, SHA-256:

```
830a79e3b9b710820d16bc279c24dd4c86c69c8c752c09f9a7edd251fac23f59
```

Header inspection: valid Nintendo logo and header checksum; title `TORONTODISPATCH`; CGB-only; MBC5+RUMBLE+RAM+BATTERY; 32 KiB declared RAM. Build exited successfully in about 10 seconds. An upstream Node `DEP0190` deprecation warning was emitted; no compile error was reported. Cartridge compatibility is unverified.

PyBoy 2.7.0, CGB mode, exact native frame sequence:

1. Boot through frame 180 with neutral input. Observed `TORONTO DISPATCH / SETUP BUILD / PRESS A` in the workshop.
2. Hold A for 60 frames, reaching 240. Observed `CITY AND DRIVING / COMING NEXT. / TEST ROOM ONLY.`
3. Release for 15 frames, then hold A for 60, reaching 315. Dialogue closed and player became visible.
4. Hold right for 30 frames, reaching 345. Player moved right.
5. Keep right held for 150 frames, reaching 495. Samples at frames 420 and 495 showed the player stationary at the right room boundary.
6. Hold left for 30 frames, reaching 525. Player moved away from the boundary. Release for one frame; close the owned emulator and retain its recording.

Recording and genuine frame PNGs are local in ignored `project/build/setup-playtest/`. This proves boot text, basic movement and the starter room's right boundary only. It does not test car collision, isometric controls, delivery logic, performance under traffic, save persistence or audio.

The official browser export is a separate build in `project/build/web/`, with its own 64 KiB ROM digest:

```
a3249acdebf3e840592528e02a719f636f5d77273e287fc112a9deafeb9d81fd
```

Web preview source revision: `d6f17afcdfeb6dbc1e840926cc4148f0589c116637dc91c852c2467462be175d`. A screenshot showed the workshop running in the plugin's Chromatic-style browser player. Local capability URLs and browser save states are not published. Native frame tests above apply to the native ROM, not automatically to this different browser-export ROM.

## Native city and user revision — 2026-10-02

The user explicitly replaced isometric presentation with a perpendicular north-up view, retaining momentum/braking and requesting wider roads, larger city space, pedestrians, visible walking/car entry and proper building occlusion. The scene extension, original assets, native collision grid and runtime campaign now implement this prototype.

### Current build

`project/build/toronto-dispatch.gbc`: 262,144 bytes, CGB-only, MBC5+RUMBLE+RAM+BATTERY, 32 KiB declared SRAM; logo/header checks passed. SHA-256:

```
9eaec68843e2888b0aac4accb6e651270c438592719597c02f9157fc6660e793
```

GB Studio CLI 4.3.2 / engine 4.3.0-e1 / GBDK 4.5.0, PyBoy 2.7.0. Build succeeded. Upstream Node DEP0190 and four SDCC conditional-flow optimizer warnings remain; no compile error. Project inspection before the final transit-only correction reported zero health errors/warnings.

Original background: 1,024 × 976 pixels, 15,616 tiles, 80 buildings and six architecture styles. Plugin analysis counted 200 exact / 151 flip-deduplicated patterns, below the CGB 384-pattern budget. Source pattern counts do not establish sprite scanline performance; actual OAM samples are described below. The map remains compressed central Toronto and Island service areas, not verified full Old Toronto.

### Retained native recordings

| Recording / ROM SHA-256 | Actual checks |
| --- | --- |
| `north-up-playtest-v2` / `d1a84c60ffe0ecb58c3e830c73cf0d3d03a7503d7a76915bb9e1a3055c95fea5` | Package delivery, pedestrians, parking/entry, steering momentum, paid subway/bus/ferry, Island water boundary, map pause, save recovery and fragile-job timeout/retry |
| `north-up-final-smoke` / `47e9de4d986e19229d977364499f8d27be4ec5b513e14c86037673237ed1551b` | Final 60-building art: delivery, walking, building boundary/roof occlusion, sampled OAM budget and paid ferry beside Centre Island pavilion |
| `north-up-published-smoke` / current digest above | 80-building density update: boot, delivery, walking, wall boundary and roof occlusion |
| `north-up-final-v3` / `f9c03d2369b282b7970c82d650ccaadbce2135ba809df852045992ae84c28f7a` | Delivery regression, leftward ferry destination cycling to Ward's Island, pause while waiting and riding, resumed arrival with one fare |

All inputs were ordinary emulator buttons. WRAM/OAM inspection was read-only. No emulator memory writes, fabricated frames or injected progression were used. Native state `_td` was observed at WRAM offset 509 for these builds. Frame timings refer to emulator video frames; engine updates may skip render frames, so 8-frame presses with neutral intervals were used for reliable menu tests.

The broader v2 recording ended at frame 30,343, with 2,095 journal events and digest `5c47fa5a0d24ccb59b177e76990d3fa692a3e5ec1b32b05fec24c9bb05677275`. Observations:

- First contract collected at Union (560,720), delivered near St. Lawrence (774.25,720): unique completed count 1, cash 30 → 203, cargo 100. Re-entered free roaming.
- Parking displayed the courier beside the parked vehicle. Walking back and pressing A animated the approach/open door, then restored driving at the parked position; `onfoot` changed 1 → 0.
- During a turn with acceleration held, speed remained 17 Q4 units, heading 13 and position advanced. This checks the observed corner; further handling tuning is still needed.
- Line 1 reached Queen and Wellesley; switching at Wellesley to the 94 bus charged 2 credits and reached the compressed Ossington stop (144,64). Train fare was 3. Ferry fare was 4 and Centre Island arrival was (720,920). Walking north on the Island stopped at its shoreline.
- Map panning moved the view while the full game state and clock remained unchanged at frames 20,529 → 20,769. Pause's save restored position (806.625,624.375), cash 179 and unique count 1 after the engine's soft reset. The immediate post-save roaming sample had moved slightly while coasting in reverse; the restored position matches the position at the save command. Physical power-off persistence remains unverified.
- A fragile contract timed out with `job=none`, cargo 0 and RESULT mode, while cash/completed count remained unchanged. A returned to the dispatch board for retry.
- One crowded OAM sample reported 16 visible hardware objects, peak 8 on a scanline, zero over-limit scanlines. This is a sample, not a whole-city performance certification.

Final art recording ended at frame 4,717, 519 events, digest `d694d15868edeff053a3f8a593fd8fb44ddadabdac74235362798816f6e1eb1b`. At (769.8125,663.5), holding down for 180 frames did not penetrate the building footprint; genuine frames showed the roof lip occluding the lower part of the courier. At frame 1,374, OAM peak was 4 with no over-limit scanlines. Ferry arrival on Centre Island charged 4 and rendered the new pavilion. The last build changed transit pause/resume and destination cycling only; it retained the same city/sprite assets and collision grid.

In the transit-correction build, waiting pause at frames 1,250 → 1,490 kept the state/clock identical. After boarding, fare reduced cash 203 → 199; pausing the ride at frames 2,634 → 2,874 kept clock, position, fare and seven remaining ride seconds unchanged. Resuming arrived at Ward's Island (848,896), `onfoot=1`, with unique completion count still 1. That recording's final identity is recorded alongside genuine screenshot provenance in `docs/screenshots/provenance.json`; full journals remain ignored locally.

### Checks and limits

`make check` now validates 72 native contracts, 24 stop footprints, actual road/pedestrian collision connectivity plus ferry links, unlock availability, vehicle-required endpoints, generated C consistency, architecture styles, roof priority flags and single-bank map-array size. It does not prove every quest can meet its deadline, geographic survey accuracy, two-hour duration or fun.

The official web export previously compiled but preview replacement failed with `Browser recording close acknowledgement is UNKNOWN`. The listener remained unresolved. The old starter state and recording were preserved; the top-down build is **not browser-verified**. The plugin's authoring instructions require resolving unknown outcomes before arbitrary reload/replay.

Audio, full campaign/unlock playthrough, two-hour duration, full Old Toronto coverage, whole-city frame pacing, physical streaming, cartridge write/read-back and cold-boot save recovery remain pending. No physical cartridge operation was attempted.

The published density update adds smaller properties in remaining street blocks, bringing authored buildings to 80. All 24 stop footprints and their vehicle/pedestrian/ferry connectivity were rechecked. Its native smoke recording ended at frame 1,374, 333 events, digest `dad5476df470a5febb1bcb1329a6f79885b362c2c339efc98f98f2cab54da3a2`; delivery again paid 203 total credits with one unique completion, and the walker stayed at (769.8125,663.5) against the same occluding roof lip. The engine remained unchanged from the transit-correction build.
