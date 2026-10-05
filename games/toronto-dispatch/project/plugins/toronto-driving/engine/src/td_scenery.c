#pragma bank 255
#include <string.h>
#include "td_scenery.h"
#include "td_game.h"
#include "td_district.h"
#ifdef CGB
#include "data_manager.h"
#include "gbs_types.h"
#include "scroll.h"
#include "compat.h"
#endif

/* Original scenery data, generated from final visible furniture and raw terrain.
 * Signed bank1 BKG80..97 is separate from aircraft64..78, lights47/48 and
 * modal UI192..252. No OBJ, actor slot or save-layout expansion is used. */
#define TD_SCENERY_FIRST 80
#define TD_SCENERY_PATCHES 18
#define TD_SCENERY_DISTRICTS 7
#define TD_SCENERY_FLASHES 4
typedef struct { UBYTE x,y,kind,ground; } td_prop_t;
#include "td_scenery_data.h"
#define TD_SCENERY_PROPS (sizeof(td_props)/sizeof(td_props[0]))
typedef struct { UWORD offset; UBYTE tile,attr; } td_prop_patch_t;
typedef struct { UBYTE district,id,ticks; } td_prop_flash_t;
/* Absolute source indices pack district boundaries into the same byte safely.
 *1135 props use142 bytes; no district-local padding or saved fields are added. */
static UBYTE td_broken[(TD_SCENERY_PROPS+7)/8];
#ifdef CGB
static td_prop_patch_t td_prop_patches[TD_SCENERY_PATCHES];
static UBYTE td_prop_patch_count;
static UBYTE *td_prop_map;
static far_ptr_t td_prop_scene;
static UBYTE td_prop_same_scene(void){
    return current_scene.bank==td_prop_scene.bank&&current_scene.ptr==td_prop_scene.ptr;
}
#endif
static td_prop_flash_t td_prop_flashes[TD_SCENERY_FLASHES];
/* Low2 bits select the four-entry flash ring. Bit7 records whether any prop
 * has broken since cold reset, so pristine views need no geometry/VRAM work.
 * Render/scene resets preserve this flag with all persistent district bits. */
static UBYTE td_prop_flash_next;

#ifdef __SDCC
typedef char td_scenery_native_static_budget[(sizeof(td_broken)+sizeof(td_prop_patches)+
    sizeof(td_prop_flashes)+sizeof(td_prop_patch_count)+sizeof(td_prop_flash_next)+
    sizeof(td_prop_map)+sizeof(td_prop_scene)==233)?1:-1];
#endif
static UBYTE td_prop_dead(UWORD index){
    return (td_broken[index>>3]>>(index&7))&1;
}
static UWORD td_prop_lower(UBYTE district,UWORD y,UWORD x){
    UWORD first=td_prop_offsets[district],end=td_prop_offsets[district+1],mid;
    while(first<end){mid=first+(end-first)/2;if(td_props[mid].y<y||(td_props[mid].y==y&&td_props[mid].x<x))first=mid+1;else end=mid;}
    return first;
}
/* Exact segment against expanded whole-body prop rectangle. Products are
 * signed32-bit, so fractional diagonal corner approaches do not disappear. */
static UBYTE td_prop_sweep(WORD ou,WORD ov,WORD u,WORD v,WORD left,WORD top,WORD right,WORD bottom){
    WORD du=u-ou,dv=v-ov,edge,lo,hi;UBYTE axis,k;long n,a,b,t;
    if((ou<left&&u<left)||(ou>right&&u>right)||(ov<top&&v<top)||(ov>bottom&&v>bottom))return FALSE;
    if((ou>=left&&ou<=right&&ov>=top&&ov<=bottom)||(u>=left&&u<=right&&v>=top&&v<=bottom))return TRUE;
    for(axis=0;axis<2;axis++){
        if(!(axis?dv:du))continue;
        for(k=0;k<2;k++){
            edge=axis?(k?bottom:top):(k?right:left);
            lo=axis?ov:ou;hi=axis?v:u;
            if(edge<(lo<hi?lo:hi)||edge>(lo>hi?lo:hi))continue;
            n=(long)(axis?du:dv)*(edge-lo);
            a=(long)((axis?left:top)-(axis?ou:ov))*(axis?dv:du);
            b=(long)((axis?right:bottom)-(axis?ou:ov))*(axis?dv:du);
            if(a>b){t=a;a=b;b=t;}if(n>=a&&n<=b)return TRUE;
        }
    }
    return FALSE;
}
static UWORD td_prop_distance(WORD u,WORD v,WORD x,WORD y){
    WORD du=u-x,dv=v-y;return (du<0?-du:du)+(dv<0?-dv:dv);
}
void td_scenery_reset(void) BANKED {
    memset(td_broken,0,sizeof(td_broken));memset(td_prop_flashes,0,sizeof(td_prop_flashes));
    td_prop_flash_next=0;td_scenery_render_reset();
}
void td_scenery_render_reset(void) BANKED {
#ifdef CGB
    td_prop_patch_count=0;
#endif
}
UBYTE td_scenery_contact(UWORD old_u,UWORD old_v,UWORD u,UWORD v,UBYTE half,BYTE speed) BANKED {
    UBYTE district=td_district_current(),id;UWORD i,end,lower,upper,row,left,right;WORD x,y,pad,du,dv;
    td_prop_flash_t *flash;
    if(district>=TD_SCENERY_DISTRICTS||half>16||old_u>16383||u>16383||old_v>15615||v>15615)return TD_SCENERY_BLOCK;
    du=(WORD)u-old_u;dv=(WORD)v-old_v;
    if(du>16||du< -16||dv>16||dv< -16)return TD_SCENERY_BLOCK;
    if(!du&&!dv)return TD_SCENERY_CLEAR;
    pad=(WORD)half*16+64;
    lower=(old_v<v?old_v:v);lower=lower>pad?((lower-pad)>>7):0;
    upper=((old_v>v?old_v:v)+pad)>>7;
    left=old_u<u?old_u:u;left=left>pad?(left-pad)>>7:0;
    right=((old_u>u?old_u:u)+pad)>>7;
    end=td_prop_offsets[district+1];
    /* <=4x4 candidate cells at half7 and <=1px movement, never256 actors.
     * Wider valid half16 probes remain capped at6x6 candidate cells. */
    for(row=lower;row<=upper;row++)for(i=td_prop_lower(district,row,left);
        i<end&&td_props[i].y==row&&td_props[i].x<=right;i++){
        id=i-td_prop_offsets[district];if(td_prop_dead(i))continue;
        x=(WORD)td_props[i].x*128+64;y=(WORD)td_props[i].y*128+64;
        if(!td_prop_sweep(old_u,old_v,u,v,x-pad,y-pad,x+pad-1,y+pad-1))continue;
        /* A loaded/parked body can overlap a prop. Always let it move away. */
        if(old_u>=x-pad&&old_u<x+pad&&old_v>=y-pad&&old_v<y+pad&&
           td_prop_distance(u,v,x,y)>=td_prop_distance(old_u,old_v,x,y))continue;
        if(speed>-3&&speed<3)return TD_SCENERY_BLOCK;
        td_broken[i>>3]|=1<<(i&7);
        flash=&td_prop_flashes[td_prop_flash_next&3];flash->district=district;flash->id=id;flash->ticks=24;
        td_prop_flash_next=((td_prop_flash_next+1)&3)|128;
        return TD_SCENERY_BROKE;
    }
    return TD_SCENERY_CLEAR;
}
void td_scenery_restore(void) BANKED {
#ifdef CGB
    UBYTE i,tile,attr,save_vbk=VBK_REG&1;td_prop_patch_t *patch;
    if(!td_prop_patch_count)return;
    if(td_prop_same_scene())for(i=0;i<td_prop_patch_count;i++){
        patch=&td_prop_patches[i];VBK_REG=0;tile=get_vram_byte(td_prop_map+patch->offset);
        VBK_REG=1;attr=get_vram_byte(td_prop_map+patch->offset);
        /* Scroll, atlas, a shop or another overlay wins ownership. */
        if(tile==TD_SCENERY_FIRST+i&&attr==((patch->attr&7)|8)){
            set_vram_byte(td_prop_map+patch->offset,patch->attr);
            VBK_REG=0;set_vram_byte(td_prop_map+patch->offset,patch->tile);
        }
    }
    td_prop_patch_count=0;VBK_REG=save_vbk;
#endif
}
#ifdef CGB
static UBYTE td_prop_phase(UBYTE district,UBYTE id){
    UBYTE i;for(i=0;i<TD_SCENERY_FLASHES;i++)if(td_prop_flashes[i].ticks&&
        td_prop_flashes[i].district==district&&td_prop_flashes[i].id==id)
        return td_prop_flashes[i].ticks>16?2:td_prop_flashes[i].ticks>8?1:0;
    return 0;
}
/* Compose one original8x8 ground tile with scattered fragments. Bright flash,
 * falling fragments, then low rubble; the original underlay remains visible. */
static void td_prop_pixels(const td_prop_t *prop,UBYTE phase,UBYTE *pixels){
    UBYTE row,mask,shade=prop->kind==1?2:3;
    memcpy(pixels,td_prop_ground[prop->ground],16);
    for(row=0;row<8;row++){
        mask=phase==2?td_prop_flash[row]:phase==1?td_prop_falling[row]:td_prop_rubble[prop->kind&1][row];
        pixels[row*2]=(pixels[row*2]&~mask)|(shade&1?mask:0);
        pixels[row*2+1]=(pixels[row*2+1]&~mask)|(shade&2?mask:0);
    }
}
#endif
void td_scenery_render(void) BANKED {
#ifdef CGB
    UBYTE district,id,i,save_vbk=VBK_REG&1,pixels[16],attr;
    UWORD index,end,lower,upper,offset;WORD x,y;td_prop_patch_t *patch;
    if(!(td_prop_flash_next&128))return;
    if(td.mode!=TD_ROAM&&td.mode!=TD_WAIT&&td.mode!=TD_RIDE)return;
    district=td_district_current();
    if(district>=TD_SCENERY_DISTRICTS||draw_scroll_x< -160||draw_scroll_x>1024||draw_scroll_y< -144||draw_scroll_y>976)return;
    lower=draw_scroll_y>7?(draw_scroll_y-7)>>3:0;upper=(draw_scroll_y+143)>>3;
    end=td_prop_offsets[district+1];td_prop_map=GetBkgAddr();td_prop_scene=current_scene;
    for(index=td_prop_lower(district,lower,0);index<end&&td_props[index].y<=upper;index++){
        id=index-td_prop_offsets[district];if(!td_prop_dead(index))continue;
        x=td_props[index].x*8;y=td_props[index].y*8;
        if(x>draw_scroll_x+159||x+7<draw_scroll_x||y>draw_scroll_y+143||y+7<draw_scroll_y)continue;
        if(td_prop_patch_count==TD_SCENERY_PATCHES)break;
        offset=((UWORD)(td_props[index].y&31)<<5)|(td_props[index].x&31);
        VBK_REG=1;attr=get_vram_byte(td_prop_map+offset);if(attr&128)continue;
        patch=&td_prop_patches[td_prop_patch_count];patch->offset=offset;patch->attr=attr;
        VBK_REG=0;patch->tile=get_vram_byte(td_prop_map+offset);
        td_prop_pixels(&td_props[index],td_prop_phase(district,id),pixels);
        VBK_REG=1;set_bkg_data(TD_SCENERY_FIRST+td_prop_patch_count,1,pixels);
        set_vram_byte(td_prop_map+offset,(attr&7)|8);VBK_REG=0;
        set_vram_byte(td_prop_map+offset,TD_SCENERY_FIRST+td_prop_patch_count);td_prop_patch_count++;
    }
    for(i=0;i<TD_SCENERY_FLASHES;i++)if(td_prop_flashes[i].ticks)td_prop_flashes[i].ticks--;
    VBK_REG=save_vbk;
#endif
}
