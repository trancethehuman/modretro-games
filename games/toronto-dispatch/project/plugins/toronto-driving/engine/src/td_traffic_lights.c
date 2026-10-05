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
#include "td_signal_graphics.h"

/* Larger original heads: separate EW/NS poles, coral red/park green palettes. */
static const UBYTE td_light_patterns[32]={126,126,126,70,126,70,126,126,126,126,24,24,24,24,24,24,126,126,126,126,126,126,126,70,126,70,24,24,24,24,24,24};
static UBYTE td_light_patterns_ready;

void td_traffic_lights_reset(void) BANKED {td_light_patterns_ready=0;}

void td_traffic_lights_render(void) BANKED {
#ifdef CGB
    UWORD i,first,end,middle,lower,upper,offset;WORD x,y;
    UBYTE *map;UBYTE district,tile,save_vbk,head,tx,ty,attr,green;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE){td_light_patterns_ready=0;return;}
    district=td_district_current();
    if(district>=TD_DISTRICT_COUNT||district>=TD_TRAFFIC_SIGNAL_DISTRICTS)return;
    if(draw_scroll_x>=1024||draw_scroll_y>=976||draw_scroll_x<-160||draw_scroll_y<-144)return;
    first=td_signal_offsets[district];end=td_signal_offsets[district+1];
    /* Source/native geometry gates prove every8px head is at junction
     * X[-32,32],Y[-32,24]. Cull whole junctions before decoding both heads;
     * retain the exact per-head clip and repaint checks at viewport edges. */
    lower=draw_scroll_y>31?draw_scroll_y-31:0;
    upper=draw_scroll_y+175;
    while(first<end){middle=first+(end-first)/2;if(td_signals_h[middle].v<lower)first=middle+1;else end=middle;}
    if(first>=td_signal_offsets[district+1]||td_signals_h[first].v>upper)return;
    save_vbk=VBK_REG&1;green=td.seconds%12<7;map=GetBkgAddr();
    for(i=first;i<td_signal_offsets[district+1]&&td_signals_h[i].v<=upper;i++){
        x=(WORD)td_signals_h[i].u-draw_scroll_x;
        if(x<-39||x>191)continue;
        for(head=0;head<2;head++){
            tx=head?td_signal_extra[i][0]:td_signals_h[i].tile_x;
            ty=head?td_signal_extra[i][1]:td_signals_h[i].tile_y;
            x=tx*8;y=ty*8;
            if(x>draw_scroll_x+159||x+7<draw_scroll_x||y>draw_scroll_y+143||y+7<draw_scroll_y)continue;
            if(!td_light_patterns_ready){
                VBK_REG=1;set_bkg_data(47,2,td_light_patterns);td_light_patterns_ready=1;
            }
            tile=(green^head)?48:47;attr=(green^head)?14:11;
            offset=((UWORD)(ty&31)<<5)|(tx&31);
            VBK_REG=1;if(get_vram_byte(map+offset)!=attr)set_vram_byte(map+offset,attr);
            VBK_REG=0;if(get_vram_byte(map+offset)!=tile)set_vram_byte(map+offset,tile);
        }
    }
    VBK_REG=save_vbk;
#endif
}
