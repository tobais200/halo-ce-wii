#include "wii_tex.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG_CACHE_BASE 0x803A6000u
#define BITM_GROUP 0x6d746962u
#define FORMAT_DXT1 14
#define FORMAT_DXT3 15
#define FORMAT_DXT5 16

static uint32_t read_u32(uint8_t const *bytes)
{
	return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8)
		| ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint16_t read_u16(uint8_t const *bytes)
{
	return (uint16_t)(bytes[0] | (bytes[1] << 8));
}

static uint32_t spread(uint32_t mask, uint32_t value)
{
	uint32_t result = 0;
	uint32_t bit = 1;

	while (value && bit) {
		if (mask & bit) {
			if (value & 1u)
				result |= bit;
			value >>= 1;
		}
		bit <<= 1;
	}
	return result;
}

uint32_t wii_tex_swizzle_index(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	uint32_t masks_x = 0;
	uint32_t masks_y = 0;
	uint32_t bit = 1;
	uint32_t mask_bit = 1;
	int done;

	do {
		done = 1;
		if (bit < width) {
			masks_x |= mask_bit;
			mask_bit <<= 1;
			done = 0;
		}
		if (bit < height) {
			masks_y |= mask_bit;
			mask_bit <<= 1;
			done = 0;
		}
		bit <<= 1;
	} while (!done && bit);
	return spread(masks_x, x) | spread(masks_y, y);
}

static uint32_t expand5(uint32_t value)
{
	return (value << 3) | (value >> 2);
}

static uint32_t expand6(uint32_t value)
{
	return (value << 2) | (value >> 4);
}

static uint32_t expand4(uint32_t value)
{
	return value * 0x11u;
}

static void store_rgba(uint8_t *pixel, uint32_t argb)
{
	pixel[0] = (uint8_t)((argb >> 16) & 0xff);
	pixel[1] = (uint8_t)((argb >> 8) & 0xff);
	pixel[2] = (uint8_t)(argb & 0xff);
	pixel[3] = (uint8_t)((argb >> 24) & 0xff);
}

static uint32_t from_16(uint8_t const *source)
{
	return (uint32_t)source[0] | ((uint32_t)source[1] << 8);
}

static uint32_t from_32(uint8_t const *source)
{
	return from_16(source) | ((uint32_t)source[2] << 16) | ((uint32_t)source[3] << 24);
}

static int texel_bytes(int format)
{
	switch (format) {
	case 0: case 1: case 2: return 1;
	case 3: case 6: case 8: case 9: return 2;
	case 10: case 11: return 4;
	case FORMAT_DXT1: return 8;
	case FORMAT_DXT3: case FORMAT_DXT5: return 16;
	default: return 0;
	}
}

static int is_dxt(int format)
{
	return format == FORMAT_DXT1 || format == FORMAT_DXT3 || format == FORMAT_DXT5;
}

static uint32_t level_bytes(int format, uint16_t width, uint16_t height)
{
	if (is_dxt(format)) {
		uint32_t blocks = ((uint32_t)width + 3u) / 4u * (((uint32_t)height + 3u) / 4u);
		return blocks * (format == FORMAT_DXT1 ? 8u : 16u);
	}
	return (uint32_t)width * (uint32_t)height * (uint32_t)texel_bytes(format);
}

static uint32_t convert_texel(int format, uint8_t const *source)
{
	uint32_t bits;

	switch (format) {
	case 0: return 0xffffff00u | source[0];
	case 1: case 2: return 0xff000000u | (source[0] << 16) | (source[0] << 8) | source[0];
	case 3:
		return ((uint32_t)source[1] << 24) | (source[0] << 16) | (source[0] << 8) | source[0];
	case 6:
		bits = from_16(source);
		return 0xff000000u | (expand5(bits >> 11) << 16) | (expand6((bits >> 5) & 0x3f) << 8) | expand5(bits & 0x1f);
	case 8:
		bits = from_16(source);
		return ((bits & 0x8000u) ? 0xff000000u : 0) | (expand5((bits >> 10) & 0x1f) << 16)
			| (expand5((bits >> 5) & 0x1f) << 8) | expand5(bits & 0x1f);
	case 9:
		bits = from_16(source);
		return (expand4(bits >> 12) << 24) | (expand4((bits >> 8) & 0xf) << 16)
			| (expand4((bits >> 4) & 0xf) << 8) | expand4(bits & 0xf);
	case 10: return from_32(source) | 0xff000000u;
	case 11: return from_32(source);
	default: return 0;
	}
}

static uint32_t mix_channel(uint32_t a, uint32_t b, uint32_t wa, uint32_t wb)
{
	uint32_t result = 0xff000000u;
	int shift;

	for (shift = 0; shift < 24; shift += 8) {
		uint32_t channel = ((((a >> shift) & 0xff) * wa) + (((b >> shift) & 0xff) * wb)) / 3u;
		result |= channel << shift;
	}
	return result;
}

static void dxt_color(uint8_t const *block, int punch, uint32_t colors[16])
{
	uint32_t c0 = from_16(block);
	uint32_t c1 = from_16(block + 2);
	uint32_t palette[4];
	uint32_t bits = read_u32(block + 4);
	int index;

	palette[0] = 0xff000000u | (expand5(c0 >> 11) << 16) | (expand6((c0 >> 5) & 0x3f) << 8) | expand5(c0 & 0x1f);
	palette[1] = 0xff000000u | (expand5(c1 >> 11) << 16) | (expand6((c1 >> 5) & 0x3f) << 8) | expand5(c1 & 0x1f);
	if (c0 > c1 || !punch) {
		palette[2] = mix_channel(palette[0], palette[1], 2, 1);
		palette[3] = mix_channel(palette[0], palette[1], 1, 2);
	} else {
		palette[2] = 0xff000000u | ((((palette[0] & 0xff00ffu) + (palette[1] & 0xff00ffu)) >> 1) & 0xff00ffu)
			| ((((palette[0] & 0xff00u) + (palette[1] & 0xff00u)) >> 1) & 0xff00u);
		palette[3] = 0;
	}
	for (index = 0; index < 16; index++)
		colors[index] = palette[(bits >> (index * 2)) & 3];
}

static void write_pixel(struct wii_rgba *image, uint16_t x, uint16_t y, uint32_t argb)
{
	store_rgba(image->pixels + ((uint32_t)y * image->width + x) * 4u, argb);
}

int wii_tex_decode(
	uint8_t const *source,
	uint32_t source_size,
	int format,
	int linear,
	uint16_t width,
	uint16_t height,
	struct wii_rgba *image)
{
	uint16_t mip_w;
	uint16_t mip_h;
	uint32_t skip = 0;
	uint32_t bytes;
	int bpp;

	if (!source || !image || !image->pixels || !image->cap || !width || !height) {
		if (image)
			snprintf(image->message, sizeof(image->message), "bitmap arguments missing");
		return -1;
	}
	mip_w = width;
	mip_h = height;
	while ((mip_w > image->cap || mip_h > image->cap) && mip_w > 1 && mip_h > 1) {
		skip += level_bytes(format, mip_w, mip_h);
		mip_w = (uint16_t)(mip_w > 1 ? mip_w / 2 : 1);
		mip_h = (uint16_t)(mip_h > 1 ? mip_h / 2 : 1);
	}
	if (mip_w > image->cap || mip_h > image->cap || texel_bytes(format) == 0) {
		snprintf(image->message, sizeof(image->message), "bitmap format %d is not decoded", format);
		return -1;
	}
	bytes = level_bytes(format, mip_w, mip_h);
	if ((uint64_t)skip + bytes > source_size) {
		snprintf(image->message, sizeof(image->message), "bitmap pixels are short");
		return -1;
	}
	source += skip;
	image->width = mip_w;
	image->height = mip_h;
	image->format = format;
	bpp = texel_bytes(format);
	if (is_dxt(format)) {
		uint32_t blocks_x = ((uint32_t)mip_w + 3u) / 4u;
		uint32_t blocks_y = ((uint32_t)mip_h + 3u) / 4u;
		uint32_t bx, by, i;

		for (by = 0; by < blocks_y; by++) {
			for (bx = 0; bx < blocks_x; bx++) {
				uint8_t const *block = source + (by * blocks_x + bx) * (uint32_t)bpp;
				uint32_t colors[16];

				if (format == FORMAT_DXT1)
					dxt_color(block, 1, colors);
				else
					dxt_color(block + 8, 0, colors);
				for (i = 0; i < 16; i++) {
					uint16_t x = (uint16_t)(bx * 4u + (i & 3u));
					uint16_t y = (uint16_t)(by * 4u + (i / 4u));
					if (x < mip_w && y < mip_h)
						write_pixel(image, x, y, colors[i]);
				}
			}
		}
	} else if (linear) {
		uint16_t y, x;
		for (y = 0; y < mip_h; y++) {
			for (x = 0; x < mip_w; x++)
				write_pixel(image, x, y, convert_texel(format, source + ((uint32_t)y * mip_w + x) * (uint32_t)bpp));
		}
	} else {
		uint16_t y, x;
		for (y = 0; y < mip_h; y++) {
			for (x = 0; x < mip_w; x++) {
				uint32_t index = wii_tex_swizzle_index(x, y, mip_w, mip_h);
				write_pixel(image, x, y, convert_texel(format, source + index * (uint32_t)bpp));
			}
		}
	}
	snprintf(image->message, sizeof(image->message), "bitmap %ux%u format %d", (unsigned)mip_w, (unsigned)mip_h, format);
	image->ok = 1;
	return 0;
}

static int pull(bsp_read_fn read, void *context, uint32_t file_size, uint32_t offset, void *destination, uint32_t size)
{
	if ((uint64_t)offset + size > file_size)
		return -1;
	return read(context, offset, destination, size);
}

static int map_tag_va(uint32_t va, uint32_t tag_offset, uint32_t tag_size, uint32_t *file_offset)
{
	if (!tag_size || va < TAG_CACHE_BASE || (va - TAG_CACHE_BASE) >= tag_size)
		return -1;
	*file_offset = tag_offset + (va - TAG_CACHE_BASE);
	return 0;
}

int wii_tex_load_group(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_offset,
	uint32_t tag_size,
	uint32_t group_file,
	uint16_t bitmap_index,
	struct wii_rgba *image)
{
	uint8_t group[0x6c];
	uint8_t data[0x30];
	uint8_t pixels_header[20];
	uint32_t count;
	uint32_t address;
	uint32_t data_file;
	uint32_t blob_file = 0;
	uint32_t pixels_offset;
	uint32_t pixels_size;
	uint16_t width;
	uint16_t height;
	uint16_t flags;
	uint16_t format;
	uint8_t *encoded;
	int linear;
	int result;

	if (pull(read, context, file_size, group_file, group, sizeof(group)) != 0) {
		snprintf(image->message, sizeof(image->message), "bitmap group unreadable");
		return -1;
	}
	count = read_u32(group + 0x60);
	address = read_u32(group + 0x64);
	if (bitmap_index >= count || map_tag_va(address, tag_offset, tag_size, &data_file) != 0) {
		snprintf(image->message, sizeof(image->message), "bitmap index is outside the group");
		return -1;
	}
	if (pull(read, context, file_size, data_file + (uint32_t)bitmap_index * 0x30u, data, sizeof(data)) != 0)
		return -1;
	width = read_u16(data + 0x04);
	height = read_u16(data + 0x06);
	format = read_u16(data + 0x0c);
	flags = read_u16(data + 0x0e);
	pixels_offset = read_u32(data + 0x18);
	pixels_size = read_u32(data + 0x1c);
	if (pull(read, context, file_size, group_file + 0x30u, pixels_header, sizeof(pixels_header)) == 0) {
		uint32_t blob_va = read_u32(pixels_header + 12);
		uint32_t blob_raw = read_u32(pixels_header + 8);
		if (map_tag_va(blob_va, tag_offset, tag_size, &blob_file) != 0 && blob_raw < file_size)
			blob_file = blob_raw;
	}
	if ((uint64_t)blob_file + pixels_offset + pixels_size > file_size || !pixels_size) {
		snprintf(image->message, sizeof(image->message), "bitmap pixels are outside the file");
		return -1;
	}
	if (pixels_size > 1024u * 1024u) {
		snprintf(image->message, sizeof(image->message), "bitmap is larger than the 1 MiB read cap");
		return -1;
	}
	encoded = (uint8_t *)malloc(pixels_size);
	if (!encoded) {
		snprintf(image->message, sizeof(image->message), "bitmap decode buffer failed");
		return -1;
	}
	if (pull(read, context, file_size, blob_file + pixels_offset, encoded, pixels_size) != 0) {
		free(encoded);
		return -1;
	}
	linear = (flags & (1u << 4)) != 0 || format >= FORMAT_DXT1;
	result = wii_tex_decode(encoded, pixels_size, (int)format, linear, width, height, image);
	free(encoded);
	return result;
}

int wii_tex_load_first_bitm(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_offset,
	uint32_t tag_size,
	struct wii_rgba *image)
{
	uint8_t header[0x28];
	uint32_t instances_va;
	uint32_t instances_file;
	uint32_t tag_count;
	uint32_t index;
	uint32_t limit;

	if (pull(read, context, file_size, tag_offset, header, sizeof(header)) != 0)
		return -1;
	instances_va = read_u32(header);
	tag_count = read_u32(header + 0x0c);
	if (map_tag_va(instances_va, tag_offset, tag_size, &instances_file) != 0)
		return -1;
	limit = tag_count > 1024u ? 1024u : tag_count;
	for (index = 0; index < limit; index++) {
		uint8_t instance[0x20];
		uint32_t group;
		uint32_t base;

		if (pull(read, context, file_size, instances_file + index * 0x20u, instance, sizeof(instance)) != 0)
			return -1;
		group = read_u32(instance);
		if (group != BITM_GROUP)
			continue;
		base = read_u32(instance + 0x14);
		if (map_tag_va(base, tag_offset, tag_size, &base) != 0)
			continue;
		if (wii_tex_load_group(read, context, file_size, tag_offset, tag_size, base, 0, image) == 0)
			return 0;
	}
	snprintf(image->message, sizeof(image->message), "no decodable bitm in the first %u tags", (unsigned)limit);
	return -1;
}
