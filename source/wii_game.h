/*
 * OpenCE game.c, players.c, units, weapons, and ai.c under the Wii device stubs.
 * game_tick calls those functions. A player biped is created by player_spawn
 * at the scenario starting location when one has been recorded.
 */
#ifndef WII_GAME_H
#define WII_GAME_H

void wii_game_boot(void);
void wii_game_tick(void);
void wii_game_set_player_start(float x, float y, float z, float facing);
long wii_game_player_unit(void);
int wii_game_live_objects(void);
long wii_game_spawn_projectile(void);
void wii_control_set(float forward, float strafe, float yaw, float pitch, int fire);
float wii_control_yaw(void);
float wii_control_pitch(void);
float wii_control_forward(void);
float wii_control_strafe(void);
int wii_control_fire(void);

extern int wii_game_unit_ticks;
extern int wii_game_ai_ticks;
extern int wii_game_weapon_ticks;
extern int wii_game_player_ticks;
extern int wii_game_objects_placed;
extern int wii_game_ai_placed;

#endif
