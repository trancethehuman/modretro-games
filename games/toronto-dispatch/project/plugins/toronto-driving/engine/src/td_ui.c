#pragma bank 255
#include <string.h>
#include <stdio.h>
#include "td_game.h"
#include "td_font.h"
#include "td_audio.h"
#include "td_atlas.h"
#include "actor.h"
#include "data_manager.h"
#include "ui.h"
#include "camera.h"
#include "scroll.h"
#include "system.h"

static UBYTE td_tiles[20];
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
static UBYTE td_glyph(char c) {
    if(c>='a'&&c<='z')c-=32;
    if(c>='A'&&c<='Z')return 193+c-'A';
    if(c>='0'&&c<='9')return 219+c-'0';
    switch(c){case ':':return 229;case '/':return 230;case '.':return 231;case '-':return 232;case '+':return 233;case '?':return 234;case '<':return 235;case '>':return 236;case '$':return 237;case '%':return 238;case '#':return 239;case '=':return 240;}
    return 192;
}
static void td_row(UBYTE y,const char *s) {
    UBYTE i; for(i=0;i<20;i++){td_tiles[i]=td_glyph(*s);if(*s)s++;}
    if(td.mode!=TD_MAP){
        if(!memcmp(td_cached_rows[y],td_tiles,20))return;
        memcpy(td_cached_rows[y],td_tiles,20);
    }
    VBK_REG=0; set_win_tiles(0,y,20,1,td_tiles);
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
    else{sprintf(td_line,"%s%s",trip?"TRIP: ":td.job==TD_NONE?"DEPOT: ":"JOB: ",target->name);td_row(15,td_line);}
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
        glyph=td_glyph(bits&4?'O':bits&1?'P':'C')-192;
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
void td_map_close(void) BANKED {
    UBYTE i;
    if(!td_map_active)return;
    for(i=0;i<td_map_actor_count;i++)actors[i].flags=(actors[i].flags&~ACTOR_FLAG_HIDDEN)|td_map_hidden[i];
    camera_x=td_map_camera_x;camera_y=td_map_camera_y;camera_settings=td_map_camera_settings;
    td_map_active=0;memset(td_cached_rows,255,sizeof(td_cached_rows));VBK_REG=0;
}
void td_ui_init(void) BANKED {
    UBYTE i;td_ui_mode=255;td_map_active=0;memset(td_cached_rows,255,sizeof(td_cached_rows));memset(td_tiles,15,20);
    /* The window always uses font bank 1 / UI palette 7; attributes need one upload. */
    VBK_REG=1;for(i=0;i<18;i++)set_win_tiles(0,i,20,1,td_tiles);
    VBK_REG=1;set_bkg_data(192,sizeof(td_font)/16,td_font);VBK_REG=0;
    text_drawn=TRUE;td_ui_draw();
}
void td_ui_draw(void) BANKED {
    UBYTE i,wait,service,changed=td_ui_mode!=td.mode; UWORD u=td.u>>4,v=td.v>>4;
    td_ui_mode=td.mode;
    text_drawn=TRUE;
    if(td.mode==TD_MAP){
        ui_set_pos(0,0);if(changed)for(i=0;i<18;i++)td_row(i,"");
        td_map_headers();return;
    }
    if(td.mode==TD_ROAM || td.mode==TD_WAIT || td.mode==TD_RIDE) {
        ui_set_pos(0,120);
        if(td.mode==TD_WAIT){
            wait=td_next_departure(td.transit_origin,td.seconds);service=td_service(td.transit_origin);
            sprintf(td_line,"DEPARTS IN %u SEC",wait);td_row(0,td_line);td_row(1,service==1?"LINE 1 TRAIN":service==2?"94 WELLESLEY BUS":"ISLAND FERRY");td_row(2,"B CANCEL WAIT");return;
        }
        if(td.mode==TD_RIDE){sprintf(td_line,"RIDING %u SEC",td.ride_left);td_row(0,td_line);td_row(1,td_cursor.name);td_row(2,"FARE PAID / ON TIME");return;}
        if(td.msg){
            const char *m[]={"","STOP TO INTERACT","WRONG VEHICLE","JOB IS LOCKED","NO FARE MONEY","CRASH: CARGO HURT","STOP AT THE BEACON","RED SIGNAL: FINE","HEAVY CARGO: DRIVE","VEHICLE IS PARKED","NO WATER CROSSING","STOP TO PARK","SAVED TO CARTRIDGE","PEDESTRIAN: BRAKE","TURN GENTLY: RIDER","DOOR PATH BLOCKED","PARK THEN WALK"};
            td_row(0,m[td.msg]);
        }else {
            if(td.district==0&&u>608&&u<672&&v>496&&v<560)td_row(0,td.seconds%12<7?"YONGE: E/W GREEN":"YONGE: N/S GREEN");
            else{td_get_street(u,v,td_line);td_row(0,td_line);}
        }
        if(td.job!=TD_NONE){sprintf(td_line,"%u/%u %uS C%u",td.stage+1,td_job.count,td.left,td.health);td_row(1,td_line);if(td_target.district!=td.district){if(td_route_district!=TD_NONE)td_get_district_name(td_route_district,td_line);else strcpy(td_line,"NO ROAD ROUTE");td_row(2,td_line);}else td_row(2,td_target.name);}
        else {sprintf(td_line,"$%u %s %u/%u",td.cash,td.onfoot?"WALK":td_vehicles[td.vehicle],td.done,TD_QUESTS);td_row(1,td_line);td_row(2,td.onfoot?"A CAR / B TRANSIT":"SELECT JOBS START UI");}
        return;
    }
    ui_set_pos(0,0);if(changed)for(i=0;i<18;i++)td_row(i,"");
    td_row(0," TORONTO DISPATCH");
    if(td.mode==TD_HELP){
        td_row(2,"A GAS / B BRAKE");td_row(3,"LEFT RIGHT STEERING");td_row(4,"B HELD: REVERSE");td_row(6,"SELECT: DELIVER/JOBS");td_row(7,"START: PAUSE MENU");td_row(8,"STOP NEAR THE BEACON");td_row(10,"PARK TO WALK / TTC");td_row(12,"ON FOOT: A ENTER CAR");td_row(13,"B TTC AT A STATION");td_row(11,"FARES + TIMETABLES");td_row(15,"A OR B: ENTER CITY");return;
    }
    if(td.mode==TD_PAUSE){
        sprintf(td_line,"$%u  DONE %u/%u",td.cash,td.done,TD_QUESTS);td_row(2,td_line);
        td_row(4,td.menu==0?"> RESUME":"  RESUME");td_row(5,td.menu==1?"> SCROLL CITY MAP":"  SCROLL CITY MAP");td_row(6,td.menu==2?"> DISPATCH JOBS":"  DISPATCH JOBS");td_row(7,td.menu==3?"> PARK / RECOVER CAR":"  PARK / RECOVER CAR");td_row(8,td.menu==4?"> CHANGE VEHICLE":"  CHANGE VEHICLE");td_row(9,td.menu==5?"> TRANSIT TIMETABLE":"  TRANSIT TIMETABLE");td_row(10,td.menu==6?"> SAVE PROGRESS":"  SAVE PROGRESS");td_row(11,td.menu==7?"> CANCEL ACTIVE JOB":"  CANCEL ACTIVE JOB");
        sprintf(td_line,"%c AUDIO: %s",td.menu==8?'>':' ',td_audio_get_mode()==TD_AUDIO_FULL?"MUSIC+SFX":td_audio_get_mode()==TD_AUDIO_EFFECTS?"SFX ONLY":"SILENT");td_row(12,td_line);
        td_row(14,"UP DOWN / A CHOOSE");td_row(15,td_vehicles[td.vehicle]);td_row(17,"B BACK");return;
    }
    if(td.mode==TD_BOARD){
        sprintf(td_line,"CONTRACT %02u/%u",td.menu+1,TD_QUESTS);td_row(2,td_line);td_row(4,td_offer.title);
        td_get_brief(td.menu,td_line);td_row(6,td_line+18);td_line[18]=0;td_row(5,td_line);td_row(7,td_kinds[td_offer.kind]);
        sprintf(td_line,"%u STOPS  %u SEC",td_offer.count,td_offer.seconds);td_row(8,td_line);sprintf(td_line,"PAYS $%u",td_offer.reward);td_row(9,td_line);td_row(10,td_offer.vehicle==TD_NONE?"ANY VEHICLE / TTC":td_vehicles[td_offer.vehicle]);
        td_get_stop(td_offer.route[0],&td_cursor);td_row(11,td_cursor.name);
        if(td.complete[td.menu>>3]&(1<<(td.menu&7)))td_row(12,"COMPLETE / REPLAY");else if(td.done<td_offer.min_done){sprintf(td_line,"NEEDS %u COMPLETED",td_offer.min_done);td_row(12,td_line);}else td_row(12,"READY TO ACCEPT");
        td_row(14,"LEFT RIGHT: BROWSE");td_row(15,"A ACCEPT  B BACK");td_row(17,"PAUSE FREEZES CLOCK");return;
    }
    if(td.mode==TD_TRANSIT){
        service=td_service(td.transit_origin);td_row(2,service==1?"LINE 1 TRAIN":service==2?"94 WELLESLEY BUS":"ISLAND FERRY");td_row(4,td_cursor.name);wait=td_next_departure(td.transit_origin,td.seconds);sprintf(td_line,"DEPARTS IN %u SEC",wait);td_row(6,td_line);td_row(8,"LEFT RIGHT: STOPS");td_row(9,"A: WAIT AND BOARD");td_row(10,"B: BACK");td_row(12,"TRAIN $3 BUS $2");td_row(13,"FERRY $4 GAME FARES");td_row(15,"UP: BUS/TRAIN AT");td_row(16,"WELLESLEY INTERCHANGE");td_row(17,"SCHEDULES ARE FICTION");return;
    }
    if(td.mode==TD_RESULT){
        td_row(4,td.health && td.left?"CONTRACT DELIVERED":"CONTRACT FAILED");sprintf(td_line,"$%u  DONE %u/%u",td.cash,td.done,TD_QUESTS);td_row(7,td_line);td_row(10,td.done==TD_QUESTS?"CITY COURIER MASTER":"MORE ROUTES AWAIT");td_row(12,"A: DISPATCH BOARD");td_row(14,"B: FREE ROAM");td_row(16,"PROGRESS AUTO-SAVED");
    }
}
