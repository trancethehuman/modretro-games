# Old Toronto world reference

Researched 2026-10-02. Scope adopted from the user: **the former City of Toronto, its waterfront, and the Toronto Islands**. Exclude surrounding municipalities and the GTA. This is a present-day city game within a historical municipal footprint, not a game set before amalgamation.

## Maps inspected

| Reference | What it establishes | Limit |
| --- | --- | --- |
| [Former Toronto zoning key](https://www.toronto.ca/city-government/planning-development/zoning-by-law-preliminary-zoning-reviews/zoning-bylaws/former-city-of-toronto-zoning-information/) | Visually inspected the highlighted former-city footprint, including its irregular northern extension | Small overview; not a precise parcel/boundary polygon |
| [City neighbourhood map](https://www.toronto.ca/wp-content/uploads/2022/04/964e-New-158-Neighbourhoods-with-Streets-and-List-Table.pdf) | Visually inspected broad neighbourhood relationships, major streets, shoreline and central waterfront inset | Statistical map across the amalgamated city; use only in-scope areas, and verify its currency before importing boundaries |
| [TTC downtown map](https://cdn.ttc.ca/-/media/Project/TTC/DevProto/Images/Home/Routes-and-Schedules/Landing-page-pdfs/TTC_DowntownMap.pdf?rev=bae64f7e21bb4370b91cd5938956c2c8) | Visually inspected September 2026 downtown street/transit layout and landmark positions | Schematic transit map, not exact street geometry; detours are time-specific |
| [Waterfront BIA map](https://www.toronto.ca/wp-content/uploads/2021/10/97b2-edc-Waterfront-BIA-map2021-accessible.pdf) | Visually inspected piers, Queens Quay, island shoreline and three ferry connections | The map itself is dated May 2019; the download path says 2021. It is a BIA boundary map, not the full waterfront or current Port Lands |
| [Island Park Master Plan](https://www.toronto.ca/legdocs/mmis/2024/cc/bgrd/backgroundfile-247910.pdf) | Visually inspected Figure 6-1, printed page 162 / PDF page 39: Hanlan's Point, Centre Island, Ward's Island, Gibraltar Point, Inner Harbour and terminal | Includes a master plan; proposed improvements are not proof of completed infrastructure |
| [Biidaasige Park map](https://www.waterfrontoronto.ca/sites/default/files/2025-08/Biidaasige_Park_Map_August_7%2C_2025_FINAL-ua.pdf) | Visually inspected the new Don River channel, bridges, Ookwemin Minising, Commissioners Street and park paths | The sign map is not north-up; follow its compass. Reproduction requires permission according to its notice |

Source PDFs and renderings are retained only in ignored `.local/research/` for inspection. No published map image or geometry is incorporated into game art. This document records observations and original design interpretations.

## City structure to preserve

The historical footprint is irregular. Do not replace it with a downtown rectangle, a TTC service area, or the larger Toronto-and-East-York community council area. Exact clipping of edge districts and the northern boundary remains to be checked against geographic boundary data before the production map is drawn.

The inspected downtown map provides useful ground-plane ordering:

- Bloor / Danforth is the northern east-west transit spine of the initial central districts; College/Carlton, Dundas, Queen, King and Front lie progressively farther south in the core, with local bends and exceptions.
- Key downtown north-south streets appear west-to-east as Dufferin, Ossington, Bathurst, Spadina, University, Bay, Yonge, Church, Jarvis, Sherbourne, Parliament and the Don corridor. This is an orientation guide, not an exhaustive parallel-road grid.
- The Union rail corridor separates much of the downtown street fabric from South Core and waterfront districts. Street crossings need actual underpasses/bridges, not arbitrary gaps in railway scenery.
- Preserve the Don River as a meaningful east-west crossing constraint. Queen, Dundas, Gerrard and Bloor/Danforth crossings need individual research before gameplay use.
- Lake Ontario is south of the city. The central waterfront has slips and piers, not a straight rectangle. The Islands sit across the Inner Harbour; they are not a road-connected continuation of downtown.

Use [Toronto Centreline](https://open.toronto.ca/dataset/toronto-centreline-tcl/) for detailed road graph work. The maps above establish layout and research priorities; lane directions, turn restrictions and collision routes still need verification.

## Neighbourhood art and gameplay groups

These are **proposed production groups**, not assertions that the named places share official boundaries. The City [explains that statistical neighbourhoods do not fully describe historical and cultural neighbourhoods](https://www.toronto.ca/city-government/data-research-maps/neighbourhoods-communities/neighbourhood-profiles/about-toronto-neighbourhoods/). Use familiar names for player orientation and retain geographic source boundaries separately.

| Group | Places to represent | Original art direction | Delivery character |
| --- | --- | --- | --- |
| Downtown / civic core | Financial District, Downtown Yonge, Nathan Phillips Square, South Core | Tall glass blocks, stone civic buildings, station columns | Office parcels, pickup queues, dense signals |
| Old Town | St. Lawrence, Corktown, Distillery | Red brick, triangular corner building, warehouses, cobbled pedestrian space | Market parcels and loading stops outside pedestrian-only areas |
| Central west | Chinatown, Kensington Market, Grange Park, Queen West, Trinity Bellwoods | Low shopfronts, awnings, narrow streets, park canopy | Small parcels, constrained parking, permitted lane alternatives |
| West industrial / residential | Liberty Village, Parkdale, Roncesvalles | Converted brick factories, rail edges, mixed main streets | Truck loading versus smaller-vehicle access |
| Far west within old-city footprint | High Park, Swansea, Sunnyside, Junction area | Large green park, lakefront paths, railway junction character | Longer journeys with clear main-road choices |
| North central | Annex, University of Toronto, Yorkville, Casa Loma; later northern in-scope areas | Campus stone, tree-lined houses, museum crystal, hilltop castle | Campus/event deliveries and constrained access |
| East bank | Riverside, Riverdale, Leslieville | Bridge approaches, brick storefronts, residential side streets | Crossing choice and east-west streetcar traffic |
| East main streets | Danforth, Gerrard/Little India, Beaches | Distinct retail strips, terraces, beach/boardwalk edge | Longer corridor jobs; promenade remains outside ordinary driving lanes |
| Waterfront / industrial east | Harbourfront, East Bayfront, Port Lands, Cherry Beach | Water slips, silos, cranes, bridges, parkland and warehouses | Waterfront queues, heavy cargo, controlled industrial access |
| Islands | Hanlan's Point, Centre Island, Ward's / Algonquin, Gibraltar Point | Beaches, lagoons, footbridges, cottages, lighthouse | Ferry-connected jobs; public car access excluded |

Character research references: [City Centre](https://www.destinationtoronto.com/neighbourhoods/city-centre/), [Old Town](https://www.destinationtoronto.com/neighbourhoods/old-town/), [Midtown](https://www.destinationtoronto.com/neighbourhoods/midtown/), [Eastside](https://www.destinationtoronto.com/neighbourhoods/eastside/), and [High Park area](https://www.destinationtoronto.com/neighbourhoods/high-park/). Tourism group boundaries may extend beyond the game's adopted municipal scope; do not use them as the clipping boundary.

## Landmark shortlist for original pixel art

Anchors below are coarse research positions. Drawing scale, palettes, visibility rules and mission roles are original proposals. Precise entrances and road access need collision-map validation.

| Landmark | Geographic anchor | Features to retain in pixel art | Proposed purpose / reference |
| --- | --- | --- | --- |
| CN Tower | Near Front / Bremner in the Entertainment District | Slender shaft, observation pod and antenna; height suggested by scene composition | City orientation; [tower history](https://www.cntower.ca/history) and TTC map |
| Rogers Centre | West of CN Tower | Retractable roof silhouette | Event deliveries; [venue reference](https://www.mlb.com/bluejays/ballpark/information/history) |
| Union Station | 65 Front Street West, Bay–York block | Long symmetrical stone frontage and column rhythm | First depot area; [City address](https://www.toronto.ca/services-payments/venues-facilities-bookings/booking-city-facilities/union-station/) and [architecture](https://www.toronto.ca/services-payments/venues-facilities-bookings/booking-city-facilities/union-station/history-of-union-station/) |
| City Hall / Nathan Phillips Square | 100 Queen Street West | Two unequal curved towers around a low round council chamber; open square | Civic job hub; [heritage description](https://www.toronto.ca/legdocs/pre1998bylaws/toronto%20-%20former%20city%20of/1991-0147.pdf) |
| Old City Hall | Queen / Bay, east of new City Hall | Heavy masonry and prominent clock tower | Adjacent navigation marker; [City view-protection reference](https://www.toronto.ca/wp-content/uploads/2018/03/9712-City-Planning-Downtown-Tall-Building-Web.pdf) |
| Gooderham / Flatiron Building | 49 Wellington Street East; Front/Wellington wedge | Narrow triangular plan, rounded corner turret, brick body | Memorable cornering landmark; [City heritage record](https://secure.toronto.ca/HeritagePreservation/details.do?folderRsn=2437345&propertyRsn=211418) |
| St. Lawrence South Market | 93 Front Street East | Broad market volume and recognisable frontage; roof details need photo study | First delivery stop; [market address/reference](https://www.stlawrencemarket.com/events/event_detail/784/Public%20Markets%20Week%20Tour) |
| Distillery complex | Mill / Trinity / Parliament area | Clustered Victorian industrial brick buildings, chimneys and pedestrian courts | Drop-off at a researched accessible edge; [district description](https://www.thedistillerydistrict.com/about/) and [City study](https://www.toronto.ca/legdocs/mmis/2016/pb/bgrd/backgroundfile-98819.pdf) |
| Royal Ontario Museum | Bloor / Queen's Park | Angular silver/glass Crystal attached to heritage stone | Cultural delivery hub; [ROM architecture](https://www.rom.on.ca/about-rom) and [location](https://www.rom.on.ca/visit/location-parking) |
| Art Gallery of Ontario | Dundas / McCaul | Long bowed glass frontage with warm wood ribs | West-central landmark; [AGO architectural description](https://ago.ca/around-block-audio-description-tour) |
| OCAD Sharp Centre | 100 McCaul, near AGO | Black/white elevated box and coloured supporting legs | Distinct skyline block; [OCAD reference](https://www.ocadu.ca/events-and-exhibitions/exan-visiting-artist-series-graeme-patterson) |
| Casa Loma | 1 Austin Terrace | Hilltop stone castle, turrets and gardens | Later northern mission; [official venue](https://casaloma.ca/project/the-rooms/) |
| R.C. Harris plant | 2701 Queen Street East, lake edge | Buff masonry, stepped terraces and Art Deco geometry | East-end landmark; [City designation](https://www.toronto.ca/legdocs/bylaws/1998/law0303.htm) |
| Gibraltar Point Lighthouse | Gibraltar Point on the Islands | Compact stone lighthouse silhouette | Island landmark; [City Island guide](https://www.toronto.ca/explore-enjoy/toronto-island-ferries/things-to-do-on-toronto-island/) |

## Waterfront and Island mechanics

[City access guidance](https://www.toronto.ca/explore-enjoy/toronto-island-ferries/getting-around/) restricts Island vehicles to emergency and commercial service use. Ordinary car/motorcycle/scooter roaming would undermine the accuracy requirement. Proposed options are parking on the mainland and carrying a parcel on foot via ferry, or a specifically authorised service-vehicle mission. A cycling courier could be added later if the user wants another vehicle category; it is not an accepted replacement for the requested vehicles.

Keep three destinations distinct: Hanlan's Point to the west, Centre Island in the middle, and Ward's Island to the east, with internal trails, bridges and lagoons. The mainland terminal is the Jack Layton Ferry Terminal. Do not invent a car bridge over the harbour or merge the Island airport with unrestricted park roads.

The [2025 Biidaasige map](https://www.waterfrontoronto.ca/sites/default/files/2025-08/Biidaasige_Park_Map_August_7%2C_2025_FINAL-ua.pdf) is needed for the changed Don mouth and Ookwemin Minising. Select a map baseline explicitly; do not build present-day Port Lands from the 2019 waterfront reference. Distinguish planned projects from opened infrastructure. The current route/road baseline remains proposed, not fixed.

R.C. Harris is a public-view landmark with [restricted vehicle entry](https://www.toronto.ca/services-payments/water-environment/tap-water-in-toronto/fast-facts-about-the-citys-water-treatment-plants/). Use a roadside destination or explicitly authorised fictional service job rather than unrestricted driving into the real facility.

## First-map production order

1. Validate a small Union–St. Lawrence–Distillery road graph, keeping the rail corridor, Gooderham wedge and pedestrian Distillery courts recognisable.
2. Establish isometric rendering and car handling on one intersection before drawing district-scale art.
3. Extend south through real rail crossings to Queens Quay, Harbourfront and ferry terminal.
4. Extend west/central/east districts along researched street links, then add Island transitions and industrial jobs.
5. Resolve exact historic boundary clipping and northern coverage before declaring the whole Old Toronto map complete.

No playable map, collision graph, island transport system or landmark sprite is implemented by this research. Map source URLs and reviewed scopes are also indexed in `content/sources.json`.
