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
#include "collision.h"

td_boat_state_t td_boat;
#ifdef __SDCC
typedef char td_boat_state_fits[(sizeof(td_boat_state_t)<=40)?1:-1];
#endif
typedef struct { UWORD u,top,bottom; } td_boat_route_t;
typedef struct { UWORD x,y,w,h; } td_boat_rect_t;
typedef struct { UBYTE district;UWORD u,v; } td_boat_dock_t;
static const td_boat_route_t td_boat_routes[2]={{560,840,896},{464,64,432}};
/* Exact authored water rectangles, not surveyed navigation channels. */
static const td_boat_rect_t td_boat_water[8]={
    {0,168,96,760},{96,168,392,48},{432,0,64,528},{96,464,400,72},
    {96,640,704,64},{752,608,112,120},{792,704,32,224},{0,936,1024,40}
};
/* Existing64px bridge/deck coverage; no new pedestrian/road crossing. */
static const td_boat_rect_t td_boat_decks[6]={
    {416,96,96,64},{416,256,96,64},{160,144,64,96},
    {160,448,64,104},{160,624,64,96},{776,792,64,64}
};
static const td_boat_dock_t td_boat_docks[6]={
    {0,560,800},{0,400,800},{0,736,800},{4,424,72},{4,424,352},{4,504,384}
};
static const BYTE td_boat_dx[16]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
static const BYTE td_boat_dy[16]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};

static UWORD td_boat_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static UBYTE td_boat_route(td_boat_route_t *route){
    if(td_boat.district!=0&&td_boat.district!=4)return FALSE;
    *route=td_boat_routes[td_boat.district==4];return TRUE;
}
static UBYTE td_boat_same_scene(void){
    return td_boat.scene.bank==current_scene.bank&&td_boat.scene.ptr==current_scene.ptr;
}
static UBYTE td_boat_live(void){
    return td_boat.bound&&td_boat_same_scene()&&(td_boat.district==0||td_boat.district==4)&&
        (td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE);
}
static UBYTE td_boat_rectangle(const td_boat_rect_t *r,UWORD u,UWORD v){
    return u>=r->x&&u-r->x<r->w&&v>=r->y&&v-r->y<r->h;
}
static UBYTE td_boat_water_point(UWORD u,UWORD v){
    UBYTE i;
    if(u>=1024||v>=976)return FALSE;
    if(td_boat.district==0)return v>=816;
    if(td_boat.district!=4)return FALSE;
    for(i=0;i<8;i++)if(td_boat_rectangle(&td_boat_water[i],u,v))return TRUE;
    return FALSE;
}
static UBYTE td_boat_under_deck(UWORD u,UWORD v){
    UBYTE i;
    if(td_boat.district!=4)return FALSE;
    for(i=0;i<6;i++)if(td_boat_rectangle(&td_boat_decks[i],u,v))return TRUE;
    return FALSE;
}
/* All approved water/deck boundaries are tile-aligned. Check every overlapped
 * tile, not just the corners: an interior railing or land cell stays blocked. */
UBYTE td_boats_under_cover(void) BANKED {
    return td_boats_controlled()&&td_boat_under_deck(td_boat.u>>4,td_boat.v>>4);
}
static UBYTE td_boat_body(UWORD q_u,UWORD q_v,UBYTE heading){
    UWORD u=q_u>>4,v=q_v>>4,px,py;UBYTE x,y,left,right,top,bottom;
    UBYTE vertical=(((heading+2)&15)>>2)&1;
    UBYTE half_x=vertical?8:12,half_y=vertical?12:8;
    if(u<half_x||v<half_y||u>1024-half_x||v>976-half_y)return FALSE;
    left=(u-half_x)>>3;right=(u+half_x-1)>>3;
    top=(v-half_y)>>3;bottom=(v+half_y-1)>>3;
    for(y=top;y<=bottom;y++)for(x=left;x<=right;x++){
        px=(UWORD)x*8+4;py=(UWORD)y*8+4;
        if(!td_boat_water_point(px,py)||
           (tile_at(x,y)!=15&&!td_boat_under_deck(px,py)))return FALSE;
    }
    return TRUE;
}
/* Update-local reuse avoids banked collision reads on each subpixel step.
 * Both geometry and collision are tile-aligned and fixed during boat drive. */
typedef struct {UBYTE left,right,top,bottom,valid,result;} td_boat_terrain_t;
static UBYTE td_boat_cached_body(UWORD q_u,UWORD q_v,UBYTE heading,td_boat_terrain_t *cache){
    UWORD u=q_u>>4,v=q_v>>4;UBYTE vertical=(((heading+2)&15)>>2)&1;
    UBYTE hx=vertical?8:12,hy=vertical?12:8,left,right,top,bottom;
    if(u<hx||v<hy||u>1024-hx||v>976-hy)return FALSE;
    left=(u-hx)>>3;right=(u+hx-1)>>3;top=(v-hy)>>3;bottom=(v+hy-1)>>3;
    if(cache->valid&&cache->left==left&&cache->right==right&&cache->top==top&&cache->bottom==bottom)
        return cache->result;
    cache->left=left;cache->right=right;cache->top=top;cache->bottom=bottom;cache->valid=1;
    cache->result=td_boat_body(q_u,q_v,heading);return cache->result;
}
static UBYTE td_boat_shore(UWORD q_u,UWORD q_v){
    UWORD u=q_u>>4,v=q_v>>4;UBYTE x,y;
    if(u<3||v<3||u>1020||v>972)return FALSE;
    if(td.park_district==td_boat.district&&td_boat_distance(q_u,td.park_u)<192&&
       td_boat_distance(q_v,td.park_v)<192)return FALSE;
    for(y=(v-3)>>3;y<=(v+3)>>3;y++)for(x=(u-3)>>3;x<=(u+3)>>3;x++)
        if(tile_at(x,y)&15)return FALSE;
    return TRUE;
}
/* Normalize packed compiled relative offsets to authored absolute cell x/y.
 * GB Studio emits cell(x-8,-8-y), where source Y grows upward.
 * N/scratch use a2x2 canvas; E has three cells(24x16). */
static UBYTE td_boat_pose(UBYTE frame,metasprite_t *parts){
    const metasprite_t *const *frames;const metasprite_t *pose;metasprite_t item;
    BYTE x=0,y=0;UBYTE i,count=frame==1?3:4,slot,seen=0;
    if(frame>2||ReadBankedUBYTE(&((const spritesheet_t*)td_boat.sprite.ptr)->n_metasprites,td_boat.sprite.bank)!=4)return FALSE;
    MemcpyBanked(&frames,&((const spritesheet_t*)td_boat.sprite.ptr)->metasprites,sizeof(frames),td_boat.sprite.bank);
    MemcpyBanked(&pose,frames+frame,sizeof(pose),td_boat.sprite.bank);
    for(i=0;i<count;i++){
        MemcpyBanked(&item,pose+i,sizeof(item),td_boat.sprite.bank);
        if(item.dy==metasprite_end||(item.dtile&1)||(item.props&0x80))return FALSE;
        x+=item.dx;y+=item.dy;
        item.dx=x+8;item.dy=y+(frame==1?8:24);
        if(frame!=1){
            if((item.dx!=0&&item.dx!=8)||(item.dy!=0&&item.dy!=16))return FALSE;
            slot=(item.dy>>4)*2+(item.dx>>3);
        }else{
            if(item.dy!=0||item.dx<0||item.dx>=(BYTE)(count*8)||(item.dx&7))return FALSE;
            slot=item.dx>>3;
        }
        if(seen&(1<<slot))return FALSE;
        seen|=1<<slot;parts[slot]=item;
    }
    MemcpyBanked(&item,pose+count,sizeof(item),td_boat.sprite.bank);
    return item.dy==metasprite_end&&seen==((1<<count)-1);
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
    UBYTE district=td_district_current(),index=(district==0||district==1||district==3)?5:4,i,j,f;
    actor_t *loader;metasprite_t parts[4],scratch[4];td_boat_route_t route;
    td_boats_reset();
    if(district==TD_DISTRICT_NORTH||district>=TD_DISTRICT_COUNT||actors_len<=index)return;
    loader=&actors[index];
    if(!loader->sprite.bank||!loader->sprite.ptr)return;
    td_boat.sprite=loader->sprite;td_boat.scene=current_scene;td_boat.base=loader->base_tile;td_boat.district=district;
    if(!td_boat_pose(2,scratch))return;
    for(i=0;i<4;i++){
        if(!td_boat_tiles(scratch[i].dtile,scratch[i].props,NULL)||
           (UWORD)td_boat.base+scratch[i].dtile+1>=128||((td_boat.base+scratch[i].dtile)&1))return;
        for(j=0;j<i;j++)if(scratch[j].dtile==scratch[i].dtile&&
                            !((scratch[j].props^scratch[i].props)&8))return;
        td_boat.scratch[i]=scratch[i].dtile;td_boat.scratch_props[i]=scratch[i].props&8;
    }
    for(f=0;f<2;f++){
        if(!td_boat_pose(f,parts))return;
        for(i=0;i<(f?3:4);i++){
            if(!td_boat_tiles(parts[i].dtile,parts[i].props,NULL))return;
            for(j=0;j<4;j++)if(parts[i].dtile==scratch[j].dtile&&
                               !((parts[i].props^scratch[j].props)&8))return;
        }
    }
    td_boat.bound=1;
    if(td_boat_route(&route)){
        td_boat.u=route.u*16;td_boat.v=route.top*16;td_boat.heading=4;td_boat.speed=4;
    }
    if(loader->flags&ACTOR_FLAG_ACTIVE)deactivate_actor(loader);
    if(loader->prev)loader->prev->next=loader->next;
    else if(actors_inactive_head==loader)actors_inactive_head=loader->next;
    if(loader->next)loader->next->prev=loader->prev;
    loader->prev=loader->next=NULL;loader->flags=ACTOR_FLAG_HIDDEN;
}
void td_boats_update(UWORD elapsed) BANKED {
    td_boat_route_t route;UWORD period,step,span,travel;
    if(!td_boat_live()||!td_boat_route(&route)||td_boat.occupied||td_boat.moored)return;
    /* A nearby on-foot boarder gets a stationary launch, without teleporting. */
    if(td.mode==TD_ROAM&&td.onfoot&&td_boat_distance(td.u,td_boat.u)<=56*16&&
       td_boat_distance(td.v,td_boat.v)<=56*16)return;
    period=(route.bottom-route.top)*32;step=(elapsed%(period/4))*4;
    td_boat.phase=(td_boat.phase+step)%period;
    span=period/2;travel=td_boat.phase<span?td_boat.phase:period-td_boat.phase;
    td_boat.u=route.u*16;td_boat.v=route.top*16+travel;
    td_boat.heading=td_boat.phase<span?4:12;
    td_boat.tick+=elapsed;
}
UBYTE td_boats_controlled(void) BANKED {
    return td_boat.bound&&td_boat_same_scene()&&td_boat.occupied&&
        (td_boat.district==0||td_boat.district==4);
}
UBYTE td_boats_interact(UWORD *u,UWORD *v) BANKED {
    UBYTE i;UWORD shore_u,shore_v;
    if(!u||!v||!td_boat_live()||td.mode!=TD_ROAM)return 0;
    if(td_boat.occupied){
        if(td_boat.speed>2||td_boat.speed<-2)return 0;
        for(i=0;i<6;i++){
            if(td_boat_docks[i].district!=td_boat.district)continue;
            shore_u=td_boat_docks[i].u*16;shore_v=td_boat_docks[i].v*16;
            if(td_boat_distance(td_boat.u,shore_u)>56*16||td_boat_distance(td_boat.v,shore_v)>56*16||
               !td_boat_shore(shore_u,shore_v))continue;
            td_boat.occupied=0;td_boat.moored=1;td_boat.speed=0;td_boat.vx=td_boat.vy=0;
            *u=shore_u;*v=shore_v;return 2;
        }
        return 0;
    }
    if(!td.onfoot||!td_boat_shore(*u,*v))return 0;
    for(i=0;i<6;i++){
        if(td_boat_docks[i].district!=td_boat.district)continue;
        shore_u=td_boat_docks[i].u*16;shore_v=td_boat_docks[i].v*16;
        if(td_boat_distance(*u,shore_u)>24*16||td_boat_distance(*v,shore_v)>24*16||
           td_boat_distance(*u,td_boat.u)>56*16||td_boat_distance(*v,td_boat.v)>56*16||
           !td_boat_body(td_boat.u,td_boat.v,td_boat.heading))continue;
        td_boat.shore_u=*u;td_boat.shore_v=*v;td_boat.occupied=1;td_boat.moored=0;
        td_boat.speed=0;td_boat.vx=td_boat.vy=0;td_boat.turn_tick=0;
        *u=td_boat.u;*v=td_boat.v;return 1;
    }
    return 0;
}
UBYTE td_boats_restore_shore(UWORD *u,UWORD *v) BANKED {
    if(!u||!v||!td_boats_controlled()||!td_boat_shore(td_boat.shore_u,td_boat.shore_v))return FALSE;
    *u=td_boat.shore_u;*v=td_boat.shore_v;return TRUE;
}
static WORD td_boat_traction(WORD velocity,WORD target){
    WORD delta=target-velocity;return delta>=-1&&delta<=1?target:velocity+delta/2;
}
void td_boats_drive(UBYTE keys,UWORD elapsed,UWORD *u,UWORD *v) BANKED {
    UBYTE step,heading;WORD nu,nv;td_boat_terrain_t terrain;
    if(!u||!v||!td_boats_controlled()||td.mode!=TD_ROAM||!elapsed)return;
    if(elapsed>16)elapsed=16;
    terrain.valid=0;
    for(step=0;step<elapsed;step++){
        td_boat.tick++;
        if(!!(keys&J_LEFT)!=!!(keys&J_RIGHT)){
            if(++td_boat.turn_tick>=12){
                td_boat.turn_tick=0;heading=(td_boat.heading+((keys&J_LEFT)?15:1))&15;
                if(td_boat_cached_body(td_boat.u,td_boat.v,heading,&terrain))td_boat.heading=heading;
            }
        }else td_boat.turn_tick=0;
        if(keys&J_B){if(!(td_boat.tick&3)&&td_boat.speed>-4)td_boat.speed--;}
        else if(keys&J_A){if(!(td_boat.tick&7)&&td_boat.speed<8)td_boat.speed++;}
        else if(!(td_boat.tick&15)){if(td_boat.speed>0)td_boat.speed--;else if(td_boat.speed<0)td_boat.speed++;}
        td_boat.vx=td_boat_traction(td_boat.vx,td_boat_dx[td_boat.heading]*td_boat.speed/16);
        td_boat.vy=td_boat_traction(td_boat.vy,td_boat_dy[td_boat.heading]*td_boat.speed/16);
        nu=(WORD)td_boat.u+td_boat.vx;nv=(WORD)td_boat.v+td_boat.vy;
        if(nu>=0&&nv>=0&&td_boat_cached_body(nu,nv,td_boat.heading,&terrain)){td_boat.u=nu;td_boat.v=nv;}
        else{
            if(nu>=0&&td_boat_cached_body(nu,td_boat.v,td_boat.heading,&terrain)){td_boat.u=nu;td_boat.vy=0;}
            else if(nv>=0&&td_boat_cached_body(td_boat.u,nv,td_boat.heading,&terrain)){td_boat.v=nv;td_boat.vx=0;}
            else{td_boat.vx=td_boat.vy=0;td_boat.speed=0;}
        }
    }
    *u=td_boat.u;*v=td_boat.v;
}
static UBYTE td_boat_reverse(UBYTE value){
    value=((value&0x55)<<1)|((value>>1)&0x55);
    value=((value&0x33)<<2)|((value>>2)&0x33);return (value<<4)|(value>>4);
}
static void td_boat_cell(const metasprite_t *part,UBYTE orientation,UBYTE frame,WORD *left,WORD *top){
    WORD u=td_boat.u>>4,v=td_boat.v>>4;
    if(frame){*left=u-12+(orientation==0?16-part->dx:part->dx);*top=v-8;}
    else{*left=u-8+part->dx;*top=v+(orientation==1?-4-part->dy:-12+part->dy);}
}
/* Water, deck and world boundaries are8px-aligned. An8px cell row spans
 * at most two such intervals; each whole interval shares one exact mask. */
static UBYTE td_boat_row_mask(WORD left,WORD y){
    UBYTE done=0,span,mask=0,bits;WORD x=left;
    if(y<0||y>=976)return 0;
    while(done<8){
        span=8-(x&7);if(span>8-done)span=8-done;
        bits=(UBYTE)(255>>done)^(UBYTE)(255>>(done+span));
        if(x>=0&&x<1024&&td_boat_water_point(x,y)&&!td_boat_under_deck(x,y))mask|=bits;
        done+=span;x+=span;
    }
    return mask;
}
static UBYTE td_boat_pixels(metasprite_t *part,UBYTE orientation,UBYTE frame,UBYTE *pixels,WORD *left,WORD *top){
    UBYTE r,low,high,mask=0,opaque=0,flip_y,flip_x,wave;
    WORD px,py;
    if(!td_boat_tiles(part->dtile,part->props,pixels))return FALSE;
    /* First normalize compiler flips, then add source-coordinate wake pixels,
     * then orient the resulting cell. Each pass starts from pristine ROM. */
    if(part->props&0x40)for(r=0;r<8;r++){
        low=pixels[r*2];high=pixels[r*2+1];pixels[r*2]=pixels[(15-r)*2];pixels[r*2+1]=pixels[(15-r)*2+1];
        pixels[(15-r)*2]=low;pixels[(15-r)*2+1]=high;
    }
    if(part->props&0x20)for(r=0;r<32;r++)pixels[r]=td_boat_reverse(pixels[r]);
    wave=(!td_boat.occupied&&!td_boat.moored)||td_boat.speed!=0;
    if(wave)for(r=0;r<16;r++){
        py=part->dy+r;mask=0;
        if(!frame&&py>=24&&py<=29&&((py+((td_boat.tick>>3)&1))&1)==0){
            px=3+((py-24)>>1)-part->dx;if(px>=0&&px<8)mask|=128>>px;
            px=12-((py-24)>>1)-part->dx;if(px>=0&&px<8)mask|=128>>px;
        }else if(frame&&part->dx==16&&(py==1||py==14))mask=(td_boat.tick&8)?20:42;
        pixels[r*2]|=mask;pixels[r*2+1]&=~mask;
    }
    /* Runtime orientation acts on normalized pixels, not compiler flags. */
    flip_y=orientation==1;flip_x=orientation==0;
    if(flip_y)for(r=0;r<8;r++){
        low=pixels[r*2];high=pixels[r*2+1];pixels[r*2]=pixels[(15-r)*2];pixels[r*2+1]=pixels[(15-r)*2+1];
        pixels[(15-r)*2]=low;pixels[(15-r)*2+1]=high;
    }
    if(flip_x)for(r=0;r<32;r++)pixels[r]=td_boat_reverse(pixels[r]);
    td_boat_cell(part,orientation,frame,left,top);
    for(r=0;r<16;r++){
        py=*top+r;if(!r||!(py&7))mask=td_boat_row_mask(*left,py);
        pixels[r*2]&=mask;pixels[r*2+1]&=mask;opaque|=pixels[r*2]|pixels[r*2+1];
    }
    return opaque!=0;
}
static UBYTE td_boat_capacity(volatile OAM_item_t *oam,UBYTE ground,const OAM_item_t *items,UBYTE count){
    BYTE events[33],total=0;WORD low=144,high=-1,top,bottom;UBYTE i,span,y;
    if(!count||ground+count>40)return FALSE;
    for(i=0;i<count;i++){top=(WORD)items[i].y-16;bottom=top+15;if(top<low)low=top;if(bottom>high)high=bottom;}
    if(low<0)low=0;if(high>143)high=143;
    if(high<low||high-low>=32)return FALSE;
    if(ground+count<=10)return TRUE;
    span=high-low+1;memset(events,0,span+1);
    for(i=0;i<ground+count;i++){
        y=i<ground?oam[i].y:items[i-ground].y;top=(WORD)y-16;bottom=top+16;
        if(top<low)top=low;if(bottom>high+1)bottom=high+1;
        if(top>=bottom)continue;events[top-low]++;events[bottom-low]--;
    }
    for(i=0;i<span;i++){total+=events[i];if(total>10)return FALSE;}
    return TRUE;
}
static UBYTE td_boat_clear_window(WORD x,WORD y){
    WORD left=win_pos_x<win_dest_pos_x?win_pos_x:win_dest_pos_x;
    WORD top=win_pos_y<win_dest_pos_y?win_pos_y:win_dest_pos_y,hardware_x;
    if(WX_REG&&WY_REG<144){
        hardware_x=(WORD)WX_REG-7;if(hardware_x<0)hardware_x=0;
        if(hardware_x<left)left=hardware_x;if(WY_REG<top)top=WY_REG;
    }
    return left>=160||top>=144||x-1<left||y-1<top;
}
void td_boats_render(void) BANKED {
#ifdef CGB
    metasprite_t parts[4];UBYTE pixels[32],indices[4],i,count=0,frame,orientation,save_vbk;
    WORD left,top,x,y;OAM_item_t items[4];volatile OAM_item_t *oam;
    if(!td_boat_live())return;
    orientation=((td_boat.heading+2)&15)>>2;frame=(orientation&1)?0:1;
    /* Cull the complete original hull/wake bounds before reading any sprite
     * metadata or composing pixels. Each orientation's bounds include all
     *32source rows even where only the animated wake uses the final rows. */
    left=(WORD)(td_boat.u>>4)-(frame?12:8)-draw_scroll_x;
    top=(WORD)(td_boat.v>>4)-(frame?8:orientation==1?20:12)-draw_scroll_y;
    if(left+(frame?24:16)<=0||left>=160||top+(frame?16:32)<=0||top>=144||
       !td_boat_clear_window(left+8,top+16))return;
    if(!td_boat_pose(frame,parts))return;
    for(i=0;i<(frame?3:4);i++){
        td_boat_cell(&parts[i],orientation,frame,&left,&top);
        x=left+8-draw_scroll_x;y=top+16-draw_scroll_y;
        if(x<=0||x>=168||y<=0||y>=160)continue;
        if(!td_boat_pixels(&parts[i],orientation,frame,pixels,&left,&top))continue;
        if(!td_boat_clear_window(x,y))return;
        items[count].x=x;items[count].y=y;items[count].tile=td_boat.base+td_boat.scratch[i];
        items[count].prop=td_boat.scratch_props[i]|7;indices[count++]=i;
    }
    oam=__render_shadow_OAM==(UBYTE)((UWORD)&shadow_OAM>>8)?shadow_OAM:shadow_OAM2;
    if(!td_boat_capacity(oam,allocated_hardware_sprites,items,count))return;
    save_vbk=VBK_REG&1;
    for(i=0;i<count;i++){
        td_boat_pixels(&parts[indices[i]],orientation,frame,pixels,&left,&top);
        VBK_REG=!!(td_boat.scratch_props[indices[i]]&8);
        set_sprite_data(td_boat.base+td_boat.scratch[indices[i]],2,pixels);
        oam[allocated_hardware_sprites++]=items[i];
    }
    VBK_REG=save_vbk;
#endif
}
