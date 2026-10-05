/* Pure production flight state, with the real 8/16-bit storage widths.
 * These fixtures establish logic, not GBDK timing, rendering or hardware. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "td_aircraft.h"

static unsigned checks,failures;
static void expect(int condition,const char *name) {
    checks++;
    if(!condition){failures++;if(failures<30)fprintf(stderr,"FAIL %s\n",name);}
}

/* Independent bit-vector oracle: apply each linear XOR stage bit by bit.
 * It never uses the production shift expression or mutates production state. */
static uint16_t shift_xor(uint16_t value,int shift) {
    uint16_t result=0;
    for(int bit=0;bit<16;bit++){
        int source=bit-shift;
        unsigned output=(value>>bit)&1u;
        if(source>=0&&source<16)output^=(value>>source)&1u;
        result|=(uint16_t)(output<<bit);
    }
    return result;
}
static uint16_t next_seed(uint16_t value) {
    value=shift_xor(value,7);
    value=shift_xor(value,-9);
    value=shift_xor(value,8);
    return value?value:0x9D27;
}
static int equal_state(td_aircraft_state_t a,td_aircraft_state_t b) {
    return a.u==b.u&&a.v==b.v&&a.wait==b.wait&&a.ticks==b.ticks&&
        a.seed==b.seed&&a.active==b.active&&a.kind==b.kind&&a.direction==b.direction;
}
static int duration(void){return td_aircraft.kind==1?448:224;}
static int speed(void){return td_aircraft.kind==1?12:24;}
static void check_frame(void) {
    unsigned frame=td_aircraft_frame();
    expect(frame<12||(frame>=14&&frame<18),"frame is an original aircraft pose or one of four new jet shadows, never an ellipse/empty");
    expect((td_aircraft.kind==2?frame-14:frame)%4==td_aircraft.direction,"pose agrees with direction");
    expect(td_aircraft.kind==2?frame>=14:td_aircraft.kind==1?(frame>=4&&frame<12):(frame<4),"pose agrees with vehicle type");
    if(td_aircraft.kind==1)expect((frame>=8)==((td_aircraft.ticks&8)!=0),"rotor follows elapsed phase");
    if(td_aircraft.kind==2)expect(frame==14+td_aircraft.direction,"jet never borrows a helicopter rotor phase");
}
static void check_position(td_aircraft_state_t start,unsigned elapsed) {
    int distance=(int)elapsed*speed();
    int du=td_aircraft.direction==0?distance:td_aircraft.direction==2?-distance:0;
    int dv=td_aircraft.direction==1?distance:td_aircraft.direction==3?-distance:0;
    expect(td_aircraft.u==(int)start.u+du&&td_aircraft.v==(int)start.v+dv,
           "flight displacement is cardinal and maintains momentum");
    expect(td_aircraft.u>=-4096&&td_aircraft.u<=20480&&
           td_aircraft.v>=-4096&&td_aircraft.v<=20480,"cosmetic coordinates stay signed Q4");
}

static void all_seeds(void) {
    unsigned distribution[3][4]={{0}},min_initial=65535,max_initial=0,min_repeat=65535,max_repeat=0;
    unsigned seen_frames=0;
    for(unsigned initial=0;initial<=65535;initial++){
        uint16_t first=next_seed(initial?(uint16_t)initial:0x9D27);
        uint16_t second=next_seed(first),third=next_seed(second);
        td_aircraft_reset((UWORD)initial);
        expect(td_aircraft.seed==first,"seed/reset matches independent 16-bit oracle");
        expect(!td_aircraft.active&&!td_aircraft.ticks&&!td_aircraft.u&&!td_aircraft.v,
               "reset drops old decorative flight");
        expect(td_aircraft.wait==360+(first&127),"initial wait matches deterministic sample");
        if(td_aircraft.wait<min_initial)min_initial=td_aircraft.wait;
        if(td_aircraft.wait>max_initial)max_initial=td_aircraft.wait;
        td_aircraft_state_t frozen=td_aircraft;
        td_aircraft_update(0,65535,65535);
        expect(equal_state(frozen,td_aircraft),"zero elapsed freezes waiting state and RNG");
        td_aircraft_update(td_aircraft.wait-1,560,720);
        expect(!td_aircraft.active&&td_aircraft.wait==1&&td_aircraft.seed==first,
               "no early spawn immediately before initial window");
        td_aircraft_update(1,560,720);
        expect(td_aircraft.active&&!td_aircraft.ticks&&td_aircraft.seed==second,
               "flight starts exactly at wait boundary");
        expect(td_aircraft.kind==((second&1)?1:(second&0x100)?2:0)&&td_aircraft.direction==((second>>1)&3),
               "spawn type/direction match independent RNG oracle");
        expect(td_aircraft.kind<3&&td_aircraft.direction<4,"spawn state remains in finite ranges");
        distribution[td_aircraft.kind][td_aircraft.direction]++;
        td_aircraft_state_t start=td_aircraft;
        td_aircraft_update(0,0,0);
        expect(equal_state(start,td_aircraft),"zero elapsed freezes active flight and RNG");
        check_frame();seen_frames|=1u<<td_aircraft_frame();
        unsigned length=(unsigned)duration();
        td_aircraft_update((UWORD)(length-1),560,720);
        expect(td_aircraft.active&&td_aircraft.ticks==length-1&&td_aircraft.seed==second,
               "flight persists one tick before lifetime");
        check_position(start,length-1);check_frame();seen_frames|=1u<<td_aircraft_frame();
        td_aircraft_update(1,560,720);
        expect(!td_aircraft.active&&td_aircraft.ticks==length,"flight ends at exact lifetime");
        check_position(start,length);
        expect(td_aircraft.seed==third&&td_aircraft.wait==1200+(third&1023),
               "repeat flight gets a single rare cooldown draw");
        if(td_aircraft.wait<min_repeat)min_repeat=td_aircraft.wait;
        if(td_aircraft.wait>max_repeat)max_repeat=td_aircraft.wait;
        frozen=td_aircraft;
        td_aircraft_update(0,0,0);
        expect(equal_state(frozen,td_aircraft),"zero elapsed freezes rare cooldown");
        td_aircraft_update(td_aircraft.wait-1,560,720);
        expect(!td_aircraft.active&&td_aircraft.wait==1,"cooldown remains closed until final tick");
    }
    expect(min_initial==360&&max_initial==487,"all initial wait endpoints occur");
    expect(min_repeat==1200&&max_repeat==2223,"all rare cooldown endpoints occur");
    for(unsigned type=0;type<3;type++)for(unsigned direction=0;direction<4;direction++)
        expect(distribution[type][direction]>=(type==1?8191u:4095u)&&distribution[type][direction]<=(type==1?8193u:4097u),
               "all seeds preserve helicopter frequency while splitting the original planes into commuter/jet flights in all directions");
    expect(seen_frames==0x3CFFF,"exhaustive lifetime endpoints cover all twelve original poses and four jet shadows");
}

static void focus_and_partition(void) {
    const UWORD focus[]={0,1,79,80,81,87,88,89,500,919,920,921,943,944,945,1024,65535};
    unsigned found[12]={0},selected[12]={0};
    for(unsigned seed=1;seed<=65535;seed++){
        td_aircraft_reset((UWORD)seed);td_aircraft_update(td_aircraft.wait,500,500);
        unsigned index=td_aircraft.kind*4+td_aircraft.direction;
        if(!found[index]){selected[index]=seed;found[index]=1;}
    }
    for(unsigned pattern=0;pattern<12;pattern++){
        expect(found[pattern],"each aircraft direction has a usable fixture seed");
        for(unsigned x=0;x<sizeof(focus)/sizeof(*focus);x++)
            for(unsigned y=0;y<sizeof(focus)/sizeof(*focus);y++){
                td_aircraft_reset((UWORD)selected[pattern]);
                uint16_t second=next_seed(td_aircraft.seed);
                td_aircraft_update(td_aircraft.wait,focus[x],focus[y]);
                int cx=focus[x]<80?80:focus[x]>944?944:focus[x];
                int cy=focus[y]<88?72:focus[y]>920?904:(int)focus[y]-16;
                int jitter=((second>>4)&63)-32;
                int expected_u=cx,expected_v=cy;
                switch(pattern%4){
                    case 0:expected_u-=128;expected_v+=jitter;break;
                    case 1:expected_v-=128;expected_u+=jitter;break;
                    case 2:expected_u+=128;expected_v+=jitter;break;
                    case 3:expected_v+=128;expected_u+=jitter;break;
                }
                expect(td_aircraft.u==expected_u*16&&td_aircraft.v==expected_v*16,
                       "spawn matches clamped north-up camera and signed edge positions");
                /* Independently intersect a conservative entire craft/shadow
                 * rectangle (with camera slack) against160x144 actual pixels. */
                int screen_u=td_aircraft.u/16-(cx-80),screen_v=td_aircraft.v/16-(cy-72);
                expect(screen_u+32<0||screen_u-24>159||screen_v+48<0||screen_v-24>143,
                       "every whole aircraft and shadow begins beyond the viewport including camera slack");
                td_aircraft_state_t start=td_aircraft;
                unsigned length=(unsigned)duration();
                td_aircraft_update((UWORD)(length-1),focus[x],focus[y]);
                td_aircraft_state_t batched=td_aircraft;
                td_aircraft=start;
                for(unsigned tick=0;tick<length-1;tick++){
                    /* Moving the camera after spawn must not drag aircraft. */
                    td_aircraft_update(1,(UWORD)(tick&1?0:65535),(UWORD)(tick&1?65535:0));
                    check_frame();
                }
                expect(equal_state(batched,td_aircraft),"one-tick and batched flight agree despite camera movement");
                check_position(start,length-1);
                td_aircraft=start;
                td_aircraft_update(65535,focus[x],focus[y]);
                expect(!td_aircraft.active&&td_aircraft.ticks==length,
                       "maximum elapsed stops one flight without a catch-up swarm");
                check_position(start,length);
                expect(td_aircraft.seed==next_seed(start.seed)&&td_aircraft.wait>=1200&&td_aircraft.wait<=2223,
                       "maximum active elapsed draws exactly one bounded cooldown");
                td_aircraft_reset((UWORD)selected[pattern]);
                uint16_t spawned_seed=next_seed(td_aircraft.seed);
                td_aircraft_update(65535,focus[x],focus[y]);
                expect(td_aircraft.active&&td_aircraft.ticks==0&&td_aircraft.seed==spawned_seed,
                       "maximum waiting elapsed presents exactly one new flight at its offscreen origin");
                expect(td_aircraft.u==expected_u*16&&td_aircraft.v==expected_v*16,
                       "delayed waiting update cannot skip the offscreen arrival path");
            }
        td_aircraft_reset((UWORD)selected[pattern]);
        UWORD wait=td_aircraft.wait;
        td_aircraft_update(wait+17,500,500);
        td_aircraft_state_t batched=td_aircraft;
        td_aircraft_reset((UWORD)selected[pattern]);
        td_aircraft_update(wait-1,500,500);td_aircraft_update(18,500,500);
        /* wait is dormant while active: a split wait retains its last 1,
         * whereas the batched call retains its initial duration. Compare
         * the active flight and RNG; cooldown is replaced at retirement. */
        batched.wait=td_aircraft.wait;
        expect(equal_state(batched,td_aircraft),"elapsed residual crossing spawn agrees with split update");
        expect(td_aircraft.ticks==0,"waiting residual never advances a newly spawned flight into the view");
        td_aircraft_state_t arrived=td_aircraft;
        td_aircraft_update(17,500,500);
        expect(td_aircraft.ticks==17,"subsequent active update advances the established flight");
        check_position(arrived,17);
    }
    td_aircraft_reset(0);td_aircraft_state_t zero=td_aircraft;
    td_aircraft_reset(0x9D27);
    expect(equal_state(zero,td_aircraft),"zero seed uses documented nonzero fallback");
}

static unsigned bounded_focus(int value,unsigned limit){
    return value<8?8:value>(int)limit?limit:(unsigned)value;
}
static int visible_complete_flight(unsigned focus_u,unsigned focus_v){
    int left=focus_u<80?0:focus_u>944?864:(int)focus_u-80;
    int top=focus_v<88?0:focus_v>920?832:(int)focus_v-88;
    int u=td_aircraft.u>=0?td_aircraft.u/16:-((-td_aircraft.u+15)/16);
    int v=td_aircraft.v>=0?td_aircraft.v/16:-((-td_aircraft.v+15)/16);
    return !(u+32<left||u-24>left+159||v+48<top||v-24>top+143);
}
static void followed_flights(void){
    unsigned selected[12]={0};
    for(unsigned seed=1;seed<=65535;seed++){
        td_aircraft_reset(seed);td_aircraft_update(td_aircraft.wait,500,500);
        unsigned pattern=td_aircraft.kind*4+td_aircraft.direction;
        if(!selected[pattern])selected[pattern]=seed;
    }
    for(unsigned pattern=0;pattern<12;pattern++){
        td_aircraft_reset(selected[pattern]);td_aircraft_update(td_aircraft.wait,500,500);
        td_aircraft_state_t start=td_aircraft;
        unsigned minimum=duration(),continued=0,ended=0;
        expect(!visible_complete_flight(500,500),"Followed flight genuinely begins beyond the complete viewport bounds");
        for(unsigned tick=1;tick<2000;tick++){
            /* A moving courier follows at60px/s for a plane,30px/s for a
             * helicopter, stopping at the real district camera boundary. */
            int advance=(int)(tick*(td_aircraft.kind==1?1:2)/2);
            int x=500+(pattern%4==0?advance:pattern%4==2?-advance:0);
            int y=500+(pattern%4==1?advance:pattern%4==3?-advance:0);
            unsigned focus_u=bounded_focus(x,1016),focus_v=bounded_focus(y,968);
            td_aircraft_update(1,focus_u,focus_v);check_position(start,tick);
            if(!td_aircraft.active){
                expect(tick>=minimum&&!visible_complete_flight(focus_u,focus_v),
                       "Following the camera cannot retire an aircraft or shadow while it intersects the viewport");
                expect(td_aircraft.seed==next_seed(start.seed),"A completed followed flight draws exactly one cooldown");
                ended=1;break;
            }
            if(tick==minimum)expect(visible_complete_flight(focus_u,focus_v),
                    "The chase fixture genuinely keeps the whole flight visible at its former timer cutoff");
            if(tick>minimum)continued=1;
        }
        expect(continued&&ended,"Every direction/type extends visibly then leaves the finite district before retirement");
    }
}

static int whole_offscreen(unsigned fu,unsigned fv){
    int left=fu<80?0:fu>944?864:(int)fu-80;
    int top=fv<88?0:fv>920?832:(int)fv-88;
    int u=td_aircraft.u/16,v=td_aircraft.v/16;
    return u+32<left||u-24>left+159||v+48<top||v-24>top+143;
}
static void police_stationary(void){
    td_aircraft_reset(0x4721);td_aircraft_police_target(2,560*16,480*16,1);
    td_aircraft_update(1,560,480);
    for(unsigned i=0;i<260;i++)td_aircraft_update(1,560,480);
    expect(td_aircraft.active&&td_aircraft.kind&&td_aircraft_police_observed(),
           "Offscreen helicopter approaches and actually observes a stationary exposed courier");
}
static void police_pursuit_checks(void){
    for(unsigned seed=0;seed<65536;seed++){
        td_aircraft_reset(seed);td_aircraft_police_target(1,560*16,480*16,1);
        td_aircraft_update(65535,560,480);
        expect(td_aircraft.active&&td_aircraft.kind&&td_aircraft.ticks==0&&whole_offscreen(560,480),
               "Wanted helicopter birth remains wholly offscreen even after a delayed update");
        expect(!td_aircraft_police_observed(),"An incoming offscreen helicopter cannot already observe the courier");
        td_aircraft_state_t first=td_aircraft;
        td_aircraft_update(65535,560,480);
        expect(td_aircraft.ticks==16&&abs(td_aircraft.u-first.u)<=16*16&&abs(td_aircraft.v-first.v)<=16*16,
               "A delayed pursuit update has at most sixteen bounded world steps without snapping");
        check_frame();
    }
    police_stationary();
    td_aircraft_state_t visible=td_aircraft;
    td_aircraft_police_target(2,560*16,480*16,0);
    expect(!td_aircraft_police_observed(),"Authored cover immediately removes aerial observation");
    td_aircraft_update(0,560,480);expect(equal_state(visible,td_aircraft),"Pause freezes pursuit/rotor/world coordinates");
    /*Two different unseen targets produce the SAME last-known trajectory. */
    td_aircraft_state_t hidden_path[100];
    for(unsigned pass=0;pass<2;pass++){
        police_stationary();td_aircraft_police_target(2,(pass?900:100)*16,(pass?800:100)*16,0);
        for(unsigned i=0;i<100;i++){
            td_aircraft_state_t before=td_aircraft;td_aircraft_update(1,560,480);
            expect(abs(td_aircraft.u-before.u)<=14&&abs(td_aircraft.v-before.v)<=14,
                   "Pursuit respects attention-dependent velocity and bounded drift each real tick");
            expect(!td_aircraft_police_observed(),"Hidden courier never magically updates aerial sight");
            if(!pass)hidden_path[i]=td_aircraft;
            else expect(equal_state(hidden_path[i],td_aircraft),"Unseen destination cannot steer the helicopter omnisciently");
        }
    }
    police_stationary();td_aircraft_police_target(0,560*16,480*16,1);
    unsigned lived=0;
    for(unsigned i=0;i<500&&td_aircraft.active;i++){
        td_aircraft_state_t before=td_aircraft;td_aircraft_update(1,560,480);
        expect(abs(td_aircraft.u-before.u)<=16&&abs(td_aircraft.v-before.v)<=16,
               "Resolved police attention flies away continuously without a camera snap");
        if(td_aircraft.active){lived++;expect(!whole_offscreen(560,480),"Visible departure remains alive until whole-craft exit");}
        else expect(whole_offscreen(560,480),"Departure retires only after hull/shadow fully clear the viewport");
    }
    expect(lived>20&&!td_aircraft.active&&td_aircraft.wait==1800,"Visible helicopter departs and leaves a thirty-second aerial escape window");
    td_aircraft_police_target(3,560*16,480*16,1);td_aircraft_update(1799,560,480);
    expect(!td_aircraft.active&&td_aircraft.wait==1,"Escaped pursuit cannot instantly respawn beside the courier");
    td_aircraft_update(1,560,480);expect(td_aircraft.active&&td_aircraft.kind&&whole_offscreen(560,480),
           "A renewed search starts beyond the screen only after the full escape window");
    /*A visible ambient plane keeps its identity and path until it leaves. */
    td_aircraft_reset(1);td_aircraft.active=1;td_aircraft.kind=0;td_aircraft.direction=0;
    td_aircraft.u=560*16;td_aircraft.v=480*16;td_aircraft.ticks=0;
    td_aircraft_police_target(3,560*16,480*16,1);td_aircraft_update(1,560,480);
    expect(td_aircraft.kind==0&&td_aircraft.u==560*16+24&&td_aircraft.v==480*16,
           "Visible plane never pops into a differently shaped police helicopter");
    td_aircraft_reset(1);td_aircraft.active=1;td_aircraft.kind=2;td_aircraft.direction=0;
    td_aircraft.u=560*16;td_aircraft.v=480*16;td_aircraft.ticks=7;
    td_aircraft_police_target(3,560*16,480*16,1);td_aircraft_update(1,560,480);
    expect(td_aircraft.kind==2&&td_aircraft.u==560*16+24&&td_aircraft.v==480*16&&
           td_aircraft.ticks==8&&td_aircraft_frame()==14&&!td_aircraft_police_observed(),
           "visible jet retains its exact identity, plane pacing and ground-only frame when police attention rises");
    td_aircraft_police_target(3,65535,65535,1);expect(!td_aircraft_police_observed(),"Invalid Q4 targets cannot create spurious sight");
}
int main(void) {
    expect(sizeof(UWORD)==2&&sizeof(WORD)==2&&sizeof(UBYTE)==1,
           "host fixture keeps Game Boy storage widths");
    all_seeds();focus_and_partition();followed_flights();police_pursuit_checks();
    printf("%u aircraft checks, %u failures\n",checks,failures);
    return failures?1:0;
}
