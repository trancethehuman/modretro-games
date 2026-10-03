/* Included after the runtime adapters; genuine native code runs under host
 * hardware stubs. These fixtures do not prove native banking or playability. */
static void test_island_objective_guidance(void){
    const UWORD mainland[5][2]={{560,720},{400,528},{512,240},{128,528},{912,64}};
    const UBYTE first_district[5]={TD_DISTRICT_NONE,0,1,0,3};
    td_stop_t ferry,client;
    for(UBYTE district=0;district<5;district++)for(UBYTE foot=0;foot<2;foot++){
        native_case();td.district=test_current_district=district;td.onfoot=foot;
        td.u=mainland[district][0]*16;td.v=mainland[district][1]*16;
        td.job=7;td_get_job(td.job,&td_job);td.stage=1;td.left=103;td.health=67;
        td_get_stop(21,&client);td_get_stop(10,&ferry);
        td_state_t before=td;td_stop_t cursor_before=td_cursor;
        td_set_target();
        expect(!memcmp(&before,&td,sizeof(td))&&!memcmp(&cursor_before,&td_cursor,sizeof(td_cursor)),
               "ferry guidance changes neither saved game nor an existing transit cursor");
        expect(td_target.district==5&&td_target.u==client.u&&td_target.v==client.v&&
               !strcmp(td_target.name,client.name)&&!td_target.reserved,
               "a mainland approach keeps the actual Island quest client identity and district");
        expect(!(actors[1].flags&ACTOR_FLAG_HIDDEN)&&td_route_district==first_district[district],
               "ferry guidance uses the real mainland branch before the local terminal");
        if(!district)expect(actors[1].pos.x==ferry.u*32&&actors[1].pos.y==(ferry.v-12)*32,
                           "Core's Island objective beacon points at terminal10 without delivering there");
        else{
            UBYTE real=0;
            for(unsigned i=0;i<TD_PORTALS;i++)if(td_portals[i].from==district&&
                td_portals[i].to==first_district[district]&&
                actors[1].pos.x==td_portals[i].u*32&&actors[1].pos.y==(td_portals[i].v-12)*32)real=1;
            expect(real,"a remote terminal approach beacon is an authored ordinary mainland seam");
        }
        td_interact();expect(td.stage==1&&td.job==7&&td.msg==6,
                            "ferry approach coordinates never substitute for the actual Island handoff");
    }
    const UBYTE clients[3]={24,25,26},docks[3]={20,21,22},jobs[3]={15,7,23};
    for(UBYTE region=0;region<3;region++){
        native_case();td.district=test_current_district=TD_DISTRICT_ISLANDS;td.onfoot=1;
        td_get_stop(clients[region],&client);td_get_stop(docks[region],&ferry);
        td.u=client.u*16;td.v=client.v*16;td.job=jobs[region];td_get_job(td.job,&td_job);
        td.stage=0;td.left=103;td.health=67;
        td_state_t before=td;td_set_target();
        expect(!memcmp(&before,&td,sizeof(td))&&td_target.district==0&&td_target.u==640&&td_target.v==784,
               "an Island return cue preserves the actual mainland terminal pickup and saved fields");
        expect(td_route_district==TD_DISTRICT_NONE&&!(actors[1].flags&ACTOR_FLAG_HIDDEN)&&
               actors[1].pos.x==ferry.u*32&&actors[1].pos.y==(ferry.v-12)*32,
               "a client return displays a nearby real Island dock without a mainland road route");
        td.job=TD_NONE;td.stage=11;before=td;td_set_target();
        expect(!memcmp(&before,&td,sizeof(td))&&td_target.district==0&&td_target.u==560&&td_target.v==720&&
               actors[1].pos.x==ferry.u*32&&actors[1].pos.y==(ferry.v-12)*32,
               "retired Island work keeps Union as the real objective while guiding the ferry return");
        td.job=jobs[region];td_get_job(td.job,&td_job);td.stage=2;td.left=103;
        before=td;td_set_target();
        expect(!memcmp(&before,&td,sizeof(td))&&td_target.district==TD_DISTRICT_ISLANDS&&
               td_target.u==client.u&&td_target.v==client.v&&td_route_district==TD_DISTRICT_NONE&&
               !(actors[1].flags&ACTOR_FLAG_HIDDEN)&&actors[1].pos.x==client.u*32&&
               actors[1].pos.y==(client.v-12)*32,
               "a local Island handoff uses its exact public-path client instead of a ferry or parking anchor");
    }
    native_case();td.district=test_current_district=TD_DISTRICT_ISLANDS;td.onfoot=1;
    td.u=512*16;td.v=744*16;td_set_target();
    expect(actors[1].pos.x==512*32&&actors[1].pos.y==(448-12)*32,
           "a free Centre walker initially receives the Centre return dock");
    td.u=904*16;td.v=440*16;td_state_t before=td;td_stop_t objective=td_target;
    td_second();before.seconds++;
    expect(!memcmp(&td,&before,58)&&!memcmp(&td_target,&objective,sizeof(objective))&&
           actors[1].pos.x==920*32&&actors[1].pos.y==(280-12)*32,
           "an active-world second refreshes the nearest return dock after walking without moving the real objective or parked car");
}
