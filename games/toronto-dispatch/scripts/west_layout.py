"""Original compressed western road graphs, not projected City GIS geometry.

Facts, references and reuse terms live in content/districts/west-research.json.
All coordinates below are proposed native world pixels; north is up.
"""
WIDTH, HEIGHT = 1024, 976
ROAD_HALF, WALK_HALF = 24, 32
CORE_ROWS = [64, 288, 400, 528, 640]
WEST_ROWS = [96, 240, 352, 640, 832]


def path(name, points, **extra):
    return {"name": name, "points": points, **extra}


WEST = {
    "id": 1, "slug": "west", "name": "Parkdale and Roncesvalles",
    "roads": [
        path("Bloor Street West", [[1000,64],[816,64],[816,112],[512,112],[512,160],[384,160],[384,240],[24,240]]),
        path("Dundas Street West", [[1000,400],[800,400],[800,352],[640,352],[640,320],[480,320],[480,240],[416,240],[416,160],[384,160],[384,96],[24,96]]),
        path("College Street", [[1000,288],[800,288],[800,320],[640,320]], note="Ends at Dundas; no extension through High Park."),
        path("Roncesvalles Avenue", [[416,240],[416,464],[512,464],[512,608],[640,608]]),
        path("Howard Park Avenue", [[480,320],[416,320],[416,352],[24,352]]),
        path("Queen Street West", [[1000,528],[800,528],[800,576],[640,576],[640,608]]),
        path("King Street West", [[1000,640],[880,640],[880,688],[704,688],[704,608],[640,608]]),
        path("The Queensway", [[640,608],[512,608],[512,640],[24,640]]),
        path("Lansdowne Avenue", [[800,64],[800,528]], note="Ends at Queen; underpass south of Dundas."),
        path("Brock Avenue", [[912,64],[912,528]]),
        path("Sorauren Avenue", [[544,320],[544,464],[704,464],[704,576]]),
        path("Lake Shore Boulevard West", [[24,832],[1000,832]], note="East end closed until a researched waterfront link is registered."),
    ],
    "footpaths": [
        path("Martin Goodman waterfront path", [[24,896],[1000,896]]),
        path("Beaty pedestrian bridge", [[640,608],[608,608],[608,896]], note="Foot-only crossing of rail/Gardiner corridor."),
        path("Sorauren park path", [[544,464],[600,464],[600,424]]),
    ],
    "ports": [
        *[{"edge":"east", "name":name,"x":1000,"y":y,"target":0,"target_x":24,"target_y":y,"foot_only":False} for name,y in zip(["Bloor","College","Dundas","Queen","King"],CORE_ROWS)],
        *[{"edge":"west", "name":name,"x":24,"y":y,"target":2,"target_x":1000,"target_y":y,"foot_only":False} for name,y in zip(["Dundas","Bloor","Howard Park","The Queensway","Lake Shore"],WEST_ROWS)],
        {"edge":"west","name":"Martin Goodman","x":24,"y":896,"target":2,"target_x":1000,"target_y":896,"foot_only":True},
    ],
    "parks": [{"name":"Sorauren Park", "rect":[576,384,128,96]}],
    "water": [[0,928,1024,48]],
    "rails": [path("West Toronto rail corridor", [[512,24],[576,112],[560,320],[960,480],[1024,488]]), path("Lakeshore rail corridor", [[0,752],[1024,752]])],
    "gardiner": [0,784,1024,16],
    "landmarks": [{"name":"Roncesvalles Carhouse", "x":432,"y":680,"width":112,"depth":40,"style":5,"kind":"carhouse"}],
    "stop_candidates": [
        {"name":"Dufferin-side College", "x":976,"y":288,"foot_only":False,"fictional_service_point":True,"source_relation":"College west of retained core Dufferin; not a relocated Dufferin Grove park."},
        {"name":"Lansdowne / Bloor", "x":800,"y":64,"foot_only":False,"fictional_service_point":True},
        {"name":"Parkdale / Queen", "x":864,"y":528,"foot_only":False,"fictional_service_point":True},
        {"name":"Roncesvalles / Howard", "x":416,"y":352,"foot_only":False,"fictional_service_point":True},
        {"name":"Sorauren Works", "x":544,"y":432,"foot_only":False,"fictional_service_point":True},
    ],
    "traffic_loops": [
        [[808,72],[904,72],[904,280],[808,280]],
        [[808,296],[904,296],[904,392],[808,392]],
        [[904,400],[904,488],[920,488],[920,400]],
        [[552,328],[632,328],[632,360],[792,360],[792,568],[712,568],[712,456],[552,456]],
        [[424,248],[472,248],[472,312],[424,312]],
        [[632,600],[520,600],[520,456],[424,456],[424,328],[536,328],[536,472],[696,472],[696,568],[632,568]],
    ],
    "traffic_lane_segments": [
        [
            {"kind":"lane","road_sections":[{"road":"Bloor Street West","segment":0}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Brock Avenue","segment":0}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"College Street","segment":0}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Lansdowne Avenue","segment":0},{"road":"College Street","segment":1}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"College Street","segment":0}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Brock Avenue","segment":0}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":0}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Lansdowne Avenue","segment":0},{"road":"College Street","segment":1},{"road":"Dundas Street West","segment":1}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Brock Avenue","segment":0}],"direction":"S"},
            {"kind":"turnaround","direction":"E","road_sections":[{"road":"Brock Avenue","segment":0}],"reason":"Bounded16px lane turnaround inside existing paved junction/endcap; not through-road travel."},
            {"kind":"lane","road_sections":[{"road":"Brock Avenue","segment":0}],"direction":"N"},
            {"kind":"turnaround","direction":"W","road_sections":[{"road":"Brock Avenue","segment":0}],"reason":"Bounded16px lane turnaround inside existing paved junction/endcap; not through-road travel."},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":4},{"road":"College Street","segment":2}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":3}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":2}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Lansdowne Avenue","segment":0},{"road":"Dundas Street West","segment":1},{"road":"Queen Street West","segment":1}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Queen Street West","segment":2}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Sorauren Avenue","segment":2}],"direction":"N"},
            {"kind":"lane","road_sections":[{"road":"Sorauren Avenue","segment":1}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Sorauren Avenue","segment":0}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":6}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":5}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Howard Park Avenue","segment":0},{"road":"Dundas Street West","segment":4}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":7},{"road":"Roncesvalles Avenue","segment":0},{"road":"Howard Park Avenue","segment":1}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Roncesvalles Avenue","segment":3},{"road":"The Queensway","segment":0},{"road":"King Street West","segment":4}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Roncesvalles Avenue","segment":2},{"road":"The Queensway","segment":1}],"direction":"N"},
            {"kind":"lane","road_sections":[{"road":"Roncesvalles Avenue","segment":1}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Roncesvalles Avenue","segment":0},{"road":"Howard Park Avenue","segment":1}],"direction":"N"},
            {"kind":"lane","road_sections":[{"road":"Howard Park Avenue","segment":0},{"road":"Dundas Street West","segment":4}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Sorauren Avenue","segment":0}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Sorauren Avenue","segment":1}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Sorauren Avenue","segment":2}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Queen Street West","segment":2}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Queen Street West","segment":3}],"direction":"S"},
        ],
    ],
    "traffic_rule": "Right-hand8px lanes; annotations describe point i to point(i+1) modulo count. Named-road joins allow at most8px endpoint extension; turnarounds permit only explicit16px opposing-lane endcaps.",
}

HIGH_PARK = {
    "id": 2, "slug": "high_park", "name": "High Park, Swansea and Junction",
    "roads": [
        path("Bloor Street West", [[1000,240],[944,240],[944,272],[656,272],[656,304],[432,304],[432,352],[96,352]]),
        path("Dundas Street West", [[1000,96],[848,96],[848,112],[432,112]], note="Western endpoint is Runnymede, before outside-boundary Dundas/Jane."),
        path("Keele Street", [[944,240],[944,176],[848,176],[848,96]]),
        path("Parkside Drive", [[944,240],[944,640]]),
        path("Howard Park Avenue", [[1000,352],[976,352],[976,384],[944,384]], note="Ordinary road terminates at Parkside. TTC loop inside park is not a through car road."),
        path("Annette Street", [[960,96],[960,192],[432,192],[432,224],[176,224],[176,256]]),
        path("Runnymede Road", [[432,112],[432,496]], note="Southern road endpoint at Morningside; not a straight link to Queensway."),
        path("Jane Street", [[176,256],[176,352]]),
        path("Morningside Avenue", [[432,496],[576,496]]),
        path("Ellis Avenue", [[576,496],[576,688]]),
        path("The Queensway", [[1000,640],[576,640],[576,688],[384,688],[384,736],[160,736],[160,784],[64,784]]),
        path("South Kingsway", [[144,352],[256,352],[256,512],[384,512],[384,736]]),
        path("Colborne Lodge Drive south approach", [[736,640],[736,832]], note="Represents the lower collector connection; northern park access is foot-only in this game."),
        path("Lake Shore Boulevard West", [[1000,832],[384,832],[384,848],[64,848]]),
    ],
    "footpaths": [
        path("Martin Goodman waterfront path", [[1000,896],[64,896]]),
        path("High Park formal spine", [[656,304],[672,304],[672,416],[736,416],[736,640]]),
        path("High Park Boulevard park walk", [[944,480],[736,480]]),
        path("Spring Road park walk", [[944,560],[864,560],[864,624],[736,624]]),
        path("High Park Loop platform", [[944,384],[896,384],[896,408],[920,408],[920,384],[944,384]]),
        path("Colborne fictional service entrance", [[736,608],[784,608]]),
        path("Sunnyside pavilion approach", [[752,832],[752,896]]),
    ],
    "ports": [
        *[{"edge":"east","name":name,"x":1000,"y":y,"target":1,"target_x":24,"target_y":y,"foot_only":False} for name,y in zip(["Dundas","Bloor","Howard Park","The Queensway","Lake Shore"],WEST_ROWS)],
        {"edge":"east","name":"Martin Goodman","x":1000,"y":896,"target":1,"target_x":24,"target_y":896,"foot_only":True},
    ],
    "parks": [{"name":"High Park", "rect":[608,312,352,320]}],
    "water": [[0,960,1024,16]],
    "pond": [[608,480],[672,480],[704,600],[640,624],[608,576]],
    "rails": [path("Junction railway barrier", [[432,32],[1024,32]]),path("Lakeshore rail corridor", [[0,752],[1024,752]])],
    "gardiner": [0,784,1024,16],
    "landmarks": [
        {"name":"Colborne Lodge", "x":792,"y":544,"width":48,"depth":40,"style":2,"kind":"regency"},
        {"name":"Sunnyside Bathing Pavilion", "x":784,"y":912,"width":128,"depth":24,"style":3,"kind":"pavilion"},
        {"name":"Junction heritage brick block", "x":672,"y":48,"width":112,"depth":24,"style":1,"kind":"junction"},
    ],
    "stop_candidates": [
        {"name":"High Park Gate", "x":656,"y":272,"foot_only":False,"fictional_service_point":True,"source_relation":"Roadside Bloor service point, not a visitor car entrance into High Park."},
        {"name":"Parkside Depot", "x":944,"y":608,"foot_only":False,"fictional_service_point":True},
        {"name":"Colborne Service", "x":784,"y":608,"foot_only":True,"fictional_service_point":True,"parking_anchor":[736,640],"source_relation":"Fictional park walk from south Queensway approach; not a claim of real all-day access policy."},
    ],
    "traffic_loops": [
        [[856,104],[952,104],[952,168],[856,168]],
        [[440,120],[840,120],[840,184],[440,184]],
        [[440,216],[168,216],[168,360],[440,360]],
        [[440,304],[424,304],[424,504],[568,504],[568,680],[376,680],[376,736],[392,736],[392,504],[264,504],[264,360],[440,360]],
        [[968,88],[840,88],[840,184],[936,184],[936,200],[968,200]],
        [[424,344],[184,344],[184,232],[424,232]],
    ],
    "traffic_lane_segments": [
        [
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":0}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Annette Street","segment":0}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Keele Street","segment":1}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":1},{"road":"Keele Street","segment":2}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":2}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":1},{"road":"Keele Street","segment":2}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Annette Street","segment":1}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Runnymede Road","segment":0},{"road":"Annette Street","segment":2}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Annette Street","segment":3}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Annette Street","segment":4},{"road":"Jane Street","segment":0}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Bloor Street West","segment":6},{"road":"South Kingsway","segment":0}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Runnymede Road","segment":0},{"road":"Annette Street","segment":2},{"road":"Bloor Street West","segment":5}],"direction":"N"},
        ],
        [
            {"kind":"turnaround","direction":"W","road_sections":[{"road":"Runnymede Road","segment":0},{"road":"Bloor Street West","segment":5}],"reason":"Bounded16px lane turnaround inside existing paved junction/endcap; not through-road travel."},
            {"kind":"lane","road_sections":[{"road":"Runnymede Road","segment":0},{"road":"Bloor Street West","segment":5}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Morningside Avenue","segment":0}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Ellis Avenue","segment":0},{"road":"The Queensway","segment":1}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"The Queensway","segment":2}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"South Kingsway","segment":3},{"road":"The Queensway","segment":3}],"direction":"S"},
            {"kind":"turnaround","direction":"E","road_sections":[{"road":"South Kingsway","segment":3},{"road":"The Queensway","segment":3}],"reason":"Bounded16px lane turnaround inside existing paved junction/endcap; not through-road travel."},
            {"kind":"lane","road_sections":[{"road":"South Kingsway","segment":3},{"road":"The Queensway","segment":3}],"direction":"N"},
            {"kind":"lane","road_sections":[{"road":"South Kingsway","segment":2}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"South Kingsway","segment":1}],"direction":"N"},
            {"kind":"lane","road_sections":[{"road":"Bloor Street West","segment":6},{"road":"South Kingsway","segment":0}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Runnymede Road","segment":0},{"road":"Bloor Street West","segment":5}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":0}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Dundas Street West","segment":1},{"road":"Keele Street","segment":2}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Keele Street","segment":1}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Keele Street","segment":0}],"direction":"S"},
            {"kind":"lane","road_sections":[{"road":"Annette Street","segment":1}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Annette Street","segment":0}],"direction":"N"},
        ],
        [
            {"kind":"lane","road_sections":[{"road":"Bloor Street West","segment":6},{"road":"South Kingsway","segment":0}],"direction":"W"},
            {"kind":"lane","road_sections":[{"road":"Annette Street","segment":4},{"road":"Jane Street","segment":0}],"direction":"N"},
            {"kind":"lane","road_sections":[{"road":"Annette Street","segment":3}],"direction":"E"},
            {"kind":"lane","road_sections":[{"road":"Runnymede Road","segment":0},{"road":"Annette Street","segment":2},{"road":"Bloor Street West","segment":5}],"direction":"S"},
        ],
    ],
    "traffic_rule": "Right-hand8px lanes; annotations describe point i to point(i+1) modulo count. Named-road joins allow at most8px endpoint extension; turnarounds permit only explicit16px opposing-lane endcaps.",
}
DISTRICTS = [WEST, HIGH_PARK]


def extended_points(route, district, foot=False):
    """Extend only registered seams through the image border (safe inset arrivals)."""
    points = [p[:] for p in route["points"]]
    for port in district["ports"]:
        if bool(port["foot_only"]) != foot:
            continue
        end = [port["x"], port["y"]]
        outside = [0 if port["edge"] == "west" else WIDTH, port["y"]]
        if points[0] == end:
            points.insert(0, outside)
        elif points[-1] == end:
            points.append(outside)
    return points
