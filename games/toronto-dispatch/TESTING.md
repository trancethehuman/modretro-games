# Testing record

## Street prototype — source/build/scoped native checks pass, broader acceptance pending, 2026-10-02

The five-scene source integrates distinct original civilian/fleet art, non-graphic recovery, escalating impact fines/attention, local police pursuit, signal-following road drivers and cosmetic boats. Current compiled campaign is **96 contracts / 59 stops**, with seven parking anchors; the eight Port jobs append after all previous 88/51 native fields/briefs unchanged. One of the eight new jobs now passes native delivery/reset/car-recovery checks; the other seven and tuning remain pending. No 114 service is added. [DESIGN.md](DESIGN.md) records mechanics. Save v8 remains 58 bytes/16 completion bytes; valid pre-v8 imports clear attention. Preserved old saves remain readable, while an old 88-job ROM rejects saved new jobs/completion bits. Historical ROMs/recordings below retain their own identities.

Vehicle-body and cardinal swept terrain queries use the stock NONBANKED `tile_col_test_range_x/y` APIs with mask 255 to inspect every overlapped collision tile. They preserve `tile_hit_x/y`, keep bank switching inside the fixed-ROM API and add no persistent RAM. The host oracle checks the actual five collision maps, rather than treating endpoints or a trusted flag as proof of clearance. Ordinary traffic advances every sixteen active VBlanks and chasing police every four; courier/contact/person/attention queries retain each-active-update cadence. Validated eight-Q4 overlap retreats keep their per-render cadence. All paid RIDE states suppress pursuit. Light-pattern readiness is cached and reset at scene/map restoration; only changed cells are written.

Each motion batch uses a caller-owned 87-byte native stack snapshot, with no added persistent RAM or save fields. Begin validates all six vehicle coordinates/extents, visible pedestrian bodies and the active parked car once. Twelve bytes of 16-pixel centre buckets replace the earlier 48-byte hull-edge arrays; five-bucket separation rejects only bodies beyond the maximum 58-pixel sweep/priority reach. Nearby bodies retain exact full-body checks, with cached visibility/priority mask iteration. Admission records a pending candidate; only after terrain and tram guards pass does commit advance its coordinates/buckets and the matching live cache. Later movers see accepted endpoints sequentially; red, blocked or aborted proposals never become occupancy. Validated eight-Q4 separating retreat keeps its legacy guards and commit protocol. Each batch rebuilds the snapshot.

The hot-path `td_traffic_epoch_move` now performs admission, whole-body terrain/future-tram guards and commit in one banked call. Private begin/accepted commits prove the old body, removing only its redundant revalidation; public old-position arguments still require exact cache equality, and every candidate retains extent/map bounds, cardinality and 8/128-Q4 limits. Validated separating retreat keeps the existing caller-proven full retreat guard before its preserved future-tram exception. Failed moves clear pending ownership and leave bodies/buckets fixed; successful moves publish the same endpoint to the live cache. The standalone BANKED signal-stop API moves unchanged to its own translation unit, duplicating generated constants in ROM rather than adding RAM, after two 16-KiB traffic-bank failures.

| Current host gate | Passing checks |
| --- | ---: |
| Production road terrain / hit-state preservation | 1,950,051 |
| Game update/save/contact integration | 699,955 |
| Local police planner | 11,133 |
| Traffic admission / light rendering | 2,424,828 / 12,842 |
| Original-PNG boat masks, OAM and loader integration | 1,739,244 |
| Actual-map boat full-hull water/deck validation | 54,528 |
| Civilian contact hotspots / native-sheet binding adapters | 2,400,013 / 1,217 |
| Atlas API / UI | 1,475,876 / 24,151,787 |

These are production-C/source checks with host banked-ROM/VRAM/GBVM adapters, not native ABI, CPU timing or cartridge proof. Final full `make check` passes. Sixteen-VBlank ordinary motion retains an eight-pixel sweep cap: nominal 30-pixel/second advance is a responsiveness compromise, and capped delayed updates discard elapsed time rather than guaranteeing speed. [PERFORMANCE.md](docs/PERFORMANCE.md) keeps every measured candidate and diagnostic cut separate.

Current Port-campaign/clock ROM `c625e6dc…` passes the official build/header/compiled aircraft/Queen/memory gates and full suite. Ordinary controls deliver three original jobs at condition 100 and the new Fire Hall Books contract at condition 36, then restore its saved completion and recover the Port car. Retained `a243…` street and Port travel samples remain separate. Wider higher-heat fines/escape/paid-transit/map/water/bridge/roof/crowd coverage, whole-city pacing and deepest stack remain pending. Full Old Toronto/fuller Islands, two measured enjoyable hours, all campaign/vehicle/seam cases, human feedback, browser recovery and physical streaming/write/read-back/cold boot remain unverified. Published Prototype 6 stays the separate earlier release.

## Current Port campaign and clock candidate — first new-job replay passed, 2026-10-02

Official output `project/build/toronto-port-campaign-clock.gbc` is 524,288 bytes, SHA-256 `c625e6dce80a56a787c940885da5abd332fce47335e52f57698c3395a14ba364`. Source fingerprint is `ed6b11e18955ff84532f06ab8ad40de69a034159b392f5e7ef3b15ee868d32c6`; matching NOI SHA-256 `1e4d7d05715fb86e3552594b2c3511bba526c801f37867b9874841435435f05f` and globals `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a`. Local ROM/NOI/globals digests were independently read. Official build completes in 60,620 ms; inspection passes CGB-only/MBC5+RUMBLE+RAM+BATTERY/32-KiB SRAM, matching compiled aircraft/Queen validators and heap `DAB6`→stack `DF00`, 1,098 static bytes. Full `make check` passes with 699,955 engine checks. Static allocation does not measure deepest native stack.

Source appends clients 51–58 and jobs 88–95, preserving every original stop/job/brief field and the unchanged 58-byte v8 state/16-byte completion bitmap. Seven parking anchors and sole Leslie travel pass source/body/route/progression checks. Host fixtures cover all eight ordered routes, wrong-district/out-of-order rejection, foot 58/parking/car entry, truck transit denial, final 95 unique credit/repeat payout/save and fixed-offset old 88 v6/v7/v8 saves. Deadlines 170/175/205/185/150/180/120/200 seconds are initial tuning values; no new actor or 114 boarding is added.

Exact-ROM ordinary controls deliver native jobs 0,2,1 (displayed 01,03,02), each at condition 100, reaching cash 401/done 3 and unlocking Fire Hall Books. Stationary Core frames 482→842 count 23/18/17/17/17/18 loops across six 60-VBlank windows: **110/360**, versus retained `a243…`108. The short-update clock skips division below 60 elapsed VBlanks while preserving subsecond carry/tick rules. This tiny scoped difference is not substantial or whole-city performance evidence.

Contract 90 FIRE HALL BOOKS (native index 89) is selected at 5035; pickup at Riverside `(138.5625,528.6875)` at 5179 reaches stage 1 / condition 100 / left 172 / cash 381 / H1 after a separate 20-credit roaming impact. Sole Leslie entry reaches actual Port at 8148/8172; the car is `(910.625,66.625)`, condition 36 / H3 / cash 31 / left 123. Police capture at 8557 leaves cash 0. At 10108 parking/exit leaves courier `(309.9375,369.3125)` and car `(309.9375,351.3125)`, both district 4, condition 36 / left 91. Direct south walking stops at the Fire Hall footprint v 391.8125 at 10266; ordinary controls go around its east edge near x 328, then south/west to the client. At 10462 RESULT records delivered job 89, foot `(311.4375,428.3125)`, cash 71 / done 4/condition 36/left 86, bitmap `07000000000000000000000200000000`. This demonstrates the legal walking approach and condition-scaled payout, not an isolated high-heat fine test.

Actual A+B+Select+Start 120 plus neutral 180 restores HELP at 10762 with cash 71 / done 4/bit 89 and the saved foot/Port car. Resume, legal walk back around the footprint and entry reach 11024 driving at `(309.9375,351.3125)`, with done4 retained. The [portable sample](docs/NATIVE_PORT_CAMPAIGN_SAMPLES.json) closes/archives scoped `PASSED` at 11,024 frames, 6,228 events, digest `6fb5412929c304ce265a3dc437974f64e5d7c5fe30c429125a14549491390201`; native session `19e21ca1186d4c53af3ed741367012d5`, original recording ID `437acdf5-9d2f-4f0d-b8f1-ee7ded64fcaa`, archive `a09d89c8-52bd-4d8a-8d60-7b6fd667dd4e`, manifest `97c36533129625d087ffa8923e9f60b9f76860dfa496b6c5db1495dda09a2366`. These are evidence identities, not downloadable gameplay.

This is **one of eight new jobs**, alongside three original completions. H3 escalation/capture and damaged payout are sampled, but exact sufficient-cash higher fines/escape are not isolated. The other seven jobs, all 96/unlock closure in native play, vehicle/handling/deadline/reward tuning, whole-city/full Old Toronto, two hours, browser and physical acceptance remain pending.

## Retained fused candidate — two scoped native replays passed, 2026-10-02

Official output `project/build/toronto-streetlife-fused.gbc` is 524,288 bytes, SHA-256 `a243834899097d9660190022f03ce31d322d2584d0d7cfe1fdf8adb84a8902ea`. Matching NOI is `f56570b2eea99e62d634bf7fd2e6a1643696fdd0aa3aa58f9f7356f841e3bdc3`, source fingerprint `f9c81b07cca407abcd83f8e3fd4687cdbc2b6042e018f86feb000b4c1ff8ffb4`; globals/compiler/project/ABI retain the recorded street-build identities. Header and exact compiled art/pose/resource gates pass across all five scenes: OBJ 116/116/106/116/106 of 128; bank-1 BKG 16/15/0/0/0 below scratch tile 32. Heap `DAB6` to stack `DF00` gives 1,098 static bytes. Final full `make check` passes; static reserve does not establish deepest runtime stack.

The correct `_game_time` counter is `C0B9`, with game state at `C1F6`. Paired neutral windows record `b7→cd→df→ef→00→11→23`: 108 completed loops / 360 VBlanks. An initial `C0B4` read addressed the wrong symbol and is archived as `needs-review`, excluded from pacing. Two failed official stages used 16,619 then 16,659 bytes in the 16,384-byte traffic bank; extracting the unchanged standalone BANKED signal-stop API/constants into its own ROM-only translation unit permits the final build. No diagnostic guard cut remains. [NATIVE_STREETLIFE_SAMPLES.json](docs/NATIVE_STREETLIFE_SAMPLES.json) retains identities, counter windows, failed stages and invalid measurement separately.

The final ordinary-control street replay renders humans and records impact at frame 1,494, cash 30→10/H1 with stumble. Map 1,638→1,998 preserves all 58 game, 13 flight and 13 boat bytes. Pause Save and real A+B+Select+Start reset restore HELP at 2,458 with cash 10/H1/countdown 29 and vehicle `(578.375,667.375)`, then resume at 2,474. The later live release sample at 2,158 has v `667.9375`; it is 0.5625 pixels beyond the saved checkpoint, so this is not a claim that all 58 bytes restore that later snapshot exactly. Sampled actor coordinates show road police approach; capture at 4,034 clears H, cash 10→0/message 20, with the cop at `(556,640)`. Park/exit at 4,154 shows the courier on foot. Walking down stops at Core shore v `807.875` and stays there over 4,514→5,114. At 4,754 a single boat OBJ is visible at screen `(58,85)`, tile 114/palette 7; sampled peak is two objects per scanline without overflow. This does not sample all bridge underpasses or dense crowds.

The street replay ends scoped `PASSED` at 5,114, 907 events, digest `08af6dfbfb087e4aafaf902330d3da1f97dbb249a69c77741225da20b0430678`; native session `81508d7c0fbe4cde99572995b5ecdd40`, archive `3b6929df-2f13-4c27-8b74-73d62349187a`, manifest `15de07060bad8ebabb4d124e2cebd81d499ee33e6f59604848d1f629ba7ee83c`. A separate fresh final-ROM replay completes first delivery at frame 790, `(779.9375,720)`, cash 139/condition 100/done one/job clear/RESULT. It closes/archives scoped `PASSED` with 165 events, digest `ca2615e778837c3a2bc44e021b3c086a717a33cebacd10e1dd96c4bcbeb11be6`; native session `45a285d602224b0896a30e058043d7e4`, archive `b555b9cd-45ab-4289-8cf1-42fbd9ae0086`, manifest `067bcc5da9ad5e4930fd9f237bf24b2bd07abde72b95356481b1d4bd31082211`. Both recordings are closed/archived, with no source changes after the successful build. These are retained evidence identities, not downloadable gameplay.

The 108-loop sample remains below the retained 132-loop baseline; native H2/H3/escape/all-paid-ride branches, boat/deck cases, whole-city performance, deepest stack, full campaign/two hours, browser and physical execution remain pending. Earlier cadence/hull/epoch results keep their own source/debug/ROM scope.

## Retained fused ROM — Port car/foot/reset exploration, 2026-10-02

The separate [portable Port driving sample](docs/NATIVE_PORT_DRIVING_SAMPLE.json) belongs to `a2438348…`, source fingerprint `f9c81b07…`, scoped passed/closed/archived at 9,988 frames. Ordinary inputs enter Port through East Leslie and cross Lake Shore, Cherry North, Commissioners, Cherry South, Ship Channel and Unwin decks by car. Parking/visible exit and Commissioners walking retain the Port car. Genuine four-button reset restores HELP courier `(402.5,288.625)`, car `(551.5,290.125)`, both district 4, cash 0 / world 54 / sub 31; later live release subsecond 45 is not asserted identical. Walking back/re-entry works. Regatta parking reaches the future Beach handoff, then a clear-gap actual water block `(423.625,935.625)` held over 8046→8166, and re-entry/Leslie car return reaches actual East at 9987. Older `(432,919.5)` is a tree-trunk stop, not this lake test.

Native H2/H3 escalation occurs with cash already zero, proving attention levels but not exact higher fine amounts or escape. Truck/motorcycle/scooter bridges, boat deck clipping, whole-city pacing/deep stack and new 96-job play are outside this record. It retains event digest `e7689689507c8b934f665169ecbf77bff16f1d1087fd34308d475c7105e42626`, archive `54067b4e-73e5-45c3-a1cb-59a184113b75`, manifest `f87898a59a962d5b37cf8775ae40483f8cf7edec3cedd2a3b6c48bdc08517028`; these are evidence identities, not downloadable gameplay.

## Retained cadence candidate — scoped delivery and timing, 2026-10-02

Official output `project/build/toronto-streetlife-cadence.gbc` is 524,288 bytes, SHA-256 `bd09f1c30f8dd5be35f56c1da58fcdff37b343ecb669bea2b39353aa1d89388f`. Matching NOI is `e668853e82feba0831a1a7e8103238509bf81e1588df5a06877ef437be2cb379`, source fingerprint `4fba44146e887e0d4ee1f4c0c5b0eadbb79bdcde184af5e478745deb939f6c60`. Header, compiled exact poses/allocation/isolation and a 1,098-byte static reserve pass; reserve does not measure deepest runtime stack.

With an 87-byte snapshot, sixteen-VBlank ordinary traffic, four-VBlank police and per-render separating retreat, the paired sample records 107 completed loops / 360 VBlanks (counter `b7→cd→de→ef→00→11→22`). Ordinary controls complete first delivery at frame 1,158, cash 30→139, condition 100 and one unique completion. The scoped recording closes/archives with 203 events, digest `0e8fc4e0243c9face6da6180d48f30a13d729aea250edfd65f14e09e7693f8b0`; native session `94d7400c8663445fb62e814949316957`, archive `199bdd93-f5dc-4f54-9d08-6b81be70c25f`, manifest `81f751de79078c16b3f0f63539f7fe6bbb8cfedddf05eba9e5827ddc32637609`. These are retained evidence identities, not downloadable gameplay. The sample remains below the earlier 132-loop baseline and does not rerun hull contact/police/boat cases on this exact ROM.

## Retained 123-byte hull candidate — scoped street functionality, 2026-10-02

Hull ROM `555f3d319e91d7e0ac593ba9445805bf6bbd36d169214e1fde736a4f2ceb51a8` has source fingerprint `695160b75028bbc24abf342fb56ef28c2108727b2343244a57742e2a7f88087e`, NOI `0559ce168d0e8654f5f8f3536aea29c14646eb03abf38077b3905dc4e3cb4ee5`, 1,098-byte reserve and compiled OBJ budgets 116/106 passing. It records 91 loops / 360 VBlanks; its scope is separate from the later 87-byte bucket/cadence builds.

A genuine human impact at frame 1,008 changes cash 30→10 and H0→H1, with message 19 and rendered stumble OAM. Map frames 1,156→1,516 preserve all 58 game, 13 flight and 13 boat bytes. Save followed by the real A+B+Select+Start reset restores HELP at 1,976 with cash 10/H1/countdown 29 and saved courier `(560,669.4375)` plus Core parked car `(560,720)`; presentation/subsecond belong to the saved checkpoint, not later inspection. Police proximity refreshes the countdown at 3,072/world second 32; capture at 3,312/world second 36 reduces cash 10→0, clears heat and displays message 20 with visible police OAM. Ordinary parking/exit and walking down reach the blocked shore `(560,807.9375)` at 3,792; a single visible boat OBJ uses tile 114/palette 7 at 3,792 and 3,912 with no sampled overflow. This does not test every underpass or crowd case.

Scoped functionality passes at frame 4,152, 567 events, digest `992e4e53cf7c9674e127c51ed729da6e580b7e63491ed805f8077de43fe6b9b3`; recording `0d7593f0-d193-4d75-9346-6f6cfe09cea1`, archive `6db59f38-33f8-4a05-9578-33d28ebc9080`, manifest `741093973b9e5508442037a144fced18c8f3b9d7c252b1b1814db0f718997dbd`. [NATIVE_STREETLIFE_SAMPLES.json](docs/NATIVE_STREETLIFE_SAMPLES.json) retains exact state/OAM/counter/build evidence. Broader pacing, other attention levels, paid rides and physical acceptance remain pending.

## Retained 75-byte epoch candidate — scoped courier replay, 2026-10-02

The earlier 75-byte epoch ROM `35337509…` records 90 loops / 360 VBlanks. Its ordinary-control courier sample delivers at frame 1,872: cash rises from 30 to 122, carried condition is 86 after traffic/curb contacts, unique completion count becomes one, the job clears and the result screen appears. Continued steering, reverse and dispatch controls work in that scope. Diagnostic `90ea51fd…` reaches 95 loops / 360 only with external terrain/future-tram guards cut; its recording is closed/archived and full guards are restored. That cut is never gameplay acceptance, and neither sample verifies the later 87-byte cadence/fused ROMs or whole-city performance. [NATIVE_STREETLIFE_SAMPLES.json](docs/NATIVE_STREETLIFE_SAMPLES.json) keeps performance candidates separate; source fingerprint `75ea2597…` and NOI `afe312eb…` identify that epoch replay, not the later cadence/fused source.

## Port Lands candidate — three scoped native replays passed, 2026-10-02

Official `project/build/toronto-port-lands-labels.gbc` is 524,288 bytes, SHA-256 `a212dd9ed479310a98e58b701416f8af88aaa09174f186747dca678ff4c164f4`. Matching NOI is `b4a500d607a73fff08d1f4764b543ff3798d556aca302e9ead4b56c7f3d9649f`, globals `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a`, source fingerprint `025edad8ffaa318a4739c0c1f4aee3b539789a8808dd1c11a20a1d9943dab98b`, project revision `151a988b2b77e3312da36ee82d0f376b8fc15369fe21bbee0780b7d47b8682cc`. Public header inspection passes CGB-only/MBC5+RUMBLE+RAM+BATTERY/32-KiB SRAM. Matching compiled aircraft/Queen allocations pass for all five city scenes: OBJ 92/92/82/92/82 of 128 and bank-1 BKG 16/15/0/0/0 of 32. Heap `DA68` to stack base `DF00` retains 1,176 linked bytes; deepest runtime stack and physical execution are separate checks.

Full local `make check` passes. The five registered districts have 15 reciprocal seam pairs, 235 original buildings, 565 fixed foot-safe pedestrian routes and 24 expansion traffic loops with six live slots per loaded scene. Atlas API/UI sanitizer checks pass 1,475,876/24,151,786 cases; two synthetic banking fixtures pass 6,280,488/6,280,132 cases, rejecting12 malformed-data/append-policy cases. Aircraft renderer passes 41,341,613 checks; runtime passes695,825. Vertical and horizontal seams have38 independent checker cases. The regenerated street table has a read-only freshness gate; all existing 51 stop,88 contract and transit content fields were compared unchanged after regeneration.

The official PyBoy 2.7.0 CGB worker is `6271cbbb9ca76d4d149173f4110705cfa3c567349f0cb7ceb347bf1df7598925`. Inputs are ordinary buttons and observations read-only. [NATIVE_PORT_LANDS_SAMPLES.json](docs/NATIVE_PORT_LANDS_SAMPLES.json) retains exact frame/state/flight/OAM/build and archive identities. No emulator state or campaign progress was injected.

The travel replay boards Queen46→49, pays once (`30 → 27`) and reaches actual East at 6,648. Walking south crosses Leslie into authenticated Port Lands4 at 7,640. The courier explores industrial streets, crosses the Unwin circulation-channel bridge and reaches Cherry Beach at 13,636. Down for 300 frames reaches the blocked southern edge `(432,919.5)` at 13,936. Other routes encountered vehicles and terrain, requiring alternate foot approaches; this does not establish all bridges or every vehicle's clearance.

Map frames13,976→16,076 pan down and left into the unmapped lower hole, then right to Port Lands. All58 gameplay bytes and 13 flight bytes remain exactly unchanged across these map steps. Save at 16,228 records foot `(432,919.5)` in district 4, Core car `(560,720)`, cash 27/condition 100/world 227. Actual A+B+Select+Start for 120 frames in the same worker reaches HELP at 16,360; neutral for 180 frames ends16,540 with those saved fields restored. A for 8 frames and neutral for 12 frames resumes and Up for 120 frames moves to `(432,859.5)` at 16,680. This is soft-reset persistence, not cartridge power-off evidence.

At16,980 a visible plane has four aircraft OAM objects plus its shadow. At18,900 and 18,908 a helicopter has four aircraft objects plus shadow with two different compiled rotor poses; ticks 213 → 221. Those sampled OAM views peak at ten objects per scanline with zero over-limit lines. Other sampled active-flight moments deliberately lack visible aircraft when crowded ground capacity wins. Native walking returns through Leslie4→3 at 26,554; neutral for 180 frames ends26,734 in authenticated East `(816,952)`, cash 27. Four unmodified native images and their exact frame/journal provenance are in `docs/screenshots/`.

A fresh driving replay picks up the first package and delivers at 740, cash 139/done 1/job NONE/condition 100. A+right for 48 frames ends840 at speed 20/heading 4; held A for 24 frames ends864 still accelerating at 24. B for 64 frames reaches reverse −6 at 928; A+right for 48 frames recovers 6 at 976 and A for 48 frames reaches 18 at 1,024. Neutral180 ends1,204 stopped at `(724.0625,751.4375)`, condition 100. This tests the observed turn/recovery, separately from truck/motorcycle/scooter and Port Lands driving acceptance.

A third fresh Front Street replay reads `_game_time` from this matching NOI at `C0B9`: six neutral 60-VBlank windows give 29,20,15,19,29,20 completed engine loops, total 132. Stationary player/car/cash/condition remain intact; end samples peak 4/4/6/4/4/4 without overflow. [PERFORMANCE.md](docs/PERFORMANCE.md) compares this scoped result to its actual baselines; it is not whole-city or physical frame pacing.

| Scoped pass | Original recording / session | Frames / events / final journal digest | Archive / manifest |
| --- | --- | --- | --- |
| Travel, map, saved Port Lands, plane/helicopter, return | `session-4c31f047-edb7-4696-9d45-6c9a020d6588` / `058330fbae7248b49c073f20b646f959` | 26,734 /2,181 / `25867d4ca3a726a5c3efc6062429f3fe149fa60971ab6fe5feb0d5fe24f8c87e` | `6b5223b6-19fb-4d5f-8c98-a86b7b350195` / `95031ba5fe18555d3df388912d358316259c86a32f7dfaa3b6a2242ab987c317` |
| First delivery and driving | `session-01327aea-575b-4433-8ef2-ee70ab9c5f1b` / `4fee1fe623884936ad539e24b3735980` |1,204 /165 / `7aaca9fae0fbfb088f24c48d99667a7687af3913d27b94a9a931ec530257797f` | `943661f9-55e4-47d3-a138-add3abd99530` / `c368e3630184582caafee369e84e234ee265f0613e8771ddbea5cb8b9d07c33c` |
| Front Street loop samples | `session-83930f52-6eb8-4c7a-8c03-2969e4da0db7` / `1b2b11e1852648af86811e877c1b94d0` |842 /75 / `521b3b715d482dba12550bed62e7b7d6e1909959aa698d59f261f4201d9eadbd` | `48ad8468-c2e2-4d21-84ff-90c582f3f9ca` / `1c71e7d3b2f0635d40656b5ac0e096f685a4fca5081955fcd53cc5dbc689c57e` |

All three owned sessions are stopped, closed and archived through the plugin. Earlier exploratory `6a8a9f96…` passes performance/delivery but its travel outcome remains `needs-review` for stale Bloor Street HUD in Port Lands; [NATIVE_FIFTH_SAMPLES.json](docs/NATIVE_FIFTH_SAMPLES.json) retains that identity. Regenerating the street table fixes it in `a212dd9e…`; evidence is repeated against the new ROM instead of relabelled.

Port Lands is currently free roaming with eight proposed clients, not eight additional playable contracts. The 88-contract campaign, full Old Toronto/full Islands, measured two hours, all seams/vehicles/bridges, crowded frame pacing, deepest stack, human handling/listening, browser recovery and physical cartridge write/read-back/cold boot remain pending. Existing contact/tram/legacy-transit/roof tests below keep their own ROM identities. No physical cartridge operation was attempted.



## Contact recovery candidate — three scoped native replays passed, 2026-10-02

Official `project/build/toronto-contact-recovery.gbc` is 524,288 bytes, SHA-256 `1400566247fddfa9a1acd5ef42e5f90bb19db9caa43a51fab0ce82400caa137f`. Matching NOI is `6f8b6d662fe3d080a8a0a00f5d398521a35e4ddc62dcfddec5125b942264ea68`, globals `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a`, source fingerprint `e6bafd235ddb547928f8c760672c20d4f55518f99f24d519e3758142d6a253dd` and project revision `03e03e4f63cda3ece1d5732ca557e5f3f62ea88a1cf08cb92056342073519f64`. Local binary/debug-artifact hashes were independently read. Public header inspection and aircraft/Queen compiled gates pass; heap `DA68` / stack base `DF00` leaves 1,176 bytes of linked static reserve, not proof of deepest runtime stack use.

The first contact build exceeded the TORONTO code bank, 16,750 bytes against 16,384. Moving the pure route-segment query into the existing banked runtime resolves that build failure without changing thresholds or layouts. The official corrected build and full `make check` pass: 476,667 engine, 8,036,664 atlas-UI checks and the retained flight/renderer/atlas-API/transit/streetcar/bridge/navigation/memory/source suites. Host fixtures cover booked-tram HOLD occupancy and landing clearance, one tram penalty over 180 real world ticks with A held, rejection of invalid impacts, and independent legacy NPC/terrain cooldowns. These checks do not establish native forced-HOLD reachability or crowded performance.

A read-only geometry audit checks 21 actual transit nodes × eight candidate directions × 19 intermediate samples, with zero intermediate full-footprint/terrain mismatches on the current collision grids. This scoped result does not prove that centre-path validation remains sufficient on a future map; changed terrain still requires swept full-footprint checks.

Fresh ordinary-button PyBoy 2.7.0 uses worker `6271cbbb9ca76d4d149173f4110705cfa3c567349f0cb7ceb347bf1df7598925` and the new source-debug identity. The walking guide is adapted to the new native cadence: Save records foot `(823.5,239)`, car `(560,720)`, cash 30/condition 100 and world 35. This is near the earlier `(824,238)` failure, with deliberately recorded differences in position/timing rather than an identical-trajectory claim. Actual A+B+Select+Start for 120 frames resets the same worker; frame 2,721 reaches HELP with the saved 58-byte checkpoint restored, including that safe foot position. A8 plus neutral12 reaches 2,741 with the foot position unchanged and the NPC sprite already eight pixels south of the courier sprite after beginning its retreat.

Holding down for 600 frames reaches 3,341 at `(823.5,384)`, world 45/subsecond 19, instead of trapping the walker. Sixty frames each of left/right/up/down then produce `(796,385)` at 3,401, `(826.5,385)` at 3,461, `(827,356)` at 3,521 and `(827,384.5)` at 3,581. Neutral32 ends at 3,613, `(827,385)`, world 49/subsecond 51. Cash 30, condition 100 and parked car `(560,720)` remain intact. A sampled plane/shadow OAM frame peaks at eight objects per scanline with no over-limit lines; it is not a general crowded-scene certification.

The recording has scoped `PASSED`, 3,613 frames / 413 events, digest `96639f2f486b99194b691098d2f78906a3139b6a7395b721d41ddb202e0ef7f1`: original ID `session-af2b9158-ec10-479c-87e1-975b02740d88`, session `f0f6be4b220046698f53edd0737ce862`, closed/archive `fed85989-d8f8-4db7-909d-d3d1e2a10d72`, manifest `13951d46b7cd01ac848f8eab67375e0a53167e55a1e6afee6ee0d5d91d8f81ff`. Private immutable journal/frame evidence remains ignored locally; no RAM state was injected.

A second fresh source-authenticated recording on the same ROM/worker follows ordinary foot controls from parked Core car `(560,720)` to Yonge `(673.5,553)` at frame 1,336. Selecting Queen 46→43 enters WAIT at 1,404, cash 30/world 18; at 3,264 it is RIDE, cash 27/world 49/subsecond 33, 11 seconds left. Paid-map frames 3,296→3,416 preserve all 58 gameplay and 13 flight bytes exactly. Actual A+B+Select+Start 120 followed by neutral 180 reaches HELP at 3,740 with the latest paid checkpoint: cash 27/world 49/subsecond 1, 11 seconds left, origin 46/target 43 and the car still `(560,720)` in Core. This restores the saved paid checkpoint rather than the later map-frame clock.

Resuming with A8/neutral12/neutral1000 reaches an authenticated actual West scene at 4,760, foot `(836,556)`, cash 27/world 65/subsecond 40, on-foot ROAM. Parkdale 43→Leslie 49 boards in the current open window at 4,840 after the 4,832 menu, cash 24/world 65, 24 seconds left; neutral1600 reaches an authenticated actual East scene at 6,440, `(780,524)`, cash 24/world 91/subsecond 56. Leslie 49→Saulter 48 enters WAIT at 6,484/world 92; neutral960 reaches ROAM at 7,444, `(128,556)`, cash 21/world 108/subsecond 11. The three journeys charge `30→27→24→21` once each, with no additional fare after the paid reset.

A northbound plane at 7,844 is `(133,577)`, tick 50/direction 3, with four actual aircraft objects and shadow, sampled peak eight per scanline and no over-limit lines. At 14,257 the logical westbound helicopter `(128.25,557)`, tick 149, has no aircraft OAM because cosmetic rendering yields to crowded ground capacity; it is not claimed as visible. Up28 reaches 14,285 with foot `(128,545.5)`, helicopter `(108,557)`, tick 176/rotor phase 0; neutral12 reaches 14,297 with foot `(128,544)`, helicopter `(100.5,557)`, tick 186/phase 1. Both later poses have four fully visible aircraft objects and a separated shadow. Their sampled hardware peak is ten objects per scanline with zero over-limit lines, proving those two actual rendered poses rather than only logical flight state.

This second recording has scoped `PASSED`, 14,297 frames / 1,235 events, digest `659d6daa87f0a20d9e694a08e63319a026e7d44c45ebccbeb384da1242057e42`: original ID `session-1d9313da-e8a2-4121-aeaf-42c7f11ad963`, session `5a13e9500d2a4445914b916f6fb93234`, closed/archive `fc96bd6c-725f-4ffb-aab1-5b30138d11d5`, manifest `6d8b7fe80542bdf39155a08cf2b77ba64a4fbb21ad4977f13c98ba1509515637`. It does not repeat delivery/driving, all four gameplay scenes or roof restoration on this exact ROM.

A third fresh source-authenticated recording repeats first delivery and driving on this same ROM. Pickup at 502 has job 0/stage 1/condition 100. The initial Select step at 740 still has an active job because the engine input is pending; the retained frame-744 image visibly reads `CONTRACT DELIVERED`, without a cash-state claim at that frame. Read-only state at 792 confirms cash 139/done 1/job NONE/condition 100 and speed 8. A+right reaches speed 20/heading 4 at 840; held A reaches an actual terrain stop at 864. B reaches reverse −6 at 928, A+right recovers speed 6/heading 10 at 976, and A reaches speed 18/heading 11 at 1,024. Neutral input ends stopped at `(748.375,701)`, condition 100, frame 1,204. These values belong to this corrected ROM, rather than the earlier aircraft replay's different recovery values.

The driving recording has scoped `PASSED`, 1,204 frames / 165 events, digest `e08f7b1e68524535a55db782c6a766f67f169f5f6b43b2c6ddbf4e57d7c8eaac`: original ID `session-becf18a7-7908-424b-83ee-20b2aa8cb102`, session `947d665ea4b34b4c9de3619a3ca30a5d`, closed/archive `910a768c-9c72-465d-86a9-d3eabb7a223f`, manifest `76fcbab6d3824ed27ac55b7b115aa44b5a0d76bdcf21920cffbbd80add011196`. All three corrected-ROM recordings are closed/archived; their scoped passes do not establish the full campaign, whole-city performance or hardware behaviour.

Native forced-HOLD, blocked alighting fallback, rare tram-retreat conditions and crowded CPU performance remain pending despite host coverage and these scoped ordinary journeys. Broader handling, full city/campaign/two-hour duration, browser refresh and physical hardware remain unverified. Earlier deadlock and aircraft-scoped passes below retain their original build identities and outcomes.

## Reachable walker contact deadlock — retained pre-fix native failure, 2026-10-02

A new ordinary-button replay on the preceding `20370fea5661e2b9789bf6b1f77dc84535f06348698e2de3e6652f013a5ae189` aircraft ROM reproduces a route-consistent walker/traffic overlap after an actual save and game-button reset. This is a newly verified failure outside the earlier aircraft replay's passing scope; those scoped passes below remain retained.

The player parks the car at `(560,720)`, walks normally to `(824,238)`, opens pause and selects Save. Holding A+B+Select+Start for 120 frames resets the game within the same native worker. The restored foot position overlaps Core's naturally cold-started autonomous traffic slot 4 at `(824,240)`; no RAM writes, injected positions or off-route actor fixtures are used. At frame 2,692 the courier is `(824,238)`, world 34/subsecond 30. Holding down for 600 frames reaches 3,292 with the same position, world 44/subsecond 31. Holding left, right, up and down for 60 frames each reaches 3,532 still at `(824,238)`, world 48, cash 30 and condition 100. The world advances while all four walking directions fail to escape the overlap.

The replay uses ROM-mode inspection of that exact older binary because its source was being changed for the contact correction; it is not claimed as authenticated playback of the edited source. Read-only WRAM/OAM observations establish the retained native failure. The immutable archived journal independently confirms `failed`, 3,532 frames / 383 events and digest `31991f7a4efac74150736f5e5c703888e10fdc314ebea48891b01ac1e0e3f6df`. Original recording ID `session-96ab5bc6-fe59-4891-914b-cbffb0506831`, session `8b201ff8e7b4497d95dda4199173cd3d`, is closed/archive `f4e4a824-72da-4bfc-addd-dae14c878e04`, manifest `8c0a4358d3ebd5af03c754aec5cce5df2aa99f8828a7d51c64d22ca2e6ecd2b3`. Journals and frames remain private ignored local evidence, not a downloadable gameplay artifact.

This pre-fix failure required a contact correction, new official build and matching ordinary-button replay; the later candidate above records its scoped recovery evidence. Earlier aircraft rendering, map, delivery and driving passes do not establish this crowded-contact recovery gate or full-game acceptance.

## Aircraft portability candidate — scoped native replay passed, 2026-10-02

Linux/GCC CI at `a3b6984` rejected the renderer/harness with `-Wmisleading-indentation`. Splitting adjacent statements onto separate lines corrects that formatting warning while retaining `-Werror`. Full local `make check` after the whitespace-only fix passes 4,501,851 flight, 40,631,157 renderer and 110,995 engine checks and the retained suites. Hosted GNU CI passes at source-fix commit `3a1869adadbee61b6a7b0faf578ebdd3c135c624`: [PR run 37065405078](https://github.com/trancethehuman/modretro-games/actions/runs/37065405078) and [push run 37065401191](https://github.com/trancethehuman/modretro-games/actions/runs/37065401191) both execute successful `Run make check` steps with these same counts. This is hosted source verification, separate from native gameplay.

The official rebuild is `project/build/toronto-aircraft-portable.gbc`, 524,288 bytes, SHA-256 `20370fea5661e2b9789bf6b1f77dc84535f06348698e2de3e6652f013a5ae189`, with source fingerprint `8eca3164dd29a33ade067e2e4b4ecc145cc7a023c55028f3c537fae71801bc09`. Matching NOI `cda7c22498fad7c73b30e7cf10ac778147d0eee93af105fd564ac021d8f430bb`, globals `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a`, compiler identity and project revision `03e03e4f63cda3ece1d5732ca557e5f3f62ea88a1cf08cb92056342073519f64` match the preceding build. ROM/debug hashes were independently read. Public ROM inspection passes CGB/MBC5/32-KiB-RAM header checks; compiled aircraft/Queen and 1,178-byte memory gates pass.

Byte comparison against `8e7af3ec…` finds exactly five changed bytes: global checksum byte `0x14F` and four `_save_signature` bytes `0x481–0x484`. All other ROM bytes are identical. The earlier recorded source fingerprint remains `37927f56eff7941139717c22cfd8d8bae16662a36e866c4b7a3fc27751a4d4d1`; its recordings are not reassigned to this rebuild. Separate fresh native acceptance below authenticates the new source-debug identity.

Ordinary-button PyBoy 2.7.0 with the same `6271cbbb…` worker repeats first pickup/delivery, with delivery at 740 yielding cash 139/done 1/condition 100. Driving reads speed 8 at 792, 20/heading 4 at 840, terrain stop at 864, reverse −6 at 928, recovery 5 at 976 and 17/heading 10 at 1,024; frame 1,204 ends stopped at `(720.6875,684.125)`, condition 100. A westbound plane at 5,979 is `(649,693)`, tick 122, partly clipped at the left edge with its shadow. A southbound helicopter at 7,934 is `(698,718)`, tick 216, rotor phase 1; at 7,943 it is `(698,724.75)`, tick 225, phase 0. Both poses have four visible aircraft objects and a separated shadow, peak six per scanline with no over-limit lines. Map frames 7,979→8,099 keep all 58 gameplay and 13 flight bytes exactly unchanged.

Recording `session-9c9166e5-fae7-4ee1-8f3b-782361b17e2d`, session `05149aa9206c4521806f90c9f658ef52`, stops with scoped `PASSED` at 8,099, 653 events, digest `2db354e455e03c62e25ef33d33560f3615ba56a7ca48f7e1bcb26c0d6c61ddf5`; it is closed/archive `d7a3496c-9b63-499c-84cd-5eefe1ad060a`, manifest `e39f5b3550037bc8c8b0b439187a6136ca9b333f5e52b3a8281343b77d555690`. This replay does not repeat roof restoration, paid transit, all scenes or reset on the new exact ROM. Those observations below remain tied to `8e7af3ec…`. The three contact branches, broader native/human/crowded performance, full city/campaign and hardware remain pending.

## Previous aircraft candidate — retained scoped native samples, 2026-10-02

Official `project/build/toronto-aircraft.gbc` is 524,288 bytes, SHA-256 `8e7af3ec08349c4dbef473bae30ac940ca727c0524b04b9711c04817b8678258`. Matching NOI is `cda7c22498fad7c73b30e7cf10ac778147d0eee93af105fd564ac021d8f430bb`, globals `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a`, build source fingerprint `37927f56eff7941139717c22cfd8d8bae16662a36e866c4b7a3fc27751a4d4d1` and project revision `03e03e4f63cda3ece1d5732ca557e5f3f62ea88a1cf08cb92056342073519f64`. Local ROM size/digest and both debug-artifact digests were independently read. The official build, CGB-only MBC5 header, compiled aircraft/Queen frame/resource gates and 1,178-byte memory guard pass. The compiled-pose cache remains 170 persistent bytes. Full `make check` passes 4,501,851 flight, 40,631,157 renderer and 110,995 engine checks, plus the retained atlas/transit/streetcar/bridge/navigation/memory and source-content suites. Host checks and linked static reserve remain separate from native timing and physical hardware.

The exact-ROM ordinary-button PyBoy 2.7.0 recordings use worker SHA-256 `6271cbbb9ca76d4d149173f4110705cfa3c567349f0cb7ceb347bf1df7598925` with matching source-debug identity and read-only state/OAM/VRAM observations. The scoped flyby recording shows:

- Frame 666: plane at `(545,701.5)`, tick 73, and separated shadow. A later westbound plane at 6,309 is `(515.5,697)`, tick 45.
- Frame 4,102: northbound helicopter at `(562,725.25)`, tick 121. Frames 4,162 versus 4,171 show both rotor phases, with separated shadow. Sampled OAM peaks at six objects per scanline with no over-limit lines.
- Map frames 966→1,086 preserve all 13 flight bytes and all 58 gameplay bytes exactly. Ordinary walking/camera samples at 4,903/5,003 and 8,293 retain aircraft world paths.
- Frame 8,281: a southbound helicopter at `(449,674.75)`, tick 153, overlaps a priority roof. Actual background indices 695/696 reference scratch tiles 35/36 with attribute 140. Twelve left-walking frames and twelve Start frames reach pause at 8,305; those cells restore original tiles 31/26 with attributes 164/132 and no renderer-owned scratch references remain. This is sampled native roof/scroll/pause restoration; ground-through-transparent-gap preservation has host coverage, without a new general crowded native claim.

Plane frames 666→786 advance 47 updates over 120 VBlanks with the flight visible for part of the interval. Aircraft-inactive frames 1,362→1,422 advance 18 over 60; helicopter-visible 4,102→4,162 advances 15 over 60 (about 15/s). These differently timed NPC phases are bounded observations, not a matched global benchmark. Crowded-scene and whole-city performance remain pending.

A fresh same-ROM driving replay picks up contract 01 at frame 502 and delivers at 740 with cash 139, one unique completion and condition 100. With A held, speed is 8 at 792; A+right reaches speed 20/heading 4 at 840, followed by an actual terrain-edge stop at 864. B for 64 frames reverses to speed −6/y722 at 928; A+right for 48 reaches speed 5 at 976, then A for 48 reaches speed 17/heading 10 at 1,024. Neutral input for 180 frames ends stopped at `(720.6875,684.125)`, condition 100, frame 1,204. This establishes the sampled delivery, positive-speed steering before solid contact, braking/reverse and recovery. It does not claim turns never stop, every traffic condition passes, or the older ROM's speed-24 result.

- Flyby scoped `PASSED`: original recording `session-dd795e15-ad93-4db1-9e8c-1105154141bb`, session `7b27ac4d571744718b0e03e8c13fa8c9`, 8,305 frames / 697 events, digest `aaa376fe644838f155434a885915c1cfaad09d8cfca22fd0b78bffcf2cd639c0`; closed/archive `95663e5e-45dc-41d2-9285-e2840c9f5a51`, manifest `6f78af8614102d6c0a903a717aec4c898c1bf38784b30e4f8550457d3b447bfe`.
- Driving scoped `PASSED`: original recording `session-84c08448-2d56-4f46-b377-c97eed8a3f9d`, session `62d2d503b97c4954a21a3a2bf4fc6a19`, 1,204 frames / 175 events, digest `13a4d27c0fc9bf0c7e91e03528ec740a68c1ed67107ef3532d215c073cc3a974`; closed/archive `0c9bdcd7-83d3-46b9-9de8-c8571302b431`, manifest `f0a84c544b55995c47990d552c53ffbfe64636f642c7132fc5e749c2fa476b9b`.

A third ordinary-button recording reviews three paid Queen journeys and all four actual native scenes on the same ROM. The courier exits the car at frame 274 at `(560,738)` and walks near Yonge by 1,336, `(674,553)`. Yonge 46→Parkdale 43 boards at world second 48, charges cash `30→27` and is observed alighted in West at frame 4,080, `(836,556)`, world 62/subsecond 49. Parkdale 43→Alton 50 next boards at world 64, charges `27→24`, freezes all 58 gameplay and 13 flight bytes exactly on paid-map frames 5,160→5,280, and alights in East at 6,636, `(880,524)`, world 96/subsecond 45. Sixteen left-walking frames leave the platform before a third Alton 50→Parkdale 43 trip charges `24→21`; West arrival is observed at 9,544, `(836,556)`, world 141/subsecond 53. These are functional observations, without a final recording-level `PASSED` label.

Ordinary westward walking reaches `(617.5,640)` at 10,234. At 11,434 the logical district is 2 but the West scene change is still pending; that frame is not claimed as a loaded High Park scene. After 500 neutral frames, 11,934 has the actual High Park scene ID `992c5526-486f-5c69-9130-578b374b5123`, courier `(1000,641)`, cash 21 and the car parked at `(560,720)` in Core. A westbound helicopter there is `(1013.25,643)`, tick 57, kind 1/direction 2, with four aircraft objects plus two shadow objects; sampled peak is six with no over-limit scanlines. Debug observations confirm all four actual gameplay scenes, rather than inferring loaded geometry from a logical district field.

After pausing at 11,946, an attempted checkpoint restart used the tool's `emulator_run restart:true` and created a fresh worker with clean SRAM, Core/cash 30 at frame 300. This is a tool clean-boot outcome, not a ROM save failure or paid-reset test. It automatically stopped the original recording at 11,946 without an assessed `PASSED` outcome. Original recording `session-85f3a704-b42d-4209-b946-3ccfc5586842`, session `15764619bdbf4682810d08189ce78597`, has 1,140 events and digest `f7538be4397286d068d3ed6f9d38cac985de79610a0bebbfb2d4a80a91599688`; it is closed/archive `c8027a0b-911c-4c3f-8931-1619592944f1`, manifest `58c41fe7badfde87573824268936505fd927f4e1361da1813b4e01a8a29e7c7c`. Its reviewed journey/scene samples remain valid; the immutable recording status is not relabelled.

A fourth fresh recording verifies SRAM recovery through the game's actual reset buttons. Contract 01 delivery at 740 again yields cash 139/done 1/condition 100. The car is reversing at 752, `(759.5,720)`, speed −2, then pauses at 760. Holding A+B+Select+Start for 120 frames reaches blank boot at 880; 180 neutral frames reach HELP at 1,060 with the saved delivery checkpoint restored: player `(759.625,720)`, parked car `(560,720)`, cash 139/done 1/condition 100, world 9/subsecond 11, speed 0. This restores the saved delivery checkpoint, not the later reversing pose. Eight A frames and 420 neutral frames resume to 1,488 with cash/count retained and no fare change. Cosmetic flight state starts anew: kind 1/direction 0 is an eastbound helicopter at `(670.25,694)`, tick 31, mostly clipped at the left edge. The immutable stop reason incorrectly calls it a plane; the inspected kind/direction correct that annotation, without claiming a fully visible post-reset pose.

The reset recording has scoped `PASSED`: original `session-15812b3d-a28f-4433-9296-a4f758ad7688`, session `e9a50e9fcebb4de99b310354f9494413`, 1,488 frames / 181 events, digest `9ea55c8516eeff9adafeaebe3adf2caa984d5c3cfb79150f5d9797adc583cd49`; closed/archive `433d4de1-e37c-43ad-8839-cecab1bf97d7`, manifest `0adfc8cb79f23bad55d8bfc922fed8aec97b0ed347f195e320c09268ee860d2c`. This is same-worker emulated SRAM/button-reset evidence; exact-ROM paid-ride reset and physical cold boot/interrupted power remain unverified.

Every aircraft direction/district, crowded native rendering, deepest stack use, human handheld assessment, full former Toronto/two-hour campaign and browser refresh remain separate open gates. The three documented contact/held-arrival/alighting branches and their final policy/native replays remain pending. Published Prototype 6 is unchanged; [current loading instructions](docs/LOADING.md) identify this newer source candidate separately. Earlier aircraft intermediates below retain their own observations and are not overwritten by these scoped samples.

## Aircraft cache intermediate — rendering sampled, further optimization pending, 2026-10-02

Official `project/build/toronto-aircraft-cache.gbc` is a retained intermediate, not the final aircraft build or a recommended loading artifact. Its 524,288-byte ROM SHA-256 is `4b83cfb68cd6d9d4d769faaa1afecffb52e1c32e91a54c66110dea143263b895`; matching NOI is `d6b71db62352e9fd961207068db2136049fc403712ee84c970b5a0831b29037a`, globals `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a`, source fingerprint `c03f04eb3f6d108f5c9b9c4435de54ff8cea638b581579e21c1ab0fa7312b7f1` and project revision `03e03e4f63cda3ece1d5732ca557e5f3f62ea88a1cf08cb92056342073519f64`. The local ROM size/digest was independently read. Compiled aircraft/Queen frames and resource gates pass; heap `DA66` / stack base `DF00` leaves 1,178 bytes of static reserve. The renderer's 170-byte compiled-pose cache and interval-event scanline sweep pass 40,047,786 actual-source host checks. Host checks and linked reserve do not measure deepest native stack use, visuals or performance.

The ordinary-button PyBoy 2.7.0 replay uses the same worker SHA-256 as the first aircraft attempt, `6271cbbb9ca76d4d149173f4110705cfa3c567349f0cb7ceb347bf1df7598925`, with matching source-debug identity. Read-only observations include:

- Frame 666: plane at world `(545,704.5)`, flight tick 75, with its shadow; sampled OAM peaks at six objects per scanline.
- Map frames 846→966: all 13 cosmetic-flight bytes and all 58 gameplay bytes remain exactly unchanged.
- Frame 4,112: northbound helicopter at `(564,714.75)`, tick 135, with shadow. Compiled tile references show rotor phase 0 at 4,112/4,172 and phase 1 at 4,181.
- Ordinary walking samples at 5,275 and 8,455 scroll the camera while flights retain world paths rather than following the player.
- Frame 10,115: plane at `(449,666.5)`, tick 79, overlaps a priority roof. Actual background map indices 631/663/664 reference scratch tiles 32/33/34 with attribute 140. Twelve Start frames enter pause at 10,127 and restore tiles 16/17/21 with original attributes 132/164/132. This verifies the sampled native roof patch and restoration. Exact ground-opaque preservation has host coverage; no general crowded native occlusion claim follows.

Aircraft-inactive frames 1,212→1,332 advance `game_time 2F→57`, 40 updates over 120 VBlanks (about 20/s). Helicopter-visible 4,112→4,172 advances `BD→CB`, 14 over 60 VBlanks (about 14/s); later aircraft-inactive 4,491→4,551 advances 18 over 60. These intervals have different NPC phases and are scoped observations, not a matched global benchmark. Further common-path optimization and a new official build/replay are required. This intermediate has no delivery, transit, reset or district-transition acceptance.

Recording `session-a4ad0f51-1e0f-4375-a3e0-140c387d334b`, session `5bdc27776b8a43989386d409cbecf586`, stops at 10,127 with scoped `needs-review`, 955 events and digest `1ba69f8cd71147d6db8ae1306ee1c9200c62944eade26ff0f16b70bb5c8a489e`. It is closed/archived at `85d4ad9d-57fa-4ee1-bf48-76c7a1675356`, manifest `07cd0b70bb5d34314ddbef8c9be87ed61ddfc8e5dcde170968c026fde273063a`; original-path mapping and immutable journals remain ignored locally. Every aircraft direction/district, crowded native frames, browser refresh, handheld assessment and physical cartridge execution remain separate pending gates. Published Prototype 6/loading is unchanged.

## Aircraft first native candidate — rendering sampled, optimization required, 2026-10-02

Official `project/build/toronto-ambient-aircraft.gbc` is 524,288 bytes, SHA-256 `9c1a9fcb0d7bd4302a62bb52dd54a3163a1ac55bfdb7bc0280b3f3bdae04b1f7`. Its matching NOI is `4a529097da3898281a8750cea839f960f5ac9c60bead3207d629cecbef0f150c`, globals `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a`, source fingerprint `5ffd0ee210697db9d22cb4e2824f554dad20237d1731eaa212f85458c2b864bd`, project revision `03e03e4f63cda3ece1d5732ca557e5f3f62ea88a1cf08cb92056342073519f64`. The CGB-only MBC5 header and compiled aircraft/Queen frame gates pass. Core/West/East allocate 92 OBJ tiles per bank, High Park 82; bank-1 gameplay backgrounds use 16/15/0/0 tiles before scratch IDs32–46. Heap `D9BC` leaves 1,348 bytes below `DF00`, above the 1,024-byte guard.

Full `make check` passes 4,501,851 flight, 39,684,119 renderer and 110,995 engine checks, plus retained atlas/transit/streetcar/bridge/navigation and memory suites. Host pixel/OAM adapters and linked counts do not prove native timing or hardware. The actual-source renderer tests cover all poses, OBJ/BKG flips, sub-tile alignments, priority masks, conditional restoration, scroll/window boundaries, double OAM and rejection under sprite pressure.

An ordinary-button PyBoy 2.7.0 recording on this exact ROM shows southbound planes/shadows around frames618–666 and2494–2586, including aircraft above the player's car. Frames4102–4118 show a northbound helicopter, both rotor phases and its separated shadow. Frame666 OAM peaks at six objects per scanline, frame4102 at four, with no over-limit lines. These are samples; roof overlap, every direction/district and crowded native frames remain unverified. The native source-debug identity matches the build.

The first sequence of four-frame taps failed to reach the map; longer ordinary presses entered pause/map. Pausing at818 preserves active flight `(545,919)`, ticks218. At986 the map retains this state; panning through1106 preserves all58 gameplay bytes and has no visible OAM. Closing map/pause applies no long paused catch-up. Holding B long enough to close pause also begins ordinary reverse after resuming, so final `(557.3125,720)` is not claimed as an exact stationary-position restoration.

Stationary Union's aircraft-inactive interval1154→1274 advances39 engine updates/120 VBlanks (~19.5/s); helicopter-visible4118→4238 advances22 (~11/s). These differently timed local samples show a slowdown worth fixing, not a matched predecessor/whole-city benchmark. The older streetcar57/120 ride sample uses a different mode/view and is not directly comparable. The first renderer rereads loaded OBJ pixels and loops over ground objects per scanline; a cache/scanline optimization and exact rebuilt replay are required. No first-delivery, transit, reset or district-transition acceptance was attempted on this aircraft candidate.

Recording `session-e3f312dc-1cfa-4cb5-937f-177851db8332`, session`bde2ff14c81c4434aebf41be4b5f6cca`, stops at4238 with scoped `needs-review`,491 events, digest`793fd5d925c9e8d5ca87fafd2d7d22fc98c5c4764f0ce6294e216e4c848f016b`. Its worker SHA-256 is `6271cbbb9ca76d4d149173f4110705cfa3c567349f0cb7ceb347bf1df7598925`. It is closed/archived at`1abd3518-ddf6-4cf7-803d-b86199df6625`, manifest`8e07b874d17ae3479f90382cbc66025d3727edc2ee123030915d54c2e76d8cf0`; original-path mapping and immutable journal bytes remain ignored locally. Published Prototype 6/loading and browser/hardware acceptance remain separate.

## Moving Queen streetcar — reviewable candidate, scoped native checks passed, 2026-10-02

Current candidate `a0e23f03…` restores one-pass scalar traffic while retaining direct clock-derived streetcar phases and additive range queries. It has two stopped, closed and archived ordinary-button recordings on the **same ROM**: three paid Queen journeys with map/reset/car recovery, and fresh first-delivery/held-acceleration driving. This is a reviewable candidate, not an accepted whole-game milestone or new loading recommendation.

| Identity | Value |
| --- | --- |
| Native output / bytes | `project/build/toronto-streetcar-one-pass.gbc` / 524,288 |
| ROM SHA-256 | `a0e23f030425ebf0f0435f5a88d6a3c17eb36564d5dedc2c2871c5e03af13eeb` |
| Matching NOI SHA-256 | `35d3c9ad7656768a9f9ceab79dd0bf0e3a85505bfc5ea088eda22afdcd86c266` |
| Globals SHA-256 | `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a` |
| Build source fingerprint | `99494efe984da1354dccf2f8b3a1de6fb113b4f45bf38b67ef516570ab94ebf8` |
| Plugin project revision | `f9c648669ddf0df7ec2d34550dad083dd613411ef3f3be4c08aad358973761c1` |

Local ROM bytes/digest and matching NOI digest were independently read. Final full `make check` exits 0 with **110,986 engine**, **3,771,783 streetcar-motion**, **299,366 atlas API**, **8,036,093 atlas UI**, **609,452 transit**, **2,974 district bridge** and **16,997 navigation** checks, all zero failures, plus nine memory-checker tests and the content/source gates. The failed batching APIs/fixtures were removed with scalar traffic restoration; historical larger host counts below remain scoped to those intermediates. Matching compiled-frame inspection passes eight four-object cardinal/door frames and empty startup frame 8. Linked heap `D968` / stack `DF00` leaves **1,432 bytes** reserve. Compiled tileset descriptors contain 56 courier + 10 tram OBJ tiles in **each OBJ bank**, 66 total against 128; native frame 5,592 samples tram base 56 with four objects, peak four on a scanline and no over-limit lines. These static/scoped results do not establish every crowded scene, palette/readability or physical behavior.

The transit recording completes Yonge 46→Parkdale 43→Alton 50→Yonge 46 across actual West/Core/East scenes, charging cash 30→27→24→21 once per ride. Paid map frames 4,448→4,828 preserve **all 58 saved bytes exactly**, then restore the tram at 4,872. The third ride's paused soft reset restores its paid checkpoint at world 96/subsecond 0, cash 21 and ride_left 16; resuming arrives on foot in Core, and ordinary walking/car entry ends at frame 8,822 driving at `(560,720)` with matching parked position and condition 100. Core frames 5,472→5,592 advance `game_time C6→FF`: **57 logic updates / 120 VBlanks, about 28.5/s** in that paid-ride sample, versus 45/22.5 on `b1cf9a37…` and 35/17.5 on failed `84e5fe20…`. This is a bounded comparison, not whole-city frame pacing.

The fresh driving recording picks up contract 01 at frame 502, condition 100 / cash 30; stopped delivery via Select at 740 is confirmed at 752 with cash 139/done 1/condition 100. With A continuously held, frame 792 has speed 10/heading 0, frame 840 after A+right has speed 22/heading 4, and frame 864 has speed 16 with `PED BRAKE`. **The current-ROM result at 864 is 16, not the historical 24**: pedestrian timing differs. Further held acceleration reaches blocked terrain/speed 0 at 944; ordinary reverse reaches speed −6 at 1,008, then steering/acceleration resumes speed 6 at 1,056 and 18 at 1,104. This verifies the observed recovery and positive-speed turn, not an absence of traffic braking or every handling condition.

- Transit scoped `PASSED`: original recording `session-ebb97eaa-d39b-4c84-970e-788b5c106dee`, session `1119ef35b76d4ee9b25c293e49de018d`, 8,822 frames / 541 events; journal `9930fe6d6c34db029612a89e7fc675fd1ad8ba8368ef397677615724ba6c2700`; archive `1a07ba7a-2f3e-4783-88f7-dcffbaaf7728`, manifest `39c5401cef10158f225e2a6574dbc26b6b84de2e7753f94802c7241740d42158`.
- Driving scoped `PASSED`: original recording `45ba255d-30a3-40b8-809c-b9cdbab756c5`, session `4079af99b1cb4e23ad15707d51eb12de`, 1,105 frames / 130 events; journal `a28a198ad4f1ace133c166749fcdb8aa85757f2311df8e4e7d3f4864e471584f`; archive `29d5c565-e001-4ccd-a6e3-1553bdb94482`, manifest `a72a13462bdd4a8d88fc406e8d4e6e32de32f420020a40b7fe00baa8cc86a627`.

Fully crowded contact recovery can still return `BLOCKED`; its release policy, broader collision/traffic/held-arrival coverage, palette/readability and crowded performance remain pending. Full former Toronto, the complete campaign, two measured hours of enjoyable play, browser refresh and physical cartridge boot/save/audio remain unverified. Published Prototype 6/loading is unchanged. Earlier failures and intermediates below retain their own identities and are not overwritten by these scoped passes.

## Moving Queen implementation and historical intermediates, 2026-10-02

The newer source adds original rail artwork and a single scheduled moving Queen streetcar to West, Central and East. It follows the existing fictional 64-second service cycle, with two-second door dwells, the retained four-second stop intervals, the East bend and reciprocal Queen scene seams. Its original sprite has four closed cardinal poses and four door poses, using four 8×16 OAM objects per pose; the body is 28×12 pixels horizontally or 12×28 vertically. Source geometry queries validate the entire footprint and swept path on the actual registered collision grids. Corrected candidate `b1cf9a37…` now has scoped native sprite/paid-ride evidence below, but its measured core update rate leaves performance work and final acceptance pending.

The banked runtime derives a paid ride's visual scene, tram pose and camera focus while retaining the serialized boarding origin, booked destination, job, cash and parked-car identity until safe alighting. Pause/map stop the clock and tram motion; pending scene allocation and warm scene initialization reset the frame timestamp rather than charging catch-up time. Host fixtures cover loaded-destination alighting, failed queue retry, cold paid restore, stationary car/foot separation, rail parking refusal and valid older parked-car recovery, carried-only damage and proactive traffic yielding. The WAIT guard also requires the courier to remain within the actual origin's interaction region before boarding: displacement cancels the wait without a fare. Boarding-cue interpolation stays within signed 16-bit arithmetic. A fully crowded contact search returns `BLOCKED` without changing the saved position; its player-facing recovery policy and native crowded-contact acceptance remain pending.

Current writes use save version **7**, retaining the same **58-byte** state and two CRC16-checked records in SRAM bank 3. Reserved bit 0 now records a genuinely blocked Queen arrival only for a paid RIDE with one second left; it pins the booked destination after timetable cycles and cold restore. Valid version-6 records require the formerly reserved byte to be zero. Version-5 records migrate their original 48 bytes into the core district; version-4 fallback retains earnings/completions and retires obsolete active work. Host cases cover every interrupted current-record write, semantic rejection and older-record fallback. Version-6 binaries cannot read version-7 records; alternating slots preserve an older checkpoint during a write, but continued version-7 saves can replace both older slots. Physical cold boot and interrupted-power behavior remain unverified.

Before the native presentation corrections below, full `make check` completed with zero failures: **3,450 engine**, **299,366 atlas API**, **8,036,093 atlas UI**, **609,452 transit API**, **3,771,783 streetcar-motion**, **2,974 district bridge** and **16,997 world-navigation** checks, plus **nine memory-checker tests**. Campaign checks retained all 88 contracts and 51 stops, three parking anchors and their original prefixes; sprite regeneration verified 36 unique 8×8 patterns / 20 unique 8×16 patterns. Four scenes, 14 reciprocal seam pairs, 18 added-district traffic loops and 486 fixed pedestrian routes passed their resource checks. Atlas and western/eastern job metadata were refreshed only for source-provenance hashes; generated engine data, routes, deadlines and collision permissions did not change. Sanitized host adapters do not establish native actor lists, bank ABI, OAM/VRAM ownership, frame pacing or physical persistence. Source/art changes made after this checkpoint require their own fresh checks and native replay.

### Retained failed native replay

| Identity | Value |
| --- | --- |
| ROM SHA-256 / bytes | `deb78bbdc3f90535a787f08a60fed68afcc9477a862b61299629a453f02139ce` / 524,288 |
| Build source fingerprint | `fac772218749cdcc8089cf6541bdd54f71dafb3ba6cc9d8652488f0956d32bb3` |
| Plugin project revision | `81a99b21ea9687ef412ed4e2234ce4e05ccf0a8040e1d3757342a3ab0f0ba402` |
| Matching NOI SHA-256 | `6793cc89350ae99ee12fe505b46e8200d16ed6fffdd088f01cd32b78c131fdda` |
| Emulator session / recording | `ff436f67-0dbe-46b5-82ab-6a2464664b72` / `0eab8cf046994347a8f6b25a56217092` |
| Frames / events | 3,444 / 247 |
| Journal digest | `fd8f9aef2baf89460471010fb7bd7efd8bd2ee6f4a014f3e2dc7b8a72fbd4659` |
| Archive / manifest SHA-256 | `6ff7f48e-631c-4294-9e21-de9bfadd65b3` / `cce545df3dd7351767d52d99f1c2a8cfa9aec68397d29a16d1ed879303fc39c2` |

The official ROM built, but its ordinary-button native replay failed: the tram was missing from OAM despite a valid paid Queen Yonge 46 → Parkdale 43 booking, cash 30→27 and remaining ride time 12→8. Initial PLAYER absence was investigated; it is not retained as a confirmed PLAYER defect. The recording was stopped, its worker closed and its original evidence archived. Read-only compiled-asset inspection found empty horizontal tram metasprite frames: the optimizer discarded the authored negative editor-Y positions. An actor-size ABI mismatch was investigated and disproved; the actor pointer parse was corrected. This failed build is not the recommended loading artifact.

The source correction preserves the original PNG and native sprite/actor IDs while changing editor origins/positions so the optimizer sees all cells inside its 32×32 canvas. Runtime actor bounds now use GBVM Q5, with horizontal `−448…447` / `−192…191` and vertical axes exchanged. Actual-source engine fixtures pass **3,454 checks / zero failures** after two baseline failures, including all 3,840 poses in a 64-second cycle and Q4 geometry → Q5 actor-body equivalence with saved-state preservation. The fresh full `make check` at the corrected `b1cf9a37…` source checkpoint also passed, retaining the atlas/transit/motion/bridge/navigation/memory counts above. Later performance edits require fresh checks and their own ROM evidence.

### Corrected candidate: scoped rendering and paid rides

| Identity | Value |
| --- | --- |
| Native output | `project/build/toronto-streetcar-render-fix.gbc` |
| ROM SHA-256 / bytes | `b1cf9a3790058b56e596d37f6e816642a8ddc18c9db855ea033d14c9ae3bdda8` / 524,288 |
| Build source fingerprint | `2e8ad18dddf40f7f54bd0a2ab1c2da49c6b00ab8646a0362e68ecb0d26964ff7` |
| Plugin project revision | `f9c648669ddf0df7ec2d34550dad083dd613411ef3f3be4c08aad358973761c1` |
| Matching NOI SHA-256 | `0ac98492d2642cb1e9acbae77b7743c62b22f089eca790ec40da55fc5b2f808b` |
| Original recording / session | `aac78988-5a72-40a6-a9b7-dabec9eb07a9` / `d2105ffa1a8c407cbed2689284c298cd` |
| Frames / events | 6,612 / 465 |
| Journal digest | `da12553bb31a3c0ec814fcc84444afbb08e9f5f07dba8762627173c331358102` |
| Archive / manifest SHA-256 | `e839c8e1-5649-4b39-8f23-dd5668fc4206` / `0d2e0491329ea0e55bb47f4c3ec086e946d0e0962cc3ad024a4ed1f8080fb90d` |

The local candidate bytes and ROM digest were independently read after the native recording. Ordinary buttons showed horizontal tram approach/open doors and terminal vertical poses at frames 4,136–4,226. Queen Yonge 46 → Parkdale 43 boarded at world second 48 with cash 30→27 and arrived by second 60. Parkdale 43 → Alton 50 then boarded at second 64 with cash 27→24, crossed the actual West/Core/East scenes and alighted at East `(880,524)` at frame 6,612 / world second 92. Each fare was charged once. Native bound inspection confirmed the horizontal 28×12-pixel Q5 body. The paid map sample froze clock, fare and remaining ride, browsed the East objective and restored the visible tram after cancellation.

A bounded paid-ride core sample at world second 74 advanced **45 game updates over 120 VBlanks**, approximately **22.5 updates per second**. This measures that scene/phase, not the whole world or worst case. Performance optimization is pending. The recording was stopped with `needs-review` and archived; these rendering/travel samples do not establish an accepted final moving-streetcar milestone, all poses/conditions, collision recovery, paid cold reset, crowded-scene performance or handheld feel. The earlier PLAYER/actor-size hypotheses are withdrawn: compiled horizontal-frame loss was identified, while the separate Q5 bounds correction was tested. No browser restart or fresh browser result is claimed.

[check_streetcar_rom.py](../../scripts/check_streetcar_rom.py) separately passes the official matching `b1cf9a37…` ROM/NOI pair: eight four-object cardinal/door frames plus empty frame 8. Against retained matching `deb78bbd…`, it exits 1 with all eight poses failing: horizontal frames empty and vertical frames only two objects. The gate prints hashes and parses bounded banked pointers/8×16 cells; it does not authenticate source provenance or prove playback/hardware. [BUILD.md](docs/BUILD.md) gives the reproduction command. Native pixel/OAM observations above remain distinct evidence.

### Retained performance intermediate: slower matched native sample

Candidate `84e5fe20…` passes full `make check` with **418,184 engine checks**, the other suite counts above unchanged, the compiled-frame gate and the actual `D968` heap / `DF00` stack guard (**1,432-byte reserve**). Its fresh-boot ordinary-button replay repeats both paid Queen trips and East `(880,524)` alighting at frame 6,612 with cash 24. The paid map at frames 4,448→4,828 preserves **all 58 saved bytes exactly**; tram restoration is observed at 4,872.

Its matched Core sample starts at frame 5,472, world second 74/subsecond 38. The `game_time` counter advances `9A→BD` by frame 5,592: **35 updates over 120 VBlanks**, approximately **17.5 updates/second**, slower than `b1cf9a37…`'s 45 / 22.5 at this phase. This candidate fails the performance comparison despite the sampled functional results. No cause or final optimization result is inferred; a later source fix needs its own build/check/replay.

| Intermediate identity | Value |
| --- | --- |
| ROM SHA-256 | `84e5fe205887586efbc27ddc9b86d4c2ef7cabb8e1921c8fcdd4851663728b0d` |
| NOI SHA-256 | `009d7e24f108f9c8dfe4564039baad26f8a0e7ce9d63625248ffe50e5c38d5b9` |
| Build source fingerprint | `9415894fd5d499de61fbce20f5a6068f7ab31a226a67baecfe7932827d2a85c7` |
| Plugin project revision | `f9c648669ddf0df7ec2d34550dad083dd613411ef3f3be4c08aad358973761c1` |
| Original recording / session | `96121777-3434-48ea-bbb6-588d66e7d841` / `2880749615304c6bb6286a8650847402` |
| Frames / events | 6,612 / 481 |
| Journal digest | `8c5023ae0d40719015f6565ae1bfb5661cb3c177845c3c282fde3ea05613a603` |
| Archive / manifest SHA-256 | `9f2fcfaa-c353-4d2f-a095-d8d39c1c5a7e` / `2732900a3285fd07ee996ac0841359124a771e69faed9f45f72c36b8411b828b` |

The recording is stopped as `needs-review` and archived. This intermediate is not recommended for loading and establishes no browser, hardware, full-campaign or enjoyment result.

### Retained phase-math intermediate: prior pace restored

Candidate `815ef0b0…` derives section/tick directly from world seconds and uses additive range selection; the preceding batching remains unchanged. Full `make check` passes with 418,184 engine checks and unchanged other-suite counts; the 1,432-byte linked guard and compiled-frame gate pass. Its matched Core frame 5,472 / world 74 / subsecond 44 sample advances `game_time A6→D3` by frame 5,592: **45 updates / 120 VBlanks, about 22.5/s**. This restores `b1cf9a37…`'s pace after the slower `84e5fe20…`; it is not final performance acceptance.

The same two paid rides repeat, with all 58 saved bytes frozen during map frames 4,448→4,828 and tram restoration at 4,872. A third trip, Alton 50→Yonge 46, boards at world 96 with cash 24→21 and 16 seconds remaining. A paused soft reset begins at frame 6,856; HELP at 7,156 restores the boarding checkpoint at 96/subsecond 1, cash 21, ride_left 16 in actual East. Resuming reaches Core `(676,556)` at 8,214 without another fare. Ordinary walking/car entry finishes at 8,822 in Core `(560,720)`, driving, condition 100.

| Intermediate identity | Value |
| --- | --- |
| ROM SHA-256 | `815ef0b065c375a43aa294e611ba67ee94801cc17297125b15f57ef4cf3096fb` |
| NOI SHA-256 | `ba75a4ca62cfa146f22096665d6536f25eacce17604299955ba9beb00de0c5b7` |
| Build source fingerprint | `879fd7c66ad153b5b0d6a066ec28287470137a61cc071e315725a23e98b3321c` |
| Plugin project revision | `f9c648669ddf0df7ec2d34550dad083dd613411ef3f3be4c08aad358973761c1` |
| Original recording / session | `7456229a-beed-4092-a296-c301edb6ca3d` / `4b446426ed6c42d6b885017720c86f1b` |
| Frames / events | 8,822 / 571 |
| Journal digest | `7ff75652ecc4ffec2cce25467fd3c653c1c53dfa64ef521275f63231a221f3d3` |
| Archive / manifest SHA-256 | `dbee9598-6597-4cfa-81d5-832fc79ca33d` / `210c949d2ed9db76bc8996fdec63e60cc0415f4c3a12530d8094c856b20668a9` |

The recording is stopped as `needs-review` and archived. These paid-reset/car-recovery samples and restored update pace leave final optimization/replay pending. No loading recommendation, browser result, hardware result or full-game acceptance follows from this intermediate.

Published Prototype 6 remains the separate `23b2a7a2…` timetable/safe-alighting ROM recorded below. The pickup-condition `64be19fa…` contract-81 journey also retains its own identity; neither proves moving-tram rendering. The current browser lifecycle remains unresolved. No fresh browser result, physical cartridge operation, full former-Toronto coverage, complete campaign or measured two-hour enjoyment is claimed for this candidate.

## Courier contract with scheduled transit — native candidate, 2026-10-02

Ordinary buttons on the pickup-condition ROM `64be19fa3da7ba4231ba8c8decec4720c409116c034f586974fd4ddce789945a` completed contracts01,02 and03 on foot, unlocked contract 81 **EAST FIRST ROUND**, collected at Union, paid for Queen Yonge→Saulter, delivered at Riverside Queen and Danforth Hall, returned on foot through the Bloor seam and recovered the car at Union. This is the earlier timetable/text ride implementation; it does not establish the later moving-streetcar source or assets.

- Acceptance/pickup: frame4,730, three distinct completions, cash401, condition 100; route `[0,37,35,0]`,205-second allowance.
- Queen choice: frame5,316, eastbound8 seconds/$3, countdown60. The courier reached the platform just after its previous departure; nearly60 seconds of optional waiting made this particular transit leg slower than walking. This is a tuning finding, not evidence of player enjoyment.
- Boarding: frame 8,938, clock140, cash398, ride_left8, contract stage 1,135 seconds left. Arrival at frame9,478 is `(128,556)` in East, clock149, stage 1,126 seconds left; the car remains `(560,720)` in Central. No second fare is charged.
- Riverside and Danforth stopped handoffs reach stage3 at frame10,726. Direct walking through the Danforth/Bloor seam returns to Union; frame13,198 shows RESULT, cash598, done4, condition 100 and64 seconds left. Contract81 consumed141 world-clock seconds including the long optional wait; reward is 188 plus12 remaining-time dollars. The completion bitmap sets contract 81's actual bit.
- Car entry finishes at frame13,258, `(560,720)`, driving, clock212, cash598; the parked car identity and position are preserved throughout the journey.

Recording session `77b08afcb57e4e0ca2e3fe4f757cd968` contains 13,258 frames/2,123 events, journal digest `4345f21a8c7addc6b4f102dc5630a88e8ef0d8a4e36b1f7c94e2c285677b6c13`. Its worker was closed and original bytes reversibly archived under `f1df3dec-5a4c-421d-bf59-8cb1c8899b05`; manifest SHA `a6ed0404cd7d7016f2e10ad2d695a4e78bdce73b4c50c6ab0b309f71465823a0`. Read-only bounded WRAM inspections use this exact ROM's linked state atC1F6; no save/progression injection or browser input was used. This one route does not validate all campaign types, a measured two-hour campaign, physical hardware, or a same-phase car-versus-transit comparison.

## Pickup condition lifecycle correction — native candidate, 2026-10-02

Accepting a contract now leaves cargo/comfort at 100 until the first actual pickup. The four traffic, wall, curb and passenger-steering damage paths require an active carrying stage; collision motion, cooldowns, fines and the acceptance-time deadline are unchanged. Empty vehicles show `CRASH: BRAKE EARLY`, and an unoccupied passenger approach no longer shows a rider warning. Cold startup restores valid older active-stage-0 saves to 100 only after CRC and semantic validation; actual carried damage and no-job failure condition remain unchanged. The 58-byte version-6 save format is retained.

Official candidate `project/build/toronto-pickup-condition.gbc` is **524,288 bytes**, SHA-256 **`64be19fa3da7ba4231ba8c8decec4720c409116c034f586974fd4ddce789945a`**. Source fingerprint `7a61baf9dfa688dd423be21b8444380abc1969481ee8a00286f15cae55c6ebf3`, matching NOI SHA `9e82436532668b27e9b68059dce8c9cb8696ddc06bb1a5e4b2cb854812843f18`. The official build exited 0 in 39,175 ms, header inspection passes, and actual linked allocation retains `D950` heap / `DF00` stack / **1,456 bytes** reserve. This candidate is newer than the published Prototype 6 bundle; that bundle does not include this correction.

The actual-C harness first reports **3,058 checks / 14 failures** on the old engine. After correction it reports **3,058 checks / zero failures**. Cases accept actual authored parcel/fragile/passenger offers and collect through the real stopped interaction handler. They independently compare approach/carrying/no-job traffic, wall, curb and steering responses; stage-0 timeout still fails without pay. Genuine CRC-valid older saves cover core and remote recovery, preserved carried/no-job condition and earnings/clock/vehicle positions; active health 0/101 remains rejected before normalization. Full `make check` also passes the unchanged transit/atlas/bridge/navigation/memory suites. Host fixtures do not establish native rendering, CPU timing or physical persistence.

Three ordinary-button native recordings preserve the defect and its replay; WRAM/OAM inspection was read-only, without injected progress. Each journal was stopped, its worker closed and the original bytes archived through the plugin.

| Scope / exact ROM | Session | Frames / events | Journal digest | Archive ID |
| --- | --- | --- | --- | --- |
| Pre-pickup defect / published `23b2a7a2…` | `649fc667c0ae440e89baa65d837a9a09` | 998 / 262 | `b5f7cfbe52b294cbfddc58a5d212ab6cd92bb0fae9b979f11ba0f0b6c1645355` | `3e2e3089-fe58-4c77-be99-6f2579d767b7` |
| Corrected lifecycle / `64be19fa…` | `e04b29901a75452cb97538ef27ea5900` | 3,696 / 1,014 | `a6e1459d3e26f22e56a85628104252a3f3710f74086f3ec146b3b80e6608bb55` | `11710051-0d5d-4a7d-981b-0835950972e4` |
| Controlled delivery/turn / `64be19fa…` | `1bbb466a76bc4003a3700ef5e1d4e538` | 864 / 260 | `946c87cbfd42f1a310bff7b6da4568cd148acf40d2864f000056684997e3feb4` | `310d1669-7a6f-47f6-ada8-1c36e8b9d304` |

- The defect/replay use clean boot 180, A2, neutral300, Select4, neutral4, A4, neutral4, A500. Both reach frame998 at `(842.9375,720)`, contract 01 stage0, 112 seconds left; the old ROM shows condition92, the correction100. The corrected obstacle warning is generic while the vehicle still stops.
- Ordinary reverse/return reaches an actual Union pickup at frame2,616 with condition 100/stage 1. Another obstacle pass reaches frame3,116 at condition92/stage 1. Braking/reverse and a stopped Market handoff finish at frame3,420 with cash123/done1: the condition-scaled base 79 plus remaining-time bonus14 gives the expected93 reward.
- A separate clean boot repeats the original first-delivery/held-turn inputs and world phase: pickup502, first delivery740 with cash139/done1/condition 100, then B4, neutral4, A40, A+right48, A24 reach864 at speed24/heading4 in the actual core scene. OAM at864 has four visible objects, peak four per scanline, zero over-limit lines.
- The longer lifecycle recording also retains an exploratory B-dismiss that continued into reverse and caused a correct stop-to-park refusal at speed-4. A later turn at a different world phase encounters a visible pedestrian and slows; it is not the controlled handling regression. The separate matched-phase replay above is the handling evidence.

Native coverage here is parcel lifecycle, reward and controlled driving. Passenger/fragile penalties and older damaged-stage-0 recovery have host coverage but still need native samples. The later ordinary-button contract 81 journey is recorded above. Full two-hour varied gameplay, the current browser preview, full former-Toronto coverage and physical cartridge acceptance remain pending.

## Final Queen scheduled service — sampled native acceptance, 2026-10-02

Published Prototype 6 source implements eight supplemental 501 Queen curb platforms across West, Central and East. The campaign contains 51 service points while retaining the original 43 records, all 88 contracts and the 58-byte version-6 save layout. Original signs contain no TTC logo. Service 4 uses a three-dollar game fare, a 64-second directional period, two-second boarding windows and four seconds per stop interval. Destination selection derives east/west direction; schedule phase comes from the existing world clock. [STREETCAR.md](docs/STREETCAR.md) records the researched identities and deliberate normal-corridor compression. Current construction detours, full 501/504 coverage and an adopted map era are outside this implementation.

Final official output `project/build/toronto-queen-streetcar-safe.gbc`: **524,288 bytes**, SHA-256 **`23b2a7a25c9c593a51967e16a275cfb162bbb3e59f709eecd37dd77e2bb408f0`**. The local file size and SHA were independently read after the native tests. Source fingerprint `15ef9fbe8c74d6b1603d298fb4f0ec3d899bd279c4e220f65eb3ce8481a4873b`; matching NOI SHA `36ec47e25446b3959c9746c27a46222361150095a9ef87bd7a565c3ff51cbac5`. Official build/inspection and the following scoped native checks passed. Prototype 6 release publication is tracked separately through bundle/release metadata. [BUILD.md](docs/BUILD.md) owns the final toolchain/allocation record.

Final `make check` passes **2,857 actual-engine checks**, **609,452 transit API checks**, **299,366 atlas API checks**, **8,036,093 atlas renderer checks**, **2,974 bridge checks**, **16,997 navigation/math checks**, nine memory fixtures and repository/generated-resource checks. The new [banked transit module tests](../../scripts/test_transit.py) use an independent oracle under ASan/UBSan and `-Werror`. They exercise service membership and invalid encodings, route/menu bounds, self-target rejection, fares/durations, labels and unchanged failed outputs, every valid origin/target phase, both Queen directions, clock edges and complete 16-bit clock sweeps for representative services. Engine fixtures cover paid remote arrival/reset/retry, retained parked-car districts, pause/deadline behaviour and corrected alighting. Host tests adapt native types/bank annotations; they do not prove native bank ABI, linked allocation, raster timing or cartridge behaviour. Those have separate build/native evidence, still bounded by the scenarios below.

### Exact final native journeys and safe alighting

Both final recordings used ordinary buttons with source/WRAM/OAM inspected read-only. No save/progression was injected. Journals were stopped, owned workers closed and bytes reversibly archived through the official plugin.

| Recording scope | Session | Frames / events | Journal digest | Archive ID |
| --- | --- | --- | --- | --- |
| Final Queen journeys, driving/map/reset and car recovery | `22b00640f39d46dc95d5a875c1f579a9` | 10,848 / 5,732 | `dcd342feb7872634cebb0796ef8bd29f3212c98fa6f1734b95c5528e17373a9e` | `b02ee327-f7e9-44a4-8cac-0737b199b5f8` |
| Final legacy train/bus/ferry and clear Union alighting | `f2330a06a474449f8b577a38c41567f0` | 14,214 / 7,546 | `0ce31916545301b7dc08514e3495cf0c6d519136a3b97249e31cf3fbc541b054` | `81e6fa91-1bde-499a-898d-2513abceea07` |

- First delivery completes at frame 740. The exact held-acceleration turn reaches frame 864 at speed 24, preserving the previous handling regression.
- Queen Yonge→Alton arrives in the actual East scene at frame 7,092, Alton→Parkdale in West at 9,084 and Parkdale→Yonge in Central at 10,236. Each ride charges once: cash `139→136→133→130`. The parked car stays in Central through all three journeys; A completes entry at 10,364. Free-roam guidance returns to Union depot.
- Captured state hex is identical across map frames `2,082→2,322` in WAIT and `5,524→5,764` in paid RIDE. The booked stop remains the map objective. A normal soft reset during the paid journey restores HELP at 6,064 with world clock 76, cash 136 and 16 ride seconds left. Resuming reaches the booked destination without another fare.
- The separate legacy sample verifies train map/reset, a 94 Wellesley bus journey arriving at Castle Frank at frame 7,280 with cash 19, and a Centre Island ferry round trip. Island arrival is frame 11,230 at `(720,920)`, cash 12, followed by walking to `(732.5,928.5)`. Mainland return is frame 13,788 at `(640,784)`, cash 8.
- Corrected Union arrival is frame 8,495 at `(572,720)`, cash 16 and world clock 103, beside the parked car at `(560,720)`. Ordinary right input reaches `(639.5,720)` at 8,634, resolving the predecessor's trapped arrival. The later car return/entry finishes at 14,094 at `(560,720)`, world clock 195, cash 8.

Bounded stationary samples advanced the update counter by 59 over 120 video frames, about 29.5 updates per second: main frames `10,728→10,848` (`148→207`) and legacy `14,094→14,214` (`120→179`). Each preceding OAM snapshot showed eight visible objects, peak four per scanline and zero over-limit scanlines. These are native emulator observations in sampled scenes, not crowded-world worst-case performance or physical display proof.

### Published bundle verification

[Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) was published at 2026-10-02 15:40:50 UTC after PR 8 passed real push and pull-request `make check` jobs plus the security check. PR 8 merged source commit `e85006f07d07ff628a1aad0a45dd9dc6271e4695` into main at merge `114d787f0d62be38d47b047e18822353435f24b6`; main's actual `make check` job also passed. The release tag and bundle declare that exact source commit. This later documentation update changes no ROM source or native asset.

The published ZIP `toronto-dispatch-v0.2.0-prototype.6.zip` is 85,940 bytes, SHA-256 `ecfee71c28227ee8d48d3d841af76111139af97ce5e52be73cab92d867137c87`. GitHub release ID `401945463` and asset ID `605921047` report a published prerelease and uploaded asset with that size/digest. An anonymous public download at 15:42:22 UTC independently matches the ZIP, all member checksums, the 524,288-byte native ROM `23b2a7a2…`, declared source commit, MIT licence and City attribution notices. Packaging, server metadata, anonymous download, native execution and physical cartridge acceptance remain separate evidence. No hardware write, read-back or cold boot is claimed.

### Earlier native Queen sample and retained failure

The unpublished predecessor ROM was **524,288 bytes**, SHA-256 **`f56ff75e7e43ed9050f2d1da1247c0c6d9882d606f053e99dbfd257ebea530ea`**. Ordinary-button recordings were retained and archived through the plugin; no save/progression injection was used. The three Queen journeys and the later legacy failure both belong to this exact predecessor, before safe alighting was corrected.

| Recording scope | Session | Frames / events | Journal digest | Archive ID |
| --- | --- | --- | --- | --- |
| Three Queen rides, map/reset and car recovery | `26d2a0be2816450e9e6e2f3a1a1afd04` | 10,728 / 5,696 | `5bebf5449bf039a5925be548f8b623ec61e60fd92e3fb6fc650c8499f4cabc2a` | `84390656-fc17-48e2-adce-d39fa8c57813` |
| Legacy bus/Line 1 arrival defect; needs review | `820ac93d3aae40b3ab52ea9be81bf6c6` | 9,010 / 4,846 | `dbab6c8dc4207d0915d9db61295674f5bceca8a3f7c211684ca8a674b57f7089` | `0f95d7f4-c724-4f7d-b87a-c1b731982a6c` |

- The first delivery and exact held-turn sequence reached frame 864 at speed 24, heading 4. This repeats the handling regression without asserting a broader performance result.
- Yonge→Alton travelled Central→East with a paid map/pause and reset; the car remained in Central. Alton→Parkdale travelled East→West, then Parkdale→Yonge returned West→Central and recovered the parked car. Each journey charged three game dollars once, taking cash from 139 to 130 across the three fares. Native source/state inspection confirmed the destination scenes and car retention. This samples three journeys rather than every platform, timetable edge or failure case.
- Separate legacy transit arrived at Union `(560,720)` on top of the player's parked car at frame 8,498. Holding right remained blocked through frame 9,010. That recording is a failure requiring review, not successful legacy acceptance. The earlier Queen sample did not expose this condition because its new platforms are on sidewalks away from that car.

### Safe-alighting correction and remaining acceptance

Current source keeps the destination centre if clear, otherwise checks cardinal offsets of 12 then 18 pixels. It requires bounds, destination-scene walking permission, a sampled connected path, exclusion of the player's parked car and clearance from traffic in the loaded scene. It does not compare a remote destination against the origin scene's traffic cache. If all candidates are blocked, the already-paid trip retains a retry second; deadlines and failure handling continue without another fare. Host fixtures cover these branches, including same-district and remote arrivals, blocked retries and time expiry.

The corrected final ROM separately passes the sampled ordinary-button journeys and alighting cases above. It does not exercise every Queen platform, both window edges at every origin, blocked-arrival/queue failure under native execution or every mission/deadline/condition combination. Those source branches retain host coverage and need further representative native play. Earlier published Prototype 3–5 results below keep their original scope. Moving streetcar artwork, the full former-Toronto map, measured two-hour enjoyable gameplay, crowded-scene/human assessment and physical cartridge boot/save/audio remain unverified.

## Final portable atlas build — 2026-10-02 (Prototype 5)

Final official output `project/build/toronto-city-atlas-portable.gbc`: **524,288 bytes**, SHA-256 **`2d1f6e4e7ae48a434757e63454d216b02b81957ecf5f8582d879149d447d7311`**. Source fingerprint `efe054a611bebeb91231f37db6102e71c1c305f2f861d09010491a4340f9aea4`; project revision, compiler, NOI `ad657f…` and globals `930e45…` match the identities below. The official build, valid CGB/MBC5+RUMBLE+RAM+BATTERY/32KiB SRAM inspection and actual-ROM memory guard pass; reserve remains1,456 bytes.

GitHub's Linux GCC check rejected a generated `for` and following `return` on the same line as misleading indentation. The generator now places the return on its own line; no warning was disabled. Full `make check` passes with the same API/renderer/engine/bridge/navigation counts below. This additional build changes exactly five ROM bytes: the stock `_save_signature` at0273–0276 and the low global-checksum byte at014F. Both global checksums independently validate. All other524,283 ROM bytes, NOI and globals are identical to `e812f7ef…`; the precise serialized compiler-input difference behind the stock signature is not reconstructed. Toronto's custom save does not reference that stock signature. New byte identity still required its own native acceptance.

Two fresh source-debug recordings repeated all251 driving/map and153 transit ordinary-button steps from the optimized candidate, including the normal soft reset. The first-job completion at740, held-turn full speed at864 and car re-entry at5,066 match. Full58-byte state at map980/4,228 and active-job5,090/5,410 matches the preceding captured outputs exactly. All thirteen captured transit checkpoints match, including fare/cash, closed-window WAIT, map pause, paid ride, reset HELP and King arrival at4,466 without another fare. No memory/progress was injected. Frame1,048 is an unmodified framebuffer in [screenshots/provenance.json](docs/screenshots/provenance.json).

| Final recording | Frames / events | Journal digest | Archive ID |
| --- | --- | --- | --- |
| `atlas-portable-driving-map-20261002` |5,434 /690|`bce3fd09faaa0df59e6321ac491b4cd1b14bbfe6387b3e034a5dc644491e918d`|`6d165799-0e71-45cb-8824-f2f602dfadee`|
| `atlas-portable-transit-20261002` |4,466 /472|`3863567274eb6fc36515dd79ca6f7fc8dc3e38cda26dc35542ca97fa35e1c259`|`3cfa3ba1-3cc0-4857-8986-3da1142671a5`|

Recordings were finalized, owned workers closed and bytes reversibly archived. The detailed optimized-candidate observations below remain scoped to their original `e812f7ef…` ROM; the final rebuild separately repeats those input scenarios and state checkpoints. Timing/OAM readings below were taken on that predecessor. Neither binary establishes full Old Toronto, two hours of varied play, human review or physical cartridge acceptance.


## Optimized city atlas candidate — 2026-10-02

Final official native ROM `project/build/toronto-city-atlas.gbc`: **524,288 bytes**, SHA-256 **`e812f7ef3bee91e13e8ee0551c936eeed74283c60cb8c497ed45518d7b15d128`**. CGB-only, MBC5+RUMBLE+RAM+BATTERY, 32 KiB SRAM; CLI build/header inspection passed. Source fingerprint `bbff1b78d37e3abf900a1b082d70bb33af235ef882228cccca2b5c6129ea5cde`; project revision `375cff6b012a8acd6bc0fcf11fbd22fb49b9cdb085179063ecdd4929e875bec4`; matching NOI `ad657f05786ee9230aa413bb335d2e65f7b93a91aada696693d25c65135d3a4a`, globals `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a`. Compiler warnings remain DEP0190, five TORONTO optimizer warnings and two upstream SFX unreachable-code warnings; no error.

The paused map now spans all four registered areas in their geographic order, using a 512×122 collision-derived schematic and a160×96 viewport. It marks the courier, vehicle and job, booked transit stop or Union depot. It changes no city art, collision, client, route, job, fare or version-6 save field. [CITY_MAP.md](docs/CITY_MAP.md) specifies controls, geometry and budgets. Its source-status metadata deliberately does not claim native execution; evidence is recorded here by actual ROM.

Full `make check` passes **299,366 atlas API checks**, **8,036,093 actual-renderer checks**, **2,404 engine checks**, **2,974 bridge checks**, **16,997 navigation/math checks**, nine memory fixtures and all generated-resource/repository checks. API/renderer/engine/bridge fixtures use ASan/UBSan. Renderer fixtures validate decoded VRAM for all225 legal viewports, all marker overlaps, remote parked-car and road-anchor/foot-client focus, free WAIT/RIDE booked-stop focus, partial cancellation, exact camera/hidden-bit restoration and failed getters. Sparse-cache fixtures force collision chains through every possible single vacant slot and full tables; they verify termination, no eviction and correct upload. Host adapters do not establish LCDC addressing, actual bank ABI, raster timing or hardware behaviour.

The exact binary's memory guard passes: heapD950 / stackDF00 / **1,456-byte reserve**, with28 new transient UI bytes and unchanged360-byte cache. Fixed-bank occupied end3F81 leaves127 bytes. Atlas code/data in bank0F occupies10,100 bytes (648 API code +9,452 data); its whole shared bank leaves6 bytes. UI bank11 hexadecimal leaves7 bytes. Bank packing needs reinspection after changes; these reserves are not deepest-stack measurements. Exact binary BG/sprite/font decoding confirms disjoint bank-1 addresses: sprites8000–837F, gameplay BG9000–907F, markers9080–90EF, atlas8800–8BBF /9100–97FF and font8C00–8F0F.

Native checks used PyBoy2.7.0 CGB mode and worker `6271cbbb9ca76d4d149173f4110705cfa3c567349f0cb7ceb347bf1df7598925`. Inputs were ordinary held-button sets; source scenes, WRAM and OAM were inspected read-only. No state/progress was injected. Recordings were stopped, owned workers closed and unchanged bytes reversibly archived through the official plugin.

| Recording | Frames / events | Final journal digest | Archive ID |
| --- | --- | --- | --- |
| `atlas-final-driving-map-20261002` |5,434 /690|`f1ddccf5e5f87813d068b57a99ba5a534ad456b3250112a9c98505d998bf38ed`|`eea14575-c66e-42dc-ae5d-9b0b03e6cef2`|
| `atlas-final-transit-20261002` |4,466 /472|`f6642524fcdbcdedebdaacc9607b82a432957d16b9aa2469af37482dc25e530f`|`dcc747d1-9578-4a23-8e98-96027fb388bc`|

### Driving and map acceptance

The final ROM repeated the original98 ordinary input steps through the former stopping corner. Market delivery finished at frame740, condition 100, cash139/count1. Holding A40, A+right48 and A24 reached frame 864 at(798.75,774.25), speed24/heading4. Acceleration and movement continued through the turn. Later parking, walking and entry returned to driving at frame5,066 with the car at(798.75,800.5625).

The atlas rendered Central Toronto, High Park/Junction, Toronto East End and West End; the genuine source debugger confirmed the gameplay scene remained core. Frames980→4,228 preserved all58 bytes of td state while browsing, including position, cash139, world11 and subsecond32. Hardware OAM had zero visible sprites under the map. After a partial Select repaint, B returned to pause and B to the street. Camera bytes `c0634066` were restored; camera settings returned0→3. Start also closed the map; opposite directional pairs kept the viewport still. Ordinary foot play verified You/Parked Vehicle/Depot focus. Accepting contract 02 then browsing retained position, cash, health, world15/subsecond11 and deadline120 across frames5,090→5,410, apart from the expected mode change.

A completed-view focus sample advanced the UBYTE update counterF1→29, **56 updates over68 video frames**, with the map fully painted at the end. It is a bounded mixed redraw/steady sample, not a worst-case rate. The172-slot bounded double hash averages2.37 source-table reads per query over the225 authored viewports; this analytical count is separate from native timing. Full native coverage of every viewport, remote parked-car focus and all three parking/client marker changes remains pending; host fixtures cover those cases.

### Paid transit, waiting and reset from the map

- Frame314 showed an open Union Line1 departure window at world1/cash30. A8 boarded immediately, leaving cash27 and ride_left1 at322. The map showed **O STOP / TRIP: KING STATION**, froze world1/ride1 while open, then resumed. By806 the courier was on foot at King(640,640), cash27/world3, with the car parked at Union(560,720).
- A second King→Union ride charged27→24. Map frames1,170→1,410 preserved all58 td bytes, including world3/subsecond30/ride1 and cash24. The map labelled the booked Union stop. Resuming completed that trip without another fare.
- At world20, closed-window Union→King confirmation entered WAIT without charging: cash24. Map frames2,826→2,986 preserved all58 td bytes, including world20/subsecond48. Resuming advanced the actual schedule to world35, then boarded at36, charging once to21 and setting ride1.
- During that paid ride, opening the map and pressing the normal A+B+Start+Select reset restored HELP at4,338 with world36/cash21/ride1, foot player and parked car still in core. Source debugging confirmed the actual core scene. A resumed the saved ride; frame4,466 arrived at King, world38/cash21. There was no duplicate fare. This is in-worker SRAM soft-reset evidence; physical cold boot and persistence across emulator processes are unverified.

A stationary Union sample after the second trip advanced `_game_time`CD→07 modulo256, **58 updates/120 video frames** (~29/s). Its preceding OAM snapshot had13 visible objects, peak6/scanline and no over-limit lines. This is a bounded core sample, not crowded-world performance.

### Intermediate and remaining scope

Unpublished `toronto-atlas.gbc`, SHA`ec982d0c90307f3433d27704ee8fa6fee1cf8c9211ef22dbc59877cfb07e71e2`, established initial all-four map rendering, frozen state and cancellation, but exposed slow linear-cache lookup. Its9,588-frame/520-event journal digest is `989cec4029b2faaa713c2e0204cdabc4c54059a8eb45167852f6887fa8ccc8a1`, archive`91a4b86a-929c-435a-a9e9-bcbba1a5a80e`. It is superseded by the optimized binary above. Its later held-turn sample encountered traffic/curb contact and did not reproduce the exact regression sequence; that pass comes from the final recording.

Prototype4's nine-job/four-loaded-scene progression below remains evidence for its separate ROM. The new ROM repeats first-job driving and adds map/transit/reset acceptance; it does not replay those nine quests or load all four gameplay scenes. Full former Toronto/waterfront/fuller Islands, remaining routes/seam endpoints, at least two measured hours of varied enjoyable gameplay, human handling/listening, crowded-scene/deepest-stack checks and physical cartridge verification remain open. The older browser is unrefreshed because its recording-close acknowledgement is UNKNOWN; native tests do not update or validate it.


## Four districts and parking guidance — 2026-10-02 (Prototype 4)

Final official native ROM `project/build/toronto-four-districts.gbc`: **524,288 bytes**, SHA-256 **`1da71ba549aaf6b0b1fc641d4f4f9e0e317550e396bf80ffaf401f6ff7e82b2b`**. CGB-only, MBC5+RUMBLE+RAM+BATTERY, 32 KiB SRAM; logo/header inspection and official CLI build passed. Source fingerprint `e029dac64f995a5466df744fad68ebbf4e14ef28b9d5d0df944ff0deed44a5ee`; project revision `375cff6b012a8acd6bc0fcf11fbd22fb49b9cdb085179063ecdd4929e875bec4`; matching NOI `24a627853b5d39766d336901b52ad871ad022b7c767afe6b14e26cb3d670f289`, globals digest `930e459cba58eca33586d76ab1bd13f21fbe3decfcb004d9ecc121897b4d7c2a`. The subsequent planning-metadata regeneration did not alter the generated engine content; source-debug boot of this exact ROM still succeeded. Build warnings remain DEP0190, five TORONTO optimizer warnings and two upstream unreachable SFX warnings; no build error.

`make check` passes **2,352 actual-engine checks**, **2,974 independent bridge checks**, **16,997 navigation/math checks**, nine ROM-memory fixtures, and all repository/content/artwork/generator checks. Engine and bridge fixtures run under ASan/UBSan. Actual production content getters are linked into engine fixtures, including all three road parking anchors, absent/NULL lookup behavior, original record preservation and driving-versus-foot target changes. Four registered 128×122 scenes contain 88 contracts/43 stops, 211 buildings, 486 fixed pedestrian routes, 18 non-core traffic loops and 14 reciprocal seam pairs. These counts do not establish two hours of gameplay.

GitHub's Linux runner exposed platform-dependent PNG compression bytes in the eastern freshness check. Validation now compares decoded PNG mode, dimensions, pixels, palette and image metadata to the regenerated artwork, then retains the committed PNG's SHA-256 and still requires its registered copy/metadata to match exact bytes. Alternate compression passes; changed pixels, mode or dimensions fail. No native asset, engine source or tested ROM changed for this portability fix.

Native memory guard passes: heap `D934`, stack `DF00`, reserve **1,484 bytes**; eight VM contexts. Fixed-bank occupied end including initialization/startup is `3F81`, leaving 127 bytes. Banked world metadata/code and 82-byte BFS local frame were separately inspected on the preceding four-scene intermediate; that allocation is not a worst-case stack-depth proof. Current native samples below establish bounded timing and OAM observations only.

Both final recordings used PyBoy 2.7.0 CGB mode, worker `6271cbbb9ca76d4d149173f4110705cfa3c567349f0cb7ceb347bf1df7598925`. Inputs were ordinary held-button sets; coordinates, scene identity, clocks and OAM were inspected read-only. No progress/state was injected. Both journals were finalized, owned transports closed and bytes reversibly archived through the official plugin.

| Recording | Frames/events | Journal digest | Archive ID |
| --- | --- | --- | --- |
| `four-districts-parking-progression-20261002` | 31,480 / 11,894 | `cabb314d585e6451b33e93d03ef0f27b30373cd1d6ed158fa2b527249073a505` | `efe1b627-ab0f-437f-9bd6-cc06031fb38a` |
| `four-districts-final-transit-20261002` | 562 / 298 | `ed0f8b776016ad10af8116d8c1cdeed3d81f6acd62342b8cb1b856ae12788fac` | `fc614adf-9114-4968-8c48-53ade87b46bc` |

### Driving, nine distinct contracts and all four scenes

The first Market delivery completed at frame 740, condition 100, cash 139/count 1. Holding A40, A+right48 and A24 reached frame 864 at (798.75,774.25), speed 24/heading 4: the car retained acceleration and movement through the formerly stopping corner.

| Completed contract | Completion frame | Unique completed count / cash | Observed scope |
| --- | --- | --- | --- |
| 01, 02, 03 | 3,197 after the third | 3 / 404 | Core pickup and delivery progression |
| 81 | 6,909 | 4 / 622 | Queen car crossing to Riverside, Danforth handoff and return to core |
| 07 | 9,742 | 5 / 761 | Core progression/unlock |
| 04 | 10,576 | 6 / 917 | Truck contract |
| 82 | 16,532 | 7 / 1,099 | Fragile car route; Danforth entrance/Dundas return, condition 64 after traffic collisions |
| 83 | 22,242 | 8 / 1,120 | Motorcycle express route through Leslie/Ashbridge/Gerrard; completed under deadline, condition 4 after traffic collisions |
| 85 | 27,405 | 9 / 1,304 | Mixed route with Withrow park-and-walk delivery, motorcycle recovery and Union finish |

For contract 85, frame 23,879 showed the driving beacon at legal road anchor (224,144). Select near parked position (221.8125,140.8125) reported **PARK THEN WALK** without advancing the stage at frame 24,247. Exiting recomputed the actual client target (320,144). Ordinary walking reached (320.3125,144.8125); Select completed that leg at 24,581. Walking back and pressing A re-entered the motorcycle at 24,811. The remainder used Bain/Carlaw around the railway barrier, Dundas back into core and Union. The quest finished with condition 64 at world second 432. This covers a real foot-only handoff and return to the parked vehicle, not teleportation.

After those quests, ordinary driving entered the actual western scene and walking traversed its King/Queensway dogleg into actual High Park. The parked motorcycle remained in west at (861,640.875), player in High Park at (979.5,639.375), cash 1,304/count 9/world second 486. Save followed by A+B+Start+Select120 and neutral 180 restored the genuine High Park scene/HELP and those values at frame 31,272. This verifies in-worker cartridge-RAM soft reset; physical cold boot and persistence across separate emulator processes remain unverified.

The local High Park map panned during frames 31,320→31,480 while the player/world clock remained frozen. Actual camera WRAM changed **31,328→23,648**, Y remained 20,960. The journal's final `stopReason` mistakenly states 31,840→27,904; those numerals are an annotation error corrected here from the retained read-only camera output. The immutable journal was preserved. This is a scrollable district map; a browsable full-city atlas remains pending.

### Departure window and bounded performance

The fresh transit recording reproduced the same confirmation timing as the preceding regression. Frame 314 showed **DEPARTS IN 0 SEC**, world second 1/cash 30. A8 entered RIDE at frame 322 and deducted one three-credit fare. At frame 442 the player arrived on foot at King (640,640), ROAM/world second 3/cash 27; the car stayed at Union (560,720). There was no extra clock tick that caused the displayed departure to be missed.

During stationary King frames 442→562, `_game_time` was **248→51 modulo 256**, or **59 updates/120 video frames** (about 29.5 rendered updates/second). Final OAM sample: 12 visible hardware objects, peak 4/scanline, zero over-limit lines. The journal's final `stopReason` incorrectly states 88→148/60 updates/peak 6; actual read-only clock/OAM results above supersede that annotation. The retained journal is unchanged. This is one stationary core sample, not whole-city performance verification.

Nine distinct completed quests took 432 game-clock seconds through the last delivery; the full test with subsequent roaming reached 486 seconds. Tool latency, paused menus, contract counts and deadline budgets are not gameplay-duration evidence. Full former City of Toronto coverage, the remaining 79 contracts, all 28 native seam endpoints, a full-city atlas, at least two measured hours of varied enjoyable gameplay, human handling/audio assessment and physical cartridge boot/save/read-back remain open. The historical browser remains unrefreshed because its recording-close acknowledgement is UNKNOWN; these native tests do not update or validate that browser.

## Four-scene intermediate — 2026-10-02

Official native build `project/build/toronto-four-districts.gbc`: 524,288 bytes, SHA-256 `7a299125675b7e08aeb3ba939b2382b28597bfbae584ec2a5c4255ed5abf24f0`, CGB-only/MBC5+RUMBLE+RAM+BATTERY/32 KiB SRAM, valid inspected header. Source fingerprint `e8719774b958cd9114030e0d2bda83fee963c42e745505beb588968a6856cce4`; project revision `375cff6b012a8acd6bc0fcf11fbd22fb49b9cdb085179063ecdd4929e875bec4`; NOI digest `67affe33b3f214f6eaa6998c277b6186f4698ba1bfe325ac276523388b0d4dca`. It precedes the subsequent parking-approach guidance fix, which requires another build/test identity. This intermediate has not been published.

Source validation passed 2,273 engine, 2,974 bridge, 16,997 navigation and nine memory fixtures plus all content/artwork/generator checks. Four registered 128×122 scenes contain 88 contracts/43 stops, 211 buildings, 486 fixed pedestrian routes, 18 non-core traffic loops and 14 reciprocal seam pairs. East has 85 exact/63 flip-canonical tiles. Counts and route estimates do not measure gameplay duration.

Native allocation guard passed: heap `D934`, stack `DF00`, reserve 1,484 bytes. Fixed-bank occupied end including initialized payload/startup is `3F81`, leaving 127 bytes, unchanged from Prototype 3. World code/data occupy about 4,986 bytes in bank `0F`, including 1,554 bytes of metadata; no new long-arithmetic imports or persistent world-module WRAM. BFS local frame is 82 bytes. These observations do not establish deepest stack usage or cartridge performance.

All following inputs were ordinary buttons; state/scene/OAM inspection was read-only. PyBoy 2.7.0 CGB mode, worker `6271cbbb9ca76d4d149173f4110705cfa3c567349f0cb7ceb347bf1df7598925`. Recordings were finalized, owned emulator transports closed and bytes reversibly archived through the supported plugin; original-path mappings remain.

| Recording | Exact observed scope | Frames/events; final journal digest; archive ID |
| --- | --- | --- |
| `transit-window-before-20261002` on published `99eb430c…` | At frame 314, Line 1 selector/world second 1. Confirmation entered WAIT; frame 442 still WAIT/cash 30/world second 3/departure in 15 sec. Its stop reason incorrectly interpreted **NEXT STOP 2**, a destination ordinal, as a countdown. Read-only clock/phase and the later wait independently establish the bug. | 442/232; `351feaa6ee3d8b578c6405332083139e1a5202dfc8079dce8b75823ede1afd0e`; `af9d71d4-4715-40a4-b947-97ca5c4b49e2` |
| `four-districts-transit-20261002-v2` | Same sequence: frame 314 **DEPARTS IN 0 SEC**; frame 322 immediate RIDE/cash 27; frame 442 King (640,640), ROAM/on foot/world second 3. One fare; parked Union car retained. | 442/234; `9b411b1cc5546dca870d25150ee8da67534a4ae8a9899a3c3c86ca7625eab96a`; `dc8f32e1-d7e3-472a-89c9-d6e1ee137881` |
| `four-districts-driving-world-20261002` | First Market delivery: cash 139/count 1. A40, A+right48, A24 reached frame 864 at (798.75,774.25), speed 24/heading 4, continuing through the former stopping corner. Queen car core→East→core, then foot core→East with parked core car retained. Source debugger confirmed actual East scene. | 2,669/1,590; `cff77fe92ee215efe32b5baa273b8ad83ef07659b4e8b66524da330c67eba9e1`; `9d4f1eb9-8140-4357-912f-9e4a004e144e` |
| `four-districts-remote-reset-20261002` | A fresh isolated worker did not import the previous recording's cartridge RAM; that was not cross-process persistence evidence. A new ordinary foot route and explicit Save reached East player (57.5,536), parked Union car (560,720) in core, cash 30/world second 32. In-game A+B+Start+Select120 then neutral 180 restored actual East/HELP with these values retained at frame 2,546. | 2,546/1,366; `d4f211c70d4538ac3cedd268563cc04ab823da6d58b4d37cfc9aec3d4ab55c7f`; `258245d5-b850-41c6-9c29-dcfbecd2c942` |

East frame 2,573 had 12 visible OAM objects, peak 6/scanline, zero over-limit lines. `_game_time` advanced 59 updates over video frames 2,453→2,573: about 29.5 rendered updates/second in one stationary Queen Street sample. This does not establish whole-city pacing.

The graphics analyzer reported opaque priority attributes and unresolved palette slot 6/zero overrides. Zero overrides contradicts the seven authored palette IDs; independent same-build ROM decoding confirms East/core bind the complete palette, including slot 6 `E7DECC/B8A785/526879/172B38`, rendered in native East frames. Priority/flip bits remain. The analyzer discrepancy's cause is unproven; no valid palette was replaced.

Full former Toronto coverage, eastern contract/unlock playthrough, all seam endpoints, a full-city atlas, measured two-hour campaign duration, whole-city performance, human handling/audio assessment and physical cold-boot/save/read-back remain open. The historical browser remains unrefreshed because its recording-close acknowledgement is UNKNOWN.

## Linked western districts — 2026-10-02 (published Prototype 3)

Published Prototype 3 native ROM: `project/build/toronto-districts.gbc`, 262,144 bytes, CGB-only, valid MBC5+RUMBLE+RAM+BATTERY header with 32 KiB SRAM. Official plugin/GB Studio CLI build exited 0. SHA-256:

```
99eb430cc59cbb51631d343a4b626d07db03ff10ad36dd567128b438d36c528f
```

Build source fingerprint `93038e0d626669ee1d1b6809ebd68b492997c28a931300e3e59bf9e1eb207f08`; project revision `ca42f5fb8f7ef29e5c8c4dcfb18bf770e145257fe7acc99d54624db4d5d10f25`; matching NOI digest `2e8fca83ec54ad8517f1b6125710bc388b6952b8b9144c0b4e24133321e50526`. PyBoy 2.7.0 CGB mode, worker `6271cbbb9ca76d4d149173f4110705cfa3c567349f0cb7ceb347bf1df7598925`. The build emitted the known Node DEP0190 warning, five SDCC conditional-flow warnings in TORONTO.c and two upstream sfx_player.h unreachable-code warnings; no build error.

### Source and memory checks

`make check` passes 1,212 behavioral checks around unchanged production driving/save/route C, 710 independent district-bridge checks under ASan/UBSan, nine ROM-memory guard fixtures, and all repository/campaign/district/generator checks. The three actual 128×122 native scenes have 11 reciprocal seam pairs, 35 service points, 80 contracts, 12 new swept-clear traffic loops, 358 fixed pedestrian routes and 166 buildings. Source tile patterns are core185/136, west156/133, HighPark89/75 (exact/flip-canonical). These establish source consistency, not full geographical fidelity, all native endpoints or playtime.

Post-build `check_rom_memory.py --min-stack-reserve 1024` passes: heap ends at D90F, fixed stack/OAM boundary DF00, leaving 1,521 bytes. Project-local VM_MAX_CONTEXTS=8 is applied through the official compiler's file-backed engine field. Only six nearby pedestrian route starts (24 bytes) are cached in WRAM; the full tables are banked ROM. Save recovery uses one 58-byte candidate and expands legacy tails in place. This allocation margin does not measure maximum stack depth.

Two failed attempts are retained privately: `districts-first-native-20261002` / d9e5eeb0… reached kernel panic with heap DF90 overlapping the DF00 OAM page; `districts-native-fixed-20261002` /36119ebf… passed ordinary district travel but exposed corrupted remote soft-reset recovery with only345 bytes of stack reserve. Their final event digests are respectively `d6517b5d1b27fb207c117c2b5cb7877a76570b3e3b5e45343ce88aab09164d44` and `f1998e2f0ecf8d3a88186ec6eeb02ba0064c31ab591ed95df3a17fcb8c06eb7b`. Neither is a release build. A separately compiled intermediate22882ba1… was not native-tested.

### Exact current-ROM native recordings

All inputs were ordinary held-button sets. State, scene identity and OAM were inspected read-only; no progress or memory was injected. Both clean-boot workers and recordings were closed after testing.

- `districts-native-release-20261002`: 912 frames,96 events, digest `ddf3222aa18410d0a71acdcdacdf5cc2587a32d933e235d204d664d9355c38b7`. Market Start pickup/delivery completed at frame740, condition 100, unique completion1, cash30→139. Holding A+right for48 frames progressed from (777.6875,720), speed8, heading0 to (803.125,744.5625), speed21, heading4. Another24 frames of A reached (803.1875,776.0625), speed24. Steering retained acceleration and movement. Braking was then applied.
- `districts-crossings-final-20261002`: 7,366 frames,386 events, digest `e45e7b73ca4dd3da2c88075df7253db875e1de3a5f7d265d223d55f8e24db848`. Normal driving crossed core→west, walking crossed west→HighPark, returned HighPark→west, recovered/entered the parked car and drove west→core. Plugin source debugging confirmed each actual destination scene, rather than inferring scene changes from saved district IDs.
- At frame3576, HighPark walking position was (974,638.875), parked car (953.0625,628.375) remained in west; cash30/world second55. Soft reset with A+B+Start+Select120 frames, then neutral 180, restored HighPark and HELP at frame3876, with player/parked districts2/1 and cash/world clock retained. Bootstrap guard was0100 (live/pending).
- HighPark's local map scrolled left for400 frames and displayed HIGH PARK/JUNCTION. The world remained at second55 while MAP was open. A focused the route marker; B returned to pause/roam. This verifies the loaded district's scrollable map, not a browsable full-city atlas.
- Car-entry samples at6578→6610 showed the approach/entry and returned to driving with onfoot0 in west. At6824 the genuine core scene was loaded, with both player and car district0. A bounded OAM sample had8 visible hardware objects, peak4/scanline and zero over-limit lines; it does not establish crowded-scene performance throughout the city.
- A new Market Start contract was accepted before crossing back into west. At7066 the native state was job0/stage0/left118, cash30, world99/subsecond 45. The same reset/release sequence restored the actual west scene at7366 with those values unchanged, speed reset0 behind HELP. HUD guidance named CENTRAL TORONTO for the remote objective.

- `districts-transit-final-20261002`: 1,638 frames,123 events, digest `66fd9211937e7ee9f8da8f7a98b084ac6ddfcdfef5867b0336c6d474cf212593`. Ordinary parking/walking and route-selection inputs entered Line1 WAIT at Union. Pausing for180 frames froze the wait clock; after resuming, the autonomous second18 departure deducted3 credits once. At1638 the courier arrived at KING STATION (640,640), onfoot1, cash27, worldsecond19. No game state was injected. This is a core subway smoke, not verification of all bus/ferry services or new western routes.

Native smoke covers four directed seams and two remote saved-district restarts; it does not cover every one of22 directed portals, all80 contracts, v5 migration on physical hardware or a complete campaign. Existing core-only subway/bus/ferry and audio evidence below applies to its identified earlier ROM. The following current-ROM transit regression is separate from earlier audio PCM checks. Full former City of Toronto, at least two hours of varied enjoyable gameplay, human audio/handling assessment and physical cartridge boot/save/read-back remain unverified. Read-only USB discovery on2026-10-02 succeeded with zero devices and zero unmatched USB functions. No stream, flash, firmware action or hardware cold boot was attempted. Browser recording-close acknowledgement remains UNKNOWN; the retained historical browser was not refreshed.


## Wider roads, corner handling, world actors and audio — 2026-10-02

Latest native ROM: `project/build/toronto-dispatch.gbc`, 262,144 bytes, CGB-only, MBC5+RUMBLE+RAM+BATTERY, 32 KiB declared SRAM. The official GB Studio CLI build exited 0 and the plugin verified the logo/header. SHA-256:

```
a2f00db4ef834112a3491e50cec832653023a0456f0d9cbca6d2386be7322a59
```

Build source fingerprint: `439c22c598c9b82687ee3c8eb19560456948afd37e4184749dfe2e79397c9d33`. Project revision: `483aa222d1a1785bdfca3df6674e0232c13b2a0a544879ce76ced3f2bd315c3e`. Tool versions remain pinned as below. Upstream DEP0190, four SDCC optimizer warnings and two inline `sfx_player.h` unreachable-code warnings remain; compilation succeeded. BUILDINFO.json in a distribution bundle records its source commit.

### Host logic and authored assets

`make check` passes **471 real-engine host checks** under AddressSanitizer/UndefinedBehaviorSanitizer, plus repository/content and generated audio/route consistency checks. The hardware/SRAM adapters and limits described in the previous milestone still apply. New fixtures cover every tile overlapped by the car footprint, thin rail rejection, a swept 1–6-pixel corner adjustment, broad walls, boundaries, reverse/braking, opposing walking inputs, occupied car doors and traffic yielding while waiting on foot. The corner adjustment is limited to once per rendered update, including catch-up substeps. A seven-pixel clearance requirement is rejected.

Four checks failed against the pre-transition-fix runtime: simultaneous car-entry/transit inputs or paused-entry transit could suspend entry, and an old failed job could cause a new free-roaming transit arrival to reopen RESULT. Entry now excludes transit until it finishes; fresh no-job boarding clears the prior failure condition. A genuine deadline expiring during an already-paid trip still reaches its destination and shows failure. All 471 checks pass after correction.

Traffic motion is separate from sprite presentation. A 12,000-tick host fixture checks continuous vehicle loops, usable road coordinates and a bus step bounded to half a pixel per tick, including world-clock wrap. Pedestrian routes are fixed in the world, retain visible identities and are generated from actual native walkable cells. There are 102 route spans and six active pedestrian sprites. Host audio stubs establish cue dispatch and mode handling only; actual sound evidence is below.

Regenerated original artwork and collision retain all 80 building footprints. Roads are 48 pixels wide with 8-pixel sidewalks. Shadows and roof priority are clipped at asphalt edges. Plugin background analysis counted 185 exact / 136 flip-canonical patterns, below the CGB 384-pattern budget. The 128 × 122 map has 15,616 cells, within one 16 KiB array bank. The analyzer reports opaque tile-color attributes for native priority bytes; these are not a claim that roof priority or all sprite budgets were analyzed successfully. Sprite art remains unchanged.

### Exact-ROM native regression

`city-life-release-final-20261002` ended at frame **1,528**, **215 journal events**, digest `27129d445158c58f1ed96e443bb18aa71d17e078275c99a80e9a9a19c961c83a`. It intentionally repeats the comparable input sequence on the rebuilt final ROM. All inputs were ordinary buttons. WRAM/OAM was inspected read-only; no progression was injected. The native emulator and recording were closed after testing.

- Boot/help, first Union → St. Lawrence delivery: condition 100, cash 30 → 139, unique completion count 1.
- After the first delivery and audio-menu checks, the previous stopping sequence held A for 40 frames, A+right for 48, then A for 24. At frame 1,160 the car was at (797.125,771.5625), speed 24, heading 4. It continued through the corner while acceleration stayed held.
- Braking, parking, completing the exit animation and pressing A near the car changed `onfoot` 1 → 0 and restored the parked position (797.125,789.4375).
- Pause-menu audio cycled full → effects only → silent → full without advancing the mission clock. Actual PCM mode evidence is below.
- `_game_time` advanced 59 updates during frames 1,408 → 1,528: approximately 29.5 rendered updates per second in this bounded stationary waterfront sample. This does **not** establish improved whole-city frame pacing. Performance work remains open.
- Final OAM sample: four visible hardware objects, peak two on a scanline, zero over-limit lines. An earlier audio/world-actor build had a crowded sample peaking at four. These are bounded observations, not a whole-city sprite/performance certification.

The latest Front Street screenshot is an unmodified frame 676 from this ROM. Its digest and recording provenance are in `docs/screenshots/provenance.json`. Retained review images are sampled; they do not establish every intervening animation frame.

Two further exact-ROM recordings verify the transition fixes:

- `city-life-entry-transit-final-20261002`: 424 frames, 117 events, digest `ea42c063e39e2cb0f163193f1804fb2021ac9e9abe77ab401092d03f0145852f`. After parking and walking within 13 pixels of the Union car/station, simultaneous A+B retained ROAM and entry. Pausing mid-entry and choosing Transit retained PAUSE; resuming completed entry at (560,720), `onfoot=0`.
- `city-life-failed-job-transit-final-20261002`: 8,848 frames, 158 events, digest `cb5a437110181e6f211c948df5f86afaee53da7f122837a397b262e7df4515ba`. Ordinary neutral frames let the first job time out at Union: RESULT, no active job, condition 0. Free roaming, parking and a new Line 1 booking then reached **King station** (640,640), ROAM, condition 100, cash 30 → 27, on foot. The recording's stop-reason text mistakenly calls this destination Queen; the authored stop, framebuffer and read-only state establish King. Genuine mid-ride deadline expiry remains covered by the host fixture and historical native scope separately.

The pre-transition-fix `city-life-corner-guarded-20261002` ROM `038f1561…` also passed the corner/delivery/entry sequence at 1,528 frames and 215 events, digest `8b8078448e8af57d81e6591a6f42ddaef1f55c6fa7fec7a85148db1870d2c780`. It does not verify the later transition fixes.

Earlier recordings remain separate evidence: `city-life-native-20261002` (`9d7e1fcf…`, 1,368 frames, 231 events, digest `ca0756f7a10e9c92e603e779bb29579d8ccab25d2778c4835924b1c5d668a4f2`) verified first delivery/audio modes/walking but still stopped during the turn. Its stop-reason text overstated car entry: A was pressed before the exit animation finished. Re-entry is established by the final recording above. `city-life-wide-roads-20261002` (`bba90831…`, 1,160 frames, 151 events, digest `033323e0c123865b3bef2792643000ab51d4936772bb081df68e28082258d1a9`) showed wider roads alone still stopped the same turn. These results led to the swept corner adjustment; they are not passes for the final behavior.

Older recordings were archived unchanged through the plugin's supported operation after closing the emulator, releasing recording admission reservations while retaining their bytes and original-path mappings. `handling-before-20261002` is retained under archive `3e855334-d14c-49af-b9e3-12c2860050b2`; `city-life-native-20261002` under `2c5ab980-e5ef-4d97-95c8-e47f66b2b38d`; `city-life-wide-roads-20261002` under `519c34f9-9b0f-4780-913c-1b8e1c77ebb9`; and `city-life-corner-guarded-20261002` under `7aa6d1b5-d863-4fdb-805e-95c9e93ea5e6`. Each is in `project/artifacts/recording-archives/<id>/recording`. Archives and capture journals remain excluded from Git and distribution bundles.

### Actual emulator audio

The opt-in public PyBoy 2.7.0 capture script ran against verified final ROM bytes `a2f00db4…`, with no injected memory/save writes or adjacent save autoload. It copies the public signed-byte buffer through its byte head after each frame. Eight 48 kHz stereo WAVs and their manifest are retained locally in ignored `project/build/audio-evidence/native-a2f00db4-modes/`. The separately captured pre-transition-fix `038f1561…` measurements produced the same WAV digests; its manifest remains bound to its own binary. Capture and reproduction details are in [AUDIO.md](docs/AUDIO.md).

| Scenario | Captured stereo samples | Absolute PCM peak |
| --- | ---: | ---: |
| City music | 480,000 (10 seconds) | 2,304 |
| Held acceleration / engine | 96,000 | 3,072 |
| Braking | 38,400 | 2,560 |
| Paused, effects only | 96,000 | 0 |
| Driving, effects only | 96,000 | 768 |
| Silent menu | 96,000 | 0 |
| Silent driving | 96,000 | 0 |
| Resumed city music | 480,000 (10 seconds) | 4,352 |

This proves actual emulated sound output and mode silence in these scenarios. It does not establish human listening quality, all event sounds, or physical speaker/headphone behavior.

### Remaining acceptance gates

The browser preview still has an unresolved recording-close acknowledgement and an older build identity. The new ROM has not been refreshed or verified there. Its state was preserved under the plugin authoring rule to resolve unknown outcomes before reload/replay. Native tests continued independently.

Full Old Toronto districts, dedicated TTC vehicle art and matching visible boarding schedules, a representative full campaign/unlock playthrough, measured two-hour duration, whole-city frame pacing, human listening, streaming, cartridge write/read-back and physical cold-boot/save tests remain open. No connected device or physical write is established. [OLD_TORONTO_EXPANSION.md](docs/OLD_TORONTO_EXPANSION.md) describes proposed district work, not implemented map coverage.

## Handling, campaign and save polish — 2026-10-02 (historical)

This milestone's native ROM was 262,144 bytes, CGB-only, MBC5+RUMBLE+RAM+BATTERY, 32 KiB declared SRAM. Official GB Studio CLI build exited 0; `rom_inspect` verified logo/header. SHA-256:

```
4db8413ab8ad8f7c20e9f1030632a0abcd323b9d512ddfcd29b77bb1be52e61f
```

Same pinned CLI/engine/GBDK/PyBoy versions as below. Build source fingerprint: `67a92d219b181feda1307b6209c0745cdaacd578dff44232d19c1284ce7e3e39`. Upstream DEP0190 and four SDCC optimizer warnings remain; no compilation error. The source revision for the published bundle is recorded in its BUILDINFO.json.

### Real-engine host regressions

`make check` runs `scripts/test_engine.py` with Clang/GCC, AddressSanitizer and UndefinedBehaviorSanitizer. It includes the actual TORONTO.c with hardware/input/tile/UI/actor stubs; the SRAM adapter redirects literal cartridge addresses to a bounded buffer and observes actual stores. **411 checks pass**, with no host compiler warnings. An earlier 135-check subset produced 18 failures against pre-fix main, demonstrating repaired behavior.

Fixtures cover held acceleration through steering on clear ground, opposing directions, continuing glancing curb contact, head-on stop/reverse recovery, brake priority, coasting/inertia, input edges across substeps, full 16-bit clock gaps/wrap/pause, passenger comfort, car entry across an actual blocked rail fixture, hidden pedestrian contact, authored traffic stop lines and world activity during transit waiting.

Save fixtures interrupt after every actual byte store in both alternating records, including overwriting an older valid destination slot. They verify final-magic commit, CRC fallback, rejection when both slots are corrupt, sequence wrap, a standard CRC check vector, 23 invalid but CRC-consistent states, legacy version-4 migration, paid-trip checkpoints, expiry during a ride and cancellation on the departure tick. These establish C logic under host adapters, **not GBDK ABI, cartridge CPU timing or physical power-loss persistence**.

### Retained native recordings

| Recording | ROM and observed scope |
| --- | --- |
| `handling-before-20261002` | Prior published `9eaec688…`, 572 frames, 62 events; held A+right turning reproduced rapid yaw and repeated curb/NPC slowing |
| `handling-after-20261002` | Intermediate `119565b4…`, 610 frames, 72 events; same steering sequence showed calmer yaw, speed reaching 24 during curb sliding, real corner impacts stopping and throttle recovery |
| `polish-delivery-transit-save-20261002` | Intermediate `119565b4…`, 3,648 frames, 305 events; revised briefs, first delivery, next eligible job, car entry, paused waiting, paid ferry reset recovery, Centre arrival and walking to within one pixel of new service point (760,944) |
| `polish-final-native-20261002` | Before portable-compiler warning cleanup `9ed8b60e…`, 3,880 frames, 353 events; delivery/entry, cancellation persisted through reset, paid-trip remaining time persisted, paused ride and Centre arrival |
| `polish-portable-final-20261002` | Latest `4db8413a…`, 3,880 frames, 349 events; repeated delivery/entry, next eligible job, saved cancellation, paid-trip remaining time, pause and Centre arrival |

All native inputs were ordinary buttons; WRAM/OAM was inspected read-only, with no injected progress or fabricated frames. `_td` was at WRAM offset 502 in the polish builds. Latest recording event digest: `67d941cac8c89b13d2f43d8580b053dbede73c718f5e19178f2429f79aa7a3e9`. Pre-cleanup recording digest: `7c2ba85cb4258500ea6e1d55f04d6f38df4b92571701d537968ff1d881931392`. Intermediate delivery/ferry recording digest: `6f9f7fd58a510aa1efaedf1b9a71041a2e43334c8fa169d9125cc594b3e2212f`.

Final build observations:

- First contract collected at Union, delivered near St. Lawrence with condition 100, cash 30 → 139 and unique count 1; A selected contract index 1 as the next eligible unfinished job.
- Parking and A entry restored the parked-car position with `onfoot` 1 → 0. The courier then walked to the ferry terminal.
- B cancelled WAIT to ROAM without reopening transit or charging a fare. A soft reset preserved that cancellation and cash 139.
- A new Centre-bound departure deducted one fare of 4 (cash 135). At game second 32, the ride had 6 seconds remaining. Pause for 240 video frames kept the entire state identical. After soft reset and leaving help, that paid trip still had 6 seconds and cash 135; arrival was (720,920), on foot, count 1.
- The pre-cleanup ride sample advanced 59 engine updates during 120 video frames, approximately 29.5 rendered updates per second. This is a bounded native CPU observation, not whole-city or physical-device frame pacing certification. Motion/clock compensation handles skipped updates separately.
- Pre-cleanup Centre Island OAM sample: 2 visible objects, peak 2 per scanline, no over-limit lines. Prior crowded and roof-occlusion samples below remain scoped to their earlier ROMs.

Content checks verify 72 unique authored titles/routes, 27 reachable service points, chapter unlock closure, native content consistency and Island foot access. They do not establish every deadline or two hours of play. Physical USB discovery succeeded but found no connected devices; no stream/write/firmware action was dispatched.

The owned browser preview still reports unresolved recording-close acknowledgement, with a different historical build identity. It was preserved. **The new ROM has not been refreshed or verified in that browser view.** Audio, full Old Toronto coverage, full campaign/unlocks, two-hour duration and physical cartridge boot/save tests remain pending. Loading and binary-notice instructions are in [LOADING.md](docs/LOADING.md) and [DISTRIBUTION.md](docs/DISTRIBUTION.md).

## Scaffold — 2026-10-01

Repository/content checks passed on initial setup and were rerun after research updates. The validator checks structure, source references, unique content identifiers, endpoint references, vehicle compatibility, and sensible mission values. It does not verify geographic placement, map reachability, handling, or engine integration.

## Bootstrap evidence (historical)

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

### Initial published north-up build (superseded above)

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
