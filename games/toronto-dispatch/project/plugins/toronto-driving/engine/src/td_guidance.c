#pragma bank 255
#include <string.h>
#include "td_guidance.h"
#include "td_game.h"
#include "actor.h"
#include "ui.h"
#ifdef CGB
#include "td_navigation.h"
#include "td_district.h"
#include "td_boats.h"
#include "scroll.h"
#include "gbs_types.h"
#include "compat.h"
/* The prior arrow's tile is restored only while it retains ownership. Scene
 * initialization discards this address; menus/atlas may safely replace it. */
static UWORD td_road_arrow_offset;
/* Zero is inactive; job+1 tags the existing patch byte without more WRAM. */
static UBYTE td_road_arrow_tile,td_road_arrow_attr,td_road_arrow_active;
/* Exact goal and eight-pixel player cell determine the ROM waypoint. Retain
 * successes, arrivals and failures until that key changes, without copying
 * any city graph or changing the displayed route. Bounds precede truncation. */
typedef struct { UWORD goal_u,goal_v;UBYTE district,onfoot,x,y; } td_road_route_key_t;
static td_road_route_key_t td_road_route_key;
static td_navigation_waypoint_t td_road_route_waypoint;
static UBYTE td_road_route_valid;
#ifdef __SDCC
typedef char td_guidance_ground_static_budget[(sizeof(td_road_arrow_offset)+
    sizeof(td_road_arrow_tile)+sizeof(td_road_arrow_attr)+sizeof(td_road_arrow_active)==5)?1:-1];
typedef char td_guidance_route_static_budget[(sizeof(td_road_route_key)+
    sizeof(td_road_route_waypoint)+sizeof(td_road_route_valid)==14)?1:-1];
#endif
static UBYTE td_guidance_road_plan(UBYTE district,UWORD goal_u,UWORD goal_v){
    UBYTE x,y;
    if(td.u>=1024u*16||td.v>=976u*16)return FALSE;
    x=td.u>>7;y=td.v>>7;
    if(!td_road_route_valid||td_road_route_key.goal_u!=goal_u||td_road_route_key.goal_v!=goal_v||
       td_road_route_key.district!=district||td_road_route_key.onfoot!=td.onfoot||
       td_road_route_key.x!=x||td_road_route_key.y!=y){
        td_road_route_key.goal_u=goal_u;td_road_route_key.goal_v=goal_v;
        td_road_route_key.district=district;td_road_route_key.onfoot=td.onfoot;
        td_road_route_key.x=x;td_road_route_key.y=y;td_road_route_valid=1;
        td_road_route_waypoint.direction=0;
        td_navigation_next(district,td.onfoot,goal_u,goal_v,td.u>>4,td.v>>4,&td_road_route_waypoint);
    }
    return td_road_route_waypoint.direction>=1&&td_road_route_waypoint.direction<=4;
}
static UWORD td_guidance_road_offset(UBYTE *map){
    return (((UWORD)((td_road_route_waypoint.v>>3)&31)<<5)|
        ((td_road_route_waypoint.u>>3)&31))|(map==(UBYTE*)0x9c00?1024:0);
}
static UBYTE td_guidance_road_owned(UWORD offset){
    UBYTE tile,attr,bank=VBK_REG&1;UBYTE *map;
    if(!td_road_arrow_active||td_road_arrow_offset!=offset)return FALSE;
    map=offset&1024?(UBYTE*)0x9c00:(UBYTE*)0x9800;offset&=1023;
    VBK_REG=0;tile=get_vram_byte(map+offset);VBK_REG=1;attr=get_vram_byte(map+offset);
    VBK_REG=bank;
    return tile==240+td_road_route_waypoint.direction&&attr==15;
}
static void td_guidance_road_restore_local(void){
    UBYTE tile,attr,bank=VBK_REG&1;UBYTE *map;UWORD offset;
    if(!td_road_arrow_active)return;
    /* Offsetbit10 records the original LCDC BKG page; switching a menu's
     * map page cannot restore this ground patch into another tilemap. */
    map=td_road_arrow_offset&1024?(UBYTE*)0x9c00:(UBYTE*)0x9800;
    offset=td_road_arrow_offset&1023;VBK_REG=0;tile=get_vram_byte(map+offset);
    VBK_REG=1;attr=get_vram_byte(map+offset);
    /* Scroll, menus and later overlays win if either ownership byte changed. */
    if(tile>=241&&tile<=244&&attr==15){
        set_vram_byte(map+offset,td_road_arrow_attr);
        VBK_REG=0;set_vram_byte(map+offset,td_road_arrow_tile);
    }
    td_road_arrow_active=0;VBK_REG=bank;
}
#endif
/* Original eight-pixel arrows and accents occupy unused tiles after the font.
 * The paused atlas owns only16..187, so normal guidance survives map visits. */
static const UBYTE td_arrows[]={
    0x18,0x18,0x3c,0x3c,0x7e,0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0,0,0,0,
    0,0,0x08,0x08,0x0c,0x0c,0x7e,0x7e,0x0c,0x0c,0x08,0x08,0,0,0,0,
    0x18,0x18,0x18,0x18,0x18,0x18,0x7e,0x7e,0x3c,0x3c,0x18,0x18,0,0,0,0,
    0,0,0x10,0x10,0x30,0x30,0x7e,0x7e,0x30,0x30,0x10,0x10,0,0,0,0,
    0,0,0x3e,0x3e,0x06,0x06,0x0a,0x0a,0x12,0x12,0x20,0x20,0,0,0,0,
    0,0,0x20,0x20,0x12,0x12,0x0a,0x0a,0x06,0x06,0x3e,0x3e,0,0,0,0,
    0,0,0x04,0x04,0x48,0x48,0x50,0x50,0x60,0x60,0x7c,0x7c,0,0,0,0,
    0,0,0x7c,0x7c,0x60,0x60,0x50,0x50,0x48,0x48,0x04,0x04,0,0,0,0,
    0,0x10,0,0x38,0,0xfe,0,0x7c,0,0x38,0,0x6c,0,0x44,0,0,
    0x10,0,0x28,0,0xc6,0,0x44,0,0x28,0,0x54,0,0x44,0,0,0,
    0xff,0,0,0,0xff,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0x10,0,0x18,0,0x1c,0,0x18,0,0x10,0,0,0,0
};
void td_guidance_init(void) BANKED {set_bkg_data(241,sizeof(td_arrows)/16,td_arrows);}
void td_guidance_label(char *dest,const char *label) BANKED {
    WORD du,dv;UWORD au,av;UBYTE n=0;
    if(actors[1].flags&ACTOR_FLAG_HIDDEN){while(n<20&&label[n])n++;memmove(dest,label,n);dest[n]=0;return;}
    du=(WORD)(actors[1].pos.x>>5)-(WORD)(td.u>>4);
    dv=(WORD)(actors[1].pos.y>>5)+12-(WORD)(td.v>>4);
    au=du<0?-du:du;av=dv<0?-dv:dv;
    if(!strcmp(label,"RETURN FERRY AT DOCK"))label="RETURN FERRY DOCK";
    else if(!strcmp(label,"GO TO FERRY TERMINAL"))label="FERRY TERMINAL";
    while(n<18&&label[n])n++;
    memmove(dest+2,label,n);dest[n+2]=0;dest[1]=' ';
    dest[0]=au<12&&av<12?'+':au&&av&&au<av*2&&av<au*2?
        (du>0?(dv>0?6:5):(dv>0?7:8)):au>av?(du>0?2:4):(dv>0?3:1);
}
void td_guidance_compact(char *dest,UBYTE guided) BANKED {
    UWORD value;UBYTE i,n=0;
    while(n<11&&dest[n])n++;
    for(i=n;i<11;i++)dest[i]=' ';
    if(guided){
        value=td.left>999?999:td.left;
        dest[11]=' ';dest[12]=value>=100?'0'+value/100:' ';
        dest[13]=value>=10?'0'+value/10%10:' ';dest[14]='0'+value%10;
        dest[15]='S';dest[16]=' ';
    }else{
        value=td.cash;dest[11]='$';
        for(i=0;i<5;i++){dest[16-i]='0'+value%10;value/=10;}
        for(i=12;i<16&&dest[i]=='0';i++)dest[i]=' ';
    }
    for(i=0;i<3;i++)dest[17+i]=i<td.wanted&&
        !(td.wanted_left<30&&(td.seconds&1))?9:10;
    dest[20]=0;
}

void td_guidance_road_reset(void) BANKED {
#ifdef CGB
    td_road_arrow_active=0;td_road_route_valid=0;
#endif
}

void td_guidance_road_restore(void) BANKED {
#ifdef CGB
    td_guidance_road_restore_local();
#endif
}

void td_guidance_road_prepare(void) BANKED {
#ifdef CGB
    UBYTE district;
    if(!td_road_arrow_active)return;
    /* Retention uses the existing exact cache only. It never searches ROM,
     * updates route advice, or keeps an arrow across a changed job/goal/cell. */
    if(td.mode==TD_ROAM&&td.job!=TD_NONE&&td_road_arrow_active==td.job+1&&
       !(actors[1].flags&ACTOR_FLAG_HIDDEN)&&!td_boats_controlled()&&
       td_road_route_valid&&td.u<1024u*16&&td.v<976u*16&&
       td_road_route_key.goal_u==(actors[1].pos.x>>5)&&
       td_road_route_key.goal_v==(actors[1].pos.y>>5)+12&&
       td_road_route_key.onfoot==td.onfoot&&td_road_route_key.x==(td.u>>7)&&
       td_road_route_key.y==(td.v>>7)&&td_road_route_waypoint.direction>=1&&
       td_road_route_waypoint.direction<=4){
        district=td_district_current();
        if(district==td.district&&td_road_route_key.district==district&&
           td_guidance_road_owned(td_guidance_road_offset(GetBkgAddr())))return;
    }
    td_guidance_road_restore_local();
#endif
}

void td_guidance_road_render(void) BANKED {
#ifdef CGB
    UWORD goal_u,goal_v,offset;UBYTE *map;UBYTE attr,bank,district;
    if(td.mode!=TD_ROAM||td.job==TD_NONE||(actors[1].flags&ACTOR_FLAG_HIDDEN)||td_boats_controlled()){
        td_guidance_road_restore_local();return;}
    district=td_district_current();if(district!=td.district){td_guidance_road_restore_local();return;}
    goal_u=actors[1].pos.x>>5;goal_v=(actors[1].pos.y>>5)+12;
    if(!td_guidance_road_plan(district,goal_u,goal_v)){td_guidance_road_restore_local();return;}
    /* The arrow sits on a collision-validated ground tile ahead of the car,
     * without modifying its stop marker or hiding a roof, sign, light or prop. */
    if((WORD)td_road_route_waypoint.u<draw_scroll_x+4||(WORD)td_road_route_waypoint.u>draw_scroll_x+155||
       (WORD)td_road_route_waypoint.v<draw_scroll_y+4||(WORD)td_road_route_waypoint.v>draw_scroll_y+123){
        td_guidance_road_restore_local();return;}
    map=GetBkgAddr();offset=td_guidance_road_offset(map);
    /* Preserve the original underlay when this complete arrow is unchanged.
     * Never recapture our bank1 arrow glyph as its own restoration target. */
    if(td_road_arrow_active==td.job+1&&td_guidance_road_owned(offset))return;
    td_guidance_road_restore_local();bank=VBK_REG&1;offset&=1023;
    VBK_REG=1;attr=get_vram_byte(map+offset);
    /* Dynamic bank1 overlays and native raised canopy ownership win. */
    if(attr&136){VBK_REG=bank;return;}
    td_road_arrow_offset=offset|(map==(UBYTE*)0x9c00?1024:0);td_road_arrow_attr=attr;
    VBK_REG=0;td_road_arrow_tile=get_vram_byte(map+offset);
    set_vram_byte(map+offset,240+td_road_route_waypoint.direction);
    VBK_REG=1;set_vram_byte(map+offset,15);td_road_arrow_active=td.job+1;
    VBK_REG=bank;
#endif
}
