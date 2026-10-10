#pragma bank 255
/* Animation: tyre smoke, exhaust and dust, collection pops and sparkles on
 * two particle actors, and the driving camera. All
 * of it is presentation only: nothing here changes physics or the save.
 * Cost control: one banked call per rendered update, table-driven particle
 * ages, and the particle actors join GBVM's active list only while they are
 * shown, so idle effects cost the actor loops nothing. The night headlamps
 * are drawn by the courier vehicle's own lit frames (TORONTO.c). */
#include <string.h>
#include "td_life_int.h"
#include "td_anim.h"
#include "camera.h"
#include "input.h"

td_anim_part_t td_anim_parts[TD_PARTS];
UBYTE td_anim_pose_base,td_anim_pose_time;
BYTE td_look_x,td_look_y;
static UBYTE an_cool,an_side,an_live,an_launch;
static BYTE an_tx,an_ty;
typedef char td_anim_two_parts[(TD_PARTS==2)?1:-1];

/* Rear axle 7 px behind and the wheels 3 px either side, by heading
 * (E=0 clockwise, +y south). */
static const BYTE an_rear_u[16]={-7,-7,-5,-3,0,3,5,7,7,7,5,3,0,-3,-5,-7};
static const BYTE an_rear_v[16]={0,-3,-5,-7,-7,-7,-5,-3,0,3,5,7,7,7,5,3};
static const BYTE an_side_u[16]={0,-1,-2,-3,-3,-3,-2,-1,0,1,2,3,3,3,2,1};
static const BYTE an_side_v[16]={3,3,2,1,0,-1,-2,-3,-3,-3,-2,-1,0,1,2,3};
static const UBYTE an_life[5]={0,24,12,TD_PART_POP_TICKS,10};
static const UBYTE an_rank[5]={0,1,0,3,2};
/* Per remaining tick: frame offset for smoke (fresh puff, cloud, haze) and
 * the pixels a pop rises (16 in all: quickly, then every other tick). */
static const UBYTE an_smoke[25]={2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0};
static const UBYTE an_rise[31]={0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1};
typedef char td_anim_rise_16[(TD_PART_POP_TICKS==30)?1:-1];

/* GBVM's activate_actor resets the idle animation (frame 0, the courier's
 * car), so the particle's own frame is put back afterwards. */
static void an_show(actor_t *a){
    UBYTE f;
    if(!(a->flags&ACTOR_FLAG_ACTIVE)){
        f=a->frame_start;a->flags&=~(ACTOR_FLAG_DISABLED|ACTOR_FLAG_HIDDEN);activate_actor(a);
        a->frame=a->frame_start=f;a->frame_end=f+1;a->anim_tick=255;
    }else a->flags&=~ACTOR_FLAG_HIDDEN;
}
static void an_hide(actor_t *a){
    a->flags|=ACTOR_FLAG_HIDDEN;
    if(a->flags&ACTOR_FLAG_ACTIVE)deactivate_actor(a);
}

void td_anim_reset(void) BANKED {
    memset(td_anim_parts,0,sizeof(td_anim_parts));
    td_anim_pose_time=an_cool=an_live=0;an_launch=1;td_look_x=td_look_y=an_tx=an_ty=0;
    an_hide(&actors[TD_ACTOR_PARTS]);an_hide(&actors[TD_ACTOR_PARTS+1]);an_hide(&actors[TD_ACTOR_RETICLE]);td_aim_target=TD_NONE;
}

void td_anim_spawn(UBYTE kind,UBYTE frame,UWORD u,UWORD v) BANKED {
    td_anim_part_t *p=td_anim_parts;
    if(p[0].time&&p[1].time){
        /* Replace the lower-ranked (then older) particle, never a higher one. */
        if(an_rank[p[1].kind]<an_rank[p[0].kind]||(an_rank[p[1].kind]==an_rank[p[0].kind]&&p[1].time<p[0].time))p++;
        if(an_rank[kind]<an_rank[p->kind])return;
    }else if(p[0].time)p++;
    p->kind=kind;p->frame=frame;p->time=an_life[kind]+1;p->x=u<<5;p->y=v<<5;
    an_live|=p==td_anim_parts?1:2;
}

void td_anim_pose(UBYTE base,UBYTE ticks) BANKED {td_anim_pose_base=base;td_anim_pose_time=ticks;}

/* Smoke behind the rear axle; alternate wheels for a two-track trail. */
static void an_rear(UBYTE kind,UBYTE wheels){
    UBYTE h=td.heading&15;UWORD u=(td.u>>4)+an_rear_u[h],v=(td.v>>4)+an_rear_v[h];
    if(wheels){an_side^=1;if(an_side){u+=an_side_u[h];v+=an_side_v[h];}else{u-=an_side_u[h];v-=an_side_v[h];}}
    td_anim_spawn(kind,0,u,v);
}

/* Driving triggers: a launch from rest, hard braking, a braking slide
 * into a turn and a sliding tail. Smoke is spaced out so a particle is rarely alive for long. */
static void an_triggers(void){
    UBYTE a,h;
    if(td.onfoot||td.mode!=TD_ROAM||td_entry_timer||lf_down||lf_arrest||lf_hurt>22)return;
    /* A damaged engine smokes from the bonnet: wisps, then thick smoke. */
    if(td_car_damage>=TD_DAMAGE_SMOKE&&!(td_tick&(td_car_damage>=TD_DAMAGE_FAIL?30:62))){
        h=td.heading&15;
        td_anim_spawn(td_car_damage>=TD_DAMAGE_FAIL?TD_PART_SMOKE:TD_PART_PUFF,0,(td.u>>4)-an_rear_u[h],(td.v>>4)-an_rear_v[h]-2);
        an_cool=4;return;
    }
    if(!td.speed){an_launch=1;return;}
    a=td.speed<0?(UBYTE)-td.speed:(UBYTE)td.speed;
    if(INPUT_B&&!!INPUT_LEFT!=!!INPUT_RIGHT){if(a>10){an_rear(TD_PART_SMOKE,1);an_cool=8;}}
    else if(INPUT_B||lf_exit_req){if(td.speed>12){an_rear(TD_PART_SMOKE,1);an_cool=10;}}
    else if(a>12&&lf_abs(td_vx-lf_scale_x)+lf_abs(td_vy-lf_scale_y)>240){an_rear(TD_PART_SMOKE,1);an_cool=10;}
    else if(an_launch&&INPUT_A&&td.speed>0){an_launch=0;an_rear(TD_PART_PUFF,0);an_cool=12;}
}

/* Age, move and draw live particles. */
static void an_parts(void){
    td_anim_part_t *p=td_anim_parts;actor_t *a=&actors[TD_ACTOR_PARTS];UBYTE t,f,bit=1;
    do{
        if(an_live&bit){
            t=p->time;
            if(t<=1){p->time=0;an_live&=~bit;an_hide(a);}
            else{
                p->time=--t;f=p->frame;
                if(p->kind==TD_PART_POP){
                    if(an_rise[t])p->y-=32;
                    /* Pops blink out over their last ticks. */
                    if(t<10&&(t&2)){a->flags|=ACTOR_FLAG_HIDDEN;goto next;}
                }else if(p->kind==TD_PART_FLASH)f=TD_FRAME_SPARKLE+(t<5);
                else{
                    /* Smoke drifts up a pixel every eighth tick. */
                    if(!(t&7))p->y-=32;
                    f=TD_FRAME_SMOKE+(p->kind==TD_PART_PUFF?(t>=7?0:2):an_smoke[t]);
                }
                a->pos.x=p->x;a->pos.y=p->y;
                if(a->frame_start!=f){a->frame=a->frame_start=f;a->frame_end=f+1;}
                an_show(a);
            }
        }
next:
        p++;a++;bit<<=1;
    }while(bit<4);
}

void td_anim_update(void) BANKED {
    UBYTE cam=0;BYTE shake=0;WORD x;
    if(an_live)an_parts();
    if(an_cool)an_cool--;else if(!(td_tick&1))an_triggers();
    /* Look ahead of a moving vehicle (up to 24 px across, 16 px down the
     * screen), easing a pixel every other frame; an impact shakes the view.
     * The target follows the velocity every fourth frame. */
    if(!(td_tick&3)){
        an_tx=an_ty=0;
        if(!td.onfoot&&td.mode==TD_ROAM){
            x=td_vx>>4;an_tx=(BYTE)(x>24?24:x<-24?-24:x);
            x=td_vy>>4;an_ty=(BYTE)(x>16?16:x<-16?-16:x);
        }
    }
    if(lf_shake){lf_shake--;shake=lf_shake?((lf_shake&2)?2:-2):0;cam=1;}
    if((td_tick&1)&&(td_look_x!=an_tx||td_look_y!=an_ty)){
        if(td_look_x<an_tx)td_look_x++;else if(td_look_x>an_tx)td_look_x--;
        if(td_look_y<an_ty)td_look_y++;else if(td_look_y>an_ty)td_look_y--;
        cam=1;
    }
    if(cam){camera_offset_x=shake-td_look_x;camera_offset_y=-td_look_y;}
}
