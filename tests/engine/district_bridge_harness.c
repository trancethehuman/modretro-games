/* Independent hardware adapters around the unmodified production helper.
 * This checks logic and queued bytecode, not GBDK ABI or real GBVM execution. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

typedef uint8_t UBYTE;
typedef uint16_t UWORD;
#define BANKED
#define TRUE 1
#define FALSE 0
#define COLLISION_ALL 15
#define EXCEPTION_CHANGE_SCENE 2
typedef struct { UBYTE bank;void *ptr; } far_ptr_t;
typedef struct { UBYTE width,height;far_ptr_t collisions; } scene_t;
typedef struct { UBYTE bank;UBYTE *PC; } SCRIPT_CTX;

enum { CITY_SCENE_BANK=3,WEST_SCENE_BANK=17,PARK_SCENE_BANK=29,
       CITY_COLLISION_BANK=5,WEST_COLLISION_BANK=23,PARK_COLLISION_BANK=41 };
#define TD_HOST_BANK_scene_toronto_city CITY_SCENE_BANK
#define TD_HOST_BANK_scene_toronto_west WEST_SCENE_BANK
#define TD_HOST_BANK_scene_toronto_high_park PARK_SCENE_BANK
#define TO_FAR_PTR_T(A) {.bank=TD_HOST_BANK_##A,.ptr=(void*)&(A)}

static UBYTE collision_data[3][128*122];
const scene_t scene_toronto_city={128,122,{CITY_COLLISION_BANK,collision_data[0]}};
const scene_t scene_toronto_west={128,122,{WEST_COLLISION_BANK,collision_data[1]}};
const scene_t scene_toronto_high_park={128,122,{PARK_COLLISION_BANK,collision_data[2]}};
far_ptr_t current_scene;
/* Unrelated active-map globals must not be used or swapped by these queries. */
static UBYTE active_collision_bank=77,active_width=45,active_height=61;
static UBYTE *active_collision_ptr;
static UBYTE host_rom_bank=91;

void MemcpyBanked(void *to,const void *from,size_t size,UBYTE bank);
UBYTE ReadBankedUBYTE(const unsigned char *ptr,UBYTE bank);
SCRIPT_CTX *script_execute(UBYTE bank,UBYTE *pc,UWORD *handle,UBYTE nargs,...);
#include "district_under_test.c"

static unsigned checks,failures,metadata_reads,tile_reads,script_calls;
static UBYTE metadata_fault[3],fail_allocation,live_context;
static SCRIPT_CTX captured_context;
static const scene_t *const resources[]={&scene_toronto_city,&scene_toronto_west,&scene_toronto_high_park};
static const UBYTE scene_banks[]={CITY_SCENE_BANK,WEST_SCENE_BANK,PARK_SCENE_BANK};
static const UBYTE collision_banks[]={CITY_COLLISION_BANK,WEST_COLLISION_BANK,PARK_COLLISION_BANK};

static void expect(int condition,const char *name){
    checks++;if(!condition){failures++;fprintf(stderr,"FAIL %s\n",name);}
}

void MemcpyBanked(void *to,const void *from,size_t size,UBYTE bank){
    UBYTE i,saved=host_rom_bank;host_rom_bank=bank;metadata_reads++;
    for(i=0;i<3;i++)if(from==resources[i])break;
    if(i==3||bank!=scene_banks[i]||size!=sizeof(scene_t)){
        expect(0,"metadata uses the resource's actual far pointer/bank");
        memset(to,0,size);host_rom_bank=saved;return;
    }
    memcpy(to,from,size);
    if(metadata_fault[i]==1)((scene_t*)to)->width=127;
    if(metadata_fault[i]==2)((scene_t*)to)->height=121;
    if(metadata_fault[i]==3)((scene_t*)to)->collisions.ptr=NULL;
    host_rom_bank=saved;
}

UBYTE ReadBankedUBYTE(const unsigned char *ptr,UBYTE bank){
    UBYTE i,saved=host_rom_bank,result;uintptr_t address=(uintptr_t)ptr;
    host_rom_bank=bank;tile_reads++;
    for(i=0;i<3;i++){
        uintptr_t start=(uintptr_t)collision_data[i];
        if(address>=start&&address<start+sizeof(collision_data[i]))break;
    }
    if(i==3||bank!=collision_banks[i]){
        expect(0,"tile read stays bounded and uses the collision bank, not the scene bank");
        host_rom_bank=saved;return COLLISION_ALL;
    }
    result=*ptr;host_rom_bank=saved;return result;
}

SCRIPT_CTX *script_execute(UBYTE bank,UBYTE *pc,UWORD *handle,UBYTE nargs,...){
    script_calls++;
    expect(handle==NULL&&nargs==0,"scene bridge has no stack arguments or dangling thread handle");
    if(fail_allocation||live_context)return NULL;
    captured_context.bank=bank;captured_context.PC=pc;live_context=1;
    return &captured_context;
}

static void select_scene(UBYTE id){
    current_scene.bank=scene_banks[id];current_scene.ptr=(void*)resources[id];
}

static void reset_case(void){
    /* Models stock bootstrap discarding contexts before resetting native session. */
    live_context=fail_allocation=0;td_district_reset();
    memset(collision_data,0,sizeof(collision_data));memset(metadata_fault,0,sizeof(metadata_fault));
    memset(&captured_context,0,sizeof(captured_context));select_scene(0);
    metadata_reads=tile_reads=script_calls=0;host_rom_bank=91;
    active_collision_bank=77;active_width=45;active_height=61;active_collision_ptr=collision_data[0]+19;
}

static void expect_script_target(UBYTE id){
    /* Decode the pinned protocol independently: lock, modal fade, then a
     * three-byte change-scene payload. Host pointer width is deliberately
     * wider than GBVM; compare the explicit native16-bit wire address. */
    UBYTE *pc=captured_context.PC;UWORD wire;
    expect(live_context&&pc!=NULL,"successful queue allocates a live VM context");
    if(!live_context||!pc)return;
    expect(pc[0]==0x25,"queued script locks native scene updates");
    expect(pc[1]==0x57&&pc[2]==1,"queued script performs a modal fade-out");
    expect(pc[3]==0x27&&pc[4]==3&&pc[5]==2,"queued script raises change-scene with a three-byte payload");
    wire=(UWORD)(pc[7]|(UWORD)pc[8]<<8);
    expect(pc[6]==scene_banks[id]&&wire==(UWORD)(uintptr_t)resources[id],"queued target uses compiled resource bank and address");
}

static void complete_load(UBYTE id){
    expect_script_target(id);
    /* Only simulate completion after the wire target has been checked. Real
     * GBVM loader ordering, banking, actors and fades remain native evidence. */
    live_context=0;select_scene(id);
}

static void test_scene_identity(void){
    reset_case();
    for(UBYTE i=0;i<3;i++){
        far_ptr_t out={66,(void*)&failures};select_scene(i);
        expect(td_district_current()==i,"current district requires its compiled resource identity");
        expect(td_district_scene(i,&out)&&out.bank==scene_banks[i]&&out.ptr==resources[i],"lookup returns compiler-resolved scene tuple");
    }
    far_ptr_t untouched={66,(void*)&failures};
    expect(!td_district_scene(3,&untouched)&&untouched.bank==66&&untouched.ptr==&failures,"invalid lookup leaves the output untouched");
    expect(!td_district_scene(255,&untouched),"sentinel district is not a scene");
    expect(!td_district_scene(0,NULL),"null scene output is rejected");
    select_scene(1);current_scene.bank=18;
    expect(td_district_current()==255,"same pointer in the wrong ROM bank is not the resource");
    current_scene.ptr=&failures;current_scene.bank=WEST_SCENE_BANK;
    expect(td_district_current()==255,"same bank with an unknown pointer is not a district");
    expect(!metadata_reads&&!tile_reads,"scene identity does not inspect collision bytes");
}

static void test_queue_transactions(void){
    reset_case();
    expect(!td_district_queue(0)&&!td_district_queue(3)&&!td_district_queue(255),"current and invalid districts do not queue");
    expect(script_calls==0,"rejected targets do not allocate VM contexts");
    fail_allocation=1;
    expect(!td_district_queue(1),"allocation failure is reported");
    expect(td_district_current()==0&&!live_context,"allocation failure leaves source loaded and no context live");
    fail_allocation=0;
    expect(td_district_queue(2),"allocation failure does not leave a stuck queue guard");
    expect_script_target(2);
    UBYTE snapshot[9];memcpy(snapshot,captured_context.PC,sizeof(snapshot));
    unsigned calls=script_calls;
    expect(!td_district_queue(1)&&!td_district_queue(2),"pending load rejects repeated and competing requests");
    expect(script_calls==calls&&!memcmp(snapshot,captured_context.PC,sizeof(snapshot)),"pending script remains allocated and byte-for-byte stable");
    expect(td_district_current()==0&&!td_district_queue(1),"observing source does not acknowledge destination");
    current_scene.bank=55;current_scene.ptr=&failures;
    expect(td_district_current()==255&&!td_district_queue(1),"an unrelated scene does not release the pending buffer");
    complete_load(2);
    expect(td_district_current()==2,"loaded destination acknowledges the pending transition");
    expect(td_district_queue(1),"acknowledged transition permits the next district");
    complete_load(1);
    /* Queue itself observes the matching loaded resource even without a
     * separate current() call; it still rejects reloading the current scene. */
    expect(!td_district_queue(1),"matching current resource cannot be redundantly reloaded");
    expect(td_district_queue(0),"return to original district reuses the bridge safely");
    expect_script_target(0);
    /* Interrupt the queued trip using the real bootstrap lifetime contract. */
    live_context=0;select_scene(0);td_district_reset();
    expect(td_district_queue(1),"bootstrap reset releases a discarded context's queue guard");
    expect_script_target(1);
}

static void test_arbitrary_district_collision(void){
    reset_case();
    collision_data[0][10*128+10]=15;
    collision_data[1][10*128+10]=16;
    collision_data[2][10*128+10]=0;
    expect(td_district_tile(0,10,10)==15&&td_district_tile(1,10,10)==16&&td_district_tile(2,10,10)==0,"identical coordinates read each district's independent collision map");
    expect(!td_district_walkable(0,84,84)&&td_district_walkable(1,84,84)&&td_district_walkable(2,84,84),"walking respects walls while retaining existing rail-property behavior");
    expect(!td_district_drivable(0,84,84)&&!td_district_drivable(1,84,84)&&td_district_drivable(2,84,84),"vehicles block both walls and road-only rail properties");
    expect(td_district_current()==0&&active_collision_bank==77&&active_collision_ptr==collision_data[0]+19&&active_width==45&&active_height==61,"remote queries preserve active-map identity and globals");
    expect(host_rom_bank==91,"banked fixture reads restore the caller bank");
    collision_data[2][121*128+127]=4;
    expect(td_district_tile(2,127,121)==4,"last authored tile uses the full16-bit row offset");
    unsigned reads=tile_reads;
    expect(td_district_tile(2,128,121)==15&&td_district_tile(2,127,122)==15&&td_district_tile(2,255,255)==15,"out-of-bounds tiles are blocked");
    expect(tile_reads==reads,"out-of-bounds tiles never dereference beyond the collision array");
    reads=metadata_reads;
    expect(td_district_tile(3,0,0)==15&&!td_district_walkable(255,84,84)&&!td_district_drivable(255,84,84),"unknown districts are blocked for all collision APIs");
    expect(metadata_reads==reads,"unknown districts never resolve an out-of-range resource");
}

static void test_vehicle_footprint(void){
    reset_case();
    /* One obstructed eight-pixel tile lies inside an eleven-pixel footprint
     * whose four corner tiles are clear. A corner-only test would pass. */
    collision_data[1][10*128+10]=16;
    expect(!td_district_drivable(1,84,84),"interior rail tile blocks a three-by-three tile footprint");
    for(UWORD u=72;u<=96;u++)for(UWORD v=72;v<=96;v++){
        /* Geometric oracle: the car rectangle intersects occupied pixel square
         * [80,87]x[80,87]. It does not duplicate production tile traversal. */
        int overlaps=u+5>=80&&u<=92&&v+5>=80&&v<=92;
        expect(td_district_drivable(1,u,v)==!overlaps,"every footprint position agrees with occupied pixel geometry");
    }
    memset(collision_data[1],0,sizeof(collision_data[1]));
    for(unsigned x=9;x<=11;x++)collision_data[1][10*128+x]=16;
    expect(!td_district_drivable(1,84,84),"horizontal thin rail between corner rows blocks the car");
    memset(collision_data[1],0,sizeof(collision_data[1]));
    for(unsigned y=9;y<=11;y++)collision_data[1][y*128+10]=16;
    expect(!td_district_drivable(1,84,84),"vertical thin rail between corner columns blocks the car");
    expect(td_district_drivable(2,84,84),"remote rail does not contaminate another district footprint");
}

static void test_bounds_and_bad_metadata(void){
    reset_case();
    expect(td_district_drivable(1,8,8)&&td_district_drivable(1,1016,968),"clear footprint accepts documented inward map margins");
    expect(!td_district_drivable(1,7,8)&&!td_district_drivable(1,8,7)&&!td_district_drivable(1,1017,968)&&!td_district_drivable(1,1016,969),"car margins reject each outward boundary");
    expect(td_district_walkable(1,0,0)&&td_district_walkable(1,1023,975),"walking can reach the last in-bounds pixel");
    expect(!td_district_walkable(1,1024,975)&&!td_district_walkable(1,1023,976),"walking rejects both outward boundaries");
    unsigned reads=tile_reads,copies=metadata_reads;
    expect(!td_district_drivable(1,65535,65535)&&!td_district_walkable(1,65535,65535),"large unsigned coordinates cannot wrap into local tiles");
    expect(tile_reads==reads&&metadata_reads==copies,"invalid large coordinates never query metadata or tiles");
    for(UBYTE fault=1;fault<=3;fault++){
        metadata_fault[1]=fault;reads=tile_reads;
        expect(td_district_tile(1,10,10)==15&&!td_district_walkable(1,84,84)&&!td_district_drivable(1,84,84),"wrong dimensions or missing collision pointer are blocked");
        expect(tile_reads==reads,"invalid compiled metadata cannot reach collision memory");
        expect(td_district_drivable(2,84,84),"bad district metadata does not disable an independent district");
    }
}

int main(void){
    test_scene_identity();test_queue_transactions();test_arbitrary_district_collision();
    test_vehicle_footprint();test_bounds_and_bad_metadata();
    printf("District bridge host regressions: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
