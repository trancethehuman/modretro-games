/* Intentional ordinary-driver yielding is separate from byte-only broad phase
 * equivalence. Exercise actual state/admission, not a replacement driver. */
static void courier_yield_fixture(unsigned slot){
    reset_case();td.onfoot=1;td.u=400*16;td.v=450*16;td.park_u=600*16;td.park_v=600*16;
    td_traffic_u[slot]=389*16;td_traffic_v[slot]=450*16;
    td_traffic_samples[slot].u=480*16;td_traffic_samples[slot].v=450*16;td_traffic_advance=128;
    if(slot==2)td_police_waypoint=(td_police_plan_t){480*16,450*16,0,1};
}
static void test_ordinary_courier_yielding(void){
    for(unsigned half=5;half<=8;half++)for(int dx=-208;dx<=208;dx+=8)
    for(int dy=-176;dy<=176;dy+=8)for(int direction=-1;direction<=1;direction+=2){
        reset_case();td.u=6400+dx;td.v=7200+dy;
        int x=6400+direction*128,y=7200,r=(half+3)*16;
        int old_overlap=abs(dx)<r&&abs(dy)<r;
        int left=(x<6400?x:6400)-r,right=(x>6400?x:6400)+r;
        int occupied=(int)td.u>left&&(int)td.u<right&&(int)td.v>y-r&&(int)td.v<y+r;
        expect(td_traffic_courier_clear(6400,7200,x,y,half)==(old_overlap||!occupied),
               "ordinary courier yielding matches independent open swept-body rectangles and retains prior-overlap recovery");
    }
    for(unsigned slot=0;slot<TD_TRAFFIC_SLOTS;slot++){
        courier_yield_fixture(slot);td_state_t saved=td;UWORD start=td_traffic_u[slot];
        td_traffic_motion(1<<slot);
        expect(td_traffic_u[slot]==start&&!td_player_hurt&&!memcmp(&td,&saved,58),
               "each ordinary role including taxi and nonpursuing police yields before newly entering the visible foot courier");
        td.v=480*16;td_traffic_motion(1<<slot);
        expect(td_traffic_u[slot]>start&&!td_player_hurt,
               "queued ordinary traffic resumes continuously on its authored candidate when the courier clears the road");
    }
    courier_yield_fixture(0);td_traffic_u[1]=374*16;td_traffic_v[1]=450*16;
    td_traffic_samples[1].u=480*16;td_traffic_samples[1].v=450*16;
    td_traffic_motion(3);
    expect(td_traffic_u[0]==389*16&&td_traffic_u[1]==374*16,
           "courier crossing creates a physical queue instead of letting a following full truck body pass the stopped car");
    courier_yield_fixture(0);td.u=800*16;td.v=900*16;
    actors[9].flags=0;actors[9].pos.x=400*32;actors[9].pos.y=450*32;
    td_traffic_motion(1);
    expect(td_traffic_u[0]==389*16,"visible normal pedestrian crossing retains the same full-body epoch obstruction as the courier");
    courier_yield_fixture(0);td.seconds=8;td.u=400*16;td.v=176*16;
    td_traffic_u[0]=184*16;td_traffic_v[0]=176*16;td_traffic_samples[0].u=300*16;td_traffic_samples[0].v=176*16;
    td_traffic_motion(1);
    expect(td_traffic_u[0]==184*16,"new foot yielding still leaves the real red-light stopline authoritative");
    courier_yield_fixture(0);test_boat_active=1;UWORD start=td_traffic_u[0];td_traffic_motion(1);
    expect(td_traffic_u[0]>start&&!td_player_hurt,"boat passenger coordinates do not create an invisible surface crossing obstruction");
    courier_yield_fixture(0);td.mode=TD_RIDE;td_streetcar_ride_view=1;start=td_traffic_u[0];td_traffic_motion(1);
    expect(td_traffic_u[0]>start&&!td_player_hurt,"paid TTC passenger is excluded while parked-road-car and ordinary traffic safety remain");
    courier_yield_fixture(2);td.wanted=1;td.wanted_left=30;td.job=0;td.stage=1;td.health=100;
    td_traffic_motion(4);
    expect(td_traffic_u[2]>389*16&&td_player_hurt==72&&td.health==85&&td.onfoot,
           "pursuing police retain actual survivable moving contact and cargo damage rather than granting courier collision immunity");
}
