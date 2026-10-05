#pragma bank 255
#include "td_navigation.h"
#include "td_district.h"
typedef struct { UWORD u,v;UBYTE district; } td_navigation_goal_t;
#include "td_navigation_goals.h"

static UWORD td_navigation_goal(UBYTE district,UWORD u,UWORD v){
    UWORD low=0,high=TD_NAVIGATION_GOALS,middle;
    const td_navigation_goal_t *goal;
    while(low<high){
        middle=low+(high-low)/2;goal=&td_navigation_goals[middle];
        if(goal->district<district||(goal->district==district&&
           (goal->v<v||(goal->v==v&&goal->u<u))))low=middle+1;else high=middle;
    }
    if(low==TD_NAVIGATION_GOALS)return 65535;
    goal=&td_navigation_goals[low];
    return goal->district==district&&goal->u==u&&goal->v==v?low:65535;
}

UBYTE td_navigation_direction(UBYTE district,UBYTE onfoot,UWORD goal_u,UWORD goal_v,
    UWORD u,UWORD v) BANKED {
    UWORD goal;
    if(district>=TD_DISTRICT_COUNT||onfoot>1||u>=1024||v>=976||goal_u>=1024||goal_v>=976)return 0;
    goal=td_navigation_goal(district,goal_u,goal_v);
    if(goal==65535)return 0;
    return td_navigation_cell(goal,onfoot,u>>3,v>>3);
}

UBYTE td_navigation_next(UBYTE district,UBYTE onfoot,UWORD goal_u,UWORD goal_v,
    UWORD u,UWORD v,td_navigation_waypoint_t *out) BANKED {
    UWORD goal;UBYTE x,y,direction,next,step;
    if(!out||district>=TD_DISTRICT_COUNT||onfoot>1||u>=1024||v>=976||goal_u>=1024||goal_v>=976)return FALSE;
    goal=td_navigation_goal(district,goal_u,goal_v);
    if(goal==65535)return FALSE;
    x=u>>3;y=v>>3;direction=td_navigation_cell(goal,onfoot,x,y);
    if(!direction)return FALSE;
    for(step=0;step<4&&direction!=TD_NAVIGATION_ARRIVED;step++){
        if(direction==TD_NAVIGATION_NORTH){if(!y)return FALSE;y--;}
        else if(direction==TD_NAVIGATION_EAST){if(x==127)return FALSE;x++;}
        else if(direction==TD_NAVIGATION_SOUTH){if(y==121)return FALSE;y++;}
        else if(direction==TD_NAVIGATION_WEST){if(!x)return FALSE;x--;}
        else return FALSE;
        next=td_navigation_cell(goal,onfoot,x,y);
        if(!next)return FALSE;
        if(next!=direction){direction=next;break;}
    }
    out->u=(UWORD)x*8+4;out->v=(UWORD)y*8+4;out->direction=direction;return TRUE;
}
