/* Production main/menu/story/combat/save integration on the host. Input and
 * SRAM are hardware adapters; these checks do not establish native timing,
 * rendering, physical persistence, or cartridge execution. Legacy records use
 * the existing independent fixed-offset historical encoder unchanged. */
static void story_fixture_completions(UBYTE count){
    memset(td.complete,0,sizeof(td.complete));td.done=count;
    for(unsigned i=0;i<count;i++)td.complete[i>>3]|=1<<(i&7);
}

static void test_story_combat_save_v11(void){
    expect(sizeof(td)==58&&offsetof(td_state_t,wanted)==52&&
           offsetof(td_state_t,wanted_left)==53&&offsetof(td_state_t,vitality)==54&&
           offsetof(td_state_t,ammo)==55&&offsetof(td_state_t,district)==56,
           "v11 keeps the exact58-byte record and explicitly repacks former word offsets");
    for(unsigned hp=0;hp<=100;hp++)for(unsigned ammo=0;ammo<=24;ammo++){
        native_case();td.vitality=hp;td.ammo=ammo;td.onfoot=1;
        td.cash=1234;td.seconds=81;td.subsecond=59;td.job=0;td.stage=1;td.left=93;td.health=67;
        td.wanted=2;td.wanted_left=17;td.complete[13]=(hp+ammo)&255;td_state_t expected=td;
        td_save();volatile UBYTE *record=td_save_address(td_save_slot);
        expect(record[2]==11&&record[3]==58&&record[8+52]==2&&record[8+53]==17&&
               record[8+54]==hp&&record[8+55]==ammo&&record[8+39]==expected.complete[13],
               "every valid vitality/ammo pair has exact v11 disk offsets independent of field decoding");
        memset(&td,0,sizeof(td));unsigned stores=sram_writes;
        expect(td_restore()&&!memcmp(&td,&expected,58)&&sram_writes==stores,
               "all2525 health/ammo pairs retain independent cargo, story, active quest, heat and earnings on restore");
    }
    for(unsigned flags=0;flags<256;flags++){
        native_case();story_fixture_completions(104);td.complete[13]=flags;td_state_t expected=td;td_save();
        memset(&td,0,sizeof(td));
        expect(td_restore()&&td.done==104&&!memcmp(&td,&expected,58),
               "all256 story flag combinations are independent of the104 quest completion count");
    }
    for(unsigned fault=0;fault<6;fault++){
        native_case();td.cash=111;td_save();td_state_t stable=td;td.cash=222;td_save();
        volatile UBYTE *record=td_save_address(td_save_slot);
        switch(fault){
            case 0:record[8+54]=101;break;case 1:record[8+55]=25;break;
            case 2:record[8+40]=1;break;case 3:record[8+41]=128;break;
            case 4:record[8+24]=1;break;case 5:record[8+52]=4;record[8+53]=17;break;
        }
        refresh_record_crc(record);memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&stable,58),
               "CRC-valid invalid vitality, ammo, unused story bytes, quest count and heat select the prior committed state");
    }
}

static void test_story_combat_legacy_layout(void){
    for(UBYTE version=8;version<=10;version++){
        native_case();story_fixture_completions(version<10?95:104);
        td.job=version<10?7:103;td_get_job(td.job,&td_job);td.stage=1;td.left=93;td.health=67;
        td.cash=2417;td.seconds=65535;td.subsecond=59;td.wanted=3;td.wanted_left=30;
        td.vitality=3;td.ammo=24;td_state_t expected=td;expected.vitality=100;expected.ammo=12;
        island_disk_record(0,version,&td,37);memset(&td,0,sizeof(td));unsigned stores=sram_writes;
        expect(td_restore()&&!memcmp(&td,&expected,58)&&sram_writes==stores&&td_save_address(0)[2]==version,
               "independent v8-v10 historical words hydrate health100/ammo12 and preserve complete original progress without rewriting");
        for(unsigned offset=53;offset<=55;offset+=2)for(unsigned value=1;value<256;value++){
            native_case();td_state_t old=expected;island_disk_record(0,version,&old,37);
            volatile UBYTE *record=td_save_address(0);record[8+offset]=value;refresh_record_crc(record);
            memset(&td,0,sizeof(td));td.cash=999;td_state_t untouched=td;stores=sram_writes;
            expect(!td_restore()&&!memcmp(&td,&untouched,58)&&sram_writes==stores,
                   "every nonzero high byte in either historical v8-v10 heat word is rejected before field repacking");
        }
        native_case();td_state_t old=td;old.complete[13]=1;island_disk_record(0,version,&old,37);
        memset(&td,0,sizeof(td));
        expect(!td_restore(),"story flags cannot launder forbidden spare completion bits from any historical v8-v10 record");
    }
}

static void test_story_combat_controller_integration(void){
    static const UBYTE thresholds[8]={0,1,8,16,32,56,80,104};
    static const UBYTE pages[8]={6,4,5,5,5,5,5,6};
    unsigned total_pages=0;
    for(UBYTE scene=0;scene<8;scene++){
        native_case();story_fixture_completions(thresholds[scene]);td.complete[13]=(1<<scene)-1;
        td.mode=scene?TD_RESULT:TD_HELP;td.menu=0;td_resume_mode=TD_ROAM;
        td_aircraft.wait=123;td_player_delay=8;td_patrol_delay=17;td_immunity=12;
        td_state_t before=td;td_aircraft_state_t plane=td_aircraft;
        UWORD fleet_u[8],fleet_v[8];memcpy(fleet_u,td_traffic_u,sizeof(fleet_u));memcpy(fleet_v,td_traffic_v,sizeof(fleet_v));
        td_person_t people[TD_PEOPLE_COUNT];memcpy(people,td_people,sizeof(people));
        unsigned stores=sram_writes;world_tick(J_A,600);
        expect(td.mode==TD_DIALOG&&td.complete[13]==before.complete[13]&&td.done==thresholds[scene]&&sram_writes==stores,
               "actual Welcome/result A triggers the correct eligible unseen chapter without accepting work or saving it early");
        UBYTE first_page=td.menu;td_state_t frozen=td;world_tick(J_A,600);
        expect(!memcmp(&td,&frozen,58)&&sram_writes==stores,
               "holding the Welcome/result A edge cannot immediately advance or dismiss the triggered dialogue");
        for(unsigned wait=0;wait<4;wait++)world_tick(0,600);
        expect(!memcmp(&td,&frozen,58)&&!memcmp(&td_aircraft,&plane,sizeof(plane))&&
               !memcmp(fleet_u,td_traffic_u,sizeof(fleet_u))&&!memcmp(fleet_v,td_traffic_v,sizeof(fleet_v))&&
               !memcmp(people,td_people,sizeof(people))&&td_player_delay==8&&td_patrol_delay==17&&td_immunity==12&&sram_writes==stores,
               "paused dialogue freezes every saved field, fleet, humans, aircraft and combat timers across40 seconds");
        for(UBYTE page=1;page<pages[scene];page++){
            world_tick(0,0);world_tick(J_A,600);total_pages++;
            expect(td.mode==TD_DIALOG&&td.menu==first_page+page&&td.done==thresholds[scene]&&td.complete[13]==before.complete[13]&&sram_writes==stores,
                   "each fresh A advances exactly one actual chapter page while retaining world time and uncommitted story flags");
            UBYTE current=td.menu;world_tick(J_A,600);
            expect(td.mode==TD_DIALOG&&td.menu==current&&sram_writes==stores,
                   "a held page-advance button never repeats or saves the chapter");
        }
        world_tick(0,0);world_tick(J_A,600);total_pages++;
        expect(td.mode==(scene?TD_BOARD:TD_ROAM)&&td.complete[13]==((1<<(scene+1))-1)&&
               td.done==thresholds[scene]&&td.job==TD_NONE&&td.cash==before.cash&&td.seconds==before.seconds&&td.subsecond==before.subsecond&&sram_writes==stores+67,
               "final A commits only the finished story bit once, returns intro to streets or later chapters to offers, and keeps time/wallet/quests");
        /* The controller commits before selecting the next transient offer;
           the historical menu byte therefore contains the last story page. */
        td_state_t expected=td;expected.mode=TD_ROAM;expected.menu=first_page+pages[scene]-1;
        UWORD close_u=td.u,close_v=td.v;world_tick(J_A,4);
        expect(td.mode==(scene?TD_BOARD:TD_ROAM)&&td.job==TD_NONE&&td.u==close_u&&td.v==close_v&&td.speed==0&&sram_writes==stores+67,
               "holding final A cannot accept another job or accelerate the borrowed car after the dialogue closes");
        memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&expected,58),
               "each actual chapter completion restores its committed seen bit with all original world fields");
    }
    expect(total_pages==41,"Welcome/result controller visits all41 original pages across eight actual thresholds");
    for(UBYTE skip=0;skip<2;skip++){
        native_case();td.mode=TD_HELP;td_resume_mode=TD_ROAM;world_tick(J_A,1);world_tick(0,0);
        unsigned stores=sram_writes;world_tick(skip?J_START:J_B,600);
        expect(td.mode==TD_ROAM&&td.complete[13]==1&&td.done==0&&td.job==TD_NONE&&sram_writes==stores+67,
               "B or Start skips only the current introduction and commits it once through actual menu input");
    }
    native_case();td.job=0;td.stage=1;td.left=93;td.health=67;td_get_job(0,&td_job);
    td.mode=TD_HELP;td_resume_mode=TD_ROAM;td_state_t active=td;world_tick(J_A,600);
    active.mode=TD_ROAM;expect(!memcmp(&td,&active,58)&&td_story_due()==0,
               "Welcome cannot interrupt or replace an existing active quest even when intro is unseen");
    for(UBYTE mode=TD_WAIT;mode<=TD_RIDE;mode++){
        reset_case();td.mode=mode;td.onfoot=1;td_state_t booked=td;
        expect(!td_story_maybe_begin()&&!memcmp(&td,&booked,58),
               "queued story cannot interrupt either an unpaid transit wait or a paid ride");
    }
    for(UBYTE scene=1;scene<8;scene++){
        native_case();story_fixture_completions(thresholds[scene]-1);td.complete[13]=(1<<scene)-1;
        td.mode=TD_RESULT;world_tick(J_A,600);
        expect(td.mode==TD_BOARD&&td.job==TD_NONE&&td.complete[13]==((1<<scene)-1)&&sram_writes==0,
               "the production result menu cannot trigger a later chapter before its genuine completion threshold");
    }
    native_case();td.complete[13]=1;td.job=0;td_get_job(0,&td_job);td.stage=td_job.count-1;td.left=99;
    td_finish(TRUE);expect(td.done==1&&td.mode==TD_RESULT&&td.job==TD_NONE,"genuine completed delivery reaches the first-pay story threshold");
    world_tick(J_A,1);expect(td.mode==TD_DIALOG&&td.menu==6&&td.complete[13]==1,
                           "an actual successful delivery result triggers the first-pay scene through the production menu");
}

static void test_story_combat_downed_quest_guards(void){
    native_case();td.onfoot=1;td.vitality=0;unsigned downed_stores=sram_writes;world_tick(J_SELECT,1);
    expect(td.mode==TD_ROAM&&td.job==TD_NONE&&sram_writes==downed_stores&&td.msg==24,
           "an actual Select input cannot open dispatch or accept a quest for a zero-health courier");
    for(UBYTE injury=0;injury<2;injury++){
        native_case();td.onfoot=1;td.job=0;td_get_job(0,&td_job);td.stage=td_job.count-1;td.left=93;
        td_target.u=td.u>>4;td_target.v=td.v>>4;td_target.district=td.district;
        td.vitality=injury?100:0;if(injury)td_combat_damage(1,0,0);
        unsigned stores=sram_writes;UBYTE stage=td.stage;UWORD cash=td.cash;
        td_interact();expect(td.job==0&&td.stage==stage&&td.mode==TD_ROAM&&td.cash==cash&&td.done==0&&sram_writes==stores&&td.msg==24,
                             "zero-health and fresh-hit locked couriers cannot complete a quest or collect its payout");
        td.job=TD_NONE;td.mode=TD_BOARD;td.menu=0;td_get_job(0,&td_offer);world_tick(J_A,1);
        expect(td.mode==TD_BOARD&&td.job==TD_NONE&&td.cash==cash&&sram_writes==stores,
               "zero-health and hit-locked couriers cannot accept a fresh delivery from the actual job board");
        for(UBYTE menu=3;menu<=5;menu++){
            td.mode=TD_PAUSE;td_resume_mode=TD_ROAM;td.menu=menu;td_state_t before=td;world_tick(0,0);world_tick(J_A,1);
            expect(td.mode==TD_PAUSE&&td.onfoot==before.onfoot&&td.vehicle==before.vehicle&&td.u==before.u&&td.v==before.v&&
                   td.park_u==before.park_u&&td.park_v==before.park_v&&td.cash==before.cash&&!td_entry_timer&&sram_writes==stores,
                   "downed or hit-locked pause actions cannot enter a car, switch vehicles or start a TTC journey");
        }
    }
    reset_case();td.onfoot=1;td.vitality=15;td.park_u=600*16;td.park_v=600*16;td.cash=99;
    td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=93;
    expect(td_motion_player_hit(td.u-160,td.v,td.u+160,td.v,5)&&td.vitality==0&&td_player_hurt==72&&td_combat_locked(),
           "an actual lethal moving-vehicle hit creates independent health downing alongside existing knockback");
    UBYTE recovered=td_combat_update(120);
    expect((recovered&TD_COMBAT_RECOVERED)&&td.vitality==100&&td.ammo==12&&td.cash==59&&td.job==TD_NONE&&td.mode==TD_RESULT&&
           !td_player_hurt&&!td_player_push_x&&!td_player_push_y&&td.speed==0&&td_vx==0&&td_vy==0,
           "real recovery clears old vehicle knockback, fails carried work once, restores health/ammo and charges the fee once");
}

static void test_hospital_transition_and_arrest(void){
    /* Native-grid geometry is loaded from every original scene, independently
       of the combat candidate implementation or its bounded test adapter. */
    static const UWORD hu[9]={504,520,488,504,504,520,488,520,488};
    static const UWORD hv[9]={344,344,344,360,328,360,360,328,328};
    for(UBYTE candidate=0;candidate<9;candidate++){
        native_case();UBYTE clear=TRUE;
        for(UWORD v=hv[candidate]-3;v<=hv[candidate]+3;v++)
            for(UWORD u=hu[candidate]-3;u<=hu[candidate]+3;u++)
                if(!td_district_walkable(0,u,v))clear=FALSE;
        expect(clear==(hu[candidate]!=520),"hospital exit candidates match all49 original-grid foot-body pixels; eastern facade is blocked");
    }
    for(UBYTE from=0;from<TD_DISTRICT_COUNT;from++)for(UBYTE active=0;active<2;active++){
        native_case();td_session_live=1;td.district=test_current_district=from;td.onfoot=1;
        td.vehicle=2;td.cash=99;td.wanted=3;td.wanted_left=30;td.ammo=1;
        if(active){td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=93;td.health=73;}
        td_streetcar_runtime_prepare(0);test_refresh_scene();
        UWORD park_u=td.park_u,park_v=td.park_v;UBYTE park_district=td.park_district;
        expect(td_combat_damage(100,0,0)==(TD_COMBAT_CHANGED|TD_COMBAT_DOWN),
               "all seven genuine loaded districts admit a lethal health event on foot");
        unsigned stores=sram_writes;world_tick(J_RIGHT|J_A|J_B,120);
        expect(td.district==0&&td.onfoot&&td.u==504*16&&td.v==344*16&&td.safe_u==td.u&&td.safe_v==td.v&&
               td.vitality==100&&td.ammo==12&&td.cash==59&&!td.wanted&&td.msg==25&&td.speed==0&&
               td.mode==(active?TD_RESULT:TD_ROAM)&&td.job==TD_NONE&&sram_writes==stores+67,
               "actual TORONTO recovery places one fully healed courier at the hospital, fails carried work, charges40 and commits once before held controls");
        expect(td.park_u==park_u&&td.park_v==park_v&&td.park_district==park_district&&td.vehicle==2,
               "hospital admission from every district preserves the exact separately parked owned vehicle");
        expect((from?td_transition_pending==1&&test_queued_district==0:!td_transition_pending),
               "remote hospital arrival queues the actual Core scene while local arrival needs no scene allocation");
        td_state_t expected=td;expected.mode=TD_ROAM;
        memset(&td,0,sizeof(td));
        expect(td_restore()&&!memcmp(&td,&expected,58),
               "the hospital record genuinely restores final Core coordinates, notice, fee, independent health/ammo and preserved parked car");
    }
    for(UBYTE active=0;active<2;active++){
        native_case();td_session_live=1;td.district=test_current_district=3;td.onfoot=1;td.cash=100;
        if(active){td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=93;}
        td_streetcar_runtime_prepare(0);test_refresh_scene();test_queue_fail=1;
        expect(td_combat_damage(100,0,0)==3,"remote retry test begins with a real lethal hit");
        world_tick(0,120);td_state_t committed=td;unsigned stores=sram_writes;
        expect(td_transition_pending==2&&test_queued_district==TD_DISTRICT_NONE&&td.district==0&&td.cash==60&&stores==67,
               "allocation failure retains the single committed hospital arrival and marks a frozen Core retry");
        UWORD fleet_u[8],fleet_v[8];memcpy(fleet_u,td_traffic_u,sizeof(fleet_u));memcpy(fleet_v,td_traffic_v,sizeof(fleet_v));
        for(unsigned retry=0;retry<4;retry++)world_tick(J_A|J_B|J_RIGHT|J_SELECT,600);
        expect(!memcmp(&td,&committed,58)&&!memcmp(fleet_u,td_traffic_u,sizeof(fleet_u))&&
               !memcmp(fleet_v,td_traffic_v,sizeof(fleet_v))&&sram_writes==stores&&td_transition_pending==2,
               "a failed hospital scene retry freezes complete serialized state, health, deadlines, fleet and fees without wrong-scene collisions");
        test_queue_fail=0;world_tick(0,600);
        expect(td_transition_pending==1&&test_queued_district==0&&!memcmp(&td,&committed,58)&&sram_writes==stores,
               "a later available scene allocation retries Core once without charging or failing the job again");
        test_current_district=0;test_queued_district=TD_DISTRICT_NONE;toronto_init();
        expect(!td_transition_pending&&td.district==0&&td.u==committed.u&&td.v==committed.v&&td.msg==25&&
               td.mode==committed.mode&&td.vitality==100&&td.ammo==12&&td.cash==60&&sram_writes==stores,
               "genuine new-scene initialization completes hospital arrival and retains its result/notice without another save");
    }
    native_case();td.onfoot=1;td.cash=99;td.wanted=3;td.wanted_left=30;
    expect(td_combat_damage(100,0,0)==3,"downed capture test creates a real independent-health death");
    td_traffic_u[2]=td.u;td_traffic_v[2]=td.v;unsigned stores=sram_writes;
    td_traffic_contacts();expect(td.cash==99&&td.wanted==3&&sram_writes==stores,
                                "a patrol touching a downed courier cannot arrest or add a second episode fine");
    native_case();td.onfoot=1;td.cash=100;td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=1;td.health=73;
    expect(td_combat_damage(100,0,0)==3,"deadline overlap begins with actual health downing");
    stores=sram_writes;world_tick(0,60);
    expect(!td.vitality&&td.job==0&&!td.health&&!td.left&&td.mode==TD_ROAM&&td.cash==100&&
           td_combat_locked()&&sram_writes==stores,
           "a deadline expiring while downed marks cargo failure without opening a modal or repeatedly saving the injured state");
    world_tick(0,60);
    expect(td.vitality==100&&td.job==TD_NONE&&td.mode==TD_RESULT&&td.msg==25&&td.cash==60&&sram_writes==stores+67,
           "a simultaneous deadline failure still reaches hospital autonomously at120 active VBlanks and fails/saves once");
    native_case();td.onfoot=1;td.cash=100;td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=93;
    td.health=td.vitality=15;td.park_u=600*16;td.park_v=720*16;
    expect(td_motion_player_hit(td.u-160,td.v,td.u+160,td.v,5)&&!td.vitality&&!td.health&&
           td.mode==TD_ROAM&&td.job==0&&td_player_hurt==72&&sram_writes==67,
           "one lethal vehicle hit simultaneously destroying the parcel retains autonomous downed streets and saves the injury once");
    stores=sram_writes;world_tick(0,120);
    expect(td.vitality==100&&td.job==TD_NONE&&td.mode==TD_RESULT&&td.msg==25&&td.cash==60&&
           !td_player_hurt&&sram_writes==stores+67,
           "a fatal vehicle/cargo overlap clears old knockback and reaches hospital/result without requiring any menu dismissal");
    native_case();td.onfoot=1;td.cash=100;td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=93;
    td.health=td.vitality=15;td.park_u=600*16;td.park_v=720*16;
    expect(td_district_drivable(td.park_district,600,720),"cold fatal-impact fixture retains a genuinely valid original-grid parked vehicle");
    expect(td_motion_player_hit(td.u-160,td.v,td.u+160,td.v,5),"cold reset test persists a genuine fatal impact");
    td_session_live=0;toronto_init();
    expect(!td.vitality&&td.job==0&&td.mode==TD_HELP&&td_combat_locked()&&td.cash==100,
           "genuine cold boot restores a fatal cargo/health record as a new hospital episode behind Welcome");
    world_tick(J_B,1);stores=sram_writes;world_tick(0,120);
    expect(td.vitality==100&&td.job==TD_NONE&&td.mode==TD_RESULT&&td.cash==60&&td.msg==25&&sram_writes==stores+67,
           "resuming the genuine restored downed courier reaches hospital once with its original failed quest and capped fee");
    for(UBYTE foot=0;foot<2;foot++)for(UBYTE cash=0;cash<2;cash++){
        reset_case();td.onfoot=foot;td.cash=cash?100:12;td.wanted=1;td.wanted_left=30;
        td_traffic_u[2]=td.u;td_traffic_v[2]=td.v;
        UWORD u=td.u,v=td.v;td_traffic_contacts();
        expect(td.cash==(cash?75:0)&&!td.wanted&&td.msg==20&&td_combat_locked()&&td.vitality==100&&td.ammo==12,
               "a real low-attention arrest clamps its25 fine, clears pursuit and holds foot/car controls without weapon damage");
        if(foot)expect(PLAYER.frame==32&&PLAYER.frame_end==33,"arrest replaces an armed or hit sheet with the ordinary foot pose");
        stores=sram_writes;world_tick(J_RIGHT|J_A|J_B,59);
        expect(td.u==u&&td.v==v&&td_combat_locked()&&sram_writes==stores,
               "held movement, interaction and firing remain blocked through59 active arrest VBlanks");
        td.mode=TD_PAUSE;td_resume_mode=TD_ROAM;world_tick(0,600);
        expect(td.u==u&&td.v==v&&td_combat_locked(),"pausing freezes the short arrest hold along with street simulation");
        td.mode=TD_ROAM;world_tick(0,1);
        expect(!td_combat_locked()&&td.vitality==100&&td.cash==(cash?75:0),
               "the sixtieth active arrest VBlank releases normal controls without another fine or health loss");
    }
}

static void test_pending_hospital_save_bounds_and_journal(void){
    native_case();td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=93;td.health=73;
    td_state_t candidate=td;
    for(UBYTE version=4;version<=11;version++)for(UBYTE mode=0;mode<=TD_HELP+1;mode++)
    for(UBYTE foot=0;foot<3;foot++)for(unsigned hp=0;hp<=102;hp++)for(UBYTE failed=0;failed<3;failed++){
        candidate.mode=mode;candidate.onfoot=foot;candidate.vitality=hp==102?255:hp;
        candidate.left=failed?0:93;candidate.health=failed==1?73:0;
        expect(td_valid_fields(&candidate,version)==(version==11&&mode==TD_ROAM&&foot==1&&hp==0),
               "only v11/ROAM/on-foot/zero-vitality admits failed active work across all historical versions, modes, foot flags and health values");
    }
    candidate=td;candidate.onfoot=1;candidate.vitality=candidate.health=0;
    candidate.stage=td_job.count;
    expect(!td_valid_fields(&candidate,11),"pending hospital cannot relax the active job's valid stage bound");
    candidate.stage=1;candidate.job=TD_QUESTS;
    expect(!td_valid_fields(&candidate,11),"pending hospital cannot relax valid job identity");

    /* Cut every actual byte store, keeping the last committed independent
       snapshot. Then boot through the real scene/menu/recovery path. */
    for(UBYTE phase=0;phase<2;phase++){
        native_case();td.onfoot=1;td.cash=100;td.job=0;td_get_job(0,&td_job);td.stage=1;td.left=93;
        td.health=td.vitality=15;td.park_u=600*16;td.park_v=720*16;td.complete[13]=255;
        if(phase){
            td_motion_player_hit(td.u-160,td.v,td.u+160,td.v,5);
            expect(!td.vitality&&!td.health&&td.job==0&&td.mode==TD_ROAM,
                   "hospital journal starts from an actual committed fatal vehicle/cargo record");
        }else td_save();
        td_state_t stable=td;UBYTE stable_slot=td_save_slot,stable_seq=td_save_seq;
        UBYTE stable_image[sizeof(td_test_sram)];memcpy(stable_image,td_test_sram,sizeof(stable_image));
        sram_writes=0;
        if(phase)world_tick(0,120);
        else td_motion_player_hit(td.u-160,td.v,td.u+160,td.v,5);
        td_state_t committed=td;committed.mode=TD_ROAM;unsigned count=sram_writes;
        expect(count==67,"both injury and hospital episodes use exactly one full58-byte journal commit");
        for(volatile unsigned cut=1;cut<=count;cut++){
            memcpy(td_test_sram,stable_image,sizeof(stable_image));td=stable;
            td_save_slot=stable_slot;td_save_seq=stable_seq;td_motion_reset();td_combat_reset();
            if(phase)td_combat_resume_downed();
            td_transition_pending=0;td_streetcar_runtime_prepare(0);joy=joy_pressed=0;sys_time=td_last_frame=0;
            sram_writes=0;sram_interrupt_after=cut;sram_interrupt_enabled=1;
            if(setjmp(interrupted_save)==0){
                if(phase)world_tick(0,120);else td_motion_player_hit(td.u-160,td.v,td.u+160,td.v,5);
                expect(0,"configured injury/hospital interruption must reach its real SRAM store");
            }
            sram_interrupt_enabled=0;memset(&td,0,sizeof(td));
            const td_state_t *expected=cut==count?&committed:&stable;
            expect(td_restore()&&!memcmp(&td,expected,58),
                   "every interrupted injured/hospital record restores the entire previous snapshot until the single final commit byte");
            td_session_live=0;actors_inactive_head=NULL;toronto_init();
            UBYTE downed=!expected->vitality;UWORD balance=expected->cash;
            expect(td.mode==TD_HELP&&td.vitality==expected->vitality&&td.cash==balance&&td_combat_locked()==downed,
                   "genuine boot resumes only a committed pending-death episode; fully committed hospital records do not become another death");
            world_tick(J_B,1);unsigned stores=sram_writes;world_tick(0,120);
            if(downed){
                expect(td.vitality==100&&td.ammo==12&&td.district==0&&td.u==504*16&&td.v==344*16&&
                       td.job==TD_NONE&&td.mode==TD_RESULT&&td.cash==(balance>40?balance-40:0)&&sram_writes==stores+67,
                       "an interrupted death/hospital write autonomously recovers the retained pending courier and commits the40 fee exactly once");
            }else expect(td.cash==balance&&td.vitality==expected->vitality,
                         "a retained healthy or already hospitalized boot cannot charge another medical fee");
            UWORD final_cash=td.cash;world_tick(0,120);
            expect(td.cash==final_cash,"continued post-boot play cannot repeat the same interrupted hospital episode fee");
        }
    }
}

static void test_story_combat_save_integration(void){
    test_story_combat_save_v11();test_story_combat_legacy_layout();
    test_story_combat_controller_integration();test_story_combat_downed_quest_guards();test_hospital_transition_and_arrest();
    test_pending_hospital_save_bounds_and_journal();
}
