"""Generate/check original North source art and full-body collision.

This generator has no native registration, engine or campaign side effects. All output
paths stay inside this game. Official City material supplies
public names/topology only; artwork and compression are original.
"""
import argparse
from collections import deque
from functools import lru_cache
import hashlib
import io
import json
from pathlib import Path
from PIL import Image, ImageDraw
from north_layout import (WIDTH, HEIGHT, ROAD_HALF, WALK_HALF,
                          FOOT_HALF, RAIL_HALF, read_layout,
                          pairs, pixel_points)

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / 'project/original-art'
CONTENT = ROOT / 'content/districts'
COLORS = ['#071821', '#306850', '#86c06c', '#e0f8cf']
RGB = [tuple(bytes.fromhex(c[1:])) for c in COLORS]
TW, TH = WIDTH//8, HEIGHT//8
FLEET_HALVES = (5, 6, 5, 7, 6, 7)
EXPECTED_LAYOUT = 'bcec25cc0bd402bfeef20de4e36ea3be68c38f47fadfcaef12117a3b932b8733'


def paint_path(draw, points, half, fill):
    for (x,y),(u,v) in pairs(points):
        draw.rectangle((min(x,u)-half,min(y,v)-half,max(x,u)+half-1,max(y,v)+half-1),fill=fill)


def rect_overlap(a,b):
    x,y,w,h=a;u,v,ww,hh=b
    return x<u+ww and u<x+w and y<v+hh and v<y+h


def render():
    layout, layout_sha = read_layout()
    assert layout_sha == EXPECTED_LAYOUT, 'Review revised North topology before freezing source art'
    image=Image.new('RGB',(WIDTH,HEIGHT),RGB[2]);draw=ImageDraw.Draw(image)
    road=Image.new('1',image.size);walk=Image.new('1',image.size)
    rail=Image.new('1',image.size);cross=Image.new('1',image.size)
    roof=Image.new('1',image.size)
    rd,wd,rld,cd=map(ImageDraw.Draw,(road,walk,rail,cross))
    attrs=[6]*(TW*TH);grid=[16]*(TW*TH);blocks=[];canopies=[]
    for route in layout['roads']:
        paint_path(rd,route['points'],ROAD_HALF,1)
        paint_path(wd,route['points'],WALK_HALF,1)
    for route in layout['footpaths']:paint_path(wd,route['points'],FOOT_HALF,1)
    for route in layout['rails']:paint_path(rld,route['points'],RAIL_HALF,1)
    for crossing in layout['rail_crossings']:
        x,y,w,h=crossing['rect'];cd.rectangle((x,y,x+w-1,y+h-1),fill=1)
    assert len(layout['rail_crossings'])==5 and len(layout['ports'])==2

    def cells(x,y,w,h):
        for ty in range(max(0,y//8),min(TH,(y+h+7)//8)):
            for tx in range(max(0,x//8),min(TW,(x+w+7)//8)):
                yield tx,ty,ty*TW+tx

    def box(x,y,w,h,color):
        assert w>0 and h>0
        draw.rectangle((x,y,x+w-1,y+h-1),fill=RGB[color])

    def crown(x,y):
        # Original16px stepped foliage with a small highlight, distinct from
        # a rectangular building/window module at native screen scale.
        for dx,dy,w,h in ((4,0,8,2),(2,2,12,2),(0,4,16,8),(2,12,12,2),(4,14,8,2)):box(x+dx,y+dy,w,h,0)
        for dx,dy,w,h in ((4,2,8,2),(2,4,12,8),(4,12,8,2)):box(x+dx,y+dy,w,h,1)
        box(x+4,y+4,6,4,2)

    def solid(rect):
        for _,_,i in cells(*rect):grid[i]=15

    def palette(rect,slot):
        assert 0<=slot<=6
        for _,_,i in cells(*rect):attrs[i]=slot

    def mask_rect(rect):
        x,y,w,h=rect
        assert all(n%8==0 for n in rect)
        return x,y,x+w-1,y+h-1

    for forced in layout['forced_foot_masks']:
        wd.rectangle(mask_rect(forced['rect']),fill=1)
        rd.rectangle(mask_rect(forced['rect']),fill=0)
    # The two curb clients are intentionally pedestrians-only. Rosehill's
    # point lies on the northern asphalt edge, so this small original curb
    # removes that edge tile without changing the road's280px centre line.
    rose_curb=tuple(next(f['rect'] for f in layout['forced_foot_masks'] if f['name']=='ROSEHILL PUBLIC FOOT CURB'))
    rail_cells=[];unapproved_asphalt=[]
    for ty in range(TH):
        for tx in range(TW):
            x,y,i=tx*8,ty*8,ty*TW+tx
            asphalt=road.getpixel((x+4,y+4));pavement=walk.getpixel((x+4,y+4))
            track=rail.getpixel((x+4,y+4));approved=cross.getpixel((x+4,y+4))
            if track:rail_cells.append(i)
            if track and not approved:
                if asphalt:unapproved_asphalt.append([x,y])
                asphalt=pavement=0;rd.rectangle((x,y,x+7,y+7),fill=0);wd.rectangle((x,y,x+7,y+7),fill=0)
                grid[i]=15;box(x,y,8,8,0);attrs[i]=0
            elif asphalt:
                grid[i]=0;box(x,y,8,8,1);attrs[i]=0
            elif pavement:
                box(x,y,8,8,3);attrs[i]=0
    assert not unapproved_asphalt, ('Unverified rail crossing in authored road',unapproved_asphalt)
    for route in layout['roads']:
        for (x,y),(u,v) in pairs(route['points']):
            if y==v:
                for xx in range((min(x,u)//32+1)*32,max(x,u),32):
                    if 0<=xx<WIDTH and 0<=y<HEIGHT and road.getpixel((xx,y)):box(xx,y,8,1,3)
            else:
                for yy in range((min(y,v)//32+1)*32,max(y,v),32):
                    if 0<=x<WIDTH and 0<=yy<HEIGHT and road.getpixel((x,yy)):box(x,yy,1,8,3)
    # Original sleepers, plus retaining shoulders at five separated underpasses.
    # Asphalt remains visually clear; no added rail service or timetable.
    for route in layout['rails']:
        for (x,y),(u,v) in pairs(route['points']):
            if y==v:
                for xx in range(min(x,u),max(x,u),8):
                    if not cross.getpixel((xx,y)):box(xx,y-6,2,12,1);box(xx,y-3,8,1,2)
            else:
                for yy in range(min(y,v),max(y,v),8):
                    if not cross.getpixel((x,yy)):box(x-6,yy,12,2,1);box(x-3,yy,1,8,2)
    for crossing in layout['rail_crossings']:
        x,y,w,h=crossing['rect'];centre=x+w//2
        for xx in (centre-ROAD_HALF-5,centre+ROAD_HALF+3):
            box(xx,y+8,2,h-16,0)
    # Stairs have alternating treads and remain collision16 through the
    # escarpment gap; road endcaps are not connected through the hill.
    for yy in range(512,552,8):box(328,yy,16,2,1)
    # Only declared public original green ground stays passable. The northern
    # scene edge/ravine and museum/castle estate fences are closed.
    # The creek/ravine remains closed; green contours and forest crowns
    # distinguish it from harbour water. Repeated modules conserve tile ROM.
    for rect in layout['water']:
        x,y,w,h=rect;solid(rect);box(x,y,w,h,1);palette(rect,0)
        for yy in range(y+8,y+h-8,24):box(x+8,yy,8,8,2);box(x+10,yy+2,4,4,1)
    for area in layout['blocked_ravines']:
        rect=area['rect'];x,y,w,h=rect;solid(rect);box(x,y,w,h,2);palette(rect,6)
        for yy in range(y+8,y+h-15,24):
            for xx in range(x+8,x+w-15,24):
                crown(xx,yy)
        for yy in range(y+8,y+h-8,48):box(x+2,yy,2,24,1)
    # Sparse original ridge crowns below the closed north frontier. The
    # absent farther-neighbourhood scene is not an extra exit or park route.
    for yy in (48,96):
        for xx in range(48,WIDTH-32,64):
            crown(xx,yy)
            solid((xx,yy,16,16));palette((xx,yy,16,16),6)
    for area in layout['private_ground']:
        rect=area['rect'];x,y,w,h=rect
        assert all(not road.getpixel((tx*8+4,ty*8+4)) for tx,ty,_ in cells(*rect)), ('Private grounds over road',area)
        solid(rect);box(x,y,w,h,2);palette(rect,6)
        draw.rectangle((x,y,x+w-1,y+h-1),outline=RGB[0])
        for xx in range(x+4,x+w,8):box(xx,y,1,4,0);box(xx,y+h-4,1,4,0)
    frontiers=[(0,0,WIDTH,16),(0,HEIGHT-16,WIDTH,16),(0,0,16,HEIGHT),(WIDTH-16,0,16,HEIGHT)]
    for rect in frontiers:solid(rect);box(*rect,1);palette(rect,0)
    for port in layout['ports']:
        assert port['edge']=='south' and port['y']==952
        x=port['x']
        for tx,ty,i in cells(x-WALK_HALF,HEIGHT-16,WALK_HALF*2,16):
            grid[i]=0 if abs(tx*8+4-x)<ROAD_HALF else 16;attrs[i]=0
            box(tx*8,ty*8,8,8,1 if grid[i]==0 else 3)

    reserved=[p['rect'] for p in layout['parks']]
    reserved+=[p['rect'] for p in layout['private_ground']]
    reserved+=[(s['x']-16,s['y']-16,32,32) for s in layout['stop_candidates']]
    reserved+=[(p['x']-12,p['y']-16,p['width']+24,p['depth']+32) for p in layout['landmarks']]
    reserved += [rose_curb] + [f['rect'] for f in layout['forced_foot_masks']]
    for s in layout['stop_candidates']:
        if s.get('parking_anchor'):
            x,y=s['parking_anchor'];reserved.append((x-32,y-32,64,64))

    def may_build(x,y,w,h,landmark=False):
        if not(x>=32 and y>=64 and x+w<=WIDTH-32 and y+h<=HEIGHT-40):return False
        rect=(x,y,w,h)
        if not landmark and any(rect_overlap((x-8,y-8,w+16,h+16),r) for r in reserved):return False
        return all(not walk.getpixel((tx*8+4,ty*8+4)) and not rail.getpixel((tx*8+4,ty*8+4)) and
                   (grid[i]!=15 or landmark) for tx,ty,i in cells(*rect))

    def building(x,y,w,h,style,kind=None,name=None):
        assert may_build(x,y,w,h,bool(name)), ('Building footprint intersects pavement',name,x,y,w,h)
        layer=Image.new('RGBA',image.size,(0,0,0,0));ld=ImageDraw.Draw(layer)
        def elevated(xx,yy,ww,hh,c):
            ld.rectangle((xx,yy,xx+ww-1,yy+hh-1),fill=RGB[c]+(255,))
        def volume(xx,top,ww,bottom,c=2):
            elevated(xx,top,ww,bottom-top,0)
            elevated(xx+2,top+2,ww-4,bottom-top-4,1)
            if ww>=16 and bottom-top>=16:
                elevated(xx+4,top+4,ww-8,bottom-top-8,c)
                for yy in range(top+8,bottom-3,8):elevated(xx+4,yy,ww-8,1,1)
        height=(8,16,24,16,8,32)[style]
        top=y-height
        volume(x,top,w,y+h)
        for yy in range(y+8,y+h-8,8):
            for xx in range(x+8,x+w-7,16):elevated(xx,yy,4,4,3)
        elevated(x+w//2-2,y+h-8,4,8,0)
        if kind=='casa_loma':
            volume(x,y-40,16,y+h);volume(x+w-16,y-56,16,y+h)
            for xx,tt in ((x,y-40),(x+w-16,y-56)):
                for sx in (xx,xx+8):elevated(sx+2,tt,4,4,3)
                elevated(xx+6,tt+12,4,16,0)
            elevated(x+24,y+h-16,8,16,0)
            for xx in range(x+20,x+w-20,8):elevated(xx,y+8,4,8,0)
        elif kind=='heritage_station':
            volume(x+w-16,y-48,16,y+h)
            elevated(x+w-13,y-36,10,10,3)
            ld.line((x+w-8,y-34,x+w-8,y-30,x+w-5,y-30),fill=RGB[0]+(255,))
            for xx in range(x+8,x+w-24,16):elevated(xx,y+6,8,12,0);elevated(xx+2,y+8,4,8,3)
            elevated(x,y-8,w-16,4,3)
        elif kind=='heritage_house':
            elevated(x+8,top+4,w-16,4,3)
            for xx in range(x+8,x+w-7,8):elevated(xx,y+h-8,2,6,3)
        elif kind=='deer_park_junction':
            volume(x,y-32,32,y+h);volume(x+w-32,y-48,32,y+h)
            for xx,tt in ((x,y-32),(x+w-32,y-48)):
                for yy in range(tt+8,y+h-8,8):
                    for sx in range(xx+8,xx+25,8):elevated(sx,yy,4,4,3)
        elif kind=='station_entrance':
            elevated(x+4,y-4,w-8,4,1);elevated(x+8,y+4,w-16,8,0)
        elif style==0: # bay-gabled Annex house, repeated original roof module
            for xx in range(x+8,x+w-7,8):elevated(xx,top+4,4,4,3)
            elevated(x+4,y+h-8,w-8,3,3)
        elif style==1: # wide brick retail bays
            for yy in range(y,y+h-8,8):
                for xx in range(x+4,x+w-7,16):elevated(xx,yy,8,1,1)
            for xx in range(x+4,x+w-7,16):elevated(xx,y+h-12,8,8,0)
        elif style==2: # pitched heritage roof
            elevated(x+w//2-1,top+4,2,height+4,0)
        elif style==3: # midrise civic apartment
            elevated(x+8,top+8,w-16,2,3)
        elif style==4: # shop awning with small roof utility
            for xx in range(x+4,x+w-4,8):elevated(xx,y+h-8,4,3,3)
            elevated(x+8,top+8,8,8,1)
        else: # original glass/podium towers
            for yy in range(top+8,y+h-8,8):
                for xx in range(x+8,x+w-7,8):elevated(xx,yy,4,4,3)
        # Roof pixels can overhang public walking terrain, but never paint or
        # take priority over asphalt. Mask is the actual generated artwork.
        alpha=layer.getchannel('A');ad=ImageDraw.Draw(alpha)
        for tx,ty,i in cells(x-8,top-8,w+16,y+h-top+16):
            if grid[i]==0:ad.rectangle((tx*8,ty*8,tx*8+7,ty*8+7),fill=0)
        layer.putalpha(alpha);image.paste(layer,(0,0),alpha)
        ImageDraw.Draw(roof).bitmap((0,0),alpha,fill=1)
        slot=4 if kind=='casa_loma' else 1 if kind=='heritage_station' else 2 if kind=='deer_park_junction' else 0 if kind=='station_entrance' else (1,1,4,3,4,2)[style]
        for tx,ty,i in cells(x-8,top-8,w+16,y+h-top+16):
            if alpha.crop((tx*8,ty*8,tx*8+8,ty*8+8)).getbbox():attrs[i]=slot
        solid((x,y,w,h))
        blocks.append({'x':x,'y':y,'width':w,'depth':h,'height':height,'style':style,'landmark':name,'kind':kind})

    for landmark in layout['landmarks']:
        building(landmark['x'],landmark['y'],landmark['width'],landmark['depth'],landmark['style'],landmark['kind'],landmark['name'])
    # Deterministic original street blocks. Full footprint/sidewalk/POI checks
    # govern placement rather than decorating a road with a solid obstacle.
    for yy in range(208,HEIGHT-56,32):
        for xx in range(40,800,32):
            style=(xx//32*3+yy//32)%6
            if yy>=800: style=(xx//32+yy//32)%3;w,h=((24,24),(32,40),(48,32))[style]
            elif yy<352:w,h=((48,48),(64,40),(40,32))[style%3]
            else:w,h=((32,32),(48,32),(24,32),(32,40),(48,24),(64,40))[style]
            if may_build(xx,yy,w,h):building(xx,yy,w,h,style)
            elif may_build(xx,yy,24,24):building(xx,yy,24,24,style)
    # Park trees have separate solid trunks and precise nonblank canopies.
    for park in layout['parks']:
        x,y,w,h=park['rect']
        for yy in range((y+7)//8*8,y+h-23,32):
            for xx in range((x+7)//8*8,x+w-15,32):
                if any(walk.getpixel((tx*8+4,ty*8+4)) or grid[i]==15 or
                       any(rect_overlap((xx,yy,16,24),r) for r in reserved[3:])
                       for tx,ty,i in cells(xx,yy,16,24)):continue
                box(xx+6,yy+16,3,8,0);solid((xx,yy+16,16,8))
                crown(xx,yy)
                ImageDraw.Draw(roof).rectangle((xx,yy,xx+15,yy+15),fill=1)
                palette((xx,yy,16,16),6);canopies.append([xx,yy,16,16])
    for ty in range(TH):
        for tx in range(TW):
            i=ty*TW+tx
            if grid[i]!=0 and roof.crop((tx*8,ty*8,tx*8+8,ty*8+8)).getbbox():attrs[i]|=128
    return validate(image,attrs,grid,blocks,canopies,layout,layout_sha,rail_cells,rose_curb)


def validate(image,attrs,grid,blocks,canopies,layout,layout_sha,rail_cells,rose_curb):
    checks=0;swept=0
    @lru_cache(None)
    def clear(x,y,half,car):
        if not(0<=x-half and x+half<WIDTH and 0<=y-half and y+half<HEIGHT):return False
        return all(grid[ty*TW+tx]==0 if car else not(grid[ty*TW+tx]&15)
                   for ty in range((y-half)//8,(y+half)//8+1) for tx in range((x-half)//8,(x+half)//8+1))
    def check(value,*label):
        nonlocal checks
        checks+=1;assert value,label
    def sweep(points,half=5,car=False,closed=False):
        nonlocal swept
        for x,y in pixel_points(points,closed):
            check(clear(x,y,half,car),'Full-body sweep',x,y,half,car);swept+=1
    for route in layout['roads']:
        points=[[max(8,min(WIDTH-9,x)),max(8,min(HEIGHT-9,y))] for x,y in route['points']]
        sweep(points,7,True)
    for route in layout['footpaths']:sweep(route['points'],5,False)
    for i,loop in enumerate(layout['traffic_loops']):
        check(4<=len(loop)<=16,'Route metadata limits',i)
        sweep(loop,FLEET_HALVES[i],True,True)
        sweep(loop,8,True,True)
    for i in range(6):
        for j in range(i+1,6):
            for (x,y),(u,v) in pairs(layout['traffic_loops'][i],True):
                for (a,b),(c,d) in pairs(layout['traffic_loops'][j],True):
                    if y==v and b==d and (u-x)*(c-a)<0 and max(min(x,u),min(a,c))<min(max(x,u),max(a,c)):
                        check(abs(y-b)>=FLEET_HALVES[i]+FLEET_HALVES[j],'Opposing horizontal fleet',i,j,y,b)
                    if x==u and a==c and (v-y)*(d-b)<0 and max(min(y,v),min(b,d))<min(max(y,v),max(b,d)):
                        check(abs(x-a)>=FLEET_HALVES[i]+FLEET_HALVES[j],'Opposing vertical fleet',i,j,x,a)
    for crossing in layout['rail_crossings']:
        x,y,w,h=crossing['rect'];u=x+w//2
        for offset in range(-18,19):sweep([[u+offset,y],[u+offset,y+h-1]],5,True)
        for offset in range(-14,15):sweep([[u+offset,y],[u+offset,y+h-1]],7,True)
        for offset in range(-18,19):sweep([[u+offset,y],[u+offset,y+h-1]],5,False)
    for port in layout['ports']:
        x=port['x']
        for y in range(944,969):
            for offset in range(-18,19):check(clear(x+offset,y,5,True),'Native car seam',x,y,offset)
            for offset in range(-14,15):check(clear(x+offset,y,7,True),'Conservative car seam',x,y,offset)
            for offset in range(-28,29):check(clear(x+offset,y,3,False),'Native foot seam',x,y,offset)
            for offset in range(-24,25):check(clear(x+offset,y,5,False),'Conservative foot seam',x,y,offset)
    for x,y in ((336,532),(264,488)):
        check(clear(x,y,5,False) and not clear(x,y,5,True),'Stairs / boulevard vehicle exclusion',x,y)
    for area in layout['private_ground']:
        x,y,w,h=area['rect']
        for yy in range(y,y+h,8):
            for xx in range(x,x+w,8):check(grid[(yy//8)*TW+xx//8]==15,'Closed estate',xx,yy)
    for index in rail_cells:
        tx,ty=index%TW,index//TW;cx,cy=tx*8+4,ty*8+4
        allowed=any(x<=cx<x+w and y<=cy<y+h for x,y,w,h in [c['rect'] for c in layout['rail_crossings']])
        check(allowed or grid[index]==15,'No unverified rail crossing',cx,cy)
    for stop in layout['stop_candidates']:
        x,y=stop['x'],stop['y'];check(clear(x,y,5,False),'Client foot body',stop)
        check(clear(x,y,3,False),'Transit landing body',stop)
        if stop['foot_only']:
            check(not clear(x,y,5,True),'Foot client vehicle excluded',stop)
            check(clear(*stop['parking_anchor'],8,True),'Full parking body',stop)
        else:check(clear(x,y,7,True),'Full car client',stop)

    def connected(car,half):
        origin=(336,952);check(clear(*origin,half,car),'Connected origin')
        queue=deque([origin]);seen={origin};parents={origin:None}
        while queue:
            x,y=queue.popleft()
            for point in ((x-8,y),(x+8,y),(x,y-8),(x,y+8)):
                if point not in seen and clear(*point,half,car):seen.add(point);parents[point]=(x,y);queue.append(point)
        return seen,parents
    foot,foot_parents=connected(False,5);car,car_parents=connected(True,7)
    for stop in layout['stop_candidates']:
        check((stop['x'],stop['y']) in foot,'Full-foot graph',stop)
        if not stop['foot_only']:check((stop['x'],stop['y']) in car,'Full-car graph',stop)
        if stop.get('parking_anchor'):check(tuple(stop['parking_anchor']) in car,'Parking graph',stop)
    check((640,952) in foot and (640,952) in car,'Both seams join')
    # Real full-body shortest routes on this conservative8px graph. Every
    # compressed section is swept again at1px; NPC/transit timing is excluded.
    routes=[]
    def graph_path(target,parents,half,is_car,label):
        points=[];cursor=tuple(target)
        while cursor is not None:points.append(list(cursor));cursor=parents[cursor]
        points.reverse();compressed=[points[0]]
        for index in range(1,len(points)-1):
            a,b,c=points[index-1:index+2]
            if (b[0]-a[0],b[1]-a[1])!=(c[0]-b[0],c[1]-b[1]):compressed.append(b)
        if len(points)>1:compressed.append(points[-1]);sweep(compressed,half,is_car)
        routes.append({'name':label,'points':compressed,'pixels':(len(points)-1)*8,'body_half':half,'vehicle':is_car,
                       'only_static_source_geometry':True,'native_stopping_tolerance_or_traffic_not_guaranteed':True})
    for stop in layout['stop_candidates']:
        graph_path([stop['x'],stop['y']],foot_parents,5,False,f"Foot from Spadina seam to{stop['id_proposal']}")
        target=stop.get('parking_anchor') if stop['foot_only'] else [stop['x'],stop['y']]
        graph_path(target,car_parents,7,True,f"Vehicle from Spadina seam to{stop['id_proposal']} or park")
    graph_path([640,952],car_parents,7,True,'Vehicle between both mainland seams')
    check(clear(336,488,7,True) and (336,488) in car,'Upper Casa parking alternative')
    graph_path([336,488],car_parents,7,True,'Upper Casa drive via St Clair; no car on steps')
    # Full-body fixed63px horizontal routes, not point-only sidewalk samples.
    peds=set()
    for route in layout['roads']+layout['footpaths']:
        for a,b in pairs(route['points']):
            if a[1]!=b[1]:continue
            left,right=sorted((a[0],b[0]))
            for offset in ((-28,28) if route in layout['roads'] else (0,)):
                for x in range((left+7)//8*8,right-62,64):
                    y=a[1]+offset
                    if all(clear(px,y,5,False) for px in range(x,x+64)):peds.add((x,y))
    peds=sorted(peds,key=lambda p:(p[1],p[0]));check(len(peds)>=24,'Civilian route variety',len(peds))
    if len(peds)>128:peds=[peds[i*len(peds)//128] for i in range(128)]
    for x,y in peds:sweep([[x,y],[x+63,y]],5,False)
    # Meaningful car-door landing samples around each legal anchor. Existing
    # native exit uses±18pxcardinal targets and a10.5px conservative exclusion.
    doors=[]
    parking_stops=layout['stop_candidates']+[dict(next(s for s in layout['stop_candidates'] if s['id_proposal']==61),parking_anchor=[336,488],parking_kind='upper alternative; source option only')]
    for stop in parking_stops:
        park=stop.get('parking_anchor')
        if not park:continue
        candidates=[]
        for dx,dy in ((18,0),(-18,0),(0,18),(0,-18)):
            point=(park[0]+dx,park[1]+dy)
            if clear(*point,5,False):candidates.append(list(point))
        check(bool(candidates),'At least one conservative parked exit',stop)
        robust=[]
        for dx,dy in ((18,0),(-18,0),(0,18),(0,-18)):
            ok=True
            # Match the actual native landing terrain floor; the exact Q4
            # parked exclusion is independently checked before pixel lookup.
            for fu in range(-48,49):
                for fv in range(-48,49):
                    pu,pv=park[0]*16+fu,park[1]*16+fv;u,v=pu+dx*16,pv+dy*16
                    parked_body=clear(pu>>4,pv>>4,7,True)
                    exclusion=abs(u-pu)>=168 or abs(v-pv)>=168
                    native=clear(u>>4,v>>4,3,False);conservative=clear(u>>4,v>>4,5,False)
                    check(parked_body,'Stopped tolerance parking footprint',stop,fu,fv)
                    check(exclusion,'Native parked Q4 exclusion',stop,dx,dy)
                    ok=ok and native and conservative
            if ok:robust.append([park[0]+dx,park[1]+dy])
        check(bool(robust),'A full-half5 exit across all +/-3px Q4 stopped poses',stop)
        doors.append({'stop_id':stop['id_proposal'],'parking_kind':stop.get('parking_kind','authored anchor'),'park':park,'full_half5_foot_candidates':candidates,
                      'native_half3_and_conservative_half5_all_q4_stopped_tolerance_candidates':robust,
                      'q4_stopped_offsets_each_axis':[-48,48],'q4_positions_per_cardinal_exit':9409,
                      'exact_q4_parked_exclusion':168,'source_collision_only':True,'native_exit_or_actor_traffic_unverified':True})
    raw=set();canonical=set();blank_priority=0
    for ty in range(TH):
        for tx in range(TW):
            i=ty*TW+tx;tile=image.crop((tx*8,ty*8,tx*8+8,ty*8+8));raw.add(tile.tobytes())
            canonical.add(min(t.tobytes() for t in (tile,tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT),tile.transpose(Image.Transpose.FLIP_TOP_BOTTOM),tile.transpose(Image.Transpose.ROTATE_180))))
            check(not(attrs[i]&128) or grid[i]!=0,'No asphalt priority',tx,ty)
            if attrs[i]&128 and grid[i]==16 and set(tile.get_flattened_data())=={RGB[2]}:blank_priority+=1
    check(blank_priority==0,'No blank passable grass priority')
    check(len(raw)<=320 and len(canonical)<=240,'Source tile target',len(raw),len(canonical))
    check(set(image.get_flattened_data())<=set(RGB),'Exactly four source shades')
    check(len(grid)==15616 and set(grid)<={0,15,16},'Native collision alphabet')
    check(all((a&7)<=6 and not(a&0x78) for a in attrs),'UI palette and attribute bits')
    for block in blocks:
        x,y,w,h=block['x'],block['y'],block['width'],block['depth']
        check(all(grid[yy*TW+xx]==15 for yy in range(y//8,(y+h)//8) for xx in range(x//8,(x+w)//8)),'Solid actual building base',block)
    buffer=io.BytesIO();image.save(buffer,format='PNG');png=buffer.getvalue()
    metadata={**layout,'status':'Generated original North source asset; native registration/build/play/hardware evidence remains separate.',
        'projection':'Original orthogonal north-up game compression, not converted City geometry or artwork.',
        'background_filename':'toronto_north.png','source_png':'project/original-art/toronto_north.png',
        'source_attributes':'project/original-art/north_attributes.json','tile_dimensions':[TW,TH],
        'source_layout_sha256':layout_sha,'background_sha256':hashlib.sha256(png).hexdigest(),
        'background_pixel_sha256':hashlib.sha256(image.tobytes()).hexdigest(),
        'blocks':blocks,'canopies':canopies,'collisions':grid,'forced_foot_curbs_applied_from_source':True,
        'pedestrian_routes':[{'x':x,'y':y,'axis':'horizontal','length_pixels':63} for x,y in peds],
        'door_candidates':doors,'full_body_route_proposals':routes,'collision_rules':{'road':0,'foot_only':16,'solid':15},
        'palette_proposal':{'source_shades':COLORS,'slots0_to5':'Existing stone,brick,glass,heritage and pale sandstone families.',
                            'architecture_style_slots':[1,1,4,3,4,2],'castle_slot':4,'heritage_station_slot':1,'deer_park_glass_slot':2,'subway_entrance_slot':0,
                            'slot6':'Existing Island green park palette','slot7':'Existing UI reserved; unused by source art.'},
        'rail_crossing_art':{'rail_segments_stop_outside_approved_clearance_envelopes':True,'retaining_shoulders_on_sidewalk_edges':True,'raised_deck_over_asphalt':False,'overhead_rail_priority':False,'native_underpass_occlusion_claimed':False},
        'simulation_proposal':{'six_existing_fleet_slots':True,'queen':False,'boats':False,'new_sprite_sheets':0,'new_persistent_bytes':0},
        'validation':{'source_checks':checks,'swept_pixel_samples':swept,'raw_unique_tiles':len(raw),'flip_canonical_unique_tiles':len(canonical),
            'source_raw_ceiling':320,'source_flip_target':240,'native_car_half_pixels':5,'conservative_car_half_pixels':7,'conservative_foot_half_pixels':5,
            'all_five_verified_rail_crossings':True,'underpass_full_half5_foot_offsets':[-18,18],'outer_walkway_corner_widths_not_claimed':True,'rail_outside_crossings_solid':True,'stairs_and_austin_gap_foot_only':True,
            'traffic_loops':6,'fleet_body_halves':list(FLEET_HALVES),'traffic_conservative_half8_sweeps':True,'parallel_opposing_fleet_bodies_disjoint':True,
            'seam_swept_y':[944,968],'car_half5_offsets':[-18,18],'car_half7_offsets':[-14,14],'foot_half3_offsets':[-28,28],'foot_half5_offsets':[-24,24],
            'fixed_pedestrian_routes':len(peds),'connected_half5_foot_grid_positions':len(foot),'connected_half7_car_grid_positions':len(car),
            'all_client_foot_and_car_or_park_paths_connected':True,'all_private_grounds_solid':True,'no_blank_public_grass_priority':True,
            'native_registration_verified':False,'native_build_verified':False,'native_play_verified':False,
            'compiled_background_bank1_gate_verified':False,'compiled_sprite_budget_verified':False,'hardware_verified':False,'measured_duration_verified':False}}
    return png,attrs,metadata


def same_png(a,b):
    with Image.open(io.BytesIO(a)) as x,Image.open(io.BytesIO(b)) as y:
        return x.format==y.format=='PNG' and x.mode==y.mode=='RGB' and x.size==y.size and x.info==y.info and x.tobytes()==y.tobytes()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    modes=parser.add_mutually_exclusive_group();modes.add_argument('--check',action='store_true');modes.add_argument('--dry-run',action='store_true')
    args=parser.parse_args();png,attrs,metadata=render()
    if args.check:
        actual=(ART/'toronto_north.png').read_bytes();assert same_png(png,actual),'North source pixels/mode/dimensions differ'
        png=actual;metadata['background_sha256']=hashlib.sha256(actual).hexdigest()
    if not args.dry_run:
        files={ART/'toronto_north.png':png,ART/'north_attributes.json':(json.dumps(attrs)+'\n').encode(),
               CONTENT/'north_art.json':(json.dumps(metadata,indent=2)+'\n').encode()}
        for path,data in files.items():
            if args.check:assert path.read_bytes()==data,('Stale source candidate',path)
            else:path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    v=metadata['validation'];mode='matches' if args.check else 'validated in memory' if args.dry_run else 'generated'
    print(f"North source {mode}: {len(metadata['blocks'])} buildings; {v['raw_unique_tiles']} raw/{v['flip_canonical_unique_tiles']} flipped patterns; {v['fixed_pedestrian_routes']} full-foot routes; {v['source_checks']} checks/{v['swept_pixel_samples']} swept samples.Native compiled allocation, play and hardware require separate verification.")


if __name__=='__main__':main()
