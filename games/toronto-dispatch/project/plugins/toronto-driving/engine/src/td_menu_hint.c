#pragma bank 255
#include "td_menu_hint.h"
#include "td_game.h"
#include "td_combat.h"
#include "td_motion.h"
#include "td_transit.h"

static UWORD distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static UBYTE near_transit(void){
    td_stop_t stop;UBYTE i;
    for(i=0;i<TD_STOPS;i++){
        td_get_stop(i,&stop);
        if(stop.transit&&stop.district==td.district&&td_transit_can_origin(i)&&
           distance(td.u>>4,stop.u)<15&&distance(td.v>>4,stop.v)<15)return TRUE;
    }
    return FALSE;
}
static const char *hint(void){
    UBYTE trip=td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE;
    if(td.menu>=3&&td.menu<=5&&(!td.vitality||td_combat_locked()))return "RESUME TO RECOVER";
    if(trip&&td.menu>=2&&td.menu<=7)return td.menu==6?"SAVE AFTER THIS TRIP":"WAIT UNTIL TRIP ENDS";
    if(td.menu==3){
        if(td.speed>2||td.speed<-2)return "STOP TO USE CAR DOOR";
        if(td.onfoot&&!td_motion_near_car())return "WALK TO YOUR CAR";
        /* Reuse an actual failed-door/rail notice. Repainting the menu must
         * not sweep terrain or modify engine collision markers. */
        if(td.msg==15)return "DOOR PATH BLOCKED";
        if(td.msg==17)return "MOVE OFF TRAM RAILS";
    }else if(td.menu==4){
        if(td.job!=TD_NONE)return "FINISH OR CANCEL JOB";
        if(td.onfoot)return "ENTER YOUR CAR FIRST";
        if(td.speed>2||td.speed<-2)return "STOP TO CHANGE CAR";
    }else if(td.menu==5){
        if(td_entry_timer)return "FINISH CAR ENTRY";
        if(!td.onfoot)return "PARK THEN WALK";
        if(td.job!=TD_NONE&&(td_job.kind==3||td_job.kind==5))return "DRIVE FOR THIS JOB";
        if(!near_transit())return "WALK TO A TTC STOP";
    }
    return trip?"TTC: MAP/SETTINGS":"SELECT CITY MAP";
}
void td_menu_hint(char *dest) BANKED {
    const char *source=hint();UBYTE i=0;
    while(i<20&&*source)dest[i++]=*source++;
    dest[i]=0;
}
