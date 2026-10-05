#pragma bank 255
#include "td_police_lanes.h"
#include "td_roads.h"

typedef struct { UBYTE x,y; UWORD next0,next1,returns; } td_police_vertex_t;
#include "td_police_road_data.h"
#define TD_POLICE_NONE 65535u
#define td_police_distance(a,b) ((a)>(b)?(a)-(b):(b)-(a))

static UWORD td_police_exit(UBYTE district,UWORD index,UBYTE choice){
    const td_police_vertex_t *node=&td_police_vertices[td_police_offsets[district]+index];
    UWORD i;
    if(!choice)return node->next0;
    if(choice==1)return node->next1==TD_POLICE_NONE?TD_POLICE_NONE:node->next1&32767;
    if(choice!=2||node->next1==TD_POLICE_NONE||!(node->next1&32768))return TD_POLICE_NONE;
    for(i=td_police_third_offsets[district];i<td_police_third_offsets[district+1];i++)
        if(td_police_thirds[i][0]==index)return td_police_thirds[i][1];
    return TD_POLICE_NONE;
}
static UBYTE td_police_direction(UWORD u,UWORD v,UWORD next_u,UWORD next_v){
    return u!=next_u?(next_u>u?0:2):(next_v>v?1:3);
}
static UBYTE td_police_between(UWORD position,UWORD a,UWORD b){
    return a<b?(position>a&&position<b):(position<a&&position>b);
}
static void td_police_projection(UWORD u,UWORD v,UWORD goal_u,UWORD goal_v,
    UWORD *next_u,UWORD *next_v){
    if(u!=*next_u){
        if(td_police_between(goal_u,u,*next_u))*next_u=goal_u;
    }else if(td_police_between(goal_v,v,*next_v))*next_v=goal_v;
}

/* Look through at most four existing short connector edges, scoring the
 * intended outgoing street rather than an eight-pixel corner waypoint.
 * An explicit iterative depth-first walk bounds both work and local scratch;
 * it is not recursion, a whole-city search or a remembered pursuit route. */
static UWORD td_police_chase(UBYTE district,UWORD start,UWORD goal_u,UWORD goal_v,
    UBYTE heading){
    UWORD path[4],first=TD_POLICE_NONE,best=TD_POLICE_NONE,best_score=65535;
    UWORD next,u,v,next_u,next_v,score;
    UBYTE choices[4],depth=0,direction;
    const td_police_vertex_t *node,*destination;
    path[0]=start;choices[0]=0;
    for(;;){
        next=td_police_exit(district,path[depth],choices[depth]++);
        if(next==TD_POLICE_NONE){
            if(!depth)break;
            depth--;continue;
        }
        if(!depth)first=next;
        node=&td_police_vertices[td_police_offsets[district]+path[depth]];
        destination=&td_police_vertices[td_police_offsets[district]+next];
        u=(UWORD)node->x*128;v=(UWORD)node->y*128;
        next_u=(UWORD)destination->x*128;next_v=(UWORD)destination->y*128;
        if(depth<3&&td_police_distance(u,next_u)+td_police_distance(v,next_v)<=16*16){
            depth++;path[depth]=next;choices[depth]=0;continue;
        }
        direction=td_police_direction(u,v,next_u,next_v);
        td_police_projection(u,v,goal_u,goal_v,&next_u,&next_v);
        score=td_police_distance(next_u,goal_u)+td_police_distance(next_v,goal_v);
        if(direction!=heading)score+=((direction+2)&3)==heading?128:32;
        if(score<best_score){best_score=score;best=first;}
    }
    return best;
}

UBYTE td_police_lanes_next(UBYTE district,UBYTE wanted,UWORD u,UWORD v,
    UWORD target_u,UWORD target_v,UBYTE heading,td_police_plan_t *out) BANKED {
    UWORD index,next=TD_POLICE_NONE,goal=TD_POLICE_NONE,count;
    UWORD node_u,node_v,next_u=0,next_v=0;
    UBYTE choice,direction;
    const td_police_vertex_t *node,*destination;
    td_police_plan_t result;
    if(district>=TD_POLICE_ROAD_DISTRICTS||!out)return FALSE;
    count=td_police_offsets[district+1]-td_police_offsets[district];
    if(!count)return FALSE;
    if(!wanted){
        for(index=td_police_goal_offsets[district];index<td_police_goal_offsets[district+1];index++)
            if(td_police_goals[index][0]*16==target_u&&td_police_goals[index][1]*16==target_v){
                goal=index-td_police_goal_offsets[district];break;
            }
        if(goal==TD_POLICE_NONE)return FALSE;
    }
    for(index=0;index<count;index++){
        node=&td_police_vertices[td_police_offsets[district]+index];
        node_u=(UWORD)node->x*128;node_v=(UWORD)node->y*128;
        /* An exact point or cardinal edge must share an origin coordinate.
           Skip unrelated nodes before extracting their directed exits. */
        if(node_u!=u&&node_v!=v)continue;
        if(node_u==u&&node_v==v){
            if(wanted)next=td_police_chase(district,index,target_u,target_v,heading);
            else{
                choice=(node->returns>>(goal*2))&3;
                if(choice)next=td_police_exit(district,index,choice-1);
            }
            if(next==TD_POLICE_NONE)return FALSE;
            destination=&td_police_vertices[td_police_offsets[district]+next];
            next_u=(UWORD)destination->x*128;next_v=(UWORD)destination->y*128;
            break;
        }
        /* Between real vertices retain the exact directed lane coordinate.
         * Fractions along its axis are valid; no lateral snap is permitted. */
        for(choice=0;choice<3;choice++){
            next=td_police_exit(district,index,choice);
            if(next==TD_POLICE_NONE)break;
            destination=&td_police_vertices[td_police_offsets[district]+next];
            next_u=(UWORD)destination->x*128;next_v=(UWORD)destination->y*128;
            direction=td_police_direction(node_u,node_v,next_u,next_v);
            if(direction!=heading)continue;
            if((node_v==v&&next_v==v&&td_police_between(u,node_u,next_u))||
               (node_u==u&&next_u==u&&td_police_between(v,node_v,next_v)))break;
        }
        if(choice<3&&next!=TD_POLICE_NONE)break;
    }
    if(index==count)return FALSE;
    if(wanted||(u==target_u&&next_u==target_u)||(v==target_v&&next_v==target_v))
        td_police_projection(u,v,target_u,target_v,&next_u,&next_v);
    if((u==next_u)==(v==next_v)||
       !td_road_sweep(u>>4,v>>4,next_u>>4,next_v>>4,5))return FALSE;
    result.u=next_u;result.v=next_v;
    result.heading=td_police_direction(u,v,next_u,next_v);result.valid=TRUE;
    *out=result;return TRUE;
}
