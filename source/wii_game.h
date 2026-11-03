/*
 * OpenCE game.c brought up under the Wii device stubs.
 * game_tick is the real function. Unit, weapon, and AI bodies are counted,
 * not simulated.
 */
#ifndef WII_GAME_H
#define WII_GAME_H

void wii_game_boot(void);
void wii_game_tick(void);

extern int wii_game_unit_ticks;
extern int wii_game_ai_ticks;
extern int wii_game_weapon_ticks;
extern int wii_game_player_ticks;
extern int wii_game_objects_placed;
extern int wii_game_ai_placed;

#endif
