#pragma bank 255
/* Day/night palettes: the tables and the copy into GBVM's palette buffers. */
#include <gbdk/platform.h>
#include <string.h>
#include "td_game.h"
#include "td_daynight.h"
#include "td_daynight_data.h"
#include "td_overlay.h"
#ifdef __SDCC
#include "palette.h"
#endif

UBYTE td_daynight_set,td_daynight_lights;
/* The phase is a mask of the play clock: keep a power-of-two day and 16
 * seconds per step. */
typedef char td_dn_day_is_1024_seconds[(TD_DN_DAY_SECONDS==1024&&TD_DN_STEPS==64)?1:-1];

static UWORD td_dn_phase(void){return (td.seconds+TD_DN_START)&(TD_DN_DAY_SECONDS-1);}

UWORD td_daynight_minutes(void) BANKED {return (td_dn_phase()*45)>>5;}

#ifdef __SDCC
/* Overcast skies dim the city a sixteenth; rain about a fifth, keeping
 * the blues so it reads wet. (RGB555: red 0..4, green 5..9, blue 10..14.) */
static void td_dn_tint(UWORD *c,UBYTE n){
    UBYTE r,g,b,rain=td_weather==TD_WEATHER_RAIN;
    for(;n;n--,c++){
        r=*c&31;g=(*c>>5)&31;b=(*c>>10)&31;
        r-=rain?(r>>3)+(r>>4):r>>4;g-=rain?(g>>3)+(g>>4):g>>4;b-=b>>4;
        *c=(UWORD)r|((UWORD)g<<5)|((UWORD)b<<10);
    }
}
#endif

static UBYTE td_dn_weather=255;
UBYTE td_daynight_apply(UBYTE flags) BANKED {
    UWORD phase=td_dn_phase();UBYTE set=td_dn_step_set[phase>>4];
    td_daynight_lights=phase>=TD_DN_LIGHTS_ON||phase<TD_DN_LIGHTS_OFF;
    if(set==td_daynight_set&&td_weather==td_dn_weather&&!(flags&TD_DN_FORCE))return FALSE;
    td_daynight_set=set;td_dn_weather=td_weather;
#ifdef __SDCC
    memcpy(BkgPalette,td_dn_bkg[set],sizeof(td_dn_bkg[0]));
    memcpy(SprPalette,td_dn_spr[set],sizeof(td_dn_spr[0]));
    if(td_weather!=TD_WEATHER_CLEAR){
        td_dn_tint((UWORD *)BkgPalette,28);td_dn_tint((UWORD *)SprPalette,32);
    }
    if(flags&TD_DN_HW){
        set_bkg_palette(0,7,(const palette_color_t *)BkgPalette);
        set_sprite_palette(0,8,(const palette_color_t *)SprPalette);
    }
#endif
    return TRUE;
}
