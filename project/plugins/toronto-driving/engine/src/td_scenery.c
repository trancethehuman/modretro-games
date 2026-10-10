#pragma bank 255
/* Animated scenery: background tiles that change as the city runs. Open
 * water in every scene shares one 32 x 32 px ripple texture (sixteen tiles
 * on a fixed grid); the roof at Yonge and Dundas carries a video screen
 * (eight tiles). When a scene loads, td_scenery_find() looks those tiles up
 * in the scene's tilesets, which tells where GB Studio's loader put them in
 * VRAM; while the city runs, td_scenery_tick() redraws them with the next of
 * four frames (scripts/city_kit.py, create_scenery.py). Shore tiles are calm
 * shallows and never change.
 *
 * The tiles are written by a vertical-blank interrupt handler, while VRAM is
 * free: GBDK's ordinary copy waits for the LCD on every byte, which cost
 * whole frames while driving. The update arms a chunk of eight tiles; the
 * handler copies it at the next vertical blank (three for the 24 tiles), in
 * about five of its ten lines, and only while the city shows (menus and the
 * map own those tiles). */
#include <gbdk/platform.h>
#include <string.h>
#include "td_game.h"
#define TD_SCENERY_DATA
#include "td_scenery_data.h"
#include "system.h"
#ifdef __SDCC
#include "gbs_types.h"
#include "data_manager.h"
#include "bankdata.h"
#endif

/* VRAM tile of each scenery tile, the ones this scene has (bit per tile)
 * and those loaded in VRAM bank 1. */
UBYTE td_scenery_slot[TD_SCENERY_TILES];
UBYTE td_scenery_found[(TD_SCENERY_TILES+7)/8],td_scenery_bank1[(TD_SCENERY_TILES+7)/8];
static UBYTE td_scenery_frame,td_scenery_any,td_scenery_next,td_scenery_clock;
static volatile UBYTE td_scenery_armed;
static UBYTE td_scenery_isr;
#define TD_SCENERY_CHUNK 8
#ifdef __SDCC
BANKREF(td_scenery_frames)
BANKREF_EXTERN(td_scenery_frames)
#endif
typedef char td_scenery_frames_power_of_two[(!(TD_SCENERY_FRAMES&(TD_SCENERY_FRAMES-1)))?1:-1];
#define TD_BIT(set,k) ((set)[(k)>>3]&(1<<((k)&7)))

/* VRAM tile of entry k of an n-tile tileset, as GB Studio's
 * load_bkg_tileset places it (ALLOC_BKG_TILES_TOWARDS_SPR): the first 128
 * from tile 0, a remainder of up to 64 ending at tile 191, a larger one from
 * tile 128. */
UBYTE td_scenery_vram_tile(UWORD k,UWORD n) BANKED {
    if(k<128)return (UBYTE)k;
    n-=128;
    return (UBYTE)(n<65?192-n+(k-128):k);
}
/* Records tileset entry k (16 bytes) if it is one of the scenery tiles. */
void td_scenery_match(const UBYTE *tile,UWORD k,UWORD n,UBYTE bank1) BANKED {
    UBYTE w;
    for(w=0;w<TD_SCENERY_TILES;w++){
        if(TD_BIT(td_scenery_found,w)||tile[0]!=td_scenery_frames[0][w][0]||memcmp(tile,td_scenery_frames[0][w],16))continue;
        td_scenery_slot[w]=td_scenery_vram_tile(k,n);td_scenery_found[w>>3]|=1<<(w&7);td_scenery_any=1;
        if(bank1)td_scenery_bank1[w>>3]|=1<<(w&7);
        return;
    }
}
static void td_scenery_clear(void){
    memset(td_scenery_found,0,sizeof(td_scenery_found));memset(td_scenery_bank1,0,sizeof(td_scenery_bank1));
    td_scenery_armed=0;td_scenery_frame=0;td_scenery_any=0;td_scenery_next=TD_SCENERY_TILES;
}
#ifdef __SDCC
static void td_scenery_scan(const tileset_t *set,UBYTE bank,UBYTE bank1){
    UWORD k,n;UBYTE tile[16];
    if(!set)return;
    n=ReadBankedUWORD(&set->n_tiles,bank);
    for(k=0;k<n;k++){MemcpyBanked(tile,set->tiles+(k<<4),16,bank);td_scenery_match(tile,k,n,bank1);}
}
void td_scenery_vbl(void) NONBANKED;
void td_scenery_find(void) BANKED {
    scene_t scene;background_t bkg;
    td_scenery_clear();
    if(!td_scenery_isr){td_scenery_isr=1;CRITICAL{add_VBL(td_scenery_vbl);}}
    MemcpyBanked(&scene,current_scene.ptr,sizeof(scene),current_scene.bank);
    MemcpyBanked(&bkg,scene.background.ptr,sizeof(bkg),scene.background.bank);
    td_scenery_scan(bkg.tileset.ptr,bkg.tileset.bank,0);
    td_scenery_scan(bkg.cgb_tileset.ptr,bkg.cgb_tileset.bank,1);
}
#else
/* Host builds have no ROM tilesets; tests feed td_scenery_match directly. */
void td_scenery_find(void) BANKED {td_scenery_clear();}
#endif
/* Called at the start of every update while the city shows: about four
 * times a second a new frame begins; each update arms the next chunk for
 * the vertical-blank handler. */
void td_scenery_tick(void) BANKED {
    if(!td_scenery_any)return;
    if((UBYTE)(sys_time>>4)!=td_scenery_clock){
        td_scenery_clock=(UBYTE)(sys_time>>4);
        td_scenery_frame=(td_scenery_frame+1)&(TD_SCENERY_FRAMES-1);td_scenery_next=0;
    }
    if(td_scenery_next<TD_SCENERY_TILES)td_scenery_armed=1;
}
#ifdef __SDCC
/* Copies one chunk (eight tiles from td_scenery_next) into VRAM: e = the
 * chunk's found bits, d = its VRAM-bank-1 bits, bc = the chunk's first tile
 * in the current frame (sdcccall(1)). A tile's VRAM address is 0x8000 +
 * 16t, or 0x9000 + 16t for t below 128 in signed addressing (LCDC bit 4
 * clear, as the city runs). About 140 machine cycles a tile, so a chunk
 * takes five of the ten lines of vertical blank. */
static void td_scenery_chunk(UWORD bits,const UBYTE *src) NONBANKED __naked {
    bits;src;
    __asm
    ld h,b
    ld l,c
    ld b,d
    ld c,e
    ld a,(_td_scenery_next)
    add a,#<(_td_scenery_slot)
    ld e,a
    ld a,#0
    adc a,#>(_td_scenery_slot)
    ld d,a
00001$:
    srl c
    jr nc,00003$
    ld a,b
    and a,#1
    ldh (_VBK_REG+0),a
    push de
    ld a,(de)
    swap a
    ld e,a
    and a,#0x0F
    or a,#0x80
    ld d,a
    bit 3,a
    jr nz,00002$
    ldh a,(_LCDC_REG+0)
    bit 4,a
    jr nz,00002$
    ld a,d
    add a,#0x10
    ld d,a
00002$:
    ld a,e
    and a,#0xF0
    ld e,a
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    inc e
    ld a,(hl+)
    ld (de),a
    pop de
    jr 00004$
00003$:
    ld a,l
    add a,#16
    ld l,a
    jr nc,00004$
    inc h
00004$:
    inc de
    srl b
    ld a,c
    or a,a
    jr nz,00001$
    ret
    __endasm;
}
#define TD_SCENERY_IN_VBLANK() (LY_REG>=144&&LY_REG<148)
#else
#define TD_SCENERY_IN_VBLANK() 1
#endif
typedef char td_scenery_chunk_is_a_byte[(TD_SCENERY_CHUNK==8&&!(TD_SCENERY_TILES%8))?1:-1];
/* Vertical-blank handler: copies an armed chunk straight into VRAM. A chunk
 * is one byte of the found and bank sets. Keeps the caller's ROM and VRAM
 * banks; if it starts late in vertical blank (interrupts held off), the
 * chunk waits for the next one. */
void td_scenery_vbl(void) NONBANKED {
    UBYTE w,f,b1,vbk;
#ifdef __SDCC
    UBYTE rom;
#else
    UBYTE k;const UBYTE *src;
#endif
    if(!td_scenery_armed||!TD_SCENERY_IN_VBLANK())return;
    td_scenery_armed=0;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE)return;
    w=td_scenery_next;
    f=td_scenery_found[w>>3];b1=td_scenery_bank1[w>>3];
    vbk=VBK_REG&1;
#ifdef __SDCC
    rom=_current_bank;SWITCH_ROM(BANK(td_scenery_frames));
    td_scenery_chunk(((UWORD)b1<<8)|f,td_scenery_frames[td_scenery_frame][w]);
    SWITCH_ROM(rom);
#else
    for(k=w,src=td_scenery_frames[td_scenery_frame][w];f;f>>=1,b1>>=1,k++,src+=16){
        if(!(f&1))continue;
        VBK_REG=b1&1;set_bkg_data(td_scenery_slot[k],1,src);
    }
#endif
    VBK_REG=vbk;td_scenery_next=w+TD_SCENERY_CHUNK;
}
