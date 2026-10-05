#pragma bank 255
#include "td_city_sprites.h"
#include "td_district.h"
#include "data_manager.h"

typedef struct { far_ptr_t sprite; UBYTE base; } td_city_sprite_t;
static td_city_sprite_t td_city_player,td_city_fleet,td_city_civilian;

static void td_city_capture(td_city_sprite_t *cache,actor_t *loader){
    cache->sprite=loader->sprite;cache->base=loader->base_tile;
    /* The scriptless startup frame is empty. Preserve all other authored
     * loaders, including the following boat, in the inactive linked list. */
    if(loader->flags&ACTOR_FLAG_ACTIVE)deactivate_actor(loader);
    if(loader->prev)loader->prev->next=loader->next;
    else if(actors_inactive_head==loader)actors_inactive_head=loader->next;
    if(loader->next)loader->next->prev=loader->prev;
    loader->prev=loader->next=NULL;loader->flags=ACTOR_FLAG_HIDDEN;
}

void td_city_sprites_bind(void) BANKED {
    UBYTE district=td_district_current(),index=(district==0||district==1||district==3)?3:2;
    td_city_player.sprite=PLAYER.sprite;td_city_player.base=PLAYER.base_tile;
    td_city_fleet=td_city_civilian=td_city_player;
    if(district>=TD_DISTRICT_COUNT||actors_len<=index+1)return;
    if(!actors[index].sprite.bank||!actors[index].sprite.ptr||
       !actors[index+1].sprite.bank||!actors[index+1].sprite.ptr)return;
    td_city_capture(&td_city_fleet,&actors[index]);
    td_city_capture(&td_city_civilian,&actors[index+1]);
}

static void td_city_pose(actor_t *actor,const td_city_sprite_t *cache,UBYTE frame){
    /* Changing sheets with the same numerical frame still resets frame.
     * actor_set_frames otherwise checks only the existing frame interval. */
    if(actor->sprite.bank==cache->sprite.bank&&actor->sprite.ptr==cache->sprite.ptr&&
       actor->base_tile==cache->base&&actor->frame==frame&&
       actor->frame_start==frame&&actor->frame_end==frame+1){
        if(actor->anim_tick!=255)actor->anim_tick=255;
        return;
    }
    actor->sprite=cache->sprite;actor->base_tile=cache->base;
    actor->frame=actor->frame_start=frame;actor->frame_end=frame+1;
    actor->anim_tick=255;
}

void td_fleet_present(actor_t *actor,UBYTE kind,UBYTE orientation) BANKED {
    if(kind>=7||orientation>=4)return;
    if(kind<2||(td_city_fleet.sprite.bank==td_city_player.sprite.bank&&
                td_city_fleet.sprite.ptr==td_city_player.sprite.ptr))
        td_city_pose(actor,&td_city_player,(kind<2?kind:0)*8+orientation*2);
    else td_city_pose(actor,&td_city_fleet,(kind-2)*4+orientation);
}

void td_civilian_present(actor_t *actor,UBYTE variant,UBYTE pose) BANKED {
    if(variant>=4||pose>=6)return;
    if(td_city_civilian.sprite.bank==td_city_player.sprite.bank&&
       td_city_civilian.sprite.ptr==td_city_player.sprite.ptr)
        td_city_pose(actor,&td_city_player,32+(pose<4?pose:0));
    else td_city_pose(actor,&td_city_civilian,variant*6+pose);
}

void td_vehicle_present(actor_t *actor,UBYTE vehicle,UBYTE heading) BANKED {
    if(vehicle<4&&heading<16)td_city_pose(actor,&td_city_player,vehicle*8+((heading+1)&15)/2);
}

void td_scooter_parked_present(actor_t *actor,UBYTE heading) BANKED {
    if(heading<16)td_city_pose(actor,&td_city_player,49+((((heading+1)&15)/4)&1));
}

void td_player_sprite_restore(void) BANKED {
    PLAYER.sprite=td_city_player.sprite;PLAYER.base_tile=td_city_player.base;
}
