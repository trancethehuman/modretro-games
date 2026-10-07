#pragma bank 255
#include <string.h>
#include <stdarg.h>
#include "td_game.h"
#include "td_transit.h"
#include "td_font.h"
#define TD_UI_ART_DATA
#include "td_ui_art.h"
#include "td_audio.h"
#include "td_atlas.h"
#include "td_life.h"
#include "td_daynight.h"
#include "td_hud.h"
#include "td_radio_data.h"
#include "actor.h"
#include "data_manager.h"
#include "ui.h"
#include "camera.h"
#include "scroll.h"
#include "system.h"

/* One 40-byte cell buffer: window tiles 0..19, CGB attributes 20..39. */
static UBYTE td_cells[40];
#define td_tiles td_cells
#define td_attrs (td_cells+20)
/* The atlas is paused and owns this cache until the text UI is repainted. */
static union { UBYTE rows[18][20]; UWORD patterns[TD_ATLAS_VISIBLE_LIMIT]; } td_ui_cache;
#define td_cached_rows td_ui_cache.rows
typedef char td_atlas_cache_fits_existing_wram[(sizeof(td_ui_cache)==360)?1:-1];
typedef char td_atlas_tiles_leave_font[(16+TD_ATLAS_VISIBLE_LIMIT<=192)?1:-1];
typedef char td_atlas_hash_capacity[(TD_ATLAS_VISIBLE_LIMIT==172)?1:-1];
static UBYTE td_map_x,td_map_y,td_map_row,td_map_count,td_map_focus,td_map_active,td_map_error;
static UBYTE td_map_camera_settings,td_map_actor_count,td_map_hidden[TD_ACTORS];
static UWORD td_map_camera_x,td_map_camera_y;
static UBYTE td_ui_mode=255;
/* First window row of the HUD: 0, or 3 below a radio card. */
static UBYTE td_hud_y;
static UBYTE td_hud_rows;
UWORD td_last_pay;
static char td_line[40];
static const char *td_vehicles[]={"CAR","TRUCK","MOTORCYCLE","SCOOTER"};
static const char *td_kinds[]={"PARCEL ROUND","FRAGILE: NO CRASH","EXPRESS DEADLINE","TRUCK FREIGHT","TRANSIT FRIENDLY","PASSENGER: SMOOTH","RETURN DOCUMENTS","ISLAND FERRY POST"};
/* sprintf subset for HUD text (%u, %0Nu, %s, %c). SDCC's sprintf divides by
 * ten through a slow runtime helper per digit; constant subtraction, with
 * byte arithmetic for the final two digits, keeps HUD refreshes cheap. */
static char *td_digits(char *d,UWORD v,UBYTE width){
    UBYTE low,digit,started=0;
    if(v>=10000||width>=5){digit='0';while(v>=10000){v-=10000;digit++;}*d++=digit;started=1;}
    if(started||v>=1000||width>=4){digit='0';while(v>=1000){v-=1000;digit++;}*d++=digit;started=1;}
    if(started||v>=100||width>=3){digit='0';while(v>=100){v-=100;digit++;}*d++=digit;started=1;}
    low=(UBYTE)v;
    if(started||low>=10||width>=2){digit='0';while(low>=10){low-=10;digit++;}*d++=digit;}
    *d++='0'+low;
    return d;
}
static void td_format(char *d,const char *f,...){
    va_list ap;UBYTE width;const char *s;
    va_start(ap,f);
    while(*f){
        if(*f!='%'){*d++=*f++;continue;}
        f++;width=0;
        if(*f=='0'){f++;width=*f++-'0';}
        switch(*f++){
            case 'u':d=td_digits(d,va_arg(ap,unsigned int),width);break;
            case 's':s=va_arg(ap,const char *);while(*s)*d++=*s++;break;
            case 'c':*d++=(char)va_arg(ap,int);break;
        }
    }
    *d=0;va_end(ap);
}
/* Rows are cached as source characters (art codes 0x80.. included), so an
 * unchanged row costs one compare; changed rows write CGB attributes (bank,
 * palette, flips) and then tile indices. The atlas map draws below its text
 * rows and bypasses the cache. Copy and conversion run in assembly with the
 * C loops below as their reference; globals let the emulator checker compare. */
char td_row_buf[20];
const char *td_row_src;
#ifdef __SDCC
void td_row_copy(void) NAKED {
    __asm
        ld hl, #_td_row_src
        ld a, (hl+)
        ld h, (hl)
        ld l, a
        ld de, #_td_row_buf
        ld b, #20
    1$:
        ld a, (hl)
        or a, a
        jr z, 3$
        inc hl
        ld (de), a
        inc de
        dec b
        jr nz, 1$
        ret
    3$:
        ld (de), a
        inc de
        dec b
        jr nz, 3$
        ret
    __endasm;
}
void td_row_convert(void) NAKED {
    __asm
        ld de, #_td_row_buf
        ld hl, #_td_cells
        ld b, #20
    1$:
        ld a, (de)
        inc de
        sub a, #32
        jr nc, 2$
        xor a, a
    2$:
        push de
        push hl
        ld e, a
        ld d, #0
        ld hl, #_td_glyph_tile
        add hl, de
        ld c, (hl)
        ld hl, #_td_glyph_attr
        add hl, de
        ld a, (hl)
        pop hl
        ld (hl), c
        ld de, #20
        add hl, de
        ld (hl), a
        ld de, #-19
        add hl, de
        pop de
        dec b
        jr nz, 1$
        ret
    __endasm;
}
#else
static void td_row_copy(void){UBYTE i;const char *s=td_row_src;for(i=0;i<20;i++){td_row_buf[i]=*s;if(*s)s++;}}
static void td_row_convert(void){
    UBYTE i,c;
    for(i=0;i<20;i++){c=(UBYTE)td_row_buf[i];c=c>=32?c-32:0;td_tiles[i]=td_glyph_tile[c];td_attrs[i]=td_glyph_attr[c];}
}
#endif
static void td_row(UBYTE y,const char *s) {
    td_row_src=s;td_row_copy();
    if(td.mode!=TD_MAP){
        if(!memcmp(td_cached_rows[y],td_row_buf,20))return;
        memcpy(td_cached_rows[y],td_row_buf,20);
    }
    td_row_convert();
    VBK_REG=1;set_win_tiles(0,y,20,1,td_attrs);
    VBK_REG=0;set_win_tiles(0,y,20,1,td_tiles);
}
/* Framed rows: the frame's sides around up to eighteen characters. */
#define TD_T3 TD_UI_FRAME_T TD_UI_FRAME_T TD_UI_FRAME_T
#define TD_T18 TD_T3 TD_T3 TD_T3 TD_T3 TD_T3 TD_T3
#define TD_B3 TD_UI_FRAME_B TD_UI_FRAME_B TD_UI_FRAME_B
#define TD_B18 TD_B3 TD_B3 TD_B3 TD_B3 TD_B3 TD_B3
#define TD_R3 TD_UI_RULE TD_UI_RULE TD_UI_RULE
#define TD_R18 TD_R3 TD_R3 TD_R3 TD_R3 TD_R3 TD_R3
static const char td_frame_top[]=TD_UI_FRAME_TL TD_T18 TD_UI_FRAME_TR;
static const char td_frame_bottom[]=TD_UI_FRAME_BL TD_B18 TD_UI_FRAME_BR;
static const char td_frame_join[]=TD_UI_JOIN_L TD_R18 TD_UI_JOIN_R;
static char td_frame_buf[21];
static void td_framed(UBYTE y,const char *s){
    UBYTE i;
    td_frame_buf[0]=TD_UI_FRAME_L[0];
    for(i=1;i<19;i++){td_frame_buf[i]=*s?*s:' ';if(*s)s++;}
    td_frame_buf[19]=TD_UI_FRAME_R[0];td_frame_buf[20]=0;
    td_row(y,td_frame_buf);
}
static const td_stop_t *td_map_destination(void){
    return td.job==TD_NONE&&(td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)?&td_cursor:&td_target;
}
static UBYTE td_map_point(UBYTE focus,UWORD *x,UWORD *y){
    const td_stop_t *target;
    if(!focus)return td_atlas_position(td.district,td.u>>4,td.v>>4,x,y);
    if(focus==1)return td_atlas_position(td.onfoot?td.park_district:td.district,
        (td.onfoot?td.park_u:td.u)>>4,(td.onfoot?td.park_v:td.v)>>4,x,y);
    target=td_map_destination();return td_atlas_position(target->district,target->u,target->v,x,y);
}
static void td_map_centre(void){
    UWORD x,y;
    if(!td_map_point(td_map_focus,&x,&y))return;
    x>>=3;y>>=3;
    td_map_x=x>10?x-10:0;td_map_y=y>6?y-6:0;
    if(td_map_x>TD_ATLAS_TILE_WIDTH-20)td_map_x=TD_ATLAS_TILE_WIDTH-20;
    if(td_map_y>TD_ATLAS_TILE_HEIGHT-12)td_map_y=TD_ATLAS_TILE_HEIGHT-12;
}
static void td_map_headers(void){
    const td_stop_t *target=td_map_destination();UBYTE trip=target==&td_cursor;
    td_row(0," TORONTO CITY MAP");
    if(td_atlas_district((td_map_x+10)*8,(td_map_y+6)*8,td_line))td_row(1,td_line);
    else td_row(1,"CITY EDGE");
    td_row(14,trip?"P YOU C CAR O STOP":td.job==TD_NONE?"P YOU C CAR O DEPOT":"P YOU C CAR O JOB");
    if(!td_map_focus)td_row(15,td.onfoot?"YOU ON FOOT":"YOU DRIVING");
    else if(td_map_focus==1)td_row(15,td.onfoot?"YOUR PARKED VEHICLE":"YOUR DRIVING VEHICLE");
    else{td_format(td_line,"%s%s",trip?"TRIP: ":td.job==TD_NONE?"DEPOT: ":"JOB: ",target->name);td_row(15,td_line);}
    td_row(16,"DPAD PAN SELECT VIEW");
    td_row(17,td_map_error?"MAP ERROR / B BACK":td_map_row<12?"DRAWING MAP / B BACK":trip?"A STOP / B BACK":td.job==TD_NONE?"A DEPOT / B BACK":"A JOB / B BACK");
}
static void td_map_begin(void){
    UBYTE i;
    td_map_count=td_map_row=td_map_error=0;
    memset(td_ui_cache.patterns,255,sizeof(td_ui_cache.patterns));
    for(i=2;i<14;i++)td_row(i,"");
}
static void td_map_markers(void){
    UBYTE bits,glyph,ticks;
    /* A full glyph identifies the main marker; bottom ticks retain overlaps. */
    VBK_REG=1;
    for(bits=1;bits<8;bits++){
        glyph=td_glyph_tile[(bits&4?'O':bits&1?'P':'C')-32]-TD_FONT_FIRST;
        memcpy(td_line,td_font+((UWORD)glyph<<4),16);
        ticks=(bits&1?128:0)|(bits&2?24:0)|(bits&4?1:0);
        td_line[14]=td_line[15]=ticks;
        set_bkg_data(7+bits,1,(UBYTE *)td_line);
    }
    VBK_REG=0;
}
static void td_map_paint_row(void){
    UWORD patterns[20],x,y,hash;UBYTE i,slot,stride,probes,bits,focus,valid=0,mark_x[3],mark_y[3],row=td_map_y+td_map_row;
    if(!td_atlas_row(td_map_x,row,20,patterns)){td_map_error=1;td_map_row=12;td_map_headers();return;}
    for(focus=0;focus<3;focus++)if(td_map_point(focus,&x,&y)){
        mark_x[focus]=x>>3;mark_y[focus]=y>>3;valid|=1<<focus;
    }
    for(i=0;i<20;i++){
        bits=0;
        for(focus=0;focus<3;focus++)if((valid&(1<<focus))&&
            mark_x[focus]==td_map_x+i&&mark_y[focus]==row)bits|=1<<focus;
        if(bits){td_tiles[i]=7+bits;continue;}
        /* Odd strides 1..31 are coprime to 172: every slot is reachable.
         * The largest slot+stride is 202, fitting the eight-bit counter. */
        hash=patterns[i];while(hash>=TD_ATLAS_VISIBLE_LIMIT)hash-=TD_ATLAS_VISIBLE_LIMIT;
        slot=hash;stride=1+((patterns[i]&15)<<1);
        for(probes=0;probes<TD_ATLAS_VISIBLE_LIMIT;probes++){
            if(td_ui_cache.patterns[slot]==patterns[i]||td_ui_cache.patterns[slot]==65535)break;
            slot+=stride;if(slot>=TD_ATLAS_VISIBLE_LIMIT)slot-=TD_ATLAS_VISIBLE_LIMIT;
        }
        if(probes==TD_ATLAS_VISIBLE_LIMIT){td_map_error=1;td_map_row=12;td_map_headers();return;}
        if(td_ui_cache.patterns[slot]==65535){
            if(td_map_count>=TD_ATLAS_VISIBLE_LIMIT||!td_atlas_pattern(patterns[i],(UBYTE *)td_line)){
                td_map_error=1;td_map_row=12;td_map_headers();return;
            }
            td_ui_cache.patterns[slot]=patterns[i];td_map_count++;
            VBK_REG=1;set_bkg_data(16+slot,1,(UBYTE *)td_line);
        }
        td_tiles[i]=16+slot;
    }
    VBK_REG=0;set_win_tiles(0,2+td_map_row,20,1,td_tiles);
    if(++td_map_row==12)td_map_headers();
}
void td_map_open(void) BANKED {
    UWORD width,height;UBYTE i;
    if(td_map_active)return;
    td_map_camera_x=camera_x;td_map_camera_y=camera_y;td_map_camera_settings=camera_settings;
    camera_settings=0;td_map_focus=0;td_map_active=1;
    td_map_actor_count=actors_len<TD_ACTORS?actors_len:TD_ACTORS;
    for(i=0;i<td_map_actor_count;i++){
        td_map_hidden[i]=actors[i].flags&ACTOR_FLAG_HIDDEN;actors[i].flags|=ACTOR_FLAG_HIDDEN;
    }
    if(!td_atlas_bounds(&width,&height)||width!=TD_ATLAS_WIDTH_PIXELS||height!=TD_ATLAS_HEIGHT_PIXELS){
        td_map_error=1;td_map_row=12;return;
    }
    td_map_centre();td_map_markers();td_map_begin();
    /* The atlas was drawn for the original UI colours. */
    set_bkg_palette(7,1,td_map_palette);
}
void td_map_update(UBYTE buttons,UBYTE pressed) BANKED {
    UBYTE x=td_map_x,y=td_map_y,changed=0;
    if(!td_map_active)return;
    if(pressed&J_SELECT){td_map_focus=td.onfoot?(td_map_focus+1)%3:td_map_focus==2?0:2;td_map_centre();changed=1;}
    else if(pressed&J_A){td_map_focus=2;td_map_centre();changed=1;}
    else if(td_map_row==12&&!td_map_error){
        if((buttons&(J_LEFT|J_RIGHT))==J_LEFT)td_map_x=x>2?x-2:0;
        else if((buttons&(J_LEFT|J_RIGHT))==J_RIGHT)td_map_x=x+2<TD_ATLAS_TILE_WIDTH-20?x+2:TD_ATLAS_TILE_WIDTH-20;
        if((buttons&(J_UP|J_DOWN))==J_UP)td_map_y=y?y-1:0;
        else if((buttons&(J_UP|J_DOWN))==J_DOWN)td_map_y=y<TD_ATLAS_TILE_HEIGHT-12?y+1:y;
        changed=x!=td_map_x||y!=td_map_y;
    }
    if(changed){td_map_begin();td_map_headers();}
    if(td_map_row<12)td_map_paint_row();
}
#ifdef __SDCC
#include "palette.h"
void load_bkg_tileset(const tileset_t *tiles,UBYTE bank) BANKED;
/* The atlas borrows CGB bank-1 background tiles 8..187. Scenes whose own
 * background spills into bank 1 (the core uses IDs 0..9) must get those
 * patterns back before the city is visible again. */
static void td_restore_scene_tiles(void){
    scene_t scene;background_t bkg;
    MemcpyBanked(&scene,current_scene.ptr,sizeof(scene),current_scene.bank);
    MemcpyBanked(&bkg,scene.background.ptr,sizeof(bkg),scene.background.bank);
    if(bkg.cgb_tileset.ptr){VBK_REG=1;load_bkg_tileset(bkg.cgb_tileset.ptr,bkg.cgb_tileset.bank);VBK_REG=0;}
}
/* The title swaps in its own BG palettes 0..6; the scene's come back as
 * the current time of day's set. The title only appears while a scene
 * initializes, before its fade-in, which applies BkgPalette: writing the
 * hardware palettes here would flash full colour for a frame. Leaving the
 * title sets them. */
static void td_title_palettes_on(void){
    memcpy(BkgPalette,td_title_palettes,sizeof(td_title_palettes));
}
static void td_restore_scene_palettes(void){td_daynight_apply(TD_DN_FORCE|TD_DN_HW);}
typedef char td_title_palettes_fill_seven_slots[(sizeof(td_title_palettes)==7*sizeof(palette_entry_t))?1:-1];
#else
static void td_restore_scene_tiles(void){}
static void td_title_palettes_on(void){}
static void td_restore_scene_palettes(void){}
#endif
/* The title illustration borrows the atlas-owned bank-1 tiles and BG
 * palettes 0..6 while the fullscreen title hides the city; leaving it
 * restores the scene's tiles and palettes. A scene load replaces both, so
 * td_ui_init forgets that the tiles were loaded. */
static UBYTE td_title_shown;
static void td_title_show(void){
    UBYTE y;
    if(!td_title_shown){VBK_REG=1;set_bkg_data(TD_TITLE_FIRST,TD_TITLE_TILES,td_title_tiles);td_title_shown=1;}
    td_title_palettes_on();
    for(y=0;y<TD_TITLE_ROWS;y++){
        VBK_REG=1;set_win_tiles(0,y,20,1,td_title_attr+(UWORD)y*20);
        VBK_REG=0;set_win_tiles(0,y,20,1,td_title_map+(UWORD)y*20);
        memset(td_cached_rows[y],255,20);
    }
}
static void td_title_hide(void){
    if(!td_title_shown)return;
    td_title_shown=0;td_restore_scene_tiles();td_restore_scene_palettes();VBK_REG=0;
}
void td_map_close(void) BANKED {
    UBYTE i;
    if(!td_map_active)return;
    td_restore_scene_tiles();set_bkg_palette(7,1,td_ui_palette);
    for(i=0;i<td_map_actor_count;i++)actors[i].flags=(actors[i].flags&~ACTOR_FLAG_HIDDEN)|td_map_hidden[i];
    camera_x=td_map_camera_x;camera_y=td_map_camera_y;camera_settings=td_map_camera_settings;
    td_map_active=0;memset(td_cached_rows,255,sizeof(td_cached_rows));VBK_REG=0;
}
void td_ui_init(void) BANKED {
    UBYTE i;td_ui_mode=255;td_map_active=0;td_hud_y=0;td_title_shown=0;memset(td_cached_rows,255,sizeof(td_cached_rows));memset(td_attrs,15,20);
    /* A fresh scene reports its objective once and starts with no pop-ups. */
    td_hud_rows=0;td_hud_reset();
    /* Font in bank 1 from tile 192, art in bank 0 tiles 128..191 (above the
     * scene's 128 bank-0 tiles and outside the sprite tiles). Every window
     * cell starts as a bank-1 font cell on UI palette 7. */
    VBK_REG=1;for(i=0;i<18;i++)set_win_tiles(0,i,20,1,td_attrs);
    VBK_REG=1;set_bkg_data(TD_FONT_FIRST,sizeof(td_font)/16,td_font);
    VBK_REG=0;set_bkg_data(TD_UI_ART_FIRST,TD_UI_ART_TILES,td_ui_art);
    text_drawn=TRUE;td_ui_draw();
}
static const char *const td_vehicle_short[]={"CAR","TRUCK","MOTO","SCOOTER"};
/* Pause menu: nine actions, four on screen at a time with scroll marks, and
 * a one-line hint for the highlighted action. */
#define TD_MENU_ITEMS 9
#define TD_MENU_ROWS 4
static const char *const td_menu_items[TD_MENU_ITEMS]={
    TD_UI_RESUME " RESUME",TD_UI_MAP " CITY MAP",TD_UI_JOBS " DISPATCH JOBS",TD_UI_PARK " PARK AND WALK",
    TD_UI_CAR " CHANGE VEHICLE",TD_UI_TRANSIT " TTC TIMETABLE",TD_UI_MEDIC " SUPPLIES $20",TD_UI_CANCEL " CANCEL JOB",
    TD_UI_AUDIO " SOUND"};
static const char *const td_menu_hints[TD_MENU_ITEMS]={
    "BACK TO THE CITY","SEE ROUTE AND CITY","PICK A DELIVERY","STEP OUT ON FOOT",
    "PARKED, NO JOB","AT A STOP, ON FOOT","HEAL, AMMO, REPAIR","DROP THE JOB","A: CHANGE SOUND"};
static const char *const td_audio_names[]={"MUSIC+FX","FX ONLY","SILENT"};
static UBYTE td_ui_blink,td_menu_top;
static const char *td_service_icon(UBYTE service){
    return service==TD_TRANSIT_TRAIN?TD_UI_SUBWAY:service==TD_TRANSIT_BUS?TD_UI_BUS:service==TD_TRANSIT_FERRY?TD_UI_FERRY:TD_UI_TRANSIT;
}
/* Writes n characters into window row y from column x, keeping the row
 * cache in step. */
static void td_put(UBYTE x,UBYTE y,const char *s,UBYTE n){
    UBYTE i,c;
    for(i=0;i<n;i++){c=(UBYTE)s[i];td_cached_rows[y][x+i]=c;c=c>=32?c-32:0;td_tiles[i]=td_glyph_tile[c];td_attrs[i]=td_glyph_attr[c];}
    VBK_REG=1;set_win_tiles(x,y,n,1,td_attrs);
    VBK_REG=0;set_win_tiles(x,y,n,1,td_tiles);
}
/* Distance to the objective as four characters: tens of metres at about
 * 4 m per pixel ("320M"), kilometres from 1 km ("1.2K"); blank without a
 * reachable objective. */
static void td_distance_text(char *d){
    UWORD ax,ay,m,pu=td.u>>4,pv=td.v>>4;char t[6],*e;UBYTE n,i;
    d[0]=d[1]=d[2]=d[3]=' ';
    if(!td_beacon_shown)return;
    ax=td_beacon_u>pu?td_beacon_u-pu:pu-td_beacon_u;ay=td_beacon_v>pv?td_beacon_v-pv:pv-td_beacon_v;
    m=(ax>ay?ax+((ay*3)>>3):ay+((ax*3)>>3))<<2;
    if(m<1000){e=td_digits(t,m/10*10,0);*e++='M';}
    else{t[0]='0'+(UBYTE)(m/1000);t[1]='.';t[2]='0'+(UBYTE)((m%1000)/100);t[3]='K';e=t+4;}
    n=(UBYTE)(e-t);for(i=0;i<n;i++)d[4-n+i]=t[i];
}
/* Menus animate their cursor (and the title its prompt) every 16 frames. */
void td_ui_tick(void) BANKED {
    UBYTE blink=(sys_time>>4)&1;
    if(blink==td_ui_blink)return;
    td_ui_blink=blink;
    if(td.mode==TD_PAUSE||td.mode==TD_HELP||td.mode==TD_BOARD||td.mode==TD_TRANSIT||td.mode==TD_RESULT)td_ui_draw();
}
static const char *const td_messages[]={"","STOP TO INTERACT","WRONG VEHICLE","JOB IS LOCKED","NO FARE MONEY","CRASH: CARGO HURT","STOP AT THE BEACON","RED SIGNAL: FINE","HEAVY CARGO: DRIVE","VEHICLE IS PARKED","NO WATER CROSSING","STOP TO PARK","SAVED TO CARTRIDGE","PEDESTRIAN: BRAKE","TURN GENTLY: RIDER","DOOR PATH BLOCKED","PARK THEN WALK",
            "OUT OF AMMO","CAR STOLEN","SHOT: FIND COVER","POLICE LOST YOU","POLICE ALERTED","SUPPLIES BOUGHT","NOT ENOUGH CASH","PEDESTRIAN HIT","HOLD A+B: GET OUT","HOSPITAL",
            "FOUND CASH +$15","FIRST AID +40","AMMO +6","ENGINE SMOKING","CAR WRECKED: REPAIR"};
typedef char td_messages_match[(sizeof(td_messages)/sizeof(td_messages[0])==TD_MSG_COUNT)?1:-1];
/* ------------------------------------------------------------ pop-up HUD
 * Nothing covers the city by default. Up to two rows rise from the bottom
 * edge when there is something to say and sink again afterwards:
 *   row A, one item by priority: a notice, the station within reach, the
 *     next stop after a job starts or advances, a new street name, wanted
 *     stars;
 *   row B, what changed or matters now: the job (time, distance, cargo)
 *     when time is short, the stop has just changed or the courier stands
 *     still; cash, vitality, ammunition and car condition for a few seconds
 *     after they change (ammunition after every shot), vitality while low.
 * Standing still for a moment shows the whole status line. Timers count
 * calls of td_ui_hud_tick (every eighth update, about 7 a second). */
static char *td_cat(char *d,const char *s){while(*s)*d++=*s++;*d=0;return d;}
/* Row A text into td_line; FALSE when there is nothing to say. */
static UBYTE td_hud_row_a(void){
    UBYTE i;
    if(td.msg){strcpy(td_line,td.msg==5&&(td.job==TD_NONE||!td.stage)?"CRASH: BRAKE EARLY":td_messages[td.msg]);return TRUE;}
    if(td_station_near!=TD_NONE){td_line[0]=TD_UI_BTN_B[0];td_transit_label(td_station_near,td_line+1);return TRUE;}
    if(td_pop_target&&td.job!=TD_NONE){
        td_line[0]=TD_UI_PIN[0];
        if(td_target.district!=td.district){
            if(td_route_district!=TD_NONE)td_get_district_name(td_route_district,td_line+1);
            else strcpy(td_line+1,"NO ROAD ROUTE");
        }else strcpy(td_line+1,td_target.name);
        return TRUE;
    }
    if(td_pop_street){strcpy(td_line,td_street_name);return TRUE;}
    if(td.wanted){
        strcpy(td_line,"WANTED ");
        for(i=0;i<td.wanted;i++)td_line[7+i]=TD_UI_STAR[0];
        td_line[7+i]=0;return TRUE;
    }
    return FALSE;
}
/* Row B text into td_line; FALSE when nothing is worth showing. */
static UBYTE td_hud_row_b(void){
    char *d=td_line;UBYTE all=td_still>=10;
    *d=0;
    if(td.job!=TD_NONE&&(td_pop_job||all||td.left<=15)){
        *d++=TD_UI_BOX[0];d=td_digits(d,td.stage+1,0);*d++='/';d=td_digits(d,td_job.count,0);*d++=' ';
        d=td_cat(d,td.left<=10&&(td.left&1)?TD_UI_CLOCK_ALERT:TD_UI_CLOCK);d=td_digits(d,td.left,0);*d++='S';
        if(td_beacon_shown){char t[5],*k=t;td_distance_text(t);t[4]=0;while(*k==' ')k++;if(*k!='0'){*d++=' ';d=td_cat(d,k);}}
        if(td.health<100){*d++=' ';d=td_cat(d,TD_UI_HEART);d=td_digits(d,td.health,0);}
        *d=0;
    }
    if(td_pop_cash||all){if(d!=td_line)*d++=' ';d=td_cat(d,TD_UI_COIN);d=td_digits(d,td.cash,0);}
    if(td_pop_vit||(all&&td.vitality<100)||td.vitality<=30){if(d!=td_line)*d++=' ';d=td_cat(d,TD_UI_MEDIC);d=td_digits(d,td.vitality,0);}
    if(td_pop_ammo||(all&&td.onfoot)){if(d!=td_line)*d++=' ';d=td_cat(d,TD_UI_AMMO);d=td_digits(d,td.ammo,0);}
    if(!td.onfoot&&(td_pop_car||(all&&td_car_damage))){if(d!=td_line)*d++=' ';d=td_cat(d,TD_UI_CAR);d=td_digits(d,TD_DAMAGE_WRECK-td_car_damage,0);}
    *d=0;td_line[20]=0;
    return d!=td_line;
}
static void td_radio_paint(void);
static void td_hud(UBYTE changed){
    UBYTE rows,a,b,top;
    char row_a[21];
    /* A radio call puts its three-row card above the pop-up rows. */
    if(td_radio_script!=TD_NONE){if(!td_hud_y||changed){td_hud_y=3;td_radio_paint();changed=1;}}
    else if(td_hud_y){td_hud_y=0;changed=1;}
    if(td.mode==TD_WAIT||td.mode==TD_RIDE){
        rows=3;if(changed||td_hud_rows!=rows){td_hud_rows=rows;ui_set_pos(0,(UBYTE)(144-((td_hud_y+rows)<<3)));}
        top=td_hud_y;
        if(td.mode==TD_WAIT){
            td_format(td_line,TD_UI_CLOCK "DEPARTS IN %u SEC",td_transit_departure(td.transit_origin,td.transit_target,td.seconds));td_row(top,td_line);
            strcpy(td_line,td_service_icon(td_transit_service(td.transit_origin)));td_transit_label(td.transit_origin,td_line+1);td_row(top+1,td_line);
            td_row(top+2,TD_UI_BTN_B "CANCEL WAIT");
        }else{
            td_format(td_line,"%sRIDING %u SEC",td_service_icon(td_transit_service(td.transit_origin)),td.ride_left);td_row(top,td_line);
            td_line[0]=TD_UI_PIN[0];strcpy(td_line+1,td_cursor.name);td_row(top+1,td_line);
            td_row(top+2,TD_UI_CHECK "FARE PAID / ON TIME");
        }
        return;
    }
    a=td_hud_row_a();if(a){memcpy(row_a,td_line,20);row_a[20]=0;}
    b=td_hud_row_b();
    rows=a+b;
    if(changed||rows!=td_hud_rows){
        td_hud_rows=rows;
        ui_set_pos(0,(UBYTE)(144-((td_hud_y+rows)<<3)));
    }
    top=td_hud_y;
    if(a)td_row(top++,row_a);
    if(b)td_row(top,td_line);
}
/* td_hud.c decides when the pop-ups change; this repaints them. */
void td_ui_hud_paint(void) BANKED {if(td_ui_mode==td.mode&&td.mode==TD_ROAM)td_hud(0);}
/* The controls card below the title illustration starts at row 11. */
typedef char td_title_leaves_card_rows[(TD_TITLE_ROWS==11)?1:-1];
/* Menus are bottom sheets over the paused city: a frame of `rows` window
 * rows anchored to the bottom of the screen (rows below it are off screen). */
static void td_sheet(UBYTE rows){
    ui_set_pos(0,(UBYTE)(144-(rows<<3)));
    td_row(0,td_frame_top);td_row(rows-1,td_frame_bottom);
}
/* Pads td_line with spaces to n characters. */
static void td_pad(UBYTE n){
    UBYTE i;
    for(i=0;i<n&&td_line[i];i++);
    for(;i<n;i++)td_line[i]=' ';
    td_line[n]=0;
}
static void td_pause_sheet(UBYTE changed){
    UBYTE i,n;UWORD minutes;const char *hint;
    td_sheet(11);
    /* Status: cash, unique deliveries and the clock with a sun or moon. */
    minutes=td_daynight_minutes();
    td_format(td_line,TD_UI_COIN "%u " TD_UI_BOX "%u %s%02u:%02u",td.cash,td.done,
              minutes>=7*60&&minutes<19*60?TD_UI_SUN:TD_UI_MOON,minutes/60,minutes%60);
    td_framed(1,td_line);td_row(2,td_frame_join);
    /* Four actions at a time; the window follows the cursor. */
    if(changed)td_menu_top=td.menu>=TD_MENU_ROWS?td.menu-(TD_MENU_ROWS-1):0;
    if(td.menu<td_menu_top)td_menu_top=td.menu;
    else if(td.menu>=td_menu_top+TD_MENU_ROWS)td_menu_top=td.menu-(TD_MENU_ROWS-1);
    for(i=0;i<TD_MENU_ROWS;i++){
        n=td_menu_top+i;
        td_line[0]=td.menu==n?(td_ui_blink?TD_UI_CURSOR_ALT[0]:TD_UI_CURSOR[0]):' ';
        strcpy(td_line+1,n==3&&td.onfoot?TD_UI_PARK " GET IN CAR":td_menu_items[n]);
        if(n==8){strcat(td_line," ");strcat(td_line,td_audio_names[td_audio_get_mode()]);}
        td_pad(17);
        td_line[17]=i==0&&td_menu_top?TD_UI_ARROW_N[0]:i==TD_MENU_ROWS-1&&td_menu_top+TD_MENU_ROWS<TD_MENU_ITEMS?TD_UI_ARROW_S[0]:' ';
        td_line[18]=0;td_framed(3+i,td_line);
    }
    td_row(7,td_frame_join);
    hint=td_menu_hints[td.menu];
    if(td.menu==2&&td.job!=TD_NONE)hint="VIEW YOUR JOB";
    else if(td.menu==3&&td.onfoot)hint="STAND BESIDE IT";
    else if(td.menu==7&&td.job==TD_NONE)hint="NO JOB ACTIVE";
    if(td.menu==4){td_format(td_line,"NOW: %s",td.onfoot?"ON FOOT":td_vehicles[td.vehicle]);td_framed(8,td_line);}
    else td_framed(8,hint);
    td_format(td_line,TD_UI_BTN_A "CHOOSE " TD_UI_BTN_B "BACK  %u/%u",td.menu+1,TD_MENU_ITEMS);td_framed(9,td_line);
}
static void td_board_sheet(void){
    td_sheet(13);
    td_line[0]=TD_UI_JOBS[0];td_chapter_name(td.menu>>3,td_line+1);td_framed(1,td_line);
    td_row(2,td_frame_join);
    td_framed(3,td_offer.title);
    td_get_brief(td.menu,td_line);td_framed(5,td_line+18);td_line[18]=0;td_framed(4,td_line);
    td_format(td_line,TD_UI_BOX "%s",td_kinds[td_offer.kind]);td_framed(6,td_line);
    td_format(td_line,TD_UI_STAR "%u STOPS " TD_UI_CLOCK "%uS",td_offer.count,td_offer.seconds);td_framed(7,td_line);
    td_format(td_line,TD_UI_COIN "PAYS $%u " TD_UI_CAR "%s",td_offer.reward,td_offer.vehicle==TD_NONE?"ANY":td_vehicle_short[td_offer.vehicle]);td_framed(8,td_line);
    td_row(9,td_frame_join);
    if(td.complete[td.menu>>3]&(1<<(td.menu&7)))strcpy(td_line,TD_UI_CHECK "DONE: REPLAY PAYS");
    else if(td.done<td_offer.min_done)td_format(td_line,TD_UI_CANCEL "NEEDS %u DONE",td_offer.min_done);
    else strcpy(td_line,TD_UI_STAR "READY TO TAKE");
    td_framed(10,td_line);
    td_format(td_line,TD_UI_BTN_A "TAKE " TD_UI_BTN_B "BACK " TD_UI_DPAD "%u/%u",td.menu+1,TD_QUESTS);td_framed(11,td_line);
}
static void td_transit_sheet(void){
    UBYTE service=td_transit_service(td.transit_origin),wait;
    td_sheet(10);
    strcpy(td_line,td_service_icon(service));td_transit_label(td.transit_origin,td_line+1);td_framed(1,td_line);
    td_row(2,td_frame_join);
    td_format(td_line,TD_UI_PIN "%s",td_cursor.name);td_framed(3,td_line);
    wait=td_transit_departure(td.transit_origin,td.transit_target,td.seconds);
    td_format(td_line,TD_UI_CLOCK "DEPARTS IN %uS",wait);td_framed(4,td_line);
    td_format(td_line,"RIDE %uS " TD_UI_COIN "$%u%s",td_transit_duration(td.transit_origin,td.transit_target),td_transit_fare(td.transit_origin),
              service==4?(td.transit_target>=td.transit_origin?" EAST":" WEST"):"");td_framed(5,td_line);
    td_row(6,td_frame_join);
    td_framed(7,TD_UI_DPAD "STOP " TD_UI_BTN_A "WAIT " TD_UI_BTN_B "BACK");
    td_framed(8,(td.transit_origin&63)==16?"UP/DOWN: BUS/TRAIN":"SCHEDULES: FICTION");
}
static void td_outcome_sheet(void){
    UBYTE busted=td.mode==TD_BUSTED;
    td_sheet(10);
    td_framed(1,busted?"     " TD_UI_STAR " BUSTED " TD_UI_STAR:"     " TD_UI_MEDIC " WASTED " TD_UI_MEDIC);
    td_row(2,td_frame_join);
    td_format(td_line,busted?TD_UI_COIN "FINE PAID $%u":TD_UI_COIN "HOSPITAL $%u",td_life_fine);td_framed(3,td_line);
    td_framed(4,busted?"POLICE TOOK HALF":"YOU WOKE UP AT");
    td_framed(5,busted?"OF YOUR AMMO":"TORONTO HOSPITAL");
    td_framed(6,TD_UI_STAR "STARS CLEARED");
    td_framed(7,TD_UI_CANCEL "ANY JOB IS LOST");
    td_framed(8,TD_UI_BTN_A "CONTINUE");
}
static void td_result_sheet(void){
    UBYTE ok=td.health&&td.left;
    td_sheet(9);
    td_framed(1,ok?"   " TD_UI_STAR " DELIVERED " TD_UI_STAR:" " TD_UI_CANCEL " CONTRACT FAILED");
    td_row(2,td_frame_join);
    if(ok)td_format(td_line,TD_UI_COIN "+$%u PAID",td_last_pay);else strcpy(td_line,TD_UI_COIN "NO PAY THIS TIME");
    td_framed(3,td_line);
    td_format(td_line,TD_UI_COIN "%u " TD_UI_BOX "%u/%u DONE",td.cash,td.done,TD_QUESTS);td_framed(4,td_line);
    td_framed(5,td.done==TD_QUESTS?TD_UI_STAR "COURIER MASTER":TD_UI_SAVE "AUTO-SAVED");
    td_row(6,td_frame_join);
    td_framed(7,TD_UI_BTN_A "JOBS " TD_UI_BTN_B "FREE ROAM");
}
void td_ui_draw(void) BANKED {
    UBYTE i,changed=td_ui_mode!=td.mode;
    td_ui_mode=td.mode;
    text_drawn=TRUE;
    if(td.mode!=TD_HELP)td_title_hide();
    if(td.mode==TD_MAP){
        ui_set_pos(0,0);if(changed)for(i=0;i<18;i++)td_row(i,"");
        td_map_headers();return;
    }
    if(td.mode==TD_ROAM || td.mode==TD_WAIT || td.mode==TD_RIDE){td_hud(changed);return;}
    /* The HUD's rows are reused by the sheets; it repaints in full later. */
    if(changed)td_hud_y=0;
    if(td.mode==TD_HELP){
        ui_set_pos(0,0);if(changed)for(i=0;i<18;i++)td_row(i,"");
        /* Title: illustration rows 0..10, then a compact controls card. */
        if(changed)td_title_show();
        td_row(11,td_frame_top);
        td_framed(12,TD_UI_BTN_A "GAS " TD_UI_BTN_B "BRAKE " TD_UI_DPAD "STEER");
        td_framed(13,TD_UI_BTN_SEL_0 TD_UI_BTN_SEL_1 "DELIVER " TD_UI_BTN_START_0 TD_UI_BTN_START_1 TD_UI_BTN_START_2 "MENU");
        td_framed(14,TD_UI_BTN_A TD_UI_BTN_B "HOLD: LEAVE CAR");
        td_framed(15,TD_UI_WALK TD_UI_BTN_A "PUNCH " TD_UI_BTN_B "SHOOT/TTC");
        td_framed(16,td_ui_blink?" PRESS " TD_UI_BTN_A " TO START":"");
        td_row(17,td_frame_bottom);return;
    }
    if(td.mode==TD_PAUSE)td_pause_sheet(changed);
    else if(td.mode==TD_BOARD)td_board_sheet();
    else if(td.mode==TD_TRANSIT)td_transit_sheet();
    else if(td.mode==TD_BUSTED||td.mode==TD_WASTED)td_outcome_sheet();
    else if(td.mode==TD_RESULT)td_result_sheet();
}

/* ------------------------------------------------------------ radio calls */
/* The card above the HUD for td_radio.c: Rosa's portrait and name, then
 * the typed part of the current page. */
static void td_radio_paint(void){
    UBYTE r;
    for(r=0;r<3;r++){
        td_line[0]=(char)(TD_UI_PORTRAIT_0[0]+r*3);td_line[1]=td_line[0]+1;td_line[2]=td_line[0]+2;
        if(!r)strcpy(td_line+3,TD_RADIO_SPEAKER);else td_radio_line(r-1,td_line+3);
        td_row(r,td_line);
    }
}
UBYTE td_ui_radio_ready(void) BANKED {
    if(td_ui_mode!=td.mode||(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE))return FALSE;
    if(!td_hud_y){td_ui_draw();return FALSE;}
    return TRUE;
}
void td_ui_radio_put(UBYTE column,UBYTE line,char c) BANKED {td_put(3+column,1+line,&c,1);}
void td_ui_draw_radio(void) BANKED {if(td_hud_y)td_radio_paint();}
