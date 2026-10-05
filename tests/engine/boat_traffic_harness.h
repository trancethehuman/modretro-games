/* Actual TORONTO traffic/admission/contact code on the committed Port bridge.
 * The boat-controlled flag is the module boundary adapter; boat steering and
 * full hull/water geometry have their separate actual-source sanitizer tests.
 * This representative surface move is not an authored NPC route-start claim. */
static void boat_bridge_surface_case(UBYTE occupied,UBYTE parked){
    native_case();
    test_current_district=td.district=td.park_district=td_streetcar_view_district=TD_DISTRICT_PORT_LANDS;
    td.onfoot=1;td.u=464*16;td.v=136*16;td.safe_u=td.u;td.safe_v=td.v;
    td.park_u=(parked?468:192)*16;td.park_v=(parked?136:552)*16;
    td.seconds=34;td.health=93;td.cash=42;td.job=88;td.stage=1;td.left=100;
    test_boat_active=occupied;td_traffic_advance=128;
    expect(td_world_traffic_init(TD_DISTRICT_PORT_LANDS,td_traffic_u,td_traffic_v,td_traffic_leg,td_traffic_samples),
           "bridge fixture initializes the real Port fleet before its isolated representative surface sweep");
    for(unsigned i=1;i<TD_TRAFFIC_SLOTS;i++){
        td_traffic_u[i]=(i<6?800+i*32:704+(i-6)*32)*16;td_traffic_v[i]=928*16;
        if(i>=6)expect(td_world_traffic_one(TD_DISTRICT_PORT_LANDS,0,1,&td_traffic_samples[i]),
                       "added distant cars retain valid authored sample metadata");
    }
    td_traffic_u[0]=452*16;td_traffic_v[0]=136*16;
    td_traffic_samples[0].u=460*16;td_traffic_samples[0].v=136*16;
    expect(td_district_drivable(TD_DISTRICT_PORT_LANDS,452,136)&&
           td_district_drivable(TD_DISTRICT_PORT_LANDS,460,136)&&
           td_road_sweep(452,136,460,136,5),"both endpoints and the entire surface car hull clear the actual Don bridge collision grid");
}

/* Actual toronto_update input dispatch, with a recording boat boundary.
 * Actual hull/dock checks and post-boarding acceleration remain separately
 * exercised against td_boats.c in boats_harness.c. */
static void test_deliberate_boat_docking_inputs(void){
    native_case();td.onfoot=1;td.u=560*16;td.v=840*16;
    td.safe_u=560*16;td.safe_v=800*16;
    test_boat_active=test_boat_exit_allowed=test_boat_shore_active=1;
    test_boat_shore_u=560*16;test_boat_shore_v=800*16;
    UWORD park_u=td.park_u,park_v=td.park_v;UBYTE vehicle=td.vehicle;

    /* The boarding press is still held, including a subsequently held Down. */
    joy=J_A;td_result_b_release=J_A;world_tick(J_A|J_DOWN,8);
    expect(test_boat_active&&!test_boat_interact_calls&&
           !(test_boat_drive_keys&J_A),
           "held boarding A cannot immediately dock or also accelerate even when Down joins the held input");
    world_tick(0,1);unsigned drives=test_boat_drive_calls;
    world_tick(J_A,8);
    expect(test_boat_active&&!test_boat_interact_calls&&
           test_boat_drive_calls==drives+1&&test_boat_drive_keys==J_A,
           "a fresh plain A after boarding reaches actual boat gas dispatch without requesting an exit at the dock");
    world_tick(J_A,8);
    expect(test_boat_active&&!test_boat_interact_calls&&test_boat_drive_keys==J_A,
           "continued plain gas keeps its controlled identity while moving away from shore");
    world_tick(J_DOWN,1);drives=test_boat_drive_calls;
    expect(test_boat_active&&!test_boat_interact_calls,
           "held Down alone cannot disembark a controlled boat");
    world_tick(J_DOWN|J_A,1);
    expect(!test_boat_active&&test_boat_interact_calls==1&&
           test_boat_drive_calls==drives&&td.u==560*16&&td.v==800*16&&
           td.safe_u==td.u&&td.safe_v==td.v&&(td_result_b_release&J_A),
           "held Down plus fresh A exits once to the dock and consumes that edge without gas dispatch");
    world_tick(J_DOWN|J_A,1);
    expect(test_boat_interact_calls==1&&!test_boat_active&&td.onfoot&&
           td.park_u==park_u&&td.park_v==park_v&&td.vehicle==vehicle,
           "holding the consumed docking chord cannot board again or steal the preserved road vehicle");
}

static void test_boat_traffic_exclusions(void){
    for(UBYTE mode=TD_ROAM;mode<=TD_WAIT;mode++){
        if(mode!=TD_ROAM&&mode!=TD_WAIT)continue;
        boat_bridge_surface_case(1,0);td.mode=mode;
        td_traffic_retreat_mask=1;td_state_t before=td;unsigned stores=sram_writes;
        test_boat_controlled_calls=0;
        td_traffic_motion(1);
        expect(test_boat_controlled_calls==2,
               "an actual occupied-boat movement batch reads occupancy once for admission and once for contacts");
        expect(td_traffic_u[0]==460*16&&td_traffic_v[0]==136*16&&!td_traffic_retreat_mask,
               "occupied boat beneath the bridge clears stale foot-retreat ownership and lets surface traffic advance its full8px");
        expect(!memcmp(&td,&before,58)&&!td_player_hurt&&sram_writes==stores,
               "surface traffic above an occupied boat cannot injure the passenger, alter cargo/money, or write a false injury save");

        boat_bridge_surface_case(0,0);td.mode=mode;
        test_boat_controlled_calls=0;
        td_traffic_motion(1);
        expect(test_boat_controlled_calls==2,
               "an actual pedestrian movement batch retains only two pure boat-occupancy queries");
        expect(td_traffic_u[0]==452*16&&!td_player_hurt&&!td.cooldown&&td.health==93,
               "the same ordinary surface vehicle yields to the visible on-foot courier while a boat underneath lets it pass");

        boat_bridge_surface_case(1,1);td.mode=mode;
        before=td;stores=sram_writes;test_boat_controlled_calls=0;td_traffic_motion(1);
        expect(test_boat_controlled_calls==2,
               "a parked-car denied batch does not repeat boat queries for its eight fleet slots");
        expect(td_traffic_u[0]==452*16&&td_traffic_v[0]==136*16&&!memcmp(&td,&before,58)&&sram_writes==stores,
               "boat occupancy does not waive the original parked road car's complete collision footprint on the bridge");
    }
    boat_bridge_surface_case(0,0);td_traffic_u[0]=460*16;
    td_traffic_motion(1);
    expect((td_traffic_retreat_mask&1)&&td_traffic_u[0]==460*16&&!td_player_hurt,
           "a real foot obstruction retains fail-closed separating policy instead of treating every on-foot courier as a boat passenger");
    boat_bridge_surface_case(1,0);test_boat_controlled_calls=0;td_traffic_contacts();
    expect(test_boat_controlled_calls==1,
           "a contact-only batch reads the pure boat control predicate exactly once");
    test_deliberate_boat_docking_inputs();
}
