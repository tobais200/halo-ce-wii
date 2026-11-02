/*
 * Decode Halo bitmap texels the way OpenCE's xbox_textures.c does:
 * the same swizzle bit masks, the same 565/DXT expansion, bitmap_data
 * formats from bitmap_group.h (DXT1 is 14). Output is straight RGBA8.
 */
#ifndef WII_TEX_H
#define WII_TEX_H

#include "bsp_preview.h"

#include <stdint.h>

struct wii_rgba {
	uint8_t *pixels;
	uint16_t cap;
	uint16_t width;
	uint16_t height;
	int format;
	int ok;
	char message[80];
};

/* Swizzle index for one texel. depth is 1 for 2D. */
uint32_t wii_tex_swizzle_index(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

/* 0 when a mip that fits `cap` was decoded into pixels. */
int wii_tex_decode(
	uint8_t const *source,
	uint32_t source_size,
	int format,
	int linear,
	uint16_t width,
	uint16_t height,
	struct wii_rgba *image);

/* bitmap_group at group_file, one bitmap_data index. */
int wii_tex_load_group(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_offset,
	uint32_t tag_size,
	uint32_t group_file,
	uint16_t bitmap_index,
	struct wii_rgba *image);

/* First 2D bitm tag in the tag index, at most 1024 instances scanned. */
int wii_tex_load_first_bitm(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_offset,
	uint32_t tag_size,
	struct wii_rgba *image);

#endif
