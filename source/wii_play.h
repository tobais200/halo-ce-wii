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

#endif
