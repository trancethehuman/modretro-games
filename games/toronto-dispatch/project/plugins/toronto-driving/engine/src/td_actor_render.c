#pragma bank 255
#include <string.h>
#include "td_actor_render.h"
#include "bankdata.h"
#include "actor.h"
#include "data_manager.h"
#include "shadow.h"
#include "scroll.h"
#include "math.h"
#include "macro.h"
#include "td_traffic_lights.h"
#include "td_scenery.h"
#include "td_boats.h"
#include "td_sandbox.h"
#include "td_scooter.h"
#include "td_aircraft_render.h"
#include "td_combat_render.h"
#include "td_guidance.h"
#include "td_hospital.h"

/* Stock actor.c keeps these byte coordinates private to its translation
 * unit. Match its width so window masking retains native wrap semantics. */
extern UBYTE screen_x,screen_y;
/* One render-cycle bit prevents repeated banked restoration after the state
 * has already prepared VRAM for camera/scroll. No saved or gameplay state. */
static UBYTE td_actor_render_restored;

#define TD_GROUND_POSE_OBJECTS 8
#define TD_GROUND_POSE_HEIGHT 64

/* Internal calls remain in this switchable code bank. Keep the public BANKED
 * entry for other units without reselecting this bank for each ground cell. */
static void td_actor_render_local(const metasprite_t *pose,UBYTE bank,UBYTE base,WORD x,WORD y) {
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

void td_actor_render(const metasprite_t *pose,UBYTE bank,UBYTE base,WORD x,WORD y) BANKED {
    td_actor_render_local(pose,bank,base,x,y);
}

/* Only this compact function lives in fixed HOME. Its code remains mapped
 * while reading the selected sprite bank; every path restores the caller.
 * Combine count/table/frame binding into one temporary selection, without
 * caching mutable actor state or reading ahead of a bounded frame index. */
const metasprite_t *td_actor_render_pose(const void *descriptor,UBYTE bank,UBYTE frame) NONBANKED {
    const spritesheet_t *sprite=descriptor;
    UBYTE saved_bank=CURRENT_BANK;
    const metasprite_t *pose=NULL;
    if(!sprite||!bank)return NULL;
    SWITCH_ROM(bank);
    if(frame<sprite->n_metasprites&&sprite->metasprites)pose=sprite->metasprites[frame];
    SWITCH_ROM(saved_bank);
    return pose;
}

static void td_actor_render_actor_local(actor_t *actor) {
    const metasprite_t *pose;
    WORD x=SUBPX_TO_PX(actor->pos.x),y=SUBPX_TO_PX(actor->pos.y);
    if(!(actor->flags&ACTOR_FLAG_PINNED)){x-=draw_scroll_x;y-=draw_scroll_y;}
    /* Reject only when every possible original ground cell is outside the
     * native OAM clip. The compiled gate proves these cumulative bounds for
     * every frame; edge-crossing and pinned sprites retain the full path. */
    if(x+TD_GROUND_MAX_X<=0||x+TD_GROUND_MIN_X>=168||
       y+TD_GROUND_MAX_Y<=0||y+TD_GROUND_MIN_Y>=160)return;
    if(!actor->sprite.bank||!actor->sprite.ptr)return;
    pose=td_actor_render_pose(actor->sprite.ptr,actor->sprite.bank,actor->frame);
    td_actor_render_local(pose,actor->sprite.bank,actor->base_tile,x,y);
}

void td_actor_render_actor(actor_t *actor) BANKED {
    td_actor_render_actor_local(actor);
}

/* Scene-bounded stable ground traversal. Never switch ROM manually while
 * executing banked code; td_actor_render_actor uses bank-safe metadata reads. */
void td_actor_render_ground(UBYTE window_hide_actors) BANKED {
    actor_t *actor;
    UBYTE ground_index,ground_count;
    // Stable runtime slot order: fleet/parked car, people, tram, then marker.
    // The stock activation-list order can rotate when actors enter/leave view.
    ground_count=actors_len<MAX_ACTORS?actors_len:MAX_ACTORS;
    for (ground_index = 2; ground_index <= ground_count; ground_index++) {
        actor = &actors[ground_index == ground_count ? 1 : ground_index];
        if (!CHK_FLAG(actor->flags, ACTOR_FLAG_ACTIVE) ||
            CHK_FLAG(actor->flags, ACTOR_FLAG_HIDDEN | ACTOR_FLAG_DISABLED)) {
           continue;
        }

        if (CHK_FLAG(actor->flags, ACTOR_FLAG_PINNED)) {
            screen_x = SUBPX_TO_PX(actor->pos.x);
            screen_y = SUBPX_TO_PX(actor->pos.y);
        } else {
            screen_x = SUBPX_TO_PX(actor->pos.x) - draw_scroll_x;
            screen_y = SUBPX_TO_PX(actor->pos.y) - draw_scroll_y;
        }

        if (((window_hide_actors) && (((screen_x + 8) > WX_REG) && ((screen_y - 8) > WY_REG)))) {
            continue;
        }
        td_actor_render_actor_local(actor);
    }

}

/* TORONTO prepares once after ordinary simulation, before stock scrolling.
 * The core call remains a fallback when a locked VM skips the state update.
 * Repeated modal/transition/core calls do no additional restoration work. */
static void td_actor_render_prepare_local(UBYTE force){
    if(td_actor_render_restored){
        if(force)td_guidance_road_restore();
        return;
    }
    td_actor_render_restored=1;
    /* Reverse the draw ownership: sky/shadow, muzzle, arrow, scenery. */
    td_aircraft_render_restore();td_combat_render_restore();
    if(force)td_guidance_road_restore();else td_guidance_road_prepare();
    td_scenery_restore();
}
void td_actor_render_before(void) BANKED {td_actor_render_prepare_local(1);}
void td_actor_render_prepare(void) BANKED {td_actor_render_prepare_local(0);}

/* Only these dispatches moved out of fixed bank0. Preserve original overlay
 * order and the ambient/controlled launch distinction after ground OAM. */
void td_actor_render_after(void) BANKED {
    td_traffic_lights_render();
    td_scenery_render();td_hospital_render();
    if(!td_boats_controlled())td_boats_render();
    td_sandbox_render();td_scooter_render();td_guidance_road_render();td_combat_render();
    td_aircraft_render();
    td_actor_render_restored=0;
}
