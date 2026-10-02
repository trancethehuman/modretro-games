#pragma bank 255
#include <string.h>
#include "td_aircraft_render.h"
#include "td_aircraft.h"
#include "td_game.h"
#include "td_district.h"
#include "actor.h"
#include "data_manager.h"
#include "gbs_types.h"
#include "scroll.h"
#include "shadow.h"
#include "ui.h"
#include "compat.h"

/* Gameplay's bank-1 BKG allocation is at most 16 tiles. IDs 32..46 are
 * signed BKG tiles, physically separate from bank-1 OBJ tiles 0..127.
 * The atlas owns these IDs only while flybys are suppressed/restored. */
#define TD_AIRCRAFT_SCRATCH_FIRST 32
#define TD_AIRCRAFT_PATCHES 15
#define TD_AIRCRAFT_OBJECTS 4
#define TD_AIRCRAFT_SHADOWS 2
typedef struct { UWORD offset; UBYTE tile,attr; } td_aircraft_patch_t;
typedef struct { BYTE x,y; UBYTE tile,props; } td_aircraft_object_t;
typedef struct { td_aircraft_object_t objects[4]; UBYTE masks[4][16]; } td_aircraft_pose_t;
static td_aircraft_patch_t td_aircraft_patches[TD_AIRCRAFT_PATCHES];
static UBYTE td_aircraft_patch_count,td_aircraft_base,td_aircraft_bound;
static far_ptr_t td_aircraft_sprite,td_aircraft_scene;
static UBYTE *td_aircraft_map;
/* At most two rotor poses per flight. Templates/masks come from compiled ROM
 * once, while the new flight is still offscreen, rather than reading OBJ
 * VRAM and banked metadata during every visible frame. 170 bytes total. */
static td_aircraft_pose_t td_aircraft_poses[2];
static td_aircraft_object_t td_aircraft_shadow[2];
static UBYTE td_aircraft_cache_ready,td_aircraft_cache_frame;
typedef char td_aircraft_pose_size[(sizeof(td_aircraft_pose_t)==80)?1:-1];

static UBYTE td_aircraft_same_scene(void){
    return current_scene.bank==td_aircraft_scene.bank&&current_scene.ptr==td_aircraft_scene.ptr;
}
static UBYTE td_aircraft_patch_attr(UBYTE attr){return (attr&0x9f)|8;}
static UBYTE td_aircraft_reverse(UBYTE value){
    value=((value&0x55)<<1)|((value>>1)&0x55);
    value=((value&0x33)<<2)|((value>>2)&0x33);
    return (value<<4)|(value>>4);
}

void td_aircraft_render_reset(void) BANKED {
    /* A scene load has replaced graphics. Never restore old tile references
     * into that new scene, even if its tilemap occupies the same VRAM page. */
    td_aircraft_patch_count=td_aircraft_bound=td_aircraft_cache_ready=0;
}

void td_aircraft_render_bind(void) BANKED {
    UBYTE district=td_district_current(),index=district==2?1:2;
    actor_t *loader=&actors[index];
    td_aircraft_cache_ready=0;
    td_aircraft_bound=district<TD_DISTRICT_COUNT&&actors_len>index&&
        loader->sprite.bank&&loader->sprite.ptr;
    if(!td_aircraft_bound)return;
    td_aircraft_sprite=loader->sprite;td_aircraft_base=loader->base_tile;
    td_aircraft_scene=current_scene;
    /* deactivate_actor moves an active loader to the inactive list. Remove
     * it there in either case, without losing the preceding Queen loader. */
    if(loader->flags&ACTOR_FLAG_ACTIVE)deactivate_actor(loader);
    if(loader->prev)loader->prev->next=loader->next;
    else if(actors_inactive_head==loader)actors_inactive_head=loader->next;
    if(loader->next)loader->next->prev=loader->prev;
    loader->prev=loader->next=NULL;
    loader->flags=ACTOR_FLAG_HIDDEN;
}

void td_aircraft_render_restore(void) BANKED {
#ifdef CGB
    UBYTE i,tile,attr,save_vbk=VBK_REG&1;
    td_aircraft_patch_t *patch;
    if(!td_aircraft_patch_count)return;
    if(td_aircraft_same_scene())for(i=0;i<td_aircraft_patch_count;i++){
        patch=&td_aircraft_patches[i];
        VBK_REG=0;tile=get_vram_byte(td_aircraft_map+patch->offset);
        VBK_REG=1;attr=get_vram_byte(td_aircraft_map+patch->offset);
        /* Scroll/UI may already have repainted this cell. Its new contents
         * win; only references still owned by this flight are restored. */
        if(tile==TD_AIRCRAFT_SCRATCH_FIRST+i&&attr==td_aircraft_patch_attr(patch->attr)){
            set_vram_byte(td_aircraft_map+patch->offset,patch->attr);
            VBK_REG=0;set_vram_byte(td_aircraft_map+patch->offset,patch->tile);
        }
    }
    td_aircraft_patch_count=0;VBK_REG=save_vbk;
#endif
}

/* Cache native cumulative coordinates, actual compiled tile references and
 * flips. Sprite VRAM mirrors these same primary/CGB ROM tilesets. */
static UBYTE td_aircraft_cache_pose(UBYTE frame,td_aircraft_object_t *objects,
                                   UBYTE expected,UBYTE masks[4][16]){
    const metasprite_t *const *frames;
    const metasprite_t *pose;
    const tileset_t *tileset;far_ptr_t ref;
    metasprite_t item;UBYTE i,count,row,index,mask,pixels[32];
    WORD x=0,y=0;UWORD n_tiles;
    count=ReadBankedUBYTE(&((const spritesheet_t*)td_aircraft_sprite.ptr)->n_metasprites,td_aircraft_sprite.bank);
    if(frame>=count)return FALSE;
    MemcpyBanked(&frames,&((const spritesheet_t*)td_aircraft_sprite.ptr)->metasprites,sizeof(frames),td_aircraft_sprite.bank);
    MemcpyBanked(&pose,frames+frame,sizeof(pose),td_aircraft_sprite.bank);
    for(i=0;i<=expected;i++){
        MemcpyBanked(&item,pose+i,sizeof(item),td_aircraft_sprite.bank);
        if(item.dy==metasprite_end)return i==expected;
        if(i==expected)return FALSE;
        x+=item.dx;y+=item.dy;
        if(x<-128||x>127||y<-128||y>127||(item.dtile&1))return FALSE;
        objects[i].x=x;objects[i].y=y;objects[i].tile=item.dtile;objects[i].props=item.props&0x7f;
        if(!masks)continue;
        MemcpyBanked(&ref,item.props&8?&((const spritesheet_t*)td_aircraft_sprite.ptr)->cgb_tileset:
                     &((const spritesheet_t*)td_aircraft_sprite.ptr)->tileset,sizeof(ref),td_aircraft_sprite.bank);
        if(!ref.bank||!ref.ptr)return FALSE;
        tileset=ref.ptr;
        MemcpyBanked(&n_tiles,&tileset->n_tiles,sizeof(n_tiles),ref.bank);
        if((UWORD)item.dtile+1>=n_tiles)return FALSE;
        MemcpyBanked(pixels,tileset->tiles+(UWORD)item.dtile*16,32,ref.bank);
        for(row=0;row<16;row++){
            index=(item.props&0x40?15-row:row)*2;
            mask=pixels[index]|pixels[index+1];
            masks[i][row]=item.props&0x20?td_aircraft_reverse(mask):mask;
        }
    }
    return FALSE;
}

static UBYTE td_aircraft_cache(UBYTE frame){
    UBYTE base=frame<4?frame:4+((frame-4)&3);
    if(frame>=12)return FALSE;
    if(td_aircraft_cache_ready&&td_aircraft_cache_frame==base)return TRUE;
    td_aircraft_cache_ready=0;
    if(!td_aircraft_cache_pose(base,td_aircraft_poses[0].objects,4,td_aircraft_poses[0].masks)||
       (base>=4&&!td_aircraft_cache_pose(base+4,td_aircraft_poses[1].objects,4,td_aircraft_poses[1].masks))||
       !td_aircraft_cache_pose(12,td_aircraft_shadow,2,NULL))return FALSE;
    td_aircraft_cache_frame=base;td_aircraft_cache_ready=1;return TRUE;
}

/* Clip signed positions before casting to OAM, so flybys cannot wrap through
 * the opposite viewport edge. Cache coordinates remain relative to centre. */
static void td_aircraft_objects(const td_aircraft_object_t *pose,OAM_item_t *objects,
                                UBYTE count,WORD x,WORD y){
    UBYTE i;WORD ox,oy;
    for(i=0;i<count;i++){
        ox=x+pose[i].x;oy=y+pose[i].y;
        objects[i].tile=(td_aircraft_base+pose[i].tile)&0xfe;objects[i].prop=pose[i].props;
        if(ox<=0||ox>=168||oy<=0||oy>=160){objects[i].x=objects[i].y=0;}
        else{objects[i].x=ox;objects[i].y=oy;}
    }
}

static volatile OAM_item_t *td_aircraft_oam(void){
    /* GBDK metasprite calls target __render_shadow_OAM, not the buffer
     * currently on screen. Respect the stock engine's double buffering. */
    return __render_shadow_OAM==(UBYTE)((UWORD)&shadow_OAM>>8)?shadow_OAM:shadow_OAM2;
}

static UBYTE td_aircraft_clear_window(const OAM_item_t *objects,UBYTE count){
    UBYTE i;WORD x=win_pos_x<win_dest_pos_x?win_pos_x:win_dest_pos_x;
    WORD y=win_pos_y<win_dest_pos_y?win_pos_y:win_dest_pos_y,hardware_x;
    /* UI update follows actors_render. Protect both current and destination
     * rectangles, plus the currently displayed hardware window, so a slide or
     * an instant position change cannot expose cosmetic sprites over text. */
    if(WX_REG&&WY_REG<144){
        hardware_x=(WORD)WX_REG-7;if(hardware_x<0)hardware_x=0;
        if(hardware_x<x)x=hardware_x;if(WY_REG<y)y=WY_REG;
    }
    if(x>=160||y>=144)return TRUE;
    for(i=0;i<count;i++)if(objects[i].y&&
        (WORD)objects[i].x-1>=x&&(WORD)objects[i].y-1>=y)return FALSE;
    return TRUE;
}

static UBYTE td_aircraft_capacity(const OAM_item_t *objects,UBYTE count,
                                 volatile OAM_item_t *oam,UBYTE ground){
    BYTE events[65],total=0;UBYTE i,span,y;WORD low=144,high=-1,top,bottom;
    if(ground+count>40)return FALSE;
    for(i=0;i<count;i++)if(objects[i].y&&objects[i].y<160){
        top=(WORD)objects[i].y-16;bottom=top+15;
        if(top<low)low=top;if(bottom>high)high=bottom;
    }
    if(high<0||low>=144)return FALSE;
    if(low<0)low=0;if(high>143)high=143;
    span=high-low+1;if(span>64)return FALSE;
    memset(events,0,span+1);
    /* Each item adds an interval. A prefix sum gives exact hardware Y-only
     * occupancy in O(items+height), including X-hidden ground objects. The
     * bounded40 total guarantees every signed byte event/count is safe. */
    for(i=0;i<ground+count;i++){
        y=i<ground?oam[i].y:objects[i-ground].y;
        top=(WORD)y-16;bottom=top+16;
        if(top<low)top=low;if(bottom>high+1)bottom=high+1;
        if(top>=bottom)continue;
        events[top-low]++;events[bottom-low]--;
    }
    for(i=0;i<span;i++){
        total+=events[i];
        if(total>10)return FALSE;
    }
    return TRUE;
}

#ifdef CGB
static UBYTE td_aircraft_roofs(const OAM_item_t *objects,const UBYTE masks[4][16]){
    UBYTE base[16],tile,attr,x,y,i,row,index,mask,changed;
    WORD left=32767,top=32767,right=-32767,bottom=-32767,ox,oy,dx,dy,tile_screen_x,tile_screen_y;
    UWORD offset;td_aircraft_patch_t *patch;
    for(i=0;i<TD_AIRCRAFT_OBJECTS;i++)if(objects[i].y){
        ox=(WORD)objects[i].x-8+draw_scroll_x;oy=(WORD)objects[i].y-16+draw_scroll_y;
        if(ox<left)left=ox;if(oy<top)top=oy;
        if(ox+7>right)right=ox+7;if(oy+15>bottom)bottom=oy+15;
    }
    if(right<left||bottom<top)return FALSE;
    /* The compiled cardinal poses fit 32x16 or16x32. Reject malformed art
     * instead of partially punching a larger footprint with15 scratch slots. */
    if(right-left>31||bottom-top>31||(right-left>15&&bottom-top>15))return FALSE;
    if(left<draw_scroll_x)left=draw_scroll_x;if(top<draw_scroll_y)top=draw_scroll_y;
    if(right>draw_scroll_x+159)right=draw_scroll_x+159;
    if(bottom>draw_scroll_y+143)bottom=draw_scroll_y+143;
    if(left<0)left=0;if(top<0)top=0;
    if(right>1023)right=1023;if(bottom>975)bottom=975;
    if(right<left||bottom<top)return FALSE;
    td_aircraft_map=GetBkgAddr();
    for(y=top>>3;y<=bottom>>3;y++)for(x=left>>3;x<=right>>3;x++){
        offset=((UWORD)(y&31)<<5)|(x&31);
        VBK_REG=1;attr=get_vram_byte(td_aircraft_map+offset);
        if(!(attr&0x80))continue;
        tile_screen_x=(WORD)x*8-draw_scroll_x+8;
        tile_screen_y=(WORD)y*8-draw_scroll_y+16;
        VBK_REG=0;tile=get_vram_byte(td_aircraft_map+offset);
        VBK_REG=(attr>>3)&1;get_bkg_data(tile,1,base);
        /* Normalize CGB tile flips before replacing with unflipped scratch.
         * Palette, priority and every uncovered original pixel stay intact. */
        if(attr&0x40)for(row=0;row<4;row++){
            mask=base[row*2];base[row*2]=base[(7-row)*2];base[(7-row)*2]=mask;
            mask=base[row*2+1];base[row*2+1]=base[(7-row)*2+1];base[(7-row)*2+1]=mask;
        }
        if(attr&0x20)for(row=0;row<16;row++)base[row]=td_aircraft_reverse(base[row]);
        changed=FALSE;
        for(i=0;i<TD_AIRCRAFT_OBJECTS;i++)if(objects[i].y){
            dx=(WORD)objects[i].x-tile_screen_x;dy=tile_screen_y-(WORD)objects[i].y;
            if(dx<=-8||dx>=8||dy<=-8||dy>=16)continue;
            for(row=0;row<8;row++){
                index=dy+row;if(index>=16)continue;
                mask=masks[i][index];mask=dx<0?mask<<(-dx):mask>>dx;
                if(mask&(base[row*2]|base[row*2+1])){
                    base[row*2]&=~mask;base[row*2+1]&=~mask;changed=TRUE;
                }
            }
        }
        if(!changed)continue;
        if(td_aircraft_patch_count==TD_AIRCRAFT_PATCHES){td_aircraft_render_restore();return FALSE;}
        index=td_aircraft_patch_count;patch=&td_aircraft_patches[index];
        patch->offset=offset;patch->tile=tile;patch->attr=attr;
        VBK_REG=1;set_bkg_data(TD_AIRCRAFT_SCRATCH_FIRST+index,1,base);
        set_vram_byte(td_aircraft_map+offset,td_aircraft_patch_attr(attr));
        VBK_REG=0;set_vram_byte(td_aircraft_map+offset,TD_AIRCRAFT_SCRATCH_FIRST+index);
        td_aircraft_patch_count++;
    }
    return TRUE;
}
#endif

void td_aircraft_render(void) BANKED {
#ifdef CGB
    OAM_item_t objects[TD_AIRCRAFT_OBJECTS+TD_AIRCRAFT_SHADOWS];
    UBYTE count=TD_AIRCRAFT_OBJECTS,i,ground,frame,save_vbk=VBK_REG&1;
    volatile OAM_item_t *oam;
    const td_aircraft_pose_t *pose;
    WORD x,y;
    if(!td_aircraft_bound||!td_aircraft_same_scene()||!td_aircraft.active||
       (td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE))return;
    frame=td_aircraft_frame();if(!td_aircraft_cache(frame))return;
    pose=&td_aircraft_poses[frame>=8?1:0];
    x=(td_aircraft.u>>4)-draw_scroll_x;y=(td_aircraft.v>>4)-draw_scroll_y;
    td_aircraft_objects(pose->objects,objects,TD_AIRCRAFT_OBJECTS,x,y);
    if(!td_aircraft_clear_window(objects,TD_AIRCRAFT_OBJECTS))return;
    ground=allocated_hardware_sprites;oam=td_aircraft_oam();
    td_aircraft_objects(td_aircraft_shadow,objects+TD_AIRCRAFT_OBJECTS,TD_AIRCRAFT_SHADOWS,x+8,y+24);
    /* Most flybys fit with their shadow. Check that combined occupancy once;
     * only crowded/window-limited frames need the aircraft-only fallback. */
    if(td_aircraft_clear_window(objects+TD_AIRCRAFT_OBJECTS,TD_AIRCRAFT_SHADOWS)&&
       td_aircraft_capacity(objects,TD_AIRCRAFT_OBJECTS+TD_AIRCRAFT_SHADOWS,oam,ground))
        count+=TD_AIRCRAFT_SHADOWS;
    else if(!td_aircraft_capacity(objects,count,oam,ground))return;
    if(!td_aircraft_roofs(objects,pose->masks)){VBK_REG=save_vbk;return;}
    /* Earlier CGB OAM wins sprite overlaps. Put opaque aircraft first while
     * keeping ground order, then append the optional shadow behind actors.
     * CGB BKG priority hides it under roofs; OBJ bit7 would hide it on roads. */
    for(i=ground;i;i--)oam[i+TD_AIRCRAFT_OBJECTS-1]=oam[i-1];
    for(i=0;i<TD_AIRCRAFT_OBJECTS;i++)oam[i]=objects[i];
    for(i=TD_AIRCRAFT_OBJECTS;i<count;i++)oam[ground+i]=objects[i];
    allocated_hardware_sprites=ground+count;VBK_REG=save_vbk;
#endif
}
