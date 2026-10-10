/* Original Toronto Dispatch soundtrack and effects, MIT copyright 2026 Hai Nghiem.
   The existing GBVM timer/music manager owns IRQs, banks and APU writes.
   Each song (title, day, night, chase) is its own autobanked td_song_*.c:
   music_load() hands GBVM a song's bank and address, and the music interrupt
   switches to that bank before hUGEDriver reads it, so this file never reads
   song data itself. Event cues and engine samples stay in this file's bank. */
#pragma bank 255
#include <stddef.h>
#include "bankdata.h"
#include "td_audio.h"
#include "td_game.h"
#include "td_daynight.h"
#include "music_manager.h"
#include "sfx_player.h"

BANKREF(td_audio_assets)
BANKREF_EXTERN(td_audio_assets)

/* The pinned GBVM music_pause() is a near function in music_manager's bank.
   Resolve that compilation unit's bank from its exported banked init function;
   execute this wrapper in HOME so switching banks cannot unmap its own code. */
static UBYTE td_audio_music_bank(void) NONBANKED NAKED {
#if defined(__SDCC)
    __asm
        .globl b_music_init_driver
        ld a, #b_music_init_driver
        ret
    __endasm;
#endif
}

static void td_audio_pause_music(UBYTE pause) NONBANKED {
    UBYTE saved_bank = CURRENT_BANK;
    SWITCH_ROM(td_audio_music_bank());
    music_pause(pause);
    SWITCH_ROM(saved_bank);
}

/* BEGIN GENERATED AUDIO: scripts/create_audio.py */
/* Original soundtrack and effects. Edit project/original-audio/ and run
   scripts/create_audio.py; each song is its own autobanked td_song_*.c. */
#define TD_SONG_TITLE 0 /* Skyline Call: 32 bars, about 120 BPM, 6108 bytes */
#define TD_SONG_DAY 1 /* King Street Iron: 40 bars, about 137 BPM, 5746 bytes */
#define TD_SONG_NIGHT 2 /* Six After Dark: 32 bars, about 87 BPM, 3990 bytes */
#define TD_SONG_CHASE 3 /* Gardiner Heat: 32 bars, about 160 BPM, 5148 bytes */
#define TD_SONGS 4
BANKREF_EXTERN(td_song_title)
extern const hUGESong_t td_song_title;
BANKREF_EXTERN(td_song_day)
extern const hUGESong_t td_song_day;
BANKREF_EXTERN(td_song_night)
extern const hUGESong_t td_song_night;
BANKREF_EXTERN(td_song_chase)
extern const hUGESong_t td_song_chase;
static const far_ptr_t td_audio_songs[TD_SONGS] = {TO_FAR_PTR_T(td_song_title), TO_FAR_PTR_T(td_song_day), TO_FAR_PTR_T(td_song_night), TO_FAR_PTR_T(td_song_chase)};
static const UBYTE td_audio_impact[] = {
    0xB2, 0xF8, 0x00, 0x40, 0x81, 0x58, 0x83, 0x7B, 0x00, 0x81, 0x35, 0x80,
    0xF2, 0xF8, 0x00, 0x40, 0x41, 0x43, 0x81, 0x7B, 0x00, 0x31, 0x53, 0x80,
    0x01, 0x28, 0x00, 0xC0, 0x01, 0x2B, 0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_pickup[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x71, 0x59, 0x87, 0x30, 0xF1, 0xF8, 0x00, 0x40,
    0x61, 0x7B, 0x87, 0x70, 0x01, 0x28, 0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_complete[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x81, 0x39, 0x87, 0x70, 0xF1, 0xF8, 0x00, 0x40,
    0x81, 0x59, 0x87, 0x70, 0xF1, 0xF8, 0x00, 0x40, 0x71, 0x7B, 0x87, 0x70,
    0xF1, 0xF8, 0x00, 0x40, 0x61, 0x9D, 0x87, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_transit[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x61, 0x21, 0x87, 0xB0, 0xF1, 0xF8, 0x00, 0x40,
    0x51, 0x6B, 0x87, 0xF0, 0x70, 0x01, 0x28, 0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_fail[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x71, 0xF7, 0x86, 0xB0, 0xF1, 0xF8, 0x00, 0x40,
    0x61, 0xB2, 0x86, 0xB0, 0xF1, 0xF8, 0x00, 0x40, 0x51, 0x72, 0x86, 0xF0,
    0x70, 0x01, 0x28, 0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_menu[] = {
    0xB1, 0xF8, 0x00, 0x40, 0x41, 0x39, 0x87, 0x01, 0x28, 0x00, 0xC0, 0x01,
    0x07,
};
static const UBYTE td_audio_brake[] = {
    0xB1, 0x7B, 0x00, 0x41, 0x16, 0x80, 0xB1, 0x7B, 0x00, 0x21, 0x26, 0x80,
    0x01, 0x2B, 0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE * const td_audio_cues[] = {td_audio_impact, td_audio_pickup, td_audio_complete, td_audio_transit, td_audio_fail, td_audio_menu};
static const UBYTE td_audio_cue_priorities[] = {8, 4, 8, 4, 8, 2};
static const UBYTE td_audio_cue_masks[] = {9, 1, 1, 1, 1, 1};
static const UBYTE td_audio_drone_car_0[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xE8, 0x81, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_car_1[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x14, 0x83, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_car_2[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x00, 0x84, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_car_3[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xB8, 0x84, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_truck_0[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x78, 0x80, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_truck_1[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x9A, 0x81, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_truck_2[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xAB, 0x82, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_truck_3[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x6E, 0x83, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_motorcycle_0[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x20, 0xBC, 0x83, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_motorcycle_1[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x20, 0xB8, 0x84, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_motorcycle_2[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x20, 0x8A, 0x85, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_motorcycle_3[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x20, 0x08, 0x86, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_scooter_0[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xC7, 0x82, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_scooter_1[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xF0, 0x83, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_scooter_2[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xCD, 0x84, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_drone_scooter_3[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x6A, 0x85, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE * const td_audio_drones[4][4] = {
    {td_audio_drone_car_0, td_audio_drone_car_1, td_audio_drone_car_2, td_audio_drone_car_3},
    {td_audio_drone_truck_0, td_audio_drone_truck_1, td_audio_drone_truck_2, td_audio_drone_truck_3},
    {td_audio_drone_motorcycle_0, td_audio_drone_motorcycle_1, td_audio_drone_motorcycle_2, td_audio_drone_motorcycle_3},
    {td_audio_drone_scooter_0, td_audio_drone_scooter_1, td_audio_drone_scooter_2, td_audio_drone_scooter_3},
};
static const UBYTE td_audio_rev_car_0[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0x9A, 0x81, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0xE8, 0x81, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0x8E, 0x82, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_car_1[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0xD4, 0x82, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0x14, 0x83, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0x96, 0x83, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_car_2[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0xCE, 0x83, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0x00, 0x84, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0x6B, 0x84, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_car_3[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0x8A, 0x84, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0xB8, 0x84, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0x13, 0x85, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_truck_0[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0x20, 0x80, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0x78, 0x80, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0x43, 0x81, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_truck_1[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0x43, 0x81, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0x9A, 0x81, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0x50, 0x82, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_truck_2[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0x60, 0x82, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0xAB, 0x82, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0x42, 0x83, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_truck_3[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0x2B, 0x83, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0x6E, 0x83, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0xE7, 0x83, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_motorcycle_0[] = {
    0x71, 0xF8, 0x00, 0x40, 0x30, 0x82, 0x83, 0xB1, 0xF8, 0x00, 0x40, 0x30,
    0xBC, 0x83, 0xF1, 0xF8, 0x00, 0x40, 0x32, 0x2E, 0x84, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_motorcycle_1[] = {
    0x71, 0xF8, 0x00, 0x40, 0x30, 0x8A, 0x84, 0xB1, 0xF8, 0x00, 0x40, 0x30,
    0xB8, 0x84, 0xF1, 0xF8, 0x00, 0x40, 0x32, 0x13, 0x85, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_motorcycle_2[] = {
    0x71, 0xF8, 0x00, 0x40, 0x30, 0x6A, 0x85, 0xB1, 0xF8, 0x00, 0x40, 0x30,
    0x8A, 0x85, 0xF1, 0xF8, 0x00, 0x40, 0x32, 0xCD, 0x85, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_motorcycle_3[] = {
    0x71, 0xF8, 0x00, 0x40, 0x30, 0xED, 0x85, 0xB1, 0xF8, 0x00, 0x40, 0x30,
    0x08, 0x86, 0xF1, 0xF8, 0x00, 0x40, 0x32, 0x3E, 0x86, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_scooter_0[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0x7F, 0x82, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0xC7, 0x82, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0x58, 0x83, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_scooter_1[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0xBC, 0x83, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0xF0, 0x83, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0x5E, 0x84, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_scooter_2[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0xA2, 0x84, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0xCD, 0x84, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0x24, 0x85, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_rev_scooter_3[] = {
    0x71, 0xF8, 0x00, 0x40, 0x40, 0x47, 0x85, 0xB1, 0xF8, 0x00, 0x40, 0x40,
    0x6A, 0x85, 0xF1, 0xF8, 0x00, 0x40, 0x42, 0xB2, 0x85, 0x70, 0x01, 0x28,
    0x00, 0xC0, 0x01, 0x07,
};
static const UBYTE * const td_audio_revs[4][4] = {
    {td_audio_rev_car_0, td_audio_rev_car_1, td_audio_rev_car_2, td_audio_rev_car_3},
    {td_audio_rev_truck_0, td_audio_rev_truck_1, td_audio_rev_truck_2, td_audio_rev_truck_3},
    {td_audio_rev_motorcycle_0, td_audio_rev_motorcycle_1, td_audio_rev_motorcycle_2, td_audio_rev_motorcycle_3},
    {td_audio_rev_scooter_0, td_audio_rev_scooter_1, td_audio_rev_scooter_2, td_audio_rev_scooter_3},
};
/* END GENERATED AUDIO */

#define TD_AUDIO_NO_CUE 255
#define TD_AUDIO_NO_SONG 255
#define TD_AUDIO_NO_STAGE 255
#define TD_AUDIO_ALL_CHANNELS (MUSIC_CH_1 | MUSIC_CH_2 | MUSIC_CH_3 | MUSIC_CH_4)
#define TD_AUDIO_BRAKE_PRIORITY 1
/* Kinds of the effect this file last started. */
#define TD_AUDIO_KIND_ENGINE 1
#define TD_AUDIO_KIND_BRAKE 2
#define TD_AUDIO_KIND_CUE 3
/* Police attention (stars) that brings in the chase theme. */
#define TD_AUDIO_CHASE_STARS 2
/* Frames a different world song must stay wanted before it starts, so a
   flickering state never restarts a tune; with one star left the chase
   winds down more slowly. */
#define TD_AUDIO_HOLD 60u
#define TD_AUDIO_CHASE_HOLD 180u
/* Music mode: at most one short engine rev per this many frames. */
#define TD_AUDIO_REV_GAP 30u

static UBYTE td_audio_mode, td_audio_active, td_audio_paused;
static UBYTE td_audio_pending, td_audio_effect_kind;
static UBYTE td_audio_song, td_audio_want, td_audio_stage;
static UWORD td_audio_last_ambient, td_audio_last_rev, td_audio_want_since;

/* Stop the current finite SFX through the public player, then restore the
   music manager's state exactly as its normal end-of-sample path does. */
static void td_audio_cut_effect(void) {
    CRITICAL {
        sfx_reset_sample();
        music_sound_cut_mask(music_mute_mask);
        music_mute_mask = MUTE_MASK_NONE;
        music_sfx_priority = MUSIC_SFX_PRIORITY_MINIMAL;
        music_effective_mute = driver_set_mute_mask(music_global_mute_mask);
    }
    td_audio_effect_kind = 0;
}

static void td_audio_cut_ambient(void) {
    if ((td_audio_effect_kind == TD_AUDIO_KIND_ENGINE || td_audio_effect_kind == TD_AUDIO_KIND_BRAKE) &&
        sfx_play_bank == BANK(td_audio_assets)) td_audio_cut_effect();
}

/* The tune plays in music + effects mode on the title screen and while the
   world clock runs; menus pause it in place. Music uses all four channels:
   an effect mutes only the channels it uses, and only while it plays. */
static void td_audio_apply_music(void) {
    UBYTE pause = (td_audio_mode != TD_AUDIO_FULL ||
                   !(td_audio_active || td.mode == TD_HELP));
    UBYTE mask = (td_audio_mode == TD_AUDIO_FULL) ? MUTE_MASK_NONE : TD_AUDIO_ALL_CHANNELS;
    CRITICAL {
        music_global_mute_mask = mask;
        /* hUGE initialization clears its own mask; restore it after a song
           change as well as after a mode/SFX transition. */
        mask |= music_mute_mask;
        if (music_effective_mute != mask || hUGE_mute_mask != mask)
            music_effective_mute = driver_set_mute_mask(mask);
        if (pause != td_audio_paused) {
            td_audio_pause_music(pause);
            td_audio_paused = pause;
        }
    }
}

/* Which song the current state calls for. Menus pause the tune but this
   keeps following the world, so a change made behind a menu (an arrest
   clears the stars) has already waited out its hold when play resumes. */
static UBYTE td_audio_choose(void) {
    if (td.mode == TD_HELP) return TD_SONG_TITLE;
    if (td.wanted >= TD_AUDIO_CHASE_STARS) return TD_SONG_CHASE;
    return td_daynight_lights ? TD_SONG_NIGHT : TD_SONG_DAY;
}

/* GBVM initializes the new song (hUGE_init) on its next music tick. */
static void td_audio_load(UBYTE song) {
    td_audio_song = td_audio_want = song;
    CRITICAL { music_load(td_audio_songs[song].bank, (const hUGESong_t *)td_audio_songs[song].ptr); }
}

/* Changes song only at a safe moment: the wanted song has held long enough,
   the tune is audible and no effect is playing or queued (hUGE_init cuts all
   four channels and clears the driver's mute mask). */
static void td_audio_pick_song(UWORD now) {
    UBYTE want = td_audio_choose();
    UWORD hold;
    if (want == td_audio_song) {
        td_audio_want = want;
        return;
    }
    if (want != td_audio_want) {
        td_audio_want = want;
        td_audio_want_since = now;
    }
    if (td_audio_paused || sfx_play_bank != SFX_STOP_BANK || td_audio_pending != TD_AUDIO_NO_CUE)
        return;
    /* The chase lingers while stars remain; once they are gone (lost,
       resprayed or arrested) it ends like any other change. */
    hold = (want == TD_SONG_TITLE || td_audio_song == TD_SONG_TITLE) ? 0u :
           ((td_audio_song == TD_SONG_CHASE && td.wanted) ? TD_AUDIO_CHASE_HOLD : TD_AUDIO_HOLD);
    if ((UWORD)(now - td_audio_want_since) >= hold) td_audio_load(want);
}

void td_audio_init(void) BANKED {
    td_audio_mode = TD_AUDIO_FULL;
    td_audio_active = FALSE;
    td_audio_paused = FALSE;
    td_audio_pending = TD_AUDIO_NO_CUE;
    td_audio_effect_kind = 0;
    td_audio_stage = TD_AUDIO_NO_STAGE;
    td_audio_last_ambient = td_audio_last_rev = td_audio_want_since = sys_time;
    td_audio_cut_effect();
    td_audio_song = TD_AUDIO_NO_SONG;
    td_audio_load(td_audio_choose());
    td_audio_apply_music();
}

UBYTE td_audio_get_mode(void) BANKED { return td_audio_mode; }

void td_audio_set_mode(UBYTE mode) BANKED {
    if (mode >= TD_AUDIO_MODES) mode = TD_AUDIO_FULL;
    if (mode == td_audio_mode) return;
    td_audio_mode = mode;
    td_audio_pending = TD_AUDIO_NO_CUE;
    td_audio_cut_effect();
    /* music_pause cuts both music and SFX. The explicit music cut also covers
       switching between two modes that were already paused. */
    CRITICAL { music_sound_cut_mask(TD_AUDIO_ALL_CHANNELS); }
    td_audio_apply_music();
    td_audio_last_ambient = sys_time;
}

void td_audio_play(UBYTE cue) BANKED {
    if (cue >= TD_AUDIO_CUES || td_audio_mode == TD_AUDIO_SILENT) return;
    if (td_audio_pending == TD_AUDIO_NO_CUE ||
        td_audio_cue_priorities[cue] >= td_audio_cue_priorities[td_audio_pending])
        td_audio_pending = cue;
}

static void td_audio_start(const UBYTE *sample, UBYTE mask,
                           UBYTE priority, UBYTE kind) {
    CRITICAL {
        if (priority >= music_sfx_priority) {
            music_play_sfx(BANK(td_audio_assets), sample, mask, priority);
            td_audio_effect_kind = kind;
        }
    }
}

void td_audio_update(WORD speed, UBYTE vehicle, UBYTE onfoot,
                     UBYTE braking, UBYTE world_active) BANKED {
    UWORD magnitude, now = sys_time;
    UBYTE cue, stage;
    if (world_active != td_audio_active) {
        td_audio_active = world_active;
        td_audio_cut_ambient();
        td_audio_last_ambient = now;
    }
    td_audio_apply_music();
    td_audio_pick_song(now);
    /* A new song starts on GBVM's next music tick and cuts every channel;
       hold effects until then so the start cannot swallow them. */
    if (!td_audio_paused && music_next_track) return;
    /* Flush after pause transitions, so entering a result screen does not
       immediately cut the newly queued delivery/timeout jingle. */
    cue = td_audio_pending;
    td_audio_pending = TD_AUDIO_NO_CUE;
    if (cue < TD_AUDIO_CUES && td_audio_mode != TD_AUDIO_SILENT)
        td_audio_start(td_audio_cues[cue], td_audio_cue_masks[cue],
                       td_audio_cue_priorities[cue], TD_AUDIO_KIND_CUE);
    if (!world_active || td_audio_mode == TD_AUDIO_SILENT || onfoot) {
        td_audio_cut_ambient();
        td_audio_last_ambient = now;
        td_audio_stage = TD_AUDIO_NO_STAGE;
        return;
    }
    magnitude = (speed < 0) ? (UWORD)(-(speed + 1)) + 1u : (UWORD)speed;
    if (magnitude < 2) {
        td_audio_cut_ambient();
        td_audio_last_ambient = now;
        td_audio_stage = TD_AUDIO_NO_STAGE;
        return;
    }
    stage = (magnitude < 7u) ? 0 : ((magnitude < 13u) ? 1 : ((magnitude < 19u) ? 2 : 3));
    if (vehicle > 3) vehicle = 0;
    if (braking && speed > 6) {
        /* Slowing two stages re-arms the rev for the next acceleration. */
        if ((UBYTE)(stage + 1u) < td_audio_stage) td_audio_stage = stage;
        if (sfx_play_bank == SFX_STOP_BANK && (UWORD)(now - td_audio_last_ambient) >= 10u) {
            td_audio_last_ambient = now;
            td_audio_start(td_audio_brake, MUSIC_CH_4, TD_AUDIO_BRAKE_PRIORITY, TD_AUDIO_KIND_BRAKE);
        }
        return;
    }
    if (td_audio_mode == TD_AUDIO_EFFECTS) {
        /* No tune to cover: keep the steady drone on CH1. */
        if (sfx_play_bank != SFX_STOP_BANK || (UWORD)(now - td_audio_last_ambient) < 10u)
            return;
        td_audio_last_ambient = now;
        td_audio_start(td_audio_drones[vehicle][stage], MUSIC_CH_1,
                       MUSIC_SFX_PRIORITY_MINIMAL, TD_AUDIO_KIND_ENGINE);
        return;
    }
    /* Music mode: CH1 carries the counter-line, so the engine only revs
       briefly when pulling away or reaching a faster speed stage. */
    if (td_audio_stage == TD_AUDIO_NO_STAGE || stage > td_audio_stage) {
        if (sfx_play_bank == SFX_STOP_BANK && (UWORD)(now - td_audio_last_rev) >= TD_AUDIO_REV_GAP) {
            td_audio_last_rev = now;
            td_audio_start(td_audio_revs[vehicle][stage], MUSIC_CH_1,
                           MUSIC_SFX_PRIORITY_MINIMAL, TD_AUDIO_KIND_ENGINE);
        }
        td_audio_stage = stage;
    } else if ((UBYTE)(stage + 1u) < td_audio_stage) td_audio_stage = stage;
}
