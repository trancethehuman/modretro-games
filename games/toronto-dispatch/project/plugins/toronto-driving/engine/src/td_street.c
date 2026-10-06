#pragma bank 255
/* Curbside knock-over props and transit berths. The tables live in this
 * bank; the scene keeps four nearby props in a small WRAM cache. Props are
 * decoration only: they never change collision, traffic or saved state. */
#include <gbdk/platform.h>
#include <string.h>
#include "td_game.h"
#include "scroll.h"
#define TD_STREET_DATA
#include "td_street.h"

/* Active slots: district-local prop index (255 free) and its cached data.
 * td_prop_sdown marks knocked slots; td_prop_dirty asks the scene to
 * re-present a slot's actor. Neither is saved: a prop that leaves the view
 * is tidied away and stands again next time. */
UBYTE td_prop_slot[TD_PROP_SLOTS];
UWORD td_prop_su[TD_PROP_SLOTS],td_prop_sv[TD_PROP_SLOTS];
UBYTE td_prop_sk[TD_PROP_SLOTS],td_prop_su8[TD_PROP_SLOTS],td_prop_sv8[TD_PROP_SLOTS];
UBYTE td_prop_sdown,td_prop_dirty;
extern UBYTE td_ss_i;
/* Refreshes left during which props may appear in view (scene fade-in). */
static UBYTE td_prop_warm;

void td_street_reset(void) BANKED {
    memset(td_prop_slot,255,sizeof(td_prop_slot));
    td_prop_sdown=0;td_prop_dirty=(1<<TD_PROP_SLOTS)-1;td_ss_i=255;td_prop_warm=12;
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
 * examined per frame. */
void td_street_refresh(UBYTE district,UBYTE pu8,UBYTE pv8) BANKED {
    UBYTE s,n,i,mask,free_slot,start,sl=(UBYTE)(scroll_x>>3),st=(UBYTE)(scroll_y>>3);
    for(s=0,mask=1;s<TD_PROP_SLOTS;s++,mask<<=1){
        if(td_prop_slot[s]==255)continue;
        if((UBYTE)(td_prop_su8[s]-pu8+15)<30&&(UBYTE)(td_prop_sv8[s]-pv8+14)<28)continue;
        td_prop_slot[s]=255;td_prop_sdown&=~mask;td_prop_dirty|=mask;
    }
    td_ss_count=td_prop_count[district];
    if(!td_ss_count)return;
    start=td_prop_start[district];
    /* A new district (or first call) restarts the sweep at its own table. */
    if(td_ss_base!=td_prop_uv8+(UWORD)start*2||td_ss_i>=td_ss_count){
        td_ss_base=td_prop_uv8+(UWORD)start*2;td_ss_i=0;td_ss_ptr=td_ss_base;
    }
    td_ss_pu8=pu8;td_ss_pv8=pv8;
    td_street_scan();
    if(td_prop_warm)td_prop_warm--;
    for(n=0;n<td_ss_found;n++){
        i=td_ss_hits[n];free_slot=255;
        /* A prop already in view never pops in: it waits until it is outside
         * the 20x18-tile screen plus one tile, except during scene fade-in. */
        if(!td_prop_warm&&(UBYTE)(td_prop_uv8[(UWORD)(start+i)*2]-sl+1)<22&&
           (UBYTE)(td_prop_uv8[(UWORD)(start+i)*2+1]-st+1)<20)continue;
        for(s=0;s<TD_PROP_SLOTS;s++){
            if(td_prop_slot[s]==i){free_slot=254;break;}
            if(td_prop_slot[s]==255&&free_slot==255)free_slot=s;
        }
        if(free_slot>=TD_PROP_SLOTS)continue;
        td_prop_slot[free_slot]=i;
        td_prop_su8[free_slot]=td_prop_uv8[(UWORD)(start+i)*2];td_prop_sv8[free_slot]=td_prop_uv8[(UWORD)(start+i)*2+1];
        td_prop_su[free_slot]=td_prop_u[start+i];td_prop_sv[free_slot]=td_prop_v[start+i];
        td_prop_sk[free_slot]=td_prop_kind[start+i];
        td_prop_dirty|=1<<free_slot;
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
