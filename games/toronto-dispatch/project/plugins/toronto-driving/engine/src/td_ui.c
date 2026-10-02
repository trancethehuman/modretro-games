#pragma bank 255
#include <string.h>
#include <stdio.h>
#include "td_game.h"
#include "td_font.h"
#include "td_audio.h"
#include "ui.h"
#include "camera.h"
#include "scroll.h"
#include "system.h"

static UBYTE td_tiles[20];
static UBYTE td_cached_rows[18][20];
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
    if(!memcmp(td_cached_rows[y],td_tiles,20))return;
    memcpy(td_cached_rows[y],td_tiles,20);
    VBK_REG=0; set_win_tiles(0,y,20,1,td_tiles);
}
void td_ui_init(void) BANKED {
    UBYTE i;td_ui_mode=255;memset(td_cached_rows,255,sizeof(td_cached_rows));memset(td_tiles,15,20);
    /* The window always uses font bank 1 / UI palette 7; attributes need one upload. */
    VBK_REG=1;for(i=0;i<18;i++)set_win_tiles(0,i,20,1,td_tiles);
    VBK_REG=1;set_bkg_data(192,sizeof(td_font)/16,td_font);VBK_REG=0;
    text_drawn=TRUE;td_ui_draw();
}
void td_ui_draw(void) BANKED {
    UBYTE i,wait,service,changed=td_ui_mode!=td.mode; UWORD u=td.u>>4,v=td.v>>4;
    td_ui_mode=td.mode;
    text_drawn=TRUE;
    if(td.mode==TD_ROAM || td.mode==TD_WAIT || td.mode==TD_RIDE || td.mode==TD_MAP) {
        ui_set_pos(0,120);
        if(td.mode==TD_MAP) {td_row(0,"MAP: DPAD SCROLL");td_row(1,"A TARGET  B BACK");td_row(2,td.job==TD_NONE?"FREE CITY":td_target.name);return;}
        if(td.mode==TD_WAIT){
            wait=td_next_departure(td.transit_origin,td.seconds);service=td_service(td.transit_origin);
            sprintf(td_line,"DEPARTS IN %u SEC",wait);td_row(0,td_line);td_row(1,service==1?"LINE 1 TRAIN":service==2?"94 WELLESLEY BUS":"ISLAND FERRY");td_row(2,"B CANCEL WAIT");return;
        }
        if(td.mode==TD_RIDE){sprintf(td_line,"RIDING %u SEC",td.ride_left);td_row(0,td_line);td_row(1,td_cursor.name);td_row(2,"FARE PAID / ON TIME");return;}
        if(td.msg){
            const char *m[]={"","STOP TO INTERACT","WRONG VEHICLE","JOB IS LOCKED","NO FARE MONEY","CRASH: CARGO HURT","STOP AT THE BEACON","RED SIGNAL: FINE","HEAVY CARGO: DRIVE","VEHICLE IS PARKED","NO WATER CROSSING","STOP TO PARK","SAVED TO CARTRIDGE","PEDESTRIAN: BRAKE","TURN GENTLY: RIDER","DOOR PATH BLOCKED"};
            td_row(0,m[td.msg]);
        }else {
            if(u>608&&u<672&&v>496&&v<560)td_row(0,td.seconds%12<7?"YONGE: E/W GREEN":"YONGE: N/S GREEN");
            else{td_get_street(u,v,td_line);td_row(0,td_line);}
        }
        if(td.job!=TD_NONE){sprintf(td_line,"%u/%u %uS C%u",td.stage+1,td_job.count,td.left,td.health);td_row(1,td_line);td_row(2,td_target.name);}
        else {sprintf(td_line,"$%u %s %u/72",td.cash,td.onfoot?"WALK":td_vehicles[td.vehicle],td.done);td_row(1,td_line);td_row(2,td.onfoot?"A CAR / B TRANSIT":"SELECT JOBS START UI");}
        return;
    }
    ui_set_pos(0,0);if(changed)for(i=0;i<18;i++)td_row(i,"");
    td_row(0," TORONTO DISPATCH");
    if(td.mode==TD_HELP){
        td_row(2,"A GAS / B BRAKE");td_row(3,"LEFT RIGHT STEERING");td_row(4,"B HELD: REVERSE");td_row(6,"SELECT: DELIVER/JOBS");td_row(7,"START: PAUSE MENU");td_row(8,"STOP NEAR THE BEACON");td_row(10,"PARK TO WALK / TTC");td_row(12,"ON FOOT: A ENTER CAR");td_row(13,"B TTC AT A STATION");td_row(11,"FARES + TIMETABLES");td_row(15,"A OR B: ENTER CITY");return;
    }
    if(td.mode==TD_PAUSE){
        sprintf(td_line,"$%u  DONE %u/72",td.cash,td.done);td_row(2,td_line);
        td_row(4,td.menu==0?"> RESUME":"  RESUME");td_row(5,td.menu==1?"> SCROLL CITY MAP":"  SCROLL CITY MAP");td_row(6,td.menu==2?"> DISPATCH JOBS":"  DISPATCH JOBS");td_row(7,td.menu==3?"> PARK / RECOVER CAR":"  PARK / RECOVER CAR");td_row(8,td.menu==4?"> CHANGE VEHICLE":"  CHANGE VEHICLE");td_row(9,td.menu==5?"> TRANSIT TIMETABLE":"  TRANSIT TIMETABLE");td_row(10,td.menu==6?"> SAVE PROGRESS":"  SAVE PROGRESS");td_row(11,td.menu==7?"> CANCEL ACTIVE JOB":"  CANCEL ACTIVE JOB");
        sprintf(td_line,"%c AUDIO: %s",td.menu==8?'>':' ',td_audio_get_mode()==TD_AUDIO_FULL?"MUSIC+SFX":td_audio_get_mode()==TD_AUDIO_EFFECTS?"SFX ONLY":"SILENT");td_row(12,td_line);
        td_row(14,"UP DOWN / A CHOOSE");td_row(15,td_vehicles[td.vehicle]);td_row(17,"B BACK");return;
    }
    if(td.mode==TD_BOARD){
        sprintf(td_line,"CONTRACT %02u/72",td.menu+1);td_row(2,td_line);td_row(4,td_offer.title);
        td_get_brief(td.menu,td_line);td_row(6,td_line+18);td_line[18]=0;td_row(5,td_line);td_row(7,td_kinds[td_offer.kind]);
        sprintf(td_line,"%u STOPS  %u SEC",td_offer.count,td_offer.seconds);td_row(8,td_line);sprintf(td_line,"PAYS $%u",td_offer.reward);td_row(9,td_line);td_row(10,td_offer.vehicle==TD_NONE?"ANY VEHICLE / TTC":td_vehicles[td_offer.vehicle]);
        td_get_stop(td_offer.route[0],&td_cursor);td_row(11,td_cursor.name);
        if(td.complete[td.menu>>3]&(1<<(td.menu&7)))td_row(12,"COMPLETE / REPLAY");else if(td.done<td_offer.min_done){sprintf(td_line,"NEEDS %u COMPLETED",td_offer.min_done);td_row(12,td_line);}else td_row(12,"READY TO ACCEPT");
        td_row(14,"LEFT RIGHT: BROWSE");td_row(15,"A ACCEPT  B BACK");td_row(17,"PAUSE FREEZES CLOCK");return;
    }
    if(td.mode==TD_TRANSIT){
        service=td_service(td.transit_origin);td_row(2,service==1?"LINE 1 TRAIN":service==2?"94 WELLESLEY BUS":"ISLAND FERRY");td_row(4,td_cursor.name);sprintf(td_line,"NEXT STOP %u",td.menu+1);td_row(6,td_line);td_row(8,"LEFT RIGHT: STOPS");td_row(9,"A: WAIT AND BOARD");td_row(10,"B: BACK");td_row(12,"TRAIN $3 BUS $2");td_row(13,"FERRY $4 GAME FARES");td_row(15,"UP: BUS/TRAIN AT");td_row(16,"WELLESLEY INTERCHANGE");td_row(17,"SCHEDULES ARE FICTION");return;
    }
    if(td.mode==TD_RESULT){
        td_row(4,td.health && td.left?"CONTRACT DELIVERED":"CONTRACT FAILED");sprintf(td_line,"$%u  DONE %u/72",td.cash,td.done);td_row(7,td_line);td_row(10,td.done==72?"CITY COURIER MASTER":"MORE ROUTES AWAIT");td_row(12,"A: DISPATCH BOARD");td_row(14,"B: FREE ROAM");td_row(16,"PROGRESS AUTO-SAVED");
    }
}
