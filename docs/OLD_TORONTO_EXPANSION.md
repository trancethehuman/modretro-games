# Full Old Toronto native expansion plan

Researched 2026-10-02. This is an implementation proposal for the accepted full-city scope. It does not expand the current ROM. The existing 1,024 × 976 map remains a compressed central-city prototype; neither its 27 service points nor its 72 contracts establishes complete Old Toronto coverage or two hours of measured gameplay.

## Boundary authority and map era

Use the **former City of Toronto immediately before 1998 amalgamation**, including its waterfront and Toronto Islands, as the playable clipping boundary. The City Archives records that Forest Hill and Swansea joined Toronto in 1967; Leaside joined East York. Therefore Swansea and Forest Hill belong in the expansion, while Leaside and other former municipalities do not. This is a present-day city within a historical municipal footprint, rather than a game set in 1997. [City Archives: former municipalities](https://www.toronto.ca/city-government/accountability-operations-customer-service/access-city-information-or-records/city-of-toronto-archives/whats-in-the-archives/research-by-topic/resources-on-former-municipalities/).

The stronger geographic authority located for this pass is the City's [Former Municipality polygon layer, ID 6](https://gis.toronto.ca/arcgis/rest/services/cot_geospatial27/MapServer/6). A live, read-only query with `where=AREA_NAME='TORONTO'`, `outSR=4326`, `returnGeometry=true`, and `f=geojson` returned one polygon with one ring and 3,656 vertices. Its inspected longitude/latitude extent was:

| Limit | Longitude/latitude |
| --- | --- |
| Bounding extent | `[-79.492817713, 43.611760177, -79.280015332, 43.736911906]` |
| Westernmost inspected vertex | `[-79.492817713, 43.646729982]` |
| Easternmost inspected vertex | `[-79.280015332, 43.671000698]` |
| Northernmost inspected vertex | `[-79.392407872, 43.736911906]` |

These are observations from the City service, not game coordinates or a replacement rectangular boundary. The polygon's irregular northern extension, western rail/Humber edges and eastern property edges must be preserved. Do not substitute contemporary wards, Toronto-and-East-York community council, all 158 statistical neighbourhoods, a straight Victoria Park line, or a downtown rectangle. The [City neighbourhood explanation](https://www.toronto.ca/city-government/data-research-maps/neighbourhoods-communities/neighbourhood-profiles/about-toronto-neighbourhoods/) also distinguishes statistical operational areas from familiar community geography.

Only the query's summary is recorded here. No polygon, official map image or street geometry was bundled into art or runtime in this pass. Before committing a clipping dataset, identify its reuse terms and provenance explicitly; a blank copyright field in an ArcGIS response does not establish a licence. The [Toronto Centreline portal](https://open.toronto.ca/dataset/toronto-centreline-tcl/) and [Open Data Licence](https://www.toronto.ca/city-government/data-research-maps/open-data/open-data-licence/) provide the intended road-data path. Keep source geometry separate from the original compressed game graph.

**Proposed map baseline:** researched 2026 geography and normal TTC corridors, with fictional game schedules and clearly documented omitted construction detours. Adopt this baseline explicitly before generation. A 2026 baseline needs `TMU` for the former Dundas subway station and `Cedarvale` for the former Eglinton West station; these changes concern station names, not an assumed rename of Dundas Street. [TTC Spring 2026 newsletter](https://cdn.ttc.ca/-/media/Project/TTC/DevProto/Documents/Home/Wheel-Trans/Newsletter/2026/260415_WT_Newsletter_Spring-web.pdf?hash=C632E90C4FF39FDC9C58A7B325E15CE7&rev=861d23267e23490eae93bc851f23733b).

Line 5 Eglinton began introductory service in February 2026, and the TTC reports an August 29 celebration following the phased launch. Its in-scope Eglinton corridor should be represented in a 2026 northern expansion, rather than treating the whole line as future construction. Do not add the out-of-scope Mount Dennis/Kennedy regions to satisfy that corridor. [TTC launch announcement](https://www.ttc.ca/news/2026/February/TTC-to-begin-phased-start-to-service-on-Line-5-Eglinton), [TTC September account](https://www.ttc.ca/TheInterchange/2026/September/Line-5-Eglinton-Celebration).

## Required missing coverage

The following groups are **original production districts**, not official neighbourhood boundaries. Each must be clipped against the former-Toronto polygon before entrances, roads or service points are accepted.

| Priority | Geography missing or too compressed | Road/access structure to preserve | Distinctive art and delivery choices |
| --- | --- | --- | --- |
| 1 | Central west: Chinatown, Kensington, Little Italy, Trinity Bellwoods, Little Portugal, Dufferin Grove | College, Dundas West, Queen West and Bloor links; Ossington, Bathurst, Dufferin and side streets have different alignments | Shop awnings, row houses, park canopies; walking last metres versus parking on a wider street |
| 1 | Parkdale, Roncesvalles, Liberty Village, Exhibition and Fort York | Roncesvalles/Howard Park/Dundas bends; rail barriers; Dufferin and Strachan access to Exhibition, Bathurst bridge toward Fort York | Converted industrial brick, wide exhibition buildings and Princes' Gates; truck loading versus light courier access |
| 1 | Riverside, Riverdale, Leslieville and Danforth | Actual Don crossings; Queen, Dundas and Gerrard are separate corridors; Bloor/Prince Edward Viaduct connects to Danforth | Brick retail strips, residential streets, bridge approaches; choose crossing and traffic corridor |
| 1 | Port Lands and eastern waterfront | New Don mouth, Keating Channel, Cherry Street and Commissioners bridges; Don Roadway and industrial roads; keep water/rail barriers | Cranes, silos, warehouses and Biidaasige Park; freight yards versus pedestrian park deliveries |
| 2 | High Park, Swansea, Sunnyside and Junction | Humber edge; Bloor West, Parkside, Roncesvalles, The Queensway, Dundas West and rail junction; distinguish public park paths from vehicle roads | Large green areas, cottages, pavilion and railway character; park, walk and return to the vehicle |
| 2 | Annex, Yorkville, Casa Loma, Rosedale and Wychwood | Bloor, Dupont, Davenport, St. Clair, Bathurst, Spadina Road and Yonge; hill/ravine routes must not become a generic square grid | Campus/museum buildings, tall civic/cultural silhouettes, houses and streetcar barns; gentler fragile/passenger routes |
| 2 | Forest Hill and North Toronto through the polygon's northern extension | Eglinton, Avenue Road, Spadina Road, Yonge, Lawrence and local boundary roads; clip east/west limits rather than including adjacent North York/Leaside | Leafier streets and mixed main-street retail; longer north/south driving versus rail plus last-mile walking |
| 2 | East Danforth, Gerrard/Little India and Beaches | Danforth eastward, Upper/Lower Gerrard and Coxwell bend, Woodbine/Main, Kingston Road and Queen East; shoreline pedestrian routes | Main-street variety, beach edge, Leuty silhouette and R.C. Harris terraces; boardwalk remains walk-only |
| 1 | Full Islands: Hanlan, Centre, Ward/Algonquin and Gibraltar Point | Three mainland ferry destinations, real Island paths/footbridges, lagoons and airport/service restrictions | Cottages, pavilion, beach and lighthouse; meaningful on-foot route choice instead of completing at the dock |

Verified new art anchors include Colborne Lodge at 11 Colborne Lodge Drive, Sunnyside Pavilion at 1755 Lake Shore Boulevard West, Spadina Museum beside Casa Loma above the Baldwin Steps, Fort York at 250 Fort York Boulevard, Princes' Gates at Exhibition and Wychwood's former streetcar barns south of St. Clair. These addresses/descriptions are source facts; exact fictional courier entrances remain design work. [Colborne Lodge](https://www.toronto.ca/explore-enjoy/history-art-culture/museums/colborne-lodge/), [Sunnyside](https://www.toronto.ca/explore-enjoy/parks-recreation/places-spaces/parks-and-recreation-facilities/location/?id=395&title=Sunnyside-Park-Bathing-Pavillion), [Spadina Museum](https://www.toronto.ca/explore-enjoy/history-art-culture/museums/spadina-museum/), [Fort York](https://www.toronto.ca/explore-enjoy/history-art-culture/museums/fort-york-national-historic-site/), [Exhibition heritage](https://www.explace.on.ca/about/story-heritage/plaques/), [Wychwood heritage designation](https://www.toronto.ca/legdocs/mmis/2007/te/bgrd/backgroundfile-4913.pdf).

Keep the already researched CN Tower, Union, City Hall, Gooderham, St. Lawrence, Distillery, AGO, ROM, OCAD, Casa Loma, R.C. Harris and Gibraltar lighthouse anchors. Add original interpretations, not copied venue logos or map art. See [TORONTO_RESEARCH.md](TORONTO_RESEARCH.md) for those sources.

High Park's current visitor access is restricted, including car-free roads and weekend closures; its official page distinguishes roads, trail entrances and authorised vehicles. Use a declared fictional shift rule, or roadside parking plus foot deliveries, rather than opening every park path to cars. Island public vehicles are similarly restricted. [High Park access](https://www.toronto.ca/explore-enjoy/parks-recreation/places-spaces/beaches-gardens-attractions/high-park/), [Island access](https://www.toronto.ca/explore-enjoy/toronto-island-ferries/getting-around/).

Port Lands geography must use the opened/new river layout. Waterfront Toronto records the 2025 park/river opening and names Biidaasige Park and Ookwemin Minising. Its map depicts Cherry Street North/South bridges, Commissioners Street Bridge, Ship Channel and park paths, but carries a reproduction-permission notice; use it as reference only. Planned bridges/streets in older reports must not be drawn as completed current infrastructure. [Biidaasige Park](https://www.waterfrontoronto.ca/our-projects/biidaasige-park), [2025 official reference map](https://www.waterfrontoronto.ca/sites/default/files/2025-08/Biidaasige_Park_Map_August_7%2C_2025_FINAL-ua.pdf).

## Native district atlas and coordinate proposal

Use linked native `TORONTO` scenes, each **768 × 768 pixels** (96 × 96 tiles). A sparse **5 × 5 logical atlas** permits a 3,840 × 3,840 pixel city envelope while loading only one district's graphics/collision at once. Atlas cells organise original compression; they are not latitude/longitude rectangles and cannot themselves establish municipal coverage. A street/portal graph carries geographic connectivity across cells.

| District ID | Atlas cell `(column,row)` | Proposed scene coverage |
| --- | --- | --- |
| 0 | (2,2) | Downtown / Old Town core; remake the central prototype rather than pretending its current coordinates fit the new scale |
| 1 | (1,2) | Central west, Dufferin–Bathurst communities |
| 2 | (0,2) | Junction, High Park north and Roncesvalles; clipped rail/Humber edges |
| 3 | (0,3) | Swansea, High Park south and Sunnyside |
| 4 | (1,3) | Parkdale south, Liberty Village, Exhibition and Fort York |
| 5 | (2,3) | Central waterfront, Harbourfront, ferry terminal and East Bayfront |
| 6 | (3,2) | Riverdale/Riverside/Leslieville and western Danforth |
| 7 | (4,2) | Eastern Danforth, Upper Gerrard and northern Beaches streets |
| 8 | (3,3) | Port Lands / Don mouth / Cherry Beach industrial and park areas |
| 9 | (4,3) | Ashbridges / Beaches shoreline, Leuty and R.C. Harris edge |
| 10 | (2,1) | Annex/Yorkville/Casa Loma/Rosedale; local branches around ravines |
| 11 | (1,1) | Wychwood, Corso Italia east and Forest Hill south |
| 12 | (0,1) | In-scope western St. Clair / Davenport rail edge; mask the large out-of-scope portion |
| 13 | (1,0) | Forest Hill north, clipped against the former-city polygon |
| 14 | (2,0) | North Toronto / Eglinton / Lawrence / northern extension |
| 15 | (2,4) | Hanlan and Centre/Gibraltar Island paths; no mainland road seam |
| 16 | (3,4) | Ward/Algonquin Island paths; ferry and verified Island footbridge links |

These 17 scenes are a planning estimate. Split or combine only after road-graph density, native art budgets and the polygon mask are inspected. Large stretches of the atlas remain outside the municipality or over water. Block them or omit them; do not fill empty cells with invented Toronto suburbs.

Store `district_id` plus local Q4 `(u,v)` for player and parked vehicle. GBVM actors/camera stay local Q5. A display/route planner may derive `atlas_x = column*768 + local_x` and `atlas_y = row*768 + local_y`. The logical envelope fits unsigned Q4 coordinates, but **global Q5 positions overflow 16 bits**; never assign atlas coordinates directly to GBVM actor/camera fields.

Add `district_id` to each globally indexed service point. Preserve existing stop IDs 0–26 while migrating their coordinates deliberately; append new IDs for expanded places. Keep total stop IDs below the `255` sentinel unless the whole route/save representation is widened. `td_near`, the marker and parked-car actor must compare district as well as local coordinates. A target in another district should mark the next useful street portal and show the destination district, rather than drawing a local beacon at an unrelated matching coordinate.

For the first pair of scenes, propose matched **40-pixel-wide road ports** with an additional sidewalk band. For an east/west seam, use source centre `(752,y)` and destination `(16,y)`; preserve lateral lane offset and heading. Example central-road port rows for the first art study: Bloor `80`, College `272`, Dundas `368`, Queen `480`, King `576`. These are original pixel coordinates, not surveyed street positions. Local bends and intersections must be fitted to the source graph before generation.

## Road and transit connections

Extract a graph from licensed Toronto Centreline data using stable centreline/intersection IDs, street name and road classification; keep direction restrictions separately. The inspected [City Centreline service](https://gis.toronto.ca/arcgis/rest/services/cot_geospatial/MapServer/2) exposes `CENTRELINE_ID`, `LINEAR_NAME_FULL`, `FROM_INTERSECTION_ID`, `TO_INTERSECTION_ID`, `ONEWAY_DIR_CODE_DESC` and `FEATURE_CODE_DESC`. Exclude highways/private yards/trails from ordinary driving unless the design explicitly supports that access. Verify category and direction meanings in the dataset documentation rather than guessing codes.

Required connection studies, in order:

1. Core ↔ central west: continuous Bloor, College, Dundas, Queen and King corridors at a verified Bathurst-side cut; model intersections, not a row-name interpolation.
2. Core ↔ east bank: Prince Edward/Bloor–Danforth, Gerrard, Dundas and Queen Don crossings. Verify Eastern/Lake Shore crossings individually. Do not invent a King or Front bridge over the Don.
3. Core ↔ waterfront: rail crossings/underpasses at supported streets, with Bathurst, Spadina, Bay/Yonge and eastern access inspected separately. Liberty/Exhibition require actual Dufferin/Strachan access and a rail barrier. Pedestrian rail tunnels do not become truck shortcuts.
4. Central west ↔ far west: Bloor/Dundas/Queen/King continuity, Roncesvalles and Howard Park bends, High Park path/road distinctions, Humber clipping.
5. Core ↔ northern districts: Yonge, Bathurst/Spadina Road/Avenue Road and Davenport/St. Clair connections; inspect ravines and the northern polygon limits.
6. East inner ↔ east outer: Danforth, Queen and Gerrard, including the Coxwell connection between Lower and Upper Gerrard. Keep Kingston Road diagonal character where the compressed graph permits it.
7. Waterfront ↔ Port Lands: Cherry Street/Commissioners/Don Roadway with new river/channel crossings; no straight drive through wetlands or Ship Channel.
8. Mainland ↔ Islands: ferry only for ordinary courier/player access. Island-to-Island foot connections require an inspected park-path/bridge graph; no artificial land bridge over the Inner Harbour.

Use an explicit portal record `(from_district, from_local_point, to_district, to_local_point, street_id, access_mask)`. Each portal must have a reciprocal arrival and a footprint clearance test for all permitted vehicles. Different street choices should carry different distance, signal, congestion and parking tradeoffs. Corner blockers and discontinuous sidewalks need checks at both sides of every seam.

Expand the autonomous transit graph alongside the city, preserving real identity but tuning fictional fares/frequency/travel times:

- **Line 1:** both researched downtown branches, then in-scope St. Clair/Eglinton/Lawrence stops on Yonge and the appropriate west branch. Do not rename the U-shaped line into a single straight north/south service. Show vehicles continuing off the playable map where needed; only expose in-scope exits. Station inclusion needs polygon/access verification.
- **Line 2:** the in-scope Bloor–Danforth corridor, connecting western districts, Bloor–Yonge/St George and eastern Danforth. Validate Jane/Victoria Park edge stations against the polygon; do not add Kipling/Kennedy roaming districts. [TTC Line 2 station order](https://www.ttc.ca/routes-and-schedules/2/0/13782).
- **Line 5:** in-scope Eglinton connections, including the Line 1 transfer naming above. Add only after the 2026 baseline and station clipping are adopted.
- **94 Wellesley:** preserve Ossington–Harbord/Hoskin/Queen's Park–Wellesley–Parliament/Bloor–Castle Frank geometry; the prototype's three representative stops do not represent the full corridor. [TTC route reference](https://www.ttc.ca/routes-and-schedules/94/1/8008).
- **Streetcars:** prioritise Queen, King, Carlton/College, Harbourfront and St. Clair corridors. The 506's verified normal route bends through Howard Park/Dundas in the west, Parliament in the core and Coxwell in the east. Avoid representing every streetcar as a horizontal looping truck. [TTC 506 route description](https://www.ttc.ca/routes-and-schedules/506/0/3030). St. Clair currently has construction substitutions; normal-corridor game service must be labelled as the declared baseline rather than live service. [TTC September 2026 advisory](https://www.ttc.ca/service-advisories/Service-Changes/512-312-Route-change-due-to-rail-bridge-construction).
- **Island ferry:** mainland spokes to Hanlan, Centre and Ward, with no ordinary road vehicles onboard. Foot paths provide the last leg. Service time continues while other districts are loaded.

Represent transit trips as `(service_id, origin_stop, destination_stop, departure_clock, arrival_clock, fare_paid)` using the persistent world clock. Draw only nearby vehicles/people; compute off-screen scheduled positions from the same clock. Changes of scene must not restart timetables, refund/recharge a fare, despawn the parked car logically or erase a carried package.

## Engine, art and save feasibility

The current map has `128*122 = 15,616` entries in each native tile/attribute/collision array, leaving only 768 bytes below a 16 KiB bank. The installed engine stores each array behind a banked pointer; its collision lookup does not cross banks for one map. Expanding that single array is the wrong implementation path.

Official GB Studio documentation permits backgrounds up to 2,040 pixels per dimension but also caps total area at 1,048,320 pixels. Consequently a 1,024 × 1,024 image is outside the documented area limit even though its 16,384 byte tilemap exactly fills a bank. The proposed 768 × 768 district uses 9,216 entries and stays well below both limits. [Background limits](https://www.gbstudio.dev/docs/assets/backgrounds/).

Color Only scenes allow up to 384 unique background tiles. Keep a working art target at or below 320, with repeated road/sidewalk/brick/window modules and district-specific landmark tiles; reserve UI palette/tile allocations and preserve roof/canopy priority. Audit actual sprite/background sharing rather than spending every available tile. [GB Studio scene memory limits](https://www.gbstudio.dev/es/docs/project-editor/scenes/limits/).

Seventeen scenes use 470,016 bytes (about 459 KiB) of raw tile/attribute/collision arrays before tilesets, bank padding, engine, content, sprites and audio. A **2 MiB ROM budgeting target** is plausible but unverified; inspect the real linker allocation and identified writable cartridge capacity before adopting a release size. Linked scenes bound the active sprite/CPU load; they do not excuse crowded-scene native performance tests.

Use the engine's ordinary scene-change lifecycle, with a small project-local district event/adapter. The inspected installed GBVM calls `load_scene` through `EXCEPTION_CHANGE_SCENE`, resets scene script/timer/input-event contexts, then calls `state_init`. Calling `load_scene` inside driving update would bypass parts of that lifecycle. Official [Change Scene events](https://www.gbstudio.dev/docs/scripting/script-glossary/scene/), [GBVM scene exceptions](https://www.gbstudio.dev/docs/scripting/gbvm/gbvm-operations/) and [custom scene types](https://www.gbstudio.dev/docs/extending-gbstudio/plugins/) establish the supported path; prove the adapter in two scenes before expanding.

Separate cold boot/save restore from district presentation initialisation. Current `toronto_init` restores a single-city record and reconstructs local actors; every linked scene must instead preserve the live courier state, initialise only its local artwork/actor bindings and set the correct local camera. A scene transition should retain speed and heading on a clear matched road, while capping speed near uncertain arrival geometry. Use a short fade or proven common tileset to avoid tile-load glitches. [GB Studio scene transitions/common tilesets](https://www.gbstudio.dev/docs/project-editor/scenes/).

Persistent state must include current district/local position, parked-car district/local position, mission/stage/condition/deadline, completion bits, money, transit trip phase/fare status and world clock. Keep transient actor pointers and camera addresses out of SRAM. Add a versioned migration for old single-city records; preserve completed contracts/cash where mapping is safe and cancel an incompatible active route explicitly. Validate destination district, stop/service IDs, stage, local footprint and parked-car location when restoring. Save transition/arrival atomically through the existing CRC/checkpoint framework; test interruption before and after commit, fare charge and scene load.

The map UI needs a full-city scrollable overview, with district outlines, named road spines, landmarks, player, parked vehicle and current objective. A small dedicated native map scene/atlas can use original repeated tiles at reduced scale. Preserve world state while viewing it and return to the exact live district; opening the map is not a vehicle teleport. Detailed district view must also identify the next usable portal for an out-of-district objective.

## Campaign and implementation acceptance

Expand the 72 authored contracts onto meaningful new districts, preserving early tutorial IDs/routes where practical. Assign real neighbourhood businesses fictional clients, original briefs and accessible entrances. Use local multi-stop rounds, west/east freight, north/south passenger trips, fragile gallery exchanges and transit/foot last-mile routes. Longer trips should come from useful geography and route decisions, not repeated dock visits or mandatory stationary timers.

For 72 contracts, two hours implies about 100 seconds per contract on average, but that arithmetic is only planning. Record real representative jobs across vehicle/type/district combinations, then a complete practiced campaign and a novice playthrough. Aim for enough purposeful content that faster legal route choices still leave a substantial campaign. Revise routes, depth, traffic, rewards and optional quests from those timings; keep `duration_verified=false` until measured evidence meets the full target. The current shortest-path/control model does not meet that gate.

Implement in these native milestones:

1. Confirm baseline and data terms; retain a hashed source manifest and an independent former-Toronto clipping mask. Verify road topology/entrances before pixels.
2. Build two connected districts (core and central west) with reciprocal road/sidewalk ports, global stops and save/transit preservation. Test crossing at speed, on foot, with cargo, while returning to a parked car and after a cold boot.
3. Add eastern crossings, waterfront, Port Lands and fuller Islands; playtest route choice, fare affordability, foot access and blocked vehicle access.
4. Add far west, northern extension and outer east. For each scene audit coverage mask, tile/palette/sprite/ROM resources, reachable service points and all seams.
5. Expand transit/traffic lane graphs and original neighbourhood art. Verify schedules continue off-screen and visible vehicles follow the named corridors.
6. Recompile the expanded authored campaign, measure gameplay duration/enjoyment and tune condition/deadline/reward rules. A static flood fill is only one part of the acceptance evidence.
7. Verify exact ROM build/header/linker identity, native emulator scenarios, cartridge loading instructions and manual physical cold boot/save/audio/control checks separately.

This plan deliberately leaves generation and native implementation to a dedicated milestone. No runtime, artwork, scene, campaign, source registry or build changed during this research pass.
