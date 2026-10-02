#pragma bank 255
#include <stdint.h>
#include "td_district.h"
#include "gbs_types.h"
#include "collision.h"
#include "data_manager.h"
#include "vm.h"
#include "vm_exceptions.h"
#include "data/scene_toronto_city.h"
#include "data/scene_toronto_west.h"
#include "data/scene_toronto_high_park.h"
#include "data/scene_toronto_east.h"

static const far_ptr_t td_district_scenes[TD_DISTRICT_COUNT]={
    TO_FAR_PTR_T(scene_toronto_city),
    TO_FAR_PTR_T(scene_toronto_west),
    TO_FAR_PTR_T(scene_toronto_high_park),
    TO_FAR_PTR_T(scene_toronto_east)
};

/* This persistent WRAM buffer remains valid until core consumes the exception.
 * Pinned GBVM vm.i: VM_LOCK=25, VM_FADE=57 (modal out=01), VM_RAISE=27.
 * VM_STEP reads the context PC directly, including WRAM; its ROM bank is
 * irrelevant here. The last three bytes are GBVM's bank/lo-pointer/hi-pointer,
 * resolved from compiled scene data each time, never an observed ROM address. */
static UBYTE td_district_script[9]={0x25,0x57,0x01,0x27,3,EXCEPTION_CHANGE_SCENE,0,0,0};
static UBYTE td_district_queued=TD_DISTRICT_NONE;

UBYTE td_district_scene(UBYTE district,far_ptr_t *dest) BANKED {
    if(district>=TD_DISTRICT_COUNT||dest==NULL)return FALSE;
    *dest=td_district_scenes[district];return TRUE;
}

UBYTE td_district_current(void) BANKED {
    UBYTE i;
    for(i=0;i<TD_DISTRICT_COUNT;i++){
        if(current_scene.bank==td_district_scenes[i].bank&&current_scene.ptr==td_district_scenes[i].ptr){
            if(td_district_queued==i)td_district_queued=TD_DISTRICT_NONE;
            return i;
        }
    }
    return TD_DISTRICT_NONE;
}

void td_district_reset(void) BANKED {td_district_queued=TD_DISTRICT_NONE;}

UBYTE td_district_queue(UBYTE district) BANKED {
    UWORD address;
    if(district>=TD_DISTRICT_COUNT)return FALSE;
    if(td_district_current()==district||td_district_queued!=TD_DISTRICT_NONE)return FALSE;
    address=(UWORD)(uintptr_t)td_district_scenes[district].ptr;
    td_district_script[6]=td_district_scenes[district].bank;
    td_district_script[7]=(UBYTE)address;td_district_script[8]=(UBYTE)(address>>8);
    if(script_execute(1,td_district_script,NULL,0)==NULL)return FALSE;
    td_district_queued=district;return TRUE;
}

static UBYTE td_district_metadata(UBYTE district,scene_t *scene){
    far_ptr_t ref;
    if(!td_district_scene(district,&ref))return FALSE;
    MemcpyBanked(scene,ref.ptr,sizeof(*scene),ref.bank);
    /* All authored districts use this size. Reject a mismatched resource
     * rather than silently pairing native road bounds with different artwork. */
    return scene->width==TD_DISTRICT_TILE_WIDTH&&scene->height==TD_DISTRICT_TILE_HEIGHT&&scene->collisions.ptr!=NULL;
}

static UBYTE td_district_read_tile(const scene_t *scene,UBYTE x,UBYTE y){
    if(x>=scene->width||y>=scene->height)return COLLISION_ALL;
    return ReadBankedUBYTE((const UBYTE*)scene->collisions.ptr+(UWORD)y*scene->width+x,scene->collisions.bank);
}

UBYTE td_district_tile(UBYTE district,UBYTE x,UBYTE y) BANKED {
    scene_t scene;
    if(!td_district_metadata(district,&scene))return COLLISION_ALL;
    return td_district_read_tile(&scene,x,y);
}

UBYTE td_district_walkable(UBYTE district,UWORD u,UWORD v) BANKED {
    scene_t scene;
    if(u>=TD_DISTRICT_PIXEL_WIDTH||v>=TD_DISTRICT_PIXEL_HEIGHT||!td_district_metadata(district,&scene))return FALSE;
    return !(td_district_read_tile(&scene,u>>3,v>>3)&COLLISION_ALL);
}

UBYTE td_district_drivable(UBYTE district,UWORD u,UWORD v) BANKED {
    scene_t scene;UBYTE x,y,left,right,top,bottom;
    if(u<8||v<8||u>TD_DISTRICT_PIXEL_WIDTH-8||v>TD_DISTRICT_PIXEL_HEIGHT-8||!td_district_metadata(district,&scene))return FALSE;
    left=(u-5)>>3;right=(u+5)>>3;top=(v-5)>>3;bottom=(v+5)>>3;
    for(y=top;y<=bottom;y++)for(x=left;x<=right;x++)if(td_district_read_tile(&scene,x,y))return FALSE;
    return TRUE;
}
