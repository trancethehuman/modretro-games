#ifndef TD_HUD_H
#define TD_HUD_H
/* Pop-up HUD timers (calls of td_ui_hud_tick, about seven a second). */
#define TD_POP_LONG 28
#define TD_POP_SHORT 20
#define TD_POP_JUNCTION 16
extern UBYTE td_pop_street,td_pop_target,td_pop_cash,td_pop_vit,td_pop_ammo,td_pop_car,td_pop_job,td_still;
/* Standing still shows the status line: on foot after about a second; in a
 * vehicle after about three, so it stays away while waiting at a light.
 * td_still counts HUD ticks (every eighth update). */
#define TD_IDLE_FOOT 10
#define TD_IDLE_DRIVE 24
#define TD_HUD_IDLE() (td_still>=(td.onfoot?TD_IDLE_FOOT:TD_IDLE_DRIVE))
extern char td_street_name[21];
/* Neighbourhood, landmark and junction name ids (td_places.h) and how long
 * the neighbourhood's name stays up. */
extern UBYTE td_place_ids[3],td_pop_area;
void td_hud_reset(void) BANKED;
void td_ui_hud_paint(void) BANKED;
#endif
