/* Actual save source with immutable v8 raw tiles and explicit disk offsets.
 * No sixth map is synthesized: every new checkpoint uses registered geometry. */
static void island_disk_record(UBYTE slot,UBYTE version,const td_state_t *s,UBYTE sequence){
    UBYTE bytes[58]={0};volatile UBYTE *record=td_save_address(slot);UWORD crc=0xFFFF;
    old_word(bytes,0,s->u);old_word(bytes,2,s->v);old_word(bytes,4,s->park_u);old_word(bytes,6,s->park_v);
    old_word(bytes,8,s->cash);old_word(bytes,10,s->seconds);old_word(bytes,12,s->left);old_word(bytes,14,s->speed);
    bytes[16]=s->heading;bytes[17]=s->vehicle;bytes[18]=s->onfoot;bytes[19]=s->mode;
    bytes[20]=s->menu;bytes[21]=s->job;bytes[22]=s->stage;bytes[23]=s->health;
    bytes[24]=s->done;bytes[25]=s->subsecond;memcpy(bytes+26,s->complete,16);
    bytes[42]=s->transit_origin;bytes[43]=s->transit_target;bytes[44]=s->ride_left;
    bytes[45]=s->cooldown;bytes[46]=s->msg;bytes[47]=s->reserved;
    old_word(bytes,48,s->safe_u);old_word(bytes,50,s->safe_v);
    old_word(bytes,52,version<8?1400:s->wanted);old_word(bytes,54,version<8?2200:s->wanted_left);
    bytes[56]=s->district;bytes[57]=s->park_district;
    record[0]=0x54;record[1]=0xD7;record[2]=version;record[3]=58;record[4]=sequence;record[7]=0;
    for(unsigned i=2;i<=4;i++)crc=td_crc_byte(crc,record[i]);
    for(unsigned i=0;i<58;i++){record[8+i]=bytes[i];crc=td_crc_byte(crc,bytes[i]);}
    record[5]=crc;record[6]=crc>>8;
}

static UBYTE island_historical_point(unsigned u,unsigned v){
    const unsigned boxes[3][4]={{42,114,34,6},{80,112,19,8},{100,110,17,7}};
    if(u>=1024||v>=976)return 255;
    for(unsigned region=0;region<3;region++){
        unsigned x=u/8,y=v/8;
        if(x<boxes[region][0]||y<boxes[region][1]||x>=boxes[region][0]+boxes[region][2]||y>=boxes[region][1]+boxes[region][3])continue;
        unsigned tile=(y-boxes[region][1])*boxes[region][2]+x-boxes[region][0];
        return td_fixture_legacy_tiles[region][tile]&15?254:region;
    }
    return 255;
}

static td_state_t island_old_state(unsigned u,unsigned v){
    native_case();td_state_t s=td;
    s.u=s.safe_u=u*16;s.v=s.safe_v=v*16;s.onfoot=1;
    s.cash=2417;s.seconds=65535;s.subsecond=59;s.job=7;s.stage=2;s.left=103;s.health=67;
    s.complete[0]=7;s.done=3;s.heading=9;s.vehicle=3;s.menu=33;s.cooldown=7;s.msg=12;
    s.wanted=2;s.wanted_left=17;
    return s;
}

static td_state_t island_new_state(td_state_t s,UBYTE stop){
    const td_stop_t *checkpoint=&td_fixture_stops[stop];
    s.district=checkpoint->district;s.u=s.safe_u=checkpoint->u*16;s.v=s.safe_v=checkpoint->v*16;
    return s;
}

static void test_island_historical_geometry(void){
    const unsigned boxes[3][4]={{42,114,34,6},{80,112,19,8},{100,110,17,7}};
    unsigned admitted=0,blocked=0;
    expect(sizeof(td_legacy_island_masks)==60,"historical migration masks occupy exactly60 ROM bytes");
    for(unsigned region=0;region<3;region++){
        unsigned left=boxes[region][0]*8,top=boxes[region][1]*8;
        for(unsigned v=top;v<top+boxes[region][3]*8;v++)for(unsigned u=left;u<left+boxes[region][2]*8;u++){
            UBYTE expected=island_historical_point(u,v);
            expect(td_legacy_island_region(u,v)==expected,"every historical whole-pixel classification agrees with independent raw v8 collision tiles");
            if(!(u%8)&&!(v%8)){if(expected==254)blocked++;else admitted++;}
        }
        for(unsigned side=0;side<4;side++){
            unsigned u=side==0?left-1:side==1?left+boxes[region][2]*8:left;
            unsigned v=side==2?top-1:side==3?top+boxes[region][3]*8:top;
            expect(td_legacy_island_region(u,v)==island_historical_point(u,v),"historical rectangle boundaries cannot admit neighbouring water by truncation");
        }
    }
    expect(admitted==354&&blocked==121,"frozen raw geometry contains354 foot tiles and121 blocked tiles with no public car tile");
    expect(td_legacy_island_region(1024,928)==255&&td_legacy_island_region(444,976)==255&&
           td_legacy_island_region(65535,65535)==255,"historical classification fails closed on whole-pixel overflow and map bounds");
}

static void test_island_point_migration(void){
    const unsigned boxes[3][4]={{42,114,34,6},{80,112,19,8},{100,110,17,7}};
    const unsigned clients[3][2]={{560,928},{760,944},{912,912}};
    unsigned accepted=0,rejected=0;
    for(unsigned region=0;region<3;region++)for(unsigned y=0;y<boxes[region][3];y++)for(unsigned x=0;x<boxes[region][2];x++){
        unsigned u=(boxes[region][0]+x)*8+4,v=(boxes[region][1]+y)*8+4;
        td_state_t old=island_old_state(u,v);island_disk_record(0,8,&old,37);
        memset(&td,0,sizeof(td));td_state_t untouched=td;unsigned stores=sram_writes;
        UBYTE expected=island_historical_point(u,v),valid=td_restore();
        expect(valid==(expected<3),"every historical admitted/blocked tile controls restore independently of the new Core water");
        if(expected<3){
            accepted++;unsigned du=u>clients[region][0]?u-clients[region][0]:clients[region][0]-u;
            unsigned dv=v>clients[region][1]?v-clients[region][1]:clients[region][1]-v;
            td_state_t wanted=island_new_state(old,du<15&&dv<15?24+region:20+region);
            expect(!memcmp(&td,&wanted,58),"v8 tile migration changes only player district/checkpoint/safe coordinates and preserves active progress and genuine attention");
            expect(td_valid_state(&td),"each migrated tile has a current semantically valid real scene state");
        }else{rejected++;expect(!memcmp(&td,&untouched,58),"a blocked old tile cannot repair or replace live game state");}
        expect(sram_writes==stores&&td_save_address(0)[2]==8,"reading a historical candidate never overwrites its committed record");
    }
    expect(accepted==354&&rejected==121,"restore covers every distinct historical foot and blocked tile");
    const unsigned old_docks[3][2]={{444,928},{720,920},{848,896}};
    for(unsigned region=0;region<3;region++)for(unsigned fractional=0;fractional<16;fractional++){
        td_state_t old=island_old_state(old_docks[region][0],old_docks[region][1]);
        old.u+=fractional;old.v+=15-fractional;
        island_disk_record(0,8,&old,37);memset(&td,0,sizeof(td));
        td_state_t expected=island_new_state(old,20+region);
        expect(td_restore()&&!memcmp(&td,&expected,58),"all sixteen Q4 fractions retain exact old-dock membership without moving the car or progress");
    }
}

static void test_island_booking_migration(void){
    const unsigned docks[3][2]={{444,928},{720,920},{848,896}};
    for(unsigned region=0;region<3;region++)for(UBYTE mode=TD_WAIT;mode<=TD_RIDE;mode++){
        for(UBYTE remaining=1;remaining<=8;remaining++){
            td_state_t old=island_old_state(docks[region][0]+14,docks[region][1]);
            /* If a dock edge is old solid ground, use the opposite14px edge. */
            if(island_historical_point(old.u/16,old.v/16)!=region)old=island_old_state(docks[region][0]-14,docks[region][1]);
            expect(island_historical_point(old.u/16,old.v/16)==region,"booking fixture uses genuine historical foot terrain");
            old.u+=15;old.mode=mode;old.transit_origin=20+region;old.transit_target=10;
            old.ride_left=mode==TD_RIDE?remaining:0;
            island_disk_record(0,8,&old,37);memset(&td,0,sizeof(td));td_state_t expected=island_new_state(old,20+region);
            expect(td_restore()&&!memcmp(&td,&expected,58),"legitimate near-origin WAIT and partial paid RIDE preserve mode, cash, duration and active deadline at the new dock");
        }
        td_state_t old=island_old_state(docks[region][0],docks[region][1]);
        old.mode=mode;old.transit_origin=20+region;old.transit_target=10;old.ride_left=mode==TD_RIDE?4:0;
        old.u=old.safe_u=640*16;old.v=old.safe_v=784*16;
        island_disk_record(0,8,&old,37);memset(&td,0,sizeof(td));
        if(mode==TD_WAIT){old.mode=TD_ROAM;expect(td_restore()&&!memcmp(&td,&old,58),"an away mainland WAIT cancels uncharged without snapping a valid mainland walker onto an Island");}
        else expect(!td_restore(),"inconsistent paid legacy geometry accepted broadly by v8 is rejected rather than inventing a journey");
    }
    for(unsigned region=0;region<3;region++)for(UBYTE remaining=1;remaining<=8;remaining++){
        td_state_t old=island_old_state(640,784);old.mode=TD_RIDE;old.transit_origin=10;old.transit_target=20+region;old.ride_left=remaining;
        island_disk_record(0,8,&old,37);memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&old,58),"all three outward partial paid ferries remain at their unchanged mainland origin until actual arrival");
        old=island_old_state(docks[region][0],docks[region][1]);old.mode=TD_RIDE;
        old.transit_origin=20+region;old.transit_target=10;old.ride_left=remaining;old.job=TD_NONE;old.health=0;old.left=0;
        island_disk_record(0,8,&old,37);memset(&td,0,sizeof(td));td_state_t expected=island_new_state(old,20+region);
        expect(td_restore()&&!memcmp(&td,&expected,58),"failed already-paid Island rides preserve zero condition, no job, remaining time and paid cash through migration");
    }
    const unsigned clients[3][2]={{560,928},{760,944},{912,912}};
    for(unsigned region=0;region<3;region++){
        td_state_t old=island_old_state(clients[region][0],clients[region][1]);old.mode=TD_WAIT;
        old.transit_origin=20+region;old.transit_target=10;
        island_disk_record(0,8,&old,37);memset(&td,0,sizeof(td));td_state_t expected=island_new_state(old,24+region);expected.mode=TD_ROAM;
        expect(td_restore()&&!memcmp(&td,&expected,58),"an unpaid away WAIT on a poised old client cancels without losing the imminent handoff checkpoint");
    }
    unsigned near=0,away=0;
    for(unsigned region=0;region<3;region++)for(unsigned axis=0;axis<2;axis++)
        for(int sign=-1;sign<=1;sign+=2)for(unsigned distance=14;distance<=15;distance++)
            for(unsigned fractional=0;fractional<16;fractional+=15)for(UBYTE mode=TD_WAIT;mode<=TD_RIDE;mode++){
                td_state_t old=island_old_state(docks[region][0],docks[region][1]);
                if(axis)old.v=(docks[region][1]+sign*(int)distance)*16+fractional;
                else old.u=(docks[region][0]+sign*(int)distance)*16+fractional;
                if(island_historical_point(old.u/16,old.v/16)!=region)continue;
                /* The original boarding rule compares truncated whole pixels,
                   not the visual subpixel distance. */
                unsigned u=old.u/16,v=old.v/16;
                unsigned du=u>docks[region][0]?u-docks[region][0]:docks[region][0]-u;
                unsigned dv=v>docks[region][1]?v-docks[region][1]:docks[region][1]-v;
                UBYTE inside=du<15&&dv<15;old.mode=mode;old.transit_origin=20+region;
                old.transit_target=10;old.ride_left=mode==TD_RIDE?8:0;
                island_disk_record(0,8,&old,37);memset(&td,0,sizeof(td));
                if(inside){
                    near++;td_state_t expected=island_new_state(old,20+region);
                    expect(td_restore()&&!memcmp(&td,&expected,58),"both axes and signs preserve the genuine14-pixel boarding boundary at all endpoint Q4 fractions");
                }else{
                    away++;
                    if(mode==TD_RIDE)expect(!td_restore(),"a paid old ferry15 whole pixels from its origin is not a genuine boarded snapshot");
                    else{
                        td_state_t expected=island_new_state(old,20+region);expected.mode=TD_ROAM;
                        expect(td_restore()&&!memcmp(&td,&expected,58),"a WAIT at the15-pixel edge cancels uncharged before ordinary old-region checkpoint selection");
                    }
                }
            }
    expect(near&&away,"booking boundary cases include actual admitted old ground on both sides of the strict radius");
    for(unsigned region=0;region<3;region++)for(unsigned axis=0;axis<2;axis++)
        for(int sign=-1;sign<=1;sign+=2)for(unsigned distance=14;distance<=15;distance++){
            td_state_t old=island_old_state(clients[region][0],clients[region][1]);
            if(axis)old.v=(clients[region][1]+sign*(int)distance)*16+15;
            else old.u=(clients[region][0]+sign*(int)distance)*16+15;
            if(island_historical_point(old.u/16,old.v/16)!=region)continue;
            td_state_t expected=island_new_state(old,distance<15?24+region:20+region);
            island_disk_record(0,8,&old,37);memset(&td,0,sizeof(td));
            expect(td_restore()&&!memcmp(&td,&expected,58),"the imminent client checkpoint uses the same strict whole-pixel14/15 boundary as real handoffs");
        }
}

static void island_v4_record(const td_state_t *s){
    UBYTE bytes[48],xor=0;volatile UBYTE *record=td_save_address(0);
    /* Reuse the independently written fixed48-byte disk fields, not the
       production structure expansion or migration helpers. */
    write_v5(0,s,0);memcpy(bytes,(const void *)(record+8),sizeof(bytes));
    record[0]=0x54;record[1]=0xD7;record[2]=4;
    for(unsigned i=0;i<48;i++){record[4+i]=bytes[i];xor^=bytes[i];}
    record[3]=xor;
}

static void test_island_v4_import(void){
    const unsigned checkpoints[6][2]={{444,928},{720,920},{848,896},{560,928},{760,944},{912,912}};
    for(unsigned point=0;point<6;point++){
        td_state_t old=island_old_state(checkpoints[point][0],checkpoints[point][1]);
        island_v4_record(&old);memset(&td,0,sizeof(td));sram_writes=0;
        td_state_t expected=island_new_state(old,point<3?20+point:24+point-3);
        expected.wanted=expected.wanted_left=0;expected.job=TD_NONE;expected.stage=0;expected.left=0;
        expected.health=100;expected.mode=TD_ROAM;expected.speed=0;
        expect(td_restore()&&!memcmp(&td,&expected,58),"all six genuine v4 Island points keep earnings/completions while retiring changed ancient work at the appropriate new checkpoint");
        expect(sram_writes==67&&td_save_address(0)[2]==4&&td_save_address(1)[2]==TD_SAVE_VERSION,
               "v4's existing immediate upgrade uses the other slot and preserves its committed historical record");
    }
    td_state_t old=island_old_state(560,928);island_v4_record(&old);
    UBYTE image[sizeof(td_test_sram)];memcpy(image,td_test_sram,sizeof(image));
    td_state_t expected=island_new_state(old,24);expected.wanted=expected.wanted_left=0;
    expected.job=TD_NONE;expected.stage=0;expected.left=0;expected.health=100;expected.mode=TD_ROAM;expected.speed=0;
    for(volatile unsigned cut=1;cut<=67;cut++){
        memcpy(td_test_sram,image,sizeof(image));memset(&td,0,sizeof(td));
        sram_writes=0;sram_interrupt_after=cut;sram_interrupt_enabled=1;
        if(setjmp(interrupted_save)==0){td_restore();expect(0,"v4 upgrade interruption must occur at a real SRAM store");}
        sram_interrupt_enabled=0;memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&expected,58)&&td_save_address(0)[2]==4,
               "every interrupted immediate v4 upgrade recovers the original valid Island point or final current-version checkpoint without changing old earnings");
    }
}

static void test_island_invalid_and_ordering(void){
    td_state_t old=island_old_state(444,928);native_case();td.cash=111;td_state_t stable=td;
    for(unsigned fault=0;fault<20;fault++){
        native_case();island_disk_record(1,9,&stable,36);td_state_t bad=old;
        switch(fault){
            case 0:bad.vehicle=4;break;case 1:bad.heading=16;break;case 2:bad.onfoot=2;break;
            case 3:bad.health=101;break;case 4:bad.subsecond=60;break;case 5:bad.done=4;break;
            case 6:bad.complete[15]=128;break;case 7:bad.job=96;break;case 8:bad.stage=255;break;
            case 9:bad.left=0;break;case 10:bad.health=0;break;case 11:bad.wanted=4;break;
            case 12:bad.wanted_left=0;break;case 13:bad.park_u=444*16;bad.park_v=928*16;break;
            case 14:bad.park_district=5;break;case 15:bad.district=5;break;case 16:bad.reserved=1;break;
            case 17:bad.mode=TD_RIDE;bad.transit_origin=84;bad.transit_target=10;bad.ride_left=4;break;
            case 18:bad.mode=TD_RIDE;bad.transit_origin=20;bad.transit_target=21;bad.ride_left=4;break;
            case 19:bad.mode=TD_RIDE;bad.transit_origin=20;bad.transit_target=10;bad.ride_left=9;break;
        }
        island_disk_record(0,8,&bad,37);memset(&td,0,sizeof(td));unsigned stores=sram_writes;
        expect(td_restore()&&!memcmp(&td,&stable,58)&&sram_writes==stores,
               "CRC-valid invalid nongeographic/car/old-booking data cannot be laundered by a safe map checkpoint or corrupt the fallback");
    }
    native_case();island_disk_record(1,9,&stable,36);island_disk_record(0,8,&old,37);
    td_save_address(0)[8]^=1;stop_reads=0;memset(&td,0,sizeof(td));
    expect(td_restore()&&!memcmp(&td,&stable,58)&&stop_reads==0,"CRC failure precedes every content lookup and map transform");
    native_case();geometry=ISLAND_CHECKPOINT_EDGE;island_disk_record(1,9,&stable,36);island_disk_record(0,8,&old,37);
    expect(td_district_walkable(5,320,280)&&!td_district_walkable(5,317,280),"checkpoint fault leaves its real centre clear but blocks one half3 edge tile");
    memset(&td,0,sizeof(td));expect(td_restore()&&!memcmp(&td,&stable,58),"a centre-only valid new landing with blocked full-foot edge cannot become a migration checkpoint");

    td_state_t current=island_new_state(old,20);current.cash=333;
    for(unsigned direction=0;direction<2;direction++){
        native_case();island_disk_record(0,direction?9:8,direction?&current:&old,255);
        island_disk_record(1,direction?8:9,direction?&old:&current,0);memset(&td,0,sizeof(td));
        td_state_t expected=direction?island_new_state(old,20):current;
        expect(td_restore()&&!memcmp(&td,&expected,58)&&td_save_slot==1&&td_save_seq==0,
               "sequence255 to0 selects the newest genuine mixed v8/v9 record after per-candidate semantic migration");
    }
}

static void test_island_versions_and_upgrade(void){
    const unsigned parks[5][2]={{560,720},{800,64},{736,640},{224,528},{912,128}};
    for(UBYTE version=6;version<=9;version++)for(UBYTE district=0;district<5;district++){
        td_state_t old=island_old_state(444,928);old.park_district=district;old.park_u=parks[district][0]*16;old.park_v=parks[district][1]*16;
        expect(td_district_drivable(district,parks[district][0],parks[district][1]),"each retained mainland parked-car fixture has an actual full native vehicle footprint");
        td_state_t expected=island_new_state(old,20);
        if(version==9)old=expected;
        island_disk_record(0,version,&old,37);memset(&td,0,sizeof(td));
        if(version<8)expected.wanted=expected.wanted_left=0;
        expect(td_restore()&&!memcmp(&td,&expected,58),"v6/v7 clear obsolete cursor words, v8/v9 preserve real attention, and all five parked mainland cars remain fixed");
    }
    td_state_t old=island_old_state(560,928);old.complete[0]=0;old.done=1;old.complete[8]=128;old.wanted=old.wanted_left=0;
    write_v5(0,&old,37);memset(&td,0,sizeof(td));td_state_t expected=island_new_state(old,24);
    expect(td_restore()&&!memcmp(&td,&expected,58),"genuine48-byte v5 expansion still preserves old progress while choosing the imminent new Hanlan client");

    old=island_old_state(560,928);island_disk_record(0,8,&old,37);memset(&td,0,sizeof(td));
    expect(td_restore(),"interrupted upgrade begins with an admitted CRC-valid legacy walker");
    td_state_t migrated=td;UBYTE image[sizeof(td_test_sram)];memcpy(image,td_test_sram,sizeof(image));
    sram_writes=0;td_save();unsigned count=sram_writes;
    expect(count==67&&td_save_address(td_save_slot)[2]==TD_SAVE_VERSION&&td_save_address(td_save_slot)[3]==58,
           "first normal current-version upgrade writes exactly the unchanged payload plus9 metadata stores");
    for(volatile unsigned cut=1;cut<=count;cut++){
        memcpy(td_test_sram,image,sizeof(image));memset(&td,0,sizeof(td));expect(td_restore(),"each interruption recovers the retained v8 candidate first");
        sram_writes=0;sram_interrupt_after=cut;sram_interrupt_enabled=1;
        if(setjmp(interrupted_save)==0){td_save();expect(0,"configured current-version interruption must occur at a real store");}
        sram_interrupt_enabled=0;memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&migrated,58),"every interrupted current-version migration retains all job, cash, car, clock and checkpoint fields without replaying a fare");
        expect(td_save_address(0)[2]==8,"the historical source record remains unchanged during every attempted dual-slot upgrade");
    }
}

static void test_island_save_migration(void){
    expect(TD_SAVE_VERSION==10&&sizeof(td_state_t)==58&&TD_DISTRICT_COUNT==7,
           "the real seven-scene world keeps58 serialized bytes and the earlier Island migration");
    test_island_historical_geometry();test_island_point_migration();test_island_booking_migration();
    test_island_invalid_and_ordering();test_island_versions_and_upgrade();test_island_v4_import();
}
