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
ISLANDS = [[336,912,600,952],[640,896,784,952],[800,880,928,928]]
ROAD_HALF, WALK_HALF = 24, 32

def interpolate(value, old, new):
    for i in range(len(old)-1):
        if value <= old[i+1]:
            return round(new[i]+(value-old[i])*(new[i+1]-new[i])/(old[i+1]-old[i]))
    return new[-1]+(value-old[-1])*2

def location(u,v):
    return interpolate(u,OLD_COLS,COLS),interpolate(v,OLD_ROWS,NEW_ROWS)

def road(u,v,half=ROAD_HALF):
    if not(24<=u<=992 and 24<=v<=808):return False
    if RIVER[0]<=u<=RIVER[1] and not any(abs(v-r)<half for r in BRIDGES):return False
    return any(abs(v-r)<half and not(r in (640,720) and u>848) for r in ROWS) or any(abs(u-c)<half for c in COLS)

def walkable(u,v):
    return road(u,v,WALK_HALF) or any(x<=u<=right and y<=v<=bottom for x,y,right,bottom in ISLANDS)
