#pragma bank 255
#include <string.h>
#include "td_world.h"
#define TD_WORLD_DATA
#include "td_district_world.h"

typedef char td_world_district_count_matches[(TD_WORLD_GENERATED_DISTRICTS==TD_DISTRICT_COUNT)?1:-1];
#define TD_NONE_LEG 255
/* Farthest Manhattan distance (px) at which a vehicle is brought back. */
#define TD_RECYCLE_REACH 280

static UWORD td_world_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
/* Exact floor(a*b/c), with a,c<=16383 and b<=c. Keep remainder<c between
 * bits; doubled remainder plus b stays below49149. No32-bit math helpers are
 * needed in the almost-full fixed ROM bank. Straight crossings return early. */
static UWORD td_world_muldiv(UWORD a,UWORD b,UWORD c){
    UWORD bit=32768,result=0,remainder=0;
    if(!a||!b||!c)return 0;
    if(b==c)return a;
    while(bit>a)bit>>=1;
    for(;bit;bit>>=1){
        result<<=1;remainder<<=1;
        if(a&bit)remainder+=b;
        if(remainder>=c){
            remainder-=c;result++;
            if(remainder>=c){remainder-=c;result++;}
        }
    }
    return result;
}
/* Only opposite boundary pairs preserve the driver's heading/velocity. */
static UBYTE td_world_horizontal(const td_portal_t *p){return p->u==24||p->u==1000;}
static UBYTE td_world_valid_portal(const td_portal_t *p,UBYTE districts){
    if(p->from>=districts||p->to>=districts||p->from==p->to||p->vehicle>1)return FALSE;
    if(td_world_horizontal(p))
        return p->arrival_u==1024-p->u&&p->v>=32&&p->v<944&&p->arrival_v>=32&&p->arrival_v<944;
    return (p->v==24||p->v==952)&&p->arrival_v==976-p->v&&
           p->u>=32&&p->u<992&&p->arrival_u>=32&&p->arrival_u<992;
}

/* Reverse BFS handles directed, cyclic and disconnected graphs. Bounded local
 * scratch costs64 bytes, with no persistent WRAM or per-edge banked calls. */
static UBYTE td_world_route_from(const td_portal_t *portals,UWORD count,UBYTE districts,
                                UBYTE from,UBYTE to,UBYTE onfoot,UWORD u,UWORD v,
                                UWORD target_u,UWORD target_v,td_portal_t *out){
    UBYTE distance[TD_WORLD_MAX_DISTRICTS],queue[TD_WORLD_MAX_DISTRICTS],head=0,tail=0,node;
    UWORD i,score,best=65535;const td_portal_t *p,*selected=NULL;
    if(!portals||!out||!districts||districts>TD_WORLD_MAX_DISTRICTS||count>TD_WORLD_MAX_PORTALS||
       from>=districts||to>=districts||from==to||onfoot>1||u>=1024||v>=976||target_u>=1024||target_v>=976)return FALSE;
    memset(distance,255,sizeof(distance));distance[to]=0;queue[tail++]=to;
    while(head<tail){
        node=queue[head++];
        for(i=0;i<count;i++){
            p=&portals[i];
            if(!td_world_valid_portal(p,districts)||p->to!=node||(!onfoot&&!p->vehicle)||distance[p->from]!=255)continue;
            distance[p->from]=distance[node]+1;queue[tail++]=p->from;
        }
    }
    if(distance[from]==255)return FALSE;
    for(i=0;i<count;i++){
        p=&portals[i];
        if(!td_world_valid_portal(p,districts)||p->from!=from||(!onfoot&&!p->vehicle)||
           distance[p->to]==255||distance[p->to]+1!=distance[from])continue;
        score=td_world_distance(u,p->u)+td_world_distance(v,p->v)+
              td_world_distance(target_u,p->arrival_u)+td_world_distance(target_v,p->arrival_v);
        if(score<best){best=score;selected=p;}
    }
    if(!selected)return FALSE;
    *out=*selected;return TRUE;
}

static UBYTE td_world_crossing_from(const td_portal_t *portals,UWORD count,UBYTE districts,
                                   UBYTE district,UBYTE onfoot,UWORD old_u,UWORD old_v,
                                   UWORD u,UWORD v,td_crossing_t *out){
    UWORD i,axis,old_axis,boundary,lateral,old_lateral,cross_lateral,change;WORD offset,cross_offset,limit;
    UBYTE horizontal;const td_portal_t *p;td_crossing_t result;
    if(!portals||!out||!districts||districts>TD_WORLD_MAX_DISTRICTS||count>TD_WORLD_MAX_PORTALS||
       district>=districts||onfoot>1||old_u>=1024*16||u>=1024*16||old_v>=976*16||v>=976*16)return FALSE;
    /* Lanes are 24 px either side of a road's centre and sidewalks 16 more. */
    limit=(onfoot?44:30)*16;
    for(i=0;i<count;i++){
        p=&portals[i];
        if(!td_world_valid_portal(p,districts)||p->from!=district||(!onfoot&&!p->vehicle))continue;
        horizontal=td_world_horizontal(p);axis=horizontal?u:v;old_axis=horizontal?old_u:old_v;
        boundary=(horizontal?p->u:p->v)*16;
        if(boundary==24*16){if(axis>boundary||axis>=old_axis)continue;}
        else if(axis<boundary||axis<=old_axis)continue;
        lateral=horizontal?v:u;old_lateral=horizontal?old_v:old_u;
        offset=(WORD)lateral-(WORD)((horizontal?p->v:p->u)*16);
        if(offset<-limit||offset>limit)continue;
        /* Also check the actual swept intersection without an overflowing
         * 16-bit product or new32-bit runtime library. Already-outbound positions
         * may retry a previously failed VM allocation without crossing again. */
        if((old_axis>=boundary&&axis<=boundary)||(old_axis<=boundary&&axis>=boundary)){
            change=td_world_muldiv(td_world_distance(lateral,old_lateral),
                                  td_world_distance(boundary,old_axis),td_world_distance(axis,old_axis));
            cross_lateral=lateral>=old_lateral?old_lateral+change:old_lateral-change;
            cross_offset=(WORD)cross_lateral-(WORD)((horizontal?p->v:p->u)*16);
            if(cross_offset<-limit||cross_offset>limit)continue;
        }
        result.district=p->to;result.u=p->arrival_u*16+(horizontal?0:offset);
        result.v=p->arrival_v*16+(horizontal?offset:0);
        if(result.u>=1024*16||result.v>=976*16)continue;
        *out=result;return TRUE;
    }
    return FALSE;
}

UBYTE td_world_name(UBYTE district,char *name) BANKED {
    if(district>=TD_DISTRICT_COUNT||!name)return FALSE;
    memcpy(name,td_district_names[district],19);return TRUE;
}
/* The generated next-hop table (the same reverse breadth-first search, run
 * at build time) names the neighbour; the nearest of its entrances wins. */
UBYTE td_world_route(UBYTE from,UBYTE to,UBYTE onfoot,UWORD u,UWORD v,
                    UWORD target_u,UWORD target_v,td_portal_t *portal) BANKED {
    UBYTE next;UWORD i,score,best=65535;const td_portal_t *p,*selected=NULL;
    if(!portal||from>=TD_DISTRICT_COUNT||to>=TD_DISTRICT_COUNT||from==to||onfoot>1||u>=1024||v>=976)return FALSE;
    next=td_world_next[onfoot][from][to];
    if(next>=TD_DISTRICT_COUNT)return FALSE;
    for(i=0,p=td_portals;i<TD_PORTALS;i++,p++){
        if(p->from!=from||p->to!=next||(!onfoot&&!p->vehicle))continue;
        score=td_world_distance(u,p->u)+td_world_distance(v,p->v);
        if(next==to)score+=td_world_distance(target_u,p->arrival_u)+td_world_distance(target_v,p->arrival_v);
        if(score<best){best=score;selected=p;}
    }
    if(!selected)return FALSE;
    *portal=*selected;return TRUE;
}
/* A district's four scenes join along open seams: x=1000 (west scenes) and
 * x=24 (east), y=952 (north) and y=24 (south). Crossing one anywhere keeps
 * the position in the district; scenes overlap by 976 x 928 px offsets.
 * The caller validates the destination tile. Seams between districts are
 * the portal table's. */
static UBYTE td_world_inner(UBYTE district,UWORD old_u,UWORD old_v,UWORD u,UWORD v,td_crossing_t *out){
    UBYTE q=district&3;
    if(!(q&1)&&u>=1000*16&&u>old_u){out->district=district+1;out->u=u-976*16;out->v=v;return TRUE;}
    if((q&1)&&u<=24*16&&u<old_u){out->district=district-1;out->u=u+976*16;out->v=v;return TRUE;}
    if(!(q&2)&&v>=952*16&&v>old_v){out->district=district+2;out->u=u;out->v=v-928*16;return TRUE;}
    if((q&2)&&v<=24*16&&v<old_v){out->district=district-2;out->u=u;out->v=v+928*16;return TRUE;}
    return FALSE;
}
UBYTE td_world_crossing(UBYTE district,UBYTE onfoot,UWORD old_u,UWORD old_v,
                       UWORD u,UWORD v,td_crossing_t *crossing) BANKED {
    if(!crossing||district>=TD_DISTRICT_COUNT)return FALSE;
    if(td_world_crossing_from(td_portals,TD_PORTALS,TD_DISTRICT_COUNT,district,onfoot,old_u,old_v,u,v,crossing))return TRUE;
    return td_world_inner(district,old_u,old_v,u,v,crossing);
}

static UBYTE td_world_valid_traffic(UBYTE district,const UBYTE *legs){
    UBYTE i,count;
    if(district>=TD_DISTRICT_COUNT)return FALSE;
    for(i=0;i<TD_TRAFFIC_COUNT;i++){
        count=td_world_traffic_counts[district][i];
        if(count<2||count>TD_TRAFFIC_POINTS||(legs&&legs[i]>=count))return FALSE;
    }
    return TRUE;
}
static void td_world_sample(UBYTE district,UBYTE i,UBYTE leg,td_traffic_sample_t *sample){
    const UWORD (*path)[2]=td_world_traffic[district][i];
    UBYTE count=td_world_traffic_counts[district][i],previous=leg?leg-1:count-1;
    sample->u=path[leg][0]*16;sample->v=path[leg][1]*16;sample->count=count;
    sample->frame=path[leg][0]>path[previous][0]?0:path[leg][0]<path[previous][0]?4:path[leg][1]>path[previous][1]?2:6;
    sample->frame+=(i==5?1:i%4)*8;
}
UBYTE td_world_traffic_init(UBYTE district,UWORD *u,UWORD *v,UBYTE *legs,td_traffic_sample_t *samples) BANKED {
    UBYTE i;
    if(!u||!v||!legs||!samples||!td_world_valid_traffic(district,NULL))return FALSE;
    for(i=0;i<TD_TRAFFIC_COUNT;i++){
        u[i]=td_world_traffic[district][i][0][0]*16;v[i]=td_world_traffic[district][i][0][1]*16;
        legs[i]=1;td_world_sample(district,i,1,&samples[i]);
    }
    return TRUE;
}
UBYTE td_world_traffic_samples(UBYTE district,const UBYTE *legs,td_traffic_sample_t *samples) BANKED {
    UBYTE i;
    if(!legs||!samples||!td_world_valid_traffic(district,legs))return FALSE;
    for(i=0;i<TD_TRAFFIC_COUNT;i++)td_world_sample(district,i,legs[i],&samples[i]);
    return TRUE;
}

/* Ambient traffic: put vehicle i on the point of its own loop nearest the
 * courier (whole pixels pu,pv) that lies outside the box |du|<vx,|dv|<vy,
 * preferring a point from which it drives toward the courier. Loops are
 * cardinal, so each leg is checked once. FALSE (outputs unchanged) when no
 * point lies within reach. */
UBYTE td_world_traffic_recycle(UBYTE district,UBYTE i,UWORD pu,UWORD pv,UWORD vx,UWORD vy,
                              UWORD *u,UWORD *v,UBYTE *leg,td_traffic_sample_t *sample) BANKED {
    const UWORD (*path)[2];UBYTE k,count,best_leg=TD_NONE_LEG;
    UWORD ax,ay,bx,by,lo,hi,x,y,d,best=TD_RECYCLE_REACH,best_x=0,best_y=0;
    if(!u||!v||!leg||!sample||!td_world_valid_traffic(district,NULL)||i>=TD_TRAFFIC_COUNT)return FALSE;
    path=td_world_traffic[district][i];count=td_world_traffic_counts[district][i];
    for(k=0;k<count;k++){
        ax=path[k?k-1:count-1][0];ay=path[k?k-1:count-1][1];bx=path[k][0];by=path[k][1];
        if(ay==by){
            lo=ax<bx?ax:bx;hi=ax<bx?bx:ax;y=ay;
            if(td_world_distance(y,pv)>=vy)x=pu<lo?lo:pu>hi?hi:pu;
            else{x=bx>ax?pu-vx:pu+vx;if(pu<vx&&bx>ax)continue;}
            if(x<lo||x>hi)continue;
        }else if(ax==bx){
            lo=ay<by?ay:by;hi=ay<by?by:ay;x=ax;
            if(td_world_distance(x,pu)>=vx)y=pv<lo?lo:pv>hi?hi:pv;
            else{y=by>ay?pv-vy:pv+vy;if(pv<vy&&by>ay)continue;}
            if(y<lo||y>hi)continue;
        }else continue;
        if(td_world_distance(x,pu)<vx&&td_world_distance(y,pv)<vy)continue;
        d=td_world_distance(x,pu)+td_world_distance(y,pv);
        if(d<best){best=d;best_leg=k;best_x=x;best_y=y;}
    }
    if(best_leg==TD_NONE_LEG)return FALSE;
    *u=best_x*16;*v=best_y*16;*leg=best_leg;td_world_sample(district,i,best_leg,sample);
    return TRUE;
}
/* The scene's signal junctions (whole pixels); 0xFFFF marks an unused slot. */
UBYTE td_world_signals(UBYTE district,UWORD *u,UWORD *v) BANKED {
    UBYTE i;
    if(!u||!v||district>=TD_DISTRICT_COUNT)return FALSE;
    for(i=0;i<TD_SIGNALS;i++){u[i]=td_world_signals_at[district][i][0];v[i]=td_world_signals_at[district][i][1];}
    return TRUE;
}
