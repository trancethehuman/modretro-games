# Native build and preview

Current selected local candidate is **`toronto-dispatch-ui-polish-direct.gbc`**, 524,288 bytes, SHA-256 **`03e09fa85351c91f56d7370a3f0e0f856cd147a3c8d5ebfd328a2786a38f25d6`**. Active-job previews show CURRENT STOP and A RESUME while retaining the full pickup-first itinerary; completion of car entry repaints the vehicle/control HUD immediately. These direct edits fit without fallback. The [loading guide](LOADING.md) selects this exact file; intended new bundle basename is `toronto-dispatch-ui-polish-direct.zip`, packaged after committing the matching source. The bundle’s BUILDINFO.json pins that source commit.

Official build takes 58,494 ms; fingerprint `1140f844d78d0f841a94509ad99609e8aac8fc00687b7f9e233da6b983f81744`, matching NOI `037dab5b328250b9dff9e508150f1527a5b1f85f0d3e62f0a28981e67c479130`. Source is cache checkpoint `db7a1d00…` plus two frozen project files: `TORONTO.c` SHA-256 `077eef79c99763dc27dbb0266674ca4af09549802cff3f95e8441a60b7aed34c`, and `td_ui.c` `2550d3a62bda5a57869736629ad7ac7cab74f5d6956a8bfa953ac75eb93b04d3`. The eventual commit/package identity is pending; current project has 194 files rather than the historical e797 package's 192.

Gameplay bank 2 is 16,380 / 16,384 bytes, leaving four bytes; no bank workaround or fallback is applied. Header and scene/resource gates pass; persistent reserve remains 1,096 bytes (`DAB8`→`DF00`) and save v9 / 58 bytes is unchanged. Full `make check` passes 804,864 engine / 25,110,098 UI / 2,430,335 pedestrian / 2,339,979 road checks. Cache differential matches 6,463 actual-C snapshots / 3,431,853 fields. Seven engine/eight UI old-source failures are retained. Static reserve and fitting code do not prove deepest stack or whole-city pace.

A [fresh native replay](NATIVE_UI_POLISH_SAMPLES.json) checks active-job captions/resume/exit/chord rules, paused board/map and interrupted entry, immediate entry HUD refresh, one condition-100 Market Start (credit 108 / cash 138 / done 1), rejected remote First Art pickup and genuine in-worker SRAM recovery. First Art remains unpicked stage 0; there is no second completion. Independent compiled and raw native reviews pass 597 / 5,253 checks; public projection review passes 481 checks. Packaging has a separate verification record. Detailed timing/provenance and limitations are in [TESTING.md](../TESTING.md). The earlier e797 22-completion campaign and 825 workload gains are not inherited by this exact build.

Its own [single Core timing trial](NATIVE_UI_POLISH_PERFORMANCE.json) records 385 completed loops / 1,080 VBlanks and 80 / 240 in the driving tail, with 1,085 independent receipt/journal/image checks. These match retained825 values; neither repeated-trial acceptance nor a new whole-city gain is claimed.

## Retained courier-clearance candidate

Retained local candidate is **`toronto-dispatch-courier-clearance.gbc`**, 524,288 bytes, SHA-256 **`e797f5725574248915f89945dcb1c1b59c7906c8ebe8dc1f155f97b56000374f`**. Visible walkers defer proposed steps against the occupied courier car at every speed, preserving their previous visible pose. Genuine driving contacts retain fines, recovery and police heat. Dispatch Select advances to the next eight-job chapter; twelve compact captions and an invalid-index guard improve navigation without adding save bytes. Its historical bundle basename is `toronto-dispatch-courier-clearance.zip`; current loading selection is described above.

Official build takes 64,964 ms; source fingerprint `aa8417d346d660ea1efb2a98cac2f706bc15037438a73e7dbfd1400f1e2129b7`, NOI `7ca4749489938523e81a847527841b2b3fcf9dd08bee8844713044c859ed0b92`. Build source is base commit `a2df700ec529928ded6fa56aeab7072a18df14df` plus three frozen engine files: `td_people.c` = `dff370c9f04b915dd65ca6d4968cfc88e4c51f407157d1f3038791715611f936`, `TORONTO.c` = `c9f5e62c16f561f529237408a101df0c0e6af92c42d83bcb2f4247dcfc564a59`, `td_ui.c` = `877d8bb9d6bd9eacd5f65b492700b2cab45b677b79e25be318156be5370092e8`. Packaging records the actual later commit containing these exact files.

Official header/resource/ABI guards, 1,242 scoped compiled comparisons and full `make check` pass, including 804,856 engine, 2,430,335 pedestrian and 25,109,617 UI checks. The strict 160-Q4 proposed-step guard has no speed read; continuous impact remains strict 128-Q4, and road-only fallback preserves genuine contact with the old visible pose. Persistent allocation, save v9 / 58 bytes and linked reserve 1,096 bytes (`DAB8`→`DF00`) are unchanged. Static allocation does not measure deepest stack or crowded performance.

The [closed native records](NATIVE_COURIER_CLEARANCE_SAMPLES.json) cover all twelve chapter starts, held/chord priorities, locked offers, first delivery (condition 100 / base 86 / time 22 / credit 108 / cash 138 / done 1), held RESULT B, an ordinary-input visible-human wait/reverse/clearance, a genuine forward impact ($20/H1/six-second recovery), map freeze/panning and manual save. A separate record imports the first record's genuine same-ROM checkpoint and tests in-worker SRAM soft reset. Fresh workers intentionally start blank SRAM; the root's initial contrary expectation is retained and corrected. The literal failed `c4fa…` trajectory drifted on the replacement, so its phase 36 native counterfactual is not claimed. Exact host regression plus adapted native observations are separate evidence. All 96 contracts, two measured enjoyable human hours, wider new-ROM transit/heat/district/pacing/stack, browser and hardware remain pending.

The committed source snapshot is `25b659785d94ff8eaa8eeeb402243005acfe146a`. Prepared `project/build/toronto-dispatch-courier-clearance.zip` is 130,292 bytes, SHA-256 `0dd42c60772bf77ea8a5cf19fb5da06852f8db92c66f836140e1cfac7e1cf39b`. Its six root members contain this exact ROM, `SHA256SUMS`, `BUILDINFO.json`, committed loading instructions, MIT licence and distribution notices. Independent packaging audit passes 1,256 checks; all 192 project files match that source snapshot, and seven loading links resolve at that commit. Both source CI runs [37152386944](https://github.com/trancethehuman/modretro-games/actions/runs/37152386944) and [37152383742](https://github.com/trancethehuman/modretro-games/actions/runs/37152383742) complete their actual `Run make check` steps successfully. The ZIP stays in the ignored build folder; public source is on draft PR 11.

## Retained update-local terrain cache experiment — 825 scope

`project/build/toronto-dispatch-update-terrain-banked.gbc` is 524,288 bytes, SHA-256 `825444a0d9d82a3b2bb9a58f184ff30f3c6aa16d61f893531ceaa3a4f68e4c48`. Official build takes 59,636 ms; source fingerprint `118dd17e27c3e2e750f1d8258df6a91c28db4255d405c379332689935563fcc8`, matching NOI `73bcec021bf1c05c59cae192e87f29f5ea4b1bd74b9f1064972c940925a98d46`. CGB-only/MBC5+RUMBLE+RAM+BATTERY, 32 KiB declared SRAM and logo/header/resource gates pass.

The first placement produces no ROM: `TORONTO.o` reaches 16,501 bytes, 117 beyond one bank. Only the new cache helper moves to its own banked unit. In `825…` it is at bank 9 / `7F42`; gameplay bank 2 fits at 16,382 bytes. Six automatic cache bytes enlarge the update frame 10→16; the helper has eight local bytes and a nested road call. Independent compiled review passes 611 checks. Persistent allocation stays at heap `DAB8` / stack base `DF00`, reserve 1,096, with save v9 / 58 bytes unchanged. Neither linked reserve nor these local frames bound the deepest runtime stack.

[The native comparison](NATIVE_TERRAIN_CACHE_COMPARISON.json) repeats idle 361→385 completed loops / 1,080 VBlanks (+6.648%) and records a fixed-input moving tail 77→80 / 240 (+3.896%). [Separate fresh gameplay](NATIVE_TERRAIN_CACHE_GAMEPLAY.json) checks delivery/driving/contacts/map/entry/in-worker SRAM recovery; it also exposes stale WALK controls after car entry. Active-job dispatch acceptance/WRONG VEHICLE feedback is a separate source-confirmed UI issue. The two UI corrections now have the separate direct `03e09…` build/replay above; the 825 pacing observations remain historical. The e797 bundle remains unchanged; wider pace/stack, remaining contracts/full city, two human hours, browser and hardware remain pending. [TESTING.md](../TESTING.md) distinguishes host equivalence, static review and native scopes.

Retained admission candidate is **`toronto-dispatch-pedestrian-admission.gbc`**, 524,288 bytes, SHA-256 **`964f4ad40275eb373c7a2e2cb500a0bdc84220fe0093f6333d317a25e89d409a`**. Newly selected/hidden walkers defer near the physically occupied car; continuously visible impacts retain the existing rules. Its original build used base `3674c615…` plus frozen `td_people.c` SHA-256 `6ba73602a0f8198b246572002e3f4bb19e64be6ff1bc0cb080c13e510a7da4f1`, later published in `a2df700e…`. Its historical bundle basename is `toronto-dispatch-pedestrian-admission.zip`.

Official build takes 55,964 ms; source fingerprint `d453a3f065fc718c9647fa400153fc0edccf8607d2917940e22b6ae805c344a7`, NOI `b3eb0e28000a5cb446bfca27d6f93d6bd31399c9cf6b3689a10311d182c3ace1`. Header/resource/ABI guards and 580 compiled comparisons pass, including flags destination, context, strict 160-Q4 admission and unchanged 128-Q4 impact calls. Focused pedestrian ASan/UBSan and actual-engine gates pass 2,423,750 / 800,555 checks. Persistent allocation and save v9 / 58 bytes stay unchanged; linked reserve is 1,096 bytes (`DAB8`→`DF00`). The people-present local frame grows 27→28 bytes; deepest stack and performance are unmeasured. Full `make check` on the frozen source passes; repository results remain separate from the native prefix.

The [retained964f fresh native record](NATIVE_PEDESTRIAN_ADMISSION_SAMPLES.json) verifies first delivery (condition 100 / credit 109 / cash 139 / done 1), held RESULT B stop with live world, a continuously visible route-83 human impact ($20 / H1 / six-second recovery), exact game/person/fleet map freeze and panning, and genuine reset of later automatically saved progress. SAVE A briefly accelerates; early reset imagery is blank before visible HELP. One OAM sample peaks at four of ten; the final opaque emulator checkpoint is save-only. Route-19 first-appearance prevention remains host/compiled evidence. Earlier `a0bd…` matched-route and `d58…` campaign scopes below are not inherited; all 96 jobs, two measured enjoyable human hours, wider gameplay/stack/pacing, browser and hardware remain pending. [TESTING.md](../TESTING.md) retains exact pins and caveats.

Retained RESULT-controls candidate is **`toronto-dispatch-result-controls.gbc`**, 524,288 bytes, SHA-256 **`a0bd037ac8925534e40d5147ae8ae4f748e533462bd0c01502ae5004648ae634`**. Source `371641…` consumes RESULT B until release, preventing immediate reverse after closing the receipt while the world continues. Fresh first-delivery/control/truck/reverse/genuine-reset evidence is in [the portable record](NATIVE_RESULT_CONTROLS_SAMPLES.json); four earlier `d58…` scopes remain historical and are not inherited.

Official build takes 57,439 ms; fingerprint `3d984b75e841f6e5db821a37124e8e18e95a0ac604abc00639c2d6ca629ec380`, NOI `007e4aaf4de8b67ddcd6fb1b0b34a0498e81a43fad6e39a89bb3949942c63ae1`. Full checks pass with 800,554 engine cases; independent compiled audit passes 514 checks. Header/scene/table guards pass, save v9 remains 58 bytes, and the new one-byte transient release latch leaves 1,096 static bytes (`DAB8`→`DF00`). Deepest stack, broader new-ROM campaign/transit/heat/pacing, two enjoyable human hours, browser and hardware remain unverified. Its older loading bundle is `toronto-dispatch-result-controls.zip`; current selection is described above.

A [separate closed continuation and matched-route replay](NATIVE_MATCHED_TRANSIT_SAMPLES.json) tests this unchanged `a0bd…` binary after a disclosed genuine done-1 checkpoint import. Two new jobs reach done 3 / cash 41 after damaged-art pay 102, full-condition Distillery pay 124 and funded H1 foot/H2 stationary-car fines. Independent First Connection branches start from that exact neutral state: two trains/two buses plus walks cost $10 and finish at condition 100 / 163 seconds left / credit 254 / cash 285; conservative cardinal driving incurs a $5 signal fine and finishes at 100 / 154 / credit 252 / cash 288. Each reaches done 4, not a combined campaign. The 14,277 newly recorded frames include both alternatives and are not a gameplay-duration measurement. No source/build gates change; wider campaign, route/balance/human/hardware acceptance remains pending. [TESTING.md](../TESTING.md) retains the complete scope and original sampled receipts.

Retained local candidate is `toronto-dispatch-bus-lanes.gbc`, SHA-256 **`d58bc338f6dceb2b208bf855112cc9b93f0d72fdf254ac0ccdbd17ec1ff357b8`**, 524,288 bytes. It preserves six scenes, 96 contracts / 59 stops, seven parking anchors and save v9 / 58 bytes. Exact Q4 pedestrian/vehicle-body clearance and a corrected six-vertex Core bus loop remove the reproduced lane standstill. Official build/header/compiled guards and full `make check` pass, including 800,394 engine and 2,421,337 pedestrian checks on source commit `ef066ab6a3336c347297cd0d096324bc7ea4b61b`.

Build takes 60,113 ms; matching NOI SHA-256 is `dd4f20696e74688b18fcb6b07692ca631caae754ae8257e0e4e8f0e15fe70b63`, source fingerprint `b2dd5754c3f2e010690d28abfd90fef754c7e1ca843e32094843ac61c5f307c8`. Static reserve stays 1,097 bytes, OBJ allocations 116/116/106/116/106/106, and the unchanged 1,484-byte tram tables remain in bank `0x18` (24). Actual compiled target/recovery/direction tables, cold spawn, precise fleet presentation and earlier parked-flags/selector guards pass read-only inspection. No persistent RAM or save bytes are added; deepest stack remains unmeasured.

Its [closed ordinary-input replay](NATIVE_CORE_BUS_LANES_SAMPLES.json) verifies first delivery, paid Line 1 fare once, exact paid-map freeze, genuine paid reset/arrival, two bus/police passes, all six authenticated bus legs and a northbound bus/human-clearance/OAM sample. The deliberate nine-pixel human wait/resume case remains host-only evidence. The [earlier `cf2f…` standstill](NATIVE_CORE_BUS_LANE_FAILURE.json), retained `5ae4…` Queen and `7b2…` exploration/13-job campaign keep separate identities. Remaining Island jobs, native older-save imports, all 96 contracts, two measured enjoyable hours, full Old Toronto, broader pace/stack/vehicle checks, human/browser/hardware acceptance remain open. [TESTING.md](../TESTING.md) records exact scopes.

A [second closed replay](NATIVE_COURIER_HEAT_SAMPLES.json) uses exactly the same `d58…` ROM/source/symbols: three unique jobs 0/2/1, a condition-92 credit of 115, First Art entirely on foot at credit 138, and ordinary car recovery reach cash 392 / done 3. Visible H1/H2/H3 impacts charge $20/$40/$60; exact game/person/fleet map freeze and genuine committed-H3 reset pass. Live road police then isolates the funded $225 H3 capture (cash 272→47 / heat 0 / stopped car), and post-capture reset retains it. In that record the patrol waits behind the stationary courier; it supplies no return or lower-heat capture pass. No source mutation or rebuild is needed for these additional observations. Full checks remain 800,394 engine / 2,421,337 pedestrian, and hosted [PR run 37122671458](https://github.com/trancethehuman/modretro-games/actions/runs/37122671458) completes its actual `make check` step on docs checkpoint `0988d0e…`.

A [third closed replay](NATIVE_LOWER_HEAT_PATROL_SAMPLES.json) on the same retained build adds funded Core on-foot H1/H2 capture ($25: 243→218; $100: 158→58) and a sampled driven patrol rejoin after the courier clears King. Actual leg 2→0 and road endpoints are observed without reset or scene change. Two full-condition jobs provide funding; car recovery and a neutral save-only checkpoint pass. No opaque checkpoint restore, source change or rebuild occurs within that record; build checks and loading identity are unchanged.

A [fourth closed record](NATIVE_SIX_JOB_CONTINUATION_SAMPLES.json) officially restores that genuine same-ROM two-job checkpoint, then adds four new unique completions 1/6/3/4, reaching done 6 / bitmap `5F…` / cash 626. Signature/truck condition scaling and an ordered relay with two trains/four buses costing $14 pass in scope. After-braking stationary H1 decay, a funded H1 car-capture interval (post-sample speed 2), and a positive-H1 paid Line 1 westward police turn toward patrol are sampled. Its final released-input emulator checkpoint is saved only. Source, build, compiled/full-suite gates and `toronto-dispatch-bus-lanes.zip` remain unchanged; H2 car capture, broader escapes/paid modes, new-job SRAM reset, full campaign/two human hours and physical execution remain open.

Retained Queen/street-life candidate is `toronto-dispatch-queen-street-life.gbc`, SHA-256 **`5ae4a83b3cb13dfbd838e4fdf9b49df79e8db1949af360e91e69a97fe148dda2`**, 524,288 bytes. It preserves six scenes, 96 contracts / 59 stops, seven parking anchors and save v9 / 58 bytes. Explicit parked-actor flags storage corrects the SDCC-generated view-cache write; remote parked-car guards use the loaded district and invalid district selection is rejected. Official build/header/compiled resource gates and full `make check` pass, including 797,467 engine and 2,400,936 pedestrian checks on source/test commit `fb055cbab675f4387ab80fd3bb80c8ed40f27f2c`.

Build takes 58,135 ms; matching NOI SHA-256 is `fa4d8b35c3175baeab2685f8f7a28d62f055d90f9ed9a8bf6bf08d61dbc818b0`, source fingerprint `e748135b7bbb82acdb13c98ccec90bf3e3a3772cbdd3d16738e83dbe800bb120`. Static reserve remains 1,097 bytes; OBJ allocations are 116/116/106/116/106/106, and the unchanged 1,484-byte tram tables remain in bank `0x18` (24). The clean compiled flags store reloads its destination; temporary diagnostics are removed. Deepest stack remains unmeasured.

Its [closed ordinary-input replay](NATIVE_QUEEN_STREET_LIFE_SAMPLES.json) verifies first delivery, paid Queen 47→43→47, advancing West pedestrians despite the remote Core car, paid map/reset, original car recovery, three audio menu labels and resumed driving. Two 240-frame map pairs freeze all 58 bytes; reset preserves the committed ride/fare/car/completion but changes fractional subsecond. The retained `7b2…` exploration and 13-job campaign below are not attributed to this ROM. Remaining eight Island jobs, native old-v8 imports, all 96 contracts, two measured enjoyable hours, full Old Toronto, human/browser/hardware and broader pacing remain pending. [TESTING.md](../TESTING.md) retains build-specific failure and acceptance scopes.

Retained Islands candidate is `toronto-dispatch-islands-safe.gbc`, SHA-256 `7b2af59c27179bd3445c5c074b85fb4a029b50144ef27ed3095f9a3ce83f7d5a`, 524,288 bytes. Six city districts, 96 contracts / 59 stops and save v9 / 58 bytes pass the official build, header, compiled resource/table gates and full source suite. Native ordinary-input travel reaches all three Island docks/clients/bridges, inland/coastal paths, original tree occlusion, correct map/audio menus, current-save resets, scheduled ferry round trips, zero-fare return recovery and the first delivery. [Portable native record](NATIVE_ISLAND_DISTRICT_SAMPLES.json) and [TESTING.md](../TESTING.md) keep scope separate from old-save imports, the full campaign/two-hour target, human play and hardware.

The official build takes 58,551 ms; matching NOI SHA-256 is `45f3ed6aebb06e035c1321246d31597594cf99994a3e8a269fc028e913044301`, source fingerprint `4d5b5eb385898cbb763920c5c62aee061c5d697ad72e7db7ecb64768a542dbaf`. Heap `DAB7` to stack `DF00` retains 1,097 static bytes; deepest stack is unmeasured. Six scene OBJ allocations are 116/116/106/116/106/106 of 128; bank 1 background uses 10/15/0/0/0/0 below aircraft scratch start 32. The 1,484-byte tram tables are in bank `0x18` (24).

The initial main-code bank failure (16,624 > 16,384 bytes) was fixed by moving ferry guidance into the existing BANKED route unit, with no persistent RAM. The subsequent `8a96…` native attempt exposed a pause AUDIO `%c`/integer varargs mismatch that overwrote the map cache. Bounded literal-prefix/label concatenation fixes it; the safe replay verifies all three labels and YOU/CAR/DEPOT focus. [Retained failure](NATIVE_ISLAND_MAP_FAILURE.json) and [GBDK argument-width guidance](https://gbdk.org/docs/api/docs_coding_guidelines.html) explain why host libc checks were insufficient.

The retained five-scene preparation build is `toronto-dispatch-island-preparation.gbc`, SHA-256 `ff5d13561d4b41823c08abc5d6d8bde300daf5e853a3017709b532f73787069c`. It passes the official build/header, compiled sprite/tram-table and 1,097-byte static-reserve gates, with five city scenes / 96 contracts / 59 stops / save v8. In that checkpoint the original fuller-Islands background was source-only; reserved district5, shared return-assistance pricing and traffic suppression prepared later registration/migration. [TESTING.md](../TESTING.md) separates host/source proofs and its existing-ferry native regression from the earlier itinerary/payout build below. That checkpoint used the separately verified `4343…` loading bundle. [LOADING.md](LOADING.md) identifies the current separately tested candidate.

The retained five-scene itinerary/payout candidate is `toronto-dispatch-route-feedback.gbc`, SHA-256 `4343f2b858e62f9e8daa2a3576a1d06bb7ee7208d2cd4a55434c36e168fb3100`. It retains **96 contracts / 59 stops / seven parking anchors**, adds full itinerary browsing and exact payout feedback, and replaces repeated tram interpolation arithmetic with identical ROM tables. Header, compiled sprites/tables and the 1,097-byte static reserve pass. Scoped native checks cover route controls, first delivery/payment, locked walking/return previews, timeout, reset and a separate paid Line 1 journey. [TESTING.md](../TESTING.md) and the portable records separate those results from retained `c625…` Port-job and `a638…` Queen travel evidence. Full Old Toronto, two measured enjoyable hours, broader pacing, deep stack, human feedback, browser recovery and physical execution remain pending. Downloadable Prototype 6 stays the separate four-scene release; [LOADING.md](LOADING.md) identifies the current local candidate.

## Itinerary and tram-table candidate

Official build time is 150,248 ms; matching NOI SHA-256 is `187bc1109ba38099dd1624264801beb20df578b2ed9a420753a7d88e0d0766d0`, source fingerprint `367106d4a72bf17b85cd2a7d294c978bdd844da664bec3806c8db7861ff1421b`. Linked heap ends at `DAB7`, stack base is `DF00`; the single transient itinerary byte is excluded from the unchanged 58-byte version-8 save. Static reserve does not measure deepest stack.

Six constant 121-word progress tables and sixteen two-byte near pointers occupy 1,484 native ROM bytes in bank `14`, shared with pose/bounds/sweep. The source generator derives them from authored path lengths; independent host arithmetic and compiled-byte/bank guards verify them. After path changes, run `create_streetcar_progress.py` deliberately; `--check` is read-only. `make check` includes freshness, motion and compiled-guard fixture checks. Inspect the actual compiled pair separately:

```sh
python3 scripts/check_rom_memory.py games/toronto-dispatch/project/build/toronto-dispatch-route-feedback.gbc.debug/symbols.noi --min-stack-reserve 1024
python3 scripts/check_streetcar_progress_rom.py --rom games/toronto-dispatch/project/build/toronto-dispatch-route-feedback.gbc --noi games/toronto-dispatch/project/build/toronto-dispatch-route-feedback.gbc.debug/symbols.noi
python3 scripts/check_aircraft_rom.py games/toronto-dispatch/project/build/toronto-dispatch-route-feedback.gbc games/toronto-dispatch/project/build/toronto-dispatch-route-feedback.gbc.debug/symbols.noi --require-streetlife
python3 scripts/check_streetcar_rom.py games/toronto-dispatch/project/build/toronto-dispatch-route-feedback.gbc games/toronto-dispatch/project/build/toronto-dispatch-route-feedback.gbc.debug/symbols.noi
```

The new native stationary sequence counts 381 updates / 1,080 VBlanks, the same as separate tram-only `a638…`, versus repeated-phase baseline `c625…` 348. See [PERFORMANCE.md](PERFORMANCE.md) for scope and phase variation; this is not whole-city or hardware acceptance.

## Tested toolchain

The aircraft renderer uses the compiled-pose cache, checks combined aircraft/shadow capacity before an aircraft-only fallback and computes roof-mask anchors once per tile. [TESTING.md](../TESTING.md) retains the earlier portability replay and original `8e7af3ec…` recordings separately from `9c1a9fcb…` / `4b83cfb6…` attempts. Native inactive/visible-flight samples still differ in NPC timing; compiled gates, reserve and host checks do not certify whole-city performance. Each later build needs its own identity and replay.

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
2. Build source using `rom_build`, a distinct `outputPath` under `build/`, and `captureDebugArtifacts: true` for authenticated matching symbols. Current measured output is `build/toronto-dispatch-ui-polish-direct.gbc`; preserve it and use a new name after later changes. Paths are relative to the selected project; each ROM needs its own NOI/source identity and replay. The plugin invokes GB Studio CLI `make:rom`.
3. Inspect ROM headers and digest, then run the explicit native memory guard below against that build's `symbols.noi`. A successful compile alone does not establish a safe WRAM layout.
4. Run `emulator_run` on that exact ROM, then use `emulator_step` / `emulator_observe` to test native frames. Retain the returned recording paths; stop, close and archive through the public plugin without deleting the original evidence.
5. Use `web_preview` to create the official GB Studio / Binjgb export (`make:web`) and open its returned URL in Codex's built-in browser. Keep the user preview available; do not reload during human play. Browser export and native ROM may have different digests and must be tracked separately.
6. Run `make check` from the repository root for content/connectivity checks and host engine regressions. It needs Python 3.10+ and Clang/GCC with ASan/UBSan. It does not compile a ROM or automatically inspect an ignored native build.

Builds, browser save states and cartridge backups are ignored. Do not publish local preview capability URLs, personal machine paths, activation codes or saves.

## City engine and reproducible sources

Current source registers six linked 1,024 × 976 scenes using the original project-local `TORONTO` scene extension, compatible with GBVM `4.3.0-e1`: `scene_toronto_city` (district 0), `scene_toronto_west` (1), `scene_toronto_high_park` (2), `scene_toronto_east` (3) `scene_toronto_port_lands` (4) and `scene_toronto_islands` (5). Their logical atlas is 4,096 × 1,952, with Port Lands below East and Islands below Core; existing mainland coordinate systems are preserved. Source totals are 241 buildings, 595 fixed pedestrian routes, 24 non-core traffic loops and 15 reciprocal seam pairs; only six nearby pedestrian actors are active in the loaded scene. Port Lands connects solely through Leslie; Core Cherry and Carlaw gateways remain withheld. These are compressed districts, not full former Toronto coverage. The extension adds original engine files without ejecting GB Studio; the aircraft renderer also overrides one pinned MIT-licensed GBVM actor source, documented in DISTRIBUTION.md. `Development boot` remains a separate workshop scene. [DISTRICT_ENGINE_PLAN.md](DISTRICT_ENGINE_PLAN.md) describes compiled scene binding and the genuine VM change-scene bridge; [PORT_LANDS_PLAN.md](PORT_LANDS_PLAN.md) records the new district's original generation and official factual references.

From the repository root, with Python and Pillow available:

```sh
python3 games/toronto-dispatch/scripts/create_city_art.py
python3 games/toronto-dispatch/scripts/sync_city_resources.py
python3 games/toronto-dispatch/scripts/create_west_art.py
python3 games/toronto-dispatch/scripts/create_east_art.py
python3 games/toronto-dispatch/scripts/create_port_lands_art.py
python3 games/toronto-dispatch/scripts/create_island_art.py
```

These commands generate original artwork/metadata and synchronize existing core resources. They do not register new scenes or apply changed west/east/Port Lands collision and attribute resources. Use the plugin's revision-aware native workflow to register/update those assets and scenes, preserve bindings, and apply the intended reciprocal seams before continuing. Regenerating content against stale native geometry is not a valid build procedure. Current source has all six native scenes registered. Islands is ferry/foot only; no ordinary road seam, car route, traffic loop or boat route is generated there.

After the registered resources match the authored geometry:

```sh
python3 games/toronto-dispatch/scripts/create_district_world.py
python3 games/toronto-dispatch/scripts/create_district_jobs.py
python3 games/toronto-dispatch/scripts/create_east_jobs.py
python3 games/toronto-dispatch/scripts/create_port_jobs.py
python3 games/toronto-dispatch/scripts/create_campaign.py --content-only
python3 games/toronto-dispatch/scripts/create_world_routes.py
python3 games/toronto-dispatch/scripts/create_audio.py
python3 games/toronto-dispatch/scripts/create_atlas.py
python3 games/toronto-dispatch/scripts/create_streetcar_sprite.py --check
python3 games/toronto-dispatch/scripts/create_aircraft_sprite.py --check
python3 games/toronto-dispatch/scripts/create_city_sprites.py --check
python3 games/toronto-dispatch/scripts/create_boat_sprite.py --check
python3 games/toronto-dispatch/scripts/create_traffic_signals.py --check
make check
```

`create_city_art.py` draws original indexed-colour background and sprite source cells; the background is already registered as a native asset. The sprite generator produces an editable source/metadata pair in `original-art` / `dispatch_topdown.metadata.json`. Existing sprite PNG changes must be applied to the registered `assets/sprites/dispatch_topdown.png` as part of a deliberate sprite edit. New sprite registration uses the plugin's validated `native_metadata` import; keep the existing root and bindings when editing an established asset.

`create_west_art.py` draws the original west/High Park backgrounds; `create_east_art.py` draws the original eastern background, including the Leslie southern approach; `create_port_lands_art.py` draws original industrial, park, beach and supported-bridge art. They write placement, collision and priority metadata without registering/updating native scene resources. Port Lands source uses 117 raw / 98 flip-canonical patterns, 26 buildings, six swept-clear traffic loops and 79 foot routes; East has 43 buildings after opening Leslie. After geometry changes, apply collision and attributes through the plugin before regenerating routes/contracts; the checked-in registered scenes are the input to those checks. `create_district_world.py` generates 15 reciprocal seam pairs and 24 non-core traffic loops. `create_district_jobs.py` authors eight western package contracts from actual scene collision paths; `create_east_jobs.py` appends eight eastern contracts and eight service points while pinning the earlier 80-contract/35-stop prefix. `create_port_jobs.py` appends eight original contracts/clients after the pinned 88/51 native prefix; current `create_campaign.py` compiles 96 contracts / 59 stops and seven parking rows. `--content-only` retains the existing font. Queen platforms remain 43–50 from `content/streetcar.json`; no 114 service is registered. Port deadlines remain provisional. Fire Hall Books passed a scoped native replay on retained `c625…`; the other seven new jobs remain pending. The added-district art and eastern job generators support `--check` for read-only freshness checks; Port Lands also supports an in-memory `--dry-run`. `make check` includes generated-source checks.

`sync_city_resources.py` uses GB Studio's native byte-array RLE to write palette/background-priority attributes and the core scene collision map. Native CGB attribute bit 7 marks raised roof lips/canopies. The scene extension interprets collision values 0 as road, 16 as walk-only pavement/Island ground and 15 as solid; the normal engine ladder meaning of bit 4 does not apply to this custom scene. Each district has 15,616 tiles, so each tile/attribute/collision array fits one 16 KiB bank. `check_campaign.py` and `check_district_world.py` inspect registered resources for stop connectivity, compatible road routes, ferry links, reciprocal seams, traffic clearance and contract consistency.

Car physics stores local Q4 coordinates and smoothed velocity; GBVM actors/camera use Q5. Motion catch-up is bounded independently from the full 16-bit VBlank clock; menus freeze the clock. Input edges are consumed once per render, and continuing curb contact preserves forward momentum. The text UI caches unchanged rows. Banked `td_routes.c` keeps the full pedestrian tables in ROM and selects six routes into a 24-byte WRAM coordinate cache; visible slots retain their route identity.

`td_roads.c` checks the complete vehicle body/cardinal swept rectangle through stock NONBANKED `tile_col_test_range_x/y` with mask 255. It preserves `tile_hit_x/y` and keeps collision-bank switching inside fixed-ROM code, adding no persistent terrain RAM. This preserves exact coverage of rails/terrain rather than skipping interior tiles. The registered-map host gate passes 1,950,051 checks; native bank ABI and cost remain separate.

Current street modules capture native fleet/civilian/boat loaders before cloning. Six road slots show car/truck/police/fire/ambulance/bus and six nearby walkers use original human art. Local police choose actual-road waypoints with normal guards and driven patrol return; all paid RIDE modes suppress pursuit. Ordinary motion uses sixteen active VBlanks, pursuit four; safety/courier/person/attention stay each active update, guarded overlap escape per-render. Eight-pixel delayed-update caps do not guarantee nominal traffic speed. Each motion batch uses a caller-owned 87-byte native stack snapshot, with no added persistent RAM or save fields. Begin validates all six vehicle coordinates/extents, visible pedestrian bodies and the active parked car once. Twelve bytes of 16-pixel centre buckets replace the earlier 48-byte hull-edge arrays; five-bucket separation rejects only bodies beyond the maximum 58-pixel sweep/priority reach. Nearby bodies retain exact full-body checks, with cached visibility/priority mask iteration. Admission records a pending candidate; only after terrain and tram guards pass does commit advance its coordinates/buckets and the matching live cache. Later movers see accepted endpoints sequentially; red, blocked or aborted proposals never become occupancy. Validated eight-Q4 separating retreat keeps its legacy guards and commit protocol. Each batch rebuilds the snapshot. The hot-path `td_traffic_epoch_move` now performs admission, whole-body terrain/future-tram guards and commit in one banked call. Private begin/accepted commits prove the old body, removing only its redundant revalidation; public old-position arguments still require exact cache equality, and every candidate retains extent/map bounds, cardinality and 8/128-Q4 limits. Validated separating retreat keeps the existing caller-proven full retreat guard before its preserved future-tram exception. Failed moves clear pending ownership and leave bodies/buckets fixed; successful moves publish the same endpoint to the live cache. The standalone BANKED signal-stop API moves unchanged to its own translation unit, duplicating generated constants in ROM rather than adding RAM, after two 16-KiB traffic-bank failures. Light caching avoids unchanged-cell writes. Boats use one OAM object and an independent scratch pair, full-hull water validation and deck clipping; combined capacity admission preserves ground actors.

Current pedestrian body guards use exact Q4 vehicle centres and radii 144/160 Q4; fleet Q4 positions convert directly to native Q5 actors. The Core visible bus targets `(640,72) → (216,72) → (216,168) → (640,168) → (808,168) → (808,72)`, with matching recovery/direction tables and cold `(216,72)` / leg 2. This fictional traffic loop is not a new real TTC service or paid-boarding schedule. Actual compiled bytes and scoped native bus passes are separate from the source-specific nine-pixel human wait/resume regression.

`create_city_sprites.py` and `create_boat_sprite.py` preserve editable source art/metadata separately from native registration; import changed assets with the plugin's validated metadata workflow and retain dedicated loaders, palette bindings and compiled isolation checks. `create_traffic_signals.py` derives lights from registered authored roads. Their `--check` modes are read-only. Focused checks are `scripts/test_roads.py`, `test_police.py`, `test_traffic.py`, `test_people_hotspots.py`, `test_city_sprites.py` and `test_boats.py`; `make check` includes them. Source/host success does not establish a compiled allocation or native gameplay. Current traffic/light gates pass 2,436,684 / 12,842 checks; atlas API/UI pass 1,181,358 / 25,109,617. The current engine gate passes 804,856, pedestrian checks pass 2,430,335 and full `make check` passes. Historical counts below remain attached to their own builds.

`create_atlas.py` reads the six registered collision grids, authoritative district offsets and authored water masks, then writes `content/atlas.json`, the BANKED `td_atlas.h` / `td_atlas.c` API and independent units generated by `atlas_banks.py`. It changes no scene or collision resource. The 512 × 244 schematic, padded to 512 × 248, uses one map pixel per native 8 × 8 collision tile; its 20 × 12-tile viewport browses six areas without changing the loaded gameplay scene. All 900 viewports fit the 172-slot cache, at worst 164 distinct patterns. The 679-pattern dictionary, 1,984 indices and six names/origins use 14,970 ROM bytes split across two pattern units and one index unit, with no new persistent atlas WRAM. Lower-left unregistered cells are solid and unnamed. The renderer shares the existing 360-byte text cache, uses bounded double-hash pattern lookup and retains 28 transient WRAM bytes for view/actor/camera restoration. These fields are absent from the 58-byte save schema. [CITY_MAP.md](CITY_MAP.md) documents coordinate units, focus controls and original water interpretation.

For bounded source/API/renderer checks without a ROM build:

```sh
python3 -B games/toronto-dispatch/scripts/create_atlas.py --check
python3 -B scripts/test_atlas.py
python3 -B scripts/test_atlas_ui.py
python3 -B scripts/test_atlas_banks.py
python3 -B scripts/test_district_seams.py
```

The API and renderer fixtures compile unchanged production C with host adapters and sanitizers. They do not establish native redraw timing, human readability or physical behaviour; actual build-specific samples appear below.

The recorded four-scene build moves district names, portals, graph traversal and non-core traffic tables into `td_world.c`, linked in ROM bank 15. District routing selects feasible hops for the current car/foot mode, then approach distance; it does not solve street-level routes. The HUD/beacon cache the next district, refresh when entering/leaving the car, and show `NO ROAD ROUTE` when no driving connection exists. Local foot-only handoffs retain `PARK THEN WALK`. Six traffic samples plus the next-district byte add 37 persistent WRAM bytes compared with Prototype 3; the 24-byte pedestrian coordinate cache is unchanged. Prototype 4 transit choices use the same departure window as boarding and display a countdown. Confirming during the two-second window boards immediately. The later Prototype 6 Queen module adds a fictional scheduled service across West/Core/East; neither milestone copies real timetables.

The banked auxiliary getter directs drivers to legal road parking anchors for Colborne 34, Withrow 36 and Greenwood 41, then restores the actual client target immediately on foot. It changes no client record or save field. Prototype 4 native Withrow play verifies the marker changing from road `(224,144)` to client `(320,144)` after exit, handoff and car re-entry. The atlas candidate retains this source, but its native remote parked-car and foot-only anchor/client marker samples remain pending. Colborne/Greenwood remain separate native handoff checks. The earlier `7a299125…` binary predates this fix.

Current source writes save version 9 with the same 58 bytes, including a 16-byte completion bitmap and separate player/parked-car districts. Obsolete saved cursor words become attention/countdown; valid v4–v7 imports clear them before attention validation. The atlas cursor is private. Two 66-byte CRC16 records alternate in SRAM bank 3 at offsets 0x100/0x180; the magic byte commits last. Reserved bit 0 still records a blocked paid Queen arrival with one second left. Version-6 records require their formerly reserved byte zero; version-5 migrates 48 bytes and valid v4 fallback retires obsolete active work while retaining earnings/completions. Earlier formats are rejected. Pre-v8 binaries cannot restore rewritten v8 records after both slots are replaced; old 88-job v8 binaries reject saved new job IDs/completion bits. CRC-first v8 validation uses frozen old terrain before relocating Island foot checkpoints and legitimate bookings into district5. All contracts, completion bits, money and mainland parked cars retain their identities. Restore leaves valid v5–v8 records intact until the next normal alternating v9 store; v4 retains its separate immediate upgrade. Pre-v9 binaries cannot read both rewritten v9 slots. Host interrupted-write/fallback/migration checks establish source behavior; native replay and physical power-off/interrupted-write persistence remain separate. Published Prototype 3–6 reset records retain their version-6 identities.

`td_session_live` is an internal UBYTE engine field at default 0. Stock generated bootstrap resets it on cold/soft boot; ordinary district changes preserve it. Session restore/audio initialization therefore run once, while each loaded scene rebuilds its local actors and UI. Keep its project setting at 0.

The source fixes `VM_MAX_CONTEXTS` at 8 through a project-local `cType: define` field targeting the copied `include/vm.h`. This is the stock compiler's file-backed engine-field workflow; it does not modify the installed toolchain. The shared heap remains 768 words and each context stack 64 words. Restore validates one 58-byte candidate at a time and migrates legacy tails in place to reduce native CPU-stack use. Current authored scenes have no concurrent actor/trigger scripts; reassess the fixed context pool before adding scripted entities. These settings remain compiled into the atlas candidate. Prototype 4's remote High Park reset and Prototype 3's earlier recovery evidence remain scoped to their separate binaries.

## Native memory guard

After each plugin build, run from the repository root:

```sh
python3 -B scripts/check_rom_memory.py --min-stack-reserve 1024 \
  "games/toronto-dispatch/project/build/toronto-dispatch-ui-polish-direct.gbc.debug/symbols.noi"
```

This command inspects the current direct UI artifact. For a later source build, substitute its own matching NOI/output/inspection. Stock GBVM reserves the `DF00–DFFF` page for its second OAM buffer, palettes and text tiles, and starts the downward CPU stack at `.STACK=DF00`. The checker rejects linker-area overlap with those absolute buffers, inconsistent/missing symbols and heap ends at or above the stack. Require 1,024 bytes of static reserve and current compiled aircraft/Queen/fleet/civilian/boat allocation/isolation gates; the threshold does not measure deepest native call paths or crowded pacing.

The first booting expanded candidate `36119ebf…` had heap end `DDA7`, stack base `DF00` and **345 bytes** of reserve. An earlier full-table cache ended at `DF90` and corrupted the reserved OAM page before Toronto initialized. Keeping only six coordinate pairs removed that allocation overlap, but the first booting candidate later failed a remote soft reset. Published Prototype 3's eight-context build ends at **D90F**, leaving **1,521 bytes** below `DF00`, and passed native reset samples. Prototype 4 ends at **D934**, leaving **1,484 bytes**. The optimized atlas candidate ends at **D950**, leaving **1,456 bytes** below `DF00`; its actual linked symbols pass the 1,024-byte guard. The renderer adds 28 bytes compared with Prototype 4, and its lookup optimization adds no further WRAM compared with `ec982d0c…`. Allocation checks and reset evidence remain separate: neither establishes physical persistence or every deepest call path. `make check` runs checker regressions, while the explicit command inspects the actual newly linked ROM.

## Retained Port campaign and clock candidate — scoped first new-job evidence

| Identity | Value |
| --- | --- |
| Output | `project/build/toronto-port-campaign-clock.gbc`, 524,288 bytes |
| ROM SHA-256 | `c625e6dce80a56a787c940885da5abd332fce47335e52f57698c3395a14ba364` |
| Source fingerprint | `ed6b11e18955ff84532f06ab8ad40de69a034159b392f5e7ef3b15ee868d32c6` |
| Matching NOI SHA-256 | `1e4d7d05715fb86e3552594b2c3511bba526c801f37867b9874841435435f05f` |
| Globals SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Header | CGB-only; MBC5+RUMBLE+RAM+BATTERY; 32-KiB SRAM |
| Heap / stack / static reserve | `DAB6` / `DF00` / 1,098 bytes |
| Scenes / contracts / stops / parking anchors | Five / 96 / 59 / seven |

Official build completes in 60,620 ms; inspection, matching compiled aircraft/Queen gates and full source/host suite pass. Local ROM/NOI/globals digests match the authenticated build. Existing 51 stops / 88 jobs/briefs, save v8 / 58 bytes and 16 completion bytes remain unchanged. Source Port routes use only Leslie and existing package rules, with no new transit service. A shorter VBlank conversion preserves subsecond/timer semantics without persistent RAM.

Exact-ROM ordinary controls deliver original jobs 0,2,1 at condition 100/cash 401 / done 3, then unlocked FIRE HALL BOOKS (index 89/contract 90): Riverside pickup, sole Leslie entry, Port parking, legal east-side foot approach around the Fire Hall, delivery at 10462 with condition 36 / cash 71 / done 4/bit 89, genuine four-button reset restoring that saved completion/foot/Port car, and re-entry at 11024. [The portable record](NATIVE_PORT_CAMPAIGN_SAMPLES.json) is closed/archived scoped passed. Exact higher fine amounts are not isolated despite H3/capture; other seven new jobs and tuning remain pending.

Stationary frames 482→842 record 110 loops/360 VBlanks versus retained fused 108; no substantial performance conclusion follows. Earlier `a243…` [six-deck car/foot/reset/Beach evidence](NATIVE_PORT_DRIVING_SAMPLE.json) belongs to its 88-job ROM, not this expanded artifact. Full96 campaign/two hours, deeper stack, whole-city pace, browser and physical boot/save/audio remain unverified.

## Retained fused candidate — scoped build/native evidence

| Identity | Value |
| --- | --- |
| Output | `project/build/toronto-streetlife-fused.gbc`, 524,288 bytes |
| ROM SHA-256 | `a243834899097d9660190022f03ce31d322d2584d0d7cfe1fdf8adb84a8902ea` |
| NOI SHA-256 | `f56570b2eea99e62d634bf7fd2e6a1643696fdd0aa3aa58f9f7356f841e3bdc3` |
| Source fingerprint | `f9c81b07cca407abcd83f8e3fd4687cdbc2b6042e018f86feb000b4c1ff8ffb4` |
| Heap / stack / static reserve | `DAB6` / `DF00` / 1,098 bytes |
| Compiled OBJ tiles, Core/West/High Park/East/Port | 116/116/106/116/106 of 128 |
| Bank-1 BKG tiles | 16/15/0/0/0 below aircraft scratch 32 |

Official header/compiled exact art poses/resource gates and final full `make check` pass. Two earlier official stages exceeded the 16-KiB traffic bank at 16,619 then 16,659 bytes. Splitting unchanged standalone BANKED signal-stop code plus generated ROM constants into `td_traffic_signal_stop.c` resolves that allocation without persistent RAM or changed checks. Fused `td_traffic_epoch_move` retains all guards and candidate bounds; private-proven old bodies avoid only redundant validation. Public old/candidate APIs retain their arguments/cache-equality guard.

Correct `_game_time C0B9` pacing records 108 loops / 360 VBlanks; wrong-address `C0B4` evidence is archived `needs-review`, excluded. Final ordinary controls sample visible impact/H1/stumble, exact paused game/flight/boat state, genuine saved-attention reset, police road approach/capture, park/exit/walking/blocked Core shore and a visible boat. A separate fresh same-ROM first delivery reaches cash 139/condition 100/done one at frame 790. Both recordings close/archive scoped `PASSED`; no source changes follow the build. [TESTING.md](../TESTING.md) and [NATIVE_STREETLIFE_SAMPLES.json](NATIVE_STREETLIFE_SAMPLES.json) retain exact scope. Static reserve, native samples and full host checks do not verify whole-city pacing, H2/H3/escape/paid rides, every boat/bridge/crowd case, deepest stack, two-hour gameplay or physical execution.

## Retained cadence candidate — scoped build/native evidence

| Identity | Value |
| --- | --- |
| Output | `project/build/toronto-streetlife-cadence.gbc`, 524,288 bytes |
| ROM SHA-256 | `bd09f1c30f8dd5be35f56c1da58fcdff37b343ecb669bea2b39353aa1d89388f` |
| NOI SHA-256 | `e668853e82feba0831a1a7e8103238509bf81e1588df5a06877ef437be2cb379` |
| Source fingerprint | `4fba44146e887e0d4ee1f4c0c5b0eadbb79bdcde184af5e478745deb939f6c60` |
| Static reserve | 1,098 bytes; deepest call paths unmeasured |

Official header/compiled exact poses/resource limits pass. Paired pacing reaches 107 loops / 360 VBlanks and ordinary first delivery reaches cash 139/condition 100/done one at frame 1,158. Earlier 123-byte hull `555f3d31…` separately passes scoped impact/stumble, map/attention reset, police capture and visible boat at 91 loops / 360 VBlanks. These samples are not whole-city or hardware acceptance and do not verify the later fused source. [TESTING.md](../TESTING.md) records exact scope; the final fused full suite passes, under its own source/build identity.

## Retained 75-byte epoch candidate

The earlier 75-byte epoch ROM `35337509…` records 90 loops / 360 VBlanks. Its ordinary-control courier sample delivers at frame 1,872: cash rises from 30 to 122, carried condition is 86 after traffic/curb contacts, unique completion count becomes one, the job clears and the result screen appears. Continued steering, reverse and dispatch controls work in that scope. Diagnostic `90ea51fd…` reaches 95 loops / 360 only with external terrain/future-tram guards cut; its recording is closed/archived and full guards are restored. That cut is never gameplay acceptance, and neither sample verifies the current 87-byte cadence/fused candidate or whole-city performance. [TESTING.md](../TESTING.md) and [NATIVE_STREETLIFE_SAMPLES.json](NATIVE_STREETLIFE_SAMPLES.json) retain the separate build/replay scope; do not adopt its source/debug identity for a newer source build.

## Retained five-scene Port Lands baseline

| Identity | Value |
| --- | --- |
| Native output / bytes | `project/build/toronto-port-lands-labels.gbc` / 524,288 |
| ROM SHA-256 | `a212dd9ed479310a98e58b701416f8af88aaa09174f186747dca678ff4c164f4` |
| Matching NOI SHA-256 | `b4a500d607a73fff08d1f4764b543ff3798d556aca302e9ead4b56c7f3d9649f` |
| Globals SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Build source fingerprint | `025edad8ffaa318a4739c0c1f4aee3b539789a8808dd1c11a20a1d9943dab98b` |
| Plugin project revision | `151a988b2b77e3312da36ee82d0f376b8fc15369fe21bbee0780b7d47b8682cc` |
| Linked heap / stack base / static reserve | `DA68` / `DF00` / 1,176 bytes |

The official build and full `make check` pass. Public CGB/MBC5/32-KiB-SRAM header inspection, compiled aircraft allocations in all five scenes, Queen frames and the actual-ROM 1,024-byte minimum reserve guard pass. In particular, bank-1 backgrounds remain below aircraft scratch IDs 32–46. The atlas API/UI host suites pass 1,475,876 / 24,151,786 sanitizer checks, with independent banking and horizontal/vertical seam fixtures. Static reserve and host adapters do not prove deepest runtime stack, universal frame pacing or physical bank switching.

The final travel recording spans 26,734 video frames / 2,181 events. It boards Queen 46→49 once for cash `30→27`, enters the actual Port Lands scene on foot at frame 7,640, reaches Cherry Beach at 13,636, walks the Unwin crossing and stops at blocked shore `(432,919.5)`. Map frames `13,976→16,076` preserve all 58 game and 13 cosmetic flight bytes. A genuine game-button reset restores the saved Beach checkpoint, car parked in Core `(560,720)`, cash 27 and world second 227. Both four-object plane and helicopter poses, rotor change and separated shadows appear with sampled peak 10 objects per scanline and no over-limit scanlines. The courier returns from district 4 to actual East district 3 at frame 26,554. This is foot/scene/checkpoint evidence; it does not verify car travel through Port Lands or every bridge/arrival.

A separate fresh 1,204-frame / 165-event recording repeats contract 01 for cash 139, one completion and condition 100. The held turn reaches speed 20, continued A reaches 24, reverse reaches −6, steering/acceleration recovers `6→18`, and neutral input coasts to zero. A third bounded timing sample reads 132 updates over 360 VBlanks; it does not establish whole-city/crowded performance. All three scoped final recordings are closed/archived under their own exact-ROM identities in [TESTING.md](../TESTING.md). The earlier `6a8a…` Port Lands exploration remains `needs-review` after the wrong street HUD label; the corrected final journals do not overwrite it.

Full campaign/two-hour/human fun acceptance, all seam/vehicle/bridge cases, forced HOLD/contact recovery, crowded CPU/occlusion/deepest stack, browser refresh and physical hardware remain open. No Port Lands clients/jobs or new bus service are appended yet; 88 contracts / 51 service records remain. Published Prototype 6 is still the separate downloadable four-scene timetable/safe-alighting ROM.

## Previous contact-corrected candidate

| Identity | Value |
| --- | --- |
| Native output / bytes | `project/build/toronto-contact-recovery.gbc` / 524,288 |
| ROM SHA-256 | `1400566247fddfa9a1acd5ef42e5f90bb19db9caa43a51fab0ce82400caa137f` |
| Matching NOI SHA-256 | `6f8b6d662fe3d080a8a0a00f5d398521a35e4ddc62dcfddec5125b942264ea68` |
| Globals SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Build source fingerprint | `e6bafd235ddb547928f8c760672c20d4f55518f99f24d519e3758142d6a253dd` |
| Plugin project revision | `03e03e4f63cda3ece1d5732ca557e5f3f62ea88a1cf08cb92056342073519f64` |
| Linked heap / stack base / static reserve | `DA68` / `DF00` / 1,176 bytes |

The first build rejected `TORONTO.o` at 16,750 bytes against the 16,384-byte bank limit. Moving the pure authored-route segment query into the existing banked streetcar runtime resolves this allocation without reducing checks or changing saved state. Two transient WRAM bytes track traffic retreat and tram impact episodes; save v7 remains 58 bytes. Public inspection verifies CGB-only MBC5+rumble+RAM+battery, 32-KiB SRAM and valid header/logo. Aircraft/Queen compiled frame, tile and scene allocation gates pass. The 1,024-byte static-reserve guard passes, without proving deepest runtime stack use.

Full `make check` passes 476,667 actual-C engine and 8,036,664 atlas/UI checks, together with 4,501,851 flight, 40,631,157 renderer and the retained content/transit/bridge/navigation/memory suites. Host assertions cover coherent forward/reverse retreat, one tram penalty under persistent blocked acceleration, invalid no-motion behavior and explicit booked first-arrival/HOLD bodies. Native scoped passes separately verify an ordinary save-reset overlap recovering, three Queen fares/journeys with a paid reset and exact map freeze, plane/helicopter rotor/shadow samples and first delivery/driving. [TESTING.md](../TESTING.md) preserves all three closed/archive journal identities and the old failure.

Forced blocked-arrival/HOLD and rare rail-retreat native scenarios, remaining legacy services/seams, corrected-ROM roof restoration, crowded CPU/deepest stack, full campaign/city and physical hardware remain open. Earlier hosted GNU CI results below belong to earlier commits; verify the new source commit's jobs separately.

## Previous aircraft portability candidate

| Identity | Value |
| --- | --- |
| Native output / bytes | `project/build/toronto-aircraft-portable.gbc` / 524,288 |
| ROM SHA-256 | `20370fea5661e2b9789bf6b1f77dc84535f06348698e2de3e6652f013a5ae189` |
| Matching NOI SHA-256 | `cda7c22498fad7c73b30e7cf10ac778147d0eee93af105fd564ac021d8f430bb` |
| Globals SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Rebuild source fingerprint | `8eca3164dd29a33ade067e2e4b4ecc145cc7a023c55028f3c537fae71801bc09` |
| Plugin project revision | `03e03e4f63cda3ece1d5732ca557e5f3f62ea88a1cf08cb92056342073519f64` |
| Linked heap / stack base / static reserve | `DA66` / `DF00` / 1,178 bytes |

GCC CI at `a3b6984` rejected misleading indentation in the renderer/harness. The correction changes whitespace only and retains `-Werror`; full local source checks pass with the same counts below. Hosted GNU CI passes at source-fix commit `3a1869a` in both PR/push runs, including real full `make check` steps; [TESTING.md](../TESTING.md) retains run links. The official rebuild, public CGB/MBC5/32-KiB-RAM inspection and aircraft/Queen/memory gates pass; compiler, NOI and globals identities match the preceding build.

Independent byte comparison finds only the global checksum byte `0x14F` and `_save_signature` bytes `0x481–0x484` changed; all other ROM bytes are identical. The old recorded source fingerprint `37927f56…` and journals remain separate from rebuild fingerprint `8eca3164…`. Fresh exact-ROM native scoped `PASSED` repeats delivery/driving, a partly clipped westbound plane, two fully visible southbound helicopter rotor poses with shadow and exact paused-map gameplay/flight freezing. [TESTING.md](../TESTING.md) records its new immutable journal/archive identity. Roof/transit/all-scene/reset scenarios were not repeated on this new ROM; their retained `8e7af3ec…` evidence below is not relabelled. Broad/hardware acceptance remains pending.

## Previous aircraft candidate: retained scoped native samples

| Identity | Value |
| --- | --- |
| Native output / bytes | `project/build/toronto-aircraft.gbc` / 524,288 |
| ROM SHA-256 | `8e7af3ec08349c4dbef473bae30ac940ca727c0524b04b9711c04817b8678258` |
| Matching NOI SHA-256 | `cda7c22498fad7c73b30e7cf10ac778147d0eee93af105fd564ac021d8f430bb` |
| Globals SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Build source fingerprint | `37927f56eff7941139717c22cfd8d8bae16662a36e866c4b7a3fc27751a4d4d1` |
| Plugin project revision | `03e03e4f63cda3ece1d5732ca557e5f3f62ea88a1cf08cb92056342073519f64` |
| Linked heap / stack base / static reserve | `DA66` / `DF00` / 1,178 bytes |
| Renderer cache | 170 persistent bytes: two 80-byte aircraft templates, 8-byte shadow and 2 tags |

The official build, CGB-only MBC5 header, aircraft/Queen compiled-frame/resource gates and memory guard pass. Local binary/debug-artifact digests were independently read. Full `make check` passes 4,501,851 flight, 40,631,157 renderer and 110,995 engine checks, alongside the retained suites. Static reserve and host checks do not establish deepest native stack use or physical behaviour.

Three stopped, closed and archived ordinary-input PyBoy recordings on this exact ROM have scoped `PASSED`: plane/helicopter/rotor/shadow with exact 13-byte flight/58-byte gameplay map freezing and camera/roof/scroll/pause restoration; fresh first delivery with held steering, solid-terrain stop and braking/reverse recovery; and another delivery followed by a genuine button reset restoring its saved cash/completion/position/clock checkpoint. The separate journey recording reviews three single-fare Queen journeys through actual Core/West/East and walking into an actually loaded High Park scene, with parked-car identity retained and a High Park helicopter/shadow sample. A mistaken tool-level restart then clean-booted a new worker and automatically stopped that journal without an assessed `PASSED` label. It is preserved as such; it proves neither ROM save failure nor paid-reset recovery.

[TESTING.md](../TESTING.md) preserves the exact native values and all immutable journal/archive identities. A sample reaches 47 updates / 120 VBlanks over a partly visible plane interval; separate inactive 18/60 and helicopter 15/60 samples have different NPC phases and are not a matched global benchmark. Crowded performance, the three contact/held-arrival/alighting branches and exact-ROM paid-ride reset remain pending. Published Prototype 6 remains unchanged; this candidate has no physical cartridge acceptance.

## Aircraft cache intermediate: further optimization pending

| Identity | Value |
| --- | --- |
| Native output / bytes | `project/build/toronto-aircraft-cache.gbc` / 524,288 |
| ROM SHA-256 | `4b83cfb68cd6d9d4d769faaa1afecffb52e1c32e91a54c66110dea143263b895` |
| Matching NOI SHA-256 | `d6b71db62352e9fd961207068db2136049fc403712ee84c970b5a0831b29037a` |
| Globals SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Build source fingerprint | `c03f04eb3f6d108f5c9b9c4435de54ff8cea638b581579e21c1ab0fa7312b7f1` |
| Plugin project revision | `03e03e4f63cda3ece1d5732ca557e5f3f62ea88a1cf08cb92056342073519f64` |
| Linked heap / stack base / static reserve | `DA66` / `DF00` / 1,178 bytes |
| Renderer cache | 170 persistent bytes: two 80-byte aircraft templates, 8-byte shadow and 2 tags |

This official build is retained as an intermediate; it is not the final aircraft ROM or a recommended loading artifact. Matching compiled aircraft/Queen frame and resource gates pass, with 40,047,786 actual-source renderer host checks. Its ordinary-button PyBoy replay samples planes, a helicopter's two rotor phases, shadows, 13-byte flight/58-byte game-state map freezing, world-fixed flight paths during scrolling and priority-roof patch restoration. The recording ends at frame 10,127 with `needs-review`, then is closed/archived. [TESTING.md](../TESTING.md) retains its source-debug/worker identity, observations and immutable recording/archive digests.

Scoped update samples are 40/120 VBlanks while aircraft-inactive, 14/60 during a helicopter and a later inactive 18/60; their NPC phases differ, so they are not a matched global benchmark. Further common-path optimization, another official build and exact-ROM acceptance replay remain necessary. Delivery, transit, reset and district transitions have not been accepted on this intermediate; earlier moving-tram evidence does not transfer to it. Static reserve does not establish deepest native stack use or physical cartridge behaviour. Published Prototype 6 remains unchanged.

## Previous moving-Queen candidate: scoped native checks

| Identity | Value |
| --- | --- |
| Native output / bytes | `project/build/toronto-streetcar-one-pass.gbc` / 524,288 |
| ROM SHA-256 | `a0e23f030425ebf0f0435f5a88d6a3c17eb36564d5dedc2c2871c5e03af13eeb` |
| NOI SHA-256 | `35d3c9ad7656768a9f9ceab79dd0bf0e3a85505bfc5ea088eda22afdcd86c266` |
| Globals SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Build source fingerprint | `99494efe984da1354dccf2f8b3a1de6fb113b4f45bf38b67ef516570ab94ebf8` |
| Plugin project revision | `f9c648669ddf0df7ec2d34550dad083dd613411ef3f3be4c08aad358973761c1` |
| Save / memory guard | Version 7, 58 bytes / `D968` heap, `DF00` stack, 1,432-byte reserve |

Full `make check` passes with 110,986 engine, 3,771,783 streetcar-motion and the unchanged atlas/transit/bridge/navigation suites, plus nine memory tests. Matching compiled-frame inspection passes eight four-object poses and empty frame 8; linked allocation passes the 1,024-byte minimum reserve. Static compiled descriptors show 56 courier + 10 tram OBJ tiles per bank (**66 / 128**). Native frame 5,592 samples four tram objects, peak four per scanline and no over-limit lines; neither this view nor static tile counts establishes every crowded scene or palette/handheld readability.

One-pass scalar traffic is restored; direct clock-derived phases/additive ranges remain. Native recordings on this exact ROM repeat three paid Queen rides, exact 58-byte paid-map freezing, paid soft reset and ordinary Union car re-entry. The matched Core sample advances 57 updates / 120 VBlanks (~28.5/s), above b1cf's 45 and the failed 84e5's 35. A fresh same-ROM recording confirms contract 01 delivery/cash 139 and held-acceleration speeds 10→22→16; frame 864's pedestrian brake gives 16 rather than the historical 24. Reverse and steering recover from blocked terrain and reach 18. [TESTING.md](../TESTING.md) contains complete journals, observations and limits.

```sh
python3 -B scripts/check_streetcar_rom.py \
  games/toronto-dispatch/project/build/toronto-streetcar-one-pass.gbc \
  games/toronto-dispatch/project/build/toronto-streetcar-one-pass.gbc.debug/symbols.noi
python3 -B scripts/check_rom_memory.py --min-stack-reserve 1024 \
  games/toronto-dispatch/project/build/toronto-streetcar-one-pass.gbc.debug/symbols.noi
```

These read-only gates inspect the matching compiled frames/linked allocation, not source authenticity, complete playback or hardware. The candidate remains for review: fully crowded `BLOCKED` recovery policy, broader collision/traffic coverage, palette/crowded performance, full city, two-hour campaign and physical acceptance stay open. Published loading remains unchanged.

## Historical moving-Queen candidates

The newer source adds original rail pixels and a single streetcar actor with eight poses, four 8×16 OAM objects per pose and a 28×12 / 12×28-pixel body. Pure banked motion/sweep queries follow the retained 64-second fictional service cycle through West/Core/East. A banked presentation module derives the paid camera and actual viewed scene without replacing the saved origin or parked car; a held blocked arrival uses the version-7 reserved bit described above. WAIT displacement and bounded boarding-cue arithmetic have production-source regressions. Fully crowded contact recovery can return `BLOCKED`; its player-facing policy remains pending.

Full `make check` passed before the native presentation corrections: 3,450 engine, 299,366 atlas API, 8,036,093 atlas UI, 609,452 transit API, 3,771,783 streetcar-motion, 2,974 bridge and 16,997 navigation checks, plus nine memory-checker tests. Those host checks do not verify compiled metasprites or native bound units. [STREETCAR.md](STREETCAR.md) describes source behavior; [TESTING.md](../TESTING.md) retains the failed replay and original journal identities.

| Retained failed build | Identity |
| --- | --- |
| ROM SHA-256 / bytes | `deb78bbdc3f90535a787f08a60fed68afcc9477a862b61299629a453f02139ce` / 524,288 |
| Build source fingerprint | `fac772218749cdcc8089cf6541bdd54f71dafb3ba6cc9d8652488f0956d32bb3` |
| Plugin project revision | `81a99b21ea9687ef412ed4e2234ce4e05ccf0a8040e1d3757342a3ab0f0ba402` |
| Matching NOI SHA-256 | `6793cc89350ae99ee12fe505b46e8200d16ed6fffdd088f01cd32b78c131fdda` |

Compilation succeeded, but that native replay failed by frame 3,444 with missing tram OAM during a valid paid ride. Compiled horizontal tram frames were empty because the optimizer discarded negative editor-Y positions. The earlier PLAYER/actor-size hypotheses are withdrawn. Corrected source preserves PNG/native IDs with canvas-safe editor positions and converts runtime actor bounds to GBVM Q5. Engine fixtures pass 3,454 checks with zero failures, including all 3,840 cycle poses; full `make check` also passed at the corrected build's source checkpoint, before later performance edits.

| Corrected scoped candidate | Identity |
| --- | --- |
| Native output | `project/build/toronto-streetcar-render-fix.gbc` |
| ROM SHA-256 / bytes | `b1cf9a3790058b56e596d37f6e816642a8ddc18c9db855ea033d14c9ae3bdda8` / 524,288 |
| Build source fingerprint | `2e8ad18dddf40f7f54bd0a2ab1c2da49c6b00ab8646a0362e68ecb0d26964ff7` |
| Plugin project revision | `f9c648669ddf0df7ec2d34550dad083dd613411ef3f3be4c08aad358973761c1` |
| Matching NOI SHA-256 | `0ac98492d2642cb1e9acbae77b7743c62b22f089eca790ec40da55fc5b2f808b` |

The local bytes/digest match the table. Its ordinary-button recording shows horizontal approach/open doors, terminal vertical poses, paid Yonge 46 → Parkdale 43 followed by Parkdale 43 → Alton 50 through actual West/Core/East scenes, cash 30→27→24 and East alighting at `(880,524)`. A paid-map sample freezes clock/fare/ride and restores the visible tram after East-objective browsing. Native Q5 bound inspection agrees with the horizontal 28×12-pixel body. A paid core sample at world second 74 advances 45 game updates over 120 VBlanks, approximately 22.5 updates/second. Its journal was stopped as `needs-review` and archived; performance optimization and a later exact-ROM acceptance replay remain pending. [TESTING.md](../TESTING.md) records the full identities and limits.

Inspect compiled tram frames with the ROM and NOI from the same official output:

```sh
python3 -B scripts/check_streetcar_rom.py \
  games/toronto-dispatch/project/build/toronto-streetcar-render-fix.gbc \
  games/toronto-dispatch/project/build/toronto-streetcar-render-fix.gbc.debug/symbols.noi
```

The bounded read-only gate prints both hashes and decodes the banked metasprite pointer table and 8×16 cells. The matching `b1cf9a37…` pair passes eight four-object cardinal/door frames plus empty startup frame 8. The retained matching `deb78bbd…` pair fails all eight poses: horizontal frames empty, vertical frames two objects. Choose the actual matching pair and compare its hashes with the build record; this layout gate does not authenticate source/build provenance, prove playback or establish hardware behavior. Native pixels/OAM and linked-memory checks remain separate.

This candidate is not an accepted final moving-streetcar build or recommended loading artifact. Published Prototype 6/loading remains unchanged. Browser state is unresolved, with no claimed host restart or fresh browser verification. Hardware, full city and measured campaign duration remain unverified.

### Retained performance intermediate

| Identity | Value |
| --- | --- |
| ROM SHA-256 | `84e5fe205887586efbc27ddc9b86d4c2ef7cabb8e1921c8fcdd4851663728b0d` |
| NOI SHA-256 | `009d7e24f108f9c8dfe4564039baad26f8a0e7ce9d63625248ffe50e5c38d5b9` |
| Build source fingerprint | `9415894fd5d499de61fbce20f5a6068f7ab31a226a67baecfe7932827d2a85c7` |
| Plugin project revision | `f9c648669ddf0df7ec2d34550dad083dd613411ef3f3be4c08aad358973761c1` |
| Linked guard | `D968` heap / `DF00` stack; 1,432-byte reserve |

Full `make check` passes with 418,184 engine checks and the previous other-suite counts unchanged; the matching compiled-frame gate also passes. The same fresh-boot native route repeats the two paid trips, East `(880,524)` alighting/cash 24, exact 58-byte paid-map freezing and tram restoration. However, its matched Core world-second 74 sample advances 35 game updates / 120 VBlanks (**17.5/s**), below `b1cf9a37…`'s 45 (**22.5/s**). The performance comparison fails. This stopped/archived `needs-review` intermediate is not an accepted final build or loading recommendation; [TESTING.md](../TESTING.md) retains its exact measurement and journal identities. Final optimization/replay remains pending, without browser or hardware acceptance claims.

Later phase-math intermediate `815ef0b065c375a43aa294e611ba67ee94801cc17297125b15f57ef4cf3096fb` passes the same full checks, 1,432-byte guard and compiled-frame gate. Direct seconds-derived section/tick and additive ranges restore 45 updates / 120 VBlanks (**22.5/s**) with batching unchanged. It also samples a third paid Alton→Yonge ride, paused soft reset and Union car recovery. This restores the earlier pace rather than establishing final performance acceptance; its journal remains `needs-review`. [TESTING.md](../TESTING.md) retains full binary/debug/source and recording identities. Further optimization and exact-ROM acceptance remain pending; published loading stays unchanged.

## Pickup condition lifecycle: later native candidate

Official output `project/build/toronto-pickup-condition.gbc` is 524,288 bytes, CGB-only, MBC5+RUMBLE+RAM+BATTERY with 32 KiB SRAM. ROM SHA-256 `64be19fa3da7ba4231ba8c8decec4720c409116c034f586974fd4ddce789945a`; source fingerprint `7a61baf9dfa688dd423be21b8444380abc1969481ee8a00286f15cae55c6ebf3`; matching NOI SHA `9e82436532668b27e9b68059dce8c9cb8696ddc06bb1a5e4b2cb854812843f18`. Project revision, compiler and globals digests retain the Prototype 6 values below. The official build exited 0 in 39,175 ms with the existing Node/SDCC warnings and no errors. Independent header and actual linked memory checks pass: `D950` heap, `DF00` stack, 1,456 bytes reserve against the 1,024-byte minimum.

Four damage paths now require an active carrying stage. A fifth startup guard repairs condition on valid older uncollected-job saves after CRC/semantic validation. Empty-collision/rider text follows that lifecycle. There are no new save fields, persistent RAM allocations, contracts, stops or assets. Actual-C regressions pass 3,058 checks, including 14 failures captured on the predecessor; full `make check` passes. Native exact-input replay preserves uncollected condition 100 versus the predecessor92, then verifies pickup, actual carried damage and condition-scaled delivery. A separate matched-phase first delivery and held turn retain cash139 and speed24/heading4. [TESTING.md](../TESTING.md) records immutable native identities and the limits of these samples.

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

`td_transit.c` holds pure BANKED service, selection, fare, duration and departure APIs, with no persistent WRAM. The Queen service uses eight source-referenced curb signs across West/Core/East, a fictional 64-second directional timetable, four seconds per stop and a $3 fare. It runs from the shared game clock while other scenes are loaded. Original train, bus and ferry services remain available. [STREETCAR.md](STREETCAR.md) records the compressed route and source limitations. Moving-streetcar art/runtime belong to the later source candidate above, not this published ROM. Original stop IDs 0–42 and all 88 compiled contract fields remain stable. At the Prototype 6 milestone, `create_campaign.py` compiled 51 stops; art fingerprints and planning metadata changed with the new signs. Current source counts are recorded above.

Arrival chooses the stop centre when clear, then nearby cardinal positions at 12 or 18 pixels. It checks bounds, destination collision and a swept walking path, the player's parked car, and currently loaded traffic. An obstructed arrival retains its paid ride and retries without another fare. Remote Queen platforms are on sidewalks; future remote road stops need destination traffic checks. Completion/expiry also refreshes the depot objective before saving, fixing a stale completed-job map target.

Two fresh ordinary-button recordings test this exact ROM. The first repeats the Market job and held-turn regression, three Queen trips loading Core/East/West, WAIT/RIDE map freezing, paid-trip soft reset and parked-car recovery. The second repeats subway/map/reset controls, rides the 94 Wellesley bus, returns to Union at `(572,720)` beside the car at `(560,720)`, walks away, rides the Centre Island ferry out and back, walks on the Island and re-enters the Union car. Native samples report 59 updates over 120 video frames and no over-limit OAM scanlines in their sampled views. Full timings, immutable journal identifiers and predecessor failure are in [TESTING.md](../TESTING.md). These samples establish neither every route nor hardware persistence or the complete campaign's duration.

Milestone identifier: `v0.2.0-prototype.6`. Its [published bundle](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) is 85,940 bytes, ZIP SHA-256 `ecfee71c28227ee8d48d3d841af76111139af97ce5e52be73cab92d867137c87`, declaring source commit `e85006f07d07ff628a1aad0a45dd9dc6271e4695`. Anonymous download and all member checksums pass. Use the matching [loading instructions](LOADING.md); publication evidence is in [TESTING.md](../TESTING.md). A fresh source build needs its own inspection and test identity. The older browser preview remains unresolved and does not display this ROM.

## Final Prototype 5 rebuild

Current official output: `project/build/toronto-city-atlas-portable.gbc`, 524,288 bytes, SHA-256 `2d1f6e4e7ae48a434757e63454d216b02b81957ecf5f8582d879149d447d7311`. Build source fingerprint `efe054a611bebeb91231f37db6102e71c1c305f2f861d09010491a4340f9aea4`. Project revision/compiler/header and matching NOI/globals retain the identities in the preceding optimized build's table below; official build and actual1,024-byte minimum memory guard pass with1,456-byte reserve.

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
