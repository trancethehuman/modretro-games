/* Actual composed road/air observation and td_people_second behavior.
 * Roof/bridge flags adapt renderer/boat boundaries only: full authored ROM
 * attribute lookup and boat-deck geometry have independent actual-C tests. */
static void police_attention_case(void){
    native_case();td.mode=TD_ROAM;td.u=560*16;td.v=480*16;
    td.wanted=2;td.wanted_left=30;td.cash=143;td.job=4;td.health=87;td.stage=1;
    td_traffic_u[2]=944*16;td_traffic_v[2]=928*16;
    test_aircraft_exposed=1;test_boat_cover=0;td_inside_shop=0;
    td_aircraft_reset(0x4721);
}
static void test_police_aircraft_attention(void){
    police_attention_case();
    expect(!td_police_observed(),"distant ground patrol and absent helicopter cannot observe the courier");
    td_state_t initial=td;unsigned stores=sram_writes;UBYTE saved_seq=td_save_seq;
    td_aircraft_update(1,560,480);
    expect(td_aircraft.active&&td_aircraft.kind&&!td_aircraft_police_observed(),
           "real composed wanted target creates an incoming helicopter before it can see the courier");
    for(unsigned i=0;i<260;i++)td_aircraft_update(1,560,480);
    expect(td_police_observed(),"real aerial approach feeds the composite actual world-space observer");
    td.wanted_left=1;
    expect(!td_people_second()&&td.wanted==2&&td.wanted_left==30&&td.cash==initial.cash&&
           td.job==initial.job&&td.health==initial.health&&sram_writes==stores,
           "visible aerial pursuit keeps attention without a false fine, parcel change or save write");
    test_aircraft_exposed=0;
    expect(!td_police_observed()&&!td_people_second()&&td.wanted_left==29,
           "authored canopy removes air sight and begins genuine attention countdown");
    for(unsigned i=0;i<29;i++)td_people_second();
    expect(td.wanted==1&&td.wanted_left==30&&td.cash==initial.cash&&sram_writes>stores&&td_save_seq==(UBYTE)(saved_seq+1)&&
           td_save_address(td_save_slot)[0]==0x54,
           "thirty quiet active seconds cool exactly one star with the existing persistent save");

    /*Every loaded district uses the same actual nearby patrol predicate. */
    for(UBYTE district=0;district<TD_DISTRICT_COUNT;district++)for(UBYTE heading=0;heading<4;heading++){
        police_attention_case();td.district=district;
        td_traffic_u[2]=td.u;td_traffic_v[2]=td.v;
        WORD offset=heading<2?1535:-1535;
        if(heading&1)td_traffic_v[2]+=offset;else td_traffic_u[2]+=offset;
        expect(td_police_observed(),"nearby actual ground patrol observes in each district and cardinal approach");
        td.wanted_left=4;td_people_second();expect(td.wanted_left==30,"ground observation refreshes the same three-star attention timer");
        td_sandbox_mask|=4;
        expect(!td_police_observed(),"captured police fleet identity cannot keep watching from a ghost patrol location");
        td_sandbox_mask=0;
        td_traffic_u[2]=td.u;td_traffic_v[2]=td.v;
        if(heading&1)td_traffic_v[2]+=(heading<2?1536:-1536);
        else td_traffic_u[2]+=(heading<2?1536:-1536);
        expect(!td_police_observed(),"ground patrol sight has an exact96px open boundary without wrap");
    }
    police_attention_case();td_traffic_u[2]=td.u;td_traffic_v[2]=td.v;
    test_boat_active=1;test_boat_cover=1;
    expect(!td_police_observed(),"occupied launch beneath Don bridge hides its passenger from both observers");
    test_boat_cover=0;expect(td_police_observed(),"visible open-water launch may still be observed from the nearby shore");
    test_boat_active=0;td_inside_shop=1;
    expect(!td_police_observed(),"native shop clock cannot retain an outdoor ground observer while courier is indoors");
    td_inside_shop=0;
    for(UBYTE mode=0;mode<=TD_HELP;mode++){
        if(mode==TD_ROAM){ continue; }td.mode=mode;
        expect(!td_police_observed(),"transit and modal views never expose a proxy camera position as the actual courier");
    }
    td.mode=TD_ROAM;td.wanted=0;expect(!td_police_observed(),"resolved attention cannot create a renewed observer or finance consequence");
    td.wanted=3;td.wanted_left=1;test_aircraft_exposed=0;
    expect(td_people_second()&&td.wanted==2&&td.wanted_left==30,
           "quiet attention decay remains capped at three stars and drops one level at a time");
}
