#ifndef TD_HOST_GBVM_STUBS_H
#define TD_HOST_GBVM_STUBS_H

/* Host hardware adapters only. Gameplay comes from the actual TORONTO.c. */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef uint8_t UBYTE;
typedef int8_t BYTE;
typedef uint16_t UWORD;
typedef int16_t WORD;
typedef struct { UBYTE bank; const void *ptr; } far_ptr_t;
#define BANKED
#define NONBANKED
#define TRUE 1
#define FALSE 0

typedef struct actor actor_t;
struct actor {
    struct { UWORD x,y; } pos;
    actor_t *prev,*next;
    UBYTE flags,collision_group,anim_tick,frame,frame_start,frame_end;
    struct { UBYTE bank; const void *ptr; } script,script_update;
    UWORD hscript_update,hscript_hit;
    UBYTE base_tile;
    far_ptr_t sprite;
    struct { WORD left,right,top,bottom; } bounds;
};
#define MAX_ACTORS 21
typedef struct { UBYTE width,height; far_ptr_t collisions; } scene_t;
extern actor_t actors[21];
extern actor_t *actors_inactive_head;
extern UBYTE actors_len;
#define PLAYER actors[0]
#define ACTOR_FLAG_PERSISTENT 16
#define ACTOR_FLAG_HIDDEN 2
#define ACTOR_FLAG_ACTIVE 32
#define ACTOR_FLAG_DISABLED 64
#define CAMERA_LOCK_FLAG 1
extern UWORD camera_x,camera_y,image_width,image_height,sys_time;
extern UBYTE camera_settings;
extern BYTE camera_offset_x,camera_offset_y,camera_deadzone_x,camera_deadzone_y;
extern UBYTE joy,joy_pressed;

enum { J_RIGHT=1,J_LEFT=2,J_UP=4,J_DOWN=8,J_A=16,J_B=32,J_SELECT=64,J_START=128 };
#define INPUT_RIGHT (joy & J_RIGHT)
#define INPUT_LEFT (joy & J_LEFT)
#define INPUT_UP (joy & J_UP)
#define INPUT_DOWN (joy & J_DOWN)
#define INPUT_A (joy & J_A)
#define INPUT_B (joy & J_B)
#define INPUT_RIGHT_PRESSED (joy_pressed & J_RIGHT)
#define INPUT_LEFT_PRESSED (joy_pressed & J_LEFT)
#define INPUT_UP_PRESSED (joy_pressed & J_UP)
#define INPUT_DOWN_PRESSED (joy_pressed & J_DOWN)
#define INPUT_A_PRESSED (joy_pressed & J_A)
#define INPUT_B_PRESSED (joy_pressed & J_B)
#define INPUT_SELECT_PRESSED (joy_pressed & J_SELECT)
#define INPUT_START_PRESSED (joy_pressed & J_START)

extern UBYTE td_test_sram[8192];
void td_host_sram_store(volatile UBYTE *address,UBYTE value);
#define ENABLE_RAM_MBC5 ((void)0)
#define DISABLE_RAM_MBC5 ((void)0)
#define RAM_BANKS_ONLY 0
#define SWITCH_RAM_BANK(bank,mode) ((void)(bank),(void)(mode))
UBYTE tile_at(UBYTE x,UBYTE y);
void actor_set_frames(actor_t *actor,UBYTE first,UBYTE end);
void activate_actor(actor_t *actor);
void deactivate_actor(actor_t *actor);
void MemcpyBanked(void *dest,const void *src,size_t length,UBYTE bank);
UBYTE ReadBankedUBYTE(const UBYTE *src,UBYTE bank);

extern UBYTE tile_hit_x,tile_hit_y;
UBYTE tile_col_test_range_x(UBYTE mask,UBYTE row,UBYTE first,UBYTE last);
UBYTE tile_col_test_range_y(UBYTE mask,UBYTE column,UBYTE first,UBYTE last);
#endif
