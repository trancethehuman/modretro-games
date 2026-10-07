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
    # Sorauren Park: its sports field, and the fieldhouse on the south edge.
    "parks": [{"name":"Sorauren Park", "rect":[576,384,128,96],
               "features":[{"kind":"pitch","rect":[608,392,64,36]}],
               "fieldhouse":[680,408,24,16]}],
    "water": [[0,928,1024,48]],
    "rails": [path("West Toronto rail corridor", [[512,24],[576,112],[560,320],[960,480],[1024,488]]), path("Lakeshore rail corridor", [[0,752],[1024,752]])],
    "gardiner": [0,784,1024,16],
    "landmarks": [{"name":"Roncesvalles Carhouse", "x":432,"y":680,"width":112,"depth":40,"style":5,"kind":"carhouse"}],
    "spray_bay": {"street":"The Queensway", "x":248,"y":618,"door_bottom":600, "note":"Fictional body shop; gameplay position, not a surveyed business."},
    "stop_candidates": [
        {"name":"Dufferin-side College", "x":976,"y":288,"foot_only":False,"fictional_service_point":True,"source_relation":"College west of retained core Dufferin; not a relocated Dufferin Grove park."},
        {"name":"Lansdowne / Bloor", "x":800,"y":64,"foot_only":False,"fictional_service_point":True},
        {"name":"Parkdale / Queen", "x":864,"y":528,"foot_only":False,"fictional_service_point":True},
        {"name":"Roncesvalles / Howard", "x":416,"y":352,"foot_only":False,"fictional_service_point":True},
        {"name":"Sorauren Works", "x":544,"y":432,"foot_only":False,"fictional_service_point":True},
    ],
    "traffic_loops": [
        [[800,64],[912,64],[912,288],[800,288]],
        [[800,288],[912,288],[912,400],[800,400]],
        [[800,400],[912,400],[912,528],[800,528]],
        [[544,320],[640,320],[640,352],[800,352],[800,576],[704,576],[704,464],[544,464]],
        [[416,240],[480,240],[480,320],[416,320]],
        [[640,608],[512,608],[512,464],[416,464],[416,320],[544,320],[544,464],[704,464],[704,576],[640,576]],
    ],
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
    "parks": [{"name":"High Park", "rect":[608,312,352,320],
               # The zoo's paddocks on Deer Pen Road and the open lawns of the
               # park's middle; Grenadier Pond runs most of the west side.
               "features":[{"kind":"paddock","rect":[752,496,40,32]},{"kind":"paddock","rect":[800,496,40,32]},
                           {"kind":"meadow","rect":[752,424,128,48]}]}],
    "water": [[0,960,1024,16]],
    "pond": [[608,392],[632,392],[652,432],[660,472],[684,512],[704,592],[680,624],[632,624],[608,584]],
    "rails": [path("Junction railway barrier", [[432,32],[1024,32]]),path("Lakeshore rail corridor", [[0,752],[1024,752]])],
    "gardiner": [0,784,1024,16],
    "spray_bay": {"street":"Bloor Street West", "x":344,"y":330,"door_bottom":312, "note":"Fictional body shop in Bloor West Village; gameplay position, not a surveyed business."},
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
        [[848,96],[960,96],[960,192],[944,192],[944,176],[848,176]],
        [[432,112],[848,112],[848,176],[944,176],[944,192],[432,192]],
        [[432,224],[176,224],[176,352],[432,352]],
        [[432,304],[432,496],[576,496],[576,688],[384,688],[384,736],[384,512],[256,512],[256,352],[432,352]],
        [[960,96],[848,96],[848,176],[944,176],[944,192],[960,192]],
        [[432,352],[176,352],[176,224],[432,224]],
    ],
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
