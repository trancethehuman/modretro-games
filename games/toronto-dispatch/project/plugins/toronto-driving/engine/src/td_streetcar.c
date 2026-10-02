#pragma bank 255
#include "td_streetcar.h"
#include "td_transit.h"
#include "td_district.h"

typedef char td_streetcar_schedule_is_registered[
    (TD_TRANSIT_QUEEN_FIRST==43 && TD_TRANSIT_QUEEN_COUNT==8 &&
     TD_TRANSIT_QUEEN_PERIOD==64 && TD_TRANSIT_QUEEN_HOP_SECONDS==4 &&
     TD_STREETCAR_PERIOD_TICKS==TD_TRANSIT_QUEEN_PERIOD*60 &&
     TD_DISTRICT_PIXEL_WIDTH==1024 && TD_DISTRICT_PIXEL_HEIGHT==976) ? 1 : -1];
typedef struct { UWORD u,v; UBYTE district; } td_streetcar_point_t;

/* A seam has zero travel length: the existing legal scene entrance maps
 * local1000 to local24. Each other edge is cardinal. Opposite Queen lanes
 * remain eight pixels from the centreline, including East's offset miter. */
static const td_streetcar_point_t td_streetcar_paths[16][4]={
    {{836,536,TD_DISTRICT_WEST},{1000,536,TD_DISTRICT_WEST},
     {24,536,TD_DISTRICT_CITY},{116,536,TD_DISTRICT_CITY}},
    {{116,536,TD_DISTRICT_CITY},{372,536,TD_DISTRICT_CITY}},
    {{372,536,TD_DISTRICT_CITY},{676,536,TD_DISTRICT_CITY}},
    {{676,536,TD_DISTRICT_CITY},{980,536,TD_DISTRICT_CITY}},
    {{980,536,TD_DISTRICT_CITY},{1000,536,TD_DISTRICT_CITY},
     {24,536,TD_DISTRICT_EAST},{128,536,TD_DISTRICT_EAST}},
    {{128,536,TD_DISTRICT_EAST},{680,536,TD_DISTRICT_EAST},
     {680,504,TD_DISTRICT_EAST},{780,504,TD_DISTRICT_EAST}},
    {{780,504,TD_DISTRICT_EAST},{880,504,TD_DISTRICT_EAST}},
    {{880,488,TD_DISTRICT_EAST},{780,488,TD_DISTRICT_EAST}},
    {{780,488,TD_DISTRICT_EAST},{664,488,TD_DISTRICT_EAST},
     {664,520,TD_DISTRICT_EAST},{128,520,TD_DISTRICT_EAST}},
    {{128,520,TD_DISTRICT_EAST},{24,520,TD_DISTRICT_EAST},
     {1000,520,TD_DISTRICT_CITY},{980,520,TD_DISTRICT_CITY}},
    {{980,520,TD_DISTRICT_CITY},{676,520,TD_DISTRICT_CITY}},
    {{676,520,TD_DISTRICT_CITY},{372,520,TD_DISTRICT_CITY}},
    {{372,520,TD_DISTRICT_CITY},{116,520,TD_DISTRICT_CITY}},
    {{116,520,TD_DISTRICT_CITY},{24,520,TD_DISTRICT_CITY},
     {1000,520,TD_DISTRICT_WEST},{836,520,TD_DISTRICT_WEST}},
    {{880,504,TD_DISTRICT_EAST},{880,488,TD_DISTRICT_EAST}},
    {{836,520,TD_DISTRICT_WEST},{836,536,TD_DISTRICT_WEST}}
};
static const UBYTE td_streetcar_path_counts[16]={4,2,2,2,4,4,2,2,4,4,2,2,2,4,2,2};
static const UWORD td_streetcar_stop_u[8]={836,116,372,676,980,128,780,880};
static const UBYTE td_streetcar_stop_district[8]={
    TD_DISTRICT_WEST,TD_DISTRICT_CITY,TD_DISTRICT_CITY,TD_DISTRICT_CITY,
    TD_DISTRICT_CITY,TD_DISTRICT_EAST,TD_DISTRICT_EAST,TD_DISTRICT_EAST
};

static UWORD td_streetcar_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static UWORD td_streetcar_clock(UWORD seconds,UBYTE subsecond){
    return (seconds&63)*60+subsecond;
}
static UBYTE td_streetcar_route(UBYTE section){
    return section<7?section:section==7?14:section<15?section-1:15;
}
static UWORD td_streetcar_edge(const td_streetcar_point_t *a,
                              const td_streetcar_point_t *b){
    if(a->district!=b->district)return 0;
    return (td_streetcar_distance(a->u,b->u)+td_streetcar_distance(a->v,b->v))<<4;
}
static UWORD td_streetcar_length(UBYTE route){
    UBYTE i;UWORD total=0;
    for(i=1;i<td_streetcar_path_counts[route];i++)
        total+=td_streetcar_edge(&td_streetcar_paths[route][i-1],&td_streetcar_paths[route][i]);
    return total;
}
static UWORD td_streetcar_progress(UWORD length,UBYTE tick){
    /* Largest length is10944Q4; each product fits16 bits. No long arithmetic
     * or additional fixed-bank runtime helpers are needed. Tick is0..120. */
    return (length/120)*tick+((length%120)*tick)/120;
}
static UBYTE td_streetcar_heading(const td_streetcar_point_t *a,
                                 const td_streetcar_point_t *b){
    return a->u!=b->u?(a->u<b->u?0:8):(a->v<b->v?4:12);
}
static void td_streetcar_dwell(UBYTE index,UBYTE direction,td_streetcar_pose_t *pose){
    pose->u=td_streetcar_stop_u[index]<<4;
    pose->v=(index<6?(direction?520:536):(direction?488:504))<<4;
    pose->district=td_streetcar_stop_district[index];
    pose->heading=direction?8:0;pose->frame=direction?6:4;
    pose->stop=TD_TRANSIT_QUEEN_FIRST+index;pose->direction=direction;pose->doors=1;
}
static void td_streetcar_edge_pose(const td_streetcar_point_t *a,
                                  const td_streetcar_point_t *b,UWORD distance,
                                  td_streetcar_pose_t *pose){
    pose->u=a->u<<4;pose->v=a->v<<4;pose->district=a->district;
    pose->heading=td_streetcar_heading(a,b);
    if(pose->heading==0)pose->u+=distance;
    else if(pose->heading==8)pose->u-=distance;
    else if(pose->heading==4)pose->v+=distance;
    else pose->v-=distance;
    pose->frame=pose->heading>>2;
    pose->stop=TD_TRANSIT_NONE;pose->doors=0;
}
static void td_streetcar_travel(UBYTE route,UBYTE tick,td_streetcar_pose_t *pose){
    UBYTE i;UWORD distance=td_streetcar_progress(td_streetcar_length(route),tick),edge;
    for(i=1;i<td_streetcar_path_counts[route];i++){
        edge=td_streetcar_edge(&td_streetcar_paths[route][i-1],&td_streetcar_paths[route][i]);
        if(!edge)continue;
        if(distance<=edge){
            td_streetcar_edge_pose(&td_streetcar_paths[route][i-1],&td_streetcar_paths[route][i],distance,pose);
            return;
        }
        distance-=edge;
    }
}
static void td_streetcar_pose_section(UBYTE section,UBYTE tick,td_streetcar_pose_t *pose){
    UBYTE direction=section>=8;
    if(tick<120)td_streetcar_dwell(direction?15-section:section,direction,pose);
    else{
        pose->direction=direction;
        td_streetcar_travel(td_streetcar_route(section),tick-120,pose);
    }
}
static void td_streetcar_pose_local(UWORD phase,td_streetcar_pose_t *pose){
    td_streetcar_pose_section(phase/240,phase%240,pose);
}
static UBYTE td_streetcar_bounds_local(const td_streetcar_pose_t *pose,td_streetcar_box_t *box){
    UWORD half_u,half_v;
    if((pose->heading&3)||pose->heading>12||
       (pose->district!=TD_DISTRICT_CITY&&pose->district!=TD_DISTRICT_WEST&&pose->district!=TD_DISTRICT_EAST))return FALSE;
    half_u=(pose->heading&4?6:14)*16;half_v=(pose->heading&4?14:6)*16;
    if(pose->u<half_u||pose->v<half_v||
       pose->u>TD_DISTRICT_PIXEL_WIDTH*16-half_u||
       pose->v>TD_DISTRICT_PIXEL_HEIGHT*16-half_v)return FALSE;
    box->left=pose->u-half_u;box->right=pose->u+half_u-1;
    box->top=pose->v-half_v;box->bottom=pose->v+half_v-1;
    box->district=pose->district;return TRUE;
}
static UBYTE td_streetcar_box_valid(const td_streetcar_box_t *box){
    return box->district<TD_DISTRICT_COUNT&&box->left<=box->right&&box->top<=box->bottom&&
        box->right<TD_DISTRICT_PIXEL_WIDTH*16&&box->bottom<TD_DISTRICT_PIXEL_HEIGHT*16;
}
static UBYTE td_streetcar_boxes_hit(const td_streetcar_box_t *a,const td_streetcar_box_t *b){
    return a->district==b->district&&a->left<=b->right&&a->right>=b->left&&
        a->top<=b->bottom&&a->bottom>=b->top;
}
static UBYTE td_streetcar_travel_hit(UBYTE route,UBYTE first,UBYTE last,
                                    const td_streetcar_box_t *box){
    UBYTE i,prior=0;UWORD length=td_streetcar_length(route),low,high,edge,start=0,a,b;
    td_streetcar_pose_t p,q;td_streetcar_box_t swept;
    low=td_streetcar_progress(length,first);high=td_streetcar_progress(length,last);
    for(i=1;i<td_streetcar_path_counts[route];i++){
        edge=td_streetcar_edge(&td_streetcar_paths[route][i-1],&td_streetcar_paths[route][i]);
        if(!edge)continue;
        /* At an exact shared endpoint the preceding edge owns the pose.
         * Never connect the old and new local coordinates across a seam. */
        if(low<=start+edge&&high>=start&&(!prior||high>start)&&
           box->district==td_streetcar_paths[route][i-1].district){
            a=low>start?low-start:0;b=high<start+edge?high-start:edge;
            td_streetcar_edge_pose(&td_streetcar_paths[route][i-1],&td_streetcar_paths[route][i],a,&p);
            td_streetcar_edge_pose(&td_streetcar_paths[route][i-1],&td_streetcar_paths[route][i],b,&q);
            td_streetcar_bounds_local(&p,&swept);
            if(q.u>p.u)swept.right+=q.u-p.u;else swept.left-=p.u-q.u;
            if(q.v>p.v)swept.bottom+=q.v-p.v;else swept.top-=p.v-q.v;
            if(td_streetcar_boxes_hit(&swept,box))return TRUE;
        }
        start+=edge;prior=1;
    }
    return FALSE;
}
static UBYTE td_streetcar_range_hit(UWORD first,UWORD last,const td_streetcar_box_t *box){
    UBYTE section=0,route,direction;UWORD base=0,a,b;td_streetcar_pose_t pose;td_streetcar_box_t body;
    /* A boundary can belong to the previous travel endpoint and the next
       dwell. Include both; a full-cycle query still visits all16 sections. */
    while(section<15&&base+240<first){section++;base+=240;}
    for(;section<16&&base<=last;section++,base+=240){
        direction=section>=8;
        if(first<=base+119&&last>=base){
            td_streetcar_dwell(direction?15-section:section,direction,&pose);
            td_streetcar_bounds_local(&pose,&body);
            if(td_streetcar_boxes_hit(&body,box))return TRUE;
        }
        if(first<=base+240&&last>=base+120){
            a=first>base+120?first-base-120:0;b=last<base+240?last-base-120:120;
            route=td_streetcar_route(section);
            if(td_streetcar_travel_hit(route,a,b,box))return TRUE;
        }
    }
    return FALSE;
}

UBYTE td_streetcar_pose(UWORD seconds,UBYTE subsecond,td_streetcar_pose_t *pose) BANKED {
    td_streetcar_pose_t value;
    if(!pose||subsecond>=60)return FALSE;
    td_streetcar_pose_section((seconds&63)>>2,(seconds&3)*60+subsecond,&value);
    *pose=value;return TRUE;
}
UBYTE td_streetcar_ride(UBYTE origin,UBYTE target,UBYTE ride_left,UWORD seconds,
                       UBYTE subsecond,td_streetcar_pose_t *pose) BANKED {
    UBYTE source,dest,direction,duration;UWORD phase,first,last;td_streetcar_pose_t value;
    if(!pose||subsecond>=60||!ride_left||ride_left>28||origin<TD_TRANSIT_QUEEN_FIRST||
       target<TD_TRANSIT_QUEEN_FIRST||origin>=TD_TRANSIT_QUEEN_FIRST+8||
       target>=TD_TRANSIT_QUEEN_FIRST+8||origin==target)return FALSE;
    source=origin-TD_TRANSIT_QUEEN_FIRST;dest=target-TD_TRANSIT_QUEEN_FIRST;direction=source>dest;
    duration=(source>dest?source-dest:dest-source)*4;
    if(ride_left>duration)return FALSE;
    first=((direction?32+(7-source)*4:source*4)+duration-ride_left)*60;
    last=first+120;
    phase=td_streetcar_clock(seconds,subsecond);
    if(phase>=first&&phase<last)td_streetcar_pose_section((seconds&63)>>2,(seconds&3)*60+subsecond,&value);
    else if(ride_left==1)td_streetcar_dwell(dest,direction,&value);
    else return FALSE;
    *pose=value;return TRUE;
}
UBYTE td_streetcar_destination(UBYTE origin,UBYTE target,td_streetcar_pose_t *pose) BANKED {
    td_streetcar_pose_t value;
    if(!pose||origin<TD_TRANSIT_QUEEN_FIRST||target<TD_TRANSIT_QUEEN_FIRST||
       origin>=TD_TRANSIT_QUEEN_FIRST+8||target>=TD_TRANSIT_QUEEN_FIRST+8||origin==target)return FALSE;
    td_streetcar_dwell(target-TD_TRANSIT_QUEEN_FIRST,origin>target,&value);
    *pose=value;return TRUE;
}
UBYTE td_streetcar_bounds(const td_streetcar_pose_t *pose,td_streetcar_box_t *box) BANKED {
    td_streetcar_box_t value;
    if(!pose||!box||!td_streetcar_bounds_local(pose,&value))return FALSE;
    *box=value;return TRUE;
}
UBYTE td_streetcar_sweep(UWORD seconds,UBYTE subsecond,UWORD elapsed,
                        const td_streetcar_box_t *box) BANKED {
    UWORD phase,first;td_streetcar_pose_t pose;td_streetcar_box_t body;
    if(!box||subsecond>=60||!td_streetcar_box_valid(box))return TD_STREETCAR_INVALID;
    phase=td_streetcar_clock(seconds,subsecond);
    if(!elapsed){
        td_streetcar_pose_local(phase,&pose);td_streetcar_bounds_local(&pose,&body);
        return td_streetcar_boxes_hit(&body,box);
    }
    if(elapsed>=TD_STREETCAR_PERIOD_TICKS)return td_streetcar_range_hit(0,TD_STREETCAR_PERIOD_TICKS,box);
    first=phase>=elapsed?phase-elapsed:phase+TD_STREETCAR_PERIOD_TICKS-elapsed;
    return first<=phase?td_streetcar_range_hit(first,phase,box):
        td_streetcar_range_hit(first,TD_STREETCAR_PERIOD_TICKS,box)||td_streetcar_range_hit(0,phase,box);
}
