# Native performance record

Updated 2026-10-05. Preserve gameplay and graphical fidelity while removing repeated work. Selected ROM `toronto-dispatch-story-combat-r6.gbc`, SHA-256 `71a7e49363947d4f036300770e1b24da4a3df80e344bfe41675c74240bfbf2b8`, has [matched build](STORY_COMBAT_BUILD.json), [native play](NATIVE_STORY_COMBAT.json) and [optimization evidence](STORY_COMBAT_OPTIMIZATION.json). Its 243 native inputs remain unchanged. The linked heap `DCDC` leaves **548 static bytes** below stack origin `DF00`; HOME has 207 bytes free and scenes peak at 124/128 OBJ tiles. Static separation is not a deepest-stack measurement.

Current navigation's generated ROM prefix counts preserve exact mask ordinals
and reduce counted ordinal byte operations by **79.72%** in the scoped host
comparison. They add ROM data, not a runtime mask cache, and change no road,
body clearance, route or artwork. This measures the work of ordinal decoding;
it is not an emulator FPS result or a percentage gain for the whole game.

The exact **14-byte** repeated navigation-query cache keys the same goal,
district, travel mode and player cell. Its existing five-byte ground patch
makes **19 guidance bytes** total. The lifetime correction retains an
unchanged patch; it adds no guidance RAM. Forced modal and ordinary helper
restoration runs aircraft/combat/guidance/scenery in reverse composition order
and preserves newer owners and the original tilemap page in host checks. A
separate one-byte prepare-cycle flag makes restoration explicit. R6 observes
the retained road arrow in **30/30** consecutive sampled views.

R6 authenticates `_game_time` at `C0BC` from its own NOI. Separate Union windows
count **149 updates / 840 VBlanks idle** and **160 / 840 active**, roughly
**10.64 / 11.43 completed loops per emulated second** under the 60-VBlank
convention. These are scoped workloads, not display FPS, a matched causal gain
or proof of whole-city, human or physical smoothness. No traffic population,
collision guard, existing city artwork or palette is removed for speed.

R5 remains **needs-review** because its road arrow appeared in only 24 of 30 sampled
views. Its Union workload counts **148 updates / 840 VBlanks idle** and **159 / 840
active**. The phases differ from R4, so these samples establish no causal cache
gain. Preliminary R4 Union observations remain roughly ten completed engine
loops per emulated second; neither record establishes whole-city, human or
physical smoothness. Source equivalence cannot supply later ROM timing.

The accepted hospital badge uses one original bank-1 background tile 254 and no
new actor or mutable marker state. The one-star arrest hold adds one transient
byte. The actual 548-byte R6 reserve passes the 512-byte static guard; native
Core play observes hospital recovery, the badge, arrests and committed reset.
Other district recovery/failed queues/fatal-job overlap/SRAM interruption have
host checks. The hospital/arrest rules are explicit gameplay additions,
separate from optimizations that preserve existing behavior. Cartridge cold
boot, power-off saves, audio listening and physical flicker remain pending.

Retained sandbox-stable `096862ab…` keeps its [149/840 stationary sample](SANDBOX_STABLE_TIMING.json) and [lossless memory pass](SANDBOX_STABLE_OPTIMIZATION.json): 109 persistent UI bytes saved, 57 fewer reviewed automatic tram/traffic bytes and 579 linked static bytes. Those observations and its R2 package remain tied to that prior ROM.

Retained UI ROM `03e09fa8…` has one fresh exact-ROM Core workload trial: **385 completed loops / 1,080 VBlanks**, then **80 / 240** under fixed driving/braking inputs. These match the retained 825 vectors numerically, without inheriting their repeats or relative gains. Completed loops are not display FPS, human responsiveness, whole-city or physical acceptance.

The earlier read-only state, flight, counter and OAM observations are retained in [PERFORMANCE_SAMPLES.json](PERFORMANCE_SAMPLES.json). [TESTING.md](../TESTING.md) and [BUILD.md](BUILD.md) retain each candidate's broader build and gameplay scope. The later aggregated roof-mask change is outside both measurements.

## Direct UI candidate — one scoped timing trial, 2026-10-03

The [closed 03e09 trial](NATIVE_UI_POLISH_PERFORMANCE.json) pins ROM SHA-256 `03e09fa85351c91f56d7370a3f0e0f856cd147a3c8d5ebfd328a2786a38f25d6`, matching NOI `037dab5b328250b9dff9e508150f1527a5b1f85f0d3e62f0a28981e67c479130` and source fingerprint `1140f844d78d0f841a94509ad99609e8aac8fc00687b7f9e233da6b983f81744`. Fresh PyBoy 2.7.0 source boot uses A4/neutral56; frames 240→1,320 remain Core `(560,720)` / speed 0 / cash 30 / done 0 / H0. The actual counter is authenticated from this NOI at `C0BC`. Eighteen consecutive 60-VBlank modular deltas are **24,24,22,23,26,21,18,16,18,18,23,18,20,23,21,24,24,22 =385**.

A60/A60/B60/neutral60 yields **19,18,20,23 =80** across 1,320→1,560, ending stopped at `(685.3125,720)` with cash 30/H0. The one final OAM response reports peak 6/10 and zero overflow. Independent read-only audit passes **1,085 checks**, authenticating all 66 hash-linked events, 24 ordinary intervals, 92 typed memory receipts, one OAM response and all 27 archive PNG/RGBA files. The replay is closed/archived/retrieved; inert inspections remain separate from action-journal events. This is one trial with phase-correlated windows, not 18 independent samples. Earlier 825 repetitions and e797-relative 6.648%/3.896% comparisons retain their exact identities in [TESTING](../TESTING.md) and [the retained comparison](NATIVE_TERRAIN_CACHE_COMPARISON.json); no new universal gain, human duration, deepest-stack, browser or hardware result is inferred.

## Retained combined itinerary/payout sample, 2026-10-03

Official `project/build/toronto-dispatch-route-feedback.gbc` has ROM SHA-256 `4343f2b858e62f9e8daa2a3576a1d06bb7ee7208d2cd4a55434c36e168fb3100`, matching NOI `187bc1109ba38099dd1624264801beb20df578b2ed9a420753a7d88e0d0766d0` and source fingerprint `367106d4a72bf17b85cd2a7d294c978bdd844da664bec3806c8db7861ff1421b`. Build takes 150,248 ms. Heap `DAB7`→stack `DF00` leaves **1,097 static bytes**, accounting for the added itinerary byte; compiled sprite guards pass and the same 1,484-byte ROM tables remain in bank 14. Static reserve does not measure deepest stack. Host UI passes **24,185,014 checks across 796 itinerary pages**; the combined full `make check` passes with 704,884 engine regressions.

A fresh same-recipe 18-second stationary measurement independently repeats **381/1,080**, with blocks 115/123/143. The earlier tram-only recording establishes the paired 348 baseline below; this combined count verifies that result for its own ROM, without importing the older gameplay/Queen trip. It does not isolate UI drawing cost or establish scrolling/crowded/human performance.

Scoped ordinary controls verify wrap, ordered-return previews, Beach Mail's WALK/Port cues and locked denial. Accepting from a later delivery preview still targets Union pickup. The first delivery explicitly renders condition 100, base 86, remaining 116 seconds/bonus 23, credit 109, cash 139/done 1; a replay timeout shows zero payment with unchanged cash/unique completion. Rebuilt offers/reset return the preview cursor to pickup. A separate fresh same-ROM Line 1 Union→King trip deducts 30→27, freezes all 58 paid-map bytes, genuinely resets/resumes and arrives at King `(640,640)` with the original Union car `(560,720)` and paid fare retained. Native damaged/capped payouts, other contract types and cancellation remain broader gates; this ROM has no newly claimed Queen replay. [TESTING.md](../TESTING.md) retains recording/scenario identities.

## Tram-only progress lookup sample, 2026-10-03

Candidate `a638c374…` changes only native tram progress calculation: six generated 121-entry `UWORD` tables plus 16 same-bank near pointers preserve exact `floor(length*tick/120)` at both endpoints and every authored path. This is 1,484 bytes of static ROM data, with no persistent WRAM growth; schedules, doors, held arrival, full swept collision and paid-ride semantics are unchanged. Official build takes **61,893 ms**, with **1,098 bytes static stack reserve**. Linked reserve does not measure deepest runtime stack.

| Neutral stationary Core block | Paired `c625…` loops | Tram-only `a638c374…` loops |
| --- | ---: | ---: |
| First 360 VBlanks | 110 | 115 |
| Second 360 VBlanks | 112 | 123 |
| Third 360 VBlanks | 126 | 143 |
| Total 1,080 VBlanks | **348** | **381** |

The aggregate is `(381-348)/348 ≈ 9.48%`. These are paired 18-second completed-loop observations, not a physical refresh rate or a whole-city throughput promise. Keep the earlier six-second `c625…`110/360 and `a243…`108/360 distinct; neither is the full paired baseline here. Future crowded/scrolling/flight/roof scenes, human handling and physical execution still need measurement.

A separate scope within the same `a638c374…` journal completes the first delivery at condition 100/cash 139/done 1; held steering samples speed 16→23→16 and heading 0→14→12. Ordinary Queen 46→49 pays three (cash 119→116 after earlier red-light penalties), paid-map browsing preserves all 58 game bytes, and genuine game-button reset restores the paid ride with nine seconds left/cash 116/done 1 before arrival at East Leslie. It closes scoped `passed` at frame 6,817, 3,788 events, digest `c43d4393937c66fcd713c3974068a67b6707fa6ead7f8193ed46d5a5dca34936`; archive `44c18785-b6a3-4f49-b669-bdcf764e7dbc`, manifest `23e7d4b5c4cb31bcd7d8d60383e4f2e2217137ade3dbcd14134886dccd3ad1ff`. These are retained evidence identities, not downloadable gameplay. [TESTING.md](../TESTING.md) and [BUILD.md](BUILD.md) own full ROM/debug identity and scenario detail.

**This tram-only ROM does not contain itinerary/payout feedback.** Later combined `4343f2b8…` adds one transient byte and reuses existing offer/result storage; it has its own build/host UI/native/memory evidence above, with full local `make check` passed. No older gameplay pass, including this Queen trip, is reassigned to the combined ROM.

## Retained Port campaign and clock sample, 2026-10-02

Official `project/build/toronto-port-campaign-clock.gbc`, 524,288 bytes, has SHA-256 `c625e6dce80a56a787c940885da5abd332fce47335e52f57698c3395a14ba364`, source fingerprint `ed6b11e18955ff84532f06ab8ad40de69a034159b392f5e7ef3b15ee868d32c6`, matching NOI `1e4d7d05715fb86e3552594b2c3511bba526c801f37867b9874841435435f05f`. Header/compiled aircraft/Queen guards, full `make check` and 1,098 static bytes pass. `_game_time C0B9` is resolved from its matching NOI; at the stationary Core origin six 60-VBlank windows from 482→842 count 23/18/17/17/17/18, totaling **110 loops / 360 VBlanks**. Retained fused `a243…` totals 108 under the preceding recipe. No repeat/whole-city benchmark or meaningful human-speed improvement is established by two additional loops.

This retained source's short-update clock path avoids division below 60 elapsed VBlanks and preserves subsecond carry/deadlines/transit timing without persistent RAM. That campaign has 96/59/seven anchors and 699,955 real-engine host checks. Separately from timing, the [closed scoped replay](NATIVE_PORT_CAMPAIGN_SAMPLES.json) completes three original contracts at condition 100/cash 401 / done 3, then Fire Hall Books at condition 36 / cash 71 / done 4, real reset/completion restoration and Port car recovery at 11024. This is one of eight new jobs; damage/route/deadline balance, remaining jobs and whole-city responsiveness remain pending. Older six-deck Port exploration stays tied to `a243…` in [NATIVE_PORT_DRIVING_SAMPLE.json](NATIVE_PORT_DRIVING_SAMPLE.json); no timing result is transferred between recordings.

## Retained street source and intermediate diagnostics

The retained five-scene `a212…` baseline records 132 loops / 360 VBlanks. Earlier full-street `f380…` records 64, cached `5af3…` 67, eight-VBlank `20ce…` 85, bulk-terrain `826e…` 87, 75-byte epoch `35337509…` 90, 123-byte hull `555f3d31…` 91 and eight-VBlank/87-byte bucket `6665b6fa…` 92. Retained sixteen-VBlank cadence `bd09f1c3…` records 107; final fused `a2438348…` records 108. The correct final `_game_time` symbol is `C0B9` (counter `b7→cd→df→ef→00→11→23`); the initial wrong-address `C0B4` observation is archived `needs-review` and excluded. This improves the bounded recipe over the preceding bucket build while remaining below 132; it does not certify whole-city pacing or human responsiveness. Exact ROM/debug/source identities, windows and workloads are retained in [NATIVE_STREETLIFE_SAMPLES.json](NATIVE_STREETLIFE_SAMPLES.json).

Current source advances ordinary road traffic every sixteen active VBlanks and pursuing police every four; guarded separating retreat remains per-render. The ordinary eight-pixel step corresponds nominally to 30 pixels/second, but capped delayed updates discard excess time, so speed is not guaranteed. Courier/contact/person/attention still update each active city render. Exact stock NONBANKED collision ranges with mask 255 cover full terrain sweeps, preserve hit globals and add no persistent terrain RAM. Light-pattern caching avoids unchanged VRAM writes. The restored production source retains all terrain, future-tram, signal, junction and body guards.

Each motion batch uses a caller-owned 87-byte native stack snapshot, with no added persistent RAM or save fields. Begin validates all six vehicle coordinates/extents, visible pedestrian bodies and the active parked car once. Twelve bytes of 16-pixel centre buckets replace the earlier 48-byte hull-edge arrays; five-bucket separation rejects only bodies beyond the maximum 58-pixel sweep/priority reach. Nearby bodies retain exact full-body checks, with cached visibility/priority mask iteration. Admission records a pending candidate; only after terrain and tram guards pass does commit advance its coordinates/buckets and the matching live cache. Later movers see accepted endpoints sequentially; red, blocked or aborted proposals never become occupancy. Validated eight-Q4 separating retreat keeps its legacy guards and commit protocol. Each batch rebuilds the snapshot.

The hot-path `td_traffic_epoch_move` now performs admission, whole-body terrain/future-tram guards and commit in one banked call. Private begin/accepted commits prove the old body, removing only its redundant revalidation; public old-position arguments still require exact cache equality, and every candidate retains extent/map bounds, cardinality and 8/128-Q4 limits. Validated separating retreat keeps the existing caller-proven full retreat guard before its preserved future-tram exception. Failed moves clear pending ownership and leave bodies/buckets fixed; successful moves publish the same endpoint to the live cache. The standalone BANKED signal-stop API moves unchanged to its own translation unit, duplicating generated constants in ROM rather than adding RAM, after two 16-KiB traffic-bank failures.

Retained fused full `make check` passes with road/game/planner/traffic/light 1,950,051 / 695,857 / 11,133 / 2,424,828 / 12,842. Official fused `a2438348…` passes compiled poses/resource limits and 1,098-byte static reserve. Its ordinary-control street sample verifies H1 impact/stumble, game/flight/boat map freeze, saved-attention reset, police approach/capture, parking/walking/blocked shore and one rendered boat. A separate fresh same-ROM first delivery reaches cash 139/condition 100/done one. Public old-position/cache equality and candidate geometry checks remain; fusion combines admission, full road/future-tram guards and commit without diagnostic cuts. [TESTING.md](../TESTING.md) scopes the checkpoint distinction and all recordings. Higher heat/escape/paid rides, deepest stack, dense scrolling/water/deck/roof cases, human responsiveness and cartridge execution remain open.

Diagnostic cuts are never accepted gameplay: `d288…` reaches 127 only with NPC motion disabled; `90ea51fd…` reaches 95 with external terrain/future-tram guards cut; `23cc770e…` reaches 115 with epoch begin/admit/commit disabled. Their recordings are closed/archived and production guards/motion restored. These isolate costs rather than certify a playable candidate.

## Counter and replay method

Both recordings use the official ModRetro plugin's native PyBoy 2.7.0 CGB worker, SHA-256 `6271cbbb9ca76d4d149173f4110705cfa3c567349f0cb7ceb347bf1df7598925`. Each starts with a fresh 180-frame clean boot, holds A for two frames to leave HELP, then releases all buttons for 300 frames. At frame 482 the courier and car are stationary at Core `(560,720)`. Six further neutral 60-VBlank steps finish at frame 842. Inputs are ordinary buttons; WRAM and OAM are inspected read-only. No RAM state, progress or performance instrumentation is injected.

Resolve `_game_time` from the NOI belonging to the exact loaded ROM. Both matching NOIs happen to place this unsigned byte at `C0BC`; that coincidence is not permission to reuse the address after another build. Pinned GBVM increments `game_time` after scene/actor rendering and UI work, before activating the OAM buffer and waiting for VBlank. In these stable unlocked ROAM windows, `(after - before) & 255` counts completed engine loops. Every interval is short enough to avoid ambiguous multiple wraps. Reset, scene loading, menus or locked VM work would require separate interpretation and are excluded from these measurements.

Video frames and simulation steps have different meanings. The world seconds/subsecond clock follows elapsed VBlanks; the private `td_tick` advances bounded motion substeps when an update catches up. Neither is a completed-loop counter. A nominal 60-VBlank window spans about 1.0045 emulated seconds at PyBoy's approximately 59.73-Hz frame clock. The six windows span about 6.027 emulated seconds; tool latency and host wall time are not included. The recorded loop counts are not a measurement of the physical display's refresh rate.

## Paired observations

Counter values below are hexadecimal; loop deltas use modulo 256.

| Native frame interval | `14005662…` counter | Baseline loops | `792cd0e1…` counter | Candidate loops |
| --- | --- | ---: | --- | ---: |
| 482 → 542 | `BD → D1` | 20 | `D1 → EE` | 29 |
| 542 → 602 | `D1 → E4` | 19 | `EE → 02` | 20 |
| 602 → 662 | `E4 → F3` | 15 | `02 → 11` | 15 |
| 662 → 722 | `F3 → 02` | 15 | `11 → 24` | 19 |
| 722 → 782 | `02 → 16` | 20 | `24 → 42` | 30 |
| 782 → 842 | `16 → 2A` | 20 | `42 → 56` | 20 |
| Total: 360 VBlanks | | **109** | | **133** |

The common-path candidate skips the two current-tram pose/bounds queries when the already-validated full courier body is outside the conservative tram corridor. It retains mode, scene, on-foot, subsecond and player-geometry validation before that early return. It also reuses the pedestrian phase already computed for the actor's position when choosing its animation frame. These cuts change neither save fields nor authored collision thresholds. The earlier renderer is identical in these two measured candidates.

Cash remains 30, health/condition remains 100, the courier and parked-car coordinates remain `(560,720)`, and no job is active. End-of-window OAM observations peak at four or six objects per scanline, with zero over-limit scanlines in the sampled snapshots. They do not certify every intervening frame or all crowded views.

This replay controls fresh boot, input sequence, camera location and native frame endpoints. Clock/flight boundaries still differ by up to three VBlanks, and ground actor phases and visible-object counts vary between samples. The plane-over-roof interval 602 → 662 remains at 15 completed loops in both recordings. The aggregate calculation is `(133 - 109) / 109 ≈ 22.02%`; it is a scoped increase in completed loops across this sequence, without attributing a percentage to either individual cut or isolating flight rendering cost. Additional repetitions and matched workload samples are needed before generalizing it.

## Exact measured build identities

Both outputs are 524,288-byte native ROMs. Public header inspection verifies CGB-only (`0143=C0`), MBC5+RUMBLE+RAM+BATTERY (`0147=1E`), 512-KiB ROM (`0148=04`), 32-KiB SRAM (`0149=03`), and valid Nintendo logo/header checksums. Their header checksum byte is `014D=E9`; global checksums are `2F02` for the baseline and `3184` for the common-path candidate. Header validity establishes the cartridge format, separately from emulator behavior or hardware boot.

| Identity | Contact-corrected baseline | Common-path candidate |
| --- | --- | --- |
| Output, relative to game folder | `project/build/toronto-contact-recovery.gbc` | `project/build/toronto-runtime-faster.gbc` |
| ROM SHA-256 | `1400566247fddfa9a1acd5ef42e5f90bb19db9caa43a51fab0ce82400caa137f` | `792cd0e1685c8ebeaa8cd356e266f4e8045489fc4ddeae60e500bd0c42f52e00` |
| Matching NOI SHA-256 | `6f8b6d662fe3d080a8a0a00f5d398521a35e4ddc62dcfddec5125b942264ea68` | `ba9b2356da764d00e287765c50d2174284cd8cbe7a43c817d3c5d0c9e90118e5` |
| Build source fingerprint | `e6bafd235ddb547928f8c760672c20d4f55518f99f24d519e3758142d6a253dd` | `fc6028fc7913f3e97ff37e732b07672240737f0ec7bc870c85177e3851ba1318` |
| Plugin project revision | `03e03e4f63cda3ece1d5732ca557e5f3f62ea88a1cf08cb92056342073519f64` | `03e03e4f63cda3ece1d5732ca557e5f3f62ea88a1cf08cb92056342073519f64` |
| Globals SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| `_game_time` from matching NOI | `C0BC` | `C0BC` |
| Linked heap / stack base / static reserve | `DA68` / `DF00` / 1,176 bytes | `DA68` / `DF00` / 1,176 bytes |

The recorded common-path compiler SHA-256 is `78961636939edb44b0539ae26badfa07055be38124dbf08e3b84ec5608a2e627`; its engine ABI identity is `gbvm-farptr3-uword16-v1:4d051f961ba47e2e300690e3114203f64285abd72a1433702ea422cd64e5e19f`. The ROM, NOI and globals digests above were independently read from the retained local files. Source-debug identities come from the official build/recording results retained in the samples record.

The 1,176-byte linked reserve passes the project's 1,024-byte static allocation threshold. It measures the distance between linked heap end and stack base. It does not measure deepest native stack usage, interrupt nesting or every banked recovery/rendering path; those remain separate gates.

## Immutable measurement recordings

Both journals end at frame 842 with 75 events and scoped `passed` outcomes. Each owned emulator was stopped and closed; the official plugin archived its unchanged recording bytes while retaining original-path mappings. These outcomes certify the recorded measurement scenario, separately from whole-game acceptance. Journals, frames and archive contents remain ignored local evidence.

| Evidence | Baseline `14005662…` | Candidate `792cd0e1…` |
| --- | --- | --- |
| Original recording ID | `session-445da1af-009c-4ebb-9073-7a65ada960b9` | `session-ebcbea01-9a63-465e-ad31-97c86abb6a83` |
| Native session ID | `cd067401aa5647d6812af0551b04e490` | `192abd4ce36144889cc17c19d6bc971f` |
| Final journal event digest | `a9eb93d0effc32101af4b470994ff3d7339720987bd7e5084b37486514cfed99` | `67f6208c4b02eaa8e2225d34a511d8158d1176d1e921b8707e6dad919e7ae126` |
| Archive ID | `3dbab5d0-ac68-4c89-b7e1-7b51e561c48f` | `e270b708-0a1c-447b-a9f1-b081dcf3d63e` |
| Archive manifest SHA-256 | `430d7ac7eb3ce9d7e5e3730bf381dba08de6836a4781ede9b246b85e71b7bae0` | `459c71b32a9c25459bc57cfa5fa8dab9fcd18921a9e76bdc6d9e762e6cc8bab7` |
| Archive inventory digest | `30dbb89af233682ebca233c27506abc49a34a1ebd16ba2dd1f81aa70c932c817` | `594a7650daa21420a2f706986347ae43dbdd90b5c7116cf0710fc2f102ff6c3e` |

Original paths are `project/artifacts/playtests/<original recording ID>/`; preserved copies are `project/artifacts/recording-archives/<archive ID>/recording/`, relative to the game folder. Archive mappings and journal ROM/worker identities were independently read. The public sample JSON retains the exact read-only state and flight snapshots without publishing cartridge RAM or private capture files.

## Later roof-mask source and remaining gates

After both recordings were closed, the aircraft renderer was changed to OR actual compiled OBJ opacity into eight local row masks per priority roof tile. Completely transparent coverage skips the original background tile read and flip normalization; covered rows clear both background bitplanes once. Conditional tile/attribute restoration, transparent pixels, palette/priority, hardware/window limits and ground OAM ordering remain covered by the independent original-PNG oracle. The renderer's focused ASan/UBSan harness passes 41,341,613 checks. This adds eight local stack bytes and no persistent memory.

That aggregated-mask source is included in neither of the two measured ROMs above; the 22% common-path result must not be reassigned to it. At that source checkpoint, the full suite repeated the focused renderer pass before stopping at temporarily stale eastern source artwork during the next district integration. The exploratory integration measurement below has its own identity and scope. Final synchronized resources, full checks and official build gates belong to each later candidate's own evidence.

Further acceptance requires matched plane/helicopter/roof and inactive-flight samples, crowded traffic/pedestrian/tram cases, scrolling and scene transitions, all added districts and seams, and deepest native stack checks. Expanded city workloads require new ROM/source/debug identities and fresh baseline comparisons. Physical cartridge execution, handheld handling/readability and whole-city frame pacing remain unmeasured. Full Old Toronto coverage and at least two measured hours of varied gameplay also remain separate project goals.

## Exploratory five-scene replay, 2026-10-02

Exploratory ROM `6a8a9f96…` includes the common-path cuts, aggregated aircraft roof masks and the fifth Port Lands scene. A new fresh recording repeats the same 180-frame boot, A2, neutral300 and six neutral60-VBlank windows at stationary Core Front Street. This measures the loaded Core workload; adding a scene does not imply its remote actors run during the sample. Exact read-only snapshots, build/header identities and recording results are in [NATIVE_FIFTH_SAMPLES.json](NATIVE_FIFTH_SAMPLES.json).

The matching NOI now places `_game_time` at **`C0B9`**, illustrating why every build requires its own symbol lookup. The same unsigned-byte modulo-256 method records **132 completed loops / 360 VBlanks**:

| Native frame interval | `6a8a9f96…` counter | Completed loops |
| --- | --- | ---: |
| 482 → 542 | `D1 → EE` | 29 |
| 542 → 602 | `EE → 02` | 20 |
| 602 → 662 | `02 → 11` | 15 |
| 662 → 722 | `11 → 24` | 19 |
| 722 → 782 | `24 → 41` | 29 |
| 782 → 842 | `41 → 55` | 20 |
| Total: 360 VBlanks | | **132** |

This is `(132 - 109) / 109 ≈ 21.10%` more completed loops than the original baseline, and one fewer than the common-path candidate's 133. It reproduces essentially the same aggregate pace, without demonstrating an additional improvement from roof-mask aggregation or isolating its cost. The plane-over-roof interval remains 15 loops in all three recordings. Recorded stationary courier/car coordinates, cash30 and health/condition 100 are preserved; end-of-window OAM peaks are four or six with zero sampled over-limit scanlines. State/flight snapshots still retain small boundary and ground-phase differences. The frame-clock interpretation and limits above continue to apply.

| Exact exploratory identity | Value |
| --- | --- |
| Output / bytes | `project/build/toronto-port-lands.gbc` / 524,288 |
| ROM SHA-256 | `6a8a9f9664364cc09d84bf0b3c7ddc8f9b4c9db3b1f090bda96b28798e78bc94` |
| Matching NOI SHA-256 | `f36dc1b792234ffa17ba27f1db8d5641aa8f295d2c6d4a61baec703e060d3a65` |
| Globals SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Build source fingerprint | `54d7fb3a534eb48cd47df591776940961295840103a0dbb488b11c4c860b8c7a` |
| Plugin project revision | `151a988b2b77e3312da36ee82d0f376b8fc15369fe21bbee0780b7d47b8682cc` |
| Compiler SHA-256 | `78961636939edb44b0539ae26badfa07055be38124dbf08e3b84ec5608a2e627` |
| `_game_time` from matching NOI | `C0B9` |
| Linked heap / stack base / static reserve | `DA68` / `DF00` / 1,176 bytes |

The native header remains CGB-only/MBC5+RUMBLE+RAM+BATTERY, 512-KiB ROM and 32-KiB SRAM, with valid logo/header checksum `E9`. Matching ROM/NOI/globals hashes and `C0B9`, `DA68`, `DF00` symbols were independently read from retained local files. The source-debug ABI and PyBoy2.7.0 worker retain the identities stated above. The unchanged linked reserve is an allocation check, not proof of maximum native stack use.

| Scoped performance recording evidence | Value |
| --- | --- |
| Original recording ID | `session-1d09f94f-328e-47e4-a89d-b3866268e53a` |
| Native session ID | `b59722948d4d420d8244f5c4d2828ba1` |
| Outcome / frames / events | `passed` / 842 / 75 |
| Final journal event digest | `adc5c7b5da8faaad908e96ec32b91b9ffdacf2be38715723b703dd22f00e204d` |
| Archive ID | `76b5db30-ec50-41c0-9a59-840b03169859` |
| Archive manifest SHA-256 | `0ea9ff47de291ed3a70b91664d4f6eae56bba52f4907fb698865d5dc2c8d2a6d` |
| Archive inventory digest | `35a4755d0ff53581dce8358f1cf168e9dba0939b0526b2a7f1ee525886703380` |

The owned worker was stopped and closed, and the official plugin archived the original measurement bytes with portable original-path mappings. This scoped performance recording and a separate first-delivery/driving recording passed. The same exploratory ROM's separate travel journal remains **`needs-review`** because its Port Lands HUD displayed the stale street label `Bloor`. The passed measurement/delivery outcomes do not upgrade that travel result or establish correct fifth-scene street labels.

The subsequent street-label-corrected candidate requires its own ROM/source/NOI identity and fresh timing. These 132 loops must remain attached to exploratory `6a8a9f96…`; neither its measurement nor the original two records establish final-candidate, whole-city, two-hour or hardware acceptance.

## Corrected five-scene candidate

Final street-label candidate `a212dd9ed479310a98e58b701416f8af88aaa09174f186747dca678ff4c164f4` repeats the fresh Front Street recipe and returns the same **132 completed loops over 360 VBlanks**: 29, 20, 15, 19, 29, 20. Its matching NOI is `b4a500d607a73fff08d1f4764b543ff3798d556aca302e9ead4b56c7f3d9649f`, `_game_time=C0B9`, source fingerprint `025edad8ffaa318a4739c0c1f4aee3b539789a8808dd1c11a20a1d9943dab98b`, project revision `151a988b2b77e3312da36ee82d0f376b8fc15369fe21bbee0780b7d47b8682cc`. The globals/worker/compiler/ABI and 1,176-byte linked reserve retain their documented identities. The corrected ROM has its own observations in [NATIVE_PORT_LANDS_SAMPLES.json](NATIVE_PORT_LANDS_SAMPLES.json).

This independently recorded result is about 21.1% above the original 109-loop baseline and essentially unchanged from the 133-loop common-path candidate. It establishes neither an isolated roof-mask improvement nor whole-city or physical performance. Every measured endpoint keeps the courier/car stationary with cash 30 and condition 100; OAM peaks are 4, 4, 6, 4, 4, 4 with no over-limit scanlines in those samples.

Original recording `session-83930f52-6eb8-4c7a-8c03-2969e4da0db7`, session `1b2b11e1852648af86811e877c1b94d0`, ends at frame 842 with 75 events and digest `521b3b715d482dba12550bed62e7b7d6e1909959aa698d59f261f4201d9eadbd`. It is stopped, closed and archived at `48ad8468-c2e2-4d21-84ff-90c582f3f9ca`, manifest `1c71e7d3b2f0635d40656b5ac0e096f685a4fca5081955fcd53cc5dbc689c57e`, inventory digest `52c7398dfed4778edcd97f43eb8b5f3efd7398ac3fc6efae0fdab585bf006aee`. Separate corrected-ROM travel, save/reset, atlas, aircraft and driving passes are documented in [TESTING.md](../TESTING.md).
