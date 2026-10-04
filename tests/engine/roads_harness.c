#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "td_roads.h"
#include "collision.h"
#include "roads_fixture.h"

UBYTE joy,joy_pressed,tile_hit_x,tile_hit_y;
static UBYTE grid[128*122],current_bank=13,collision_bank=7;
static unsigned checks,failures,row_queries,column_queries,point_queries,bank_switches;
static void expect(int truth,const char *message){
    checks++;
    if(!truth){failures++;if(failures<10)fprintf(stderr,"FAIL: %s\n",message);}
}
UBYTE tile_at(UBYTE x,UBYTE y){
    point_queries++;
    return x<128&&y<122?grid[y*128+x]:15;
}
/* Hardware adapter for the pinned collision.c NONBANKED contract. The
 * production road code chooses rows/columns; this adapter only reads the
 * requested inclusive native range and restores the calling code bank. */
static UBYTE range(UBYTE horizontal,UBYTE mask,UBYTE fixed,UBYTE start,UBYTE end){
    UBYTE result=0,saved=current_bank;
    int position=start,step=start<end?1:-1;
    tile_hit_x=horizontal?start:fixed;tile_hit_y=horizontal?fixed:start;
    if(tile_hit_x>=128||tile_hit_y>=122)return mask&15?15:0;
    current_bank=collision_bank;bank_switches++;
    for(;;position+=step){
        tile_hit_x=horizontal?position:fixed;tile_hit_y=horizontal?fixed:position;
        if(tile_hit_x>=128||tile_hit_y>=122){result=mask&15?15:0;break;}
        UBYTE value=grid[tile_hit_y*128+tile_hit_x];
        if(value&mask){result=value;break;}
        if(position==end)break;
    }
    current_bank=saved;bank_switches++;
    return result;
}
UBYTE tile_col_test_range_x(UBYTE mask,UBYTE y,UBYTE left,UBYTE right){
    row_queries++;return range(1,mask,y,left,right);
}
UBYTE tile_col_test_range_y(UBYTE mask,UBYTE x,UBYTE top,UBYTE bottom){
    column_queries++;return range(0,mask,x,top,bottom);
}
static void reset_probe(void){
    tile_hit_x=203;tile_hit_y=197;current_bank=13;
    row_queries=column_queries=point_queries=bank_switches=0;
}
static void preserved(void){
    expect(tile_hit_x==203&&tile_hit_y==197&&current_bank==13,
           "clear, blocked and rejected queries preserve native hit globals and calling code bank");
}
/* The oracle works in pixel space, without using production tile rectangle
 * bounds or either range function. It checks every inclusive footprint pixel
 * and rejects any part outside the registered collision image. */
static int oracle_body(unsigned u,unsigned v,unsigned half){
    if(!half||half>8||u<8||v<8||u>1016||v>968)return 0;
    for(int y=(int)v-(int)half;y<=(int)v+(int)half;y++)
    for(int x=(int)u-(int)half;x<=(int)u+(int)half;x++){
        if(x<0||x>=1024||y<0||y>=976||grid[(y/8)*128+x/8])return 0;
    }
    return 1;
}
/* One-pixel centres prove the connected body union, including solids that
 * both endpoints and the four corners would miss. */
static int oracle_sweep(unsigned old_u,unsigned old_v,unsigned u,unsigned v,unsigned half){
    if(!half||half>8||(old_u!=u&&old_v!=v)||old_u<8||u<8||old_v<8||v<8||
       old_u>1016||u>1016||old_v>968||v>968)return 0;
    unsigned low=old_u==u?(old_v<v?old_v:v):(old_u<u?old_u:u);
    unsigned high=old_u==u?(old_v>v?old_v:v):(old_u>u?old_u:u);
    for(unsigned coordinate=low;coordinate<=high;coordinate++)
        if(!oracle_body(old_u==u?u:coordinate,old_u==u?coordinate:v,half))return 0;
    return 1;
}
static void body(unsigned u,unsigned v,unsigned half){
    int expected=oracle_body(u,v,half);reset_probe();
    expect(td_road_body(u,v,half)==expected,"bulk body query matches independent registered pixel-footprint oracle");
    preserved();expect(!point_queries,"full bodies use bulk ranges rather than repeated point-bank reads");
}
static void sweep(unsigned old_u,unsigned old_v,unsigned u,unsigned v,unsigned half){
    int expected=oracle_sweep(old_u,old_v,u,v,half);reset_probe();
    expect(td_road_sweep(old_u,old_v,u,v,half)==expected,"bulk cardinal sweep matches every connected footprint pixel");
    preserved();expect(!point_queries,"cardinal sweeps use bulk ranges rather than repeated point-bank reads");
}
static void test_registered_maps(void){
    const unsigned amounts[]={0,1,7,8,15,16,31,32,64,128,256,512,1008};
    for(unsigned district=0;district<sizeof(oracle_grids)/sizeof(oracle_grids[0]);district++){
        memcpy(grid,oracle_grids[district],sizeof(grid));
        for(unsigned half=1;half<=8;half++)for(unsigned v=8;v<=968;v+=8)
            for(unsigned u=8;u<=1016;u+=8)body(u,v,half);
        /* Every sub-tile phase at representative actual boundary/water/rail
         * coordinates catches flooring and edge inclusivity mistakes. */
        const unsigned anchors[][2]={{8,8},{72,64},{480,528},{560,840},{912,224},{1008,960}};
        for(unsigned a=0;a<sizeof(anchors)/sizeof(anchors[0]);a++)for(unsigned phase=0;phase<8;phase++)
            for(unsigned half=1;half<=8;half++)body(anchors[a][0]+phase,anchors[a][1]+phase,half);
        for(unsigned half=1;half<=8;half++)for(unsigned a=0;a<sizeof(amounts)/sizeof(amounts[0]);a++)
        for(unsigned lane=8;lane<=968;lane+=64){
            unsigned horizontal=8+amounts[a],vertical=8+amounts[a];
            if(horizontal>1016)horizontal=1016;
            if(vertical>968)vertical=968;
            sweep(8,lane,horizontal,lane,half);sweep(horizontal,lane,8,lane,half);
            sweep(lane,8,lane,vertical,half);sweep(lane,vertical,lane,8,half);
        }
    }
}
static void test_native_contract_and_middle_solids(void){
    memset(grid,0,sizeof(grid));
    for(unsigned bit=1;bit<=128;bit*=2){
        grid[50*128+50]=bit;body(400,400,8);grid[50*128+50]=0;
    }
    grid[50*128+60]=128;
    sweep(400,400,560,400,1);sweep(560,400,400,400,8);
    expect(oracle_body(400,400,1)&&oracle_body(560,400,1),"intermediate solid fixture has both endpoint bodies clear");
    grid[50*128+60]=0;grid[60*128+50]=16;
    sweep(400,400,400,560,1);sweep(400,560,400,400,8);
    memset(grid,0,sizeof(grid));
    for(unsigned half=1;half<=8;half++){
        body(1016,968,half);body(8,8,half);
        sweep(8,8,1016,8,half);sweep(8,8,8,968,half);
    }
    reset_probe();expect(td_road_sweep(64,68,944,68,5)&&row_queries==3&&!column_queries&&bank_switches==6,
                        "long horizontal road switches collision bank only once per each of three tile rows");preserved();
    reset_probe();expect(td_road_sweep(68,64,68,944,5)&&column_queries==3&&!row_queries&&bank_switches==6,
                        "long vertical road switches collision bank only once per each of three tile columns");preserved();
    /* The adapter itself follows the native inclusive/reverse/OOB and hit
     * result contract; production restores those intentionally changed hits. */
    grid[40*128+40]=32;current_bank=13;
    expect(tile_col_test_range_x(255,40,50,30)==32&&tile_hit_x==40&&tile_hit_y==40&&current_bank==13,
           "native reverse range reports the first matched high-bit tile and restores code bank");
    expect(!tile_col_test_range_x(15,40,30,50)&&current_bank==13,
           "native lower-bit mask would miss a high-bit-only solid, unlike the required mask255");
    reset_probe();expect(tile_col_test_range_y(255,128,0,10)==15&&!bank_switches&&current_bank==13,
                        "initial native OOB is blocked before any collision-bank switch");
    reset_probe();expect(tile_col_test_range_y(255,0,120,125)==15&&bank_switches==2&&current_bank==13,
                        "a range leaving the image restores code bank on its later OOB exit");
}
static void test_invalid(void){
    memset(grid,0,sizeof(grid));
    const UWORD values[]={0,1,7,8,1016,1017,1024,32768,65535};
    for(unsigned i=0;i<sizeof(values)/sizeof(values[0]);i++){
        body(values[i],400,5);body(400,values[i],5);
        sweep(values[i],400,500,400,5);sweep(400,values[i],400,500,5);
        sweep(500,400,values[i],400,5);sweep(400,500,400,values[i],5);
    }
    for(unsigned half=0;half<=255;half+=17){body(400,400,half);sweep(400,400,500,400,half);}
    sweep(400,400,401,401,5);sweep(400,400,500,500,8);
    reset_probe();expect(!td_road_sweep(400,400,401,401,5)&&!row_queries&&!column_queries,
                        "invalid diagonal fails before reading any collision range");preserved();
    grid[50*128+50]=16;reset_probe();
    expect(td_road_walkable(400,400)&&point_queries==1,"walkable retains its distinct low-four-bit point semantics");preserved();
    grid[50*128+50]=1;reset_probe();expect(!td_road_walkable(400,400),"walkable rejects low-bit obstruction");preserved();
}
int main(void){
    test_registered_maps();test_native_contract_and_middle_solids();test_invalid();
    printf("Road queries: %u checks, %u failures\n",checks,failures);
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
