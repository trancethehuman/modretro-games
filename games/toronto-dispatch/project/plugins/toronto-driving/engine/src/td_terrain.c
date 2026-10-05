#pragma bank 255
#include "td_terrain.h"
#include "td_roads.h"
#include "collision.h"

UBYTE td_terrain_drivable(UWORD u,UWORD v,td_terrain_cache_t *cache) BANKED {
    if(cache->valid&&cache->u==u&&cache->v==v)return cache->result;
    /* Player tyres may mount walkable sidewalks (bit16). The low collision
       nibble still blocks buildings, water, rails and every solid tile.
       NPC route admission deliberately keeps the stricter road-only API. */
    {UBYTE y,left,right,top,bottom,hit_x=tile_hit_x,hit_y=tile_hit_y;
     cache->result=FALSE;
     if(u>=8&&v>=8&&u<=1016&&v<=968){
        left=(u-7)>>3;right=(u+7)>>3;top=(v-7)>>3;bottom=(v+7)>>3;
        cache->result=TRUE;
        for(y=top;y<=bottom;y++)if(tile_col_test_range_x(15,y,left,right)){cache->result=FALSE;break;}
     }
     tile_hit_x=hit_x;tile_hit_y=hit_y;}
    cache->u=u;cache->v=v;cache->valid=1;
    return cache->result;
}
