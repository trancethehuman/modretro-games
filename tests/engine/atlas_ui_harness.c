/* Actual renderer and generated atlas, with bounded host hardware adapters.
 * The API's native pixel data is independently verified by test_atlas.py.
 * Access to static routines sets coverage fixtures; production is unchanged. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui_host.h"
#include "td_game.h"
#include "td_atlas.h"
#include "atlas_under_test.c"
#include "ui_under_test.c"

td_state_t td;
td_job_t td_job,td_offer;
td_stop_t td_target,td_cursor;
UBYTE td_route_district;
UBYTE td_resume_mode;
UWORD td_streetcar_focus_u,td_streetcar_focus_v;
UBYTE td_streetcar_view_district,td_streetcar_ride_view;
actor_t actors[21];
UBYTE actors_len;
UWORD camera_x,camera_y;
UBYTE camera_settings,VBK_REG,text_drawn;

/* One window tilemap and two pattern banks, indexed through the public tile
 * API. Native LCDC address mapping and scanline effects are not simulated. */
static UBYTE window_tiles[2][18][20],vram[2][256][16];
static UBYTE window_x,window_y;
static unsigned checks,failures,window_writes,tile_uploads,ground_uploads;
static unsigned content_reads;
static UBYTE initial_font[49][16];

static void expect(int condition,const char *name) {
    checks++;
    if(!condition){failures++;if(failures<30)fprintf(stderr,"FAIL %s\n",name);}
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
    expect(marker||ground||font,"uploads stay within marker8..14, ground16..187 or font192..240 reservations");
    if(VBK_REG!=1||!tiles||!count||(unsigned)first+count>256)return;
    if(ground)ground_uploads+=count;
    memcpy(vram[VBK_REG][first],tiles,(size_t)count*16);
}

void ui_set_pos(UBYTE x,UBYTE y) {window_x=x;window_y=y;}
UBYTE td_audio_get_mode(void) {return TD_AUDIO_FULL;}
UBYTE td_service(UBYTE origin) {(void)origin;return 1;}
UBYTE td_next_departure(UBYTE origin,UWORD seconds) {(void)origin;(void)seconds;return 7;}
void td_get_street(UWORD u,UWORD v,char *dest) {(void)u;(void)v;content_reads++;strcpy(dest,"TEST ROAD");}
void td_get_district_name(UBYTE district,char *dest) {(void)district;content_reads++;strcpy(dest,"TEST DISTRICT");}
void td_get_brief(UBYTE index,char *dest) {(void)index;content_reads++;memset(dest,' ',36);dest[36]=0;}
void td_get_stop(UBYTE index,td_stop_t *dest) {
    (void)index;content_reads++;memset(dest,0,sizeof(*dest));strcpy(dest->name,"TEST STOP");
}

typedef struct {
    td_state_t state;td_job_t job,offer;td_stop_t target,cursor;
    UBYTE route,actor_count,resume_mode;
} game_snapshot_t;

static game_snapshot_t snapshot_game(void) {
    game_snapshot_t snapshot={td,td_job,td_offer,td_target,td_cursor,td_route_district,actors_len,td_resume_mode};
    return snapshot;
}

static void expect_game_unchanged(const game_snapshot_t *snapshot) {
    expect(!memcmp(&td,&snapshot->state,sizeof(td))&&!memcmp(&td_job,&snapshot->job,sizeof(td_job))&&
           !memcmp(&td_offer,&snapshot->offer,sizeof(td_offer))&&!memcmp(&td_target,&snapshot->target,sizeof(td_target))&&
           !memcmp(&td_cursor,&snapshot->cursor,sizeof(td_cursor))&&td_route_district==snapshot->route&&
           actors_len==snapshot->actor_count&&td_resume_mode==snapshot->resume_mode,
           "real map UI preserves serialized state, job, target, cursor, route cue and actor count");
}

static void reset_case(void) {
    memset(&td,0,sizeof(td));memset(&td_job,0,sizeof(td_job));memset(&td_offer,0,sizeof(td_offer));
    memset(&td_target,0,sizeof(td_target));memset(&td_cursor,0,sizeof(td_cursor));
    memset(actors,0,sizeof(actors));memset(window_tiles,0xEE,sizeof(window_tiles));memset(vram,0xEE,sizeof(vram));
    td.district=0;td.u=560*16;td.v=720*16;td.onfoot=1;
    td.park_district=1;td.park_u=400*16;td.park_v=528*16;td.cash=123;td.seconds=4321;
    td.left=199;td.health=100;td.job=84;td.stage=3;td.map_x=43210;td.map_y=32109;td.mode=TD_PAUSE;
    td_target.district=3;td_target.u=320;td_target.v=144;td_target.reserved=TD_STOP_FOOT;strcpy(td_target.name,"WITHROW PARK");
    td_job.count=5;td_job.route[3]=36;td_job.seconds=199;td_job.reward=130;
    td_route_district=0;td_resume_mode=TD_ROAM;actors_len=TD_ACTORS;
    for(unsigned i=0;i<21;i++){actors[i].flags=0x80|(i&1?ACTOR_FLAG_HIDDEN:0);actors[i].pos.x=1000+i;actors[i].pos.y=2000+i;}
    camera_x=0x3210;camera_y=0x4560;camera_settings=0x2D;VBK_REG=0;text_drawn=0;
    window_x=window_y=0;window_writes=tile_uploads=ground_uploads=content_reads=0;
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

static void verify_viewport(void) {
    UWORD patterns[20];UBYTE expected[16];
    expect(td_map_count<=TD_ATLAS_VISIBLE_LIMIT,"real renderer cache fits the reserved native pattern count");
    unsigned occupied=0;
    for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)if(td_ui_cache.patterns[i]!=65535) {
        occupied++;
        expect(td_ui_cache.patterns[i]<TD_ATLAS_PATTERNS,"every occupied sparse cache slot contains a valid actual dictionary ID");
        for(unsigned j=0;j<i;j++)if(td_ui_cache.patterns[j]!=65535)
            expect(td_ui_cache.patterns[i]!=td_ui_cache.patterns[j],"real viewport cache assigns one tile slot per unique dictionary pattern");
    }
    expect(occupied==td_map_count,"renderer unique count exactly matches all occupied sparse table slots");
    for(unsigned y=0;y<12;y++) {
        expect(td_atlas_row(td_map_x,td_map_y+y,20,patterns),"actual atlas API supplies the rendered viewport row");
        for(unsigned x=0;x<20;x++) {
            UBYTE marker=expected_marker(td_map_x+x,td_map_y+y),tile=window_tiles[0][2+y][x];
            expect(window_tiles[1][2+y][x]==15,"rendered ground retains UI palette7 and CGB tile bank1 attributes");
            if(marker){expect(tile==7+marker,"marker tile bitset preserves current player, parked car and remote objective overlap");continue;}
            expect(tile>=16&&tile<188&&td_ui_cache.patterns[tile-16]!=65535,"completed ground cells refer to bounded occupied atlas cache slots");
            if(tile<16||tile>=188||td_ui_cache.patterns[tile-16]==65535)continue;
            expect(td_ui_cache.patterns[tile-16]==patterns[x],"ground cache slot preserves the actual atlas dictionary ID");
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
        out[x]=tile>=192&&tile<=240?td_chars[tile-192]:'?';
    }
    out[20]=0;
}

static void expect_window_text(unsigned row,const char *text,const char *name) {
    char actual[21],expected[21];size_t length=strlen(text);
    expect(row<18&&length<=20,"HUD fixture text fits its native twenty-column row");
    if(row>=18||length>20)return;
    memset(expected,' ',20);memcpy(expected,text,length);expected[20]=0;
    read_window_text(row,actual);expect(!strcmp(actual,expected),name);
}

static void test_wait_contact_hud_and_map_restore(void) {
    /* Fixed output examples exercise the shared WAIT layout, not another
     * timetable oracle. Transit arithmetic has its own independent suite. */
    const struct {UBYTE origin,target,wait;UWORD seconds;const char *service;} cases[]={
        {46,48,62,14,"501 QUEEN"},
        {46,44,62,50,"501 QUEEN"},
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

static void test_every_viewport(void) {
    reset_case();open_case();game_snapshot_t before=snapshot_game();unsigned viewports=0;
    verify_marker_patterns();
    /* Drive the unchanged private paint routine at each legal viewport origin.
     * Public control behavior is independently exercised below. */
    for(unsigned y=0;y+12<=TD_ATLAS_TILE_HEIGHT;y++)for(unsigned x=0;x+20<=TD_ATLAS_TILE_WIDTH;x++) {
        td_map_x=x;td_map_y=y;td_map_begin();td_map_headers();
        unsigned uploads_before=ground_uploads;
        for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)
            expect(td_ui_cache.patterns[i]==65535,"every viewport begins with all172sparse table slots empty");
        for(unsigned row=0;row<12;row++) {
            UBYTE cache_before[360];memcpy(cache_before,&td_ui_cache,sizeof(cache_before));
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
    expect_game_unchanged(&before);td_map_close();
    for(unsigned i=0;i<sizeof(td_ui_cache);i++)
        expect(((UBYTE*)&td_ui_cache)[i]==255,"close invalidates every byte of the360-byte text/pattern union cache");
    td.mode=TD_PAUSE;unsigned writes=window_writes;td_ui_draw();
    expect(window_writes>writes,"return from atlas repaints actual text despite reuse of the pattern cache union");
}

static void expected_centre(UBYTE district,UWORD u,UWORD v,UBYTE *x,UBYTE *y) {
    UWORD point_x=0,point_y=0;
    expect(td_atlas_position(district,u,v,&point_x,&point_y),"focus fixture uses a valid actual atlas position");
    int left=(int)(point_x/8)-10,top=(int)(point_y/8)-6;
    if(left<0)left=0;
    if(top<0)top=0;
    if(left>TD_ATLAS_TILE_WIDTH-20)left=TD_ATLAS_TILE_WIDTH-20;
    if(top>TD_ATLAS_TILE_HEIGHT-12)top=TD_ATLAS_TILE_HEIGHT-12;
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
    const UBYTE lengths[]={0,1,TD_ACTORS,21};
    for(unsigned hidden=0;hidden<3;hidden++)for(unsigned scenario=0;scenario<sizeof(lengths);scenario++) {
        reset_case();actors_len=lengths[scenario];actor_t original[21];memcpy(original,actors,sizeof(actors));
        for(unsigned i=0;i<21;i++) {
            actors[i].flags=(actors[i].flags&~ACTOR_FLAG_HIDDEN)|
                (hidden==1||(hidden==2&&(i&1))?ACTOR_FLAG_HIDDEN:0);
        }
        memcpy(original,actors,sizeof(actors));
        UWORD saved_x=camera_x,saved_y=camera_y;UBYTE saved_settings=camera_settings;
        open_case();game_snapshot_t before=snapshot_game();
        unsigned count=actors_len<TD_ACTORS?actors_len:TD_ACTORS;
        for(unsigned i=0;i<21;i++)expect(actors[i].flags==(i<count?original[i].flags|ACTOR_FLAG_HIDDEN:original[i].flags),
                                        "map open hides only the bounded actual actor list");
        td_map_update(0,0);td_map_update(0,0);
        UBYTE before_row=td_map_row,before_count=td_map_count;unsigned uploads=tile_uploads;
        camera_x=111;camera_y=222;camera_settings=63;
        td_map_open();
        expect(td_map_row==before_row&&td_map_count==before_count&&tile_uploads==uploads,
               "repeated map open preserves partial paint and does not reload marker or ground tiles");
        for(unsigned i=0;i<21;i++)actors[i].flags^=0x20;
        UBYTE flags_before_close[21];for(unsigned i=0;i<21;i++)flags_before_close[i]=actors[i].flags;
        /* Driver handles B/Start by invoking this production close before
         * changing modes. The driver path itself is tested in test_engine.py. */
        td_map_close();
        expect(camera_x==saved_x&&camera_y==saved_y&&camera_settings==saved_settings&&VBK_REG==0,
               "partial-paint close restores exact camera words/settings and neutral VRAM bank");
        for(unsigned i=0;i<21;i++) {
            UBYTE expected=i<count?(flags_before_close[i]&~ACTOR_FLAG_HIDDEN)|(original[i].flags&ACTOR_FLAG_HIDDEN):flags_before_close[i];
            expect(actors[i].flags==expected,"map close restores only the prior hidden bit and retains other actor flag changes");
            expect(actors[i].pos.x==original[i].pos.x&&actors[i].pos.y==original[i].pos.y,
                   "map never moves native actors during partial paint or cancellation");
        }
        expect_game_unchanged(&before);
        for(unsigned i=0;i<360;i++)expect(((UBYTE*)&td_ui_cache)[i]==255,"partial close invalidates the entire union cache");
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
        else {td_map_count=TD_ATLAS_VISIBLE_LIMIT;for(unsigned i=0;i<td_map_count;i++)td_ui_cache.patterns[i]=65535;}
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
        open_case();td_map_x=32;td_map_y=0;td_map_begin();td_map_headers();finish_paint();
        UWORD x,y;expect(td_atlas_position(0,560,400,&x,&y),"overlap fixture target has a genuine atlas position");
        expect(x/8>=td_map_x&&x/8<td_map_x+20&&y/8<12,"overlap fixture lies inside the painted ground viewport");
        expect(window_tiles[0][2+y/8][x/8-td_map_x]==7+bits,
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
            for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)td_ui_cache.patterns[i]=TD_ATLAS_PATTERNS+i;
            td_ui_cache.patterns[hole]=65535;
            game_snapshot_t before=snapshot_game();unsigned uploads=ground_uploads;
            td_map_paint_row();
            expect(td_ui_cache.patterns[hole]==row[0]&&td_map_count==TD_ATLAS_VISIBLE_LIMIT,
                   "actual lookup reaches every possible sole empty slot even after adversarial collisions and wraparound");
            expect(ground_uploads==uploads+1&&!memcmp(vram[1][16+hole],pattern,16),
                   "single-hole insertion uploads the correct real pattern exactly once to its stable bounded slot");
            for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)if(i!=hole)
                expect(td_ui_cache.patterns[i]==TD_ATLAS_PATTERNS+i,"collision probing never overwrites an occupied cache slot");
            if(distinct)expect(td_map_error&&td_map_row==12,"a second distinct row pattern terminates safely when the table has become full");
            else expect(!td_map_error,"an already inserted row pattern remains retrievable after the table becomes full");
            expect_game_unchanged(&before);td_map_close();
        }
        reset_case();open_case();td_map_x=cases[fixture][0];td_map_y=cases[fixture][1];td_map_row=cases[fixture][2];
        td_map_count=TD_ATLAS_VISIBLE_LIMIT;
        for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)td_ui_cache.patterns[i]=TD_ATLAS_PATTERNS+i;
        UWORD before[TD_ATLAS_VISIBLE_LIMIT];memcpy(before,td_ui_cache.patterns,sizeof(before));
        unsigned uploads=ground_uploads;td_map_paint_row();
        expect(td_map_error&&td_map_row==12&&td_map_count==TD_ATLAS_VISIBLE_LIMIT,
               "a completely occupied table with no match stops bounded probing in the real error path");
        expect(ground_uploads==uploads&&!memcmp(before,td_ui_cache.patterns,sizeof(before)),
               "an exhausted probe does not upload, commit or evict any occupied pattern");
        expect(VBK_REG==0,"exhausted sparse-table probing returns to neutral VRAM bank");td_map_close();
    }
}

int main(void) {
    test_every_viewport();test_focus_and_partial_restart();test_panning_and_bounds();
    test_paid_transit_objective_context();test_interrupt_restore_and_idempotence();
    test_error_recovery_and_repeated_sessions();test_overlap_marker_geometry();
    test_sparse_table_full_and_single_holes();
    test_wait_contact_hud_and_map_restore();
    test_appended_district_focus_and_holes();
    printf("Atlas UI host regressions: %u checks, %u failures. Native raster/banking remains separate.\n",checks,failures);
    return failures?1:0;
}
