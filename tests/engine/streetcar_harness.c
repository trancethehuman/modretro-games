/* Actual production APIs are linked as separate, unchanged translation units.
 * The oracle uses a stitched global corridor and wide host arithmetic, rather
 * than reading the production path table or repeating its16-bit quotient math. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "td_streetcar.h"
#include "td_transit.h"
#include "td_district.h"
#include "native_streetcar_fixture.h"

/* These host-only wrappers expose actual private C, not a replacement lookup.
 * No generated interpolation values are included in this oracle's inputs. */
UWORD host_streetcar_progress(UBYTE route,UWORD tick);
unsigned host_streetcar_progress_value_bytes(void);
unsigned host_streetcar_progress_route_count(void);

static unsigned long checks,failures;
static unsigned char occupied[HOST_DISTRICTS][976][1024];

static void expect(unsigned actual,unsigned expected,const char *name,unsigned clock,unsigned index){
    checks++;
    if(actual==expected)return;
    if(failures++<16)fprintf(stderr,"%s clock=%u index=%u got%u expected%u\n",name,clock,index,actual,expected);
}
static void pose_equal(const td_streetcar_pose_t *a,const td_streetcar_pose_t *b,unsigned clock,const char *name){
    expect(a->u,b->u,name,clock,0);expect(a->v,b->v,name,clock,1);
    expect(a->district,b->district,name,clock,2);expect(a->heading,b->heading,name,clock,3);
    expect(a->frame,b->frame,name,clock,4);expect(a->stop,b->stop,name,clock,5);
    expect(a->direction,b->direction,name,clock,6);expect(a->doors,b->doors,name,clock,7);
}
static void oracle_dwell(unsigned index,unsigned direction,td_streetcar_pose_t *pose){
    pose->u=host_platforms[index].u*16;
    pose->v=(host_platforms[index].v-(direction?36:20))*16;
    pose->district=host_platforms[index].district;
    pose->direction=direction;pose->heading=direction?8:0;pose->frame=direction?6:4;
    pose->doors=1;pose->stop=43+index;
}
static void oracle_corridor(unsigned direction,unsigned distance,td_streetcar_pose_t *pose){
    pose->direction=direction;pose->stop=255;pose->doors=0;
    if(!direction){
        if(distance<=164*16){pose->district=1;pose->u=836*16+distance;pose->v=536*16;pose->heading=0;}
        else if(distance<=1140*16){pose->district=0;pose->u=24*16+distance-164*16;pose->v=536*16;pose->heading=0;}
        else if(distance<=1796*16){pose->district=3;pose->u=24*16+distance-1140*16;pose->v=536*16;pose->heading=0;}
        else if(distance<=1828*16){pose->district=3;pose->u=680*16;pose->v=536*16-(distance-1796*16);pose->heading=12;}
        else{pose->district=3;pose->u=680*16+distance-1828*16;pose->v=504*16;pose->heading=0;}
    }else{
        if(distance<=216*16){pose->district=3;pose->u=880*16-distance;pose->v=488*16;pose->heading=8;}
        else if(distance<=248*16){pose->district=3;pose->u=664*16;pose->v=488*16+distance-216*16;pose->heading=4;}
        else if(distance<=888*16){pose->district=3;pose->u=664*16-(distance-248*16);pose->v=520*16;pose->heading=8;}
        else if(distance<=1864*16){pose->district=0;pose->u=1000*16-(distance-888*16);pose->v=520*16;pose->heading=8;}
        else{pose->district=1;pose->u=1000*16-(distance-1864*16);pose->v=520*16;pose->heading=8;}
    }
    pose->frame=pose->heading/4;
}
static void oracle_pose(unsigned tick,td_streetcar_pose_t *pose){
    static const unsigned stations[2][8]={{0,256,512,816,1120,1244,1928,2028},{0,100,784,908,1212,1516,1772,2028}};
    unsigned direction=tick>=7680,local=tick%7680,leg=local/960,phase=local%960;
    if(phase<240){oracle_dwell(direction?7-leg:leg,direction,pose);return;}
    if(leg==7){
        oracle_dwell(direction?0:7,direction,pose);
        pose->doors=0;pose->stop=255;pose->heading=direction?4:12;pose->frame=direction?1:3;
        if(direction)pose->v+=256*(phase-240)/720;else pose->v-=256*(phase-240)/720;
    }else{
        unsigned distance=stations[direction][leg]*16+
            (unsigned)((uint64_t)(stations[direction][leg+1]-stations[direction][leg])*16*(phase-240)/720);
        oracle_corridor(direction,distance,pose);
    }
}
static td_streetcar_box_t point(unsigned district,unsigned u,unsigned v){
    td_streetcar_box_t value={(UWORD)(u*16),(UWORD)(v*16),(UWORD)(u*16),(UWORD)(v*16),(UBYTE)district};
    return value;
}
static unsigned oracle_inside(const td_streetcar_pose_t *pose,const td_streetcar_box_t *box){
    unsigned hu=pose->heading==0||pose->heading==8?224:96,hv=pose->heading==0||pose->heading==8?96:224;
    return pose->district==box->district&&pose->u-hu<=box->right&&pose->u+hu-1>=box->left&&
        pose->v-hv<=box->bottom&&pose->v+hv-1>=box->top;
}
static void check_terrain(const td_streetcar_pose_t *pose,unsigned tick){
    td_streetcar_box_t box;unsigned x,y,hu=pose->heading==0||pose->heading==8?224:96;
    expect(td_streetcar_bounds(pose,&box),1,"body valid",tick,0);
    expect(box.right-box.left+1,hu*2,"body width",tick,0);
    expect(box.bottom-box.top+1,hu==224?192:448,"body height",tick,0);
    for(y=box.top/128;y<=box.bottom/128;y++)for(x=box.left/128;x<=box.right/128;x++)
        expect(host_collision[pose->district][y*128+x],0,"actual native ground",tick,y*128+x);
}
static void test_clock_and_poses(void){
    unsigned tick,direction,index,sub;td_streetcar_pose_t pose,expected,edge;td_streetcar_box_t box;
    for(tick=0;tick<15360;tick++){
        expect(td_streetcar_pose(tick/60,tick%60,&pose),1,"pose query",tick,0);
        oracle_pose(tick,&expected);pose_equal(&pose,&expected,tick,"global corridor oracle");
        check_terrain(&pose,tick);
        expect(pose.frame<8,1,"asset frame range",tick,0);
        box=point(pose.district,pose.u/16,pose.v/16);
        expect(td_streetcar_sweep(tick/60,tick%60,0,&box),1,"static centre hit",tick,0);
        box=point(2,pose.u/16,pose.v/16);
        expect(td_streetcar_sweep(tick/60,tick%60,0,&box),0,"other district clear",tick,0);
        expect(td_streetcar_pose(65280+tick/60,tick%60,&edge),1,"high clock query",tick,0);
        pose_equal(&edge,&pose,tick,"high clock same phase");
        if(pose.doors&&((!pose.direction&&pose.stop<50)||(pose.direction&&pose.stop>43))){
            expect(td_transit_departure(pose.stop,pose.direction?(pose.stop>43?43:pose.stop):(pose.stop<50?50:pose.stop),tick/60),0,
                   "doors match actual timetable",tick,pose.stop);
        }
    }
    for(direction=0;direction<2;direction++)for(index=0;index<8;index++)for(sub=0;sub<240;sub++){
        unsigned second=direction?128+(7-index)*16:index*16;
        expect(td_streetcar_pose(second+sub/60,sub%60,&pose),1,"platform pose",second,index);
        expect(pose.u,host_platforms[index].u*16,"authored platform longitude",second,index);
        expect(pose.v,(host_platforms[index].v-(direction?36:20))*16,"shared south platform offset",second,index);
        expect(pose.district,host_platforms[index].district,"authored platform district",second,index);
        expect(pose.stop,43+index,"doors at booked stop",second,index);
    }
    expect(td_streetcar_pose(65535,59,&pose),1,"clock before rollover",0,0);
    expect(td_streetcar_pose(0,0,&edge),1,"clock after rollover",0,0);
    expect(pose.district,edge.district,"rollover same west endpoint",0,0);
    expect(pose.u,edge.u,"rollover longitude continuous",0,0);
    expect(edge.v-pose.v<=3,1,"rollover subpixel continuity",0,0);
    /* The last stop in each direction opens for alighting, not a nonexistent
     * outbound destination. Return service starts after the lane turnaround. */
    expect(td_streetcar_pose(112,0,&pose),1,"east terminal arrival",0,0);
    expect(pose.stop,50,"east terminal doors",0,0);
    expect(td_transit_departure(50,49,112),16,"east terminal return wait",0,0);
    expect(td_streetcar_pose(240,0,&pose),1,"west terminal arrival",0,0);
    expect(pose.stop,43,"west terminal doors",0,0);
    expect(td_transit_departure(43,44,240),16,"west terminal return wait",0,0);
}
static void test_exact_interpolation_resources(void){
    /* Whole-pixel distances from the independently stitched corridor above.
     * Seams contribute no length; the two terminal lane changes are16px.
     * This wide product is deliberately different from both the old16-bit
     * quotient decomposition and the new generated table implementation. */
    static const unsigned pixels[16]={256,256,304,304,124,684,100,100,
                                      684,124,304,304,256,256,16,16};
    for(unsigned route=0;route<16;route++)for(unsigned tick=0;tick<=720;tick++){
        unsigned expected=(unsigned)((uint64_t)pixels[route]*16*tick/720);
        expect(host_streetcar_progress(route,tick),expected,
               "actual private progress versus independent wide corridor interpolation",tick,route);
    }
    expect(host_streetcar_progress_value_bytes(),8652,"six actual UWORD table resources",0,0);
    expect(host_streetcar_progress_route_count(),16,"actual same-bank route pointer count",0,0);
    expect(host_streetcar_progress_value_bytes()+host_streetcar_progress_route_count()*2,8684,
           "native table budget models guarded two-byte pointers rather than host pointer size",0,0);
}

static void test_section_endpoint_sweeps(void){
    static const int offsets[]={-225,-224,-223,-97,-96,-95,0,95,96,97,223,224,225};
    td_streetcar_pose_t last,arrival;td_streetcar_box_t obstacle;
    for(unsigned section=0;section<16;section++){
        unsigned clock=(section+1)*960;
        oracle_pose(clock-1,&last);oracle_pose(clock%15360,&arrival);
        unsigned hu=last.heading==0||last.heading==8?224:96;
        unsigned hv=hu==224?96:224;
        /* All authored final edges are longer than a last-tick advance. Thus
         * the final interval is one straight closed-body union plus the new
         * dwell body. Terminal turns need both differently oriented bodies;
         * a single enclosing rectangle would incorrectly fill their corners. */
        unsigned left=(last.u<arrival.u?last.u:arrival.u)-hu;
        unsigned right=(last.u>arrival.u?last.u:arrival.u)+hu-1;
        unsigned top=(last.v<arrival.v?last.v:arrival.v)-hv;
        unsigned bottom=(last.v>arrival.v?last.v:arrival.v)+hv-1;
        expect(last.district,arrival.district,"section endpoint remains in its actual local scene",clock,section);
        for(unsigned x=0;x<sizeof(offsets)/sizeof(offsets[0]);x++)
            for(unsigned y=0;y<sizeof(offsets)/sizeof(offsets[0]);y++){
                obstacle.left=obstacle.right=(UWORD)((int)arrival.u+offsets[x]);
                obstacle.top=obstacle.bottom=(UWORD)((int)arrival.v+offsets[y]);
                obstacle.district=arrival.district;
                unsigned dwell=oracle_inside(&arrival,&obstacle);
                unsigned previous=obstacle.left>=left&&obstacle.left<=right&&
                                  obstacle.top>=top&&obstacle.top<=bottom;
                expect(td_streetcar_sweep(clock/60,clock%60,0,&obstacle),dwell,
                       "exact Q4 section boundary has only the new static dwell body",clock,x*13+y);
                expect(td_streetcar_sweep(clock/60,clock%60,1,&obstacle),dwell||previous,
                       "last travel tick includes exact endpoint and new dwell without filling terminal corners",clock,x*13+y);
            }
    }
}
static void test_riding(void){
    unsigned origin,target,tick,left;td_streetcar_pose_t pose,expected,prior;
    for(origin=43;origin<=50;origin++)for(target=43;target<=50;target++){
        if(origin==target)continue;
        unsigned reverse=origin>target,first=(reverse?128+(50-origin)*16:(origin-43)*16)*60;
        unsigned duration=(origin>target?origin-target:target-origin)*16;
        expect(td_transit_duration(origin,target),duration,"slower actual duration",first,origin);
        expect(td_transit_fare(origin),3,"unchanged actual fare",first,origin);
        for(tick=0;tick<15360;tick++)for(left=1;left<=2;left++){
            unsigned progress=first+(duration-left)*60;
            unsigned accepted=tick>=progress&&tick<progress+240;
            memset(&pose,0xA5,sizeof(pose));prior=pose;
            expect(td_streetcar_ride(origin,target,left,tick/60,tick%60,&pose),accepted||left==1,"ride interval/retry",tick,origin*64+target);
            if(accepted){oracle_pose(tick,&expected);pose_equal(&pose,&expected,tick,"ride follows clock");}
            else if(left==1){oracle_dwell(target-43,reverse,&expected);pose_equal(&pose,&expected,tick,"retry holds target doors");}
            else expect(memcmp(&pose,&prior,sizeof(pose)),0,"invalid phase output unchanged",tick,origin);
        }
        expect(td_streetcar_ride(origin,target,112,first/60,0,&pose),duration==112,"remaining time fits actual trip",first,origin);
        expect(td_streetcar_ride(origin,target,1,65535,59,&pose),1,"restored retry rollover",0,origin);
        oracle_dwell(target-43,reverse,&expected);pose_equal(&pose,&expected,0,"restored destination pose");
    }
}
static void test_destination_and_real_ride_history(void){
    unsigned origin,target,offset,progress,cycle;td_streetcar_pose_t pose,expected,prior;
    for(origin=0;origin<256;origin++)for(target=0;target<256;target++){
        unsigned valid=origin>=43&&origin<=50&&target>=43&&target<=50&&origin!=target;
        memset(&pose,0xA5,sizeof(pose));prior=pose;
        expect(td_streetcar_destination(origin,target,&pose),valid,"pure destination membership",0,origin*256+target);
        if(!valid){expect(memcmp(&pose,&prior,sizeof(pose)),0,"destination failure unchanged",0,origin*256+target);continue;}
        oracle_dwell(target-43,origin>target,&expected);pose_equal(&pose,&expected,0,"pure directional doors");
        unsigned first=origin>target?128+(50-origin)*16:(origin-43)*16;
        unsigned duration=(origin>target?origin-target:target-origin)*16;
        for(offset=0;offset<4;offset++)for(progress=0;progress<duration;progress++){
            unsigned seconds=first+offset+progress,left=duration-progress;
            expect(td_streetcar_ride(origin,target,left,seconds,59,&pose),1,"actual boarded countdown history",seconds,origin);
            oracle_pose(seconds*60+59,&expected);pose_equal(&pose,&expected,seconds,"normal ride follows unit");
        }
        /* Geometry remains independent of a runtime's persisted held flag.
         * Calling the explicit destination API after arrival keeps the same
         * target at any future cycle, including the original final approach
         * phase that cannot be distinguished by the old58-byte state alone. */
        oracle_dwell(target-43,origin>target,&expected);
        for(cycle=0;cycle<8;cycle++){
            expect(td_streetcar_destination(origin,target,&pose),1,"held destination repeated cycles",cycle,origin);
            pose_equal(&pose,&expected,cycle,"held destination never rewinds");
            expect(td_streetcar_ride(origin,target,1,first+cycle*256,0,&pose),1,"retry at origin cycle phase",cycle,origin);
            pose_equal(&pose,&expected,cycle,"narrow retry pins target");
        }
    }
    expect(td_streetcar_destination(43,50,NULL),0,"null destination",0,0);
}
static void oracle_mark_line(unsigned district,int x1,int y1,int x2,int y2){
    int x,y,hu=y1==y2?14:6,hv=y1==y2?6:14;
    int left=(x1<x2?x1:x2)-hu,right=(x1>x2?x1:x2)+hu-1;
    int top=(y1<y2?y1:y2)-hv,bottom=(y1>y2?y1:y2)+hv-1;
    for(y=top;y<=bottom;y++)for(x=left;x<=right;x++){
        occupied[district][y][x]=1;
        expect(host_collision[district][(y/8)*128+x/8],0,"full path ground",y*1024+x,district);
    }
}
static void test_full_corridor(void){
    unsigned district,x,y,n;td_streetcar_box_t box;
    /* These corridor lines are authored independently of the production stop
     * subdivisions, transit time or seam path arrays. */
    oracle_mark_line(1,836,536,1000,536);oracle_mark_line(1,836,520,1000,520);
    oracle_mark_line(1,836,520,836,536);
    oracle_mark_line(0,24,536,1000,536);oracle_mark_line(0,24,520,1000,520);
    oracle_mark_line(3,24,536,680,536);oracle_mark_line(3,680,536,680,504);
    oracle_mark_line(3,680,504,880,504);oracle_mark_line(3,24,520,664,520);
    oracle_mark_line(3,664,520,664,488);oracle_mark_line(3,664,488,880,488);
    oracle_mark_line(3,880,488,880,504);
    for(district=0;district<HOST_DISTRICTS;district++)for(y=464;y<=560;y++)for(x=0;x<1024;x++){
        box=point(district,x,y);
        expect(td_streetcar_sweep(0,0,15360,&box),occupied[district][y][x],"axis exact full corridor",y*1024+x,district);
        if(x%19==0&&y%7==0){
            expect(td_streetcar_sweep(65535,59,65535,&box),occupied[district][y][x],"bounded long catchup",y*1024+x,district);
            expect(td_streetcar_sweep(31,47,15361,&box),occupied[district][y][x],"full corridor phase invariant",y*1024+x,district);
        }
    }
    for(district=0;district<HOST_DISTRICTS;district++)for(y=0;y<976;y+=31)for(x=0;x<1024;x+=29){
        box=point(district,x,y);
        expect(td_streetcar_sweep(63,59,15360,&box),occupied[district][y][x],"outside corridor",y*1024+x,district);
    }
    /* Car-sized and long crossing rectangles require any covered cell, not
     * just a centre/corner check. Their oracle reads the independent raster. */
    for(n=0;n<640;n++){
        unsigned right,bottom,expected=0;
        district=n%HOST_DISTRICTS;x=32+(n*73)%920;y=466+(n*31)%86;
        right=x+(n%3?10:37);bottom=y+(n%5?10:29);
        for(unsigned sy=y;sy<=bottom;sy++)for(unsigned sx=x;sx<=right;sx++)
            if(occupied[district][sy][sx])expected=1;
        box.left=x*16;box.right=right*16+15;box.top=y*16;box.bottom=bottom*16+15;box.district=district;
        expect(td_streetcar_sweep(17,41,15360,&box),expected,"whole obstacle rectangle",n,district);
    }
}
static void test_temporal_sweeps(void){
    td_streetcar_box_t box;td_streetcar_pose_t pose;
    unsigned tick,back,sample,j;static const unsigned elapsed[]={1,2,4,7,60,121,239,700,15359};
    box=point(3,145,536);
    expect(td_streetcar_sweep(84,0,0,&box),0,"jump start misses",0,0);
    expect(td_streetcar_sweep(84,42,0,&box),0,"jump end misses",0,0);
    expect(td_streetcar_sweep(84,42,42,&box),1,"full elapsed jump hits",0,0);
    box=point(3,400,504);
    expect(td_streetcar_sweep(95,59,719,&box),0,"bend does not fill interior",0,0);
    box=point(1,512,536);
    expect(td_streetcar_sweep(13,0,540,&box),0,"seam never bridges west coordinates",0,0);
    box=point(0,512,536);
    expect(td_streetcar_sweep(13,0,540,&box),0,"seam never bridges core coordinates",0,0);
    box=point(0,40,536);
    expect(td_streetcar_sweep(13,0,540,&box),1,"seam reaches actual core entrance",0,0);
    box=point(1,836,536);
    expect(td_streetcar_sweep(0,0,1,&box),1,"rollover includes endpoint",0,0);
    box=point(0,130,536);
    expect(td_streetcar_sweep(16,0,0,&box),0,"body exclusive right boundary",0,0);
    box.left=box.right=130*16-1;
    expect(td_streetcar_sweep(16,0,0,&box),1,"body final fractional unit",0,0);
    box.left=130*16;box.right=140*16+15;box.top=531*16;box.bottom=541*16+15;
    expect(td_streetcar_sweep(16,0,0,&box),0,"eleven pixel car clear edge",0,0);
    box.left--;
    expect(td_streetcar_sweep(16,0,0,&box),1,"eleven pixel car touching edge",0,0);
    /* Every independently sampled body point in an elapsed interval must be
     * reported. This proves lower-bound sweep coverage without pretending a
     * per-tick oracle captures unsampled corner motion. Static checks below
     * prove exact current-body bounds, not merely the centre. */
    for(tick=0;tick<15360;tick+=17)for(j=0;j<sizeof(elapsed)/sizeof(elapsed[0]);j++){
        back=elapsed[j];
        for(sample=0;sample<=back;sample+=back/4+1){
            unsigned phase=(tick+15360-sample)%15360;
            oracle_pose(phase,&pose);box=point(pose.district,pose.u/16,pose.v/16);
            expect(td_streetcar_sweep(tick/60,tick%60,back,&box),1,"elapsed includes intermediate body",tick,sample);
        }
    }
    for(tick=0;tick<15360;tick+=7){
        int x,y;
        oracle_pose(tick,&pose);
        for(y=-16;y<=16;y++)for(x=-16;x<=16;x++){
            box=point(pose.district,pose.u/16+x,pose.v/16+y);
            expect(td_streetcar_sweep(tick/60,tick%60,0,&box),oracle_inside(&pose,&box),"exact stationary body",tick,(y+16)*33+x+16);
        }
    }
}
static void test_invalid_queries(void){
    td_streetcar_pose_t pose,old;td_streetcar_box_t box,prior;unsigned value;
    memset(&pose,0xA5,sizeof(pose));old=pose;
    expect(td_streetcar_pose(0,60,&pose),0,"subsecond invalid",0,0);
    expect(memcmp(&pose,&old,sizeof(pose)),0,"invalid pose unchanged",0,0);
    expect(td_streetcar_pose(0,0,NULL),0,"null pose",0,0);
    for(value=0;value<256;value++){
        if(value>=43&&value<=50)continue;
        expect(td_streetcar_ride(value,50,1,0,0,&pose),0,"invalid origin",0,value);
        expect(td_streetcar_ride(43,value,1,0,0,&pose),0,"invalid target",0,value);
        expect(memcmp(&pose,&old,sizeof(pose)),0,"invalid ride unchanged",0,value);
    }
    expect(td_streetcar_ride(43,43,1,0,0,&pose),0,"self ride invalid",0,0);
    expect(td_streetcar_ride(43,50,0,0,0,&pose),0,"zero ride invalid",0,0);
    expect(td_streetcar_ride(43,50,29,0,0,&pose),0,"long ride invalid",0,0);
    expect(td_streetcar_ride(43,50,1,0,60,&pose),0,"invalid ride subsecond",0,0);
    expect(td_streetcar_ride(43,50,1,0,0,NULL),0,"null ride",0,0);
    box=point(0,500,500);prior=box;
    expect(td_streetcar_bounds(NULL,&box),0,"null bounds input",0,0);
    expect(memcmp(&box,&prior,sizeof(box)),0,"bounds failure unchanged",0,0);
    td_streetcar_pose(0,0,&pose);
    expect(td_streetcar_bounds(&pose,NULL),0,"null bounds output",0,0);
    pose.heading=1;expect(td_streetcar_bounds(&pose,&box),0,"diagonal unsupported",0,0);
    pose.heading=0;pose.u=65535;expect(td_streetcar_bounds(&pose,&box),0,"bounds overflow rejected",0,0);
    pose.u=0;expect(td_streetcar_bounds(&pose,&box),0,"bounds underflow rejected",0,0);
    pose.u=836*16;pose.district=2;expect(td_streetcar_bounds(&pose,&box),0,"unserved pose district",0,0);
    expect(memcmp(&box,&prior,sizeof(box)),0,"all bounds failures unchanged",0,0);
    pose.district=0;pose.u=1024*16-224;pose.v=976*16-96;
    expect(td_streetcar_bounds(&pose,&box),1,"body exact world boundary",0,0);
    expect(box.right,1024*16-1,"body last horizontal unit",0,0);
    expect(box.bottom,976*16-1,"body last vertical unit",0,0);
    prior=box;pose.u++;
    expect(td_streetcar_bounds(&pose,&box),0,"body one unit beyond boundary",0,0);
    expect(memcmp(&box,&prior,sizeof(box)),0,"boundary failure unchanged",0,0);
    expect(td_streetcar_sweep(0,0,1,NULL),255,"null sweep",0,0);
    expect(td_streetcar_sweep(0,60,1,&box),255,"invalid sweep clock",0,0);
    box.district=255;expect(td_streetcar_sweep(0,0,1,&box),255,"unknown box district",0,0);
    box=point(0,1024,500);expect(td_streetcar_sweep(0,0,1,&box),255,"box right out of bounds",0,0);
    box=point(0,500,976);expect(td_streetcar_sweep(0,0,1,&box),255,"box bottom out of bounds",0,0);
    box=point(0,500,500);box.left++;
    expect(td_streetcar_sweep(0,0,1,&box),255,"reversed horizontal box",0,0);
    box=point(0,500,500);box.top++;
    expect(td_streetcar_sweep(0,0,1,&box),255,"reversed vertical box",0,0);
}
static void test_lookup_public_validation_boundaries(void){
    static const UWORD clocks[]={0,1,2,3,4,31,32,63,65535};
    td_streetcar_pose_t pose,prior;td_streetcar_box_t box,box_prior;
    box=point(0,500,536);box_prior=box;
    for(unsigned i=0;i<sizeof(clocks)/sizeof(clocks[0]);i++)for(unsigned sub=60;sub<256;sub++){
        memset(&pose,0xA5,sizeof(pose));prior=pose;
        expect(td_streetcar_pose(clocks[i],sub,&pose),0,"every malformed subsecond is rejected before private table addressing",clocks[i],sub);
        expect(td_streetcar_ride(43,50,1,clocks[i],sub,&pose),0,"booked retry validates every malformed subsecond before lookup",clocks[i],sub);
        expect(memcmp(&pose,&prior,sizeof(pose)),0,"all malformed clock queries preserve every pose output byte",clocks[i],sub);
        expect(td_streetcar_sweep(clocks[i],sub,1,&box),255,"sweep validates every malformed subsecond before interpolation",clocks[i],sub);
        expect(memcmp(&box,&box_prior,sizeof(box)),0,"invalid sweep preserves its caller-owned obstacle",clocks[i],sub);
    }
    for(unsigned heading=0;heading<256;heading++){
        pose=(td_streetcar_pose_t){500*16,500*16,0,heading,255,255,255,255};
        memset(&box,0xA5,sizeof(box));box_prior=box;
        unsigned valid=heading==0||heading==4||heading==8||heading==12;
        expect(td_streetcar_bounds(&pose,&box),valid,"all encoded headings retain cardinal body validation",0,heading);
        if(!valid)expect(memcmp(&box,&box_prior,sizeof(box)),0,"invalid heading leaves bounds output unchanged",0,heading);
        else{
            unsigned hu=heading==0||heading==8?224:96,hv=hu==224?96:224;
            expect(box.right-box.left+1,hu*2,"valid body ignores non-spatial pose fields",0,heading);
            expect(box.bottom-box.top+1,hv*2,"valid cardinal body keeps independent height",0,heading);
        }
    }
    for(unsigned origin=43;origin<=50;origin++)for(unsigned target=43;target<=50;target++){
        if(origin==target)continue;
        unsigned duration=(origin>target?origin-target:target-origin)*16;
        static const unsigned malformed_left[]={0,113,255};
        for(unsigned i=0;i<sizeof(malformed_left)/sizeof(malformed_left[0]);i++){
            memset(&pose,0xA5,sizeof(pose));prior=pose;
            expect(td_streetcar_ride(origin,target,malformed_left[i],65535,59,&pose),0,
                   "all directional bookings reject zero or oversized remaining time before interpolation",malformed_left[i],origin*64+target);
            expect(memcmp(&pose,&prior,sizeof(pose)),0,"bad remaining-time query preserves booked output",malformed_left[i],origin*64+target);
        }
        memset(&pose,0xA5,sizeof(pose));prior=pose;
        expect(td_streetcar_ride(origin,target,duration+1,65535,59,&pose),0,
               "remaining time exceeding the actual booked distance is rejected even inside the global112-second bound",duration,origin*64+target);
        expect(memcmp(&pose,&prior,sizeof(pose)),0,"distance-inconsistent booking leaves outputs unchanged",duration,origin*64+target);
    }
}
int main(void){
    test_exact_interpolation_resources();test_clock_and_poses();test_riding();test_destination_and_real_ride_history();
    test_full_corridor();test_temporal_sweeps();test_invalid_queries();
    test_section_endpoint_sweeps();test_lookup_public_validation_boundaries();
    printf("Queen streetcar actual-C regressions: %lu checks, %lu failures\n",checks,failures);
    return failures?1:0;
}
