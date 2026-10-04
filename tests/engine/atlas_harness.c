/* Actual BANKED atlas source with host types only. Pixels and native resource
 * geometry come from an independently generated collision/water oracle. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <gbdk/platform.h>
#include "td_atlas.h"
#include "td_district.h"
#include "atlas_oracle.h"
#include "atlas_under_test.c"

static unsigned checks,failures;
static UWORD rows[TD_ATLAS_TILE_WIDTH*TD_ATLAS_TILE_HEIGHT];

static void expect(int condition,const char *name) {
    checks++;
    if(!condition) {
        failures++;
        if(failures<30)fprintf(stderr,"FAIL %s\n",name);
    }
}

static void test_bounds(void) {
    UWORD width=99,height=77;
    expect(td_atlas_bounds(&width,&height)&&width==ORACLE_WIDTH&&height==ORACLE_HEIGHT,
           "atlas bounds return the actual unpadded whole-world pixel dimensions");
    width=99;height=77;
    expect(!td_atlas_bounds(NULL,&height)&&height==77,
           "null bounds width rejects without changing height");
    expect(!td_atlas_bounds(&width,NULL)&&width==99,
           "null bounds height rejects without changing width");
    expect(!td_atlas_bounds(NULL,NULL),"both null bounds outputs are rejected");
}

static void test_positions(void) {
    expect(ORACLE_DISTRICT_COUNT==TD_DISTRICT_COUNT,"oracle imports every actual registered district");
    for(unsigned index=0;index<ORACLE_DISTRICT_COUNT;index++) {
        const oracle_district_t *d=&oracle_districts[index];
        for(UWORD u=0;u<d->width;u++) {
            UWORD x=65535,y=65535;
            expect(td_atlas_position(d->id,u,555,&x,&y)&&x==d->x+u/8&&y==d->y+555/8,
                   "every local horizontal pixel maps north-up at exact one-eighth scale");
        }
        for(UWORD v=0;v<d->height;v++) {
            UWORD x=65535,y=65535;
            expect(td_atlas_position(d->id,511,v,&x,&y)&&x==d->x+511/8&&y==d->y+v/8,
                   "every local vertical pixel maps north-up at exact one-eighth scale");
        }
        /* Independent resource tile centres cover every source collision cell. */
        for(UWORD v=4;v<d->height;v+=8)for(UWORD u=4;u<d->width;u+=8) {
            UWORD x=65535,y=65535;
            expect(td_atlas_position(d->id,u,v,&x,&y)&&x==d->x+u/8&&y==d->y+v/8,
                   "all registered collision centres map into the correct atlas district");
        }
        UWORD x=12345,y=23456;
        expect(!td_atlas_position(d->id,d->width,1,&x,&y)&&x==12345&&y==23456,
               "one-past local width leaves both outputs unchanged");
        expect(!td_atlas_position(d->id,1,d->height,&x,&y)&&x==12345&&y==23456,
               "one-past local height leaves both outputs unchanged");
        expect(!td_atlas_position(d->id,65535,65535,&x,&y)&&x==12345&&y==23456,
               "large native words cannot overflow into valid atlas positions");
        expect(!td_atlas_position(d->id,1,1,NULL,&y)&&y==23456,
               "null position x is rejected before writing y");
        expect(!td_atlas_position(d->id,1,1,&x,NULL)&&x==12345,
               "null position y is rejected before writing x");
        expect(!td_atlas_position(d->id,1,1,NULL,NULL),"null position pair is rejected");
    }
    UWORD x=12345,y=23456;
    expect(!td_atlas_position(TD_DISTRICT_COUNT,1,1,&x,&y)&&x==12345&&y==23456,
           "first unregistered district leaves position outputs untouched");
    expect(!td_atlas_position(255,1,1,&x,&y)&&x==12345&&y==23456,
           "sentinel district cannot map into the atlas");
}

static void test_rows_and_patterns(void) {
    UWORD ids[22];UBYTE tile[18];
    const unsigned tile_width=ORACLE_PADDED_WIDTH/8,tile_height=ORACLE_PADDED_HEIGHT/8;
    expect(tile_width==TD_ATLAS_TILE_WIDTH&&tile_height==TD_ATLAS_TILE_HEIGHT,
           "oracle derives the complete padded tile dimensions from actual resources");
    for(unsigned y=0;y<tile_height;y++)for(unsigned x=0;x<tile_width;x++) {
        ids[0]=43210;ids[1]=32109;ids[2]=21098;
        int okay=td_atlas_row((UBYTE)x,(UBYTE)y,1,&ids[1]);
        expect(okay&&ids[0]==43210&&ids[2]==21098,
               "single row lookup writes exactly its requested pattern count");
        if(!okay)continue;
        rows[y*tile_width+x]=ids[1];
        memset(tile,0xA5,sizeof(tile));
        okay=td_atlas_pattern(ids[1],tile+1);
        expect(okay&&tile[0]==0xA5&&tile[17]==0xA5,
               "pattern getter copies exactly sixteen native bytes");
        if(!okay)continue;
        /* Decode the Game Boy wire format: each pixel's low/high plane are
         * row bytes0/1, with bit7 at the left. Include holes/right/bottom padding. */
        for(unsigned py=0;py<8;py++)for(unsigned px=0;px<8;px++) {
            unsigned bit=7-px;
            UBYTE colour=((tile[1+py*2]>>bit)&1)|(((tile[2+py*2]>>bit)&1)<<1);
            unsigned position=(y*8+py)*ORACLE_PADDED_WIDTH+x*8+px;
            expect(colour==oracle_pixels[position],
                   "decoded native dictionary pixel matches independent registered collision/water oracle");
        }
    }
    for(unsigned y=0;y<tile_height;y++)for(unsigned count=1;count<=20;count++)
        for(unsigned x=0;x+count<=tile_width;x++) {
            for(unsigned i=0;i<22;i++)ids[i]=54321;
            int okay=td_atlas_row((UBYTE)x,(UBYTE)y,(UBYTE)count,ids+1);
            expect(okay&&ids[0]==54321&&ids[count+1]==54321,
                   "every legal one-to-twenty tile row is bounded at both sides");
            if(!okay)continue;
            expect(!memcmp(ids+1,rows+y*tile_width+x,count*sizeof(UWORD)),
                   "row slices agree with independently retrieved whole-world pattern IDs");
        }
    /* IDs must be a contiguous, fully referenced dictionary: a zero-filled or
     * missing pattern remains observable in the exhaustive pixel comparison. */
    UWORD highest=0;
    for(unsigned i=0;i<tile_width*tile_height;i++)if(rows[i]>highest)highest=rows[i];
    expect(highest<tile_width*tile_height,"dictionary IDs remain bounded by the authored tile count");
    if(highest>=tile_width*tile_height)return;
    for(UWORD id=0;id<=highest;id++) {
        int referenced=0;
        for(unsigned i=0;i<tile_width*tile_height;i++)if(rows[i]==id){referenced=1;break;}
        expect(referenced,"every declared dictionary pattern is used by the actual atlas");
        expect(td_atlas_pattern(id,tile+1),"every referenced dictionary ID is retrievable");
    }
    memset(tile,0xA5,sizeof(tile));
    expect(!td_atlas_pattern(highest+1,tile+1)&&tile[0]==0xA5&&tile[17]==0xA5,
           "first invalid pattern is rejected without touching guard bytes");
    for(unsigned i=1;i<=16;i++)expect(tile[i]==0xA5,"invalid pattern leaves every destination byte unchanged");
    expect(!td_atlas_pattern(65535,tile+1),"maximum native pattern word is rejected");
    expect(!td_atlas_pattern(0,NULL),"null native pattern output is rejected");
    /* The real fullscreen ground viewport is20x12tiles. Font and marker slots
     * reserve the rest of the CGB tile bank, so172unique patterns is its cap. */
    for(unsigned y=0;y+12<=tile_height;y++)for(unsigned x=0;x+20<=tile_width;x++) {
        UWORD seen[240];unsigned used=0;
        for(unsigned py=0;py<12;py++)for(unsigned px=0;px<20;px++) {
            UWORD id=rows[(y+py)*tile_width+x+px];unsigned i;
            for(i=0;i<used;i++)if(seen[i]==id)break;
            if(i==used)seen[used++]=id;
        }
        expect(used<=172,"every actual twenty-by-twelve viewport fits native atlas pattern slots");
    }
    const UBYTE invalid[][3]={{0,0,0},{0,0,21},{0,0,255},{TD_ATLAS_TILE_WIDTH,0,1},{TD_ATLAS_TILE_WIDTH-1,0,2},
                            {TD_ATLAS_TILE_WIDTH-19,0,20},{0,TD_ATLAS_TILE_HEIGHT,1},{0,255,1},{255,0,1},{255,255,255}};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++) {
        for(unsigned j=0;j<22;j++)ids[j]=54321;
        expect(!td_atlas_row(invalid[i][0],invalid[i][1],invalid[i][2],ids+1),
               "invalid row count or out-of-bounds span is rejected");
        for(unsigned j=0;j<22;j++)expect(ids[j]==54321,"rejected row leaves its complete output and guards unchanged");
    }
    expect(!td_atlas_row(0,0,1,NULL),"null atlas row output is rejected");
}

static void test_district_names(void) {
    char name[21];
    for(unsigned index=0;index<ORACLE_DISTRICT_COUNT;index++) {
        const oracle_district_t *d=&oracle_districts[index];
        for(UWORD y=0;y<d->height/8;y++)for(UWORD x=0;x<d->width/8;x++) {
            memset(name,0xA5,sizeof(name));
            int okay=td_atlas_district(d->x+x,d->y+y,name+1);
            expect(okay&&name[0]==(char)0xA5&&name[20]==(char)0xA5,
                   "every atlas pixel resolves a bounded registered district name");
            if(okay)expect(!strcmp(name+1,d->name),"atlas district name matches registered geographic placement");
        }
    }
    /* A sparse 2D atlas may contain unregistered areas within its bounding
     * rectangle. Such pixels have no district name and must preserve outputs. */
    for(UWORD y=0;y<ORACLE_HEIGHT;y++)for(UWORD x=0;x<ORACLE_WIDTH;x++) {
        int registered=0;
        for(unsigned i=0;i<ORACLE_DISTRICT_COUNT;i++) {
            const oracle_district_t *d=&oracle_districts[i];
            if(x>=d->x&&x-d->x<d->width/8&&y>=d->y&&y-d->y<d->height/8){registered=1;break;}
        }
        if(registered)continue;
        memset(name,0xA5,sizeof(name));
        expect(!td_atlas_district(x,y,name+1),"unregistered atlas holes have no district name");
        for(unsigned j=0;j<sizeof(name);j++)expect(name[j]==(char)0xA5,"atlas hole leaves every name/output guard byte unchanged");
    }
    const UWORD invalid[][2]={{ORACLE_WIDTH,0},{0,ORACLE_HEIGHT},{65535,65535},
                             {0,ORACLE_PADDED_HEIGHT>ORACLE_HEIGHT?ORACLE_PADDED_HEIGHT-1:ORACLE_HEIGHT}};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++) {
        memset(name,0xA5,sizeof(name));
        expect(!td_atlas_district(invalid[i][0],invalid[i][1],name+1),
               "unpadded district bounds reject off-world positions");
        for(unsigned j=0;j<sizeof(name);j++)expect(name[j]==(char)0xA5,"invalid district lookup leaves every output byte unchanged");
    }
    expect(!td_atlas_district(0,0,NULL),"null atlas district name is rejected");
}

int main(void) {
    test_bounds();test_positions();test_rows_and_patterns();test_district_names();
    printf("Atlas host regressions: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
