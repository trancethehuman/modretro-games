#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "td_traffic.h"
#include "td_district.h"
#include "td_roads.h"
#include "td_streetcar_runtime.h"
#include "traffic_fixture.h"

static unsigned checks,failures;
static UWORD us[6],vs[6];
static actor_t people[6];
static td_traffic_context_t ctx;
static void expect(int ok,const char *message){
    checks++;if(!ok){if(failures<15)fprintf(stderr,"FAIL: %s\n",message);failures++;}
}
/* Hardware-facing query adapters model tile solids and a future tram body,
 * rather than returning a forced pass/fail flag. The separate road/runtime
 * harnesses exercise their real implementations; here we inspect delegation
 * units, complete swept extents, short-circuit order and commit ownership. */
typedef struct { UWORD old_u,old_v,u,v;UBYTE half,district; } guard_call_t;
static guard_call_t road_call,tram_call;
static unsigned road_calls,tram_calls,guard_order,road_order,tram_order;
static UBYTE guard_loaded,road_solid,tram_body;
static int solid_left,solid_right,solid_top,solid_bottom;
static int tram_left,tram_right,tram_top,tram_bottom;
static void guard_reset(UBYTE district){
    road_calls=tram_calls=guard_order=road_order=tram_order=0;
    guard_loaded=district;road_solid=tram_body=0;
    memset(&road_call,0,sizeof(road_call));memset(&tram_call,0,sizeof(tram_call));
}
UBYTE td_road_sweep(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half) BANKED {
    road_calls++;road_order=++guard_order;road_call=(guard_call_t){old_u,old_v,u,v,half,guard_loaded};
    if(!half||half>8||(old_u!=u&&old_v!=v)||old_u<8||u<8||old_v<8||v<8||
       old_u>1016||u>1016||old_v>968||v>968)return FALSE;
    int left=((old_u<u?old_u:u)-half)/8,right=((old_u>u?old_u:u)+half)/8;
    int top=((old_v<v?old_v:v)-half)/8,bottom=((old_v>v?old_v:v)+half)/8;
    return !road_solid||right<solid_left||left>solid_right||bottom<solid_top||top>solid_bottom;
}
UBYTE td_streetcar_runtime_traffic_sweep_clear(UBYTE district,UWORD old_u,UWORD old_v,
    UWORD u,UWORD v,UBYTE half) BANKED {
    tram_calls++;tram_order=++guard_order;tram_call=(guard_call_t){old_u,old_v,u,v,half,district};
    if(district!=guard_loaded||district>=TD_DISTRICT_COUNT||half<5||half>8||
       old_u<half*16||u<half*16||old_v<half*16||v<half*16||
       old_u>(1024-half)*16||u>(1024-half)*16||old_v>(976-half)*16||v>(976-half)*16)return FALSE;
    int left=(old_u<u?old_u:u)-half*16,right=(old_u>u?old_u:u)+half*16;
    int top=(old_v<v?old_v:v)-half*16,bottom=(old_v>v?old_v:v)+half*16;
    return !tram_body||right<=tram_left||left>=tram_right||bottom<=tram_top||top>=tram_bottom;
}
/* Keep every original independent geometry/signal expectation, and also
 * compare the new snapshot path against the retained strict public query.
 * Admission itself may record pending ownership, but no body may advance
 * until the explicit commit after the caller's external guards. */
static UBYTE admit_checked(const td_traffic_context_t *context,UBYTE district,UWORD seconds,
    UBYTE slot,UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE escape){
    UBYTE strict=td_traffic_admit(context,district,seconds,slot,old_u,old_v,u,v,escape),fast=0;
    td_traffic_epoch_t epoch;memset(&epoch,0xCC,sizeof(epoch));
    if(td_traffic_epoch_begin(context,district,seconds,&epoch)){
        UWORD before_u[6],before_v[6];memcpy(before_u,epoch.u,sizeof(before_u));memcpy(before_v,epoch.v,sizeof(before_v));
        fast=td_traffic_epoch_admit(&epoch,slot,old_u,old_v,u,v,escape);
        expect(!memcmp(before_u,epoch.u,sizeof(before_u))&&!memcmp(before_v,epoch.v,sizeof(before_v)),
               "snapshot admission records pending ownership without moving any fleet body");
        if(fast){
            expect(td_traffic_epoch_commit(&epoch,slot)&&epoch.u[slot]==u&&epoch.v[slot]==v,
                   "only an admitted pending candidate commits the matching exact coordinate");
            expect(!td_traffic_epoch_commit(&epoch,slot),"a committed candidate cannot commit twice");
        }else expect(!td_traffic_epoch_commit(&epoch,slot),"denied admission creates no committable candidate");
    }
    expect(fast==strict,"snapshot admission preserves every retained strict query result and independent oracle expectation");
    return strict;
}
static void reset(void){
    memset(&ctx,0,sizeof(ctx));memset(people,0,sizeof(people));
    ctx.u=us;ctx.v=vs;ctx.peds=people;
    for(unsigned i=0;i<6;i++){us[i]=(120+i*140)*16;vs[i]=900*16;people[i].flags=ACTOR_FLAG_HIDDEN;}
    us[0]=500*16;vs[0]=500*16;
}
static int overlap(int x,int y,int ox,int oy,int hx,int hy){
    return x>ox-hx&&x<ox+hx&&y>oy-hy&&y<oy+hy;
}
/* Wide-arithmetic geometric oracle: closed swept centre interval against an
 * open Minkowski body, plus a signed one-axis derivative for prior overlaps.
 * No production helper or branch is called by the oracle. */
static int obstacle_oracle(int ox,int oy,int x,int y,int bx,int by,int hx,int hy,int escape){
    int minx=ox<x?ox:x,maxx=ox>x?ox:x,miny=oy<y?oy:y,maxy=oy>y?oy:y;
    if(maxx<=bx-hx||minx>=bx+hx||maxy<=by-hy||miny>=by+hy)return 1;
    if(!escape||!overlap(ox,oy,bx,by,hx,hy))return 0;
    int delta=x!=ox?x-ox:y-oy,relative=x!=ox?ox-bx:oy-by;
    return delta!=0&&delta*relative>=0;
}
/* Independent division of authoritative centres. Deriving cache values
 * through an admission query would miss a
 * stale cache after an external-guard abort or sequential commit. */
static int fleet_buckets_match(const td_traffic_epoch_t *epoch,const td_traffic_context_t *context){
    for(unsigned i=0;i<6;i++){
        if(epoch->bucket_u[i]!=(unsigned)context->u[i]/256||
           epoch->bucket_v[i]!=(unsigned)context->v[i]/256)return 0;
    }
    return 1;
}
static int signal_oracle(unsigned district,unsigned seconds,unsigned ou,unsigned ov,unsigned u,unsigned v){
    int horizontal=u!=ou;
    if(u==ou&&v==ov)return 0;
    if(horizontal?(seconds%12<7):(seconds%12>=7))return 0;
    int pos=horizontal?(int)ou:(int)ov,next=horizontal?(int)u:(int)v;
    int lateral=horizontal?(int)v:(int)u,arm=horizontal?(u>ou?8:2):(v>ov?1:4);
    for(unsigned i=oracle_offsets[district];i<oracle_offsets[district+1];i++){
        const unsigned *node=oracle_signals[i];
        int across=(int)node[horizontal?1:0]*16;
        if(!(node[2]&(unsigned)arm)||lateral<across-18*16||lateral>across+18*16)continue;
        int line=(int)node[horizontal?0:1]*16+(next>pos?-24*16:24*16);
        if((next>pos&&pos<=line&&next>line)||(next<pos&&pos>=line&&next<line))return 1;
    }
    return 0;
}
static void test_signals(void){
    const unsigned times[]={0,6,7,11,12,65534,65535};
    const int laterals[]={-19,-18,-8,0,8,18,19};
    for(unsigned d=0;d<TD_DISTRICT_COUNT;d++)for(unsigned i=oracle_offsets[d];i<oracle_offsets[d+1];i++){
        const unsigned *node=oracle_signals[i];
        for(unsigned direction=0;direction<4;direction++)for(unsigned t=0;t<sizeof(times)/sizeof(times[0]);t++)
        for(unsigned l=0;l<sizeof(laterals)/sizeof(laterals[0]);l++)for(int fraction=0;fraction<8;fraction++){
            int horizontal=direction<2,sign=(direction&1)?-1:1;
            int ou=(int)node[0]*16,ov=(int)node[1]*16;
            if(horizontal){ou-=sign*(24*16+fraction);ov+=laterals[l]*16;}
            else{ov-=sign*(24*16+fraction);ou+=laterals[l]*16;}
            int u=ou+(horizontal?sign*8:0),v=ov+(horizontal?0:sign*8);
            expect(td_traffic_signal_stop(d,times[t],ou,ov,u,v)==signal_oracle(d,times[t],ou,ov,u,v),
                   "all registered arms, fractional stop crossings, shared phases and lateral edges match independent signal truth");
        }
    }
    expect(td_traffic_signal_stop(0,7,184*16,64*16,184*16+8,64*16),
           "the Core bus approach at Bloor/Bathurst receives the same red signal as a car");
    expect(!td_traffic_signal_stop(0,0,184*16,64*16,184*16+8,64*16),"bus resumes on shared green");
    expect(!td_traffic_signal_stop(0,8,185*16,64*16,185*16+8,64*16),"already-entered traffic may clear instead of stopping inside the intersection");
    expect(td_traffic_signal_stop(TD_DISTRICT_COUNT,0,8000,8000,8008,8000)&&
           td_traffic_signal_stop(0,0,8000,8000,8008,8008)&&
           td_traffic_signal_stop(0,0,8000,8000,8009,8000)&&
           td_traffic_signal_stop(0,0,65535,8000,0,8000),"invalid signal district, diagonal, excessive or wrapped steps fail closed");
}
static void test_body_truth(void){
    for(int kind=0;kind<3;kind++)for(int axis=0;axis<2;axis++)for(int direction=-1;direction<=1;direction+=2)
    for(int escape=0;escape<2;escape++)for(int along=-210;along<=210;along+=5)for(int across=-210;across<=210;across+=7){
        reset();int bx=8000+(axis?across:along),by=8000+(axis?along:across);
        /* A default fleet car is5px; the parked courier is7px and a
         * pedestrian is3px. Compute each independent Minkowski extent. */
        int u=8000+(axis?0:direction*8),v=8000+(axis?direction*8:0);
        int margin=(5+(kind==1?7:kind==2?3:5))*16;
        if(kind==0){us[1]=bx;vs[1]=by;}
        else if(kind==1){ctx.parked_active=1;ctx.park_u=bx;ctx.park_v=by;}
        else{people[0].flags=0;people[0].pos.x=bx*2;people[0].pos.y=by*2;}
        td_traffic_context_t saved=ctx;UWORD before_u[6],before_v[6];actor_t saved_people[6];
        memcpy(before_u,us,sizeof(us));memcpy(before_v,vs,sizeof(vs));memcpy(saved_people,people,sizeof(people));
        expect(td_traffic_motion_clear(&ctx,0,8000,8000,u,v,escape)==obstacle_oracle(8000,8000,u,v,bx,by,margin,margin,escape),
               "full-body sweep and monotonic prior-overlap escape agree for vehicles, parked cars and visible civilians");
        expect(!memcmp(&saved,&ctx,sizeof(ctx))&&!memcmp(before_u,us,sizeof(us))&&!memcmp(before_v,vs,sizeof(vs))&&
               !memcmp(saved_people,people,sizeof(people)),"banked traffic query leaves all caller state and actor/cache positions unchanged");
    }
}
static void test_priority_and_traps(void){
    reset();us[1]=520*16;vs[1]=500*16;
    expect(td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"ordinary gap permits forward queue progress");
    ctx.priority_mask=2;
    expect(!td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"civilian approaching an active emergency unit yields before physical contact");
    expect(td_traffic_motion_clear(&ctx,0,8000,8000,7992,8000,0),"civilian may move away from emergency priority instead of becoming trapped");
    ctx.priority_mask=1;us[1]=509*16;
    expect(!td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"emergency priority never permits an occupied vehicle sweep");
    expect(td_traffic_signal_stop(0,7,184*16,64*16,184*16+8,64*16),"emergency masks cannot override the independent signal gate");
    reset();us[1]=495*16;vs[1]=500*16;us[2]=505*16;vs[2]=500*16;
    expect(!td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,1)&&
           !td_traffic_motion_clear(&ctx,0,8000,8000,7992,8000,1),"opposing prior obstructions fail closed without a teleport or a move through either body");
    us[2]=700*16;
    expect(td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,1),"removing one obstruction releases a strictly separating connected step");
    reset();us[1]=500*16;vs[1]=500*16;
    for(int step=0;step<20;step++){
        UWORD before=us[0];expect(td_traffic_motion_clear(&ctx,0,before,vs[0],before+8,vs[0],1),"existing overlap separates continuously without a cooldown or hidden actor");us[0]+=8;
    }
    expect(td_traffic_motion_clear(&ctx,0,us[0],vs[0],us[0]+8,vs[0],0),"cleared body resumes normal motion after a bounded sequence of separating steps");
    reset();people[0].pos.x=16000;people[0].pos.y=16000;
    expect(td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"hidden walkers do not cause an invisible traffic queue");
    people[0].flags=0;
    expect(!td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"visible walker blocks its full foot body");
    UBYTE half_u[6]={10,5,5,5,5,5},half_v[6]={5,5,5,5,5,5};
    reset();us[1]=514*16;vs[1]=500*16;
    expect(td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"stock small car fits a14px centre gap");
    ctx.half_u=half_u;ctx.half_v=half_v;
    expect(!td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"a longer authored truck body closes the same gap");
    half_u[0]=17;
    expect(!td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"unsupported body extent fails closed");
}
static void test_invalid(void){
    reset();
    expect(!td_traffic_motion_clear(NULL,0,8000,8000,8008,8000,0)&&
           !td_traffic_motion_clear(&ctx,6,8000,8000,8008,8000,0)&&
           !td_traffic_motion_clear(&ctx,0,8000,8000,8008,8008,0)&&
           !td_traffic_motion_clear(&ctx,0,8000,8000,8009,8000,0)&&
           !td_traffic_motion_clear(&ctx,0,8001,8000,8008,8000,0)&&
           !td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,2),"invalid pointer, slot, shape, extent, cache mismatch and escape value fail closed");
    ctx.priority_mask=64;expect(!td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"out-of-pool priority mask fails closed");
    reset();us[1]=65535;expect(!td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"invalid other-vehicle geometry cannot silently clear a queue");
    reset();ctx.u=NULL;expect(!td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"missing cache cannot be dereferenced");
    reset();people[0].flags=0;people[0].pos.x=1;people[0].pos.y=1;
    expect(!td_traffic_motion_clear(&ctx,0,8000,8000,8008,8000,0),"invalid visible pedestrian footprint fails closed");
}
static void test_junction_entry(void){
    reset();us[0]=184*16;vs[0]=64*16;
    expect(td_traffic_junction_clear(&ctx,0,0,us[0],vs[0],us[0]+8,vs[0]),"empty authored junction permits entry");
    us[1]=208*16;vs[1]=64*16;
    expect(!td_traffic_junction_clear(&ctx,0,0,us[0],vs[0],us[0]+8,vs[0]),"occupied perpendicular junction holds a new driver before the line");
    expect(td_traffic_junction_clear(&ctx,0,1,us[1],vs[1],us[1]+8,vs[1]),"driver already inside may clear even while an arrival waits");
    us[1]=232*16;
    expect(td_traffic_junction_clear(&ctx,0,0,us[0],vs[0],us[0]+8,vs[0]),"opposite driver waiting at its24px stop line does not falsely occupy the intersection");
    ctx.priority_mask=1;ctx.parked_active=1;ctx.park_u=208*16;ctx.park_v=64*16;
    expect(!td_traffic_junction_clear(&ctx,0,0,us[0],vs[0],us[0]+8,vs[0]),"emergency priority cannot enter a parked-car obstruction");
    ctx.parked_active=0;people[0].flags=0;people[0].pos.x=208*32;people[0].pos.y=64*32;
    expect(!td_traffic_junction_clear(&ctx,0,0,us[0],vs[0],us[0]+8,vs[0]),"a person occupying the crossing keeps the entry line closed");
    people[0].flags=ACTOR_FLAG_HIDDEN;
    expect(td_traffic_junction_clear(&ctx,0,0,us[0],vs[0],us[0]+8,vs[0]),"hidden person does not create an invisible crossing lock");
    expect(!td_traffic_junction_clear(NULL,0,0,us[0],vs[0],us[0]+8,vs[0])&&
           !td_traffic_junction_clear(&ctx,TD_DISTRICT_COUNT,0,us[0],vs[0],us[0]+8,vs[0]),"invalid junction query fails closed");
}
static void test_long_vehicle_junctions(void){
    const UBYTE extents[6]={5,6,5,7,6,7};
    reset();ctx.half_u=ctx.half_v=extents;
    us[3]=184*16;vs[3]=72*16;us[5]=232*16;vs[5]=56*16;
    expect(td_traffic_junction_clear(&ctx,0,3,us[3],vs[3],us[3]+8,vs[3])&&
           td_traffic_junction_clear(&ctx,0,5,us[5],vs[5],us[5]-8,vs[5]),
           "opposing7px fire and bus bodies waiting24px out do not permanently lock both approaches");
    expect(td_traffic_motion_clear(&ctx,3,us[3],vs[3],us[3]+8,vs[3],1),
           "long vehicles on actual separated lane offsets retain safe full-body entry");
    reset();ctx.half_u=ctx.half_v=extents;
    us[3]=184*16;vs[3]=72*16;us[5]=216*16;vs[5]=40*16;
    expect(!td_traffic_signal_stop(0,6,us[3],vs[3],us[3]+8,vs[3])&&
           td_traffic_junction_clear(&ctx,0,3,us[3],vs[3],us[3]+8,vs[3])&&
           td_traffic_motion_clear(&ctx,3,us[3],vs[3],us[3]+8,vs[3],1),
           "last horizontal-green half-pixel entry is safe while a vertical bus waits at its line");
    us[3]+=8;
    expect(!td_traffic_signal_stop(0,7,us[5],vs[5],us[5],vs[5]+8)&&
           !td_traffic_junction_clear(&ctx,0,5,us[5],vs[5],us[5],vs[5]+8),
           "first entered half-pixel occupies the junction when the conflicting phase turns green");
    unsigned advanced=0;
    while(us[3]<232*16&&advanced<100){
        int safe=!td_traffic_signal_stop(0,7,us[3],vs[3],us[3]+8,vs[3])&&
            td_traffic_junction_clear(&ctx,0,3,us[3],vs[3],us[3]+8,vs[3])&&
            td_traffic_motion_clear(&ctx,3,us[3],vs[3],us[3]+8,vs[3],1);
        expect(safe,"already-entered long fire vehicle clears through the changed phase without entering the waiting bus body");
        if(!safe)break;
        us[3]+=8;advanced++;
    }
    expect(us[3]==232*16&&advanced==95&&
           td_traffic_junction_clear(&ctx,0,5,us[5],vs[5],us[5],vs[5]+8)&&
           td_traffic_motion_clear(&ctx,5,us[5],vs[5],us[5],vs[5]+8,1),
           "bounded exit releases the bus under its green phase with both actor positions retained");
    /* Small cars also need immediate admission protection: their first
      23.5px position lies outside the former18+5px body-interior test. */
    reset();us[0]=184*16+8;vs[0]=72*16;us[1]=216*16;vs[1]=40*16;
    expect(!td_traffic_junction_clear(&ctx,0,1,us[1],vs[1],us[1],vs[1]+8),
           "first-half-pixel cross-phase occupancy also protects compact cars");
    /* All cardinal waiting boundaries and subpixel entries use the same
       centre proof; service half extents must not change phase ownership. */
    for(unsigned direction=0;direction<4;direction++)for(unsigned half=5;half<=7;half++){
        UBYTE sizes[6]={5,5,5,5,5,5};sizes[1]=half;
        reset();ctx.half_u=ctx.half_v=sizes;us[0]=184*16;vs[0]=64*16;
        us[1]=208*16;vs[1]=64*16;
        if(direction==0)us[1]-=24*16;else if(direction==1)us[1]+=24*16;
        else if(direction==2)vs[1]-=24*16;else vs[1]+=24*16;
        expect(td_traffic_junction_clear(&ctx,0,0,us[0],vs[0],us[0]+8,vs[0]),
               "every5..7px body waiting on each cardinal line leaves junction admission clear");
        if(direction==0)us[1]+=8;else if(direction==1)us[1]-=8;
        else if(direction==2)vs[1]+=8;else vs[1]-=8;
        expect(!td_traffic_junction_clear(&ctx,0,0,us[0],vs[0],us[0]+8,vs[0]),
               "every5..7px body immediately owns the junction after its first inward half pixel");
    }
}
static void test_admit_sweeps(void){
    const int alongs[]={-200,-144,-64,0,64,144,200},acrosses[]={-200,0,200};
    for(unsigned amount=1;amount<=128;amount++)for(int axis=0;axis<2;axis++)
    for(int direction=-1;direction<=1;direction+=2)for(int escape=0;escape<2;escape++)
    for(int kind=0;kind<3;kind++)for(unsigned a=0;a<sizeof(alongs)/sizeof(alongs[0]);a++)
    for(unsigned b=0;b<sizeof(acrosses)/sizeof(acrosses[0]);b++){
        reset();int bx=8000+(axis?acrosses[b]:alongs[a]),by=8000+(axis?alongs[a]:acrosses[b]);
        int u=8000+(axis?0:direction*(int)amount),v=8000+(axis?direction*(int)amount:0);
        if(kind==0){us[1]=bx;vs[1]=by;}
        else if(kind==1){ctx.parked_active=1;ctx.park_u=bx;ctx.park_v=by;}
        else{people[0].flags=0;people[0].pos.x=bx*2;people[0].pos.y=by*2;}
        td_traffic_context_t before=ctx;UWORD before_u[6],before_v[6];actor_t before_people[6];
        memcpy(before_u,us,sizeof(us));memcpy(before_v,vs,sizeof(vs));memcpy(before_people,people,sizeof(people));
        int margin=(5+(kind==1?7:kind==2?3:5))*16;
        expect(admit_checked(&ctx,0,0,0,8000,8000,u,v,escape)==obstacle_oracle(8000,8000,u,v,bx,by,margin,margin,escape),
               "combined admission retains exact full-body and monotonic escape truth for every1..128Q4 cardinal sweep");
        expect(!memcmp(&before,&ctx,sizeof(ctx))&&!memcmp(before_u,us,sizeof(us))&&!memcmp(before_v,vs,sizeof(vs))&&
               !memcmp(before_people,people,sizeof(people)),"combined traffic admission preserves all caller/cache/actor state");
    }
    reset();UBYTE sizes[6]={1,1,5,5,5,5};ctx.half_u=ctx.half_v=sizes;
    us[0]=496*16;vs[0]=500*16;us[1]=500*16;vs[1]=500*16;
    expect(!overlap(us[0],vs[0],us[1],vs[1],32,32)&&!overlap(504*16,500*16,us[1],vs[1],32,32)&&
           !admit_checked(&ctx,0,0,0,us[0],vs[0],504*16,500*16,1),
           "a wide sweep rejects an intermediate body even though both endpoint bodies are clear");
    reset();ctx.priority_mask=2;us[1]=524*16;vs[1]=500*16;
    expect(!admit_checked(&ctx,0,0,0,8000,8000,8128,8000,1)&&
           admit_checked(&ctx,0,0,0,8000,8000,7872,8000,1),
           "elapsed-time steps still yield while approaching emergency priority and permit safe movement away");
}
static void test_admit_signals(void){
    for(unsigned d=0;d<TD_DISTRICT_COUNT;d++)for(unsigned i=oracle_offsets[d];i<oracle_offsets[d+1];i++){
        const unsigned *node=oracle_signals[i];
        expect(!(node[0]&7)&&!(node[1]&7),"actual generated junction centres preserve the8px alignment required by the combined entry broad phase");
        for(unsigned direction=0;direction<4;direction++){
            int horizontal=direction<2,sign=(direction&1)?-1:1;
            unsigned arm=horizontal?(sign>0?8:2):(sign>0?1:4);
            if(!(node[2]&arm))continue;
            for(unsigned amount=1;amount<=128;amount++)for(unsigned phase=6;phase<=7;phase++){
                int ou=node[0]*16,ov=node[1]*16;
                if(horizontal){ou-=sign*(24*16+(int)amount-1);ov+=8*16;}
                else{ov-=sign*(24*16+(int)amount-1);ou+=8*16;}
                int u=ou+(horizontal?sign*(int)amount:0),v=ov+(horizontal?0:sign*(int)amount);
                if(ou<80||ov<80||u<80||v<80||ou>1019*16||ov>971*16||u>1019*16||v>971*16)continue;
                reset();us[0]=ou;vs[0]=ov;
                expect(admit_checked(&ctx,d,phase,0,ou,ov,u,v,1)==!signal_oracle(d,phase,ou,ov,u,v),
                       "every actual supported approach prevents red overshoot for all1..128Q4 elapsed-time steps");
                if(amount<=8)expect(admit_checked(&ctx,d,phase,0,ou,ov,u,v,1)==
                    (!td_traffic_signal_stop(d,phase,ou,ov,u,v)&&td_traffic_junction_clear(&ctx,d,0,ou,ov,u,v)&&
                     td_traffic_motion_clear(&ctx,0,ou,ov,u,v,1)),"combined small-step result equals the three retained public queries");
            }
        }
    }
    reset();us[0]=180*16;vs[0]=64*16;
    expect(!admit_checked(&ctx,0,7,0,us[0],vs[0],188*16,vs[0],1)&&
           admit_checked(&ctx,0,6,0,us[0],vs[0],188*16,vs[0],1),"an8px overshoot is stopped on red and admitted on green");
    ctx.parked_active=1;ctx.park_u=208*16;ctx.park_v=64*16;
    expect(!admit_checked(&ctx,0,6,0,us[0],vs[0],188*16,vs[0],1),
           "elapsed-time entry still respects an occupied junction before the bodies themselves meet");
    ctx.parked_active=0;us[1]=184*16+8;vs[1]=72*16;us[0]=216*16;vs[0]=36*16;
    expect(!admit_checked(&ctx,0,7,0,us[0],vs[0],us[0],44*16,1),
           "wide conflicting-green entry respects another driver's first half-pixel junction ownership");
}
static void test_admit_invalid_and_stationary(void){
    reset();expect(admit_checked(&ctx,0,65535,0,8000,8000,8000,8000,1),
                   "valid stationary admission preserves endpoint/leg handling without a signal search");
    us[1]=8000;vs[1]=8000;
    expect(!admit_checked(&ctx,0,0,0,8000,8000,8000,8000,1),"stationary overlap is not mistaken for an escaping step");
    reset();
    expect(!admit_checked(NULL,0,0,0,8000,8000,8008,8000,1)&&
           !admit_checked(&ctx,TD_DISTRICT_COUNT,0,0,8000,8000,8008,8000,1)&&
           !admit_checked(&ctx,0,0,6,8000,8000,8008,8000,1)&&
           !admit_checked(&ctx,0,0,0,8000,8000,8129,8000,1)&&
           !admit_checked(&ctx,0,0,0,8000,8000,8008,8008,1)&&
           !admit_checked(&ctx,0,0,0,7999,8000,8008,8000,1)&&
           !admit_checked(&ctx,0,0,0,8000,8000,8008,8000,2)&&
           !admit_checked(&ctx,0,0,0,65535,8000,0,8000,1),
           "combined invalid pointer/district/slot/129step/diagonal/cache/escape/wrap inputs fail closed");
    ctx.priority_mask=64;expect(!admit_checked(&ctx,0,0,0,8000,8000,8008,8000,1),"combined invalid priority fails closed");
    reset();ctx.parked_active=2;expect(!admit_checked(&ctx,0,0,0,8000,8000,8008,8000,1),"combined invalid parked-active flag fails closed");
    reset();ctx.u=NULL;expect(!admit_checked(&ctx,0,0,0,8000,8000,8008,8000,1),"combined missing array fails closed");
    reset();us[1]=65535;expect(!admit_checked(&ctx,0,0,0,8000,8000,8008,8000,1),"far-away invalid actor geometry remains rejected by the optimized common path");
    reset();UBYTE sizes[6]={5,0,5,5,5,5};ctx.half_u=sizes;
    expect(!admit_checked(&ctx,0,0,0,8000,8000,8008,8000,1),"far-away invalid extents remain rejected by the optimized common path");
    reset();ctx.parked_active=1;ctx.park_u=1;ctx.park_v=1;
    expect(!admit_checked(&ctx,0,0,0,8000,8000,8008,8000,1),"combined invalid parked body fails closed");
    reset();people[0].flags=0;people[0].pos.x=1;people[0].pos.y=1;
    expect(!admit_checked(&ctx,0,0,0,8000,8000,8008,8000,1),"combined invalid visible foot body fails closed");
    reset();expect(!td_traffic_signal_stop(0,0,8000,8000,8008,8000)&&
           !td_traffic_motion_clear(&ctx,0,8000,8000,8128,8000,1)&&
           !td_traffic_junction_clear(&ctx,0,0,8000,8000,8128,8000),"standalone rare-path APIs retain their original8Q4 limit");
}
static void test_epoch_protocol(void){
    td_traffic_epoch_t epoch;reset();
    expect(sizeof(epoch)==88,"host snapshot retains exactly the87-byte packed fields plus one alignment byte");
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&
           td_traffic_epoch_admit(&epoch,0,8000,8000,8008,8000,1),"valid epoch begins and admits an ordinary candidate");
    td_traffic_epoch_t before=epoch;
    expect(!td_traffic_epoch_commit(&epoch,1)&&!memcmp(&before,&epoch,sizeof(epoch)),
           "wrong-slot commit leaves the valid pending proposal and every position unchanged");
    expect(td_traffic_epoch_admit(&epoch,1,us[1],vs[1],us[1]+8,vs[1],1)&&
           !td_traffic_epoch_commit(&epoch,0)&&epoch.u[0]==8000,
           "an aborted external terrain/tram guard does not move the first body before a later proposal replaces it");
    expect(td_traffic_epoch_commit(&epoch,1)&&epoch.u[1]==us[1]+8,
           "the later independently guarded proposal commits to its own slot");
    expect(!td_traffic_epoch_admit(&epoch,1,us[1],vs[1],us[1]+8,vs[1],1)&&
           !td_traffic_epoch_commit(&epoch,1),"stale cached origins after commit cannot admit or commit a second movement");
    expect(td_traffic_epoch_admit(&epoch,0,8000,8000,8008,8000,1)&&
           !td_traffic_epoch_admit(&epoch,6,8000,8000,8008,8000,1)&&
           !td_traffic_epoch_commit(&epoch,0),"even an invalid later proposal discards the previous uncommitted ownership");
    us[5]=65535;
    expect(!td_traffic_epoch_begin(&ctx,0,0,&epoch)&&!epoch.valid&&
           !td_traffic_epoch_admit(&epoch,0,8000,8000,8008,8000,1)&&
           !td_traffic_epoch_commit(&epoch,0),"failed initialization rejects malformed distant bodies and invalidates the old epoch");
    reset();UBYTE halves[6]={5,6,5,7,6,7};ctx.half_u=ctx.half_v=halves;
    for(unsigned i=0;i<6;i++){
        halves[i]=0;expect(!td_traffic_epoch_begin(&ctx,0,0,&epoch)&&!epoch.valid,
                           "each malformed distant fleet extent invalidates initialization");halves[i]=5;
    }
    halves[5]=17;expect(!td_traffic_epoch_begin(&ctx,0,0,&epoch),"unsupported distant extent is rejected before any traffic advances");
    reset();people[5].flags=0;people[5].pos.x=65535;people[5].pos.y=65535;
    expect(!td_traffic_epoch_begin(&ctx,0,0,&epoch),"malformed last visible pedestrian fails complete epoch initialization");
    people[5].flags=ACTOR_FLAG_HIDDEN;
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch),"hidden malformed actors are excluded exactly like retained strict queries");
    ctx.parked_active=1;ctx.park_u=ctx.park_v=1;
    expect(!td_traffic_epoch_begin(&ctx,0,0,&epoch),"malformed active parked body invalidates initialization");
    reset();ctx.priority_mask=64;expect(!td_traffic_epoch_begin(&ctx,0,0,&epoch),"unsupported priority bits invalidate initialization");
    reset();ctx.parked_active=2;expect(!td_traffic_epoch_begin(&ctx,0,0,&epoch),"unsupported parked flag invalidates initialization");
    reset();ctx.v=NULL;expect(!td_traffic_epoch_begin(&ctx,0,0,&epoch),"missing cache array invalidates initialization");
    expect(!td_traffic_epoch_begin(NULL,0,0,&epoch)&&!td_traffic_epoch_begin(&ctx,0,0,NULL)&&
           !td_traffic_epoch_admit(NULL,0,8000,8000,8008,8000,1)&&!td_traffic_epoch_commit(NULL,0),
           "NULL snapshot/context inputs fail closed");
    reset();expect(!td_traffic_epoch_begin(&ctx,TD_DISTRICT_COUNT,0,&epoch),"unknown district invalidates initialization");
    /* Immutable actor/park/priority/time copies require a fresh begin when
     * their producer changes. No stale live pointer is read by admission. */
    reset();expect(td_traffic_epoch_begin(&ctx,0,0,&epoch),"a new actor snapshot starts from valid geometry");
    people[0].flags=0;people[0].pos.x=506*32;people[0].pos.y=500*32;
    expect(td_traffic_epoch_admit(&epoch,0,8000,8000,8008,8000,1),"captured hidden-actor snapshot does not drift through live pointers");
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&
           !td_traffic_epoch_admit(&epoch,0,8000,8000,8008,8000,1),"required reinitialization captures a newly visible obstruction before movement");
    reset();expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&
           td_traffic_epoch_admit(&epoch,0,8000,8000,8008,8000,1),"masked-slot fixture begins with an externally aborted proposal");
    /* Slot1 is masked out completely: neither admit nor commit runs for it. */
    expect(epoch.u[0]==us[0]&&epoch.v[0]==vs[0]&&epoch.u[1]==us[1]&&epoch.v[1]==vs[1],
           "an aborted candidate stays invisible across a wholly skipped subsequent slot");
    expect(td_traffic_epoch_admit(&epoch,2,us[2],vs[2],us[2]+8,vs[2],1)==
           td_traffic_admit(&ctx,0,0,2,us[2],vs[2],us[2]+8,vs[2],1)&&
           td_traffic_epoch_commit(&epoch,2)&&!td_traffic_epoch_commit(&epoch,0)&&epoch.u[0]==us[0],
           "later accepted slot replaces pending ownership while the aborted and masked positions remain unchanged");
}
static void test_epoch_sequential(void){
    static const unsigned amounts[]={0,1,8,64,127,128,129};
    static const UBYTE sizes[6]={5,6,5,7,6,7};
    for(unsigned district=0;district<TD_DISTRICT_COUNT;district++)for(unsigned phase=0;phase<12;phase++)
    for(unsigned variant=0;variant<32;variant++){
        reset();ctx.half_u=ctx.half_v=sizes;ctx.priority_mask=variant&31;
        us[0]=184*16;vs[0]=72*16;us[1]=216*16;vs[1]=40*16;
        if(variant&1){ctx.parked_active=1;ctx.park_u=208*16;ctx.park_v=64*16;}
        if(variant&2){people[0].flags=0;people[0].pos.x=208*32;people[0].pos.y=64*32;}
        td_traffic_epoch_t epoch;
        expect(td_traffic_epoch_begin(&ctx,district,phase,&epoch),"whole six-mover epoch initializes real varying bodies/priority/people/park state");
        for(unsigned slot=0;slot<6;slot++){
            int direction=(variant+slot)&3,amount=amounts[(variant+slot)%7];
            UWORD u=us[slot],v=vs[slot];
            if(direction==0)u+=amount;else if(direction==1)v+=amount;
            else if(direction==2)u-=amount;else v-=amount;
            UBYTE expected=td_traffic_admit(&ctx,district,phase,slot,us[slot],vs[slot],u,v,1);
            UBYTE actual=td_traffic_epoch_admit(&epoch,slot,us[slot],vs[slot],u,v,1);
            expect(actual==expected,"sequential epoch admission equals strict live-cache decisions after every previous guarded commit");
            expect(epoch.u[slot]==us[slot]&&epoch.v[slot]==vs[slot],"the candidate never moves before external guard acceptance");
            int external_clear=((variant+slot)%3)!=0;
            if(actual&&external_clear){
                expect(td_traffic_epoch_commit(&epoch,slot),"an admitted externally clear movement commits sequentially");
                us[slot]=u;vs[slot]=v;
            }else if(!actual)expect(!td_traffic_epoch_commit(&epoch,slot),"red/occupied/body/invalid proposals cannot commit");
            expect(!memcmp(us,epoch.u,sizeof(us))&&!memcmp(vs,epoch.v,sizeof(vs)),
                   "later movers see exactly the accepted live positions, excluding every aborted external guard");
            expect(fleet_buckets_match(&epoch,&ctx),"fleet buckets track only committed live bodies throughout each sequential movement epoch");
        }
    }
}
static void test_epoch_buckets(void){
    UBYTE half_u[6],half_v[6];
    /* Every public extent, both axes, fractional centres and every slot.
     * Alternate between an external abort and accepted commit; queries may
     * change pending ownership but must never move the obstacle bucket. */
    for(unsigned half=1;half<=16;half++)for(unsigned slot=0;slot<6;slot++)for(unsigned axis=0;axis<2;axis++){
        reset();ctx.half_u=half_u;ctx.half_v=half_v;
        for(unsigned i=0;i<6;i++){
            half_u[i]=(half+i-1)%16+1;half_v[i]=(16+half-i-1)%16+1;
            us[i]=(80+i*150)*16+((half+i)&15);vs[i]=(400+i*48)*16+((half+3*i)&15);
        }
        td_traffic_epoch_t epoch;
        expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&fleet_buckets_match(&epoch,&ctx),
               "each fleet bucket begins from exact validated fractional centres and independent asymmetric extents");
        UWORD u=us[slot]+(axis?0:128),v=vs[slot]+(axis?128:0);
        UBYTE strict=td_traffic_admit(&ctx,0,0,slot,us[slot],vs[slot],u,v,1);
        UBYTE actual=td_traffic_epoch_admit(&epoch,slot,us[slot],vs[slot],u,v,1);
        expect(actual==strict&&fleet_buckets_match(&epoch,&ctx),"full8px proposal uses the same open body boundaries without prematurely changing cached buckets");
        if(actual){
            expect(td_traffic_epoch_commit(&epoch,slot),"admitted bucket fixture commits only its accepted pending body");
            us[slot]=u;vs[slot]=v;
            expect(fleet_buckets_match(&epoch,&ctx),"commit refreshes both buckets of only the moved fleet body");
        }
        /* Leave this second accepted/rejected proposal uncommitted. */
        u=us[slot]-(axis?0:1);v=vs[slot]-(axis?1:0);
        expect(td_traffic_epoch_admit(&epoch,slot,us[slot],vs[slot],u,v,1)==
               td_traffic_admit(&ctx,0,0,slot,us[slot],vs[slot],u,v,1)&&fleet_buckets_match(&epoch,&ctx),
               "an aborted reverse proposal leaves current buckets resident for every later mover");
        td_traffic_epoch_t before=epoch;
        expect(!td_traffic_epoch_commit(&epoch,(slot+1)%6)&&!memcmp(&before,&epoch,sizeof(epoch)),
               "wrong-slot commit leaves pending ownership and all fleet buckets unchanged");
    }
    /* Exact touching edges remain open; a single Q4 penetration blocks.
     * Validated near-map-boundary bodies must not wrap cache arithmetic. */
    reset();ctx.half_u=half_u;ctx.half_v=half_v;
    for(unsigned i=0;i<6;i++)half_u[i]=half_v[i]=16;
    us[0]=16*16;vs[0]=16*16;us[1]=48*16;vs[1]=16*16;
    td_traffic_epoch_t epoch;
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&fleet_buckets_match(&epoch,&ctx)&&epoch.bucket_u[0]==1&&epoch.bucket_v[0]==1,
           "minimum valid16px body produces exact near-map buckets without unsigned wrap");
    expect(td_traffic_epoch_admit(&epoch,0,us[0],vs[0],us[0],vs[0],0)&&
           !td_traffic_epoch_admit(&epoch,0,us[0],vs[0],us[0]+1,vs[0],0),
           "exact fleet body tangency is clear while first fractional penetration is blocked");
    us[0]=(1024-16)*16;vs[0]=(976-16)*16;
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&fleet_buckets_match(&epoch,&ctx)&&
           epoch.bucket_u[0]==63&&epoch.bucket_v[0]==60,
           "maximum valid body caches exact local-map far buckets without truncation");
    expect(!td_traffic_epoch_admit(&epoch,0,us[0],vs[0],us[0]+1,vs[0],1)&&
           fleet_buckets_match(&epoch,&ctx)&&!td_traffic_epoch_commit(&epoch,0),
           "out-of-bounds candidate fails closed before it can corrupt cached fleet buckets");
}
static void bucket_pair(unsigned old_fraction,unsigned other_fraction,unsigned gap,
    unsigned axis,int direction,unsigned amount,unsigned priority,unsigned escape){
    int old=32*256+(int)old_fraction,other=(32+direction*(int)gap)*256+(int)other_fraction;
    int next=old+direction*(int)amount,ou=axis?8000:old,ov=axis?old:8000;
    int u=axis?8000:next,v=axis?next:8000,bu=axis?8000:other,bv=axis?other:8000;
    reset();static const UBYTE sizes[6]={16,16,16,16,16,16};ctx.half_u=ctx.half_v=sizes;
    us[0]=ou;vs[0]=ov;us[1]=bu;vs[1]=bv;ctx.priority_mask=priority?2:0;
    int expected=obstacle_oracle(ou,ov,u,v,bu,bv,32*16,32*16,escape);
    int before=old-other,after=next-other;
    if(before<0)before=-before;
    if(after<0)after=-after;
    if(priority&&after<before&&overlap(u,v,bu,bv,50*16,50*16))expected=0;
    td_traffic_epoch_t epoch;
    int begun=td_traffic_epoch_begin(&ctx,0,0,&epoch);
    expect(begun&&fleet_buckets_match(&epoch,&ctx)&&
           td_traffic_epoch_admit(&epoch,0,ou,ov,u,v,escape)==expected,
           "byte bucket rejection preserves independent largest-body sweep/escape/priority truth at each threshold fraction");
}
static void test_bucket_threshold(void){
    /* Exhaust every fractional pair on BOTH sides of the4/5-cell threshold,
     * in both axis orders and directions. The full128Q4 move towards the
     * other body is the worst separation; smaller movement cannot newly
     * enter its50px maximum-priority neighbourhood if this move is clear. */
    for(unsigned a=0;a<256;a++)for(unsigned b=0;b<256;b++)
    for(unsigned axis=0;axis<2;axis++)for(int direction=-1;direction<=1;direction+=2)
    for(unsigned gap=4;gap<=5;gap++)bucket_pair(a,b,gap,axis,direction,128,(a+b)&1,(a>>1)&1);
    /* Near-body geometry remains exact. Cross every boundary fraction with
     * all allowed sweep amounts, escape states and priority states, including
     * co-located bodies and centres just either side of a bucket edge. */
    static const unsigned fractions[]={0,1,127,128,254,255};
    for(unsigned a=0;a<6;a++)for(unsigned b=0;b<6;b++)for(unsigned gap=0;gap<=5;gap++)
    for(unsigned axis=0;axis<2;axis++)for(int direction=-1;direction<=1;direction+=2)
    for(unsigned amount=0;amount<=128;amount++)for(unsigned priority=0;priority<2;priority++)
    for(unsigned escape=0;escape<2;escape++)
        bucket_pair(fractions[a],fractions[b],gap,axis,direction,amount,priority,escape);
    /* This case must remain near: a4-cell difference can still require
     * civilian yielding. A5-cell difference at its closest edge is clear. */
    reset();static const UBYTE sizes[6]={16,16,16,16,16,16};ctx.half_u=ctx.half_v=sizes;
    us[0]=32*256+255;vs[0]=8000;us[1]=36*256;vs[1]=8000;ctx.priority_mask=2;
    td_traffic_epoch_t epoch;
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&
           !td_traffic_epoch_admit(&epoch,0,us[0],vs[0],us[0]+128,vs[0],1),
           "closest4-cell gap still yields to approaching priority body despite clearing its physical hull");
    us[1]=37*256;
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&
           td_traffic_epoch_admit(&epoch,0,us[0],vs[0],us[0]+128,vs[0],1),
           "closest5-cell gap is safely rejected before word geometry even when the mover enters the next bucket");
}
static int epoch_live_matches(const td_traffic_epoch_t *epoch){
    return !memcmp(us,epoch->u,sizeof(us))&&!memcmp(vs,epoch->v,sizeof(vs))&&fleet_buckets_match(epoch,&ctx);
}
static void test_epoch_move(void){
    static const UBYTE sizes[6]={5,6,5,7,6,7};
    static const unsigned amounts[]={0,1,8,127,128,129};
    for(unsigned district=0;district<TD_DISTRICT_COUNT;district++)for(unsigned slot=0;slot<6;slot++)
    for(unsigned axis=0;axis<2;axis++)for(int direction=-1;direction<=1;direction+=2)
    for(unsigned a=0;a<6;a++){
        reset();guard_reset(district);ctx.half_u=ctx.half_v=sizes;
        us[0]=120*16;vs[0]=900*16;us[slot]=8000+15;vs[slot]=8000+7;
        UWORD old_u=us[slot],old_v=vs[slot],u=old_u+(axis?0:direction*(int)amounts[a]);
        UWORD v=old_v+(axis?direction*(int)amounts[a]:0);
        td_traffic_epoch_t epoch;
        expect(td_traffic_epoch_begin(&ctx,district,0,&epoch),"fused move begins from valid real fleet extents in every district");
        UBYTE expected=td_traffic_admit(&ctx,district,0,slot,old_u,old_v,u,v,1);
        UBYTE moved=td_traffic_epoch_move(&epoch,slot,u,v,0,0);
        expect(moved==expected,"fused clear-ground movement preserves strict admission at every motion-limit and fractional whole-pixel boundary");
        if(moved){
            expect(road_calls==1&&tram_calls==1&&road_order<tram_order&&
                   road_call.old_u==old_u/16&&road_call.old_v==old_v/16&&road_call.u==u/16&&road_call.v==v/16&&road_call.half==sizes[slot]&&
                   tram_call.old_u==old_u&&tram_call.old_v==old_v&&tram_call.u==u&&tram_call.v==v&&tram_call.half==sizes[slot]&&tram_call.district==district,
                   "fused guards run road first with full whole-pixel hull, then matching district and exact Q4 future-tram hull");
            us[slot]=u;vs[slot]=v;
        }else expect(!road_calls&&!tram_calls,"red/body/129Q4 rejection occurs before either external guard");
        expect(epoch_live_matches(&epoch)&&epoch.pending_slot==255&&!td_traffic_epoch_commit(&epoch,slot),
               "fused success privately commits exactly once while every denied move remains uncommittable and immobile");
    }
    /* A discarded previous pending candidate cannot survive any rejection. */
    for(unsigned failure=0;failure<3;failure++){
        reset();guard_reset(0);td_traffic_epoch_t epoch;
        expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&td_traffic_epoch_admit(&epoch,2,us[2],vs[2],us[2]+8,vs[2],1),
               "guard-abort fixture starts with an older independent uncommitted proposal");
        if(failure==0){road_solid=1;solid_left=solid_right=64;solid_top=solid_bottom=62;}
        else if(failure==1){tram_body=1;tram_left=510*16;tram_right=520*16;tram_top=495*16;tram_bottom=505*16;}
        else guard_loaded=1;
        expect(!td_traffic_epoch_move(&epoch,0,508*16,500*16,0,0)&&epoch_live_matches(&epoch)&&epoch.pending_slot==255&&
               !td_traffic_epoch_commit(&epoch,0)&&!td_traffic_epoch_commit(&epoch,2),
               "terrain/future-tram/presentation mismatch discards all pending ownership without moving any body or bucket");
        expect(road_calls==1&&tram_calls==(failure?1:0),"terrain rejection short-circuits future tram; a clear road reaches the matching future guard once");
        road_solid=tram_body=0;guard_loaded=0;
        expect(td_traffic_epoch_move(&epoch,1,us[1]+8,vs[1],0,0),"a later clear mover can commit after another body's external guard abort");
        us[1]+=8;
        expect(epoch_live_matches(&epoch)&&epoch.u[0]==500*16&&epoch.u[2]==us[2],"later fused commits see original aborted bodies and only accepted earlier endpoints");
    }
    reset();guard_reset(0);tram_body=1;tram_left=513*16;tram_right=520*16;tram_top=495*16;tram_bottom=505*16;
    td_traffic_epoch_t epoch;
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&td_traffic_epoch_move(&epoch,0,508*16,500*16,0,0),
           "full future body tangency stays open at its exact Q4 edge");
    reset();guard_reset(0);tram_body=1;tram_left=513*16-1;tram_right=520*16;tram_top=495*16;tram_bottom=505*16;
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&!td_traffic_epoch_move(&epoch,0,508*16,500*16,0,0)&&epoch_live_matches(&epoch),
           "one-Q4 future tram penetration fails despite a clear whole-pixel road");
    /* Normal, retreat, and separately caller-proven separating retreat have
     * distinct policies. Occupied junction admission stays in normal moves;
     * retreat still has signals/body guards and a strict8Q4 continuous limit. */
    reset();guard_reset(0);us[0]=184*16;vs[0]=72*16;us[1]=208*16;vs[1]=48*16;
    expect(td_traffic_epoch_begin(&ctx,0,6,&epoch)&&!td_traffic_epoch_move(&epoch,0,us[0]+8,vs[0],0,0)&&!road_calls&&!tram_calls,
           "fused normal move retains occupied-junction entry before external guards");
    expect(td_traffic_epoch_move(&epoch,0,us[0]+8,vs[0],1,0)&&road_calls==1&&tram_calls==1,
           "rare legacy-policy retreat omits only entry while retaining road and future-tram checks");
    us[0]+=8;expect(epoch_live_matches(&epoch),"retreat commit updates the same body and bucket sequence as ordinary traffic");
    reset();guard_reset(0);tram_body=1;tram_left=490*16;tram_right=499*16;tram_top=495*16;tram_bottom=505*16;
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&!td_traffic_epoch_move(&epoch,0,8008,8000,1,0)&&road_calls==1&&tram_calls==1,
           "retreat without separate caller proof cannot waive its obstructing future tram body");
    guard_reset(0);tram_body=1;tram_left=490*16;tram_right=499*16;tram_top=495*16;tram_bottom=505*16;
    /* The fixture's courier/tram centres lie behind the old position; this
     * half-pixel forward segment strictly separates. Actual route/current-
     * and future-tram retreat proof remains the caller's native responsibility. */
    expect(td_traffic_epoch_move(&epoch,0,8008,8000,1,1)&&road_calls==1&&!tram_calls,
           "only explicit caller-proven separating retreat skips future admission while preserving the whole-body road guard");
    us[0]=8008;expect(epoch_live_matches(&epoch),"separating retreat privately commits once and refreshes the shared bucket cache");
    reset();guard_reset(0);us[0]=184*16;vs[0]=72*16;
    expect(td_traffic_epoch_begin(&ctx,0,7,&epoch)&&!td_traffic_epoch_move(&epoch,0,us[0]+8,vs[0],1,1)&&!road_calls&&!tram_calls,
           "even caller-proven separating retreat still obeys a red stop line");
    reset();guard_reset(0);us[1]=510*16;vs[1]=500*16;
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&!td_traffic_epoch_move(&epoch,0,8008,8000,1,1)&&!road_calls&&!tram_calls,
           "separating proof does not waive a newly entered neighbouring fleet body");
    reset();guard_reset(0);road_solid=1;solid_left=solid_right=63;solid_top=solid_bottom=62;
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&!td_traffic_epoch_move(&epoch,0,8008,8000,1,1)&&road_calls==1&&!tram_calls&&epoch_live_matches(&epoch),
           "caller-proven separating retreat still rejects a solid road tile and never commits");
    /* All input failures precede external guards. Invalid snapshots, flags,
     * unsupported footprints and coordinates cannot revive older pending. */
    for(unsigned invalid=0;invalid<13;invalid++){
        reset();guard_reset(0);UBYTE halves[6]={5,6,5,7,6,7},vertical[6]={5,6,5,7,6,7};ctx.half_u=halves;ctx.half_v=vertical;
        if(invalid==8)vertical[0]=6;
        if(invalid==9)halves[0]=vertical[0]=4;
        if(invalid==10)halves[0]=vertical[0]=9;
        expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&td_traffic_epoch_admit(&epoch,2,us[2],vs[2],us[2]+8,vs[2],1),
               "invalid fused request begins with a real but unrelated pending candidate");
        UBYTE slot=0,retreat=0,separating=0;UWORD u=8008,v=8000;
        if(invalid==0)slot=6;else if(invalid==1)retreat=2;else if(invalid==2)separating=2;
        else if(invalid==3)separating=1;else if(invalid==4)u=8129;
        else if(invalid==5){u=8008;v=8008;}else if(invalid==6)u=65535;
        else if(invalid==7){retreat=1;u=8009;}else if(invalid==11)epoch.valid=0;
        else if(invalid==12)epoch.phase=12;
        expect(!td_traffic_epoch_move(&epoch,slot,u,v,retreat,separating)&&epoch_live_matches(&epoch)&&
               epoch.pending_slot==255&&!road_calls&&!tram_calls&&!td_traffic_epoch_commit(&epoch,2),
               "invalid fused flags/slot/limits/body/metadata discard pending and fail closed before external queries");
    }
    expect(!td_traffic_epoch_move(NULL,0,8008,8000,0,0),"NULL fused snapshot fails closed");
    reset();guard_reset(0);us[0]=vs[0]=5*16;
    expect(td_traffic_epoch_begin(&ctx,0,0,&epoch)&&!td_traffic_epoch_move(&epoch,0,us[0],vs[0],0,0)&&
           road_calls==1&&!tram_calls&&epoch_live_matches(&epoch),"full road centre-domain bounds reject a valid5px snapshot outside the loaded engine's8px inset");
}
static void test_epoch_retreat(void){
    const int offsets[]={-170,-144,-64,0,64,144,170};
    for(unsigned amount=0;amount<=9;amount++)for(unsigned axis=0;axis<2;axis++)
    for(int direction=-1;direction<=1;direction+=2)for(unsigned escape=0;escape<2;escape++)
    for(unsigned kind=0;kind<3;kind++)for(unsigned a=0;a<sizeof(offsets)/sizeof(offsets[0]);a++){
        reset();UWORD u=8000+(axis?0:direction*(int)amount),v=8000+(axis?direction*(int)amount:0);
        UWORD other_u=8000+(axis?0:offsets[a]),other_v=8000+(axis?offsets[a]:0);
        if(kind==0){us[1]=other_u;vs[1]=other_v;}
        else if(kind==1){ctx.parked_active=1;ctx.park_u=other_u;ctx.park_v=other_v;}
        else{people[0].flags=0;people[0].pos.x=other_u*2;people[0].pos.y=other_v*2;}
        td_traffic_epoch_t epoch;expect(td_traffic_epoch_begin(&ctx,0,0,&epoch),"retreat snapshot captures valid nearby obstructions");
        UBYTE expected=!td_traffic_signal_stop(0,0,8000,8000,u,v)&&
            td_traffic_motion_clear(&ctx,0,8000,8000,u,v,escape);
        UBYTE actual=td_traffic_epoch_retreat_admit(&epoch,0,8000,8000,u,v,escape);
        expect(actual==expected,"rare epoch retreat exactly matches legacy8Q4 signal/body/priority policy");
        expect(epoch.u[0]==8000&&epoch.v[0]==8000,"retreat query does not move before caller's monotonic route/terrain/tram proof");
        if(actual){expect(td_traffic_epoch_commit(&epoch,0),"fully guarded legacy-policy retreat commits through normal pending ownership");
            us[0]=u;vs[0]=v;
            expect(td_traffic_epoch_admit(&epoch,1,us[1],vs[1],us[1]+8,vs[1],1)==
                td_traffic_admit(&ctx,0,0,1,us[1],vs[1],us[1]+8,vs[1],1),
                "committed retreat affects a later ordinary mover exactly like the strict live cache");}
    }
    reset();us[0]=184*16;vs[0]=72*16;us[1]=208*16;vs[1]=48*16;
    td_traffic_epoch_t epoch;expect(td_traffic_epoch_begin(&ctx,0,6,&epoch),"rare-entry fixture initializes a separated occupied junction");
    expect(!td_traffic_epoch_admit(&epoch,0,us[0],vs[0],us[0]+8,vs[0],1)&&
           td_traffic_epoch_retreat_admit(&epoch,0,us[0],vs[0],us[0]+8,vs[0],1),
           "only the externally validated rare retreat omits occupied-junction admission, while ordinary128Q4 policy remains intact");
    expect(td_traffic_epoch_begin(&ctx,0,7,&epoch)&&
           !td_traffic_epoch_retreat_admit(&epoch,0,us[0],vs[0],us[0]+8,vs[0],1)&&
           !td_traffic_epoch_commit(&epoch,0),"rare retreat never gains permission to run a red light");
}
int main(void){
    test_signals();test_body_truth();test_priority_and_traps();test_invalid();test_junction_entry();test_long_vehicle_junctions();
    test_admit_sweeps();test_admit_signals();test_admit_invalid_and_stationary();
    test_epoch_protocol();test_epoch_sequential();test_epoch_retreat();test_epoch_buckets();test_bucket_threshold();test_epoch_move();
    printf("Traffic helpers: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
