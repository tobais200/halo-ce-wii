/*
 * One step of play after game_tick: walk the controlled biped against the
 * structure BSP, fire the resident projectile, and let nearby actors shoot.
 */
#ifndef WII_PLAY_H
#define WII_PLAY_H

void wii_play_after_tick(void);
int wii_play_moved(void);
int wii_play_shots(void);
int wii_play_ai_shots(void);
int wii_play_grounded(void);
void wii_sound_impulse(long tag_index);
long wii_sound_last(void);
int wii_save_checkpoint(void);
int wii_save_restore(void);
int wii_save_valid(void);
int wii_play_grenades(void);
long wii_play_grenade_definition(void);
int wii_play_flank(void);
int wii_play_firing_tested(void);
int wii_play_firing_index(void);
int wii_play_firing_real(void);
int wii_play_vehicle_ready(void);
int wii_play_vehicle_seated(void);
int wii_play_path_found(void);
int wii_play_path_nodes(void);
int wii_play_firing_on_path(void);
int wii_play_enter_anim(void);
int wii_play_mass_ran(void);
int wii_play_wheel_ran(void);
int wii_play_suspension_ran(void);
int wii_play_prop_surface(void);
int wii_play_portal_crossed(void);
int wii_play_portal_inside(void);
int wii_play_vehicle_moved(void);
int wii_play_vehicle_settled(void);
int wii_play_tire_ran(void);
int wii_play_tire_slip(void);
int wii_play_portal_hops(void);
int wii_play_pursuit_ran(void);
float wii_play_firing_score(void);
void wii_play_set_seat(long unit_index);
int wii_play_magic_seat(void);
int wii_seat_anim_frame(int seated, int tick);
int wii_vehicle_seat_for(float distance);
int wii_sound_decode(uint8_t const *data, int size, int16_t *out, int cap);
void wii_sound_load(uint32_t sound_va);
int wii_sound_tags(void);

#endif
