#ifndef TD_HUD_H
#define TD_HUD_H
/* Pop-up HUD timers (calls of td_ui_hud_tick, about seven a second). */
#define TD_POP_LONG 28
#define TD_POP_SHORT 20
extern UBYTE td_pop_street,td_pop_target,td_pop_cash,td_pop_vit,td_pop_ammo,td_pop_car,td_pop_job,td_still;
extern char td_street_name[20];
void td_hud_reset(void) BANKED;
void td_ui_hud_paint(void) BANKED;
#endif
