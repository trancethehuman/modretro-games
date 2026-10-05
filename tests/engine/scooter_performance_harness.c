/* Appended after the shared actual-C hardware adapters/reset fixture. Each
 * binary executes either immutable original R8 C or production C, with the
 * same sandbox, unchanged fleet body helpers and registered raw terrain.
 * Emit exact initialized state fields; hardware/terrain call totals are
 * deliberately separate because fewer repeated reads are the optimization. */
static uint32_t replay_seed=0x183cb6a1;
static uint32_t replay_random(void){replay_seed=replay_seed*1664525u+1013904223u;return replay_seed;}
static void replay_uword(UWORD value){fputc(value&255,stdout);fputc(value>>8,stdout);}
static void replay_byte(UBYTE value){fputc(value,stdout);}
static void replay_state(UBYTE result){
    replay_byte(result);fwrite(&td,1,sizeof(td),stdout);
    for(unsigned i=0;i<2;i++){
        td_rider_t *r=&td_scooter_riders[i];replay_uword(r->u);replay_uword(r->v);replay_byte(r->leg);replay_byte(r->hit);replay_byte(r->flags);
    }
    replay_byte(td_scooter_district);replay_byte(td_scooter_elapsed);
    fwrite(td_sandbox_parked,1,sizeof(td_sandbox_parked),stdout);
    fwrite(td_sandbox_drivers,1,sizeof(td_sandbox_drivers),stdout);
    fwrite(td_sandbox_headings,1,sizeof(td_sandbox_headings),stdout);
    replay_uword(td_sandbox_seed);replay_uword(td_sandbox_last_u);replay_uword(td_sandbox_last_v);
    replay_byte(td_sandbox_district);replay_byte(td_sandbox_mask);replay_byte(td_sandbox_owner);replay_byte(td_sandbox_skin);
    replay_byte(td_sandbox_custom_player);replay_byte(td_sandbox_ready);
    fwrite(td_traffic_u,1,sizeof(td_traffic_u),stdout);fwrite(td_traffic_v,1,sizeof(td_traffic_v),stdout);
    fwrite(actors,1,sizeof(actors),stdout);
    replay_byte(tile_hit_x);replay_byte(tile_hit_y);replay_uword(td.speed);replay_uword(transfers);replay_uword(saves);
    replay_uword(parking_calls);replay_uword(render_calls);fwrite(rendered,1,sizeof(rendered),stdout);
}
int main(void){
    static const UWORD elapsed[]={0,1,3,7,8,9,15,16,24,31,56,64,65,127,600,64000};
    for(unsigned d=0;d<7;d++)for(unsigned scenario=0;scenario<80;scenario++){
        reset_case(d);use_raw=scenario&1;memset(rendered,0,sizeof(rendered));
        for(unsigned i=0;i<2;i++){
            UBYTE leg=replay_random()%4;
            td_scooter_riders[i].u=td_scooter_routes[d][i][leg][0]*16;
            td_scooter_riders[i].v=td_scooter_routes[d][i][leg][1]*16;
            td_scooter_riders[i].leg=(leg+1)&3;
            td_scooter_riders[i].flags=scenario%7==0?0:scenario%5==0?TD_SCOOTER_ACTIVE|TD_SCOOTER_WRECK|((scenario%4)<<2)|((scenario%8)<<4):TD_SCOOTER_ACTIVE;
            td_scooter_riders[i].hit=scenario%13==0?96:0;
        }
        td_sandbox_parked[1]=(td_parked_t){584*16,744*16,d,3,TD_NONE,0,1,0};
        for(unsigned step=0;step<80;step++){
            td.seconds=step/3;td.subsecond=step%3*20;td.mode=step%13==0?TD_PAUSE:step%11==0?TD_RIDE:TD_ROAM;
            td.onfoot=(step%7)==0;td.wanted=scenario%4;
            td.u=(32+replay_random()%960)*16;td.v=(32+replay_random()%912)*16;
            if(scenario%8==0){td.u=560*16;td.v=720*16;}
            td.park_u=td.u;td.park_v=td.v;td.park_district=step%9==0?(d+1)%7:d;
            td_streetcar_ride_view=step%17==0;td_streetcar_view_district=step%19==0?(d+1)%7:d;
            td_streetcar_focus_u=(48+replay_random()%928)*16;td_streetcar_focus_v=(48+replay_random()%880)*16;
            current_district=step%23==0?TD_DISTRICT_NONE:d;
            for(unsigned i=0;i<8;i++){
                td_traffic_u[i]=(16+replay_random()%992)*16;td_traffic_v[i]=(16+replay_random()%944)*16;
                actors[9+i].flags=replay_random()%4?ACTOR_FLAG_HIDDEN:0;
                actors[9+i].pos.x=(16+replay_random()%992)*32;actors[9+i].pos.y=(16+replay_random()%944)*32;
            }
            if(step%5==0){td_traffic_u[7]=td_scooter_riders[0].u;td_traffic_v[7]=td_scooter_riders[0].v;}
            if(step%29==0)td_traffic_u[scenario%8]=65535;
            if(step%31==0){actors[9+scenario%8].flags=0;actors[9+scenario%8].pos.x=0;}
            rail_clear=step%9!=0;prop_clear=step%10!=0;wall_x=step%21==0?td_scooter_riders[0].u>>7:-1;
            tile_hit_x=41;tile_hit_y=92;
            UBYTE result=td_scooter_update(elapsed[(step+scenario)%16]);
            if(step%14==0&&d!=5){
                td.mode=TD_ROAM;td.onfoot=0;td.vehicle=scenario%4;td.speed=3+scenario%18;
                td.u=td_scooter_riders[0].u-14*16;td.v=td_scooter_riders[0].v;
                result|=td_scooter_ram(td.u,td.v,td.u+8,td.v)<<1;
            }
            if(step%3==0)result|=td_scooter_take_hits()<<2;
            draw_scroll_x=td.u>>4;draw_scroll_y=td.v>>4;render_calls=0;memset(rendered,0,sizeof(rendered));td_scooter_render();
            result|=td_scooter_foot_clear(td.u,td.v)<<4;
            result|=td_scooter_clear(td.u,td.v,td.u+8,td.v,7,scenario%4==0?240:TD_NONE)<<5;
            replay_state(result);
        }
    }
    return 0;
}
