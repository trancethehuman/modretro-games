#pragma bank 255
/* Overlay sprites (td_overlay.h): the police helicopter, sharks, rain, a
 * drifting cloud with its shadow, sun rays and gulls. They are written into
 * the shadow OAM after the actors, from the free bank-0 sprite tiles. */
#include <string.h>
#include "td_game.h"
#include "td_life.h"
#include "td_life_int.h"
#include "td_shots.h"
#include "td_audio.h"
#include "td_daynight.h"
#include "td_overlay.h"
#define TD_OVERLAY_DATA
#include "td_overlay_data.h"
#include "actor.h"
#include "scroll.h"
#include "collision.h"
#ifdef __SDCC
#include <gbdk/metasprites.h>
#endif

typedef char td_overlay_tiles_fit[(TD_OVERLAY_FIRST+TD_OVERLAY_TILES<=256)?1:-1];
#define OV_X_FLIP 0x20
/* OBJ palettes (the actors' order): ice blue for the helicopter, sharks,
 * rain and clouds, signal yellow for sun rays, white for gulls. */
#define OV_PAL_BLUE TD_PAL_BLUE
#define OV_PAL_SUN TD_PAL_YELLOW
#define OV_PAL_WHITE TD_PAL_COURIER

extern UBYTE td_swimming;
UBYTE td_weather;
static UBYTE ov_on,ov_rng=0x5A;
/* Police helicopter: whole pixels in the current scene; 0 no helicopter,
 * 1 coming in or circling, 2 leaving. Shark: 0 none, 1 hunting, 2
 * leaving. Globals so emulator checks can read them. */
UBYTE heli_state,shark_state;
WORD heli_u,heli_v,shark_u,shark_v;
static UBYTE heli_turn,heli_fire,shark_wait,shark_left;
static BYTE shark_du;
/* Rain drops on the screen; the cloud, its shadow and the gulls in the
 * scene (whole pixels). */
#define OV_DROPS 5
static UBYTE drop_x[OV_DROPS],drop_y[OV_DROPS];
static WORD cloud_u,cloud_v,gull_u,gull_v;
static UBYTE gull_on,gull_wait,ray_x;

static UBYTE ov_random(void){
    ov_rng^=ov_rng<<3;ov_rng^=ov_rng>>5;ov_rng^=ov_rng<<1;
    return ov_rng^td_tick;
}
static UBYTE ov_water(WORD u,WORD v){
    if(u<0||v<0||u>=1024||v>=976)return FALSE;
    return tile_at((UBYTE)(u>>3),(UBYTE)(v>>3))==TD_COLLISION_WATER;
}
static WORD ov_abs(WORD x){return x<0?-x:x;}

/* The helicopter's eight stations round the courier (pixels). */
static const BYTE heli_du[8]={0,28,40,28,0,-28,-40,-28},heli_dv[8]={-40,-28,0,28,40,28,0,-28};

void td_overlay_init(void) BANKED {
    UBYTE i;
#ifdef __SDCC
    VBK_REG=0;set_sprite_data(TD_OVERLAY_FIRST,TD_OVERLAY_TILES,td_overlay_tiles);
#endif
    heli_state=shark_state=gull_on=0;shark_wait=120;gull_wait=200;
    for(i=0;i<OV_DROPS;i++){drop_x[i]=(UBYTE)(i*37+11);drop_y[i]=(UBYTE)(i*29);}
    cloud_u=(WORD)(td.u>>4)-120;cloud_v=(WORD)(td.v>>4)-40;ray_x=0;
    td_overlay_second();ov_on=0;
}

void td_overlay_show(UBYTE on) BANKED {ov_on=on;}

UBYTE td_overlay_heli_near(UBYTE ru,UBYTE rv) BANKED {
    return heli_state==1&&ov_abs(heli_u-(WORD)(td.u>>4))<ru&&ov_abs(heli_v-(WORD)(td.v>>4))<rv;
}

/* Weather by the play clock: a new spell every 128 play seconds (three
 * game hours), from a fixed two-day pattern starting at 08:00: rain on
 * the first evening and the second day's late morning, cloud between. */
static const UBYTE td_weather_plan[16]={0,0,1,0,2,2,1,0,1,2,2,1,0,1,2,0};
UBYTE td_overlay_second(void) BANKED {
    UBYTE w=td_weather_plan[(td.seconds>>7)&15];
    if(w==td_weather)return FALSE;
    td_weather=w;return TRUE;
}

static void heli_tick(void){
    WORD pu=(WORD)(td.u>>4),pv=(WORD)(td.v>>4),tu,tv;
    UBYTE want=td.wanted>=4&&(td.mode==TD_ROAM||td.mode==TD_WAIT);
    if(!heli_state){
        if(!want)return;
        /* It comes in from the side of the screen away from the courier's
         * nearer scene edge. */
        heli_u=pu<512?pu+150:pu-150;heli_v=pv-90;heli_state=1;heli_turn=0;heli_fire=120;
        td_message(TD_MSG_HELI);return;
    }
    if(heli_state==1&&!want&&td.wanted<3)heli_state=2;
    if(heli_state==2){tu=heli_u<pu?pu-260:pu+260;tv=pv-200;}
    else{
        /* It circles the courier slowly, a station every two seconds. */
        if(!(td_tick&127))heli_turn=(heli_turn+1)&7;
        tu=pu+heli_du[heli_turn];tv=pv+heli_dv[heli_turn];
    }
    /* Two pixels a frame when it is well away (faster than any car), one
     * on station, so it hovers rather than darts. */
    if(heli_u<tu)heli_u+=tu-heli_u>24?2:1;else if(heli_u>tu)heli_u-=heli_u-tu>24?2:1;
    if(heli_v<tv)heli_v+=tv-heli_v>24?2:1;else if(heli_v>tv)heli_v-=heli_v-tv>24?2:1;
    if(heli_state==2&&(ov_abs(heli_u-pu)>200||ov_abs(heli_v-pv)>180)){heli_state=0;return;}
    /* At five stars the crew fires down at the courier now and then: a
     * wide spread from the air, so moving keeps most rounds off. */
    if(heli_state==1&&td.wanted>=5&&td.vitality&&heli_fire&&!--heli_fire){
        heli_fire=150-(ov_random()&63);
        if(ov_abs(heli_u-pu)<64&&ov_abs(heli_v-pv)<56&&heli_u>=0&&heli_v>=0)
            td_shot_fire(TD_SHOT_POLICE,(UWORD)heli_u,(UWORD)heli_v,(UWORD)pu,(UWORD)pv,18);
    }
}

static void shark_tick(void){
    WORD pu=(WORD)(td.u>>4),pv=(WORD)(td.v>>4),du,dv;UBYTE dir;
    if(!shark_state){
        if(!td_swimming||!td.vitality){shark_wait=120;return;}
        if(shark_wait){shark_wait--;return;}
        shark_wait=60;
        /* Away from the shore, one chance in three every second; it rises
         * 56 px off in open water, in whichever direction has some. */
        if(!ov_water(pu-16,pv)&&!ov_water(pu+16,pv))return;
        if(!ov_water(pu,pv-16)&&!ov_water(pu,pv+16))return;
        if(ov_random()%3)return;
        for(dir=ov_random()&3,du=0;du<4;du++,dir=(dir+1)&3){
            shark_u=pu+(dir==0?56:dir==1?-56:0);shark_v=pv+(dir==2?56:dir==3?-56:0);
            if(ov_water(shark_u,shark_v)){shark_state=1;shark_left=255;td_message(TD_MSG_SHARK);return;}
        }
        return;
    }
    if(shark_state==1&&!td_swimming)shark_state=2;
    if(shark_state==2){
        if(!shark_left--){shark_state=0;shark_wait=180;return;}
        du=shark_u<pu?-1:1;dv=shark_v<pv?-1:1;
    }else{
        du=pu>shark_u?1:pu<shark_u?-1:0;dv=pv>shark_v?1:pv<shark_v?-1:0;
        /* Three steps in four: a swimmer who keeps moving can stay ahead. */
        if((td_tick&3)==3)du=dv=0;
        if(ov_abs(pu-shark_u)<7&&ov_abs(pv-shark_v)<7){
            td_lf_hurt(25);td_message(TD_MSG_SHARK_BITE);shark_state=2;shark_left=90;return;
        }
    }
    if(du){shark_du=(BYTE)du;if(ov_water(shark_u+du,shark_v))shark_u+=du;}
    if(dv&&ov_water(shark_u,shark_v+dv))shark_v+=dv;
}

static void sky_tick(void){
    UBYTE i;WORD cu=(WORD)(td.u>>4),cv=(WORD)(td.v>>4);
    if(td_weather==TD_WEATHER_RAIN)for(i=0;i<OV_DROPS;i++){
        drop_y[i]+=3;drop_x[i]--;
        if(drop_y[i]>=136){drop_y[i]=ov_random()&15;drop_x[i]=ov_random()%168;}
    }
    /* The cloud drifts east with the wind; once past the courier it forms
     * again upwind, just off the screen. */
    if(td_weather!=TD_WEATHER_CLEAR){
        if(td_tick&1)cloud_u++;
        if(cloud_u>cu+100||cloud_u<cu-160||cloud_v<cv-120||cloud_v>cv+120){
            cloud_u=cu-96;cloud_v=cv-56+(ov_random()&63);
        }
    }
    /* Now and then a gull glides over, low across the screen. */
    if(gull_on){
        gull_u++;if(td_tick&4)gull_v+=(td_tick&8)?1:-1;
        if(gull_u>cu+110){gull_on=0;gull_wait=200+(ov_random()&127);}
    }else if(td_weather!=TD_WEATHER_RAIN&&!--gull_wait){
        gull_on=1;gull_u=cu-110;gull_v=cv-56+(ov_random()&63);
    }
    if(!(td_tick&15))ray_x++;
}

void td_overlay_tick(void) BANKED {
    heli_tick();shark_tick();sky_tick();
}

#ifdef __SDCC
/* One 8x16 OAM entry at screen (x, y) (top-left, pixels); off-screen and
 * window rows are skipped. */
static void ov_put(WORD x,WORD y,UBYTE tile,UBYTE prop){
    OAM_item_t *o;
    if(x<=-8||x>=160||y<=-16||y>=144||allocated_hardware_sprites>=MAX_HARDWARE_SPRITES)return;
    if(WY_REG<144&&y+8>(WORD)WY_REG)return;
    o=&((OAM_item_t *)((UWORD)__render_shadow_OAM<<8))[allocated_hardware_sprites++];
    o->y=(UBYTE)(y+16);o->x=(UBYTE)(x+8);o->tile=tile;o->prop=prop;
}
#else
static void ov_put(WORD x,WORD y,UBYTE tile,UBYTE prop){(void)x;(void)y;(void)tile;(void)prop;}
#endif

/* A 16-wide figure centred on scene pixel (u, v): left half and its mirror. */
static void ov_pair(WORD u,WORD v,UBYTE tile,UBYTE pal){
    WORD x=u-draw_scroll_x-8,y=v-draw_scroll_y-8;
    ov_put(x,y,tile,pal);ov_put(x+8,y,tile,pal|OV_X_FLIP);
}

void td_overlay_render(void) BANKED {
    UBYTE i;WORD x;
    if(!ov_on||(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE))return;
    if(heli_state)ov_pair(heli_u,heli_v,(td_tick&2)?TD_OV_HELI_A:TD_OV_HELI_B,OV_PAL_BLUE);
    if(shark_state)ov_put(shark_u-draw_scroll_x-4,shark_v-draw_scroll_y-8,(td_tick&8)?TD_OV_SHARK_A:TD_OV_SHARK_B,
                          OV_PAL_BLUE|(shark_du>0?OV_X_FLIP:0));
    if(gull_on)ov_put(gull_u-draw_scroll_x-4,gull_v-draw_scroll_y-8,(td_tick&16)?TD_OV_GULL_A:TD_OV_GULL_B,OV_PAL_WHITE);
    if(td_weather!=TD_WEATHER_CLEAR){
        /* The shadow falls south-east of the cloud; both flicker on
         * alternate frames, which reads as see-through. */
        if(td_tick&1)ov_pair(cloud_u+20,cloud_v+28,TD_OV_CLOUD_SHADOW,OV_PAL_BLUE);
        else ov_pair(cloud_u,cloud_v,TD_OV_CLOUD,OV_PAL_BLUE);
    }
    if(td_weather==TD_WEATHER_RAIN)
        for(i=0;i<OV_DROPS;i++)ov_put((WORD)drop_x[i]-8,(WORD)drop_y[i],TD_OV_RAIN,OV_PAL_BLUE);
    else if(td_weather==TD_WEATHER_CLEAR&&(td_tick&1)){
        /* Low sun in the morning and the evening throws shafts of light. */
        x=(WORD)td_daynight_minutes();
        if((x>=420&&x<570)||(x>=1020&&x<1140)){
            x=(WORD)(ray_x&63);
            ov_put(12+x,8,TD_OV_SUN_RAY,OV_PAL_SUN);ov_put(28+x,24,TD_OV_SUN_RAY,OV_PAL_SUN);
        }
    }
}
