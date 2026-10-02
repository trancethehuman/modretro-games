# Native build and preview

Updated 2026-10-02. The published Queen streetcar milestone (`23b2a7a2…`, Prototype 6) has compiled and passes scoped native driving, scheduled streetcar, subway, bus, ferry, map, reset and safe-alighting checks. It retains four compressed scenes and 88 contracts, and adds eight service points. Prototype 5 (`2d1f6e4e…`) is the preceding published city-atlas milestone. Earlier builds retain their separate identities and evidence below. Select `games/toronto-dispatch/project/project.gbsproj` through the ModRetro Chromatic plugin before issuing project operations. The format is native GB Studio distributed resources (`.gbsproj` / `.gbsres`). Full former Toronto, two measured hours of varied gameplay and physical cartridge acceptance remain open.

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
2. Build using `rom_build` with `outputPath: "build/toronto-pickup-condition.gbc"` for the current pickup-condition candidate, and `captureDebugArtifacts: true` for authenticated same-build symbols. Paths are relative to the selected project's directory. The plugin invokes GB Studio CLI `make:rom`. Track each output's identity; earlier output paths identify separate builds and do not establish this candidate's behavior.
3. Inspect ROM headers and digest, then run the explicit native memory guard below against that build's `symbols.noi`. A successful compile alone does not establish a safe WRAM layout.
4. Run `emulator_run` on that exact ROM, then use `emulator_step` / `emulator_observe` to test native frames. Retain the returned recording paths; stop, close and archive through the public plugin without deleting the original evidence.
5. Use `web_preview` to create the official GB Studio / Binjgb export (`make:web`) and open its returned URL in Codex's built-in browser. Keep the user preview available; do not reload during human play. Browser export and native ROM may have different digests and must be tracked separately.
6. Run `make check` from the repository root for content/connectivity checks and host engine regressions. It needs Python 3.10+ and Clang/GCC with ASan/UBSan. It does not compile a ROM or automatically inspect an ignored native build.

Builds, browser save states and cartridge backups are ignored. Do not publish local preview capability URLs, personal machine paths, activation codes or saves.

## City engine and reproducible sources

Four linked 1,024 × 976 scenes use the original project-local `TORONTO` scene extension, compatible with GBVM `4.3.0-e1`: `scene_toronto_city` (district 0), `scene_toronto_west` (1), `scene_toronto_high_park` (2) and `scene_toronto_east` (3). Their logical atlas is 4,096 × 976, with 211 buildings and 486 fixed pedestrian routes; only six nearby pedestrian actors are active in the loaded scene. These are compressed districts, not full former Toronto coverage. The extension adds engine files without ejecting or vendoring GB Studio. `Development boot` remains a separate workshop scene. [DISTRICT_ENGINE_PLAN.md](DISTRICT_ENGINE_PLAN.md) describes compiled scene binding and the genuine VM change-scene bridge.

From the repository root, with Python and Pillow available:

```sh
python3 games/toronto-dispatch/scripts/create_city_art.py
python3 games/toronto-dispatch/scripts/sync_city_resources.py
python3 games/toronto-dispatch/scripts/create_west_art.py
python3 games/toronto-dispatch/scripts/create_east_art.py
```

These commands generate original artwork/metadata and synchronize existing core resources. They do not register new scenes or apply changed west/east collision and attribute resources. Use the plugin's revision-aware native workflow to register/update those assets and scenes, preserve bindings, and apply the intended reciprocal core seams before continuing. Regenerating content against stale native geometry is not a valid build procedure. The current candidate already has all four native scenes registered.

After the registered resources match the authored geometry:

```sh
python3 games/toronto-dispatch/scripts/create_district_world.py
python3 games/toronto-dispatch/scripts/create_district_jobs.py
python3 games/toronto-dispatch/scripts/create_east_jobs.py
python3 games/toronto-dispatch/scripts/create_campaign.py
python3 games/toronto-dispatch/scripts/create_world_routes.py
python3 games/toronto-dispatch/scripts/create_audio.py
python3 games/toronto-dispatch/scripts/create_atlas.py
make check
```

`create_city_art.py` draws original indexed-colour background and sprite source cells; the background is already registered as a native asset. The sprite generator produces an editable source/metadata pair in `original-art` / `dispatch_topdown.metadata.json`. Existing sprite PNG changes must be applied to the registered `assets/sprites/dispatch_topdown.png` as part of a deliberate sprite edit. New sprite registration uses the plugin's validated `native_metadata` import; keep the existing root and bindings when editing an established asset.

`create_west_art.py` draws the original west/High Park backgrounds; `create_east_art.py` draws the original eastern background. Both write placement, collision and priority metadata. They do not register or update native scene resources. After geometry changes, apply collision and attributes through the plugin before regenerating routes/contracts; the checked-in registered scenes are the input to those checks. `create_district_world.py` generates 14 reciprocal seam pairs and 18 non-core traffic loops. `create_district_jobs.py` authors eight western package contracts from actual scene collision paths; `create_east_jobs.py` appends eight eastern contracts and eight service points while pinning the earlier 80-contract/35-stop prefix. `create_campaign.py` compiles 88 contracts and 51 stops while retaining the original IDs; supplemental Queen platforms use `content/streetcar.json`. Both art generators and the eastern job generator support `--check` for read-only freshness checks; `make check` includes the generated-source checks.

`sync_city_resources.py` uses GB Studio's native byte-array RLE to write palette/background-priority attributes and the core scene collision map. Native CGB attribute bit 7 marks raised roof lips/canopies. The scene extension interprets collision values 0 as road, 16 as walk-only pavement/Island ground and 15 as solid; the normal engine ladder meaning of bit 4 does not apply to this custom scene. Each district has 15,616 tiles, so each tile/attribute/collision array fits one 16 KiB bank. `check_campaign.py` and `check_district_world.py` inspect registered resources for stop connectivity, compatible road routes, ferry links, reciprocal seams, traffic clearance and contract consistency.

Car physics stores local Q4 coordinates and smoothed velocity; GBVM actors/camera use Q5. Motion catch-up is bounded independently from the full 16-bit VBlank clock; menus freeze the clock. Input edges are consumed once per render, and continuing curb contact preserves forward momentum. The text UI caches unchanged rows. Banked `td_routes.c` keeps the full pedestrian tables in ROM and selects six routes into a 24-byte WRAM coordinate cache; visible slots retain their route identity.

`create_atlas.py` reads the four registered collision grids, authoritative district offsets and authored water masks, then writes `content/atlas.json` and the BANKED `td_atlas.h` / `td_atlas.c` API. It changes no scene or collision resource. The 512 × 122 schematic uses one map pixel per native 8 × 8 collision tile; its 20 × 12-tile viewport browses the four areas without changing the loaded gameplay scene. Generated source checks prove all 225 possible viewports fit the 172-slot ground cache. The renderer shares the existing 360-byte text cache, uses bounded double-hash pattern lookup and adds 28 transient WRAM bytes for view/actor/camera restoration. These runtime fields are absent from the unchanged 58-byte save schema. [CITY_MAP.md](CITY_MAP.md) documents coordinate units, focus controls and original water interpretation.

For bounded source/API/renderer checks without a ROM build:

```sh
python3 -B games/toronto-dispatch/scripts/create_atlas.py --check
python3 -B scripts/test_atlas.py
python3 -B scripts/test_atlas_ui.py
```

The API and renderer fixtures compile unchanged production C with host adapters and sanitizers. They do not establish native redraw timing, human readability or physical behaviour; actual build-specific samples appear below.

The recorded four-scene build moves district names, portals, graph traversal and non-core traffic tables into `td_world.c`, linked in ROM bank 15. District routing selects feasible hops for the current car/foot mode, then approach distance; it does not solve street-level routes. The HUD/beacon cache the next district, refresh when entering/leaving the car, and show `NO ROAD ROUTE` when no driving connection exists. Local foot-only handoffs retain `PARK THEN WALK`. Six traffic samples plus the next-district byte add 37 persistent WRAM bytes compared with Prototype 3; the 24-byte pedestrian coordinate cache is unchanged. Prototype 4 transit choices use the same departure window as boarding and display a countdown. Confirming during the two-second window boards immediately. The later Prototype 6 Queen module adds a fictional scheduled service across West/Core/East; neither milestone copies real timetables.

The banked auxiliary getter directs drivers to legal road parking anchors for Colborne 34, Withrow 36 and Greenwood 41, then restores the actual client target immediately on foot. It changes no client record or save field. Prototype 4 native Withrow play verifies the marker changing from road `(224,144)` to client `(320,144)` after exit, handoff and car re-entry. The atlas candidate retains this source, but its native remote parked-car and foot-only anchor/client marker samples remain pending. Colborne/Greenwood remain separate native handoff checks. The earlier `7a299125…` binary predates this fix.

Save version 6 serializes 58 bytes, including a 16-byte completion bitmap and separate player/parked-car districts. It alternates two 66-byte CRC16-checked records in SRAM bank 3 at offsets 0x100 and 0x180; GB Studio uses banks 0–2. The new record commits its magic byte last, preserving the old checkpoint until then. Progress checkpoints every game second; waiting/boarding/cancellation is saved explicitly. Version-5 saves are decoded in their original 48-byte layout, preserving original active work and paid transit in district 0. Valid version-4 prototype saves retain cash/completions but retire an active contract because its route changed. Earlier formats are rejected. Host interrupted-store tests are separate from native reset and physical persistence. Published Prototype 3 passed remote foot/parked-car, western active-contract and paid core-trip reset samples. Prototype 4 restores nine completions, cash/clock, a foot player in High Park and the car parked in the west after native reset. The atlas candidate separately verifies soft reset while its paid-trip map is open, recovery with remaining ride time and arrival without another fare. Physical persistence remains pending.

`td_session_live` is an internal UBYTE engine field at default 0. Stock generated bootstrap resets it on cold/soft boot; ordinary district changes preserve it. Session restore/audio initialization therefore run once, while each loaded scene rebuilds its local actors and UI. Keep its project setting at 0.

The source fixes `VM_MAX_CONTEXTS` at 8 through a project-local `cType: define` field targeting the copied `include/vm.h`. This is the stock compiler's file-backed engine-field workflow; it does not modify the installed toolchain. The shared heap remains 768 words and each context stack 64 words. Restore validates one 58-byte candidate at a time and migrates legacy tails in place to reduce native CPU-stack use. Current authored scenes have no concurrent actor/trigger scripts; reassess the fixed context pool before adding scripted entities. These settings remain compiled into the atlas candidate. Prototype 4's remote High Park reset and Prototype 3's earlier recovery evidence remain scoped to their separate binaries.

## Native memory guard

After each plugin build, run from the repository root:

```sh
python3 -B scripts/check_rom_memory.py --min-stack-reserve 1024 \
  games/toronto-dispatch/project/build/toronto-pickup-condition.gbc.debug/symbols.noi
```

Stock GBVM reserves the `DF00–DFFF` page for its second OAM buffer, palettes and text tiles, and starts the downward CPU stack at `.STACK=DF00`. The checker rejects linker-area overlap with those absolute buffers, inconsistent/missing symbols and heap ends at or above the stack. The command additionally requires 1,024 bytes of stack reserve; this project threshold does not measure the actual deepest native call path.

The first booting expanded candidate `36119ebf…` had heap end `DDA7`, stack base `DF00` and **345 bytes** of reserve. An earlier full-table cache ended at `DF90` and corrupted the reserved OAM page before Toronto initialized. Keeping only six coordinate pairs removed that allocation overlap, but the first booting candidate later failed a remote soft reset. Published Prototype 3's eight-context build ends at **D90F**, leaving **1,521 bytes** below `DF00`, and passed native reset samples. Prototype 4 ends at **D934**, leaving **1,484 bytes**. The optimized atlas candidate ends at **D950**, leaving **1,456 bytes** below `DF00`; its actual linked symbols pass the 1,024-byte guard. The renderer adds 28 bytes compared with Prototype 4, and its lookup optimization adds no further WRAM compared with `ec982d0c…`. Allocation checks and reset evidence remain separate: neither establishes physical persistence or every deepest call path. `make check` runs checker regressions, while the explicit command inspects the actual newly linked ROM.

## Pickup condition lifecycle: later native candidate

Official output `project/build/toronto-pickup-condition.gbc` is 524,288 bytes, CGB-only, MBC5+RUMBLE+RAM+BATTERY with 32 KiB SRAM. ROM SHA-256 `64be19fa3da7ba4231ba8c8decec4720c409116c034f586974fd4ddce789945a`; source fingerprint `7a61baf9dfa688dd423be21b8444380abc1969481ee8a00286f15cae55c6ebf3`; matching NOI SHA `9e82436532668b27e9b68059dce8c9cb8696ddc06bb1a5e4b2cb854812843f18`. Project revision, compiler and globals digests retain the Prototype 6 values below. The official build exited 0 in 39,175 ms with the existing Node/SDCC warnings and no errors. Independent header and actual linked memory checks pass: `D950` heap, `DF00` stack, 1,456 bytes reserve against the 1,024-byte minimum.

Four damage paths now require an active carrying stage. A fifth startup guard repairs condition on valid older uncollected-job saves after CRC/semantic validation. Empty-collision/rider text follows that lifecycle. There are no new save fields, persistent RAM allocations, contracts, stops or assets. Actual-C regressions pass 3,058 checks, including 14 failures captured on the predecessor; full `make check` passes. Native exact-input replay preserves uncollected condition100 versus the predecessor92, then verifies pickup, actual carried damage and condition-scaled delivery. A separate matched-phase first delivery and held turn retain cash139 and speed24/heading4. [TESTING.md](../TESTING.md) records immutable native identities and the limits of these samples.

This candidate is not included in the published Prototype 6 ZIP. The browser preview remains unresolved, and physical persistence, the full city and the complete campaign's duration remain unverified.

## Queen streetcar and safe alighting: Prototype 6

| Identity | Value |
| --- | --- |
| Native output | `project/build/toronto-queen-streetcar-safe.gbc` |
| ROM size / header | 524,288 bytes; CGB-only, MBC5+RUMBLE+RAM+BATTERY, 32 KiB SRAM |
| ROM SHA-256 | `23b2a7a25c9c593a51967e16a275cfb162bbb3e59f709eecd37dd77e2bb408f0` |
| Project revision | `1bfb289fdc7da0bed213dc236bcb1611388fa86faecfe8da4461bc11d7d9d019` |
| Build source fingerprint | `15ef9fbe8c74d6b1603d298fb4f0ec3d899bd279c4e220f65eb3ce8481a4873b` |
| Native `symbols.noi` SHA-256 | `36ec47e25446b3959c9746c27a46222361150095a9ef87bd7a565c3ff51cbac5` |
| Native `globals.i` SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Compiler identity | `78961636939edb44b0539ae26badfa07055be38124dbf08e3b84ec5608a2e627` |
| Scenes / contracts / stops | Four / 88 / 51 |
| Save / contexts / WRAM guard | Version 6, 58 bytes / eight / `D950` heap, `DF00` stack, 1,456-byte reserve |

The official plugin build completed in 45,861 ms. Its CGB header and matching debug digests pass inspection; linked allocation passes the explicit 1,024-byte minimum guard. The CLI reports its existing Node `DEP0190` warning and SDCC sound/optimizer warnings, with no errors. `make check` passes 299,366 atlas API, 8,036,093 atlas UI, 609,452 transit API, 2,857 engine, 2,974 bridge and 16,997 navigation checks, plus nine memory fixtures and the content/art/source checks. These checks establish distinct facts.

`td_transit.c` holds pure BANKED service, selection, fare, duration and departure APIs, with no persistent WRAM. The Queen service uses eight source-referenced curb signs across West/Core/East, a fictional 64-second directional timetable, four seconds per stop and a $3 fare. It runs from the shared game clock while other scenes are loaded. Original train, bus and ferry services remain available. [STREETCAR.md](STREETCAR.md) records the compressed route and source limitations; moving streetcar artwork is pending. Original stop IDs 0–42 and all 88 compiled contract fields remain stable. `create_campaign.py` now compiles 51 stops; art fingerprints and planning metadata change with the new signs.

Arrival chooses the stop centre when clear, then nearby cardinal positions at 12 or 18 pixels. It checks bounds, destination collision and a swept walking path, the player's parked car, and currently loaded traffic. An obstructed arrival retains its paid ride and retries without another fare. Remote Queen platforms are on sidewalks; future remote road stops need destination traffic checks. Completion/expiry also refreshes the depot objective before saving, fixing a stale completed-job map target.

Two fresh ordinary-button recordings test this exact ROM. The first repeats the Market job and held-turn regression, three Queen trips loading Core/East/West, WAIT/RIDE map freezing, paid-trip soft reset and parked-car recovery. The second repeats subway/map/reset controls, rides the 94 Wellesley bus, returns to Union at `(572,720)` beside the car at `(560,720)`, walks away, rides the Centre Island ferry out and back, walks on the Island and re-enters the Union car. Native samples report 59 updates over 120 video frames and no over-limit OAM scanlines in their sampled views. Full timings, immutable journal identifiers and predecessor failure are in [TESTING.md](../TESTING.md). These samples establish neither every route nor hardware persistence or the complete campaign's duration.

Milestone identifier: `v0.2.0-prototype.6`. Its [published bundle](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) is 85,940 bytes, ZIP SHA-256 `ecfee71c28227ee8d48d3d841af76111139af97ce5e52be73cab92d867137c87`, declaring source commit `e85006f07d07ff628a1aad0a45dd9dc6271e4695`. Anonymous download and all member checksums pass. Use the matching [loading instructions](LOADING.md); publication evidence is in [TESTING.md](../TESTING.md). A fresh source build needs its own inspection and test identity. The older browser preview remains unresolved and does not display this ROM.

## Final Prototype 5 rebuild

Current official output: `project/build/toronto-city-atlas-portable.gbc`,524,288 bytes, SHA-256 `2d1f6e4e7ae48a434757e63454d216b02b81957ecf5f8582d879149d447d7311`. Build source fingerprint `efe054a611bebeb91231f37db6102e71c1c305f2f861d09010491a4340f9aea4`. Project revision/compiler/header and matching NOI/globals retain the identities in the preceding optimized build's table below; official build and actual1,024-byte minimum memory guard pass with1,456-byte reserve.

The generated atlas C return was placed on a separate line to satisfy Linux GCC's strict indentation check. The rebuilt ROM differs from `e812f7ef…` only in its four-byte stock save signature and one global-checksum byte; runtime code/data, NOI and globals are byte-identical. The stock signature is separate from Toronto's custom version-6 saves. Full repository checks pass. Two fresh recordings repeat251 driving/map and153 transit ordinary input steps, verify the exact first-job/held-turn regression, frozen map/job state and all thirteen paid/wait/reset checkpoints. These are separate final-ROM recordings and are listed in [TESTING.md](../TESTING.md); predecessor timing/OAM readings remain scoped below.

The milestone identifier is `v0.2.0-prototype.5`. Use the matching bundle and [loading instructions](LOADING.md), and inspect/retest any newly built file against its own identity. Full former Toronto, two measured hours of varied gameplay, human review and physical cartridge acceptance remain open.

## Preceding optimized native city atlas

Milestone identity: `v0.2.0-prototype.5`. Publication and its downloadable bundle are tracked on the [releases page](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.5). This candidate retains four native scenes, 88 contracts and 43 stops, and replaces the earlier local camera map with a browsable schematic of the four registered areas.

| Identity | Value |
| --- | --- |
| Native output | `project/build/toronto-city-atlas.gbc` |
| ROM size / header | 524,288 bytes (512 KiB); CGB-only, MBC5+RUMBLE+RAM+BATTERY, 32 KiB SRAM |
| Candidate ROM SHA-256 | `e812f7ef3bee91e13e8ee0551c936eeed74283c60cb8c497ed45518d7b15d128` |
| Plugin project revision | `375cff6b012a8acd6bc0fcf11fbd22fb49b9cdb085179063ecdd4929e875bec4` |
| Build source fingerprint | `bbff1b78d37e3abf900a1b082d70bb33af235ef882228cccca2b5c6129ea5cde` |
| Compiler identity | `78961636939edb44b0539ae26badfa07055be38124dbf08e3b84ec5608a2e627` |
| Native `symbols.noi` SHA-256 | `ad657f05786ee9230aa413bb335d2e65f7b93a91aada696693d25c65135d3a4a` |
| Native `globals.i` SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Save / VM contexts | Version 6, 58-byte state / 8 contexts, unchanged |
| Memory guard | `D950` heap / `DF00` stack / 1,456-byte reserve |

The official `rom_build` completed successfully in **40,316 ms**. Read-only inspection verifies the actual ROM and sibling `.debug/symbols.noi` / `.debug/globals.i` against the captured build digests. Full `make check` passed: **299,366 atlas API checks, 8,036,093 atlas UI checks, 2,404 engine checks, 2,974 bridge checks, 16,997 navigation checks and nine memory fixtures**, alongside deterministic atlas/source validation. Host adapters/sanitizers, compilation, linked allocation and native playback establish different facts.

The actual atlas unit occupies bank `0F` at `[4000,6774)`: **10,100 bytes = 648 API code + 9,452 data**. Its origins are at `4026` (16 bytes), names at `4036` (76), 457-pattern dictionary at `4082` (7,312) and 1,024 UWORD indices at `5D12` (2,048). These arrays and the atlas/world units byte-match the predecessor. The whole shared bank `0F` has **six bytes** remaining; UI APIs remain in bank `11` hexadecimal (17), whose whole-bank headroom is **seven bytes**, down from 106 in `ec982d0c…`. Fixed bank 0 retains **127 bytes**, ending exclusively at `3F81`. These linked ranges do not measure the deepest stack or CPU-cycle performance.

Binary-decoded CGB bank-1 ownership is disjoint under stock signed background addressing: the 56 courier sprite tiles occupy `8000–837F`; gameplay background IDs 0–7 occupy `9000–907F` (West uses only 0–4; High Park/East reference none); markers 8–14 occupy `9080–90EF`; ground 16–187 uses `9100–97FF` and `8800–8BBF`; font 192–240 occupies `8C00–8F0F`. All 784 font bytes match the ROM. Runtime restoration is separately sampled below; future assets must revalidate these ranges.

On this exact ROM, the repeated **98-step first-job regression** completes the Market delivery at frame 740 with cash 139, one completion and condition 100. The original held-turn regression reaches frame 864 at `(798.75,774.25)`, speed 24, heading 4. Native map samples render all four atlas areas while keeping the gameplay scene loaded. The complete 58-byte serialized state stays identical from frames **980→4,228**. Samples also verify camera/sprite/text restoration, exit during a partial redraw and on-foot player/car focus. An active job remains at deadline 120 and world second 15 through frames **5,090→5,410**. Rendering all four schematic areas does not mean all four gameplay scenes or every atlas viewport were replayed.

The native transit sample freezes a paid ride through map frames **1,170→1,410**, retaining cash 24, world second 3 and one ride second. During waiting, frames **2,826→2,986** preserve the same 58-byte state, including cash 24, world second 20 and subsecond 48. Resuming WAIT later boards at world second 36 and charges once, leaving cash 21. A soft reset while that paid-trip map is open restores HELP at frame 4,338 with world second 36, cash 21 and one remaining ride second. Continuing arrives at King by frame 4,466, world second 38, cash still 21: no additional fare. Waiting/riding pause and reset evidence is native emulator evidence, not physical SRAM persistence.

The retained driving/map journal contains **5,434 frames / 690 events**, digest `f1ddccf5e5f87813d068b57a99ba5a534ad456b3250112a9c98505d998bf38ed`, ignored archive `eea14575-c66e-42dc-ae5d-9b0b03e6cef2`. The transit journal contains **4,466 frames / 472 events**, digest `f6642524fcdbcdedebdaacc9607b82a432957d16b9aa2469af37482dc25e530f`, archive `dcc747d1-9578-4a23-8e98-96027fb388bc`. [TESTING.md](../TESTING.md) owns the exact scenario observations.

The unpublished `project/build/toronto-atlas.gbc`, SHA-256 `ec982d0c90307f3433d27704ee8fa6fee1cf8c9211ef22dbc59877cfb07e71e2`, is the slower linear-cache predecessor. Its separate NOI digest is `def28f022f863130b6cfe2598e3cb8d9bdaa422d28f9c97aa49010c2fa9431a2`; its source fingerprint is `c91aa8e7ac82a4289477ee734cd36f0549d9f021d99d51d0566a357fdd3f7d74`. It is superseded by the optimized candidate; neither its samples nor Prototype 4's nine-job progression establish this new binary's complete acceptance.

Native coverage of all 225 atlas viewports, remote parked-car and foot-only anchor/client map markers remains pending. This candidate has not replayed all nine Prototype 4 jobs or all four gameplay scene transitions. Full former Toronto, waterfront and fuller Islands, at least two measured hours of varied gameplay, human readability/handling/audio review, crowded-scene performance, browser state and physical cartridge behaviour remain open. The proposed 2026 era is not adopted.

## Prototype 4: four scenes and sampled native acceptance

| Identity | Value |
| --- | --- |
| Native output | `project/build/toronto-four-districts.gbc` |
| ROM size | 524,288 bytes (512 KiB) |
| Candidate ROM SHA-256 | `1da71ba549aaf6b0b1fc641d4f4f9e0e317550e396bf80ffaf401f6ff7e82b2b` |
| Plugin project revision | `375cff6b012a8acd6bc0fcf11fbd22fb49b9cdb085179063ecdd4929e875bec4` |
| Build source fingerprint | `e029dac64f995a5466df744fad68ebbf4e14ef28b9d5d0df944ff0deed44a5ee` |
| Native `symbols.noi` SHA-256 | `24a627853b5d39766d336901b52ad871ad022b7c767afe6b14e26cb3d670f289` |
| Native `globals.i` SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Native scenes / content | Core 0, west 1, High Park 2, east 3 / 88 contracts, 43 stops |
| Save / VM contexts | Version 6, 58-byte state / 8 contexts |
| Added persistent WRAM | 37 bytes compared with Prototype 3 (`99eb430…`) |
| Memory guard | `D934` heap / `DF00` stack / 1,484-byte reserve |

The official plugin build compiled the updated source successfully. Full `make check` passed **2,352 host engine checks, 2,974 independent bridge checks, 16,997 world-navigation checks and nine memory-guard fixtures**, plus content/registered-resource/generated-source validation. The actual ROM, NOI and globals digests above were verified, and the linked symbols pass the explicit 1,024-byte guard. Host tests, linked allocation and native execution are distinct evidence.

The finalized ordinary-button native progression recording contains **31,480 frames / 11,894 events**, digest `cabb314d585e6451b33e93d03ef0f27b30373cd1d6ed158fa2b527249073a505`, retained in ignored archive `efe1b627-ab0f-437f-9bd6-cc06031fb38a`. Inspection was read-only. It completes nine distinct contracts: 01, 02, 03, 07, 81, 04 (truck), 82 (car), 83 (motorcycle) and 85 (Withrow park-and-walk relay). All four actual native scenes load; sampled travel covers the three core/east approaches, core-to-west driving and west-to-High Park walking. Withrow verifies the parking-anchor/client-marker switch, handoff and car re-entry. The repeated held-turn sequence retains speed 24 at frame 864.

A remote High Park reset restores nine completions, cash 1,304, world second 486, player `(979.5,639.375)` on foot and the car parked in the west at `(861,640.875)`. The local-map camera global changes from 31,328 to 23,648 across 160 frames while player and clock stay frozen. These are recorded state values, not camera pixel coordinates; the recording's stop-reason camera numerals were inaccurate and are corrected in [TESTING.md](../TESTING.md).

A separate final-ROM transit session `3b64dc833a774380a4c6814c3b85db0f` records **562 frames / 298 events**, digest `ed0f8b776016ad10af8116d8c1cdeed3d81f6acd62342b8cb1b856ae12788fac`, archived as `fc614adf-9114-4968-8c48-53ade87b46bc`. At frame 314, clock 1, the selector shows `DEPARTS IN 0` with cash 30. Eight A frames board immediately by frame 322 and charge once, leaving 27. At frame 442, the courier arrives on foot at King `(640,640)`, world second 3, with the car parked at Union `(560,720)`. This verifies boarding/arrival, not resetting during the paid ride.

From frames 442→562, the UBYTE update counter changes `248→51`: modulo 256, that is **59 updates over 120 video frames**, about 29.5 updates per second. The OAM snapshot has 12 visible sprites, peak four per scanline and zero over-limit scanlines. The immutable journal's stop reason incorrectly states `88→148`, 60 updates and peak six; the actual inspected values above and [TESTING.md](../TESTING.md) correct that text. This bounded core sample does not establish crowded-scene or whole-city performance.

The progression sample represents about eight minutes of purposeful native game-clock gameplay. It does not establish all 88 contracts, every seam lane, two hours of varied gameplay, human enjoyment, expanded-world frame pacing, full former Toronto coverage or physical cartridge behavior. These Prototype 4 samples did not complete the remaining handoffs/seams or paid-ride reset; the newer atlas candidate's paid-reset evidence belongs to its own identity above. The milestone identifier is `v0.2.0-prototype.4`; use the matching bundle from the [releases page](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.4). Exact build-specific scenarios and later acceptance results belong in [TESTING.md](../TESTING.md). No human listening or current browser/device proof is claimed.

## Historical intermediate four-scene build

| Identity | Value |
| --- | --- |
| Output path at build time | `project/build/toronto-four-districts.gbc` (now used by the final candidate) |
| ROM size | 524,288 bytes (512 KiB) |
| Candidate ROM SHA-256 | `7a299125675b7e08aeb3ba939b2382b28597bfbae584ec2a5c4255ed5abf24f0` |
| Plugin project revision | `375cff6b012a8acd6bc0fcf11fbd22fb49b9cdb085179063ecdd4929e875bec4` |
| Build source fingerprint | `e8719774b958cd9114030e0d2bda83fee963c42e745505beb588968a6856cce4` |
| Native `symbols.noi` SHA-256 | `67affe33b3f214f6eaa6998c277b6186f4698ba1bfe325ac276523388b0d4dca` |
| Native `globals.i` SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Native scenes | Core 0, west 1, High Park 2, east 3; each 1,024 × 976 |
| Compiled content | 88 contracts / 43 stops |
| Banked world functions | ROM bank 15 |
| Save / VM contexts | Version 6, 58-byte state / 8 contexts |
| Added persistent WRAM | 37 bytes compared with Prototype 3 (`99eb430…`) |
| Memory guard | `D934` heap / `DF00` stack / 1,484-byte reserve |

The official plugin build compiled all four registered scene/collision resources successfully. It reported Node `DEP0190`, an upstream unreachable sound-effect warning and five `TORONTO` optimizer warnings, with no errors. The ROM and debug-artifact sizes/digests above identify this local candidate; they do not identify a public release or a Git commit.

These linked artifacts place the 4,986-byte world unit in bank `0F` (15): 3,432 bytes of code and 1,554 bytes of data. Its public functions are `BANKED`, it allocates no static world WRAM, and its route function uses an 82-byte local frame without adding 32-bit arithmetic imports. The fixed ROM bank ends at `3F81`, leaving 127 bytes, the same headroom as Prototype 3. This linked-bank accounting is separate from the WRAM guard and does not measure the deepest native call path.

Before this build, full `make check` passed **2,273 host engine checks, 2,974 independent bridge checks, 16,997 world-navigation checks and nine memory-guard fixtures**, alongside source/content/registered-resource validation. Host hardware stubs and sanitizer checks do not prove native execution. The explicit memory guard also passed against this candidate's linked `symbols.noi`.

Native plugin samples on this earlier ROM passed immediate transit boarding over 442 frames and first-delivery/full-speed held turning over 864 frames. Driving core 0 → east 3 → core 0 and walking core 0 → east 3 were observed. It predates the parking-anchor source fix and is superseded by `1da71ba5…`; its samples are not proof of the final candidate's paid-transit behavior. Build-specific scenarios belong in [TESTING.md](../TESTING.md); the published Prototype 3 below remains a separate binary.

## Published Prototype 3: three linked scenes

| Identity | Value |
| --- | --- |
| Native output | `project/build/toronto-districts.gbc` |
| Published ROM SHA-256 | `99eb430cc59cbb51631d343a4b626d07db03ff10ad36dd567128b438d36c528f` |
| Build source fingerprint | `93038e0d626669ee1d1b6809ebd68b492997c28a931300e3e59bf9e1eb207f08` |
| Native NOI SHA-256 | `2e8fca83ec54ad8517f1b6125710bc388b6952b8b9144c0b4e24133321e50526` |
| Native scenes | Core 0, west 1, High Park 2; each 1,024 × 976 |
| Compiled content | 80 contracts / 35 stops |
| Save schema | Version 6; 58-byte state |
| Memory guard | `D90F` heap / `DF00` stack / 1,521-byte reserve |

Published Prototype 3 is CGB-only and 262,144 bytes (256 KiB), available through the [releases page](https://github.com/trancethehuman/modretro-games/releases). Its official build resolves three scene and collision resources. The ModRetro plugin confirmed clean boot, first-delivery/held-acceleration turning, a driving transition from core 0 to west 1 and walking from west 1 to High Park 2. A soft reset restored the courier on foot in the actual High Park scene while retaining the parked vehicle in district 1, cash and world clock. Western active-job and paid core-trip reset samples also passed. These samples do not establish all portals, complete campaign progression, two hours of gameplay, full Old Toronto coverage, physical cartridge behavior or the newer four-scene binary. Build-specific details belong in [TESTING.md](../TESTING.md).

At that milestone, `make check` passed with **1,212 host engine checks, 710 independent district bridge checks and nine memory-guard regressions**, alongside repository/content/generated-source validation. Those checks use host hardware stubs and do not replace the native scenarios above. Source fingerprints and symbol digests identify build inputs/artifacts; they are not Git commits.

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

Original music and engine/brake/event/transit effects are compiled into the published milestones and current candidate. The pause menu cycles music + effects, effects only and silent; the preference defaults per boot. A separate public PyBoy 2.7.0 PCM run of the single-scene ROM identified above passed all eight interval checks: nonzero music, acceleration, braking, effects-only acceleration and resumed music, and all-zero measured silent/menu/paused-effects-only intervals. [AUDIO.md](AUDIO.md) records sample counts, peaks, limitations and the opt-in reproduction command. Those captures do not establish sound continuity in the new districts. No human listening or physical sound check is claimed.

No Chromatic was connected during the latest discovery; no stream, cartridge write/read-back or cold boot is verified. The full campaign, two-hour gameplay target, full Old Toronto coverage and whole-city performance remain open gates. Follow the [loading instructions](LOADING.md) and [hardware workflow](../../../docs/HARDWARE.md) before any write.

## Browser refresh limitation

A previous official web export compiled, but refreshing the owned preview failed with `Browser recording close acknowledgement is UNKNOWN`. The listener retained unresolved closure state; its paused starter save and local recording evidence were preserved. Native gameplay testing proceeded independently. Do not arbitrarily reload or replay that unresolved close operation. Resolve the plugin's preview state before treating the current city ROM as browser-verified. The new top-down ROM has not been verified in that browser view.
