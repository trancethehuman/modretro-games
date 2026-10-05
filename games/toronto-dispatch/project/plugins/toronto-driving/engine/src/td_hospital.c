#pragma bank 255
#include "td_hospital.h"
#include "td_game.h"
#include "td_district.h"
#ifdef CGB
#include "scroll.h"
#include "ui.h"
#include "compat.h"
/* Deliberately authored eight-pixel mint plus, cream border, navy ground.
 * Editable cells/provenance live in content/hospital.json and the source PNG.
 * This is original game signage; no hospital logo or official mark is used. */
static const UBYTE td_hospital_badge[]={
    0x7e,0x7e,0x99,0x81,0x99,0x81,0xff,0x81,
    0xff,0x81,0x99,0x81,0x99,0x81,0x7e,0x7e
};
#endif
void td_hospital_init(void) BANKED {
#ifdef CGB
    UBYTE vbk=VBK_REG&1;
    VBK_REG=1;set_bkg_data(TD_HOSPITAL_TILE,1,td_hospital_badge);VBK_REG=vbk;
#endif
}
void td_hospital_render(void) BANKED {
#ifdef CGB
    WORD screen_x,screen_y;UWORD offset;UBYTE vbk,attr;UBYTE *map;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE)return;
    if(td.district!=TD_HOSPITAL_DISTRICT)return;
    screen_x=TD_HOSPITAL_MARKER_U-draw_scroll_x;
    screen_y=TD_HOSPITAL_MARKER_V-draw_scroll_y;
    /* Admit only a complete tile outside actual and destination HUD bounds.
     * Menus, dialogue, maps, rooms and the other six districts keep their art. */
    if(screen_x<0||screen_y<0||screen_x>152||screen_y>136||
       (screen_x+7>=win_pos_x&&screen_y+7>=win_pos_y)||
       (screen_x+7>=win_dest_pos_x&&screen_y+7>=win_dest_pos_y)||
       (WX_REG&&WY_REG<144&&screen_x+7>=(WORD)WX_REG-7&&screen_y+7>=WY_REG))return;
    /* Most Core views do not contain this one facade. Avoid a banked scene
     * lookup until the cheap geometry/window checks admit the whole badge. */
    if(td_district_current()!=TD_HOSPITAL_DISTRICT)return;
    map=GetBkgAddr();offset=((UWORD)((TD_HOSPITAL_MARKER_V>>3)&31)<<5)|((TD_HOSPITAL_MARKER_U>>3)&31);
    vbk=VBK_REG&1;VBK_REG=1;attr=get_vram_byte(map+offset);
    /* A permanent world annotation, rebuilt after fresh scroll uploads.
     * Keep the facade's roof priority so walkers remain correctly occluded;
     * the upright badge uses UI palette7 without inheriting source flips. */
    if((attr&127)==15){
        VBK_REG=0;
        if(get_vram_byte(map+offset)==TD_HOSPITAL_TILE){VBK_REG=vbk;return;}
    }
    VBK_REG=1;set_vram_byte(map+offset,(attr&128)|15);VBK_REG=0;
    set_vram_byte(map+offset,TD_HOSPITAL_TILE);VBK_REG=vbk;
#endif
}
