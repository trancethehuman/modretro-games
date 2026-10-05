/* Actual native boat logic with independent PNG/full-hull/water/OAM oracles. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "host_boats.h"
#include "td_game.h"
#include "td_district.h"
#include "td_boats.h"
#include "boat_art_oracle.h"

actor_t actors[22],*actors_inactive_head;
UBYTE actors_len,allocated_hardware_sprites,VBK_REG,__render_shadow_OAM;
UBYTE win_pos_x,win_pos_y,win_dest_pos_x,win_dest_pos_y,WX_REG,WY_REG;
_Alignas(256) volatile OAM_item_t shadow_OAM[40],shadow_OAM2[40];
WORD draw_scroll_x,draw_scroll_y;
far_ptr_t current_scene;
td_state_t td;
static UBYTE district,scene_ids[8],uploads,deactivations,grid[15616];
static tileset_t rom_tiles[2];
static metasprite_t poses[4][5];
static const metasprite_t *frames[4];
static spritesheet_t sheet={4,frames,{8,&rom_tiles[0]},{9,&rom_tiles[1]}};
static UBYTE obj_data[2][128][16];
static unsigned long checks,bank_reads,pixel_compositions;
static void require(int condition,const char *message){
    checks++;if(!condition){fprintf(stderr,"FAIL after%lu checks: %s\n",checks,message);exit(1);}
}
UBYTE td_district_current(void){return district;}
UBYTE tile_at(UBYTE x,UBYTE y){return x<128&&y<122?grid[y*128+x]:15;}
void deactivate_actor(actor_t *actor){
    deactivations++;actor->flags&=~ACTOR_FLAG_ACTIVE;
    actor->next=actors_inactive_head;actor->prev=NULL;
    if(actors_inactive_head)actors_inactive_head->prev=actor;actors_inactive_head=actor;
}
void MemcpyBanked(void *dest,const void *src,size_t length,UBYTE bank){
    require(bank>=7&&bank<=9,"Unexpected ROM bank");bank_reads++;if(length==32)pixel_compositions++;memcpy(dest,src,length);
}
UBYTE ReadBankedUBYTE(const UBYTE *src,UBYTE bank){
    require(bank==7,"Descriptor read must use captured metadata bank");bank_reads++;return *src;
}
void set_sprite_data(UBYTE first,UBYTE count,const UBYTE *pixels){
    require(first>=32&&first<=38&&!(first&1)&&count==2&&VBK_REG<2,
            "Upload escaped the four independent scratch pairs");
    memcpy(obj_data[VBK_REG][first],pixels,32);uploads++;
}
#include "boats_under_test.c"

static UBYTE loader_index(UBYTE area){return area==0||area==1||area==3?5:4;}
static int deck(unsigned x,unsigned y){
    return (x>=416&&x<512&&((y>=96&&y<160)||(y>=256&&y<320)))||
        (x>=160&&x<224&&((y>=144&&y<240)||(y>=448&&y<552)||(y>=624&&y<720)))||
        (x>=776&&x<840&&y>=792&&y<856);
}
static int water(unsigned area,unsigned x,unsigned y){
    if(x>=1024||y>=976)return 0;
    if(area==0)return y>=816;
    if(area!=4)return 0;
    return (x<96&&y>=168&&y<928)||(x>=96&&x<488&&y>=168&&y<216)||
        (x>=432&&x<496&&y<528)||(x>=96&&x<496&&y>=464&&y<536)||
        (x>=96&&x<800&&y>=640&&y<704)||(x>=752&&x<864&&y>=608&&y<728)||
        (x>=792&&x<824&&y>=704&&y<928)||y>=936;
}
static void encode_pair(UBYTE bank,UBYTE pair,const UBYTE *pixels,unsigned stride,unsigned ox,unsigned oy,UBYTE flip){
    UBYTE x,y,colour;
    for(y=0;y<16;y++)for(x=0;x<8;x++){
        colour=pixels[(oy+(flip&2?15-y:y))*stride+ox+(flip&1?7-x:x)];
        if(colour&1)rom_tiles[bank].tiles[pair*16+y*2]|=128>>x;
        if(colour&2)rom_tiles[bank].tiles[pair*16+y*2+1]|=128>>x;
    }
}
static void fixture(UBYTE area,UBYTE flip,UBYTE source_bank){
    const UBYTE coords[3][4][2]={{{8,0},{0,0},{8,16},{0,16}},
                               {{16,0},{8,0},{0,0},{0,0}},
                               {{8,0},{0,0},{8,16},{0,16}}};
    UBYTE f,i,previous_x,previous_y,x,y,slot,index=loader_index(area),pair;
    memset(&td,0,sizeof(td));memset(actors,0,sizeof(actors));memset(poses,0,sizeof(poses));
    memset(rom_tiles,0,sizeof(rom_tiles));memset(obj_data,0x5a,sizeof(obj_data));
    memset((void*)shadow_OAM,0,sizeof(shadow_OAM));memset((void*)shadow_OAM2,0,sizeof(shadow_OAM2));
    memcpy(grid,area==4?original_grid_4:original_grid_0,sizeof(grid));
    sheet.n_metasprites=4;sheet.tileset.bank=8;sheet.cgb_tileset.bank=9;
    rom_tiles[source_bank].n_tiles=14;rom_tiles[!source_bank].n_tiles=8;
    for(f=0;f<4;f++){
        frames[f]=poses[f];previous_x=previous_y=0;
        for(i=0;f<3&&i<(f==1?3:4);i++){
            x=coords[f][i][0];y=coords[f][i][1];slot=f==1?x/8:(y/16)*2+x/8;
            pair=f==0?slot*2:f==1?8+slot*2:slot*2;
            poses[f][i]=(metasprite_t){(BYTE)(y-previous_y-(i?0:f==1?8:24)),(BYTE)(x-previous_x-(i?0:8)),pair,
                (UBYTE)((f==2?(!source_bank)<<3:(source_bank<<3)|(flip<<5)))};
            previous_x=x;previous_y=y;
            if(f<2)encode_pair(source_bank,pair,f==0?&original_north[0][0]:&original_east[0][0],
                              f==0?16:24,x,y,flip);
        }
        poses[f][f==0||f==2?4:f==1?3:0].dy=metasprite_end;
    }
    district=area;actors_len=16;current_scene=(far_ptr_t){10,&scene_ids[area]};
    actors[index].sprite=(far_ptr_t){7,&sheet};actors[index].base_tile=32;
    actors_inactive_head=&actors[1];actors[1].next=&actors[index];actors[index].prev=&actors[1];
    actors[index].next=&actors[index+1];actors[index+1].prev=&actors[index];
    td.mode=TD_ROAM;td.cash=137;td.job=4;td.stage=1;td.health=83;
    td.u=560*16;td.v=720*16;td.park_u=560*16;td.park_v=720*16;td.park_district=0;
    td_boats_reset();allocated_hardware_sprites=uploads=0;VBK_REG=1;
    __render_shadow_OAM=(UBYTE)((UWORD)(uintptr_t)shadow_OAM>>8);
    win_pos_x=win_dest_pos_x=0;win_pos_y=win_dest_pos_y=144;WX_REG=0;WY_REG=144;
    draw_scroll_x=480;draw_scroll_y=768;
}
static void bind_ok(void){
    UBYTE i=loader_index(district);td_boats_bind();
    require(td_boat.bound&&td_boat.base==32,"Valid launch/scratch metadata was not bound");
    require(actors[1].next==&actors[i+1]&&actors[i+1].prev==&actors[1]&&
            !actors[i].next&&!actors[i].prev&&actors[i].flags==ACTOR_FLAG_HIDDEN,
            "Scriptless boat loader must detach before actor cloning");
}
static UBYTE object_pixel(OAM_item_t o,UBYTE x,UBYTE y){
    const UBYTE *row=obj_data[(o.prop>>3)&1][(o.tile&0xfe)+(y>>3)]+(y&7)*2;
    return ((row[0]>>(7-x))&1)|(((row[1]>>(7-x))&1)<<1);
}
static UBYTE screen_pixel(WORD x,WORD y){
    UBYTE i,colour=0,next;WORD left,top;
    for(i=0;i<allocated_hardware_sprites;i++){
        left=(WORD)shadow_OAM[i].x-8;top=(WORD)shadow_OAM[i].y-16;
        if(x>=left&&x<left+8&&y>=top&&y<top+16){
            next=object_pixel(shadow_OAM[i],x-left,y-top);
            if(next){require(!colour,"Boat pieces overlapped opaque pixels");colour=next;}
        }
    }
    return colour;
}
static void pixel_checks(void){
    const UWORD port_v[]={64,90,96,110,127,143,159,160,177,239,256,287,319,320,352,464,500,652,704,824};
    UBYTE area,flip,bank,orientation,x,y,expected,wake,source_x,source_y;UWORD centre_x,centre_y;
    unsigned sample;WORD left,top;td_state_t original;
    for(area=0;area<=4;area+=4)for(flip=0;flip<4;flip++)for(bank=0;bank<2;bank++){
        fixture(area,flip,bank);bind_ok();original=td;td_boat.moored=1;td_boat.speed=0;
        for(sample=0;sample<(area==4?sizeof(port_v)/sizeof(port_v[0]):3);sample++)for(orientation=0;orientation<4;orientation++)for(wake=0;wake<3;wake++){
            centre_x=area==4?(sample>=17?808:464):560;
            centre_y=area==4?port_v[sample]:(UWORD)(840+sample*32);
            td_boat.u=centre_x*16;td_boat.v=centre_y*16;td_boat.heading=orientation*4;
            td_boat.moored=!wake;td_boat.tick=wake==2?8:0;
            draw_scroll_x=centre_x-80;draw_scroll_y=centre_y-72;
            allocated_hardware_sprites=uploads=0;VBK_REG=1;td_boats_render();
            require(!memcmp(&td,&original,sizeof(td)),"Renderer changed courier/car/job/money/save state");
            require(VBK_REG==1&&uploads==allocated_hardware_sprites,"Wrong scratch count or incoming VRAM bank lost");
            for(y=0;y<(orientation&1?32:16);y++)for(x=0;x<(orientation&1?16:24);x++){
                left=centre_x-(orientation&1?8:12);
                top=centre_y-(orientation==1?20:orientation==3?12:8);
                source_x=orientation&1?x:orientation==0?23-x:x;
                source_y=orientation==1?31-y:y;
                expected=orientation&1?original_north[source_y][source_x]:original_east[source_y][source_x];
                if(wake&&((orientation&1&&source_y>=24&&source_y<=29&&
                    ((source_y+(wake==2))&1)==0&&source_x>=3&&source_x<=12&&
                    (source_x==3+(source_y-24)/2||source_x==12-(source_y-24)/2))||
                    (!(orientation&1)&&source_x>=18&&source_x<=22&&(source_y==1||source_y==14)&&
                    ((source_x+(wake==2))&1)==0)))expected=1;
                if(!water(area,left+x,top+y)||(area==4&&deck(left+x,top+y)))expected=0;
                require(screen_pixel(left+x-draw_scroll_x,top+y-draw_scroll_y)==expected,
                        "Native normalized/rotated/deck-clipped pixels differ from original PNG");
            }
        }
    }
}
static void mask_and_cull_cost_checks(void){
    for(unsigned area=0;area<=4;area+=4){
        fixture(area,0,0);bind_ok();
        for(int y=-9;y<985;y++)for(int left=-9;left<1033;left++){
            unsigned expected=0;
            for(unsigned bit=0;bit<8;bit++)if(water(area,left+bit,y)&&!(area==4&&deck(left+bit,y)))expected|=128>>bit;
            require(td_boat_row_mask(left,y)==expected,"Tile-interval row mask differs from independent per-pixel water/deck/world oracle");
        }
    }
    for(unsigned orientation=0;orientation<4;orientation++)for(unsigned side=0;side<4;side++){
        fixture(0,0,0);bind_ok();td_boat.heading=orientation*4;td_boat.moored=1;td_boat.speed=0;
        if(side==0)draw_scroll_x=600;
        if(side==1)draw_scroll_x=360;
        if(side==2)draw_scroll_y=880;
        if(side==3)draw_scroll_y=640;
        unsigned long reads=bank_reads;pixel_compositions=0;td_boats_render();
        require(!pixel_compositions&&reads==bank_reads&&!uploads&&!allocated_hardware_sprites,
            "Wholly offscreen hull/wake must read no pose or pixels and compose zero cells");
    }
    fixture(0,0,0);bind_ok();td_boat.heading=12;td_boat.moored=1;td_boat.speed=0;draw_scroll_x=564;
    pixel_compositions=0;td_boats_render();
    require(allocated_hardware_sprites==2&&uploads==2&&pixel_compositions==4,
        "Per-cell viewport culling composes only the two admitted visible hull cells, once before and once after capacity");
    fixture(0,0,0);bind_ok();win_dest_pos_y=32;
    unsigned long reads=bank_reads;pixel_compositions=0;td_boats_render();
    require(!pixel_compositions&&reads==bank_reads&&!uploads&&!allocated_hardware_sprites,
        "Wholly UI-covered hull/wake must read no metadata or compose pixels");
}
static int body_oracle(UWORD q_u,UWORD q_v,UBYTE heading){
    int x,y,u=q_u/16,v=q_v/16,vertical=((heading+2)%16)/4%2;
    int hx=vertical?8:12,hy=vertical?12:8;
    if(u<hx||v<hy||u>1024-hx||v>976-hy)return 0;
    for(y=v-hy;y<v+hy;y++)for(x=u-hx;x<u+hx;x++)
        if(!water(district,x,y)||(tile_at(x/8,y/8)!=15&&!(district==4&&deck(x,y))))return 0;
    return 1;
}
static void water_and_motion_checks(void){
    UWORD u,v,old_u,old_v,safe_u,safe_v;UBYTE h,i;td_state_t original;td_boat_state_t held;
    td_boat_terrain_t cache={0,0,0,0,0,0};
    fixture(4,0,0);bind_ok();
    for(v=1;v<976;v+=17)for(u=1;u<1024;u+=19)for(h=0;h<16;h+=3){
        require(td_boat_body(u*16,v*16,h)==body_oracle(u*16,v*16,h),
                "Hull corner-only test leaked into land or missed interior terrain");
        require(td_boat_cached_body(u*16,v*16,h,&cache)==body_oracle(u*16,v*16,h),
                "Update-local boat terrain cache changed collision at a new body boundary");
    }
    for(v=176*16;v<180*16;v++)for(u=460*16;u<464*16;u++)for(h=0;h<16;h+=3)
        require(td_boat_cached_body(u,v,h,&cache)==body_oracle(u,v,h),
                "Subpixel cache reuse changed exact continuous water body clearance");
    /* Non-water blocked buildings are not navigable, even with collision15. */
    require(!td_boat_body(544*16,384*16,12),"Boat drove into a solid industrial building");
    for(i=0;i<6;i++){
        UWORD bx=i==0||i==1?464:i==5?808:192;
        UWORD by=i==0?128:i==1?288:i==2?192:i==3?496:i==4?672:824;
        require(td_boat_body(bx*16,by*16,12),"Existing approved bridge did not admit an under-deck hull");
    }
    fixture(0,0,0);bind_ok();td.onfoot=1;u=560*16;v=800*16;original=td;
    require(td_boats_interact(&u,&v)==1&&td_boats_controlled(),"Reachable harbour dock did not board");
    require(u==560*16&&v==840*16&&!memcmp(&td,&original,sizeof(td)),"Board changed unrelated persistent game state");
    require(td_boats_restore_shore(&safe_u,&safe_v)&&safe_u==560*16&&safe_v==800*16,
            "Save/reset projection did not preserve the actual boarding shore");
    held=td_boat;td_boats_drive(J_A,8,&u,&v);
    require(td_boats_controlled()&&td_boat.speed==1&&u==560*16&&v>840*16,
            "Plain gas immediately after actual boarding must accelerate the occupied launch away from its safe dock");
    td_boat=held;u=td_boat.u;v=td_boat.v;
    td_boat.heading=0;
    for(i=0;i<20;i++)td_boats_drive(J_A,16,&u,&v);
    require(td_boat.speed==8&&u>560*16&&v==840*16,"Boat acceleration/canonical motion is too fast or drifts vertically");
    old_u=u;old_v=v;td_boats_drive(J_A,65535,&u,&v);
    require(u-old_u<=128&&v==old_v,"Delayed update tunneled beyond the eight-pixel sweep cap");
    require(!memcmp(&td,&original,sizeof(td)),"Boat drive changed parked road car, active cargo, cash or save state");
    td_boat.u=560*16;td_boat.v=828*16;td_boat.heading=12;td_boat.vx=0;td_boat.vy=-8;td_boat.speed=8;
    td_boats_drive(J_A,16,&u,&v);
    require(v>=828*16&&td_boat_body(u,v,td_boat.heading),"North harbour bank let the launch drive on land");
    for(i=0;i<12;i++)td_boats_drive(J_B,16,&u,&v);
    require(v>828*16&&td_boat.speed==-4,"Braking/reverse failed to recover from shoreline contact");
    td_boat.v=900*16;u=td_boat.u;v=td_boat.v;held=td_boat;
    require(td_boats_interact(&u,&v)==0&&!memcmp(&held,&td_boat,sizeof(held)),
            "Boat disembarked into offshore water");
    td_boat.u=560*16;td_boat.v=840*16;td_boat.speed=0;u=td_boat.u;v=td_boat.v;
    require(td_boats_interact(&u,&v)==2&&!td_boats_controlled()&&u==560*16&&v==800*16&&td_boat.moored,
            "Safe dock landing stranded courier or moved original road car");
    require(!td_boats_restore_shore(&safe_u,&safe_v),"Shore projection persisted after disembark");
    td_boat.heading=12;td_boat.u=400*16;td_boat.v=840*16;td.u=400*16;td.v=800*16;
    u=td.u;v=td.v;require(td_boats_interact(&u,&v)==1,"Moored launch could not be boarded again");
    td.mode=TD_PAUSE;held=td_boat;old_u=u;old_v=v;
    td_boats_drive(J_A|J_RIGHT,65535,&u,&v);td_boats_update(65535);td_boats_render();
    require(!memcmp(&held,&td_boat,sizeof(held))&&u==old_u&&v==old_v,"Pause advanced controlled boat or wake");
    require(td_boats_restore_shore(&safe_u,&safe_v)&&safe_u==400*16&&safe_v==800*16,
            "Pause-save projection incorrectly requires roaming");
}
static void capacity_checks(void){
    OAM_item_t candidate[4];UBYTE count,overlap,i,n,expected,orientation;unsigned y,peak,used;
    for(orientation=0;orientation<4;orientation++){
        fixture(0,0,0);bind_ok();td_boat.heading=orientation*4;td_boat.moored=1;td_boat.speed=0;
        td_boats_render();n=allocated_hardware_sprites;
        require(n==(orientation&1?4:3),"Full visible hull did not use its expected bounded OBJ count");
        for(i=0;i<n;i++)candidate[i]=shadow_OAM[i];
        for(count=0;count<=40;count++)for(overlap=0;overlap<=11;overlap++){
            memset((void*)shadow_OAM,0,sizeof(shadow_OAM));
            for(i=0;i<count;i++){shadow_OAM[i].y=i<overlap?candidate[0].y:0;shadow_OAM[i].x=0;}
            peak=0;
            for(y=0;y<144;y++){
                used=0;
                for(i=0;i<count;i++)used+=y>=(unsigned)(shadow_OAM[i].y>=16?shadow_OAM[i].y-16:0)&&y<shadow_OAM[i].y;
                for(i=0;i<n;i++)used+=y>=(unsigned)(candidate[i].y>=16?candidate[i].y-16:0)&&y<candidate[i].y;
                if(used>peak)peak=used;
            }
            expected=count+n<=40&&peak<=10;allocated_hardware_sprites=count;uploads=0;
            td_boats_render();
            require(allocated_hardware_sprites==count+(expected?n:0),
                    "Boat partly emitted a hull or exceeded40OBJ/10perscanline");
            require(uploads==(expected?n:0),"Rejected boat polluted scratch VRAM");
        }
    }
}
static void lifecycle_checks(void){
    UBYTE area,i;td_state_t original;td_boat_state_t held;UWORD phase,period,travel;
    for(area=0;area<6;area++){
        fixture(area,0,0);bind_ok();original=td;
        if(area==0||area==4){
            period=(area==4?432-64:896-840)*32;phase=(65535%(period/4))*4;
            td_boats_update(65535);travel=phase<period/2?phase:period-phase;
            require(td_boat.phase==phase&&td_boat.v==(area==4?64:840)*16+travel,
                    "Autonomous launch lost bounded actual-VBlank cadence");
            td.onfoot=1;td.u=td_boat.u;td.v=td_boat.v;held=td_boat;
            td_boats_update(120);require(!memcmp(&held,&td_boat,sizeof(held)),"Nearby boarder did not get stationary launch");
            td.onfoot=0;original=td;
        }else{
            held=td_boat;td_boats_update(65535);td_boats_render();
            require(!memcmp(&held,&td_boat,sizeof(held))&&!allocated_hardware_sprites,"Dry district invented a water route");
        }
        require(!memcmp(&td,&original,sizeof(td)),"Ambient launch changed saved courier state");
        current_scene.ptr=&scene_ids[7];held=td_boat;td_boats_update(65535);td_boats_render();
        require(!memcmp(&held,&td_boat,sizeof(held))&&!td_boats_controlled(),"Stale scene binding operated on another district");
    }
    fixture(4,0,0);bind_ok();district=6;current_scene.ptr=&scene_ids[6];actors_len=4;
    original=td;td_boats_bind();
    require(!td_boat.bound&&!memcmp(&td,&original,sizeof(td)),"North retained old boat or touched save state");
    for(i=0;i<15;i++){
        fixture(0,0,0);
        switch(i){
        case 0:actors_len=5;break;
        case 1:actors[5].sprite.bank=0;break;
        case 2:actors[5].sprite.ptr=NULL;break;
        case 3:sheet.n_metasprites=3;break;
        case 4:poses[0][0].dy=metasprite_end;break;
        case 5:poses[2][4].dy=0;break;
        case 6:poses[0][0].dtile=1;break;
        case 7:poses[0][0].props=0x80;break;
        case 8:poses[2][0].dtile=poses[2][1].dtile;break;
        case 9:sheet.tileset.bank=0;break;
        case 10:rom_tiles[0].n_tiles=129;break;
        case 11:rom_tiles[0].n_tiles=3;break;
        case 12:actors[5].base_tile=125;break;
        case 13:district=7;break;
        case 14:poses[1][0].dx=15;break;
        }
        td_boats_bind();td_boats_update(60);td_boats_render();
        require(!td_boat.bound&&!allocated_hardware_sprites&&!uploads,"Malformed loader failed open");
    }
    fixture(0,0,0);bind_ok();win_dest_pos_y=64;td_boats_render();
    require(!allocated_hardware_sprites&&!uploads,"Launch crossed approaching UI window");
    win_dest_pos_y=144;WX_REG=7;WY_REG=64;td_boats_render();
    require(!allocated_hardware_sprites&&!uploads,"Launch crossed actual hardware UI window");
}
static void wake_checks(void){
    UBYTE normal[2][128][16];unsigned differences=0;UBYTE i,y,x,colour;WORD px,py;
    fixture(4,0,0);bind_ok();td_boat.heading=12;td_boat.u=464*16;td_boat.v=176*16;
    draw_scroll_x=384;draw_scroll_y=104;td_boat.moored=1;td_boat.speed=0;td_boats_render();
    memcpy(normal,obj_data,sizeof(normal));allocated_hardware_sprites=uploads=0;
    td_boat.moored=0;td_boat.tick=0;td_boats_render();
    for(i=0;i<allocated_hardware_sprites;i++)for(y=0;y<16;y++)for(x=0;x<8;x++){
        colour=object_pixel(shadow_OAM[i],x,y);px=(WORD)shadow_OAM[i].x-8+x+draw_scroll_x;
        py=(WORD)shadow_OAM[i].y-16+y+draw_scroll_y;
        if(!water(4,px,py)||deck(px,py))require(!colour,"Moving wake painted on deck or land");
    }
    for(i=0;i<128;i++)for(y=0;y<16;y++)differences+=normal[1][i][y]!=obj_data[1][i][y];
    require(differences,"Moving launch had no original ripple animation");
    memcpy(normal,obj_data,sizeof(normal));allocated_hardware_sprites=uploads=0;td_boat.tick=8;td_boats_render();
    require(memcmp(normal,obj_data,sizeof(normal)),"Wake phase did not animate");
    td_boat.v=88*16;draw_scroll_y=16;allocated_hardware_sprites=uploads=0;td_boats_render();
    for(i=0;i<allocated_hardware_sprites;i++)for(y=0;y<16;y++)for(x=0;x<8;x++){
        px=(WORD)shadow_OAM[i].x-8+x+draw_scroll_x;py=(WORD)shadow_OAM[i].y-16+y+draw_scroll_y;
        if(deck(px,py))require(!object_pixel(shadow_OAM[i],x,y),"Bridge covered hull but exposed its animated wake");
    }
}
static void observer_cover_checks(void){
    fixture(4,0,0);td_boats_bind();td_boat.occupied=1;
    for(unsigned y=0;y<976;y+=4)for(unsigned x=0;x<1024;x+=4){
        td_boat.u=x*16;td_boat.v=y*16;
        require(td_boats_under_cover()==deck(x,y),"Boat aerial concealment differs from exact authored Don bridge/deck geometry");
    }
    td_boat.u=464*16;td_boat.v=136*16;td_boat.occupied=0;
    require(!td_boats_under_cover(),"Unoccupied cosmetic boat cannot conceal an unrelated courier");
    td_boat.occupied=1;current_scene.ptr=&actors[0];
    require(!td_boats_under_cover(),"Stale boat scene cannot supply bridge cover in another district");
    fixture(0,0,0);td_boats_bind();td_boat.occupied=1;td_boat.u=560*16;td_boat.v=840*16;
    require(!td_boats_under_cover(),"Open harbour launch has no invented bridge concealment");
}
static void boarding_wait_boundary_checks(void){
    const UWORD distances[]={55,56,57,63,64};
    for(unsigned shore=800;shore<=804;shore+=4)for(unsigned axis=0;axis<2;axis++)for(unsigned i=0;i<sizeof(distances)/sizeof(distances[0]);i++){
        fixture(0,0,0);bind_ok();td.onfoot=1;td.u=(560+(axis?distances[i]:0))*16;td.v=shore*16;
        td_boat.u=560*16;td_boat.v=(axis?shore+56:shore+distances[i])*16;
        td_boat.phase=td_boat.v-840*16;td_boat.heading=4;
        td_boat_state_t before=td_boat;td_boats_update(1);
        if(distances[i]<=56)require(!memcmp(&before,&td_boat,sizeof(before)),"Boat waits at the same inclusive56px per-axis boarding boundary");
        else require(td_boat.u==before.u&&td_boat.v==before.v+4&&td_boat.phase==before.phase+4,
            "Boat beyond boarding range keeps its exact clock/path instead of freezing or teleporting");
    }
    for(unsigned shore=800;shore<=804;shore+=4){
        fixture(0,0,0);bind_ok();td.onfoot=1;td.u=560*16;td.v=shore*16;
        require(td_boat_shore(td.u,td.v),"Boundary test uses actual reachable full shore footprint");
        td_boat.v=(shore+57)*16;td_boat.phase=td_boat.v-840*16;td_boat.heading=4;
        unsigned waited=0;
        for(unsigned tick=0;tick<449;tick++){
            td_boat_state_t before=td_boat;td_boats_update(1);
            require(td_boat.u==before.u&&td_boat_distance(td_boat.v,before.v)<=4,"Waiting launch keeps continuous native quarter-pixel path with fixed harbour axis");
            if(td_boat.phase==before.phase){
                require(td_boat_distance(td.v,td_boat.v)<=56*16,"Moving launch can first freeze only inside boarding range");
                UWORD u=td.u,v=td.v;require(td_boats_interact(&u,&v)==1&&td_boats_controlled(),
                    "At either800/804 shore the waiting launch can be boarded without a dead zone");
                waited=1;break;
            }
        }
        require(waited,"Whole-route motion returns an out-of-range launch to a reachable boarding window");
    }
    for(unsigned mode=TD_WAIT;mode<=TD_RIDE;mode++){
        fixture(0,0,0);bind_ok();td.onfoot=1;td.u=560*16;td.v=800*16;td.mode=mode;
        UWORD before=td_boat.phase;td_boats_update(1);
        require(td_boat.phase==before+4,"Wait/ride modes do not freeze the autonomous launch near an on-foot courier");
    }
}
int main(void){
    require((UBYTE)((UWORD)(uintptr_t)shadow_OAM>>8)!=(UBYTE)((UWORD)(uintptr_t)shadow_OAM2>>8),"Distinct OAM buffers required");
    pixel_checks();mask_and_cull_cost_checks();water_and_motion_checks();capacity_checks();lifecycle_checks();wake_checks();observer_cover_checks();boarding_wait_boundary_checks();
    printf("Controllable boat production C: %lu checks passed; native budget/pacing/hardware remain separate.\n",checks);return 0;
}
