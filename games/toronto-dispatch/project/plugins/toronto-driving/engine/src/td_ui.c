#pragma bank 255
#include <string.h>
#include <stdio.h>
#include "td_game.h"
#include "td_district.h"
#include "td_streetcar_runtime.h"
#include "td_transit.h"
#include "td_font.h"
#include "td_audio.h"
#include "td_atlas.h"
#include "td_traffic_lights.h"
#include "td_boats.h"
#include "td_guidance.h"
#include "actor.h"
#include "data_manager.h"
#include "ui.h"
#include "camera.h"
#include "scroll.h"
#include "system.h"

static UBYTE td_tiles[20];
/* The paused atlas owns this cache until text is repainted. Text uses six
 * bits per glyph (192..252, with 63 empty); atlas IDs use ten bits (0..772,
 * with 1023 empty). Packing preserves the logical cache and saves 90 bytes. */
static union { UBYTE rows[18][15]; UBYTE patterns[(TD_ATLAS_VISIBLE_LIMIT*10+7)/8]; } td_ui_cache;
#define td_cached_rows td_ui_cache.rows
typedef char td_atlas_cache_fits_packed_wram[(sizeof(td_ui_cache)==270)?1:-1];
typedef char td_atlas_cache_ids_fit[(TD_ATLAS_PATTERNS<1023)?1:-1];
typedef char td_atlas_tiles_leave_font[(16+TD_ATLAS_VISIBLE_LIMIT<=192)?1:-1];
typedef char td_atlas_hash_capacity[(TD_ATLAS_VISIBLE_LIMIT==172)?1:-1];
static UBYTE td_map_x,td_map_y,td_map_row,td_map_count,td_map_focus,td_map_active,td_map_error;
static UBYTE td_map_camera_settings,td_map_actor_count,td_map_hidden[(TD_ACTORS+7)/8];
typedef char td_map_hidden_pool_fits[(TD_ACTORS>0&&TD_ACTORS<=24)?1:-1];
static UWORD td_map_camera_x,td_map_camera_y;
static UBYTE td_ui_mode=255;
static char td_line[40];
static const char td_wrong_vehicle[]="WRONG VEHICLE";
static const char *td_vehicles[]={"CAR","TRUCK","MOTORCYCLE","SCOOTER"};
static const char * const td_chapters[]={
    "01/13 FIRST SHIFT",
    "02/13 NEIGHBOURHOODS",
    "03/13 CITY EVENTS",
    "04/13 CROSS THE CITY",
    "05/13 WATERFRONT",
    "06/13 ARTS/AUDIENCES",
    "07/13 EVENING",
    "08/13 NETWORK",
    "09/13 MASTER COURIER",
    "10/13 WEST ROUTES",
    "11/13 EAST ROUTES",
    "12/13 PORT LANDS",
    "13/13 UPTOWN HILLS",
};
typedef char td_chapter_offer_count[(TD_QUESTS==8*(sizeof(td_chapters)/sizeof(td_chapters[0])))?1:-1];
static const char *td_kinds[]={"PARCEL ROUND","FRAGILE: NO CRASH","EXPRESS DEADLINE","TRUCK FREIGHT","TRANSIT FRIENDLY","PASSENGER: SMOOTH","RETURN DOCUMENTS","ISLAND FERRY POST"};
static UBYTE td_glyph(char c) {
    if(c>=1&&c<=12)return 240+c;
    if(c>='a'&&c<='z')c-=32;
    if(c>='A'&&c<='Z')return 193+c-'A';
    if(c>='0'&&c<='9')return 219+c-'0';
    switch(c){case ':':return 229;case '/':return 230;case '.':return 231;case '-':return 232;case '+':return 233;case '?':return 234;case '<':return 235;case '>':return 252;case '$':return 237;case '%':return 238;case '#':return 239;case '=':return 240;}
    return 192;
}
/* Four native glyphs occupy exactly three bytes. No temporary row buffer or
 * individual bit-walk is needed, and the caller supplies td_glyph output. */
static UBYTE td_row_cache_apply(UBYTE *cache,const UBYTE *tiles){
    UBYTE i,j,a,b,c,changed=0;
    for(i=j=0;i<20;i+=4,j+=3){
        a=(tiles[i]-192)|((tiles[i+1]-192)<<6);
        b=((tiles[i+1]-192)>>2)|((tiles[i+2]-192)<<4);
        c=((tiles[i+2]-192)>>4)|((tiles[i+3]-192)<<2);
        if(cache[j]!=a||cache[j+1]!=b||cache[j+2]!=c)changed=1;
        cache[j]=a;cache[j+1]=b;cache[j+2]=c;
    }
    return changed;
}
static UWORD td_map_cache_get(UBYTE slot){
    UBYTE index,shift;UWORD pattern;
    if(slot>=TD_ATLAS_VISIBLE_LIMIT)return 65535;
    index=slot+(slot>>2);shift=(slot&3)<<1;
    pattern=((UWORD)td_ui_cache.patterns[index]|((UWORD)td_ui_cache.patterns[index+1]<<8))>>shift;
    pattern&=1023;return pattern==1023?65535:pattern;
}
static void td_map_cache_set(UBYTE slot,UWORD pattern){
    UBYTE index,shift;UWORD mask,value;
    if(slot>=TD_ATLAS_VISIBLE_LIMIT||(pattern!=65535&&pattern>=1023))return;
    index=slot+(slot>>2);shift=(slot&3)<<1;
    mask=(UWORD)1023<<shift;value=(pattern==65535?(UWORD)1023:pattern)<<shift;
    td_ui_cache.patterns[index]=(td_ui_cache.patterns[index]&~(UBYTE)mask)|(UBYTE)value;
    td_ui_cache.patterns[index+1]=(td_ui_cache.patterns[index+1]&~(UBYTE)(mask>>8))|(UBYTE)(value>>8);
}
static void td_row(UBYTE y,const char *s);
static void td_guided_row(const char *label){td_guidance_label(td_line,label);td_row(2,td_line);td_guidance_compact(td_line,TRUE);td_row(td.msg?1:0,td_line);}
static void td_row(UBYTE y,const char *s) {
    UBYTE i; for(i=0;i<20;i++){td_tiles[i]=td_glyph(*s);if(*s)s++;}
    if(td.mode!=TD_MAP){
        if(!td_row_cache_apply(td_cached_rows[y],td_tiles))return;
    }
    VBK_REG=0; set_win_tiles(0,y,20,1,td_tiles);
}
static const td_stop_t *td_map_destination(void){
    return td.job==TD_NONE&&(td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)?&td_cursor:&td_target;
}
static UBYTE td_map_point(UBYTE focus,UWORD *x,UWORD *y){
    const td_stop_t *target;
    if(!focus)return td_streetcar_ride_view?td_atlas_position(td_streetcar_view_district,td_streetcar_focus_u>>4,td_streetcar_focus_v>>4,x,y):td_atlas_position(td.district,td.u>>4,td.v>>4,x,y);
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
    if(!td_map_focus)td_row(15,td_streetcar_ride_view?"YOU ON STREETCAR":td_boats_controlled()?"YOU ON A BOAT":td.onfoot?"YOU ON FOOT":"YOU DRIVING");
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
            if(td_map_cache_get(slot)==patterns[i]||td_map_cache_get(slot)==65535)break;
            slot+=stride;if(slot>=TD_ATLAS_VISIBLE_LIMIT)slot-=TD_ATLAS_VISIBLE_LIMIT;
        }
        if(probes==TD_ATLAS_VISIBLE_LIMIT){td_map_error=1;td_map_row=12;td_map_headers();return;}
        if(td_map_cache_get(slot)==65535){
            if(td_map_count>=TD_ATLAS_VISIBLE_LIMIT||!td_atlas_pattern(patterns[i],(UBYTE *)td_line)){
                td_map_error=1;td_map_row=12;td_map_headers();return;
            }
            td_map_cache_set(slot,patterns[i]);td_map_count++;
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
    memset(td_map_hidden,0,sizeof(td_map_hidden));
    for(i=0;i<td_map_actor_count;i++){
        if(actors[i].flags&ACTOR_FLAG_HIDDEN)td_map_hidden[i>>3]|=1<<(i&7);
        actors[i].flags|=ACTOR_FLAG_HIDDEN;
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
    for(i=0;i<td_map_actor_count;i++)actors[i].flags=(actors[i].flags&~ACTOR_FLAG_HIDDEN)|
        (td_map_hidden[i>>3]&(1<<(i&7))?ACTOR_FLAG_HIDDEN:0);
    camera_x=td_map_camera_x;camera_y=td_map_camera_y;camera_settings=td_map_camera_settings;
    td_map_active=0;memset(td_cached_rows,255,sizeof(td_cached_rows));VBK_REG=0;
    td_traffic_lights_reset();
}
void td_ui_init(void) BANKED {
    UBYTE i;td_board_route=0;td_ui_mode=255;td_map_active=0;memset(td_cached_rows,255,sizeof(td_cached_rows));memset(td_tiles,15,20);
    /* The window always uses font bank 1 / UI palette 7; attributes need one upload. */
    VBK_REG=1;for(i=0;i<18;i++)set_win_tiles(0,i,20,1,td_tiles);
    VBK_REG=1;set_bkg_data(192,sizeof(td_font)/16,td_font);td_guidance_init();VBK_REG=0;
    text_drawn=TRUE;td_ui_draw();
}
void td_ui_draw(void) BANKED {
    UBYTE i,wait,service,fare,page=(td.mode==TD_PAUSE&&td.menu>=TD_SETTINGS_SOUND)?(TD_PAUSE|128):td.mode;
    UBYTE changed=td_ui_mode!=page; UWORD u=td.u>>4,v=td.v>>4;
    td_ui_mode=page;
    text_drawn=TRUE;
    if(td.mode==TD_MAP){
        ui_set_pos(0,0);if(changed)for(i=0;i<18;i++)td_row(i,"");
        td_map_headers();return;
    }
    if(td.mode==TD_ROAM || td.mode==TD_WAIT || td.mode==TD_RIDE) {
        /* Normal driving has one8px strip; notices and transit get16px. */
        ui_set_pos(0,td.mode==TD_ROAM?(td.msg?128:136):120);
        if(td.mode==TD_WAIT){
            wait=td_transit_departure(td.transit_origin,td.transit_target,td.seconds);
            sprintf(td_line,"DEPARTS IN %u SEC",wait);td_row(0,td_line);
            if(td.msg==18)td_row(1,"TRAM: STEP CLEAR");
            else if(td.district==TD_DISTRICT_ISLANDS&&td_transit_service(td.transit_origin)==TD_TRANSIT_FERRY&&
                !td_transit_booking_fare(td.transit_origin,td.transit_target,td.job,td.cash,td.district))
                td_row(1,"RETURN ASSISTANCE");
            else{td_transit_label(td.transit_origin,td_line);td_row(1,td_line);}
            td_row(2,"B CANCEL WAIT");return;
        }
        if(td.mode==TD_RIDE){sprintf(td_line,"RIDING %u SEC",td.ride_left);td_row(0,td_line);td_row(1,td_cursor.name);td_row(2,"FARE PAID / ON TIME");return;}
        fare=td.msg==4&&td.district==TD_DISTRICT_ISLANDS&&td.job!=TD_NONE&&td.cash<4;
        if(td.msg){
            const char *m[]={"","STOP TO INTERACT",td_wrong_vehicle,"JOB IS LOCKED","NO FARE MONEY","CRASH: CARGO HURT","STOP AT THE BEACON","RED SIGNAL: FINE","DRIVE FOR THIS JOB","VEHICLE IS PARKED","NO WATER CROSSING","STOP TO PARK","SAVED TO CARTRIDGE","PEDESTRIAN: BRAKE","TURN GENTLY: RIDER","DOOR PATH BLOCKED","PARK THEN WALK","NO PARKING ON RAILS","TRAM: STEP CLEAR","HUMAN HIT: FINE + H","POLICE: STOP + FINE","STREET BUSY: RETRY"};
            td_row(0,td.msg==5&&td.onfoot?"VEHICLE HIT: RECOVER":fare?"NO FARE: START MENU":td.msg==5&&(td.job==TD_NONE||!td.stage)?"CRASH: BRAKE EARLY":m[td.msg]);
        }
        if(td.job!=TD_NONE){
            sprintf(td_line,"%u/%u %uS C%u H%u",td.stage+1,td_job.count,td.left,td.health,td.wanted);td_row(1,td_line);
            if(fare)td_row(2,"CANCEL JOB TO RETURN");
            else if(td_target.district!=td.district){
                if(td_route_district!=TD_NONE)td_get_district_name(td_route_district,td_line);
                else if(td.district==TD_DISTRICT_ISLANDS)strcpy(td_line,"RETURN FERRY AT DOCK");
                else if(td_target.district==TD_DISTRICT_ISLANDS)strcpy(td_line,"GO TO FERRY TERMINAL");
                else strcpy(td_line,"NO ROAD ROUTE");
                td_guided_row(td_line);
            }else td_guided_row(td_target.name);
        }
        else {sprintf(td_line,"$%u %s H%u",td.cash,td_boats_controlled()?"BOAT":td.onfoot?"WALK":td_vehicles[td.vehicle],td.wanted);td_row(1,td_line);td_row(2,td_boats_controlled()?"A GAS DOWN+A DOCK":td.onfoot?(td.district==TD_DISTRICT_ISLANDS?"A BOAT / B FERRY":"A/B CAR A DOOR/BOAT"):"SELECT JOBS START UI");
            td_get_street(u,v,td_line);td_guidance_compact(td_line,FALSE);td_row(td.msg?1:0,td_line);}
        return;
    }
    ui_set_pos(0,0);if(changed)for(i=0;i<18;i++)td_row(i,"");
    td_row(0," TORONTO DISPATCH");if(td.mode!=TD_BOARD)td_row(1,"\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13\13");
    if(td.mode==TD_HELP){
        if(td.menu!=TD_SETTINGS_CONTROLS){
            td_row(2,"WELCOME COURIER");
            td_row(4,"MAKE DELIVERIES");td_row(5,"EXPLORE TORONTO");
            td_row(7,"A GAS / B BRAKE");td_row(8,"L/R STEER IN MOTION");
            td_row(10,"SELECT JOBS/DELIVER");td_row(11,"FOLLOW THE JOB ARROW");
            td_row(13,"START MENU / SAVE");td_row(14,"SETTINGS: CONTROLS");
            td_row(16,"A OR B: START GAME");td_row(17,"PROGRESS AUTO-SAVES");return;
        }
        td_row(2,"QUICK CONTROLS");
        td_row(4,"A GAS / B BRAKE");td_row(5,"L/R STEER IN MOTION");td_row(6,"HOLD B TO REVERSE");
        td_row(7,"SELECT JOBS/DELIVER");td_row(8,"START MENU / SAVE");
        td_row(10,"D PAD WALK ON FOOT");td_row(11,"A/B ENTER/TAKE CAR");td_row(12,"A DOOR/BOAT B TTC");
        td_row(13,"DOCK: DOWN+A EXIT");td_row(14,"FOLLOW THE JOB ARROW");td_row(15,"STARS: POLICE CHASE");
        td_row(16,td.menu==TD_SETTINGS_CONTROLS?"A OR B: SETTINGS":"A OR B: START GAME");
        td_row(17,"PROGRESS AUTO-SAVES");return;
    }
    if(td.mode==TD_PAUSE){
        if(td.menu>=TD_SETTINGS_SOUND){
            td_row(2,"SETTINGS");
            td_row(5,td.menu==TD_SETTINGS_SOUND?"> SOUND":"  SOUND");
            td_row(6,td_audio_get_mode()==TD_AUDIO_FULL?"  MUSIC + EFFECTS":td_audio_get_mode()==TD_AUDIO_EFFECTS?"  EFFECTS ONLY":"  ALL SOUND OFF");
            td_row(8,td.menu==TD_SETTINGS_CONTROLS?"> CONTROL GUIDE":"  CONTROL GUIDE");
            td_row(10,td.menu==TD_SETTINGS_BACK?"> BACK TO MENU":"  BACK TO MENU");
            td_row(13,"UP/DOWN CHOOSE");td_row(14,"A SELECT / B BACK");
            td_row(16,"SOUND: A TO CHANGE");td_row(17,"SOUND RESETS AT BOOT");return;
        }
        sprintf(td_line,"$%u  DONE %u/%u",td.cash,td.done,TD_QUESTS);td_row(2,td_line);
        td_row(4,td.menu==0?"> RESUME":"  RESUME");td_row(5,td.menu==1?"> CITY MAP":"  CITY MAP");td_row(6,td.menu==2?"> JOBS":"  JOBS");td_row(7,td.menu==3?(td.onfoot?"> ENTER YOUR CAR":"> GET OUT OF CAR"):(td.onfoot?"  ENTER YOUR CAR":"  GET OUT OF CAR"));td_row(8,td.menu==4?"> CHANGE VEHICLE":"  CHANGE VEHICLE");td_row(9,td.menu==5?"> TTC SCHEDULE":"  TTC SCHEDULE");td_row(10,(td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)?(td.menu==6?"> SAVE AFTER TRIP":"  SAVE AFTER TRIP"):(td.menu==6?"> SAVE GAME":"  SAVE GAME"));td_row(11,td.menu==7?"> CANCEL JOB":"  CANCEL JOB");
        td_row(12,td.menu==8?"> SETTINGS":"  SETTINGS");
        td_row(14,"UP/DOWN CHOOSE");td_row(15,"A SELECT / B RESUME");
        td_row(17,(td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)?"TTC: MAP/SETTINGS":"SELECT CITY MAP");return;
    }
    if(td.mode==TD_BOARD){
        td_row(1,td.menu<TD_QUESTS?td_chapters[td.menu>>3]:"DISPATCH CHAPTER");
        td_row(3,"SELECT NEXT CHAPTER");
        sprintf(td_line,"CONTRACT %02u/%u",td.menu+1,TD_QUESTS);td_row(2,td_line);td_row(4,td_offer.title);
        td_get_brief(td.menu,td_line);td_row(6,td_line+18);td_line[18]=0;td_row(5,td_line);td_row(7,td_kinds[td_offer.kind]);
        sprintf(td_line,"%u STOPS  %u SEC",td_offer.count,td_offer.seconds);td_row(8,td_line);sprintf(td_line,"BASE $%u + TIME",td_offer.reward);td_row(9,td_line);td_row(10,td_offer.vehicle==TD_NONE?"ANY VEHICLE / TTC":td_vehicles[td_offer.vehicle]);
        if(td_offer.count){
            if(td_board_route>=td_offer.count)td_board_route=0;
            td_get_stop(td_offer.route[td_board_route],&td_cursor);
            sprintf(td_line,"%u/%u %s%s",td_board_route+1,td_offer.count,
                (td_cursor.reserved&TD_STOP_FOOT)||td_cursor.district==TD_DISTRICT_ISLANDS?"WALK ":"",
                !td_board_route?"PICKUP":td_board_route+1==td_offer.count?
                (td_offer.route[0]==td_offer.route[td_board_route]?"RETURN":"DELIVER"):"HANDOFF");
            td_row(11,td_line);td_row(12,td_cursor.name);
            td_get_district_name(td_cursor.district,td_line);td_row(13,td_line);
        }else{td_row(11,"NO ROUTE");td_row(12,"");td_row(13,"");}
        if(td.job!=TD_NONE&&td.menu==td.job){
            sprintf(td_line,"CURRENT STOP %u/%u",td.stage+1,td_job.count);td_row(16,td_line);
        }else if(td.menu<TD_QUESTS&&(td.complete[td.menu>>3]&(1<<(td.menu&7)))){
            td_row(16,td.job!=TD_NONE?"COMPLETE / PREVIEW":"COMPLETE / REPLAY");
        }else if(td.done<td_offer.min_done){sprintf(td_line,"NEEDS %u COMPLETED",td_offer.min_done);td_row(16,td_line);}
        else td_row(16,td.job!=TD_NONE?"READY AFTER THIS JOB":
            td.menu<TD_QUESTS&&td_offer.vehicle!=TD_NONE&&(td.onfoot||td.vehicle!=td_offer.vehicle)?
            td_wrong_vehicle:"READY TO ACCEPT");
        td_row(14,"L/R JOB U/D STOPS");td_row(15,td.job!=TD_NONE?"A RESUME  B BACK":"A ACCEPT  B BACK");
        /* These nine preserved rounds include a normal return from the last
           Island; the longest rounds pay before that final return fare. */
        td_row(17,td.menu<72&&(td.menu&7)==7&&td_offer.kind==7?
            (td.menu<24?"FERRY BUDGET $8":td.menu<48?"FERRY BUDGET $16":"FERRY BUDGET $24"):
            "PAUSE FREEZES CLOCK");return;
    }
    if(td.mode==TD_TRANSIT){
        service=td_transit_service(td.transit_origin);td_transit_label(td.transit_origin,td_line);td_row(2,td_line);td_row(4,td_cursor.name);
        td_row(5,service==4?(td.transit_target>=td.transit_origin?"STREETCAR EASTBOUND":"STREETCAR WESTBOUND"):"");
        wait=td_transit_departure(td.transit_origin,td.transit_target,td.seconds);sprintf(td_line,"DEPARTS IN %u SEC",wait);td_row(6,td_line);
        fare=td_transit_booking_fare(td.transit_origin,td.transit_target,td.job,td.cash,td.district);
        sprintf(td_line,"RIDE %u SEC / $%u",td_transit_duration(td.transit_origin,td.transit_target),fare);td_row(7,td_line);
        td_row(8,"LEFT RIGHT: STOPS");td_row(9,"A: WAIT AND BOARD");td_row(10,"B: BACK");td_row(12,"TRAIN $3 BUS $2");td_row(13,"QUEEN $3 FERRY $4");
        td_row(11,service==TD_TRANSIT_FERRY&&!fare?"RETURN ASSISTANCE":"");
        td_row(15,service==4?"NORMAL QUEEN ROUTE":"UP: BUS/TRAIN AT");td_row(16,service==4?"GAME ROUTE ENDS HERE":"WELLESLEY INTERCHANGE");td_row(17,"SCHEDULES ARE FICTION");return;
    }
    if(td.mode==TD_RESULT){
        td_row(4,td.health && td.left?"CONTRACT DELIVERED":"CONTRACT FAILED");
        if(td.health && td.left){
            /* Match the runtime's split multiply/floor; u and v are existing
               stack locals, not a second stored reward or balance. */
            u=td_job.reward/100*td.health+(td_job.reward%100)*td.health/100;
            v=td.left/5;
            sprintf(td_line,"CONDITION %u%%",td.health);td_row(5,td_line);
            sprintf(td_line,"BASE $%u",td_job.reward);td_row(6,td_line);
            sprintf(td_line,"CONDITION PAY $%u",u);td_row(7,td_line);
            sprintf(td_line,"TIME %uS +$%u",td.left,v);td_row(8,td_line);
            if(td.cash>=td_offer.reward)sprintf(td_line,"CREDIT $%u",td.cash-td_offer.reward);
            else sprintf(td_line,"BALANCE -$%u",td_offer.reward-td.cash);
            td_row(9,td_line);
            td_row(10,td.cash==60000&&(td_offer.reward>td.cash||u+v>td.cash-td_offer.reward)?
                "BALANCE CAP $60000":td.done==TD_QUESTS?"CITY COURIER MASTER":"MORE ROUTES AWAIT");
        }else{
            sprintf(td_line,"CONDITION %u%%",td.health);td_row(5,td_line);
            td_row(6,"NO PAYMENT");
            sprintf(td_line,"TIME %uS",td.left);td_row(7,td_line);
            td_row(8,"");td_row(9,"CREDIT $0");td_row(10,"RETRY OR PICK A JOB");
        }
        sprintf(td_line,"$%u DONE %u/%u",td.cash,td.done,TD_QUESTS);td_row(11,td_line);
        td_row(13,"A: DISPATCH BOARD");td_row(14,"B: FREE ROAM");td_row(16,"PROGRESS AUTO-SAVED");
    }
}
