# Decisions

## Accepted, 2026-10-01

- Original game for the user's ModRetro device and writable cartridge.
- A collection repository with one folder per game, GitHub publication, MIT licence, README, agent instructions, and reusable skills.
- Toronto courier driving with timed missions, objectives, obstacles, traffic, and open roaming.
- Initially angled/isometric; superseded by the user's explicit top-down revision on 2026-10-02.
- More realistic driving: momentum, braking, and tighter traffic rules.
- Mixed courier jobs, with packages first and passenger rides later.
- Cars, trucks, motorcycles, and scooters with mission-dependent availability.
- Recognisable Toronto landmarks, neighbourhoods, street names, trains, streetcars, and industrial areas.
- Use the ModRetro Chromatic plugin and supplied official guide.

## Accepted, 2026-10-02

- Limit the city to Old Toronto (the former City of Toronto), including its waterfront and Toronto Islands; exclude the surrounding GTA.
- Research actual maps, neighbourhoods and iconic buildings before producing the game city.
- Release must have buildings, roads, named streets, open-world roaming, a scrollable map and varied playable quests targeting at least two hours of gameplay. Duration is a playtest acceptance target, not inferred from quest counts.
- Train and TTC bus services run autonomously on game schedules. The courier pays a small in-game fare, boards at stops and hops off strategically closer to delivery destinations.
- Transit schedules and fares are fictional gameplay parameters; real route/station names and geographic relationships remain researched.
- Street architecture must vary in building type, footprint, height and design; include tall and wide buildings rather than repeating one block shape.

## Presentation and handling revision, 2026-10-02

- Switch to a perpendicular, direct-facing top-down city because the user found isometric driving difficult. Preserve momentum and braking.
- Widen roads, grow the usable city, vary architecture and increase building density.
- Buildings must block movement, roof edges/canopies must occlude sprites, and people must visibly walk, approach a car, enter it and drive.
- Add autonomous walking NPCs. Correct overly fast turns and abrupt speed loss during steering.

## Implemented handling and reliability tuning, 2026-10-02

- Slower speed-dependent yaw, with reverse using the magnitude of speed; opposing steering inputs cancel.
- Steering keeps acceleration and smoothed velocity. Glancing contact removes blocked-axis motion without draining scalar speed on every tick. Asphalt is now 48 pixels wide with 8-pixel sidewalks; all 80 building footprints remain unchanged, and building shadows/priority regions are clipped at the road edge.
- Collision checks every tile overlapped by the car footprint, including narrow rails. A small corner correction searches 1–6 pixels of clear lateral space only with acceleration held, no brake and forward speed at least 3. It validates bounds, candidate footprint and swept lateral clearance, with at most one correction per rendered update. Broad head-on walls remain solid and stop the car.
- Hidden pedestrians cannot slow the vehicle. Traffic stop lines use the authored junction coordinates. Autonomous traffic keeps moving while the courier waits/rides.
- Traffic motion and screen presentation are separate. Road loops and a bus proxy run continuously; six nearby pedestrian actors follow 102 fixed, collision-validated world routes and retain their routes while visible. Full TTC route geometry and moving streetcars remain future work.
- Car entry/exit checks the whole door approach against the native collision grid. Pressed actions occur once across motion substeps.
- Transit cannot interrupt an active car entry/exit. If successful A entry and B transit inputs coincide, entry takes priority and its animation finishes without opening transit.
- Starting a fresh trip with no active job clears the previous job's failure condition. An actual job deadline expiring during a paid ride still causes failure, shown after arrival at the booked destination.
- Mission/transit time counts every VBlank independently from bounded motion catch-up.
- Two alternating version-5 SRAM records, CRC16 and last-byte commit preserve a recovery snapshot. Boarding and cancellation are saved, and paid trips resume after resetting. Old version-4 records keep earnings/completions and retire changed active routes.
- Authored contracts have distinct routes/briefs and chapter progression. New Island delivery points require walking, and ferry transfers go through the mainland. Condition changes base rewards; fast passenger turns reduce comfort.
- Original City Shift music, vehicle/braking ambience and event cues use the native audio driver. The pause menu exposes music + effects, effects only and silent; the preference resets on boot. Native PCM confirms output and mode behaviour, while human listening and physical audio checks remain pending.

Native driving regression evidence covers the first job, the formerly stopping Distillery turn at speed 24, and parking/re-entry. Host regressions check collision, assistance and state rules; current build identities and results belong in [TESTING.md](TESTING.md). The observed approximately 29.5 rendered updates per second is a bounded sample; performance, handheld feel, human audio review and physical SRAM behaviour remain open. These results do not establish full Old Toronto coverage or the two-hour release target.

## Implemented first western expansion, 2026-10-02

- Three native scenes now link compressed central Toronto, Parkdale/Roncesvalles and High Park/Swansea/Junction. Each local scene remains 1,024 × 976 pixels, with a logical 3,072 × 976 atlas. Camera scrolling is local; genuine GBVM scene changes handle the authored seams.
- Preserve the original 80 core footprints and add 37 western and 49 High Park/Junction footprints, for 166 total. Original architectural styles, carhouse bays, Regency veranda/chimneys, Sunnyside colonnade and Junction brick detail keep the new districts distinct. Generic building/shadow priority is clipped at asphalt; core tree canopies and CN raised lips retain intentional depth occlusion on passable ground.
- Use official City/TTC topology and original pixel compression. College ends at Dundas, Howard Park reaches Parkside without a through car road across High Park, King bends into Roncesvalles, and rail/Gardiner barriers have explicit road or foot crossings. The [western research and seam table](docs/WEST_DISTRICT.md) record source facts, City data attribution and design choices separately.
- Eleven reciprocal seam pairs connect the three scenes, including a foot-only waterfront pair. Ordinary crossings retain local player/vehicle state, job/cargo/progression, world clock and runtime audio state. A parked car carries its own district identity; foot travel into another scene leaves it where parked. Save validation reads each district's own collision resource without changing the loaded scene.
- The completion bitmap reserves capacity for 128 contracts, with 80 authored at this stage. Eight progression-gated western package routes and eight clients are appended after the unchanged 72-contract/27-stop prefix. The Colborne service flag requires an on-foot handoff, even when a parked car is nearby; its Queensway parking approach keeps the intended last-mile walk short. A signed-return description continues to mean an ordered return route, with no new signature system.
- The route pool contains 358 collision-validated paths, with six nearby pedestrian actors in the loaded scene. A banked selector stores six identities and a 24-byte coordinate cache in RAM; each western scene also has six authored closed traffic loops. Rendered actors remain bounded as the world grows.
- Version-6 alternating SRAM records store district-qualified player/parked-car positions and the larger completion bitmap. Valid version-5 state migrates into the expanded core layout; the older version-4 migration still keeps earnings/completions while retiring obsolete active work. CRC, last-byte commit and interrupted-write checks remain part of the host suite; physical cold boot/power-loss proof is pending.
- Registered PNGs, native palette/priority bytes and collision arrays must agree. Repository checks validate actual source pixel budgets, full overlapped vehicle tiles, accepted seam lanes, western clients, traffic/NPC paths and deterministic native content/headers. These checks remain separate from ROM execution evidence.

Prototype 3 is published with verified native boot, first delivery, held-acceleration turning, core/west driving, west/High Park walking in both directions, remote parked-car recovery, return driving, western active-job reset and a paid core subway trip. These are sampled routes, not every seam or western contract; [TESTING.md](TESTING.md) owns exact build evidence. Full former-Toronto coverage, measured two-hour gameplay, western scheduled TTC services, crowded-scene performance, a confirmed current browser preview and physical cartridge checks remain open. The 2026 era is still a proposal.

## Implemented eastern expansion and banked world navigation, 2026-10-02

- Four compressed native scenes now compile and load: core, west, High Park and Riverside/Riverdale/Leslieville/western Danforth. Each remains 1,024 × 976 pixels, in a logical 4,096 × 976 atlas. The total is 211 building footprints (`80+37+49+45`), 486 fixed pedestrian routes (`102+128+128+128`) with six nearby actors, and 18 closed traffic loops across the three added scenes. These counts describe bounded local resources, not full Toronto coverage or simultaneous off-screen simulation.
- Three new reciprocal pairs connect core/east through Bloor/Danforth, Dundas and Queen, for 14 pairs total. Broadview and the Don remain in the retained core. Gerrard is conditional on a core east-bank label/topology correction and stays closed; King/Front/Lake Shore add no new bridge. Pape's documented pedestrian rail connection remains car-blocked, with vehicle detours through researched road crossings. Original compressed pixels and selected City facts are distinguished in [EAST_DISTRICT.md](docs/EAST_DISTRICT.md) and `east_art.json`.
- Eight eastern package routes and eight fictional clients are appended after the preserved 80-contract/35-stop prefix, producing 88/43. Existing contract kinds support fragile venue cargo, motorcycle express, truck stock, relay/park mail, returns and a cross-city scooter round. Withrow/Greenwood handoffs require walking from short parking approaches. No new cargo animation, signature UI, operating future station or artificial loading delay is implied. Final-candidate ordinary-button play completes eastern contracts 81, 82, 83 and 85 alongside five original jobs; remaining eastern/western objectives and duration measurement stay pending.
- Names, portals, district graph traversal and non-core traffic tables move into a banked world module. Routing selects the fewest feasible district hops for the current travel mode, then approach distance; it is not street-level route optimisation. The beacon/HUD show the next district, refresh when entering/leaving the car, and show `NO ROAD ROUTE` when driving has no feasible district connection. Local foot-only handoffs retain `PARK THEN WALK`; the mode refresh is not a new global transit menu.
- The module adds a 36-byte six-vehicle target sample cache and one next-district byte, 37 runtime bytes total. The existing 24-byte pedestrian coordinate cache, eight VM contexts and 58-byte version-6 save format remain unchanged. The final candidate's linked memory guard passes with heap `D934`, stack `DF00` and 1,484 bytes of reserve. A native High Park reset restores nine completions, cash, clock, the courier on foot and the car parked in the west. This sample and host interrupted-write tests do not establish physical persistence or every deepest call path.
- A banked auxiliary getter supplies road parking anchors for Colborne 34, Withrow 36 and Greenwood 41. Drivers target the approach; exiting restores the actual client coordinates immediately, with no changed client record or save field. Native Withrow play verifies `(224,144)` parking cue → `(320,144)` foot handoff, completion and re-entry. Colborne/Greenwood still require native handoff samples.
- Transit selection and waiting use one two-second departure-window function. The choice screen displays its countdown. Confirming while the current window is open boards immediately; a closed window enters waiting. Fare deduction, paid-trip save state, cancellation priority and finishing a timed-out paid ride at its destination remain part of the state rules. This refactor changes no route names, fares or service geography and adds no scheduled eastern/western TTC service.
- The final-ROM subway sample confirms immediate open-window boarding, one fare charge (`30→27`) and arrival on foot at King while the car stays parked at Union. Its following 120-video-frame sample advances 59 rendered updates (about 29.5 per second); an OAM snapshot has 12 visible sprites, peak four per scanline and no over-limit scanlines. This is bounded core evidence, not crowded-world performance or paid-ride reset proof.
- Source/host checks cover the four registered resources, exact PNG/attributes/collision agreement, tile budgets, reciprocal lanes, actor routes, client connectivity, appended content and banked world behaviour. Eastern art uses 85 raw / 63 flip-canonical tiles under the 320 target. These are separate from compilation, native execution, public release and hardware evidence.

Final candidate `1da71ba5…` is built and sampled in the native emulator; `7a299125…` is an earlier four-scene build before the parking-anchor fix. Automated ordinary inputs complete nine distinct contracts (01, 02, 03, 04, 07, 81, 82, 83 and 85), load all four scenes, cover the three core/east approaches, drive core-to-west, walk west-to-High Park, pan the local map with player/clock frozen and repeat the held-turn regression at speed 24. The remote reset preserves that progression. This is about eight minutes of purposeful native gameplay, not a full campaign, two-hour acceptance or human review.

Prototype 4 identifies this four-scene milestone; Prototype 3 remains a separate historical release. [BUILD.md](docs/BUILD.md) records binary identity and linked memory; [TESTING.md](TESTING.md) owns exact scenarios. Remaining seam lanes/jobs and final paid-ride reset, full former-Toronto/waterfront/Islands, at least two hours of varied play, an adopted map era, new TTC services, crowded-scene performance, browser state and hardware checks remain pending.

## Browsable city atlas, 2026-10-02

The paused map now uses the four registered collision grids in geographic order High Park → west → core → east, with their authoritative offsets. At 1:8 scale the combined schematic is 512 × 122 pixels, padded to 64 × 16 native tiles. It shows actual road/walking ground and authored blocked water; roofs do not imply a drivable route. Its 160 × 96 viewport pans across districts without changing the loaded gameplay scene. This replaces camera panning over a single district.

Player, vehicle and objective markers retain district identities. The objective uses the existing road parking anchor while driving and actual client after exiting. Free-roam points to the Union depot; paused no-job waiting/riding points to the booked transit stop. Select cycles useful focus points; A centres the objective and B/Start can leave an incomplete repaint. No fare, progress, save schema, route, client or mission rule changes.

The renderer shares the existing 360-byte text cache, reserves CGB bank-1 ground tiles16–187, marker tiles8–14 and font192–240, and adds28 bytes of transient WRAM. Its172-slot double-hash dictionary avoids a linear search through every uploaded pattern; odd strides1–31 visit every slot, with bounded probing. The API/data remain autobanked. Linked budgets, host sanitizer results and native framebuffer/timing/state checks belong in [CITY_MAP.md](docs/CITY_MAP.md), [BUILD.md](docs/BUILD.md) and [TESTING.md](TESTING.md), each scoped to its exact build.

This adds navigation to the existing four areas. Full former Toronto, waterfront and fuller Islands, at least two measured hours of varied play, human handling/audio review and physical cartridge checks remain required.

## Queen 501 scheduled source milestone, 2026-10-02

The user already accepted autonomous paid transit and strategic alighting. The current implementation adds eight researched representative Queen platforms across west/core/east while retaining the four existing areas, all 88 contracts,43 original client/station records and 58-byte version-6 save format. Supplemental platforms 43–50 use original curb signs and shared directional boarding points; this is compressed game design, not surveyed TTC infrastructure or an added map era.

A banked transit module owns the service queries used by menus, runtime and saved-trip validation. Queen service4 costs three game dollars, repeats each direction every 64 world-clock seconds and has two-second boarding windows. Eastbound index`i` departs at `4*i`; westbound at `32+4*(7-i)`. Destination selection determines the direction, and a ride takes `4*abs(destination_index-origin_index)` seconds. Confirming in either open-window second boards immediately. Current construction diversions, full 501/504 coverage, TTC branding and real fares/timetables are outside this milestone. Research and source/compression distinctions are in [STREETCAR.md](docs/STREETCAR.md).

Remote paid arrivals commit an alighted state before queuing a scene; a failed queue retains the paid ride for retry. An earlier native Queen candidate completed three cross-district rides with pause/reset/map and parked-car recovery, but separate legacy-transit play exposed Union arrival onto the player's parked car. A bounded source correction retains the exact stop centre when clear, otherwise tries connected cardinal 12/18-pixel foot points clear of that car and loaded traffic; fully blocked arrivals retry without another fare. Host regressions pass. Exact final ROM `23b2a7a2…` repeats three Queen journeys with map/reset/car recovery and separately verifies subway, 94 bus, Island ferry and safe Union alighting with subsequent walking/car entry. This is the Prototype 6 milestone; these samples do not cover every platform or blocked-arrival condition. Historical results remain tied to their own ROM in [TESTING.md](TESTING.md).

Moving streetcar artwork, human feedback on strategic transit, remaining platforms/arrival conditions, full former-Toronto coverage, measured two-hour varied gameplay and physical cartridge acceptance remain open. This implementation does not adopt the proposed 2026 era.

## Implemented pickup condition lifecycle, 2026-10-02

- Cargo/comfort starts at 100 on acceptance and can decline only after the first successful pickup. Traffic, walls, curbs and fast passenger steering keep their existing carried penalties; multi-stop and return legs remain carrying until the contract ends. Deadline, collision motion, braking, fines and cooldowns still apply on the approach.
- Collision text describes braking while empty and cargo damage while carrying. Rider warnings require an occupied passenger job.
- Cold startup normalizes valid older active-stage-0 saves to 100 after CRC/semantic validation. It preserves carried damage, retired failure state, earnings, deadlines and district-qualified positions; invalid active condition remains rejected. This changes no save bytes or version.
- The native candidate and retained predecessor failure are scoped in [TESTING.md](TESTING.md). The published Prototype 6 bundle does not contain this later correction. Further quest/transit route-choice and hardware acceptance remain open.

## Performance and city presentation pass, 2026-10-06

User direction: make the game run smoothly without reducing fidelity, gameplay or richness, and improve crosswalk placement, building variety, actor/vehicle/boat design and transit presentation.

- Implemented: the native update loop now completes every frame (about 59-60 updates per second in all four districts, previously about 29). Hot paths were rewritten as table lookups and small hand-written SM83 routines with C references kept for host tests. Gameplay is unchanged: a differential host fuzz compares the new and original engines step for step, and an emulator checker compares every assembly routine call against a model of its C reference.
- Implemented: a new original actor sheet (136 frames) with eight CGB sprite palettes: orange courier car and uniform, red/blue traffic, yellow taxi and job pin, teal/violet pedestrians and red-and-white TTC vehicles. Transit frames are drawn at scale: bus 40 px, streetcar 64 px (limited by ten sprites per scanline) and ferry 16 x 40 px.
- Implemented: crosswalks are derived from road geometry. Every junction square is rebuilt from the drivable shape so sidewalks turn the corner instead of running across the crossing road, and zebra bars are painted only on arms that link two sidewalks. Lane dashes stop short of junctions.
- Implemented: buildings gain seeded rooftop equipment (water tanks, HVAC, skylights, solar, bulkheads, gravel, antennas); empty walkable lots get lawns, canopy trees, plazas or striped parking. Background palette slot 6 becomes parks and trees (`E7DECC/8FB56A/4D7A52/172B38`, previously a tan ground tone); the wide-warehouse style uses the terracotta slot instead. No collision value changed in any district.
- Implemented: closing the city map restores the scene's CGB bank-1 background tiles that the atlas borrows.
- Superseded on 2026-10-06 (see below): curbside knock-over props: traffic cones, garbage/recycling bins, newspaper boxes and construction barrels. `create_street_life.py` places 96 per district in the gutter of straight blocks, clear of stops, parking anchors and district seams; four nearby props are shown at once. Driving into one knocks it over, costs a quarter of the current speed and plays the impact cue. There is no cargo damage, fine or collision change, and props are not saved: a knocked prop stands again once it has left the view.
- Implemented (user direction: visible, scheduled transit that holds the courier): the bus, the Queen streetcar and the Island ferry now appear at their stops. While the courier waits, the scheduled vehicle decelerates into its berth so it stops exactly as the existing two-second boarding window opens. The courier boards (hidden while aboard), the vehicle pulls away, and at the destination a vehicle sets the courier down, waits 1.5 s and leaves. Streetcars and buses use the lane for their travel direction; the ferry berths in open water beside its dock. Timetables, fares, journey durations and the save format are unchanged; vehicle positions are derived from the timetable clock. Line 1 stays underground, so no subway train is drawn on the street. Vehicles are drawn at scale (bus 40 px, about 12 m; streetcar 64 px, limited by ten sprites per scanline; ferry 16 x 40 px). Props hide while a vehicle is on screen so actors never need more than 38 of the 40 hardware sprites.
- Implemented: the HUD repaint after the once-per-second clock tick happens on the next frame (16 ms later), so the save and the repaint never share one frame; the western street-name lookup scans exact per-region candidate lists. Both keep the same state and text.
- Proposals, not adopted: riding along with the camera following the vehicle between stops, pedestrians visibly boarding, and a visible GO/Line 1 train at surface rail corridors.

## Menu, HUD and title art, 2026-10-06

User direction: dress up the plain menus with good Game Boy menu art, improve the art overall, keep performance and a lively city.

- Implemented: an original bold UI font (2-pixel stems) replaces the starter glyphs in the HUD and menus. Original 8x8 art tiles (frame, separators, animated cursor, menu and HUD icons, A/B/SELECT/START button glyphs, compass arrows, CN Tower emblem) live in otherwise unused bank-0 tiles 128..191; text rows can mix glyphs and art through character codes 0x80 and up.
- Implemented: pause menu, dispatch board, TTC timetable, result and title screens use framed cards with icons; the menu cursor and title prompt animate. The UI palette (BG slot 7) becomes paper, amber, red and ink; the city map keeps its original colours by swapping them in while it is open.
- Implemented: the title screen shows an original dusk skyline with the CN Tower, Rogers Centre dome and a 16x16-letter logo. It borrows the atlas-owned bank-1 tiles while the full-screen title hides the city and restores the scene tiles on entry.
- Implemented: the HUD shows icons for money, vehicle, progress, deadline and condition, button glyphs for its hints, and a compass arrow in the street row that points to the job beacon (a ring when close). Only the compass cell repaints between HUD updates.
- Implemented: during a job's last ten seconds the HUD clock flashes red each second.
- Superseded on 2026-10-06 (see below): an ambient herring gull glides across the view every 10 to 18 seconds using the last free actor slot. It is decorative only; with it the worst case is exactly 40 hardware sprites.
- Unchanged: controls, menu order and actions, text meaning, timers, saves and the map's behaviour.

## Accepted GTA-style street life and stash migration — 2026-10-06

The user found the main-branch build's driving poor (the car could spin while
stopped), collisions ineffective, pop-in of street objects, and no weapons. They
asked for a GTA-like city at 50–60 FPS: punches and better use of A/B, officers
walking the streets, varied houses and taller buildings to pass behind, a
quest indicator, hospital recovery, arrests with fines and police that escalate
from arrest to armed response as chaos grows. The stashed
`codex/visible-queen-streetcar` work (its 23-point feedback list and the
hospital/arrest/weapons decisions) is the requirement source; its art is not
used and its gameplay was re-implemented on main's 60 Hz engine.

- Driving: no yaw at rest; steering rate grows with speed and reverses when
  backing up; throttle tapers near top speed; B stops the car before reverse
  engages; A+B is a handbrake and, held at rest, leaves the vehicle. Cars may
  use sidewalks and open lots (collision 16); buildings, water and rails (15)
  stay solid. This supersedes "sidewalks are not free shortcuts".
- Collisions: courier–vehicle impacts use mass and closing speed with
  restitution 3/4. The struck car slides, may spin and eases back into its lane;
  the courier keeps or loses speed accordingly and rebounds from head-on hits.
  Traffic never drives into the courier's car. Struck pedestrians tumble through
  the air, bounce off walls and lie down (fatal at speed); the car loses a
  quarter of its speed per person.
- On foot: A enters the own car, otherwise steals a nearby road vehicle (the
  driver runs off) or punches; B uses TTC at a station, otherwise fires the
  pistol (12 rounds to start). Pause "Supplies $20" refills 12 rounds and heals;
  progress keeps auto-saving.
- Police (tuned down and partly superseded by the revision below): a quarter of
  walkers are capped officers and one road slot is a patrol car. Witnessed crimes raise stars up to a ceiling by severity (scuffle 1,
  gunfire 2, killings and attacks on officers more); unseen chaos still adds
  stars. One star cools per 20 s out of sight. Up to two stars officers and the
  patrol car arrest on contact: $50 per star (cash floor zero), half the ammo
  confiscated, any job lost. From three stars the patrol car holds off and armed
  officers shoot; zero vitality shows WASTED and recovery at the core hospital
  forecourt (504,344) for up to $100.
- Presentation: on-screen objective pointer at the view edge, bobbing beacon
  when in view; HUD stars, vitality and ammo; new walkers and curb props only
  appear outside the camera. Ten core, six West, nine High Park and five East
  towers have 24-px upper floors overhanging the street with BG priority;
  the northern core row is detached gable-roofed houses. Footprints and
  collisions are unchanged.
- Save v7 keeps the 58-byte layout: vitality, ammo, stars and heat replace the
  unused v6 atlas-cursor words; v4–v6 records migrate with full vitality, 12
  rounds and no stars.
- Not migrated from the stash yet: controllable boats, shop interiors, fire and
  ambulance traffic, parked-car lots, social pedestrian pairs, police
  helicopter, story chapters and the extra districts. The actor sprite sheet
  uses 122 of its 128 VRAM tiles.

## Street life, police and art revision, 2026-10-06

User direction: make the police less aggressive (pursuit was too fast and killed the courier quickly); fix the items that appear on the map, look like power-ups, cannot be used, float in the middle of nowhere and are too many; make the cars look better; more pedestrians, fewer police and more NPC cars without hurting performance; fix and improve the title artwork; keep improving gameplay, performance and art.

- Accepted (user direction): police pursue more gently. The patrol car covers 0.7-0.9 px per update (the courier's car 1.5 px at full speed), starts one second after it appears and only catches up behind the courier after several seconds out of view from two stars. Officers on foot are slower than a walking courier below four stars. An arrest needs about 1.5 s of sustained contact; beside a courier on foot the patrol car stops and lets an officer out instead of arresting. Officers shoot from four stars (previously three), the patrol car only at five, one shot about every 1.5 s for 4-5 damage (halved in a vehicle). Unseen chaos needs 16 points per star and stops at four stars; witnessed crimes add one star at a time up to a ceiling (scuffle 1, gunfire 2, killing 3); attacking an officer adds one star, killing one brings at least three; only a hard ram counts as attacking the patrol car. Each star cools after 12 s without police nearby (previously 20). Arrest fines are $25 a star and the hospital bill at most $60.
- Accepted (user direction: fewer police): one walker route in eight wears a police uniform (previously two). Traffic slot 4 is an ordinary blue car; it becomes the patrol car only while out of view when a pursuit starts and changes back the same way after the pursuit.
- Accepted (user direction: the floating items): the ambient gull and the 96-per-district curbside props are removed. In their place, 12-18 real pickups per district lie on the pavement beside straight curbs, at least 144 px apart and never under a roof lip, canopy or overhanging floor: cash (+$15), first aid (+40 vitality, left in place at full vitality) and ammunition (+6 rounds, left in place when full). Walking or driving over one collects it with a message and cue. Two nearby pickups are shown at once; the last eight collected stay away until eight others have been taken. Nothing is saved. This replaces the earlier "many destructible things" props; bringing back a few clearly readable knock-over props is a separate option.
- Accepted (user direction: more pedestrians and NPC cars without hurting performance): eight walker slots instead of six. Each person now uses one 8x16 hardware sprite, so the same 40-sprite budget fits. The fixed route pool rises from 486 to 620 (core 140, other districts 160 each); a refresh re-picks at most two walkers, the scan window matches what can be visible, and empty slots only look for a route again after the courier moves. Traffic keeps six vehicles but a vehicle that drifts far out of view rejoins its own loop just beyond the screen edge, heading toward the courier. Measured in PyBoy with attention held at zero against `main` at `fb20a4e`: driving, cars on screen 0.03 -> 0.40 on average and 58.5 -> 57.5 updates per second; walking, pedestrians on screen 2.93 -> 4.18 and 59.8 -> 59.6 updates per second.
- Accepted (user direction: better car art): sedans, taxi, van/truck and patrol car are redrawn from layered east-facing designs with a dark cabin ring, raked windscreen, light upper flank, headlamps and tyres; south is the transpose and the 45-degree view a supersampled rotation, so all eight headings share one design. The patrol car is white with blue door stripes and a roof light bar.
- Accepted (user direction: title artwork): the title is redrawn as a full-width dusk scene (rows 0-10): banded sky, stars, the CN Tower, Rogers Centre dome, a lit skyline, a setting sun with retro bands and its reflection on the lake, and the courier car on the shore road, under a cream-and-gold logo. Seven title palettes replace the scene's BG palettes 0-6 while the title is shown and the scene's own palettes return when play starts. The controls card is four lines plus a blinking PRESS A TO START prompt. The title's tile cache is reloaded after every scene load, so a reset into a saved remote district shows it correctly.
- Implementation: GB Studio's unused trigger and projectile pools are reduced to one entry each through file-backed engine fields (`MAX_TRIGGERS`, `MAX_PROJECTILES`, `MAX_PROJECTILE_DEFS`); no scene uses triggers or projectiles. This restores the 1,024-byte stack-reserve guard that the street-life builds on `main` missed (911 bytes) and leaves room for the added slots.
- Unchanged: controls, driving physics, save format (58-byte version 7), contracts, transit and the city map.

## Day/night cycle and animation, 2026-10-06

User direction: send several screenshots when a task is done (added to `AGENTS.md`); add more animations when things happen (interacting, driving and so on) while keeping the game fast; add a day/night cycle; make sure the art looks good.

- Accepted (user direction): a day/night cycle on the play clock. One game day lasts 1,024 play seconds (about 17 minutes) and a new game starts at 08:00; saved games keep their time because the clock is the saved play time. Day 07:30-17:00 uses the registered scene palettes unchanged; golden hour (18:30) warms highlights and cools shadows; dusk (19:45) turns violet; night (21:00-05:00) has navy roads, sodium-lit sidewalks, dimmed facades and dark parks; dawn (06:00) is lilac. 64 steps of 16 seconds are interpolated by `scripts/create_daynight.py` into 18 unique palette sets (BG palettes 0-6 and all eight sprite palettes); the UI palette 7 is never tinted, so the HUD, menus and city map keep their colours. Sprites dim less than the city and the courier least; the beacon, taxi and pickup yellow does not dim. Palette steps are applied at the start of the next update; scene loads and leaving the title apply the current time directly.
- Accepted: the pause menu's clock shows the time of day with a sun or moon icon instead of total play time.
- Accepted (user direction: animations when driving): tyre smoke behind the rear wheels on hard braking (above speed 14), on the handbrake and when the tail slides; an exhaust puff on each launch from rest; dust off the bumper after a hard wall or kerb strike and a sparkle at the contact point of a car impact; from 19:00 to 06:30 the courier's vehicle shows headlamp beams (dithered light 7-21 px ahead) in all eight headings; the patrol car's light bar flashes while it pursues; the camera looks up to 24 px ahead across and 16 px down the screen of a moving vehicle, easing a pixel every other frame, and recentres on foot.
- Accepted (user direction: animations when interacting): punch and pistol poses for the courier on foot, with a muzzle flash; a collected pickup's icon rises 16 px and blinks out; a parcel pops up over the courier at each job pickup or intermediate drop; a coin and sparkle rise over the courier after a delivery (seen when the result card closes).
- Implementation (performance kept): two particle actors (21-22) and the `MAX_ACTORS` engine field raising GBVM's pool from 21 to 23. Particles join GBVM's active actor list only while shown; the animation layer is one banked call per update with table-driven ages. Headlamps are drawn by 32 lit vehicle frames that reuse the vehicle and beam tiles in one metasprite instead of an extra actor; the sprite canvas is lifted 16 px with a matching origin so GB Studio's frame mask does not clip beams below a south-facing car (compiled offsets of all other frames are unchanged). The sidewalk-pickup sweep now runs every fourth update (every update during the fade-in), which more than pays for the animation layer. Measured pacing is unchanged by day (57.86 updates per second over eight driving seeds) and 57.38 at night; see `TESTING.md`.
- Unchanged: controls, driving physics, save format (58-byte version 7), contracts, transit, police tuning and the city map.
- Pending human checks: day length, night readability on the Chromatic screen, smoke frequency and the camera look-ahead on the handheld.

## Working defaults and pending proposals

- Working title: **Toronto Dispatch**.
- First district: a compressed downtown area connecting Union Station, St. Lawrence Market, and the Distillery District.
- Fictional dispatch company, fictional pickup businesses, and original landmark artwork.
- One car and three package jobs for the first playable milestone, followed by the other vehicles and passenger jobs.
- Straight north-up pixel artwork and matching native collision grid, with CGB background priority for roofs and canopies.
- Implemented controls: left/right steer the moving vehicle; A accelerates; B brakes, then reverses after a short hold at rest; A+B is a handbrake and, held at rest, leaves the vehicle; Select interacts at pickup/drop-off; Start pauses. On foot, the D-pad walks; A enters the parked car, steals a nearby road vehicle or punches; B opens transit at a station or fires the pistol. Handheld comfort still needs human playtesting.
- A documented baseline transit map rather than changing live detours. The map era is not yet selected.
- The [researched expansion plan](docs/OLD_TORONTO_EXPANSION.md) proposes 17 linked native districts and a 2026 map baseline. That full layout remains a proposal; the historical three-scene prototype and four-scene milestone use their own compressed layouts. The user has not adopted the proposed era; full Old Toronto, its waterfront and Islands are the accepted scope.

## Unresolved implementation questions

- The native TORONTO scene extension builds and runs. Continue tuning driving, occlusion, input responsiveness and crowded-scene performance on the handheld.
- Remaining district seams and western/eastern handoffs, final-candidate paid-transit recovery, crowded-actor performance, physical save recovery and audio mix after listening. The four-scene build, memory guard, nine-job progression, Withrow parking cue and remote High Park reset have sampled native evidence; they do not complete the other acceptance gates.
- Car handling parameters, realistic traffic-rule penalties, and mission time budgets after playtesting.
- Exact hardware/cartridge edition and Developer Mode readiness.
- Island delivery transport: ferry/on-foot or specifically authorised service-vehicle jobs, consistent with researched access rules.

Material changes to accepted presentation, driving feel, or hardware target require a design discussion. Routine tuning and reversible implementation choices can proceed autonomously.
