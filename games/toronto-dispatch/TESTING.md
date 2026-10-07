# Testing record

## Parks and drivable ground — candidate, 2026-10-07

Same toolchain as below; CLI build and PyBoy evidence only. "Previous" is the spray-bay candidate `53602650…`.

- ROM: `make:rom` of this source, 524,288 bytes, SHA-256 `787de9dde19cbb97bc5b42a6d83496bf6c2cf560ab7e956afb22c241e6ea0bc6`. Memory guard passes: heap `DAFC`, stack `DF00`, 1,028 bytes of reserve (unchanged). Scene tiles (flip-canonical): core 382 of 384 (was 372), West 276 (256), High Park 238 (225), East 230 (202). City atlas: every 20x12 view at most 171 of 172 tiles (was 164).
- `make check` passes: 4,508 host engine checks (new: the bus keeps to asphalt for 12,000 updates, round Queen's Park Crescent too; bus steps may move both axes), 8,064,549 atlas UI checks, the campaign, district-world, story and radio checks, and the art generators' checks: park features lie on park ground, never on a road or path; the core stays within 384 tiles; district scene collisions equal the art metadata.
- Collision: downtown solid tiles fell from 5,705 to 4,750 (yards, lots, lawns and plazas opened); road tiles 7,294 -> 7,259 (University Ave north of College replaced by the crescent).
- Content: eight contracts' road routes changed with the crescent (three time limits +5 s, rewards +$1 to +$3); story, stops and the save format unchanged. Lost parcels moved with the opened ground: downtown to the Queen's Park lawn, a Cabbagetown yard, a plaza by the Port Lands, Fort York's grounds and Hanlan's Point, and one West parcel onto Sorauren Park's field. Parcels keep their numbers, so ones already found stay found.
- Emulator scenarios (PyBoy; inputs and memory writes in `docs/screenshots/provenance.json`): the Legislature seen from College at University; inside Queen's Park; the crescent's north end with the ROM; a car driven east along Hoskin Ave crosses the crescent onto the park lawn at speed 16 and keeps going; Trinity Bellwoods, Allan Gardens and Grange Park from their sidewalks; Sorauren Park, Grenadier Pond and the zoo paddocks after driving into the West and High Park scenes; Withrow and Greenwood parks after driving into the East scene. The same inputs on the previous build show the old parks for comparison. The HUD names Queens Park Cres and Hoskin Ave on the new streets.
- Pacing, PyBoy, 3,600 frames per route with attention held at zero, previous -> this build: driving 56.04 -> 56.42 updates per second over the usual ten routes, walking 58.88 -> 59.08 over four; ten more driving routes 57.76 and after the welcome call 56.80 (57.50 and 56.90 on the gameplay-and-scenery build); parked 59.44 and 59.98, unchanged.
- Not verified: hardware or cartridge display and timing, plugin build, and human judgement of whether the parks read as themselves and of driving across open ground.

## Spray bays in every scene and the parcel count — candidate, 2026-10-07

Same toolchain as below; CLI build and PyBoy evidence only. "Previous" is the gameplay and scenery candidate `2d2246d2…`.

- ROM: `make:rom` of this source, 524,288 bytes, SHA-256 `536026505bfe732f0c5778d5f1c035c5cc1fae308123da4b972dec2e9155acde` (the build tested below; a rebuild of the same source differed only in the save signature and header checksum, as noted in the next section). Memory guard passes: heap `DAFC`, stack `DF00`, 1,028 bytes of reserve (unchanged; the bay table is in ROM). Scene tiles (flip-canonical): core 372, West 256 (was 240), High Park 225 (208), East 202 (185). The core art is pixel-identical after moving its bay to the shared drawing.
- `make check` passes: 4,507 host engine checks (new: each scene's bay clears the stars for $25 and nothing happens outside the four scenes), 7,414,355 atlas UI checks (new: the pause menu's "LOST PARCELS 3/20" row, its menu rows one lower), the story and radio checks with the reworded police calls, and the art generators' new checks: every bay lies on asphalt, its door sits on a building's front, and its centre matches `td_spray_at`.
- Emulator scenarios (PyBoy; inputs and memory writes in `docs/screenshots/provenance.json`): driven across the scene edges into the West, then High Park, and separately into the East. In each scene, with two stars and $120, nearing the bay shows "SPRAY BAY: PULL IN" and stopping in it shows "RESPRAYED -$25" with stars cleared and $95 left. In High Park, Rosa's police call reads "OR HIT A SPRAY BAY. EVERY AREA HAS ONE." Collecting the Queen's Park parcel and pressing Start shows "LOST PARCELS 1/20" on the pause menu.
- Pacing, PyBoy, 3,600 frames per route with attention held at zero, previous -> this build: driving 55.94 -> 56.04 updates per second over the usual ten routes, walking 58.88 -> 58.88 over four (the change adds one table read per update while driving).
- Not verified: hardware or cartridge display and timing, plugin build, and human judgement of the bay locations and the taller pause menu.

## Gameplay, scenery and performance — candidate, 2026-10-07

Same toolchain as below; CLI build and PyBoy evidence only. "Previous" is the story candidate `bef81949…`.

- ROM: `make:rom` of this source, 524,288 bytes, SHA-256 `2d2246d29e0ab8b747f97453a7b13963785a2bebc107790ebf13917b77729b56`. Memory guard passes: heap `DAFC`, stack `DF00`, 1,028 bytes of reserve (guard 1,024; previous 1,026). The dispatch board's offer now shares the active job's record (restored when the board closes), which saved 40 bytes for the scenery state, spray bay and walker flag; found parcels use spare save bits. Radio text: 75 scripts in 558 one-card pages across two ROM banks. Scene tiles: core 372 of 384, West 240, High Park 208, East 185 (flip-canonical). The pacing runs used an earlier build of the same code. Builds differ only in GB Studio's four-byte save signature (`game_signature.c`, a hash of GB Studio's loaded project data, which changed between two builds even with no source change) and the header checksum. On this ROM two of those routes give identical results and all 17 screenshots are byte-identical.
- `make check` passes: 4,503 host engine checks, 7,411,502 atlas UI checks and the other suites. New engine checks: scenery tiles found in a scene's tilesets and placed where GB Studio loads them; the vertical-blank handler copying an armed chunk, eight tiles at a time, into the right VRAM bank and restoring the interrupted bank; nothing copied in menus or the map; the four frames repeating. Others: a lost parcel paying $50, counting, saving and staying found; the 20th paying the bonus; save validation accepting parcel bits but no other bits above the contracts; the spray bay clearing stars, heat and damage for $25, refusing without cash and repainting a stolen car; a gunshot sending nearby walkers running; walkers laid out on alternate updates. New UI checks: "LOST PARCEL 3/20", the status line after a second on foot and three seconds in a vehicle, the dented and early delivery remarks, mid and festival chatter only after their chapters, the police call's job variant. `check_story.py` also proves staged chatter mentions only facts heard before its chapter. `create_scenery.py --check` asserts every first-frame tile is textured and unique under flips.
- Emulator scenarios (PyBoy; inputs and memory writes in `docs/screenshots/provenance.json`): open water by Centre Island changes between frames 16 updates apart, and the Don and the West scene's lake are textured; the Yonge-Dundas screen shows its frames in turn; Front St roofs and the harbour no longer show UI glyphs (the previous build, same inputs, does); with two stars, nearing the King St West bay shows "SPRAY BAY: PULL IN", stopping in it shows "RESPRAYED -$25" with stars cleared and cash $120 -> $95, then Rosa's "NEW PAINT. COPS LOST YOU."; walking onto the parcel on the Queen's Park lawn shows "LOST PARCEL 1/20" and cash $30 -> $80; after a shot, nearby walkers switch to running (state read from memory; the frames do not show it clearly). In 900 frames of driving and walking, every scenery chunk copy started on line 146 and returned on line 151, inside vertical blank (lines 144-153), and 1,392 samples of the 24 animated VRAM tiles each matched one of their four frames.
- Pacing, PyBoy, 3,600 frames per route with attention held at zero, previous -> this build. Driving from boot: 55.29 -> 55.94 updates per second over the usual ten routes and 56.03 -> 57.50 over ten more; driving after the welcome call ends: 55.03 -> 56.90 over ten; walking 58.58 -> 58.88 over four. Parked from boot with no input: 59.20 -> 59.44 while a call types and 59.87 -> 59.98 otherwise, with more idle time at the vertical blank (median 45/46 -> 48 and 48 -> 50 scanlines). Profiled walker layout fell from about 34 to 22.5 profiler units per update. An intermediate build that copied tiles with GBDK's `set_bkg_data` during the update ran 53.73 on the ten driving routes. A second intermediate build copied with `memcpy` in the handler; its eight-tile chunks overran vertical blank by several lines, so it was rewritten in assembly.
- OAM: no new actors (parcels use the pickup sprite, panicking walkers their own); not remeasured.
- Not verified: hardware or cartridge display and VRAM timing, plugin build, and human judgement of the water and screen animation, parcel placement, spray bay price and status-line timing.

## Story overhaul — candidate, 2026-10-07

Same toolchain as below; CLI build and PyBoy evidence only. "Previous" is the pop-up HUD candidate `6af837c1…`.

- ROM: `make:rom` of this source, 524,288 bytes, SHA-256 `bef819490bf15b3088ba8dac0dda719b03d830914ae22eaeaa4004e6d48432eb`. Memory guard passes: heap `DAFE`, stack `DF00`, 1,026 bytes of reserve (guard 1,024; previous 1,040). The story queue first left exactly 1,024; removing write-only radio state recovered two bytes. Radio text: 47 scripts and 264 contract calls in 521 one-card pages across two ROM banks.
- `make check` passes: 4,422 host engine checks (new: contract 9 waits for contract 7 at six deliveries, dispatch skips it and a take is refused as locked, finishing contract 7 opens it; a delivery hands the radio its contract, the count and the chapters open before it; a failure is not a delivery), 7,410,743 atlas UI checks (new: the board's "AFTER JOB 7", "READY TO TAKE" and "NEEDS 6 DONE" rows; the delivery call first, then the rival who follows contract 9; contract 7 at five deliveries opening nothing until the sixth; contract 7 opening the chapter at six; a replay moving nothing on; the contract's beat before the count's; the finale after the last delivery), the story checker (88 contracts with briefing, pickup and delivery calls; speakers, places, route order and story order), campaign checks with story-order deadlock closure, and radio freshness (every line fits one card).
- Emulator scenarios (PyBoy; inputs and memory writes in `docs/screenshots/provenance.json`): the welcome and the first job's briefing, pickup and Sal's answer under her own card; with six deliveries but contract 7 open, the board shows contract 9 as "AFTER JOB 7"; delivering contract 7 plays Margo's answer and then the second chapter's opening; delivering contract 9's three drops plays Marco's thanks and then Dev's first appearance and Rosa's reply; delivering the ledger (contract 55) plays Margo tearing up Vance's offer, then the festival-week chapter with Dev quitting under "DEV - RUSHLY" and thanking Margo under "DEV - THE DEPOT".
- Pacing, PyBoy, 3,600 frames per route with attention held at zero, previous -> this build. Driving from boot: 56.29 -> 55.29 updates per second over the usual ten routes and 56.54 -> 56.03 over ten more; driving after the welcome call ends: 55.26 -> 55.03 over ten; walking 58.85 -> 58.58 over four. Seeded inputs give different paths once any timing changes (one route ran 43.7 with heavy traffic on this build; another ran 50.9), so single routes move by several updates per second between builds. Parked from boot with no input, where paths cannot differ: 59.43 -> 59.20 while a call types (the welcome call is longer now) and 59.91 -> 59.87 otherwise, with the same median idle time at the vertical blank (45/46 and 48/48 scanlines). Story calls add only a queue check at the end of a call and a few table reads per delivery.
- OAM: no sprite changes; not remeasured.
- Not verified: hardware or cartridge display and timing, plugin build, a full campaign playthrough in the emulator, and human reading of the dialogue, call pacing and the story-order gates.

## Pop-up HUD — candidate, 2026-10-07

Same toolchain as below; CLI build and PyBoy evidence only.

- ROM: `make:rom` of this source, 524,288 bytes, SHA-256 `6af837c18c6afedf7b68abc9131e048eeaabdf57e214a1feac5879923ed96265`. Memory guard passes: heap `DAF0`, stack `DF00`, 1,040 bytes of reserve. The first attempt overflowed `td_ui.c`'s bank by 106 bytes and, after moving the timing code to `td_hud.c`, left 1,020 bytes of reserve; storing station positions as bytes restored 1,040.
- `make check` passes: 4,418 host engine checks (new: every boarding stop in every scene is offered from its sidewalk sign, none away from stations; the camera has no HUD offset), 7,408,583 atlas UI checks (new: nothing shown in free roam with nothing to report, a job's next stop and status rising on start, the radio card above the pop-up rows, a shot showing only the ammunition and its fading, standing still showing the status line and moving hiding it, low vitality staying, wanted stars, notices; distance in the job row), and the other suites.
- Emulator (PyBoy; inputs in `docs/screenshots/provenance.json`): driving with nothing to report leaves the whole screen to the city; a kerb strike pops up its notice; turning onto Spadina pops up "SPADINA AVE"; accepting contract 1 raises Rosa's card above the next stop and the job row, which clear once the call ends and the car is moving; standing still shows cash and ammunition; a shot shows the ammunition only.
- Pacing, PyBoy, 3,600 frames per route from boot, previous build (`b72a8adc…`) -> this build: driving 55.40 -> 56.29 updates per second over ten routes, walking 58.70 -> 58.85 over four. The pop-up tick costs about 1.1 scanlines per frame while driving (1.7 before repaints were skipped when nothing visible changed).
- OAM, 900 samples: driving at most 15 hardware sprites and 7 on one scanline, walking 15 and 6, shooting 14 and 5; none over 10.
- Not verified: hardware or cartridge display and timing, plugin build, and human judgement of pop-up timing and legibility.

## Realistic downtown, aiming, bigger cars and transit — candidate, 2026-10-07

Same Linux toolchain as below (GB Studio CLI 4.3.2 from source, GBDK 4.5.0, PyBoy 2.7.0); CLI build and emulator evidence only, not plugin, streamed-device or cartridge evidence. "Previous" is the round-3 candidate `3f5574b5…` below.

- ROM: `make:rom` of this source, 524,288 bytes, SHA-256 `dfb4611519d480ac0d2bcb247cb34a140a57f7fcb394c5e128858752cb85a20f`. The screenshot build `b72a8adc…` is the same source; the two differ only in four bytes at `0x04A5`-`0x04A8` and the header checksum, which change on every build. Memory guard passes: heap `DAF0`, stack `DF00`, 1,040 bytes of reserve (guard 1,024). Actor sprites: 210 frames, 128 of 128 8x16 tiles. `td_content.c` overflowed its bank by 550 bytes once core street names joined the district tables (BankPack error); the street-name tables moved to `td_street_names.c`. A stray brace in `td_ui.c` (not compiled by the host harness) failed the first ROM build and was fixed.
- Core scene: 165 buildings, 326 flip-canonical background tiles (limit 384); city atlas worst 20x12 viewport 164 of 172 tiles (an unsnapped first layout needed 183, so north-south streets were snapped to 64 px and park trunks made non-blocking).
- `make check` passes: 4,396 host engine checks (updated fixtures for Union, the rail corridor, Bathurst and Parliament signals and lanes, the 13 px car footprint, the 32-second 501, fare refusal on confirmation, menus skipping the origin; new: a held B keeping a lock while strafing, release dropping it, the 60-degree cone, and the patrol car's red/blue palette flash), 7,408,508 atlas UI, 299,324 atlas, 609,452 transit, 2,974 bridge and 16,997 navigation checks, campaign (88 contracts, 51 stops; every core stop car- and foot-usable), streetcar, district world and all generator freshness checks.
- Emulator scenarios (PyBoy; inputs and memory writes in `docs/screenshots/provenance.json`): the new game starts on Front St at Union Station; the HUD names Front St, Mill St, Spadina Ave, Queens Park, Queen St and Bathurst St from the generated lookup; teleports show the CN Tower and Rogers Centre, City Hall with Old City Hall and the Eaton Centre, the Flatiron and St Lawrence Market, Kensington and Chinatown along Spadina, the Legislature, the Distillery by the Don and Liberty Village with Fort York. Leaving the car at Union shows "B LINE 1 TRAIN" and B opens the menu on King Station. At the Queen and Yonge platform the 501 pulls in from the west before boarding; at the ferry terminal the ferry comes in over the harbour. On foot the pistol locks on, keeps the lock while B is held and the courier strafes, and with nothing to lock fires along the aim.
- Pacing, PyBoy, 3,600 frames per run from boot with attention held at zero, previous -> this build, four routes each: driving 57.15 -> 56.33 updates per second, walking 58.25 -> 58.70. Before the station cache the walking figure was 55.1 (the HUD prompt scanned all 51 stops); the cache fixed it. The driving cost comes mostly from three- and four-sprite cars.
- OAM, 900 samples: driving at most 15 visible hardware sprites and 8 on one scanline, walking 15 and 6; no scanline over 10.
- Not verified: hardware or cartridge display and timing, plugin build, human playtesting of the new streets, block legibility on the Chromatic screen, handling with the larger footprint, lock-on reach, transit pacing and audio.

## City life, menus, story radio, shooting and car damage — candidate, 2026-10-06

Same Linux toolchain as the entries below (GB Studio CLI 4.3.2 from source, GBDK 4.5.0, PyBoy 2.7.0); CLI build and emulator evidence only, not plugin, streamed-device or cartridge evidence. "Previous" is the day/night candidate below, rebuilt from `744381a` (`ff2837b1…`).

- ROM: `make:rom` of this source, 524,288 bytes, SHA-256 `3f5574b5800e68ab08eaeb1737b9a15e2880d8b1b6071badcc4e115cdaa4bdf9`. The memory guard passes: heap `DA9B`, stack `DF00`, 1,125 bytes of reserve with 24 actors. The build folder's `src/core/actor.c` shows both actor draws changed to `move_metasprite_ex` by `actor.c.patch`. Actor sprites: 214 frames, 128 of 128 8x16 tiles in the generator's flip-aware count (the compiled tileset fills both VRAM banks, 64 of 64 8x16 tiles each). `td_ui.c` overflowed its 16 KB bank by 1,844 bytes once the radio was added (BankPack error); moving the radio logic and text to `td_radio.c` fixed it.
- `make check` passes: 4,388 host engine checks (new: traffic looks and palette offsets, the yellow downtown van, lock-on direction and switching, no lock-on while driving, a locked shot turning the courier and hitting within a few updates, the tracer showing at once after GBVM flags the effects actor off screen, the lock-on marker on its target, frames kept after actor activation, radio beats for the first pickup, chapters at six deliveries, west/east routes at three, the last contract, replays, failures, contract briefings and the welcome call, car damage by wall speed, vans' half damage, smoking/failing/wrecked thresholds and top speeds, bonnet smoke, repairs and a stolen car's fresh condition and paint), 7,606,263 atlas UI checks (new: the pause sheet's four-row scrolling, marks, hint and count, the board's chapter header, the result card's fee, the radio card's portrait, name, typing, queued calls on a clean card, the HUD moving below the card and back, police beats, and the distance readout in metres and kilometres), 299,366 atlas API, 609,452 transit, 2,974 bridge and 16,997 navigation checks, nine memory fixtures, and the sprite, UI art, day/night, radio, street-life and route generator freshness checks.
- Emulator scenarios (PyBoy, ordinary buttons plus the memory writes listed in `docs/screenshots/provenance.json`): Rosa's welcome call types out on leaving the title and pages through four lines; the pause sheet scrolls from items 1-4 to 6-9 with up/down marks and its hint follows the cursor; the dispatch board shows the chapter; accepting contract 1 plays its briefing and the HUD shows the distance; on foot the marker brackets a walker ahead and a shot in open air leaves a 32 px tracer travelling 8 px per update; a car at damage 80 smokes from the bonnet and the HUD shows 20; at 22:00 a blue (stolen) car carries its own headlamp beam. A crop collection from about 370 seconds of scripted driving and walking found 13 vehicle looks (sedan, van, motorcycle, taxi, compact, pickup, sports car in several paints) and 10 walker looks.
- Pacing, PyBoy, 3,600 frames per run from boot with attention held at zero, previous -> this build: driving 57.66 -> 56.79 updates per second over seven routes (cars in view 0.43 -> 0.49); walking 59.5 -> 58.9 over two routes. The radio tick runs only during a call, on a change of stars or once a second. Hood smoke is rationed to one puff every 32 updates. A trial that recycled traffic every fourth update and closer to the screen doubled cars in view on some routes but cost 3-4 updates per second, so it was not kept.
- OAM, 900 samples each: driving by day at most 14 visible hardware sprites and 7 on one scanline; at night 17 and 8; walking 11 and 7; walking while firing 16 and 9 (an east-west tracer is four sprites on one band); no scanline over 10.
- Fixed during verification: a particle or lock-on marker drew the courier's car for one frame because GBVM's activation resets the idle animation; the first tracer updates were hidden because GBVM re-checks off-screen actors every fourth frame; a queued radio call typed over the previous page.
- Not verified: hardware or cartridge display and timing, plugin build, human playtesting of radio pacing and chatter, menu legibility on the Chromatic screen, lock-on reach, tracer readability, damage rates and smoke, and audio.

## Day/night cycle and animation — candidate, 2026-10-06

Same Linux toolchain as the entry below (GB Studio CLI 4.3.2 from source, GBDK 4.5.0, PyBoy 2.7.0); CLI build and emulator evidence only, not plugin, streamed-device or cartridge evidence. "Previous" is the street-life candidate below (`836c3af3…`, pushed as `38e9d86`).

- ROM: `make:rom` of this source, 524,288 bytes, SHA-256 `fd5b866a687f8fe5aed7383dcee886313a0455c88328c3295719d780ba5d0ccb`. The memory guard passes: heap `DA48`, stack `DF00`, 1,208 bytes of reserve (GBVM's actor pool is raised from 21 to 23 by the `MAX_ACTORS` engine field). A rebuild of the final tree (`ff2837b1ebd6c826aaed53bd51a8859bbfc88b1675a998b255e36a06e8482320`) differs from it only in GB Studio's four-byte save signature, which hashes project data that includes asset load timestamps, and the header checksum. Actor sprites: 253 frames; the generator's flip-aware count is 125 of 128 8x16 tiles and the compiled tileset uses 61 of 64 8x16 tiles in each VRAM bank (the 32 lit vehicle frames reuse existing tiles).
- `make check` passes: 4,357 host engine checks (new: time-of-day palettes and clock, scene init in the current time of day, particle lifetimes and priority, pops rising 16 px, idle effect actors leaving GBVM's active list, brake smoke behind the car, launch exhaust, no smoke on foot, punch pose and return to walking, camera look-ahead and recentring, lit vehicle frames only at night and never on the parked car, the patrol car's flashing light bar), 299,366 atlas API, 7,676,400 atlas UI, 609,452 transit, 2,974 bridge and 16,997 navigation checks, nine memory fixtures, and the sprite, UI art, day/night, street-life and route generator freshness checks.
- Day/night (PyBoy, play clock written in emulator memory at the vertical-blank wait): 12:00 uses the registered scene palettes unchanged; 18:30 golden hour, 19:45 dusk with headlamps, 22:00 night (navy roads, sodium-lit sidewalks, dimmed facades, lamps on) and 06:00 dawn all render as designed; the HUD and menus keep the untinted UI palette; the pause menu shows the moon and 22:01. A new game starts at 08:00; one game day lasts 1,024 play seconds (about 17 minutes); 18 palette sets cover 64 steps.
- Animation (PyBoy scripted inputs, plus memory writes where listed in `docs/screenshots/provenance.json`): tyre smoke behind the rear wheels when braking hard and on a handbrake slide, an exhaust puff on a launch from rest, punch and pistol poses with a muzzle flash, an ammunition box rising from a collected pickup, a parcel popping up at a job pickup, a coin and sparkle over the courier after delivery, the patrol car's light bar alternating while it pursues, and headlamp beams in all eight headings at night.
- Pacing, PyBoy, 3,600 frames per run from boot with attention held at zero, eight seeds of random driving, previous -> this build: 57.86 -> 57.86 updates per second by day (2-frame updates 3.6% -> 3.4%), 57.38 at night with headlamps; walking 59.60 -> 59.65 (two seeds). The animation layer is one banked call per update (about 1-2 scanlines, more while smoke is alive); throttling the sidewalk-pickup sweep to every fourth update saved about 2.5 scanlines and pays for it. Lit frames put the headlamps in the vehicle's own metasprite instead of an extra actor.
- OAM, 900 samples each: driving by day at most 15 visible hardware sprites and 8 on one scanline; at night 16 and 10; walking 10 and 6; no scanline over 10.
- Pedestrians visible while driving averaged 1.11 against 1.49 on the previous build over the same eight seeds (walking is unchanged at 2.98); trajectories differ between builds, so this is not attributed to a cause yet.
- Not verified: hardware or cartridge display, colours and timing (palette writes wait for an accessible LCD mode in GBDK and are applied at the start of an update), plugin build, human playtesting of the day length, night readability on the Chromatic screen, smoke frequency and camera look-ahead comfort, and audio.

## Street life, police, pickups, cars and title — candidate, 2026-10-06

Linux cloud session without the ModRetro plugin: GB Studio CLI 4.3.2 built from source at `ccb891b2…` (Node.js 22.22.0, Yarn 4.4.1 through Corepack), GBVM `bd6f41cc…`, GBDK 4.5.0 for Linux (archive SHA-256 matches GB Studio's lock), Python 3.13.16, Pillow 12.3.0 and PyBoy 2.7.0. This is CLI build and emulator evidence only; it is not plugin, streamed-device or cartridge evidence. "Upstream" below is `main` at `fb20a4e` (street life and overhanging towers) built with the same toolchain.

- ROM: `make:rom` of this source on top of `fb20a4e`, 524,288 bytes, SHA-256 `836c3af3efb0ec914a05ba725996836265d8c8b8ad4e0589b88193372518fc30`. `scripts/check_rom_memory.py --min-stack-reserve 1024` passes: heap `D9BA`, stack `DF00`, 1,350 bytes of reserve. Upstream fails this guard with 911 bytes; reducing GB Studio's unused trigger/projectile pools restores it.
- `make check` passes: 3,304 host engine checks (new police, pickup and ambient-traffic regressions), 299,366 atlas API, 7,676,328 atlas UI, 609,452 transit, 2,974 bridge and 16,997 navigation checks, nine memory fixtures, and the sprite, UI art, street-life and route generator freshness checks.
- Police, PyBoy, 30 s from setting the wanted level (scripted walking in a small loop, or driving with random steering). Upstream: one star on foot arrested after 5 s; three stars on foot dead after 17.5 s; three stars driving dead after 20.5 s; one star driving escalated to five and died after about 29 s. This build: one and three stars on foot still free at full vitality after 30 s; five stars on foot vitality 10 after 30 s; driving at one, three or five stars vitality 84-92 after 30 s. An earlier candidate of this work (`3207b1fa…`, before the rebase) was arrested after 7 s at three stars on foot with the same script.
- Pickups, courier placed on foot beside table entries in the core (memory write at the vertical-blank wait, then ordinary walking): first aid 50 -> 90 twice and ammunition 12 -> 18 -> 24, each with its message; on `3207b1fa…`, cash $30 -> $45 with FOUND CASH +$15. Generated placement keeps every pickup icon off background-priority tiles (roof lips, canopies and the new overhangs).
- Pacing and density, PyBoy, 3,600 frames per run from boot with attention held at zero, upstream -> this build: driving 58.5 -> 57.5 updates per second, cars on screen 0.03 -> 0.40 and pedestrians 1.59 -> 1.76 on average; walking 59.8 -> 59.6 updates per second and pedestrians on screen 2.93 -> 4.18. Remaining two-frame updates while driving coincide mostly with pedestrian route rescans.
- OAM, 900 samples each of driving and walking: at most 13 visible hardware sprites and 6 on one scanline, no scanline over 10.
- Title: shown from frame 279 after a cold boot with the new illustration, title palettes and blinking prompt; after A the core scene's palettes and tiles are restored. On `3207b1fa…`, after driving into the West End and a soft reset, the title fades in correctly while the game reloads the West End, and play resumes there with West palettes.
- Not verified: hardware or cartridge display and timing, plugin build, human playtesting of police difficulty, pickup economy and car readability, sprite load in transit-heavy views beyond the 40-sprite budget arithmetic, and audio listening.

## GTA-style street life and towers — installed candidate, 2026-10-06

Source `e99bb9f` (street life `f881ba6` plus overhanging towers and houses).
GB Studio CLI 4.3.2 / GBDK 4.5.0 built `project/build/toronto-dispatch-street-life-e99bb9f.gbc`:
524,288 bytes, SHA-256 `b53504d318dee723d58024ac99541a39fea1a0b9f83878b0cf9bef96abadd877`.
`rom_inspect` validates the header; the memory guard passes with 911 bytes of
stack reserve. Actor sprites use 122 of 128 VRAM tiles; background patterns are
242 (core), 210 (West), 184 (High Park) and 185 (East) of 320.

- `make check` passes, including 3,493 host engine checks (20 new street-life
  checks: car theft, punches, pistol and ammo, car strikes on walkers, crime
  escalation, cooling, arrest fines, hospital recovery, supplies, no yaw at rest,
  reverse steering, handbrake exit, rear-end momentum transfer and wall rebound).
- PyBoy 2.7.0, counting completed `toronto_update` calls over 240 frames from a
  fresh start: cruise 58.8, crash-heavy driving 55.5, walking 59.0, walking at
  two stars 56.0, driving at two stars 55.2 and at four stars 53.0 updates per
  second. Main `d6cbcc4` measured 59.8–60.0 on the same recipes.
- Scripted emulator scenarios: a struck walker tumbles and lies down and the
  car loses speed; a head-on hit pushes the other car back and rolls the
  courier's car back; A beside a road vehicle drags its driver out and drives
  off with it; gunfire near officers raises attention; one star ends in a $50
  arrest with half the ammo kept; five stars bring capped officers who shoot
  until WASTED, then recovery at the hospital forecourt for $100; a car on the
  street north of a tower passes behind its overhang.
- Cartridge: fresh supported discovery found one Chromatic (Player 01, no
  diagnostics, conflicts or unmatched functions). The plugin's vendor CLI 1.2.1
  wrote the exact ROM in 47,349 ms and reported success with the matching
  digest. The earlier `f881ba6` ROM was written the same way first. Physical
  cold boot, gameplay, audio and save persistence are not yet observed.

## Latest main cartridge installation, 2026-10-06

Remote `main` was pulled to `d6cbcc4`; unfinished local work was preserved in a
named Git stash. The official plugin built `project/build/toronto-dispatch-main-20261006.gbc`
from that native game source: 524,288 bytes, SHA-256
`229159d83dea5d20a841ae44c4984d0d818d9d4dba2fb92051cf1881a65635cc`.
Header validation and the linked memory guard passed with 1,078 bytes of stack reserve.
`make check` passed after correcting the sprite validator to check its checksum
against the existing PNG bytes while retaining decoded-pixel validation across
PNG encoders. This validator change does not alter the native game inputs.
The host suite reports 3,468 engine checks, zero failures.

On that exact ROM, plugin PyBoy 2.7.0 showed the title at frame 180, gameplay
after A2 / neutral300 at frame 482, movement after A120 at frame 602, and the
new pause menu after Start30 at frame 632. These are bounded smoke checks.

Fresh supported discovery found the intended sole Chromatic without diagnostics,
conflicts or unmatched functions. One supported write succeeded in 47,846 ms;
vendor CLI 1.2.1 exited and closed with code 0. No automatic retry or complete
read-back digest was supplied. Physical cold boot, gameplay, audio and power-off
save persistence remain pending. The [sanitized installation record](docs/CARTRIDGE_MAIN_INSTALL_2026_10_06.json)
keeps this build and write separate from previous cartridge evidence.

## Menu, HUD and title art, gulls — candidate, 2026-10-06

Same Linux toolchain as the entry below (GB Studio CLI 4.3.2, GBDK 4.5.0, PyBoy 2.7.0); emulator evidence only, not plugin, device or cartridge evidence.

- ROM SHA-256 `a02419327e720df7b5e1ab14a3516cfef2a14fa4d572aa82d30dbbc3eb7bcd22`, 524,288 bytes; memory guard passed with 1,078 bytes of stack reserve. Actor sprites use 104 tiles per VRAM bank, below the UI art at tile 128.
- `make check` passed, including the new `create_ui_art.py --check`, 3,468 host engine checks (gull regressions added) and the atlas UI regressions, which now also check the map palette swap and that the map leaves the bank-0 UI art untouched.
- Frame pacing as below: core 59.8, West 59.8, High Park 59.9, East 59.7 updates per second, with 3, 3, 1 and 5 late frames in 900. The HUD repaint costs about 18k cycles, down from about 31k before this round, because unchanged inputs skip formatting and the street lookup and rows convert in assembly.
- Assembly checker on this ROM: 57,543 calls across all four districts, including the new row copy and conversion routines, 0 mismatches. Map open/close leaves every visible background cell identical to a run without the map in all four districts.
- Title: while the title is shown, 69 of the core scene's 77 bank-1 tiles are borrowed; after pressing A all 77 match the scene's tileset again.
- Screens captured in the emulator: title, driving HUD with compass, pause menu, dispatch board, job HUD, TTC timetable, waiting and riding HUD, result. A gull crossed the High Park view during a 1,500-frame sample.
- Not verified: hardware or cartridge display (including how the first window row looks on hardware; PyBoy hides its top pixel row at WY 0), plugin build, human readability review of the new font and icons.

## Performance, city art, props and visible transit — candidate, 2026-10-06

Built in a Linux cloud session with GB Studio CLI 4.3.2 (`ccb891b2…`, built from source with Node.js 22.22.0), GBDK 4.5.0 (Linux), Python 3.11.15, Pillow 12.1.1 and PyBoy 2.7.0. The plugin was not available there, so this is CLI build and PyBoy emulator evidence only; it is not plugin, streamed-device or cartridge evidence.

- ROM: `make:rom`, 524,288 bytes. Measurements below used SHA-256 `adca1fb6ca714fbc368ca14a29430a7ff18376805b94da878e37f34f2c0e0390`; a rebuild of the committed tree is `42805aceac553d247ed412ea5ec0158135c8c32463bca173a1e04bb5355962f1`. They differ only in GB Studio's per-build `_save_signature` (4 bytes at 0x4A5) and the header checksum; the source between them changed only a comment. `scripts/check_rom_memory.py --min-stack-reserve 1024` passed with 1,188 bytes of stack reserve.
- `make check` passed, including the new `create_city_art.py`, `create_west_art.py`, `create_sprites.py` and `create_street_life.py` freshness checks and 3,463 host engine checks (new prop and visible-transit regressions included).
- Frame pacing, PyBoy, 900 frames of randomized driving from a saved state in each district, counting completed `toronto_update` calls: core 59.7, West 59.7, High Park 59.9, East 59.7 updates per second, with 5, 4, 1 and 4 frames over budget. The same method measured about 29 per second on the ROM before this work. Most of the gain comes from a 60 Hz update loop: hot paths were moved into table lookups and nine small SM83 routines.
- Gameplay equivalence of the optimization: a host differential fuzz ran the original and optimized engines in lockstep with identical results (before props and transit were added). On this ROM, an emulator checker compared every call of each assembly routine against a Python model of its C reference: 57,586 calls across all four districts, 0 mismatches.
- City map: after opening and closing the map, the visible background cells decoded from VRAM match a run that never opened it, in all four districts. In the core sample, 18 visible cells used tiles that the map had overwritten.
- Props: driving east into a cone cluster on Bloor at speed 18 knocked two cones over and reduced speed to 14. Props hide when a transit vehicle is on screen.
- Transit, real menu inputs from a stop on foot: Queen Spadina to Queen Broadview (streetcar eastbound in the south lane, reaching the berth as the countdown ends, then leaving east; courier hidden while riding and set down at Queen Broadview beside the stopped streetcar); Castle Frank to Ossington (bus westbound); ferry terminal to Centre Island (ferry off the island dock on arrival).
- Not verified: hardware or cartridge execution, plugin builds, human playtesting of feel and readability, audio listening, and OAM load in the most crowded scenes beyond the 38-sprite design budget.

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

- The defect/replay use clean boot 180, A2, neutral300, Select4, neutral4, A4, neutral4, A500. Both reach frame998 at `(842.9375,720)`, contract01 stage0, 112 seconds left; the old ROM shows condition92, the correction100. The corrected obstacle warning is generic while the vehicle still stops.
- Ordinary reverse/return reaches an actual Union pickup at frame2,616 with condition100/stage1. Another obstacle pass reaches frame3,116 at condition92/stage1. Braking/reverse and a stopped Market handoff finish at frame3,420 with cash123/done1: the condition-scaled base79 plus remaining-time bonus14 gives the expected93 reward.
- A separate clean boot repeats the original first-delivery/held-turn inputs and world phase: pickup502, first delivery740 with cash139/done1/condition100, then B4, neutral4, A40, A+right48, A24 reach864 at speed24/heading4 in the actual core scene. OAM at864 has four visible objects, peak four per scanline, zero over-limit lines.
- The longer lifecycle recording also retains an exploratory B-dismiss that continued into reverse and caused a correct stop-to-park refusal at speed-4. A later turn at a different world phase encounters a visible pedestrian and slows; it is not the controlled handling regression. The separate matched-phase replay above is the handling evidence.

Native coverage here is parcel lifecycle, reward and controlled driving. Passenger/fragile penalties and older damaged-stage-0 recovery have host coverage but still need native samples. Scheduled transit within contract81, full two-hour varied gameplay, the current browser preview, full former-Toronto coverage and physical cartridge acceptance remain pending.

## Final Queen scheduled service — sampled native acceptance, 2026-10-02

Current source implements eight supplemental 501 Queen curb platforms across West, Central and East. The campaign contains 51 service points while retaining the original 43 records, all 88 contracts and the 58-byte version-6 save layout. Original signs contain no TTC logo. Service 4 uses a three-dollar game fare, a 64-second directional period, two-second boarding windows and four seconds per stop interval. Destination selection derives east/west direction; schedule phase comes from the existing world clock. [STREETCAR.md](docs/STREETCAR.md) records the researched identities and deliberate normal-corridor compression. Current construction detours, full 501/504 coverage and an adopted map era are outside this implementation.

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

The final ROM repeated the original98 ordinary input steps through the former stopping corner. Market delivery finished at frame740, condition100, cash139/count1. Holding A40, A+right48 and A24 reached frame864 at(798.75,774.25), speed24/heading4. Acceleration and movement continued through the turn. Later parking, walking and entry returned to driving at frame5,066 with the car at(798.75,800.5625).

The atlas rendered Central Toronto, High Park/Junction, Toronto East End and West End; the genuine source debugger confirmed the gameplay scene remained core. Frames980→4,228 preserved all58 bytes of td state while browsing, including position, cash139, world11 and subsecond32. Hardware OAM had zero visible sprites under the map. After a partial Select repaint, B returned to pause and B to the street. Camera bytes `c0634066` were restored; camera settings returned0→3. Start also closed the map; opposite directional pairs kept the viewport still. Ordinary foot play verified You/Parked Vehicle/Depot focus. Accepting contract02 then browsing retained position, cash, health, world15/subsecond11 and deadline120 across frames5,090→5,410, apart from the expected mode change.

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

After those quests, ordinary driving entered the actual western scene and walking traversed its King/Queensway dogleg into actual High Park. The parked motorcycle remained in west at (861,640.875), player in High Park at (979.5,639.375), cash 1,304/count 9/world second 486. Save followed by A+B+Start+Select120 and neutral180 restored the genuine High Park scene/HELP and those values at frame 31,272. This verifies in-worker cartridge-RAM soft reset; physical cold boot and persistence across separate emulator processes remain unverified.

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
| `four-districts-remote-reset-20261002` | A fresh isolated worker did not import the previous recording's cartridge RAM; that was not cross-process persistence evidence. A new ordinary foot route and explicit Save reached East player (57.5,536), parked Union car (560,720) in core, cash 30/world second 32. In-game A+B+Start+Select120 then neutral180 restored actual East/HELP with these values retained at frame 2,546. | 2,546/1,366; `d4f211c70d4538ac3cedd268563cc04ab823da6d58b4d37cfc9aec3d4ab55c7f`; `258245d5-b850-41c6-9c29-dcfbecd2c942` |

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

- `districts-native-release-20261002`: 912 frames,96 events, digest `ddf3222aa18410d0a71acdcdacdf5cc2587a32d933e235d204d664d9355c38b7`. Market Start pickup/delivery completed at frame740, condition100, unique completion1, cash30→139. Holding A+right for48 frames progressed from (777.6875,720), speed8, heading0 to (803.125,744.5625), speed21, heading4. Another24 frames of A reached (803.1875,776.0625), speed24. Steering retained acceleration and movement. Braking was then applied.
- `districts-crossings-final-20261002`: 7,366 frames,386 events, digest `e45e7b73ca4dd3da2c88075df7253db875e1de3a5f7d265d223d55f8e24db848`. Normal driving crossed core→west, walking crossed west→HighPark, returned HighPark→west, recovered/entered the parked car and drove west→core. Plugin source debugging confirmed each actual destination scene, rather than inferring scene changes from saved district IDs.
- At frame3576, HighPark walking position was (974,638.875), parked car (953.0625,628.375) remained in west; cash30/world second55. Soft reset with A+B+Start+Select120 frames, then neutral180, restored HighPark and HELP at frame3876, with player/parked districts2/1 and cash/world clock retained. Bootstrap guard was0100 (live/pending).
- HighPark's local map scrolled left for400 frames and displayed HIGH PARK/JUNCTION. The world remained at second55 while MAP was open. A focused the route marker; B returned to pause/roam. This verifies the loaded district's scrollable map, not a browsable full-city atlas.
- Car-entry samples at6578→6610 showed the approach/entry and returned to driving with onfoot0 in west. At6824 the genuine core scene was loaded, with both player and car district0. A bounded OAM sample had8 visible hardware objects, peak4/scanline and zero over-limit lines; it does not establish crowded-scene performance throughout the city.
- A new Market Start contract was accepted before crossing back into west. At7066 the native state was job0/stage0/left118, cash30, world99/subsecond45. The same reset/release sequence restored the actual west scene at7366 with those values unchanged, speed reset0 behind HELP. HUD guidance named CENTRAL TORONTO for the remote objective.

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
