#pragma bank 255
#include <string.h>
#include "td_boats.h"
#include "td_game.h"
#include "td_district.h"
#include "actor.h"
#include "data_manager.h"
#include "gbs_types.h"
#include "scroll.h"
#include "shadow.h"
#include "ui.h"
#include "compat.h"

td_boat_state_t td_boat;
#ifdef __SDCC
typedef char td_boat_state_fits[(sizeof(td_boat_state_t)<=20)?1:-1];
#endif
typedef struct { UWORD u,top,bottom; } td_boat_route_t;
/* Original compressed water lanes, not surveyed navigation channels. Hull8x16.
 * Core harbour stops north of Island land. Port Don passes under the supported
 * Lake Shore and Commissioners decks; neither route adds a crossing. */
static const td_boat_route_t td_boat_routes[2]={{560,840,896},{464,64,432}};
static UBYTE td_boat_route(td_boat_route_t *route){
    if(td_boat.district!=0&&td_boat.district!=4)return FALSE;
    *route=td_boat_routes[td_boat.district==4];return TRUE;
}
static UBYTE td_boat_same_scene(void){
    return td_boat.scene.bank==current_scene.bank&&td_boat.scene.ptr==current_scene.ptr;
}
static UBYTE td_boat_live(void){
    return td_boat.bound&&td_boat_same_scene()&&
        (td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE);
}
static UBYTE td_boat_pose(UBYTE frame,metasprite_t *item){
    const metasprite_t *const *frames;const metasprite_t *pose;metasprite_t end;
    UBYTE count=ReadBankedUBYTE(&((const spritesheet_t*)td_boat.sprite.ptr)->n_metasprites,td_boat.sprite.bank);
    if(count!=3||frame>1)return FALSE;
    MemcpyBanked(&frames,&((const spritesheet_t*)td_boat.sprite.ptr)->metasprites,sizeof(frames),td_boat.sprite.bank);
    MemcpyBanked(&pose,frames+frame,sizeof(pose),td_boat.sprite.bank);
    MemcpyBanked(item,pose,sizeof(*item),td_boat.sprite.bank);
    MemcpyBanked(&end,pose+1,sizeof(end),td_boat.sprite.bank);
    return item->dy!=metasprite_end&&end.dy==metasprite_end&&!(item->dtile&1)&&
        !((item->props)&0x80);
}
static UBYTE td_boat_tiles(UBYTE tile,UBYTE props,UBYTE *pixels){
    far_ptr_t ref;const tileset_t *tileset;UWORD count;
    MemcpyBanked(&ref,props&8?&((const spritesheet_t*)td_boat.sprite.ptr)->cgb_tileset:
                 &((const spritesheet_t*)td_boat.sprite.ptr)->tileset,sizeof(ref),td_boat.sprite.bank);
    if(!ref.bank||!ref.ptr)return FALSE;
    tileset=ref.ptr;MemcpyBanked(&count,&tileset->n_tiles,sizeof(count),ref.bank);
    if((UWORD)tile+1>=count||count>128)return FALSE;
    if(pixels)MemcpyBanked(pixels,tileset->tiles+(UWORD)tile*16,32,ref.bank);
    return TRUE;
}
void td_boats_reset(void) BANKED {memset(&td_boat,0,sizeof(td_boat));}
void td_boats_bind(void) BANKED {
    UBYTE district=td_district_current(),index=(district==0||district==1||district==3)?5:4;
    actor_t *loader;metasprite_t hull,scratch;
    td_boats_reset();
    if(district==TD_DISTRICT_NORTH||district>=TD_DISTRICT_COUNT||actors_len<=index)return;
    loader=&actors[index];
    if(!loader->sprite.bank||!loader->sprite.ptr)return;
    td_boat.sprite=loader->sprite;td_boat.scene=current_scene;td_boat.base=loader->base_tile;td_boat.district=district;
    if(!td_boat_pose(0,&hull)||!td_boat_pose(1,&scratch)||
       (hull.dtile==scratch.dtile&&((hull.props^scratch.props)&8)==0)||
       !td_boat_tiles(hull.dtile,hull.props,NULL)||!td_boat_tiles(scratch.dtile,scratch.props,NULL)||
       (UWORD)td_boat.base+scratch.dtile+1>=128||((td_boat.base+scratch.dtile)&1))return;
    td_boat.scratch=scratch.dtile;td_boat.scratch_props=scratch.props&8;td_boat.bound=1;
    if(loader->flags&ACTOR_FLAG_ACTIVE)deactivate_actor(loader);
    if(loader->prev)loader->prev->next=loader->next;
    else if(actors_inactive_head==loader)actors_inactive_head=loader->next;
    if(loader->next)loader->next->prev=loader->prev;
    loader->prev=loader->next=NULL;loader->flags=ACTOR_FLAG_HIDDEN;
}
void td_boats_update(UWORD elapsed) BANKED {
    td_boat_route_t route;UWORD period,step;
    if(!td_boat_live()||!td_boat_route(&route))return;
    /* 4Q4 units/VBlank=15px/s at nominal60Hz. Both periods fitUWORD. */
    period=(route.bottom-route.top)*32;
    step=(elapsed%(period/4))*4;
    td_boat.phase=(td_boat.phase+step)%period;
}
static UBYTE td_boat_reverse(UBYTE value){
    value=((value&0x55)<<1)|((value>>1)&0x55);
    value=((value&0x33)<<2)|((value>>2)&0x33);
    return (value<<4)|(value>>4);
}
static UBYTE td_boat_under_deck(UWORD u,UWORD v){
    /* Full64px walking deck widths; clipped OBJ pixels cannot show on either
     * roadway or railing, even during an aircraft's temporary BKG roof mask. */
    return td_boat.district==4&&u>=416&&u<512&&
        ((v>=96&&v<160)||(v>=256&&v<320));
}
static UBYTE td_boat_capacity(volatile OAM_item_t *oam,UBYTE count,UBYTE y){
    BYTE events[17],total=1;UBYTE i,span;WORD low=(WORD)y-16,high=(WORD)y-1,top,bottom;
    if(count>=40||high<0||low>=144)return FALSE;
    if(low<0)low=0;
    if(high>143)high=143;
    span=high-low+1;memset(events,0,span+1);
    for(i=0;i<count;i++){
        top=(WORD)oam[i].y-16;bottom=top+16;
        if(top<low)top=low;
        if(bottom>high+1)bottom=high+1;
        if(top>=bottom)continue;
        events[top-low]++;events[bottom-low]--;
    }
    for(i=0;i<span;i++){total+=events[i];if(total>10)return FALSE;}
    return TRUE;
}
static UBYTE td_boat_clear_window(WORD x,WORD y){
    WORD left=win_pos_x<win_dest_pos_x?win_pos_x:win_dest_pos_x;
    WORD top=win_pos_y<win_dest_pos_y?win_pos_y:win_dest_pos_y,hardware_x;
    if(WX_REG&&WY_REG<144){
        hardware_x=(WORD)WX_REG-7;if(hardware_x<0)hardware_x=0;
        if(hardware_x<left)left=hardware_x;
        if(WY_REG<top)top=WY_REG;
    }
    return left>=160||top>=144||x-1<left||y-1<top;
}
void td_boats_render(void) BANKED {
#ifdef CGB
    td_boat_route_t route;metasprite_t hull;UBYTE pixels[32],r,low,high,mask,column,opaque=0,save_vbk;
    UWORD span,travel,v;WORD x,y;volatile OAM_item_t *oam;UBYTE south;
    if(!td_boat_live()||!td_boat_route(&route))return;
    span=(route.bottom-route.top)*16;
    south=td_boat.phase<span;travel=south?td_boat.phase:span*2-td_boat.phase;
    v=route.top+(travel>>4);x=(WORD)route.u+4-draw_scroll_x;y=(WORD)v+8-draw_scroll_y;
    if(x<=0||x>=168||y<=0||y>=160||!td_boat_clear_window(x,y))return;
    oam=__render_shadow_OAM==(UBYTE)((UWORD)&shadow_OAM>>8)?shadow_OAM:shadow_OAM2;
    if(!td_boat_capacity(oam,allocated_hardware_sprites,y)||!td_boat_pose(0,&hull)||
       !td_boat_tiles(hull.dtile,hull.props,pixels))return;
    /* Normalize compiler flips/direction in one local buffer, always from ROM;
     * clipping never accumulates and needs no persistent mask cache. */
    if((!!(hull.props&0x40))^south)for(r=0;r<8;r++){
        low=pixels[r*2];high=pixels[r*2+1];pixels[r*2]=pixels[(15-r)*2];pixels[r*2+1]=pixels[(15-r)*2+1];
        pixels[(15-r)*2]=low;pixels[(15-r)*2+1]=high;
    }
    for(r=0;r<16;r++){
        if(hull.props&0x20){pixels[r*2]=td_boat_reverse(pixels[r*2]);pixels[r*2+1]=td_boat_reverse(pixels[r*2+1]);}
        mask=255;
        for(column=0;column<8;column++)if(td_boat_under_deck(route.u-4+column,v-8+r))mask&=~(128>>column);
        pixels[r*2]&=mask;pixels[r*2+1]&=mask;opaque|=pixels[r*2]|pixels[r*2+1];
    }
    if(!opaque)return;
    save_vbk=VBK_REG&1;VBK_REG=!!(td_boat.scratch_props&8);
    set_sprite_data(td_boat.base+td_boat.scratch,2,pixels);VBK_REG=save_vbk;
    oam[allocated_hardware_sprites].x=x;oam[allocated_hardware_sprites].y=y;
    oam[allocated_hardware_sprites].tile=td_boat.base+td_boat.scratch;
    oam[allocated_hardware_sprites].prop=td_boat.scratch_props|7;
    allocated_hardware_sprites++;
#endif
}
