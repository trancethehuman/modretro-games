#pragma bank 255
#include <string.h>
#include "td_streetcar_runtime.h"
#include "td_streetcar.h"
#include "td_game.h"
#include "td_transit.h"
#include "td_district.h"
#include "actor.h"
#include "data_manager.h"
#include "gbs_types.h"
#include "compat.h"

UWORD td_streetcar_focus_u,td_streetcar_focus_v;
UBYTE td_streetcar_view_district,td_streetcar_ride_view;
static td_streetcar_pose_t td_streetcar_display;
static UWORD td_streetcar_elapsed;
static UBYTE td_streetcar_bound,td_streetcar_valid,td_streetcar_was_ride,td_streetcar_cue;

typedef char td_streetcar_actor_fits_pool[(TD_STREETCAR_ACTOR<MAX_ACTORS)?1:-1];

/* Original cross-street approaches in the registered pixel graphs. These
 * are game parking recovery anchors, not real Toronto parking permissions.
 * Queen row is included so East's32px bend retains its connected road path. */
typedef struct {UWORD u,v,row;UBYTE district;} td_streetcar_anchor_t;
static const td_streetcar_anchor_t td_streetcar_anchors[]={
    {80,480,528,0},{80,576,528,0},{208,480,528,0},{208,576,528,0},
    {336,480,528,0},{336,576,528,0},{480,480,528,0},{480,576,528,0},
    {560,480,528,0},{560,576,528,0},{640,480,528,0},{640,576,528,0},
    {720,480,528,0},{720,576,528,0},{816,480,528,0},{816,576,528,0},
    {944,480,528,0},{944,576,528,0},{800,480,528,1},{912,480,528,1},
    {224,480,528,3},{384,480,528,3},{544,480,528,3},
    {704,448,496,3},{816,448,496,3},{944,448,496,3}
};
#define TD_STREETCAR_ANCHORS (sizeof(td_streetcar_anchors)/sizeof(td_streetcar_anchors[0]))

static UWORD td_streetcar_runtime_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static UBYTE td_streetcar_runtime_mode(void){
    return td.mode==TD_HELP||td.mode==TD_PAUSE||td.mode==TD_MAP?td_resume_mode:td.mode;
}
static UBYTE td_streetcar_runtime_box(UWORD u,UWORD v,UWORD hx,UWORD hy,UBYTE district,
                             td_streetcar_box_t *box){
    if(district>=TD_DISTRICT_COUNT||u>=1024*16||v>=976*16||u<hx||v<hy||
       u+hx>=1024*16||v+hy>=976*16)return FALSE;
    box->left=u-hx;box->right=u+hx;box->top=v-hy;box->bottom=v+hy;box->district=district;
    return TRUE;
}
static UBYTE td_streetcar_runtime_overlap(const td_streetcar_box_t *a,const td_streetcar_box_t *b){
    return a->district==b->district&&a->left<=b->right&&a->right>=b->left&&
        a->top<=b->bottom&&a->bottom>=b->top;
}
static UBYTE td_streetcar_runtime_near(const td_streetcar_box_t *box){
    UWORD min_v,max_v,min_u,max_u;
    if(box->district==2)return FALSE;
    min_u=10*16;max_u=1014*16;
    if(box->district==1){min_u=822*16;min_v=506*16;max_v=550*16;}
    else if(box->district==0){min_v=514*16;max_v=542*16;}
    else{max_u=894*16;min_v=474*16;max_v=550*16;}
    return box->right>=min_u&&box->left<=max_u&&box->bottom>=min_v&&box->top<=max_v;
}
static UBYTE td_streetcar_runtime_clear(UWORD u,UWORD v,UWORD half,UBYTE district,UWORD elapsed){
    td_streetcar_box_t box;
    if(td.subsecond>=60||!td_streetcar_runtime_box(u,v,half,half,district,&box))return FALSE;
    return !td_streetcar_runtime_near(&box)||
        td_streetcar_sweep(td.seconds,td.subsecond,elapsed,&box)==TD_STREETCAR_CLEAR;
}

void td_streetcar_runtime_reset(void) BANKED {
    td_streetcar_bound=td_streetcar_valid=td_streetcar_cue=0;td_streetcar_elapsed=0;
    td_streetcar_was_ride=td_streetcar_runtime_mode()==TD_RIDE;
    td_streetcar_focus_u=td.u;td_streetcar_focus_v=td.v;
    td_streetcar_view_district=td.district;td_streetcar_ride_view=0;
}

UBYTE td_streetcar_runtime_bind(void) BANKED {
    actor_t *loader=&actors[1],*tram=&actors[TD_STREETCAR_ACTOR];
    UBYTE district=td_district_current();
    td_streetcar_bound=actors_len>1&&(district==0||district==1||district==3);
    *tram=td_streetcar_bound?*loader:PLAYER;
    if(td_streetcar_bound){
        /* Remove the authored loader before the caller reuses slot1. It may
         * be inactive because its scriptless source position is off-screen. */
        if(loader->flags&ACTOR_FLAG_ACTIVE)deactivate_actor(loader);
        if(loader->prev)loader->prev->next=loader->next;
        else if(actors_inactive_head==loader)actors_inactive_head=loader->next;
        if(loader->next)loader->next->prev=loader->prev;
        loader->prev=loader->next=NULL;
    }
    tram->flags=ACTOR_FLAG_PERSISTENT|ACTOR_FLAG_HIDDEN;tram->collision_group=0;
    tram->script.bank=tram->script_update.bank=0;
    tram->script.ptr=tram->script_update.ptr=NULL;
    tram->hscript_update=tram->hscript_hit=0;tram->prev=NULL;
    tram->next=actors_inactive_head;if(actors_inactive_head)actors_inactive_head->prev=tram;
    actors_inactive_head=tram;activate_actor(tram);
    if(td_streetcar_bound)actor_set_frames(tram,8,9);
    return td_streetcar_bound;
}

void td_streetcar_runtime_prepare(UWORD elapsed) BANKED {
    UBYTE ride=td_streetcar_runtime_mode()==TD_RIDE&&
        td.transit_origin>=TD_TRANSIT_QUEEN_FIRST&&
        td.transit_origin<TD_TRANSIT_QUEEN_FIRST+TD_TRANSIT_QUEEN_COUNT;
    td_streetcar_elapsed=elapsed;
    td_streetcar_focus_u=td.u;td_streetcar_focus_v=td.v;
    td_streetcar_view_district=td.district;td_streetcar_ride_view=0;
    td_streetcar_valid=td_streetcar_pose(td.seconds,td.subsecond,&td_streetcar_display);
    if(ride&&((td.reserved&TD_STREETCAR_HOLD)?
        td_streetcar_destination(td.transit_origin,td.transit_target,&td_streetcar_display):
        td_streetcar_ride(td.transit_origin,td.transit_target,td.ride_left,
                         td.seconds,td.subsecond,&td_streetcar_display))){
        td_streetcar_valid=td_streetcar_ride_view=1;
        td_streetcar_focus_u=td_streetcar_display.u;td_streetcar_focus_v=td_streetcar_display.v;
        td_streetcar_view_district=td_streetcar_display.district;
        if(!td_streetcar_was_ride&&!(td.reserved&TD_STREETCAR_HOLD))td_streetcar_cue=12;
    }
    if(!ride)td_streetcar_cue=0;
    else if(td.mode==TD_RIDE&&elapsed){
        td_streetcar_cue=elapsed>=td_streetcar_cue?0:td_streetcar_cue-elapsed;
    }
    td_streetcar_was_ride=ride;
}

static WORD td_streetcar_runtime_cue_offset(WORD delta,UBYTE cue){
    if(cue>12)cue=12;
    /* Keep both products inside signed16 even for distant saved origins.
       The quotient is integral, so splitting preserves truncation exactly. */
    return (delta/12)*cue+(delta%12)*cue/12;
}

void td_streetcar_runtime_present(void) BANKED {
    actor_t *tram=&actors[TD_STREETCAR_ACTOR];UWORD u,v;WORD du,dv;
    if(td.mode==TD_MAP)return;
    if(td_streetcar_bound&&td_streetcar_valid&&td_streetcar_display.district==td_district_current()){
        tram->pos.x=td_streetcar_display.u*2;tram->pos.y=td_streetcar_display.v*2;
        /* Stock actor bounds share its Q5 position units. */
        tram->bounds.left=td_streetcar_display.heading&4?-6*32:-14*32;
        tram->bounds.right=(td_streetcar_display.heading&4?6*32:14*32)-1;
        tram->bounds.top=td_streetcar_display.heading&4?-14*32:-6*32;
        tram->bounds.bottom=(td_streetcar_display.heading&4?14*32:6*32)-1;
        actor_set_frames(tram,td_streetcar_display.frame,td_streetcar_display.frame+1);
        tram->anim_tick=255;tram->flags&=~(ACTOR_FLAG_HIDDEN|ACTOR_FLAG_DISABLED);
    }else tram->flags|=ACTOR_FLAG_HIDDEN;
    if(td_streetcar_ride_view){
        /* Saved job/depot routing belongs to the boarding scene. The booked
           stop remains in the HUD/atlas; no origin beacon floats in this view. */
        actors[1].flags|=ACTOR_FLAG_HIDDEN;
        PLAYER.pos.x=td_streetcar_focus_u*2;PLAYER.pos.y=td_streetcar_focus_v*2;
        if(td_streetcar_cue){
            /* Presentation-only northward door step; no fare/state delay and
             * no origin coordinate is written back into the serialized td. */
            u=td_streetcar_display.u;v=td_streetcar_display.v+14*16;
            du=(WORD)td.u-(WORD)u;dv=(WORD)td.v-(WORD)v;
            u+=td_streetcar_runtime_cue_offset(du,td_streetcar_cue);
            v+=td_streetcar_runtime_cue_offset(dv,td_streetcar_cue);
            PLAYER.pos.x=u*2;PLAYER.pos.y=v*2;
            actor_set_frames(&PLAYER,38+((td_streetcar_cue>>2)&1),39+((td_streetcar_cue>>2)&1));
            PLAYER.anim_tick=255;PLAYER.flags&=~(ACTOR_FLAG_HIDDEN|ACTOR_FLAG_DISABLED);
        }else PLAYER.flags|=ACTOR_FLAG_HIDDEN;
    }else PLAYER.flags&=~ACTOR_FLAG_HIDDEN;
}

UBYTE td_streetcar_runtime_foot_clear(UWORD u,UWORD v) BANKED {
    return td_streetcar_runtime_clear(u,v,3*16,td.district,td_streetcar_elapsed);
}
UBYTE td_streetcar_runtime_car_clear(UWORD old_u,UWORD old_v,UWORD u,UWORD v) BANKED {
    td_streetcar_box_t box;
    if(!td_streetcar_runtime_box(u,v,5*16,5*16,td.district,&box)||old_u>=1024*16||old_v>=976*16||
       old_u<5*16||old_v<5*16||
       old_u+5*16>=1024*16||old_v+5*16>=976*16)return FALSE;
    if(old_u-5*16<box.left)box.left=old_u-5*16;
    if(old_u+5*16>box.right)box.right=old_u+5*16;
    if(old_v-5*16<box.top)box.top=old_v-5*16;
    if(old_v+5*16>box.bottom)box.bottom=old_v+5*16;
    return td.subsecond<60&&(!td_streetcar_runtime_near(&box)||
        td_streetcar_sweep(td.seconds,td.subsecond,td_streetcar_elapsed,&box)==TD_STREETCAR_CLEAR);
}
UBYTE td_streetcar_runtime_traffic_clear(UBYTE district,UWORD u,UWORD v) BANKED {
    td_streetcar_box_t box;UWORD future=td.seconds+1;
    if(td.subsecond>=60||!td_streetcar_runtime_box(u,v,5*16,5*16,district,&box))return FALSE;
    /* Yield before entering the rails, not only after the tram swept past.
     * The one-second future interval remains exact across clock rollover. */
    return !td_streetcar_runtime_near(&box)||
        td_streetcar_sweep(future,td.subsecond,60,&box)==TD_STREETCAR_CLEAR;
}
UBYTE td_streetcar_runtime_parking_allowed(UBYTE district,UWORD u,UWORD v) BANKED {
    return td_streetcar_runtime_clear(u,v,5*16,district,TD_STREETCAR_PERIOD_TICKS);
}

static UBYTE td_streetcar_runtime_terrain(const scene_t *scene,UWORD u,UWORD v,UBYTE foot){
    UBYTE x,y,left,right,top,bottom,value;
    if(u>=1024*16||v>=976*16)return FALSE;
    u>>=4;v>>=4;
    if(foot){
        if(u<3||v<3||u>1020||v>972)return FALSE;
        left=(u-3)>>3;right=(u+3)>>3;top=(v-3)>>3;bottom=(v+3)>>3;
    }
    else{
        if(u<8||v<8||u>1016||v>968)return FALSE;
        left=(u-5)>>3;right=(u+5)>>3;top=(v-5)>>3;bottom=(v+5)>>3;
    }
    for(y=top;y<=bottom;y++)for(x=left;x<=right;x++){
        value=ReadBankedUBYTE((const UBYTE*)scene->collisions.ptr+(UWORD)y*scene->width+x,scene->collisions.bank);
        if(foot?(value&15):value)return FALSE;
    }
    return TRUE;
}
static UBYTE td_streetcar_runtime_scene(UBYTE district,scene_t *scene){
    far_ptr_t ref;
    if(!td_district_scene(district,&ref))return FALSE;
    MemcpyBanked(scene,ref.ptr,sizeof(*scene),ref.bank);
    return scene->width==128&&scene->height==122&&scene->collisions.ptr!=NULL;
}
static UBYTE td_streetcar_runtime_others(UBYTE district,UWORD u,UWORD v,UBYTE foot){
    UBYTE i;UWORD radius;
    if(foot&&td.park_district==district&&td_streetcar_runtime_distance(u,td.park_u)<168&&
       td_streetcar_runtime_distance(v,td.park_v)<168)return FALSE;
    if(district!=td_district_current())return TRUE;
    for(i=2;i<15;i++){
        radius=i<8?180:foot?96:160;
        if(i==8)continue;
        if(i>=9&&(actors[i].flags&ACTOR_FLAG_HIDDEN))continue;
        if(td_streetcar_runtime_distance(u,actors[i].pos.x>>1)<radius&&
           td_streetcar_runtime_distance(v,actors[i].pos.y>>1)<radius)return FALSE;
    }
    return TRUE;
}
static UBYTE td_streetcar_runtime_path(const scene_t *scene,UBYTE district,UWORD from_u,UWORD from_v,
                              UWORD to_u,UWORD to_v,UBYTE foot){
    WORD dx=(WORD)(to_u>>4)-(WORD)(from_u>>4),dy=(WORD)(to_v>>4)-(WORD)(from_v>>4);
    UWORD steps=dx<0?-dx:dx,length=dy<0?-dy:dy,i,u,v;
    if(length>steps)steps=length;
    if(steps>256)return FALSE;
    if(!steps)return td_streetcar_runtime_terrain(scene,to_u,to_v,foot)&&td_streetcar_runtime_others(district,to_u,to_v,foot);
    for(i=1;i<=steps;i++){
        /* At most256*256 can overflow signed16; cardinal anchor legs have
         * one zero delta, and separation legs are at most48 pixels. */
        u=dx==0?from_u:dy==0?(dx>0?from_u+i*16:from_u-i*16):
            ((WORD)(from_u>>4)+dx*(WORD)i/(WORD)steps)*16;
        v=dy==0?from_v:dx==0?(dy>0?from_v+i*16:from_v-i*16):
            ((WORD)(from_v>>4)+dy*(WORD)i/(WORD)steps)*16;
        if(!td_streetcar_runtime_terrain(scene,u,v,foot)||!td_streetcar_runtime_others(district,u,v,foot))return FALSE;
    }
    return td_streetcar_runtime_terrain(scene,to_u,to_v,foot)&&td_streetcar_runtime_others(district,to_u,to_v,foot);
}
static UWORD td_streetcar_runtime_anchor_distance(UWORD u,UWORD v,const td_streetcar_anchor_t *a){
    UWORD row=a->district==3&&v<=512*16?496:528;
    UWORD distance=td_streetcar_runtime_distance(v,row*16)+td_streetcar_runtime_distance(a->v*16,a->row*16);
    if(row==a->row)return distance+td_streetcar_runtime_distance(u,a->u*16);
    return distance+td_streetcar_runtime_distance(u,672*16)+32*16+td_streetcar_runtime_distance(672*16,a->u*16);
}
static UBYTE td_streetcar_runtime_anchor_path(const scene_t *scene,UBYTE district,UWORD u,UWORD v,
                                    const td_streetcar_anchor_t *a,UBYTE foot){
    UWORD row=district==3&&v<=512*16?496:528,nu=u,nv=row*16;
    if(!td_streetcar_runtime_path(scene,district,u,v,nu,nv,foot))return FALSE;
    v=nv;
    if(row!=a->row){
        nu=672*16;if(!td_streetcar_runtime_path(scene,district,u,v,nu,v,foot))return FALSE;
        u=nu;
        nv=a->row*16;if(!td_streetcar_runtime_path(scene,district,u,v,u,nv,foot))return FALSE;
        v=nv;
    }
    nu=a->u*16;nv=a->v*16;
    return td_streetcar_runtime_path(scene,district,u,v,nu,v,foot)&&
        td_streetcar_runtime_path(scene,district,nu,v,nu,nv,foot)&&
        td_streetcar_runtime_clear(nu,nv,foot?3*16:5*16,district,TD_STREETCAR_PERIOD_TICKS);
}
static UBYTE td_streetcar_runtime_find_anchor(const scene_t *scene,UBYTE district,UWORD u,UWORD v,
                                    UBYTE foot,UWORD *out_u,UWORD *out_v){
    UBYTE tried[4]={0,0,0,0},attempt,i,best;UWORD distance,nearest;
    /* Three closest candidates bound cold/contact recovery to three short
     *verified approaches. Every successful endpoint is off the rails. */
    for(attempt=0;attempt<3;attempt++){
        best=TD_NONE;nearest=256*16+1;
        for(i=0;i<TD_STREETCAR_ANCHORS;i++)if(!(tried[i>>3]&(1<<(i&7)))&&td_streetcar_anchors[i].district==district){
            distance=td_streetcar_runtime_anchor_distance(u,v,&td_streetcar_anchors[i]);
            if(distance<nearest){nearest=distance;best=i;}
        }
        if(best==TD_NONE)return FALSE;
        tried[best>>3]|=1<<(best&7);
        if(td_streetcar_runtime_anchor_path(scene,district,u,v,&td_streetcar_anchors[best],foot)){
            *out_u=td_streetcar_anchors[best].u*16;*out_v=td_streetcar_anchors[best].v*16;return TRUE;
        }
    }
    return FALSE;
}
UBYTE td_streetcar_runtime_recover_park(void) BANKED {
    scene_t scene;UWORD u,v;
    if(!td.onfoot||td_streetcar_runtime_parking_allowed(td.park_district,td.park_u,td.park_v))return TD_STREETCAR_PARK_UNCHANGED;
    if(!td_streetcar_runtime_scene(td.park_district,&scene)||
       !td_streetcar_runtime_find_anchor(&scene,td.park_district,td.park_u,td.park_v,FALSE,&u,&v))return TD_STREETCAR_PARK_BLOCKED;
    td.park_u=u;td.park_v=v;return TD_STREETCAR_PARK_MOVED;
}
static UBYTE td_streetcar_runtime_separate(const scene_t *scene,const td_streetcar_box_t *tram,
                                 WORD dx,WORD dy,UBYTE foot,UWORD *out_u,UWORD *out_v){
    WORD u=(WORD)td.u+dx*16,v=(WORD)td.v+dy*16;td_streetcar_box_t candidate;
    if(u<0||v<0||!td_streetcar_runtime_box(u,v,foot?3*16:5*16,foot?3*16:5*16,td.district,&candidate)||
       td_streetcar_runtime_overlap(&candidate,tram)||!td_streetcar_runtime_path(scene,td.district,td.u,td.v,u,v,foot))return FALSE;
    *out_u=u;*out_v=v;return TRUE;
}
UBYTE td_streetcar_runtime_recover_contact(UBYTE onfoot) BANKED {
    scene_t scene;td_streetcar_pose_t pose;td_streetcar_box_t tram,body;
    WORD radius,dx,dy;UWORD u,v;
    if(onfoot>1||onfoot!=td.onfoot)return TD_STREETCAR_PARK_BLOCKED;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT)return TD_STREETCAR_PARK_UNCHANGED;
    if(!td_streetcar_pose(td.seconds,td.subsecond,&pose)||!td_streetcar_bounds(&pose,&tram)||
       !td_streetcar_runtime_box(td.u,td.v,onfoot?3*16:5*16,onfoot?3*16:5*16,td.district,&body))return TD_STREETCAR_PARK_BLOCKED;
    if(!td_streetcar_runtime_overlap(&tram,&body))return TD_STREETCAR_PARK_UNCHANGED;
    if(!td_streetcar_runtime_scene(td.district,&scene))return TD_STREETCAR_PARK_BLOCKED;
    /* Manhattan rings visit the nearest connected4px-grid escape first.
     * Four axes are included; mixed candidates handle a blocked corner. */
    for(radius=4;radius<=48;radius+=4)for(dx=-radius;dx<=radius;dx+=4){
        dy=radius-(dx<0?-dx:dx);
        if(td_streetcar_runtime_separate(&scene,&tram,dx,dy,onfoot,&u,&v)||
           (dy&&td_streetcar_runtime_separate(&scene,&tram,dx,-dy,onfoot,&u,&v)))goto moved;
    }
    if(!td_streetcar_runtime_find_anchor(&scene,td.district,td.u,td.v,onfoot,&u,&v))return TD_STREETCAR_PARK_BLOCKED;
moved:
    td.u=td.safe_u=u;td.v=td.safe_v=v;
    if(!onfoot){td.park_u=u;td.park_v=v;td.park_district=td.district;}
    return TD_STREETCAR_PARK_MOVED;
}
