#pragma bank 255
#include "td_traffic.h"
#include "td_district.h"
#include "td_roads.h"
#include "td_streetcar_runtime.h"

typedef struct { UWORD u,v; UBYTE arms,tile_x,tile_y; } td_signal_t;
#include "td_traffic_signals.h"

#define td_traffic_distance(a,b) ((a)>(b)?(a)-(b):(b)-(a))
static UBYTE td_traffic_step_limit(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UWORD limit){
    return old_u<1024*16&&u<1024*16&&old_v<976*16&&v<976*16&&
        !(old_u!=u&&old_v!=v)&&
        td_traffic_distance(old_u,u)+td_traffic_distance(old_v,v)<=limit;
}
#define td_traffic_step_valid(a,b,c,d) td_traffic_step_limit(a,b,c,d,8)
static UWORD td_traffic_first(const td_signal_t *signals,UWORD first,UWORD end,UWORD lateral,UBYTE horizontal){
    UWORD middle,centre,lower=lateral>18*16?lateral-18*16:0;
    while(first<end){
        middle=first+(end-first)/2;
        centre=(horizontal?signals[middle].v:signals[middle].u)*16;
        if(centre<lower)first=middle+1;else end=middle;
    }
    return first;
}

#define td_traffic_extent(extents,slot) ((extents)?(extents)[slot]:5)
#define td_traffic_body_valid(u,v,hu,hv) ((hu)&&(hv)&&(hu)<=16&&(hv)<=16&&\
    (u)>=(hu)*16&&(v)>=(hv)*16&&(u)<=(1024-(hu))*16&&(v)<=(976-(hv))*16)
static UBYTE td_traffic_overlap(UWORD u,UWORD v,UBYTE hu,UBYTE hv,
    UWORD other_u,UWORD other_v,UBYTE other_hu,UBYTE other_hv){
    return td_traffic_distance(u,other_u)<(hu+other_hu)*16&&
        td_traffic_distance(v,other_v)<(hv+other_hv)*16;
}
typedef struct {
    UWORD old_u,old_v,u,v,left,right,top,bottom;
    UBYTE hu,hv;
} td_traffic_motion_t;
/* Validated bodies usually lie far outside this exact swept hull. Keep that
 * common rejection in the caller: SDCC otherwise pushes the full obstacle
 * argument frame for all five distant cars and each visible human. These
 * macro arguments are side-effect-free local values, not calls/increments.
 * Validation MUST happen first, including for an apparently distant body. */
#define td_traffic_outside(m,u,v,hu,hv) ((u)+(hu)*16<=(m)->left||\
    (u)>=(m)->right+(hu)*16||(v)+(hv)*16<=(m)->top||\
    (v)>=(m)->bottom+(hv)*16)
static UBYTE td_traffic_prepare(const td_traffic_context_t *ctx,UBYTE slot,
    UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE escape,UWORD limit,td_traffic_motion_t *m){
    UBYTE hu,hv;
    if(!ctx||!ctx->u||!ctx->v||slot>=TD_TRAFFIC_SLOTS||escape>1||
       ctx->parked_active>1||(ctx->priority_mask&0xC0)||
       !td_traffic_step_limit(old_u,old_v,u,v,limit))return FALSE;
    hu=td_traffic_extent(ctx->half_u,slot);hv=td_traffic_extent(ctx->half_v,slot);
    if(ctx->u[slot]!=old_u||ctx->v[slot]!=old_v||
       !td_traffic_body_valid(old_u,old_v,hu,hv)||!td_traffic_body_valid(u,v,hu,hv))return FALSE;
    m->old_u=old_u;m->old_v=old_v;m->u=u;m->v=v;m->hu=hu;m->hv=hv;
    /* Valid endpoint bodies prove these bounds cannot under/overflow. Build
     * the exact cardinal swept hull once, rather than once per obstacle. */
    m->left=(old_u<u?old_u:u)-hu*16;m->right=(old_u>u?old_u:u)+hu*16;
    m->top=(old_v<v?old_v:v)-hv*16;m->bottom=(old_v>v?old_v:v)+hv*16;
    return TRUE;
}
static UBYTE td_traffic_obstacle(const td_traffic_motion_t *m,UWORD other_u,UWORD other_v,
    UBYTE other_hu,UBYTE other_hv,UBYTE escape){
    /* Only nearby swept hulls reach this existing-overlap exception. */
    if(!escape||!td_traffic_overlap(m->old_u,m->old_v,m->hu,m->hv,other_u,other_v,other_hu,other_hv))return FALSE;
    /* An endpoint-distance test alone could cross a centre and briefly move
     * deeper into the body. Require the entire cardinal step to move away. */
    if(m->u>m->old_u?m->old_u<other_u:m->u<m->old_u?m->old_u>other_u:
       m->v>m->old_v?m->old_v<other_v:m->v<m->old_v?m->old_v>other_v:TRUE)return FALSE;
    /* A nonzero cardinal step wholly directed away from the centre strictly
     * increases separation; no four-distance sum or16bit wrap is needed. */
    return TRUE;
}
static UBYTE td_traffic_bodies(const td_traffic_context_t *ctx,UBYTE slot,const td_traffic_motion_t *m,UBYTE escape){
    UBYTE i,other_hu,other_hv;UWORD other_u,other_v;
    for(i=0;i<TD_TRAFFIC_SLOTS;i++)if(i!=slot){
        other_u=ctx->u[i];other_v=ctx->v[i];
        other_hu=td_traffic_extent(ctx->half_u,i);other_hv=td_traffic_extent(ctx->half_v,i);
        if(!td_traffic_body_valid(other_u,other_v,other_hu,other_hv))return FALSE;
        if(!td_traffic_outside(m,other_u,other_v,other_hu,other_hv)&&
           !td_traffic_obstacle(m,other_u,other_v,other_hu,other_hv,escape))return FALSE;
        if(!(ctx->priority_mask&(1<<slot))&&(ctx->priority_mask&(1<<i))){
            if((m->u!=m->old_u?td_traffic_distance(m->u,other_u)<td_traffic_distance(m->old_u,other_u):
                 td_traffic_distance(m->v,other_v)<td_traffic_distance(m->old_v,other_v))&&
               td_traffic_overlap(m->u,m->v,m->hu,m->hv,other_u,other_v,other_hu+18,other_hv+18))return FALSE;
        }
    }
    if(ctx->parked_active){
        if(!td_traffic_body_valid(ctx->park_u,ctx->park_v,7,7))return FALSE;
        if(!td_traffic_outside(m,ctx->park_u,ctx->park_v,7,7)&&
           !td_traffic_obstacle(m,ctx->park_u,ctx->park_v,7,7,escape))return FALSE;
    }
    if(ctx->peds)for(i=0;i<TD_TRAFFIC_PEOPLE;i++){
        if(ctx->peds[i].flags&ACTOR_FLAG_HIDDEN)continue;
        other_u=ctx->peds[i].pos.x>>1;other_v=ctx->peds[i].pos.y>>1;
        if(!td_traffic_body_valid(other_u,other_v,3,3))return FALSE;
        if(!td_traffic_outside(m,other_u,other_v,3,3)&&
           !td_traffic_obstacle(m,other_u,other_v,3,3,escape))return FALSE;
    }
    return TRUE;
}
UBYTE td_traffic_motion_clear(const td_traffic_context_t *ctx,UBYTE slot,
    UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE escape) BANKED {
    td_traffic_motion_t m;
    return td_traffic_prepare(ctx,slot,old_u,old_v,u,v,escape,8,&m)&&td_traffic_bodies(ctx,slot,&m,escape);
}

UBYTE td_traffic_junction_clear(const td_traffic_context_t *ctx,UBYTE district,
    UBYTE slot,UWORD old_u,UWORD old_v,UWORD u,UWORD v) BANKED {
    const td_signal_t *signals;UWORD i,line,pos,next,lateral,centre;
    UBYTE horizontal,arm,other,hu,hv;
    if(!ctx||!ctx->u||!ctx->v||slot>=TD_TRAFFIC_SLOTS||ctx->parked_active>1||
       district>=TD_DISTRICT_COUNT||district>=TD_TRAFFIC_SIGNAL_DISTRICTS||
       !td_traffic_step_valid(old_u,old_v,u,v)||ctx->u[slot]!=old_u||ctx->v[slot]!=old_v)return FALSE;
    if(old_u==u&&old_v==v)return TRUE;
    horizontal=old_u!=u;pos=horizontal?old_u:old_v;next=horizontal?u:v;lateral=horizontal?v:u;
    arm=horizontal?(u>old_u?8:2):(v>old_v?1:4);
    signals=horizontal?td_signals_h:td_signals_v;
    i=td_traffic_first(signals,td_signal_offsets[district],td_signal_offsets[district+1],lateral,horizontal);
    for(;i<td_signal_offsets[district+1];i++){
        centre=(horizontal?signals[i].v:signals[i].u)*16;
        if(centre>lateral+18*16)break;
        if(!(signals[i].arms&arm))continue;
        centre=(horizontal?signals[i].u:signals[i].v)*16;
        line=next>pos?centre-24*16:centre+24*16;
        if(next>pos?!(pos<=line&&next>line):!(pos>=line&&next<line))continue;
        for(other=0;other<TD_TRAFFIC_SLOTS;other++)if(other!=slot){
            hu=td_traffic_extent(ctx->half_u,other);hv=td_traffic_extent(ctx->half_v,other);
            if(!td_traffic_body_valid(ctx->u[other],ctx->v[other],hu,hv)||
               (td_traffic_distance(ctx->u[other],signals[i].u*16)<24*16&&
                td_traffic_distance(ctx->v[other],signals[i].v*16)<24*16))return FALSE;
        }
        if(ctx->parked_active&&(!td_traffic_body_valid(ctx->park_u,ctx->park_v,7,7)||
           td_traffic_overlap(ctx->park_u,ctx->park_v,7,7,signals[i].u*16,signals[i].v*16,18,18)))return FALSE;
        if(ctx->peds)for(other=0;other<TD_TRAFFIC_PEOPLE;other++){
            if(ctx->peds[other].flags&ACTOR_FLAG_HIDDEN)continue;
            if(!td_traffic_body_valid(ctx->peds[other].pos.x>>1,ctx->peds[other].pos.y>>1,3,3)||
               td_traffic_overlap(ctx->peds[other].pos.x>>1,ctx->peds[other].pos.y>>1,3,3,
                                  signals[i].u*16,signals[i].v*16,18,18))return FALSE;
        }
    }
    return TRUE;
}

/* Called only after td_traffic_bodies validated every used body. No second
 * geometry pass, large argument frames, or banked public helper calls. */
static UBYTE td_traffic_entry_clear(const td_traffic_context_t *ctx,UBYTE slot,UWORD u,UWORD v){
    UBYTE i;UWORD other_u,other_v;
    for(i=0;i<TD_TRAFFIC_SLOTS;i++)if(i!=slot&&
       td_traffic_distance(ctx->u[i],u)<24*16&&td_traffic_distance(ctx->v[i],v)<24*16)return FALSE;
    if(ctx->parked_active&&td_traffic_overlap(ctx->park_u,ctx->park_v,7,7,u,v,18,18))return FALSE;
    if(ctx->peds)for(i=0;i<TD_TRAFFIC_PEOPLE;i++){
        if(ctx->peds[i].flags&ACTOR_FLAG_HIDDEN)continue;
        other_u=ctx->peds[i].pos.x>>1;other_v=ctx->peds[i].pos.y>>1;
        if(td_traffic_overlap(other_u,other_v,3,3,u,v,18,18))return FALSE;
    }
    return TRUE;
}
UBYTE td_traffic_admit(const td_traffic_context_t *ctx,UBYTE district,UWORD seconds,
    UBYTE slot,UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE escape) BANKED {
    td_traffic_motion_t m;const td_signal_t *signals;
    UWORD i,end,entry,line,pos,next,lateral,centre;UBYTE horizontal,arm;
    if(district>=TD_DISTRICT_COUNT||district>=TD_TRAFFIC_SIGNAL_DISTRICTS||
       !td_traffic_prepare(ctx,slot,old_u,old_v,u,v,escape,128,&m))return FALSE;
    /* A stationary endpoint still validates/checks all bodies, but requires
     * neither phase division nor any signal/junction ROM search. */
    if(old_u==u&&old_v==v)return td_traffic_bodies(ctx,slot,&m,escape);
    horizontal=old_u!=u;pos=horizontal?old_u:old_v;next=horizontal?u:v;lateral=horizontal?v:u;
    /* Actual generated junction centres are8px aligned (host data gate).
     * Their +/-24px lines therefore lie on128Q4 boundaries. Most small
     * advances cross none, so skip both ROM lane search and phase division.
     * Bodies are still validated/swept, including malformed far-away ones. */
    if(next>pos?((pos-1)>>7)==((next-1)>>7):(pos>>7)==(next>>7))
        return td_traffic_bodies(ctx,slot,&m,escape);
    arm=horizontal?(u>old_u?8:2):(v>old_v?1:4);
    signals=horizontal?td_signals_h:td_signals_v;end=td_signal_offsets[district+1];
    i=td_traffic_first(signals,td_signal_offsets[district],end,lateral,horizontal);
    entry=end;
    for(;i<end;i++){
        centre=(horizontal?signals[i].v:signals[i].u)*16;
        if(centre>lateral+18*16)break;
        if(!(signals[i].arms&arm))continue;
        centre=(horizontal?signals[i].u:signals[i].v)*16;
        line=next>pos?centre-24*16:centre+24*16;
        if(next>pos?(pos<=line&&next>line):(pos>=line&&next<line)){
            entry=i;break;
        }
    }
    if(entry<end&& (horizontal?(seconds%12>=7):(seconds%12<7)))return FALSE;
    if(!td_traffic_bodies(ctx,slot,&m,escape))return FALSE;
    if(entry==end)return TRUE;
    /* Entered junctions are rare. Continue this same lane scan in case a
     * compressed pair shares a crossing line; never accept just the first. */
    for(i=entry;i<end;i++){
        centre=(horizontal?signals[i].v:signals[i].u)*16;
        if(centre>lateral+18*16)break;
        if(!(signals[i].arms&arm))continue;
        centre=(horizontal?signals[i].u:signals[i].v)*16;
        line=next>pos?centre-24*16:centre+24*16;
        if((next>pos?(pos<=line&&next>line):(pos>=line&&next<line))&&
           !td_traffic_entry_clear(ctx,slot,signals[i].u*16,signals[i].v*16))return FALSE;
    }
    return TRUE;
}

#define TD_TRAFFIC_EPOCH_VALID 0xA5
UBYTE td_traffic_epoch_begin(const td_traffic_context_t *ctx,UBYTE district,
    UWORD seconds,td_traffic_epoch_t *epoch) BANKED {
    UBYTE i,hu,hv,bit;UWORD u,v;
    if(!epoch)return FALSE;
    epoch->valid=0;epoch->pending_slot=255;
    if(!ctx||!ctx->u||!ctx->v||ctx->parked_active>1||(ctx->priority_mask&0xC0)||
       district>=TD_DISTRICT_COUNT||district>=TD_TRAFFIC_SIGNAL_DISTRICTS)return FALSE;
    for(i=0;i<TD_TRAFFIC_SLOTS;i++){
        u=ctx->u[i];v=ctx->v[i];
        hu=td_traffic_extent(ctx->half_u,i);hv=td_traffic_extent(ctx->half_v,i);
        if(!td_traffic_body_valid(u,v,hu,hv))return FALSE;
        epoch->u[i]=u;epoch->v[i]=v;epoch->half_u[i]=hu;epoch->half_v[i]=hv;
        epoch->bucket_u[i]=u>>8;epoch->bucket_v[i]=v>>8;
    }
    epoch->parked_active=ctx->parked_active;epoch->priority_mask=ctx->priority_mask;
    epoch->park_u=ctx->park_u;epoch->park_v=ctx->park_v;epoch->people_mask=0;
    if(ctx->parked_active&&!td_traffic_body_valid(ctx->park_u,ctx->park_v,7,7))return FALSE;
    if(ctx->peds)for(i=0,bit=1;i<TD_TRAFFIC_PEOPLE;i++,bit<<=1){
        if(ctx->peds[i].flags&ACTOR_FLAG_HIDDEN)continue;
        u=ctx->peds[i].pos.x>>1;v=ctx->peds[i].pos.y>>1;
        if(!td_traffic_body_valid(u,v,3,3))return FALSE;
        epoch->ped_u[i]=u;epoch->ped_v[i]=v;epoch->people_mask|=bit;
    }
    epoch->district=district;epoch->phase=seconds%12;epoch->valid=TD_TRAFFIC_EPOCH_VALID;
    return TRUE;
}

/* Snapshot construction validated every used body exactly once. Only the
 * narrowly checked commit changes its fleet positions; people/park/extents
 * are immutable for this epoch. Avoid repeatedly validating thirty distant
 * fleet bodies and rereading actor structs through live caller pointers. */
static UBYTE td_traffic_epoch_bodies(const td_traffic_epoch_t *epoch,UBYTE slot,
    const td_traffic_motion_t *m,UBYTE escape){
    UBYTE i,hu,hv,bit,bucket,bu=m->old_u>>8,bv=m->old_v>>8;
    UBYTE priority=(epoch->priority_mask&(1<<slot))?0:epoch->priority_mask;UWORD u,v;
    for(i=0,bit=1;i<TD_TRAFFIC_SLOTS;i++,bit<<=1)if(i!=slot){
        /* Five16px buckets imply >64px separation on this axis. Maximum
         * valid half extents16+16, priority margin18 and sweep8px reach only
         *58px. Thus this body can affect neither sweep/escape nor priority.
         * Ordered byte differences cannot underflow or need word math. */
        bucket=epoch->bucket_u[i];
        if(bucket>bu?(UBYTE)(bucket-bu)>=5:(UBYTE)(bu-bucket)>=5)continue;
        bucket=epoch->bucket_v[i];
        if(bucket>bv?(UBYTE)(bucket-bv)>=5:(UBYTE)(bv-bucket)>=5)continue;
        u=epoch->u[i];v=epoch->v[i];hu=epoch->half_u[i];hv=epoch->half_v[i];
        if(!td_traffic_outside(m,u,v,hu,hv)&&!td_traffic_obstacle(m,u,v,hu,hv,escape))return FALSE;
        if(priority&bit){
            if((m->u!=m->old_u?td_traffic_distance(m->u,u)<td_traffic_distance(m->old_u,u):
                  td_traffic_distance(m->v,v)<td_traffic_distance(m->old_v,v))&&
               td_traffic_overlap(m->u,m->v,m->hu,m->hv,u,v,hu+18,hv+18))return FALSE;
        }
    }
    if(epoch->parked_active&&!td_traffic_outside(m,epoch->park_u,epoch->park_v,7,7)&&
       !td_traffic_obstacle(m,epoch->park_u,epoch->park_v,7,7,escape))return FALSE;
    for(i=0,bit=1;i<TD_TRAFFIC_PEOPLE;i++,bit<<=1)if(epoch->people_mask&bit){
        u=epoch->ped_u[i];v=epoch->ped_v[i];
        if(!td_traffic_outside(m,u,v,3,3)&&!td_traffic_obstacle(m,u,v,3,3,escape))return FALSE;
    }
    return TRUE;
}
static UBYTE td_traffic_epoch_entry(const td_traffic_epoch_t *epoch,UBYTE slot,UWORD u,UWORD v){
    UBYTE i,bit;
    for(i=0;i<TD_TRAFFIC_SLOTS;i++)if(i!=slot&&
       td_traffic_distance(epoch->u[i],u)<24*16&&td_traffic_distance(epoch->v[i],v)<24*16)return FALSE;
    if(epoch->parked_active&&td_traffic_overlap(epoch->park_u,epoch->park_v,7,7,u,v,18,18))return FALSE;
    for(i=0,bit=1;i<TD_TRAFFIC_PEOPLE;i++,bit<<=1)if((epoch->people_mask&bit)&&
       td_traffic_overlap(epoch->ped_u[i],epoch->ped_v[i],3,3,u,v,18,18))return FALSE;
    return TRUE;
}
static UBYTE td_traffic_epoch_query(const td_traffic_epoch_t *epoch,UBYTE slot,
    const td_traffic_motion_t *m,UBYTE escape,UBYTE junction){
    const td_signal_t *signals;UWORD i,end,entry,line,pos,next,lateral,centre;
    UBYTE horizontal,arm;
    if(m->old_u==m->u&&m->old_v==m->v)return td_traffic_epoch_bodies(epoch,slot,m,escape);
    horizontal=m->old_u!=m->u;pos=horizontal?m->old_u:m->old_v;
    next=horizontal?m->u:m->v;lateral=horizontal?m->v:m->u;
    if(next>pos?((pos-1)>>7)==((next-1)>>7):(pos>>7)==(next>>7))
        return td_traffic_epoch_bodies(epoch,slot,m,escape);
    arm=horizontal?(m->u>m->old_u?8:2):(m->v>m->old_v?1:4);
    signals=horizontal?td_signals_h:td_signals_v;end=td_signal_offsets[epoch->district+1];
    i=td_traffic_first(signals,td_signal_offsets[epoch->district],end,lateral,horizontal);
    entry=end;
    for(;i<end;i++){
        centre=(horizontal?signals[i].v:signals[i].u)*16;
        if(centre>lateral+18*16)break;
        if(!(signals[i].arms&arm))continue;
        centre=(horizontal?signals[i].u:signals[i].v)*16;
        line=next>pos?centre-24*16:centre+24*16;
        if(next>pos?(pos<=line&&next>line):(pos>=line&&next<line)){entry=i;break;}
    }
    if(entry<end&&(horizontal?epoch->phase>=7:epoch->phase<7))return FALSE;
    if(!td_traffic_epoch_bodies(epoch,slot,m,escape))return FALSE;
    if(entry==end||!junction)return TRUE;
    for(i=entry;i<end;i++){
        centre=(horizontal?signals[i].v:signals[i].u)*16;
        if(centre>lateral+18*16)break;
        if(!(signals[i].arms&arm))continue;
        centre=(horizontal?signals[i].u:signals[i].v)*16;
        line=next>pos?centre-24*16:centre+24*16;
        if((next>pos?(pos<=line&&next>line):(pos>=line&&next<line))&&
           !td_traffic_epoch_entry(epoch,slot,signals[i].u*16,signals[i].v*16))return FALSE;
    }
    return TRUE;
}
/* Both public policies share candidate validation and pending ownership.
 * Retreat keeps its smaller sweep and omits only junction entry. */
static UBYTE td_traffic_epoch_try(td_traffic_epoch_t *epoch,UBYTE slot,
    UWORD u,UWORD v,UBYTE escape,UBYTE limit,UBYTE junction){
    td_traffic_motion_t m;UBYTE hu,hv;UWORD old_u,old_v;
    if(!epoch)return FALSE;
    epoch->pending_slot=255;
    if(epoch->valid!=TD_TRAFFIC_EPOCH_VALID||slot>=TD_TRAFFIC_SLOTS||escape>1||
       epoch->district>=TD_DISTRICT_COUNT||epoch->district>=TD_TRAFFIC_SIGNAL_DISTRICTS||epoch->phase>=12)return FALSE;
    old_u=epoch->u[slot];old_v=epoch->v[slot];
    hu=epoch->half_u[slot];hv=epoch->half_v[slot];
    /* Begin and accepted commits prove the private old body. Public wrappers
     * still require exact caller-cache equality. Validate every candidate,
     * including extents/map bounds, cardinality and full continuous limit. */
    if(!td_traffic_body_valid(u,v,hu,hv)||(old_u!=u&&old_v!=v)||
       td_traffic_distance(old_u,u)+td_traffic_distance(old_v,v)>limit)return FALSE;
    m.old_u=old_u;m.old_v=old_v;m.u=u;m.v=v;m.hu=hu;m.hv=hv;
    m.left=(old_u<u?old_u:u)-hu*16;m.right=(old_u>u?old_u:u)+hu*16;
    m.top=(old_v<v?old_v:v)-hv*16;m.bottom=(old_v>v?old_v:v)+hv*16;
    if(!td_traffic_epoch_query(epoch,slot,&m,escape,junction))return FALSE;
    epoch->pending_u=u;epoch->pending_v=v;epoch->pending_slot=slot;
    return TRUE;
}
UBYTE td_traffic_epoch_admit(td_traffic_epoch_t *epoch,UBYTE slot,
    UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE escape) BANKED {
    if(!epoch)return FALSE;
    epoch->pending_slot=255;
    if(slot>=TD_TRAFFIC_SLOTS||epoch->u[slot]!=old_u||epoch->v[slot]!=old_v)return FALSE;
    return td_traffic_epoch_try(epoch,slot,u,v,escape,128,TRUE);
}
UBYTE td_traffic_epoch_retreat_admit(td_traffic_epoch_t *epoch,UBYTE slot,
    UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE escape) BANKED {
    if(!epoch)return FALSE;
    epoch->pending_slot=255;
    if(slot>=TD_TRAFFIC_SLOTS||epoch->u[slot]!=old_u||epoch->v[slot]!=old_v)return FALSE;
    return td_traffic_epoch_try(epoch,slot,u,v,escape,8,FALSE);
}
static UBYTE td_traffic_epoch_commit_inner(td_traffic_epoch_t *epoch,UBYTE slot){
    UWORD u,v;
    if(!epoch||epoch->valid!=TD_TRAFFIC_EPOCH_VALID||slot>=TD_TRAFFIC_SLOTS||
       epoch->pending_slot!=slot)return FALSE;
    u=epoch->pending_u;v=epoch->pending_v;
    epoch->u[slot]=u;epoch->v[slot]=v;
    epoch->bucket_u[slot]=u>>8;epoch->bucket_v[slot]=v>>8;epoch->pending_slot=255;
    return TRUE;
}
UBYTE td_traffic_epoch_commit(td_traffic_epoch_t *epoch,UBYTE slot) BANKED {
    return td_traffic_epoch_commit_inner(epoch,slot);
}
UBYTE td_traffic_epoch_move(td_traffic_epoch_t *epoch,UBYTE slot,
    UWORD u,UWORD v,UBYTE retreat,UBYTE separating) BANKED {
    UBYTE half;
    if(!epoch)return FALSE;
    epoch->pending_slot=255;
    if(slot>=TD_TRAFFIC_SLOTS||retreat>1||separating>1||(separating&&!retreat))return FALSE;
    half=epoch->half_u[slot];
    if(half<5||half>8||epoch->half_v[slot]!=half||
       !td_traffic_epoch_try(epoch,slot,u,v,1,retreat?8:128,!retreat))return FALSE;
    if(!td_road_sweep(epoch->u[slot]>>4,epoch->v[slot]>>4,u>>4,v>>4,half)||
       (!separating&&!td_streetcar_runtime_traffic_sweep_clear(epoch->district,epoch->u[slot],epoch->v[slot],u,v,half))||
       !td_traffic_epoch_commit_inner(epoch,slot)){
        epoch->pending_slot=255;return FALSE;
    }
    return TRUE;
}
