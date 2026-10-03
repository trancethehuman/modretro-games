/* Independent PNG-pixel, OAM and lifecycle checks on unchanged native C. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "host_boats.h"
#include "td_game.h"
#include "td_boats.h"
#include "boat_art_oracle.h"

actor_t actors[21],*actors_inactive_head;
UBYTE actors_len,allocated_hardware_sprites,VBK_REG,__render_shadow_OAM;
UBYTE win_pos_x,win_pos_y,win_dest_pos_x,win_dest_pos_y,WX_REG,WY_REG;
_Alignas(256) volatile OAM_item_t shadow_OAM[40];
_Alignas(256) volatile OAM_item_t shadow_OAM2[40];
WORD draw_scroll_x,draw_scroll_y;
far_ptr_t current_scene;
td_state_t td;
static UBYTE district,scene_ids[6],uploads;
static tileset_t rom_tiles[2];
static metasprite_t poses[3][2];
static const metasprite_t *frames[3];
static spritesheet_t sheet={3,frames,{8,&rom_tiles[0]},{9,&rom_tiles[1]}};
static UBYTE obj_data[2][128][16];
static unsigned long checks;
static void require(int condition,const char *message){
    checks++;
    if(!condition){fprintf(stderr,"FAIL after %lu checks: %s\n",checks,message);exit(1);}
}
UBYTE td_district_current(void){return district;}
void deactivate_actor(actor_t *actor){
    actor->flags&=~ACTOR_FLAG_ACTIVE;
    actor->next=actors_inactive_head;actor->prev=NULL;
    if(actors_inactive_head)actors_inactive_head->prev=actor;
    actors_inactive_head=actor;
}
void MemcpyBanked(void *dest,const void *src,size_t length,UBYTE bank){
    require(bank>=7&&bank<=9,"Unexpected compiled ROM bank");memcpy(dest,src,length);
}
UBYTE ReadBankedUBYTE(const UBYTE *src,UBYTE bank){
    require(bank==7,"Frame count must come from the captured metadata bank");return *src;
}
void set_sprite_data(UBYTE first,UBYTE count,const UBYTE *pixels){
    require(first==34&&count==2&&VBK_REG<2,"Boat upload escaped its independent scratch pair");
    memcpy(obj_data[VBK_REG][first],pixels,32);uploads++;
}

#include "boats_under_test.c"

static UBYTE loader_index(UBYTE area){return area==0||area==1||area==3?5:4;}
static UWORD top(UBYTE area){return area==4?64:840;}
static UWORD bottom(UBYTE area){return area==4?432:896;}
static UWORD centre_u(UBYTE area){return area==4?464:560;}
static void fixture(UBYTE area,UBYTE flip,UBYTE bank){
    UBYTE x,y,colour,index=loader_index(area),frame;
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));
    memset(rom_tiles,0,sizeof(rom_tiles));memset(obj_data,0x5a,sizeof(obj_data));
    memset((void*)shadow_OAM,0,sizeof(shadow_OAM));memset((void*)shadow_OAM2,0,sizeof(shadow_OAM2));
    memset(poses,0,sizeof(poses));sheet.n_metasprites=3;
    sheet.tileset.bank=8;sheet.cgb_tileset.bank=9;
    for(frame=0;frame<3;frame++)frames[frame]=poses[frame];
    poses[0][0]=(metasprite_t){-8,-4,0,(UBYTE)((flip<<5)|(bank<<3))};
    poses[0][1].dy=metasprite_end;
    poses[1][0]=(metasprite_t){-8,-4,2,(UBYTE)(bank<<3)};
    poses[1][1].dy=metasprite_end;poses[2][0].dy=metasprite_end;
    rom_tiles[0].n_tiles=rom_tiles[1].n_tiles=4;
    for(y=0;y<16;y++)for(x=0;x<8;x++){
        colour=original_pixels[flip&2?15-y:y][flip&1?7-x:x];
        if(colour&1)rom_tiles[bank].tiles[y*2]|=128>>x;
        if(colour&2)rom_tiles[bank].tiles[y*2+1]|=128>>x;
    }
    district=area;actors_len=16;current_scene=(far_ptr_t){10,&scene_ids[area]};
    actors[index].sprite=(far_ptr_t){7,&sheet};actors[index].base_tile=32;
    actors_inactive_head=&actors[1];actors[1].next=&actors[index];
    actors[index].prev=&actors[1];actors[index].next=&actors[index+1];actors[index+1].prev=&actors[index];
    td.mode=TD_ROAM;td.cash=37;td.u=888;td.v=999;td.park_u=123;td.park_v=456;
    td_boats_reset();allocated_hardware_sprites=uploads=0;VBK_REG=1;
    __render_shadow_OAM=(UBYTE)((UWORD)(uintptr_t)shadow_OAM>>8);
    win_pos_x=win_dest_pos_x=0;win_pos_y=win_dest_pos_y=144;WX_REG=0;WY_REG=144;
    draw_scroll_x=centre_u(area)-76;draw_scroll_y=top(area)-68;
}
static void bind_ok(void){
    UBYTE index=loader_index(district);td_boats_bind();
    require(td_boat.bound&&td_boat.base==32&&td_boat.scratch==2,"Valid native loader was not captured");
    require(actors[1].next==&actors[index+1]&&actors[index+1].prev==&actors[1]&&
            !actors[index].next&&!actors[index].prev&&actors[index].flags==ACTOR_FLAG_HIDDEN,
            "Captured empty loader remained in the actor list");
}
static UBYTE object_pixel(OAM_item_t object,UBYTE x,UBYTE y){
    const UBYTE *row=obj_data[(object.prop>>3)&1][(object.tile&0xfe)+(y>>3)]+(y&7)*2;
    return ((row[0]>>(7-x))&1)|(((row[1]>>(7-x))&1)<<1);
}
static int under_deck(UBYTE area,UWORD y){
    return area==4&&((y>=96&&y<160)||(y>=256&&y<320));
}
static void pixel_checks(void){
    UBYTE area,flip,bank,x,y,south,expected,visible,base_uploads;
    UWORD phase,period,half,travel,v;td_state_t original;
    OAM_item_t object;
    for(area=0;area<=4;area+=4)for(flip=0;flip<4;flip++)for(bank=0;bank<2;bank++){
        fixture(area,flip,bank);bind_ok();original=td;
        period=(bottom(area)-top(area))*32;half=period/2;
        /* Seven Q4 units visits both directions and subpixel fractions while
         * avoiding a test that merely repeats the renderer's integer stride. */
        for(phase=0;phase<period;phase+=7){
            td_boat.phase=phase;south=phase<half;
            travel=south?phase:period-phase;v=top(area)+travel/16;
            draw_scroll_y=v-68;allocated_hardware_sprites=0;base_uploads=uploads;VBK_REG=1;
            visible=0;
            for(y=0;y<16;y++)for(x=0;x<8;x++)
                if(original_pixels[south?15-y:y][x]&&!under_deck(area,v-8+y))visible=1;
            td_boats_render();
            require(!memcmp(&td,&original,sizeof(td)),"Cosmetic renderer modified courier/save state");
            require(VBK_REG==1,"Scratch upload failed to restore incoming VRAM bank");
            require(allocated_hardware_sprites==visible,"Bridge clipping emitted a fully hidden boat or lost a visible one");
            require(uploads==(UBYTE)(base_uploads+visible),"Hidden boat uploaded stale or unnecessary scratch pixels");
            if(!visible)continue;
            object=shadow_OAM[0];
            require(object.x==80&&object.y==76&&object.tile==34&&object.prop==(UBYTE)((bank<<3)|7),
                    "Boat screen origin, palette or scratch ownership changed");
            for(y=0;y<16;y++)for(x=0;x<8;x++){
                expected=under_deck(area,v-8+y)?0:original_pixels[south?15-y:y][x];
                require(object_pixel(object,x,y)==expected,"Independent PNG pixel oracle disagrees with normalized/deck-clipped hull");
            }
        }
    }
}
static void capacity_checks(void){
    UBYTE count,overlap,i,effective,expected;volatile OAM_item_t *oam;
    fixture(0,0,0);bind_ok();
    for(count=0;count<=40;count++)for(overlap=0;overlap<=11;overlap++){
        effective=overlap<count?overlap:count;
        memset((void*)shadow_OAM,0,sizeof(shadow_OAM));
        for(i=0;i<count;i++){shadow_OAM[i].y=i<effective?76:150;shadow_OAM[i].x=0;}
        allocated_hardware_sprites=count;uploads=0;td_boats_render();
        expected=count<40&&effective<10;
        require(allocated_hardware_sprites==count+expected,"Boat displaced ground OAM or crossed the40/10 hardware limits");
        require(uploads==expected,"Capacity rejection must omit both OBJ and scratch upload");
    }
    /* Edge-Y sweep independently counts every object, including X-hidden OBJ.
     * Test both double buffers so admission cannot inspect the wrong frame. */
    for(i=0;i<2;i++){
        UWORD candidate,ground;UBYTE j,peak;
        oam=i?shadow_OAM2:shadow_OAM;
        for(candidate=1;candidate<160;candidate++)for(ground=0;ground<160;ground+=7){
            memset((void*)oam,0,40*sizeof(OAM_item_t));
            for(j=0;j<10;j++){oam[j].x=0;oam[j].y=ground;}
            peak=0;
            for(j=0;j<144;j++)if((WORD)j>=(WORD)candidate-16&&j<candidate&&
                                      (WORD)j>=(WORD)ground-16&&j<ground)peak=1;
            require(td_boat_capacity(oam,10,candidate)==(!peak&&candidate>0&&candidate<160),
                    "Local event sweep disagrees with hardware-Y row oracle");
        }
        __render_shadow_OAM=(UBYTE)((UWORD)(uintptr_t)oam>>8);
        allocated_hardware_sprites=0;td_boats_render();
        require(allocated_hardware_sprites==1&&oam[0].tile==34,"Boat wrote into the wrong shadow OAM buffer");
    }
}
static void lifecycle_checks(void){
    UBYTE area,mode,index;UWORD period;td_state_t original;
    for(area=0;area<5;area++){
        fixture(area,0,0);bind_ok();original=td;
        td_boats_update(60);
        require(td_boat.phase==((area==0||area==4)?240:0),"Unsupported district animated a water route");
        require(!memcmp(&td,&original,sizeof(td)),"Boat clock modified courier/save state");
        for(mode=0;mode<=TD_HELP;mode++){
            td.mode=mode;td_boat.phase=0;allocated_hardware_sprites=uploads=0;
            td_boats_update(1);td_boats_render();
            require(td_boat.phase==((area==0||area==4)&&(mode==TD_ROAM||mode==TD_WAIT||mode==TD_RIDE)?4:0),
                    "Boat failed active-mode/pause clock gating");
            require(allocated_hardware_sprites==((area==0||area==4)&&(mode==TD_ROAM||mode==TD_WAIT||mode==TD_RIDE)),
                    "Boat failed active-mode/pause presentation gating");
        }
        td.mode=TD_ROAM;td_boat.phase=123;current_scene.bank++;
        td_boats_update(600);allocated_hardware_sprites=0;td_boats_render();
        require(td_boat.phase==123&&!allocated_hardware_sprites,"Stale scene bank binding remained live");
        current_scene.bank--;current_scene.ptr=&scene_ids[5];td_boats_update(600);td_boats_render();
        require(td_boat.phase==123&&!allocated_hardware_sprites,"Stale scene pointer binding remained live");
        fixture(area,0,0);index=loader_index(area);actors[index].flags=ACTOR_FLAG_ACTIVE;
        actors[index].prev=actors[index].next=NULL;actors[1].next=NULL;
        td_boats_bind();require(td_boat.bound&&actors_inactive_head==&actors[1],"Active boat loader was not deactivated and removed");
    }
    for(area=0;area<=4;area+=4){
        fixture(area,0,0);bind_ok();period=(bottom(area)-top(area))*32;td_boat.phase=period-1;
        td_boats_update(65535);
        require(td_boat.phase==((uint32_t)period-1+65535u*4)%period,"Large active elapsed value overflowed boat phase");
    }
    fixture(0,0,0);bind_ok();td_boats_reset();td_boats_update(60);td_boats_render();
    require(!td_boat.bound&&!allocated_hardware_sprites,"Explicit reset retained an old boat binding");
}
static void invalid_binding_checks(void){
    UBYTE scenario;
    for(scenario=0;scenario<13;scenario++){
        fixture(0,0,0);
        switch(scenario){
            case 0:actors_len=5;break;
            case 1:actors[5].sprite.bank=0;break;
            case 2:actors[5].sprite.ptr=NULL;break;
            case 3:sheet.n_metasprites=2;break;
            case 4:poses[0][0].dy=metasprite_end;break;
            case 5:poses[1][1].dy=0;break;
            case 6:poses[0][0].dtile=1;break;
            case 7:poses[0][0].props=0x80;break;
            case 8:poses[1][0].dtile=0;break;
            case 9:sheet.tileset.bank=0;break;
            case 10:rom_tiles[0].n_tiles=129;break;
            case 11:rom_tiles[0].n_tiles=3;break;
            case 12:actors[5].base_tile=125;break;
        }
        td_boats_bind();td_boats_update(60);td_boats_render();
        require(!td_boat.bound&&!allocated_hardware_sprites&&!uploads,"Malformed or missing native loader did not fail closed");
    }
}
static void window_checks(void){
    fixture(0,0,0);bind_ok();
    win_dest_pos_y=64;td_boats_render();
    require(!allocated_hardware_sprites&&!uploads,"Boat crossed an approaching UI window");
    win_dest_pos_y=144;WX_REG=7;WY_REG=64;td_boats_render();
    require(!allocated_hardware_sprites&&!uploads,"Boat crossed the actual hardware UI window");
    WX_REG=0;WY_REG=144;draw_scroll_x=centre_u(0)+4;td_boats_render();
    require(!allocated_hardware_sprites&&!uploads,"Offscreen boat enlarged the OAM sweep");
}
int main(void){
    require((UBYTE)((UWORD)(uintptr_t)shadow_OAM>>8)!=(UBYTE)((UWORD)(uintptr_t)shadow_OAM2>>8),
            "Host OAM buffers must occupy distinct hardware-page adapters");
    pixel_checks();capacity_checks();lifecycle_checks();invalid_binding_checks();window_checks();
    printf("Boat production C: %lu checks passed; native allocation/play and hardware remain separate.\n",checks);
    return 0;
}
