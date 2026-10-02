#pragma bank 255
#include <string.h>
#include "states/TORONTO.h"
#include "td_game.h"
#include "td_world_routes.h"
#include "td_audio.h"
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
static UBYTE td_traffic_leg[6],td_ped_route[6],td_ped_refresh;
static UWORD td_ped_anchor_u,td_ped_anchor_v;
static UBYTE td_tick,td_notice_timer,td_red_cooldown,td_turn_tick,td_entry_timer,td_entry_target,td_walk_dir,td_resume_mode;
static WORD td_vx,td_vy;
static UWORD td_last_frame;
static UBYTE td_corner_used;
static UBYTE td_input_edge;
static UBYTE td_save_slot=TD_NONE,td_save_seq;

static UWORD td_distance(UWORD a,UWORD b){return a>b?a-b:b-a;}
static void td_position(actor_t *a,UWORD u,UWORD v){
    // GBVM actors use five fractional bits; driving state uses four.
    a->pos.x=u*32; a->pos.y=v*32;
}
static void td_frame(actor_t *a,UBYTE f){if(a->frame_start!=f||a->frame_end!=f+1)actor_set_frames(a,f,f+1);a->anim_tick=255;}
static void td_message(UBYTE m){td.msg=m;td_notice_timer=90;if(m==5||m==13)td_audio_play(TD_AUDIO_IMPACT);td_ui_draw();}
static void td_sound_update(void){td_audio_update(td.speed,td.vehicle,td.onfoot,!!INPUT_B,td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE);}
static UBYTE td_near(td_stop_t *s){return td_distance(td.u>>4,s->u)<15&&td_distance(td.v>>4,s->v)<15;}
static UBYTE td_drivable(UWORD u,UWORD v){
    UBYTE x,y,left,right,top,bottom;
    if(u<8||v<8||u>1016||v>968)return FALSE;
    left=(u-5)>>3;right=(u+5)>>3;top=(v-5)>>3;bottom=(v+5)>>3;
    /* Eleven pixels can overlap three tile rows/columns: corners alone miss rails. */
    for(y=top;y<=bottom;y++)for(x=left;x<=right;x++)if(tile_at(x,y))return FALSE;
    return TRUE;
}
static UBYTE td_walkable(UWORD u,UWORD v){
    if(u>=1024||v>=976)return FALSE;
    return !(tile_at(u>>3,v>>3)&15);
}
static UBYTE td_traffic_free(UWORD u,UWORD v){
    UBYTE i;
    for(i=0;i<6;i++)if(td_distance(u,td_traffic_u[i])<168&&td_distance(v,td_traffic_v[i])<168)return FALSE;
    return TRUE;
}
static UBYTE td_foot_free(UWORD u,UWORD v){
    return td_walkable(u>>4,v>>4)&&!(td_distance(u,td.park_u)<168&&td_distance(v,td.park_v)<168)&&td_traffic_free(u,v);
}
static UBYTE td_near_car(void){return td_distance(td.u,td.park_u)<384&&td_distance(td.v,td.park_v)<384;}
static UBYTE td_door_path(UWORD u,UWORD v,UWORD car_u,UWORD car_v){
    UBYTE i;WORD du=(WORD)car_u-(WORD)u,dv=(WORD)car_v-(WORD)v;
    /* A one-pixel sample catches rails and corners between usable endpoints. */
    for(i=0;i<=24;i++)if(!td_walkable((u+du*i/24)>>4,(v+dv*i/24)>>4))return FALSE;
    return TRUE;
}
static void td_enter_exit(void){
    UWORD door_u=td.u,door_v=td.v;UBYTE found=0,i;
    if(td.speed>2||td.speed<-2){td_message(11);return;}
    if(td.onfoot){
        if(!td_near_car()){td_message(9);return;}
        if(!td_door_path(td.u,td.v,td.park_u,td.park_v)){td_message(15);return;}
        td_entry_target=0;
    }else{
        // Choose a visible, walkable door side without spawning inside a building.
        for(i=0;i<4;i++){
            door_u=td.u;door_v=td.v;
            if(i==0)door_v+=288;else if(i==1)door_u+=288;else if(i==2)door_v-=288;else door_u-=288;
            if(td_door_path(td.u,td.v,door_u,door_v)&&td_traffic_free(door_u,door_v)){found=1;break;}
        }
        if(!found){td_message(15);return;}
        td.park_u=td.u;td.park_v=td.v;td.u=door_u;td.v=door_v;td.onfoot=1;td_entry_target=1;
    }
    td.speed=0;td_vx=td_vy=0;td_entry_timer=12;td.mode=TD_ROAM;td_save();td_ui_draw();
}
static UWORD td_crc_byte(UWORD crc,UBYTE value){
    UBYTE bit;crc^=(UWORD)value<<8;
    for(bit=0;bit<8;bit++)crc=(crc&0x8000)?(crc<<1)^0x1021:crc<<1;
    return crc;
}
static volatile UBYTE *td_save_address(UBYTE slot){return slot?(volatile UBYTE*)0xA180:(volatile UBYTE*)0xA100;}
static UBYTE td_valid_state(td_state_t *s){
    UBYTE i,bits=0,value;td_job_t job;td_stop_t stop;
    if(s->vehicle>3||s->heading>15||s->onfoot>1||s->health>100||s->subsecond>=60||s->mode>TD_HELP)return FALSE;
    if(s->u>=1024*16||s->v>=976*16||s->park_u>=1024*16||s->park_v>=976*16)return FALSE;
    if(!td_drivable(s->park_u>>4,s->park_v>>4)||!(s->onfoot?td_walkable(s->u>>4,s->v>>4):td_drivable(s->u>>4,s->v>>4)))return FALSE;
    for(i=0;i<9;i++){value=s->complete[i];while(value){bits+=value&1;value>>=1;}}
    if(bits!=s->done)return FALSE;
    if(s->job!=TD_NONE){if(s->job>=TD_QUESTS)return FALSE;td_get_job(s->job,&job);if(s->stage>=job.count||!s->left||!s->health)return FALSE;}
    if(s->mode==TD_WAIT||s->mode==TD_RIDE){
        if(!s->onfoot||(s->transit_origin&128)||(s->transit_origin&63)>=TD_STOPS||s->transit_target>=TD_STOPS||s->transit_target==(s->transit_origin&63))return FALSE;
        if((s->transit_origin&64)&&(s->transit_origin&63)!=16)return FALSE;
        td_get_stop(s->transit_origin&63,&stop);if(!stop.transit)return FALSE;
        /* Explicitly validate route membership; an unknown stop must not become index zero. */
        value=td_service(s->transit_origin);bits=0;
        if(value==1){for(i=0;i<7;i++)if(td_train[i]==(s->transit_origin&63))bits=1;}
        else if(value==2){for(i=0;i<3;i++)if(td_bus[i]==(s->transit_origin&63))bits=1;}
        else bits=(s->transit_origin&63)==10||((s->transit_origin&63)>=20&&(s->transit_origin&63)<=22);
        if(!bits)return FALSE;
        bits=0;
        if(value==1){for(i=0;i<7;i++)if(td_train[i]==s->transit_target)bits=1;}
        else if(value==2){for(i=0;i<3;i++)if(td_bus[i]==s->transit_target)bits=1;}
        else bits=(s->transit_origin&63)==10?(s->transit_target>=20&&s->transit_target<=22):s->transit_target==10;
        if(!bits||s->mode==TD_RIDE&&(!s->ride_left||s->ride_left>8))return FALSE;
    }
    return TRUE;
}
void td_save(void) BANKED {
    UBYTE i,slot=td_save_slot==0?1:0,mode=td.mode;UWORD crc=0xFFFF;
    const UBYTE *src=(const UBYTE*)&td;volatile UBYTE *ram=td_save_address(slot);
    /* Two records in SRAM bank 3 retain the last committed snapshot during power loss. */
    if(mode==TD_PAUSE||mode==TD_MAP||mode==TD_HELP)td.mode=td_resume_mode;
    if(td.mode!=TD_WAIT&&td.mode!=TD_RIDE)td.mode=TD_ROAM;
    ENABLE_RAM_MBC5;SWITCH_RAM_BANK(3,RAM_BANKS_ONLY);
    ram[0]=0;ram[1]=0xD7;ram[2]=5;ram[3]=sizeof(td);ram[4]=td_save_seq+1;
    crc=td_crc_byte(crc,ram[2]);crc=td_crc_byte(crc,ram[3]);crc=td_crc_byte(crc,ram[4]);
    for(i=0;i<sizeof(td);i++){ram[8+i]=src[i];crc=td_crc_byte(crc,src[i]);}
    ram[5]=crc;ram[6]=crc>>8;ram[7]=0;ram[0]=0x54;
    td_save_slot=slot;td_save_seq=ram[4];SWITCH_RAM_BANK(0,RAM_BANKS_ONLY);td.mode=mode;
}
static UBYTE td_read_slot(UBYTE slot,td_state_t *dest,UBYTE *seq){
    UBYTE i;UWORD crc=0xFFFF;UBYTE *dst=(UBYTE*)dest;volatile UBYTE *ram=td_save_address(slot);
    if(ram[0]!=0x54||ram[1]!=0xD7||ram[2]!=5||ram[3]!=sizeof(td))return FALSE;
    for(i=2;i<=4;i++)crc=td_crc_byte(crc,ram[i]);
    for(i=0;i<sizeof(td);i++){dst[i]=ram[8+i];crc=td_crc_byte(crc,dst[i]);}
    if(crc!=(ram[5]|(UWORD)ram[6]<<8))return FALSE;
    *seq=ram[4];return TRUE;
}
static UBYTE td_restore(void){
    td_state_t a,b;UBYTE va,vb,sa=0,sb=0,i,check=0;volatile UBYTE *ram=td_save_address(0);
    td_save_slot=TD_NONE;td_save_seq=0;
    ENABLE_RAM_MBC5;SWITCH_RAM_BANK(3,RAM_BANKS_ONLY);
    va=td_read_slot(0,&a,&sa);vb=td_read_slot(1,&b,&sb);
    SWITCH_RAM_BANK(0,RAM_BANKS_ONLY);
    if(va)va=td_valid_state(&a);
    if(vb)vb=td_valid_state(&b);
    if(va||vb){
        if(va&&(!vb||(BYTE)(sa-sb)>=0)){td=a;td_save_slot=0;td_save_seq=sa;}
        else{td=b;td_save_slot=1;td_save_seq=sb;}return TRUE;
    }
    /* Version 4 used older contract routes: keep earnings/completions, retire active work. */
    ENABLE_RAM_MBC5;SWITCH_RAM_BANK(3,RAM_BANKS_ONLY);
    va=ram[0]==0x54&&ram[1]==0xD7&&ram[2]==4;
    if(va){for(i=0;i<sizeof(a);i++){((UBYTE*)&a)[i]=ram[4+i];check^=((UBYTE*)&a)[i];}va=check==ram[3];}
    SWITCH_RAM_BANK(0,RAM_BANKS_ONLY);
    if(va){a.job=TD_NONE;a.stage=0;a.left=0;a.health=100;a.mode=TD_ROAM;a.speed=0;if(td_valid_state(&a)){td=a;td_save_slot=0;td_save();return TRUE;}}
    return FALSE;
}
void td_set_target(void) BANKED {
    if(td.job!=TD_NONE)td_get_stop(td_job.route[td.stage],&td_target);
    else td_get_stop(0,&td_target);
    td_position(&actors[1],td_target.u,td_target.v-12);
}
static void td_finish(UBYTE success){
    td_audio_play(success?TD_AUDIO_COMPLETE:TD_AUDIO_FAIL);
    if(success){
        if(!(td.complete[td.job>>3]&(1<<(td.job&7)))){td.complete[td.job>>3]|=1<<(td.job&7);td.done++;}
        { UWORD reward=td_job.reward/100*td.health+(td_job.reward%100)*td.health/100+td.left/5;
          td.cash=td.cash>60000-reward?60000:td.cash+reward; }
    }
    td.job=TD_NONE;td.speed=0;td_vx=td_vy=0;td.mode=TD_RESULT;td_save();td_ui_draw();
}
static void td_ready_offer(void){
    UBYTE i;
    for(i=0;i<TD_QUESTS;i++){
        td_get_job(i,&td_offer);
        if(td.done>=td_offer.min_done&&!(td.complete[i>>3]&(1<<(i&7)))){td.menu=i;return;}
    }
    td.menu=0;td_get_job(0,&td_offer);
}
static void td_interact(void){
    if(td.speed>2||td.speed<-2){td_message(1);return;}
    if(td.job==TD_NONE){td.mode=TD_BOARD;td_ready_offer();td_ui_draw();return;}
    if(!td_near(&td_target)){td_message(6);return;}
    if(td_job.vehicle!=TD_NONE&&(td.onfoot||td.vehicle!=td_job.vehicle)){td_message(2);return;}
    td.stage++;
    if(td.stage==td_job.count){td_finish(TRUE);return;}
    td_audio_play(TD_AUDIO_PICKUP);
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
    UBYTE elapsed=(seconds%period+period-phase)%period;
    return elapsed<2?0:period-elapsed;
}
static UBYTE td_route_stop(UBYTE origin,UBYTE idx){
    /* Public Island ferries connect each dock to the mainland, not to each other. */
    if(td_service(origin)==3)return (origin&63)==10?td_ferry[1+idx%3]:10;
    if(td_service(origin)==2)return td_bus[idx%3];
    return td_train[idx%7];
}
static void td_transit_open(void){
    UBYTE origin;
    if(td_entry_timer){td_message(2);return;}
    if(!td.onfoot){td_message(9);return;}
    if(td.job!=TD_NONE&&(td_job.kind==3||td_job.kind==5)){td_message(8);return;}
    origin=td_origin();if(origin==TD_NONE){td_message(6);return;}
    td.transit_origin=origin;td.menu=0;td.transit_target=td_route_stop(origin,0);td_get_stop(td.transit_target,&td_cursor);td.mode=TD_TRANSIT;td_ui_draw();
}
static void td_pause_choose(void){
    if((td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)&&td.menu>1&&td.menu!=8){td_message(2);return;}
    switch(td.menu){
        case 0:td.mode=td_resume_mode;break;
        case 1:td.mode=TD_MAP;td.map_x=camera_x;td.map_y=camera_y;camera_settings=0;break;
        case 2:td.mode=TD_BOARD;if(td.job==TD_NONE)td_ready_offer();else{td.menu=td.job;td_get_job(td.menu,&td_offer);}break;
        case 3:td_enter_exit();return;
        case 4:
            if(td.job!=TD_NONE||td.speed>2||td.speed<-2||td.onfoot){td_message(2);return;}
            td.vehicle=(td.vehicle+1)&3;td_save();break;
        case 5:td_transit_open();return;
        case 6:td_save();td.mode=TD_ROAM;td_message(12);break;
        case 7:td.job=TD_NONE;td.speed=0;td.mode=TD_ROAM;td_set_target();td_save();break;
        case 8:td_audio_set_mode((td_audio_get_mode()+1)%TD_AUDIO_MODES);break;
    }
    td_audio_play(TD_AUDIO_MENU);
    td_ui_draw();
}
static void td_menu_update(void){
    if(td.mode==TD_HELP){if(INPUT_A_PRESSED||INPUT_B_PRESSED){td.mode=td_resume_mode;td_ui_draw();}return;}
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
        if(INPUT_DOWN_PRESSED)td.menu=(td.menu+1)%9;
        if(INPUT_UP_PRESSED)td.menu=(td.menu+8)%9;
        if(INPUT_A_PRESSED){td_pause_choose();return;}
    }else if(td.mode==TD_BOARD){
        if(INPUT_RIGHT_PRESSED){td.menu=(td.menu+1)%TD_QUESTS;td_get_job(td.menu,&td_offer);}
        if(INPUT_LEFT_PRESSED){td.menu=(td.menu+TD_QUESTS-1)%TD_QUESTS;td_get_job(td.menu,&td_offer);}
        if(INPUT_A_PRESSED){
            if(td.job!=TD_NONE){td_message(2);return;}
            if(td.done<td_offer.min_done){td_message(3);return;}
            if(td_offer.vehicle!=TD_NONE&&(td.onfoot||td.vehicle!=td_offer.vehicle)){td_message(2);return;}
            td.job=td.menu;td_job=td_offer;td.stage=0;td.health=100;td.left=td_job.seconds;td.mode=TD_ROAM;td_audio_play(TD_AUDIO_MENU);td_set_target();td_save();td_ui_draw();return;
        }
    }else if(td.mode==TD_TRANSIT){
        if((INPUT_UP_PRESSED||INPUT_DOWN_PRESSED)&&(td.transit_origin&63)==16){
            td.transit_origin^=64;td.menu=0;td.transit_target=td_route_stop(td.transit_origin,0);td_get_stop(td.transit_target,&td_cursor);
        }
        if(INPUT_RIGHT_PRESSED){UBYTE count=td_service(td.transit_origin)==1?7:td_service(td.transit_origin)==2?3:(td.transit_origin&63)==10?3:1;td.menu=(td.menu+1)%count;td.transit_target=td_route_stop(td.transit_origin,td.menu);td_get_stop(td.transit_target,&td_cursor);}
        if(INPUT_LEFT_PRESSED){UBYTE count=td_service(td.transit_origin)==1?7:td_service(td.transit_origin)==2?3:(td.transit_origin&63)==10?3:1;td.menu=(td.menu+count-1)%count;td.transit_target=td_route_stop(td.transit_origin,td.menu);td_get_stop(td.transit_target,&td_cursor);}
        if(INPUT_A_PRESSED){
            if(td.transit_target==(td.transit_origin&63))return;
            td.mode=TD_WAIT;td.speed=0;td_save();td_ui_draw();return;
        }
    }else if(td.mode==TD_RESULT&&INPUT_A_PRESSED){td.mode=TD_BOARD;td_ready_offer();}
    if(INPUT_A_PRESSED||INPUT_UP_PRESSED||INPUT_DOWN_PRESSED||INPUT_LEFT_PRESSED||INPUT_RIGHT_PRESSED)td_ui_draw();
}
static void td_second(void){
    UBYTE service,fare,steps; td_stop_t origin;
    td.seconds++;
    if(td.job!=TD_NONE){if(td.left)td.left--;if(!td.left){td.health=0;
        /* A failed parcel still finishes its already-paid trip; never strand it in transit. */
        if(td.mode==TD_RIDE){td.job=TD_NONE;td_save();}else{td_finish(FALSE);return;}
    }}
    if(td.mode==TD_WAIT){
        td_get_stop(td.transit_origin&63,&origin);service=td_service(td.transit_origin);
        if(!td_next_departure(td.transit_origin,td.seconds)){
            fare=service==1?3:service==2?2:4;
            if(td.cash<fare){td.mode=TD_ROAM;td_save();td_message(4);return;}
            steps=td_distance(td_stop_index(td.transit_origin,td.transit_target),td_stop_index(td.transit_origin,td.transit_origin&63));
            /* A fresh free-roaming trip cannot inherit an old contract failure. */
            if(td.job==TD_NONE)td.health=100;
            td.cash-=fare;td.mode=TD_RIDE;td.ride_left=service==3?8:service==2?2+steps*2:1+steps/2;td_audio_play(TD_AUDIO_TRANSIT);td_save();
        }
    }else if(td.mode==TD_RIDE){
        if(td.ride_left)td.ride_left--;
        if(!td.ride_left){td.u=td_cursor.u*16;td.v=td_cursor.v*16;td.safe_u=td.u;td.safe_v=td.v;td.mode=td.health?TD_ROAM:TD_RESULT;td_audio_play(td.health?TD_AUDIO_TRANSIT:TD_AUDIO_FAIL);td_save();}
    }
    td_save();td_ui_draw();
}
static UBYTE td_signal_stop(UWORD pos,UBYTE vertical,UBYTE reverse){
    UBYTE i,count=vertical?8:9;UWORD line;
    for(i=0;i<count;i++){
        line=(vertical?td_rows[i]:td_cols[i])*16+(reverse?384:-384);
        if(pos>=line&&pos<line+8)return TRUE;
    }
    return FALSE;
}
static void td_traffic_step(void){
    static const UWORD bus_u[]={144,208,208,640,816,816};
    static const UWORD bus_v[]={64,64,176,176,176,64};
    UBYTE i,leg,phase=td.seconds%12,blocked;UWORD u,v,target_u,target_v;
    for(i=0;i<6;i++){
        u=td_traffic_u[i];v=td_traffic_v[i];leg=td_traffic_leg[i];blocked=0;
        if(i<4){
            target_u=(leg<2?840:48)*16;target_v=(td_rows[2+i]+(leg==0||leg==3?-8:8))*16;
            if((leg==0||leg==2)&&phase>=7)blocked=td_signal_stop(u,0,leg==2);
        }else if(i==4){
            target_u=(leg==0||leg==3?824:808)*16;target_v=(leg<2?792:48)*16;
            if((leg==0||leg==2)&&phase<7)blocked=td_signal_stop(v,1,leg==2);
        }else {target_u=bus_u[leg]*16;target_v=bus_v[leg]*16;}
        if(!blocked){
            if(u<target_u)u+=td_distance(u,target_u)<8?td_distance(u,target_u):8;
            else if(u>target_u)u-=td_distance(u,target_u)<8?td_distance(u,target_u):8;
            else if(v<target_v)v+=td_distance(v,target_v)<8?td_distance(v,target_v):8;
            else if(v>target_v)v-=td_distance(v,target_v)<8?td_distance(v,target_v):8;
            // Road users yield to a courier crossing on foot, including between catch-up steps.
            if((td.mode==TD_ROAM||td.mode==TD_WAIT)&&td.onfoot&&td_distance(td.u,u)<208&&td_distance(td.v,v)<208)continue;
            td_traffic_u[i]=u;td_traffic_v[i]=v;
            if(u==target_u&&v==target_v)td_traffic_leg[i]=(leg+1)%(i==5?6:4);
        }
        if(td.mode==TD_ROAM&&!td.onfoot&&td_distance(td.u,u)<180&&td_distance(td.v,v)<180&&!td.cooldown){
            td.speed/=2;td_vx/=2;td_vy/=2;td.cooldown=60;
            if(td.job!=TD_NONE){td.health=td.health>12?td.health-12:0;if(!td.health){td_finish(FALSE);return;}}td_message(5);
        }
    }
}
static void td_traffic_present(void){
    UBYTE i,leg,frame;
    for(i=0;i<6;i++){
        leg=td_traffic_leg[i];frame=i<4?leg*2:i==4?(leg==0?2:leg==1?4:leg==2?6:0):8+(leg==2?2:leg==0?4:leg==5?6:0);
        td_position(&actors[i+2],td_traffic_u[i]>>4,td_traffic_v[i]>>4);td_frame(&actors[i+2],frame);
    }
    td_position(&actors[8],td.park_u>>4,td.park_v>>4);td_frame(&actors[8],td_entry_timer?44:td.vehicle*8+((td.heading+1)&15)/2);
    if(td.onfoot)actors[8].flags&=~ACTOR_FLAG_HIDDEN;else actors[8].flags|=ACTOR_FLAG_HIDDEN;
}
static UWORD td_pedestrian_u(UBYTE route){
    UBYTE phase=(td.seconds*12+td.subsecond/5+route*37)&127;
    return td_sidewalk_routes[route][0]+(phase<64?phase:127-phase);
}
static void td_pedestrians(void){
    UBYTE i,j,k,route,phase,refresh;UWORD u,v,score,best_score,player_u=td.u>>4,player_v=td.v>>4;
    refresh=!--td_ped_refresh||td_distance(player_u,td_ped_anchor_u)>64||td_distance(player_v,td_ped_anchor_v)>64;
    if(refresh){
        td_ped_refresh=16;td_ped_anchor_u=player_u;td_ped_anchor_v=player_v;
        for(i=0;i<6;i++){
            route=td_ped_route[i];
            // Retain identities throughout an extended viewport; visible people never jump routes.
            if(route!=TD_NONE&&td_distance(player_u,td_pedestrian_u(route))<144&&td_distance(player_v,td_sidewalk_routes[route][1])<112)continue;
            td_ped_route[i]=TD_NONE;best_score=65535;
            for(j=0;j<TD_PEDESTRIAN_ROUTES;j++){
                if(td_distance(player_u,td_sidewalk_routes[j][0]+32)>176||td_distance(player_v,td_sidewalk_routes[j][1])>112)continue;
                for(k=0;k<6;k++)if(td_ped_route[k]==j)break;
                if(k<6)continue;
                u=td_pedestrian_u(j);score=td_distance(player_u,u)+td_distance(player_v,td_sidewalk_routes[j][1]);
                if(score<best_score){best_score=score;td_ped_route[i]=j;}
            }
        }
    }
    for(i=0;i<6;i++){
        route=td_ped_route[i];if(route==TD_NONE){actors[9+i].flags|=ACTOR_FLAG_HIDDEN;continue;}
        phase=(td.seconds*12+td.subsecond/5+route*37)&127;u=td_pedestrian_u(route);v=td_sidewalk_routes[route][1];
        td_position(&actors[9+i],u,v);td_frame(&actors[9+i],32+(phase<64?0:2)+((td_tick>>3)&1));
        if(td_distance(player_u,u)<112&&td_distance(player_v,v)<96)actors[9+i].flags&=~ACTOR_FLAG_HIDDEN;
        else actors[9+i].flags|=ACTOR_FLAG_HIDDEN;
        if(td.mode==TD_ROAM&&!(actors[9+i].flags&ACTOR_FLAG_HIDDEN)&&!td.onfoot&&!td.cooldown&&td_distance(td.u>>4,u)<10&&td_distance(td.v>>4,v)<10){td.speed/=2;td_vx/=2;td_vy/=2;td.cooldown=45;td_message(13);}
    }
}
static UBYTE td_corner_slide(WORD nu,WORD nv){
    UBYTE i,j,side,clear;WORD shift,shifted;UWORD ax=td_vx<0?-td_vx:td_vx,ay=td_vy<0?-td_vy:td_vy;
    if(!INPUT_A||INPUT_B||td.speed<3||ax==ay||td_corner_used)return FALSE;
    /* A quantised corner may clip the car by a few pixels although a parallel
       lane is open. Sweep only across usable current and proposed footprints. */
    for(i=1;i<=6;i++)for(side=0;side<2;side++){
        shift=side?-(WORD)i*16:(WORD)i*16;
        if(ay>ax&&nv!=(WORD)td.v){
            shifted=(WORD)td.u+shift;
            if(shifted<128||shifted>1016*16||nv<128||nv>968*16||!td_drivable(shifted>>4,nv>>4))continue;
            clear=1;for(j=1;j<=i;j++)if(!td_drivable(((WORD)td.u+(side?-(WORD)j*16:(WORD)j*16))>>4,td.v>>4)){clear=0;break;}
            if(clear){td.u=shifted;td.v=nv;td_vx=0;td_corner_used=1;return TRUE;}
        }else if(ax>ay&&nu!=(WORD)td.u){
            shifted=(WORD)td.v+shift;
            if(shifted<128||shifted>968*16||nu<128||nu>1016*16||!td_drivable(nu>>4,shifted>>4))continue;
            clear=1;for(j=1;j<=i;j++)if(!td_drivable(td.u>>4,((WORD)td.v+(side?-(WORD)j*16:(WORD)j*16))>>4)){clear=0;break;}
            if(clear){td.u=nu;td.v=shifted;td_vy=0;td_corner_used=1;return TRUE;}
        }
    }
    return FALSE;
}
static void td_drive(void){
    WORD nu,nv,target_x,target_y;BYTE walk_x,walk_y;UBYTE limit,turn_period,moving=0,slide=0,speed;UWORD u,v;
    if(td.cooldown)td.cooldown--;
    if(td_red_cooldown)td_red_cooldown--;
    if(td_entry_timer){
        if(!td_entry_target){td.u=(td.u*3+td.park_u)/4;td.v=(td.v*3+td.park_v)/4;}
        if(!--td_entry_timer){if(!td_entry_target){td.u=td.park_u;td.v=td.park_v;td.onfoot=0;}td_save();}
        td_frame(&PLAYER,td.onfoot?32:td.vehicle*8+((td.heading+1)&15)/2);return;
    }
    if(td.onfoot){
        nu=td.u;nv=td.v;
        walk_x=!!INPUT_RIGHT-!!INPUT_LEFT;walk_y=!!INPUT_DOWN-!!INPUT_UP;
        if(walk_x){nu+=walk_x*(walk_y?6:8);td_walk_dir=walk_x>0?0:1;moving=1;}
        if(walk_y){nv+=walk_y*(walk_x?6:8);td_walk_dir=walk_y>0?2:3;moving=1;}
        if(td_foot_free(nu,nv)){td.u=nu;td.v=nv;}
        else{if(nu!=(WORD)td.u&&td_foot_free(nu,td.v))td.u=nu;if(nv!=(WORD)td.v&&td_foot_free(td.u,nv))td.v=nv;}
        td.speed=0;td_frame(&PLAYER,32+td_walk_dir*2+(moving?((td_tick>>3)&1):0));
        if(td_input_edge&&INPUT_A_PRESSED&&td_near_car())td_enter_exit();
        if(td_input_edge&&INPUT_B_PRESSED&&!td_entry_timer)td_transit_open();
        return;
    }
    limit=td.vehicle==1?20:td.vehicle==2?28:td.vehicle==3?18:24;
    speed=td.speed<0?-td.speed:td.speed;
    turn_period=speed>18?12:speed>8?10:8;
    if(td.vehicle==1)turn_period+=2;
    // A dedicated yaw counter keeps turns regular as speed changes.
    if(!!INPUT_LEFT!=!!INPUT_RIGHT){if(++td_turn_tick>=turn_period){td_turn_tick=0;if(INPUT_LEFT)td.heading=(td.heading+15)&15;else td.heading=(td.heading+1)&15;
        if(td.job!=TD_NONE&&td_job.kind==5&&speed>18){if(td.health)td.health--;td_message(14);}
    }}
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
        if(nu!=(WORD)td.u&&td_drivable(nu>>4,td.v>>4)){td.u=nu;td_vy=0;slide=1;}
        if(nv!=(WORD)td.v&&td_drivable(td.u>>4,nv>>4)){td.v=nv;td_vx=0;slide=1;}
        if(!slide)slide=td_corner_slide(nu,nv);
        /* Remove only blocked-axis motion. Repeated curb scrapes must not beat the throttle. */
        if(slide){if(speed>8&&!td.cooldown){td.cooldown=30;if(td.job!=TD_NONE){UBYTE damage=td_job.kind==1?4:1;td.health=td.health>damage?td.health-damage:0;}td_message(5);}}
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
    UBYTE i;td_tick=td_notice_timer=td_red_cooldown=td_entry_timer=td_turn_tick=0;td_vx=td_vy=0;td_last_frame=sys_time;td_corner_used=0;
    if(!td_restore()){
        memset(&td,0,sizeof(td));td.u=560*16;td.v=720*16;td.park_u=td.u;td.park_v=td.v;td.cash=30;td.job=TD_NONE;td.heading=0;td.health=100;
    }
    td.speed=0;td_resume_mode=td.mode==TD_WAIT||td.mode==TD_RIDE?td.mode:TD_ROAM;td.mode=TD_HELP;td.msg=0;td.menu=0;td.safe_u=td.u;td.safe_v=td.v;
    if(td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)td_get_stop(td.transit_target,&td_cursor);
    if(td.job!=TD_NONE){td_get_job(td.job,&td_job);if(td.stage>=td_job.count)td.job=TD_NONE;}
    actors_len=15;
    for(i=1;i<15;i++){
        actors[i]=PLAYER;actors[i].prev=actors[i].next=NULL;actors[i].flags=ACTOR_FLAG_PERSISTENT;actors[i].collision_group=0;actors[i].script.bank=actors[i].script_update.bank=0;
        // Place copied actors on the inactive list before activating them.
        actors[i].next=actors_inactive_head; if(actors_inactive_head)actors_inactive_head->prev=&actors[i];actors_inactive_head=&actors[i];
        activate_actor(&actors[i]);
    }
    for(i=0;i<6;i++){td_traffic_u[i]=(i<4?80+i*120:i==4?824:144)*16;td_traffic_v[i]=(i<4?td_rows[2+i]-8:i==4?240:64)*16;td_traffic_leg[i]=i==5?1:0;td_ped_route[i]=TD_NONE;}
    td_ped_refresh=1;td_ped_anchor_u=td.u>>4;td_ped_anchor_v=td.v>>4;
    td_frame(&actors[1],40);td_set_target();td_position(&PLAYER,td.u>>4,td.v>>4);
    camera_settings=CAMERA_LOCK_FLAG;camera_offset_x=0;camera_offset_y=-16;camera_deadzone_x=8;camera_deadzone_y=8;
    td_audio_init();td_ui_init();
}
void toronto_update(void) BANKED {
    UWORD now=sys_time,elapsed=now-td_last_frame,seconds;UBYTE motion,step,consumed=0;td_last_frame=now;
    td_corner_used=0;
    motion=elapsed>4?4:elapsed;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE){td_menu_update();td_sound_update();return;}
    if(INPUT_START_PRESSED){td_resume_mode=td.mode;td.mode=TD_PAUSE;td.menu=0;td_audio_play(TD_AUDIO_MENU);td_ui_draw();td_sound_update();return;}
    /* A deliberate cancel wins over departure on the same input frame. */
    if(td.mode==TD_WAIT&&INPUT_B_PRESSED){td.mode=TD_ROAM;consumed=1;td_save();td_ui_draw();}
    if(td_notice_timer){if(!--td_notice_timer){td.msg=0;td_ui_draw();}}
    /* Keep deadlines and transit tied to every VBlank, even when rendering falls behind. */
    seconds=elapsed/60;elapsed%=60;elapsed+=td.subsecond;
    if(elapsed>=60){elapsed-=60;seconds++;}td.subsecond=elapsed;
    while(seconds--&&(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE))td_second();
    for(step=0;step<motion&&(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE);step++){
        td_tick++;td_input_edge=step==0&&!consumed;if(td.mode==TD_ROAM)td_drive();td_traffic_step();
    }
    if(td.mode==TD_ROAM&&!consumed&&INPUT_SELECT_PRESSED)td_interact();
    if(td.mode==TD_ROAM||td.mode==TD_WAIT||td.mode==TD_RIDE){td_traffic_present();td_pedestrians();}
    td_position(&PLAYER,td.u>>4,td.v>>4);
    td_sound_update();
}
