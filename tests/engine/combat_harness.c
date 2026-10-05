#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gbvm_stubs.h"
#include "td_combat.h"
#include "td_game.h"
#include "td_people.h"
#include "td_motion.h"
#include "td_city_sprites.h"
#include "td_audio.h"
#include "td_hospital.h"
#include "td_district.h"

actor_t actors[22];td_state_t td;
WORD td_vx,td_vy;UBYTE td_walk_dir,td_entry_timer;
static UBYTE district,blocked[TD_DISTRICT_COUNT][122][128],dead[8],last_slot,last_pose,last_direction;
static UBYTE live_foot_blocked;
static UWORD live_u,live_v;
static unsigned checks,saves,finishes,sounds,queries,restore_count;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"Combat check failed at %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
UBYTE td_district_current(void){return district;}
UBYTE td_district_walkable(UBYTE requested,UWORD u,UWORD v){queries++;return requested<TD_DISTRICT_COUNT&&u<1024&&v<976&&!blocked[requested][v>>3][u>>3];}
UBYTE td_road_walkable(UWORD u,UWORD v){return td_district_walkable(district,u,v);}
UBYTE td_motion_foot_clear(UWORD u,UWORD v){return !live_foot_blocked&&td_road_walkable(u>>4,v>>4)&&
    (u!=live_u||v!=live_v);}
void td_audio_play(UBYTE cue){CHECK(cue==TD_AUDIO_IMPACT);sounds++;}
void td_save(void){saves++;}
void td_motion_finish(UBYTE success){CHECK(!success);finishes++;td.job=TD_NONE;td.mode=TD_RESULT;td_save();}
UBYTE td_people_shot(UBYTE slot,BYTE dx,BYTE dy){
    CHECK(slot<8);CHECK(dx>=-1&&dx<=1&&dy>=-1&&dy<=1);CHECK(dx||dy);
    if(dead[slot])return FALSE;
    dead[slot]=1;last_slot=slot;last_direction=dx>0?0:dx<0?1:dy>0?2:3;return TRUE;
}
void td_motion_reset(void){}
void td_player_sprite_restore(void){restore_count++;}
void td_civilian_present(actor_t *actor,UBYTE variant,UBYTE pose){CHECK(actor==&PLAYER&&variant==0);last_pose=pose;}
static void human(UBYTE slot,UWORD u,UWORD v){actors[9+slot].flags=0;actors[9+slot].pos.x=u*32;actors[9+slot].pos.y=v*32;}
static void car(UBYTE index,UWORD u,UWORD v){actors[index].flags=0;actors[index].pos.x=u*32;actors[index].pos.y=v*32;}
static void reset(void){
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));memset(dead,0,sizeof(dead));memset(blocked,0,sizeof(blocked));
    for(unsigned i=0;i<22;i++)actors[i].flags=ACTOR_FLAG_HIDDEN;
    td.u=td.safe_u=320*16;td.v=td.safe_v=320*16;td.mode=TD_ROAM;td.onfoot=1;td.vitality=100;td.ammo=12;
    td.health=73;td.cash=100;td.job=TD_NONE;td.district=district=0;td_walk_dir=0;td_entry_timer=0;
    live_foot_blocked=0;live_u=live_v=65535;
    /* Bounded adapter models the known hospital's eastern building tiles.
       The main engine suite separately uses the complete original raw grid. */
    for(unsigned y=40;y<46;y++)blocked[TD_HOSPITAL_DISTRICT][y][64]=blocked[TD_HOSPITAL_DISTRICT][y][65]=1;
    td_vx=td_vy=0;last_slot=last_pose=last_direction=255;saves=finishes=sounds=queries=restore_count=0;td_combat_reset();
}
static void weapons(void){
    UWORD u,v;UBYTE direction;
    for(UBYTE dir=0;dir<4;dir++)for(UBYTE offset=16;offset<=80;offset+=8){
        reset();td_walk_dir=dir;
        human(0,320+(dir==0?offset:dir==1?-offset:0),320+(dir==2?offset:dir==3?-offset:0));
        CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(dead[0]);CHECK(last_direction==dir);
        CHECK(td.ammo==11&&td.vitality==100&&td.health==73&&td.wanted==2&&td.wanted_left==30);
        CHECK(td_combat_flash(&u,&v,&direction)&&u==320&&v==320&&direction==dir);
        CHECK(td_combat_fire()==TD_COMBAT_BLOCKED);CHECK(td.ammo==11);
        CHECK(!td_combat_update(6));CHECK(!td_combat_flash(&u,&v,&direction));
        CHECK(!td_combat_update(12));CHECK(td_combat_fire()==TD_COMBAT_SHOT&&td.ammo==10);
    }
    reset();human(0,384,320);human(1,352,320);CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(!dead[0]&&dead[1]&&last_slot==1);
    reset();human(0,352,320);human(1,288,320);human(2,352,328);CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(dead[0]&&!dead[1]&&!dead[2]);
    reset();human(0,384,320);car(2,352,320);CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(!dead[0]);
    reset();human(0,384,320);car(8,352,320);CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(!dead[0]);
    reset();human(0,384,320);car(18,352,320);CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(!dead[0]);
    reset();human(0,400,320);blocked[0][40][45]=1;CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(!dead[0]&&td.ammo==11);
    reset();td.v=td.safe_v=327*16;human(0,360,332);blocked[0][40][43]=1;
    CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(!dead[0]);
    reset();human(0,401,320);CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(!dead[0]);
    reset();td.ammo=0;CHECK(td_combat_fire()==TD_COMBAT_EMPTY);CHECK(!td.wanted&&!sounds);
    reset();td.u=td.safe_u=8*16;td.v=td.safe_v=8*16;human(0,1016,8);
    CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(!dead[0]);
    reset();td.u=td.safe_u=1016*16;td.v=td.safe_v=968*16;td_walk_dir=1;human(0,8,968);
    CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(!dead[0]);
    reset();td_entry_timer=1;CHECK(td_combat_fire()==TD_COMBAT_BLOCKED);CHECK(td.ammo==12);
    reset();td.onfoot=0;CHECK(td_combat_fire()==TD_COMBAT_BLOCKED);CHECK(td.ammo==12);
    reset();district=1;CHECK(td_combat_fire()==TD_COMBAT_BLOCKED);
    reset();CHECK(td_combat_fire()==TD_COMBAT_SHOT);td_combat_present();
    CHECK(PLAYER.frame==45&&PLAYER.frame_start==45&&PLAYER.frame_end==46&&PLAYER.anim_tick==255);
    reset();td_walk_dir=4;CHECK(td_combat_fire()==TD_COMBAT_BLOCKED);
    reset();CHECK(!td_combat_flash(NULL,&v,&direction));CHECK(!td_combat_flash(&u,NULL,&direction));CHECK(!td_combat_flash(&u,&v,NULL));
}
static void police(void){
    UWORD u,v;UBYTE dir;
    for(UBYTE heat=0;heat<4;heat++){
        reset();car(4,368,320);td.wanted=heat;
        UBYTE result=td_combat_update(1);
        CHECK(td.vitality==(heat<2?100:heat==2?88:82));CHECK(result==(heat<2?0:TD_COMBAT_CHANGED));
        CHECK(td.health==73);CHECK(td_combat_flash(&u,&v,&dir)==(heat>=2));
        if(heat>=2){CHECK(u==368&&v==320&&dir==1);CHECK(td_combat_locked());CHECK(!td_combat_update(44));CHECK(!td_combat_locked());CHECK(td.vitality==(heat==2?88:82));}
    }
    reset();td.wanted=3;car(4,368,320);blocked[0][40][44]=1;CHECK(!td_combat_update(60));CHECK(td.vitality==100);
    reset();td.wanted=3;car(4,368,320);car(2,344,320);CHECK(!td_combat_update(60));CHECK(td.vitality==100);
    reset();td.u=td.safe_u=8*16;td.v=td.safe_v=8*16;td.wanted=3;car(4,1016,968);
    CHECK(!td_combat_update(60));CHECK(td.vitality==100);
    reset();td.wanted=3;car(4,417,320);CHECK(!td_combat_update(60));CHECK(td.vitality==100);
    reset();td.wanted=3;car(4,368,320);actors[4].flags=ACTOR_FLAG_HIDDEN;CHECK(!td_combat_update(60));CHECK(td.vitality==100);
    reset();td.wanted=3;car(4,368,320);td.onfoot=0;CHECK(!td_combat_update(60));CHECK(td.vitality==100);
    reset();car(4,368,320);CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(!td_combat_police_disabled()&&td.wanted==3);
    CHECK(td_combat_update(18)==TD_COMBAT_CHANGED);CHECK(td_combat_fire()==TD_COMBAT_BLOCKED);CHECK(!td_combat_update(30));
    CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(td_combat_police_disabled());
    UBYTE life=td.vitality;CHECK(!td_combat_update(179));CHECK(td.vitality==life&&td_combat_police_disabled());
    CHECK(td_combat_update(1)==TD_COMBAT_CHANGED);CHECK(!td_combat_police_disabled());
    reset();CHECK(td_combat_fire()==TD_COMBAT_SHOT);CHECK(td_combat_flash(&u,&v,&dir));
    td.wanted=td.wanted_left=0;td_combat_arrested();CHECK(!td_combat_flash(&u,&v,&dir));
    CHECK(td.vitality==100&&td.ammo==11&&td_combat_locked()&&td_combat_fire()==TD_COMBAT_BLOCKED);
    CHECK(PLAYER.frame==32&&PLAYER.frame_start==32&&PLAYER.frame_end==33&&restore_count==1);
    CHECK(!td_combat_update(59)&&td_combat_locked());CHECK(!td_combat_update(1)&&!td_combat_locked());
    CHECK(td_combat_fire()==TD_COMBAT_SHOT);
    reset();td.wanted=2;car(4,368,320);CHECK(td_combat_update(1)==TD_COMBAT_CHANGED);
    CHECK(td_combat_flash(&u,&v,&dir));td.wanted=td.wanted_left=0;td_combat_arrested();
    CHECK(!td_combat_flash(&u,&v,&dir));CHECK(td.vitality==88&&td.ammo==12);
}
static void health(void){
    reset();CHECK(td_combat_damage(12,1,0)==TD_COMBAT_CHANGED);CHECK(td.vitality==88&&td.health==73&&td_combat_locked());
    CHECK(!td_combat_damage(90,1,0));CHECK(td.vitality==88);td_combat_present();CHECK(last_pose==TD_CIVILIAN_HIT);
    CHECK(!td_combat_update(15));CHECK(!td_combat_locked());CHECK(!td_combat_damage(90,1,0));
    CHECK(!td_combat_update(30));CHECK(td_combat_damage(90,1,0)==(TD_COMBAT_CHANGED|TD_COMBAT_DOWN));CHECK(!td.vitality&&td.health==73&&td_combat_locked());
    td_combat_present();CHECK(last_pose==TD_CIVILIAN_PRONE);CHECK(!td_combat_update(119));CHECK(!td.vitality&&!saves);
    CHECK(td_combat_update(1)==(TD_COMBAT_CHANGED|TD_COMBAT_RECOVERED));CHECK(td.vitality==100&&td.ammo==12&&td.cash==60&&!td.wanted&&!td_combat_locked());
    CHECK(td.district==TD_HOSPITAL_DISTRICT&&td.onfoot&&td.u==TD_HOSPITAL_U*16&&td.v==TD_HOSPITAL_V*16&&
          td.safe_u==td.u&&td.safe_v==td.v&&td.msg==25);
    CHECK(saves==1&&!finishes&&restore_count==1);CHECK(!td_combat_update(300));CHECK(td.cash==60&&saves==1);
    reset();td.cash=12;td.job=3;CHECK(td_combat_damage(255,0,1)==3);CHECK(td_combat_update(120)==5);
    CHECK(td.cash==0&&td.job==TD_NONE&&td.mode==TD_RESULT&&finishes==1&&saves==1&&td.health==0&&td.vitality==100);
    reset();CHECK(td_combat_damage(100,0,0)==3);memset(blocked,1,sizeof(blocked));CHECK(!td_combat_update(120));CHECK(!td.vitality&&td.cash==100&&td_combat_locked()&&!saves);
    memset(blocked,0,sizeof(blocked));CHECK(td_combat_update(60)==5);CHECK(td.cash==60&&saves==1);
    reset();td.vitality=0;CHECK(!td_combat_update(600));CHECK(!td.vitality&&td.cash==100&&!saves&&!td_combat_locked());
    reset();td.vitality=0;td_combat_resume_downed();CHECK(td_combat_locked());CHECK(td_combat_update(120)==5);CHECK(td.vitality==100&&td.cash==60&&saves==1);
    reset();td.vitality=10;CHECK(!td_combat_damage(0,0,0));CHECK(td.vitality==10);
}
static void hospital(void){
    for(UBYTE from=0;from<TD_DISTRICT_COUNT;from++){
        reset();td.district=district=from;td.park_district=from;td.park_u=900*16;td.park_v=928*16;td.vehicle=3;
        if(from!=TD_HOSPITAL_DISTRICT){
            /* A solid old district and its full live fleet cannot reject Core. */
            memset(blocked[from],1,sizeof(blocked[from]));live_foot_blocked=1;
        }
        CHECK(td_combat_damage(100,0,0)==3);CHECK(td_combat_update(120)==5);
        CHECK(td.district==TD_HOSPITAL_DISTRICT&&td.u==TD_HOSPITAL_U*16&&td.v==TD_HOSPITAL_V*16&&td.onfoot);
        CHECK(td.park_district==from&&td.park_u==900*16&&td.park_v==928*16&&td.vehicle==3);
        CHECK(td.cash==60&&td.health==73&&td.vitality==100&&td.ammo==12&&saves==1&&!finishes);
        if(from!=TD_HOSPITAL_DISTRICT)CHECK(!td_combat_update(600)&&saves==1&&td.cash==60);
    }
    reset();blocked[0][42][62]=1;
    CHECK(td_district_walkable(0,TD_HOSPITAL_U,TD_HOSPITAL_V));
    CHECK(td_combat_damage(100,0,0)==3&&td_combat_update(120)==5);
    CHECK(td.u==488*16&&td.v==344*16); /* Center-only admission would fail this. */
    reset();live_u=TD_HOSPITAL_U*16;live_v=TD_HOSPITAL_V*16;
    CHECK(td_combat_damage(100,0,0)==3&&td_combat_update(120)==5);
    CHECK(td.u==488*16&&td.v==344*16); /* East+16 must remain solid. */
    reset();td.district=district=3;td.park_district=0;td.park_u=TD_HOSPITAL_U*16;td.park_v=TD_HOSPITAL_V*16;
    CHECK(td_combat_damage(100,0,0)==3&&td_combat_update(120)==5);
    CHECK(td.u==488*16&&td.v==344*16&&td.park_u==504*16&&td.park_v==344*16&&td.park_district==0);
    reset();live_foot_blocked=1;CHECK(td_combat_damage(100,0,0)==3);
    CHECK(!td_combat_update(120)&&td.cash==100&&!saves&&!td.vitality&&td_combat_locked());
    live_foot_blocked=0;CHECK(td_combat_update(60)==5&&td.cash==60&&saves==1);
}
static void frozen(void){
    for(UBYTE mode=TD_PAUSE;mode<=TD_HELP;mode++){
        reset();CHECK(td_combat_damage(100,1,0)==3);td.mode=mode;td.wanted=3;car(4,368,320);
        CHECK(!td_combat_update(600));CHECK(!td_combat_fire());CHECK(!td.vitality&&td_combat_locked()&&td.cash==100&&!saves);
        td.mode=TD_ROAM;CHECK(!td_combat_update(119));CHECK(!td.vitality);CHECK(td_combat_update(1)==5);
    }
    reset();td_combat_fire();td.mode=TD_PAUSE;CHECK(!td_combat_update(600));td.mode=TD_ROAM;CHECK(!td_combat_fire());
    CHECK(!td_combat_update(18));CHECK(td_combat_fire()==TD_COMBAT_SHOT);
    reset();td_combat_arrested();td.mode=TD_PAUSE;CHECK(!td_combat_update(600)&&td_combat_locked());
    td.mode=TD_ROAM;CHECK(!td_combat_update(59)&&td_combat_locked());CHECK(!td_combat_update(1)&&!td_combat_locked());
}
int main(void){weapons();police();health();hospital();frozen();printf("Combat behavior: %u checks passed (actual C, bounded host geometry)\n",checks);return 0;}
