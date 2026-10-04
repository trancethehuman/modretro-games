# Port Lands native district inputs

Reviewed **2026-10-02**. Port Lands is now the registered fifth **1,024 × 976** north-up scene, with original generated art/collision and the sole Leslie connection to East. Current source appends eight courier jobs/clients for 96 contracts/59 service points; matching `c625…` build/full-suite gates and the first Fire Hall Books delivery/reset/car-recovery pass; the other seven jobs and tuning remain pending. Retained `a212…` and `a243…` native travel proofs keep their own identities. The older 768-pixel/17-district proposal does not supply this layout. Representative contemporary normal corridors remain a working default; the user has not adopted a specific 2026 era. Unbuilt neighbourhoods and later transit remain future features; this scope does not verify full Old Toronto or two hours.

## Verified geography and rights

The City describes the Port Lands between the Inner Harbour, Lake Shore Boulevard East, Leslie Street and the Outer Harbour, retaining industrial/port activity alongside redevelopment. McCleary is north of Commissioners and east of Don Roadway; its future mixed-use plans do not establish completed towers. [City Port Lands overview](https://www.toronto.ca/city-government/planning-development/planning-studies-initiatives/port-lands/).

Ookwemin Minising lies between Keating Channel to the north, the new Don River to the east/south and the harbour to the west. It is distinct from the Toronto Islands across the Inner Harbour. [Waterfront Toronto island project](https://www.waterfrontoronto.ca/our-projects/ookwemin-minising).

Cherry North connects the mainland to the island across Keating Channel. Its separate transit span is reserved for transit, not proof of operating streetcar service. Commissioners crosses the new river to the eastern Port Lands. Cherry South crosses the new river mouth farther south; the older Cherry Ship Channel bascule crossing is another bridge. The 2024 backgrounder documents red/white North, orange/white Commissioners and yellow/white South bridge character, Cherry's westward realignment, retained older island alignment and rebuilt Commissioners. [Road/bridge backgrounder](https://www.waterfrontoronto.ca/sites/default/files/2024-01/Port%20Lands%20Road%20and%20Bridges%20Backgrounder%20AODA.pdf), [City completed-infrastructure record](https://secure.toronto.ca/council/agenda-item.do?item=2026.EX29.2).

Biidaasige Park opened eastern river parkland in 2025 and additional western parkland in July 2026. Old Fire Hall 30 is at 39 Commissioners; the west has a promenade, Canoe Cove and preserved Atlas Crane. These sources distinguish opened space from later phases. [Park project](https://www.waterfrontoronto.ca/our-projects/biidaasige-park), [July 2026 opening](https://www.waterfrontoronto.ca/news/torontos-largest-park-in-a-generation-just-got-bigger), [City fire hall](https://www.toronto.ca/explore-enjoy/parks-recreation/places-spaces/beaches-gardens-attractions/biidaasige-park/book-an-amenity-at-biidaasige-park/).

Unwin connects Cherry and Leslie south of Ship Channel, with bends and a separate crossing of Hearn's circulation channel. The April 2025 City study documents the existing single-lane Bailey bridge and a proposed replacement; it does not establish that replacement's completion. Do not replace this channel with a fictitious straight street or claim a live closure. [Unwin bridge study](https://www.toronto.ca/legdocs/mmis/2025/ie/bgrd/backgroundfile-254868.pdf).

No official map, photograph, artwork, logo or raw GIS geometry is included here. The registered 2025 [park reference map](https://www.waterfrontoronto.ca/sites/default/files/2025-08/Biidaasige_Park_Map_August_7%2C_2025_FINAL-ua.pdf) carries a City reproduction-permission notice; it is reference material, not a tracing template. The current web fetch exceeded its size limit; this pass uses the registered prior visual review, new primary text and live City geometry rather than claiming a fresh visual review of that PDF. Proposed bridge arches, facades and paths below must be original pixels. Copy no public-art sculpture or Indigenous design motif; factual place names remain intact.

City street/boundary facts use the [Toronto Centreline](https://open.toronto.ca/dataset/toronto-centreline-tcl/) and [Former Municipality Boundaries](https://open.toronto.ca/dataset/former-municipality-boundaries/) sources under [Open Government Licence – Toronto v1.0](https://www.toronto.ca/city-government/data-research-maps/open-data/open-data-licence/). Preserve **Contains information licensed under the Open Government Licence – Toronto.** and the licence link with derived metadata/distributed packages. The licence excludes third-party rights and official symbols and conveys no endorsement. Waterfront Toronto/TTC material is a factual reference only; the City dataset licence does not license their maps or artwork. Existing [root notices](../../../THIRD_PARTY_NOTICES.md) retain these boundaries.

### Read-only boundary/topology audit

Live in-memory queries used City Centreline `cot_geospatial/MapServer/2/query` and former-municipality `cot_geospatial27/MapServer/6/query`. Centreline selected `LINEAR_NAME_FULL IN ('Cherry St','Ookwemin St','Commissioners St','Don Roadway','Lake Shore Blvd E','Unwin Ave','Leslie St','Carlaw Ave','Saulter St S','Villiers St','Polson St','Regatta Rd','Old Cherry St')`, envelope `[-79.374,43.626,-79.318,43.661]`, `inSR=outSR=4326`, envelope/intersects, GeoJSON, geometry plus centreline/name/from/to/status/direction fields. It returned **123 features**, without a transfer-limit flag. Shared FROM/TO IDs yielded **19 named junctions**, all inside the actual `AREA_NAME='TORONTO'`, `OBJECTID=14035041` polygon by point-in-polygon testing. No `Old Cherry St` records returned; **Ookwemin St** is already the name for the older island alignment in this response.

| Junction | Intersection ID | Rounded longitude, latitude | Former Toronto |
| --- | --- | --- | --- |
| Cherry / Lake Shore | 30155158 | `−79.356449, 43.648997` | inside |
| Cherry / Commissioners | 30137961 | `−79.353630, 43.644810` | inside |
| Ookwemin / Villiers | 13467182 | `−79.353735, 43.646882` | inside |
| Ookwemin / Commissioners | 13467444 | `−79.352436, 43.645307` | inside |
| Don Roadway / Lake Shore | 13466497 | `−79.346945, 43.651008` | inside |
| Don Roadway / Commissioners | 13466878 | `−79.345028, 43.648676` | inside |
| Carlaw / Commissioners | 13466194 | `−79.335540, 43.652615` | inside |
| Leslie / Commissioners | 13465538 | `−79.326336, 43.656515` | inside |
| Cherry / Polson | 13467736 | `−79.350581, 43.643045` | inside |
| Cherry / Unwin | 13468233 | `−79.346662, 43.639063` | inside |
| Regatta / Unwin | 13467847 | `−79.341221, 43.642113` | inside |
| Leslie / Unwin | 13466203 | `−79.322920, 43.652348` | inside |

Centreline response SHA-256: `1e62af648fecc5517297d0472d5e61b7d0ee8579e692d5bdf685cd7130444444`. Boundary query used `where=AREA_NAME='TORONTO'`, `outFields=AREA_NAME,OBJECTID`, `outSR=4326`, `returnGeometry=true`, `f=geojson`; response SHA-256: `1db98bac67409e1a658469ebf5cc981f69233f4df049777010e3d58fa74ebaf5`. Different field selection explains a different response hash from earlier eastern research. These hashes identify research responses, not generated asset hashes.

Selected points inside the polygon do not validate every proposed pixel, parcel or endpoint. Before importing any geometry, retain the dataset licence/version, query/download identity and hash; clip source roads and proposed landmark reference points to the actual former TORONTO polygon. Then author a separate compressed graph. Do not use a bounding rectangle, modern ward or all Toronto East York as the scope mask. Water and later infill require explicit shoreline interpretation rather than treating every point inside the municipal polygon as dry land. This proposal adds no GTA municipality.

## Original compressed source layout

Local `(u,v)` has east right and north up; corridor coordinates below are **original authored game pixels**, not surveyed geometry. The registered collision/art metadata is authoritative for runtime footprints. Use 128 × 122 tiles. Major asphalt remains 48 pixels wide with 8-pixel sidewalks per side; give trucks 64-pixel loading aprons and clear braking approaches. Preserve narrow crossings as deliberate exceptions with readable approaches. Cardinal steps convey the actual northeast trend; no oblique steering is required.

Keep the island in the western half, industry to its east and south, Cherry Beach south of Unwin, and the Inner Harbour west. A river/channel is blocked to foot and vehicles except the explicitly listed bridge decks. Bank protection, vegetation and fences must leave visible pavement boundaries. Only building ground footprints block; raised roof/canopy pixels carry occlusion without blocking the adjacent footpath.

| Corridor | Authored centreline points | Generation rule |
| --- | --- | --- |
| Lake Shore East reference strip | `(24,128)→(672,128)→(672,64)→(976,64)` | Cross Don at `(464,128)` on a deck; no invented King/Front bridge |
| Cherry Street | `(192,24)→(192,832)` | North, South and Ship Channel are three separate decks, not filled land |
| Commissioners | `(192,352)→(336,352)→(336,288)→(592,288)→(592,224)→(912,224)` | Bridge over eastern Don arm at `(464,288)`; broad industrial approaches |
| Ookwemin Street | `(272,256)→(272,352)` | Older island alignment, east of realigned Cherry; no mainland channel crossing |
| Villiers | `(272,256)→(400,256)` and `(512,192)→(544,192)` | Separate bank fragments; **no drivable river bridge** between them |
| Don Roadway | `(544,128)→(544,288)` | East of the river; stop at Commissioners, do not extend across Ship Channel |
| Carlaw southern approach | `(736,24)→(736,224)` | Join Commissioners; no bridge over Turning Basin |
| Leslie | `(912,24)→(912,768)` | Eastern perimeter; joins Lake Shore, Commissioners and Unwin |
| Polson | `(128,576)→(192,576)` | Westbound dead end on Polson Quay, south of new river mouth, inset from harbour |
| Unwin | `(192,824)→(864,824)→(864,768)→(912,768)` | Retain two bends east of circulation-channel crossing near `(808,824)`; width/status requires baseline choice |
| Regatta | `(352,824)→(352,864)` | Parking/service approach ends before beach promenade; no road on beach |
| Fictional Turning Basin service drive | `(672,224)→(672,576)→(704,576)` | Original private courier-yard access; does not assert a surveyed public street or cross Ship Channel |

Water-mask starting shapes, expressed as `[x,y,width,height]`, are original schematics: harbour `[0,168,96,760]`; Keating `[96,168,392,48]`; eastern Don `[432,0,64,528]`; mouth `[96,464,400,72]`; Ship Channel `[96,640,704,64]`; Turning Basin `[752,608,112,120]`; Hearn circulation channel `[792,704,32,224]`; lake `[0,936,1024,40]`. The Don Greenway spillway/wetland strip `[432,536,64,104]` is blocked terrain, without claiming permanent open water. Unite connected water shapes, then reopen **only** Cherry North `u=192,v=168..216`, Cherry South `u=192,v=464..536`, Commissioners `v=288,u=432..496`, Lake Shore `v=128,u=432..496`, Cherry Ship Channel `u=192,v=640..704`, and the separate Unwin crossing. Extend bridge approaches past banks; whole-footprint clearance determines the eventual legal deck, not the centre point alone. The generator resolves bank/deck meetings and validates full footprints; registered metadata/collision bytes, rather than this abbreviated water-shape list, determine legal movement.

Biidaasige east occupies the island river edge around `(304,392)`, west occupies `(128,392)`; fictional walking circuits are 16 pixels wide and connect to Commissioners/Cherry pavements without fording water. Leave wetlands blocked. Any additional park footbridge needs its specific location checked against the official reference before generation; a proposed path may not quietly create a new river crossing. South-bank paths remain reachable from Cherry South. Beach walking promenade at `v=888` stays vehicle-blocked, connected to Regatta parking by a short path.

### Native integration seams

The East Leslie corridor now extends from `(816,256)` to `(816,952)` and its southern frontier opens only at the authored lane. Registered reciprocal East `(816,952)` ↔ Port `(912,24)` is the **sole** Port seam. District 4 occupies atlas cell `(3072,976)` without moving existing local coordinates; endpoints need not share the same atlas x. Core has a compressed Don/Island strip without a researched Cherry/Keating gateway. Core Cherry and East Carlaw remain withheld. The five-scene 512 × 244 atlas passes static banking/budget checks and sampled second-row native pans; complete viewport/marker coverage remains separate.

| Approach status | Port Lands arrival | Geographic link / acceptance gate |
| --- | --- | --- |
| **Withheld** Core Cherry approach; choose only after core Don-mouth review | `(192,24)` | Mainland Cherry → Lake Shore → Cherry North; cutline lies north of Keating so the player drives the actual new bridge in this scene |
| **Registered** East Leslie `(816,952)` | `(912,24)` | Leslie → Lake Shore → Port Lands; source and retained native foot/car travel pass |
| **Withheld** East Carlaw extension from `(384,720)` to a future `(384,952)` | `(736,24)` | Carlaw → Lake Shore → Commissioners; reciprocal lane and rail/grade clearance must be generated |

Do not stamp a Core portal over the current harbour or Island art. First inspect/re-author the actual Cherry/Lake Shore approach and validate road/foot connectivity to the retained Core network, parked-car recovery and destination cues. A Don Roadway northern seam may follow only after its East Harbour approach is researched and authored. Keep existing three Core/East crossings unchanged during the first Port Lands milestone. Omit future Equinox/Keating pedestrian bridges and future Waterfront East streetcars from initial routing.

## Buildings, original art and useful jobs

Use fewer enormous footprints alongside small service structures, rather than a uniform downtown tower grid. Authored landmark ground boxes below are `[x,y,width,depth]`; generator checks must reject road/water/footpath overlap. Tall silhouettes need roof priority and clear base edges, with the same blocked footprint used by art and collision.

| Anchor / character | Original pixel treatment and footprint | Courier use |
| --- | --- | --- |
| Old Fire Hall 30, south of Commissioners on the island | Warm brick, pitched roof, small tower; `[288,392,32,32]` | Fictional community parcel entrance on the pavement, not a fire emergency mission |
| Atlas Crane / west park | Open-frame crane silhouette near `(136,440)` with compact fenced base; promenade keeps clearance | Landmark near a fictional park handoff; no copied sculpture or logo |
| McCleary / north Commissioners industry | 32–64-pixel workshops, yards and occasional office block; no unbuilt skyline | Short parcel pickup or returns stop |
| Studio corridor east of Don Roadway | Two 64–96-pixel soundstage halls with sawtooth/skylight roof modules, loading apron | Fictional **Channel Stage**, not a real studio/loading-door claim |
| South Port / Turning Basin yards | Broad low warehouses, grain bins, tanks, fenced outdoor stacks, crane rails | Truck load/drop at an apron; yard fences prevent free shortcuts |
| Hearn north of Unwin, between Cherry and Leslie | Broad stepped brick hall `[560,720,144,64]` and a tall separate stack | Landmark with a fictional adjacent **Unwin Works** handoff; actual restricted building interior unavailable |
| Cherry Beach / Regatta | Compact pavilion, washroom hut, trees, open sand and water edge | Park car at `(352,856)`, walk to `(432,888)` |

Hearn's City record supports its north-Unwin location and massive brick hall/stack silhouette; building dimensions above are our compression. [City Hearn notice](https://secure.toronto.ca/nm/api/individual/notice/710.do). Industrial and creative uses derive from the City district overview, not copied business branding. Reuse tile modules for corrugated roofs, brick bays, tanks, dock bollards, sand/reeds and bridge rails; aim at the existing ≤320-pattern art budget, then verify actual tile-bank/palette limits. **Compiled background bank 1 must use at most 32 tiles before aircraft scratch IDs 32..46**; a source-pattern count alone does not establish that gate. Dynamic NPC/traffic budgets must be established by the benchmark before adding actors. Aircraft remain cosmetic and use the existing mechanism.

Eight original contracts are appended in source after the preserved 88, for **96 contracts/59 stops**. [The deterministic generator](../scripts/create_port_jobs.py) and [authored content](../content/districts/port_lands_jobs.json) pin all previous native stop/job/brief fields. Client IDs 51–58 follow the unchanged Queen platforms 43–50. Existing loading is a stopped interaction; no loading delay, signature system, new actor or SRAM field is introduced.

| Contract / source title | Ordered route and rule | Meaningful choice / initial deadline |
| --- | --- | --- |
| 89 Channel Stock Run | Union (0) → Channel Stage (51) `(672,272)`; truck freight | Brake into the studio apron after Leslie; 170s |
| 90 Fire Hall Books | Riverside (37) → Fire Hall (52) `(312,432)`; parcel, any vehicle/foot | Park `(304,352)`, modeled 104px final walk; 175s |
| 91 Crane Walk Packet | Union (0) → Crane Walk (53) `(144,432)`; light relay | Drive/park `(192,352)` versus existing Queen 49 plus Leslie/park walking; 205s |
| 92 Turning Basin Load | Stage (51) → Unwin (55) `(616,808)` → yard (54) `(704,576)`; truck | Equipment circuit needs real bridge/yard approaches rather than a five-second isolated drop; 185s |
| 93 Unwin Glass | Carlaw (39) → Unwin (55); fragile, car | Leslie/Unwin approach or legal internal bridge loop; preserve condition; 150s |
| 94 Beach Mail | Fire Hall (52) → Riverbank (58) `(352,592)` → Beach (56) `(432,888)`; parcel | Continuous park walking versus driving/reparking at Riverbank `(192,552)` and Beach `(352,856)`; 180s |
| 95 Harbour Paper Run | Polson (57) `(128,576)` → St Lawrence (1); express, motorcycle | Return through Leslie/Queen and choose legal connected lanes; no western Core seam; 120s |
| 96 Stage Return Kit | Yard (54) → Stage (51) → Carlaw (39) → yard (54); car | Closed pickup/handoff/return circuit using existing rules; 200s |

Unlock gates are 8/3/8/8/6/12/6/12 unique completions. Full-foot/body source checks validate all eight entrances, clear half-eight car loading/parking bodies and a connected half-two foot component of 10,678 tiles from Leslie. Four new foot-only clients 52/53/56/58 have modeled anchor walks 104/128/112/200px; seven total anchors preserve the stop/save layouts. These are fictional compressed entrances, not surveyed venues/access rules. Matching `c625…` ordinary play verifies Fire Hall Books/client 52 delivery and parked-car recovery; the other seven jobs and wider loaded-occupancy interactions remain pending.

Deadlines are provisional estimates using actual collision routes and the existing generator formula. The eight jobs model 248–282.3 moving seconds plus 100 seconds of assumed handling, not imposed waits or observed gameplay. Beach Mail's parked-car model walks back to anchors; the native rules permit walking continuously instead. Count, deadline sums and these estimates do not establish two hours or fun. Existing 16-byte completion bitmap/58-byte v8 state fits 96 IDs; old 88-job ROMs reject saved new jobs/completion bits. No 114 bus is added.

TTC's official visitor guide supports **114 Queens Quay East** from Union to Commissioners/Cherry and a **seasonal 202 Cherry Beach** stop at Cherry/Unwin. Begin with a proposed Union ↔ Commissioners/Cherry bus connection using fictional shared curb signs, fare/frequency and the existing paid-ride state machine; verify exact stop identities before registration. Do not substitute 121 Esplanade–River as a Port Lands route. Waterfront East's dedicated streetcar extension remains design work; an installed transit bridge is not an operating train. [TTC visitor connections](https://www.ttc.ca/riding-the-ttc/Explore-Toronto), [TTC future waterfront network](https://www.ttc.ca/about-the-ttc/projects-and-plans/Waterfront-Transit-Network-Expansion). Bus choice should shorten light-cargo travel to the park while keeping the parked truck/car recoverable; heavy freight cannot teleport with its vehicle.

## Implemented generation and remaining acceptance

The [layout module](../scripts/port_lands_layout.py) and [art generator](../scripts/create_port_lands_art.py) now produce registered native assets: **26 buildings, 117 raw / 98 flip-canonical patterns, six swept-clear traffic loops and 79 full-footprint-safe pedestrian routes**. The art/collision checks validate clients/parking, foot/car connectivity, six supported decks and water blocked elsewhere. Only Leslie's border is open; inset Core/Carlaw approaches remain sealed. Metadata exports rectangle lists as `water` and named shapes as `water_bodies`. The initial dry-run preceded generation; it is historical, not the current registration status. Across five scenes, the atlas has 589 patterns/13,507 bounded banked bytes, a worst visible 164/172 pattern budget and no new persistent WRAM. Compiled gates on retained five-scene builds protect aircraft bank-one scratch IDs 32–46; newer builds require their own checks.

Retained **`a243834899097d9660190022f03ce31d322d2584d0d7cfe1fdf8adb84a8902ea`** has a separate [ordinary-control Port record](NATIVE_PORT_DRIVING_SAMPLE.json), scoped passed at 9,988 frames. It enters by car through East Leslie, crosses Lake Shore/Cherry North/Commissioners/Cherry South/Ship Channel/Unwin decks, parks and walks Commissioners, genuinely resets/restores the Port walker/car, re-enters, walks Beach and returns by car to East. The clear-gap Beach water block is `(423.625,935.625)`; the older `(432,919.5)` stop is a tree, not the lake. H2/H3 escalation occurs with cash already zero, so it does not establish higher fine amounts. This 88-contract ROM does not verify the 96-contract source.

Current **`c625e6dc…`** compiles 96/59/seven anchors and passes full 699,955-engine-check/build/header/compiled/memory gates. The [closed 11,024-frame sample](NATIVE_PORT_CAMPAIGN_SAMPLES.json) completes three original jobs, unlocks Fire Hall Books/index 89, picks up in Riverside and enters through Leslie. Parking near `(309.9375,351.3125)` requires walking around the Fire Hall's east edge rather than through its blocked footprint. At 10462 the handoff near `(312,432)` completes with condition 36 / cash 71 / done 4 and bit 89 saved; genuine reset restores that foot/Port-car checkpoint and re-entry succeeds. This verifies **one of eight** new contracts, not all source jobs or deadlines. H3/capture/damaged payout is sampled; exact higher fine amounts and balance are not isolated.

1. Broaden native coverage on the exact 96/59 `c625…` candidate. Its clock fast path samples 110/360 versus retained 108, a tiny scoped difference without substantial/whole-city performance proof. Source 699,955 checks cover ordered handoffs, foot 58/parking, old saves, completion 95 and truck transit rejection; host cases do not replace ordinary native job play.
2. Play the other seven contracts and broaden Fire Hall coverage through ordinary controls: correct district/order, truck loading, fragile condition, motorcycle express, Fire Hall/Crane/Riverbank/Beach foot handoffs, continuous walking choice, closed return, cancellation/timeout and saved active new-job recovery. Tune deadlines/condition/rewards from retained observations.
3. Broaden truck/motorcycle/scooter bridge handling, loaded occupancy, roof/crane/boat deck occlusion and crowded pacing. Keep future Core/Carlaw seams closed until their researched approaches and full-body connectivity are authored and tested. Every current Core→Port journey uses East Leslie.
4. Research/register a later 114 connection separately, with exact stop identities, original curb signs, fictional fare/timing and paid map/reset/alighting tests. Heavy freight remains road-vehicle-bound. No seasonal 202 or future waterfront streetcar service is implemented by these jobs.
5. Measure practiced and first-time normal-speed campaign play, meaningful route choice and enjoyment. These jobs expand purposeful scope; measured two hours, full Old Toronto/fuller Islands and physical Chromatic/cartridge acceptance remain unfinished.
