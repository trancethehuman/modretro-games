# Native build and preview

Updated 2026-10-02. The linked-district ROM is a candidate under native testing; the published single-scene milestone is identified separately below. Select `games/toronto-dispatch/project/project.gbsproj` through the ModRetro Chromatic plugin before issuing project operations. The format is native GB Studio distributed resources (`.gbsproj` / `.gbsres`), not a hand-built interchange stub.

## Tested toolchain

| Component | Version / identity |
| --- | --- |
| ModRetro Chromatic plugin | 1.0.33 |
| GB Studio CLI | 4.3.2, commit `ccb891b2670134ba8237416772eea4ed09d34e1e` |
| GBVM engine | `bd6f41cc5e05cbe6601dcc7f8e2db89bed527fe3` |
| GBDK | 4.5.0, macOS arm64 |
| Python / PyBoy | 3.13.12 / 2.7.0 |
| Pillow | 12.3.0 (plugin emulator); 12.1.1 (repository art/content scripts and CI) |
| Node.js | 26.6.0 |

The plugin manages dependencies outside the repo. Start with its setup skill and `toolchain_doctor` for authoring, projectBuild and play. Prepare missing build/emulator components using `toolchain_prepare`; do not modify the installed plugin cache.

## Plugin operations

1. Select the existing native project, inspect its health, then edit native scenes/scripts with the plugin's revision-aware tools.
2. Build using `rom_build` with `outputPath: "build/toronto-districts.gbc"` for the current expansion candidate. Paths are relative to the selected project's directory. The plugin invokes GB Studio CLI `make:rom`. Track each output's identity; an older `toronto-dispatch.gbc` does not establish this candidate's behavior.
3. Inspect ROM headers and digest, then run the explicit native memory guard below against that build's `symbols.noi`. A successful compile alone does not establish a safe WRAM layout.
4. Run `emulator_run` on that exact ROM, then use `emulator_step` / `emulator_observe` to test native frames. Retain recordings in ignored `build/`.
5. Use `web_preview` to create the official GB Studio / Binjgb export (`make:web`) and open its returned URL in Codex's built-in browser. Keep the user preview available; do not reload during human play. Browser export and native ROM may have different digests and must be tracked separately.
6. Run `make check` from the repository root for content/connectivity checks and host engine regressions. It needs Python 3.10+ and Clang/GCC with ASan/UBSan. It does not compile a ROM or automatically inspect an ignored native build.

Builds, browser save states and cartridge backups are ignored. Do not publish local preview capability URLs, personal machine paths, activation codes or saves.

## City engine and reproducible sources

Three linked 1,024 × 976 scenes use the original project-local `TORONTO` scene extension, compatible with GBVM `4.3.0-e1`: `scene_toronto_city` (district 0), `scene_toronto_west` (1) and `scene_toronto_high_park` (2). It adds engine files without ejecting or vendoring GB Studio. `Development boot` remains a separate workshop scene. [DISTRICT_ENGINE_PLAN.md](DISTRICT_ENGINE_PLAN.md) describes compiled scene binding and the genuine VM change-scene bridge.

From the repository root, with Python and Pillow available:

```sh
python3 games/toronto-dispatch/scripts/create_city_art.py
python3 games/toronto-dispatch/scripts/sync_city_resources.py
python3 games/toronto-dispatch/scripts/create_west_art.py
python3 games/toronto-dispatch/scripts/create_district_world.py
python3 games/toronto-dispatch/scripts/create_district_jobs.py
python3 games/toronto-dispatch/scripts/create_campaign.py
python3 games/toronto-dispatch/scripts/create_world_routes.py
python3 games/toronto-dispatch/scripts/create_audio.py
make check
```

`create_city_art.py` draws original indexed-colour background and sprite source cells; the background is already registered as a native asset. The sprite generator produces an editable source/metadata pair in `original-art` / `dispatch_topdown.metadata.json`. Existing sprite PNG changes must be applied to the registered `assets/sprites/dispatch_topdown.png` as part of a deliberate sprite edit. New sprite registration uses the plugin's validated `native_metadata` import; keep the existing root and bindings when editing an established asset.

`create_west_art.py` draws the original west/High Park backgrounds and writes placement, collision and priority metadata. It does not register or update their native scene resources. After changing their geometry, apply the new collision and attribute data through the plugin's native resource workflow before regenerating routes/contracts; the checked-in registered scenes are the input to those checks. `create_district_world.py` generates reciprocal seams and traffic loops; `create_district_jobs.py` authors eight appended package contracts from actual scene collision paths. `create_campaign.py` compiles 80 contracts and 35 stops while retaining the original IDs.

`sync_city_resources.py` uses GB Studio's native byte-array RLE to write palette/background-priority attributes and the core scene collision map. Native CGB attribute bit 7 marks raised roof lips/canopies. The scene extension interprets collision values 0 as road, 16 as walk-only pavement/Island ground and 15 as solid; the normal engine ladder meaning of bit 4 does not apply to this custom scene. Each district has 15,616 tiles, so each tile/attribute/collision array fits one 16 KiB bank. `check_campaign.py` and `check_district_world.py` inspect registered resources for stop connectivity, compatible road routes, ferry links, reciprocal seams, traffic clearance and contract consistency.

Car physics stores local Q4 coordinates and smoothed velocity; GBVM actors/camera use Q5. Motion catch-up is bounded independently from the full 16-bit VBlank clock; menus freeze the clock. Input edges are consumed once per render, and continuing curb contact preserves forward momentum. The UI uploads unchanged rows only once. Banked `td_routes.c` keeps the full pedestrian tables in ROM and selects six routes into a 24-byte WRAM coordinate cache; visible slots retain their route identity.

Save version 6 serializes 58 bytes, including a 16-byte completion bitmap and separate player/parked-car districts. It alternates two 66-byte CRC16-checked records in SRAM bank 3 at offsets 0x100 and 0x180; GB Studio uses banks 0–2. The new record commits its magic byte last, preserving the old checkpoint until then. Progress checkpoints every game second; waiting/boarding/cancellation is saved explicitly. Version-5 saves are decoded in their original 48-byte layout, preserving original active work and paid transit in district 0. Valid version-4 prototype saves retain cash/completions but retire an active contract because its route changed. Earlier formats are rejected. Host interrupted-store tests are separate from native reset and physical persistence. The current candidate passed one remote foot/parked-car soft-reset recovery; active-contract recovery also passed in west; paid-trip reset and physical recovery remain further checks.

`td_session_live` is an internal UBYTE engine field at default 0. Stock generated bootstrap resets it on cold/soft boot; ordinary district changes preserve it. Session restore/audio initialization therefore run once, while each loaded scene rebuilds its local actors and UI. Keep its project setting at 0.

The source fixes `VM_MAX_CONTEXTS` at 8 through a project-local `cType: define` field targeting the copied `include/vm.h`. This is the stock compiler's file-backed engine-field workflow; it does not modify the installed toolchain. The shared heap remains 768 words and each context stack 64 words. Restore validates one 58-byte candidate at a time and migrates legacy tails in place to reduce native CPU-stack use. Current authored scenes have no concurrent actor/trigger scripts; reassess the fixed context pool before adding scripted entities. These changes are compiled into the candidate identified below and passed its remote reset replay.

## Native memory guard

After each plugin build, run from the repository root:

```sh
python3 -B scripts/check_rom_memory.py --min-stack-reserve 1024 \
  games/toronto-dispatch/project/build/toronto-districts.gbc.debug/symbols.noi
```

Stock GBVM reserves the `DF00–DFFF` page for its second OAM buffer, palettes and text tiles, and starts the downward CPU stack at `.STACK=DF00`. The checker rejects linker-area overlap with those absolute buffers, inconsistent/missing symbols and heap ends at or above the stack. The command additionally requires 1,024 bytes of stack reserve; this project threshold does not measure the actual deepest native call path.

The first booting expanded candidate `36119ebf…` had heap end `DDA7`, stack base `DF00` and **345 bytes** of reserve. An earlier full-table cache ended at `DF90` and corrupted the reserved OAM page before Toronto initialized. Keeping only six coordinate pairs removed that allocation overlap, but the first booting candidate later failed a remote soft reset. The current eight-context build ends at **D90F**, leaving **1,521 bytes** below `DF00`, and passed the native reset replay. Allocation checks and reset evidence remain separate: neither establishes physical persistence or every deepest call path. `make check` runs checker regressions, while the explicit command inspects the actual newly linked ROM.

## Linked-district candidate under test

| Identity | Value |
| --- | --- |
| Native output | `project/build/toronto-districts.gbc` |
| Candidate ROM SHA-256 | `99eb430cc59cbb51631d343a4b626d07db03ff10ad36dd567128b438d36c528f` |
| Build source fingerprint | `93038e0d626669ee1d1b6809ebd68b492997c28a931300e3e59bf9e1eb207f08` |
| Native NOI SHA-256 | `2e8fca83ec54ad8517f1b6125710bc388b6952b8b9144c0b4e24133321e50526` |
| Native scenes | Core 0, west 1, High Park 2; each 1,024 × 976 |
| Compiled content | 80 contracts / 35 stops |
| Save schema | Version 6; 58-byte state |
| Memory guard | `D90F` heap / `DF00` stack / 1,521-byte reserve |

This CGB-only candidate is 262,144 bytes (256 KiB). Its official build resolves all three scene and collision resources. The ModRetro plugin confirmed clean boot, first-delivery/held-acceleration turning, a driving transition from core 0 to west 1 and walking from west 1 to High Park 2. A soft reset then restored the courier on foot in the actual High Park scene while retaining the parked vehicle in district 1, cash and world clock. Native checks are continuing; this candidate has not established all portals, complete campaign progression, two hours of gameplay, full Old Toronto coverage or physical cartridge behavior. Build-specific playtest details belong in [TESTING.md](../TESTING.md).

`make check` passed with **1,212 host engine checks, 710 independent district bridge checks and nine memory-guard regressions**, alongside repository/content/generated-source validation. Those checks use host hardware stubs and do not replace the native scenarios above. Source fingerprints and symbol digests identify build inputs/artifacts; they are not Git commits.

## Published single-scene milestone

Milestone identifier: `v0.2.0-prototype.2`. This earlier ROM includes the car-entry/transit transition fixes and uses save schema 5. It is CGB-only, 262,144 bytes (256 KiB), MBC5+RUMBLE+RAM+BATTERY with 32 KiB declared RAM. Its official build/header checks passed; physical cartridge compatibility remains unverified. Download available bundles from the [releases page](https://github.com/trancethehuman/modretro-games/releases). Its results below do not establish the linked-district candidate's behavior.

| Identity | Value |
| --- | --- |
| Native output | `project/build/toronto-dispatch.gbc` |
| ROM SHA-256 | `a2f00db4ef834112a3491e50cec832653023a0456f0d9cbca6d2386be7322a59` |
| Build source fingerprint | `439c22c598c9b82687ee3c8eb19560456948afd37e4184749dfe2e79397c9d33` |
| Plugin project revision | `483aa222d1a1785bdfca3df6674e0232c13b2a0a544879ce76ced3f2bd315c3e` |

The fingerprint and plugin revision identify build/project state; neither is a Git commit. A ROM bundle's `BUILDINFO.json` records its separate source commit, pinned tool versions and binary digest. A later build must be inspected and tested under its own identity.

At that milestone, `make check` passed with **471 host engine checks and zero failures**, including audio integration and entry/transit transitions, alongside repository/campaign/generated-source validation. These checks cover host logic with hardware stubs; they do not establish native sound, physical behaviour or cartridge performance. Native plugin checks passed for representative first-delivery, corner, car-entry, transit-pause and failed-job flows, followed by transit checks on that ROM. [TESTING.md](../TESTING.md) maps the actual scenarios to their exact build identities.

Original music and engine/brake/event/transit effects are compiled into both milestones. The pause menu cycles music + effects, effects only and silent; the preference defaults per boot. A separate public PyBoy 2.7.0 PCM run of the single-scene ROM identified above passed all eight interval checks: nonzero music, acceleration, braking, effects-only acceleration and resumed music, and all-zero measured silent/menu/paused-effects-only intervals. [AUDIO.md](AUDIO.md) records sample counts, peaks, limitations and the opt-in reproduction command. Those captures do not establish sound continuity in the new districts. No human listening or physical sound check is claimed.

No Chromatic was connected during the latest discovery; no stream, cartridge write/read-back or cold boot is verified. The full campaign, two-hour gameplay target, full Old Toronto coverage and whole-city performance remain open gates. Follow the [loading instructions](LOADING.md) and [hardware workflow](../../../docs/HARDWARE.md) before any write.

## Browser refresh limitation

A previous official web export compiled, but refreshing the owned preview failed with `Browser recording close acknowledgement is UNKNOWN`. The listener retained unresolved closure state; its paused starter save and local recording evidence were preserved. Native gameplay testing proceeded independently. Do not arbitrarily reload or replay that unresolved close operation. Resolve the plugin's preview state before treating the current city ROM as browser-verified. The new top-down ROM has not been verified in that browser view.
