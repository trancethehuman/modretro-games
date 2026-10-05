#pragma bank 255
#include "td_roads.h"
#include "collision.h"
#include "input.h"
static UBYTE td_road_rectangle(UBYTE left,UBYTE right,UBYTE top,UBYTE bottom){
    UBYTE axis,clear=TRUE,saved_x=tile_hit_x,saved_y=tile_hit_y;
    /* The pinned engine range API executes in fixed ROM and switches the
       collision bank once for the complete row/column. Do not switch banks
       directly while executing this banked module. Preserve native hit state. */
    if(bottom-top<=right-left){
        for(axis=top;axis<=bottom;axis++)if(tile_col_test_range_x(255,axis,left,right)){clear=FALSE;break;}
    }else{
        for(axis=left;axis<=right;axis++)if(tile_col_test_range_y(255,axis,top,bottom)){clear=FALSE;break;}
    }
    tile_hit_x=saved_x;tile_hit_y=saved_y;return clear;
}

UBYTE td_road_body(UWORD u,UWORD v,UBYTE half) BANKED {
    UBYTE left,right,top,bottom;
    if(!half||half>8||u<8||v<8||u>1016||v>968)return FALSE;
    left=(u-half)>>3;right=(u+half)>>3;top=(v-half)>>3;bottom=(v+half)>>3;
    /* Eleven pixels can overlap three tile rows/columns: corners alone miss rails. */
    return td_road_rectangle(left,right,top,bottom);
}

static UBYTE td_road_drivable(UWORD u,UWORD v){return td_road_body(u,v,7);}

UBYTE td_road_sweep(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half) BANKED {
    UBYTE left,right,top,bottom;
    if(!half||half>8||(old_u!=u&&old_v!=v)||old_u<8||u<8||old_v<8||v<8||old_u>1016||u>1016||old_v>968||v>968)return FALSE;
    left=((old_u<u?old_u:u)-half)>>3;right=((old_u>u?old_u:u)+half)>>3;
    top=((old_v<v?old_v:v)-half)>>3;bottom=((old_v>v?old_v:v)+half)>>3;
    /* A cardinal sweep is this exact body union. Read each covered tile once. */
    return td_road_rectangle(left,right,top,bottom);
}

UBYTE td_road_walkable(UWORD u,UWORD v) BANKED {
    if(u>=1024||v>=976)return FALSE;
    return !(tile_at(u>>3,v>>3)&15);
}

UBYTE td_road_corner(td_state_t *state,WORD *vx,WORD *vy,UBYTE *used,WORD nu,WORD nv) BANKED {
    UBYTE i,j,side,clear;WORD shift,shifted;UWORD ax=(*vx)<0?-(*vx):(*vx),ay=(*vy)<0?-(*vy):(*vy);
    if(!INPUT_A||INPUT_B||state->speed<3||ax==ay||(*used))return FALSE;
    /* A quantised corner may clip the car by a few pixels although a parallel
       lane is open. Sweep only across usable current and proposed footprints. */
    for(i=1;i<=6;i++)for(side=0;side<2;side++){
        shift=side?-(WORD)i*16:(WORD)i*16;
        if(ay>ax&&nv!=(WORD)state->v){
            shifted=(WORD)state->u+shift;
            if(shifted<128||shifted>1016*16||nv<128||nv>968*16||!td_road_drivable(shifted>>4,nv>>4))continue;
            clear=1;for(j=1;j<=i;j++)if(!td_road_drivable(((WORD)state->u+(side?-(WORD)j*16:(WORD)j*16))>>4,state->v>>4)){clear=0;break;}
            if(clear){state->u=shifted;state->v=nv;(*vx)=0;(*used)=1;return TRUE;}
        }else if(ax>ay&&nu!=(WORD)state->u){
            shifted=(WORD)state->v+shift;
            if(shifted<128||shifted>968*16||nu<128||nu>1016*16||!td_road_drivable(nu>>4,shifted>>4))continue;
            clear=1;for(j=1;j<=i;j++)if(!td_road_drivable(state->u>>4,((WORD)state->v+(side?-(WORD)j*16:(WORD)j*16))>>4)){clear=0;break;}
            if(clear){state->u=nu;state->v=shifted;(*vy)=0;(*used)=1;return TRUE;}
        }
    }
    return FALSE;
}
