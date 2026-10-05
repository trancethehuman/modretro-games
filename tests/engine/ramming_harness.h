/* Real two-body motion, actual fleet admission and raw loaded terrain. */
static void ram_fixture(UBYTE slot){
    reset_case();sandbox_place_fleet();
    static const UBYTE half[8]={5,6,5,7,6,7,5,5};
    td.speed=14;td_vx=224;td.u=400*16;td.v=450*16;
    td_traffic_u[slot]=td.u+(7+half[slot])*16;td_traffic_v[slot]=td.v;
    td_traffic_samples[slot].u=700*16;td_traffic_samples[slot].v=td.v;
}
static void test_corner_physical_admission(void){
    for(unsigned parked=0;parked<2;parked++){
        reset_case();sandbox_place_fleet();geometry=SOUTHWEST_CORNER;td.u=406*16;td.v=392*16;
        td.heading=4;td.speed=16;td_vy=256;
        UWORD body_u=(parked?420:418)*16+12;
        if(parked)td_sandbox_parked[0]=(td_parked_t){body_u,393*16,0,0,0,0,1,0};
        else{td_traffic_u[0]=body_u;td_traffic_v[0]=393*16;}
        expect(td_motion_vehicle_clear(td.u,td.v,td.u,393*16),"the original straight candidate leaves the adjacent car outside its complete body sweep");
        UWORD u=td.u,v=td.v;driving_tick(J_A);
        expect(td.u==u&&td.v==v&&td.speed==0&&!td_corner_used,
               "the real corner assist is rolled back when its lateral segment enters a fleet or parked car");
        expect(!td_ramming_active(0)&&(parked?td_sandbox_parked[0].u:td_traffic_u[0])==body_u,
               "geometry assistance never applies a hidden two-body impulse to a neighbouring car");
    }
    unsigned found=0;td_streetcar_pose_t pose;
    for(unsigned second=0;second<256&&!found;second++){
        if(!td_streetcar_pose(second,0,&pose)||!(pose.heading&4)||pose.u<40*16||pose.v<40*16)continue;
        reset_case();sandbox_place_fleet();td.seconds=second;test_current_district=td.district=td_streetcar_view_district=td_sandbox_district=pose.district;
        td.u=((pose.u>>4)-14)*16;td.v=((pose.v>>4)&~7)*16;
        test_corner_tile_x=(((td.u>>4)-7)>>3)+1;test_corner_tile_y=((td.v>>4)+7)/8+1;
        unsigned shift=test_corner_tile_x*8+7-(td.u>>4);if(shift>6)continue;
        geometry=TRAM_CORNER;
        if(!td_streetcar_runtime_car_clear(td.u,td.v,td.u,td.v+16))continue;
        if(td_streetcar_runtime_car_clear(td.u,td.v,td.u+shift*16,td.v+16))continue;
        found=1;td.heading=4;td.speed=16;td_vy=256;UWORD u=td.u,v=td.v;driving_tick(J_A);
        expect(td.u==u&&td.v==v&&td.speed==0&&!td_corner_used,
               "a genuinely scheduled vertical tram rejects the assisted segment even though the original straight path was clear");
    }
    expect(found,"the native scheduled tram supplies a real corner-clearance boundary for the composed regression");
}
static void test_two_body_ramming(void){
    reset_case();td_state_t clean=td;UWORD clean_u[8],clean_v[8];
    memcpy(clean_u,td_traffic_u,sizeof(clean_u));memcpy(clean_v,td_traffic_v,sizeof(clean_v));
    expect(!td_ramming_step()&&!memcmp(&td,&clean,58)&&!memcmp(clean_u,td_traffic_u,sizeof(clean_u))&&!memcmp(clean_v,td_traffic_v,sizeof(clean_v)),
           "empty ramming phase reports immutable geometry and changes no identity or live position");
    for(unsigned slot=0;slot<8;slot++){
        reset_case();td_ram_state[slot]=4;td_ram_anchor_u[slot]=td_traffic_u[slot];td_ram_anchor_v[slot]=td_traffic_v[slot];
        td_sandbox_mask=1<<slot;
        expect(td_ramming_step()&&!td_ram_state[slot],
               "an active captured identity invalidates stationary reuse even when retiring without movement");
    }
    static const UBYTE half[8]={5,6,5,7,6,7,5,5};
    for(UBYTE slot=0;slot<8;slot++){
        ram_fixture(slot);UWORD original=td_traffic_u[slot],player=td.u;UBYTE leg=td_traffic_leg[slot];
        driving_tick(J_A);
        expect(td_traffic_u[slot]>original&&td_ramming_active(slot),"a real slower NPC body receives a bounded physical impulse in its existing cache slot");
        if(slot==1||slot==3||slot==5)expect(td_vx<0&&td_motion_rebounding(),"a heavier truck/fire/bus body retains the stronger player rebound");
        else expect(td.u>player&&td.speed>=8&&td_vx>0&&!td_motion_rebounding(),"a faster rear impact on a lighter car retains most forward driving momentum");
        td.u=300*16;td.v=450*16;td_vx=td_vy=0;td.speed=0;td_motion_reset();
        unsigned recovered=0;
        for(unsigned n=0;n<80;n++){
            UWORD before=td_traffic_u[slot];expect(td_ramming_step(),"every active impulse or recovery reports dirty before its checked displacement, including its final anchor tick");
            expect(td_ram_distance(before,td_traffic_u[slot])<=16&&td_ram_distance(td_traffic_u[slot],original)<=384,
                   "impulse and route recovery move at most one checked pixel without unbounded displacement");
            expect(td_traffic_leg[slot]==leg,"temporary physical displacement preserves the original authored route phase");
            if(!td_ramming_active(slot)){recovered=1;break;}
        }
        expect(recovered&&td_traffic_u[slot]==original&&td_traffic_v[slot]==450*16,"a pushed driver visibly returns to its real anchor before autonomous route movement resumes");
    }
    UWORD slow_displacement,fast_displacement,heavy_displacement;UBYTE slow_keep,fast_keep,heavy_keep;
    for(unsigned example=0;example<3;example++){
        UBYTE slot=example==2?1:0;ram_fixture(slot);if(!example){td.speed=6;td_vx=96;}
        UWORD original=td_traffic_u[slot];driving_tick(J_A);UBYTE retained=td.speed;
        td.u=300*16;td.v=450*16;td_vx=td_vy=0;td.speed=0;td_motion_reset();
        for(unsigned tick=0;tick<3;tick++)td_ramming_step();
        UWORD displacement=td_traffic_u[slot]-original;
        if(!example){slow_displacement=displacement;slow_keep=retained;}
        else if(example==1){fast_displacement=displacement;fast_keep=retained;}
        else{heavy_displacement=displacement;heavy_keep=retained;}
    }
    expect(fast_displacement>slow_displacement&&fast_keep>slow_keep,
           "a faster real rear impact transfers a larger visible NPC displacement while retaining a higher driving speed");
    expect(fast_keep>=10&&fast_displacement>heavy_displacement&&fast_keep>heavy_keep,
           "the same fast player car pushes a light NPC car farther and retains more speed than against a heavy truck");
    ram_fixture(0);td_traffic_samples[0].u=200*16;UWORD npc=td_traffic_u[0],player=td.u;driving_tick(J_A);
    expect(td_traffic_u[0]>npc&&td.u==player&&td_vx<0&&td.speed==7,"head-on relative velocity pushes the real other body while imposing stronger player recoil");
    ram_fixture(0);td_traffic_u[1]=td_traffic_u[0]+11*16;td_traffic_v[1]=td.v;npc=td_traffic_u[0];driving_tick(J_A);
    expect(td_traffic_u[0]==npc&&!td_ramming_active(0)&&td_vx<0,"a full truck body immediately ahead blocks two-body displacement and leaves no fake impulse");
    ram_fixture(0);actors[9].flags=0;actors[9].pos.x=(td_traffic_u[0]+8*16)*2;actors[9].pos.y=td.v*2;npc=td_traffic_u[0];driving_tick(J_A);
    expect(td_traffic_u[0]==npc&&!td_ramming_active(0),"pushed NPC body sweeps its complete footprint against a visible normal pedestrian");
    ram_fixture(0);td_sandbox_drivers[0]=(td_driver_t){td_traffic_u[0]+8*16,td.v,0,0,60,0,0};actors[19].flags=0;npc=td_traffic_u[0];driving_tick(J_A);
    expect(td_traffic_u[0]==npc&&!td_ramming_active(0),"extracted living drivers also obstruct the pushed body's full physical sweep");
    ram_fixture(0);geometry=EAST_WALL;td.u=375*16;td_traffic_u[0]=395*16;td.v=td_traffic_v[0]=450*16;
    npc=td_traffic_u[0];expect(!td_ramming_try(0,td.u+14,td.v)&&td_traffic_u[0]==npc&&!td_ramming_active(0),"a solid tile across the pushed car's complete body blocks impulse without teleportation");
    ram_fixture(0);td_traffic_u[0]=td.u;td_traffic_v[0]=td.v;td_traffic_u[1]=td.u+11*16;td_traffic_v[1]=td.v;
    npc=td_traffic_u[0];expect(!td_ramming_try(0,td.u+14,td.v)&&td_traffic_u[0]==npc,"an unrecoverable pre-existing overlap stays fail-closed when the separating path contains another whole vehicle");
    ram_fixture(0);npc=td_traffic_u[0];expect(!td_ramming_try(0,td.u+17,td.v)&&td_traffic_u[0]==npc,"malformed over-one-pixel contact cannot create a displacement or corrupt an anchor");
    /* The real brightened stopline remains an admission boundary for a
       temporary impulse; physical contact cannot silently waive signals. */
    ram_fixture(0);td.seconds=8;td.u=172*16;td.v=176*16;td_traffic_u[0]=184*16;td_traffic_v[0]=176*16;
    npc=td_traffic_u[0];expect(!td_ramming_try(0,td.u+14,td.v)&&td_traffic_u[0]==npc,"traffic-signal stopline admission still applies to physical NPC displacement");
    /* One registered wide street tests the same physical path against raw
       native collision bytes, without the synthetic clear-road adapter. */
    ram_fixture(0);geometry=NATIVE_GRID;unsigned x=0,y=0;
    for(unsigned row=24;row<920&&!x;row++)for(unsigned col=40;col<920;col++){
        int clear=1;for(unsigned px=col;px<=col+30;px++)if(!sandbox_raw_body(0,px,row,7)){clear=0;break;}
        if(clear){x=col;y=row;break;}
    }
    expect(x!=0,"native terrain contains a genuinely wide full-body street for two-body replay");
    td.u=x*16;td.v=y*16;td_traffic_u[0]=(x+12)*16;td_traffic_v[0]=td.v;td_traffic_samples[0].u=(x+100)*16;td_traffic_samples[0].v=td.v;
    npc=td_traffic_u[0];driving_tick(J_A);
    expect(td_traffic_u[0]>npc&&td_ramming_active(0)&&sandbox_raw_body(0,td_traffic_u[0]/16,td_traffic_v[0]/16,half[0]),
           "actual registered road terrain admits a moved NPC with its full original body and identity");
}
