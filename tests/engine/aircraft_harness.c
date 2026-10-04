/* Pure production flight state, with the real 8/16-bit storage widths.
 * These fixtures establish logic, not GBDK timing, rendering or hardware. */
#include <stdint.h>
#include <stdio.h>
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
static int duration(void){return td_aircraft.kind?448:224;}
static int speed(void){return td_aircraft.kind?12:24;}
static void check_frame(void) {
    unsigned frame=td_aircraft_frame();
    expect(frame<12,"frame is one of twelve aircraft poses");
    expect(frame%4==td_aircraft.direction,"pose agrees with direction");
    expect(td_aircraft.kind?(frame>=4):(frame<4),"pose agrees with vehicle type");
    if(td_aircraft.kind)expect((frame>=8)==((td_aircraft.ticks&8)!=0),"rotor follows elapsed phase");
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
    unsigned distribution[2][4]={{0}},min_initial=65535,max_initial=0,min_repeat=65535,max_repeat=0;
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
        expect(td_aircraft.kind==(second&1)&&td_aircraft.direction==((second>>1)&3),
               "spawn type/direction match independent RNG oracle");
        expect(td_aircraft.kind<2&&td_aircraft.direction<4,"spawn state remains in finite ranges");
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
    for(unsigned type=0;type<2;type++)for(unsigned direction=0;direction<4;direction++)
        expect(distribution[type][direction]>=8191&&distribution[type][direction]<=8193,
               "exhaustive seeds distribute both aircraft in all four directions");
    expect(seen_frames==0xFFF,"exhaustive lifetime endpoints cover all twelve poses");
}

static void focus_and_partition(void) {
    const UWORD focus[]={0,1,79,80,81,87,88,89,500,919,920,921,943,944,945,1024,65535};
    unsigned found[8]={0},selected[8]={0};
    for(unsigned seed=1;seed<=65535;seed++){
        td_aircraft_reset((UWORD)seed);td_aircraft_update(td_aircraft.wait,500,500);
        unsigned index=td_aircraft.kind*4+td_aircraft.direction;
        if(!found[index]){selected[index]=seed;found[index]=1;}
    }
    for(unsigned pattern=0;pattern<8;pattern++){
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
                    case 0:expected_u-=112;expected_v+=jitter;break;
                    case 1:expected_v-=112;expected_u+=jitter;break;
                    case 2:expected_u+=112;expected_v+=jitter;break;
                    case 3:expected_v+=112;expected_u+=jitter;break;
                }
                expect(td_aircraft.u==expected_u*16&&td_aircraft.v==expected_v*16,
                       "spawn matches clamped north-up camera and signed edge positions");
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
                uint16_t last=next_seed(next_seed(td_aircraft.seed));
                td_aircraft_update(65535,focus[x],focus[y]);
                expect(!td_aircraft.active&&td_aircraft.ticks==length&&td_aircraft.seed==last,
                       "maximum waiting elapsed starts and retires at most one flight");
                expect(td_aircraft.wait>=1200&&td_aircraft.wait<=2223,"maximum waiting elapsed retains rare cooldown");
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
        expect(td_aircraft.ticks==17,"spawn consumes only its pre-flight wait");
    }
    td_aircraft_reset(0);td_aircraft_state_t zero=td_aircraft;
    td_aircraft_reset(0x9D27);
    expect(equal_state(zero,td_aircraft),"zero seed uses documented nonzero fallback");
}

int main(void) {
    expect(sizeof(UWORD)==2&&sizeof(WORD)==2&&sizeof(UBYTE)==1,
           "host fixture keeps Game Boy storage widths");
    all_seeds();focus_and_partition();
    printf("%u aircraft checks, %u failures\n",checks,failures);
    return failures?1:0;
}
