# Port Lands native district inputs

Reviewed **2026-10-02**. This is a generation proposal for one new **1,024 × 976** north-up district, alongside the existing four scenes. It is not a generated map, registered native scene, playable quest set or duration claim. The older 768-pixel/17-district proposal does not supply this layout. The working assumption is representative contemporary geography with normal corridors and fictional game timings; the user has not adopted a specific 2026 era. The new Don mouth and opened park portions inform this proposal, while unbuilt neighbourhoods and transit remain future features.

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

## Original compressed generation specification

Local `(u,v)` has east right and north up; all table coordinates are **proposed game pixels**, not surveyed geometry or existing runtime positions. Use 128 × 122 tiles. Major asphalt remains 48 pixels wide with 8-pixel sidewalks per side; give trucks 64-pixel loading aprons and clear braking approaches. Preserve narrow crossings as deliberate exceptions with readable approaches. Cardinal steps convey the actual northeast trend; no oblique steering is required.

Keep the island in the western half, industry to its east and south, Cherry Beach south of Unwin, and the Inner Harbour west. A river/channel is blocked to foot and vehicles except the explicitly listed bridge decks. Bank protection, vegetation and fences must leave visible pavement boundaries. Only building ground footprints block; raised roof/canopy pixels carry occlusion without blocking the adjacent footpath.

| Corridor | Proposed centreline points | Generation rule |
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

Water-mask starting shapes, expressed as `[x,y,width,height]`, are original schematics: harbour `[0,168,96,760]`; Keating `[96,168,392,48]`; eastern Don `[432,0,64,528]`; mouth `[96,464,400,72]`; Ship Channel `[96,640,704,64]`; Turning Basin `[752,608,112,120]`; Hearn circulation channel `[792,704,32,224]`; lake `[0,936,1024,40]`. The Don Greenway spillway/wetland strip `[432,536,64,104]` is blocked terrain, without claiming permanent open water. Unite connected water shapes, then reopen **only** Cherry North `u=192,v=168..216`, Cherry South `u=192,v=464..536`, Commissioners `v=288,u=432..496`, Lake Shore `v=128,u=432..496`, Cherry Ship Channel `u=192,v=640..704`, and the separate Unwin crossing. Extend bridge approaches past banks; whole-footprint clearance determines the eventual legal deck, not the centre point alone. Final bank shapes and the Unwin/Turning Basin meeting need generator review before these sketches become collision masks.

Biidaasige east occupies the island river edge around `(304,392)`, west occupies `(128,392)`; fictional walking circuits are 16 pixels wide and connect to Commissioners/Cherry pavements without fording water. Leave wetlands blocked. Any additional park footbridge needs its specific location checked against the official reference before generation; a proposed path may not quietly create a new river crossing. South-bank paths remain reachable from Cherry South. Beach walking promenade at `v=888` stays vehicle-blocked, connected to Regatta parking by a short path.

### Native integration seams

The existing East artwork ends at Eastern Avenue; its Leslie column is `u=816,v=256..656`. The current Core has a compressed Don and Island strip, not a researched Cherry/Keating road gateway. **No seam below is currently registered.** The first native integration should open **only East Leslie ↔ Port Lands north**; Core and Carlaw gateways stay withheld until their approach geometry is authored and validated. Reserve a fifth district ID only after reading current world/atlas structures. Propose an atlas cell south of East, `(3072,976)`, without moving the four existing local coordinate systems; atlas placement is diagram organisation, and seam endpoints need not share the same global x. A second row requires actual atlas/HUD checks.

| Existing scene approach to author | Port Lands arrival | Geographic link / acceptance gate |
| --- | --- | --- |
| Core researched Cherry approach, inset endpoint to be chosen after core Don-mouth review | `(192,24)` | Mainland Cherry → Lake Shore → Cherry North; cutline lies north of Keating so the player drives the actual new bridge in this scene |
| East Leslie extension from `(816,656)` south to proposed `(816,952)` | `(912,24)` | Existing Leslie → Lake Shore → Port Lands; no Don crossing on this eastern approach |
| East Carlaw extension from `(384,720)` to proposed `(384,952)` | `(736,24)` | Carlaw → Lake Shore → Commissioners; reciprocal lane and rail/grade clearance must be generated |

Do not stamp a Core portal over the current harbour or Island art. First inspect/re-author the actual Cherry/Lake Shore approach and validate road/foot connectivity to the retained Core network, parked-car recovery and destination cues. A Don Roadway northern seam may follow only after its East Harbour approach is researched and authored. Keep existing three Core/East crossings unchanged during the first Port Lands milestone. Omit future Equinox/Keating pedestrian bridges and future Waterfront East streetcars from initial routing.

## Buildings, original art and useful jobs

Use fewer enormous footprints alongside small service structures, rather than a uniform downtown tower grid. Proposed ground boxes below are `[x,y,width,depth]`; generator checks must reject road/water/footpath overlap. Tall silhouettes need roof priority and clear base edges, with the same blocked footprint used by art and collision.

| Anchor / character | Original pixel treatment and proposed footprint | Courier use |
| --- | --- | --- |
| Old Fire Hall 30, south of Commissioners on the island | Warm brick, pitched roof, small tower; `[288,392,32,32]` | Fictional community parcel entrance on the pavement, not a fire emergency mission |
| Atlas Crane / west park | Open-frame crane silhouette near `(136,440)` with compact fenced base; promenade keeps clearance | Landmark near a fictional park handoff; no copied sculpture or logo |
| McCleary / north Commissioners industry | 32–64-pixel workshops, yards and occasional office block; no unbuilt skyline | Short parcel pickup or returns stop |
| Studio corridor east of Don Roadway | Two 64–96-pixel soundstage halls with sawtooth/skylight roof modules, loading apron | Fictional **Channel Stage**, not a real studio/loading-door claim |
| South Port / Turning Basin yards | Broad low warehouses, grain bins, tanks, fenced outdoor stacks, crane rails | Truck load/drop at an apron; yard fences prevent free shortcuts |
| Hearn north of Unwin, between Cherry and Leslie | Broad stepped brick hall `[560,720,144,64]` and a tall separate stack | Landmark with a fictional adjacent **Unwin Works** handoff; actual restricted building interior unavailable |
| Cherry Beach / Regatta | Compact pavilion, washroom hut, trees, open sand and water edge | Park car at `(352,856)`, walk to `(432,888)` |

Hearn's City record supports its north-Unwin location and massive brick hall/stack silhouette; building dimensions above are our compression. [City Hearn notice](https://secure.toronto.ca/nm/api/individual/notice/710.do). Industrial and creative uses derive from the City district overview, not copied business branding. Reuse tile modules for corrugated roofs, brick bays, tanks, dock bollards, sand/reeds and bridge rails; aim at the existing ≤320-pattern art budget, then verify actual tile-bank/palette limits. **Compiled background bank 1 must use at most 32 tiles before aircraft scratch IDs 32..46**; a source-pattern count alone does not establish that gate. Dynamic NPC/traffic budgets must be established by the benchmark before adding actors. Aircraft remain cosmetic and use the existing mechanism.

Append eight original contracts after the existing 88 without replacing their IDs or declaring measured duration. Reuse native rules first; loading means the existing stopped-vehicle interaction, not an arbitrary waiting timer.

| Proposed title | Route / rule | Meaningful decision |
| --- | --- | --- |
| Channel Stage Stock | Existing Core depot → studio apron `(672,272)`; truck freight | Earlier braking and wide approach around Commissioners river crossing |
| Fire Hall Books | Existing East parcel client → foot entrance `(312,432)`; parcel | Park at `(304,352)` and walk the final 80 pixels |
| Crane Walk Packet | Core pickup → west park `(144,432)`; transit-friendly relay | Drive/park versus 114 and promenade walking |
| Turning Basin Cases | Studio apron → yard bay `(704,576)`; truck freight | Correct loading apron rather than cutting through the basin/fence |
| Unwin Glass | Existing East fragile pickup → Works apron `(616,808)`; fragile | Cherry's separate bridges versus Leslie/Unwin bend, preserve condition |
| Beach Mail | Fire Hall pickup → `(432,888)`; on-foot final parcel | Regatta parking or transit plus longer legal walk; no beach driving |
| Harbour Paper Run | Polson handoff `(128,576)` → existing Core client; express | Choose western Cherry approach or longer east perimeter according to traffic |
| Stage Return Kit | Yard `(704,576)` → studio `(672,272)` → existing East depot; return rule | Distinct return leg using existing native contract semantics |

These entrances, clients, loading bays and route points are fictional. Verify each pickup/parking/handoff against generated foot/car graphs, building edges, water masks, loaded occupancy and safe alighting; adjust coordinates before appending campaign content. Freight remains vehicle-bound; a parcel's final park path permits foot travel. Passenger rides can follow once the vehicle/comfort rule and road coverage are played, with fictional clients rather than real-platform affiliation.

TTC's official visitor guide supports **114 Queens Quay East** from Union to Commissioners/Cherry and a **seasonal 202 Cherry Beach** stop at Cherry/Unwin. Begin with a proposed Union ↔ Commissioners/Cherry bus connection using fictional shared curb signs, fare/frequency and the existing paid-ride state machine; verify exact stop identities before registration. Do not substitute 121 Esplanade–River as a Port Lands route. Waterfront East's dedicated streetcar extension remains design work; an installed transit bridge is not an operating train. [TTC visitor connections](https://www.ttc.ca/riding-the-ttc/Explore-Toronto), [TTC future waterfront network](https://www.ttc.ca/about-the-ttc/projects-and-plans/Waterfront-Transit-Network-Expansion). Bus choice should shorten light-cargo travel to the park while keeping the parked truck/car recoverable; heavy freight cannot teleport with its vehicle.

## Next generation and acceptance order

The new [layout module](../scripts/port_lands_layout.py) and [art generator](../scripts/create_port_lands_art.py) were checked with `python3 games/toronto-dispatch/scripts/create_port_lands_art.py --dry-run`: **26 buildings, 117 exact raw / 98 flip-canonical tile patterns, six swept-clear cardinal traffic loops and 79 full-footprint-safe pedestrian routes**. The checks also validate the three inset approaches, only Leslie's open border, all proposed clients/parking anchors, connected foot/car graphs, six supported bridge decks and blocked water outside decks. PNG/attributes/collision metadata were rendered in memory only; this task wrote no project asset or art metadata and registered no scene. Native compiler bank-1 scratch separation, actor/scene integration, play/performance and hardware gates remain pending. The generated metadata exports atlas-compatible rectangle lists as `water` and retains named shapes as `water_bodies`.

1. Select the representative corridor baseline for the Unwin crossing/park phases, retain boundary/provenance, and inspect Core gateway geometry. This need not ask the user to choose a calendar year to prototype the supported existing crossings.
2. Generate original Port Lands PNG/attributes/collision metadata from the proposed graph, resolve water/deck meetings and parking boxes, and validate full footprints. Author the Core/East approaches separately; add reciprocal seams only after both ends pass. Keep local Q4/Q5 coordinates separate from atlas coordinates and inspect fifth-scene/static stack/tile/ROM budgets.
3. Build the matching native scene through the ModRetro plugin. Play ordinary-button Core→Cherry North→Commissioners→East and East→Leslie→Unwin→Cherry Ship Channel→Core journeys, plus park-foot/car recovery, scene/reset recovery, blocked water/fences, roof/crane occlusion and sampled peak traffic/aircraft conditions. A map preview or source graph does not satisfy these gates.
4. Add the eight reachable contracts and one researched bus connection; test freight loading, fragile condition, bus fare/map/reset/alighting and both beach/park walking deliveries. Record failures and timings by exact ROM identity.
5. Measure practiced and first-time quest/campaign play, route choice and enjoyment. These new jobs are purposeful scope, not proof of two hours. Full Old Toronto/fuller Islands and physical Chromatic/cartridge checks remain separate unfinished requirements.
