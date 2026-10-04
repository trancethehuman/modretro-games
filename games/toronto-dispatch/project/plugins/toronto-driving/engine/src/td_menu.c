#pragma bank 255
#include <gb/gb.h>
#include "td_menu.h"
static UBYTE td_menu_direction,td_menu_wait;
void td_menu_reset(void) BANKED {td_menu_direction=td_menu_wait=0;}
UBYTE td_menu_repeat(UBYTE held,UWORD elapsed) BANKED {
    UBYTE direction=held&(J_UP|J_DOWN|J_LEFT|J_RIGHT);
    /* A consumed action chord cannot repeat navigation in its destination menu. */
    if(held&(J_A|J_B|J_SELECT|J_START)){td_menu_reset();return 0;}
    /* Opposing directions cancel that axis rather than flicker the cursor. */
    if((direction&(J_UP|J_DOWN))==(J_UP|J_DOWN))direction&=~(J_UP|J_DOWN);
    if((direction&(J_LEFT|J_RIGHT))==(J_LEFT|J_RIGHT))direction&=~(J_LEFT|J_RIGHT);
    if(!direction){td_menu_reset();return 0;}
    if(direction!=td_menu_direction){td_menu_direction=direction;td_menu_wait=24;return 0;}
    if(elapsed<td_menu_wait){td_menu_wait-=elapsed;return 0;}
    /* One navigation step per render even after a delayed UI frame. */
    td_menu_wait=8;return direction;
}
