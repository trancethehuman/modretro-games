"""Original Port Lands compression, not projected or traced official geometry.

Research/provenance: docs/PORT_LANDS_PLAN.md and content/sources.json.
Only the Leslie north approach is proposed for the first native integration.
Importing this module never writes assets or registers a scene.
"""
WIDTH, HEIGHT = 1024, 976
ROAD_HALF, WALK_HALF = 24, 32


def path(name, points, **extra):
    return {"name": name, "points": points, **extra}


RESEARCH = {
    "reviewed_on": "2026-10-02",
    "map_era": "Representative contemporary corridors are a working assumption; no user-adopted 2026 era, live detours, fares or schedules.",
    "boundary": "AREA_NAME=TORONTO, OBJECTID14035041. All 19 selected named source junctions were inside the actual polygon; this compression is not a projected whole-scene polygon clip.",
    "licence": {
        "name": "Open Government Licence - Toronto", "version": "1.0",
        "url": "https://www.toronto.ca/city-government/data-research-maps/open-data/open-data-licence/",
        "attribution": "Contains information licensed under the Open Government Licence – Toronto.",
        "included": "Derived named-road/bridge relationships; no official imagery, logos, geometry, photographs or public-art designs.",
        "rights": "Preserve source attribution/licence; third-party rights and official symbols excluded; no endorsement.",
    },
    "live_query_evidence": {
        "centreline_endpoint": "https://gis.toronto.ca/arcgis/rest/services/cot_geospatial/MapServer/2/query",
        "centreline_envelope_wgs84": [-79.374, 43.626, -79.318, 43.661],
        "centreline_feature_count": 123, "shared_named_junctions": 19,
        "selected_junctions_inside_former_toronto": 19,
        "centreline_response_sha256": "1e62af648fecc5517297d0472d5e61b7d0ee8579e692d5bdf685cd7130444444",
        "boundary_endpoint": "https://gis.toronto.ca/arcgis/rest/services/cot_geospatial27/MapServer/6/query",
        "boundary_response_sha256": "1db98bac67409e1a658469ebf5cc981f69233f4df049777010e3d58fa74ebaf5",
        "method": "Read-only in-memory WGS84 query, named-road FROM/TO joins and point-in-polygon; full query/selected records retained in PORT_LANDS_PLAN.md.",
    },
    "source_registry_ids": [
        "port-lands-city-context-20261002", "port-lands-bridge-backgrounder-20261002",
        "port-lands-completed-infrastructure-20261002", "port-lands-ookwemin-island-20261002",
        "port-lands-biidaasige-project-20261002", "port-lands-biidaasige-west-opening-20261002",
        "port-lands-fire-hall-30-20261002", "port-lands-unwin-bridge-study-20261002",
        "port-lands-hearn-heritage-20261002", "port-lands-centreline-audit-20261002",
        "port-lands-former-toronto-audit-20261002", "ttc-port-lands-visitor-connections-20261002",
        "ttc-waterfront-east-future-20261002",
    ],
    "limitations": [
        "No imported whole-city clipping dataset or native scene proof.",
        "Unwin width is widened to the common 48px game road for initial handling; the source's 2025 single-lane bridge/replacement status is not asserted as a 2026 completed replacement.",
        "Only existing supported road crossings are drawn; no added park/Equinox footbridge or operating Waterfront East streetcar.",
        "Client entrances, building footprints, paths, traffic circulation and distances are original game design.",
    ],
}


PORT_LANDS = {
    "id": 4, "slug": "port_lands", "name": "Port Lands and Don mouth",
    "status": "Original source specification; registration, compiled limits and native play remain separate gates.",
    "roads": [
        path("Lake Shore Boulevard East", [[24,128],[672,128],[672,64],[976,64]]),
        path("Cherry Street", [[192,24],[192,832]]),
        path("Commissioners Street", [[192,352],[336,352],[336,288],[592,288],[592,224],[912,224]]),
        path("Ookwemin Street", [[272,256],[272,352]], note="Older island Cherry alignment; no new northern channel bridge."),
        path("Villiers west bank fragment", [[272,256],[400,256]]),
        path("Villiers east bank fragment", [[512,192],[544,192]], note="No bridge joins the fragments."),
        path("Don Roadway", [[544,128],[544,288]], note="No invented southern extension over Ship Channel."),
        path("Carlaw Avenue southern approach", [[736,24],[736,224]]),
        path("Leslie Street", [[912,24],[912,768]]),
        path("Polson Street", [[128,576],[192,576]]),
        path("Unwin Avenue", [[192,824],[864,824],[864,768],[912,768]], note="Two cardinal bends east of the circulation channel; widened original game lane, no completion claim for proposed replacement bridge."),
        path("Regatta Road", [[352,824],[352,864]], note="Stops before the beach promenade."),
        path("Fictional Turning Basin service drive", [[672,224],[672,576],[704,576]], note="Original private courier-yard access, not a surveyed or named public street; stops north of Ship Channel."),
    ],
    "footpaths": [
        path("Fire Hall community walk", [[304,352],[328,352],[328,432],[312,432]], note="Original handoff approach around the footprint, not surveyed door access."),
        path("West park promenade walk", [[192,352],[144,352],[144,432]]),
        path("South riverbank walk", [[192,552],[352,552],[352,592]]),
        path("Cherry Beach last mile", [[352,864],[352,888],[432,888]], note="Foot-only sand-edge path, no beach road."),
    ],
    "aprons": [
        {"name":"Unwin Works loading apron", "rect":[592,792,80,40], "fictional":True},
        {"name":"Channel Stage loading apron", "rect":[640,256,64,32], "fictional":True},
    ],
    "ports": [{"edge":"north", "name":"Leslie north", "x":912, "y":24,
               "target":3, "target_x":816, "target_y":952, "foot_only":False,
               "registration_status":"Proposed first seam only; matching East extension must be authored/validated and runtime explicitly registered."}],
    "conditional_ports": [
        {"edge":"north", "name":"Carlaw north", "x":736, "y":24, "target":3,
         "target_x":384, "target_y":952, "foot_only":False,
         "condition":"Withheld pending authored/validated East Carlaw southern approach."},
        {"edge":"north", "name":"Cherry mainland", "x":192, "y":24, "target":0,
         "target_x":None, "target_y":None, "foot_only":False,
         "condition":"Withheld: research and correct actual Core Cherry/Lake Shore/Don-mouth approach first; never portal over harbour or Island art."},
    ],
    "water": [
        {"name":"Inner Harbour", "rect":[0,168,96,760]},
        {"name":"Keating Channel", "rect":[96,168,392,48]},
        {"name":"Don River eastern arm", "rect":[432,0,64,528]},
        {"name":"New Don river mouth", "rect":[96,464,400,72]},
        {"name":"Ship Channel", "rect":[96,640,704,64]},
        {"name":"Turning Basin", "rect":[752,608,112,120]},
        {"name":"Hearn circulation channel", "rect":[792,704,32,224]},
        {"name":"Lake Ontario", "rect":[0,936,1024,40]},
    ],
    "wetlands": [{"name":"Don Greenway wetland/spillway", "rect":[432,536,64,104], "permanent_open_water":False}],
    "bridges": [
        {"name":"Cherry North road bridge", "points":[[192,152],[192,232]], "water":"Keating Channel", "color_reference":"red/white", "palette":1},
        {"name":"Cherry South bridge", "points":[[192,448],[192,552]], "water":"New Don river mouth", "color_reference":"yellow/white", "palette":3},
        {"name":"Commissioners bridge", "points":[[416,288],[512,288]], "water":"Don River eastern arm", "color_reference":"orange/white", "palette":1},
        {"name":"Lake Shore Don bridge", "points":[[416,128],[512,128]], "water":"Don River eastern arm", "palette":0},
        {"name":"Cherry Ship Channel bascule bridge", "points":[[192,624],[192,720]], "water":"Ship Channel", "palette":0},
        {"name":"Unwin circulation-channel crossing", "points":[[776,824],[840,824]], "water":"Hearn circulation channel", "palette":0,
         "note":"Game deck widened for readable handling; bridge replacement status unresolved, no construction/current-service claim."},
    ],
    "parks": [
        {"name":"Biidaasige west", "rect":[104,304,56,144]},
        {"name":"Biidaasige island river edge", "rect":[280,376,128,80]},
        {"name":"Biidaasige south riverbank", "rect":[224,544,176,72]},
        {"name":"Cherry Beach promenade", "rect":[112,872,592,56]},
    ],
    "industrial_zones": [[512,160,352,448],[104,544,312,80],[224,720,544,72]],
    "landmarks": [
        {"name":"Old Fire Hall 30", "x":288,"y":392,"width":32,"depth":32,"style":2,"kind":"fire_hall","source_ids":["port-lands-fire-hall-30-20261002"]},
        {"name":"Channel Stage", "x":720,"y":272,"width":64,"depth":64,"style":5,"kind":"soundstage","fictional":True},
        {"name":"Second stage hall", "x":800,"y":304,"width":64,"depth":72,"style":5,"kind":"sawtooth","fictional":True},
        {"name":"Hearn Generating Station silhouette", "x":560,"y":720,"width":144,"depth":64,"style":3,"kind":"hearn","source_ids":["port-lands-hearn-heritage-20261002"]},
        {"name":"Atlas Crane fenced base", "x":112,"y":424,"width":16,"depth":16,"style":5,"kind":"crane","source_ids":["port-lands-biidaasige-west-opening-20261002"]},
    ],
    "stop_candidates": [
        {"key":"channel_stage","name":"Channel Stage","x":672,"y":272,"foot_only":False,"fictional_service_point":True},
        {"key":"fire_hall_books","name":"Fire Hall Books","x":312,"y":432,"foot_only":True,"parking_anchor":[304,352],"fictional_service_point":True},
        {"key":"crane_walk","name":"Crane Walk","x":144,"y":432,"foot_only":True,"parking_anchor":[192,352],"fictional_service_point":True},
        {"key":"turning_basin","name":"Turning Basin Cases","x":704,"y":576,"foot_only":False,"fictional_service_point":True},
        {"key":"unwin_works","name":"Unwin Works","x":616,"y":808,"foot_only":False,"fictional_service_point":True},
        {"key":"beach_mail","name":"Beach Mail","x":432,"y":888,"foot_only":True,"parking_anchor":[352,856],"fictional_service_point":True},
        {"key":"polson_quay","name":"Polson Packet","x":128,"y":576,"foot_only":False,"fictional_service_point":True},
        {"key":"south_park","name":"Riverbank Parcel","x":352,"y":592,"foot_only":True,"parking_anchor":[192,552],"fictional_service_point":True},
    ],
    "traffic_loops": [
        [[184,248],[200,248],[200,320],[184,320]],
        [[376,280],[408,280],[408,296],[376,296]],
        [[536,160],[552,160],[552,248],[536,248]],
        [[904,272],[920,272],[920,408],[904,408]],
        [[280,816],[440,816],[440,832],[280,832]],
        [[752,216],[832,216],[832,232],[752,232]],
    ],
    "traffic_identity":"Six original lane circulation loops; no literal TTC/live road traffic route.",
}


def extended_points(route):
    """Only the proposed Leslie seam is open to the image's north edge."""
    points = [p[:] for p in route["points"]]
    for port in PORT_LANDS["ports"]:
        if points[0] == [port["x"],port["y"]]:
            points.insert(0,[port["x"],0])
    return points
