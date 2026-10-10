#pragma bank 255
/* Sidewalk pickups, lost parcels and transit berths. The tables live in
 * this bank; the scene keeps two nearby pickups in a small WRAM cache.
 * Pickups never change collision or traffic; a short ring of recently
 * collected pickups keeps them from reappearing straight away, and a lost
 * parcel, once found, is saved and never comes back. */
#include <gbdk/platform.h>
#include <string.h>
#include "td_game.h"
#include "scroll.h"
#define TD_STREET_DATA
#include "td_street.h"

/* Active slots: district-local pickup index (255 free) and its cached
 * data; td_pickup_dirty asks the scene to re-present a slot's actor. The
 * taken ring holds global table indices of the last pickups collected. */
UBYTE td_pickup_slot[TD_PICKUP_SLOTS];
UWORD td_pickup_su[TD_PICKUP_SLOTS],td_pickup_sv[TD_PICKUP_SLOTS];
UBYTE td_pickup_sk[TD_PICKUP_SLOTS],td_pickup_su8[TD_PICKUP_SLOTS],td_pickup_sv8[TD_PICKUP_SLOTS];
UBYTE td_pickup_dirty,td_pickup_taken[TD_PICKUP_TAKEN],td_pickup_taken_at;
extern UBYTE td_ss_i;
/* Refreshes left during which pickups may appear in view (scene fade-in). */
static UBYTE td_pickup_warm;
static UBYTE td_pickup_base;
typedef char td_pickup_ring_is_power_of_two[((TD_PICKUP_TAKEN&(TD_PICKUP_TAKEN-1))==0)?1:-1];

void td_street_reset(UBYTE cold) BANKED {
    memset(td_pickup_slot,255,sizeof(td_pickup_slot));
    if(cold){memset(td_pickup_taken,255,sizeof(td_pickup_taken));td_pickup_taken_at=0;}
    td_pickup_dirty=(1<<TD_PICKUP_SLOTS)-1;td_ss_i=255;td_pickup_warm=12;
}

static UBYTE td_pickup_was_taken(UBYTE global){
    UBYTE k;
    for(k=0;k<TD_PICKUP_TAKEN;k++)if(td_pickup_taken[k]==global)return TRUE;
    return FALSE;
}

typedef char td_parcels_match[(TD_PICKUP_PARCELS==TD_PARCELS&&TD_PARCEL_BIT>=TD_QUESTS&&
    TD_PARCEL_BIT+TD_PARCELS<=TD_COMPLETE_BYTES*8)?1:-1];
static UBYTE td_parcel_taken(UBYTE global){
    UBYTE p=td_parcel_of[global];
    return p!=255&&TD_DONE(TD_PARCEL_BIT+p);
}
/* The lost parcel a slot holds (255 for other pickups). */
UBYTE td_street_parcel(UBYTE slot) BANKED {
    if(slot>=TD_PICKUP_SLOTS||td_pickup_slot[slot]==255)return 255;
    return td_parcel_of[td_pickup_base+td_pickup_slot[slot]];
}
UBYTE td_parcels_found(void) BANKED {
    UBYTE i,n=0;
    for(i=0;i<TD_PARCELS;i++)if(TD_DONE(TD_PARCEL_BIT+i))n++;
    return n;
}

void td_street_take(UBYTE slot) BANKED {
    if(slot>=TD_PICKUP_SLOTS||td_pickup_slot[slot]==255)return;
    td_pickup_taken[td_pickup_taken_at]=td_pickup_base+td_pickup_slot[slot];
    td_pickup_taken_at=(td_pickup_taken_at+1)&(TD_PICKUP_TAKEN-1);
    td_pickup_slot[slot]=255;td_pickup_dirty|=1<<slot;
}

/* Range scan over the interleaved coarse table: examines td_ss_budget
 * entries from td_ss_i/td_ss_ptr (wrapping to td_ss_base at td_ss_count)
 * and records up to four in-range local indices in td_ss_hits. Byte
 * arithmetic: (UBYTE)(d+r)<2r tests |d|<r for a wrapped difference. Globals
 * so the emulator checker can compare it against the C reference. */
UBYTE td_ss_i,td_ss_count,td_ss_pu8,td_ss_pv8,td_ss_found,td_ss_hits[4];
const UBYTE *td_ss_ptr,*td_ss_base;
#define TD_SS_BUDGET 16
#ifdef __SDCC
void td_street_scan(void) NAKED {
    __asm
        ld a, (_td_ss_ptr)
        ld l, a
        ld a, (_td_ss_ptr + 1)
        ld h, a
        ld a, (_td_ss_i)
        ld b, a
        ld a, (_td_ss_count)
        ld c, a
        ld a, (_td_ss_pu8)
        ld d, a
        ld e, #16
        xor a, a
        ld (_td_ss_found), a
    1$:
        ld a, (hl+)
        sub a, d
        add a, #13
        cp a, #26
        jr nc, 3$
        push de
        ld a, (_td_ss_pv8)
        ld d, a
        ld a, (hl)
        sub a, d
        pop de
        add a, #12
        cp a, #24
        jr nc, 3$
        ld a, (_td_ss_found)
        cp a, #4
        jr nc, 3$
        push hl
        ld hl, #_td_ss_hits
        add a, l
        ld l, a
        adc a, h
        sub a, l
        ld h, a
        ld (hl), b
        ld hl, #_td_ss_found
        inc (hl)
        pop hl
    3$:
        inc hl
        inc b
        ld a, b
        cp a, c
        jr c, 5$
        ld b, #0
        ld a, (_td_ss_base)
        ld l, a
        ld a, (_td_ss_base + 1)
        ld h, a
    5$:
        dec e
        jr nz, 1$
        ld a, l
        ld (_td_ss_ptr), a
        ld a, h
        ld (_td_ss_ptr + 1), a
        ld a, b
        ld (_td_ss_i), a
        ret
    __endasm;
}
#else
void td_street_scan(void) {
    UBYTE n;
    td_ss_found=0;
    for(n=0;n<TD_SS_BUDGET;n++){
        if((UBYTE)(td_ss_ptr[0]-td_ss_pu8+13)<26&&(UBYTE)(td_ss_ptr[1]-td_ss_pv8+12)<24&&td_ss_found<4)
            td_ss_hits[td_ss_found++]=td_ss_i;
        td_ss_ptr+=2;
        if(++td_ss_i>=td_ss_count){td_ss_i=0;td_ss_ptr=td_ss_base;}
    }
}
#endif

/* A slot is released beyond 120x112 px and filled within 104x96 px of the
 * courier, so it never flickers at the edge; sixteen table entries are
 * examined per sweep. */
static UBYTE td_street_skip;
void td_street_refresh(UBYTE district,UBYTE pu8,UBYTE pv8) BANKED {
    UBYTE s,n,i,mask,free_slot,start,sl,st;
    /* Pickups only change off screen, so the sweep runs every fourth
     * update (every update during the scene fade-in). */
    if(!td_pickup_warm&&(++td_street_skip&3))return;
    sl=(UBYTE)(scroll_x>>3);st=(UBYTE)(scroll_y>>3);
    for(s=0,mask=1;s<TD_PICKUP_SLOTS;s++,mask<<=1){
        if(td_pickup_slot[s]==255)continue;
        if((UBYTE)(td_pickup_su8[s]-pu8+15)<30&&(UBYTE)(td_pickup_sv8[s]-pv8+14)<28)continue;
        td_pickup_slot[s]=255;td_pickup_dirty|=mask;
    }
    td_ss_count=td_pickup_count[district];
    if(!td_ss_count)return;
    start=td_pickup_base=td_pickup_start[district];
    /* A new district (or first call) restarts the sweep at its own table. */
    if(td_ss_base!=td_pickup_uv8+(UWORD)start*2||td_ss_i>=td_ss_count){
        td_ss_base=td_pickup_uv8+(UWORD)start*2;td_ss_i=0;td_ss_ptr=td_ss_base;
    }
    td_ss_pu8=pu8;td_ss_pv8=pv8;
    td_street_scan();
    if(td_pickup_warm)td_pickup_warm--;
    for(n=0;n<td_ss_found;n++){
        i=td_ss_hits[n];free_slot=255;
        /* A pickup already in view never pops in: it waits until it is
         * outside the 20x18-tile screen plus two tiles (no sliver at the
         * edge as the camera moves), except during the scene fade-in.
         * Recently collected pickups stay away. */
        if(!td_pickup_warm&&(UBYTE)(td_pickup_uv8[(UWORD)(start+i)*2]-sl+2)<24&&
           (UBYTE)(td_pickup_uv8[(UWORD)(start+i)*2+1]-st+2)<22)continue;
        for(s=0;s<TD_PICKUP_SLOTS;s++){
            if(td_pickup_slot[s]==i){free_slot=254;break;}
            if(td_pickup_slot[s]==255&&free_slot==255)free_slot=s;
        }
        if(free_slot>=TD_PICKUP_SLOTS||td_pickup_was_taken(start+i)||td_parcel_taken(start+i))continue;
        td_pickup_slot[free_slot]=i;
        td_pickup_su8[free_slot]=td_pickup_uv8[(UWORD)(start+i)*2];td_pickup_sv8[free_slot]=td_pickup_uv8[(UWORD)(start+i)*2+1];
        td_pickup_su[free_slot]=td_pickup_u[start+i];td_pickup_sv[free_slot]=td_pickup_v[start+i];
        td_pickup_sk[free_slot]=td_pickup_kind[start+i];
        td_pickup_dirty|=1<<free_slot;
    }
}

/* Transit vehicle berth for a stop in the given district: 0 none (subway,
 * non-transit, another district), 1 street berth on an east-west road
 * centre, 2 ferry with water south, 3 ferry with water north. */
typedef char td_street_stops_match[(TD_STREET_STOPS==TD_STOPS)?1:-1];
UBYTE td_street_berth(UBYTE stop,UBYTE district,UWORD *u,UWORD *v) BANKED {
    if(stop>=TD_STOPS||!td_berth_u[stop]||td_berth_district[stop]!=district)return 0;
    *u=td_berth_u[stop];*v=td_berth_v[stop];
    return 1+td_berth_side[stop];
}
