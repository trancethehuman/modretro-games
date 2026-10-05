#pragma bank 255
#include "td_combat_render.h"
#include "td_combat.h"
#include "td_game.h"
#include "scroll.h"
#include "ui.h"
#include "compat.h"
#define TD_MUZZLE_TILE 253
/* x255 means no patch. xbit5 stores the selected BKG map page, so a later
 * LCDC map switch cannot restore the old reference into another map. */
static UBYTE td_muzzle_x=255,td_muzzle_y,td_muzzle_tile,td_muzzle_attr;
static UBYTE td_muzzle_attribute(UBYTE original){return (original&0xf7)|8;}
void td_combat_render_reset(void) BANKED {td_muzzle_x=255;}
void td_combat_render_restore(void) BANKED {
#ifdef CGB
    UBYTE vbk,tile,attr;UWORD offset;UBYTE *map;
    if(td_muzzle_x==255)return;
    vbk=VBK_REG&1;offset=((UWORD)td_muzzle_y<<5)|(td_muzzle_x&31);
    map=td_muzzle_x&32?(UBYTE*)0x9c00:(UBYTE*)0x9800;
    VBK_REG=0;tile=get_vram_byte(map+offset);VBK_REG=1;attr=get_vram_byte(map+offset);
    /* Any UI/scroll/other overlay repaint wins. Only our exact scratch
     * tile+attribute pair can be replaced with the captured original. */
    if(tile==TD_MUZZLE_TILE&&attr==td_muzzle_attribute(td_muzzle_attr)){
        set_vram_byte(map+offset,td_muzzle_attr);VBK_REG=0;set_vram_byte(map+offset,td_muzzle_tile);
    }
    td_muzzle_x=255;VBK_REG=vbk;
#endif
}
void td_combat_render(void) BANKED {
#ifdef CGB
    static const UBYTE burst[]={0,0x18,0x3c,0x7e,0x7e,0x3c,0x18,0};
    static const UBYTE glow[]={0,0,0x18,0x3c,0x3c,0x18,0,0};
    UWORD u,v,offset;WORD x,y,screen_x,screen_y;UBYTE direction,vbk,attr,tile,row,pixels[16];UBYTE *map;
    if(td_muzzle_x!=255||!td_combat_flash(&u,&v,&direction)||direction>3)return;
    x=u+(direction==0?12:direction==1?-12:0);
    y=v+(direction==2?12:direction==3?-12:0);
    if(x<0||y<0||x>=1024||y>=976)return;
    screen_x=(x&~7)-draw_scroll_x;screen_y=(y&~7)-draw_scroll_y;
    /* Keep a whole tile visible and outside both current/destination HUD
     * rectangles. Effects cannot punch through dialogue, map or text boxes. */
    if(screen_x<0||screen_y<0||screen_x>152||screen_y>136||
       (screen_x+7>=win_pos_x&&screen_y+7>=win_pos_y)||
       (screen_x+7>=win_dest_pos_x&&screen_y+7>=win_dest_pos_y)||
       (WX_REG&&WY_REG<144&&screen_x+7>=(WORD)WX_REG-7&&screen_y+7>=WY_REG))return;
    map=GetBkgAddr();offset=((UWORD)((y>>3)&31)<<5)|((x>>3)&31);vbk=VBK_REG&1;
    VBK_REG=1;attr=get_vram_byte(map+offset);
    if(attr&0x80){VBK_REG=vbk;return;} /* Preserve every roof/canopy pixel. */
    VBK_REG=0;tile=get_vram_byte(map+offset);
    /* Bank1253 is outside authored bank1 BKG0..63/OBJ0..119, the atlas
     * scratch and aircraft64..78. Guidance bank1241..252 stays unchanged. */
    VBK_REG=(attr>>3)&1;get_bkg_data(tile,1,pixels);
    for(row=0;row<8;row++){
        /* Existing bright palette colour0 with a dark3 outline. Symmetric
         * masks preserve original flips without touching uncovered pixels. */
        pixels[row*2]=(pixels[row*2]|burst[row])&~glow[row];
        pixels[row*2+1]=(pixels[row*2+1]|burst[row])&~glow[row];
    }
    VBK_REG=1;set_bkg_data(TD_MUZZLE_TILE,1,pixels);
    td_muzzle_x=((x>>3)&31)|(map==(UBYTE*)0x9c00?32:0);td_muzzle_y=(y>>3)&31;
    td_muzzle_tile=tile;td_muzzle_attr=attr;
    set_vram_byte(map+offset,td_muzzle_attribute(attr));VBK_REG=0;set_vram_byte(map+offset,TD_MUZZLE_TILE);
    VBK_REG=vbk;
#endif
}
