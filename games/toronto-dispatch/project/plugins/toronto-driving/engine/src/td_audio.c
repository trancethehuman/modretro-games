/* Original Toronto Dispatch score and effects, MIT copyright 2026 Hai Nghiem.
   The existing GBVM timer/music manager owns IRQs, banks and APU writes. */
#pragma bank 255
#include <stddef.h>
#include "td_audio.h"
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
/* Original City Shift. Edit project/original-audio/city_shift.json. */
static const UBYTE td_audio_order_count = 8;
static const UBYTE td_audio_rest[] = {
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
};
static const UBYTE td_audio_pulse2_0[] = {
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(D_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(C_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(A_4, 1, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
};
static const UBYTE td_audio_pulse2_1[] = {
    DN(D_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(A_4, 1, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(D_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(Fs4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(D_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(Fs4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(D_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(Fs4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
};
static const UBYTE td_audio_pulse2_2[] = {
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(D_5, 1, 0x000),
    DN(E_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(D_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(C_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(E_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(D_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(C_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
};
static const UBYTE td_audio_pulse2_3[] = {
    DN(D_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(D_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(B_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(D_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(Fs4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(D_5, 1, 0x000),
    DN(___, 0, 0x000),
    DN(A_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(Fs4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(Fs4, 1, 0x000),
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(D_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(B_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
};
static const UBYTE td_audio_wave3_0[] = {
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(B_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(C_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(C_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
};
static const UBYTE td_audio_wave3_1[] = {
    DN(G_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(D_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(G_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(B_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(D_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(A_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(D_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(Fs4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
};
static const UBYTE td_audio_wave3_2[] = {
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(B_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(C_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(G_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(C_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(E_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
};
static const UBYTE td_audio_wave3_3[] = {
    DN(G_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(D_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(G_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(B_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(D_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(A_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(D_4, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
    DN(B_3, 1, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0x000),
    DN(___, 0, 0xC00),
    DN(___, 0, 0x000),
};
static const UBYTE * const td_audio_order1[] = {td_audio_rest, td_audio_rest, td_audio_rest, td_audio_rest};
static const UBYTE * const td_audio_order2[] = {td_audio_pulse2_0, td_audio_pulse2_1, td_audio_pulse2_2, td_audio_pulse2_3};
static const UBYTE * const td_audio_order3[] = {td_audio_wave3_0, td_audio_wave3_1, td_audio_wave3_2, td_audio_wave3_3};
static const UBYTE * const td_audio_order4[] = {td_audio_rest, td_audio_rest, td_audio_rest, td_audio_rest};
static const hUGEDutyInstr_t td_audio_duty[] = {{0, 64, 98, NULL, 0x80}};
static const hUGEWaveInstr_t td_audio_wave_instruments[] = {{0, 96, 0, NULL, 0x80}};
static const hUGENoiseInstr_t td_audio_noise_instruments[] = {{0, NULL, 0, 0, 0}};
static const UBYTE td_audio_wave[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98,
    0x76, 0x54, 0x32, 0x10,
};
static const hUGESong_t td_city_shift_song = {
    9, &td_audio_order_count,
    (const UBYTE **)td_audio_order1, (const UBYTE **)td_audio_order2,
    (const UBYTE **)td_audio_order3, (const UBYTE **)td_audio_order4,
    td_audio_duty, td_audio_wave_instruments, td_audio_noise_instruments,
    NULL, td_audio_wave
};
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
static const UBYTE td_audio_engine_car_0[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xE8, 0x81, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_car_1[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x14, 0x83, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_car_2[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x00, 0x84, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_car_3[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xB8, 0x84, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_truck_0[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x78, 0x80, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_truck_1[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x9A, 0x81, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_truck_2[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xAB, 0x82, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_truck_3[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x6E, 0x83, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_motorcycle_0[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x20, 0xBC, 0x83, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_motorcycle_1[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x20, 0xB8, 0x84, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_motorcycle_2[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x20, 0x8A, 0x85, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_motorcycle_3[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x20, 0x08, 0x86, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_scooter_0[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xC7, 0x82, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_scooter_1[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xF0, 0x83, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_scooter_2[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0xCD, 0x84, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE td_audio_engine_scooter_3[] = {
    0xF1, 0xF8, 0x00, 0x40, 0x30, 0x6A, 0x85, 0xF0, 0xF0, 0x01, 0x28, 0x00,
    0xC0, 0x01, 0x07,
};
static const UBYTE * const td_audio_engines[4][4] = {
    {td_audio_engine_car_0, td_audio_engine_car_1, td_audio_engine_car_2, td_audio_engine_car_3},
    {td_audio_engine_truck_0, td_audio_engine_truck_1, td_audio_engine_truck_2, td_audio_engine_truck_3},
    {td_audio_engine_motorcycle_0, td_audio_engine_motorcycle_1, td_audio_engine_motorcycle_2, td_audio_engine_motorcycle_3},
    {td_audio_engine_scooter_0, td_audio_engine_scooter_1, td_audio_engine_scooter_2, td_audio_engine_scooter_3},
};
/* END GENERATED AUDIO */

#define TD_AUDIO_NO_CUE 255
#define TD_AUDIO_TUNE_CHANNELS (MUSIC_CH_2 | MUSIC_CH_3)
#define TD_AUDIO_EFFECT_CHANNELS (MUSIC_CH_1 | MUSIC_CH_4)
#define TD_AUDIO_BRAKE_PRIORITY 1

static UBYTE td_audio_mode, td_audio_active, td_audio_paused;
static UBYTE td_audio_pending, td_audio_effect_kind;
static UWORD td_audio_last_ambient;

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
    if ((td_audio_effect_kind == 1 || td_audio_effect_kind == 2) &&
        sfx_play_bank == BANK(td_audio_assets)) td_audio_cut_effect();
}

static void td_audio_apply_music(void) {
    UBYTE pause = (td_audio_mode != TD_AUDIO_FULL || !td_audio_active);
    UBYTE mask = (td_audio_mode == TD_AUDIO_FULL) ?
        TD_AUDIO_EFFECT_CHANNELS :
        (TD_AUDIO_EFFECT_CHANNELS | TD_AUDIO_TUNE_CHANNELS);
    CRITICAL {
        music_global_mute_mask = mask;
        /* hUGE initialization clears its own mask; restore reserved channels
           after initialization as well as after a mode/SFX transition. */
        mask |= music_mute_mask;
        if (music_effective_mute != mask || hUGE_mute_mask != mask)
            music_effective_mute = driver_set_mute_mask(mask);
        if (pause != td_audio_paused) {
            td_audio_pause_music(pause);
            td_audio_paused = pause;
        }
    }
}

void td_audio_init(void) BANKED {
    td_audio_mode = TD_AUDIO_FULL;
    td_audio_active = FALSE;
    td_audio_paused = FALSE;
    td_audio_pending = TD_AUDIO_NO_CUE;
    td_audio_effect_kind = 0;
    td_audio_last_ambient = sys_time;
    td_audio_cut_effect();
    CRITICAL { music_load(BANK(td_audio_assets), &td_city_shift_song); }
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
    CRITICAL { music_sound_cut_mask(TD_AUDIO_TUNE_CHANNELS); }
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
    /* Flush after pause transitions, so entering a result screen does not
       immediately cut the newly queued delivery/timeout jingle. */
    cue = td_audio_pending;
    td_audio_pending = TD_AUDIO_NO_CUE;
    if (cue < TD_AUDIO_CUES && td_audio_mode != TD_AUDIO_SILENT)
        td_audio_start(td_audio_cues[cue], td_audio_cue_masks[cue],
                       td_audio_cue_priorities[cue], 3);
    if (!world_active || td_audio_mode == TD_AUDIO_SILENT || onfoot) {
        td_audio_cut_ambient();
        td_audio_last_ambient = now;
        return;
    }
    magnitude = (speed < 0) ? (UWORD)(-(speed + 1)) + 1u : (UWORD)speed;
    if (magnitude < 2) {
        td_audio_cut_ambient();
        td_audio_last_ambient = now;
        return;
    }
    if (sfx_play_bank != SFX_STOP_BANK || (UWORD)(now - td_audio_last_ambient) < 10u)
        return;
    td_audio_last_ambient = now;
    if (braking && speed > 6) {
        td_audio_start(td_audio_brake, MUSIC_CH_4, TD_AUDIO_BRAKE_PRIORITY, 2);
        return;
    }
    stage = (magnitude < 7u) ? 0 : ((magnitude < 13u) ? 1 : ((magnitude < 19u) ? 2 : 3));
    if (vehicle > 3) vehicle = 0;
    td_audio_start(td_audio_engines[vehicle][stage], MUSIC_CH_1,
                   MUSIC_SFX_PRIORITY_MINIMAL, 1);
}
