#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host_actor_scene.h"
void actors_render(void);
void td_actor_render_before(void);
void td_actor_render_prepare(void);
actor_t actors[22],*emote_actor;
UBYTE CURRENT_BANK,actors_len,emote_timer,LCDC_REG,WX_REG,WY_REG,show_actors_on_overlay,screen_x,screen_y;
UBYTE allocated_sprite_tiles,allocated_hardware_sprites,__render_shadow_OAM;
UBYTE _is_CGB,overlay_priority;
WORD draw_scroll_x,draw_scroll_y,scroll_x,scroll_y;
const BYTE emote_offsets[1]={0};
const metasprite_t emote_metasprite_8_16[]={{metasprite_end,0,0,0}},emote_metasprite_8_8[]={{metasprite_end,0,0,0}};
_Alignas(256) volatile OAM_item_t shadow_OAM[40],shadow_OAM2[40];
static const metasprite_t person[]={{8,4,0,1},{metasprite_end,0,0,0}};
static const metasprite_t *const frames[]={person};
static spritesheet_t sheets[22];
static unsigned checks,order[32],used,boat_calls,boat_after_count,post_order[9],post_count,pre_order[4],pre_count;
static UBYTE boat_active,guide_owned;
static unsigned guide_force_calls;
static void require(int ok,const char *message){checks++;if(!ok){fprintf(stderr,"FAIL %s\n",message);exit(1);}}
void MemcpyBanked(void *out,const void *in,size_t bytes,UBYTE bank){require(bank==7,"Ground metadata bank stays exact");memcpy(out,in,bytes);}
UBYTE ReadBankedUBYTE(const UBYTE *source,UBYTE bank){
    (void)source;(void)bank;require(0,"Metadata binding must not regress to a separate count-bank read");return 0;
}
void host_actor_pose_probe(const spritesheet_t *sprite){
    require(CURRENT_BANK==7,"Combined count/table/frame binding selects the exact native sprite bank before reading");
    for(unsigned i=0;i<22;i++)if(sprite==&sheets[i]){require(used<32,"Ground call log stays bounded");order[used++]=i;return;}
    require(0,"Unexpected ground metadata source");
}
UBYTE move_metasprite(const metasprite_t *pose,UBYTE base,UBYTE index,WORD x,WORD y){
    (void)pose;(void)base;(void)index;(void)x;(void)y;require(0,"Fixture does not invent a stock emote");return 0;
}
void td_guidance_road_prepare(void){require(pre_count<4&&used==0&&CURRENT_BANK==5,"Normal guidance preparation precedes ground");pre_order[pre_count++]=3;}
void td_guidance_road_restore(void){require(used==0&&CURRENT_BANK==5,"Forced guidance release precedes modal reuse");guide_force_calls++;guide_owned=0;if(pre_count<4)pre_order[pre_count++]=3;}
void td_hospital_render(void){require(post_count<9,"Hospital facade dispatch bounded");post_order[post_count++]=8;}
void td_combat_render_restore(void){require(pre_count<4&&used==0&&CURRENT_BANK==5,"Muzzle restore precedes ground");pre_order[pre_count++]=4;}
void td_guidance_road_render(void){require(post_count<9,"Guidance dispatch bounded");post_order[post_count++]=6;}
void td_combat_render(void){require(post_count<9,"Muzzle dispatch bounded");post_order[post_count++]=7;}
void td_scenery_restore(void){require(pre_count<4&&used==0&&CURRENT_BANK==5,"Scenery restore precedes ground and preserves saved bank");pre_order[pre_count++]=2;}
void td_scenery_render(void){require(post_count<9,"Post-ground log bounded");post_order[post_count++]=2;}
void td_aircraft_render_restore(void){require(pre_count<4&&used==0&&CURRENT_BANK==5,"Aircraft restore precedes ground and preserves saved bank");pre_order[pre_count++]=1;}
void td_aircraft_render(void){require(post_count<9,"Post-ground log bounded");post_order[post_count++]=5;}
void td_traffic_lights_render(void){require(CURRENT_BANK==5,"Core restores saved bank before banked overlay dispatch");require(post_count<9,"Post-ground log bounded");post_order[post_count++]=1;}
void td_scooter_render(void){require(post_count<9,"Scooter post-ground log bounded");post_order[post_count++]=9;}
void td_sandbox_render(void){require(post_count<9,"Post-ground log bounded");post_order[post_count++]=4;}
UBYTE td_boats_controlled(void){return boat_active;}
void td_boats_render(void){boat_calls++;boat_after_count=used;if(post_count){require(post_count<9,"Post-ground log bounded");post_order[post_count++]=3;}}
static void reset(UBYTE length,UBYTE buffer){
    memset(actors,0,sizeof(actors));memset((void*)shadow_OAM,0,sizeof(shadow_OAM));memset((void*)shadow_OAM2,0,sizeof(shadow_OAM2));
    for(unsigned i=0;i<22;i++){
        sheets[i]=(spritesheet_t){.n_metasprites=1,.metasprites=frames};
        actors[i].sprite=(far_ptr_t){7,&sheets[i]};actors[i].base_tile=i*2;
        actors[i].pos.x=(40+(i%4)*16)*32;actors[i].pos.y=(24+(i/4)*20)*32;
        actors[i].flags=ACTOR_FLAG_ACTIVE;
    }
    actors_len=length;used=boat_calls=boat_after_count=post_count=pre_count=0;boat_active=0;
    allocated_hardware_sprites=0;LCDC_REG=LCDCF_OBJ16;CURRENT_BANK=5;
    draw_scroll_x=draw_scroll_y=scroll_x=scroll_y=0;WX_REG=7;WY_REG=144;show_actors_on_overlay=1;emote_actor=NULL;_is_CGB=1;overlay_priority=0;
    __render_shadow_OAM=(UBYTE)((UWORD)(uintptr_t)(buffer?shadow_OAM2:shadow_OAM)>>8);
}
static void counts(void){
    const unsigned lengths[]={0,1,2,3,12,21,22,23,255};
    for(unsigned b=0;b<2;b++)for(unsigned n=0;n<sizeof(lengths)/sizeof(lengths[0]);n++){
        reset(lengths[n],b);actors_render();
        unsigned count=lengths[n]<22?lengths[n]:22;
        /* Player remains first even on a malformed empty scene. No ACTIVE
         * tail beyond its validated native scene count may be dereferenced. */
        unsigned expected=count?count:1;
        require(used==expected&&order[0]==0,"Scene count bounds every actual ground metadata read despite all tail actors being ACTIVE");
        require(pre_count==4&&pre_order[0]==1&&pre_order[1]==4&&pre_order[2]==3&&pre_order[3]==2,"Actual banked restore dispatch keeps aircraft/scenery order before every ground actor");
        for(unsigned i=1;i<used;i++)require(order[i]==(i==used-1?1:i+1),"Stable slots stay2..last then beacon/keeper1");
        require(allocated_hardware_sprites==expected&&CURRENT_BANK==5,"Scene-limited ground OAM and saved ROM bank stay exact");
        volatile OAM_item_t *oam=b?shadow_OAM2:shadow_OAM;
        require(allocated_hardware_sprites<=40,"Ground scene rendering preserves40-object cap");
        for(int row=0;row<144;row++){
            unsigned objects=0;
            for(unsigned i=0;i<allocated_hardware_sprites;i++)if(row>=(int)oam[i].y-16&&row<(int)oam[i].y)objects++;
            require(objects<=10,"Ground scene rendering preserves10 objects on every scanline");
        }
        require(boat_calls==1&&boat_after_count==used,"Ambient boat remains behind all admitted ground roles");
        require(post_count==9&&post_order[0]==1&&post_order[1]==2&&post_order[2]==8&&post_order[3]==3&&post_order[4]==4&&post_order[5]==9&&post_order[6]==6&&post_order[7]==7&&post_order[8]==5,
                "Actual banked dispatch keeps lights/scenery/ambientboat/sandbox/aircraft order after all ground actors");
    }
}
static void prepared_before_scroll(void){
    for(unsigned buffer=0;buffer<2;buffer++){
        reset(4,buffer);
        /* The real state prepares after simulation, before stock scrolling.
         * Repeated modal preparation and core's later fallback share it. */
        td_actor_render_prepare();
        require(pre_count==4,"Preparing before camera/scroll restores all four overlay owners once");
        td_actor_render_prepare();
        require(pre_count==4,"A second same-cycle preparation performs no overlay dispatch");
        guide_owned=1;guide_force_calls=0;td_actor_render_before();
        require(!guide_owned&&guide_force_calls==1&&pre_count==4,"A modal force releases a retained arrow even after normal preparation");
        actors_render();
        require(pre_count==4&&post_count==9,"Core fallback keeps the prepared underlay and still composes every overlay");
        require(used==4&&allocated_hardware_sprites==4&&CURRENT_BANK==5,"Earlier preparation does not change ground actors or caller bank");
        used=post_count=pre_count=boat_calls=boat_after_count=0;allocated_hardware_sprites=0;
        /* VM lock may skip state_update on the next frame. Core must then
         * restore the last completed overlays itself, before ground OAM. */
        actors_render();
        require(pre_count==4&&post_count==9,"Post-ground completion rearms the skipped-state-update fallback");
    }
}
static void shop_and_roles(void){
    reset(2,0);actors_render();
    require(used==2&&order[0]==0&&order[1]==1&&shadow_OAM[0].tile==0&&shadow_OAM[1].tile==2,
            "Native shop player and keeper are isolated from ACTIVE outdoor cars, people, drivers and tram tail");
    reset(22,1);actors[0].flags|=ACTOR_FLAG_HIDDEN;boat_active=1;actors_render();
    require(used==21&&boat_calls==1&&boat_after_count==0,"Controlled boat retains priority before every outdoor ground role");
    require(post_count==8&&post_order[0]==1&&post_order[1]==2&&post_order[2]==8&&post_order[3]==4&&post_order[4]==9&&post_order[5]==6&&post_order[6]==7&&post_order[7]==5,
            "Controlled launch remains before ground and is never duplicated by banked post-ground dispatch");
    require(order[15]==17&&order[16]==18&&order[17]==19&&order[18]==20&&order[19]==21&&order[20]==1,
            "Full outdoor scene keeps both extra cars, both drivers and separate tram21 before beacon1");
    reset(22,0);actors[17].flags|=ACTOR_FLAG_HIDDEN;actors[20].flags|=ACTOR_FLAG_DISABLED;actors[21].flags&=~ACTOR_FLAG_ACTIVE;actors_render();
    for(unsigned i=0;i<used;i++)require(order[i]!=17&&order[i]!=20&&order[i]!=21,"Hidden/disabled/inactive street roles retain their independent visibility gates");
}
static void overlay_and_pinning(void){
    require(sizeof(screen_x)==1&&sizeof(screen_y)==1,"Host screen coordinate ABI matches native UBYTE width");
    for(unsigned buffer=0;buffer<2;buffer++)for(unsigned mode=0;mode<5;mode++){
        reset(4,buffer);draw_scroll_x=40;draw_scroll_y=20;show_actors_on_overlay=0;_is_CGB=0;WX_REG=64;WY_REG=72;
        actors[0].pos.x=90*32;actors[0].pos.y=90*32;
        actors[1].pos.x=50*32;actors[1].pos.y=30*32;
        actors[2].pos.x=100*32;actors[2].pos.y=100*32;
        actors[3].pos.x=100*32;actors[3].pos.y=100*32;actors[3].flags|=ACTOR_FLAG_PINNED;
        if(mode==1)_is_CGB=1;
        if(mode==2)overlay_priority=S_PRIORITY;
        if(mode==3)show_actors_on_overlay=1;
        if(mode==4)WX_REG=DEVICE_WINDOW_PX_OFFSET_X;
        actors_render();
        volatile OAM_item_t *oam=buffer?shadow_OAM2:shadow_OAM;
        require(order[0]==0&&order[1]==2&&order[used-1]==1,"Ground helper preserves player-first, boundary-visible NPC and marker-last with camera offsets");
        require(oam[1].x==64&&oam[1].y==88,"Ground helper matches native camera-relative coordinates at exact overlay boundary");
        require(used==(mode?4u:3u),"Only the DMG window mask suppresses the pinned actor behind the overlay");
        if(mode)require(order[2]==3&&oam[2].x==104&&oam[2].y==108,"Pinned screen-space sprite ignores camera and keeps exact native origin");
        require(CURRENT_BANK==5,"Banked ground traversal and overlays return the caller bank intact");
    }
    reset(4,0);draw_scroll_x=40;draw_scroll_y=20;show_actors_on_overlay=0;_is_CGB=0;WX_REG=64;WY_REG=72;
    actors[0].pos.x=100*32;actors[0].pos.y=110*32;
    actors[1].pos.x=50*32;actors[1].pos.y=30*32;
    actors[2].pos.x=100*32;actors[2].pos.y=100*32;
    actors[3].pos.x=100*32;actors[3].pos.y=100*32;actors[3].flags|=ACTOR_FLAG_PINNED;
    actors_render();
    require(used==2&&order[0]==2&&order[1]==1,"Player overlay clipping remains independent of the banked NPC loop");
    reset(3,0);show_actors_on_overlay=0;_is_CGB=0;WX_REG=64;WY_REG=72;draw_scroll_x=80;draw_scroll_y=120;
    actors[0].pos.x=20*32;actors[0].pos.y=10*32;
    actors[1].pos.x=100*32;actors[1].pos.y=100*32;
    actors[2].pos.x=20*32;actors[2].pos.y=10*32;
    actors_render();
    require(!used&&!allocated_hardware_sprites,"Native byte-wrap window mask and signed viewport cull hide negative camera positions without any pose reads");
    require(screen_x==20&&screen_y==236,"Marker-last leaves the exact stock byte-wrapped camera coordinate globals");
}
int main(void){counts();prepared_before_scroll();shop_and_roles();overlay_and_pinning();printf("Actual core scene traversal and ground OAM: %u checks, 0 failures\n",checks);return 0;}
