#pragma bank 255
/* The special vehicles' shared tile block (td_special.h). GB Studio gives
 * the placeholder slices their own tiles; their VRAM places (tile and
 * bank) are read from the compiled metasprites of the east and south
 * frames, so the block follows wherever the sheet is packed. */
#include <string.h>
#include "td_game.h"
#include "td_sprites.h"
/* The designs live in this bank (td_special.h includes the data header,
 * so the data switch comes first). */
#define TD_SPECIAL_DATA
#include "td_special.h"
#include "actor.h"
#ifdef __SDCC
#include "gbs_types.h"
#include "bankdata.h"
#endif

UBYTE td_special_kind,td_player_special;
/* Canonical slice k (east view left to right, then south view top-left,
 * top-right, bottom-left, bottom-right): its 8x8 tile index from the
 * sheet base and its OAM properties (bank bit 3, flips). */
static UBYTE sp_tile[8],sp_prop[8],sp_found;

#ifdef __SDCC
/* Absolute positions, tiles and properties of a compiled frame's items. */
static UBYTE sp_items(UBYTE frame,BYTE *x,BYTE *y,UBYTE *tile,UBYTE *prop){
    spritesheet_t sheet;const metasprite_t *ms;metasprite_t items[5];UBYTE n,bank=PLAYER.sprite.bank;BYTE ax=0,ay=0;
    MemcpyBanked(&sheet,PLAYER.sprite.ptr,sizeof(sheet),bank);
    MemcpyBanked(&ms,sheet.metasprites+frame,sizeof(ms),bank);
    MemcpyBanked(items,ms,sizeof(items),bank);
    for(n=0;n<4&&items[n].dy!=metasprite_end;n++){
        ay+=items[n].dy;ax+=items[n].dx;x[n]=ax;y[n]=ay;tile[n]=items[n].dtile;prop[n]=items[n].props;
    }
    return n;
}
/* Order four items by row, then column, into slices first..first+3. */
static UBYTE sp_sort(UBYTE frame,UBYTE first){
    BYTE x[4],y[4];UBYTE t[4],p[4],used=0,k,i,best;
    if(sp_items(frame,x,y,t,p)!=4)return FALSE;
    for(k=0;k<4;k++){
        best=255;
        for(i=0;i<4;i++){
            if(used&(1<<i))continue;
            if(best==255||y[i]<y[best]||(y[i]==y[best]&&x[i]<x[best]))best=i;
        }
        used|=1<<best;sp_tile[first+k]=t[best];sp_prop[first+k]=p[best];
    }
    return TRUE;
}
static UBYTE sp_reverse(UBYTE b){
    b=(b>>4)|(b<<4);b=((b&0xCC)>>2)|((b&0x33)<<2);return ((b&0xAA)>>1)|((b&0x55)<<1);
}
static void sp_write(void){
    UBYTE k,i,buf[32];const UBYTE *src;
    if(!sp_found||!td_special_kind||td_special_kind>TD_SPECIAL_KINDS)return;
    src=td_special_tiles[td_special_kind-1];
    for(k=0;k<8;k++,src+=32){
        /* A slice the compiler flipped is stored flipped, so it shows the
         * design the right way round. */
        if(sp_prop[k]&0x40)for(i=0;i<16;i++){buf[i*2]=src[30-i*2];buf[i*2+1]=src[31-i*2];}
        else memcpy(buf,src,32);
        if(sp_prop[k]&0x20)for(i=0;i<32;i++)buf[i]=sp_reverse(buf[i]);
        VBK_REG=(sp_prop[k]&0x08)?1:0;
        set_sprite_data((UBYTE)(PLAYER.base_tile+sp_tile[k]),2,buf);
    }
    VBK_REG=0;
}
#else
static UBYTE sp_sort(UBYTE frame,UBYTE first){(void)frame;(void)first;return TRUE;}
UBYTE td_test_special_writes;
static void sp_write(void){if(sp_found&&td_special_kind)td_test_special_writes++;}
#endif

void td_special_init(void) BANKED {
    sp_found=sp_sort(TD_FRAME_SPECIAL,0)&&sp_sort(TD_FRAME_SPECIAL+2,4);
    /* Only the courier's own vehicle keeps its design across a scene change. */
    td_special_kind=td_player_special;
    sp_write();
}

void td_special_restore(void) BANKED {sp_write();}

void td_special_load(UBYTE kind) BANKED {
    if(kind==td_special_kind)return;
    td_special_kind=kind;sp_write();
}
