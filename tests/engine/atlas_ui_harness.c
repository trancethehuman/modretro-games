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
td_job_t td_job;
td_stop_t td_target,td_cursor;
UBYTE td_route_district;
UBYTE td_resume_mode;
/* Objective pointer and arrest/hospital receipt owned by td_life.c. */
UWORD td_beacon_u,td_beacon_v;
UBYTE td_beacon_shown,td_life_fine;
/* Car wear owned by td_drive.c. */
UBYTE td_car_damage,td_car_colour;
UBYTE td_station_near=TD_NONE;
actor_t actors[TD_ACTORS];
/* Day/night palettes (td_daynight.c) are not part of this fixture. */
UBYTE td_daynight_apply(UBYTE flags) {(void)flags;return 0;}
UWORD td_daynight_minutes(void) {return 8*60;}
UBYTE actors_len;
UWORD camera_x,camera_y,sys_time;
UBYTE camera_settings,VBK_REG,text_drawn;

/* One window tilemap and two pattern banks, indexed through the public tile
 * API. Native LCDC address mapping and scanline effects are not simulated. */
static UBYTE window_tiles[2][18][20],vram[2][256][16];
static UBYTE window_x,window_y;
static unsigned checks,failures,window_writes,tile_uploads,ground_uploads;
static unsigned content_reads;
static UBYTE initial_font[TD_FONT_GLYPHS][16],initial_bank0[256][16];
static unsigned palette_writes;static UWORD palette7[4];

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
    expect(VBK_REG<2&&tiles&&count&&(unsigned)first+count<=256,
           "all UI and atlas pattern uploads use bounded CGB tile indices");
    int marker=VBK_REG==1&&first>=8&&(unsigned)first+count<=15;
    int ground=VBK_REG==1&&first>=16&&(unsigned)first+count<=188;
    int font=VBK_REG==1&&first>=TD_FONT_FIRST&&(unsigned)first+count<=TD_FONT_FIRST+TD_FONT_GLYPHS;
    int art=VBK_REG==0&&first>=TD_UI_ART_FIRST&&(unsigned)first+count<=256;
    expect(marker||ground||font||art,"uploads stay within marker8..14, ground16..187, font or bank-0 art192..255 reservations");
    if(VBK_REG>=2||!tiles||!count||(unsigned)first+count>256)return;
    if(ground)ground_uploads+=count;
    memcpy(vram[VBK_REG][first],tiles,(size_t)count*16);
}

void ui_set_pos(UBYTE x,UBYTE y) {window_x=x;window_y=y;}
void set_bkg_palette(UBYTE first,UBYTE count,const UWORD *rgb) {
    palette_writes++;expect(first==7&&count==1&&rgb,"only the UI palette slot is swapped");
    if(first==7&&count==1&&rgb)memcpy(palette7,rgb,sizeof(palette7));
}
UBYTE td_audio_get_mode(void) {return TD_AUDIO_FULL;}
/* Radio calls chirp and pace themselves on the update counter. */
UBYTE td_tick;static unsigned audio_plays;
void td_audio_play(UBYTE cue) {(void)cue;audio_plays++;}
UBYTE td_service(UBYTE origin) {(void)origin;return 1;}
/* Lost parcels found (td_street.c). */
UBYTE td_parcels_found(void) {return 3;}
UBYTE td_next_departure(UBYTE origin,UWORD seconds) {(void)origin;(void)seconds;return 7;}
/* Streets and places (td_street_names.c, td_places.c): the tests set what
 * the courier is in; names spell the kind and id. */
static UBYTE stub_street,stub_place[3]={255,255,255};
UBYTE td_get_street(UWORD u,UWORD v) {(void)u;(void)v;content_reads++;return stub_street;}
void td_get_street_name(UBYTE id,char *dest) {sprintf(dest,"TEST ROAD %u",id);}
void td_get_places(UWORD u,UWORD v,UBYTE *ids) {(void)u;(void)v;memcpy(ids,stub_place,3);}
void td_get_place_name(UBYTE kind,UBYTE id,char *dest) {sprintf(dest,"%s %u",kind==0?"AREA":kind==1?"MARK":"JUNC",id);}
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
    game_snapshot_t snapshot={td,td_job,td_job,td_target,td_cursor,td_route_district,actors_len,td_resume_mode};
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
    memset(&td,0,sizeof(td));memset(&td_job,0,sizeof(td_job));
    memset(&td_target,0,sizeof(td_target));memset(&td_cursor,0,sizeof(td_cursor));
    memset(actors,0,sizeof(actors));memset(window_tiles,0xEE,sizeof(window_tiles));memset(vram,0xEE,sizeof(vram));
    td.district=0;td.u=560*16;td.v=720*16;td.onfoot=1;
    td.park_district=1;td.park_u=400*16;td.park_v=528*16;td.cash=123;td.seconds=4321;
    td.left=199;td.health=100;td.job=84;td.stage=3;td.vitality=77;td.ammo=33;td.mode=TD_PAUSE;
    td_target.district=3;td_target.u=320;td_target.v=144;td_target.reserved=TD_STOP_FOOT;strcpy(td_target.name,"WITHROW PARK");
    td_job.count=5;td_job.route[3]=36;td_job.seconds=199;td_job.reward=130;
    td_route_district=0;td_resume_mode=TD_ROAM;actors_len=TD_ACTORS;
    for(unsigned i=0;i<TD_ACTORS;i++){actors[i].flags=0x80|(i&1?ACTOR_FLAG_HIDDEN:0);actors[i].pos.x=1000+i;actors[i].pos.y=2000+i;}
    camera_x=0x3210;camera_y=0x4560;camera_settings=0x2D;VBK_REG=0;text_drawn=0;
    window_x=window_y=0;window_writes=tile_uploads=ground_uploads=content_reads=0;
    td_ui_init();memcpy(initial_font,vram[1]+TD_FONT_FIRST,sizeof(initial_font));memcpy(initial_bank0,vram[0],sizeof(initial_bank0));
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
        expect(window_tiles[0][y][x]>=TD_FONT_FIRST&&window_tiles[0][y][x]<TD_FONT_FIRST+TD_FONT_GLYPHS,
               "map titles, legend and controls use only reserved native font tiles");
    expect(!memcmp(initial_font,vram[1]+TD_FONT_FIRST,sizeof(initial_font)),"all viewport uploads preserve the original font patterns");
    for(unsigned tile=0;tile<128;tile++)for(unsigned byte=0;byte<16;byte++)
        expect(vram[0][tile][byte]==0xEE,"real map leaves the gameplay background pattern bank untouched");
    expect(!memcmp(initial_bank0,vram[0],sizeof(initial_bank0)),"real map leaves the bank-0 UI art untouched");
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
        out[x]=tile>=TD_FONT_FIRST&&tile<TD_FONT_FIRST+TD_FONT_GLYPHS?td_chars[tile-TD_FONT_FIRST]:'?';
    }
    out[20]=0;
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
        expect(td_map_row==12&&!td_map_error,"all225legal viewports finish real cached rendering");
        expect(ground_uploads-uploads_before==td_map_count,"each viewport uploads each distinct ground pattern exactly once");
        verify_viewport();viewports++;
    }
    expect(viewports==225,"fixture renders every actual legal twenty-by-twelve atlas viewport");
    expect(!memcmp(palette7,td_map_palette,sizeof(palette7)),"the open map shows its original colours in UI palette 7");
    expect_game_unchanged(&before);td_map_close();
    expect(!memcmp(palette7,td_ui_palette,sizeof(palette7)),"closing the map restores the menu colours");
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
    for(unsigned i=0;i<50;i++){td_map_update(J_LEFT|J_UP,0);finish_paint();}
    expect(td_map_x==0&&td_map_y==0,"repeated diagonal panning clamps exactly at the northwest atlas bounds");
    verify_viewport();
    for(unsigned i=0;i<50;i++){td_map_update(J_RIGHT|J_DOWN,0);finish_paint();}
    expect(td_map_x==44&&td_map_y==4,"repeated diagonal panning clamps exactly at the southeast padded atlas bounds");
    verify_viewport();
    for(unsigned i=0;i<50;i++){td_map_update(J_LEFT|J_DOWN,0);finish_paint();}
    expect(td_map_x==0&&td_map_y==4,"horizontal motion never underflows the atlas while bottom edge remains clamped");
    for(unsigned i=0;i<50;i++){td_map_update(J_RIGHT|J_UP,0);finish_paint();}
    expect(td_map_x==44&&td_map_y==0,"horizontal motion never overflows the atlas while top edge remains clamped");
    td_map_update(J_A,J_A);x=td_map_x;y=td_map_y;
    expect(td_map_row==1,"focus begins a genuine partial paint for held-input gating");
    td_map_update(J_LEFT|J_UP,0);
    expect(td_map_x==x&&td_map_y==y&&td_map_row==2,"held panning waits until the current atlas viewport has fully painted");
    finish_paint();verify_viewport();expect_game_unchanged(&before);td_map_close();
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
        reset_case();actors_len=lengths[scenario];actor_t original[TD_ACTORS];memcpy(original,actors,sizeof(actors));
        for(unsigned i=0;i<TD_ACTORS;i++) {
            actors[i].flags=(actors[i].flags&~ACTOR_FLAG_HIDDEN)|
                (hidden==1||(hidden==2&&(i&1))?ACTOR_FLAG_HIDDEN:0);
        }
        memcpy(original,actors,sizeof(actors));
        UWORD saved_x=camera_x,saved_y=camera_y;UBYTE saved_settings=camera_settings;
        open_case();game_snapshot_t before=snapshot_game();
        unsigned count=actors_len<TD_ACTORS?actors_len:TD_ACTORS;
        for(unsigned i=0;i<TD_ACTORS;i++)expect(actors[i].flags==(i<count?original[i].flags|ACTOR_FLAG_HIDDEN:original[i].flags),
                                        "map open hides only the bounded actual actor list");
        td_map_update(0,0);td_map_update(0,0);
        UBYTE before_row=td_map_row,before_count=td_map_count;unsigned uploads=tile_uploads;
        camera_x=111;camera_y=222;camera_settings=63;
        td_map_open();
        expect(td_map_row==before_row&&td_map_count==before_count&&tile_uploads==uploads,
               "repeated map open preserves partial paint and does not reload marker or ground tiles");
        for(unsigned i=0;i<TD_ACTORS;i++)actors[i].flags^=0x20;
        UBYTE flags_before_close[TD_ACTORS];for(unsigned i=0;i<TD_ACTORS;i++)flags_before_close[i]=actors[i].flags;
        /* Driver handles B/Start by invoking this production close before
         * changing modes. The driver path itself is tested in test_engine.py. */
        td_map_close();
        expect(camera_x==saved_x&&camera_y==saved_y&&camera_settings==saved_settings&&VBK_REG==0,
               "partial-paint close restores exact camera words/settings and neutral VRAM bank");
        for(unsigned i=0;i<TD_ACTORS;i++) {
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
            for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)td_ui_cache.patterns[i]=1000+i;
            td_ui_cache.patterns[hole]=65535;
            game_snapshot_t before=snapshot_game();unsigned uploads=ground_uploads;
            td_map_paint_row();
            expect(td_ui_cache.patterns[hole]==row[0]&&td_map_count==TD_ATLAS_VISIBLE_LIMIT,
                   "actual lookup reaches every possible sole empty slot even after adversarial collisions and wraparound");
            expect(ground_uploads==uploads+1&&!memcmp(vram[1][16+hole],pattern,16),
                   "single-hole insertion uploads the correct real pattern exactly once to its stable bounded slot");
            for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)if(i!=hole)
                expect(td_ui_cache.patterns[i]==1000+i,"collision probing never overwrites an occupied cache slot");
            if(distinct)expect(td_map_error&&td_map_row==12,"a second distinct row pattern terminates safely when the table has become full");
            else expect(!td_map_error,"an already inserted row pattern remains retrievable after the table becomes full");
            expect_game_unchanged(&before);td_map_close();
        }
        reset_case();open_case();td_map_x=cases[fixture][0];td_map_y=cases[fixture][1];td_map_row=cases[fixture][2];
        td_map_count=TD_ATLAS_VISIBLE_LIMIT;
        for(unsigned i=0;i<TD_ATLAS_VISIBLE_LIMIT;i++)td_ui_cache.patterns[i]=1000+i;
        UWORD before[TD_ATLAS_VISIBLE_LIMIT];memcpy(before,td_ui_cache.patterns,sizeof(before));
        unsigned uploads=ground_uploads;td_map_paint_row();
        expect(td_map_error&&td_map_row==12&&td_map_count==TD_ATLAS_VISIBLE_LIMIT,
               "a completely occupied table with no match stops bounded probing in the real error path");
        expect(ground_uploads==uploads&&!memcmp(before,td_ui_cache.patterns,sizeof(before)),
               "an exhausted probe does not upload, commit or evict any occupied pattern");
        expect(VBK_REG==0,"exhausted sparse-table probing returns to neutral VRAM bank");td_map_close();
    }
}


static UBYTE glyph_tile(const char *code) {return td_glyph_tile[(UBYTE)code[0]-32];}
/* Runs the radio until it plays something other than script; returns that. */
static UBYTE radio_after(UBYTE script) {
    for(unsigned i=0;i<40000&&td_radio_playing()==script;i++){td_tick++;td_radio_tick();}
    return td_radio_playing();
}

/* One place lookup (update phase 4) then one HUD tick (phase 0). */
static void hud_step(void) {td_hud_places();td_ui_hud_tick();}
static void test_menus_and_radio(void) {
    char text[21],text2[21];
    /* The pause menu is a twelve-row sheet over the city: status, the lost
     * parcels found, four actions at a time, scroll marks and a hint for the
     * highlighted action. */
    reset_case();td.mode=TD_PAUSE;td.menu=0;td_ui_init();td_ui_draw();
    expect(window_y==144-12*8,"the pause menu is a bottom sheet that leaves the city visible");
    read_window_text(2,text);expect(strstr(text,"LOST PARCELS 3/20")!=NULL,"the pause menu counts the lost parcels found");
    read_window_text(4,text);expect(strstr(text,"RESUME")!=NULL,"the first visible action is resume");
    read_window_text(7,text);expect(strstr(text,"GET IN CAR")!=NULL,"four actions are listed; on foot the fourth gets back in the car");
    expect(window_tiles[0][7][18]==glyph_tile(TD_UI_ARROW_S)&&window_tiles[0][4][18]!=glyph_tile(TD_UI_ARROW_N),
           "a down mark shows more actions below, none above");
    read_window_text(9,text);expect(strstr(text,"BACK TO THE CITY")!=NULL,"the hint describes the highlighted action");
    read_window_text(10,text);expect(strstr(text,"1/9")!=NULL,"the footer counts the position in the list");
    td.menu=8;td_ui_draw();
    read_window_text(7,text);expect(strstr(text,"SOUND MUSIC+FX")!=NULL,"moving to the last action scrolls it into view");
    read_window_text(4,text);expect(strstr(text,"TTC")!=NULL,"the window keeps four actions, ending on the cursor");
    expect(window_tiles[0][4][18]==glyph_tile(TD_UI_ARROW_N)&&window_tiles[0][7][18]!=glyph_tile(TD_UI_ARROW_S),
           "an up mark shows more actions above at the end of the list");
    td.menu=0;td_ui_draw();read_window_text(4,text);
    expect(strstr(text,"RESUME")!=NULL,"wrapping to the first action scrolls back to the top");
    td.menu=6;td.mode=TD_BOARD;td_ui_draw();
    expect(window_y==144-13*8,"the dispatch board is a thirteen-row card");
    read_window_text(1,text);expect(strstr(text,"FIRST SHIFT")!=NULL,"the board names the contract's chapter");
    td.menu=12;td_ui_draw();read_window_text(1,text);expect(strstr(text,"NEIGHBOURHOODS")!=NULL,"the next eight contracts are chapter two");
    td_offer.min_done=6;td_offer.after=6;td.done=6;memset(td.complete,0,sizeof(td.complete));td.complete[0]=0x3F;td.menu=8;td_ui_draw();
    read_window_text(10,text);expect(strstr(text,"AFTER JOB 7")!=NULL,"a contract the story has not reached names the job it waits for");
    td.complete[0]|=0x40;td.done=7;td_ui_draw();
    read_window_text(10,text);expect(strstr(text,"READY TO TAKE")!=NULL,"and is ready once that job is done");
    td.done=5;td_ui_draw();read_window_text(10,text);expect(strstr(text,"NEEDS 6 DONE")!=NULL,"too few deliveries still reads as such");
    memset(td.complete,0,sizeof(td.complete));td.done=0;td.menu=12;
    td.mode=TD_RESULT;td.health=100;td.left=9;td_last_pay=86;td_ui_draw();
    read_window_text(3,text);expect(strstr(text,"+$86 PAID")!=NULL,"the result card shows the fee paid");

    /* A radio call raises the HUD and types Rosa's lines, then closes. */
    reset_case();td.mode=TD_ROAM;td_ui_init();td_ui_draw();
    expect(window_y==144,"nothing covers the city before there is something to report");
    td_ui_hud_tick();
    expect(window_y==144-16,"an active job pops up its next stop and its status row");
    read_window_text(0,text);expect(window_tiles[0][0][0]==glyph_tile(TD_UI_PIN)&&strstr(text,"TEST DISTRICT"),"the first pop-up row names where to go");
    read_window_text(1,text);expect(strstr(text,"199S")!=NULL&&strstr(text,"4/5")!=NULL,"the second shows the stop count and time left");
    unsigned plays=audio_plays;
    td_radio_say(TD_RADIO_INTRO);td_tick=1;td_radio_tick();
    expect(window_y==144-48&&audio_plays==plays+1,"a call chirps and raises its card above the pop-up rows");
    read_window_text(0,text);expect(!strncmp(text+3,"ROSA - DISPATCH",15),"the card names the speaker beside the portrait");
    expect(window_tiles[0][0][0]==glyph_tile(TD_UI_PORTRAIT_0)&&window_tiles[0][2][2]==glyph_tile(TD_UI_PORTRAIT_8),
           "the card shows the portrait's nine tiles");
    read_window_text(5,text);expect(strstr(text,"199S")!=NULL,"the job row sits below the card");
    read_window_text(1,text);expect(text[3]==' ',"text starts blank");
    for(unsigned i=0;i<40;i++){td_tick++;td_radio_tick();}
    read_window_text(1,text);expect(!strncmp(text+3,"MORNING, ROOKIE.",16),"the first line types out");
    read_window_text(2,text);expect(!strncmp(text+3,"ROSA ON DISPATCH.",17),"then the second");
    /* A contract briefing queues behind the welcome and plays in turn. */
    td_radio_contract(0,0);
    for(unsigned i=0;i<40000&&!(td_radio_playing()==TD_RADIO_CONTRACT&&td_radio_pos>=TD_RADIO_PAGE);i++){td_tick++;td_radio_tick();}
    read_window_text(1,text);expect(!strcmp(text+3,"FIRST JOB. SAL   "),"a queued contract briefing follows the welcome on a clean card");
    read_window_text(2,text);expect(!strcmp(text+3,"AT ST LAWRENCE.  "),"with its own second line");
    for(unsigned i=0;i<40000&&td_radio_playing()!=TD_NONE;i++){td_tick++;td_radio_tick();}
    expect(window_y==144-16&&td_radio_playing()==TD_NONE,"the card drops away once the calls end");
    read_window_text(1,text);expect(strstr(text,"199S")!=NULL,"the pop-up rows repaint in their own rows");
    /* Clients speak through the caller portrait under their own name. */
    td_radio_contract(0,2);td_tick=1;td_radio_tick();
    read_window_text(0,text);expect(!strncmp(text+3,"SAL - THE MARKET",16),"a client's line names the client");
    expect(window_tiles[0][0][1]==glyph_tile(TD_UI_CALLER_T)&&window_tiles[0][1][0]==glyph_tile(TD_UI_CALLER_FL)&&
           window_tiles[0][2][2]==glyph_tile(TD_UI_CALLER_BR),"and shows the caller portrait instead of Rosa's");
    /* A pickup waits for its briefing; a delivery cuts both short. */
    td_radio_script=td_radio_next=TD_NONE;
    td_radio_contract(5,0);td_radio_contract(5,1);
    expect(td_radio_playing()==TD_RADIO_CONTRACT&&td_radio_next==TD_RADIO_CONTRACT,"a pickup call waits for the briefing");
    td_radio_contract(5,2);{char who[20];td_radio_speaker(who);
    expect(td_radio_next==TD_NONE&&!strcmp(who,"DR HALE - MUSEUM"),"a delivery replaces older talk about its contract at once");}
    /* A first delivery moves the story on after its delivery call: the
     * beat that follows the contract, a count beat, the chapters it opens
     * and the finale. A replay adds nothing. */
    td_radio_script=td_radio_next=TD_NONE;memset(td.complete,0,sizeof(td.complete));
    td.seconds=4320;/* an even clock: no early-arrival remark in these cases */
    td.done=6;td.complete[0]=0x5F;
    {UWORD open=td_radio_open();expect(open==3,"six deliveries and contract 7 open the second chapter");
    td.complete[1]|=1;td.done=7;td_radio_done(8,6,open);}
    expect(td_radio_playing()==TD_RADIO_CONTRACT,"the delivery call comes first");
    expect(radio_after(TD_RADIO_CONTRACT)==TD_RADIO_BEAT_RIVAL,"then the rival, who follows contract 9");
    expect(radio_after(TD_RADIO_BEAT_RIVAL)==TD_NONE,"and nothing else");
    td.done=4;td.complete[0]=0x0F;td.complete[1]=0;
    {UWORD open=td_radio_open();td.complete[0]|=0x40;td.done=5;td_radio_done(6,4,open);
    expect(radio_after(TD_RADIO_CONTRACT)==TD_NONE,"contract 7 at five deliveries does not open a chapter yet");
    open=td_radio_open();td.complete[0]|=0x10;td.done=6;td_radio_done(4,5,open);}
    expect(radio_after(TD_RADIO_CONTRACT)==TD_RADIO_CHAPTER_1,"the sixth delivery opens the second chapter with its call");
    radio_after(TD_RADIO_CHAPTER_1);
    td.done=6;td.complete[0]=0x3F;
    {UWORD open=td_radio_open();expect(open==1,"six deliveries without contract 7 keep the second chapter shut");
    td.complete[0]|=0x40;td.done=7;td_radio_done(6,6,open);}
    expect(radio_after(TD_RADIO_CONTRACT)==TD_RADIO_CHAPTER_1,"finishing contract 7 then opens it with its call");
    radio_after(TD_RADIO_CHAPTER_1);
    {UWORD open=td_radio_open();td_radio_done(8,7,open);}
    expect(radio_after(TD_RADIO_CONTRACT)==TD_NONE,"a replay moves no story on");
    td.done=2;td.complete[0]=0x03;td.complete[1]=0;
    {UWORD open=td_radio_open();td.complete[1]|=1;td.done=3;td_radio_done(8,2,open);}
    expect(radio_after(TD_RADIO_CONTRACT)==TD_RADIO_BEAT_RIVAL&&radio_after(TD_RADIO_BEAT_RIVAL)==TD_RADIO_OPEN_ENDS,
           "story calls wait their turn: the contract's beat, then the count's");
    radio_after(TD_RADIO_OPEN_ENDS);
    memset(td.complete,0xFF,TD_QUESTS/8);td.complete[TD_QUESTS/8-1]&=0x7F;td.done=TD_QUESTS-1;
    {UWORD open=td_radio_open();expect(open==(1u<<TD_STORY_CHAPTERS)-1,"every chapter is open near the end");
    td.complete[TD_QUESTS/8-1]|=0x80;td.done=TD_QUESTS;td_radio_done(TD_QUESTS-1,TD_QUESTS-1,open);}
    expect(radio_after(TD_RADIO_CONTRACT)==TD_RADIO_MASTER,"the last delivery brings the finale");
    radio_after(TD_RADIO_MASTER);memset(td.complete,0,sizeof(td.complete));td.done=0;
    /* How it went: dented cargo is noticed before the story moves on. */
    td.done=6;td.complete[0]=0x5F;td.health=30;
    {UWORD open=td_radio_open();td.complete[1]|=1;td.done=7;td_radio_done(8,6,open);}
    expect(radio_after(TD_RADIO_CONTRACT)>=TD_RADIO_DENTED&&td_radio_playing()<TD_RADIO_DENTED+TD_RADIO_DENTED_COUNT,
           "a client notices dented cargo after the delivery call");
    expect(radio_after(td_radio_playing())==TD_RADIO_BEAT_RIVAL,"then the story goes on");
    radio_after(TD_RADIO_BEAT_RIVAL);td.health=100;
    td.left=100;td_job.seconds=120;td.seconds=1;
    {UWORD open=td_radio_open();td_radio_done(8,7,open);}
    expect(radio_after(TD_RADIO_CONTRACT)>=TD_RADIO_EARLY&&td_radio_playing()<TD_RADIO_EARLY+TD_RADIO_EARLY_COUNT,
           "an early arrival sometimes earns a word");
    radio_after(td_radio_playing());td.seconds=2;
    {UWORD open=td_radio_open();td_radio_done(8,7,open);}
    expect(radio_after(TD_RADIO_CONTRACT)==TD_NONE,"not every time");
    /* Chatter follows the story: festival-week people once that chapter is open. */
    memset(td.complete,0,sizeof(td.complete));td.done=0;td.seconds=1;td_radio_chatter();
    expect(td_radio_playing()>=TD_RADIO_CHATTER&&td_radio_playing()<TD_RADIO_CHATTER+TD_RADIO_CHATTER_COUNT,"early on, the city's own chatter");
    radio_after(td_radio_playing());
    memset(td.complete,0xFF,7);td.done=56;td_radio_chatter();
    expect(td_radio_playing()>=TD_RADIO_LATE&&td_radio_playing()<TD_RADIO_LATE+8,"in festival week, its people talk");
    radio_after(td_radio_playing());td.seconds=2;td_radio_chatter();
    expect(td_radio_playing()>=TD_RADIO_CHATTER&&td_radio_playing()<TD_RADIO_CHATTER+TD_RADIO_CHATTER_COUNT,"half the time");
    radio_after(td_radio_playing());memset(td.complete,0,sizeof(td.complete));td.done=0;td.seconds=4321;
    td_radio_say(TD_RADIO_CHATTER);td_radio_say(TD_RADIO_WANTED);
    expect(td_radio_playing()==TD_RADIO_WANTED,"chatter gives way to a story call at once");
    /* A call that starts while an old card is still up gets a clean card. */
    td_radio_say(TD_RADIO_INTRO);for(unsigned i=0;i<20;i++){td_tick++;td_radio_tick();}
    td_radio_script=TD_NONE;td_radio_say(TD_RADIO_NIGHT);
    for(unsigned i=0;i<60;i++){td_tick=(UBYTE)(td_tick+1)|1;td_radio_tick();}
    read_window_text(1,text);expect(!strcmp(text+3,"NIGHT SHIFT.     "),"a new call never types over an old card");
    td_radio_script=td_radio_next=TD_NONE;td.mode=TD_PAUSE;td_ui_draw();td.mode=TD_ROAM;td_ui_draw();
    /* Stars bring a call; the arrest clears them without "lost them". */
    td.job=TD_NONE;td.wanted=1;td_tick=1;td_radio_tick();expect(td_radio_playing()==TD_RADIO_WANTED,"a first star brings a police call");
    td_radio_script=td_radio_next=TD_NONE;td.wanted=0;td_radio_tick();td_radio_script=td_radio_next=TD_NONE;
    td.job=84;td.wanted=1;td_radio_tick();expect(td_radio_playing()==TD_RADIO_WANTED_JOB,"on a job the call says so");
    td_radio_script=TD_NONE;td_radio_next=TD_NONE;td_radio_say(TD_RADIO_BUSTED);td.wanted=0;td_radio_tick();
    expect(td_radio_playing()==TD_RADIO_BUSTED&&td_radio_next==TD_NONE,"an arrest does not also report losing the police");

    /* On a job the status row shows the distance to the objective. */
    td_radio_script=td_radio_next=TD_NONE;reset_case();td.mode=TD_ROAM;td.district=0;td_target.district=0;td_beacon_shown=1;
    td_beacon_u=(td.u>>4)+100;td_beacon_v=td.v>>4;td_ui_init();td_ui_draw();td_tick=1;td_ui_hud_tick();
    read_window_text(1,text);expect(strstr(text,"400M")!=NULL,"a hundred pixels reads as 400 m");
    td_beacon_u=(td.u>>4)+300;td_ui_hud_tick();
    read_window_text(1,text);expect(strstr(text,"1.2K")!=NULL,"three hundred pixels reads as 1.2 km");

    /* Pop-ups: free roam shows nothing until something changes; a shot
     * shows the ammunition for a few seconds; standing still shows the
     * status line; low vitality stays up. */
    reset_case();td.mode=TD_ROAM;td.job=TD_NONE;td_beacon_shown=0;td_ui_init();td_ui_draw();td_tick=1;
    td_ui_hud_tick();expect(window_y==144,"free roam with nothing to report leaves the whole screen to the city");
    td.u+=16;td.ammo--;td_ui_hud_tick();
    read_window_text(0,text);expect(window_y==144-8&&strstr(text,"32")!=NULL&&strstr(text,"123")==NULL,
                                    "a shot pops up the ammunition only");
    for(unsigned i=0;i<30;i++){td.u+=16;td_ui_hud_tick();}
    expect(window_y==144,"the ammunition count sinks again after a few seconds");
    for(unsigned i=0;i<12;i++)td_ui_hud_tick();
    read_window_text(0,text);expect(window_y==144-8&&strstr(text,"123")&&strstr(text,"77")&&strstr(text,"33")==NULL&&strstr(text,"32"),
                                    "standing still shows cash, vitality and ammunition");
    td.u+=16;td_ui_hud_tick();expect(window_y==144,"moving again hides the status line");
    td.onfoot=0;for(unsigned i=0;i<12;i++)td_ui_hud_tick();
    expect(window_y==144,"in a vehicle a short stop (a red light) shows nothing");
    for(unsigned i=0;i<14;i++)td_ui_hud_tick();
    expect(window_y==144-8,"a longer stop in a vehicle shows the status line");
    td.u+=16;td_ui_hud_tick();td.onfoot=1;
    td.vitality=20;td_ui_hud_tick();td_ui_hud_tick();
    for(unsigned i=0;i<30;i++){td.u+=16;td_ui_hud_tick();}
    read_window_text(0,text);expect(window_y==144-8&&strstr(text,"20"),"low vitality stays on screen");
    td.vitality=90;td.wanted=2;for(unsigned i=0;i<30;i++){td.u+=16;td_ui_hud_tick();}
    read_window_text(0,text);expect(window_y==144-8&&strstr(text,"WANTED"),"wanted stars stay up while the police are looking");
    td.wanted=0;td.msg=TD_MSG_PARCEL;td_ui_draw();read_window_text(0,text);
    expect(strstr(text,"LOST PARCEL 3/20")!=NULL,"a found parcel pops up the count");
    td.msg=9;td_ui_draw();read_window_text(0,text);
    expect(window_y==144-8&&strstr(text,"VEHICLE IS PARKED"),"a notice pops up on its own");

    /* Places: a neighbourhood names itself on entry (also the first look in
     * a scene) in the status row; a junction on its edge shows above it;
     * a landmark reached while a junction is named waits a moment, and is
     * named if the courier is still near; a junction takes over from a
     * landmark; a new street names itself. */
    reset_case();td.mode=TD_ROAM;td.job=TD_NONE;td_beacon_shown=0;stub_street=4;stub_place[0]=7;stub_place[1]=stub_place[2]=255;
    td_ui_init();td_ui_draw();td_tick=0;hud_step();
    read_window_text(0,text);expect(window_y==144-8&&!strncmp(text,"AREA 7",6),"a scene opens with its neighbourhood's name");
    stub_place[2]=3;td.u+=16;hud_step();read_window_text(0,text);read_window_text(1,text2);
    expect(window_y==144-16&&!strncmp(text,"JUNC 3",6)&&!strncmp(text2,"AREA 7",6),
           "a junction on a neighbourhood's edge shows above the neighbourhood's name");
    for(unsigned i=0;i<30;i++){td_tick+=8;td.u+=16;hud_step();}
    expect(window_y==144,"both names sink after a few seconds");
    stub_place[2]=5;td.u+=16;hud_step();read_window_text(0,text);
    expect(window_y==144-8&&!strncmp(text,"JUNC 5",6),"crossing a junction names its two streets");
    stub_place[1]=2;td.u+=16;hud_step();read_window_text(0,text);
    expect(!strncmp(text,"JUNC 5",6),"a landmark waits while a junction is named");
    for(unsigned i=0;i<8;i++){td_tick+=8;td.u+=16;hud_step();}
    read_window_text(0,text);expect(!strncmp(text,"MARK 2",6),"then names itself while the courier is still near it");
    stub_place[0]=9;td.u+=16;hud_step();read_window_text(0,text);read_window_text(1,text2);
    expect(!strncmp(text,"MARK 2",6)&&!strncmp(text2,"AREA 9",6),"a new neighbourhood shows under the landmark");
    stub_place[2]=4;td.u+=16;hud_step();read_window_text(0,text);
    expect(!strncmp(text,"JUNC 4",6),"a junction takes over from a landmark");
    stub_place[1]=255;td.u+=16;hud_step();stub_place[1]=6;stub_place[2]=255;td.u+=16;hud_step();
    for(unsigned i=0;i<30;i++){td_tick+=8;td.u+=16;hud_step();if(i==1)stub_place[1]=255;}
    expect(window_y==144,"a landmark left behind before it could be named stays quiet");
    td_tick=0;stub_street=6;td.u+=16;hud_step();read_window_text(0,text);
    expect(window_y==144-8&&!strncmp(text,"TEST ROAD 6",11),"a new street names itself");
    for(unsigned i=0;i<30;i++){td_tick+=8;td.u+=16;hud_step();}
    stub_place[1]=2;td.u+=16;hud_step();read_window_text(0,text);
    expect(!strncmp(text,"MARK 2",6),"coming back to a landmark names it again");
    for(unsigned i=0;i<30;i++){td_tick+=8;td.u+=16;hud_step();}
    td.vitality=20;stub_place[0]=11;td.u+=16;hud_step();read_window_text(0,text);read_window_text(1,text2);
    expect(!strncmp(text,"AREA 11",7)&&strstr(text2,"20"),"with the status row busy the neighbourhood takes the top row");
    stub_place[0]=255;stub_street=0;td.vitality=90;
}

int main(void) {
    test_every_viewport();test_focus_and_partial_restart();test_panning_and_bounds();
    test_paid_transit_objective_context();test_interrupt_restore_and_idempotence();
    test_error_recovery_and_repeated_sessions();test_overlap_marker_geometry();
    test_sparse_table_full_and_single_holes();test_menus_and_radio();
    printf("Atlas UI host regressions: %u checks, %u failures. Native raster/banking remains separate.\n",checks,failures);
    return failures?1:0;
}
