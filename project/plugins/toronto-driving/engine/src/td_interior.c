#pragma bank 255
/* This module reads the generated interior and door tables. */
#define TD_INTERIOR_DATA
#define TD_DOORS_DATA
/* Inside buildings (td_interior.h). Interiors are scenes of the courier's
 * scene type, drawn by scripts/create_interiors.py; their points of
 * interest, people and the doors that lead to them are generated tables
 * (scripts/create_interior_data.py). Inside, the city does not run: no
 * traffic, street life, weather or day/night tint; the clock and contract
 * timers keep going (toronto_update runs the seconds first). People inside
 * are walker slots showing streamed looks (td_people.c) with a role:
 * wandering, gazing at art, sitting, behind a counter, on patrol, following
 * a teacher or at the windows. */
#include <string.h>
#include "td_game.h"
#include "td_life.h"
#include "td_interior.h"
#include "td_doors.h"
#include "td_people.h"
#include "td_sprites.h"
#include "td_audio.h"
#include "td_anim.h"
#include "td_overlay.h"
#include "td_hud.h"
#include "td_ui_art.h"
#include "td_district.h"
#include "actor.h"
#include "camera.h"
#include "input.h"
#include "collision.h"
#ifdef __SDCC
#include "data_manager.h"
#include "vm.h"
#include "vm_exceptions.h"
#include "palette.h"
#include "td_interior_scenes.h"
#endif

UBYTE td_interior=TD_NONE,td_door_near=TD_NONE;
char td_card[5][19];
extern UBYTE td_transition_pending,td_tick,td_walk_dir;
extern WORD td_vx,td_vy;
/* The courier inside (whole pixels; feet), the elevator ride (1 up, 2
 * down) and its ticks, the gallery room shown. Globals so emulator checks
 * can read them. */
UWORD in_u,in_v;
UBYTE in_ride,in_ride_t,in_room;
static UBYTE in_arrive,in_talk_cool,in_bump_cool;
/* The door the courier came in by (its name shows; TD_NONE for the gallery). */
static UBYTE in_door=TD_NONE;
/* People inside: slot positions (whole pixels), facing (0 E, 1 W, 2 S,
 * 3 N), what they do, a timer and the spot they belong to. */
static UWORD in_nu[TD_PEDS],in_nv[TD_PEDS];
static UBYTE in_nd[TD_PEDS],in_nb[TD_PEDS],in_nt[TD_PEDS],in_ns[TD_PEDS],in_count,in_first;
/* The interior's own palettes, kept for after the city map. */
static UWORD in_bkg[32],in_spr[32];
/* Where the courier arrives: 0 the entrance, else the point index+1. */
#define IN_ARRIVE_ENTRANCE 0
#define IN_REACH 14
#define IN_RIDE_TICKS 240

/* Persistent scene-change script, as td_district.c (VM_LOCK, VM_FADE out,
 * VM_RAISE EXCEPTION_CHANGE_SCENE with the scene's far pointer). */
#ifdef __SDCC
static UBYTE in_script[9]={0x25,0x57,0x01,0x27,3,EXCEPTION_CHANGE_SCENE,0,0,0};
static UBYTE in_queue(UBYTE k){
    UWORD address;
    if(k>=TD_INTERIORS)return FALSE;
    address=(UWORD)(uintptr_t)td_in_scene[k].ptr;
    in_script[6]=td_in_scene[k].bank;in_script[7]=(UBYTE)address;in_script[8]=(UBYTE)(address>>8);
    return script_execute(1,in_script,NULL,0)!=NULL;
}
static UBYTE in_here(UBYTE k){return k<TD_INTERIORS&&current_scene.bank==td_in_scene[k].bank&&current_scene.ptr==td_in_scene[k].ptr;}
static UBYTE in_solid(UWORD u,UWORD v){
    if(u>=(UWORD)image_tile_width<<3||v>=(UWORD)image_tile_height<<3)return TRUE;
    return (tile_at((UBYTE)(u>>3),(UBYTE)(v>>3))&15)==15;
}
#else
UBYTE td_test_in_queued=TD_NONE,td_test_in_here=TD_NONE;
static UBYTE in_queue(UBYTE k){if(k>=TD_INTERIORS)return FALSE;td_test_in_queued=k;return TRUE;}
static UBYTE in_here(UBYTE k){return k<TD_INTERIORS&&td_test_in_here==k;}
UBYTE td_test_in_grid[64][64];
static UBYTE in_solid(UWORD u,UWORD v){return u>=512||v>=512||td_test_in_grid[v>>3][u>>3];}
#endif

static void in_beacon(void);
static void in_npc(UBYTE k);
void td_interior_reset(void) BANKED {td_interior=TD_NONE;td_door_near=TD_NONE;in_ride=0;in_door=TD_NONE;}

static void in_hud(const char *s){
    strcpy(td_street_name,s);td_pop_street=TD_POP_LONG;td_pop_target=0;td_ui_hud_paint();
}

/* Into interior k (arriving at its entrance, or at point index arrive-1). */
static UBYTE in_go(UBYTE k,UBYTE arrive){
    if(!in_queue(k))return FALSE;
    td_interior=k;in_arrive=arrive;td_transition_pending=1;in_ride=0;td_door_near=TD_NONE;
    td_overlay_show(0);
    return TRUE;
}
/* Back out to the city at the door (td.u/td.v never moved). */
static void in_leave(void){
    td_interior=TD_NONE;
    td_transition_pending=td_district_queue(td.district)?1:2;
}

/* ------------------------------------------------------------ doors */
UBYTE td_door_scan(void) BANKED {
    UBYTE d,near=TD_NONE;UWORD pu=td.u>>4,pv=td.v>>4,du,dv;
    if(td.onfoot&&td.mode==TD_ROAM&&td.district<16&&td_interior==TD_NONE){
        for(d=td_door_first[td.district];d<td_door_first[td.district+1];d++){
            du=pu>td_door_u[d]?pu-td_door_u[d]:td_door_u[d]-pu;dv=pv>td_door_v[d]?pv-td_door_v[d]:td_door_v[d]-pv;
            if(du<12&&dv<12){near=d;break;}
        }
    }
    if(near!=td_door_near){td_door_near=near;return TRUE;}
    return FALSE;
}
void td_door_name(UBYTE d,char *dest) BANKED {
    if(d>=TD_DOORS){*dest=0;return;}
    td_interior_text(td_door_text[d],0,dest);
}
UBYTE td_interior_door(void) BANKED {
    UBYTE d;
    td_door_scan();d=td_door_near;
    if(d==TD_NONE)return FALSE;
    /* Security turns away a courier with the police on them. */
    if(td.wanted){td_message(TD_MSG_DOOR_LOCKED);return TRUE;}
    td.speed=0;
    if(in_go(td_door_in[d],IN_ARRIVE_ENTRANCE)){td_audio_play(TD_AUDIO_MENU);td_door_near=TD_NONE;in_door=d;}
    return TRUE;
}
UBYTE td_gallery_open(void) BANKED {
    if(td.wanted||td.speed>2||td.speed<-2)return FALSE;
    in_room=0;in_door=TD_NONE;
    return in_go(TD_IN_GALLERY_HALL,IN_ARRIVE_ENTRANCE);
}

/* ------------------------------------------------------------ setup */
static void in_place(actor_t *a,UWORD u,UWORD v){a->pos.x=u<<5;a->pos.y=v<<5;}
static void in_frame(actor_t *a,UBYTE f){
    if(a->frame_start!=f||a->frame_end!=f+1)actor_set_frames(a,f,f+1);
    a->anim_tick=255;
}
static UBYTE in_pose_of(UBYTE face){return face==2?TD_POSE_FRONT:face==3?TD_POSE_BACK:TD_POSE_SIDE;}

/* The exhibits of the gallery room in_room: a look on each plinth. */
static void in_gallery_room(void){
    UBYTE k,p,look;actor_t *a;
    in_count=0;
    for(k=0,p=td_in_pt_first[td_interior];p<td_in_pt_first[td_interior+1]&&k<TD_PEDS;p++){
        if(td_in_pt_kind[p]!=TD_IP_EXHIBIT)continue;
        look=td_gallery_looks[in_room][k];
        a=&actors[TD_ACTOR_PEDS+k];
        if(look>=TD_LOOKS){a->flags|=ACTOR_FLAG_HIDDEN;k++;continue;}
        td_people_show(k,look,TD_POSE_FRONT);
        in_nu[k]=td_in_pt_px[p];in_nv[k]=td_in_pt_py[p];in_nd[k]=2;in_nb[k]=TD_NB_EXHIBIT;in_nt[k]=k<<4;in_ns[k]=p;
        in_place(a,in_nu[k],in_nv[k]);a->flags&=~ACTOR_FLAG_HIDDEN;
        k++;in_count=k;
    }
    td_interior_text(td_gallery_title[in_room],0,td_card[0]);in_hud(td_card[0]);
}

static void in_people(void){
    UBYTE s,k,look;actor_t *a;
    in_first=td_in_np_first[td_interior];in_count=td_in_np_first[td_interior+1]-in_first;
    if(in_count>TD_PEDS)in_count=TD_PEDS;
    for(k=0;k<in_count;k++){
        s=in_first+k;
        /* Each spot has a few looks for its role; which one depends on the
         * clock, so a return visit can find someone else there. */
        look=td_in_np_look[s][(UBYTE)(td.seconds>>6)&3];
        td_people_show(k,look,td_in_np_behave[s]==TD_NB_SIT?TD_POSE_ACT:in_pose_of(td_in_np_face[s]));
        in_nu[k]=td_in_np_x[s];in_nv[k]=td_in_np_y[s];in_nd[k]=td_in_np_face[s];in_nb[k]=td_in_np_behave[s];in_nt[k]=(UBYTE)(k*41);in_ns[k]=s;
        a=&actors[TD_ACTOR_PEDS+k];in_place(a,in_nu[k],in_nv[k]);a->flags&=~ACTOR_FLAG_HIDDEN;
    }
}

UBYTE td_interior_init(void) BANKED {
    UBYTE i,k=td_interior,p;actor_t *a;
    if(!in_here(k))return FALSE;
    td_transition_pending=0;td.speed=0;td_vx=td_vy=0;
    actors_len=TD_ACTORS;TD_PALETTE(&PLAYER)=TD_PAL_COURIER;
    for(i=1;i<TD_ACTORS;i++){
        actors[i]=PLAYER;actors[i].prev=actors[i].next=NULL;actors[i].flags=ACTOR_FLAG_PERSISTENT|ACTOR_FLAG_HIDDEN;
        actors[i].collision_group=0;actors[i].script.bank=actors[i].script_update.bank=0;
        actors[i].next=actors_inactive_head;if(actors_inactive_head)actors_inactive_head->prev=&actors[i];actors_inactive_head=&actors[i];
        activate_actor(&actors[i]);
    }
    PLAYER.flags&=~ACTOR_FLAG_HIDDEN;
    td_people_init();
    /* The scene's own palettes are already loaded: keep them untinted. */
#ifdef __SDCC
    memcpy(in_bkg,BkgPalette,sizeof(in_bkg));memcpy(in_spr,SprPalette,sizeof(in_spr));
#endif
    in_u=td_in_entry[k][0];in_v=td_in_entry[k][1];td_walk_dir=3;
    if(in_arrive){p=td_in_pt_first[k]+in_arrive-1;in_u=td_in_pt_x[p];in_v=td_in_pt_y[p];}
    if(td_in_kind[k]==TD_IK_GALLERY&&k==TD_IN_GALLERY_HALL)in_gallery_room();
    else{in_count=0;in_people();}
    in_place(&PLAYER,in_u,in_v);in_frame(&PLAYER,TD_FRAME_COURIER_WALK+(td_walk_dir<<1));
    /* Every person gets a frame now: the scene fades in before the first
     * update, and an actor left on frame 0 would show as a car. */
    for(i=0;i<in_count;i++)in_npc(i);
    camera_settings=CAMERA_LOCK_FLAG;camera_offset_x=0;camera_offset_y=0;camera_deadzone_x=8;camera_deadzone_y=8;
    in_ride=0;in_talk_cool=0;in_bump_cool=0;td.mode=TD_ROAM;
    td_ui_init();td_scenery_find();
    /* A shared interior takes the name on its door (Dufferin Mall, a cafe). */
    if(in_door!=TD_NONE&&td_in_kind[k]!=TD_IK_GALLERY&&k!=TD_IN_CN_LOOKOUT)td_door_name(in_door,td_card[0]);
    else td_interior_text(td_in_name[k],0,td_card[0]);
    if(td_in_kind[k]!=TD_IK_GALLERY||k!=TD_IN_GALLERY_HALL)in_hud(td_card[0]);
    in_beacon();
    return TRUE;
}

void td_interior_palettes(void) BANKED {
#ifdef __SDCC
    memcpy(BkgPalette,in_bkg,sizeof(in_bkg));memcpy(SprPalette,in_spr,sizeof(in_spr));
    set_bkg_palette(0,7,(const palette_color_t *)BkgPalette);
    set_sprite_palette(0,8,(const palette_color_t *)SprPalette);
#endif
}

/* ------------------------------------------------------------ cards */
/* A card of a text entry: its title and three lines (td.mode TD_CARD). */
static void in_card_text(UBYTE id){
    UBYTE i;
    for(i=0;i<4;i++)td_interior_text(id,i,td_card[i]);
    strcpy(td_card[4],TD_UI_BTN_B " CLOSE");
    td.mode=TD_CARD;td_audio_play(TD_AUDIO_MENU);td_ui_draw();
}
/* A person's gallery card: name, where to meet them and their traits. */
static void in_card_look(UBYTE look){
    UBYTE i;
    td_people_text(look,TD_PT_NAME,td_card[0]);
    td_people_text(look,TD_PT_HOME,td_card[1]);
    for(i=0;i<3;i++)td_people_text(look,TD_PT_TRAIT+i,td_card[2+i]);
    td.mode=TD_CARD;td_audio_play(TD_AUDIO_MENU);td_ui_draw();
}

/* ------------------------------------------------------------ deliveries */
/* The active contract's next stop is inside interior k, entered by the
 * door the stop names (a shared interior has many doors). */
static UBYTE in_stop_is(UBYTE k){
    return td.job!=TD_NONE&&(td_target.reserved>>1)==(UBYTE)(k+1)&&in_door<TD_DOORS&&td_target.district==td.district&&
           td_door_u[in_door]==td_target.u&&td_door_v[in_door]==td_target.v;
}
static UBYTE in_delivers_here(void){return in_stop_is(td_interior);}
/* The beacon inside: over the desk taking the delivery, or over the lift
 * when it is up at the LookOut. */
static void in_beacon(void){
    UBYTE p,want=TD_NONE;actor_t *b=&actors[1];
    b->flags|=ACTOR_FLAG_HIDDEN;
    if(in_delivers_here())want=TD_IP_DESK;
    else if(td_interior==TD_IN_CN_BASE&&in_stop_is(TD_IN_CN_LOOKOUT))want=TD_IP_ELEVATOR;
    if(want==TD_NONE)return;
    for(p=td_in_pt_first[td_interior];p<td_in_pt_first[td_interior+1];p++)if(td_in_pt_kind[p]==want)break;
    if(p==td_in_pt_first[td_interior+1])return;
    in_place(b,td_in_pt_x[p],td_in_pt_y[p]-18);in_frame(b,TD_FRAME_BEACON);
    b->flags&=~ACTOR_FLAG_HIDDEN;
    td_message(want==TD_IP_DESK?TD_MSG_DESK:TD_MSG_LIFT);
}
static void in_deliver(void){
    td.stage++;
    if(td.stage==td_job.count){td_finish(TRUE);in_beacon();return;}
    td_audio_play(TD_AUDIO_PICKUP);td_anim_spawn(TD_PART_POP,TD_FRAME_PARCEL,in_u,in_v-10);
    if(td.stage==1)td_radio_contract(td.job,1);
    td_set_target();in_beacon();td_save();td_ui_draw();
}

/* ------------------------------------------------------------ the courier */
static UBYTE in_free(UWORD u,UWORD v){
    return !in_solid(u-3,v)&&!in_solid(u+3,v)&&!in_solid(u-3,v-4)&&!in_solid(u+3,v-4);
}
static UBYTE in_point(void){
    UBYTE p,best=TD_NONE;UWORD d,best_d=0xFFFF,du,dv;
    for(p=td_in_pt_first[td_interior];p<td_in_pt_first[td_interior+1];p++){
        du=in_u>td_in_pt_x[p]?in_u-td_in_pt_x[p]:td_in_pt_x[p]-in_u;dv=in_v>td_in_pt_y[p]?in_v-td_in_pt_y[p]:td_in_pt_y[p]-in_v;
        if(du>=IN_REACH||dv>=IN_REACH)continue;
        d=du+dv;if(d<best_d){best_d=d;best=p;}
    }
    return best;
}
static UBYTE in_person(void){
    UBYTE k,best=TD_NONE;UWORD d,best_d=0xFFFF,du,dv;
    for(k=0;k<in_count;k++){
        if(actors[TD_ACTOR_PEDS+k].flags&ACTOR_FLAG_HIDDEN)continue;
        du=in_u>in_nu[k]?in_u-in_nu[k]:in_nu[k]-in_u;dv=in_v>in_nv[k]?in_v-in_nv[k]:in_nv[k]-in_v;
        if(du>=18||dv>=16)continue;
        d=du+dv;if(d<best_d){best_d=d;best=k;}
    }
    return best;
}
static void in_talk(UBYTE k){
    char a[19],b[19];
    td_people_text(td_slot_look[k],TD_PT_TALK,a);td_people_text(td_slot_look[k],TD_PT_TALK+1,b);
    td_radio_speech(a,b);
    /* They turn to face the courier. */
    in_nd[k]=in_v>in_nv[k]+4?2:in_v+4<in_nv[k]?3:in_u>=in_nu[k]?0:1;in_nt[k]=0;
}
static void in_use(UBYTE p){
    UBYTE kind=td_in_pt_kind[p],k;
    switch(kind){
    case TD_IP_ELEVATOR:
        in_ride=td_interior==TD_IN_CN_LOOKOUT?2:1;in_ride_t=0;PLAYER.flags|=ACTOR_FLAG_HIDDEN;
        td_audio_play(TD_AUDIO_TRANSIT);
        td_radio_speech(in_ride==1?"GLASS ELEVATOR UP:":"GLASS ELEVATOR DOWN","58 SECONDS, 346 M");
        return;
    case TD_IP_DESK:
        if(in_delivers_here()){in_deliver();return;}
        break;
    case TD_IP_ARCH_NEXT:
        if(td_interior==TD_IN_GALLERY_HALL){
            if(in_room+1<TD_GALLERY_ROOMS){in_room++;in_gallery_room();in_u=td_in_pt_x[p]-24;}
            else in_go(TD_IN_GALLERY_ART,IN_ARRIVE_ENTRANCE);
            td_audio_play(TD_AUDIO_MENU);return;
        }
        if(td_interior==TD_IN_GALLERY_ART){in_room=0;in_go(TD_IN_GALLERY_HALL,IN_ARRIVE_ENTRANCE);return;}
        break;
    case TD_IP_ARCH_PREV:
        if(td_interior==TD_IN_GALLERY_HALL&&in_room){in_room--;in_gallery_room();in_u=td_in_pt_x[p]+24;td_audio_play(TD_AUDIO_MENU);return;}
        if(td_interior==TD_IN_GALLERY_ART){in_room=TD_GALLERY_ROOMS-1;in_go(TD_IN_GALLERY_HALL,IN_ARRIVE_ENTRANCE);return;}
        break;
    case TD_IP_EXHIBIT:
        if(td_interior==TD_IN_GALLERY_HALL){
            for(k=0;k<in_count;k++)if(in_ns[k]==p){in_card_look(td_slot_look[k]);return;}
            return;
        }
        break;
    }
    if(td_in_pt_text[p]!=TD_NONE)in_card_text(td_in_pt_text[p]);
}

static void in_ride_tick(UBYTE motion){
    char t[21];UWORD m;
    in_ride_t+=motion;
    if(in_ride_t>=IN_RIDE_TICKS){
        PLAYER.flags&=~ACTOR_FLAG_HIDDEN;
        if(in_ride==1)in_go(TD_IN_CN_LOOKOUT,TD_IN_LOOKOUT_LIFT+1);else in_go(TD_IN_CN_BASE,TD_IN_BASE_LIFT+1);
        return;
    }
    if(in_ride_t&7)return;
    /* Altitude on the HUD: 0 to 346 m (58 seconds compressed to four). */
    m=(UWORD)in_ride_t+((UWORD)in_ride_t>>2)+((UWORD)in_ride_t>>4)+((UWORD)in_ride_t>>7);
    if(m>346)m=346;
    if(in_ride==2)m=346-m;
    strcpy(t,TD_UI_PIN "ALTITUDE    0 M");
    t[13]='0'+(UBYTE)(m%10);m/=10;if(m)t[12]='0'+(UBYTE)(m%10);m/=10;if(m)t[11]='0'+(UBYTE)m;
    in_hud(t);
}

/* ------------------------------------------------------------ people */
static void in_npc(UBYTE k){
    actor_t *a=&actors[TD_ACTOR_PEDS+k];UBYTE s=in_ns[k],b=in_nb[k],f=TD_PEOPLE_FRAME(k),moving=0;
    UWORD tu,tv,du,dv;
    in_nt[k]++;
    switch(b){
    case TD_NB_WANDER:
    case TD_NB_PATROL:
    case TD_NB_WINDOW:
    case TD_NB_GAZE:
        /* Stroll to a spot in their area, linger, pick another. */
        if(in_nt[k]<96)break;
        tu=td_in_np_area[s][0]+((in_nt[k]*29+k*53)&63)%(1+(UBYTE)(td_in_np_area[s][2]-td_in_np_area[s][0]));
        tv=td_in_np_area[s][1]+((in_nt[k]*17+k*31)&31)%(1+(UBYTE)(td_in_np_area[s][3]-td_in_np_area[s][1]));
        if(b==TD_NB_GAZE||b==TD_NB_WINDOW)tv=td_in_np_y[s];
        du=tu>in_nu[k]?tu-in_nu[k]:in_nu[k]-tu;dv=tv>in_nv[k]?tv-in_nv[k]:in_nv[k]-tv;
        if(du<2&&dv<2){in_nt[k]=0;in_nd[k]=b==TD_NB_GAZE||b==TD_NB_WINDOW?3:td_in_np_face[s];break;}
        if(!(in_nt[k]&1))break;
        if(du>=2){UBYTE east=tu>in_nu[k];tu=east?in_nu[k]+1:in_nu[k]-1;if(!in_solid(tu,in_nv[k])){in_nu[k]=tu;in_nd[k]=east?0:1;moving=1;}}
        else if(dv>=2){UBYTE south=tv>in_nv[k];tv=south?in_nv[k]+1:in_nv[k]-1;if(!in_solid(in_nu[k],tv)){in_nv[k]=tv;in_nd[k]=south?2:3;moving=1;}}
        if(!moving)in_nt[k]=0;
        if(in_nt[k]>250)in_nt[k]=0;
        break;
    case TD_NB_FOLLOW:
        /* A step behind their teacher. */
        {UBYTE l=td_in_np_leader[s];
         if(l<in_count){tu=in_nu[l]+((k&1)?10:-10);tv=in_nv[l]+8+((k&2)?4:0);
            if(!(in_nt[k]&1)){
                if(in_nu[k]+1<tu&&!in_solid(in_nu[k]+1,in_nv[k])){in_nu[k]++;in_nd[k]=0;moving=1;}
                else if(in_nu[k]>tu+1&&!in_solid(in_nu[k]-1,in_nv[k])){in_nu[k]--;in_nd[k]=1;moving=1;}
                else if(in_nv[k]+1<tv&&!in_solid(in_nu[k],in_nv[k]+1)){in_nv[k]++;in_nd[k]=2;moving=1;}
                else if(in_nv[k]>tv+1&&!in_solid(in_nu[k],in_nv[k]-1)){in_nv[k]--;in_nd[k]=3;moving=1;}
            }}}
        break;
    case TD_NB_EXHIBIT:
        /* On a plinth, turning to show every pose. */
        td_people_pose(k,(in_nt[k]>>7)&3);
        f+=(in_nt[k]>>4)&1;
        in_place(a,in_nu[k],in_nv[k]);in_frame(a,f);
        return;
    }
    if(b==TD_NB_SIT){td_people_pose(k,TD_POSE_ACT);f+=(in_nt[k]>>5)&1;}
    else{
        td_people_pose(k,in_pose_of(in_nd[k]));
        if(in_nd[k]==1)f+=2;
        if(moving)f+=(in_nt[k]>>3)&1;
        else if(b==TD_NB_COUNTER)f+=(in_nt[k]>>6)&1;
    }
    in_place(a,in_nu[k],in_nv[k]);in_frame(a,f);
}

void td_interior_update(UBYTE motion) BANKED {
    BYTE dx,dy;UBYTE k,step,p;UWORD nu,nv;
    if(td_transition_pending)return;
    td_tick+=motion;
    if(in_talk_cool)in_talk_cool--;
    /* Answers and calls keep typing on the radio strip indoors. */
    if(TD_RADIO_DUE())td_radio_tick();
    if(in_ride){in_ride_tick(motion);if(in_ride)return;}
    for(k=0;k<in_count;k++)in_npc(k);
    if(!(actors[1].flags&ACTOR_FLAG_HIDDEN))in_frame(&actors[1],TD_FRAME_BEACON+((td_tick>>4)&1));
    if(!motion)return;
    dx=!!INPUT_RIGHT-!!INPUT_LEFT;dy=!!INPUT_DOWN-!!INPUT_UP;
    for(step=0;step<motion;step++){
        if(!dx&&!dy)break;
        nu=in_u+dx;nv=in_v+dy;
        /* Half a pixel a tick, the same pace as walking outside. */
        if((td_tick+step)&1)continue;
        if(dx&&in_free(nu,in_v))in_u=nu;
        if(dy&&in_free(in_u,nv))in_v=nv;
    }
    if(dx)td_walk_dir=dx>0?0:1;else if(dy)td_walk_dir=dy>0?2:3;
    in_frame(&PLAYER,TD_FRAME_COURIER_WALK+(td_walk_dir<<1)+((dx||dy)?((td_tick>>3)&1):0));
    in_place(&PLAYER,in_u,in_v);
    /* Out through the doors. */
    if(in_u>=td_in_exit[td_interior][0]&&in_u<td_in_exit[td_interior][2]&&in_v>=td_in_exit[td_interior][1]&&in_v<td_in_exit[td_interior][3]){
        /* The LookOut's stair door: 1,776 steps; the lift is quicker. */
        if(td_interior==TD_IN_CN_LOOKOUT){in_v-=4;if(!in_talk_cool){td_radio_speech("1,776 STEPS DOWN.","TAKE THE ELEVATOR.");in_talk_cool=120;}return;}
        in_leave();return;
    }
    if(!INPUT_A_PRESSED&&!INPUT_SELECT_PRESSED)return;
    p=in_point();
    if(p!=TD_NONE&&(td_in_pt_kind[p]==TD_IP_DESK||td_in_pt_kind[p]==TD_IP_ELEVATOR||td_in_pt_kind[p]==TD_IP_ARCH_NEXT||td_in_pt_kind[p]==TD_IP_ARCH_PREV||td_in_pt_kind[p]==TD_IP_EXHIBIT)){in_use(p);return;}
    k=in_person();
    if(k!=TD_NONE&&!in_talk_cool){in_talk(k);in_talk_cool=40;return;}
    if(p!=TD_NONE){in_use(p);return;}
    /* A courier with a parcel for this building is pointed to the desk. */
    if(in_delivers_here())td_message(TD_MSG_DESK);
}
