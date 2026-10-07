#pragma bank 255
/* Pop-up HUD timing: what changed, how long each pop-up stays, and when
 * the rows need repainting (td_ui.c builds and paints them). */
#include <string.h>
#include "td_game.h"
#include "td_life.h"
#include "td_hud.h"

UBYTE td_pop_street,td_pop_target,td_pop_cash,td_pop_vit,td_pop_ammo,td_pop_car,td_pop_job,td_still;
char td_street_name[20];
static UWORD td_seen_cash,td_seen_u,td_seen_v,td_street_u,td_street_v,td_hud_seen[6];
static UBYTE td_seen_vit,td_seen_ammo,td_seen_dmg,td_seen_stage,td_seen_job,td_seen_district,td_street_d;

void td_hud_reset(void) BANKED {
    td_pop_street=td_pop_target=td_pop_cash=td_pop_vit=td_pop_ammo=td_pop_car=td_pop_job=td_still=0;
    memset(td_hud_seen,255,sizeof(td_hud_seen));td_seen_cash=td_street_u=65535;
    td_seen_vit=td_seen_ammo=td_seen_dmg=td_seen_stage=td_seen_job=td_seen_district=255;
}
/* Every eighth update while driving or walking: notice what changed, age
 * the pop-ups and ask td_ui.c to repaint only when what could show differs. */
void td_ui_hud_tick(void) BANKED {
    UWORD u=td.u>>4,v=td.v>>4,key[6];
    char name[20];
    if(td.mode!=TD_ROAM)return;
    if(td.cash!=td_seen_cash){if(td_seen_cash!=65535)td_pop_cash=TD_POP_LONG;td_seen_cash=td.cash;}
    if(td.vitality!=td_seen_vit){if(td_seen_vit!=255)td_pop_vit=TD_POP_LONG;td_seen_vit=td.vitality;}
    if(td.ammo!=td_seen_ammo){if(td_seen_ammo!=255)td_pop_ammo=TD_POP_SHORT;td_seen_ammo=td.ammo;}
    if(td_car_damage!=td_seen_dmg){if(td_seen_dmg!=255&&td_car_damage>td_seen_dmg)td_pop_car=TD_POP_LONG;td_seen_dmg=td_car_damage;}
    if(td.job!=td_seen_job||td.stage!=td_seen_stage){
        if(td.job!=TD_NONE){td_pop_job=TD_POP_LONG;td_pop_target=TD_POP_LONG;}
        td_seen_job=td.job;td_seen_stage=td.stage;
    }
    /* A new street (looked up a few times a second) announces itself. */
    if(!(td_tick&24)&&(u!=td_street_u||v!=td_street_v||td.district!=td_street_d)){
        td_get_street(u,v,name);td_street_u=u;td_street_v=v;
        if(td.district!=td_street_d||strcmp(name,td_street_name)){
            if(td_seen_district!=255)td_pop_street=TD_POP_LONG;
            strcpy(td_street_name,name);td_street_d=td.district;td_seen_district=td.district;td_hud_seen[0]=65535;
        }
    }
    if(u==td_seen_u&&v==td_seen_v){if(td_still<255)td_still++;}else{td_still=0;td_seen_u=u;td_seen_v=v;}
    if(td_pop_street)td_pop_street--;
    if(td_pop_target)td_pop_target--;
    if(td_pop_cash)td_pop_cash--;
    if(td_pop_vit)td_pop_vit--;
    if(td_pop_ammo)td_pop_ammo--;
    if(td_pop_car)td_pop_car--;
    if(td_pop_job)td_pop_job--;
    /* Repaint only when something that could show has changed. */
    key[0]=(td_pop_street?1:0)|(td_pop_target?2:0)|(td_pop_cash?4:0)|(td_pop_vit?8:0)|(td_pop_ammo?16:0)|(td_pop_car?32:0)|
           (td_pop_job?64:0)|(td_still>=10?128:0)|((UWORD)td.wanted<<8)|((UWORD)(td_station_near!=TD_NONE)<<11)|((UWORD)td.msg<<12);
    key[1]=td.cash;key[2]=td.vitality|((UWORD)td.ammo<<8);key[3]=td_car_damage|((UWORD)td.health<<8);
    key[4]=td.job==TD_NONE?65535:td.left|((UWORD)td.stage<<10);
    key[5]=td.job==TD_NONE||!td_beacon_shown?0:((td_beacon_u>u?td_beacon_u-u:u-td_beacon_u)+(td_beacon_v>v?td_beacon_v-v:v-td_beacon_v))>>2;
    if(!memcmp(key,td_hud_seen,sizeof(key)))return;
    memcpy(td_hud_seen,key,sizeof(key));
    td_ui_hud_paint();
}
