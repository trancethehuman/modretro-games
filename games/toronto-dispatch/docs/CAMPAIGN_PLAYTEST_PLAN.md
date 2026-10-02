# Campaign playtest and duration plan

Reviewed 2026-10-02 against the published `v0.2.0-prototype.2` baseline, ROM SHA-256 `a2f00db4ef834112a3491e50cec832653023a0456f0d9cbca6d2386be7322a59`, source commit `0cf7ee2f5f8da63edea5911c150a27dd54032cd0`. This document is a source/content audit and a proposed testing plan. No new game execution, duration measurement or player enjoyment result was produced in this pass. The full campaign remains incomplete as an acceptance gate.

Sources inspected: [campaign content](../content/campaign.json), [campaign generator](../scripts/create_campaign.py), [native state/gameplay](../project/plugins/toronto-driving/engine/src/states/TORONTO.c), [native UI](../project/plugins/toronto-driving/engine/src/td_ui.c), [campaign validator](../scripts/check_campaign.py), [design](../DESIGN.md), [decisions](../DECISIONS.md), [testing evidence](../TESTING.md) and [Old Toronto expansion proposal](OLD_TORONTO_EXPANSION.md). Geography proposals below reuse that researched plan; no new street geometry, map-era decision or district layout is adopted here.

## What the current campaign actually contains

There are 72 unique titles/routes: nine chapters with eight types each, 27 service points and 612 between-stop legs. Each route starts with an interaction at its first pickup, then requires ordered interactions at the following points. A completed distinct contract increments progression once; replays can pay again but do not add a new completion. The mastery message requires all 72 distinct completions.

The type names are broader than the implemented rules. Fragile art has larger curb/wall damage; express files have tighter later deadlines; freight requires the truck and excludes transit; passenger jobs require the car, exclude transit and penalise fast steering. Parcel rounds, return papers and transit relays otherwise share the generic ordered interaction mechanism. Return routes end at their depot, but there is no separate signature-record mechanic. Transit relays remain intentionally feasible by road: riding is an alternative, not a compulsory timer objective. Island jobs combine ferry-only crossings with short on-foot delivery legs.

At a stop, Select works only within the target's proximity threshold and at speed magnitude no greater than 2; required-vehicle jobs also reject walking or the wrong vehicle. The proximity test does not model a customer entrance or a signature. New district entrances need reachable interaction points and a check that walls cannot put an inaccessible objective within interaction range.

### Progression coverage

| Unique completions | Total contracts unlocked | New coverage |
| --- | --- | --- |
| 0 | 4 | 01 parcel, 02 fragile, 03 express, 07 returns |
| 3 | 6 | 04 freight, 05 relay |
| 6 | 12 | Six second-chapter contracts |
| 8 | 14 | 06 and 14 passenger jobs |
| 12 | 24 | First three Island jobs and third chapter |
| 18 / 24 / 30 / 36 / 42 / 48 | 32 / 40 / 48 / 56 / 64 / 72 | Later chapters, eight additional jobs per gate |

All contracts become available at 48 unique completions; availability is not campaign completion. Static unlock closure is already checked, but does not prove that players can finish the required jobs, afford their transit or understand the next offer.

One ordinary-play path to representative coverage is **01, 02, 03, 04, 05, 07, 09, 10, 06, 11, 12, 13**, then **08**. This reaches the Island gate through 12 distinct successes and exercises the other seven types without editing completion bits. Change to the truck before accepting 04 or 12, and return to the car before accepting 06. The player remains free to choose another valid order.

## Duration feasibility from source, not a playthrough

The generated metadata sums to **171,456 road pixels**, **3,080 Island walking pixels**, **33 required ferry legs**, **43.5 modeled movement minutes**, and **51 minutes of assumed handling**. Handling is simply five seconds per between-stop leg. It is a design allowance, not an interaction delay enforced by the game. All deadlines together sum to **299.4 minutes**; spending the entire deadline is not the content's length. The first three deadlines are deliberately generous 120-second tutorials.

Native positions use Q4. The movement loop advances at most four simulation steps per rendered update while the clock counts every VBlank. With regular one/two-frame updates, 60 simulation steps correspond to one game second. Larger render gaps can reduce movement relative to the deadline, so performance remains part of deadline feasibility.

| Travel mode | Configured limit / per-step movement | Nominal cardinal pixels per game second | Integer-settled cardinal estimate |
| --- | --- | --- | --- |
| Car | Speed 24, Q4 | 90 | 86.25 |
| Truck | Speed 20, Q4 | 75 | 71.25 |
| Motorcycle | Speed 28, Q4 | 105 | 101.25 |
| Scooter | Speed 18, Q4 | 67.5 | 63.75 |
| Walking | 8 Q4 units per cardinal step | 30 | 30 |

The cardinal driving correction comes from the actual integer traction recurrence: velocity approaches `16*speed` from below, can settle three units below it, then `velocity/16` truncates again before updating Q4 position. These are clear-ground source estimates, not observed handheld speeds. Acceleration, turns, braking, collisions and route geometry add time. Different headings and diagonals need their own native measurements.

Using those settled values and the authored road distances gives about **44.2 moving minutes** with scooters on unrestricted jobs, cars for passengers and trucks for freight, or **32.2 minutes** with motorcycles on unrestricted jobs. Adding the generator's unmeasured handling allowance gives **95.2** or **83.2 minutes** of movement/handling. Those estimates exclude initial pickup travel, repositioning between jobs, actual traffic, route decisions and genuine retries. Tile-centre paths are conservative and cannot serve as strict lower bounds on a driven route.

The 33 mandatory ferry rides add 4.4 modeled minutes at eight seconds each. The 30-second schedule's two-second boarding window gives zero to 28 seconds of waiting per leg: at most another 15.4 minutes, or about 7.4 minutes under a hypothetical uniformly distributed integer arrival phase. Neither distribution nor actual journey time has been measured. Do not count ferry waiting as proof of two hours of engaging content. Later Island rounds repeatedly transfer through the mainland; shorter verified Island foot links should replace unnecessary transfers where the researched geography permits them.

The conclusion is a production risk, not a measured campaign length: **72 distinct entries and five hours of deadlines do not establish 120 minutes of purposeful gameplay**. Preserve `duration_verified=false` until a practiced complete run and a first-time run support the claim.

## Ordinary-button native scenarios

Use the official plugin on the intended exact ROM. Start with an isolated fresh boot or a genuine save reached through ordinary inputs. Record save provenance. Never inject money, unlocks, position, cargo, stages or transit state. The model chooses bounded inputs, inspects genuine frames, then chooses the next input. Release buttons between separate interaction presses. Avoid blind long scripts near stops, deadlines or scene boundaries.

The table names current contract IDs/routes. District expansion may deliberately remap coordinates while retaining IDs; bind every recording to that new ROM and source revision. A route's last interaction, not arrival near its last landmark, establishes completion.

| Type and representative contract | Ordinary input route | Required observations and variant |
| --- | --- | --- |
| Parcel — 01 MARKET START, initially unlocked | Select opens dispatch; A accepts. Select collects at Union. A drives east on Front; B brakes; Select delivers at St. Lawrence | Stop interaction at speed and away from the target must reject. Holding Select must not advance two stages. Exactly one distinct completion and one reward. Replay gives no second unlock completion |
| Fragile — 02 FIRST ART CRATE, initially unlocked | Union → AGO/Grange, with two legal road choices where the actual map permits them | Compare a clean, early-braked route with one controlled curb/wall contact. Record condition before/after and scaled payout. Repeated damage to zero must fail, preserve earlier completions and allow retry. A clear roof/entrance must not mean a passable building |
| Express — 03 DISTILLERY FILES, then 11 CROSSING DEADLINE after six completions | Tutorial Union → Distillery; later Union → City Hall → King → Distillery → Riverside → Castle Frank → Bloor–Yonge → Union | Do not judge express tension from the generous tutorial. Compare two legal corridor choices on the later job. Pause/map freezes time; free roaming resumes it. Test final interaction just before expiry, and an intentional timeout. Keep timeout-test idling outside duration totals |
| Freight — 04 SHORE SUPPLIES after three completions | Stop with no job; Start → Change Vehicle selects truck. Union → Market → Distillery → East Bayfront | Acceptance in the car rejects. After acceptance, walking cannot collect/deliver and transit must reject without a charge. Truck braking/turning and every loading point remain reachable. Compare a wider route with a shorter route without changing the vehicle mid-job |
| Relay — 05 FIRST CONNECTION after three completions | Union → Bloor–Yonge → Wellesley → Ossington → Wellesley → Castle Frank → Wellesley → Union | Compare the legal road route with train/bus/foot travel from comparable schedule phases. Park, walk into the stop threshold, B opens transit, arrows select, A waits; at Wellesley up/down changes service. Charge each boarded fare once, retain the car, continue objective/deadline through arrival and never auto-deliver at a station |
| Passenger — 06 STATION PICKUPS after eight completions | Car: Union → Market → Distillery → Riverside → Bloor–Yonge → ROM → CN Tower → Union | Wrong vehicle/on-foot acceptance and transit reject. Compare gentle braking/steering with steering above speed 18: comfort and payout should differ. Check every door access. Do not claim distinct passenger boarding/alighting animations from generic waypoint interactions |
| Returns — 07 SIGNED AND SEALED, initially unlocked | Union → City Hall → Market → Union | Stop order must hold; revisiting Union early cannot finish. Last interaction at Union awards once. Baseline uses ordinary stage changes, so separately mark the proposed signature/receipt presentation as unimplemented |
| Island — 08 CENTRE LETTERS after 12 completions, then 24 WARD COTTAGE MAIL and a late multi-Island route | Ferry terminal → Centre dock → Centre Park Post → Centre dock → ferry terminal; park the vehicle on the mainland and walk the service-point legs | Delivering at the dock cannot substitute for the inland objective. Two boarded crossings cost eight credits. No vehicle water/Island shortcut. For multi-Island jobs verify required mainland transfers, all last-mile paths, remaining-time feasibility and the eventual route back to the parked car |

### Shared reliability scenarios

- **Cancel:** cancel an active job from the pause menu; it must return to roaming with no payout/completion. B cancels WAIT before boarding, including the departure tick, without charging. Once RIDE has charged, cancellation must not silently refund or charge again. These cases are separate from deliberately abandoned content time.
- **Failure and retry:** expire an ordinary job and one already-paid trip. Failure must not increment completion or pay. A paid ride still reaches its booked destination; retry/next free-roaming trip must not inherit the previous failure. Re-accept through dispatch normally.
- **Save:** Save Progress after an intermediate stage and after completion; use a documented save-preserving reset and record its actual method. Verify position, parked car, unique count, cash, stage, condition and remaining time. WAIT/RIDE checkpoint automatically; recover them without recharging. A lifecycle reset is recorded separately from gameplay buttons. Physical cold boot/power-loss checks remain distinct hardware evidence.
- **Pause and map:** freeze ROAM, WAIT and RIDE; pan the map and centre the objective; return to the same position/trip/clock. Record both the game clock and actual video-frame interval. Audio mode changes must not advance missions or charge fares.
- **Entry:** ordinary parking and A re-entry must finish before a transit selection. A+B together beside the car and transit from a paused entry must not move a half-entered courier to another stop. Do not recreate progress by a memory write.
- **Fare and schedule choice:** reach insufficient cash by genuine fare spending in a separate fresh run. A failed boarding leaves the courier recoverable with no negative balance. Compare immediate boarding, a mid-cycle arrival and a just-missed departure; record when driving is faster as well as when transit wins.
- **District seams:** for the first two native districts, cross each reciprocal road/sidewalk seam with all permitted vehicles and on foot; repeat while carrying, returning to a parked car and after a save. Preserve objective/stage/condition, heading, fare status and global clock. A local coordinate in the wrong district must not satisfy pickup/delivery. Foot-only ports reject the truck; matched road ports do not launch the courier into a wall or reset progression.

After one successful representative run per type, cover a mid/late job of every type and every vehicle, then complete all 72 through unique ordinary-play successes. Sampling eight jobs does not close the campaign gate.

## Content required for a meaningful longer campaign

This is a revision proposal, not new implemented content or a change to accepted camera/physics. Preserve the early tutorial IDs and short learning routes. Expand their later counterparts across the researched Old Toronto footprint rather than extending the same compressed loops or increasing deadlines.

For the first core/central-west district pair, require actual selectable routes along the researched Queen/Dundas/College/Bloor connections, reachable fictional client entrances, clear portal/parked-car/objective markers and at least one job crossing the seam. Prove one car route, one truck route and one park/walk/transit alternative before multiplying districts. Name precise street connections only after the geographic/access graph is verified. The 17-scene atlas and 2026 era remain proposals in the separate expansion plan.

| Content family | Purposeful new objective | Route/environment decision |
| --- | --- | --- |
| Local parcels | Several distinct storefront handoffs with a visible client/parcel response | Park once and walk a cluster, or drive between wider-road entrances; choose a legal alley only for vehicles allowed by its access mask |
| Fragile art | Intact gallery/museum exchanges with readable condition feedback | A longer, quieter corridor versus a busy direct route; early braking matters without an obligatory stationary delay |
| Express files | A real timed office-to-office exchange, with landmarks and an intelligible cutoff | Compare two named crossings/corridors and their signals; do not invent a King/Front bridge over the Don |
| Freight | Yard/loading-door delivery restricted to the truck, with unmistakable loading access | Wide industrial approaches versus restricted park paths, rails/water and narrow streets; use researched Port Lands/Exhibition geography |
| Relays | Small-package handoffs beside real transit access, followed by a useful on-foot leg | Departure phase, fare, transfer, walking distance and parked-car recovery can change the best route; transit remains optional when road travel is feasible |
| Passengers | Visible rider approach/boarding and a distinct drop-off response, with comfort feedback | Smooth main-road turns versus a shorter congestion-heavy route; avoid scoring every fast straight segment as discomfort |
| Signed returns | Display which client acknowledged the original, then return that same document to its depot | Plan an ordered approval route and depot return; use clear signature progress rather than renaming parcel pickups |
| Island post | Longer genuine cottage/park/service entrances beyond ferry docks and verified Island path choices | Park mainland, choose the appropriate ferry, walk useful inland routes; preserve public vehicle restrictions and avoid needless dock shuttles |

Every later contract should introduce a client/objective response, meaningful access/condition/time rule, or new geographic decision. A new title plus the same sequence is insufficient. Maintain several legally useful corridors, living traffic/pedestrians and readable landmark/neighbourhood differences. Optional exploration and a repeat-job economy can add replay value, but do not count forced repeats toward the first campaign clear.

A planning budget for nine eight-job content groups is **8, 12, 14, 16, 14, 16, 16, 18 and 18 purposeful minutes**, totaling **132 minutes** before obligatory idle waits. This is an authoring target with a modest margin, not a predicted result. Gates may interleave those groups; do not force the first chapter's currently locked passenger/Island jobs to finish before their normal unlocks. Seventy-two jobs at 132 minutes imply about 110 purposeful seconds per job on average, with brief tutorials balanced by substantial later trips. Replace the budget with measured distributions; add genuine objectives/district decisions if a practiced run is short, rather than slowing vehicles or padding queues.

## Findings to check before expansion

| Source-derived finding | Small reproduction/acceptance check | Classification |
| --- | --- | --- |
| Walking uses 8 Q4 cardinal units but 6+6 diagonally: about 6.1% faster by vector magnitude | Walk equal unobstructed distances cardinally and diagonally under equal video frames. Normalize diagonals or explicitly discuss a deliberate change; preserve collision sliding/opposing-input behavior | Conflict with the accepted no-accidental-diagonal-speed-bonus rule; no native repro performed in this audit |
| `td_ready_offer` chooses an unlocked unfinished job without testing vehicle/on-foot compatibility | Finish 01–03 in the car: default offer becomes truck-only 04 and A rejects. Prefer a compatible default or explain the required vehicle/change action clearly; keep manual browsing and gates | Source-proven UX friction, not an unlock deadlock |
| Timing generation uses nominal maximum speeds; native integer traction can move more slowly | Measure straight acceleration/steady movement/braking on each vehicle, then compare a mid express/truck budget at realistic render pacing | Model mismatch; current generous deadlines are not evidence of an unreachable job |
| Transit origin stores service selection in bit 6 and masks stop IDs with `&63` | Keep first-milestone transit origins below 64, or migrate the service flag to a separate versioned field before adding higher origin IDs. Test both sides of the representational limit | Expansion constraint; the current 27-stop map is within it |
| Pickup/delivery uses target proximity, without line-of-sight or a client entrance check | Put every new interaction marker on reachable terrain and try Select from the other side of nearby thin walls; require the correct entrance/district | Regression risk for denser district art; no wall-interaction exploit has been demonstrated here |

Record defects with the exact ROM, source, inputs, frames and observed result before changing runtime. Rebuild and repeat the relevant native scenario after a fix; do not convert this static audit into a claim of native failure or success.

## Duration and player-feedback record

The next authored slice is [eight western package contracts](../content/districts/west_jobs.json), appended as IDs 73–80 with stop IDs 27–34. The [content generator](../scripts/create_district_jobs.py) preserves the existing 72 contracts and core stop fields with canonical hashes, then checks actual registered collision grids and [reciprocal district portals](../content/districts/world.json). This is a content/connectivity check; native integration and ordinary-input campaign evidence are separate gates.

That slice models 32,656 vehicle pixels and 160 required walking pixels. Using integer-settled cardinal estimates gives 6.65–7.76 moving minutes plus 3.67 minutes of assumed interaction. These allowances are unmeasured. The Colborne service point is a fictional foot-only entrance with an 80-pixel approach and an 80-pixel return to a nearby drivable Queensway parking anchor. The Bloor gate is roadside and does not create a visitor vehicle entrance into High Park. The relay uses existing core services; the new stops do not add western train or bus schedules.

Cover all eight new jobs through ordinary inputs, including their car/truck/motorcycle/scooter restrictions, correct-district interactions, reciprocal crossings, Lodge park/walk/re-entry and a saved active cross-district job. The original eight-type matrix still applies. After integration, the full unique-completion gate becomes 80 jobs; eight new entries do not establish the requested two-hour duration.

Keep detailed recordings/saves local and ignored; publish a concise aggregate and hashes without capability URLs, activation/device data or personal player information. For each attempt record:

```json
{
  "schema_version": 1,
  "run_id": "campaign-practiced-01",
  "rom_sha256": "<exact tested ROM digest>",
  "source_commit": "<source commit>",
  "platform": "native emulator or identified physical cartridge",
  "runtime_version": "<emulator/version, if used>",
  "player_profile": "first-time or practiced",
  "contract_id": "contract-01",
  "attempt": 1,
  "save_origin": "fresh boot or genuine recorded save",
  "unique_before": 0,
  "unique_after": 1,
  "start_and_end_video_frames": [0, 0],
  "start_and_end_game_clock": [0, 0],
  "purposeful_seconds": {
    "drive": 0, "walk": 0, "interaction": 0, "route_planning": 0
  },
  "separate_seconds": {
    "schedule_wait": 0, "automated_ride": 0, "administrative_pause": 0,
    "tool_latency": 0, "intentional_failure_test": 0
  },
  "route_choice": "<streets, district portals and services actually used>",
  "pickup_and_handoff_frames": [],
  "outcome": "completed, failed or cancelled",
  "condition": 100,
  "cash_delta": 0,
  "fares_paid": 0,
  "evidence": {"recording_id": "<retained ID>", "event_digest": "<hash>"},
  "feedback": "<observed decision, confusion, repetition or handling issue>"
}
```

Use actual values, not the zero placeholders. A native video-frame count is useful for simulation timing, but unthrottled emulator speed, host throughput and time spent between paused model calls do not measure human gameplay. Measure human sessions at normal speed. Count ordinary brief/map planning only when it supports a real route decision; exclude unattended paused screens. Report natural retries separately from the successful unique-content total, and never inflate duration with deliberate failure fixtures.

Collect per-type/chapter medians and ranges, completion/failure/cancellation counts, condition/reward distribution, transfer waits, route-choice use and feedback. Perform a practiced efficient first clear and a first-time first clear, reaching all 72 unique completions without mandatory replay farming or inserted waits. Verify the proposed full-city coverage independently. Claim at least two hours only when retained normal-play evidence meets that target through purposeful content, with waits/retries disclosed. Enjoyment still requires human feedback about navigation, handling, variety and choices; neither a duration sum nor this plan certifies fun.
