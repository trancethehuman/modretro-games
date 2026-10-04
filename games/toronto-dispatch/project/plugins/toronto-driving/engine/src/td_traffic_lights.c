#pragma bank 255
#include "td_traffic_lights.h"
#include "td_game.h"
#include "td_district.h"
#include "scroll.h"
#include "gbs_types.h"
#include "compat.h"

typedef struct { UWORD u,v; UBYTE arms,tile_x,tile_y; } td_signal_t;
#define TD_TRAFFIC_SIGNALS_HORIZONTAL_ONLY
#include "td_traffic_signals.h"

/* Original two-head signals/pole; create_signal_art.py. Left EW, right NS. */
static const UBYTE td_light_patterns[32]={255,255,249,255,255,255,255,159,255,255,126,126,24,24,24,24,255,255,159,255,255,255,255,249,255,255,126,126,24,24,24,24};
static UBYTE td_light_patterns_ready;

void td_traffic_lights_reset(void) BANKED {td_light_patterns_ready=0;}

void td_traffic_lights_render(void) BANKED {
#ifdef CGB
    UWORD i,first,end,middle,lower,upper,offset;WORD x,y;
    UBYTE *map;UBYTE district,tile,save_vbk;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE){td_light_patterns_ready=0;return;}
    district=td_district_current();
    if(district>=TD_DISTRICT_COUNT||district>=TD_TRAFFIC_SIGNAL_DISTRICTS)return;
    if(draw_scroll_x>=1024||draw_scroll_y>=976||draw_scroll_x<-160||draw_scroll_y<-144)return;
    first=td_signal_offsets[district];end=td_signal_offsets[district+1];
    lower=draw_scroll_y>32?draw_scroll_y-32:0;
    upper=draw_scroll_y+176;
    while(first<end){middle=first+(end-first)/2;if(td_signals_h[middle].v<lower)first=middle+1;else end=middle;}
    save_vbk=VBK_REG&1;tile=td.seconds%12<7?47:48;map=GetBkgAddr();
    for(i=first;i<td_signal_offsets[district+1]&&td_signals_h[i].v<=upper;i++){
        x=td_signals_h[i].tile_x*8;y=td_signals_h[i].tile_y*8;
        if(x>draw_scroll_x+159||x+7<draw_scroll_x||y>draw_scroll_y+143||y+7<draw_scroll_y)continue;
        if(!td_light_patterns_ready){
            VBK_REG=1;set_bkg_data(47,2,td_light_patterns);td_light_patterns_ready=1;
        }
        offset=((UWORD)(td_signals_h[i].tile_y&31)<<5)|(td_signals_h[i].tile_x&31);
        VBK_REG=1;if(get_vram_byte(map+offset)!=15)set_vram_byte(map+offset,15);
        VBK_REG=0;if(get_vram_byte(map+offset)!=tile)set_vram_byte(map+offset,tile);
    }
    VBK_REG=save_vbk;
#endif
}
