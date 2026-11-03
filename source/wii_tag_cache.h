/*
 * Streamed tag-cache fixup. The Xbox cache is 23 MiB at 0x803A6000 and is
 * not mapped. The blocks the sim reads (scenario starts, weapons, encounters,
 * the player biped, its model and animation seats) are copied off the map
 * and stored native-endian. tag_get returns those copies.
 */
#ifndef WII_TAG_CACHE_H
#define WII_TAG_CACHE_H

#include "bsp_preview.h"

#include <stdint.h>

struct scenario;
struct game_globals;

int wii_tag_cache_mount(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_offset,
	uint32_t tag_size);
void wii_tag_cache_unmount(void);
int wii_tag_cache_ready(void);
uint32_t wii_tag_cache_resident(void);
float wii_tag_cache_camera_height(void);
int wii_tag_cache_weapon_count(void);
int wii_tag_cache_encounter_count(void);
void wii_tag_cache_player(float *x, float *y, float *z);

struct scenario *wii_tag_cache_scenario(void);
struct game_globals *wii_tag_cache_globals(void);
void *wii_tag_cache_get(long group_tag, long tag_index);
long wii_tag_cache_group(long tag_index);

/* Synthetic cache with the retail layout. Leaves the cache mounted. */
int wii_tag_cache_self_test(void);

#endif
