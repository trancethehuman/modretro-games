#pragma bank 255
/* Special vehicles in traffic (TTC buses, box and garbage trucks,
 * ambulances, fire trucks) and the hidden ones parked by their landmarks
 * (td_hidden.h). They share the special tile block (td_special.c). */
#include "td_life_int.h"
#include "td_special.h"
#define TD_HIDDEN_DATA
#include "td_hidden.h"

UBYTE td_hidden_slot=TD_NONE;
static UBYTE vh_hidden_k;
static UWORD vh_rng=0xBEEF;
static UBYTE vh_random(void){
    vh_rng^=vh_rng<<7;vh_rng^=vh_rng>>9;vh_rng^=vh_rng<<8;
    return (UBYTE)vh_rng^(UBYTE)td_tick;
}

/* A slot coming into play may be a special vehicle: TTC buses, box and
 * garbage trucks most often, an ambulance or a fire truck now and then
 * (more often after violence, lights flashing). They share one tile block
 * (td_special.c), so a new kind only loads while no special vehicle is in
 * use; otherwise the slot takes the kind already loaded. */
static UBYTE vh_busy(UBYTE except){
    UBYTE k;
    if(td_player_special)return TRUE;
    for(k=0;k<6;k++)if(k!=except&&td_traffic_bases[k]==TD_FRAME_SPECIAL&&tr_mode[k]!=TR_GONE)return TRUE;
    return FALSE;
}
static const UBYTE vh_pal[TD_SPECIAL_KINDS]={TD_PAL_RED,TD_PAL_RED,TD_PAL_RED,TD_PAL_YELLOW,TD_PAL_TEAL,TD_PAL_TEAL};
static void vh_paint(UBYTE i){
    UBYTE pal=vh_pal[td_special_kind-1];
    /* Box trucks come in yellow or blue. */
    if(td_special_kind==TD_SPECIAL_BOX_TRUCK&&(vh_random()&1))pal=TD_PAL_BLUE;
    td_traffic_bases[i]=TD_FRAME_SPECIAL;TD_PALETTE(&actors[2+i])=pal;
}
/* Near a hidden vehicle's spot, traffic leaves the tile block to it. */
static UBYTE vh_near_hidden(void){
    UBYTE k;UWORD pu=td.u>>4,pv=td.v>>4;
    for(k=0;k<TD_HIDDEN_COUNT;k++)
        if(td_hidden_scene[k]==td.district&&lf_dist(td_hidden_u[k],pu)<360&&lf_dist(td_hidden_v[k],pv)<320)return TRUE;
    return FALSE;
}
void td_veh_special(UBYTE i) BANKED {
    UBYTE r=vh_random()&15,kind;
    if(vh_busy(i)){if(td_special_kind&&td_special_kind!=TD_SPECIAL_TANK)vh_paint(i);return;}
    if(vh_near_hidden())return;
    kind=r<5?TD_SPECIAL_BUS:r<9?TD_SPECIAL_BOX_TRUCK:r<12?TD_SPECIAL_GARBAGE_TRUCK:r<14?TD_SPECIAL_AMBULANCE:TD_SPECIAL_FIRE_TRUCK;
    if(lf_chaos>=4&&(r&1))kind=TD_SPECIAL_AMBULANCE;
    td_special_load(kind);vh_paint(i);
}

/* Hidden special vehicles (td_hidden.h) stand parked by their landmarks:
 * one is put there when the courier comes near while the spot is out of
 * view, borrowing a traffic slot that is out of view too, and goes back to
 * being ordinary traffic once the courier has left. Taken, it is theirs. */
void td_veh_hidden(void) BANKED {
    UBYTE k,i;UWORD pu=td.u>>4,pv=td.v>>4,u,v;
    if(td_hidden_slot!=TD_NONE){
        i=td_hidden_slot;
        if(tr_mode[i]!=TR_STILL&&tr_mode[i]!=TR_PUSH){td_hidden_slot=TD_NONE;return;}
        u=td_hidden_u[vh_hidden_k];v=td_hidden_v[vh_hidden_k];
        if((lf_dist(u,pu)>320||lf_dist(v,pv)>280)&&!lf_on_screen(td_traffic_u[i]>>4,td_traffic_v[i]>>4)){
            tr_mode[i]=TR_PARK;td_hidden_slot=TD_NONE;
        }
        return;
    }
    if(td_player_special)return;
    for(k=0;k<TD_HIDDEN_COUNT;k++){
        if(td_hidden_scene[k]!=td.district)continue;
        u=td_hidden_u[k];v=td_hidden_v[k];
        if(lf_dist(u,pu)>240||lf_dist(v,pv)>200||lf_on_screen(u,v))continue;
        if(vh_busy(TD_NONE)&&td_special_kind!=td_hidden_kind[k]){
            /* The block holds another kind: take it back once every vehicle
             * showing it is out of view (they turn into ordinary traffic). */
            for(i=0;i<6;i++)
                if(td_traffic_bases[i]==TD_FRAME_SPECIAL&&((td_tr_ctrl&(1<<i))||lf_on_screen(td_traffic_u[i]>>4,td_traffic_v[i]>>4)))break;
            if(i<6||td_player_special)continue;
            for(i=0;i<6;i++)if(td_traffic_bases[i]==TD_FRAME_SPECIAL)td_lf_new_look(i,vh_random());
        }
        for(i=0;i<6;i++){
            if(i==TD_POLICE_SLOT||(td_tr_ctrl&(1<<i)))continue;
            if(!lf_on_screen(td_traffic_u[i]>>4,td_traffic_v[i]>>4))break;
        }
        if(i==6)return;
        td_lf_own_car(i,TR_STILL);
        td_traffic_u[i]=u<<4;td_traffic_v[i]=v<<4;tr_head[i]=td_hidden_head[k];
        td_special_load(td_hidden_kind[k]);vh_paint(i);
        td_hidden_slot=i;vh_hidden_k=k;
        return;
    }
}

