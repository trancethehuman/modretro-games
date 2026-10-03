#ifndef TD_CITY_SPRITES_H
#define TD_CITY_SPRITES_H
#include <gbdk/platform.h>
#include "actor.h"

/* Capture/unlink empty authored loaders after aircraft/Queen binding and
 * before the caller clones runtime actors. No sprite patterns are copied. */
void td_city_sprites_bind(void) BANKED;
/* Kind0 car and1 truck use original player art;2 police,3 fire,4 ambulance,
 *5 bus use the fleet resource. Orientation is east0,south1,west2,north3. */
void td_fleet_present(actor_t *actor,UBYTE kind,UBYTE orientation) BANKED;
/* Variant0 teal/1 ochre. Poses: east steps0/1, west steps2/3, stumble4.
 * These functions only set sprite/base/frame; callers own motion and flags. */
void td_civilian_present(actor_t *actor,UBYTE variant,UBYTE pose) BANKED;
#endif
