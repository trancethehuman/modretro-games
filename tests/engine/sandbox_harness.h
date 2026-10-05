/* Actual native sandbox functions; raw registered collision bytes are the
 * independent parked-placement oracle. No duplicated theft implementation. */
static unsigned sandbox_raw_body(UBYTE district,UWORD u,UWORD v,unsigned half){
    if(district>=TD_DISTRICT_COUNT||u<half||v<half||u+half>=1024||v+half>=976)return 0;
    for(unsigned y=(v-half)/8;y<=(v+half)/8;y++)for(unsigned x=(u-half)/8;x<=(u+half)/8;x++)
        if(native_collision[district][y*128+x]&15)return 0;
    return 1;
}
static void sandbox_place_fleet(void){
    td_sandbox_u=td_traffic_u;td_sandbox_v=td_traffic_v;td_sandbox_district=td_streetcar_view_district=td.district=0;td_sandbox_ready=1;
    for(unsigned i=0;i<8;i++){td_traffic_u[i]=(704+i*32)*16;td_traffic_v[i]=928*16;}
}
static void test_sandbox_viewport_cull(void){
    static const int screen_xs[]={-9,-8,-7,0,159,167,168,169};
    static const int screen_ys[]={-1,0,1,143,159,160,161};
    reset_case();sandbox_place_fleet();
    td_sandbox_parked[0]=(td_parked_t){512*16,512*16,0,0,0,0,1,0};
    actor_t original_player=PLAYER;td_state_t original_td=td;
    for(unsigned family=0;family<11;family++)for(unsigned heading=0;heading<16;heading++)
        for(unsigned ix=0;ix<sizeof(screen_xs)/sizeof(screen_xs[0]);ix++)
            for(unsigned iy=0;iy<sizeof(screen_ys)/sizeof(screen_ys[0]);iy++){
                td_sandbox_parked[0].skin=family<7?family:TD_NONE;
                td_sandbox_parked[0].vehicle=family<7?td_sandbox_class(family):family-7;
                td_sandbox_parked[0].heading=heading;
                draw_scroll_x=512-screen_xs[ix];draw_scroll_y=512-screen_ys[iy];
                unsigned visible=0;
                /* Independent pixels of the authored two-cell vehicle pose,
                   with native OAM origins(8,16). Include partially visible
                   sprites at every edge, for every role/heading. */
                for(int py=0;py<16;py++)for(int px=0;px<16;px++){
                    int x=screen_xs[ix]+px-8,y=screen_ys[iy]+py-16;
                    if(x>=0&&x<160&&y>=0&&y<144)visible=1;
                }
                sandbox_render_calls=sandbox_pose_calls=0;td_sandbox_render();
                expect(sandbox_render_calls==visible&&sandbox_pose_calls==visible,
                       "full parked-car pose cull retains every partial viewport edge and skips sprite work wholly outside");
                expect(!memcmp(&PLAYER,&original_player,sizeof(PLAYER))&&!memcmp(&td,&original_td,sizeof(td)),
                       "viewport culling never modifies live player, vehicle ownership or saved game state");
            }
    draw_scroll_x=432;draw_scroll_y=424;td_sandbox_owner=8;
    sandbox_render_calls=sandbox_pose_calls=0;td_sandbox_render();
    expect(!sandbox_render_calls&&!sandbox_pose_calls,"owned aliased parked row remains excluded even when its whole pose is visible");
    td_sandbox_owner=TD_NONE;test_current_district=TD_DISTRICT_NONE;td_sandbox_render();
    expect(!sandbox_render_calls&&!sandbox_pose_calls,"inside shop no stale outside parked row reaches pose work");
}
/* Wider independent geometry covers arbitrary raw endpoint hulls. The bucket
 * gate is an optimization only: existing-overlap recovery, diagonal/long
 * probes and malformed wrapping old endpoints retain the previous policy. */
static unsigned sandbox_reference_body(unsigned ou,unsigned ov,unsigned u,unsigned v,unsigned cu,unsigned cv,unsigned radius){
    unsigned du=ou>cu?ou-cu:cu-ou,dv=ov>cv?ov-cv:cv-ov;
    unsigned next_du=u>cu?u-cu:cu-u,next_dv=v>cv?v-cv:cv-v;
    if(du<radius&&dv<radius){
        if((u>ou&&ou<cu)||(u<ou&&ou>cu)||(v>ov&&ov<cv)||(v<ov&&ov>cv))return 0;
        return next_du+next_dv>du+dv;
    }
    unsigned left=ou<u?ou:u,right=ou>u?ou:u,top=ov<v?ov:v,bottom=ov>v?ov:v;
    return (cu<left&&left-cu>=radius)||(cu>right&&cu-right>=radius)||
        (cv<top&&top-cv>=radius)||(cv>bottom&&cv-bottom>=radius);
}
static unsigned sandbox_reference_clear(UWORD ou,UWORD ov,UWORD u,UWORD v,UBYTE half,UBYTE skip){
    const unsigned extent[8]={5,6,5,7,6,7,5,5};
    if(!half||half>8||u>=16384||v>=15616)return 0;
    if(skip>=8&&td_streetcar_view_district==td_sandbox_district&&td_sandbox_u&&td_streetcar_view_district!=5)
        for(unsigned i=0;i<8;i++)if(i!=skip&&!(skip==TD_NONE&&i==td_sandbox_owner))
            if(!sandbox_reference_body(ou,ov,u,v,td_sandbox_u[i],td_sandbox_v[i],(half+extent[i])*16))return 0;
    for(unsigned i=0;i<TD_SANDBOX_PARKED;i++)if(td_sandbox_parked[i].active&&td_sandbox_parked[i].district==td_streetcar_view_district&&
        !(skip==TD_NONE&&td_sandbox_owner==i+8))
        if(!sandbox_reference_body(ou,ov,u,v,td_sandbox_parked[i].u,td_sandbox_parked[i].v,(half+7)*16))return 0;
    if(skip<8)for(unsigned i=0;i<2;i++)if(td_sandbox_drivers[i].timer&&!(td_sandbox_drivers[i].kind&128))
        if(!sandbox_reference_body(ou,ov,u,v,td_sandbox_drivers[i].u,td_sandbox_drivers[i].v,(half+3)*16))return 0;
    return 1;
}
static uint32_t sandbox_oracle_seed=0xe752b139;
static uint32_t sandbox_oracle_random(void){sandbox_oracle_seed=sandbox_oracle_seed*1664525u+1013904223u;return sandbox_oracle_seed;}
static void test_sandbox_bucket_hulls(void){
    reset_case();sandbox_place_fleet();
    for(unsigned n=0;n<400000;n++){
        for(unsigned i=0;i<8;i++){td_sandbox_u[i]=(UWORD)(sandbox_oracle_random()>>8);td_sandbox_v[i]=(UWORD)(sandbox_oracle_random()>>8);}
        for(unsigned i=0;i<TD_SANDBOX_PARKED;i++)td_sandbox_parked[i]=(td_parked_t){
            (UWORD)(sandbox_oracle_random()>>8),(UWORD)(sandbox_oracle_random()>>8),(UBYTE)(n%3),0,0,0,(UBYTE)((n>>(i+3))&1),0};
        for(unsigned i=0;i<2;i++)td_sandbox_drivers[i]=(td_driver_t){
            (UWORD)(sandbox_oracle_random()>>8),(UWORD)(sandbox_oracle_random()>>8),0,0,(UBYTE)((n>>(i+5))&1),(UBYTE)((n>>(i+7))&128),0};
        td_sandbox_owner=n%14;td_streetcar_view_district=n%4;td_sandbox_district=n%5;
        UWORD ou=sandbox_oracle_random()>>8,ov=sandbox_oracle_random()>>8;
        UWORD u=(sandbox_oracle_random()>>8)%16384,v=(sandbox_oracle_random()>>8)%15616;
        if(!(n&3)){ou=td_sandbox_u[0]+(int)(n%481)-240;ov=td_sandbox_v[0]+(int)((n*13)%481)-240;td_streetcar_view_district=td_sandbox_district=0;}
        if(!(n&15)){u=sandbox_oracle_random()>>8;v=sandbox_oracle_random()>>8;}
        UBYTE half=n%11,skip=n%12<8?n%8:n%12==8?TD_NONE:n%12==9?254:n%12;
        expect(td_sandbox_clear(ou,ov,u,v,half,skip)==sandbox_reference_clear(ou,ov,u,v,half,skip),
               "byte broad rejection preserves arbitrary long/diagonal/wrapped hulls, strict radius, recovered overlap and every ownership/driver branch");
    }
    static const UWORD edges[]={0,1,15,16,239,240,255,256,257,511,512,767,768,16383,16384,32767,65535};
    for(unsigned i=0;i<sizeof(edges)/sizeof(edges[0]);i++)for(unsigned j=0;j<sizeof(edges)/sizeof(edges[0]);j++)
        for(unsigned half=1;half<=8;half++){
            sandbox_place_fleet();memset(td_sandbox_parked,0,sizeof(td_sandbox_parked));memset(td_sandbox_drivers,0,sizeof(td_sandbox_drivers));
            td_sandbox_owner=TD_NONE;td_sandbox_u[0]=edges[i];td_sandbox_v[0]=edges[j];
            UWORD ou=edges[j],ov=edges[i],u=edges[i]%16384,v=edges[j]%15616;
            expect(td_sandbox_clear(ou,ov,u,v,half,TD_NONE)==sandbox_reference_clear(ou,ov,u,v,half,TD_NONE),
                   "full endpoint hull preserves exact map, byte-bucket and strict combined-radius boundaries");
        }
}
static void test_taxi_lifetime(void){
    for(unsigned direction=0;direction<4;direction++){
        reset_case();sandbox_place_fleet();td.onfoot=1;td.u=400*16;td.v=450*16;td.park_u=80*16;td.park_v=64*16;
        td_traffic_u[7]=410*16;td_traffic_v[7]=450*16;td_fleet_present(&actors[18],6,direction);
        expect(td_sandbox_interact()==1&&td_sandbox_skin==6&&td.vehicle==0&&td.heading==direction*4,
               "taxi theft extracts a living commuter and retains its canonical sedan class and all four gold taxi headings");
        for(unsigned tick=0;tick<12;tick++){td_sandbox_tick();driving_tick(0);}
        expect(!td.onfoot&&!td_entry_timer&&td.u==410*16&&td.v==450*16&&td_sandbox_owned(7),
               "actual native entry completes into the existing taxi identity without adding an actor or moving its body");
        for(unsigned heading=0;heading<16;heading++){
            td.heading=heading;td_sandbox_present();
            expect(PLAYER.frame==16+((heading+1)&15)/4&&td.speed==0&&!td_vx&&!td_vy,
                   "every at-rest owned taxi heading selects the new original taxi sheet without changing stationary physics");
        }
        td.heading=direction*4;td_motion_enter_exit();
        for(unsigned tick=0;tick<12;tick++)driving_tick(0);
        td_sandbox_sync();td_traffic_present();td_sandbox_present();
        expect(td.onfoot&&td_sandbox_skin==6&&actors[8].frame==16+direction&&td_sandbox_captured(7),
               "leaving the taxi preserves the visible parked gold livery and captured physical identity");
        td.u=480*16;td.v=450*16;td_traffic_u[0]=490*16;td_traffic_v[0]=450*16;
        expect(td_sandbox_interact()==1&&td_sandbox_owned(0)&&td_sandbox_captured(7),
               "taking another car preserves the abandoned taxi instead of spawning a replacement or losing it");
        td_traffic_present();
        expect(!(actors[18].flags&ACTOR_FLAG_HIDDEN)&&actors[18].frame==16+direction&&td_traffic_u[7]==410*16,
               "abandoned taxi remains a distinct visible solid original car at its preserved cardinal heading");
    }
}
static void test_sandbox_actual(void){
    test_taxi_lifetime();
    test_sandbox_bucket_hulls();
    test_sandbox_viewport_cull();
    for(UBYTE district=0;district<TD_DISTRICT_COUNT;district++)for(unsigned seed=1;seed<257;seed++){
        reset_case();geometry=NATIVE_GRID;test_current_district=td.district=td_streetcar_view_district=district;td_sandbox_seed=seed;td_sandbox_u=td_sandbox_v=NULL;
        td_sandbox_bind(td_traffic_u,td_traffic_v,district);
        unsigned count=0;
        for(unsigned i=0;i<TD_SANDBOX_PARKED;i++)if(td_sandbox_parked[i].active){
            count++;expect(sandbox_raw_body(district,td_sandbox_parked[i].u/16,td_sandbox_parked[i].v/16,8),
                           "random curb residents have clear full half8 bodies in registered native collision tiles");
        }
        expect(count==(district==TD_DISTRICT_ISLANDS?0:2),"every mainland seeds two actual curb vehicles, Islands seed none");
    }
    for(unsigned role=0;role<8;role++)for(unsigned direction=0;direction<4;direction++){
        reset_case();sandbox_place_fleet();td.onfoot=1;td.u=400*16;td.v=450*16;td.park_u=80*16;td.park_v=64*16;
        td.vehicle=2;td.heading=0;td_traffic_u[role]=410*16;td_traffic_v[role]=450*16;
        td_fleet_present(&actors[role<6?role+2:role+11],role<6?role:role==7?6:0,direction);
        UWORD old_u=td.park_u,old_v=td.park_v;
        expect(td_sandbox_interact()==1,"all six service roles plus both new ordinary cars are quickly controllable");
        expect(td_sandbox_owned(role)&&td_sandbox_captured(role)&&td.vehicle==(role==1||role==3||role==5?1:0)&&td.heading==direction*4,
               "theft uses role physics and exact cardinal native heading, including east/south/west/north original car frames");
        expect(td.onfoot&&td_entry_timer==12&&!td_entry_target&&td_sandbox_extracting()&&td.wanted==1,
               "theft begins visible nonlethal extraction before native entry and adds one attention level");
        expect(td_sandbox_parked[2].active&&td_sandbox_parked[2].protected&&td_sandbox_parked[2].u==old_u&&td_sandbox_parked[2].v==old_v&&td_sandbox_parked[2].vehicle==2,
               "taking a road vehicle preserves the original owned motorcycle rather than destroying it");
        for(unsigned tick=0;tick<12;tick++){td_sandbox_tick();td_sandbox_present();}
        expect(!td_sandbox_extracting()&&td_sandbox_drivers[0].timer==60&&!(actors[19].flags&ACTOR_FLAG_HIDDEN),
               "twelve-tick extraction becomes a living visible walking driver, never a dead-state corpse");
        td.onfoot=0;td.heading=7;td.u=430*16;td.v=460*16;td_sandbox_sync();
        expect(td_traffic_u[role]==td.u&&td_traffic_v[role]==td.v&&td_sandbox_heading(role)==7,"owned fleet cache aliases the driven car without a duplicate road body");
        td_traffic_present();expect(actors[role<6?role+2:role+11].flags&ACTOR_FLAG_HIDDEN,"current stolen vehicle is hidden in autonomous fleet presentation");
        td_sandbox_bind(td_traffic_u,td_traffic_v,0);
        expect(td_sandbox_owned(role)&&td_sandbox_captured(role)&&td_sandbox_drivers[0].timer==60,"a same-district shop return retains stolen fleet ownership, driver identity and physical positions");
        td.onfoot=1;td_entry_timer=0;td.u=444*16;td.v=460*16;td.park_u=430*16;td.park_v=460*16;td_sandbox_sync();
        expect(td_sandbox_interact()==2,"either vehicle interaction can reuse the actual abandoned owned car");
        unsigned other=(role+1)%8;td.u=480*16;td.v=460*16;td_traffic_u[other]=490*16;td_traffic_v[other]=460*16;
        expect(td_sandbox_interact()==1&&td_sandbox_owned(other)&&td_sandbox_captured(role),"taking a second NPC car retains the prior captured car as a solid abandoned body");
        td_traffic_present();unsigned old_frame=role<2?role*8+4:role<6?(role-2)*4+2:role==7?18:4;
        expect(!(actors[role<6?role+2:role+11].flags&ACTOR_FLAG_HIDDEN)&&actors[role<6?role+2:role+11].frame==old_frame,
               "the abandoned captured vehicle remains visible at its driven heading rather than snapping to an authored-route pose");
        expect(!td_sandbox_clear(416*16,460*16,430*16,460*16,7,TD_NONE),"an abandoned captured fleet car remains physically collidable while a different car is owned");
    }
    reset_case();sandbox_place_fleet();td.onfoot=1;td.u=400*16;td.v=450*16;
    td_sandbox_parked[0]=(td_parked_t){410*16,450*16,0,1,1,4,1,0};
    expect(td_sandbox_interact()==1&&td_sandbox_owner==8&&td.vehicle==1&&td.heading==4&&!td_sandbox_extracting(),"a vacant parked curb truck is controllable immediately without inventing an occupant");
    td.onfoot=0;td.u=440*16;td.v=460*16;td_sandbox_sync();
    expect(td_sandbox_parked[0].u==td.u&&td_sandbox_parked[0].v==td.v&&td_sandbox_parked[0].protected,"driving a parked car updates its protected real-world identity");
    reset_case();sandbox_place_fleet();td.onfoot=1;td.u=400*16;td.v=450*16;
    td_sandbox_drivers[0].timer=td_sandbox_drivers[1].timer=60;
    td_sandbox_drivers[0].u=390*16;td_sandbox_drivers[0].v=450*16;td_sandbox_drivers[1].u=420*16;td_sandbox_drivers[1].v=450*16;
    td_traffic_u[0]=410*16;td_traffic_v[0]=450*16;td_state_t before=td;
    expect(td_sandbox_interact()==3&&!memcmp(&td,&before,58)&&!td_sandbox_mask,
           "third simultaneous visible extraction reports bounded capacity without replacing humans, taking a fare or changing ownership");
    for(unsigned i=0;i<2;i++){td_sandbox_drivers[i].u=390*16;td_sandbox_drivers[i].v=450*16;td_sandbox_drivers[i].dx=1;td_sandbox_drivers[i].dy=0;}
    td_sandbox_tick();expect(td_sandbox_drivers[0].timer&&td_sandbox_drivers[1].timer,"walking ejected drivers cannot retire while wholly visible");
    td_sandbox_drivers[0].u=40*16;td_sandbox_tick();expect(!td_sandbox_drivers[0].timer&&td_sandbox_drivers[1].timer,
           "driver identity retires only after whole pose and camera margin are offscreen");
    reset_case();sandbox_place_fleet();td_sandbox_parked[0]=(td_parked_t){400*16,450*16,0,0,0,0,1,0};
    draw_scroll_x=320;draw_scroll_y=362;
    td_sandbox_render();expect(sandbox_render_calls==1,"street scene renders one actual resident car through safe native actor renderer");
    test_current_district=TD_DISTRICT_NONE;sandbox_render_calls=0;td_sandbox_render();
    expect(!sandbox_render_calls,"a real interior scene's unregistered district prevents all stale outdoor parked-car sprite rendering");
    reset_case();sandbox_place_fleet();td_traffic_u[0]=400*16;td_traffic_v[0]=450*16;
    for(unsigned half=1;half<=8;half++)for(int offset=-240;offset<=240;offset++)for(int delta=-16;delta<=16;delta++){
        unsigned old=400*16+offset,next=old+delta,r=(half+5)*16;
        unsigned expected;
        if(abs(offset)<(int)r)expected=(offset<0?delta<=0:offset>0?delta>=0:1)&&abs(offset+delta)>abs(offset);
        else expected=(unsigned)(abs(offset)<abs(offset+delta)?abs(offset):abs(offset+delta))>=r;
        expect(td_sandbox_clear(old,450*16,next,450*16,half,TD_NONE)==expected,
               "physical player-car sweep matches independent strict cardinal interval and monotonic overlap-separation oracle");
    }
    expect(!td_sandbox_clear(300*16,450*16,500*16,450*16,7,TD_NONE),"long sweep cannot tunnel through a stationary traffic body");
    td_sandbox_owner=0;expect(td_sandbox_clear(400*16,450*16,401*16,450*16,7,TD_NONE),"the current owned aliased fleet body never collides with itself");
    td_sandbox_owner=TD_NONE;td.onfoot=1;td.u=100*16;td.v=100*16;
    expect(td_sandbox_foot_clear(418*16,450*16),"remote door/alighting occupancy is queried at the endpoint without sweeping from a distant courier");
    expect(!td_sandbox_foot_clear(409*16,450*16),"foot interaction clearance retains the actual10.5px traffic exclusion");
    reset_case();sandbox_place_fleet();td.onfoot=1;td.u=400*16;td.v=450*16;td_traffic_u[0]=410*16;td_traffic_v[0]=450*16;
    joy_pressed=joy=J_B;td_input_edge=1;td_drive();
    expect(td_sandbox_owned(0)&&td_entry_timer&&td_result_b_release&J_B&&td.mode==TD_ROAM,
           "a fresh B near an NPC car steals once through actual native motion callback and consumes transit input");
    reset_case();sandbox_place_fleet();td.onfoot=1;geometry=EAST_WALL;td.u=392*16;td.v=450*16;td_traffic_u[0]=410*16;td_traffic_v[0]=450*16;
    td_state_t door_before=td;expect(td_sandbox_interact()==4&&!memcmp(&td,&door_before,58)&&!td_sandbox_mask,"quick theft cannot animate a courier through a solid building to a nearby vehicle");
    reset_case();sandbox_place_fleet();td_sandbox_drivers[0]=(td_driver_t){405*16,450*16,1,0,60,1,0};
    actors[19].flags=0;td.onfoot=0;td.speed=10;td_sandbox_last_u=390*16;td_sandbox_last_v=450*16;td.u=410*16;td.v=450*16;
    expect(td_sandbox_human_hits()==1&&(td_sandbox_drivers[0].kind&128),"a driven car sweeps an extracted living driver into the same airborne/prone human state");
    expect(!td_sandbox_human_hits(),"one struck extracted identity cannot repeatedly charge or slow the vehicle");
    for(unsigned i=0;i<24;i++){ td_sandbox_tick(); }td_sandbox_present();
    expect(td_sandbox_drivers[0].recover==24&&td_sandbox_drivers[0].u==417*16&&actors[19].frame_start==32+TD_CIVILIAN_PRONE,
           "struck driver moves twelve checked ground pixels and remains prone after a bounded non-graphic arc");
    test_sandbox_entry_and_boat_save();
    /* Rebound composes the real motion, fleet-body sweep and registered
       terrain. Find a genuinely wide Core road with a clear rear escape. */
    reset_case();sandbox_place_fleet();geometry=NATIVE_GRID;unsigned road_u=0,road_v=0;
    for(unsigned y=24;y<920&&!road_u;y++)for(unsigned x=24;x<960;x++)
        if(sandbox_raw_body(0,x,y,7)&&sandbox_raw_body(0,x-6,y,7)&&sandbox_raw_body(0,x+12,y,5)){
            road_u=x;road_v=y;break;
        }
    expect(road_u!=0,"native collision data provides a genuine full-body roadway for a physical rebound");
    td.u=road_u*16;td.v=road_v*16;td.safe_u=td.u;td.safe_v=td.v;td.speed=14;td_vx=224;td.heading=0;
    td_traffic_u[0]=(road_u+12)*16;td_traffic_v[0]=td.v;td.job=0;td.stage=1;td.health=100;
    UWORD rebound_start=td.u;driving_tick(J_A);
    expect(td.u==rebound_start&&td_vx<0&&td.health==88,"actual swept fleet body rejects overlap and reverses the player's momentum once");
    for(unsigned i=0;i<6;i++){
        UWORD before_u=td.u;driving_tick(J_A|J_LEFT);
        expect(td.u<=before_u&&before_u-td.u<=16&&td.heading==0&&sandbox_raw_body(0,td.u/16,td.v/16,7),
               "native-grid rebound moves visibly away with a full terrain-valid car and fixed heading under held inputs");
    }
    expect(td.u<rebound_start&&rebound_start-td.u<=6*16&&td.safe_u==td.u,"real fleet recoil has a bounded visible displacement and valid safe coordinates");
    for(unsigned i=0;i<13;i++)driving_tick(J_A);
    expect(td.u<=rebound_start&&td.health==88,"same held contact cannot tunnel through the real fleet car or repeat cargo damage");
    reset_case();sandbox_place_fleet();geometry=EAST_WALL;td.u=392*16;td.v=450*16;td.safe_u=td.u;td.safe_v=td.v;
    td.heading=8;td.speed=14;td_vx=-224;td_traffic_u[0]=380*16;td_traffic_v[0]=td.v;
    td_traffic_samples[0].u=500*16;td_traffic_samples[0].v=td.v; /* Head-on contact retains strict recoil. */
    driving_tick(J_A);for(unsigned i=0;i<6;i++)driving_tick(J_A);
    expect(td.u>=392*16&&td.u<393*16&&td_vx==0&&td_drivable(td.u/16,td.v/16),
           "full car recoil clips at a real solid tile behind its body instead of moving through the wall");
    /* Bench, fence and pole contacts use actual authored prop data and raw
       registered terrain, then execute the real driver rather than a copy
       of its destruction or rebound formulas. */
    const UBYTE furniture_kind[3]={7,2,1};
    static const signed char approach_x[4]={1,0,-1,0},approach_y[4]={0,1,0,-1};
    for(unsigned kind=0;kind<3;kind++)for(unsigned fast=0;fast<2;fast++){
        reset_case();sandbox_place_fleet();geometry=NATIVE_GRID;unsigned found=0,prop_index=0,px=0,py=0,district=0,direction=0;
        for(unsigned d=0;d<TD_DISTRICT_COUNT&&!found;d++)for(unsigned i=td_prop_offsets[d];i<td_prop_offsets[d+1]&&!found;i++)
            if(td_props[i].kind==furniture_kind[kind])for(unsigned dir=0;dir<4&&!found;dir++){
                unsigned x=td_props[i].x*8+4,y=td_props[i].y*8+4;int clear=x>=32&&x<990&&y>=32&&y<940;
                for(unsigned step=10;clear&&step<=18;step++)if(!sandbox_raw_body(d,x-approach_x[dir]*step,y-approach_y[dir]*step,7))clear=0;
                if(!clear)continue;
                test_current_district=d;td_scenery_reset();
                UWORD u=(x-approach_x[dir]*11)*16,v=(y-approach_y[dir]*11)*16;
                td_scenery_contact(u,v,u+approach_x[dir]*2,v+approach_y[dir]*2,7,14);
                if(td_prop_dead(i)){found=1;prop_index=i-td_prop_offsets[d];px=x;py=y;district=d;direction=dir;}
            }
        expect(found,"each requested bench/fence/pole kind has an actual full-body reachable native approach");
        if(!found)continue;
        reset_case();sandbox_place_fleet();geometry=NATIVE_GRID;test_current_district=td.district=td_streetcar_view_district=td_sandbox_district=district;
        td.u=(px-approach_x[direction]*11)*16;td.v=(py-approach_y[direction]*11)*16;td.heading=direction*4;
        td.safe_u=td.u;td.safe_v=td.v;td.speed=fast?14:2;td_vx=approach_x[direction]*td.speed*16;td_vy=approach_y[direction]*td.speed*16;
        UWORD original_u=td.u,original_v=td.v;driving_tick(fast?J_A:0);
        expect(td.u==original_u&&td.v==original_v&&(td_vx*approach_x[direction]+td_vy*approach_y[direction])<0&&td_vehicle_recoil==6,
               "intact furniture interrupts the accepted physical movement and starts a bounded rebound");
        expect(td_prop_dead(td_prop_offsets[district]+prop_index)==fast,"only the sufficiently fast actual car breaks its contacted furniture identity");
        for(unsigned tick=0;tick<6;tick++){
            driving_tick(J_A|J_RIGHT);
            expect(((int)td.u-original_u)*approach_x[direction]+((int)td.v-original_v)*approach_y[direction]<=0&&
                   abs((int)td.u-original_u)<=6*16&&abs((int)td.v-original_v)<=6*16&&td.heading==direction*4&&sandbox_raw_body(district,td.u/16,td.v/16,7),
                   "furniture rebound remains terrain-valid and cannot be undone by held acceleration or steering");
        }
        if(fast){
            /* A fence can consist of adjacent original cells; break any
               remaining contacted neighbours through its real API too. */
            for(unsigned n=0;n<16;n++)if(td_scenery_contact(original_u,original_v,
                original_u+approach_x[direction]*2,original_v+approach_y[direction]*2,7,14)==TD_SCENERY_CLEAR)break;
            td_motion_reset();td.cooldown=0;td.u=original_u;td.v=original_v;td.speed=2;
            td_vx=approach_x[direction]*32;td_vy=approach_y[direction]*32;
            driving_tick(0);expect(((int)td.u-original_u)*approach_x[direction]+((int)td.v-original_v)*approach_y[direction]>0,
                                  "the destroyed furniture contact becomes physically passable for the actual driver");
        }
    }
}
