"""Original compressed eastern city layout; these pixels are not City GIS geometry.

Reviewed junction facts and reuse attribution are exported in east_art.json.
Broadview and the Don are retained in the core: this viewport starts east of them.
"""
WIDTH, HEIGHT = 1024, 976
ROAD_HALF, WALK_HALF = 24, 32


def path(name, points, **extra):
    return {"name": name, "points": points, **extra}


RESEARCH = {
    "reviewed_on": "2026-10-02",
    "map_era": "Unadopted; review date is not an accepted 2026 gameplay era. Stable corridors only, no live closures, schedules, fares or future stations.",
    "boundary": "Former municipality AREA_NAME=TORONTO, OBJECTID=14035041; selected named road junctions tested against its actual polygon. This authored viewport is not a projected or complete municipality clip.",
    "licence": {
        "name": "Open Government Licence - Toronto", "version": "1.0",
        "url": "https://www.toronto.ca/city-government/data-research-maps/open-data/open-data-licence/",
        "attribution": "Contains information licensed under the Open Government Licence – Toronto.",
        "included": "Selected junction IDs, rounded coordinates, inside-boundary results and road/rail connectivity facts; no raw official geometry, map imagery, photos or logos bundled.",
        "rights": "City portals declare this licence. Commercial adaptation/distribution requires attribution; third-party rights and official marks/logos are excluded, and no endorsement is implied.",
        "publication_action": "Retain this attribution and source links with derived facts; update THIRD_PARTY_NOTICES.md before publishing the expanded source assets.",
    },
    "sources": [
        {"id": "east_city_centreline", "url": "https://open.toronto.ca/dataset/toronto-centreline-tcl/", "supports": "Named junctions, street continuity, disconnected Pape road ends and major railway alignment.", "rights": "Open Government Licence - Toronto"},
        {"id": "east_former_boundary", "url": "https://open.toronto.ca/dataset/former-municipality-boundaries/", "supports": "Final former TORONTO polygon, separating Old Toronto from East York/GTA.", "rights": "Open Government Licence - Toronto"},
        {"id": "east_city_licence", "url": "https://www.toronto.ca/city-government/data-research-maps/open-data/open-data-licence/", "supports": "Version 1.0 attribution, adaptation and exclusions.", "rights": "Open Government Licence - Toronto"},
        {"id": "east_viaduct", "url": "https://www.toronto.ca/explore-enjoy/history-art-culture/online-exhibits/web-exhibits/web-exhibits-architecture-infrastructure/bridging-the-don-the-prince-edward-viaduct/", "supports": "Bloor connects to Danforth across the Don; Gerrard and Queen historical crossing context.", "rights": "Reference only; no archival images copied"},
        {"id": "east_lower_don", "url": "https://www.toronto.ca/city-government/planning-development/construction-new-facilities/improvements-expansion-redevelopment/lower-don-trail/", "supports": "Queen/Dundas and Riverdale Park bridge connect to Lower Don trail; construction status is not a gameplay baseline.", "rights": "Reference only"},
        {"id": "east_rail_bridges", "url": "https://secure.toronto.ca/council/agenda-item.do?item=2020.EX16.5", "supports": "Persistent rail bridge corridors at Eastern, Queen, Dundas, Logan, Carlaw and Gerrard; future track works not adopted.", "rights": "Reference only"},
        {"id": "east_pape_footbridge", "url": "https://www.toronto.ca/legdocs/mmis/2018/ex/bgrd/backgroundfile-113811.pdf", "supports": "Page 26 describes an existing Pape pedestrian overpass, with a proposed replacement underpass; no road crossing asserted.", "rights": "Reference only; future project is not an operating station"},
        {"id": "east_jones_corridor", "url": "https://www.toronto.ca/legdocs/1998/minutes/council/appa/cc980603/to6rpt.htm", "supports": "Jones corridor between Danforth and Queen; historical road/bicycle proposal used for continuity only.", "rights": "Reference only; turn rules not adopted"},
        {"id": "east_withrow", "url": "https://www.toronto.ca/legdocs/bills/2024/bill0369.pdf", "supports": "Withrow Park at 725 Logan Avenue.", "rights": "Reference only"},
        {"id": "east_withrow_context", "url": "https://www.toronto.ca/legdocs/pre1998bylaws/toronto%20-%20former%20city%20of/1993-0425.pdf", "supports": "Withrow between Logan, Carlaw and Bain; north-side streets omitted in compressed viewport.", "rights": "Reference only; map neither copied nor traced"},
        {"id": "east_greenwood", "url": "https://www.toronto.ca/city-government/planning-development/construction-new-facilities/park-facility-projects/greenwood-park-playground-improvements/", "supports": "Greenwood Park at 150 Greenwood Avenue.", "rights": "Reference only"},
        {"id": "east_riverside", "url": "https://www.toronto.ca/business-economy/business-operation-growth/business-improvement-areas/bia-list/bia-list-q-y/", "supports": "Riverside Queen corridor from East Don Roadway to Empire; BIA is not a residential neighbourhood boundary.", "rights": "Reference only"},
        {"id": "east_leslieville", "url": "https://www.toronto.ca/business-economy/business-operation-growth/business-improvement-areas/bia-list/bia-list-f-p/", "supports": "Leslieville Queen commercial corridor, Empire to Vancouver; broader neighbourhood name is informal.", "rights": "Reference only"},
        {"id": "east_music_hall", "url": "https://www.thedanforth.com/visit", "supports": "Danforth Music Hall at 147 Danforth, near Broadview; original facade only.", "rights": "Reference only; venue branding and photos not copied"},
        {"id": "east_opera_house", "url": "https://www.theoperahousetoronto.com/about", "supports": "Opera House at 735 Queen East, historical theatre character.", "rights": "Reference only; no logos, signage or photos copied"},
        {"id": "east_ashbridge", "url": "https://www.heritagetrust.on.ca/pages/sites/archaeology/see-what-weve-discovered/ashbridge-estate", "supports": "Ashbridge Estate/Jesse Ashbridge House, built 1854.", "rights": "Reference only"},
        {"id": "east_ashbridge_address", "url": "https://www.heritagetrust.on.ca/user_assets/documents/Inventory-of-OHT-owned-properties-ENG-Sep-30-2019-FINAL.pdf", "supports": "Ashbridge Estate at 1444 Queen East.", "rights": "Reference only"},
        {"id": "east_ttc_line2", "url": "https://www.ttc.ca/routes-and-schedules/2/1/14908", "supports": "Line 2 station order Broadview→Chester→Pape→Donlands→Greenwood; no game schedule implemented here.", "rights": "Reference only"},
        {"id": "east_ttc_chester", "url": "https://www.ttc.ca/subway-stations/chester-station", "supports": "Chester entrance north of Danforth, between Broadview and Pape.", "rights": "Reference only; no TTC logo copied"},
        {"id": "east_ttc_506", "url": "https://www.ttc.ca/routes-and-schedules/506/1/1082", "supports": "506 Carlton serves Gerrard East at Broadview; route name does not imply Carlton crosses the Don.", "rights": "Reference only; no live timetable copied"},
    ],
    "live_query_evidence": {
        "centreline_endpoint": "https://gis.toronto.ca/arcgis/rest/services/cot_geospatial/MapServer/2/query",
        "centreline_envelope_wgs84": [-79.385, 43.645, -79.310, 43.711],
        "centreline_names": ["Danforth Ave", "Broadview Ave", "Carlton St", "Gerrard St E", "Dundas St E", "Queen St E", "Carlaw Ave", "Pape Ave", "Logan Ave", "Jones Ave", "Leslie St", "Greenwood Ave", "Riverdale Ave", "Bain Ave", "Eastern Ave", "Chester Ave", "Donlands Ave", "Withrow Ave"],
        "centreline_feature_count": 786,
        "centreline_response_sha256": "a9e79cc73cc81c24966085f3ae36d2e2109b1590e9e490c9413f6b63aa4a3e85",
        "boundary_endpoint": "https://gis.toronto.ca/arcgis/rest/services/cot_geospatial27/MapServer/6/query",
        "boundary_where": "AREA_NAME='TORONTO'", "boundary_objectid": 14035041,
        "boundary_response_sha256": "e37998611b7167f2fcbf8b264ac0d537a77e3d582726c4af721817be0d0eda24",
        "rail_where": "FEATURE_CODE_DESC LIKE '%Rail%'",
        "rail_envelope_wgs84": [-79.36, 43.650, -79.31, 43.684], "rail_feature_count": 30,
        "rail_response_sha256": "7fac85c7c0265ea5cab0b667f3e8801d85896dd0c123c6b0c38af37399cd0f48",
        "method": "Fetch WGS84 feature geometry in memory; join road names by shared FROM/TO intersection ID; ray-cast selected junctions against the TORONTO polygon. Pape continuity separately inspected from its terminal road edges. Store derived facts only, not raw geometry.",
    },
    "junctions": [
        {"id": 13462375, "names": ["Broadview", "Danforth"], "wgs84": [-79.358873, 43.676202], "inside_former_toronto": True},
        {"id": 13462012, "names": ["Logan", "Danforth"], "wgs84": [-79.349678, 43.677992], "inside_former_toronto": True},
        {"id": 13461936, "names": ["Carlaw", "Danforth"], "wgs84": [-79.347345, 43.678460], "inside_former_toronto": True},
        {"id": 13461839, "names": ["Pape", "Danforth"], "wgs84": [-79.344875, 43.678929], "inside_former_toronto": True},
        {"id": 13461657, "names": ["Jones", "Danforth"], "wgs84": [-79.340178, 43.679875], "inside_former_toronto": True},
        {"id": 13461356, "names": ["Greenwood", "Danforth"], "wgs84": [-79.332374, 43.681462], "inside_former_toronto": True},
        {"id": 13463892, "names": ["Logan", "Gerrard"], "wgs84": [-79.345196, 43.667095], "inside_former_toronto": True},
        {"id": 13463796, "names": ["Carlaw", "Gerrard"], "wgs84": [-79.342763, 43.667629], "inside_former_toronto": True},
        {"id": 13463705, "names": ["Pape", "Gerrard"], "wgs84": [-79.340413, 43.668140], "inside_former_toronto": True},
        {"id": 13464409, "names": ["Logan", "Dundas"], "wgs84": [-79.343873, 43.663919], "inside_former_toronto": True},
        {"id": 13464254, "names": ["Carlaw", "Dundas"], "wgs84": [-79.341607, 43.664844], "inside_former_toronto": True},
        {"id": 13464162, "names": ["Pape", "Dundas"], "wgs84": [-79.339254, 43.665410], "inside_former_toronto": True},
        {"id": 13464948, "names": ["Logan", "Queen"], "wgs84": [-79.342488, 43.660520], "inside_former_toronto": True},
        {"id": 13464855, "names": ["Carlaw", "Queen"], "wgs84": [-79.340079, 43.661058], "inside_former_toronto": True},
        {"id": 13464771, "names": ["Pape", "Queen"], "wgs84": [-79.337699, 43.661584], "inside_former_toronto": True},
        {"id": 13464498, "names": ["Leslie", "Queen"], "wgs84": [-79.330200, 43.663254], "inside_former_toronto": True},
        {"id": 13464286, "names": ["Greenwood", "Queen"], "wgs84": [-79.325296, 43.664373], "inside_former_toronto": True},
        {"id": 13465276, "names": ["Carlaw", "Eastern"], "wgs84": [-79.338967, 43.658402], "inside_former_toronto": True},
    ],
    "pape_discontinuity": {
        "north_road_terminal": {"intersection_id": 13463403, "wgs84": [-79.341077, 43.669695]},
        "south_road_terminal": {"intersection_id": 13463593, "wgs84": [-79.340684, 43.668781]},
        "source_interpretation": "The two Pape road fragments are not joined by a road centreline across the railway. A persistent pedestrian crossing is documented separately. This is not a fictional car prohibition or an adoption of a temporary construction closure.",
    },
    "limitations": ["Junction clipping checks selected source points, not every invented pixel.", "Danforth is not asserted to be the entire former-municipality boundary; East York extensions north of this viewport are omitted.", "Bain is included, smaller Riverdale/Withrow residential streets are omitted at this compression.", "Broadview, Don Jail, Riverdale Park East and the Don river body remain retained-core refinement work, not moved into this viewport.", "Eastern Avenue is a closed eastern approach, not a new Don or Port Lands crossing."],
}
for source in RESEARCH["sources"]:
    source["reviewed_on"] = RESEARCH["reviewed_on"]


EAST = {
    "id": 3, "slug": "east", "name": "Riverside, Riverdale and Leslieville",
    "status": "Authored standalone source asset; scene registration and native build are separate gates.",
    "roads": [
        path("Danforth Avenue", [[24,64],[976,64]]),
        path("Gerrard Street East", [[24,288],[672,288],[672,256],[976,256]], note="Left connection conditional on core east-bank Gerrard correction; named road still exists in this viewport."),
        path("Dundas Street East", [[24,400],[672,400],[672,368],[976,368]]),
        path("Queen Street East", [[24,528],[672,528],[672,496],[976,496]]),
        path("Bain Avenue", [[224,208],[544,208]], note="Only the researched Logan→Carlaw→Pape portion is represented; other residential streets omitted."),
        path("Logan Avenue", [[224,64],[224,720]]),
        path("Carlaw Avenue", [[384,64],[384,720]]),
        path("Pape Avenue north fragment", [[544,64],[544,208]], note="Road terminates north of the rail corridor; compressed street endpoint is not an exact geographic projection."),
        path("Pape Avenue south fragment", [[544,288],[544,688]], note="No car link across railway to north fragment."),
        path("Jones Avenue", [[704,64],[704,496]], note="Ends at Queen; no invented extension to Eastern."),
        path("Leslie Street", [[816,256],[816,952]], note="Continues south through the existing Eastern junction to the Port Lands cutline. No invented Leslie→Danforth road through the railway. Junction facts and separate source provenance in docs/PORT_LANDS_PLAN.md."),
        path("Greenwood Avenue", [[944,64],[944,496]], note="Southern road endpoint at Queen."),
        path("Eastern Avenue", [[80,720],[384,720],[384,688],[704,688],[704,656],[976,656]], note="Viewport endpoints, not real street termini. No west portal or Don-mouth bridge authored."),
    ],
    "footpaths": [
        path("Withrow park walk", [[224,144],[384,144]], note="Original park path, not an official entrance survey."),
        path("Pape pedestrian rail crossing", [[544,208],[544,288]], note="Source-derived pedestrian connection; authored flat pixels do not reproduce physical stairs/bridge construction."),
        path("Greenwood park walk", [[816,312],[944,312]], note="Original short park-and-walk service path."),
        path("Chester station approach", [[176,64],[176,32]], note="Compressed foot approach north of Danforth and west of Logan; no scheduled transit added here."),
    ],
    "ports": [
        {"edge":"west","name":name,"x":24,"y":y,"target":0,"target_x":1000,"target_y":y,"foot_only":False,"registration_status":"Proposed source seam; root must open matching core edge and register reciprocal transition."}
        for name,y in [("Danforth",64),("Dundas",400),("Queen",528)]
    ] + [{"edge":"south","name":"Leslie south","x":816,"y":952,"target":4,"target_x":912,"target_y":24,"foot_only":False,"registration_status":"Authored reciprocal approach for the new Port Lands scene; native build and travel remain separate gates."}],
    "conditional_ports": [{"edge":"west","name":"Gerrard","x":24,"y":288,"target":0,"target_x":1000,"target_y":288,"foot_only":False,"condition":"Correct core College/Carlton label/topology east of Parliament first; closed border and no registered transition in this asset."}],
    "parks": [{"name":"Withrow Park","rect":[256,104,96,72],"source_ids":["east_withrow","east_withrow_context"]},{"name":"Greenwood Park","rect":[848,288,64,48],"source_ids":["east_greenwood"]}],
    "water": [],
    "rails": [path("Lakeshore East railway barrier", [[80,800],[160,688],[176,528],[208,400],[352,304],[400,288],[544,232],[704,176],[1008,128]], note="Original compressed curve from City major-rail features, not a trace. Roads reopen only represented real crossing corridors; Pape remains foot-only.")],
    "closed_frontiers": [{"rect":rect,"name":"Unexpanded southern viewport","source_fact":False,"note":"Game map cutline beside the authored Leslie approach, not a municipal border or real-world closure."} for rect in ([16,816,768,144],[848,816,160,144])],
    "landmarks": [
        {"name":"Danforth Music Hall","x":104,"y":112,"width":80,"depth":48,"style":1,"kind":"music_hall","source_ids":["east_music_hall"]},
        {"name":"Opera House","x":64,"y":448,"width":80,"depth":32,"style":1,"kind":"theatre","source_ids":["east_opera_house"]},
        {"name":"Ashbridge Estate house","x":856,"y":408,"width":48,"depth":40,"style":2,"kind":"heritage_house","source_ids":["east_ashbridge","east_ashbridge_address"]},
        {"name":"Carlaw Works","x":424,"y":576,"width":80,"depth":56,"style":5,"kind":"factory","fictional":True,"source_relation":"Original depot and factory silhouette in the researched industrial corridor; not a specific real business."},
    ],
    "stop_candidates": [
        {"key":"danforth_hall","name":"Danforth Hall","x":144,"y":64,"foot_only":False,"fictional_service_point":True},
        {"key":"withrow_walk","name":"Withrow Parcel","x":320,"y":144,"foot_only":True,"parking_anchor":[224,144],"fictional_service_point":True},
        {"key":"riverside_queen","name":"Riverside Queen","x":128,"y":528,"foot_only":False,"fictional_service_point":True},
        {"key":"gerrard_pape","name":"Gerrard / Pape","x":544,"y":288,"foot_only":False,"fictional_service_point":True},
        {"key":"carlaw_works","name":"Carlaw Works","x":384,"y":608,"foot_only":False,"fictional_service_point":True},
        {"key":"leslie_queen","name":"Leslie / Queen","x":816,"y":496,"foot_only":False,"fictional_service_point":True},
        {"key":"greenwood_walk","name":"Greenwood Parcel","x":880,"y":312,"foot_only":True,"parking_anchor":[816,312],"fictional_service_point":True},
        {"key":"ashbridge_queen","name":"Ashbridge Queen","x":880,"y":496,"foot_only":False,"fictional_service_point":True},
    ],
    "traffic_loops": [
        [[232,72],[376,72],[376,200],[232,200]],
        [[392,72],[536,72],[536,200],[392,200]],
        [[232,296],[376,296],[376,392],[232,392]],
        [[392,296],[536,296],[536,392],[392,392]],
        [[696,368],[696,464],[712,464],[712,368]],
        [[376,560],[376,688],[392,688],[392,560]],
    ],
    "traffic_lane_segments": [
        [
            {"kind":"lane","road_sections":[{"road":"Danforth Avenue","segment":0}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Carlaw Avenue","segment":0}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Bain Avenue","segment":0}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Logan Avenue","segment":0}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Danforth Avenue","segment":0}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Pape Avenue north fragment","segment":0}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Bain Avenue","segment":0}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Carlaw Avenue","segment":0}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Gerrard Street East","segment":0}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Carlaw Avenue","segment":0}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Dundas Street East","segment":0}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Logan Avenue","segment":0}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Gerrard Street East","segment":0}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Pape Avenue south fragment","segment":0}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Dundas Street East","segment":0}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Carlaw Avenue","segment":0}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Jones Avenue","segment":0}],"direction":"S"},
            {"kind":"turnaround","direction":"E","road_sections":[{"road":"Jones Avenue","segment":0}],"reason":"Bounded16px lane turnaround inside existing paved junction/endcap; not through-road travel."},
            {"kind":"lane","road_sections":[{"road":"Jones Avenue","segment":0}],"direction":"N"},
            {"kind":"turnaround","direction":"W","road_sections":[{"road":"Jones Avenue","segment":0}],"reason":"Bounded16px lane turnaround inside existing paved junction/endcap; not through-road travel."},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Carlaw Avenue","segment":0},{"road":"Eastern Avenue","segment":1}],"direction":"S"},
            {"kind":"turnaround","direction":"E","road_sections":[{"road":"Carlaw Avenue","segment":0},{"road":"Eastern Avenue","segment":1}],"reason":"Bounded16px lane turnaround inside existing paved junction/endcap; not through-road travel."},
            {"kind":"lane","road_sections":[{"road":"Carlaw Avenue","segment":0},{"road":"Eastern Avenue","segment":1}],"direction":"N"},
            {"kind":"turnaround","direction":"W","road_sections":[{"road":"Carlaw Avenue","segment":0},{"road":"Eastern Avenue","segment":1}],"reason":"Bounded16px lane turnaround inside existing paved junction/endcap; not through-road travel."},
        ],
    ],
    "traffic_rule": "Right-hand8px lanes; annotations describe point i to point(i+1) modulo count. Named-road joins allow at most8px endpoint extension; turnarounds permit only explicit16px opposing-lane endcaps.",
    "traffic_identity": "Six original circulation loops, not literal TTC or live traffic routes.",
}


def extended_points(route):
    """Only registered, reciprocal source approaches continue to the border."""
    points = [p[:] for p in route["points"]]
    for port in EAST["ports"]:
        border={"west":[0,port["y"]],"east":[WIDTH,port["y"]],
                "north":[port["x"],0],"south":[port["x"],HEIGHT]}[port["edge"]]
        if points[0] == [port["x"],port["y"]]:
            points.insert(0,border)
        elif points[-1] == [port["x"],port["y"]]:
            points.append(border)
    return points
