#ifndef TD_HOSPITAL_H
#define TD_HOSPITAL_H
#include <gbdk/platform.h>

/* Fictional game forecourt, not a surveyed Toronto General entrance. The
 * location preserves the researched east-University/south-College relation.
 * Recovery still validates the entire foot body and live traffic clearance. */
#define TD_HOSPITAL_DISTRICT 0
#define TD_HOSPITAL_U 504
#define TD_HOSPITAL_V 344
#define TD_HOSPITAL_MARKER_U 512
#define TD_HOSPITAL_MARKER_V 344
#define TD_HOSPITAL_TILE 254

/* One original mint medical badge in bank1 BKG254. Upload during UI init;
 * paint the visible facade in live Core views before other overlays. No new
 * mutable state, OAM object, collision change or saved byte is required. */
void td_hospital_init(void) BANKED;
void td_hospital_render(void) BANKED;
#endif
