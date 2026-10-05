/* Generated atlas private BANKED data API. Outputs are WRAM. */
#ifndef TD_ATLAS_DATA_H
#define TD_ATLAS_DATA_H
#include <gbdk/platform.h>
void td_atlas_pattern_unit_0(UWORD local_id,UBYTE *tile16) BANKED;
void td_atlas_pattern_unit_1(UWORD local_id,UBYTE *tile16) BANKED;
void td_atlas_row_unit_0(UWORD local_offset,UBYTE count,UWORD *out) BANKED;
#endif
