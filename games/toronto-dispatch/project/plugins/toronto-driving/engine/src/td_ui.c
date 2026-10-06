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
    UBYTE i;td_ui_mode=255;td_map_active=0;td_title_shown=0;memset(td_cached_rows,255,sizeof(td_cached_rows));memset(td_attrs,15,20);
    /* Font in bank 1 from tile 192, art in bank 0 tiles 128..191 (above the
     * scene's 128 bank-0 tiles and outside the sprite tiles). Every window
     * cell starts as a bank-1 font cell on UI palette 7. */
    VBK_REG=1;for(i=0;i<18;i++)set_win_tiles(0,i,20,1,td_attrs);
    VBK_REG=1;set_bkg_data(TD_FONT_FIRST,sizeof(td_font)/16,td_font);
    VBK_REG=0;set_bkg_data(TD_UI_ART_FIRST,TD_UI_ART_TILES,td_ui_art);
    text_drawn=TRUE;td_ui_draw();
}
static const char *const td_vehicle_short[]={"CAR","TRUCK","MOTO","SCOOTER"};
static const char *const td_vehicle_icon[]={TD_UI_CAR,TD_UI_CAR,TD_UI_CAR,TD_UI_CAR};
static const char *const td_menu_items[9]={
    TD_UI_RESUME " RESUME",TD_UI_MAP " CITY MAP",TD_UI_JOBS " DISPATCH JOBS",TD_UI_PARK " PARK OR GET CAR",
    TD_UI_CAR " CHANGE VEHICLE",TD_UI_TRANSIT " TTC TIMETABLE",TD_UI_MEDIC " SUPPLIES $20",TD_UI_CANCEL " CANCEL JOB",
    TD_UI_AUDIO};
static const char *const td_audio_names[]={"MUSIC+SFX","SFX ONLY","SILENT"};
static UBYTE td_ui_blink,td_compass;
static const char *td_service_icon(UBYTE service){
    return service==TD_TRANSIT_TRAIN?TD_UI_SUBWAY:service==TD_TRANSIT_BUS?TD_UI_BUS:service==TD_TRANSIT_FERRY?TD_UI_FERRY:TD_UI_TRANSIT;
}
/* Direction from the courier to the objective (td_beacon_u/v), in eight
 * steps; a ring once close, blank without a reachable objective. */
static char td_compass_code(void){
    UWORD bu=td_beacon_u,bv=td_beacon_v,pu=td.u>>4,pv=td.v>>4,ax,ay;UBYTE east,south;
    if(!td_beacon_shown)return ' ';
    east=bu>=pu;south=bv>=pv;ax=east?bu-pu:pu-bu;ay=south?bv-pv:pv-bv;
    if(ax<12&&ay<12)return TD_UI_TARGET[0];
    if(ax>(ay<<1))return east?TD_UI_ARROW_E[0]:TD_UI_ARROW_W[0];
    if(ay>(ax<<1))return south?TD_UI_ARROW_S[0]:TD_UI_ARROW_N[0];
    return south?(east?TD_UI_ARROW_SE[0]:TD_UI_ARROW_SW[0]):(east?TD_UI_ARROW_NE[0]:TD_UI_ARROW_NW[0]);
}
/* Called every few frames while driving or walking: repaints only the
 * compass cell when the direction changes. */
void td_ui_compass(void) BANKED {
    char c;
    if(td.mode!=TD_ROAM||td_ui_mode!=TD_ROAM||td.msg)return;
    c=td_compass_code();
    if(c==td_compass)return;
    td_compass=c;td_cached_rows[0][19]=c;c=(UBYTE)c-32;
    VBK_REG=1;set_win_tiles(19,0,1,1,&td_glyph_attr[(UBYTE)c]);
    VBK_REG=0;set_win_tiles(19,0,1,1,&td_glyph_tile[(UBYTE)c]);
}
/* Menus animate their cursor (and the title its prompt) every 16 frames. */
void td_ui_tick(void) BANKED {
    UBYTE blink=(sys_time>>4)&1;
    if(blink==td_ui_blink)return;
    td_ui_blink=blink;
    if(td.mode==TD_PAUSE||td.mode==TD_HELP||td.mode==TD_BOARD||td.mode==TD_TRANSIT||td.mode==TD_RESULT)td_ui_draw();
}
/* HUD inputs from the previous repaint: row 1's values and the courier
 * position whose street name is cached. A repaint after another screen
 * (changed) always reformats. */
static const char *const td_messages[]={"","STOP TO INTERACT","WRONG VEHICLE","JOB IS LOCKED","NO FARE MONEY","CRASH: CARGO HURT","STOP AT THE BEACON","RED SIGNAL: FINE","HEAVY CARGO: DRIVE","VEHICLE IS PARKED","NO WATER CROSSING","STOP TO PARK","SAVED TO CARTRIDGE","PEDESTRIAN: BRAKE","TURN GENTLY: RIDER","DOOR PATH BLOCKED","PARK THEN WALK",
            "OUT OF AMMO","CAR STOLEN","SHOT: FIND COVER","POLICE LOST YOU","POLICE ALERTED","SUPPLIES BOUGHT","NOT ENOUGH CASH","PEDESTRIAN HIT","HOLD A+B: GET OUT","HOSPITAL",
            "FOUND CASH +$15","FIRST AID +40","AMMO +6"};
typedef char td_messages_match[(sizeof(td_messages)/sizeof(td_messages[0])==TD_MSG_COUNT)?1:-1];
static UWORD td_hud_key[6];
static UWORD td_street_u=65535,td_street_v;static UBYTE td_street_d;
static char td_street_name[20];
static void td_hud(UBYTE changed){
    UBYTE service,i;UWORD key[6],u=td.u>>4,v=td.v>>4;
    ui_set_pos(0,120);
    if(td.mode==TD_WAIT){
        service=td_transit_service(td.transit_origin);
        td_format(td_line,TD_UI_CLOCK "DEPARTS IN %u SEC",td_transit_departure(td.transit_origin,td.transit_target,td.seconds));td_row(0,td_line);
        strcpy(td_line,td_service_icon(service));td_transit_label(td.transit_origin,td_line+1);td_row(1,td_line);
        td_row(2,TD_UI_BTN_B "CANCEL WAIT");td_hud_key[0]=255;return;
    }
    if(td.mode==TD_RIDE){
        td_format(td_line,"%sRIDING %u SEC",td_service_icon(td_transit_service(td.transit_origin)),td.ride_left);td_row(0,td_line);
        td_line[0]=TD_UI_PIN[0];strcpy(td_line+1,td_cursor.name);td_row(1,td_line);
        td_row(2,TD_UI_CHECK "FARE PAID / ON TIME");td_hud_key[0]=255;return;
    }
    if(td.msg){
        td_row(0,td.msg==5&&(td.job==TD_NONE||!td.stage)?"CRASH: BRAKE EARLY":td_messages[td.msg]);
    }else{
        if(td.wanted){
            /* Police attention replaces the street name while it lasts. */
            strcpy(td_line,"WANTED ");
            for(i=0;i<td.wanted;i++)td_line[7+i]=TD_UI_STAR[0];
            td_line[7+i]=0;
        }else if(td.district==0&&u>608&&u<672&&v>496&&v<560)strcpy(td_line,td.seconds%12<7?"YONGE: E/W GREEN":"YONGE: N/S GREEN");
        else{
            if(u!=td_street_u||v!=td_street_v||td.district!=td_street_d){
                td_get_street(u,v,td_street_name);td_street_u=u;td_street_v=v;td_street_d=td.district;
            }
            strcpy(td_line,td_street_name);
        }
        /* Street name, then the compass in the last column. */
        for(i=0;i<19&&td_line[i];i++);
        for(;i<19;i++)td_line[i]=' ';
        td_compass=td_compass_code();td_line[19]=td_compass;td_line[20]=0;td_row(0,td_line);
    }
    key[5]=td.district|((UWORD)td_route_district<<8)|((UWORD)td_target.district<<12);
    if(td.job!=TD_NONE){key[0]=1;key[1]=td.stage;key[2]=td_job.count;key[3]=td.left;key[4]=td.health|((UWORD)td.vitality<<8);}
    else{key[0]=2;key[1]=td.cash;key[2]=td.onfoot|(td.vehicle<<1);key[3]=td.done;key[4]=td.vitality|((UWORD)td.ammo<<8);}
    if(!changed&&!memcmp(key,td_hud_key,sizeof(key)))return;
    memcpy(td_hud_key,key,sizeof(key));
    if(td.job!=TD_NONE){
        /* The last ten seconds flash a red clock. */
        td_format(td_line,TD_UI_BOX "%u/%u %s%uS " TD_UI_HEART "%u",td.stage+1,td_job.count,
                  td.left<=10&&(td.left&1)?TD_UI_CLOCK_ALERT:TD_UI_CLOCK,td.left,td.health);td_row(1,td_line);
        td_line[0]=TD_UI_PIN[0];
        if(td_target.district!=td.district){
            if(td_route_district!=TD_NONE)td_get_district_name(td_route_district,td_line+1);
            else strcpy(td_line+1,"NO ROAD ROUTE");
        }else strcpy(td_line+1,td_target.name);
        /* Objective name, then the courier's own vitality. */
        td_line[14]=0;for(i=1;i<14&&td_line[i];i++);
        td_format(td_line+i," " TD_UI_MEDIC "%u",td.vitality);
        td_row(2,td_line);
    }else{
        td_format(td_line,TD_UI_COIN "%u " TD_UI_MEDIC "%u " TD_UI_AMMO "%u " TD_UI_BOX "%u",td.cash,td.vitality,td.ammo,td.done);td_row(1,td_line);
        td_row(2,td.onfoot?TD_UI_BTN_A "PUNCH " TD_UI_BTN_B "SHOOT " TD_UI_BTN_START_0 TD_UI_BTN_START_1 TD_UI_BTN_START_2:
                           TD_UI_BTN_SEL_0 TD_UI_BTN_SEL_1 "JOBS " TD_UI_BTN_A TD_UI_BTN_B "HOLD: EXIT");
    }
}
/* The controls card below the title illustration starts at row 11. */
typedef char td_title_leaves_card_rows[(TD_TITLE_ROWS==11)?1:-1];
static const char td_title_1[]=TD_UI_EMBLEM_TL TD_UI_EMBLEM_TR " TORONTO";
static const char td_title_2[]=TD_UI_EMBLEM_BL TD_UI_EMBLEM_BR " DISPATCH";
static void td_page(const char *a,const char *b){
    /* Common frame: emblem title, then a rule; rows 3..16 belong to the page. */
    td_row(0,td_frame_top);td_framed(1,a);td_framed(2,b);td_row(3,td_frame_join);td_row(17,td_frame_bottom);
}
void td_ui_draw(void) BANKED {
    UBYTE i,wait,service,changed=td_ui_mode!=td.mode;UWORD minutes;
    td_ui_mode=td.mode;
    text_drawn=TRUE;
    if(td.mode!=TD_HELP)td_title_hide();
    if(td.mode==TD_MAP){
        ui_set_pos(0,0);if(changed)for(i=0;i<18;i++)td_row(i,"");
        td_map_headers();return;
    }
    if(td.mode==TD_ROAM || td.mode==TD_WAIT || td.mode==TD_RIDE){td_hud(changed);return;}
    ui_set_pos(0,0);if(changed)for(i=0;i<18;i++)td_row(i,"");
    if(td.mode==TD_HELP){
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
    if(td.mode==TD_PAUSE){
        td_page(td_title_1,td_title_2);
        td_format(td_line,TD_UI_COIN "%u  " TD_UI_BOX "%u/%u DONE",td.cash,td.done,TD_QUESTS);td_framed(4,td_line);
        /* Time of day: the sun from 07:00, the moon from 19:00. */
        minutes=td_daynight_minutes();
        td_format(td_line,"%s%s %s%02u:%02u",td.onfoot?TD_UI_WALK:td_vehicle_icon[td.vehicle],td.onfoot?"WALK":td_vehicle_short[td.vehicle],
                  minutes>=7*60&&minutes<19*60?TD_UI_SUN:TD_UI_MOON,minutes/60,minutes%60);td_framed(5,td_line);
        td_row(6,td_frame_join);
        for(i=0;i<9;i++){
            td_line[0]=td.menu==i?(td_ui_blink?TD_UI_CURSOR_ALT[0]:TD_UI_CURSOR[0]):' ';
            strcpy(td_line+1,td_menu_items[i]);
            if(i==8){strcat(td_line," ");strcat(td_line,td_audio_names[td_audio_get_mode()]);}
            td_framed(7+i,td_line);
        }
        td_framed(16,TD_UI_BTN_A "CHOOSE  " TD_UI_BTN_B "BACK");return;
    }
    if(td.mode==TD_BOARD){
        td_format(td_line,"< CONTRACT %02u/%u >",td.menu+1,TD_QUESTS);td_page(TD_UI_JOBS " DISPATCH BOARD",td_line);
        td_framed(4,td_offer.title);
        td_get_brief(td.menu,td_line);td_framed(6,td_line+18);td_line[18]=0;td_framed(5,td_line);
        td_format(td_line,TD_UI_BOX "%s",td_kinds[td_offer.kind]);td_framed(7,td_line);
        td_row(8,td_frame_join);
        td_format(td_line,TD_UI_STAR "%u STOPS " TD_UI_CLOCK "%uS",td_offer.count,td_offer.seconds);td_framed(9,td_line);
        td_format(td_line,TD_UI_COIN "PAYS $%u",td_offer.reward);td_framed(10,td_line);
        td_format(td_line,TD_UI_CAR "%s",td_offer.vehicle==TD_NONE?"ANY VEHICLE/TTC":td_vehicles[td_offer.vehicle]);td_framed(11,td_line);
        td_get_stop(td_offer.route[0],&td_cursor);td_format(td_line,TD_UI_PIN "%s",td_cursor.name);td_framed(12,td_line);
        if(td.complete[td.menu>>3]&(1<<(td.menu&7)))strcpy(td_line,TD_UI_CHECK "COMPLETE / REPLAY");
        else if(td.done<td_offer.min_done)td_format(td_line,TD_UI_CANCEL "NEEDS %u DONE",td_offer.min_done);
        else strcpy(td_line,TD_UI_STAR "READY TO ACCEPT");
        td_framed(13,td_line);td_row(14,td_frame_join);
        td_framed(15,TD_UI_BTN_A "ACCEPT " TD_UI_BTN_B "BACK");td_framed(16,TD_UI_DPAD "BROWSE " TD_UI_CLOCK "PAUSED");return;
    }
    if(td.mode==TD_TRANSIT){
        service=td_transit_service(td.transit_origin);
        strcpy(td_line,td_service_icon(service));td_transit_label(td.transit_origin,td_line+1);
        td_page(td_line,service==4?(td.transit_target>=td.transit_origin?"STREETCAR EAST":"STREETCAR WEST"):"");
        td_format(td_line,TD_UI_PIN "%s",td_cursor.name);td_framed(4,td_line);
        wait=td_transit_departure(td.transit_origin,td.transit_target,td.seconds);td_format(td_line,TD_UI_CLOCK "DEPARTS IN %uS",wait);td_framed(5,td_line);
        td_format(td_line,"RIDE %uS  " TD_UI_COIN "$%u",td_transit_duration(td.transit_origin,td.transit_target),td_transit_fare(td.transit_origin));td_framed(6,td_line);
        td_row(7,td_frame_join);
        td_framed(8,TD_UI_DPAD "STOPS " TD_UI_BTN_A "WAIT " TD_UI_BTN_B "BACK");
        td_row(9,td_frame_join);
        td_framed(10,TD_UI_SUBWAY "TRAIN $3 " TD_UI_BUS "BUS $2");td_framed(11,TD_UI_TRANSIT "501 $3 " TD_UI_FERRY "FERRY $4");
        td_framed(12,"");
        td_framed(13,service==4?"NORMAL QUEEN ROUTE":"UP: BUS OR TRAIN");td_framed(14,service==4?"GAME ROUTE ENDS":"AT WELLESLEY");
        td_framed(15,"");td_framed(16,"SCHEDULES: FICTION");return;
    }
    if(td.mode==TD_BUSTED||td.mode==TD_WASTED){
        UBYTE busted=td.mode==TD_BUSTED;
        td_page(td_title_1,td_title_2);
        td_framed(4,"");
        td_framed(5,busted?TD_UI_STAR " BUSTED " TD_UI_STAR:TD_UI_MEDIC " WASTED " TD_UI_MEDIC);
        td_framed(6,"");
        td_format(td_line,busted?TD_UI_COIN "FINE PAID $%u":TD_UI_COIN "HOSPITAL $%u",td_life_fine);td_framed(7,td_line);
        td_row(8,td_frame_join);
        td_framed(9,busted?"POLICE TOOK HALF":"YOU WOKE UP AT");
        td_framed(10,busted?"OF YOUR AMMO":"TORONTO HOSPITAL");
        td_framed(11,"");
        td_framed(12,TD_UI_STAR "STARS CLEARED");
        td_framed(13,TD_UI_CANCEL "ANY JOB IS LOST");
        td_row(14,td_frame_join);
        td_framed(15,TD_UI_BTN_A "CONTINUE");td_framed(16,"");return;
    }
    if(td.mode==TD_RESULT){
        UBYTE ok=td.health&&td.left;
        td_page(td_title_1,td_title_2);
        td_framed(4,"");
        td_framed(5,ok?TD_UI_STAR " DELIVERED " TD_UI_STAR:TD_UI_CANCEL " CONTRACT FAILED");
        td_framed(6,"");
        td_format(td_line,TD_UI_COIN "%u  " TD_UI_BOX "%u/%u DONE",td.cash,td.done,TD_QUESTS);td_framed(7,td_line);
        td_row(8,td_frame_join);
        td_framed(9,td.done==TD_QUESTS?TD_UI_STAR "CITY COURIER MASTER":"MORE ROUTES AWAIT");
        td_framed(10,"");
        td_framed(11,TD_UI_BTN_A "DISPATCH BOARD");td_framed(12,TD_UI_BTN_B "FREE ROAM");
        td_framed(13,"");td_row(14,td_frame_join);td_framed(15,TD_UI_SAVE "AUTO-SAVED");td_framed(16,"");
    }
}
