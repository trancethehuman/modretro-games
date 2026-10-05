#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "host_actor_render.h"
#include "td_actor_render.h"
UBYTE allocated_hardware_sprites,__render_shadow_OAM,LCDC_REG,CURRENT_BANK;
actor_t actors[22];
UBYTE actors_len,screen_x,screen_y,WX_REG,WY_REG;
WORD draw_scroll_x,draw_scroll_y;
_Alignas(256) volatile OAM_item_t shadow_OAM[40],shadow_OAM2[40];
static unsigned long checks,bank_reads;
static unsigned long bank_switches;
static UBYTE mapped_active;
static spritesheet_t mapped_window,mapped_sheets[2];
void host_actor_switch(UBYTE bank){
    CURRENT_BANK=bank;bank_switches++;
    if(mapped_active)mapped_window=bank==7?mapped_sheets[0]:bank==11?mapped_sheets[1]:(spritesheet_t){0,NULL};
}
static uint32_t seed=0x30201007;
static uint32_t random_word(void){seed=seed*1664525u+1013904223u;return seed;}
static void require(int result,const char *message){checks++;if(!result){fprintf(stderr,"FAIL %lu: %s\n",checks,message);exit(1);}}
void MemcpyBanked(void *dest,const void *source,size_t size,UBYTE bank){require(bank==7,"Read exact authored sprite bank");bank_reads++;memcpy(dest,source,size);}

UBYTE ReadBankedUBYTE(const UBYTE *source,UBYTE bank){require(bank==7,"Exact sprite count bank");bank_reads++;return *source;}

static unsigned oracle(const metasprite_t *pose,int x,int y,unsigned base,unsigned height,
                       const OAM_item_t *ground,unsigned used,OAM_item_t *out){
    int counts[144]={0},low=144,high=-1;unsigned count=0;
    if(x<-2048||x>2048||y<-2048||y>2048||used>40)return 0;
    for(unsigned i=0;i<=8;i++){
        if(pose[i].dy==metasprite_end)break;
        if(i==8)return 0;
        x+=pose[i].dx;y+=pose[i].dy;
        if(x<=0||x>=168||y<=(int)(16-height)||y>=160)continue;
        out[count++]=(OAM_item_t){y,x,base+pose[i].dtile,pose[i].props};
        int a=y-16,b=a+(int)height-1;
        if(a<low)low=a;
        if(b>high)high=b;
    }
    if(!count||count+used>40||high-low+1>64)return 0;
    if(low<0)low=0;
    if(high>143)high=143;
    for(unsigned i=0;i<used+count;i++){
        unsigned oy=i<used?ground[i].y:out[i-used].y;
        for(int line=0;line<144;line++)if(line>=(int)oy-16&&line<(int)oy-16+(int)height)counts[line]++;
    }
    for(int line=low;line<=high;line++)if(counts[line]>10)return 0;
    return count;
}

static void compare(const metasprite_t *pose,WORD x,WORD y,UBYTE base,UBYTE used,UBYTE buffer){
    OAM_item_t before[40],expected[8];unsigned count;
    volatile OAM_item_t *selected=buffer?shadow_OAM2:shadow_OAM;
    volatile OAM_item_t *other=buffer?shadow_OAM:shadow_OAM2;
    for(unsigned i=0;i<40;i++){before[i]=selected[i];other[i]=(OAM_item_t){77,66,55,44};}
    count=oracle(pose,x,y,base,LCDC_REG&LCDCF_OBJ16?16:8,before,used,expected);
    allocated_hardware_sprites=used;__render_shadow_OAM=(UBYTE)((UWORD)(uintptr_t)selected>>8);
    td_actor_render(pose,7,base,x,y);
    require(allocated_hardware_sprites==used+count,"Exact atomic oracle admission and OAM count");
    for(unsigned i=0;i<40;i++){
        OAM_item_t actual=selected[i],opposite=other[i];
        const OAM_item_t *want=i>=used&&i<used+count?&expected[i-used]:&before[i];
        require(!memcmp(&actual,want,sizeof(actual)),"Existing priorities, complete rejected poses and OAM tail preserved");
        require(opposite.y==77&&opposite.x==66&&opposite.tile==55&&opposite.prop==44,"Only current double-buffer page changes");
    }
}

void td_traffic_lights_render(void){}
void td_aircraft_render_restore(void){}
void td_scenery_restore(void){}
void td_scenery_render(void){}
UBYTE td_boats_controlled(void){return 0;}
void td_boats_render(void){}
void td_sandbox_render(void){}
void td_aircraft_render(void){}
static void actor_viewport_cull_checks(void){
    const metasprite_t edge[]={{0,-8,0,0},{16,24,2,8},{metasprite_end,0,0,0}};
    const metasprite_t *frames[]={edge};spritesheet_t sheet={1,frames};
    const WORD coordinates[][2]={{-17,60},{-16,60},{-15,60},{175,60},{176,60},{177,60},
        {80,-17},{80,-16},{80,-15},{80,159},{80,160},{80,161},{0,0},{160,144}};
    for(unsigned buffer=0;buffer<2;buffer++)for(unsigned mode=0;mode<2;mode++)for(unsigned n=0;n<sizeof(coordinates)/sizeof(coordinates[0]);n++){
        OAM_item_t want[8],empty[40]={{0}};WORD x=coordinates[n][0],y=coordinates[n][1];
        actor_t actor={.pos={300*32,300*32},.frame=0,.base_tile=4,.sprite={7,&sheet}};
        draw_scroll_x=300-x;draw_scroll_y=300-y;LCDC_REG=mode?LCDCF_OBJ16:0;
        memset((void*)shadow_OAM,0,sizeof(shadow_OAM));memset((void*)shadow_OAM2,0,sizeof(shadow_OAM2));
        volatile OAM_item_t *page=buffer?shadow_OAM2:shadow_OAM;
        __render_shadow_OAM=(UBYTE)((UWORD)(uintptr_t)page>>8);allocated_hardware_sprites=0;
        unsigned expected=oracle(edge,x,y,4,mode?16:8,empty,0,want);unsigned long before=bank_reads,switches=bank_switches;
        CURRENT_BANK=19;
        td_actor_render_actor(&actor);
        require(allocated_hardware_sprites==expected,"Early viewport cull preserves exact original signed edge admission in8/16OBJ modes");
        for(unsigned i=0;i<expected;i++){OAM_item_t actual=page[i];require(!memcmp(&actual,&want[i],sizeof(actual)),"Clipped camera edge retains complete original pose pixels/tiles/properties");}
        unsigned outside=x<=-16||x>=176||y<=-16||y>=160;
        require(outside?bank_reads==before:bank_reads>before,"Only provably offscreen actors skip all banked metadata reads");
        require(CURRENT_BANK==19&&bank_switches-switches==(outside?0u:2u),"A visible actor binds count/table/pose with exactly one source selection and one caller restoration");
    }
    for(unsigned buffer=0;buffer<2;buffer++){
        actor_t actor={.pos={80*32,80*32},.flags=ACTOR_FLAG_PINNED,.frame=0,.base_tile=4,.sprite={7,&sheet}};
        draw_scroll_x=draw_scroll_y=1000;LCDC_REG=LCDCF_OBJ16;allocated_hardware_sprites=0;
        __render_shadow_OAM=(UBYTE)((UWORD)(uintptr_t)(buffer?shadow_OAM2:shadow_OAM)>>8);
        unsigned long before=bank_reads;td_actor_render_actor(&actor);
        require(allocated_hardware_sprites==2&&bank_reads>before,"Pinned full pose ignores an offscreen world camera before the metadata cull");
        actor.pos.x=176*32;allocated_hardware_sprites=0;before=bank_reads;td_actor_render_actor(&actor);
        require(!allocated_hardware_sprites&&bank_reads==before,"Offscreen pinned pose skips metadata using its screen coordinates");
    }
    draw_scroll_x=draw_scroll_y=0;
}
static void metadata_bank_checks(void){
    const metasprite_t first[]={{8,0,0,0},{metasprite_end,0,0,0}};
    const metasprite_t second[]={{8,4,2,1},{metasprite_end,0,0,0}};
    const metasprite_t *bank7[]={first,second},*bank11[]={second,NULL,first};
    const metasprite_t *replacement[]={second};
    mapped_sheets[0]=(spritesheet_t){2,bank7};mapped_sheets[1]=(spritesheet_t){3,bank11};mapped_active=1;
    /* Two physical ROM banks expose different descriptors at one near
     * address. Every byte-valued caller bank and frame index is exercised. */
    for(unsigned caller=0;caller<256;caller++)for(unsigned resource=0;resource<2;resource++)for(unsigned frame=0;frame<256;frame++){
        UBYTE bank=resource?11:7;host_actor_switch(caller);
        UBYTE old_count=mapped_window.n_metasprites;
        const metasprite_t *const *old_table=mapped_window.metasprites;
        unsigned long before=bank_switches,reads=bank_reads;
        const metasprite_t *want=frame<mapped_sheets[resource].n_metasprites?mapped_sheets[resource].metasprites[frame]:NULL;
        const metasprite_t *actual=td_actor_render_pose(&mapped_window,bank,frame);
        require(actual==want,"Count, table and pose are read from the same selected physical ROM bank at the same near address");
        require(CURRENT_BANK==caller&&mapped_window.n_metasprites==old_count&&mapped_window.metasprites==old_table,"Every valid, invalid and empty frame restores the exact caller ROM mapping");
        require(bank_switches-before==2&&bank_reads==reads,"Descriptor binding uses exactly two selections and no three separate banked metadata copies");
    }
    for(unsigned caller=0;caller<256;caller++){
        host_actor_switch(caller);unsigned long before=bank_switches;
        require(!td_actor_render_pose(NULL,7,0)&&!td_actor_render_pose(&mapped_window,0,0),"Null resources and bank0 are rejected without a descriptor dereference");
        require(CURRENT_BANK==caller&&bank_switches==before,"Invalid resource rejection leaves the caller bank untouched");
    }
    host_actor_switch(19);
    mapped_sheets[0]=(spritesheet_t){0,bank7};
    require(!td_actor_render_pose(&mapped_window,7,0)&&CURRENT_BANK==19,"Fresh zero-count resources cannot reuse a stale first frame");
    mapped_sheets[0]=(spritesheet_t){2,NULL};
    require(!td_actor_render_pose(&mapped_window,7,0)&&CURRENT_BANK==19,"Missing frame tables are safe and restore the caller");
    mapped_sheets[0]=(spritesheet_t){1,replacement};
    require(td_actor_render_pose(&mapped_window,7,0)==second&&CURRENT_BANK==19,"An updated frame table is rebound instead of cached");
    replacement[0]=first;
    require(td_actor_render_pose(&mapped_window,7,0)==first&&CURRENT_BANK==19,"Mutable pose pointer entries are read freshly from their resource bank");
    replacement[0]=NULL;
    require(!td_actor_render_pose(&mapped_window,7,0)&&CURRENT_BANK==19,"Empty selected pose entries safely suppress rendering");
    mapped_active=0;
}
int main(void){
    const metasprite_t car[]={{8,0,0,0},{0,8,2,8},{metasprite_end,0,0,0}};
    const metasprite_t human[]={{8,4,0,1},{metasprite_end,0,0,0}};
    const metasprite_t tall[]={{0,0,0,0},{16,0,2,3},{16,8,4,0},{metasprite_end,0,0,0}};
    const metasprite_t empty[]={{metasprite_end,0,0,0}};
    const metasprite_t oversized[]={{0,0,0,0},{0,8,2,0},{0,8,4,0},{0,8,6,0},{0,8,8,0},{0,8,10,0},{0,8,12,0},{0,8,14,0},{0,8,16,0},{metasprite_end,0,0,0}};
    const metasprite_t *poses[]={car,human,tall,empty,oversized};
    for(UBYTE buffer=0;buffer<2;buffer++)for(UBYTE mode=0;mode<2;mode++){
        LCDC_REG=mode?LCDCF_OBJ16:0;
        for(int x=-20;x<=180;x+=4)for(int y=-20;y<=170;y+=5){
            memset((void*)shadow_OAM,0,sizeof(shadow_OAM));memset((void*)shadow_OAM2,0,sizeof(shadow_OAM2));
            compare(car,x,y,250,0,buffer);
        }
        for(unsigned n=0;n<5000;n++){
            unsigned used=random_word()%41;
            for(unsigned i=0;i<40;i++){shadow_OAM[i]=(OAM_item_t){random_word()%180,random_word()%180,random_word(),random_word()};shadow_OAM2[i]=shadow_OAM[i];}
            compare(poses[random_word()%5],(int)(random_word()%300)-70,(int)(random_word()%280)-70,random_word(),used,buffer);
        }
        /* The banked wrapper retains scroll/pinned conventions and validates
         * frame count before reading an authored frame pointer. */
        const metasprite_t *frames[]={car,human};
        spritesheet_t sheet={2,frames};
        actor_t actor={.pos={80*32,90*32},.frame=0,.base_tile=4,.sprite={7,&sheet}};
        volatile OAM_item_t *page=buffer?shadow_OAM2:shadow_OAM;
        allocated_hardware_sprites=0;draw_scroll_x=20;draw_scroll_y=30;
        __render_shadow_OAM=(UBYTE)((UWORD)(uintptr_t)page>>8);
        td_actor_render_actor(&actor);
        require(allocated_hardware_sprites==2&&page[0].x==60&&page[0].y==68,"Banked wrapper preserves signed camera subtraction and metasprite origin");
        allocated_hardware_sprites=0;actor.flags=ACTOR_FLAG_PINNED;
        td_actor_render_actor(&actor);
        require(allocated_hardware_sprites==2&&page[0].x==80&&page[0].y==98,"Pinned actors preserve screen coordinates");
        actor.frame=2;td_actor_render_actor(&actor);
        require(allocated_hardware_sprites==2,"Invalid frame cannot read beyond the compiled pose table");
        draw_scroll_x=draw_scroll_y=0;
        /* Player's earlier objects keep their exact slots in a crowded row. */
        memset((void*)shadow_OAM,0,sizeof(shadow_OAM));memset((void*)shadow_OAM2,0,sizeof(shadow_OAM2));
        allocated_hardware_sprites=0;__render_shadow_OAM=(UBYTE)((UWORD)(uintptr_t)(buffer?shadow_OAM2:shadow_OAM)>>8);
        for(unsigned n=0;n<5;n++)td_actor_render(car,7,0,20+n*25,80);
        require(allocated_hardware_sprites==10,"Five complete cars fill ten scanline slots");
        compare(human,80,80,20,10,buffer);
        require(allocated_hardware_sprites==10,"An eleventh object cannot create alternating hardware losses");
        compare(car,-1024,80,0,10,buffer);compare(car,1024,80,0,10,buffer);
    }
    actor_viewport_cull_checks();
    metadata_bank_checks();
    printf("Ground actor OAM harness: %lu checks, 0 failures\n",checks);return 0;
}
