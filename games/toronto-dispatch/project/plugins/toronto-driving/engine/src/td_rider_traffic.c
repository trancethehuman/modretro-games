#pragma bank 255
#include <stddef.h>
#include "td_traffic.h"
#include "td_district.h"

/* Independent bank for the new riders. The original traffic unit remains
 * byte-for-byte R6; these exact private readonly helpers add no RAM/cache.
 * Private symbol names also permit the actual-C unity host adapter to compare
 * both implementations in one translation unit without altering either. */
typedef struct { UWORD u,v; UBYTE arms,tile_x,tile_y; } td_rider_signal_t;
#define td_signal_t td_rider_signal_t
#define td_signal_offsets td_rider_signal_offsets
#define td_signals_h td_rider_signals_h
#define td_signals_v td_rider_signals_v
/* Each native unit owns its ROM copy. The host adapter can include this unit
 * after the old fleet unit; reset only the generated table inclusion guard. */
#undef TD_TRAFFIC_SIGNALS_H
#include "td_traffic_signals.h"
#undef td_signal_t
#undef td_signal_offsets
#undef td_signals_h
#undef td_signals_v

#define td_rider_distance(a,b) ((a)>(b)?(a)-(b):(b)-(a))
static UBYTE td_rider_step_limit(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UWORD limit){
    return old_u<1024*16&&u<1024*16&&old_v<976*16&&v<976*16&&
        !(old_u!=u&&old_v!=v)&&
        td_rider_distance(old_u,u)+td_rider_distance(old_v,v)<=limit;
}
#define td_rider_step_valid(a,b,c,d) td_rider_step_limit(a,b,c,d,8)
static UWORD td_rider_first(const td_rider_signal_t *signals,UWORD first,UWORD end,UWORD lateral,UBYTE horizontal){
    UWORD middle,centre,lower=lateral>18*16?lateral-18*16:0;
    while(first<end){
        middle=first+(end-first)/2;
        centre=(horizontal?signals[middle].v:signals[middle].u)*16;
        if(centre<lower)first=middle+1;else end=middle;
    }
    return first;
}

#define td_rider_extent(extents,slot) ((extents)?(extents)[slot]:5)
#define td_rider_body_valid(u,v,hu,hv) ((hu)&&(hv)&&(hu)<=16&&(hv)<=16&&\
    (u)>=(hu)*16&&(v)>=(hv)*16&&(u)<=(1024-(hu))*16&&(v)<=(976-(hv))*16)
static UBYTE td_rider_overlap(UWORD u,UWORD v,UBYTE hu,UBYTE hv,
    UWORD other_u,UWORD other_v,UBYTE other_hu,UBYTE other_hv){
    return td_rider_distance(u,other_u)<(hu+other_hu)*16&&
        td_rider_distance(v,other_v)<(hv+other_hv)*16;
}
typedef struct {
    UWORD old_u,old_v,u,v,left,right,top,bottom;
    UBYTE hu,hv;
} td_rider_motion_t;
/* Validated bodies usually lie far outside this exact swept hull. Keep that
 * common rejection in the caller: SDCC otherwise pushes the full obstacle
 * argument frame for all five distant cars and each visible human. These
 * macro arguments are side-effect-free local values, not calls/increments.
 * Validation MUST happen first, including for an apparently distant body. */
#define td_rider_outside(m,u,v,hu,hv) ((u)+(hu)*16<=(m)->left||\
    (u)>=(m)->right+(hu)*16||(v)+(hv)*16<=(m)->top||\
    (v)>=(m)->bottom+(hv)*16)

static UBYTE td_rider_obstacle(const td_rider_motion_t *m,UWORD other_u,UWORD other_v,
    UBYTE other_hu,UBYTE other_hv,UBYTE escape){
    /* Only nearby swept hulls reach this existing-overlap exception. */
    if(!escape||!td_rider_overlap(m->old_u,m->old_v,m->hu,m->hv,other_u,other_v,other_hu,other_hv))return FALSE;
    /* An endpoint-distance test alone could cross a centre and briefly move
     * deeper into the body. Require the entire cardinal step to move away. */
    if(m->u>m->old_u?m->old_u<other_u:m->u<m->old_u?m->old_u>other_u:
       m->v>m->old_v?m->old_v<other_v:m->v<m->old_v?m->old_v>other_v:TRUE)return FALSE;
    /* A nonzero cardinal step wholly directed away from the centre strictly
     * increases separation; no four-distance sum or16bit wrap is needed. */
    return TRUE;
}
#define td_rider_bucket_far(a,b,n) ((a)>(b)?(UBYTE)((a)-(b))>=(n):(UBYTE)((b)-(a))>=(n))
static UBYTE td_rider_bodies(const td_traffic_context_t *ctx,const td_rider_motion_t *m,UBYTE escape,UBYTE validated){
    UBYTE i,bit,other_hu,other_hv,bucket,bu=m->old_u>>8,bv=m->old_v>>8;UWORD other_u,other_v;const actor_t *person;
    /* An external rider never owns a fleet slot. Iterate the same eight
     * bodies directly and shift the priority bit once, avoiding a dynamic
     * 16-bit 1<<slot/1<<i sequence for every body on the native CPU. */
    for(i=0,bit=1;i<TD_TRAFFIC_SLOTS;i++,bit<<=1){
        /* begin proved every body, including apparently distant ones.
         * Four16px buckets imply at least48.0625px separation. A half8
         * rider, half16 fleet, 18px priority halo and half-pixel step can
         * reach only42.5px. Reject only beyond both exact policies. */
        if(validated){
            bucket=ctx->u[i]>>8;if(td_rider_bucket_far(bucket,bu,4))continue;
            bucket=ctx->v[i]>>8;if(td_rider_bucket_far(bucket,bv,4))continue;
        }
        other_u=ctx->u[i];other_v=ctx->v[i];
        other_hu=td_rider_extent(ctx->half_u,i);other_hv=td_rider_extent(ctx->half_v,i);
        if(!validated&&!td_rider_body_valid(other_u,other_v,other_hu,other_hv))return FALSE;
        if(!td_rider_outside(m,other_u,other_v,other_hu,other_hv)&&
           !td_rider_obstacle(m,other_u,other_v,other_hu,other_hv,escape))return FALSE;
        if(ctx->priority_mask&bit){
            if((m->u!=m->old_u?td_rider_distance(m->u,other_u)<td_rider_distance(m->old_u,other_u):
                 td_rider_distance(m->v,other_v)<td_rider_distance(m->old_v,other_v))&&
               td_rider_overlap(m->u,m->v,m->hu,m->hv,other_u,other_v,other_hu+18,other_hv+18))return FALSE;
        }
    }
    if(ctx->parked_active){
        if(!validated&&!td_rider_body_valid(ctx->park_u,ctx->park_v,7,7))return FALSE;
        if(!td_rider_outside(m,ctx->park_u,ctx->park_v,7,7)&&
           !td_rider_obstacle(m,ctx->park_u,ctx->park_v,7,7,escape))return FALSE;
    }
    if(ctx->peds)for(i=0,person=ctx->peds;i<TD_TRAFFIC_PEOPLE;i++,person++){
        if(person->flags&ACTOR_FLAG_HIDDEN)continue;
        /* Q5>>9 is exactly Q4>>8. Two buckets imply16.0625px, beyond
         * half8+half3+0.5=11.5px. Full malformed human geometry was already
         * validated; close humans retain exact sweep and escape tests. */
        if(validated){
            bucket=(UBYTE)(person->pos.x>>8)>>1;if(td_rider_bucket_far(bucket,bu,2))continue;
            bucket=(UBYTE)(person->pos.y>>8)>>1;if(td_rider_bucket_far(bucket,bv,2))continue;
        }
        other_u=person->pos.x>>1;other_v=person->pos.y>>1;
        if(!validated&&!td_rider_body_valid(other_u,other_v,3,3))return FALSE;
        if(!td_rider_outside(m,other_u,other_v,3,3)&&
           !td_rider_obstacle(m,other_u,other_v,3,3,escape))return FALSE;
    }
    return TRUE;
}

static UBYTE td_rider_entry_clear(const td_traffic_context_t *ctx,UWORD u,UWORD v){
    UBYTE i;UWORD other_u,other_v;const actor_t *person;
    for(i=0;i<TD_TRAFFIC_SLOTS;i++)if(
       td_rider_distance(ctx->u[i],u)<24*16&&td_rider_distance(ctx->v[i],v)<24*16)return FALSE;
    if(ctx->parked_active&&td_rider_overlap(ctx->park_u,ctx->park_v,7,7,u,v,18,18))return FALSE;
    if(ctx->peds)for(i=0,person=ctx->peds;i<TD_TRAFFIC_PEOPLE;i++,person++){
        if(person->flags&ACTOR_FLAG_HIDDEN)continue;
        other_u=person->pos.x>>1;other_v=person->pos.y>>1;
        if(td_rider_overlap(other_u,other_v,3,3,u,v,18,18))return FALSE;
    }
    return TRUE;
}

static UBYTE td_rider_admit(const td_traffic_context_t *ctx,UBYTE district,
    UWORD seconds,UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half,UBYTE validated) {
    td_rider_motion_t m;const td_rider_signal_t *signals;
    UWORD i,end,line,pos,next,lateral,centre;UBYTE horizontal,arm;
    if(!ctx||!ctx->u||!ctx->v||ctx->parked_active>1||!half||half>8||
       district>=TD_DISTRICT_COUNT||district>=TD_TRAFFIC_SIGNAL_DISTRICTS||
       !td_rider_step_valid(old_u,old_v,u,v)||
       !td_rider_body_valid(old_u,old_v,half,half)||!td_rider_body_valid(u,v,half,half))return FALSE;
    m.old_u=old_u;m.old_v=old_v;m.u=u;m.v=v;m.hu=m.hv=half;
    m.left=(old_u<u?old_u:u)-half*16;m.right=(old_u>u?old_u:u)+half*16;
    m.top=(old_v<v?old_v:v)-half*16;m.bottom=(old_v>v?old_v:v)+half*16;
    if(!td_rider_bodies(ctx,&m,1,validated))return FALSE;
    if(old_u==u&&old_v==v)return TRUE;
    horizontal=old_u!=u;pos=horizontal?old_u:old_v;next=horizontal?u:v;lateral=horizontal?v:u;
    if(next>pos?((pos-1)>>7)==((next-1)>>7):(pos>>7)==(next>>7))return TRUE;
    arm=horizontal?(u>old_u?8:2):(v>old_v?1:4);
    signals=horizontal?td_rider_signals_h:td_rider_signals_v;end=td_rider_signal_offsets[district+1];
    /* Sidewalk centres can be24px from the junction. Extend only this new
     * mover's signal corridor to32px; old fleet queries remain byte-for-byte.
     * The private lower-bound helper subtracts18px, so shift its key14px. */
    i=td_rider_first(signals,td_rider_signal_offsets[district],end,lateral>14*16?lateral-14*16:0,horizontal);
    for(;i<end;i++){
        centre=(horizontal?signals[i].v:signals[i].u)*16;
        if(centre>lateral+32*16)break;
        if(!(signals[i].arms&arm))continue;
        centre=(horizontal?signals[i].u:signals[i].v)*16;
        line=next>pos?centre-24*16:centre+24*16;
        if(next>pos?(pos<=line&&next>line):(pos>=line&&next<line)){
            if(horizontal?seconds%12>=7:seconds%12<7)return FALSE;
            if(!td_rider_entry_clear(ctx,signals[i].u*16,signals[i].v*16))return FALSE;
        }
    }
    return TRUE;
}

UBYTE td_traffic_external_admit(const td_traffic_context_t *ctx,UBYTE district,
    UWORD seconds,UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half) BANKED {
    return td_rider_admit(ctx,district,seconds,old_u,old_v,u,v,half,FALSE);
}

#define TD_RIDER_BATCH_VALID 0xB6
UBYTE td_traffic_external_begin(const td_traffic_context_t *ctx,td_rider_batch_t *batch) BANKED {
    UBYTE i,hu,hv;UWORD u,v;const actor_t *person;
    if(!batch)return FALSE;
    batch->valid=0;batch->ctx=NULL;
    if(!ctx||!ctx->u||!ctx->v||ctx->parked_active>1)return FALSE;
    for(i=0;i<TD_TRAFFIC_SLOTS;i++){
        u=ctx->u[i];v=ctx->v[i];hu=td_rider_extent(ctx->half_u,i);hv=td_rider_extent(ctx->half_v,i);
        if(!td_rider_body_valid(u,v,hu,hv))return FALSE;
    }
    if(ctx->parked_active&&!td_rider_body_valid(ctx->park_u,ctx->park_v,7,7))return FALSE;
    if(ctx->peds)for(i=0,person=ctx->peds;i<TD_TRAFFIC_PEOPLE;i++,person++){
        if(person->flags&ACTOR_FLAG_HIDDEN)continue;
        u=person->pos.x>>1;v=person->pos.y>>1;
        if(!td_rider_body_valid(u,v,3,3))return FALSE;
    }
    batch->ctx=ctx;batch->valid=TD_RIDER_BATCH_VALID;return TRUE;
}
UBYTE td_traffic_external_batch_admit(const td_rider_batch_t *batch,UBYTE district,
    UWORD seconds,UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half) BANKED {
    if(!batch||batch->valid!=TD_RIDER_BATCH_VALID||!batch->ctx)return FALSE;
    return td_rider_admit(batch->ctx,district,seconds,old_u,old_v,u,v,half,TRUE);
}
