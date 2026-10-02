/* Independent full-pixel and OAM checks around the actual native source. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "host_aircraft.h"
#include "td_game.h"
#include "td_aircraft.h"
#include "art_oracle.h"

actor_t actors[21],*actors_inactive_head;
UBYTE actors_len,allocated_hardware_sprites,VBK_REG,__render_shadow_OAM;
UBYTE win_pos_x,win_pos_y,win_dest_pos_x,win_dest_pos_y,WX_REG,WY_REG;
_Alignas(256) volatile OAM_item_t shadow_OAM[40];
_Alignas(256) volatile OAM_item_t shadow_OAM2[40];
WORD draw_scroll_x,draw_scroll_y;
far_ptr_t current_scene;
td_state_t td;
td_aircraft_state_t td_aircraft;
static UBYTE district,selected_frame;
static UBYTE address_page[1024],maps[2][1024],base_maps[2][1024];
static UBYTE bkg_data[2][256][16],obj_data[2][256][16];
static tileset_t rom_tiles[2];
static metasprite_t poses[14][5];
static const metasprite_t *frames[14];
static spritesheet_t sheet={14,frames,{8,&rom_tiles[0]},{9,&rom_tiles[1]}};
static unsigned long checks;
static unsigned long metadata_reads,rom_tile_bytes;
static void require(int result,const char *message){
    checks++;
    if(!result){fprintf(stderr,"FAIL after %lu checks: %s\n",checks,message);exit(1);}
}
UBYTE td_district_current(void){return district;}
UBYTE td_aircraft_frame(void){return selected_frame;}
void deactivate_actor(actor_t *actor){
    actor_t *head=actors_inactive_head;
    actor->flags&=~ACTOR_FLAG_ACTIVE;
    actor->next=head;actor->prev=NULL;
    if(head)head->prev=actor;
    actors_inactive_head=actor;
}
void MemcpyBanked(void *dest,const void *src,size_t length,UBYTE bank){
    require(bank>=7&&bank<=9,"Compiled aircraft bank changed during ROM reads");
    if(bank==7)metadata_reads++;
    else if(length==32)rom_tile_bytes+=length;
    memcpy(dest,src,length);
}
UBYTE ReadBankedUBYTE(const UBYTE *src,UBYTE bank){
    require(bank==7,"Compiled aircraft bank changed during frame-count read");metadata_reads++;return *src;
}
UBYTE *GetBkgAddr(void){return address_page;}
UBYTE get_vram_byte(UBYTE *addr){
    ptrdiff_t index=addr-address_page;
    require(index>=0&&index<1024&&VBK_REG<2,"VRAM map read out of bounds");return maps[VBK_REG][index];
}
void set_vram_byte(UBYTE *addr,UBYTE value){
    ptrdiff_t index=addr-address_page;
    require(index>=0&&index<1024&&VBK_REG<2,"VRAM map write out of bounds");maps[VBK_REG][index]=value;
}
void get_bkg_data(UBYTE first,UBYTE count,UBYTE *data){
    require(count==1&&VBK_REG<2,"BKG read outside one complete tile");memcpy(data,bkg_data[VBK_REG][first],16);
}
void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *data){
    require(count==1&&VBK_REG==1&&first>=32&&first<=46,"BKG scratch allocation crossed native ownership");
    memcpy(bkg_data[VBK_REG][first],data,16);
}
void get_sprite_data(UBYTE first,UBYTE count,UBYTE *data){
    (void)first;(void)count;(void)data;
    require(0,"Cached renderer must read compiled ROM, not OBJ VRAM");
}

#include "renderer_under_test.c"

static void fixture(UBYTE object_flip){
    UBYTE frame,i,count,cx,cy,x,y,value,bank,tile=16,last_x,last_y;
    memset(obj_data,0,sizeof(obj_data));
    for(frame=0;frame<14;frame++){
        frames[frame]=poses[frame];count=frame==13?0:frame==12?2:4;last_x=last_y=0;
        for(i=0;i<count;i++){
            if(frame==12){cx=8+i*8;cy=8;}
            else if(frame%4==0||frame%4==2){cx=i*8;cy=8;}
            else{cx=8+(i&1)*8;cy=(i>>1)*16;}
            poses[frame][i].dx=(BYTE)(cx-8-last_x);poses[frame][i].dy=(BYTE)(cy-last_y);
            poses[frame][i].dtile=tile-16;bank=(i+frame)&1;
            poses[frame][i].props=bank*8|(object_flip<<5)|((frame+i)&7)|0x80;
            last_x=cx-8;last_y=cy;
            for(y=0;y<16;y++)for(x=0;x<8;x++){
                value=original_pixels[frame][cy+(object_flip&2?15-y:y)][cx+(object_flip&1?7-x:x)];
                if(value&1)obj_data[bank][tile+y/8][(y&7)*2]|=1<<(7-x);
                if(value&2)obj_data[bank][tile+y/8][(y&7)*2+1]|=1<<(7-x);
            }
            tile+=2;
        }
        poses[frame][count].dy=metasprite_end;
    }
    for(bank=0;bank<2;bank++){
        rom_tiles[bank].n_tiles=100;
        memcpy(rom_tiles[bank].tiles,obj_data[bank][16],100*16);
    }
}

static void reset(UBYTE flip){
    UWORD i;UBYTE bank,tile,row,x,value;
    td_aircraft_render_reset();memset(&td,0,sizeof(td));memset(&td_aircraft,0,sizeof(td_aircraft));
    memset(actors,0,sizeof(actors));memset((void*)shadow_OAM,0,sizeof(shadow_OAM));
    memset((void*)shadow_OAM2,0,sizeof(shadow_OAM2));
    fixture(flip);district=0;actors_len=3;current_scene.bank=9;current_scene.ptr=&district;
    actors[2].sprite.bank=7;actors[2].sprite.ptr=&sheet;actors[2].base_tile=16;
    actors_inactive_head=&actors[1];actors[1].next=&actors[2];actors[2].prev=&actors[1];
    td_aircraft_render_bind();
    require(actors_inactive_head==&actors[1]&&!actors[1].next&&!actors[2].prev,"Binding lost the Queen loader or left aircraft linked");
    td_aircraft.active=1;td.mode=TD_ROAM;selected_frame=0;
    draw_scroll_x=184;draw_scroll_y=184;
    __render_shadow_OAM=(UBYTE)((UWORD)(uintptr_t)shadow_OAM>>8);allocated_hardware_sprites=0;VBK_REG=0;
    win_pos_x=win_dest_pos_x=0;win_pos_y=win_dest_pos_y=144;WX_REG=0;WY_REG=144;
    for(bank=0;bank<2;bank++)for(tile=0;tile<32;tile++)for(row=0;row<8;row++){
        bkg_data[bank][tile][row*2]=bkg_data[bank][tile][row*2+1]=0;
        for(x=0;x<8;x++){
            value=1+((tile+row*3+x*5+bank)%3);
            if(value&1)bkg_data[bank][tile][row*2]|=1<<(7-x);
            if(value&2)bkg_data[bank][tile][row*2+1]|=1<<(7-x);
        }
    }
    for(i=0;i<1024;i++){maps[0][i]=i%16;maps[1][i]=0x80|((i&1)<<3)|(i&7);}
}

static UBYTE background_pixel(const UBYTE map[2][1024],WORD wx,WORD wy){
    UWORD position=(((UWORD)wy>>3)&31)*32+(((UWORD)wx>>3)&31);
    UBYTE attr=map[1][position],tile=map[0][position],x=wx&7,y=wy&7,*data;
    if(attr&0x20)x=7-x;
    if(attr&0x40)y=7-y;
    data=bkg_data[(attr>>3)&1][tile];
    return ((data[y*2]>>(7-x))&1)|(((data[y*2+1]>>(7-x))&1)<<1);
}
static UBYTE object_pixel(const OAM_item_t *object,WORD screen_x,WORD screen_y){
    WORD x=screen_x-(WORD)object->x+8,y=screen_y-(WORD)object->y+16;
    UBYTE *data;
    if(!object->y||x<0||x>=8||y<0||y>=16)return 0;
    if(object->prop&0x20)x=7-x;
    if(object->prop&0x40)y=15-y;
    data=obj_data[(object->prop>>3)&1][(object->tile&0xfe)+(y>>3)];
    return ((data[(y&7)*2]>>(7-x))&1)|(((data[(y&7)*2+1]>>(7-x))&1)<<1);
}
static void pixel_checks(WORD centre_x,WORD centre_y,UBYTE frame,UBYTE bg_flip,UBYTE obj_flip){
    WORD wx,wy;UBYTE original,actual,air,i,original_attr,actual_attr;
    UWORD cell;
    reset(obj_flip);selected_frame=frame;td_aircraft.u=centre_x*16;td_aircraft.v=centre_y*16;
    for(cell=0;cell<1024;cell++)maps[1][cell]|=bg_flip<<5;
    memcpy(base_maps,maps,sizeof(maps));VBK_REG=1;td_aircraft_render();
    require(VBK_REG==1,"Renderer failed to preserve incoming VRAM bank");
    require(allocated_hardware_sprites==6,"Uncrowded flight must preserve optional two-object shadow");
    require(td_aircraft_patch_count<=15,"Flyby exhausted bounded roof scratch tiles");
    for(i=0;i<6;i++)require(!(shadow_OAM[i].prop&0x80),"Flyby/shadow OBJ priority hides it on nonzero road pixels");
    for(wy=centre_y-16;wy<centre_y+16;wy++)for(wx=centre_x-16;wx<centre_x+16;wx++){
        air=original_pixels[frame][wy-centre_y+16][wx-centre_x+16];
        original=background_pixel(base_maps,wx,wy);actual=background_pixel(maps,wx,wy);
        require(actual==(air?0:original),"Roof mask changed an uncovered pixel or missed an opaque flight pixel");
        cell=(((UWORD)wy>>3)&31)*32+(((UWORD)wx>>3)&31);
        original_attr=base_maps[1][cell];actual_attr=maps[1][cell];
        require((actual_attr&0x87)==(original_attr&0x87),"Roof palette/priority changed under aircraft");
        original=0;
        for(i=0;i<4;i++){
            OAM_item_t object=shadow_OAM[i];
            actual=object_pixel(&object,wx-draw_scroll_x,wy-draw_scroll_y);
            if(actual&&!original)original=actual;
        }
        require(original==air,"Loaded OBJ masks or accumulated offsets disagree with original sprite pixels");
    }
    td_aircraft_render_restore();
    require(VBK_REG==1&&!memcmp(base_maps,maps,sizeof(maps)),"Roof restore did not recover exact original map and attributes");
}

static void restoration_checks(void){
    UWORD offset;UBYTE index;
    reset(0);td_aircraft.u=264*16;td_aircraft.v=264*16;memcpy(base_maps,maps,sizeof(maps));
    td_aircraft_render();require(td_aircraft_patch_count>1,"Restore fixture did not touch distinct priority tiles");
    offset=td_aircraft_patches[0].offset;maps[0][offset]=3;maps[1][offset]=8;
    base_maps[0][offset]=3;base_maps[1][offset]=8;
    offset=td_aircraft_patches[1].offset;maps[1][offset]=8;
    base_maps[0][offset]=maps[0][offset];base_maps[1][offset]=8;
    td_aircraft_render_restore();require(!memcmp(base_maps,maps,sizeof(maps)),"Fresh scroll/atlas writes lost to old scratch restore");
    td_aircraft_render_restore();require(!td_aircraft_patch_count,"Restore must be idempotent");
    td_aircraft_render();index=td_aircraft_patch_count;
    current_scene.bank++;td_aircraft_render_restore();
    require(!td_aircraft_patch_count&&index>0,"Scene replacement must discard cached old tile references");
    allocated_hardware_sprites=0;td_aircraft_render();require(!allocated_hardware_sprites,"Old bound sprite escaped into a different scene");
}

static void binding_checks(void){
    reset(0);td_aircraft_render_reset();district=2;actors_len=2;
    actors[1].sprite=actors[2].sprite;actors[1].base_tile=16;actors[1].flags=ACTOR_FLAG_ACTIVE;
    actors_inactive_head=NULL;actors[1].next=actors[1].prev=NULL;
    td_aircraft_render_bind();require(td_aircraft_bound&&actors_inactive_head==NULL,"Active High Park loader was not removed from slot1");
    district=255;td_aircraft_render_bind();require(!td_aircraft_bound,"Unknown scene must not bind a flight loader");
}

static void capacity_checks(void){
    UBYTE i,mode;OAM_item_t ground[40];
    reset(0);td_aircraft.u=264*16;td_aircraft.v=264*16;
    for(i=0;i<40;i++){ground[i].x=40+i;ground[i].y=120;ground[i].tile=i*2;ground[i].prop=i&7;shadow_OAM[i]=ground[i];}
    allocated_hardware_sprites=37;td_aircraft_render();
    require(allocated_hardware_sprites==37&&!td_aircraft_patch_count&&!memcmp((void*)shadow_OAM,ground,37*4),"OAM-full frame must preserve all ground actors");
    allocated_hardware_sprites=35;td_aircraft_render();
    require(allocated_hardware_sprites==39,"OAM pressure must suppress only optional shadow first");
    for(i=0;i<35;i++)require(!memcmp((void*)&shadow_OAM[i+4],&ground[i],4),"Prepending aircraft changed existing ground OAM order/data");
    td_aircraft_render_restore();reset(0);td_aircraft.u=264*16;td_aircraft.v=264*16;
    for(i=0;i<7;i++){shadow_OAM[i].y=88;shadow_OAM[i].x=0;}
    allocated_hardware_sprites=7;td_aircraft_render();
    require(allocated_hardware_sprites==7&&!td_aircraft_patch_count,"10-per-line count must include X-hidden ground objects");
    reset(0);td_aircraft.u=264*16;td_aircraft.v=264*16;
    for(i=0;i<9;i++){shadow_OAM[i].y=112;shadow_OAM[i].x=40;}
    allocated_hardware_sprites=9;td_aircraft_render();
    require(allocated_hardware_sprites==13,"Crowded shadow scanlines must retain aircraft and ground without shadow");
    for(mode=TD_PAUSE;mode<=TD_HELP;mode++)if(mode!=TD_WAIT&&mode!=TD_RIDE){
        reset(0);td_aircraft.u=264*16;td_aircraft.v=264*16;td.mode=mode;memcpy(base_maps,maps,sizeof(maps));
        td_aircraft_render();require(!allocated_hardware_sprites&&!memcmp(base_maps,maps,sizeof(maps)),"Modal UI must suppress flight VRAM/OAM changes");
    }
    reset(0);td_aircraft.u=264*16;td_aircraft.v=264*16;
    __render_shadow_OAM=(UBYTE)((UWORD)(uintptr_t)shadow_OAM2>>8);td_aircraft_render();
    require(shadow_OAM2[0].y&&!shadow_OAM[0].y,"Flyby wrote displayed OAM instead of the selected render buffer");
}

static void edge_checks(void){
    WORD x,y;UBYTE i,frame;
    for(frame=0;frame<4;frame++)for(y=-24;y<=168;y+=8)for(x=-24;x<=184;x+=8){
        reset(frame);selected_frame=frame;draw_scroll_x=248;draw_scroll_y=248;
        td_aircraft.u=(248+x)*16;td_aircraft.v=(248+y)*16;td_aircraft_render();
        for(i=0;i<allocated_hardware_sprites;i++){
            require(!shadow_OAM[i].y||(shadow_OAM[i].x>0&&shadow_OAM[i].x<168&&shadow_OAM[i].y<160),"Edge flyby wrapped through OAM byte conversion");
        }
        require(td_aircraft_patch_count<=15,"Wrapped tilemap edge exceeded roof scratch ceiling");
        td_aircraft_render_restore();
    }
    reset(0);draw_scroll_x=draw_scroll_y=0;td_aircraft.u=td_aircraft.v=0;td_aircraft_render();
    require(td_aircraft_patch_count<=15,"Negative body world footprint escaped tilemap bounds");
    td_aircraft_render_restore();
}

static void window_checks(void){
    WORD centre_y;UBYTE frame,i;
    for(frame=0;frame<4;frame++)for(centre_y=90;centre_y<=150;centre_y++){
        reset(0);selected_frame=frame;td_aircraft.u=264*16;td_aircraft.v=(184+centre_y)*16;
        win_pos_y=win_dest_pos_y=120;WX_REG=7;WY_REG=120;
        td_aircraft_render();
        for(i=0;i<allocated_hardware_sprites;i++)require(!shadow_OAM[i].y||shadow_OAM[i].y-1<120,"Flyby or shadow overlaps handheld HUD window");
        td_aircraft_render_restore();
    }
    reset(0);td_aircraft.u=264*16;td_aircraft.v=294*16;
    win_pos_y=144;win_dest_pos_y=116;td_aircraft_render();
    require(!allocated_hardware_sprites&&!td_aircraft_patch_count,"Window slide destination was not reserved before ui_update");
    reset(0);td_aircraft.u=264*16;td_aircraft.v=294*16;
    win_pos_y=win_dest_pos_y=144;WX_REG=7;WY_REG=116;td_aircraft_render();
    require(!allocated_hardware_sprites&&!td_aircraft_patch_count,"Old displayed window was ignored during an instant UI change");
    reset(0);td_aircraft.u=264*16;td_aircraft.v=264*16;
    win_pos_y=win_dest_pos_y=100;td_aircraft_render();
    require(allocated_hardware_sprites==4,"HUD boundary should suppress shadow independently of visible aircraft");
}

static void untouched_ground_checks(void){
    UWORD i;
    reset(0);td_aircraft.u=264*16;td_aircraft.v=264*16;
    for(i=0;i<1024;i++)maps[1][i]&=0x7f;
    memcpy(base_maps,maps,sizeof(maps));td_aircraft_render();
    require(allocated_hardware_sprites==6&&!td_aircraft_patch_count&&!memcmp(base_maps,maps,sizeof(maps)),"Nonpriority road tiles must not be punched");
    reset(0);td_aircraft.u=264*16;td_aircraft.v=264*16;
    memset(bkg_data,0,sizeof(bkg_data));memcpy(base_maps,maps,sizeof(maps));td_aircraft_render();
    require(allocated_hardware_sprites==6&&!td_aircraft_patch_count&&!memcmp(base_maps,maps,sizeof(maps)),"Already-transparent roofs must not consume scratch tiles");
}

static void cache_checks(void){
    unsigned long reads,bytes;UBYTE frame;
    reset(3);selected_frame=4;td_aircraft.u=72*16;td_aircraft.v=264*16;
    bytes=rom_tile_bytes;td_aircraft_render();
    require(td_aircraft_cache_ready&&!allocated_hardware_sprites,"New offscreen flight should prefetch compiled poses without drawing");
    require(rom_tile_bytes-bytes==256,"Helicopter must fetch exactly two four-object compiled ROM poses");
    reads=metadata_reads;bytes=rom_tile_bytes;
    td_aircraft.u=264*16;
    for(frame=0;frame<80;frame++){
        selected_frame=frame&1?8:4;allocated_hardware_sprites=0;td_aircraft_render();
        require(allocated_hardware_sprites==6,"Cached rotor phase lost its native objects");
        td_aircraft_render_restore();
    }
    require(metadata_reads==reads&&rom_tile_bytes==bytes,"Cached rotor frames repeated banked metadata/tileset reads");
    selected_frame=0;allocated_hardware_sprites=0;td_aircraft_render();
    require(rom_tile_bytes-bytes==128,"New plane direction/kind did not refresh exactly one pose");
    td_aircraft_render_restore();
    selected_frame=12;allocated_hardware_sprites=0;bytes=rom_tile_bytes;td_aircraft_render();
    require(!allocated_hardware_sprites&&rom_tile_bytes==bytes,"Invalid aircraft pose must not draw or fetch data");
    td_aircraft_render_reset();require(!td_aircraft_cache_ready,"Scene graphics reset retained stale pose cache");
    district=2;current_scene.bank++;actors_len=2;actors_inactive_head=&actors[1];
    actors[1].prev=actors[1].next=NULL;actors[1].sprite=actors[2].sprite;actors[1].base_tile=32;
    td_aircraft_render_bind();require(!td_aircraft_cache_ready,"New district/base-tile bind failed to invalidate cached masks");
    selected_frame=0;allocated_hardware_sprites=0;td_aircraft_render();
    require(rom_tile_bytes-bytes==128&&shadow_OAM[0].tile==32+poses[0][0].dtile,"High Park did not reload its cache or relocate native OBJ base");
    td_aircraft_render_restore();reset(0);td_aircraft.u=td_aircraft.v=264*16;
    rom_tiles[0].n_tiles=rom_tiles[1].n_tiles=0;td_aircraft_render();
    require(!allocated_hardware_sprites&&!td_aircraft_cache_ready&&!td_aircraft_patch_count,"Truncated compiled tileset must reject flight without modifying graphics");
}

static UBYTE brute_capacity(const OAM_item_t *objects,UBYTE count,
                            volatile OAM_item_t *oam,UBYTE ground){
    int row,i,min=144,max=-1,total;
    if((int)ground+count>40)return FALSE;
    /* Independent per-screen-row enumeration, deliberately unlike events. */
    for(row=0;row<144;row++)for(i=0;i<count;i++)if(row+16>=objects[i].y&&row<objects[i].y){
        if(row<min)min=row;
        if(row>max)max=row;
    }
    if(max<min||max-min+1>64)return FALSE;
    for(row=min;row<=max;row++){
        total=0;
        for(i=0;i<ground;i++)if(row+16>=oam[i].y&&row<oam[i].y)total++;
        for(i=0;i<count;i++)if(row+16>=objects[i].y&&row<objects[i].y)total++;
        if(total>10)return FALSE;
    }
    return TRUE;
}
static uint32_t random_state=0x1937a52u;
static uint32_t random_value(void){
    random_state=random_state*1664525u+1013904223u;return random_state;
}
static void capacity_differential_checks(void){
    OAM_item_t objects[6];unsigned attempt;UBYTE ground,count,i,base;
    for(attempt=0;attempt<60000;attempt++){
        ground=(random_value()>>8)%41;count=random_value()&1?4:6;base=random_value()>>16;
        for(i=0;i<ground;i++){
            shadow_OAM[i].y=random_value()>>24;shadow_OAM[i].x=random_value()>>24;
        }
        for(i=0;i<count;i++){
            objects[i].y=base+(i>=4?24:i>=2?16:0);objects[i].x=random_value()>>24;
            if(!(random_value()&7))objects[i].y=0;
        }
        if(td_aircraft_capacity(objects,count,shadow_OAM,ground)!=brute_capacity(objects,count,shadow_OAM,ground)){
            fprintf(stderr,"capacity attempt %u ground %u count %u object Y",attempt,ground,count);
            for(i=0;i<count;i++)fprintf(stderr," %u",objects[i].y);
            fprintf(stderr,"\n");
        }
        require(td_aircraft_capacity(objects,count,shadow_OAM,ground)==brute_capacity(objects,count,shadow_OAM,ground),
                "Event sweep changed exact hardware Y-only capacity decision");
    }
    memset(objects,0,sizeof(objects));objects[0].y=16;objects[1].y=64;
    require(td_aircraft_capacity(objects,4,shadow_OAM,0),"Exactly64 visible rows must fit the65-byte event buffer");
    objects[1].y=65;
    require(!td_aircraft_capacity(objects,4,shadow_OAM,0),"Malformed pose beyond64 visible rows must reject conservatively");
    for(i=0;i<4;i++)objects[i].y=80;
    for(i=0;i<7;i++)shadow_OAM[i].y=80;
    require(td_aircraft_capacity(objects,4,shadow_OAM,6),"Exact hardware10-object boundary must remain allowed");
    require(!td_aircraft_capacity(objects,4,shadow_OAM,7),"Eleventh scanline object must suppress cosmetic flight");
}

/* Final-output oracle uses known original source-cell geometry and the old
 * independent per-row capacity rule: aircraft first, then optional shadow.
 * It does not call cached placement or the renderer's event implementation. */
static void oracle_objects(UBYTE frame,WORD x,WORD y,OAM_item_t *objects){
    UBYTE i,cx,cy;WORD ox,oy;
    for(i=0;i<6;i++){
        if(i>=4){cx=8+(i-4)*8;cy=8;ox=x+cx;oy=y+cy+24;}
        else if(frame%4==0||frame%4==2){cx=i*8;cy=8;ox=x+cx-8;oy=y+cy;}
        else{cx=8+(i&1)*8;cy=(i>>1)*16;ox=x+cx-8;oy=y+cy;}
        if(ox<=0||ox>=168||oy<=0||oy>=160)objects[i].x=objects[i].y=0;
        else{objects[i].x=ox;objects[i].y=oy;}
    }
}
static UBYTE oracle_window(const OAM_item_t *objects,UBYTE first,UBYTE count,UBYTE window_y){
    UBYTE i;
    if(window_y>=144)return TRUE;
    for(i=first;i<first+count;i++)if(objects[i].y&&objects[i].y-1>=window_y)return FALSE;
    return TRUE;
}
static void final_outcome_checks(void){
    OAM_item_t expected_objects[6],ground_objects[40];
    unsigned attempt,seen[3]={0,0,0};UBYTE ground,frame,i,expected,window_y;
    WORD x,y;
    for(attempt=0;attempt<5000;attempt++){
        frame=(random_value()>>16)%12;reset((random_value()>>16)&3);selected_frame=frame;
        x=(WORD)((random_value()>>16)%209)-24;y=(WORD)((random_value()>>16)%193)-24;
        ground=(random_value()>>16)%41;window_y=(random_value()>>16)&1?120:144;
        win_pos_y=win_dest_pos_y=window_y;WY_REG=window_y;WX_REG=window_y<144?7:0;
        td_aircraft.u=(draw_scroll_x+x)*16;td_aircraft.v=(draw_scroll_y+y)*16;
        for(i=0;i<ground;i++){
            ground_objects[i].y=random_value()>>24;ground_objects[i].x=random_value()>>24;
            ground_objects[i].tile=i*2;ground_objects[i].prop=i&7;shadow_OAM[i]=ground_objects[i];
        }
        allocated_hardware_sprites=ground;oracle_objects(frame,x,y,expected_objects);
        expected=0;
        if(oracle_window(expected_objects,0,4,window_y)&&brute_capacity(expected_objects,4,shadow_OAM,ground)){
            expected=4;
            if(oracle_window(expected_objects,4,2,window_y)&&brute_capacity(expected_objects,6,shadow_OAM,ground))expected=6;
        }
        memcpy(base_maps,maps,sizeof(maps));td_aircraft_render();
        require(allocated_hardware_sprites==ground+expected,"Six-first common path changed final0/4/6-object outcome");
        seen[expected==0?0:expected==4?1:2]++;
        for(i=0;i<ground;i++)require(!memcmp((void*)&shadow_OAM[i+(expected?4:0)],&ground_objects[i],4),
                                    "Final capacity decision modified/reordered an existing ground object");
        if(expected){
            for(i=0;i<4;i++)require(shadow_OAM[i].x==expected_objects[i].x&&shadow_OAM[i].y==expected_objects[i].y,
                                   "Capacity optimization changed clipped aircraft placement");
            if(expected==6)for(i=4;i<6;i++)require(shadow_OAM[ground+i].x==expected_objects[i].x&&shadow_OAM[ground+i].y==expected_objects[i].y,
                                                 "Capacity optimization changed optional shadow placement");
        }else require(!memcmp(base_maps,maps,sizeof(maps)),"Suppressed cosmetic output modified roof references");
        td_aircraft_render_restore();require(!memcmp(base_maps,maps,sizeof(maps)),"Outcome check failed exact final roof restoration");
    }
    require(seen[0]&&seen[1]&&seen[2],"Randomized final-outcome oracle must exercise all0/4/6 branches");
}

int main(void){
    UBYTE x,y,frame,flip,obj_flip;
    for(frame=0;frame<12;frame++)for(flip=0;flip<4;flip++)for(obj_flip=0;obj_flip<4;obj_flip++)
        for(y=0;y<8;y++)for(x=0;x<8;x++)pixel_checks(260+x,260+y,frame,flip,obj_flip);
    restoration_checks();binding_checks();capacity_checks();edge_checks();window_checks();untouched_ground_checks();
    cache_checks();capacity_differential_checks();final_outcome_checks();
    printf("Aircraft renderer: %lu checks, 0 failures (host VRAM/OAM adapters, no timing claim)\n",checks);
    return 0;
}
