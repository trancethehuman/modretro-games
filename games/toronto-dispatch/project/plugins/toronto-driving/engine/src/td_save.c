#pragma bank 255
#include <string.h>
#include "td_game.h"
#include "td_district.h"
#include "td_transit.h"
#include "compat.h"
#include "system.h"
static UBYTE td_save_slot=TD_NONE,td_save_seq;
extern UBYTE td_resume_mode;

/* CRC-16/CCITT (poly 0x1021, MSB first): byte table replaces the bit loop. */
static const UWORD td_crc_table[256]={
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50A5,0x60C6,0x70E7,
    0x8108,0x9129,0xA14A,0xB16B,0xC18C,0xD1AD,0xE1CE,0xF1EF,
    0x1231,0x0210,0x3273,0x2252,0x52B5,0x4294,0x72F7,0x62D6,
    0x9339,0x8318,0xB37B,0xA35A,0xD3BD,0xC39C,0xF3FF,0xE3DE,
    0x2462,0x3443,0x0420,0x1401,0x64E6,0x74C7,0x44A4,0x5485,
    0xA56A,0xB54B,0x8528,0x9509,0xE5EE,0xF5CF,0xC5AC,0xD58D,
    0x3653,0x2672,0x1611,0x0630,0x76D7,0x66F6,0x5695,0x46B4,
    0xB75B,0xA77A,0x9719,0x8738,0xF7DF,0xE7FE,0xD79D,0xC7BC,
    0x48C4,0x58E5,0x6886,0x78A7,0x0840,0x1861,0x2802,0x3823,
    0xC9CC,0xD9ED,0xE98E,0xF9AF,0x8948,0x9969,0xA90A,0xB92B,
    0x5AF5,0x4AD4,0x7AB7,0x6A96,0x1A71,0x0A50,0x3A33,0x2A12,
    0xDBFD,0xCBDC,0xFBBF,0xEB9E,0x9B79,0x8B58,0xBB3B,0xAB1A,
    0x6CA6,0x7C87,0x4CE4,0x5CC5,0x2C22,0x3C03,0x0C60,0x1C41,
    0xEDAE,0xFD8F,0xCDEC,0xDDCD,0xAD2A,0xBD0B,0x8D68,0x9D49,
    0x7E97,0x6EB6,0x5ED5,0x4EF4,0x3E13,0x2E32,0x1E51,0x0E70,
    0xFF9F,0xEFBE,0xDFDD,0xCFFC,0xBF1B,0xAF3A,0x9F59,0x8F78,
    0x9188,0x81A9,0xB1CA,0xA1EB,0xD10C,0xC12D,0xF14E,0xE16F,
    0x1080,0x00A1,0x30C2,0x20E3,0x5004,0x4025,0x7046,0x6067,
    0x83B9,0x9398,0xA3FB,0xB3DA,0xC33D,0xD31C,0xE37F,0xF35E,
    0x02B1,0x1290,0x22F3,0x32D2,0x4235,0x5214,0x6277,0x7256,
    0xB5EA,0xA5CB,0x95A8,0x8589,0xF56E,0xE54F,0xD52C,0xC50D,
    0x34E2,0x24C3,0x14A0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xA7DB,0xB7FA,0x8799,0x97B8,0xE75F,0xF77E,0xC71D,0xD73C,
    0x26D3,0x36F2,0x0691,0x16B0,0x6657,0x7676,0x4615,0x5634,
    0xD94C,0xC96D,0xF90E,0xE92F,0x99C8,0x89E9,0xB98A,0xA9AB,
    0x5844,0x4865,0x7806,0x6827,0x18C0,0x08E1,0x3882,0x28A3,
    0xCB7D,0xDB5C,0xEB3F,0xFB1E,0x8BF9,0x9BD8,0xABBB,0xBB9A,
    0x4A75,0x5A54,0x6A37,0x7A16,0x0AF1,0x1AD0,0x2AB3,0x3A92,
    0xFD2E,0xED0F,0xDD6C,0xCD4D,0xBDAA,0xAD8B,0x9DE8,0x8DC9,
    0x7C26,0x6C07,0x5C64,0x4C45,0x3CA2,0x2C83,0x1CE0,0x0CC1,
    0xEF1F,0xFF3E,0xCF5D,0xDF7C,0xAF9B,0xBFBA,0x8FD9,0x9FF8,
    0x6E17,0x7E36,0x4E55,0x5E74,0x2E93,0x3EB2,0x0ED1,0x1EF0
};
#define TD_CRC(crc,value) ((crc)=(UWORD)((UWORD)(UBYTE)(crc)<<8)^td_crc_table[(UBYTE)((crc)>>8)^(UBYTE)(value)])
static UWORD td_crc_byte(UWORD crc,UBYTE value){
    return TD_CRC(crc,value);
}
#ifdef __SDCC
/* td_crc_value over td_crc_len (>=1) bytes at td_crc_ptr, as TD_CRC. */
const UBYTE *td_crc_ptr;UBYTE td_crc_len;UWORD td_crc_value;
void td_crc_run(void) NAKED {
    __asm
        ld hl, #_td_crc_ptr
        ld a, (hl+)
        ld d, (hl)
        ld e, a
        ld a, (_td_crc_len)
        ld b, a
        ld a, (_td_crc_value)
        ld c, a
    1$:
        ld a, (de)
        inc de
        ld hl, #(_td_crc_value + 1)
        xor a, (hl)
        ld l, a
        ld h, #0
        add hl, hl
        push de
        ld de, #_td_crc_table
        add hl, de
        pop de
        ld a, (hl+)
        ld (_td_crc_value), a
        ld a, (hl)
        xor a, c
        ld (_td_crc_value + 1), a
        ld a, (_td_crc_value)
        ld c, a
        dec b
        jr nz, 1$
        ret
    __endasm;
}
#endif
static volatile UBYTE *td_save_address(UBYTE slot){return slot?(volatile UBYTE*)0xA180:(volatile UBYTE*)0xA100;}
static UBYTE td_valid_state(td_state_t *s){
    UBYTE i,bits=0,value;td_job_t job;td_stop_t stop;
    if(s->vehicle>3||s->heading>15||s->onfoot>1||s->health>100||s->subsecond>=60||s->mode>TD_HELP)return FALSE;
    if(s->u>=1024*16||s->v>=976*16||s->park_u>=1024*16||s->park_v>=976*16)return FALSE;
    if(s->district>=TD_DISTRICT_COUNT||s->park_district>=TD_DISTRICT_COUNT||s->reserved)return FALSE;
    if(!td_district_drivable(s->park_district,s->park_u>>4,s->park_v>>4)||!(s->onfoot?td_district_walkable(s->district,s->u>>4,s->v>>4):td_district_drivable(s->district,s->u>>4,s->v>>4)))return FALSE;
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
        td_get_stop(s->transit_origin&63,&stop);if(!stop.transit||stop.district!=s->district)return FALSE;
        if(!td_transit_valid(s->transit_origin,s->transit_target))return FALSE;
        if(s->mode==TD_RIDE&&(!s->ride_left||s->ride_left>(td_transit_service(s->transit_origin)==4?28:8)))return FALSE;
    }
    return TRUE;
}
void td_save(void) BANKED {
    UBYTE i,slot=td_save_slot==0?1:0,mode=td.mode;UWORD crc=0xFFFF;
    const UBYTE *src=(const UBYTE*)&td;volatile UBYTE *ram=td_save_address(slot);
    /* Two records in SRAM bank 3 retain the last committed snapshot during power loss. */
    if(mode==TD_PAUSE||mode==TD_MAP||mode==TD_HELP)td.mode=td_resume_mode;
    if(td.mode!=TD_WAIT&&td.mode!=TD_RIDE)td.mode=TD_ROAM;
    ENABLE_RAM_MBC5;SWITCH_RAM_BANK(3,RAM_BANKS_ONLY);
    ram[0]=0;ram[1]=0xD7;ram[2]=6;ram[3]=sizeof(td);ram[4]=td_save_seq+1;
    TD_CRC(crc,ram[2]);TD_CRC(crc,ram[3]);TD_CRC(crc,ram[4]);
#ifdef __SDCC
    /* Same bytes and store order; the CRC over td runs in assembly. */
    for(i=0;i<sizeof(td);i++)ram[8+i]=src[i];
    td_crc_ptr=src;td_crc_len=sizeof(td);td_crc_value=crc;td_crc_run();crc=td_crc_value;
#else
    for(i=0;i<sizeof(td);i++){ram[8+i]=src[i];TD_CRC(crc,src[i]);}
#endif
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
static UBYTE td_read_slot(UBYTE slot,td_state_t *dest,UBYTE *seq){
    UBYTE i,version,length;UWORD crc=0xFFFF;UBYTE *dst=(UBYTE*)dest;volatile UBYTE *ram=td_save_address(slot);
    version=ram[2];length=ram[3];
    if(ram[0]!=0x54||ram[1]!=0xD7||!((version==6&&length==sizeof(td))||(version==5&&length==48)))return FALSE;
    for(i=2;i<=4;i++)crc=td_crc_byte(crc,ram[i]);
    for(i=0;i<length;i++){dst[i]=ram[8+i];crc=td_crc_byte(crc,ram[8+i]);}
    if(crc!=(ram[5]|(UWORD)ram[6]<<8))return FALSE;
    if(version==5)td_migrate_old(dest);
    *seq=ram[4];return TRUE;
}
UBYTE td_restore(void) BANKED {
    td_state_t candidate;UBYTE valid,seq=0,slot,i,check=0;volatile UBYTE *ram=td_save_address(0);
    UBYTE *raw=(UBYTE*)&candidate;
    td_save_slot=TD_NONE;td_save_seq=0;
    /* Validate one slot at a time. Keeping two states plus a legacy buffer on
       the native C stack exhausted its reserve during banked collision reads. */
    for(slot=0;slot<2;slot++){
        ENABLE_RAM_MBC5;SWITCH_RAM_BANK(3,RAM_BANKS_ONLY);
        valid=td_read_slot(slot,&candidate,&seq);
        SWITCH_RAM_BANK(0,RAM_BANKS_ONLY);
        if(valid&&td_valid_state(&candidate)&&
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
    if(valid){candidate.job=TD_NONE;candidate.stage=0;candidate.left=0;candidate.health=100;candidate.mode=TD_ROAM;candidate.speed=0;if(td_valid_state(&candidate)){td=candidate;td_save_slot=0;td_save();return TRUE;}}
    return FALSE;
}
