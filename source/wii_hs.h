/*
 * Wii side of the Halo script VM. The evaluator is OpenCE hs_runtime.c.
 * This header is the only piece the rest of the DOL includes.
 */
#ifndef WII_HS_H
#define WII_HS_H

#include <stdint.h>

/* Retail maps store 19001 nodes. header 0x38 + nodes * 0x14. */
#define WII_HS_SYNTAX_MAX_NODES 19001
#define WII_HS_SYNTAX_BYTES (0x38u + (uint32_t)WII_HS_SYNTAX_MAX_NODES * 0x14u)

int wii_hs_mount(
	uint8_t const *syntax_le,
	uint32_t syntax_bytes,
	uint8_t const *scripts_le,
	int script_count,
	uint8_t const *globals_le,
	int global_count,
	uint8_t const *strings,
	uint32_t string_bytes);

void wii_hs_shutdown(void);
int wii_hs_ready(void);
void wii_hs_set_time(uint32_t ticks);
void wii_hs_update(void);
int wii_hs_unlinked(void);
int wii_hs_last_unlinked(void);
int wii_hs_global_real(int index, float *value);
char const *wii_hs_note(void);

#endif
