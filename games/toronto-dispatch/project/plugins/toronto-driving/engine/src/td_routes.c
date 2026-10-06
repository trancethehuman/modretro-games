#pragma bank 255
#include <string.h>
#include "td_game.h"
#define TD_WORLD_ROUTE_DATA
#include "td_world_routes.h"

static UWORD td_route_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
/* Only seven phase bits matter: eight-bit wraparound is exact and keeps
   runtime multiply/divide helpers out of the per-route scan. */
static UBYTE td_route_base(void){
    UBYTE s=(UBYTE)td.seconds;
    return (UBYTE)((UBYTE)(s<<3)+(UBYTE)(s<<2)+td.subsecond/5);
}
static UWORD td_route_position(UBYTE base,UBYTE identity,UWORD start){
    UBYTE phase=(UBYTE)(base+(UBYTE)(identity<<5)+(UBYTE)(identity<<2)+identity)&127;
    return start+(phase<64?phase:127-phase);
}

#define TD_ROUTE_CANDIDATES 48
/* A slot keeps its route while the walker is this close (whole pixels);
   walkers show within 112 x 96 of the courier (TORONTO.c). */
#define TD_ROUTE_KEEP_U 136
#define TD_ROUTE_KEEP_V 104
/* Identities per district stay below TD_ROUTE_IDS (td_world_routes.h);
   held/window sets keep one bit each. */
#define TD_ROUTE_BYTES (TD_ROUTE_IDS/8)
typedef char td_route_ids_fit_sets[(TD_ROUTE_IDS%8==0&&TD_ROUTE_IDS<=248)?1:-1];
/* The window scan assembly uses literal 20 set bytes and 48 candidates. */
typedef char td_route_scan_literals_match[(TD_ROUTE_BYTES==20&&TD_ROUTE_CANDIDATES==48)?1:-1];
/* Refresh scratch lives in WRAM: SDCC indexes static arrays far more cheaply
   than stack frames, and the refresh runs from the main loop only.
   td_route_free[c] is the score of candidate c, or 65535 while a slot holds
   it; real scores never exceed 400, so the first strict minimum below 65535
   is exactly the first available best route. Globals (not static) expose
   the scratch to native emulator verification of the assembly pick. */
UBYTE td_route_found,td_route_pick_index,td_route_candidates[TD_ROUTE_CANDIDATES],td_route_held[TD_ROUTE_BYTES],td_route_window_set[TD_ROUTE_BYTES];
UWORD td_route_free[TD_ROUTE_CANDIDATES];
static const UBYTE td_route_bit[8]={1,2,4,8,16,32,64,128};
#ifdef __SDCC
/* Index of the first strict minimum of td_route_free[0..found), or 255. */
static UBYTE td_route_pick(void) NAKED {
    __asm
        ld a, #0xff
        ld (_td_route_pick_index), a
        ld a, (_td_route_found)
        or a, a
        jr z, 3$
        ld b, a
        ld c, #0
        ld de, #0xffff
        ld hl, #_td_route_free
    1$:
        ld a, (hl+)
        sub a, e
        ld a, (hl+)
        sbc a, d
        jr nc, 2$
        dec hl
        ld d, (hl)
        dec hl
        ld e, (hl)
        inc hl
        inc hl
        ld a, c
        ld (_td_route_pick_index), a
    2$:
        inc c
        dec b
        jr nz, 1$
    3$:
        ld a, (_td_route_pick_index)
        ret
    __endasm;
}
#else
static UBYTE td_route_pick(void){
    UBYTE c;UWORD best=65535;td_route_pick_index=TD_NONE;
    for(c=0;c<td_route_found;c++)if(td_route_free[c]<best){best=td_route_free[c];td_route_pick_index=c;}
    return td_route_pick_index;
}
#endif
/* Collect identities whose route lies in the refresh window, in ascending
   identity order: |pu-(x+32)|<=144 and |pv-y|<=96. The generated y-band
   index limits the scan to nearby rows; a 160-bit set restores identity
   order. Scores and availability come from the same pass. Returns FALSE if
   more than 32 routes match. */
#ifdef __SDCC
/* Window scan inputs prepared by td_route_window(); td_rw_* are WRAM scratch. */
UBYTE td_rw_start,td_rw_count,td_rw_base,td_rw_j,td_rw_found,td_rw_bits,td_rw_byte;
UWORD td_rw_left,td_rw_top,td_rw_pu,td_rw_pv,td_rw_y;
const UBYTE *td_rw_routes;const UBYTE *td_rw_order;
/* Same window set, candidates and td_route_free scores as the C reference;
 * returns the candidate count, or 255 when more than 32 routes match. */
UBYTE td_route_window_scan(void) NAKED {
    __asm
        ld hl, #_td_route_window_set
        ld b, #20
        xor a, a
    1$:
        ld (hl+), a
        dec b
        jr nz, 1$
        ld a, (_td_rw_count)
        or a, a
        jp z, 20$
        ld c, a
        ld a, (_td_rw_start)
        ld e, a
        ld d, #0
        ld hl, #_td_rw_order
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        add hl, de
        ld e, l
        ld d, h
    2$:
        ld a, (de)
        inc de
        push de
        push bc
        ld (_td_rw_j), a
        ld l, a
        ld h, #0
        add hl, hl
        add hl, hl
        ld a, (_td_rw_routes)
        add a, l
        ld l, a
        ld a, (_td_rw_routes+1)
        adc a, h
        ld h, a
        ld a, (_td_rw_left)
        ld c, a
        ld a, (hl+)
        sub a, c
        ld e, a
        ld a, (_td_rw_left+1)
        ld c, a
        ld a, (hl+)
        sbc a, c
        ld d, a
        or a, a
        jr z, 3$
        dec a
        jr nz, 9$
        ld a, e
        cp a, #0x21
        jr nc, 9$
    3$:
        ld a, (_td_rw_top)
        ld c, a
        ld a, (hl+)
        sub a, c
        ld e, a
        ld a, (_td_rw_top+1)
        ld c, a
        ld a, (hl)
        sbc a, c
        or a, a
        jr nz, 9$
        ld a, e
        cp a, #0xc1
        jr nc, 9$
        ld a, (_td_rw_j)
        ld c, a
        srl a
        srl a
        srl a
        ld hl, #_td_route_window_set
        add a, l
        ld l, a
        adc a, h
        sub a, l
        ld h, a
        ld a, c
        and a, #7
        ld c, a
        ld a, #1
        inc c
    4$:
        dec c
        jr z, 5$
        add a, a
        jr 4$
    5$:
        or a, (hl)
        ld (hl), a
    9$:
        pop bc
        pop de
        dec c
        jr nz, 2$
    20$:
        xor a, a
        ld (_td_rw_found), a
        ld (_td_rw_byte), a
    21$:
        ld a, (_td_rw_byte)
        ld e, a
        ld d, #0
        ld hl, #_td_route_window_set
        add hl, de
        ld a, (hl)
        ld (_td_rw_bits), a
        ld a, e
        add a, a
        add a, a
        add a, a
        ld (_td_rw_j), a
    22$:
        ld a, (_td_rw_bits)
        or a, a
        jp z, 29$
        srl a
        ld (_td_rw_bits), a
        jp nc, 28$
        ld a, (_td_rw_found)
        cp a, #48
        jr c, 23$
        ld a, #255
        ret
    23$:
        ld c, a
        ld b, #0
        ld hl, #_td_route_candidates
        add hl, bc
        ld a, (_td_rw_j)
        ld (hl), a
        ld l, a
        ld h, #0
        add hl, hl
        add hl, hl
        ld a, (_td_rw_routes)
        add a, l
        ld l, a
        ld a, (_td_rw_routes+1)
        adc a, h
        ld h, a
        ld a, (hl+)
        ld e, a
        ld a, (hl+)
        ld d, a
        ld a, (hl+)
        ld (_td_rw_y), a
        ld a, (hl)
        ld (_td_rw_y+1), a
        ld a, (_td_rw_j)
        ld c, a
        add a, a
        add a, a
        ld b, a
        add a, a
        add a, a
        add a, a
        add a, b
        add a, c
        ld b, a
        ld a, (_td_rw_base)
        add a, b
        and a, #0x7f
        cp a, #64
        jr c, 24$
        cpl
        sub a, #128
    24$:
        add a, e
        ld e, a
        ld a, d
        adc a, #0
        ld d, a
        ld a, (_td_rw_pu)
        sub a, e
        ld e, a
        ld a, (_td_rw_pu+1)
        sbc a, d
        ld d, a
        jr nc, 25$
        xor a, a
        sub a, e
        ld e, a
        ld a, #0
        sbc a, d
        ld d, a
    25$:
        push de
        ld hl, #_td_rw_y
        ld a, (_td_rw_pv)
        sub a, (hl)
        ld e, a
        inc hl
        ld a, (_td_rw_pv+1)
        sbc a, (hl)
        ld d, a
        jr nc, 26$
        xor a, a
        sub a, e
        ld e, a
        ld a, #0
        sbc a, d
        ld d, a
    26$:
        pop hl
        add hl, de
        ld e, l
        ld d, h
        ld a, (_td_rw_byte)
        ld c, a
        ld b, #0
        ld hl, #_td_route_held
        add hl, bc
        ld b, (hl)
        ld a, (_td_rw_j)
        and a, #7
        inc a
    27$:
        dec a
        jr z, 127$
        srl b
        jr 27$
    127$:
        bit 0, b
        jr z, 128$
        ld de, #0xffff
    128$:
        ld a, (_td_rw_found)
        ld c, a
        ld b, #0
        ld hl, #_td_route_free
        add hl, bc
        add hl, bc
        ld a, e
        ld (hl+), a
        ld (hl), d
        ld hl, #_td_rw_found
        inc (hl)
    28$:
        ld hl, #_td_rw_j
        inc (hl)
        jp 22$
    29$:
        ld hl, #_td_rw_byte
        inc (hl)
        ld a, (hl)
        cp a, #20
        jp c, 21$
        ld a, (_td_rw_found)
        ret
    __endasm;
}
static UBYTE td_route_window(UBYTE district,UBYTE base,UWORD pu,UWORD pv){
    UBYTE start,end,found,first=0,last=(UBYTE)((pv+96)>>5);
    if(pv>=96)first=(UBYTE)((pv-96)>>5);
    if(last>31)last=31;
    start=td_route_band[district][first];end=td_route_band[district][last+1];
    td_rw_start=start;td_rw_count=end>start?end-start:0;
    td_rw_routes=(const UBYTE *)td_district_routes[district];td_rw_order=td_route_order[district];
    td_rw_left=pu-176;td_rw_top=pv-96;td_rw_base=base;td_rw_pu=pu;td_rw_pv=pv;
    found=td_route_window_scan();
    if(found==255)return FALSE;
    td_route_found=found;return TRUE;
}
#else
static UBYTE td_route_window(UBYTE district,UBYTE base,UWORD pu,UWORD pv){
    const UWORD (*routes)[2]=td_district_routes[district];const UBYTE *order=td_route_order[district];
    UWORD left=pu-176,top=pv-96,u;UBYTE k,end,j,bits,held,first=0,last=(UBYTE)((pv+96)>>5),found=0;
    if(pv>=96)first=(UBYTE)((pv-96)>>5);
    if(last>31)last=31;
    memset(td_route_window_set,0,sizeof(td_route_window_set));
    end=td_route_band[district][last+1];
    for(k=td_route_band[district][first];k<end;k++){
        j=order[k];
        if((UWORD)(routes[j][0]-left)>288||(UWORD)(routes[j][1]-top)>192)continue;
        td_route_window_set[j>>3]|=td_route_bit[j&7];
    }
    for(k=0;k<TD_ROUTE_BYTES;k++){
        bits=td_route_window_set[k];if(!bits)continue;held=td_route_held[k];
        for(j=k<<3;bits;j++,bits>>=1,held>>=1)if(bits&1){
            if(found==TD_ROUTE_CANDIDATES)return FALSE;
            td_route_candidates[found]=j;u=td_route_position(base,j,routes[j][0]);
            td_route_free[found]=held&1?65535:td_route_distance(pu,u)+td_route_distance(pv,routes[j][1]);found++;
        }
    }
    td_route_found=found;return TRUE;
}
#endif

/* Scan ROM only when the viewport changes. Keep TD_PEDS identities and their
   coordinates in WRAM, rather than copying the whole route table.
   Window candidates and scores do not depend on the slot, so they are
   collected once; each slot then picks the same first minimum (ascending
   identity, strict <) among routes no slot holds, exactly as a full per-slot
   scan does. Authored districts have at most 29 window candidates; a denser
   window falls back to the full scan. */
UBYTE td_refresh_routes(UBYTE *identities,UWORD (*nearby)[2],UBYTE retry_empty) BANKED {
    UBYTE i,j,k,c,route,phase,count=td_route_counts[td.district],base=td_route_base(),scanned=0,picks=0;
    UWORD score,best,u,pu=td.u>>4,pv=td.v>>4;
    const UWORD (*routes)[2]=td_district_routes[td.district];
    for(i=0;i<TD_PEDS;i++){
        route=identities[i];
        if(route<count){
            /* Inline td_route_position/td_route_distance: this check runs
               for every slot on every refresh. */
            phase=(UBYTE)(base+(UBYTE)(route<<5)+(UBYTE)(route<<2)+route)&127;
            u=nearby[i][0]+(phase<64?phase:127-phase);
            if((u>pu?u-pu:pu-u)<TD_ROUTE_KEEP_U){u=nearby[i][1];if((u>pv?u-pv:pv-u)<TD_ROUTE_KEEP_V)continue;}
        }else if(route==TD_NONE&&!retry_empty)continue;   /* nothing new nearby */
        /* Bound one call's work: the remaining slots wait for the next call. */
        if(picks==TD_ROUTE_PICKS)return TRUE;
        picks++;
        if(!scanned){
            /* Bit j: some slot holds identity j (identities stay below TD_ROUTE_IDS). */
            memset(td_route_held,0,sizeof(td_route_held));
            for(k=0;k<TD_PEDS;k++){j=identities[k];if(j<TD_ROUTE_IDS)td_route_held[j>>3]|=td_route_bit[j&7];}
            scanned=td_route_window(td.district,base,pu,pv)?1:2;
        }
        if(scanned==1){
            identities[i]=TD_NONE;
            if(route<TD_ROUTE_IDS){
                for(k=0;k<TD_PEDS;k++)if(identities[k]==route)break;
                if(k==TD_PEDS){
                    td_route_held[route>>3]&=~td_route_bit[route&7];
                    if(td_route_window_set[route>>3]&td_route_bit[route&7])
                        for(c=0;c<td_route_found;c++)if(td_route_candidates[c]==route){
                            u=td_route_position(base,route,routes[route][0]);
                            td_route_free[c]=td_route_distance(pu,u)+td_route_distance(pv,routes[route][1]);break;
                        }
                }
            }
            c=td_route_pick();
            if(c!=TD_NONE){
                j=td_route_candidates[c];td_route_held[j>>3]|=td_route_bit[j&7];td_route_free[c]=65535;
                identities[i]=j;nearby[i][0]=routes[j][0];nearby[i][1]=routes[j][1];
            }
            continue;
        }
        identities[i]=TD_NONE;best=65535;
        for(j=0;j<count;j++){
            if(td_route_distance(pu,routes[j][0]+32)>144||td_route_distance(pv,routes[j][1])>96)continue;
            for(k=0;k<TD_PEDS;k++)if(identities[k]==j)break;
            if(k<TD_PEDS)continue;
            u=td_route_position(base,j,routes[j][0]);
            score=td_route_distance(pu,u)+td_route_distance(pv,routes[j][1]);
            if(score<best){best=score;identities[i]=j;nearby[i][0]=routes[j][0];nearby[i][1]=routes[j][1];}
        }
    }
    return FALSE;
}
