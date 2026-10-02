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
