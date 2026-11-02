/*
 * First structure BSP of a Halo cache, using the layouts in OpenCE
 * (cache_files.c, structure_bsp_definitions.h, cache_file_formats.c).
 * Reads slices through the callback. Keeps at most the caller's vertex
 * and triangle caps. Does not run the game loop or sample shaders.
 */
#ifndef BSP_PREVIEW_H
#define BSP_PREVIEW_H

#include <stdint.h>

typedef int (*bsp_read_fn)(void *context, uint64_t offset, void *destination, uint32_t size);

struct bsp_preview_store {
	float *positions;
	uint8_t *colors;
	uint16_t *indices;
	uint16_t vertex_cap;
	uint16_t triangle_cap;
};

struct bsp_preview_info {
	int ok;
	int truncated;
	uint16_t vertices;
	uint16_t triangles;
	uint32_t vertices_seen;
	uint32_t triangles_seen;
	float center[3];
	float radius;
	char message[96];
};

/* 0 when at least one triangle was stored. */
int bsp_preview_load(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_data_offset,
	uint32_t tag_data_size,
	struct bsp_preview_store const *store,
	struct bsp_preview_info *info);

#endif
