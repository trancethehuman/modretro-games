/* Actual renderer and generated atlas, with bounded host hardware adapters.
 * The API's native pixel data is independently verified by test_atlas.py.
 * Access to static routines sets coverage fixtures; production is unchanged. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui_host.h"
#include "td_game.h"
#include "td_atlas.h"
#include "td_combat.h"
#include "td_menu_hint.h"
#include "atlas_under_test.c"
#include "guidance_under_test.c"
#include "ui_under_test.c"
#include "ui_content_oracle.h"

td_state_t td;
td_job_t td_job,td_offer;
td_stop_t td_target,td_cursor;
UBYTE td_route_district;
UBYTE td_resume_mode;
UBYTE td_board_route;
UBYTE td_ui_actual_near_stop(UBYTE id);
UWORD td_streetcar_focus_u,td_streetcar_focus_v;
UBYTE td_streetcar_view_district,td_streetcar_ride_view;
actor_t actors[22];
UBYTE actors_len;
static UBYTE boat_control_fixture;
UBYTE td_boats_controlled(void){return boat_control_fixture;}
static UBYTE combat_locked_fixture,near_car_fixture;
UBYTE td_entry_timer;
UBYTE td_combat_locked(void){return combat_locked_fixture;}
UBYTE td_motion_near_car(void){return near_car_fixture;}
UWORD camera_x,camera_y;
UBYTE camera_settings,VBK_REG,text_drawn;

/* One window tilemap and two pattern banks, indexed through the public tile
 * API. Native LCDC address mapping and scanline effects are not simulated. */
static UBYTE window_tiles[2][18][20],vram[2][256][16];
static UBYTE window_x,window_y;
static unsigned checks,failures,window_writes,tile_uploads,ground_uploads;
static UBYTE ground_upload_indices[TD_ATLAS_VISIBLE_LIMIT];
static unsigned light_resets;
static UBYTE audio_fixture_mode;
void td_traffic_lights_reset(void){light_resets++;}
static UBYTE initial_font[49][16];
static void expected_centre(UBYTE district,UWORD u,UWORD v,UBYTE *x,UBYTE *y);

static void expect(int condition,const char *name) {
    checks++;
    if(!condition){failures++;if(failures<30||getenv("TD_UI_DIAGNOSTICS"))fprintf(stderr,"FAIL %s\n",name);}
}

void set_win_tiles(UBYTE x,UBYTE y,UBYTE width,UBYTE height,const UBYTE *tiles) {
    window_writes++;
    expect(VBK_REG<2&&tiles&&width&&height&&(unsigned)x+width<=20&&(unsigned)y+height<=18,
           "every native window upload stays inside its actual twenty-by-eighteen map and selected bank");
    if(VBK_REG>=2||!tiles||!width||!height||(unsigned)x+width>20||(unsigned)y+height>18)return;
    for(unsigned py=0;py<height;py++)for(unsigned px=0;px<width;px++)
        window_tiles[VBK_REG][y+py][x+px]=tiles[py*width+px];
}

void set_bkg_data(UBYTE first,UBYTE count,const UBYTE *tiles) {
    tile_uploads++;
    expect(VBK_REG==1&&tiles&&count&&(unsigned)first+count<=256,
           "all UI and atlas pattern uploads use bounded CGB bank1 tile indices");
    int marker=first>=8&&(unsigned)first+count<=15;
    int ground=first>=16&&(unsigned)first+count<=188;
    int font=first>=192&&(unsigned)first+count<=241;
    int guidance=first==241&&count==12;
    int frame=first==188&&count==3;
    expect(marker||ground||font||guidance||frame,"uploads stay within marker, atlas, original three-tile frame gap, font and twelve reserved original guidance/accent tiles");
    if(VBK_REG!=1||!tiles||!count||(unsigned)first+count>256)return;
    if(ground){
        for(unsigned i=0;i<count;i++)ground_upload_indices[(ground_uploads+i)%TD_ATLAS_VISIBLE_LIMIT]=first+i;
        ground_uploads+=count;
    }
    memcpy(vram[VBK_REG][first],tiles,(size_t)count*16);
}

void ui_set_pos(UBYTE x,UBYTE y) {window_x=x;window_y=y;}
UBYTE td_audio_get_mode(void) {return audio_fixture_mode;}
UBYTE td_service(UBYTE origin) {(void)origin;return 1;}
UBYTE td_next_departure(UBYTE origin,UWORD seconds) {(void)origin;(void)seconds;return 7;}

typedef struct {
    td_state_t state;td_job_t job,offer;td_stop_t target,cursor;
    UBYTE route,actor_count,resume_mode;
    UWORD streetcar_u,streetcar_v;
    UBYTE streetcar_district,streetcar_ride;
} game_snapshot_t;

static game_snapshot_t snapshot_game(void) {
    game_snapshot_t snapshot={td,td_job,td_offer,td_target,td_cursor,td_route_district,actors_len,td_resume_mode,
        td_streetcar_focus_u,td_streetcar_focus_v,td_streetcar_view_district,td_streetcar_ride_view};
    return snapshot;
}

static void expect_game_unchanged(const game_snapshot_t *snapshot) {
    expect(!memcmp(&td,&snapshot->state,sizeof(td))&&!memcmp(&td_job,&snapshot->job,sizeof(td_job))&&
           !memcmp(&td_offer,&snapshot->offer,sizeof(td_offer))&&!memcmp(&td_target,&snapshot->target,sizeof(td_target))&&
           !memcmp(&td_cursor,&snapshot->cursor,sizeof(td_cursor))&&td_route_district==snapshot->route&&
           actors_len==snapshot->actor_count&&td_resume_mode==snapshot->resume_mode&&
           td_streetcar_focus_u==snapshot->streetcar_u&&td_streetcar_focus_v==snapshot->streetcar_v&&
           td_streetcar_view_district==snapshot->streetcar_district&&td_streetcar_ride_view==snapshot->streetcar_ride,
           "real map UI preserves serialized state, job, target, cursor, route cue, actor count and paid-view cache");
}

static void reset_case(void) {
    memset(&td,0,sizeof(td));memset(&td_job,0,sizeof(td_job));memset(&td_offer,0,sizeof(td_offer));
    memset(&td_target,0,sizeof(td_target));memset(&td_cursor,0,sizeof(td_cursor));
    memset(actors,0,sizeof(actors));actors[1].flags=ACTOR_FLAG_HIDDEN;memset(window_tiles,0xEE,sizeof(window_tiles));memset(vram,0xEE,sizeof(vram));
    td.district=0;td.u=560*16;td.v=720*16;td.onfoot=1;td.vitality=100;td.ammo=12;
    td.park_district=1;td.park_u=400*16;td.park_v=528*16;td.cash=123;td.seconds=4321;
    td.left=199;td.health=100;td.job=84;td.stage=3;td.wanted=2;td.wanted_left=21;td.mode=TD_PAUSE;
    td_target.district=3;td_target.u=320;td_target.v=144;td_target.reserved=TD_STOP_FOOT;strcpy(td_target.name,"WITHROW PARK");
    td_job.count=5;td_job.route[3]=36;td_job.seconds=199;td_job.reward=130;
    td_route_district=0;td_resume_mode=TD_ROAM;actors_len=TD_ACTORS;
    for(unsigned i=0;i<22;i++){actors[i].flags=0x80|(i&1?ACTOR_FLAG_HIDDEN:0);actors[i].pos.x=1000+i;actors[i].pos.y=2000+i;}
    camera_x=0x3210;camera_y=0x4560;camera_settings=0x2D;VBK_REG=0;text_drawn=0;
    window_x=window_y=0;window_writes=tile_uploads=ground_uploads=0;
    audio_fixture_mode=TD_AUDIO_FULL;combat_locked_fixture=0;near_car_fixture=1;td_entry_timer=0;
    td_streetcar_focus_u=td_streetcar_focus_v=0;td_streetcar_view_district=td_streetcar_ride_view=0;
    td_ui_init();memcpy(initial_font,vram[1]+192,sizeof(initial_font));
}

static void open_case(void) {
    td.mode=TD_MAP;td_map_open();td_ui_draw();
    expect(td_map_active&&camera_settings==0&&window_x==0&&window_y==0,
           "real map open freezes camera and uses a fullscreen window");
}

static void finish_paint(void) {
    unsigned steps=0;
    while(td_map_row<12&&steps++<12)td_map_update(0,0);
    expect(td_map_row==12&&!td_map_error,"each real atlas viewport completes within twelve row updates without an error");
}

static UBYTE expected_marker(unsigned atlas_tile_x,unsigned atlas_tile_y) {
    UWORD x,y;UBYTE bits=0;
    if(td_atlas_position(td.district,td.u/16,td.v/16,&x,&y)&&x/8==atlas_tile_x&&y/8==atlas_tile_y)bits|=1;
    UBYTE district=td.onfoot?td.park_district:td.district;
    UWORD u=td.onfoot?td.park_u/16:td.u/16,v=td.onfoot?td.park_v/16:td.v/16;
    if(td_atlas_position(district,u,v,&x,&y)&&x/8==atlas_tile_x&&y/8==atlas_tile_y)bits|=2;
    const td_stop_t *destination=td.job==TD_NONE&&(td_resume_mode==TD_WAIT||td_resume_mode==TD_RIDE)?&td_cursor:&td_target;
    if(td_atlas_position(destination->district,destination->u,destination->v,&x,&y)&&x/8==atlas_tile_x&&y/8==atlas_tile_y)bits|=4;
    return bits;
}

static UBYTE decoded_pixel(const UBYTE *tile,unsigned x,unsigned y) {
    unsigned bit=7-x;
    return ((tile[y*2]>>bit)&1)|(((tile[y*2+1]>>bit)&1)<<1);
}

/* The previous unpacked table is an independent logical reference. Keep its
 * original insertion/probe order: a different slot or upload order would
 * change native tile bytes even when the rendered pattern looks alike. */
static void verify_original_dictionary(void){
    UWORD cache[TD_ATLAS_VISIBLE_LIMIT],patterns[20];unsigned count=0;
    for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)cache[i]=65535;
    for(unsigned y=0;y<12;y++){
        expect(td_atlas_row(td_map_x,td_map_y+y,20,patterns),"logical cache reference reads an unchanged native atlas row");
        for(unsigned x=0;x<20;x++){
            if(expected_marker(td_map_x+x,td_map_y+y))continue;
            unsigned slot=patterns[x]%TD_ATLAS_VISIBLE_LIMIT,stride=1+((patterns[x]&15)<<1),probes;
            for(probes=0;probes<TD_ATLAS_VISIBLE_LIMIT;probes++){
                if(cache[slot]==patterns[x]||cache[slot]==65535)break;
                slot+=stride;if(slot>=TD_ATLAS_VISIBLE_LIMIT)slot-=TD_ATLAS_VISIBLE_LIMIT;
            }
            expect(probes<TD_ATLAS_VISIBLE_LIMIT,"original unpacked reference fits each genuine viewport");
            if(probes==TD_ATLAS_VISIBLE_LIMIT)return;
            if(cache[slot]==65535){
                cache[slot]=patterns[x];
                expect(ground_upload_indices[(ground_uploads-td_map_count+count)%TD_ATLAS_VISIBLE_LIMIT]==16+slot,
                    "packed atlas preserves the original ordered sequence of native VRAM upload slots");
                count++;
            }
            expect(window_tiles[0][2+y][x]==16+slot,
                "packed atlas preserves each exact window tile byte from the original sparse hash/probe sequence");
        }
    }
    expect(count==td_map_count,"packed and original logical atlas cache insert the same number of dictionary IDs");
    for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)expect(td_map_cache_get(i)==cache[i],
        "every packed atlas slot exactly matches the original unpacked logical dictionary including vacant slots");
}

static void verify_viewport(void) {
    UWORD patterns[20];UBYTE expected[16];
    expect(td_map_count<=TD_ATLAS_VISIBLE_LIMIT,"real renderer cache fits the reserved native pattern count");
    unsigned occupied=0;
    for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)if(td_map_cache_get(i)!=65535) {
        occupied++;
        expect(td_map_cache_get(i)<TD_ATLAS_PATTERNS,"every occupied sparse cache slot contains a valid actual dictionary ID");
        for(unsigned j=0;j<i;j++)if(td_map_cache_get(j)!=65535)
            expect(td_map_cache_get(i)!=td_map_cache_get(j),"real viewport cache assigns one tile slot per unique dictionary pattern");
    }
    expect(occupied==td_map_count,"renderer unique count exactly matches all occupied sparse table slots");
    verify_original_dictionary();
    for(unsigned y=0;y<12;y++) {
        expect(td_atlas_row(td_map_x,td_map_y+y,20,patterns),"actual atlas API supplies the rendered viewport row");
        for(unsigned x=0;x<20;x++) {
            UBYTE marker=expected_marker(td_map_x+x,td_map_y+y),tile=window_tiles[0][2+y][x];
            expect(window_tiles[1][2+y][x]==15,"rendered ground retains UI palette7 and CGB tile bank1 attributes");
            if(marker){expect(tile==7+marker,"marker tile bitset preserves current player, parked car and remote objective overlap");continue;}
            expect(tile>=16&&tile<188&&td_map_cache_get(tile-16)!=65535,"completed ground cells refer to bounded occupied atlas cache slots");
            if(tile<16||tile>=188||td_map_cache_get(tile-16)==65535)continue;
            expect(td_map_cache_get(tile-16)==patterns[x],"ground cache slot preserves the actual atlas dictionary ID");
            expect(td_atlas_pattern(patterns[x],expected),"actual atlas dictionary supplies ground wire bytes");
            expect(!memcmp(vram[1][tile],expected,16),"ground VRAM tile matches its actual native atlas pattern bytes");
            for(unsigned py=0;py<8;py++)for(unsigned px=0;px<8;px++)
                expect(decoded_pixel(vram[1][tile],px,py)==decoded_pixel(expected,px,py),
                       "every decoded ground VRAM pixel matches the actual atlas API wire pattern");
        }
    }
    for(unsigned y=0;y<18;y++)if(y<2||y>=14)for(unsigned x=0;x<20;x++)
        expect(window_tiles[0][y][x]>=192&&window_tiles[0][y][x]<=240,
               "map titles, legend and controls use only reserved native font tiles");
    expect(!memcmp(initial_font,vram[1]+192,sizeof(initial_font)),"all viewport uploads preserve the original font patterns");
    for(unsigned tile=0;tile<256;tile++)for(unsigned byte=0;byte<16;byte++)
        expect(vram[0][tile][byte]==0xEE,"real map leaves the gameplay background pattern bank untouched");
}

static void verify_marker_patterns(void) {
    for(UBYTE bits=1;bits<8;bits++) {
        char label=bits&4?'O':bits&1?'P':'C';const char *found=strchr(td_chars,label);
        expect(found!=NULL,"marker label has an original font glyph");
        if(!found)continue;
        const UBYTE *font=td_font+(found-td_chars)*16;
        expect(!memcmp(vram[1][7+bits],font,14),"marker retains the main original P/C/O glyph above its overlap ticks");
        UBYTE ticks=(bits&1?128:0)|(bits&2?24:0)|(bits&4?1:0);
        expect(vram[1][7+bits][14]==ticks&&vram[1][7+bits][15]==ticks,
               "marker bottom ticks independently identify all three overlapping roles");
    }
}

static void read_window_text(unsigned y,char out[21]) {
    for(unsigned x=0;x<20;x++) {
        UBYTE tile=window_tiles[0][y][x];
        out[x]=tile>=192&&tile<=240?td_chars[tile-192]:tile==252?'>':tile==251?'-':
            tile>=241&&tile<=250?(char)(tile-240):'?';
    }
    out[20]=0;
}

/* Independent row-layout oracle: paused panels have eighteen-column cards,
 * while world HUD, map, chapter labels and contextual footers stay twenty.
 * This table specifies the design; it never calls the renderer's helper. */
static unsigned expected_frame(unsigned row){
    static const UBYTE cards[7][18]={
        {2,1,1,4,1,1,1,1,1,1,1,1,1,1,3,0,0,0}, /* Pause */
        {2,1,1,4,1,1,1,1,1,1,1,1,3,0,0,0,0,0}, /* Settings */
        {2,1,1,4,1,1,1,1,1,1,1,1,1,1,1,3,0,0}, /* Welcome */
        {2,1,1,1,1,1,1,1,1,3,2,1,1,1,1,1,3,0}, /* Controls */
        {2,0,0,2,1,1,1,1,1,1,2,1,1,1,3,0,0,0}, /* Dispatch */
        {2,1,1,4,1,1,1,1,1,1,1,1,1,1,3,0,0,0}, /* TTC */
        {2,1,1,4,1,1,1,1,1,1,1,1,3,0,0,0,0,0}, /* Result */
    };
    if(row>=18)return 0;
    if(td.mode==TD_PAUSE)return cards[td.menu>=TD_SETTINGS_SOUND?1:0][row];
    if(td.mode==TD_HELP)return cards[td.menu==TD_SETTINGS_CONTROLS?3:2][row];
    if(td.mode==TD_BOARD)return cards[4][row];
    if(td.mode==TD_TRANSIT)return cards[5][row];
    if(td.mode==TD_RESULT)return cards[6][row];
    return 0;
}

static void expect_window_text(unsigned row,const char *text,const char *name) {
    UBYTE expected[20];size_t length=strlen(text);unsigned frame=expected_frame(row);
    unsigned limit=frame?18:20,start=frame?1:0;
    expect(row<18&&length<=limit,"every complete caption fits its native card interior or twenty-column footer/HUD");
    if(length>limit)fprintf(stderr,"Caption overflow: mode=%u menu=%u row=%u width=%u text=%s\n",td.mode,td.menu,row,limit,text);
    if(row>=18||length>limit)return;
    memset(expected,frame>1?189:192,sizeof(expected));
    if(frame){expected[0]=expected[19]=(frame==1||frame==4)?190:188;}
    if(frame>1)start=1+(18-length)/2;
    for(unsigned i=0;i<length;i++)expected[start+i]=td_glyph(text[i]);
    expect(!memcmp(window_tiles[0][row],expected,sizeof(expected)),name);
}

static void expect_text_screen_safe(void) {
    UBYTE safe=1;
    for(unsigned row=0;row<18;row++)for(unsigned column=0;column<20;column++)
        if(window_tiles[0][row][column]<188||window_tiles[0][row][column]>252||
           (window_tiles[1][row][column]&~96)!=15||
           ((window_tiles[1][row][column]&96)&&
            (window_tiles[0][row][column]<188||window_tiles[0][row][column]>190)))safe=0;
    expect(safe&&window_x==0&&window_y==0&&VBK_REG==0,
           "dispatch/result cards use only bounded original border/font tiles, upright text and neutral VRAM bank");
    expect(!memcmp(initial_font,vram[1]+192,sizeof(initial_font)),
           "itinerary and payment draws preserve actual uploaded font patterns");
}

static void expect_compact_status(unsigned row,int active){
    char expected[7];UBYTE star;
    if(active){
        unsigned seconds=td.left>999?999:td.left;
        snprintf(expected,sizeof(expected),"%3uS ",seconds);
        for(unsigned i=0;i<5;i++)expect(window_tiles[0][row][12+i]==td_glyph(expected[i]),
             "the visible compact job strip preserves the actual bounded remaining deadline");
    }else{
        snprintf(expected,sizeof(expected),"%5u",td.cash);
        expect(window_tiles[0][row][11]==237,"idle compact notices retain their wallet dollar glyph");
        for(unsigned i=0;i<5;i++)expect(window_tiles[0][row][12+i]==td_glyph(expected[i]),
             "idle compact notices preserve every digit of the actual wallet");
    }
    for(unsigned i=0;i<3;i++){
        star=i<td.wanted&&!(td.wanted_left<30&&(td.seconds&1))?249:250;
        expect(window_tiles[0][row][17+i]==star,"the visible strip has exactly three attention stars and blinks during actual evasion");
    }
}

static void expect_foot_health(void){
    char expected[21];
    snprintf(expected,sizeof(expected),"HP%u  AMMO%u",td.vitality,td.ammo);
    for(unsigned i=(unsigned)strlen(expected);i<17;i++)expected[i]=' ';
    for(unsigned i=0;i<3;i++)expected[17+i]=i<td.wanted&&!(td.wanted_left<30&&(td.seconds&1))?9:10;
    expected[20]=0;
    expect_window_text(1,expected,"walking always displays actual health, ammunition and three evasion-aware stars");
}

static void expect_three_row_hud_safe(void){
    UBYTE safe=1;
    for(unsigned row=0;row<3;row++)for(unsigned column=0;column<20;column++)
        if(window_tiles[0][row][column]<192||window_tiles[0][row][column]>252||
           window_tiles[1][row][column]!=15)safe=0;
    expect(safe&&window_x==0&&window_y==(td.mode==TD_ROAM?(td.onfoot?(td.msg?120:128):(td.msg?128:136)):120)&&VBK_REG==0,
           "gameplay HUD occupies one row normally, two for notices and three for transit, within reserved native patterns");
    expect(!memcmp(initial_font,vram[1]+192,sizeof(initial_font)),
           "ferry cancellation HUD preserves the actual uploaded font patterns");
}

static void expect_board_preserves_game(const game_snapshot_t *before) {
    expect(!memcmp(&td,&before->state,sizeof(td))&&!memcmp(&td_job,&before->job,sizeof(td_job))&&
           !memcmp(&td_offer,&before->offer,sizeof(td_offer))&&!memcmp(&td_target,&before->target,sizeof(td_target))&&
           td_route_district==before->route&&td_resume_mode==before->resume_mode&&actors_len==before->actor_count,
           "itinerary draw only refreshes its cursor; saved game, active contract, offer, target and booking stay fixed");
}

static void expect_pause_hint(const char *expected){
    game_snapshot_t before=snapshot_game();actor_t before_actors[TD_ACTORS];
    UBYTE old_lock=combat_locked_fixture,old_near=near_car_fixture,old_entry=td_entry_timer;
    UWORD old_camera_x=camera_x,old_camera_y=camera_y;UBYTE old_camera_settings=camera_settings;
    memcpy(before_actors,actors,sizeof(before_actors));
    td_ui_draw();expect_window_text(17,expected,"highlighted Pause action explains its actual availability or rejection without truncation");
    expect_text_screen_safe();expect_game_unchanged(&before);
    expect(!memcmp(before_actors,actors,sizeof(before_actors))&&camera_x==old_camera_x&&camera_y==old_camera_y&&
           camera_settings==old_camera_settings&&combat_locked_fixture==old_lock&&near_car_fixture==old_near&&td_entry_timer==old_entry,
           "contextual action hints preserve actor, camera, recovery, door and input-guard state");
    char bounded[40];memset(bounded,0x5a,sizeof(bounded));td_menu_hint(bounded);
    size_t length=strlen(expected);
    expect(length<=20&&!strcmp(bounded,expected),"BANKED hint copies a complete bounded line into caller-owned WRAM");
    for(size_t i=length+1;i<sizeof(bounded);i++)expect(bounded[i]==0x5a,"hint never writes beyond its own terminator or overwrites the caller buffer tail");
    expect_game_unchanged(&before);
    unsigned writes=window_writes,uploads=tile_uploads;td_ui_draw();
    expect(window_writes==writes&&tile_uploads==uploads,"unchanged availability hint reuses the existing packed text cache");
}

static void test_pause_action_hints(void){
    reset_case();td.mode=TD_PAUSE;td.job=TD_NONE;td.msg=0;td.onfoot=0;td.speed=0;
    for(UBYTE menu=0;menu<9;menu++){
        td.menu=menu;
        expect_pause_hint(menu==5?"PARK THEN WALK":"SELECT CITY MAP");
    }
    /* A lethal hit or active recovery makes car/transit actions unavailable,
       while planning, Save and Settings retain their ordinary availability. */
    for(UBYTE dead=0;dead<2;dead++){
        td.vitality=dead?0:37;combat_locked_fixture=dead?0:1;
        for(UBYTE menu=0;menu<9;menu++){
            td.menu=menu;expect_pause_hint(menu>=3&&menu<=5?"RESUME TO RECOVER":"SELECT CITY MAP");
        }
    }
    td.vitality=100;combat_locked_fixture=0;td.menu=4;
    td.onfoot=1;expect_pause_hint("ENTER YOUR CAR FIRST");
    td.job=0;expect_pause_hint("FINISH OR CANCEL JOB");
    td.onfoot=0;expect_pause_hint("FINISH OR CANCEL JOB");
    td.job=TD_NONE;
    const WORD speeds[]={-4,-3,-2,0,2,3,14};
    for(unsigned i=0;i<sizeof(speeds)/sizeof(*speeds);i++){
        td.speed=speeds[i];expect_pause_hint(speeds[i]<-2||speeds[i]>2?"STOP TO CHANGE CAR":"SELECT CITY MAP");
    }
    /* These notices come from the unchanged actual rejection path. Switching
       selection clears the longer footer rather than leaking stale letters. */
    td.speed=0;td.menu=3;td.onfoot=1;near_car_fixture=0;td.msg=9;expect_pause_hint("WALK TO YOUR CAR");
    near_car_fixture=1;td.msg=15;expect_pause_hint("DOOR PATH BLOCKED");
    td.onfoot=0;td.msg=17;expect_pause_hint("MOVE OFF TRAM RAILS");
    td.msg=11;td.speed=-3;expect_pause_hint("STOP TO USE CAR DOOR");
    td.menu=0;expect_pause_hint("SELECT CITY MAP");
    td.menu=5;td.speed=0;td.msg=0;td_entry_timer=1;expect_pause_hint("FINISH CAR ENTRY");
    td_entry_timer=0;expect_pause_hint("PARK THEN WALK");
    td.onfoot=1;td.job=0;
    for(UBYTE kind=0;kind<8;kind++){
        td_job.kind=kind;td.u=560*16;td.v=720*16;td.district=TD_DISTRICT_CITY;
        expect_pause_hint(kind==3||kind==5?"DRIVE FOR THIS JOB":"SELECT CITY MAP");
    }
    td.job=TD_NONE;
    /* Every authored boarding point, its exact15px boundary and a remote
       district use the same origin eligibility as actual TTC interaction. */
    for(UBYTE stop=0;stop<TD_STOPS;stop++){
        td.district=host_ui_stops[stop].district;td.u=host_ui_stops[stop].u*16;td.v=host_ui_stops[stop].v*16;
        if(host_ui_stops[stop].transit&&td_transit_can_origin(stop))expect_pause_hint("SELECT CITY MAP");
    }
    td.district=TD_DISTRICT_CITY;td.u=0;td.v=0;td.msg=6;expect_pause_hint("WALK TO A TTC STOP");
    td.u=host_ui_stops[0].u*16;td.v=host_ui_stops[0].v*16;
    td.u+=14*16;expect_pause_hint("SELECT CITY MAP");
    td.u+=16;expect_pause_hint("WALK TO A TTC STOP");
    /* Preserve the actual interaction's whole-pixel floor policy, including
       its directional asymmetry. A negative239 Q4 offset floors to15px away;
       positive239 floors to14px. Do not silently alter boarding in a UI fix.
       Literal eligibility and the unchanged main-engine proximity function
       independently check both axes and opposing fractional combinations. */
    static const struct {WORD offset;UBYTE allowed;} fractional_edges[]={
        {-241,0},{-240,0},{-239,0},{-225,0},{-224,1},{-1,1},{0,1},
        {1,1},{224,1},{225,1},{239,1},{240,0},{241,0}
    };
    for(unsigned x=0;x<sizeof(fractional_edges)/sizeof(*fractional_edges);x++)
        for(unsigned y=0;y<sizeof(fractional_edges)/sizeof(*fractional_edges);y++){
            UBYTE expected=fractional_edges[x].allowed&&fractional_edges[y].allowed;
            td.u=host_ui_stops[0].u*16+fractional_edges[x].offset;
            td.v=host_ui_stops[0].v*16+fractional_edges[y].offset;
            expect(td_ui_actual_near_stop(0)==expected,
                   "fractional boundary oracle matches the unchanged actual main-engine boarding proximity on both axes");
            expect_pause_hint(expected?"SELECT CITY MAP":"WALK TO A TTC STOP");
        }
    td.district=TD_DISTRICT_ISLANDS;expect_pause_hint("WALK TO A TTC STOP");
    for(UBYTE mode=TD_WAIT;mode<=TD_RIDE;mode++){
        td_resume_mode=mode;td.msg=2;
        for(UBYTE menu=0;menu<9;menu++){
            td.menu=menu;expect_pause_hint(menu>=2&&menu<=7?(menu==6?"SAVE AFTER THIS TRIP":"WAIT UNTIL TRIP ENDS"):"TTC: MAP/SETTINGS");
        }
    }
    /* The two previous overlong captions now occupy complete native rows. */
    reset_case();td.mode=TD_TRANSIT;td.transit_origin=0;td.transit_target=1;td_ui_draw();
    expect_window_text(16,"WELLESLEY TRANSFER","interchange caption fits all twenty hardware columns");
    expect_window_text(17,"FICTIONAL SCHEDULES","timetable disclaimer keeps its complete final word");
}

static void test_pause_audio_labels_and_map_cache(void){
    static const char *const labels[3]={"  MUSIC + EFFECTS","  EFFECTS ONLY","  ALL SOUND OFF"};
    expect(TD_AUDIO_FULL==0&&TD_AUDIO_EFFECTS==1&&TD_AUDIO_SILENT==2,
           "settings sound oracle covers the three actual session preference modes");
    for(UBYTE riding=0;riding<2;riding++){
        reset_case();td.district=TD_DISTRICT_ISLANDS;td.u=512*16;td.v=448*16;
        td.park_district=TD_DISTRICT_CITY;td.park_u=560*16;td.park_v=720*16;
        td_streetcar_view_district=riding?TD_DISTRICT_EAST:TD_DISTRICT_ISLANDS;
        td_streetcar_focus_u=(riding?128:512)*16;td_streetcar_focus_v=(riding?536:448)*16;
        td_streetcar_ride_view=riding;
        if(riding){td_resume_mode=TD_RIDE;td.transit_origin=43;td.transit_target=48;td.ride_left=4;}
        for(UBYTE audio=0;audio<3;audio++)for(UBYTE selected=0;selected<2;selected++){
            td.mode=TD_PAUSE;td.menu=selected?TD_SETTINGS_SOUND:TD_SETTINGS_CONTROLS;audio_fixture_mode=audio;
            game_snapshot_t before=snapshot_game();actor_t original_actors[22];
            memcpy(original_actors,actors,sizeof(actors));UWORD old_camera_x=camera_x,old_camera_y=camera_y;
            UBYTE old_camera_settings=camera_settings;
            td_ui_draw();
            expect_window_text(6,labels[audio],"settings sound mode has the exact readable label");
            expect_window_text(17,"SOUND RESETS AT BOOT","sound preferences explicitly explain their existing session lifetime");
            expect_window_text(5,selected?"> SOUND":"  SOUND","settings identifies the selected sound option");
            expect(memchr(td_line,0,sizeof(td_line))&&strlen(td_line)<=20,
                   "pause formatting terminates inside its40-byte buffer and the visible twenty-column width");
            expect_text_screen_safe();expect_game_unchanged(&before);
            expect(!memcmp(actors,original_actors,sizeof(actors))&&camera_x==old_camera_x&&camera_y==old_camera_y&&
                   camera_settings==old_camera_settings,"pause audio repaint changes no actor or camera state");
            unsigned writes=window_writes,uploads=tile_uploads;td_ui_draw();
            expect(window_writes==writes&&tile_uploads==uploads,"an unchanged pause audio label reuses all cached text rows without upload");
            expect_game_unchanged(&before);
            /* Replay the native failure's PAUSE -> MAP path after every
               audio label. A bogus paid-view flag would incorrectly reuse
               another district or keep the map's prior zero origin. */
            open_case();before=snapshot_game();finish_paint();
            expect_window_text(1,riding?"TORONTO EAST END":"TORONTO ISLANDS",
                               "YOU map focus after audio repaint names the actual walking or paid-view district");
            UBYTE expected_x,expected_y;
            expected_centre(riding?TD_DISTRICT_EAST:TD_DISTRICT_ISLANDS,
                            riding?128:512,riding?536:448,&expected_x,&expected_y);
            expect(td_map_x==expected_x&&td_map_y==expected_y,
                   "YOU map focus after audio repaint preserves exact independent atlas coordinate and clamp expectations");
            expect_game_unchanged(&before);td_map_close();
            expect(!memcmp(actors,original_actors,sizeof(actors))&&camera_x==old_camera_x&&camera_y==old_camera_y&&
                   camera_settings==old_camera_settings,"closing the audio-to-map replay restores the same actors and camera");
            td.mode=TD_PAUSE;before=snapshot_game();td_ui_draw();
            expect_window_text(6,labels[audio],"map close repaints the same sound preference without stale longer text");
            td.menu=8;td_ui_draw();
            expect_window_text(12,"> SETTINGS","main menu returns to its Settings option");
            expect_window_text(10,riding?"  SAVE AFTER TRIP":"  SAVE GAME","manual save availability is visible while transit keeps its existing automatic saves");
            expect_window_text(17,riding?"TTC: MAP/SETTINGS":"SELECT CITY MAP","travel pause explains its allowed planning controls");
            expect_window_text(6,"  JOBS","settings-to-menu removes the old sound label");
            expect_window_text(8,"  CHANGE VEHICLE","main menu does not retain the submenu cursor");
            td.menu=selected?TD_SETTINGS_SOUND:TD_SETTINGS_CONTROLS;
            expect_game_unchanged(&before);
        }
    }
}

static void expect_board_stop(unsigned job,unsigned page) {
    const td_job_t *offer=&host_ui_jobs[job];const td_stop_t *stop=&host_ui_stops[offer->route[page]];
    char expected[40];const char *role;
    if(page==0)role="PICKUP";
    else if(page+1<offer->count)role="HANDOFF";
    else role=offer->route[page]==offer->route[0]?"RETURN":"DELIVER";
    sprintf(expected,"%u/%u %s%s",page+1,offer->count,
            ((stop->reserved&TD_STOP_FOOT)||stop->district==TD_DISTRICT_ISLANDS)?"WALK ":"",role);
    expect_window_text(11,expected,"authored itinerary page identifies its number, walking requirement and handoff role");
    expect_window_text(12,stop->name,"itinerary renders the exact authored client name rather than a parking approach");
    expect_window_text(13,host_ui_districts[stop->district],"itinerary renders the actual client district");
    expect(td_cursor.u==stop->u&&td_cursor.v==stop->v&&td_cursor.district==stop->district&&
           td_cursor.reserved==stop->reserved&&!strcmp(td_cursor.name,stop->name),
           "itinerary native getter selects the true client including remote and foot-only records");
    expect(td_board_route==page,"valid itinerary draw preserves the caller's selected page");
}

static void test_dispatch_board_itineraries(void) {
    expect(sizeof(host_ui_jobs)/sizeof(host_ui_jobs[0])==TD_QUESTS&&
           sizeof(host_ui_stops)/sizeof(host_ui_stops[0])==TD_STOPS,
           "independent editable-content oracle covers every registered native job and stop");
    UBYTE districts=0,saw_return=0,saw_delivery=0,saw_foot=0;
    for(unsigned job=0;job<TD_QUESTS;job++) {
        reset_case();td.job=TD_NONE;td.mode=TD_BOARD;td.menu=job;td_get_job(job,&td_offer);
        const td_job_t *offer=&host_ui_jobs[job];char expected[40],brief[19];
        expect(td_offer.count==offer->count&&td_offer.reward==offer->reward&&td_offer.seconds==offer->seconds&&
               td_offer.vehicle==offer->vehicle&&td_offer.kind==offer->kind&&td_offer.min_done==offer->min_done&&
               !strcmp(td_offer.title,offer->title)&&!memcmp(td_offer.route,offer->route,12),
               "board fixture uses unchanged compiled content matching independent authored JSON");
        for(unsigned page=0;page<offer->count;page++) {
            td_board_route=page;game_snapshot_t before=snapshot_game();td_ui_draw();
            expect_board_stop(job,page);expect_board_preserves_game(&before);expect_text_screen_safe();
            const td_stop_t *stop=&host_ui_stops[offer->route[page]];
            districts|=1u<<stop->district;if(stop->reserved&TD_STOP_FOOT)saw_foot=1;
            if(page+1==offer->count){if(offer->route[page]==offer->route[0])saw_return=1;else saw_delivery=1;}
            if(!page) {
                expect_window_text(1,host_ui_chapters[job/8],"each actual offer displays its authored chapter and position within twelve groups");
                expect_window_text(3,"SELECT: CHAPTER","board exposes the chapter shortcut without hiding the individual offer and itinerary controls");
                sprintf(expected,"CONTRACT %02u/%u",job+1,TD_QUESTS);expect_window_text(2,expected,"board displays its actual contract ID and expanded count");
                expect_window_text(4,offer->title,"board retains the exact authored contract title");
                memcpy(brief,host_ui_briefs[job],18);brief[18]=0;expect_window_text(5,brief,"first brief line remains visible above itinerary");
                memcpy(brief,host_ui_briefs[job]+18,18);expect_window_text(6,brief,"second brief line remains visible above itinerary");
                sprintf(expected,"%u STOPS  %u SEC",offer->count,offer->seconds);expect_window_text(8,expected,"board keeps the authored stop count and deadline");
                sprintf(expected,"BASE $%u + TIME",offer->reward);expect_window_text(9,expected,"board advertises base reward with a separate time bonus");
                const char *vehicles[]={"CAR","TRUCK","MOTORCYCLE","SCOOTER"};
                expect_window_text(10,offer->vehicle==TD_NONE?"ANY VEHICLE / TTC":vehicles[offer->vehicle],
                                   "itinerary retains its authored fixed-vehicle or transit-friendly eligibility");
                expect_window_text(14,"L/R JOB U/D STOPS","itinerary exposes job and stop navigation on one native row");
                expect_window_text(15,"A ACCEPT  B BACK","itinerary retains its accept/cancel actions");
            }
        }
        /* The driver owns button dispatch. Present both wrap endpoints and
         * stale invalid cursor recovery through the real renderer here. */
        td_board_route=0;td_ui_draw();expect_board_stop(job,0);
        td_board_route=offer->count-1;td_ui_draw();expect_board_stop(job,offer->count-1);
        td_board_route=255;game_snapshot_t before=snapshot_game();td_ui_draw();
        expect_board_stop(job,0);expect_board_preserves_game(&before);
        unsigned writes=window_writes;td_ui_draw();
        expect(window_writes==writes,"an unchanged board page reuses its actual row cache without redundant tile writes");
        td.complete[job>>3]|=1u<<(job&7);td_ui_draw();expect_window_text(16,"COMPLETE / REPLAY","completed itinerary keeps its replay state");
        td.complete[job>>3]&=~(1u<<(job&7));td.done=offer->min_done;
        td.vehicle=offer->vehicle==TD_NONE?0:offer->vehicle;td.onfoot=0;td_ui_draw();
        expect_window_text(16,"READY TO ACCEPT","unlock boundary keeps the current itinerary ready");
        if(offer->min_done) {
            td.done=offer->min_done-1;td_ui_draw();sprintf(expected,"NEEDS %u COMPLETED",offer->min_done);
            expect_window_text(16,expected,"locked itinerary states its actual completion requirement");
        }
    }
    expect(districts==((1u<<TD_DISTRICT_COUNT)-1)&&saw_return&&saw_delivery&&saw_foot,
           "all-route rendering covers every registered district, final deliveries, returns and walking clients");

    reset_case();td_board_route=231;game_snapshot_t initialized=snapshot_game();td_ui_init();
    expect(td_board_route==0,"actual native UI initialization clears a dirty transient itinerary index");
    expect_game_unchanged(&initialized);

    /* A shorter name/role must clear the prior long row even in the same
     * mode. An empty offer must clear all three formerly occupied rows. */
    reset_case();td.job=TD_NONE;td.mode=TD_BOARD;td.menu=93;td_get_job(93,&td_offer);td_board_route=1;td_ui_draw();
    expect_window_text(12,"RIVERBANK PARCEL","stale-row fixture begins with a real long Port client name");
    td.menu=0;td_get_job(0,&td_offer);td_board_route=1;td_ui_draw();
    expect_window_text(1,host_ui_chapters[0],"shorter chapter captions clear the previous Port Lands row through the actual cache");
    expect_board_stop(0,1);expect_text_screen_safe();
    td_offer.count=0;game_snapshot_t before=snapshot_game();td_ui_draw();
    expect_window_text(11,"NO ROUTE","empty itinerary visibly rejects a stale route");
    expect_window_text(12,"","empty itinerary clears the previous client row");
    expect_window_text(13,"","empty itinerary clears the previous district row");
    expect_game_unchanged(&before);

    td.mode=TD_RESULT;td_ui_draw();
    expect_window_text(1,"","leaving dispatch removes the chapter caption inside the original result card");
    expect_window_text(3,"","leaving dispatch clears the chapter shortcut caption");
    for(unsigned invalid=TD_QUESTS;invalid<=255;invalid++){
        td.mode=TD_BOARD;td.menu=invalid;before=snapshot_game();td_ui_draw();
        expect_window_text(1,"DISPATCH CHAPTER","every invalid transient menu uses a bounded fallback without indexing the chapter table");
        expect_window_text(16,"READY TO ACCEPT","invalid menu never reads or invents a completed-offer bit outside the actual campaign");
        expect_game_unchanged(&before);expect_text_screen_safe();
    }
}

static void test_dispatch_active_board_captions(void) {
    reset_case();td.mode=TD_BOARD;td.job=td.menu=0;td.stage=1;td.done=0;
    td_get_job(0,&td_job);td_get_job(0,&td_offer);td_board_route=0;
    game_snapshot_t before=snapshot_game();td_ui_draw();expect_board_stop(0,0);
    expect_window_text(16,"CURRENT STOP 2/2","active offer identifies the actual next handoff while its full itinerary still opens at pickup");
    expect_window_text(15,"A RESUME  B BACK","an active preview offers resume rather than another acceptance");
    expect_board_preserves_game(&before);expect_text_screen_safe();
    td.complete[0]|=1;td.done=1;td_board_route=1;before=snapshot_game();td_ui_draw();expect_board_stop(0,1);
    expect_window_text(16,"CURRENT STOP 2/2","a replay's completed bit cannot hide the current carried handoff");
    expect_board_preserves_game(&before);
    td.menu=1;td_get_job(1,&td_offer);td_board_route=1;before=snapshot_game();td_ui_draw();expect_board_stop(1,1);
    expect_window_text(16,"READY AFTER THIS JOB","another eligible offer remains a preview until the carried job ends");
    expect_window_text(15,"A RESUME  B BACK","another offer cannot advertise replacement of an active job");
    expect_board_preserves_game(&before);expect_text_screen_safe();
    td.menu=8;td_get_job(8,&td_offer);td_board_route=0;before=snapshot_game();td_ui_draw();
    expect_window_text(16,"NEEDS 6 COMPLETED","an active preview preserves each other offer's authored completion lock");
    expect_window_text(15,"A RESUME  B BACK","a locked preview still resumes the existing job rather than attempting acceptance");
    expect_board_preserves_game(&before);
    td.menu=1;td_get_job(1,&td_offer);td.complete[0]|=2;td.done=2;before=snapshot_game();td_ui_draw();
    expect_window_text(16,"COMPLETE / PREVIEW","another completed contract is previewable but not replayable while work is carried");
    expect_board_preserves_game(&before);expect_text_screen_safe();
    td.menu=0;td_get_job(0,&td_offer);before=snapshot_game();td_ui_draw();
    expect_window_text(16,"CURRENT STOP 2/2","shorter active status clears a previous longer completed caption through the actual row cache");
    expect_board_preserves_game(&before);
    unsigned writes=window_writes;td_ui_draw();
    expect(window_writes==writes,"unchanged active status and controls reuse their native text cache");
}

static void test_dispatch_vehicle_readiness(void) {
    for(unsigned job=0;job<TD_QUESTS;job++) {
        const td_job_t *offer=&host_ui_jobs[job];
        for(unsigned vehicle=0;vehicle<4;vehicle++)for(unsigned foot=0;foot<2;foot++) {
            for(unsigned context=0;context<6;context++) {
                if(context==1&&!offer->min_done)continue;
                reset_case();td.mode=TD_BOARD;td.menu=job;td_offer=*offer;
                td.vehicle=vehicle;td.onfoot=foot;td.done=TD_QUESTS;
                td.job=TD_NONE;td.stage=1;td.msg=2;
                char expected[40];
                if(context==1) {
                    td.done=offer->min_done-1;
                    sprintf(expected,"NEEDS %u COMPLETED",offer->min_done);
                }else if(context==2) {
                    td.complete[job>>3]|=1u<<(job&7);
                    strcpy(expected,"COMPLETE / REPLAY");
                }else if(context==3) {
                    td.job=job;td_job=*offer;
                    sprintf(expected,"CURRENT STOP 2/%u",offer->count);
                }else if(context>=4) {
                    td.job=job?0:1;td_job=host_ui_jobs[td.job];
                    if(context==5)td.complete[job>>3]|=1u<<(job&7);
                    strcpy(expected,context==5?"COMPLETE / PREVIEW":"READY AFTER THIS JOB");
                }else {
                    const char *required[]={"car","truck","motorcycle","scooter"};
                    const char *occupied=foot?"foot":required[vehicle];
                    strcpy(expected,offer->vehicle!=TD_NONE&&strcmp(occupied,required[offer->vehicle])?
                        "WRONG VEHICLE":"READY TO ACCEPT");
                }
                game_snapshot_t before=snapshot_game();td_ui_draw();
                expect_window_text(16,expected,"authored vehicle/foot eligibility has clear feedback without overriding locks, completion or active-job resume");
                expect_window_text(15,td.job==TD_NONE?"A ACCEPT  B BACK":"A RESUME  B BACK",
                    "vehicle warning preserves actual accept or resume controls");
                expect_board_preserves_game(&before);expect_text_screen_safe();
                unsigned writes=window_writes;td_ui_draw();
                expect(window_writes==writes,"unchanged vehicle/foot status reuses the native row cache");
            }
        }
    }
    reset_case();td.mode=TD_ROAM;td.msg=2;
    game_snapshot_t before=snapshot_game();td_ui_draw();
    expect_window_text(0,"WRONG VEHICLE","the shared warning preserves the existing roaming interaction message");
    expect_game_unchanged(&before);expect_three_row_hud_safe();
}

static void result_fixture(unsigned job,UBYTE condition,UWORD left,UWORD previous,UBYTE done) {
    td.mode=TD_RESULT;td.job=TD_NONE;td.health=condition;td.left=left;td.done=done;
    td_get_job(job,&td_job);td_offer.reward=previous;
    /* Independent wide arithmetic follows authored payout rules, avoiding
     * the runtime's split multiply and the renderer's cached-balance logic. */
    unsigned condition_pay=(unsigned long)host_ui_jobs[job].reward*condition/100u;
    unsigned bonus=left/5u,total=condition_pay+bonus;
    unsigned after=condition&&left?previous+total:previous;
    if(after>60000)after=60000;
    td.cash=after;
    game_snapshot_t before=snapshot_game();char expected[40];td_ui_draw();
    expect_window_text(4,condition&&left?"CONTRACT DELIVERED":"CONTRACT FAILED","result distinguishes completed and failed contracts");
    sprintf(expected,"CONDITION %u%%",condition);expect_window_text(5,expected,"result displays the actual final cargo condition");
    if(condition&&left) {
        sprintf(expected,"BASE $%u",host_ui_jobs[job].reward);expect_window_text(6,expected,"result preserves authored base separately from scaled payment");
        sprintf(expected,"CARGO PAY $%u",condition_pay);expect_window_text(7,expected,"condition payment matches independent wide multiply/floor");
        sprintf(expected,"%uS BONUS$%u",left,bonus);expect_window_text(8,expected,"time bonus matches independent five-second floor");
        sprintf(expected,"CREDIT $%u",after-previous);expect_window_text(9,expected,"credit shows the actual balance increase including cash cap");
        expect_window_text(10,total>60000u-previous?"BALANCE CAP $60000":
                           done==TD_QUESTS?"MASTER COURIER":"MORE ROUTES AWAIT",
                           "result cap/master cue follows actual credit rather than nominal reward");
    }else {
        expect_window_text(6,"NO PAYMENT","failure explicitly receives no payment");
        sprintf(expected,"TIME %uS",left);expect_window_text(7,expected,"failure retains actual remaining time");
        expect_window_text(8,"","failure clears a previously visible bonus row");
        expect_window_text(9,"CREDIT $0","failure clears a previously positive credit");
        expect_window_text(10,"RETRY / CHOOSE JOB","failure offers a useful continuation cue");
    }
    sprintf(expected,"$%u DONE%u/%u",after,done,TD_QUESTS);expect_window_text(11,expected,"result retains final cash and expanded unique-completion count");
    expect_window_text(13,"A: DISPATCH BOARD","result retains dispatch continuation");
    expect_window_text(14,"B: FREE ROAM","result retains free-roam continuation");
    expect_window_text(16,"PROGRESS AUTO-SAVED","result retains its persistence cue");
    expect_game_unchanged(&before);expect_text_screen_safe();
    unsigned writes=window_writes;td_ui_draw();expect(window_writes==writes,"an unchanged result retains its actual row cache");
}

static void test_contract_payment_result(void) {
    reset_case();
    for(unsigned job=0;job<TD_QUESTS;job++) {
        result_fixture(job,36,host_ui_jobs[job].seconds,71,4);
        result_fixture(job,100,5,59999,95);
        result_fixture(job,100,4,60000,96);
    }
    /* Real Fire Hall observed values plus condition and time floor edges.
     * Keep success/failure in the same mode to expose stale cached rows. */
    result_fixture(89,36,86,0,4);
    result_fixture(0,1,1,0,0);result_fixture(0,99,4,17,95);
    result_fixture(0,100,5,17,96);
    result_fixture(0,100,1,60000-host_ui_jobs[0].reward,96);
    result_fixture(0,0,99,123,2);result_fixture(0,100,0,123,2);
    result_fixture(95,100,65535,59000,96);

    /* A defensive negative delta must be explicit, and a failed result must
     * still erase it rather than showing a made-up positive payout. */
    td.health=100;td.left=5;td.cash=50;td_offer.reward=123;game_snapshot_t before=snapshot_game();td_ui_draw();
    expect_window_text(9,"BALANCE -$73","result represents a negative cached balance delta explicitly");
    expect_game_unchanged(&before);
    td.health=0;td.left=0;before=snapshot_game();td_ui_draw();
    expect_window_text(9,"CREDIT $0","failure replaces a defensive negative balance delta");
    expect_window_text(8,"","failure removes stale successful time/bonus text");expect_game_unchanged(&before);
}

static void test_car_entry_hud_repaint(void) {
    static const char *const names[]={"CAR","TRUCK","MOTORCYCLE","SCOOTER"};
    for(UBYTE vehicle=0;vehicle<4;vehicle++){
        reset_case();td.mode=TD_ROAM;td.job=TD_NONE;td.msg=0;td.vehicle=vehicle;td.onfoot=1;
        game_snapshot_t before=snapshot_game();td_ui_draw();
        expect_foot_health();
        expect_window_text(2,"A/B CAR A DOOR/BOAT","car-entry fixture begins with the actual walking controls");
        expect_game_unchanged(&before);
        /* The engine harness proves completion calls the renderer after the
           on-foot transition. Exercise that real repaint against its cache. */
        td.onfoot=0;td.u=td.park_u;td.v=td.park_v;before=snapshot_game();td_ui_draw();
        char status[21];sprintf(status,"$123 %s H2",names[vehicle]);
        expect_window_text(1,status,"completed entry immediately displays the actual selected vehicle rather than stale WALK");
        expect_window_text(2,"SELECT JOBS START UI","completed entry removes stale car-entry and transit controls in the same mode");
        expect_game_unchanged(&before);
        unsigned writes=window_writes;td_ui_draw();
        expect(window_writes==writes,"unchanged occupied-car HUD uses its existing row cache after the completion repaint");
        td.onfoot=1;before=snapshot_game();td_ui_draw();
        expect_foot_health();
        expect_window_text(2,"A/B CAR A DOOR/BOAT","a later ordinary exit restores the walking controls");
        expect_game_unchanged(&before);
    }
}

static void test_wait_contact_hud_and_map_restore(void) {
    /* Fixed output examples exercise the shared WAIT layout, not another
     * timetable oracle. Transit arithmetic has its own independent suite. */
    const struct {UBYTE origin,target,wait;UWORD seconds;const char *service;} cases[]={
        {46,48,252,52,"501 QUEEN"},
        {46,44,252,196,"501 QUEEN"},
        {0,12,16,2,"LINE 1 TRAIN"},
        {80,19,22,6,"94 WELLESLEY BUS"},
        {10,20,28,2,"ISLAND FERRY"}
    };
    for(unsigned fixture=0;fixture<sizeof(cases)/sizeof(cases[0]);fixture++) {
        reset_case();td.mode=td_resume_mode=TD_WAIT;td.transit_origin=cases[fixture].origin;
        td.transit_target=cases[fixture].target;td.seconds=cases[fixture].seconds;
        td.ride_left=0;td.msg=0;strcpy(td_cursor.name,"BOOKED DESTINATION");
        game_snapshot_t before=snapshot_game();char countdown[21];
        td_ui_draw();
        sprintf(countdown,"DEPARTS IN %u SEC",cases[fixture].wait);
        expect_window_text(0,countdown,"ordinary WAIT retains its actual departure countdown");
        expect_window_text(1,cases[fixture].service,"ordinary WAIT identifies its booked service");
        expect_window_text(2,"B CANCEL WAIT","ordinary WAIT retains its cancellation action");
        expect_game_unchanged(&before);

        td.msg=18;before=snapshot_game();td_ui_draw();
        expect_window_text(0,countdown,"contact cue leaves the departure countdown visible in row0");
        expect_window_text(1,"TRAM: STEP CLEAR","blocked WAIT visibly gives the complete contact cue in row1");
        expect_window_text(2,"B CANCEL WAIT","contact cue preserves B cancellation in row2");
        expect(window_x==0&&window_y==120,"WAIT contact keeps the native three-row lower HUD");
        UBYTE bounded=1;
        for(unsigned row=0;row<3;row++)for(unsigned column=0;column<20;column++)
            if(window_tiles[0][row][column]<192||window_tiles[0][row][column]>240||
               window_tiles[1][row][column]!=15)bounded=0;
        expect(bounded,"WAIT contact rows use bounded font tiles and the existing native palette/bank");
        expect_game_unchanged(&before);

        /* A contact message must not make the countdown stale as the caller
         * advances the world clock, or mutate the unpaid booking itself. */
        td.seconds++;before=snapshot_game();td_ui_draw();
        sprintf(countdown,"DEPARTS IN %u SEC",cases[fixture].wait-1);
        expect_window_text(0,countdown,"departure countdown refreshes while contact cue remains visible");
        expect_window_text(1,"TRAM: STEP CLEAR","countdown refresh cannot erase the outstanding contact cue");
        expect_game_unchanged(&before);

        if(fixture==0) {
            UBYTE original_hud[2][3][20];
            memcpy(original_hud[0],window_tiles[0],sizeof(original_hud[0]));
            memcpy(original_hud[1],window_tiles[1],sizeof(original_hud[1]));
            game_snapshot_t waiting=snapshot_game();
            UWORD saved_x=camera_x,saved_y=camera_y;UBYTE settings=camera_settings;
            td.mode=TD_PAUSE;before=snapshot_game();td_ui_draw();expect_game_unchanged(&before);
            td.mode=TD_MAP;before=snapshot_game();td_map_open();td_ui_draw();td_map_update(0,0);
            expect_game_unchanged(&before);
            td_map_close();
            expect(camera_x==saved_x&&camera_y==saved_y&&camera_settings==settings,
                   "paused WAIT map restores its actual captured gameplay camera");
            td.mode=TD_PAUSE;before=snapshot_game();td_ui_draw();expect_game_unchanged(&before);
            td.mode=TD_WAIT;td_ui_draw();
            expect(!memcmp(original_hud[0],window_tiles[0],sizeof(original_hud[0]))&&
                   !memcmp(original_hud[1],window_tiles[1],sizeof(original_hud[1])),
                   "return from a partial atlas repaint restores the exact countdown/contact/cancel HUD");
            expect_game_unchanged(&waiting);
        }

        td.msg=0;before=snapshot_game();td_ui_draw();
        expect_window_text(0,countdown,"clearing contact preserves the current departure countdown");
        expect_window_text(1,cases[fixture].service,"clearing contact restores service identity without stale cue pixels");
        expect_window_text(2,"B CANCEL WAIT","cleared WAIT still exposes its cancellation action");
        expect_game_unchanged(&before);
    }
}

static void test_reserved_islands_assistance_ui(void) {
    /* These direct renderer inputs prove text/cache logic with adapters;
       even a registered scene requires separate native travel checks. */
    for(UBYTE dock=20;dock<=22;dock++){
        reset_case();td.mode=TD_TRANSIT;td.job=TD_NONE;td.district=TD_DISTRICT_ISLANDS;
        td.transit_origin=dock;td.transit_target=10;td.cash=3;td.seconds=(dock-19)*7+2;
        td_get_stop(10,&td_cursor);game_snapshot_t before=snapshot_game();td_ui_draw();
        expect_window_text(2,"ISLAND FERRY","reserved return timetable retains its actual service identity");
        expect_window_text(6,"DEPARTS IN 28 SEC","assistance retains the normal autonomous ferry departure");
        expect_window_text(7,"RIDE 8 SEC / $0","reserved eligible return visibly quotes the shared zero fare");
        expect_window_text(11,"RETURN ASSISTANCE","reserved eligible return explains the zero fare before boarding");
        expect_game_unchanged(&before);expect_text_screen_safe();
        td.cash=4;before=snapshot_game();td_ui_draw();
        expect_window_text(7,"RIDE 8 SEC / $4","exactly four dollars retains the ordinary ferry fare");
        expect_window_text(11,"","a sufficient balance clears stale assistance text without a mode transition");
        expect_game_unchanged(&before);
        td.cash=0;td.job=0;before=snapshot_game();td_ui_draw();
        expect_window_text(7,"RIDE 8 SEC / $4","an active parcel never receives the recovery discount");
        expect_window_text(11,"","an active job cannot retain an earlier assistance caption");expect_game_unchanged(&before);
        td.job=TD_NONE;td.district=TD_DISTRICT_CITY;before=snapshot_game();td_ui_draw();
        expect_window_text(7,"RIDE 8 SEC / $4","a mismatched mainland origin retains its ordinary quoted fare");
        expect_window_text(11,"","the recovery caption cannot leak into a mismatched mainland district");expect_game_unchanged(&before);

        td.district=TD_DISTRICT_ISLANDS;td.mode=td_resume_mode=TD_WAIT;td.msg=0;
        before=snapshot_game();td_ui_draw();
        expect_window_text(0,"DEPARTS IN 28 SEC","assistance WAIT retains its unchanged ferry clock");
        expect_window_text(1,"RETURN ASSISTANCE","assistance WAIT retains its eligibility cue before deduction");
        expect_window_text(2,"B CANCEL WAIT","assistance WAIT remains cancellable");expect_game_unchanged(&before);
        td.msg=18;before=snapshot_game();td_ui_draw();
        expect_window_text(1,"TRAM: STEP CLEAR","the existing safety cue has priority over an assistance caption");expect_game_unchanged(&before);
        td.msg=0;td.seconds++;before=snapshot_game();td_ui_draw();
        expect_window_text(0,"DEPARTS IN 27 SEC","clearing a contact cue refreshes the genuine ferry departure clock");
        expect_window_text(1,"RETURN ASSISTANCE","clearing a contact cue restores assistance without stale font tiles");expect_game_unchanged(&before);
        td.cash=4;before=snapshot_game();td_ui_draw();
        expect_window_text(1,"ISLAND FERRY","WAIT loses its recovery caption when ordinary fare funds become available");
        expect_window_text(2,"B CANCEL WAIT","ordinary and assisted waits share the cancellation control");expect_game_unchanged(&before);
        td.mode=TD_RIDE;td.cash=0;td.ride_left=5;before=snapshot_game();td_ui_draw();
        expect_window_text(0,"RIDING 5 SEC","paid-state presentation retains the already booked remaining duration");
        expect_window_text(2,"FARE PAID / ON TIME","a ride does not re-infer its past fare from the new post-payment balance");expect_game_unchanged(&before);
    }
    reset_case();td.mode=TD_TRANSIT;td.job=TD_NONE;td.district=TD_DISTRICT_ISLANDS;
    td.transit_origin=10;td.transit_target=20;td.cash=0;td_get_stop(20,&td_cursor);
    game_snapshot_t before=snapshot_game();td_ui_draw();
    expect_window_text(7,"RIDE 8 SEC / $4","an outward ferry keeps its ordinary fare even under the reserved UI enum");
    expect_window_text(11,"","no outward trip advertises return assistance");expect_game_unchanged(&before);
    td.mode=TD_ROAM;td.msg=0;before=snapshot_game();td_ui_draw();
    expect_window_text(2,"A BOAT / B FERRY","reserved foot-only roaming points to a ferry rather than car entry");expect_game_unchanged(&before);
    td.district=TD_DISTRICT_CITY;before=snapshot_game();td_ui_draw();
    expect_window_text(2,"A/B CAR A DOOR/BOAT","mainland walking keeps its original car and transit controls");expect_game_unchanged(&before);
}

static void test_island_objective_hud(void){
    for(UBYTE from=0;from<2;from++){
        reset_case();td.mode=TD_ROAM;td.msg=0;td.job=7;td.stage=from?0:1;
        td.left=103;td.health=67;td.wanted=0;td.onfoot=1;
        td.district=from?TD_DISTRICT_ISLANDS:TD_DISTRICT_CITY;
        td_get_job(td.job,&td_job);td_get_stop(from?10:21,&td_target);td_route_district=TD_DISTRICT_NONE;
        game_snapshot_t before=snapshot_game();td_ui_draw();
        expect_window_text(2,from?"RETURN FERRY AT DOCK":"GO TO FERRY TERMINAL",
                           "a disconnected Island objective shows its concrete ferry action in the bounded HUD");
        expect_game_unchanged(&before);
        td.mode=TD_PAUSE;before=snapshot_game();td_ui_draw();expect_game_unchanged(&before);
        td.mode=TD_ROAM;td_ui_draw();
        expect_window_text(2,from?"RETURN FERRY AT DOCK":"GO TO FERRY TERMINAL",
                           "returning from a full-screen menu repaints the correct ferry approach cue");
    }
    reset_case();td.mode=TD_ROAM;td.msg=0;td.job=7;td.stage=1;td.left=103;td.health=67;
    td.district=TD_DISTRICT_WEST;td_get_job(td.job,&td_job);td_get_stop(21,&td_target);td_route_district=0;
    game_snapshot_t before=snapshot_game();td_ui_draw();
    expect_window_text(2,host_ui_districts[0],"a remote Island approach first names its genuine mainland route branch");
    expect_game_unchanged(&before);
    td.district=TD_DISTRICT_ISLANDS;td.stage=2;td_get_stop(25,&td_target);td_route_district=TD_DISTRICT_NONE;
    before=snapshot_game();td_ui_draw();
    expect_window_text(2,"CENTRE PARK POST","a local Island client keeps its own name rather than a return or road error");
    expect_game_unchanged(&before);
}

static void test_ferry_offer_budgets(void){
    unsigned island_offers=0;
    for(unsigned job=0;job<TD_QUESTS;job++){
        const td_job_t *offer=&host_ui_jobs[job];unsigned budget=0;
        if(offer->kind==7){
            island_offers++;
            /* Count actual authored terminal/dock edges, independently of
               the renderer's offer-index groups. Include a normal return
               after a final Island payout, without altering the route. */
            for(unsigned leg=1;leg<offer->count;leg++){
                unsigned a=offer->route[leg-1],b=offer->route[leg];
                if((a==10&&b>=20&&b<=22)||(b==10&&a>=20&&a<=22))budget+=4;
            }
            if(host_ui_stops[offer->route[offer->count-1]].district==TD_DISTRICT_ISLANDS)budget+=4;
        }
        for(unsigned context=0;context<4;context++){
            reset_case();td.mode=TD_BOARD;td.menu=job;td_offer=*offer;td_board_route=1;
            td.job=context==2?job:TD_NONE;td_job=*offer;td.stage=offer->count-1;
            td.done=context==1?0:TD_QUESTS;td.cash=3;
            td.vehicle=offer->vehicle==TD_NONE?0:offer->vehicle;td.onfoot=0;
            if(context==3)td.complete[job>>3]|=1u<<(job&7);
            td_get_stop(offer->route[td_board_route],&td_cursor);
            game_snapshot_t before=snapshot_game();td_ui_draw();char expected[40],brief[19];
            if(budget)sprintf(expected,"FERRY BUDGET $%u",budget);
            else strcpy(expected,"PAUSE FREEZES CLOCK");
            expect_window_text(17,expected,"all actual offers show only their independently derived ferry reserve or ordinary pause cue");
            memcpy(brief,host_ui_briefs[job],18);brief[18]=0;
            expect_window_text(5,brief,"ferry hint preserves the first pinned authored brief");
            memcpy(brief,host_ui_briefs[job]+18,18);
            expect_window_text(6,brief,"ferry hint preserves the second pinned authored brief");
            expect_board_stop(job,td_board_route);
            if(context==2){
                sprintf(expected,"CURRENT STOP %u/%u",td.stage+1,offer->count);
                expect_window_text(16,expected,"ferry budget does not replace active actual-stage feedback");
                expect_window_text(15,"A RESUME  B BACK","active ferry preview retains resume controls");
            }else{
                if(context==3)strcpy(expected,"COMPLETE / REPLAY");
                else if(td.done<offer->min_done)sprintf(expected,"NEEDS %u COMPLETED",offer->min_done);
                else strcpy(expected,"READY TO ACCEPT");
                expect_window_text(16,expected,"ferry budget preserves locks and completed/eligible captions");
                expect_window_text(15,"A ACCEPT  B BACK","ferry preview retains acceptance controls");
            }
            expect_window_text(1,host_ui_chapters[job/8],"ferry hint retains the actual chapter caption");
            expect_window_text(14,"L/R JOB U/D STOPS","ferry hint retains offer and itinerary navigation");
            expect_game_unchanged(&before);expect_text_screen_safe();
            expect(td_board_route==1,"ferry budget does not move the itinerary cursor");
        }
    }
    expect(island_offers==9,"exact nine preserved Island offers receive ferry budgets");
    reset_case();td.mode=TD_BOARD;td.job=TD_NONE;td_get_job(7,&td_offer);td_board_route=0;
    td_get_stop(td_offer.route[0],&td_cursor);
    for(unsigned invalid=TD_QUESTS;invalid<=255;invalid++){
        td.menu=invalid;game_snapshot_t before=snapshot_game();td_ui_draw();
        expect_window_text(17,"PAUSE FREEZES CLOCK","invalid offer indices never infer ferry budgets from a stale Island offer");
        expect_game_unchanged(&before);expect_text_screen_safe();
    }
    td.menu=7;td_get_job(0,&td_offer);td_get_stop(td_offer.route[0],&td_cursor);
    game_snapshot_t before=snapshot_game();td_ui_draw();
    expect_window_text(17,"PAUSE FREEZES CLOCK","a mismatched non-Island offer keeps the ordinary pause cue");
    expect_game_unchanged(&before);
}

static void test_island_no_fare_guidance(void){
    for(unsigned island=0;island<2;island++)for(unsigned active=0;active<2;active++)
    for(unsigned cash=0;cash<8;cash++)for(unsigned local=0;local<2;local++){
        reset_case();td.mode=TD_ROAM;td.msg=4;td.onfoot=1;td.cash=cash;
        td.district=island?TD_DISTRICT_ISLANDS:TD_DISTRICT_CITY;td.job=active?7:TD_NONE;
        td_get_job(7,&td_job);td.stage=2;td.left=103;td.health=67;td.wanted=3;td_route_district=TD_NONE;
        td_get_stop(local?(island?25:0):(island?10:25),&td_target);
        game_snapshot_t before=snapshot_game();td_ui_draw();
        int rescue_cue=island&&active&&cash<4;
        expect_window_text(0,rescue_cue?"NO FARE: START MENU":"NO FARE MONEY",
                           "only an active low-cash Island notice exposes the cancellation recovery action");
        if(active){
            expect_foot_health();
            if(rescue_cue)expect_window_text(2,"CANCEL JOB TO RETURN","the eligible low-cash Island notice keeps its exact cancellation action");
            else expect_compact_status(2,1);
        }else{
            expect_foot_health();expect_compact_status(2,0);
        }
        expect_game_unchanged(&before);expect_three_row_hud_safe();
        td.msg=0;before=snapshot_game();td_ui_draw();
        expect_window_text(2,active?(local?td_target.name:island?"RETURN FERRY AT DOCK":"GO TO FERRY TERMINAL"):
                           island?"A BOAT / B FERRY":"A/B CAR A DOOR/BOAT",
                           "clearing no-fare guidance restores the exact ordinary objective/control text");
        expect_game_unchanged(&before);
    }
    /* Unrelated notices and genuine booking/WAIT/paid-RIDE states must not
       gain cancellation advice solely from an active job and empty wallet. */
    reset_case();td.job=7;td.district=TD_DISTRICT_ISLANDS;td.cash=0;td.mode=TD_ROAM;
    td_get_job(7,&td_job);td_get_stop(25,&td_target);
    static const UBYTE notices[]={1,5,19};
    static const char *const text[]={"STOP TO INTERACT","VEHICLE HIT: RECOVER","HUMAN HIT: FINE + H"};
    for(unsigned i=0;i<sizeof(notices);i++){
        td.msg=notices[i];game_snapshot_t before=snapshot_game();td_ui_draw();
        expect_window_text(0,text[i],"active Island notices retain the appropriate safety/penalty text without fare advice");
        expect_compact_status(2,1);expect_foot_health();
        expect_game_unchanged(&before);
    }
    td.msg=4;td.transit_origin=20;td.transit_target=10;td.seconds=9;td_get_stop(10,&td_cursor);
    for(unsigned mode=TD_TRANSIT;mode<=TD_RIDE;mode++){
        td.mode=mode;td.ride_left=5;game_snapshot_t before=snapshot_game();td_ui_draw();
        if(mode==TD_TRANSIT){
            expect_window_text(7,"RIDE 8 SEC / $4","active Island booking still quotes the actual unchanged fare and duration");
            expect_window_text(11,"","active booking still has no no-job return assistance");
        }else if(mode==TD_WAIT){
            expect_window_text(0,"DEPARTS IN 28 SEC","active WAIT keeps its real autonomous departure countdown");
            expect_window_text(1,"ISLAND FERRY","active WAIT keeps its service identity");
            expect_window_text(2,"B CANCEL WAIT","active WAIT retains its existing unpaid cancellation control");
        }else{
            expect_window_text(0,"RIDING 5 SEC","genuine paid RIDE keeps its booked remaining time");
            expect_window_text(2,"FARE PAID / ON TIME","a paid RIDE never reclassifies its fare from current empty cash");
        }
        expect_game_unchanged(&before);
        if(mode==TD_TRANSIT)expect_text_screen_safe();
        else expect_three_row_hud_safe();
    }
}

static void test_every_viewport(void) {
    reset_case();open_case();game_snapshot_t before=snapshot_game();unsigned viewports=0;
    verify_marker_patterns();
    /* Drive the unchanged private paint routine at each legal viewport origin.
     * Public control behavior is independently exercised below. */
    for(unsigned y=0;y+12<=TD_ATLAS_TILE_HEIGHT;y++)for(unsigned x=0;x+20<=TD_ATLAS_TILE_WIDTH;x++) {
        td_map_x=x;td_map_y=y;td_map_begin();td_map_headers();
        unsigned uploads_before=ground_uploads;
        for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)
            expect(td_map_cache_get(i)==65535,"every viewport begins with all172sparse table slots empty");
        for(unsigned row=0;row<12;row++) {
            UBYTE cache_before[sizeof(td_ui_cache)];memcpy(cache_before,&td_ui_cache,sizeof(cache_before));
            td_ui_draw();
            expect(!memcmp(cache_before,&td_ui_cache,sizeof(cache_before)),
                   "map header redraws never overwrite the shared active pattern dictionary cache");
            td_map_update(0,0);
        }
        expect(td_map_row==12&&!td_map_error,"every legal viewport finishes real cached rendering");
        expect(ground_uploads-uploads_before==td_map_count,"each viewport uploads each distinct ground pattern exactly once");
        verify_viewport();viewports++;
    }
    expect(viewports==(TD_ATLAS_TILE_WIDTH-19)*(TD_ATLAS_TILE_HEIGHT-11),
           "fixture renders every actual legal twenty-by-twelve atlas viewport");
    unsigned resets_before=light_resets;
    expect_game_unchanged(&before);td_map_close();
    expect(light_resets==resets_before+1,"closing the atlas invalidates native signal pattern residency");
    for(unsigned i=0;i<sizeof(td_ui_cache);i++)
        expect(((UBYTE*)&td_ui_cache)[i]==255,"close invalidates every byte of the270-byte text/pattern union cache");
    td.mode=TD_PAUSE;unsigned writes=window_writes;td_ui_draw();
    expect(window_writes>writes,"return from atlas repaints actual text despite reuse of the pattern cache union");
}

static void expected_centre(UBYTE district,UWORD u,UWORD v,UBYTE *x,UBYTE *y) {
    expect(district<sizeof(host_ui_atlas_origins)/sizeof(host_ui_atlas_origins[0])&&u<1024&&v<976,
           "focus fixture uses a valid independently authored local atlas position");
    int left=(host_ui_atlas_origins[district][0]+u)/64-10;
    int top=(host_ui_atlas_origins[district][1]+v)/64-6;
    if(left<0)left=0;
    if(top<0)top=0;
    if(left>HOST_UI_ATLAS_TILE_WIDTH-20)left=HOST_UI_ATLAS_TILE_WIDTH-20;
    if(top>HOST_UI_ATLAS_TILE_HEIGHT-12)top=HOST_UI_ATLAS_TILE_HEIGHT-12;
    *x=left;*y=top;
}

static void expect_focus(UBYTE focus,UBYTE district,UWORD u,UWORD v,const char *name) {
    UBYTE x,y;expected_centre(district,u,v,&x,&y);
    expect(td_map_focus==focus&&td_map_x==x&&td_map_y==y,name);
}

static void test_focus_and_partial_restart(void) {
    reset_case();open_case();game_snapshot_t before=snapshot_game();
    expect_focus(0,0,560,720,"map opens centred on current player in the loaded district");
    td_map_update(0,0);td_map_update(0,0);expect(td_map_row==2,"partial focus fixture really paints only two rows");
    td_map_update(J_SELECT,J_SELECT);
    expect_focus(1,1,400,528,"walking Select focuses a remotely parked vehicle in its owning district");
    expect(td_map_row==1,"changing focus during partial painting restarts with exactly one freshly painted row");
    finish_paint();verify_viewport();
    td_map_update(J_SELECT,J_SELECT);
    expect_focus(2,3,320,144,"walking Select next focuses the current actual eastern walking client");
    finish_paint();verify_viewport();
    td_map_update(J_SELECT,J_SELECT);expect_focus(0,0,560,720,"walking Select cycles back to current player");
    td_map_update(J_A,J_A);expect_focus(2,3,320,144,"A focuses the current objective even during partial drawing");
    finish_paint();verify_viewport();expect_game_unchanged(&before);td_map_close();

    reset_case();td.onfoot=0;td_target.u=224;td_target.v=144;open_case();before=snapshot_game();
    td_map_update(J_SELECT,J_SELECT);
    expect_focus(2,3,224,144,"driving Select skips the redundant current-car view and focuses the legal road parking anchor");
    finish_paint();verify_viewport();td_map_update(J_SELECT,J_SELECT);
    expect_focus(0,0,560,720,"driving Select alternates directly from road objective to player");
    expect_game_unchanged(&before);td_map_close();

    reset_case();td.onfoot=0;td.job=TD_NONE;td_target.district=0;td_target.u=560;td_target.v=720;
    strcpy(td_target.name,"COURIER DEPOT");open_case();td_map_update(J_A,J_A);finish_paint();
    char text[21];read_window_text(14,text);
    expect(strstr(text,"O DEPOT")!=NULL&&strstr(text,"JOB")==NULL,"free-roam atlas legend explicitly labels the depot objective");
    read_window_text(15,text);expect(strstr(text,"DEPOT:")!=NULL,"free-roam objective focus names the depot");
    read_window_text(17,text);expect(strstr(text,"A DEPOT")!=NULL,"completed free-roam atlas controls explicitly offer depot focus");
    verify_viewport();td_map_close();
}

static void test_panning_and_bounds(void) {
    reset_case();open_case();finish_paint();game_snapshot_t before=snapshot_game();
    UBYTE x=td_map_x,y=td_map_y;
    td_map_update(J_LEFT|J_RIGHT|J_UP|J_DOWN,0);
    expect(td_map_x==x&&td_map_y==y&&td_map_row==12,"opposing held pan directions cancel on both axes");
    unsigned pan_steps=TD_ATLAS_TILE_WIDTH>100?(TD_ATLAS_TILE_WIDTH+1)/2:50;
    if(pan_steps<TD_ATLAS_TILE_HEIGHT)pan_steps=TD_ATLAS_TILE_HEIGHT;
    for(unsigned i=0;i<pan_steps;i++){td_map_update(J_LEFT|J_UP,0);finish_paint();}
    expect(td_map_x==0&&td_map_y==0,"repeated diagonal panning clamps exactly at the northwest atlas bounds");
    verify_viewport();
    for(unsigned i=0;i<pan_steps;i++){td_map_update(J_RIGHT|J_DOWN,0);finish_paint();}
    expect(td_map_x==TD_ATLAS_TILE_WIDTH-20&&td_map_y==TD_ATLAS_TILE_HEIGHT-12,
           "repeated diagonal panning clamps exactly at the southeast padded atlas bounds");
    verify_viewport();
    for(unsigned i=0;i<pan_steps;i++){td_map_update(J_LEFT|J_DOWN,0);finish_paint();}
    expect(td_map_x==0&&td_map_y==TD_ATLAS_TILE_HEIGHT-12,
           "horizontal motion never underflows the atlas while bottom edge remains clamped");
    for(unsigned i=0;i<pan_steps;i++){td_map_update(J_RIGHT|J_UP,0);finish_paint();}
    expect(td_map_x==TD_ATLAS_TILE_WIDTH-20&&td_map_y==0,
           "horizontal motion never overflows the atlas while top edge remains clamped");
    td_map_update(J_A,J_A);x=td_map_x;y=td_map_y;
    expect(td_map_row==1,"focus begins a genuine partial paint for held-input gating");
    td_map_update(J_LEFT|J_UP,0);
    expect(td_map_x==x&&td_map_y==y&&td_map_row==2,"held panning waits until the current atlas viewport has fully painted");
    finish_paint();verify_viewport();expect_game_unchanged(&before);td_map_close();
}

static void test_appended_district_focus_and_holes(void) {
    if(TD_DISTRICT_COUNT==4)return;
    for(UBYTE district=4;district<TD_DISTRICT_COUNT;district++) {
        reset_case();td.district=district;td.u=480*16;td.v=700*16;
        td_target.district=district;td_target.u=512;td_target.v=512;
        open_case();game_snapshot_t before=snapshot_game();finish_paint();verify_viewport();
        expect_focus(0,district,480,700,"appended scene centres and renders its player marker without shifting earlier districts");
        td_map_update(J_A,J_A);finish_paint();verify_viewport();
        expect_focus(2,district,512,512,"appended scene objective focus uses its actual two-dimensional atlas origin");
        expect_game_unchanged(&before);td_map_close();
    }
    for(unsigned y=0;y+12<=TD_ATLAS_TILE_HEIGHT;y++)for(unsigned x=0;x+20<=TD_ATLAS_TILE_WIDTH;x++) {
        char district_name[19];
        if(td_atlas_district((x+10)*8,(y+6)*8,district_name))continue;
        reset_case();open_case();game_snapshot_t before=snapshot_game();
        td_map_x=x;td_map_y=y;td_map_begin();td_map_headers();finish_paint();verify_viewport();
        char text[21];read_window_text(1,text);
        expect(strstr(text,"CITY EDGE")!=NULL,"a sparse atlas hole renders the visible city-edge header");
        expect_game_unchanged(&before);td_map_close();return;
    }
}

static void test_paid_transit_objective_context(void) {
    const UBYTE modes[]={TD_WAIT,TD_RIDE};
    for(unsigned i=0;i<sizeof(modes);i++)for(unsigned active=0;active<2;active++) {
        reset_case();td_resume_mode=modes[i];td.job=active?84:TD_NONE;td.onfoot=1;
        td_cursor.district=0;td_cursor.u=336;td_cursor.v=288;strcpy(td_cursor.name,"WELLESLEY STATION");
        td_target.district=3;td_target.u=320;td_target.v=144;strcpy(td_target.name,"WITHROW PARK");
        open_case();game_snapshot_t before=snapshot_game();td_map_update(J_A,J_A);finish_paint();
        char text[21];read_window_text(14,text);
        if(active) {
            expect_focus(2,3,320,144,"paid ride with an active parcel keeps focus on the actual client objective");
            expect(strstr(text,"O JOB")!=NULL,"paid active contract retains the job legend");
            read_window_text(15,text);expect(strstr(text,"JOB: WITHROW PARK")!=NULL,"paid active contract still names its true client");
            read_window_text(17,text);expect(strstr(text,"A JOB")!=NULL,"paid active contract still offers job focus");
        }else {
            expect_focus(2,0,336,288,"free WAIT/RIDE focuses the actually booked transit destination rather than the depot");
            expect(strstr(text,"O STOP")!=NULL&&strstr(text,"DEPOT")==NULL,"free paid trip explicitly labels its stop objective");
            read_window_text(15,text);expect(strstr(text,"TRIP: WELLESLEY")!=NULL,"free paid trip names the booked station");
            read_window_text(17,text);expect(strstr(text,"A STOP")!=NULL,"free paid trip offers stop focus");
        }
        verify_viewport();expect_game_unchanged(&before);td_map_close();
    }
}

static void test_interrupt_restore_and_idempotence(void) {
    const UBYTE lengths[]={0,1,TD_ACTORS,21,23,255};
    for(unsigned hidden=0;hidden<3;hidden++)for(unsigned scenario=0;scenario<sizeof(lengths);scenario++) {
        reset_case();actors_len=lengths[scenario];actor_t original[22];memcpy(original,actors,sizeof(actors));
        for(unsigned i=0;i<22;i++) {
            actors[i].flags=(actors[i].flags&~ACTOR_FLAG_HIDDEN)|
                (hidden==1||(hidden==2&&(i&1))?ACTOR_FLAG_HIDDEN:0);
        }
        memcpy(original,actors,sizeof(actors));
        UWORD saved_x=camera_x,saved_y=camera_y;UBYTE saved_settings=camera_settings;
        open_case();game_snapshot_t before=snapshot_game();
        unsigned count=actors_len<TD_ACTORS?actors_len:TD_ACTORS;
        for(unsigned i=0;i<22;i++)expect(actors[i].flags==(i<count?original[i].flags|ACTOR_FLAG_HIDDEN:original[i].flags),
                                        "map open hides only the bounded actual actor list");
        td_map_update(0,0);td_map_update(0,0);
        UBYTE before_row=td_map_row,before_count=td_map_count;unsigned uploads=tile_uploads;
        camera_x=111;camera_y=222;camera_settings=63;
        td_map_open();
        expect(td_map_row==before_row&&td_map_count==before_count&&tile_uploads==uploads,
               "repeated map open preserves partial paint and does not reload marker or ground tiles");
        for(unsigned i=0;i<22;i++)actors[i].flags^=0x20;
        UBYTE flags_before_close[22];for(unsigned i=0;i<22;i++)flags_before_close[i]=actors[i].flags;
        /* Driver handles B/Start by invoking this production close before
         * changing modes. The driver path itself is tested in test_engine.py. */
        td_map_close();
        expect(camera_x==saved_x&&camera_y==saved_y&&camera_settings==saved_settings&&VBK_REG==0,
               "partial-paint close restores exact camera words/settings and neutral VRAM bank");
        for(unsigned i=0;i<22;i++) {
            UBYTE expected=i<count?(flags_before_close[i]&~ACTOR_FLAG_HIDDEN)|(original[i].flags&ACTOR_FLAG_HIDDEN):flags_before_close[i];
            expect(actors[i].flags==expected,"map close restores only the prior hidden bit and retains other actor flag changes");
            expect(actors[i].pos.x==original[i].pos.x&&actors[i].pos.y==original[i].pos.y,
                   "map never moves native actors during partial paint or cancellation");
        }
        expect_game_unchanged(&before);
        for(unsigned i=0;i<sizeof(td_ui_cache);i++)expect(((UBYTE*)&td_ui_cache)[i]==255,"partial close invalidates the entire union cache");
        camera_x=333;camera_y=444;camera_settings=17;actors[0].flags^=ACTOR_FLAG_HIDDEN;
        UBYTE after=actors[0].flags;uploads=tile_uploads;unsigned writes=window_writes;
        td_map_close();td_map_update(J_A|J_SELECT|J_LEFT,J_A|J_SELECT);
        expect(camera_x==333&&camera_y==444&&camera_settings==17&&actors[0].flags==after&&
               tile_uploads==uploads&&window_writes==writes,
               "idempotent close and inactive update cannot restore stale camera/flags or render tiles");
    }
}

static void test_error_recovery_and_repeated_sessions(void) {
    for(unsigned error=0;error<2;error++) {
        reset_case();open_case();game_snapshot_t before=snapshot_game();
        UWORD saved_x=td_map_camera_x,saved_y=td_map_camera_y;UBYTE settings=td_map_camera_settings;
        td_map_begin();
        /* Actual API rejects this deliberately invalid private row fixture;
         * valid generated origins never produce it. The second fixture tests
         * the renderer's bounded capacity guard without altered atlas data. */
        if(!error)td_map_x=255;
        else {td_map_count=TD_ATLAS_VISIBLE_LIMIT;for(unsigned i=0;i<td_map_count;i++)td_map_cache_set(i,65535);}
        unsigned uploads=tile_uploads;td_map_update(0,0);
        expect(td_map_error&&td_map_row==12,"real renderer safely enters its error UI for bad rows or a saturated dictionary");
        expect(tile_uploads==uploads&&VBK_REG==0,"renderer error cannot upload an out-of-range pattern or leave another VRAM bank selected");
        char text[21];read_window_text(17,text);expect(strstr(text,"MAP ERROR")!=NULL,"renderer error visibly retains its back action");
        td_map_close();expect(camera_x==saved_x&&camera_y==saved_y&&camera_settings==settings,
                              "error recovery still restores exact captured camera state");
        expect_game_unchanged(&before);
    }
    reset_case();
    for(unsigned session=0;session<3;session++) {
        camera_x=1000+session*31;camera_y=2000+session*37;camera_settings=17+session;
        UWORD saved_x=camera_x,saved_y=camera_y;UBYTE settings=camera_settings;
        open_case();td_map_update(J_A,J_A);finish_paint();verify_viewport();td_map_close();
        expect(camera_x==saved_x&&camera_y==saved_y&&camera_settings==settings,
               "successive real map sessions each restore their own captured camera rather than an earlier session");
        td.mode=TD_PAUSE;td_ui_draw();
    }
}

static void test_overlap_marker_geometry(void) {
    for(UBYTE bits=1;bits<8;bits++) {
        reset_case();td.onfoot=1;td.district=td.park_district=td_target.district=0;
        td.u=(bits&1?560:208)*16;td.v=(bits&1?400:64)*16;
        td.park_u=(bits&2?560:80)*16;td.park_v=(bits&2?400:176)*16;
        td_target.u=bits&4?560:944;td_target.v=bits&4?400:784;
        open_case();expected_centre(0,560,400,&td_map_x,&td_map_y);
        td_map_begin();td_map_headers();finish_paint();
        UWORD x,y;expect(td_atlas_position(0,560,400,&x,&y),"overlap fixture target has a genuine atlas position");
        expect(x/8>=td_map_x&&x/8<td_map_x+20&&y/8>=td_map_y&&y/8<td_map_y+12,
               "overlap fixture lies inside the painted ground viewport");
        expect(window_tiles[0][2+y/8-td_map_y][x/8-td_map_x]==7+bits,
               "all seven real marker overlap combinations map to their distinct reserved glyph tile");
        verify_viewport();verify_marker_patterns();td_map_close();
    }
}

static void test_sparse_table_full_and_single_holes(void) {
    const UBYTE cases[][3]={{0,0,0},{5,0,0},{14,0,0},{21,0,0},
                            {31,0,0},{32,1,5},{13,3,5},{44,4,11}};
    for(unsigned fixture=0;fixture<sizeof(cases)/sizeof(cases[0]);fixture++) {
        UWORD row[20];UBYTE pattern[16];
        expect(td_atlas_row(cases[fixture][0],cases[fixture][1]+cases[fixture][2],20,row),
               "adversarial lookup fixture uses a real registered atlas row");
        expect(td_atlas_pattern(row[0],pattern),"adversarial lookup fixture uses real native pattern bytes");
        int distinct=0;for(unsigned i=1;i<20;i++)if(row[i]!=row[0])distinct=1;
        for(unsigned hole=0;hole<TD_ATLAS_VISIBLE_LIMIT;hole++) {
            reset_case();open_case();td_map_x=cases[fixture][0];td_map_y=cases[fixture][1];
            td_map_row=cases[fixture][2];td_map_count=TD_ATLAS_VISIBLE_LIMIT-1;
            /* Deliberately force congestion with synthetic occupied IDs that
             * cannot match any actual pattern. No hash/home/stride calculation
             * is mirrored: each possible sole empty slot must be reachable. */
            for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)td_map_cache_set(i,TD_ATLAS_PATTERNS+i);
            td_map_cache_set(hole,65535);
            game_snapshot_t before=snapshot_game();unsigned uploads=ground_uploads;
            td_map_paint_row();
            expect(td_map_cache_get(hole)==row[0]&&td_map_count==TD_ATLAS_VISIBLE_LIMIT,
                   "actual lookup reaches every possible sole empty slot even after adversarial collisions and wraparound");
            expect(ground_uploads==uploads+1&&!memcmp(vram[1][16+hole],pattern,16),
                   "single-hole insertion uploads the correct real pattern exactly once to its stable bounded slot");
            for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)if(i!=hole)
                expect(td_map_cache_get(i)==TD_ATLAS_PATTERNS+i,"collision probing never overwrites an occupied cache slot");
            if(distinct)expect(td_map_error&&td_map_row==12,"a second distinct row pattern terminates safely when the table has become full");
            else expect(!td_map_error,"an already inserted row pattern remains retrievable after the table becomes full");
            expect_game_unchanged(&before);td_map_close();
        }
        reset_case();open_case();td_map_x=cases[fixture][0];td_map_y=cases[fixture][1];td_map_row=cases[fixture][2];
        td_map_count=TD_ATLAS_VISIBLE_LIMIT;
        for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)td_map_cache_set(i,TD_ATLAS_PATTERNS+i);
        UBYTE before[sizeof(td_ui_cache.patterns)];memcpy(before,td_ui_cache.patterns,sizeof(before));
        unsigned uploads=ground_uploads;td_map_paint_row();
        expect(td_map_error&&td_map_row==12&&td_map_count==TD_ATLAS_VISIBLE_LIMIT,
               "a completely occupied table with no match stops bounded probing in the real error path");
        expect(ground_uploads==uploads&&!memcmp(before,td_ui_cache.patterns,sizeof(before)),
               "an exhausted probe does not upload, commit or evict any occupied pattern");
        expect(VBK_REG==0,"exhausted sparse-table probing returns to neutral VRAM bank");td_map_close();
    }
}

static void test_walking_vehicle_hit_caption(void) {
    for(UBYTE carrying=0;carrying<2;carrying++){
        reset_case();td.mode=TD_ROAM;td.onfoot=1;td.msg=5;
        if(carrying){td.job=0;td.stage=1;td_job.count=2;}
        game_snapshot_t before=snapshot_game();
        td_ui_draw();
        expect_window_text(0,"VEHICLE HIT: RECOVER","on-foot vehicle impact explains recovery with and without cargo");
        expect_game_unchanged(&before);
    }
}

static void test_live_objective_arrows(void){
    static const int offsets[4][2]={{0,-80},{80,0},{0,80},{-80,0}};
    reset_case();td.mode=TD_ROAM;td.job=0;td.msg=0;td_target.district=td.district;
    strcpy(td_target.name,"COURIER DEPOT");actors[1].flags=0;
    for(unsigned direction=0;direction<4;direction++){
        actors[1].pos.x=((td.u>>4)+offsets[direction][0])*32;
        actors[1].pos.y=((td.v>>4)+offsets[direction][1]-12)*32;
        game_snapshot_t before=snapshot_game();td_ui_draw();
        expect(window_tiles[0][2][0]==241+direction,
               "normal gameplay shows the independently expected north/east/south/west objective arrow");
        expect(window_tiles[0][2][1]==192,"graphic guidance separates the arrow from the objective name");
        for(unsigned i=0;i<13;i++)expect(window_tiles[0][2][i+2]==td_glyph("COURIER DEPOT"[i]),
                                         "live arrow preserves the complete named objective");
        expect_three_row_hud_safe();expect_game_unchanged(&before);
    }
    td_target.district=TD_DISTRICT_WEST;td_route_district=TD_DISTRICT_WEST;
    actors[1].pos.x=((td.u>>4)-160)*32;actors[1].pos.y=((td.v>>4)-12)*32;
    td_ui_draw();expect(window_tiles[0][2][0]==244,
                       "remote objective arrows follow the loaded-scene portal beacon rather than remote map coordinates");
    actors[1].flags=ACTOR_FLAG_HIDDEN;td_ui_draw();
    expect(window_tiles[0][2][0]>=192&&window_tiles[0][2][0]<=240,
           "unreachable or unavailable objective beacons never invent a directional route");
    actors[1].flags=0;td_target.district=td.district;
    actors[1].pos.x=td.u*2;actors[1].pos.y=td.v*2-12*32;
    td_ui_draw();expect(window_tiles[0][2][0]==233,"arrival replaces directional guidance with the original interaction plus glyph");
    for(unsigned arrow=0;arrow<4;arrow++)
        expect(!memcmp(vram[1][241+arrow],td_arrows+arrow*16,16),"all four graphic arrows are uploaded to their own reserved native patterns");
}

static void test_compact_navigation_and_stars(void){
    static const int offset[8][2]={{0,-80},{80,0},{0,80},{-80,0},{80,-80},{80,80},{-80,80},{-80,-80}};
    reset_case();td.mode=TD_ROAM;td.job=0;td.left=93;td.wanted=2;td.wanted_left=30;td.msg=0;
    strcpy(td_target.name,"MARKET PICKUP");td_target.district=td.district;actors[1].flags=0;
    for(unsigned i=0;i<8;i++){
        actors[1].pos.x=((td.u>>4)+offset[i][0])*32;
        actors[1].pos.y=((td.v>>4)+offset[i][1]-12)*32;
        game_snapshot_t before=snapshot_game();td_ui_draw();
        expect(window_y==(td.onfoot?128:136)&&window_x==0,"driving preserves136 native city rows; armed walking uses a16px health strip");
        expect(window_tiles[0][0][0]==241+i,"each of eight independent target quadrants has its visible graphic direction");
        for(unsigned c=0;c<9;c++)expect(window_tiles[0][0][2+c]==td_glyph("MARKET PICKUP"[c]),
            "the visible strip preserves a readable short objective beside its arrow");
        expect_compact_status(0,1);expect_game_unchanged(&before);
    }
    td.msg=19;td_ui_draw();expect(window_y==(td.onfoot?120:128),"collision notices add one row above the walking health strip or driving objective");
    expect_window_text(0,"HUMAN HIT: FINE + H","the transient notice stays complete above the compact objective");
    expect_compact_status(td.onfoot?2:1,1);expect_foot_health();
    td.msg=0;td.wanted_left=29;td.seconds=1;td_ui_draw();expect_compact_status(0,1);
    td.seconds=2;td_ui_draw();expect_compact_status(0,1);
    td.wanted=td.wanted_left=0;td.job=TD_NONE;td.cash=60000;td_ui_draw();expect_compact_status(0,0);
    expect(window_y==(td.onfoot?128:136),"idle driving keeps8px HUD while walking exposes separate health/ammo");
    static const UBYTE gold_star[16]={0,0x10,0,0x38,0,0xfe,0,0x7c,0,0x38,0,0x6c,0,0x44,0,0};
    static const UBYTE mint_outline[16]={0x10,0,0x28,0,0xc6,0,0x44,0,0x28,0,0x54,0,0x44,0,0,0};
    expect(!memcmp(vram[1][249],gold_star,16)&&!memcmp(vram[1][250],mint_outline,16),
        "attention stars retain the independently authored gold fill and mint outline pixel planes");
}

/* Bit-at-a-time oracle deliberately shares no byte grouping, shift/index
 * arithmetic or mask expressions with the production codecs. */
static unsigned cache_bits_get(const UBYTE *bytes,unsigned first,unsigned width){
    unsigned value=0;
    for(unsigned bit=0;bit<width;bit++)if(bytes[(first+bit)/8]&(1u<<((first+bit)%8)))value|=1u<<bit;
    return value;
}
static void cache_bits_set(UBYTE *bytes,unsigned first,unsigned width,unsigned value){
    for(unsigned bit=0;bit<width;bit++){
        unsigned index=(first+bit)/8,mask=1u<<((first+bit)%8);
        bytes[index]=(bytes[index]&~mask)|((value&(1u<<bit))?mask:0);
    }
}
static void test_packed_row_codec(void){
    struct {UBYTE before[4],row[15],after[4];} guarded;
    UBYTE logical[20],expected[15],tiles[20];
    expect(sizeof(td_ui_cache)==270&&sizeof(td_cached_rows)==270&&sizeof(td_ui_cache.patterns)==215,
        "native cache union is exactly270 bytes with eighteen15-byte rows and172 ten-bit atlas slots");
    expect(sizeof(td_map_hidden)==3,
        "all22 native actor hidden flags occupy exactly three bytes with no actor/save-state growth");
    for(unsigned column=0;column<20;column++)for(unsigned glyph=192;glyph<=255;glyph++){
        memset(&guarded,0xA5,sizeof(guarded));memset(guarded.row,255,sizeof(guarded.row));
        memset(logical,255,sizeof(logical));memset(expected,255,sizeof(expected));
        for(unsigned i=0;i<20;i++)tiles[i]=192+(i*7+column)%61;
        tiles[column]=glyph;
        for(unsigned step=0;step<3;step++){
            UBYTE changed=memcmp(logical,tiles,sizeof(logical))!=0;
            memcpy(logical,tiles,sizeof(logical));
            for(unsigned i=0;i<20;i++)cache_bits_set(expected,i*6,6,logical[i]-192);
            expect(td_row_cache_apply(guarded.row,tiles)==changed,
                "packed text cache reports exactly the original twenty-byte logical row change/no-op result");
            expect(!memcmp(expected,guarded.row,sizeof(expected)),
                "every glyph at every column has the independent exact six-bit wire encoding");
            for(unsigned i=0;i<20;i++)expect(cache_bits_get(guarded.row,i*6,6)+192==logical[i],
                "six-bit row fields preserve every neighboring glyph across all byte/group boundaries");
            for(unsigned i=0;i<4;i++)expect(guarded.before[i]==0xA5&&guarded.after[i]==0xA5,
                "text codec cannot overwrite either surrounding guard region including its last glyph");
            if(step==1)tiles[(column+1)%20]=192+(tiles[(column+1)%20]-191)%61;
        }
    }
    /* An invalidated cache is all63; no actual character can generate that
     * code. Exercise every signed/unsigned char input through the real mapper. */
    for(unsigned character=0;character<256;character++){
        UBYTE glyph=td_glyph((char)character);
        expect(glyph>=192&&glyph<=252&&glyph-192!=63,
            "all256 character inputs map to real six-bit native glyphs distinct from the empty sentinel");
    }
    for(unsigned y=0;y<18;y++){
        memset(td_cached_rows,255,sizeof(td_cached_rows));
        UBYTE logical_cache[18][20];memset(logical_cache,255,sizeof(logical_cache));td.mode=TD_ROAM;
        static const char text[]="A\1B\2C\3D\4E\5F\6G\7H\10I\11J\12";
        for(unsigned step=0;step<2;step++){
            UBYTE expected_tiles[20];for(unsigned i=0;i<20;i++)expected_tiles[i]=i&1?241+i/2:193+i/2;
            unsigned changed=memcmp(logical_cache[y],expected_tiles,20)!=0,writes=window_writes;
            memcpy(logical_cache[y],expected_tiles,20);td_row(y,text);
            expect(window_writes==writes+changed&&!memcmp(window_tiles[0][y],expected_tiles,20),
                "each actual text row preserves original upload bytes and suppresses the same cached no-op write");
            for(unsigned row=0;row<18;row++)for(unsigned column=0;column<20;column++){
                unsigned code=cache_bits_get(td_cached_rows[row],column*6,6);
                expect((code==63?255:code+192)==logical_cache[row][column],
                    "real row updates preserve the original logical values of all eighteen rows including empty neighbors");
            }
        }
    }
}
static void test_packed_atlas_codec(void){
    UBYTE original[sizeof(td_ui_cache)],expected[sizeof(td_ui_cache)];
    for(unsigned slot=0;slot<TD_ATLAS_VISIBLE_LIMIT;slot++){
        memset(&td_ui_cache,0xA5,sizeof(td_ui_cache));
        memset(td_ui_cache.patterns,255,sizeof(td_ui_cache.patterns));
        memcpy(expected,&td_ui_cache,sizeof(expected));
        for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++){
            unsigned value=(i*977+slot*13)%1023;
            td_map_cache_set(i,value);cache_bits_set(expected,i*10,10,value);
        }
        expect(!memcmp(&td_ui_cache,expected,sizeof(expected)),
            "all ten-bit atlas fields match independent bit encoding while retaining the union's inactive tail");
        for(unsigned value=0;value<=1023;value++){
            UWORD logical=value==1023?65535:value;
            td_map_cache_set(slot,logical);cache_bits_set(expected,slot*10,10,value);
            expect(td_map_cache_get(slot)==logical,
                "all1023 dictionary/fixture IDs and the empty sentinel roundtrip at every one of172 slots");
            expect(!memcmp(&td_ui_cache,expected,sizeof(expected)),
                "each ten-bit insertion changes only its own exact bits and preserves neighbors and inactive union guard bytes");
            if(slot)expect(td_map_cache_get(slot-1)==(unsigned)((slot-1)*977+slot*13)%1023,
                "ten-bit writes cannot alias the previous sparse slot");
            if(slot+1<TD_ATLAS_VISIBLE_LIMIT)expect(td_map_cache_get(slot+1)==(unsigned)((slot+1)*977+slot*13)%1023,
                "ten-bit writes cannot alias the next sparse slot");
        }
    }
    memcpy(original,&td_ui_cache,sizeof(original));
    for(unsigned slot=TD_ATLAS_VISIBLE_LIMIT;slot<256;slot++){
        td_map_cache_set(slot,77);
        expect(td_map_cache_get(slot)==65535&&!memcmp(original,&td_ui_cache,sizeof(original)),
            "invalid eight-bit atlas slots return empty and cannot touch packed storage");
    }
    for(unsigned value=1023;value<65535;value++){
        td_map_cache_set(0,value);
        expect(!memcmp(original,&td_ui_cache,sizeof(original)),
            "out-of-range pattern IDs cannot truncate into valid dictionary entries or overwrite the sentinel");
    }
    memset(td_ui_cache.patterns,255,sizeof(td_ui_cache.patterns));
    for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)expect(td_map_cache_get(i)==65535,
        "the original all-FF reset empties all ten-bit sparse cache slots");
}
static void test_packed_actor_visibility(void){
    for(unsigned slot=0;slot<TD_ACTORS;slot++)for(unsigned flags=0;flags<256;flags++){
        reset_case();actors[slot].flags=flags;
        UBYTE original[TD_ACTORS];for(unsigned i=0;i<TD_ACTORS;i++)original[i]=actors[i].flags;
        td.mode=TD_MAP;td_map_open();
        for(unsigned i=0;i<TD_ACTORS;i++)expect(!!(td_map_hidden[i/8]&(1u<<(i%8)))==!!(original[i]&ACTOR_FLAG_HIDDEN),
            "every actor slot and every original flag byte retain their exact independent hidden bit in the packed pool");
        for(unsigned i=0;i<TD_ACTORS;i++)actors[i].flags^=0x20;
        td_map_close();
        for(unsigned i=0;i<TD_ACTORS;i++)expect(actors[i].flags==(original[i]^0x20),
            "map close restores each original hidden flag while preserving concurrent changes to every other flag");
    }
}

static void test_original_menu_cards(void){
    static const UBYTE cases[][2]={
        {TD_PAUSE,0},{TD_PAUSE,8},{TD_PAUSE,TD_SETTINGS_SOUND},
        {TD_PAUSE,TD_SETTINGS_CONTROLS},{TD_PAUSE,TD_SETTINGS_BACK},
        {TD_HELP,0},{TD_HELP,TD_SETTINGS_CONTROLS},{TD_BOARD,0},
        {TD_TRANSIT,0},{TD_RESULT,0}
    };
    for(unsigned page=0;page<sizeof(cases)/sizeof(cases[0]);page++){
        reset_case();td.mode=cases[page][0];td.menu=cases[page][1];
        td.job=TD_NONE;td.onfoot=0;td_get_job(0,&td_offer);td_get_stop(1,&td_cursor);
        td.transit_origin=0;td.transit_target=1;
        game_snapshot_t before=snapshot_game();td_ui_draw();
        expect_board_preserves_game(&before);
        unsigned frame_cells=0,flipped=0;
        for(unsigned y=0;y<18;y++)for(unsigned x=0;x<20;x++){
            unsigned kind=expected_frame(y),tile=window_tiles[0][y][x],attr=window_tiles[1][y][x];
            if(tile>=188&&tile<=190){
                frame_cells++;unsigned expected_attr=15|(x==19?32:0)|(kind==3?64:0);
                expect(kind&&attr==expected_attr,"every native border has the exact independently expected corner/rail/side flip and palette7/bank1 ownership");
                expect(tile==(x==0||x==19?(kind==1||kind==4?190:188):189),"all visible frame edges and all four corners use their original pattern role");
                flipped+=!!(attr&96);
            }else expect(tile>=192&&tile<=252&&attr==15,"all panel text and footer glyphs remain upright and outside the world/combat/hospital tiles");
        }
        expect(frame_cells>=32&&flipped>=8,"every full-screen menu is a visibly bordered original card rather than a text-only page");
        expect(!memcmp(vram[1][188],td_menu_frame_pixels,48),"all menus preserve the exact deliberately authored three-pattern pixels");
        expect(vram[1][253][0]==0xEE&&vram[1][254][0]==0xEE&&vram[1][255][0]==0xEE,
               "menu card uploads never claim combat253, hospital254 or the unused tail255");
        unsigned writes=window_writes,uploads=tile_uploads;td_ui_draw();
        expect(window_writes==writes&&tile_uploads==uploads,"an unchanged designed card performs no tilemap, attribute or pattern uploads");
        /* A menu change followed by world resume must not leave flipped
         * borders in the compact HUD, nor change its approved viewport. */
        td.mode=TD_ROAM;td.msg=0;td_ui_draw();
        expect(window_x==0&&window_y==136,"leaving a framed menu restores the original eight-pixel driving viewport strip");
        for(unsigned y=0;y<18;y++)for(unsigned x=0;x<20;x++)
            expect(window_tiles[1][y][x]==15,"every framed-menu flip is reset before any later gameplay HUD/map/dialogue upload");
        expect_three_row_hud_safe();
    }
    /* Welcome->guide->welcome keeps the same mode value but changes card
     * geometry. Invalidate that page once; never retain either page's text. */
    reset_case();td.mode=TD_HELP;td.menu=0;td_ui_draw();
    td.menu=TD_SETTINGS_CONTROLS;td_ui_draw();expect_window_text(11,"A/B ENTER/TAKE CAR","guide replaces the welcome objective arrow hint");
    td.menu=0;td_ui_draw();expect_window_text(11,"FOLLOW JOB ARROW","returning to welcome clears the former control-guide rows");

    /* Optional source-only render export: these are actual-C host tilemap
     * outputs, not emulator screenshots or native LCD/performance evidence. */
    const char *directory=getenv("TD_MENU_SOURCE_PREVIEW_DIR");
    if(directory){
        static const char *const names[]={"pause","settings","welcome","controls","dispatch","transit","result"};
        static const UBYTE pages[][2]={{TD_PAUSE,1},{TD_PAUSE,TD_SETTINGS_SOUND},{TD_HELP,0},{TD_HELP,TD_SETTINGS_CONTROLS},
                                    {TD_BOARD,0},{TD_TRANSIT,0},{TD_RESULT,0}};
        for(unsigned page=0;page<7;page++){
            reset_case();td.mode=pages[page][0];td.menu=pages[page][1];td.job=TD_NONE;
            td.health=100;td.left=108;td.cash=137;td.done=1;td.wanted=0;td.onfoot=0;
            td_get_job(0,&td_job);td_get_job(0,&td_offer);td_get_stop(1,&td_cursor);
            td.transit_origin=0;td.transit_target=1;td_ui_draw();
            char path[2048];snprintf(path,sizeof(path),"%s/%s.bin",directory,names[page]);
            FILE *file=fopen(path,"wb");expect(file!=NULL,"source-only menu preview output opens inside the explicitly requested ignored build directory");
            if(file){fwrite(window_tiles,1,sizeof(window_tiles),file);fwrite(vram[1],1,sizeof(vram[1]),file);fclose(file);}
        }
    }
}

void td_hospital_init(void){}

int main(void) {
    test_packed_row_codec();test_packed_atlas_codec();test_packed_actor_visibility();
    test_original_menu_cards();
    reset_case();boat_control_fixture=1;td.job=TD_NONE;td.mode=TD_ROAM;
    game_snapshot_t boat_controls_before=snapshot_game();td_ui_draw();
    expect_window_text(2,"A GAS DOWN+A DOCK","controlled boat guidance distinguishes acceleration from deliberate docking");
    expect_game_unchanged(&boat_controls_before);
    td.mode=TD_HELP;td.menu=0;boat_controls_before=snapshot_game();td_ui_draw();
    expect_window_text(2,"WELCOME COURIER","cold boot starts with a short beginner overview");
    expect_window_text(16,"A OR B: START GAME","the welcome screen explains how either action starts play");
    expect_game_unchanged(&boat_controls_before);
    td.menu=TD_SETTINGS_CONTROLS;boat_controls_before=snapshot_game();td_ui_draw();
    expect_window_text(14,"DOCK: DOWN+A EXIT","the Settings control guide teaches the actual deliberate boat exit chord");
    expect_game_unchanged(&boat_controls_before);boat_control_fixture=0;
    test_compact_navigation_and_stars();
    td.onfoot=1;td_map_focus=0;boat_control_fixture=1;td_map_headers();
    expect_window_text(15,"YOU ON A BOAT","the paused map correctly identifies a controlled boat passenger");
    boat_control_fixture=0;td_map_headers();
    expect_window_text(15,"YOU ON FOOT","disembarking restores the actual walking caption");
    test_live_objective_arrows();
    test_walking_vehicle_hit_caption();
    test_every_viewport();test_focus_and_partial_restart();test_panning_and_bounds();
    test_paid_transit_objective_context();test_interrupt_restore_and_idempotence();
    test_error_recovery_and_repeated_sessions();test_overlap_marker_geometry();
    test_sparse_table_full_and_single_holes();
    test_car_entry_hud_repaint();test_wait_contact_hud_and_map_restore();
    test_reserved_islands_assistance_ui();
    test_island_objective_hud();
    unsigned ferry_checks=checks,ferry_failures=failures;
    test_ferry_offer_budgets();test_island_no_fare_guidance();
    printf("Ferry clarity UI regressions: %u checks, %u failures.\n",checks-ferry_checks,failures-ferry_failures);
    test_appended_district_focus_and_holes();
    test_dispatch_board_itineraries();test_dispatch_active_board_captions();test_contract_payment_result();
    unsigned vehicle_checks=checks,vehicle_failures=failures;
    test_dispatch_vehicle_readiness();
    printf("Dispatch vehicle UI regressions: %u checks, %u failures.\n",checks-vehicle_checks,failures-vehicle_failures);
    test_pause_audio_labels_and_map_cache();
    test_pause_action_hints();
    printf("Atlas UI host regressions: %u checks, %u failures. Native raster/banking remains separate.\n",checks,failures);
    return failures?1:0;
}
