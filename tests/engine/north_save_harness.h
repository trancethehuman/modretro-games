/* Actual registered-seven-scene engine regression. Expected pre-North terrain
 * is frozen raw Core collision, not the production overlay implementation.
 * Includes island_save_harness.h first for its fixed independent disk writer.
 * These host fixtures prove source save logic, not native SRAM import/ABI. */
static td_state_t north_save_baseline(void){
    native_case();td_state_t s=td;s.cash=333;s.seconds=65535;s.subsecond=59;
    s.wanted=2;s.wanted_left=17;return s;
}
static UBYTE north_old_point(unsigned u,unsigned v){
    return u<1024&&v<976&&!(td_fixture_north_core_old[(v/8)*128+u/8]&15);
}
static UBYTE north_old_body(unsigned u,unsigned v){
    if(u<8||v<8||u>1016||v>968)return 0;
    for(unsigned y=(v-5)/8;y<=(v+5)/8;y++)for(unsigned x=(u-5)/8;x<=(u+5)/8;x++)
        if(td_fixture_north_core_old[y*128+x]&15)return 0;
    return 1;
}
static void test_north_old_field_bounds(void){
    for(UBYTE version=6;version<=9;version++){
        for(unsigned bit=96;bit<128;bit++){
            td_state_t bad=north_save_baseline();bad.complete[bit/8]=(UBYTE)(1u<<(bit%8));bad.done=1;
            island_disk_record(0,version,&bad,37);td_state_t untouched=td;stop_reads=0;
            expect(!td_restore()&&!memcmp(&td,&untouched,58)&&!stop_reads&&!sram_writes,
                   "every impossible old high completion bit fails before content lookup/map transform");
        }
        for(UBYTE fault=0;fault<7;fault++){
            td_state_t bad=north_save_baseline();bad.onfoot=1;bad.u=444*16;bad.v=928*16;
            bad.job=7;bad.stage=2;bad.left=99;
            if(version==9){bad.district=5;bad.u=320*16;bad.v=280*16;}
            switch(fault){case 0:bad.district=6;break;case 1:bad.park_district=6;break;case 2:bad.job=96;bad.stage=1;break;
                case 3:bad.mode=TD_WAIT;bad.transit_origin=59;bad.transit_target=17;break;
                case 4:bad.mode=TD_RIDE;bad.transit_origin=17;bad.transit_target=59;bad.ride_left=4;
                       bad.district=0;bad.u=640*16;bad.v=64*16;break;
                case 5:bad.mode=TD_WAIT;bad.transit_origin=60;bad.transit_target=17;break;
                case 6:bad.mode=TD_WAIT;bad.transit_origin=16;bad.transit_target=63;break;}
            island_disk_record(0,version,&bad,37);td_state_t untouched=td;stop_reads=0;
            expect(!td_restore()&&!memcmp(&td,&untouched,58)&&!stop_reads&&!sram_writes,
                   "legacy district6/job96/paid59+ cannot be repaired at a safe Island or North checkpoint");
        }
    }
    td_state_t crc_bad=north_save_baseline();crc_bad.onfoot=1;crc_bad.u=444*16;crc_bad.v=928*16;
    island_disk_record(0,8,&crc_bad,37);td_save_address(0)[8]^=1;stop_reads=0;td_state_t untouched=td;
    expect(!td_restore()&&!memcmp(&td,&untouched,58)&&!stop_reads&&!sram_writes,
           "CRC failure precedes every North/Island lookup and transform");
}
static void test_north_historical_core_overlay(void){
    unsigned changes=0;
    for(unsigned y=0;y<122;y++)for(unsigned x=0;x<128;x++){
        UBYTE before=td_fixture_north_core_old[y*128+x],after=native_collision[0][y*128+x];
        UBYTE changed=y<3&&((x>=38&&x<=45)||(x>=76&&x<=83));
        expect(changed?before==15&&(after==0||after==16):before==after,
               "only the exact48 old-solid Core throat tiles change in the registered North milestone");
        if(changed)changes++;
    }
    expect(changes==48,"historical Core exception contains exactly48 independently retained tiles");
    native_case();
    for(unsigned v=0;v<40;v++)for(unsigned u=280;u<700;u+=4){
        expect((td_district_walkable(0,u,v)&&td_north_legacy_clear(u,v,0))==north_old_point(u,v),
               "point overlay matches actual frozen old Core ground, not expanded foot-body semantics");
        if(v>=8)expect((td_district_drivable(0,u,v)&&td_north_legacy_clear(u,v,5))==north_old_body(u,v),
               "saved half5 body respects every frozen historical solid tile while accepted player sidewalks remain driveable");
    }
    for(unsigned x=0;x<128;x++)for(unsigned y=0;y<3;y++){
        if(!((x>=38&&x<=45)||(x>=76&&x<=83)))continue;
        for(UBYTE fraction=0;fraction<=15;fraction+=15){
            td_state_t old=north_save_baseline();old.onfoot=1;old.u=(x*8+4)*16+fraction;old.v=(y*8+4)*16+15-fraction;
            for(UBYTE version=6;version<=9;version++){
                native_case();island_disk_record(0,version,&old,37);
                expect(!td_restore(),"all changed old-solid tiles remain invalid at both endpoint Q4 fractions for every old58-byte format");
            }
            native_case();island_disk_record(0,10,&old,37);memset(&td,0,sizeof(td));
            expect(td_restore()&&!memcmp(&td,&old,58),"currentv10 uses actual opened foot ground without shifting local coordinates");
        }
    }
    for(unsigned centre=0;centre<2;centre++)for(unsigned v=23;v<=30;v++)for(UBYTE foot=0;foot<2;foot++){
        unsigned u=centre?640:336;td_state_t old=north_save_baseline();old.u=u*16+15;old.v=v*16+15;old.onfoot=foot;
        island_disk_record(0,9,&old,37);memset(&td,0,sizeof(td));UBYTE accepted=td_restore();
        expect(accepted==(foot?north_old_point(u,v):north_old_body(u,v)),
               "genuine old footv24 and bodyv29 boundaries are retained independently of new atlas offsets");
        if(accepted)expect(!memcmp(&td,&old,58),"accepted v9 Q4 pose and mainland car remain unchanged");
        old=north_save_baseline();old.park_u=u*16;old.park_v=v*16;island_disk_record(0,9,&old,37);
        expect(td_restore()==north_old_body(u,v),"old parked body cannot intrude into newly opened formerly solid tile");
    }
    for(UBYTE version=4;version<=5;version++)for(unsigned centre=0;centre<2;centre++)for(unsigned v=4;v<24;v+=8){
        td_state_t old=north_save_baseline();old.onfoot=1;old.u=(centre?640:336)*16;old.v=v*16;
        if(version==4)island_v4_record(&old);else write_v5(0,&old,37);
        expect(!td_restore()&&!sram_writes,"old48-byte saves are not repaired or auto-upgraded by North road openings");
    }
}
static void test_north_current_and_v9_upgrade(void){
    for(UBYTE id=59;id<=63;id++){
        td_state_t s=north_save_baseline();td_stop_t stop;td_get_stop(id,&stop);
        expect(stop.district==6&&td_district_walkable(6,stop.u,stop.v),"every actual appended North client is on its registered foot ground");
        s.onfoot=1;s.district=6;s.u=s.safe_u=stop.u*16+1;s.v=s.safe_v=stop.v*16+15;
        s.job=103;s.stage=1;s.left=120;memset(s.complete,255,13);s.done=104;
        island_disk_record(0,10,&s,37);memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&s,58),"actual North foot/active final job/full104 bitmap restores through unchanged58-byte format");
        if(id<=60){s.job=TD_NONE;s.mode=TD_RIDE;s.transit_origin=id;s.transit_target=17;s.ride_left=1;
            native_case();island_disk_record(0,10,&s,37);memset(&td,0,sizeof(td));
            expect(td_restore()&&!memcmp(&td,&s,58),"actual northern paid train remains at its real origin until ordinary arrival");}
    }
    for(UBYTE version=6;version<=10;version++){
        td_state_t s=north_save_baseline();s.onfoot=1;s.u=640*16;s.v=176*16;s.mode=TD_RIDE;
        s.transit_origin=80;s.transit_target=18;s.ride_left=4;island_disk_record(0,version,&s,37);
        td_state_t expected=s;if(version<8){ expected.wanted=expected.wanted_left=0; }memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&expected,58),"encoded80 Wellesley bus and obsolete/current attention rules retain exact old semantics");
    }
    for(UBYTE version=8;version<=9;version++){
        td_state_t s=north_save_baseline();s.onfoot=1;s.district=4;s.u=672*16;s.v=272*16;
        s.job=95;s.stage=1;s.left=99;s.health=67;memset(s.complete,255,12);s.done=96;
        island_disk_record(0,version,&s,37);memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&s,58),"all old96 bits and active Port95 remain compatible without replaying payment");
    }
    for(UBYTE reverse=0;reverse<2;reverse++){
        td_state_t a=north_save_baseline(),b=a;b.cash=444;
        island_disk_record(0,reverse?10:9,&a,255);island_disk_record(1,reverse?9:10,&b,0);memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&b,58)&&td_save_slot==1&&td_save_seq==0,
               "mixed9/10 slots preserve signed sequence255-to0 newest selection");
    }
    td_state_t old=north_save_baseline();island_disk_record(0,9,&old,37);memset(&td,0,sizeof(td));
    expect(td_restore()&&!memcmp(&td,&old,58),"explicit9 reader accepts a genuine snapshot unchanged before first normal upgrade");
    UBYTE image[sizeof(td_test_sram)];memcpy(image,td_test_sram,sizeof(image));sram_writes=0;td_save();
    expect(sram_writes==67&&td_save_address(1)[2]==TD_SAVE_VERSION&&td_save_address(1)[3]==58,"normal9-to11 upgrade retains67 actual atomic stores");
    for(volatile unsigned cut=1;cut<=67;cut++){
        memcpy(td_test_sram,image,sizeof(image));memset(&td,0,sizeof(td));expect(td_restore(),"interrupted upgrade begins from genuine admitted9 slot");
        sram_writes=0;sram_interrupt_after=cut;sram_interrupt_enabled=1;
        if(setjmp(interrupted_save)==0){td_save();expect(0,"upgrade interruption reaches actual SRAM store");}
        sram_interrupt_enabled=0;memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&old,58)&&td_save_address(0)[2]==9,
               "every interrupted9-to10 write recovers the intact old or equivalent committed new snapshot");
    }
}
static void test_north_save_migration(void){
    expect(TD_SAVE_VERSION==11&&TD_DISTRICT_COUNT==7&&TD_QUESTS==104&&TD_STOPS==64&&sizeof(td_state_t)==58,
           "actual North world appends7/104/64 with unchanged58-byte size with v11 player fields");
    test_north_old_field_bounds();test_north_historical_core_overlay();test_north_current_and_v9_upgrade();
}
