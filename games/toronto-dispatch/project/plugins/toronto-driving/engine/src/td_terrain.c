#pragma bank 255
#include "td_terrain.h"
#include "td_roads.h"

UBYTE td_terrain_drivable(UWORD u,UWORD v,td_terrain_cache_t *cache) BANKED {
    if(cache->valid&&cache->u==u&&cache->v==v)return cache->result;
    cache->result=td_road_body(u,v,7);cache->u=u;cache->v=v;cache->valid=1;
    return cache->result;
}
