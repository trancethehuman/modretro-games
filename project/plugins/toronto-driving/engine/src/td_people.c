#pragma bank 255
/* This module reads the generated people tables (td_people_data.h). */
#define TD_PEOPLE_DATA
#define TD_PEOPLE_MIX_DATA
/* Streamed people (td_people.h). The walker slots' tiles are found in the
 * compiled frames like the special-vehicle block (td_special.c); a look's
 * pose is copied from its ROM bank and written over them. */
#include <string.h>
#include "td_game.h"
#include "td_people.h"
#include "td_daynight.h"
#include "td_people_mix.h"
#include "actor.h"
#ifdef __SDCC
#include "gbs_types.h"
#include "bankdata.h"
#endif

UBYTE td_slot_look[TD_WALKER_SLOTS],td_slot_pose[TD_WALKER_SLOTS];
/* Each slot's A and B tile (8x8 index from the sheet base) and OAM
 * properties (bank bit 3, flips). */
static UBYTE pp_tile[TD_WALKER_SLOTS][2],pp_prop[TD_WALKER_SLOTS][2],pp_found;

#ifdef __SDCC
BANKREF_EXTERN(td_look_tiles)
extern const UBYTE td_look_tiles[];
static UBYTE pp_item(UBYTE frame,UBYTE *tile,UBYTE *prop){
    spritesheet_t sheet;const metasprite_t *ms;metasprite_t item[2];UBYTE bank=PLAYER.sprite.bank;
    MemcpyBanked(&sheet,PLAYER.sprite.ptr,sizeof(sheet),bank);
    MemcpyBanked(&ms,sheet.metasprites+frame,sizeof(ms),bank);
    MemcpyBanked(item,ms,sizeof(item),bank);
    if(item[0].dy==metasprite_end||item[1].dy!=metasprite_end)return FALSE;
    *tile=item[0].dtile;*prop=item[0].props;return TRUE;
}
static UBYTE pp_reverse(UBYTE b){
    b=(b>>4)|(b<<4);b=((b&0xCC)>>2)|((b&0x33)<<2);return ((b&0xAA)>>1)|((b&0x55)<<1);
}
static void pp_write(UBYTE slot){
    UBYTE k,i,src[64],buf[32],prop;UBYTE look=td_slot_look[slot];
    if(!pp_found||look>=TD_LOOKS)return;
    MemcpyBanked(src,td_look_tiles+((((UWORD)look<<2)+td_slot_pose[slot])<<6),64,BANK(td_look_tiles));
    for(k=0;k<2;k++){
        prop=pp_prop[slot][k];
        /* A tile the compiler stored flipped shows the look the right way round. */
        if(prop&0x40)for(i=0;i<16;i++){buf[i*2]=src[k*32+30-i*2];buf[i*2+1]=src[k*32+31-i*2];}
        else memcpy(buf,src+k*32,32);
        if(prop&0x20)for(i=0;i<32;i++)buf[i]=pp_reverse(buf[i]);
        VBK_REG=(prop&0x08)?1:0;
        set_sprite_data((UBYTE)(PLAYER.base_tile+pp_tile[slot][k]),2,buf);
    }
    VBK_REG=0;
}
#else
UBYTE td_test_people_writes;
static UBYTE pp_item(UBYTE frame,UBYTE *tile,UBYTE *prop){*tile=frame;*prop=0;return TRUE;}
static void pp_write(UBYTE slot){if(pp_found&&td_slot_look[slot]<TD_LOOKS)td_test_people_writes++;}
#endif

/* Every scene shares the one sheet, so its slot tiles are found once. */
static UBYTE pp_searched;
void td_people_init(void) BANKED {
    UBYTE i;
    if(!pp_searched){
        pp_searched=TRUE;pp_found=TRUE;
        for(i=0;i<TD_WALKER_SLOTS;i++)
            if(!pp_item(TD_PEOPLE_FRAME(i),&pp_tile[i][0],&pp_prop[i][0])||!pp_item(TD_PEOPLE_FRAME(i)+1,&pp_tile[i][1],&pp_prop[i][1]))pp_found=FALSE;
    }
    for(i=0;i<TD_WALKER_SLOTS;i++){td_slot_look[i]=TD_NONE;td_slot_pose[i]=0;}
}

void td_people_restore(void) BANKED {
    UBYTE i;
    for(i=0;i<TD_WALKER_SLOTS;i++)pp_write(i);
}

void td_people_show(UBYTE slot,UBYTE look,UBYTE pose) BANKED {
    if(slot>=TD_WALKER_SLOTS||look>=TD_LOOKS)return;
    TD_PALETTE(&actors[TD_ACTOR_PEDS+slot])=td_look_pal[look];
    if(td_slot_look[slot]==look&&td_slot_pose[slot]==pose)return;
    td_slot_look[slot]=look;td_slot_pose[slot]=pose;pp_write(slot);
}

void td_people_pose(UBYTE slot,UBYTE pose) BANKED {
    if(slot>=TD_WALKER_SLOTS||td_slot_pose[slot]==pose||td_slot_look[slot]>=TD_LOOKS)return;
    td_slot_pose[slot]=pose;pp_write(slot);
}

UBYTE td_people_pick(UBYTE district,UBYTE route,UWORD u,UWORD v) BANKED {
    UBYTE mix,h;
    /* One route in eight is an officer on foot (td_life.c relies on it). */
    if((route&7)==5)return TD_LOOK_OFFICER;
    if(district>=16||u>=1024||v>=1024)return TD_LOOK_OFFICER+1;
    mix=td_mix_grid[district][((UBYTE)(v>>7)<<3)|(UBYTE)(u>>7)];
    /* The route identity picks within the mix, so a route keeps its person. */
    h=(UBYTE)(route*37+(route>>3)+district*11);
    if(td_daynight_lights&&!(h&3))return td_mix_night[(h>>2)&31];
    return td_mix_looks[mix][(h>>2)&31];
}
