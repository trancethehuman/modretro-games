# Toronto Islands — researched district inputs

Reviewed **2026-10-03**. The original district is now registered in the six-scene source, with 241 buildings/595 pedestrian routes overall and version-9 migration at the same 58-byte size. All 96 contract fields, 59 identities and seven parking anchors remain; only six Island stop geometry triples relocate. Corrected `7b2…` passes its official build and scoped native exploration/ferry/map/v9-reset tests; the nine Island jobs and native old-v8 imports remain pending. Full Old Toronto, measured enjoyable two-hour play and hardware acceptance remain open. The user has not adopted a 2026 map era; use established geography as the working reference without adopting temporary closures or future projects.

## References and visual inspection

The actual PDFs below were opened and visually inspected, including the north arrow and bridge legend. The web screenshot calls returned references without visible pixels, so the official PDFs were rendered temporarily outside the repository. No reference image, traced shape or downloaded geometry becomes a game asset.

| Official reference | Supported use |
| --- | --- |
| [City wayfinding map, September 2026](https://www.toronto.ca/wp-content/uploads/2026/09/8ff1-Toronto-Island-Park-mapcompressed.pdf), one page | Current named paths, ferry landings, restricted areas and public bridge spurs. Its printed overview is **south-up**; follow the north arrow before authoring north-up art. |
| [Existing-context aerial, November 2022](https://www.toronto.ca/wp-content/uploads/2022/11/91bd-toronto-island-park-master-plan-open-house-info-panel-nov-30-2022-03.pdf), panel 3 | Relative shoreline, islands, landings, beaches and landmark arrangement; historical context rather than current operating guidance. |
| [Master-plan Part 1, June 2024](https://www.toronto.ca/legdocs/mmis/2024/ie/bgrd/backgroundfile-246911.pdf), printed pp. 47–51, PDF pages 68–72 | Established Lakeshore–Cibola/Avenue/boardwalk circulation. Figure 3-4 distinguishes **black existing bridges** from **red proposed bridges**. Recommendations are not proof of construction. |
| [City access guidance](https://www.toronto.ca/explore-enjoy/toronto-island-ferries/getting-around/) | Public vehicle restriction, connected walking network and dock-to-dock walking distances. |
| [City visitor guide](https://www.toronto.ca/explore-enjoy/toronto-island-ferries/things-to-do-on-toronto-island/) | Manitou/Avenue, lighthouse, maze, beaches, boardwalk and public attractions. |
| [City ferry routes](https://www.toronto.ca/explore-enjoy/toronto-island-ferries/ferry-routes-schedules/) | Jack Layton departures, three seasonal routes, year-round service and variable schedules. |
| [City Island projects](https://www.toronto.ca/city-government/planning-development/construction-new-facilities/parks-facility-plans-strategies/toronto-island-park-projects/) | Separate completed work from continuing landing/landscape/transport planning. |

The 2026 map requires written permission for reproduction, distribution or alteration and contains additional map-data credits. Treat it as factual reference only. The City dataset licence does not license its artwork. All game water, paths, buildings and signs must be newly drawn; do not bundle official maps, photographs, logos, commercial characters or public-art reproductions. `content/sources.json` records this limited use. Any later geometry import requires its own licence, version/hash and clipping to the actual former City of Toronto polygon; a modern ward or bounding rectangle is insufficient.

## Established geography and public graph

With north up, the Inner Harbour is north and Lake Ontario south. Billy Bishop Airport occupies the northwest; Hanlan's public park strip runs west beside Blockhouse Bay. Gibraltar is southwest, joining southern Hanlan to western Centre. Centre occupies the central/southern area. Ward's settlement is at the northeast/eastern end; Algonquin lies west/southwest of Ward and north of the narrow eastern park strip. Ward's and Hanlan's landings are north of Centre landing. Preserve these relationships while shortening distances. [Existing aerial](https://www.toronto.ca/wp-content/uploads/2022/11/91bd-toronto-island-park-master-plan-open-house-info-panel-nov-30-2022-03.pdf).

The public walking graph connects the three landings, rather than requiring a mainland transfer for each island visit. A walking route can remain on the Islands; ferries provide alternative arrival/departure choices. [City access guidance](https://www.toronto.ca/explore-enjoy/toronto-island-ferries/getting-around/).

| Link | Established character / generation constraint |
| --- | --- |
| Hanlan landing → Gibraltar → southern Centre → Ward | Lakeshore Avenue/Cibola Avenue park spine; follow the land, not a straight line across the lagoons. |
| Centre landing → central attractions → Manitou Bridge → Centre Beach/Pier | Avenue of the Islands crosses Long Pond. Make the water and the actual bridge distinct. |
| Eastern Cibola → Algonquin | Existing Algonquin bridge onto a northern residential/public-path spur; no shortcut through gardens or Queen City Yacht Club. |
| Eastern Cibola → Snake | Separate existing public bridge spur; it is not a bridge onward to restricted club islands. |
| Just east of Centre Beach → Ward Beach | Southern coastal boardwalk, parallel to part of the inland route; connect at established public entrances. |
| Central attractions → Olympic | Existing Centreville-side access may be added after its exact crossing is independently checked in the authored layout. Do not substitute the additional western bridge proposed in Figure 3-4. Initial source art may omit Olympic. |

The spine and Avenue are established circulation corridors. The plan's additional Lagoon Loop/Gibraltar bridges, north-shore trails through restricted land and expanded people-mover network remain proposals. Do not turn their dotted/red lines into existing paths. [Master-plan circulation](https://www.toronto.ca/legdocs/mmis/2024/ie/bgrd/backgroundfile-246911.pdf).

Keep airport/runway land, the water treatment plant, marked restricted islands, yacht-club grounds and residential yards blocked. Airport ferry/tunnel access is **not** park access. No road, walking bridge or tunnel connects this scene to the mainland. [Current wayfinding map](https://www.toronto.ca/wp-content/uploads/2026/09/8ff1-Toronto-Island-Park-mapcompressed.pdf).

Only emergency and commercial service vehicles are permitted; public vehicles are prohibited. Initial gameplay should therefore be **on foot**, leaving the courier's car/motorcycle/scooter/truck on the mainland. This does not authorise new controllable service vehicles or cycling. Actual dock distances are kilometres; compress them for useful choices instead of copying 35–70-minute walks into mandatory game time. [City access guidance](https://www.toronto.ca/explore-enjoy/toronto-island-ferries/getting-around/).

## Original 1,024 × 976 source compression

Local `(u,v)` means east right/north up. These are original compressed pixels, not surveyed positions or real loading doors. The six dock/client anchors below now match registered content; approximate scenery positions describe the source-art plan rather than surveys.

| Area / anchor | Original local position | Authored treatment |
| --- | --- | --- |
| Blocked airport | Northwest, approximately `u=32..288, v=24..248` | Fence, runway silhouettes and no traversable tunnel endpoint; background scenery only. |
| Hanlan dock / service | `(320,280)` / `(160,600)` | Dock east of the western strip; fictional small park pavilion on its public path. |
| Gibraltar / lighthouse | Southwest, approximately `(224,824)` | Lakeshore bends around Trout Pond; keep a separate beach edge and fenced plant to the east. |
| Centre dock / park post | `(512,448)` / `(512,744)` | Dock north of Long Pond; Avenue runs south across one explicit Manitou deck toward beach/pier. |
| Centre Beach / Pier | Approximately `(512,880)` | Shallow sand strip, blocked water, short walkable pier with a visible end. |
| Ward dock / cottage post | `(920,280)` / approximately `(904,440)` | Dock NE of Centre; small varied cottages behind public paths, with a fictional post on the path side. |
| Algonquin / Snake spurs | North of eastern Cibola, west of Ward | Separate water gaps and individually collision-backed bridges. |
| Eastern boardwalk | Southern Centre → Ward Beach | Distinct planks and water edge, with public path connectors; no new channel crossing. |

Draw connected land and blocked water first, then open only the researched bridge decks. Use cardinal bends to convey the northeast trend without angled movement. Main walking paths can be 24–32 pixels wide, with readable bridge approaches and two distinct eastward choices where inland/boardwalk geography supports them. Do not fill lagoons to obtain connectivity. A connected route around buildings must stay on public ground; tall roofs/canopies may occlude but cannot hide a blocked base or false doorway.

Original landmark anchors can include a pale masonry lighthouse, small pitched-roof church, waiting shed, cottage porches, farm barn, generic fairground structures, garden/maze, beach pavilions and pier. The lighthouse belongs north of Gibraltar Beach; the maze is west of the Centre Avenue; Ward's waiting shed is near its landing. Use these recognisable relationships without copying attraction characters, logos, sculptures or invented interiors. [Visitor guide](https://www.toronto.ca/explore-enjoy/toronto-island-ferries/things-to-do-on-toronto-island/), [existing landmark context](https://www.toronto.ca/wp-content/uploads/2022/11/91bd-toronto-island-park-master-plan-open-house-info-panel-nov-30-2022-03.pdf).

Vary low buildings, roof depths, gardens and clearings; avoid a downtown tower grid. Public pedestrians should follow validated paths, not occupy closed habitat. Sensitive dunes/vegetation can form legible blocked terrain. Hanlan's inclusive community significance should be represented respectfully; do not invent ceremonies or use a proposed Snake Island ceremonial facility as a built landmark. [City projects](https://www.toronto.ca/city-government/planning-development/construction-new-facilities/parks-facility-plans-strategies/toronto-island-park-projects/).

## Courier choices and integration gates

Proposed jobs should reward planning: choose the nearest ferry landing for a fragile garden delivery; carry parcels between Centre/Ward via inland versus boardwalk routes; return a borrowed kit from Algonquin across its real bridge; deliver western supplies via Hanlan/Gibraltar instead of landing at Centre. Fictional clients stay beside public paths. Use existing package/multi-stop/return semantics first, with no boat control, compulsory idle loop or unsupported emergency permission. Deadlines/rewards remain provisional until ordinary-control play measures the whole journey, including parking, ferry waits, walking and return. New counts alone cannot establish two hours of fun.

Real services run from Jack Layton to the three landings in the warmer season, with Ward service in winter and weather-dependent schedules. Retain clearly fictional compressed fares/timings; do not copy current tables or present a new inter-island ferry as an established City route. Walking between landings creates the new option. [City ferry routes](https://www.toronto.ca/explore-enjoy/toronto-island-ferries/ferry-routes-schedules/).

The registered district 5 now holds dock IDs **20–22** and fictional client IDs **24–26**; the duplicate tiny Core strips and three service buildings are removed while mainland art/collision/attributes above pixel 816 remain unchanged. Version-9 migration imports legitimate old walkers, waits and paid trips through the immutable historical terrain mask while preserving cash, active-stage/deadline, completion IDs, v8 attention and mainland parked cars. The actual-engine host migration gate and scoped native v9-reset/travel samples pass; native historical-v8 import remains pending. Current ferry stop IDs fit the six-bit transit representation; review that encoding before adding transit nodes. Non-transit stop growth and the 128-completion-bit limit need separate checks.

The first integrated campaign reuses its nine ordered Island jobs, preserving indices 7/15/23/31/39/47/55/63/71 and deadlines 195/205/200/310/305/310/360/360/360 seconds. Their original route fields still require mainland ferry transfers where authored; free roaming permits connected walking between all landings. New alternative-route jobs above remain proposals. Source has nine Island footprints, five closed cottage yards, three existing bridges and 37 full-body pedestrian routes; 120 raw/78 flip-canonical patterns fit the source budget. Independent foot/ferry/prefix gates, full checks (787,948 engine / 25,107,699 UI), the corrected official build and scoped native bridge/path travel pass. Broader palette/OBJ/crowd/occlusion, native old-v8 imports and all nine timed jobs/deadline tuning remain pending.

Current local candidate `toronto-dispatch-islands-safe.gbc` has SHA-256 `7b2af59c27179bd3445c5c074b85fb4a029b50144ef27ed3095f9a3ce83f7d5a`. Its closed ordinary-input record samples all three docks/clients/bridges, inland/coastal choices, blocked shore/private yards/airport gap, canopy occlusion, frozen map focus/panning, scheduled ferry fares and zero-fare recovery, v9/paid/free resets and distant Core car re-entry. A condition-100 first Core job then pays 109 (cash 2→111/done 1) and restores after reset. Shoreline refusal samples the native point-terrain query; full-body corridor checks are separate source evidence. It does not complete an Island mission. [Portable evidence](NATIVE_ISLAND_DISTRICT_SAMPLES.json) retains the scope; the earlier `8a96…` pause-format/map-cache failure and `4343…` five-scene evidence remain separate.

Required gates:

- Source: original art/metadata, named water shapes plus rectangular `water` export, full-foot collision/reachability for every dock/client/bridge, blocked airport/plant/club/shore tests, and no false mainland or water shortcut.
- Native resources: actual compiler tile/palette/OBJ banks, aircraft scratch isolation, scene loaders, stack reserve, atlas expansion and all six-scene fixtures. A source-pattern budget alone is insufficient.
- Save/transit: old v8 imports, district-qualified foot/parked-car state, accepted/carrying/paid checkpoints, fare once, genuine reset and timeout/cancellation, with explicit new-version compatibility.
- Ordinary inputs: all three arrivals and returns; Hanlan→Gibraltar→Centre→Ward foot travel; both eastern path choices; Algonquin/Snake bridge out-and-back; building/shore rejection, map freeze and distant mainland-car recovery.
- Release: measured campaign variety/duration, crowds/occlusion/performance, human feedback and physical Chromatic/cartridge tests. Scoped native exploration/ferry/reset proof does not complete the campaign, human or hardware gates.
