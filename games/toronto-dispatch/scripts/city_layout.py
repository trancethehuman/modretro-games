"""Compressed original world layout; units are native world pixels, north is up."""
WIDTH, HEIGHT = 1024, 976
OLD_COLS = [64,128,192,256,288,320,352,416,480]
COLS = [80,208,336,480,560,640,720,816,944]
OLD_ROWS = [64,112,160,224,288,336,368,400,416,480,496,504,512,520,544]
ROWS = [64,176,288,400,528,640,720,784]
NEW_ROWS = ROWS + [816,880,896,912,920,928,952]
BRIDGES = [64,288,400,528,784]
RIVER = [872,912]
MAINLAND = [24,24,992,816]
# The former miniature Island strips are captured in island_legacy_v8.json.
# Public Islands ground now belongs to its separate ferry-only scene.
ISLANDS = []
ROAD_HALF, WALK_HALF = 24, 32
WEST_PORTS = [64,288,400,528,640]
EAST_PORTS = [64,400,528]  # Bloor/Danforth, Dundas and Queen; no King/Front bridge.

def interpolate(value, old, new):
    for i in range(len(old)-1):
        if value <= old[i+1]:
            return round(new[i]+(value-old[i])*(new[i+1]-new[i])/(old[i+1]-old[i]))
    return new[-1]+(value-old[-1])*2

def location(u,v):
    return interpolate(u,OLD_COLS,COLS),interpolate(v,OLD_ROWS,NEW_ROWS)

def road(u,v,half=ROAD_HALF):
    if 0<=u<24 and any(abs(v-r)<half for r in WEST_PORTS):return True
    if 992<u<1024 and any(abs(v-r)<half for r in EAST_PORTS):return True
    if not(24<=u<=992 and 24<=v<=808):return False
    if RIVER[0]<=u<=RIVER[1] and not any(abs(v-r)<half for r in BRIDGES):return False
    return any(abs(v-r)<half and not(r in (640,720) and u>848) for r in ROWS) or any(abs(u-c)<half for c in COLS)

def walkable(u,v):
    return road(u,v,WALK_HALF) or any(x<=u<=right and y<=v<=bottom for x,y,right,bottom in ISLANDS)
