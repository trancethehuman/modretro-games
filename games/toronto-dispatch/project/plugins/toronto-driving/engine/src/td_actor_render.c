#pragma bank 255
#include <string.h>
#include "td_actor_render.h"
#include "actor.h"
#include "data_manager.h"
#include "shadow.h"
#include "scroll.h"
#include "math.h"

#define TD_GROUND_POSE_OBJECTS 8
#define TD_GROUND_POSE_HEIGHT 64

void td_actor_render(const metasprite_t *pose,UBYTE bank,UBYTE base,WORD x,WORD y) BANKED {
    OAM_item_t items[TD_GROUND_POSE_OBJECTS];
    BYTE events[TD_GROUND_POSE_HEIGHT+1],total=0;
    metasprite_t item;
    volatile OAM_item_t *oam;
    WORD ox=x,oy=y,low=144,high=-1,top,bottom;
    UBYTE i,count=0,index,height,span,ground=allocated_hardware_sprites;
    if(!pose||!bank||ground>40||x<-2048||x>2048||y<-2048||y>2048)return;
    height=(LCDC_REG&LCDCF_OBJ16)?16:8;
    /* Preserve cumulative signed metasprite offsets before clipping. X-only
     * hiding still consumes hardware Y slots, so omitted objects get no OAM
     * entry. A wrapped UBYTE coordinate must never cross the opposite edge. */
    for(i=0;i<=TD_GROUND_POSE_OBJECTS;i++){
        MemcpyBanked(&item,pose+i,sizeof(item),bank);
        if(item.dy==metasprite_end)break;
        if(i==TD_GROUND_POSE_OBJECTS)return;
        ox+=item.dx;oy+=item.dy;
        if(ox<=0||ox>=168||oy<=16-height||oy>=160)continue;
        items[count].x=ox;items[count].y=oy;
        items[count].tile=base+item.dtile;items[count].prop=item.props;
        top=oy-16;bottom=top+height-1;
        if(top<low)low=top;
        if(bottom>high)high=bottom;
        count++;
    }
    if(!count||ground+count>40)return;
    if(low<0)low=0;
    if(high>143)high=143;
    if(high<low)return;
    span=high-low+1;
    if(span>TD_GROUND_POSE_HEIGHT)return;
    oam=__render_shadow_OAM==(UBYTE)((UWORD)&shadow_OAM>>8)?shadow_OAM:shadow_OAM2;
    /* Ten objects total cannot exceed ten on any scanline. Avoid repeated
     * row accounting for the first admitted actors in an ordinary view. */
    if(ground+count>10){
        memset(events,0,span+1);
        for(i=0;i<ground+count;i++){
            index=i<ground?oam[i].y:items[i-ground].y;
            top=(WORD)index-16;bottom=top+height;
            if(top<low)top=low;
            if(bottom>high+1)bottom=high+1;
            if(top>=bottom)continue;
            events[top-low]++;events[bottom-low]--;
        }
        for(i=0;i<span;i++){
            total+=events[i];
            if(total>10)return;
        }
    }
    for(i=0;i<count;i++)oam[ground+i]=items[i];
    allocated_hardware_sprites=ground+count;
}

void td_actor_render_actor(actor_t *actor) BANKED {
    const metasprite_t *const *frames;
    const metasprite_t *pose;
    WORD x=SUBPX_TO_PX(actor->pos.x),y=SUBPX_TO_PX(actor->pos.y);
    if(!actor->sprite.bank||!actor->sprite.ptr||
       actor->frame>=ReadBankedUBYTE(&((const spritesheet_t*)actor->sprite.ptr)->n_metasprites,actor->sprite.bank))return;
    MemcpyBanked(&frames,&((const spritesheet_t*)actor->sprite.ptr)->metasprites,sizeof(frames),actor->sprite.bank);
    MemcpyBanked(&pose,frames+actor->frame,sizeof(pose),actor->sprite.bank);
    if(!(actor->flags&ACTOR_FLAG_PINNED)){x-=draw_scroll_x;y-=draw_scroll_y;}
    td_actor_render(pose,actor->sprite.bank,actor->base_tile,x,y);
}
