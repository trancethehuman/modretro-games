/* Exercise actual banked presentation with host actor-list adapters. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "actor.h"
#include "city_loader_fixture.h"
actor_t actors[22],*actors_inactive_head;
UBYTE actors_len;
static UBYTE district;
static unsigned long checks;
static const int player_data,fleet_data,civilian_data,boat_data;
static void require(int condition,const char *message){
    checks++;
    if(!condition){fprintf(stderr,"FAIL %lu: %s\n",checks,message);exit(1);}
}
UBYTE td_district_current(void){return district;}
void deactivate_actor(actor_t *actor){
    /* Active loaders are removed from their separate list first. */
    if(actor->prev)actor->prev->next=actor->next;
    if(actor->next)actor->next->prev=actor->prev;
    actor->prev=NULL;actor->next=actors_inactive_head;
    if(actors_inactive_head)actors_inactive_head->prev=actor;
    actors_inactive_head=actor;actor->flags&=~ACTOR_FLAG_ACTIVE;
}
#include "sprites_under_test.c"

static void init(UBYTE place,UBYTE active){
    UBYTE first=place<TD_DISTRICT_COUNT?host_loader_first[place]:2;
    memset(actors,0,sizeof(actors));district=place;actors_len=first+3;
    PLAYER.sprite=(far_ptr_t){4,&player_data};PLAYER.base_tile=0;
    actors[first].sprite=(far_ptr_t){8,&fleet_data};actors[first].base_tile=92;
    actors[first+1].sprite=(far_ptr_t){9,&civilian_data};actors[first+1].base_tile=108;
    actors[first+2].sprite=(far_ptr_t){10,&boat_data};actors[first+2].base_tile=114;
    if(active){
        actors[first].flags=actors[first+1].flags=ACTOR_FLAG_ACTIVE;
        actors[first].next=&actors[first+1];actors[first+1].prev=&actors[first];
        actors_inactive_head=&actors[first+2];
    }else{
        actors_inactive_head=&actors[1];actors[1].next=&actors[first];
        actors[first].prev=&actors[1];actors[first].next=&actors[first+1];
        actors[first+1].prev=&actors[first];actors[first+1].next=&actors[first+2];
        actors[first+2].prev=&actors[first+1];
    }
}

static void presentation(void){
    UBYTE place,active,kind,orientation,variant,pose,first;
    actor_t *actor=&actors[18];
    for(place=0;place<TD_DISTRICT_COUNT;place++)for(active=0;active<2;active++){
        first=host_loader_first[place];init(place,active);
        td_city_sprites_bind();
        require(!actors[first].prev&&!actors[first].next&&
                actors[first].flags==ACTOR_FLAG_HIDDEN,"Fleet loader detached");
        require(!actors[first+1].prev&&!actors[first+1].next&&
                actors[first+1].flags==ACTOR_FLAG_HIDDEN,"Civilian loader detached");
        if(active)require(actors_inactive_head==&actors[first+2]&&
                          !actors[first+2].prev,"Following boat retained after active loader removal");
        else require(actors_inactive_head==&actors[1]&&actors[1].next==&actors[first+2]&&
                     actors[first+2].prev==&actors[1],"Preceding Queen and following boat retained");
        /* Root clones only after binding: copied caches must survive reuse. */
        actors[first]=actors[first+1]=PLAYER;
        for(kind=0;kind<7;kind++)for(orientation=0;orientation<4;orientation++){
            actor_t before;
            memset(actor,0,sizeof(*actor));actor->pos_x=1234;actor->pos_y=4567;
            actor->flags=37;actor->prev=&actors[1];actor->next=&actors[20];before=*actor;
            td_fleet_present(actor,kind,orientation);
            require(actor->sprite.bank==(kind<2?4:8)&&actor->sprite.ptr==(kind<2?&player_data:&fleet_data),"Correct far sheet");
            require(actor->base_tile==(kind<2?0:92),"Correct base allocation");
            require(actor->frame==(kind<2?kind*8+orientation*2:(kind-2)*4+orientation)&&
                    actor->frame_start==actor->frame&&actor->frame_end==actor->frame+1&&
                    actor->anim_tick==255,"Correct fleet pose interval");
            require(actor->pos_x==before.pos_x&&actor->pos_y==before.pos_y&&
                    actor->flags==before.flags&&actor->prev==before.prev&&actor->next==before.next,"Motion/flags/list unchanged");
        }
        for(variant=0;variant<4;variant++)for(pose=0;pose<6;pose++){
            td_civilian_present(actor,variant,pose);
            require(actor->sprite.bank==9&&actor->sprite.ptr==&civilian_data&&actor->base_tile==108,"Civilian allocation");
            require(actor->frame==variant*6+pose&&actor->frame_end==actor->frame+1,"Civilian palette/pose ID");
        }
        {actor_t before=*actor;
         td_fleet_present(actor,7,0);td_fleet_present(actor,0,4);
         td_civilian_present(actor,4,0);td_civilian_present(actor,0,6);
         require(!memcmp(actor,&before,sizeof(before)),"Invalid pose leaves actor unchanged");}
    }
}

static void reset_and_banks(void){
    UBYTE first;
    init(0,0);first=3;
    /* Banked resources may share a ROM address. Far-bank identity matters. */
    actors[first].sprite.ptr=actors[first+1].sprite.ptr=PLAYER.sprite.ptr;
    td_city_sprites_bind();td_fleet_present(&actors[18],2,1);
    require(actors[18].sprite.bank==8&&actors[18].frame==1,"Same address different bank still fleet");
    td_civilian_present(&actors[18],1,4);
    require(actors[18].sprite.bank==9&&actors[18].frame==10,"Same address different bank still civilian");
    init(4,0);actors[2].sprite.bank=0;td_city_sprites_bind();
    td_fleet_present(&actors[18],5,3);td_civilian_present(&actors[19],1,4);
    require(actors[18].sprite.bank==4&&actors[18].frame==6,"Invalid binding discards previous fleet cache");
    require(actors[19].sprite.bank==4&&actors[19].frame==32,"Invalid binding discards previous civilian cache");
    td_fleet_present(&actors[18],6,3);
    require(actors[18].sprite.bank==4&&actors[18].frame==6,"Missing fleet binding makes taxi fall back safely to original sedan");
    require(actors_inactive_head==&actors[1]&&actors[1].next==&actors[2],"Invalid bind preserves native list");
    init(0,0);actors_len=4;td_city_sprites_bind();
    td_fleet_present(&actors[18],3,2);
    require(actors[18].sprite.bank==4&&actors[18].frame==4,"Short native actor list fails closed");
    init(TD_DISTRICT_COUNT,0);td_city_sprites_bind();td_civilian_present(&actors[18],0,3);
    require(actors[18].sprite.bank==4&&actors[18].frame==35,"Invalid district fails closed");
}

static void unchanged_pose(void){
    actor_t *actor=&actors[18],before;UBYTE changed;
    init(0,0);td_city_sprites_bind();td_fleet_present(actor,2,1);before=*actor;
    td_fleet_present(actor,2,1);
    require(!memcmp(actor,&before,sizeof(before)),"Unchanged pose preserves complete actor");
    /* A matching interval alone must not suppress repairing actual frame,
     * far bank/address, tile allocation or the frozen animation marker. */
    for(changed=0;changed<8;changed++){
        *actor=before;
        if(changed==0)actor->sprite.bank++;
        else if(changed==1)actor->sprite.ptr=&boat_data;
        else if(changed==2)actor->base_tile++;
        else if(changed==3)actor->frame++;
        else if(changed==4)actor->frame_start++;
        else if(changed==5)actor->frame_end++;
        else if(changed==6)actor->anim_tick=0;
        else actor->anim_tick=254;
        td_fleet_present(actor,2,1);
        require(!memcmp(actor,&before,sizeof(before)),"Pose repairs each changed sprite/animation field");
    }
    td_civilian_present(actor,1,4);before=*actor;actor->frame=0;
    td_civilian_present(actor,1,4);
    require(!memcmp(actor,&before,sizeof(before)),"Matching civilian interval repairs drifted actual frame");
}

int main(void){presentation();reset_and_banks();unchanged_pose();printf("City sprite harness: %lu checks, 0 failures\n",checks);return 0;}
