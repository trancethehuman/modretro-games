# Toronto Dispatch

A north-up, top-down pixel-art courier game for ModRetro Chromatic / Game Boy Color. Drive, brake into corners, park, walk and take timed transit through five compressed Toronto areas and selected Island routes.

**Status: five-scene source milestone; published Prototype 6 is separate.** Current candidate `a212dd9e…` builds five native scenes, four vehicles, 88 authored contracts, 235 buildings, moving pedestrians/traffic, paid scheduled subway/bus/ferry/Queen travel, a scrollable city atlas, original music/effects and SRAM progression. Roads have 48 pixels of asphalt plus sidewalks. Scoped native checks verify Port Lands foot travel, Beach checkpoint reset, paused map state, aircraft rendering and first-delivery driving. Historical Prototype 4 completed nine distinct jobs; those recordings retain their own ROM identity. The two-hour target, full Old Toronto/fuller-Islands coverage, Port Lands driving/remaining bridges, human handling and physical cartridge testing remain open.

![Native city atlas](docs/screenshots/city-atlas.png)

Unmodified Prototype5 native emulator frame; [provenance](docs/screenshots/provenance.json).

## Play

Select `project/project.gbsproj` with the ModRetro Chromatic plugin and build the current source. Run the exact inspected output file returned by the plugin in its native emulator or official browser preview. Generated ROMs are intentionally excluded from Git. See [build instructions](docs/BUILD.md) for tested build identities.

Prototype5 adds a [city atlas](docs/CITY_MAP.md) with job, parked-car and booked-transit-stop focus. Its exact ROM repeats the held-turn regression and verifies map pause/resume and paid-ride reset; see [test evidence](TESTING.md). Ready-made prototype bundles are on the [GitHub releases page](https://github.com/trancethehuman/modretro-games/releases). Follow the [loading instructions](docs/LOADING.md) for the supported development cartridge.

Published [Prototype 6](https://github.com/trancethehuman/modretro-games/releases/tag/v0.2.0-prototype.6) adds eight original curb signs and a scheduled [501 Queen game service](docs/STREETCAR.md) through west, core and east. It has 51 service points while preserving every existing client and all 88 contracts. Its sampled native scenarios cover paid cross-district travel, pause/map/reset, car recovery and legacy train/bus/ferry alighting.

Newer source adds an original moving Queen streetcar with door poses and scene-following paid rides, plus occasional [planes and helicopters](docs/AIRCRAFT.md) above the city. Flybys vary in direction and path, helicopters animate their rotors, and both cast separated shadows. These cosmetic flights pause with menus and maps. Version-7 saves still use 58 bytes and migrate valid older checkpoints. Source also corrects overlapping traffic recovery, booked tram occupancy and unsafe alighting; the earlier exact `14005662…` replay covers paid reset, walking recovery, flybys and delivery/driving.

Current output is `project/build/toronto-port-lands-labels.gbc`, SHA-256 **`a212dd9ed479310a98e58b701416f8af88aaa09174f186747dca678ff4c164f4`**. It appends original [Port Lands](docs/PORT_LANDS_PLAN.md) industry, six supported bridge decks, park paths and Cherry Beach, reached through Leslie. Its native sample pays Queen 46→49 once, walks into Port Lands and back to East, reaches the Beach, restores its saved checkpoint and Core parked car after reset, and freezes all game/flight bytes while panning the second map row. A fresh delivery/driving sample completes contract 01 and verifies held acceleration, braking/reverse, recovery and coasting. Build/header/compiled-resource/full host checks and the 1,176-byte linked reserve pass. Eight proposed Port Lands clients/jobs and a bus connection remain outside the 88-contract/51-stop campaign. These newer changes are not in downloadable Prototype 6; consult the [exact test record](TESTING.md) before building or loading.

| Action | Controls |
| --- | --- |
| Accelerate / coast | Hold A / release A |
| Steer the vehicle | Left / right; brake for tighter corners |
| Brake / reverse | B; keep holding near rest to reverse |
| Accept or deliver a package | Select opens dispatch, then A accepts; Select at a beacon while stopped collects/delivers |
| Pause menu | Start; up/down and A choose |
| Park and exit | Stop, then pause → Park / recover car |
| Walk | D-pad; A near the parked car animates entry |
| Transit | On foot, B at a station/terminal/platform; left/right chooses destination, A waits/boards |
| Change service | Up/down at Wellesley switches Line 1 / 94 bus |
| Scrollable city map | Pause → map; D-pad pans, A centres job / booked transit stop / depot; Select changes focus, B returns |
| Change vehicle / save | Pause menu; change vehicle while stopped without an active job |
| Sound | Pause → Audio; cycle full music/effects, effects only, or silent |

First job: accept contract 1 at the Union depot, press Select to collect, drive east along Front Street to St. Lawrence Market, brake and press Select to deliver. The marker and HUD identify the next waypoint. Pickup is the first of the displayed stops. The 72 original contracts, plus eight western and eight eastern jobs, have distinct routes/titles, brief instructions and nine progression chapters. Completion unlocks truck/transit jobs, passenger work and Island walking rounds. Dispatch selects the next eligible unfinished contract. Replays pay but do not increment unique completion twice.

Transit runs on a repeating **fictional game clock**, even without the player. Fares are 3 for subway, 2 for bus and 4 for ferry; Queen also costs 3. These are game credits, not real TTC prices. At a Queen sign, choose a destination; the menu shows eastbound/westbound, departure countdown and trip cost/time. Each direction repeats every 64 seconds with a two-second boarding window, and a ride takes four seconds per selected stop interval. Choosing the current stop cannot board. Waiting/riding uses mission time; menus and the map pause it. Heavy freight and passenger jobs require their road vehicle. Parking leaves that vehicle behind for recovery. Island ferries connect through the mainland, and ordinary cars cannot reach the Islands.

Driving retains momentum through steering and glancing curb contact. A bounded sideways adjustment helps clear narrow tile corners while accelerating; broad walls still stop the vehicle. Fragile crates take greater crash damage. Fast passenger turns reduce comfort, and base rewards scale with condition. Two alternating CRC-checked save records retain a previous checkpoint if a write is interrupted. Paid rides resume from their latest second after a reset. Physical power-off persistence still needs a cartridge test.

Pedestrians follow 565 fixed sidewalk routes across the five areas with six nearby people rendered at a time. The four non-core areas have 24 continuous traffic loops; cars yield to the courier crossing on foot. The visible bus currently uses a truck-shaped placeholder and has a separate animation path from the boarding timetable. Dedicated TTC vehicle art and matching visible service schedules remain planned. Original city music, engine, braking and event effects have been verified through emulator PCM capture; physical speaker/headphone testing remains open. See [audio design and reproduction](docs/AUDIO.md).

For installation, use the [loading instructions](docs/LOADING.md). ROM bundles require the [distribution notices](docs/DISTRIBUTION.md). This prototype has not been verified for two hours of gameplay.

## Source and scope

- [City map controls and implementation](docs/CITY_MAP.md).
- [Port Lands original layout, bridges, industry and proposed jobs](docs/PORT_LANDS_PLAN.md).
- [Queen platforms, fictional timetable and transit research](docs/STREETCAR.md).
- [Design](DESIGN.md), [decisions](DECISIONS.md), [roadmap](ROADMAP.md) and [test evidence](TESTING.md).
- [Old Toronto research](docs/TORONTO_RESEARCH.md) and [geography boundaries](docs/GEOGRAPHY.md).
- [Proposed district expansion](docs/OLD_TORONTO_EXPANSION.md) and [runtime audit](docs/WORLD_RUNTIME_AUDIT.md).
- [Editable GB Studio project](project/project.gbsproj) and [native scene engine](project/plugins/toronto-driving/engine/).
- [Compiled campaign specification](content/campaign.json) and [original world/art specification](content/city_art.json).
- [Asset generators and collision/connectivity validator](scripts/).

The present city compresses central Toronto, Parkdale/Roncesvalles, High Park/Swansea/Junction, Riverside/Riverdale/Leslieville/western Danforth and Port Lands/Donmouth. Five 1,024 × 976 scenes occupy a logical 4,096 × 1,952 atlas; the browsable 512 × 244 schematic leaves unregistered lower cells solid and unnamed. Queen service uses eight shared directional platform pairs on the normal corridor with fictional operation; current construction detours are omitted and the map era remains unadopted. The wider historical municipality, neighbourhood detail, fuller Islands, full 501, King 504 and full TTC network are future work. Landmark art is an original interpretation. The original three mission design samples remain in `content/missions.json`; runtime contracts are in `campaign.json`.

Original code and artwork use the root MIT licence. Starter font/metadata retain their upstream notices. No commercial courier branding, map imagery or TTC logo is included.
