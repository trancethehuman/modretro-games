/* Generated atlas API; banked data units keep every ROM array bounded. */
#pragma bank 255
#include <string.h>
#include "td_atlas.h"
#include "td_atlas_data.h"
#include "td_district.h"
typedef char td_atlas_registered_count_matches[(TD_DISTRICT_COUNT==5)?1:-1];
typedef char td_atlas_registered_dimensions_match[(TD_DISTRICT_PIXEL_WIDTH==1024&&TD_DISTRICT_PIXEL_HEIGHT==976)?1:-1];
static const UWORD td_atlas_origins[5][2]={
    {256,0},
    {128,0},
    {0,0},
    {384,0},
    {384,122},
};
static const char td_atlas_names[5][19]={
    "CENTRAL TORONTO",
    "WEST END",
    "HIGH PARK/JUNCTION",
    "TORONTO EAST END",
    "PORT LANDS",
};
UBYTE td_atlas_bounds(UWORD *width_pixels,UWORD *height_pixels) BANKED {
    if(!width_pixels||!height_pixels)return FALSE;
    *width_pixels=TD_ATLAS_WIDTH_PIXELS;*height_pixels=TD_ATLAS_HEIGHT_PIXELS;return TRUE;
}
UBYTE td_atlas_position(UBYTE district,UWORD local_u,UWORD local_v,UWORD *x,UWORD *y) BANKED {
    if(!x||!y||district>=TD_DISTRICT_COUNT||local_u>=TD_DISTRICT_PIXEL_WIDTH||local_v>=TD_DISTRICT_PIXEL_HEIGHT)return FALSE;
    *x=td_atlas_origins[district][0]+(local_u>>3);
    *y=td_atlas_origins[district][1]+(local_v>>3);return TRUE;
}
UBYTE td_atlas_row(UBYTE tile_x,UBYTE tile_y,UBYTE count,UWORD *patterns) BANKED {
    UWORD offset,local,room;UBYTE take,written=0;
    if(!patterns||!count||count>TD_ATLAS_VIEW_WIDTH||tile_x>=TD_ATLAS_TILE_WIDTH||tile_y>=TD_ATLAS_TILE_HEIGHT||count>TD_ATLAS_TILE_WIDTH-tile_x)return FALSE;
    offset=(UWORD)tile_y*TD_ATLAS_TILE_WIDTH+tile_x;
    while(written<count){
        local=offset&4095;room=4096-local;
        take=count-written;if(room<take)take=(UBYTE)room;
        switch(offset>>12){
        case 0:td_atlas_row_unit_0(local,take,patterns+written);break;
        }
        offset+=take;written+=take;
    }
    return TRUE;
}
UBYTE td_atlas_pattern(UWORD id,UBYTE *tile16) BANKED {
    if(!tile16||id>=TD_ATLAS_PATTERNS)return FALSE;
    switch(id>>9){
    case 0:td_atlas_pattern_unit_0(id&511,tile16);break;
    case 1:td_atlas_pattern_unit_1(id&511,tile16);break;
    }
    return TRUE;
}
UBYTE td_atlas_district(UWORD x,UWORD y,char *name19) BANKED {
    UBYTE district;UWORD left,top;
    if(!name19||x>=TD_ATLAS_WIDTH_PIXELS||y>=TD_ATLAS_HEIGHT_PIXELS)return FALSE;
    for(district=0;district<TD_DISTRICT_COUNT;district++){
        left=td_atlas_origins[district][0];top=td_atlas_origins[district][1];
        if(x>=left&&x-left<TD_DISTRICT_PIXEL_WIDTH/TD_ATLAS_SCALE&&y>=top&&y-top<TD_DISTRICT_PIXEL_HEIGHT/TD_ATLAS_SCALE){
            memcpy(name19,td_atlas_names[district],19);return TRUE;
        }
    }
    return FALSE;
}
