#pragma bank 255
/* Overlay sprites (td_overlay.h): the police helicopter, sharks, boats and
 * the police boat, rain, a drifting cloud with its shadow, sun rays and
 * gulls. They are written into the shadow OAM after the actors, from the
 * free bank-0 and bank-1 sprite tiles. */
#include <string.h>
#include "td_game.h"
#include "td_life.h"
#include "td_life_int.h"
#include "td_audio.h"
#include "td_daynight.h"
#include "td_district.h"
#include "td_overlay.h"
#define TD_OVERLAY_DATA
#include "td_overlay_data.h"
#include "actor.h"
#include "scroll.h"
#include "collision.h"
#ifdef __SDCC
#include <gbdk/metasprites.h>
#include "data_manager.h"
#include "bankdata.h"
#include "td_ui_art.h"
/* The bank-1 designs sit above the font glyphs. */
typedef char td_overlay1_above_font[(TD_FONT_FIRST+TD_FONT_GLYPHS<=TD_OVERLAY1_FIRST)?1:-1];
#endif

typedef char td_overlay_tiles_fit[(TD_OVERLAY_FIRST+TD_OVERLAY_TILES<=256&&TD_OVERLAY1_FIRST+TD_OVERLAY1_TILES<=256)?1:-1];
#define OV_X_FLIP 0x20
#define OV_Y_FLIP 0x40
/* OBJ palettes (the actors' order): ice blue for the helicopter, sharks,
 * boats, rain and clouds, red for the other boats and the police boat's
 * flash, signal yellow for sun rays, white for gulls. */
#define OV_PAL_BLUE TD_PAL_BLUE
#define OV_PAL_RED TD_PAL_RED
#define OV_PAL_SUN TD_PAL_YELLOW
#define OV_PAL_WHITE TD_PAL_COURIER

extern UBYTE td_swimming;
UBYTE td_weather;
static UBYTE ov_on,ov_rng=0x5A;
/* Police helicopter: whole pixels in the current scene. Shark and boats
 * likewise. Globals so emulator checks can read them. */
#define HELI_NONE 0
#define HELI_TRACK 1
#define HELI_LEAVE 2
#define HELI_SEARCH 3
#define SHARK_CIRCLE 1
#define SHARK_CLOSE 2
#define SHARK_LEAVE 3
UBYTE heli_state,shark_state;
WORD heli_u,heli_v,shark_u,shark_v;
static UBYTE heli_turn,heli_lost,heli_search,shark_wait,shark_left,shark_turn;
static WORD heli_seen_u,heli_seen_v;
static BYTE shark_du;
/* Boats on open water: 0 none, 1 motorboat, 2 sailboat, 3 police boat. */
#define BOAT_MOTOR 1
#define BOAT_SAIL 2
#define BOAT_POLICE 3
#define OV_BOATS 2
UBYTE boat_kind[OV_BOATS];
WORD boat_u[OV_BOATS],boat_v[OV_BOATS];
static BYTE boat_dir[OV_BOATS];
static UBYTE boat_pal[OV_BOATS],boat_wait;
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
static WORD ov_step(WORD from,WORD to,UBYTE fast){
    if(from<to)return from+(to-from>fast?fast:1);
    if(from>to)return from-(from-to>fast?fast:1);
    return from;
}

/* Cover from the air: a background tile with priority (a roof edge, the
 * upper floors of a tower, a tree canopy) over the courier's body. */
#ifdef __SDCC
static UBYTE ov_cover_at(WORD u,WORD v){
    if(u<0||v<0||u>=1024||v>=976)return FALSE;
    /* Every city scene is 128 tiles wide (TORONTO.c td_scan_clear). */
    return ReadBankedUBYTE(image_attr_ptr+((UWORD)(UBYTE)(v>>3)<<7)+(UBYTE)(u>>3),image_attr_bank)&0x80;
}
#else
UBYTE td_test_cover;
static UBYTE ov_cover_at(WORD u,WORD v){(void)u;(void)v;return td_test_cover;}
#endif
UBYTE td_overlay_covered(void) BANKED {
    WORD u=(WORD)(td.u>>4),v=(WORD)(td.v>>4);
    /* On foot the body stands above the feet; a car is covered at its middle. */
    return ov_cover_at(u,td.onfoot?v-5:v);
}

/* The helicopter's eight stations round the courier while it tracks
 * (pixels), and twice as wide while it searches. */
static const BYTE heli_du[8]={0,26,36,26,0,-26,-36,-26},heli_dv[8]={-36,-26,0,26,36,26,0,-26};

void td_overlay_init(void) BANKED {
    UBYTE i;
#ifdef __SDCC
    VBK_REG=0;set_sprite_data(TD_OVERLAY_FIRST,TD_OVERLAY_TILES,td_overlay_tiles);
    VBK_REG=1;set_sprite_data(TD_OVERLAY1_FIRST,TD_OVERLAY1_TILES,td_overlay_tiles1);VBK_REG=0;
#endif
    heli_state=shark_state=gull_on=0;shark_wait=180;gull_wait=200;boat_wait=30;
    memset(boat_kind,0,sizeof(boat_kind));
    for(i=0;i<OV_DROPS;i++){drop_x[i]=(UBYTE)(i*37+11);drop_y[i]=(UBYTE)(i*29);}
    cloud_u=(WORD)(td.u>>4)-120;cloud_v=(WORD)(td.v>>4)-40;ray_x=0;
    td_overlay_second();ov_on=0;
}

void td_overlay_show(UBYTE on) BANKED {ov_on=on;}

/* Weather by the play clock: a new spell every 128 play seconds (three
 * game hours), from a fixed two-day pattern starting at 08:00: rain on
 * the first evening and the second day's late morning, cloud between. */
static const UBYTE td_weather_plan[16]={0,0,1,0,2,2,1,0,1,2,2,1,0,1,2,0};
UBYTE td_overlay_second(void) BANKED {
    UBYTE w=td_weather_plan[(td.seconds>>7)&15];
    if(w==td_weather)return FALSE;
    td_weather=w;return TRUE;
}

/* The police helicopter has no weapon: it finds the courier and keeps the
 * heat on while it can see them. A courier under a roof, an upper floor or
 * a tree canopy drops out of sight; after a second the crew searches round
 * the last place they saw them, and finds them again only out in the open
 * nearby. It leaves once the stars drop below three. */
static void heli_tick(void){
    WORD pu=(WORD)(td.u>>4),pv=(WORD)(td.v>>4),tu,tv;UBYTE fast,covered,want;
    want=td.wanted>=4&&(td.mode==TD_ROAM||td.mode==TD_WAIT);
    if(!heli_state){
        if(!want)return;
        /* It comes in from the side away from the nearer scene edge. */
        heli_u=pu<512?pu+160:pu-160;heli_v=pv-100;heli_state=HELI_TRACK;heli_turn=0;heli_lost=0;
        heli_seen_u=pu;heli_seen_v=pv;
        td_message(TD_MSG_HELI);return;
    }
    if(heli_state!=HELI_LEAVE&&td.wanted<3)heli_state=HELI_LEAVE;
    covered=td_overlay_covered()||td.mode==TD_RIDE;
    if(heli_state==HELI_TRACK){
        if(covered){if(++heli_lost>=60){heli_state=HELI_SEARCH;heli_search=0;}}
        else{heli_lost=0;heli_seen_u=pu;heli_seen_v=pv;}
    }else if(heli_state==HELI_SEARCH){
        /* Out in the open within the searchlight's reach: found again. */
        if(!covered&&ov_abs(heli_u-pu)<72&&ov_abs(heli_v-pv)<64){heli_state=HELI_TRACK;heli_lost=0;}
        /* A long fruitless search at four stars: the crew gives up. */
        else if(!(td_tick&63)&&++heli_search>=40&&td.wanted<5)heli_state=HELI_LEAVE;
    }
    if(heli_state==HELI_LEAVE){tu=heli_u<pu?pu-280:pu+280;tv=pv-220;fast=2;}
    else{
        /* A station every two seconds round the courier, or round the last
         * place they were seen. */
        if(!(td_tick&127))heli_turn=(heli_turn+1)&7;
        if(heli_state==HELI_TRACK){tu=pu+heli_du[heli_turn];tv=pv+heli_dv[heli_turn];fast=ov_abs(heli_u-tu)>40||ov_abs(heli_v-tv)>40?2:1;}
        else{tu=heli_seen_u+heli_du[heli_turn]*2;tv=heli_seen_v+heli_dv[heli_turn]*2;fast=1;if(td_tick&1)return;}
    }
    heli_u=ov_step(heli_u,tu,fast);heli_v=ov_step(heli_v,tv,fast);
    if(heli_state==HELI_LEAVE&&(ov_abs(heli_u-pu)>220||ov_abs(heli_v-pv)>200))heli_state=HELI_NONE;
}

/* Sharks are rare and unhurried: one may rise somewhere off in open water,
 * circle a while, then close in a little faster than a swimmer (who can
 * still reach the shore), break off now and then, and swim away after a
 * bite or when the swimmer leaves the water. */
static const BYTE shark_ou[8]={48,34,0,-34,-48,-34,0,34},shark_ov[8]={0,34,48,34,0,-34,-48,-34};
static void shark_tick(void){
    WORD pu=(WORD)(td.u>>4),pv=(WORD)(td.v>>4),tu,tv;UBYTE dir,k;
    if(!shark_state){
        if(!td_swimming||!td.vitality){if(shark_wait<120)shark_wait=120;return;}
        if(shark_wait){shark_wait--;return;}
        shark_wait=60;
        /* Out in open water only, and one chance in four each second. */
        if(!ov_water(pu-24,pv)||!ov_water(pu+24,pv)||!ov_water(pu,pv-24)||!ov_water(pu,pv+24))return;
        if(ov_random()&3)return;
        for(k=0,dir=ov_random()&7;k<8;k++,dir=(dir+3)&7){
            tu=pu+shark_ou[dir]*2;tv=pv+shark_ov[dir]*2;
            if(ov_water(tu,tv)){shark_u=tu;shark_v=tv;shark_turn=dir;shark_state=SHARK_CIRCLE;shark_left=120+(ov_random()&127);td_message(TD_MSG_SHARK);return;}
        }
        return;
    }
    if(shark_state!=SHARK_LEAVE&&!td_swimming){shark_state=SHARK_LEAVE;shark_left=120;}
    switch(shark_state){
    case SHARK_CIRCLE:
        /* Circling at a distance, a step every other tick. */
        if(!(td_tick&15))shark_turn=(shark_turn+1)&7;
        tu=pu+shark_ou[shark_turn];tv=pv+shark_ov[shark_turn];
        if(!--shark_left){shark_state=SHARK_CLOSE;shark_left=255;}
        if(td_tick&1)return;
        break;
    case SHARK_CLOSE:
        tu=pu;tv=pv;
        if(ov_abs(pu-shark_u)<7&&ov_abs(pv-shark_v)<7){
            td_lf_hurt(25);td_message(TD_MSG_SHARK_BITE);shark_state=SHARK_LEAVE;shark_left=120;return;
        }
        /* Now and then it loses interest and circles again. */
        if(!(td_tick&63)&&!(ov_random()&3)){shark_state=SHARK_CIRCLE;shark_left=90+(ov_random()&63);}
        /* A step every third tick: a third of a pixel, against a swimmer's 0.31. */
        if(td_tick%3)return;
        break;
    default:
        if(!--shark_left){shark_state=0;shark_wait=240+(ov_random()&127);return;}
        tu=shark_u+(shark_u<pu?-16:16);tv=shark_v+(shark_v<pv?-16:16);
        if(td_tick&1)return;
        break;
    }
    if(tu!=shark_u){shark_du=tu>shark_u?1:-1;if(ov_water(shark_u+shark_du,shark_v))shark_u+=shark_du;}
    if(tv!=shark_v&&ov_water(shark_u,shark_v+(tv>shark_v?1:-1)))shark_v+=tv>shark_v?1:-1;
}

/* Boats cross open water near the courier: motorboats and sailboats come
 * in from beyond the screen edge, turn at the shore and go on their way.
 * With stars on a swimming courier, a police boat comes out, stays a few
 * boat lengths off and keeps the heat on while it can see them; it never
 * rams or shoots. */
static UBYTE boat_spot(UBYTE i,UBYTE kind){
    WORD u,v;UBYTE k,s=ov_random();
    for(k=0;k<4;k++,s+=37){
        u=(s&1)?(WORD)scroll_x-24:(WORD)scroll_x+184;
        v=(WORD)scroll_y+24+(WORD)(s%104);
        if(!ov_water(u,v)||!ov_water(u,v-6)||!ov_water(u,v+6)||!ov_water(u+(s&1?12:-12),v))continue;
        boat_kind[i]=kind;boat_u[i]=u;boat_v[i]=v;boat_dir[i]=(s&1)?1:-1;
        boat_pal[i]=kind==BOAT_POLICE||(s&2)?OV_PAL_BLUE:OV_PAL_RED;
        return TRUE;
    }
    return FALSE;
}
static void boats_tick(void){
    UBYTE i,kind,chase=td.wanted&&td_swimming&&td.mode==TD_ROAM;WORD pu=(WORD)(td.u>>4),pv=(WORD)(td.v>>4),ahead,cu;
    cu=(WORD)scroll_x+80;
    for(i=0;i<OV_BOATS;i++){
        kind=boat_kind[i];
        if(!kind)continue;
        if(kind==BOAT_POLICE){
            /* Hold station a few lengths off the swimmer, on the water. */
            if(!td.wanted){kind=boat_kind[i]=BOAT_MOTOR;}
            else if(!(td_tick&1)){
                ahead=boat_u[i]<pu-40?1:boat_u[i]>pu+40?-1:0;
                if(ahead){boat_dir[i]=(BYTE)ahead;if(ov_water(boat_u[i]+ahead*10,boat_v[i]))boat_u[i]+=ahead;}
                if(boat_v[i]!=pv&&ov_water(boat_u[i],boat_v[i]+(pv>boat_v[i]?7:-7)))boat_v[i]+=pv>boat_v[i]?1:-1;
            }
        }
        if(kind!=BOAT_POLICE&&!(td_tick&(kind==BOAT_SAIL?3:1))){
            /* Turn about at the shore. */
            if(!ov_water(boat_u[i]+boat_dir[i]*10,boat_v[i]))boat_dir[i]=-boat_dir[i];
            else boat_u[i]+=boat_dir[i];
        }
        if(ov_abs(boat_u[i]-cu)>220||ov_abs(boat_v[i]-(WORD)scroll_y-72)>180)boat_kind[i]=0;
    }
    if(boat_wait){boat_wait--;return;}
    boat_wait=90;
    if(chase&&boat_kind[0]!=BOAT_POLICE){
        if(boat_kind[1]==BOAT_POLICE)return;
        boat_spot(0,BOAT_POLICE);return;
    }
    for(i=0;i<OV_BOATS;i++)if(!boat_kind[i]){boat_spot(i,(ov_random()&3)?BOAT_MOTOR:BOAT_SAIL);break;}
}

UBYTE td_overlay_spotted(void) BANKED {
    WORD pu=(WORD)(td.u>>4),pv=(WORD)(td.v>>4);UBYTE i;
    if(heli_state==HELI_TRACK&&ov_abs(heli_u-pu)<96&&ov_abs(heli_v-pv)<88)return TRUE;
    for(i=0;i<OV_BOATS;i++)
        if(boat_kind[i]==BOAT_POLICE&&ov_abs(boat_u[i]-pu)<104&&ov_abs(boat_v[i]-pv)<88)return TRUE;
    return FALSE;
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
    heli_tick();shark_tick();boats_tick();sky_tick();
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
static void ov_pair(WORD u,WORD v,UBYTE tile,UBYTE prop){
    WORD x=u-draw_scroll_x-8,y=v-draw_scroll_y-8;
    ov_put(x,y,tile,prop);ov_put(x+8,y,tile,prop|OV_X_FLIP);
}

void td_overlay_render(void) BANKED {
    UBYTE i,pal,rotor;WORD x,y;
    if(!ov_on||(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE))return;
    if(heli_state){
        /* 16x32, centred: cabin and rotor above, tail boom below. */
        rotor=td_tick&2;
        ov_pair(heli_u,heli_v-8,rotor?TD_OV_HELI_TOP_A:TD_OV_HELI_TOP_B,OV_PAL_BLUE);
        ov_pair(heli_u,heli_v+8,rotor?TD_OV_HELI_BOT_A:TD_OV_HELI_BOT_B,OV_PAL_BLUE);
    }
    for(i=0;i<OV_BOATS;i++){
        if(!boat_kind[i])continue;
        x=boat_u[i]-draw_scroll_x;y=boat_v[i]-draw_scroll_y-8;
        /* The police boat flashes red and blue. */
        pal=boat_kind[i]==BOAT_POLICE?((td_tick&8)?OV_PAL_RED:OV_PAL_BLUE):boat_pal[i];
        if(boat_kind[i]==BOAT_SAIL)ov_put(x-4,y-4,TD_OV_SAIL,pal|TD_OV_SAIL_BANK|(boat_dir[i]<0?OV_X_FLIP:0));
        else if(boat_dir[i]>0){ov_put(x-8,y,TD_OV_BOAT_L,pal|TD_OV_BOAT_L_BANK);ov_put(x,y,TD_OV_BOAT_R,pal|TD_OV_BOAT_R_BANK);}
        else{ov_put(x-8,y,TD_OV_BOAT_R,pal|TD_OV_BOAT_R_BANK|OV_X_FLIP);ov_put(x,y,TD_OV_BOAT_L,pal|TD_OV_BOAT_L_BANK|OV_X_FLIP);}
    }
    if(shark_state)ov_put(shark_u-draw_scroll_x-4,shark_v-draw_scroll_y-8,(td_tick&8)?TD_OV_SHARK_A:TD_OV_SHARK_B,
                          OV_PAL_BLUE|(shark_du>0?OV_X_FLIP:0));
    /* The gull's wings beat: upside down its V becomes a downstroke. */
    if(gull_on)ov_put(gull_u-draw_scroll_x-4,gull_v-draw_scroll_y-8,TD_OV_GULL,OV_PAL_WHITE|TD_OV_GULL_BANK|((td_tick&16)?OV_Y_FLIP:0));
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
