# Native performance record

Updated 2026-10-02. A fresh stationary Front Street replay records **109 completed engine loops over 360 VBlanks** on contact-corrected ROM `14005662…`, and **133** on common-path candidate `792cd0e1…`: **24 additional loops, approximately 22%**. This is one paired emulator scenario. It does not establish isolated aircraft cost, whole-city frame pacing or physical cartridge performance.

The original read-only state, flight, counter and OAM observations are retained in [PERFORMANCE_SAMPLES.json](PERFORMANCE_SAMPLES.json). [TESTING.md](../TESTING.md) and [BUILD.md](BUILD.md) retain each candidate's broader build and gameplay scope. The later aggregated roof-mask change is outside both measurements.

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

This is `(132 - 109) / 109 ≈ 21.10%` more completed loops than the original baseline, and one fewer than the common-path candidate's 133. It reproduces essentially the same aggregate pace, without demonstrating an additional improvement from roof-mask aggregation or isolating its cost. The plane-over-roof interval remains 15 loops in all three recordings. Recorded stationary courier/car coordinates, cash30 and health/condition100 are preserved; end-of-window OAM peaks are four or six with zero sampled over-limit scanlines. State/flight snapshots still retain small boundary and ground-phase differences. The frame-clock interpretation and limits above continue to apply.

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
