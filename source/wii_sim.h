/*
 * Scenario script table and the 30 Hz tick counter from cseries.h.
 * The HS bytecode interpreter in hs_runtime.c is not linked: it pulls in
 * the rest of the Xbox game. This reads the script records and counts ticks.
 */
#ifndef WII_SIM_H
#define WII_SIM_H

#include "bsp_preview.h"

#include <stdint.h>

/* scenario_definitions.h: structure_bsp_references is at 0x5A4.
   The fields between hs_syntax_data and that block sum to 0x130,
   and hs_scripts is 0x28 bytes into that span. */
#define WII_SCENARIO_HS_SCRIPTS 0x49Cu
#define WII_HS_SCRIPT_BYTES 0x5Cu
#define WII_TICKS_PER_SECOND 30

struct wii_sim {
	int loaded;
	uint32_t ticks;
	uint32_t frames;
	int script_count;
	int startup_count;
	uint32_t syntax_bytes;
	char startup_name[32];
	char message[96];
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
