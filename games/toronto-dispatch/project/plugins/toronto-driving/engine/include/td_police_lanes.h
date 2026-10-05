#ifndef TD_POLICE_LANES_H
#define TD_POLICE_LANES_H
#include "td_police.h"

/* ROM-only directed right-hand lanes and registered junction/endcap joins.
 * Coordinates/output are caller WRAM; all scratch is automatic, never saved.
 * Called only after td_police_plan validates the loaded district/body/input. */
UBYTE td_police_lanes_next(UBYTE district,UBYTE wanted,UWORD u,UWORD v,
    UWORD target_u,UWORD target_v,UBYTE heading,td_police_plan_t *out) BANKED;
#endif
