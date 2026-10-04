#!/usr/bin/env python3
"""Compare the actual update-local terrain cache with its uncached actual-C reference.

Compare the unchanged actual engine with one exact-centre automatic terrain
cache. Retained fixtures use a fresh cache for their isolated private calls;
actual toronto_update calls share only its own six-byte automatic cache.
"""
from pathlib import Path
import difflib
import hashlib
import importlib.util
import json
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / 'games/toronto-dispatch'
HERE = GAME / 'project/build'
ENGINE = GAME / 'project/plugins/toronto-driving/engine'
FIXTURES = ROOT / 'tests/engine'
SOURCE = ENGINE / 'src/states/TORONTO.c'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def cache_changes():
    return [
        ('#include "td_roads.h"', '#include "td_roads.h"\n#include "td_terrain.h"'),
        ('static UBYTE td_drivable(UWORD u,UWORD v){return td_road_body(u,v,5);}',
         '/* Terrain cache queries live in their own banked unit. */'),
        ('static void td_drive(void){', 'static void td_drive(td_terrain_cache_t *cache){'),
        ('if(td_drivable(u,v)){', 'if(td_terrain_drivable(u,v,cache)){'),
        ('td_drivable(nu>>4,td.v>>4)', 'td_terrain_drivable(nu>>4,td.v>>4,cache)'),
        ('td_drivable(td.u>>4,nv>>4)', 'td_terrain_drivable(td.u>>4,nv>>4,cache)'),
        ('void toronto_update(void) BANKED {\n',
         'void toronto_update(void) BANKED {\n    td_terrain_cache_t terrain;\n'),
        ('    for(step=0;step<motion&&', '    terrain.valid=0;\n    for(step=0;step<motion&&'),
        ('            td_drive();if(!was_entering', '            td_drive(&terrain);if(!was_entering'),
    ]


def candidate_source(original):
    text = original
    for before, after in cache_changes():
        assert text.count(before) == 1, before
        text = text.replace(before, after)
    return text


def baseline_source(candidate):
    text = candidate
    for before, after in reversed(cache_changes()):
        assert text.count(after) == 1, after
        text = text.replace(after, before)
    return text


ADAPTER = '''
/* Host-only isolated private-call compatibility; no production NULL path. */
static void td_test_drive(void){td_terrain_cache_t cache;cache.valid=0;td_drive(&cache);}
static UBYTE td_test_drivable(UWORD u,UWORD v){td_terrain_cache_t cache;cache.valid=0;return td_terrain_drivable(u,v,&cache);}
#define td_drive() td_test_drive()
#define td_drivable(u,v) td_test_drivable(u,v)
'''

# Pointer-free actual state is emitted field-by-field. These keys include all
# mutable included-module state, hardware-adapter state and committed SRAM.
FIELDS = '''
td td_job td_offer td_target td_cursor td_session_live td_route_district
td_transition_pending td_transition_district td_traffic_u td_traffic_v
td_traffic_samples td_traffic_leg td_tick td_notice_timer td_red_cooldown
td_turn_tick td_entry_timer td_entry_target td_walk_dir td_resume_mode
td_board_route td_vx td_vy td_last_frame td_corner_used td_contact_episode
td_traffic_retreat_mask td_vehicle_contact_mask td_traffic_advance
td_traffic_elapsed td_police_elapsed td_police_advance td_police_waypoint
td_police_from_u td_police_from_v td_police_stuck td_input_edge td_result_b_release
td_people td_nearby_routes td_ped_route td_ped_refresh td_ped_anchor_u
td_ped_anchor_v td_people_last_u td_people_last_v td_streetcar_focus_u
td_streetcar_focus_v td_streetcar_view_district td_streetcar_ride_view
td_streetcar_display td_streetcar_elapsed td_streetcar_bound td_streetcar_valid
td_streetcar_was_ride td_streetcar_cue td_aircraft td_save_slot td_save_seq
tile_hit_x tile_hit_y actors_len camera_x camera_y image_width image_height
sys_time camera_settings joy joy_pressed camera_offset_x camera_offset_y
camera_deadzone_x camera_deadzone_y td_test_sram stop_reads ui_draws audio_updates
audio_inits audio_impacts audio_mode audio_active audio_cue audio_braking
stop0_here authored_content test_current_district test_queued_district
test_queue_fail test_queue_calls test_reset_calls test_map_opens test_map_updates
test_map_closes test_map_active test_map_buttons test_map_pressed
test_map_camera_settings test_map_camera_x test_map_camera_y geometry
sram_writes sram_interrupt_after sram_interrupt_enabled sram_offsets sram_values
proto_current_bank proto_query_result
'''.split()

TRACER = r'''
static FILE *proto_state,*proto_counts;
static UBYTE proto_query_result;
static unsigned proto_case,proto_step,proto_enabled;
static void proto_item(const char *name,const void *data,size_t length){
    uint32_t n=(uint32_t)strlen(name),l=(uint32_t)length;
    fwrite(&n,4,1,proto_state);fwrite(name,1,n,proto_state);
    fwrite(&l,4,1,proto_state);fwrite(data,1,length,proto_state);
}
#define PROTO_ITEM(x) proto_item(#x,&(x),sizeof(x))
static int proto_actor_index(const actor_t *a){
    if(!a)return -1;
    for(unsigned i=0;i<21;i++)if(a==&actors[i])return (int)i;
    fprintf(stderr,"Unexpected actor pointer at %u/%u\n",proto_case,proto_step);abort();
}
static int proto_resource_index(const void *p){
    if(!p)return 0;
    if(p==&test_player_sprite)return 1;
    if(p==&test_tram_sprite)return 2;
    fprintf(stderr,"Unexpected resource pointer at %u/%u\n",proto_case,proto_step);abort();
}
static void proto_snapshot(void){
    PROTO_ITEM(proto_case);PROTO_ITEM(proto_step);
    /*FIELDS*/
    for(unsigned i=0;i<21;i++){
        actor_t *a=&actors[i];
        proto_item("actor.pos",&a->pos,sizeof(a->pos));
        int previous=proto_actor_index(a->prev),next=proto_actor_index(a->next);
        PROTO_ITEM(previous);PROTO_ITEM(next);
        PROTO_ITEM(a->flags);PROTO_ITEM(a->collision_group);PROTO_ITEM(a->anim_tick);
        PROTO_ITEM(a->frame);PROTO_ITEM(a->frame_start);PROTO_ITEM(a->frame_end);
        PROTO_ITEM(a->script.bank);PROTO_ITEM(a->script_update.bank);
        PROTO_ITEM(a->hscript_update);PROTO_ITEM(a->hscript_hit);
        PROTO_ITEM(a->base_tile);PROTO_ITEM(a->sprite.bank);PROTO_ITEM(a->bounds);
        /* Compare resource identity, never ASLR addresses. The retained
           scene fixtures have two independently identified stock sheets. */
        int script=proto_resource_index(a->script.ptr),update=proto_resource_index(a->script_update.ptr),sprite=proto_resource_index(a->sprite.ptr);
        PROTO_ITEM(script);PROTO_ITEM(update);PROTO_ITEM(sprite);
    }
    int inactive=proto_actor_index(actors_inactive_head),active=proto_actor_index(test_actors_active_head);
    PROTO_ITEM(inactive);PROTO_ITEM(active);
    for(unsigned i=0;i<TD_DISTRICT_COUNT;i++){
        PROTO_ITEM(test_native_scenes[i].width);PROTO_ITEM(test_native_scenes[i].height);
        PROTO_ITEM(test_native_scenes[i].collisions.bank);
    }
    uint32_t end=0;fwrite(&end,4,1,proto_state);
}
void toronto_update(void){
    unsigned long old_tiles=proto_tile_reads,old_ranges=proto_range_reads,old_bodies=proto_body_calls;
    td_proto_original_update();
    if(proto_enabled){
        proto_snapshot();
        fprintf(proto_counts,"%u,%u,%lu,%lu,%lu\n",proto_case,proto_step,
                proto_tile_reads-old_tiles,proto_range_reads-old_ranges,proto_body_calls-old_bodies);
        proto_step++;
    }
}
static void proto_setup(UBYTE district,UWORD u,UWORD v,UBYTE vehicle,UBYTE foot,
                        WORD speed,WORD vx,WORD vy,UBYTE heading,int ground){
    reset_case();geometry=ground;authored_content=1;
    test_current_district=td.district=td.park_district=district;
    td.u=td.safe_u=u;td.v=td.safe_v=v;td.park_u=96*16;td.park_v=96*16;
    td.vehicle=vehicle;td.onfoot=foot;td.heading=heading;
    td_session_live=1;toronto_init();
    td.speed=speed;td_vx=vx;td_vy=vy;
    tile_hit_x=71;tile_hit_y=93;proto_current_bank=13;proto_query_result=0;
}
static void proto_stream(unsigned id){
    proto_case=id;proto_step=0;
    proto_snapshot();
    for(unsigned i=0;i<24;i++){
        UBYTE held=i<6?0:i<12?J_A:i<16?(J_A|J_RIGHT):i<20?J_B:J_A|J_LEFT;
        tile_hit_x=(UBYTE)(17+i);tile_hit_y=(UBYTE)(91-i);
        world_tick(held,1+i%4);
    }
}
static void proto_queries(unsigned id,int ground,UBYTE district){
    static const UWORD points[][2]={
        {400,450},{401,451},{402,452},{403,453},{404,454},{407,455},
        {408,456},{409,457},{7,8},{8,7},{8,8},{1016,968},
        {1017,968},{1016,969},{65535,450},{400,65535}};
    proto_setup(district,400*16,450*16,0,0,0,0,0,0,ground);
    proto_case=id;proto_step=0;
#ifdef PROTO_CANDIDATE
    td_terrain_cache_t cache;cache.valid=0;
    expect(sizeof(cache)==6,"automatic terrain cache has six bytes and no persistent state");
#endif
    for(unsigned p=0;p<sizeof(points)/sizeof(points[0]);p++)for(unsigned repeat=0;repeat<2;repeat++){
        UWORD u=points[p][0],v=points[p][1];
        UBYTE expected=u>=8&&v>=8&&u<=1016&&v<=968;
        if(expected)for(unsigned y=(v-5)/8;y<=(v+5)/8;y++)
            for(unsigned x=(u-5)/8;x<=(u+5)/8;x++)if(district_tile(district,x,y))expected=0;
        unsigned long old_tiles=proto_tile_reads,old_ranges=proto_range_reads,old_bodies=proto_body_calls;
        tile_hit_x=(UBYTE)(200-p);tile_hit_y=(UBYTE)(190-repeat);proto_current_bank=19;
#ifdef PROTO_CANDIDATE
        proto_query_result=td_terrain_drivable(u,v,&cache);
#else
        proto_query_result=td_drivable(u,v);
#endif
        expect(proto_query_result==expected,"actual cached helper matches independent full body and invalid-coordinate oracle");
        expect(tile_hit_x==200-p&&tile_hit_y==190-repeat&&proto_current_bank==19,
               "hit, miss, true, false and invalid helper queries preserve incoming hit globals and modeled code bank");
        proto_snapshot();
        fprintf(proto_counts,"%u,%u,%lu,%lu,%lu\n",proto_case,proto_step,
                proto_tile_reads-old_tiles,proto_range_reads-old_ranges,proto_body_calls-old_bodies);
        proto_step++;
    }
}
static int proto_scenarios(void){
    unsigned id=0;
    for(UBYTE vehicle=0;vehicle<4;vehicle++)for(UBYTE elapsed=1;elapsed<=4;elapsed++){
        proto_setup(0,560*16+15,720*16+7,vehicle,0,0,0,0,0,NATIVE_GRID);
        proto_case=id++;proto_step=0;
        /* First update isolates catch-up reuse, with no traffic quantum due. */
        world_tick(0,elapsed);
        expect(tile_hit_x==71&&tile_hit_y==93,"update-local cache preserves incoming native tile-hit markers");
    }
    static const UWORD points[][2]={
        {560,720},{193,350},{394,394},{395,395},{396,396},{399,399},
        {400,400},{401,401},{402,402},{403,403},{404,404},{407,407},
        {8,8},{7,8},{8,7},{1016,968},{1017,968},{1016,969},
        {65535,450},{400,65535},{993,256},{752,736},{808,152},{676,536}};
    for(unsigned p=0;p<sizeof(points)/sizeof(points[0]);p++)for(UBYTE fraction=0;fraction<2;fraction++){
        UWORD u=(UWORD)(points[p][0]*16+(fraction?15:0));
        UWORD v=(UWORD)(points[p][1]*16+(fraction?7:0));
        int ground=p%4==0?NATIVE_GRID:p%4==1?EAST_WALL:p%4==2?SOUTH_CURB:SOUTHWEST_CORNER;
        UBYTE district=p==1?4:p==20?2:0;
        proto_setup(district,u,v,p%4,0,p%3==0?0:p%3==1?12:-6,
                    p%3==0?0:p%3==1?128:-70,p%2?96:0,(p*3)&15,ground);
        if(p%5==0){td.job=6;td.stage=1;td.left=120;td_get_job(6,&td_job);td_set_target();}
        proto_stream(id++);
    }
    /* Same coordinates, different map on the next update: no stale hit. */
    proto_setup(0,400*16,450*16,0,0,0,0,0,0,CLEAR_GROUND);
    proto_case=id++;proto_step=0;world_tick(0,4);
    geometry=EAST_WALL;world_tick(0,4);
    geometry=CLEAR_GROUND;world_tick(0,4);
    int query_ground[]={CLEAR_GROUND,EAST_WALL,SOUTH_CURB,NATIVE_GRID};
    UBYTE query_district[]={0,4,5};
    for(unsigned g=0;g<4;g++)for(unsigned d=0;d<3;d++)proto_queries(id++,query_ground[g],query_district[d]);
    /* Every public walking/entry/menu/scene/tram/police path is retained. */
    void (*cases[])(void)={test_clock_boundaries,test_bounded_corner_assist,
        test_entry_collision,test_result_b_release,test_result_release_initialization_and_save,
        test_reciprocal_portals,test_queue_failure_and_remote_boot,test_car_entry_at_portal,
        test_atlas_driver_handoff_and_freeze,test_cross_district_streetcar,
        test_queen_visual_queue_retry_clock,test_streetcar_contacts_and_future_traffic_yield,
        test_contact_episode_and_invalid,test_human_impacts_and_police,
        test_visible_human_reverse_start,test_island_objective_guidance};
    for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
        proto_case=id++;proto_step=0;cases[i]();
    }
    return (int)id;
}
int main(int argc,char **argv){
    if(argc!=3)return 2;
    if(proto_retained_main())return 1;
    unsigned retained=checks;
    proto_state=fopen(argv[1],"wb");proto_counts=fopen(argv[2],"w");
    if(!proto_state||!proto_counts)return 2;
    proto_enabled=1;int cases=proto_scenarios();
    fclose(proto_state);fclose(proto_counts);
    printf("Terrain candidate differential fixtures: %d cases, %u additional retained checks, %u failures.\n",cases,checks-retained,failures);
    return failures?1:0;
}
'''


def module(path):
    spec = importlib.util.spec_from_file_location('terrain_engine_fixture', path)
    obj = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(obj)
    return obj


def host_source(toronto,candidate):
    sources = ['td_transit.c','td_world.c','td_streetcar.c','td_streetcar_runtime.c',
               'td_aircraft.c','td_people.c','td_traffic.c','td_roads.c']
    if candidate:sources.append('td_terrain.c')
    text = '\n'.join((ENGINE/'src'/name).read_text() for name in sources)
    text += '\n' + toronto + '\n' + (ENGINE/'src/td_save.c').read_text()
    text += '\n' + (ENGINE/'src/td_routes.c').read_text()
    text = text.replace('void toronto_update(void) BANKED {','void td_proto_original_update(void) BANKED {')
    old = 'UBYTE td_road_body(UWORD u,UWORD v,UBYTE half) BANKED {'
    assert text.count(old)==1
    text = text.replace(old, old+'\n    proto_body_calls++;')
    text = 'static unsigned long proto_body_calls;\n'+text
    def sram(m):
        return f"({m.group('cast')})(td_test_sram+{int(m.group('address'),16)-0xA000})"
    text, n = re.subn(r'\((?P<cast>\s*volatile\s+UBYTE\s*\*\s*)\)\s*(?P<address>0x[AaBb][0-9A-Fa-f]{3})\b',sram,text)
    assert n
    text, n = re.subn(r'\bram\[([^\]]+)\]\s*=(?!=)\s*([^;]+);',r'td_host_sram_store(&ram[\1],(UBYTE)(\2));',text)
    assert n
    return text


def host_harness(candidate):
    text=(FIXTURES/'runtime_harness.c').read_text()
    assert text.count(ADAPTER)==1, 'Review isolated private-call host adapter'
    text=text.replace(ADAPTER,'')
    if candidate:
        text='#define PROTO_CANDIDATE 1\n'+text
    if candidate:
        text=text.replace('#include "engine_under_test.c"','#include "engine_under_test.c"\n'+ADAPTER,1)
    text=text.replace('int main(void) {','int proto_retained_main(void) {',1)
    old='UBYTE tile_at(UBYTE x,UBYTE y) {return district_tile(test_current_district,x,y);}'
    assert text.count(old)==1
    text=text.replace(old,'static unsigned long proto_tile_reads,proto_range_reads;\nstatic UBYTE proto_current_bank=13;\nUBYTE tile_at(UBYTE x,UBYTE y) {proto_tile_reads++;return district_tile(test_current_district,x,y);}')
    for old in ('UBYTE tile_col_test_range_x(UBYTE mask,UBYTE row,UBYTE first,UBYTE last){',
                'UBYTE tile_col_test_range_y(UBYTE mask,UBYTE column,UBYTE first,UBYTE last){'):
        assert text.count(old)==1
        text=text.replace(old,old+'UBYTE saved_bank=proto_current_bank;proto_range_reads++;proto_current_bank=test_current_district+1;')
    text=text.replace('if(tile&mask)return tile;','if(tile&mask){proto_current_bank=saved_bank;return tile;}')
    text=text.replace('    return 0;\n}\n\nvoid actor_set_frames','    proto_current_bank=saved_bank;return 0;\n}\n\nvoid actor_set_frames')
    text=text.replace('    return 0;\n}\nUBYTE tile_col_test_range_y','    proto_current_bank=saved_bank;return 0;\n}\nUBYTE tile_col_test_range_y')
    tracer=TRACER.replace('    /*FIELDS*/','\n'.join('    PROTO_ITEM('+x+');' for x in FIELDS))
    return text+'\n'+tracer


def read_trace(path):
    with path.open('rb') as stream:
        record=[]
        while word:=stream.read(4):
            size=struct.unpack('=I',word)[0]
            if not size:
                yield record
                record=[]
                continue
            name=stream.read(size).decode()
            size=struct.unpack('=I',stream.read(4))[0]
            record.append((name,stream.read(size)))
        assert not record


def main():
    compiler=shutil.which('clang') or shutil.which('cc')
    assert compiler,'Host compiler unavailable'
    candidate=SOURCE.read_text()
    original=baseline_source(candidate)
    assert candidate_source(original)==candidate
    HERE.mkdir(exist_ok=True)
    (HERE/'TORONTO-update-terrain-banked.candidate.c').write_text(candidate)
    diff=''.join(difflib.unified_diff(original.splitlines(True),candidate.splitlines(True),
        fromfile='a/games/toronto-dispatch/project/plugins/toronto-driving/engine/src/states/TORONTO.c',
        tofile='b/games/toronto-dispatch/project/plugins/toronto-driving/engine/src/states/TORONTO.c'))
    (HERE/'update-terrain-banked.candidate.diff').write_text(diff)
    source_paths=[p for p in ENGINE.rglob('*') if p.is_file() and p.suffix in ('.c','.h')]
    before={str(p.relative_to(ROOT)):sha(p.read_bytes()) for p in source_paths}
    fixture=module(ROOT/'scripts/test_engine.py')
    native=fixture.native_fixture(GAME,ENGINE/'include')
    stats={}
    with tempfile.TemporaryDirectory(prefix='ignored-terrain-differential-') as temp:
        work=Path(temp)
        shutil.copyfile(ENGINE/'src/td_content.c',work/'content_under_test.c')
        (work/'native_collision_fixture.h').write_text(native)
        shutil.copyfile(FIXTURES/'gbvm_stubs.h',work/'gbvm_stubs.h')
        for name in ('actor','camera','scroll','collision','input','data_manager','ui','compat','system','bankdata','gbs_types'):
            (work/f'{name}.h').write_text('#include "gbvm_stubs.h"\n')
        (work/'gbdk').mkdir()
        (work/'gbdk/platform.h').write_text('#include "gbvm_stubs.h"\n')
        for label,text in [('baseline',original),('candidate',candidate)]:
            (work/'engine_under_test.c').write_text(host_source(text,label=='candidate'))
            (work/'harness.c').write_text(host_harness(label=='candidate'))
            binary=work/label
            command=[compiler,'-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
                     '-Wno-unknown-pragmas','-Wno-parentheses','-fsanitize=address,undefined',
                     '-I',str(work),'-I',str(ENGINE/'include'),'-I',str(FIXTURES),
                     str(work/'harness.c'),str(ENGINE/'src/td_police.c'),
                     str(ENGINE/'src/td_traffic_signal_stop.c'),'-o',str(binary)]
            subprocess.run(command,check=True)
            output=subprocess.check_output([str(binary),str(work/f'{label}.state'),str(work/f'{label}.counts')],text=True)
            stats[label]={'output':output.strip(),'hostStateSha256':sha((work/f'{label}.state').read_bytes())}
            counts=[list(map(int,line.split(','))) for line in (work/f'{label}.counts').read_text().splitlines()]
            stats[label]['updates']=len(counts)
            stats[label]['reads']={'tiles':sum(x[2] for x in counts),'ranges':sum(x[3] for x in counts),'bodies':sum(x[4] for x in counts)}
            stats[label]['initialCatchup']=counts[:16]
            stats[label]['counts']=counts
            print(label+': '+output.strip(),flush=True)
        comparisons=records=0
        from itertools import zip_longest
        for record_no,(a,b) in enumerate(zip_longest(read_trace(work/'baseline.state'),read_trace(work/'candidate.state'))):
            assert a is not None and b is not None, f'Trace length mismatch at record{record_no}'
            assert len(a)==len(b)
            for av,bv in zip(a,b):
                assert av==bv,f'State mismatch record{record_no}, field {av[0]}: {av[1].hex()} vs {bv[1].hex()}'
                comparisons+=1
            records+=1
        for a,b in zip(stats['baseline']['counts'],stats['candidate']['counts']):
            assert a[:2]==b[:2]
            assert all(b[i]<=a[i] for i in range(2,5)),f'Candidate performs extra reads at {a[:2]}'
        assert stats['candidate']['reads']['tiles']<stats['baseline']['reads']['tiles']
    assert before=={str(p.relative_to(ROOT)):sha(p.read_bytes()) for p in source_paths},'Tracked production changed during differential checks'
    for label in stats:stats[label].pop('counts')
    report={'scope':'Actual-C host differential only; no native fit, CPU or cartridge claim',
        'originalTorontoSha256':sha(original.encode()),'candidateTorontoSha256':sha(candidate.encode()),
        'candidateDiffSha256':sha(diff.encode()),
        'bankedHelperSha256':sha((ENGINE/'src/td_terrain.c').read_bytes()),
        'cacheHeaderSha256':sha((ENGINE/'include/td_terrain.h').read_bytes()),'prototypeScriptSha256':sha(Path(__file__).read_bytes()),
        'sourceFilesUnchanged':True,'automaticCacheBytes':6,'persistentBytesAdded':0,
        'evidenceType':'host differential; bank capacity, native timing and stack require separate artifact checks',
        'comparedSnapshots':records,'comparedFieldPayloads':comparisons,'results':stats,
        'qualification':['Exact whole-pixel centre only, including false results; no equal-tile-span key claim.',
         'Retained isolated private calls use fresh local cache; real toronto_update owns shared lifetime.',
         'ROM collision is invariant within one update; successful portals return before reuse.',
         'Actual candidate helper is BANKED; host calls cannot measure its additional far-call cost.',
         'Host hardware adapters do not prove SDCC ABI, actual bank restoration, performance or maximum stack.']}
    (HERE/'update-terrain-banked.host-result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('comparedSnapshots','comparedFieldPayloads','sourceFilesUnchanged')},indent=2))
    print(json.dumps({label:value['reads'] for label,value in stats.items()},indent=2))


if __name__=='__main__':main()
