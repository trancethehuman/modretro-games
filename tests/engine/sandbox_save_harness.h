/* Save logic from the production unit, with the separately tested boat API's
 * validated-shore projection supplied by a narrow hardware adapter. */
static void test_sandbox_entry_and_boat_save(void){
    reset_case();geometry=NATIVE_GRID;authored_content=1;
    td.msg=21;expect(td_valid_fields(&td,TD_SAVE_VERSION),"the bounded street-capacity notice is a valid current save field");
    for(unsigned notice=TD_NOTICE_MAX+1;notice<256;notice++){
        td.msg=notice;
        expect(!td_valid_fields(&td,TD_SAVE_VERSION),"a CRC-valid forged notice cannot index beyond the actual UI table");
    }
    reset_case();geometry=NATIVE_GRID;authored_content=1;td.u=560*16;td.v=720*16;
    td.park_u=td.u;td.park_v=td.v;td.safe_u=td.u;td.safe_v=td.v;
    td.onfoot=1;td.u-=6*16;td_entry_timer=12;td_entry_target=0;
    td.job=0;td.stage=1;td.left=100;td.health=81;td.cash=123;td_get_job(0,&td_job);
    td_state_t live=td;td_save();
    expect(!memcmp(&td,&live,sizeof(td)),"entry save commit restores every live field without interrupting the human animation");
    td_entry_timer=0;memset(&td,0,sizeof(td));
    expect(td_restore()&&!td.onfoot&&td.u==560*16&&td.v==720*16&&td.park_u==td.u&&td.park_v==td.v,
           "reset during entry resumes inside the actual parked vehicle rather than a trapped overlapping human");
    expect(td.job==0&&td.stage==1&&td.left==100&&td.health==81&&td.cash==123,
           "entry save normalization preserves active pickup/deadline/condition/wallet");
    reset_case();geometry=NATIVE_GRID;authored_content=1;
    td.u=560*16;td.v=840*16;td.safe_u=td.u;td.safe_v=td.v;td.onfoot=1;
    td.park_u=560*16;td.park_v=720*16;td.job=0;td.stage=1;td.left=93;td.health=76;td.cash=109;td_get_job(0,&td_job);
    test_boat_shore_active=1;test_boat_shore_u=560*16;test_boat_shore_v=800*16;
    live=td;td_save();
    expect(!memcmp(&td,&live,sizeof(td)),"boat save restores live water position and keeps the boat controlled during play");
    test_boat_shore_active=0;memset(&td,0,sizeof(td));
    expect(td_restore()&&td.onfoot&&td.u==560*16&&td.v==800*16&&td.safe_u==td.u&&td.safe_v==td.v,
           "reset aboard a transient launch returns to its actual validated boarding shore");
    expect(td.park_u==560*16&&td.park_v==720*16&&td.park_district==0&&td.job==0&&td.stage==1&&td.left==93&&td.health==76&&td.cash==109,
           "boat reset preserves the independent road car and complete mission/payment state");
}
