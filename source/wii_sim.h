/*
 * Scenario script table and the 30 Hz clock. When the map's hs_syntax_data
 * is a script-node data array, OpenCE hs_runtime.c runs it.
 */
#ifndef WII_SIM_H
#define WII_SIM_H

#include "bsp_preview.h"

#include <stdint.h>

/* scenario_definitions.h, confirmed by offsetof against structure BSPs at 0x5A4. */
#define WII_SCENARIO_BIPEDS 0x228u
#define WII_SCENARIO_BIPED_PALETTE 0x234u
#define WII_SCENARIO_WEAPONS 0x270u
/* biped_definition.biped.standing_camera_height, from the real struct. */
#define WII_BIPED_STANDING_CAMERA_HEIGHT 0x400u
#define WII_PALETTE_ENTRY_BYTES 0x30u
#define WII_BIPED_GROUP 0x64706962u
#define WII_SCENARIO_PLAYERS 0x354u
#define WII_SCENARIO_AI_ENCOUNTERS 0x42Cu
#define WII_SCENARIO_HS_SYNTAX 0x474u
#define WII_SCENARIO_HS_STRINGS 0x488u
#define WII_SCENARIO_HS_SCRIPTS 0x49Cu
#define WII_SCENARIO_HS_GLOBALS 0x4A8u
#define WII_HS_SCRIPT_BYTES 0x5Cu
#define WII_HS_GLOBAL_BYTES 0x5Cu
#define WII_PLAYER_LOCATION_BYTES 0x34u
#define WII_TICKS_PER_SECOND 30

struct wii_sim {
	int loaded;
	int vm;
	uint32_t ticks;
	uint32_t frames;
	int script_count;
	int startup_count;
	int biped_count;
	int weapon_count;
	int encounter_count;
	int have_player;
	int have_camera_height;
	float player_x;
	float player_y;
	float player_z;
	float player_facing;
	float camera_height;
	uint32_t syntax_bytes;
	char startup_name[32];
	char message[160];
};

int wii_sim_script_record(uint8_t const *record, char *name, int name_size, int *script_type);

int wii_sim_load(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_offset,
	uint32_t tag_size,
	uint32_t scenario_file,
	struct wii_sim *sim);

void wii_sim_frame(struct wii_sim *sim);

#endif
