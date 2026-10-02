"""Original, tile-aligned top-down pixel art. Requires Pillow; no downloaded art."""
from pathlib import Path
import json, math, uuid, hashlib, sys
from PIL import Image, ImageDraw
from city_layout import *
ROOT=Path(__file__).resolve().parents[1]; PROJECT=ROOT/'project'
COLORS=['#071821','#306850','#86c06c','#e0f8cf']; TRANSPARENT='#65ff00'
def ident(name):return str(uuid.uuid5(uuid.NAMESPACE_URL,'toronto-dispatch/topdown/'+name))
def main(background_only=False):
    img=Image.new('RGB',(WIDTH,HEIGHT),COLORS[2]);d=ImageDraw.Draw(img)
    attrs=[0]*(WIDTH//8)*(HEIGHT//8);blocks=[];canopies=[]
    def attr(x,y,w,h,slot,priority=False,avoid_road=False):
        for ty in range(max(0,y//8),min(HEIGHT//8,(y+h+7)//8)):
            for tx in range(max(0,x//8),min(WIDTH//8,(x+w+7)//8)):
                if avoid_road and road(tx*8+4,ty*8+4):continue
                attrs[ty*(WIDTH//8)+tx]=slot+(128 if priority else 0)
    def box(x,y,w,h,fill):d.rectangle((x,y,x+w-1,y+h-1),fill=COLORS[fill])
    box(24,24,968,792,3)
    for x,y,r,b in ISLANDS:box(x,y,r-x,b-y,3);box(x,y+16,r-x,8,1)
    # Width 48 asphalt plus 8-pixel sidewalks on both sides. Tile-aligned boundaries.
    for v in ROWS:
        end=848 if v in (640,720) else 992
        box(24,v-WALK_HALF,end-24,WALK_HALF*2,3);box(24,v-ROAD_HALF,end-24,ROAD_HALF*2,1)
        d.line((24,v-ROAD_HALF-4,end-1,v-ROAD_HALF-4),fill=COLORS[0]);d.line((24,v+ROAD_HALF+4,end-1,v+ROAD_HALF+4),fill=COLORS[0])
        for u in range(32,end,32):box(u,v,8,1,3)
    for u in COLS:
        box(u-WALK_HALF,24,WALK_HALF*2,792,3);box(u-ROAD_HALF,24,ROAD_HALF*2,792,1)
        d.line((u-ROAD_HALF-4,24,u-ROAD_HALF-4,815),fill=COLORS[0]);d.line((u+ROAD_HALF+4,24,u+ROAD_HALF+4,815),fill=COLORS[0])
        for v in range(32,816,32):box(u,v,1,8,3)
    box(872,24,40,792,2)
    for v in BRIDGES:
        box(864,v-WALK_HALF,56,WALK_HALF*2,3);box(864,v-ROAD_HALF,56,ROAD_HALF*2,1)
        d.line((864,v-ROAD_HALF-1,919,v-ROAD_HALF-1),fill=COLORS[0]);d.line((864,v+ROAD_HALF,919,v+ROAD_HALF),fill=COLORS[0])
    # Crosswalks and corner signal bollards. No text baked into the map.
    for u in COLS:
        for v in ROWS:
            if u>848 and v in (640,720):continue
            for off in range(-ROAD_HALF+4,ROAD_HALF,8):
                box(u+off,v-ROAD_HALF-4,4,4,3);box(u+off,v+ROAD_HALF,4,4,3)
            box(u-WALK_HALF+4,v-WALK_HALF+4,3,3,0);box(u+WALK_HALF-6,v+WALK_HALF-6,3,3,0)
    # Train corridor west of Union: original double rails and sleepers, not drivable.
    box(32,752,528,8,0)
    for x in range(32,560,8):box(x,754,2,4,2)
    d.line((32,753,560,753),fill=COLORS[3]);d.line((32,758,560,758),fill=COLORS[3])
    # Modular districts: varying wide/narrow footprints and architectural roof details.
    def building(x,y,w,h,style,tall=False,label=None):
        if h<32:tall=False
        if h<24:style=0
        # Preserve the existing footprints while clipping decorative shadows at asphalt.
        d.point([(px,py) for py in range(y+4,y+h+4) for px in range(x+4,x+w+4)
                 if not road(px+0.5,py+0.5)],fill=COLORS[0])
        roof=16 if tall else 8
        box(x,y,w,h,1);d.rectangle((x,y,x+w-1,y+h-1),outline=COLORS[0])
        box(x+2,y+2,w-4,h-roof-2,2)
        d.rectangle((x+4,y+4,x+w-5,y+h-roof-3),outline=COLORS[0])
        for wx in range(x+4,x+w-4,8):box(wx,y+h-roof+3,4,3,3)
        box(x+w//2-2,y+h-5,4,4,0)
        if w<24:style=2
        if style==0: # striped shop awning, rooftop utility
            for wx in range(x+2,x+w-2,8):box(wx,y+h-8,4,3,3)
            box(x+8,y+8,8,8,1)
        elif style==1: # brick apartment courtyard
            box(x+8,y+8,max(8,w-16),8,1)
            for wx in range(x+8,x+w-8,8):box(wx,y+12,4,2,3)
        elif style==2: # pitched roof row house
            d.line((x+w//2,y+4,x+w//2,y+h-roof-4),fill=COLORS[0],width=2)
            box(x+4,y+8,4,4,1)
        elif style==3: # stepped Art Deco / civic terraces
            d.rectangle((x+8,y+8,x+w-9,y+h-roof-7),outline=COLORS[3])
            box(x+w//2-4,y+8,8,8,1)
        elif style==4: # glass tower grid and mechanical penthouse
            for wx in range(x+8,x+w-8,8):d.line((wx,y+4,wx,y+h-roof-4),fill=COLORS[3])
            box(x+8,y+8,8,8,1)
        else: # wide warehouse: skylights and loading doors
            for wx in range(x+8,x+w-8,16):box(wx,y+8,8,8,3);box(wx,y+h-6,8,4,0)
        attr(x,y,w+8,h+8,1+style,True,avoid_road=True)
        # Raised north roof lip over an 8-pixel footpath; correct CGB occlusion.
        box(x,y-8,w,8,2);d.line((x,y-8,x+w-1,y-8),fill=COLORS[0]);attr(x,y-8,w,8,1+style,True,avoid_road=True)
        blocks.append({'x':x,'y':y,'width':w,'depth':h,'height':roof,'style':style,'landmark':label})
    for ci in range(len(COLS)-1):
        for ri in range(len(ROWS)-1):
            left,right=COLS[ci]+32,COLS[ci+1]-32;top,bottom=ROWS[ri]+32,ROWS[ri+1]-32
            if right-left<16 or bottom-top<16 or (left<912 and right>872):continue
            style=(ci*3+ri)%6
            if right-left<16 or bottom-top<16:continue
            width=32 if right-left>=80 else 24 if right-left>=56 else (right-left)//8*8
            width=min(width,48)
            depth=24 if bottom-top>=56 else min(40,(bottom-top)//8*8)
            for y in range(top,bottom-depth+1,depth+8):
                for x in range(left,right-width+1,width+8):
                    building(x,y,width,depth,style,style in (3,4))
    # Landmark shapes remain original and readable at native scale.
    building(368,672,80,24,3,label='Union Station colonnade')
    building(752,672,32,24,1,label='St Lawrence Market')
    building(848,672,24,24,5,label='Distillery warehouses')
    building(512,440,24,40,3,True,'City Hall civic block')
    building(400,96,48,40,4,True,'Royal Ontario Museum')
    building(368,432,48,56,3,True,'Art Gallery of Ontario')
    building(744,584,32,24,1,label='Gooderham Flatiron')
    building(480,928,32,24,3,label='Hanlans service pavilion')
    building(664,904,32,24,0,label='Centre Island pavilion')
    building(872,888,24,24,2,label='Wards Island cottages')
    # CN needle and circular observation deck in an off-road plaza west of Union.
    d.ellipse((376,664,407,695),fill=COLORS[1],outline=COLORS[0]);box(390,650,4,36,0);box(388,688,8,8,3);attr(376,648,32,48,4,True)
    # Parks, trees, street furniture and harbour bollards reuse a handful of tiles.
    for u,v in [(112,112),(152,448),(248,336),(360,568),(488,224),(584,336),(744,208),(832,448),(960,336),(352,936),(656,936),(816,904),(664,680),(664,568),(360,448),(504,336),(584,208)]:
        u=u//8*8;v=v//8*8
        box(u+6,v+16,3,8,0);d.ellipse((u,v,u+15,v+15),fill=COLORS[1],outline=COLORS[0]);box(u+4,v+4,8,8,2);attr(u,v,16,16,6,True);canopies.append([u,v,16,16])
    for u in range(32,992,32):box(u,808,4,4,0)
    # Ferry/subway stations: simple platform squares on a reachable pavement.
    stops=[(288,368),(320,64),(320,112),(320,160),(320,224),(320,288),(320,336),(96,64),(416,64),(320,400),(240,520),(352,512),(432,496)]
    for old_u,old_v in stops:
        u,v=location(old_u,old_v);d.rectangle((u-4,v-4,u+3,v+3),fill=COLORS[3],outline=COLORS[0]);d.line((u-2,v,u+1,v),fill=COLORS[0])
    img.save(PROJECT/'assets/backgrounds/toronto_city.png')
    content={'projection':'orthogonal north-up; x=u, y=v','dimensions':[WIDTH,HEIGHT],'rows':ROWS,'columns':COLS,'road_half_width':ROAD_HALF,'walk_half_width':WALK_HALF,'river':RIVER,'bridges':BRIDGES,'mainland':MAINLAND,'islands':ISLANDS,'blocks':blocks,'canopies':canopies,'scope':'Compressed central Toronto and Island service areas; full Old Toronto boundaries remain a release check'}
    (ROOT/'content/city_art.json').write_text(json.dumps(content,indent=2)+'\n')
    (PROJECT/'original-art/city_attributes.json').write_text(json.dumps(attrs)+'\n')
    if background_only:
        print(f'Authored {WIDTH}x{HEIGHT} north-up city, {len(blocks)} varied buildings; sprites unchanged.')
        return
    # 32 vehicle headings, 8 walking frames, 4 beacon frames, open-door car frame.
    sheet=Image.new('RGB',(256,48),TRANSPARENT);sd=ImageDraw.Draw(sheet)
    for frame in range(45):
        ox=(frame%16)*16;oy=(frame//16)*16
        veh=frame//8;heading=frame%8
        if frame<32 or frame==44:
            if frame==44:veh=0;heading=0
            a=heading*math.pi/4;dx,dy=math.cos(a),math.sin(a)
            length=(6,7,5,4)[veh];width=(3,4,2,2)[veh]
            pts=[(ox+8+dx*f-dy*s,oy+8+dy*f+dx*s) for f,s in ((length,width),(length,-width),(-length,-width),(-length,width))]
            sd.polygon(pts,fill=COLORS[2],outline=COLORS[0]);sd.line((ox+8+dx*2-dy*width,oy+8+dy*2+dx*width,ox+8+dx*2+dy*width,oy+8+dy*2-dx*width),fill=COLORS[3],width=2)
            if frame==44:sd.line((ox+8,oy+11,ox+11,oy+15),fill=COLORS[2],width=2)
        elif frame<40:
            direction=(frame-32)//2;step=frame&1
            sd.ellipse((ox+6,oy+2,ox+10,oy+6),fill=COLORS[3],outline=COLORS[0]);sd.rectangle((ox+6,oy+7,ox+10,oy+11),fill=COLORS[2]);sd.line((ox+6,oy+12,ox+5+step*2,oy+15),fill=COLORS[0]);sd.line((ox+10,oy+12,ox+11-step*2,oy+15),fill=COLORS[0]);sd.point((ox+(6 if direction==1 else 10),oy+5),fill=COLORS[0])
        else:
            sd.polygon([(ox+8,oy+1),(ox+14,oy+7),(ox+8,oy+14),(ox+2,oy+7)],fill=COLORS[3],outline=COLORS[0]);sd.rectangle((ox+7,oy+4,ox+9,oy+9),fill=COLORS[2])
    sheet.save(PROJECT/'original-art/dispatch_topdown.png')
    frames=[]
    for n in range(45):
        frames.append({'id':ident(f'frame-{n}'),'tiles':[{'id':ident(f'tile-{n}-{x}'),'x':x,'y':0,'sliceX':(n%16)*16+x,'sliceY':(n//16)*16,'flipX':False,'flipY':False,'palette':0,'paletteIndex':0,'objPalette':'OBP0','priority':False} for x in (0,8)]})
    states=[]
    for name,subset in [('vehicles',frames[:32]),('courier',frames[32:])]:
        animations=[{'id':ident(name+'-animation'),'frames':subset}]+[{'id':ident(f'{name}-empty-{n}'),'frames':[{'id':ident(f'{name}-emptyframe-{n}'),'tiles':[]}]} for n in range(7)]
        states.append({'id':ident(name+'-state'),'name':'' if name=='vehicles' else 'Courier and beacon','animationType':'fixed','flipLeft':False,'animations':animations})
    meta={'_resourceType':'sprite','id':ident('vehicles'),'name':'Top-down vehicles and courier','symbol':'sprite_dispatch_topdown','states':states,'width':256,'height':48,'canvasOriginX':8,'canvasOriginY':8,'canvasWidth':16,'canvasHeight':16,'boundsX':2,'boundsY':2,'boundsWidth':12,'boundsHeight':12,'animSpeed':255,'numTiles':0,'filename':'dispatch_topdown.png','checksum':hashlib.sha1((PROJECT/'original-art/dispatch_topdown.png').read_bytes()).hexdigest()}
    (PROJECT/'dispatch_topdown.metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
    print(f'Authored {WIDTH}x{HEIGHT} north-up city, {len(blocks)} varied buildings, 45 vehicle/courier frames.')
if __name__=='__main__':main(background_only='--background-only' in sys.argv)
