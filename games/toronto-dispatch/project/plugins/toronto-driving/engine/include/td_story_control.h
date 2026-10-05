#ifndef TD_STORY_CONTROL_H
#define TD_STORY_CONTROL_H
#include <gbdk/platform.h>
/* Actual welcome/result triggers; false leaves the complete state untouched. */
UBYTE td_story_maybe_begin(void) BANKED;
void td_story_control_update(UBYTE pressed) BANKED;
#endif
