#pragma bank 255
/* Pop-up HUD timing: what changed, how long each pop-up stays, and when
 * the rows need repainting (td_ui.c builds and paints them). */
#include <string.h>
#include "td_game.h"
#include "td_life.h"
#include "td_hud.h"
#include "td_places.h"

UBYTE td_pop_street,td_pop_target,td_pop_cash,td_pop_vit,td_pop_ammo,td_pop_car,td_pop_job,td_still;
char td_street_name[TD_PLACE_NAME];
static UWORD td_seen_cash,td_seen_u,td_seen_v,td_hud_seen[6];
static UBYTE td_seen_vit,td_seen_ammo,td_seen_dmg,td_seen_stage,td_seen_job,td_seen_district;
/* Name ids of the neighbourhood, landmark and junction last seen (td_ui.c
 * spells the neighbourhood while td_pop_area runs), the street's, and the
 * rank of the place name now showing in td_street_name (td_hud_place). */
UBYTE td_place_ids[3],td_pop_area;
static UBYTE td_street_id,td_pop_rank;

void td_hud_reset(void) BANKED {
    td_pop_street=td_pop_target=td_pop_cash=td_pop_vit=td_pop_ammo=td_pop_car=td_pop_job=td_still=td_pop_area=0;
    memset(td_hud_seen,255,sizeof(td_hud_seen));td_seen_cash=65535;
    td_seen_vit=td_seen_ammo=td_seen_dmg=td_seen_stage=td_seen_job=td_seen_district=255;
    td_street_id=td_place_ids[0]=td_place_ids[1]=td_place_ids[2]=TD_PLACE_NONE;
}
/* Place names in row A by rank: street 0, landmark 1, junction 2 (it is
 * passed in a moment). A name replaces one of lower or equal rank, or any
 * that has been up for TD_PLACE_SHOWN ticks; a landmark that cannot show
 * yet waits (bit 7 of td_pop_rank) while the courier is still near it. */
#define TD_PLACE_SHOWN 8
static UBYTE td_hud_place_busy(UBYTE rank){
    UBYTE cur=td_pop_rank&3;
    if(!td_pop_street||rank>=cur)return FALSE;
    return (UBYTE)((cur==2?TD_POP_JUNCTION:TD_POP_LONG)-td_pop_street)<TD_PLACE_SHOWN;
}
static void td_hud_place(UBYTE rank,UBYTE kind,UBYTE id){
    if(td_hud_place_busy(rank)){if(rank==1)td_pop_rank|=128;return;}
    if(rank)td_get_place_name(kind,id,td_street_name);else td_get_street_name(id,td_street_name);
    td_pop_street=rank==2?TD_POP_JUNCTION:TD_POP_LONG;td_pop_rank=rank;td_hud_seen[0]=65535;
}
/* Where the courier is, for learning the city: entering a neighbourhood,
 * passing a landmark and crossing a junction each name it, checked every
 * sixteenth update while moving (a car passes no junction in less), four
 * updates after a td_ui_hud_tick so the two never share a frame. The neighbourhood has the second row (td_ui.c), so
 * a junction on its boundary still shows; the first look after a scene
 * loads only names the neighbourhood. A new street (looked up a few times a
 * second by td_ui_hud_tick, and once more after stopping) names itself. */
void td_hud_places(void) BANKED {
    UBYTE ids[3];
    if(td.mode!=TD_ROAM||td_still>=2)return;
    ids[0]=td_place_ids[0];td_get_places(td.u>>4,td.v>>4,ids);
    if(ids[1]!=td_place_ids[1]){td_place_ids[1]=ids[1];td_pop_rank&=127;if(ids[1]!=TD_PLACE_NONE&&td_seen_district!=255)td_hud_place(1,TD_PLACE_MARK,ids[1]);}
    if(ids[2]!=td_place_ids[2]){td_place_ids[2]=ids[2];if(ids[2]!=TD_PLACE_NONE&&td_seen_district!=255)td_hud_place(2,TD_PLACE_JUNCTION,ids[2]);}
    if(ids[0]!=td_place_ids[0]){td_place_ids[0]=ids[0];td_pop_area=ids[0]!=TD_PLACE_NONE?TD_POP_LONG:0;td_hud_seen[0]=65535;}
    td_seen_district=td.district;
}
/* Every eighth update while driving or walking: notice what changed, age
 * the pop-ups and ask td_ui.c to repaint only when what could show differs. */
void td_ui_hud_tick(void) BANKED {
    UWORD u=td.u>>4,v=td.v>>4,key[6];
    UBYTE ids[1];
    if(td.mode!=TD_ROAM)return;
    if(td.cash!=td_seen_cash){if(td_seen_cash!=65535)td_pop_cash=TD_POP_LONG;td_seen_cash=td.cash;}
    if(td.vitality!=td_seen_vit){if(td_seen_vit!=255)td_pop_vit=TD_POP_LONG;td_seen_vit=td.vitality;}
    if(td.ammo!=td_seen_ammo){if(td_seen_ammo!=255)td_pop_ammo=TD_POP_SHORT;td_seen_ammo=td.ammo;}
    if(td_car_damage!=td_seen_dmg){if(td_seen_dmg!=255&&td_car_damage>td_seen_dmg)td_pop_car=TD_POP_LONG;td_seen_dmg=td_car_damage;}
    if(td.job!=td_seen_job||td.stage!=td_seen_stage){
        if(td.job!=TD_NONE){td_pop_job=TD_POP_LONG;td_pop_target=TD_POP_LONG;}
        td_seen_job=td.job;td_seen_stage=td.stage;
    }
    if(u==td_seen_u&&v==td_seen_v){if(td_still<255)td_still++;}else{td_still=0;td_seen_u=u;td_seen_v=v;}
    if(!(td_tick&24)&&td_still<4){
        ids[0]=td_get_street(u,v);
        if(ids[0]!=td_street_id){
            if(td_street_id!=TD_PLACE_NONE)td_hud_place(0,0,ids[0]);
            td_street_id=ids[0];
        }
    }
    if(td_pop_street)td_pop_street--;
    if((td_pop_rank&128)&&!td_hud_place_busy(1))td_hud_place(1,TD_PLACE_MARK,td_place_ids[1]);
    if(td_pop_area&&!--td_pop_area)td_hud_seen[0]=65535;
    if(td_pop_target)td_pop_target--;
    if(td_pop_cash)td_pop_cash--;
    if(td_pop_vit)td_pop_vit--;
    if(td_pop_ammo)td_pop_ammo--;
    if(td_pop_car)td_pop_car--;
    if(td_pop_job)td_pop_job--;
    /* Repaint only when something that could show has changed. */
    key[0]=(td_pop_street?1:0)|(td_pop_target?2:0)|(td_pop_cash?4:0)|(td_pop_vit?8:0)|(td_pop_ammo?16:0)|(td_pop_car?32:0)|
           (td_pop_job?64:0)|(TD_HUD_IDLE()?128:0)|((UWORD)td.wanted<<8)|((UWORD)(td_station_near!=TD_NONE)<<11)|((UWORD)td.msg<<12);
    key[1]=td.cash;key[2]=td.vitality|((UWORD)td.ammo<<8);key[3]=td_car_damage|((UWORD)td.health<<8);
    key[4]=td.job==TD_NONE?65535:td.left|((UWORD)td.stage<<10);
    key[5]=td.job==TD_NONE||!td_beacon_shown?0:((td_beacon_u>u?td_beacon_u-u:u-td_beacon_u)+(td_beacon_v>v?td_beacon_v-v:v-td_beacon_v))>>2;
    if(!memcmp(key,td_hud_seen,sizeof(key)))return;
    memcpy(td_hud_seen,key,sizeof(key));
    td_ui_hud_paint();
}
