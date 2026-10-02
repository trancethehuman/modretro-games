#pragma bank 255
#include <string.h>
#include "states/TORONTO.h"
#include "td_game.h"
#include "actor.h"
#include "camera.h"
#include "scroll.h"
#include "collision.h"
#include "input.h"
#include "data_manager.h"
#include "ui.h"
#include "compat.h"
#include "system.h"

td_state_t td;
td_job_t td_job,td_offer;
td_stop_t td_target,td_cursor;
static const BYTE td_dx[]={16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6,0,6,11,15};
static const BYTE td_dy[]={0,6,11,15,16,15,11,6,0,-6,-11,-15,-16,-15,-11,-6};
static const UWORD td_rows[]={64,176,288,400,528,640,720,784};
static const UWORD td_cols[]={80,208,336,480,560,640,720,816,944};
static const UBYTE td_train[]={0,12,13,14,15,16,17};
static const UBYTE td_bus[]={18,16,19};
static const UBYTE td_ferry[]={10,20,21,22};
static UWORD td_traffic_u[6],td_traffic_v[6];
static UBYTE td_tick,td_notice_timer,td_red_cooldown,td_turn_tick,td_entry_timer,td_entry_target,td_walk_dir,td_resume_mode;
static WORD td_vx,td_vy;
static UWORD td_last_frame;

static UWORD td_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static void td_position(actor_t *a,UWORD u,UWORD v){
    // GBVM actors use five fractional bits; driving state uses four.
    a->pos.x=u*32; a->pos.y=v*32;
}
static void td_frame(actor_t *a,UBYTE f){actor_set_frames(a,f,f+1);a->anim_tick=255;}
static void td_message(UBYTE m){td.msg=m;td_notice_timer=90;td_ui_draw();}
static UBYTE td_near(td_stop_t *s){return td_distance(td.u>>4,s->u)<15&&td_distance(td.v>>4,s->v)<15;}
static UBYTE td_drivable(UWORD u,UWORD v){
    if(u<8||v<8||u>1016||v>968)return FALSE;
    return !tile_at((u-5)>>3,(v-5)>>3)&&!tile_at((u+5)>>3,(v-5)>>3)&&!tile_at((u-5)>>3,(v+5)>>3)&&!tile_at((u+5)>>3,(v+5)>>3);
}
static UBYTE td_walkable(UWORD u,UWORD v){
    if(u>=1024||v>=976)return FALSE;
    return !(tile_at(u>>3,v>>3)&15);
}
static UBYTE td_near_car(void){return td_distance(td.u,td.park_u)<384&&td_distance(td.v,td.park_v)<384;}
static void td_enter_exit(void){
    if(td.speed>2||td.speed<-2){td_message(11);return;}
    if(td.onfoot){
        if(!td_near_car()){td_message(9);return;}
        td_entry_target=0;
    }else{
        td.park_u=td.u;td.park_v=td.v;td.onfoot=1;td_entry_target=1;
        // Choose a visible, walkable door side without spawning inside a building.
        if(td_walkable(td.u>>4,(td.v>>4)+18))td.v+=288;
        else if(td_walkable((td.u>>4)+18,td.v>>4))td.u+=288;
        else if(td_walkable(td.u>>4,(td.v>>4)-18))td.v-=288;
        else td.u-=288;
    }
    td.speed=0;td_vx=td_vy=0;td_entry_timer=12;td.mode=TD_ROAM;td_save();td_ui_draw();
}
void td_save(void) BANKED {
    UBYTE i,check=0;const UBYTE *src=(const UBYTE*)&td;volatile UBYTE *ram=(volatile UBYTE*)0xA100;
    // GB Studio owns SRAM banks 0-2. This scene owns a versioned record in bank 3.
    ENABLE_RAM_MBC5;SWITCH_RAM_BANK(3,RAM_BANKS_ONLY);
    ram[0]=0;for(i=0;i<sizeof(td);i++){ram[4+i]=src[i];check^=src[i];}
    ram[1]=0xD7;ram[2]=4;ram[3]=check;ram[0]=0x54;
    SWITCH_RAM_BANK(0,RAM_BANKS_ONLY);
}
static UBYTE td_restore(void){
    UBYTE i,check=0,valid;UBYTE *dst=(UBYTE*)&td;volatile UBYTE *ram=(volatile UBYTE*)0xA100;
    ENABLE_RAM_MBC5;SWITCH_RAM_BANK(3,RAM_BANKS_ONLY);
    valid=ram[0]==0x54&&ram[1]==0xD7&&ram[2]==4;
    if(valid){for(i=0;i<sizeof(td);i++){dst[i]=ram[4+i];check^=dst[i];}valid=check==ram[3];}
    SWITCH_RAM_BANK(0,RAM_BANKS_ONLY);
    if(valid && (td.vehicle>3||td.job!=TD_NONE&&td.job>=TD_QUESTS||td.done>72||td.u>16384||td.v>15616))valid=0;
    return valid;
}
void td_set_target(void) BANKED {
    if(td.job!=TD_NONE)td_get_stop(td_job.route[td.stage],&td_target);
    else td_get_stop(0,&td_target);
    td_position(&actors[1],td_target.u,td_target.v-12);
}
static void td_finish(UBYTE success){
    if(success){
        if(!(td.complete[td.job>>3]&(1<<(td.job&7)))){td.complete[td.job>>3]|=1<<(td.job&7);td.done++;}
        if(td.cash<50000)td.cash+=td_job.reward+(td.left/5);
    }
    td.job=TD_NONE;td.speed=0;td_vx=td_vy=0;td.mode=TD_RESULT;td_save();td_ui_draw();
}
static void td_interact(void){
    if(td.speed>2||td.speed<-2){td_message(1);return;}
    if(td.job==TD_NONE){td.mode=TD_BOARD;td.menu=0;td_get_job(td.menu,&td_offer);td_ui_draw();return;}
    if(!td_near(&td_target)){td_message(6);return;}
    if(td_job.vehicle!=TD_NONE&&(td.onfoot||td.vehicle!=td_job.vehicle)){td_message(2);return;}
    td.stage++;
    if(td.stage==td_job.count){td_finish(TRUE);return;}
    td_set_target();td_save();td_ui_draw();
}
static UBYTE td_origin(void){
    UBYTE i;td_stop_t s;
    for(i=0;i<TD_STOPS;i++){td_get_stop(i,&s);if(s.transit&&td_near(&s))return i;}
    return TD_NONE;
}
UBYTE td_service(UBYTE origin) BANKED {
    if(origin>=64||origin==18||origin==19)return 2;
    if(origin==10||origin>=20)return 3;
    return 1;
}
static UBYTE td_stop_index(UBYTE origin,UBYTE stop){
    UBYTE i,service=td_service(origin),count=service==1?7:service==2?3:4;
    const UBYTE *route=service==1?td_train:service==2?td_bus:td_ferry;
    for(i=0;i<count;i++)if(route[i]==stop)return i;
    return 0;
}
UBYTE td_next_departure(UBYTE origin,UWORD seconds) BANKED {
    UBYTE service=td_service(origin),period=service==1?18:service==2?24:30;
    UBYTE phase=td_stop_index(origin,origin&63)*(service==1?2:service==2?4:7);
    UBYTE elapsed=(seconds+period-phase)%period;
    return elapsed<2?0:period-elapsed;
}
static UBYTE td_route_stop(UBYTE origin,UBYTE idx){
    if(td_service(origin)==3)return td_ferry[idx%4];
    if(td_service(origin)==2)return td_bus[idx%3];
    return td_train[idx%7];
}
static void td_transit_open(void){
    UBYTE origin;
    if(!td.onfoot){td_message(9);return;}
    if(td.job!=TD_NONE&&(td_job.kind==3||td_job.kind==5)){td_message(8);return;}
    origin=td_origin();if(origin==TD_NONE){td_message(6);return;}
    td.transit_origin=origin;td.menu=0;td.transit_target=td_route_stop(origin,0);td_get_stop(td.transit_target,&td_cursor);td.mode=TD_TRANSIT;td_ui_draw();
}
static void td_pause_choose(void){
    if((td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)&&td.menu>1){td_message(2);return;}
    switch(td.menu){
        case 0:td.mode=td_resume_mode;break;
        case 1:td.mode=TD_MAP;td.map_x=camera_x;td.map_y=camera_y;camera_settings=0;break;
        case 2:td.mode=TD_BOARD;td.menu=td.job==TD_NONE?0:td.job;td_get_job(td.menu,&td_offer);break;
        case 3:td_enter_exit();return;
        case 4:
            if(td.job!=TD_NONE||td.speed>2||td.speed<-2||td.onfoot){td_message(2);return;}
            td.vehicle=(td.vehicle+1)&3;td_save();break;
        case 5:td_transit_open();return;
        case 6:td_save();td.mode=TD_ROAM;td_message(12);break;
        case 7:td.job=TD_NONE;td.speed=0;td.mode=TD_ROAM;td_set_target();td_save();break;
    }
    td_ui_draw();
}
static void td_menu_update(void){
    if(td.mode==TD_HELP){if(INPUT_A_PRESSED||INPUT_B_PRESSED){td.mode=TD_ROAM;td_ui_draw();}return;}
    if(td.mode==TD_MAP){
        if(INPUT_LEFT&&camera_x>160)camera_x-=48;
        if(INPUT_RIGHT&&camera_x<(image_width-80)*32)camera_x+=48;
        if(INPUT_UP&&camera_y>160)camera_y-=48;
        if(INPUT_DOWN&&camera_y<(image_height-72)*32)camera_y+=48;
        if(INPUT_A_PRESSED){camera_x=actors[1].pos.x;camera_y=actors[1].pos.y;}
        if(INPUT_B_PRESSED||INPUT_START_PRESSED){camera_settings=CAMERA_LOCK_FLAG;td.mode=TD_PAUSE;td.menu=1;td_ui_draw();}return;
    }
    if(INPUT_B_PRESSED||INPUT_START_PRESSED){td.mode=td.mode==TD_PAUSE?td_resume_mode:TD_ROAM;camera_settings=CAMERA_LOCK_FLAG;td_ui_draw();return;}
    if(td.mode==TD_PAUSE){
        if(INPUT_DOWN_PRESSED)td.menu=(td.menu+1)%8;
        if(INPUT_UP_PRESSED)td.menu=(td.menu+7)%8;
        if(INPUT_A_PRESSED){td_pause_choose();return;}
    }else if(td.mode==TD_BOARD){
        if(INPUT_RIGHT_PRESSED){td.menu=(td.menu+1)%TD_QUESTS;td_get_job(td.menu,&td_offer);}
        if(INPUT_LEFT_PRESSED){td.menu=(td.menu+TD_QUESTS-1)%TD_QUESTS;td_get_job(td.menu,&td_offer);}
        if(INPUT_A_PRESSED){
            if(td.job!=TD_NONE){td_message(2);return;}
            if(td.done<td_offer.min_done){td_message(3);return;}
            if(td_offer.vehicle!=TD_NONE&&td.vehicle!=td_offer.vehicle){td_message(2);return;}
            td.job=td.menu;td_job=td_offer;td.stage=0;td.health=100;td.left=td_job.seconds;td.mode=TD_ROAM;td_set_target();td_save();td_ui_draw();return;
        }
    }else if(td.mode==TD_TRANSIT){
        if((INPUT_UP_PRESSED||INPUT_DOWN_PRESSED)&&(td.transit_origin&63)==16){
            td.transit_origin^=64;td.menu=0;td.transit_target=td_route_stop(td.transit_origin,0);td_get_stop(td.transit_target,&td_cursor);
        }
        if(INPUT_RIGHT_PRESSED){UBYTE count=td_service(td.transit_origin)==1?7:td_service(td.transit_origin)==2?3:4;td.menu=(td.menu+1)%count;td.transit_target=td_route_stop(td.transit_origin,td.menu);td_get_stop(td.transit_target,&td_cursor);}
        if(INPUT_LEFT_PRESSED){UBYTE count=td_service(td.transit_origin)==1?7:td_service(td.transit_origin)==2?3:4;td.menu=(td.menu+count-1)%count;td.transit_target=td_route_stop(td.transit_origin,td.menu);td_get_stop(td.transit_target,&td_cursor);}
        if(INPUT_A_PRESSED){
            if(td.transit_target==(td.transit_origin&63))return;
            td.mode=TD_WAIT;td.speed=0;td_ui_draw();return;
        }
    }else if(td.mode==TD_RESULT&&INPUT_A_PRESSED){td.mode=TD_BOARD;td.menu=0;td_get_job(0,&td_offer);}
    if(INPUT_A_PRESSED||INPUT_UP_PRESSED||INPUT_DOWN_PRESSED||INPUT_LEFT_PRESSED||INPUT_RIGHT_PRESSED)td_ui_draw();
}
static void td_second(void){
    UBYTE service,fare,steps; td_stop_t origin;
    td.seconds++;
    if(td.job!=TD_NONE){if(td.left)td.left--;if(!td.left){td.health=0;td_finish(FALSE);return;}}
    if(td.mode==TD_WAIT){
        td_get_stop(td.transit_origin&63,&origin);service=td_service(td.transit_origin);
        if(!td_next_departure(td.transit_origin,td.seconds)){
            fare=service==1?3:service==2?2:4;
            if(td.cash<fare){td.mode=TD_ROAM;td_message(4);return;}
            steps=td_distance(td_stop_index(td.transit_origin,td.transit_target),td_stop_index(td.transit_origin,td.transit_origin&63));
            td.cash-=fare;td.mode=TD_RIDE;td.ride_left=service==3?8:service==2?2+steps*2:1+steps/2;
        }
    }else if(td.mode==TD_RIDE){
        if(td.ride_left)td.ride_left--;
        if(!td.ride_left){td.u=td_cursor.u*16;td.v=td_cursor.v*16;td.safe_u=td.u;td.safe_v=td.v;td.mode=TD_ROAM;td_save();}
    }
    td_ui_draw();
}
static void td_traffic(void){
    UBYTE i,phase=td.seconds%12;UWORD u,v;
    for(i=0;i<6;i++){
        u=td_traffic_u[i];v=td_traffic_v[i];
        if(i<4){if(phase<7||u%128<82||u%128>98)u+=8;if(u>848*16)u=40*16;}
        else if(i==4){if(phase>=7||v%128<82||v%128>98)v+=8;if(v>792*16)v=40*16;}
        else {u=80*16+(td.seconds%24)*32*16;v=176*16;}
        td_traffic_u[i]=u;td_traffic_v[i]=v;td_position(&actors[i+2],u>>4,v>>4);
        td_frame(&actors[i+2],i==5?8:(i==4?2:0));
        if(!td.onfoot&&td_distance(td.u,u)<180&&td_distance(td.v,v)<180&&!td.cooldown){
            td.speed/=2;td_vx/=2;td_vy/=2;td.cooldown=60;
            if(td.job!=TD_NONE){td.health=td.health>12?td.health-12:0;if(!td.health){td_finish(FALSE);return;}}td_message(5);
        }
    }
    td_position(&actors[8],td.park_u>>4,td.park_v>>4);td_frame(&actors[8],td_entry_timer?44:td.vehicle*8+((td.heading+1)&15)/2);
    if(td.onfoot)actors[8].flags&=~ACTOR_FLAG_HIDDEN;else actors[8].flags|=ACTOR_FLAG_HIDDEN;
}
static void td_pedestrians(void){
    UBYTE i,row=0,dir;UWORD distance=65535,test,u,v,segment=(td.u>>4)&0xFF80,phase;
    // Deterministic sidewalk routes continue on the world clock, including while off-screen.
    for(i=0;i<8;i++){test=td_distance(td.v>>4,td_rows[i]);if(test<distance){distance=test;row=i;}}
    for(i=0;i<6;i++){
        phase=(td.seconds*12+td.subsecond/5+i*37)%160;dir=phase<80?0:1;
        u=segment+(phase<80?phase:160-phase)+i%3*16;
        v=td_rows[row]+(i<3?24:-24);
        td_position(&actors[9+i],u,v);td_frame(&actors[9+i],32+dir*2+((td_tick>>3)&1));
        if(td_walkable(u,v)&&td_distance(td.u>>4,u)<112&&td_distance(td.v>>4,v)<96)actors[9+i].flags&=~ACTOR_FLAG_HIDDEN;
        else actors[9+i].flags|=ACTOR_FLAG_HIDDEN;
        if(!td.onfoot&&!td.cooldown&&td_distance(td.u>>4,u)<10&&td_distance(td.v>>4,v)<10){td.speed/=2;td_vx/=2;td_vy/=2;td.cooldown=45;td_message(13);}
    }
}
static void td_drive(void){
    WORD nu,nv,target_x,target_y;UBYTE limit,turn_period,moving=0,slide=0;UWORD u,v;
    if(td.cooldown)td.cooldown--;
    if(td_red_cooldown)td_red_cooldown--;
    if(td_entry_timer){
        if(!td_entry_target){td.u=(td.u*3+td.park_u)/4;td.v=(td.v*3+td.park_v)/4;}
        if(!--td_entry_timer){if(!td_entry_target){td.u=td.park_u;td.v=td.park_v;td.onfoot=0;}td_save();}
        td_frame(&PLAYER,td.onfoot?32:td.vehicle*8+((td.heading+1)&15)/2);return;
    }
    if(td.onfoot){
        nu=td.u;nv=td.v;
        if(INPUT_RIGHT){nu+=INPUT_UP||INPUT_DOWN?6:8;td_walk_dir=0;moving=1;}
        if(INPUT_LEFT){nu-=INPUT_UP||INPUT_DOWN?6:8;td_walk_dir=1;moving=1;}
        if(INPUT_DOWN){nv+=INPUT_LEFT||INPUT_RIGHT?6:8;td_walk_dir=2;moving=1;}
        if(INPUT_UP){nv-=INPUT_LEFT||INPUT_RIGHT?6:8;td_walk_dir=3;moving=1;}
        if(td_walkable(nu>>4,nv>>4)&&!(td_distance(nu,td.park_u)<144&&td_distance(nv,td.park_v)<144)){td.u=nu;td.v=nv;}
        td.speed=0;td_frame(&PLAYER,32+td_walk_dir*2+(moving?((td_tick>>3)&1):0));
        if(INPUT_A_PRESSED&&td_near_car())td_enter_exit();
        if(INPUT_B_PRESSED)td_transit_open();
        return;
    }
    limit=td.vehicle==1?20:td.vehicle==2?28:td.vehicle==3?18:24;
    turn_period=td.speed>18?10:td.speed>8?8:5;
    if(td.vehicle==1)turn_period+=2;
    // A dedicated yaw counter keeps turns regular as speed changes.
    if(INPUT_LEFT||INPUT_RIGHT){if(++td_turn_tick>=turn_period){td_turn_tick=0;if(INPUT_LEFT)td.heading=(td.heading+15)&15;else td.heading=(td.heading+1)&15;}}
    else td_turn_tick=0;
    if(INPUT_B){if(td_tick%2==0&&td.speed>-6)td.speed--;}
    else if(INPUT_A){if(td_tick%4==0&&td.speed<limit)td.speed++;}
    else if(td_tick%8==0){if(td.speed>0)td.speed--;else if(td.speed<0)td.speed++;}
    // Traction eases velocity toward heading instead of instantly rotating momentum.
    target_x=td_dx[td.heading]*td.speed;target_y=td_dy[td.heading]*td.speed;
    td_vx+=(target_x-td_vx)/4;td_vy+=(target_y-td_vy)/4;
    if(!td.speed){td_vx/=2;td_vy/=2;}
    nu=td.u+td_vx/16;nv=td.v+td_vy/16;u=nu>>4;v=nv>>4;
    if(td_drivable(u,v)){
        if(!td_red_cooldown&&td.speed>6&&td_distance(u,640)<14&&td_distance(v,528)<14&&
          (td_distance(td.u>>4,640)>=14||td_distance(td.v>>4,528)>=14)){
            UBYTE ax=td_dx[td.heading]<0?-td_dx[td.heading]:td_dx[td.heading];
            UBYTE ay=td_dy[td.heading]<0?-td_dy[td.heading]:td_dy[td.heading];
            if((ax>ay&&td.seconds%12>=7)||(ax<=ay&&td.seconds%12<7)){if(td.cash>=5)td.cash-=5;td_red_cooldown=120;td_message(7);}
        }
        td.u=nu;td.v=nv;
    }else{
        // A glancing curb contact slides along the free axis and preserves forward speed.
        if(td_drivable(nu>>4,td.v>>4)){td.u=nu;td_vy=0;slide=1;}
        if(td_drivable(td.u>>4,nv>>4)){td.v=nv;td_vx=0;slide=1;}
        if(slide){if(td.speed>8)td.speed--;}
        else{
            if(td.speed>8&&!td.cooldown){if(td.job!=TD_NONE){UBYTE damage=td_job.kind==1?20:8;td.health=td.health>damage?td.health-damage:0;}td.cooldown=45;td_message(5);}
            td.speed=0;td_vx=td_vy=0;
        }
    }
    td.safe_u=td.u;td.safe_v=td.v;
    td_frame(&PLAYER,td.vehicle*8+((td.heading+1)&15)/2);
    if(td.job!=TD_NONE&&!td.health)td_finish(FALSE);
}
void toronto_init(void) BANKED {
    UBYTE i;td_tick=td_notice_timer=td_red_cooldown=td_entry_timer=td_turn_tick=0;td_vx=td_vy=0;td_last_frame=sys_time;
    if(!td_restore()){
        memset(&td,0,sizeof(td));td.u=560*16;td.v=720*16;td.park_u=td.u;td.park_v=td.v;td.cash=30;td.job=TD_NONE;td.heading=0;td.health=100;
    }
    td.speed=0;td_resume_mode=TD_ROAM;td.mode=TD_HELP;td.msg=0;td.menu=0;td.safe_u=td.u;td.safe_v=td.v;
    if(td.job!=TD_NONE){td_get_job(td.job,&td_job);if(td.stage>=td_job.count)td.job=TD_NONE;}
    actors_len=15;
    for(i=1;i<15;i++){
        actors[i]=PLAYER;actors[i].prev=actors[i].next=NULL;actors[i].flags=ACTOR_FLAG_PERSISTENT;actors[i].collision_group=0;actors[i].script.bank=actors[i].script_update.bank=0;
        // Place copied actors on the inactive list before activating them.
        actors[i].next=actors_inactive_head; if(actors_inactive_head)actors_inactive_head->prev=&actors[i];actors_inactive_head=&actors[i];
        activate_actor(&actors[i]);
    }
    for(i=0;i<6;i++){td_traffic_u[i]=(80+i*120)*16;td_traffic_v[i]=(i<4?td_rows[2+i]:i==4?240:176)*16;}
    td_frame(&actors[1],40);td_set_target();td_position(&PLAYER,td.u>>4,td.v>>4);
    camera_settings=CAMERA_LOCK_FLAG;camera_offset_x=0;camera_offset_y=-16;camera_deadzone_x=8;camera_deadzone_y=8;
    td_ui_init();
}
void toronto_update(void) BANKED {
    UWORD now=sys_time;UBYTE elapsed=(UBYTE)(now-td_last_frame),step;td_last_frame=now;
    if(elapsed>4)elapsed=4;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE){td_menu_update();return;}
    if(INPUT_START_PRESSED){td_resume_mode=td.mode;td.mode=TD_PAUSE;td.menu=0;td_ui_draw();return;}
    if(td_notice_timer){if(!--td_notice_timer){td.msg=0;td_ui_draw();}}
    td.subsecond+=elapsed;if(td.subsecond>=60){td.subsecond-=60;td_second();}
    if(td.mode==TD_WAIT&&INPUT_B_PRESSED){td.mode=TD_ROAM;td_ui_draw();}
    if(td.mode==TD_ROAM){for(step=0;step<elapsed&&td.mode==TD_ROAM;step++){td_tick++;td_drive();td_traffic();}if(INPUT_SELECT_PRESSED)td_interact();td_pedestrians();}
    td_position(&PLAYER,td.u>>4,td.v>>4);
}
