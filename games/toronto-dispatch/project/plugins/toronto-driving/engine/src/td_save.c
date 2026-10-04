#pragma bank 255
#include <string.h>
#include "td_game.h"
#include "td_district.h"
#include "td_transit.h"
#include "td_island_legacy.h"
#include "compat.h"
#include "system.h"
static UBYTE td_save_slot=TD_NONE,td_save_seq;
extern UBYTE td_resume_mode;

static UWORD td_crc_byte(UWORD crc,UBYTE value){
    UBYTE bit;crc^=(UWORD)value<<8;
    for(bit=0;bit<8;bit++)crc=(crc&0x8000)?(crc<<1)^0x1021:crc<<1;
    return crc;
}
static volatile UBYTE *td_save_address(UBYTE slot){return slot?(volatile UBYTE*)0xA180:(volatile UBYTE*)0xA100;}
/* Field and old-origin validity must precede any map transform. A forged
 * invalid candidate cannot be repaired by choosing a safe Island checkpoint. */
static UBYTE td_valid_fields(td_state_t *s,UBYTE version){
    UBYTE i,bits=0,value;td_job_t job;td_stop_t stop;
    if(s->wanted>3||s->wanted_left>30||(!s->wanted!=!s->wanted_left))return FALSE;
    if(s->vehicle>3||s->heading>15||s->onfoot>1||s->health>100||s->subsecond>=60||s->mode>TD_HELP)return FALSE;
    if(s->u>=1024*16||s->v>=976*16||s->park_u>=1024*16||s->park_v>=976*16)return FALSE;
    if(s->district>=TD_DISTRICT_COUNT||s->park_district>=TD_DISTRICT_COUNT||(s->reserved&~TD_STREETCAR_HOLD))return FALSE;
    if(version<9&&(s->district>=TD_DISTRICT_ISLANDS||s->park_district>=TD_DISTRICT_ISLANDS))return FALSE;
    if(s->reserved&&(s->mode!=TD_RIDE||s->ride_left!=1||td_transit_service(s->transit_origin)!=TD_TRANSIT_STREETCAR))return FALSE;
    for(i=0;i<TD_COMPLETE_BYTES;i++){
        value=s->complete[i];
        if(i>=(TD_QUESTS+7)/8&&value)return FALSE;
#if (TD_QUESTS & 7)
        if(i==TD_QUESTS/8&&value>>(TD_QUESTS&7))return FALSE;
#endif
        while(value){bits+=value&1;value>>=1;}
    }
    if(bits!=s->done)return FALSE;
    if(s->job!=TD_NONE){if(s->job>=TD_QUESTS)return FALSE;td_get_job(s->job,&job);if(s->stage>=job.count||!s->left||!s->health)return FALSE;}
    if(s->mode==TD_WAIT||s->mode==TD_RIDE){
        if(!s->onfoot||(s->transit_origin&128)||(s->transit_origin&63)>=TD_STOPS||s->transit_target>=TD_STOPS||s->transit_target==(s->transit_origin&63))return FALSE;
        td_get_stop(s->transit_origin&63,&stop);
        if(!stop.transit)return FALSE;
        if(version<9&&s->transit_origin>=20&&s->transit_origin<=22){
            if(s->district!=TD_DISTRICT_CITY)return FALSE;
        }else if(stop.district!=s->district)return FALSE;
        if(!td_transit_valid(s->transit_origin,s->transit_target))return FALSE;
        if(s->mode==TD_RIDE&&(!s->ride_left||s->ride_left>td_transit_duration(s->transit_origin,s->transit_target)))return FALSE;
    }
    return TRUE;
}
static UBYTE td_valid_state(td_state_t *s){
    return td_valid_fields(s,TD_SAVE_VERSION)&&
        td_district_drivable(s->park_district,s->park_u>>4,s->park_v>>4)&&
        (s->onfoot?td_district_walkable(s->district,s->u>>4,s->v>>4):td_district_drivable(s->district,s->u>>4,s->v>>4));
}

/* Whole-pixel historical point classification: NONE is outside all old
 * rectangles;254 is an old blocked tile and cannot fall through to new land. */
static UBYTE td_legacy_island_region(UWORD u,UWORD v){
    UBYTE region,x,y;UWORD bit;
    if(u>=1024||v>=976)return TD_NONE;
    x=u>>3;y=v>>3;
    for(region=0;region<3;region++){
        const UBYTE *r=td_legacy_island_regions[region];
        if(x<r[0]||y<r[1]||x-r[0]>=r[2]||y-r[1]>=r[3])continue;
        bit=(UWORD)(y-r[1])*r[2]+x-r[0];
        return td_legacy_island_masks[r[4]+(bit>>3)]&(1<<(bit&7))?region:254;
    }
    return TD_NONE;
}
static UBYTE td_legacy_near(UWORD u,UWORD v,const UWORD *point){
    return (u>point[0]?u-point[0]:point[0]-u)<15&&
        (v>point[1]?v-point[1]:point[1]-v)<15;
}
static UBYTE td_island_checkpoint(td_state_t *s,UBYTE stop_id){
    td_stop_t stop;UWORD x,y;
    td_get_stop(stop_id,&stop);
    if(stop.district!=TD_DISTRICT_ISLANDS||stop.u<3||stop.u>1020||stop.v<3||stop.v>972)return FALSE;
    /* A six-pixel span touches at most two tiles per axis. Endpoints and
       midpoint cover the exact full half3 landing body without point-only
       acceptance or relying on a currently loaded scene's collision bank. */
    for(y=stop.v-3;y<=stop.v+3;y+=3)for(x=stop.u-3;x<=stop.u+3;x+=3)
        if(!td_district_walkable(stop.district,x,y))return FALSE;
    s->district=stop.district;s->u=s->safe_u=stop.u*16;s->v=s->safe_v=stop.v*16;
    return TRUE;
}
static UBYTE td_prepare_state(td_state_t *s,UBYTE version){
    UBYTE region=TD_NONE,origin_region;UWORD u=s->u>>4,v=s->v>>4;
    if(!td_valid_fields(s,version)||
       !td_district_drivable(s->park_district,s->park_u>>4,s->park_v>>4))return FALSE;
    if(version<9&&s->district==TD_DISTRICT_CITY)region=td_legacy_island_region(u,v);
    if(region==254)return FALSE;
    if(region<3){if(!s->onfoot)return FALSE;}
    else if(!(s->onfoot?td_district_walkable(s->district,u,v):td_district_drivable(s->district,u,v)))return FALSE;
    if(version<9&&(s->mode==TD_WAIT||s->mode==TD_RIDE)&&s->transit_origin>=20&&s->transit_origin<=22){
        origin_region=s->transit_origin-20;
        if(td_legacy_near(u,v,td_legacy_island_docks[origin_region])){
            if(region!=origin_region||!td_island_checkpoint(s,s->transit_origin))return FALSE;
            return td_valid_state(s);
        }
        /* Genuine paid rides never leave their <15px boarding point. v8's
           broader validator accepted inconsistent forged RIDE geometry;
           reject that case rather than inventing or refunding paid travel. */
        if(s->mode==TD_RIDE)return FALSE;
        s->mode=TD_ROAM; /* An away WAIT was unpaid; do not charge or snap it. */
    }
    if(region<3){
        UBYTE stop_id=td_legacy_near(u,v,td_legacy_island_clients[region])?24+region:20+region;
        if(!td_island_checkpoint(s,stop_id))return FALSE;
    }
    return td_valid_state(s);
}
void td_save(void) BANKED {
    UBYTE i,slot=td_save_slot==0?1:0,mode=td.mode;UWORD crc=0xFFFF;
    const UBYTE *src=(const UBYTE*)&td;volatile UBYTE *ram=td_save_address(slot);
    /* Two records in SRAM bank 3 retain the last committed snapshot during power loss. */
    if(mode==TD_PAUSE||mode==TD_MAP||mode==TD_HELP)td.mode=td_resume_mode;
    if(td.mode!=TD_WAIT&&td.mode!=TD_RIDE)td.mode=TD_ROAM;
    if(td.mode!=TD_RIDE)td.reserved=0;
    ENABLE_RAM_MBC5;SWITCH_RAM_BANK(3,RAM_BANKS_ONLY);
    ram[0]=0;ram[1]=0xD7;ram[2]=TD_SAVE_VERSION;ram[3]=sizeof(td);ram[4]=td_save_seq+1;
    crc=td_crc_byte(crc,ram[2]);crc=td_crc_byte(crc,ram[3]);crc=td_crc_byte(crc,ram[4]);
    for(i=0;i<sizeof(td);i++){ram[8+i]=src[i];crc=td_crc_byte(crc,src[i]);}
    ram[5]=crc;ram[6]=crc>>8;ram[7]=0;ram[0]=0x54;
    td_save_slot=slot;td_save_seq=ram[4];SWITCH_RAM_BANK(0,RAM_BANKS_ONLY);td.mode=mode;
}
static void td_migrate_old(td_state_t *dest){
    UBYTE *dst=(UBYTE*)dest;
    /* v4/v5 were exactly48 bytes. Contracts0..71 and their IDs stay unchanged. */
    /* Move tail fields first so expansion needs no second stack buffer. */
    memmove(dst+48,dst+40,8);memmove(dst+42,dst+35,5);
    memset(dst+35,0,7);dst[47]=0;dst[56]=dst[57]=0;
}
static UBYTE td_read_slot(UBYTE slot,td_state_t *dest,UBYTE *seq,UBYTE *source_version){
    UBYTE i,version,length;UWORD crc=0xFFFF;UBYTE *dst=(UBYTE*)dest;volatile UBYTE *ram=td_save_address(slot);
    version=ram[2];length=ram[3];
    if(ram[0]!=0x54||ram[1]!=0xD7||!(((version==TD_SAVE_VERSION||version==8||version==7||version==6)&&length==sizeof(td))||(version==5&&length==48)))return FALSE;
    for(i=2;i<=4;i++)crc=td_crc_byte(crc,ram[i]);
    for(i=0;i<length;i++){dst[i]=ram[8+i];crc=td_crc_byte(crc,ram[8+i]);}
    if(crc!=(ram[5]|(UWORD)ram[6]<<8))return FALSE;
    /* Legacy v6 reserved bits were required to be zero. Do not reinterpret
       a corrupt legacy record as the new paid-arrival hold flag. */
    if(version==6&&dest->reserved)return FALSE;
    if(version==5)td_migrate_old(dest);
    /* v4..7 cursor words never drove the atlas; do not turn them into heat. */
    if(version<8)dest->wanted=dest->wanted_left=0;
    *seq=ram[4];*source_version=version;return TRUE;
}
UBYTE td_restore(void) BANKED {
    td_state_t candidate;UBYTE valid,seq=0,version=0,slot,i,check=0;volatile UBYTE *ram=td_save_address(0);
    UBYTE *raw=(UBYTE*)&candidate;
    td_save_slot=TD_NONE;td_save_seq=0;
    /* Validate one slot at a time. Keeping two states plus a legacy buffer on
       the native C stack exhausted its reserve during banked collision reads. */
    for(slot=0;slot<2;slot++){
        ENABLE_RAM_MBC5;SWITCH_RAM_BANK(3,RAM_BANKS_ONLY);
        valid=td_read_slot(slot,&candidate,&seq,&version);
        SWITCH_RAM_BANK(0,RAM_BANKS_ONLY);
        if(valid&&td_prepare_state(&candidate,version)&&
           (td_save_slot==TD_NONE||(BYTE)(seq-td_save_seq)>0)){
            td=candidate;td_save_slot=slot;td_save_seq=seq;
        }
    }
    if(td_save_slot!=TD_NONE)return TRUE;
    /* Version 4 used older contract routes: keep earnings/completions, retire active work. */
    ENABLE_RAM_MBC5;SWITCH_RAM_BANK(3,RAM_BANKS_ONLY);
    valid=ram[0]==0x54&&ram[1]==0xD7&&ram[2]==4;
    if(valid){for(i=0;i<48;i++){raw[i]=ram[4+i];check^=raw[i];}valid=check==ram[3];if(valid)td_migrate_old(&candidate);}
    SWITCH_RAM_BANK(0,RAM_BANKS_ONLY);
    if(valid){candidate.wanted=candidate.wanted_left=0;candidate.job=TD_NONE;candidate.stage=0;candidate.left=0;candidate.health=100;candidate.mode=TD_ROAM;candidate.speed=0;if(td_prepare_state(&candidate,4)){td=candidate;td_save_slot=0;td_save();return TRUE;}}
    return FALSE;
}
