#ifndef TD_NORTH_LEGACY_H
#define TD_NORTH_LEGACY_H
#include <gbdk/platform.h>
/* Exactly48 Core tiles were COLLISION_ALL before the v10 North road seams:
 * x38..45 and76..83, y0..2. Frozen pre-North scene.gbsres SHA256:
 * c59b93b6c66bf1b8f9b63289f20f2d248de84cbb09d29ab25f6b9ca51a40291c.
 * All other old Core collision bytes remain unchanged. This ROM-only query
 * restores that solid overlay while validating pre-v10 saves. Whole-pixel
 * point queries use half0; old driven/parked bodies use native half5. The
 * caller has already bounded coordinates. No extra saved or persistent RAM. */
static UBYTE td_north_legacy_clear(UWORD u,UWORD v,UBYTE half){
    UWORD left,right;
    if(u>=1024||v>=976||u<half||v<half)return FALSE;
    if(v-half>=24)return TRUE;
    left=u-half;right=u+half;
    return !((left<368&&right>=304)||(left<672&&right>=608));
}
#endif
