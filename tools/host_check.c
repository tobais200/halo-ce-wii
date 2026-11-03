/*
 * Host checks for the pieces that do not need a Wii: little-endian fourcc
 * handling inside OpenCE's cache_file_identify, an XDVDFS walk, the LZ4
 * round trip, and the RAM ceiling constants. The image is synthetic.
 */
#include "bsp_preview.h"
#include "cache_file_formats.h"
#include "lz4.h"
#include "wii_mem.h"
#include "wii_tex.h"
unsigned char game_map_loaded(void);

#include "wii_game.h"
#include "wii_tag_cache.h"
#include "wii_hs.h"
#include "wii_sim.h"
#include "wii_widget.h"
#include "xdvdfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct image {
	uint8_t *bytes;
	uint32_t size;
};

static int image_read(void *context, uint64_t offset, void *destination, uint32_t size)
{
	struct image *image = context;
	if ((uint64_t)offset + size > image->size)
		return -1;
	memcpy(destination, image->bytes + offset, size);
	return 0;
}

static int opence_read(void *context, uint32_t offset, uint32_t size, void *buffer)
{
	struct image *image = context;
	if ((uint64_t)offset + size > image->size)
		return 0;
	memcpy(buffer, image->bytes + offset, size);
	return 1;
}

static void write_u16(uint8_t *bytes, uint16_t value)
{
	bytes[0] = (uint8_t)value;
	bytes[1] = (uint8_t)(value >> 8);
}

static void write_u32(uint8_t *bytes, uint32_t value)
{
	bytes[0] = (uint8_t)value;
	bytes[1] = (uint8_t)(value >> 8);
	bytes[2] = (uint8_t)(value >> 16);
	bytes[3] = (uint8_t)(value >> 24);
}

static void put_entry(uint8_t *directory, uint32_t offset, uint16_t left, uint16_t right,
	uint32_t sector, uint32_t size, int is_directory, char const *name)
{
	uint8_t length = (uint8_t)strlen(name);
	write_u16(directory + offset, left);
	write_u16(directory + offset + 2, right);
	write_u32(directory + offset + 4, sector);
	write_u32(directory + offset + 8, size);
	directory[offset + 12] = is_directory ? 0x10 : 0;
	directory[offset + 13] = length;
	memcpy(directory + offset + 14, name, length);
}

struct walk_expect {
	int maps_dir;
	int map_files;
};

static int on_visit(void *context, char const *name, int is_directory, uint32_t sector, uint32_t size)
{
	struct walk_expect *expect = context;
	(void)sector;
	(void)size;
	if (is_directory && strcmp(name, "maps") == 0)
		expect->maps_dir++;
	if (!is_directory && strcmp(name, "a.map") == 0)
		expect->map_files++;
	if (!is_directory && strcmp(name, "m.map") == 0)
		expect->map_files++;
	if (!is_directory && strcmp(name, "z.map") == 0)
		expect->map_files++;
	return 0;
}

static int fail(char const *message)
{
	fprintf(stderr, "host_check: %s\n", message);
	return 1;
}

int main(void)
{
	uint8_t image_bytes[40 * XDVDFS_SECTOR_BYTES];
	uint8_t header[CACHE_FILE_HEADER_BYTES];
	struct image image;
	struct xdvdfs_volume volume;
	struct walk_expect expect;
	struct cache_file_source source;
	struct cache_file_identity identity;
	enum cache_file_status status;
	enum cache_file_format format;
	char compressed[128];
	char restored[64];
	int stored;
	int decoded;
	uint8_t directory[96];

	if (WII_RAM_HARDWARE_BYTES != 88u * 1024u * 1024u)
		return fail("hardware ceiling is not 88 MiB");
	if (WII_TAG_RESIDENT_CAP_BYTES + WII_TEXTURE_CAP_BYTES + WII_SOUND_CAP_BYTES
		+ WII_STREAM_WINDOW_BYTES + WII_LZ4_CAPACITY + WII_CATALOG_BYTES
		+ WII_PREVIEW_BYTES + WII_MEM2_GUARD_BYTES >= WII_MEM2_HARDWARE_BYTES)
		return fail("caps do not fit in MEM2");
	if (WII_TAG_RESIDENT_CAP_BYTES >= 0x1600000u)
		return fail("tag cap is not below the Xbox tag cache");
	if (WII_TEXTURE_CAP_BYTES >= 0x1600000u)
		return fail("texture cap is not below the Xbox texture cache");

	memset(header, 0, sizeof(header));
	header[0] = 'h';
	header[1] = 'e';
	header[2] = 'a';
	header[3] = 'd';
	header[4] = 5;
	write_u32(header + 0x08, CACHE_FILE_HEADER_BYTES);
	memcpy(header + 0x20, "synth", 6);
	memcpy(header + 0x40, "wii-budget-check", 17);
	header[0x7FC] = 'f';
	header[0x7FD] = 'o';
	header[0x7FE] = 'o';
	header[0x7FF] = 't';
	format = cache_file_header_format(header);
	if (format != _cache_file_format_xbox_cache)
		return fail("cache_file_header_format missed the synthetic header");

	memset(image_bytes, 0, sizeof(image_bytes));
	memcpy(image_bytes + XDVDFS_VOLUME_OFFSET, XDVDFS_MAGIC, XDVDFS_MAGIC_BYTES);
	write_u32(image_bytes + XDVDFS_VOLUME_OFFSET + 0x14, 33);
	write_u32(image_bytes + XDVDFS_VOLUME_OFFSET + 0x18, 96);
	memset(directory, 0, sizeof(directory));
	/* Root at offset 0 is m.map, left child a.map, right child the maps directory. */
	put_entry(directory, 0, 32 / 4, 64 / 4, 34, CACHE_FILE_HEADER_BYTES, 0, "m.map");
	put_entry(directory, 32, 0, 0, 35, CACHE_FILE_HEADER_BYTES, 0, "a.map");
	put_entry(directory, 64, 0, 0, 36, 0, 1, "maps");
	memcpy(image_bytes + 33 * XDVDFS_SECTOR_BYTES, directory, sizeof(directory));
	memcpy(image_bytes + 34 * XDVDFS_SECTOR_BYTES, header, sizeof(header));

	image.bytes = image_bytes;
	image.size = (uint32_t)sizeof(image_bytes);
	if (xdvdfs_probe(image_read, &image, image.size, &volume) != 0)
		return fail("xdvdfs_probe");
	if (volume.root_sector != 33 || volume.root_size != 96)
		return fail("volume root");
	memset(&expect, 0, sizeof(expect));
	if (xdvdfs_walk(image_bytes + 33 * XDVDFS_SECTOR_BYTES, 96, on_visit, &expect) != 0)
		return fail("walk");
	if (expect.map_files != 2 || expect.maps_dir != 1)
		return fail("walk did not see both maps and the maps directory");

	source.context = &image;
	source.read = opence_read;
	source.size = CACHE_FILE_HEADER_BYTES;
	/* Point the source at the embedded header by shifting the image view. */
	image.bytes = image_bytes + 34 * XDVDFS_SECTOR_BYTES;
	image.size = CACHE_FILE_HEADER_BYTES;
	status = cache_file_identify(&source, &identity);
	if (status != _cache_file_status_ok)
		return fail(cache_file_status_describe(status));
	if (identity.format != _cache_file_format_xbox_cache)
		return fail("identify format");
	if (strcmp(identity.name, "synth") != 0)
		return fail("identify name");
	if (strcmp(identity.build, "wii-budget-check") != 0)
		return fail("identify build");
	if (identity.tag_data_size != 0)
		return fail("synthetic header should not claim tag bytes");
	if (0x1600000u <= WII_TAG_RESIDENT_CAP_BYTES)
		return fail("xbox tag cache was not capped");

	{
		uint8_t map_bytes[0x1800];
		struct image map;
		float positions[12];
		uint8_t colors[16];
		uint16_t indices[12];
		struct bsp_preview_store store;
		struct bsp_preview_info info;
		uint32_t const tag = 0x800;
		uint32_t const scenario = 0x900;
		uint32_t const reference = 0xF00;
		uint32_t const bsp = 0x1200;
		uint32_t const bsp_va = 0x80400000u;

		memset(map_bytes, 0, sizeof(map_bytes));
		map_bytes[0] = 'h';
		map_bytes[1] = 'e';
		map_bytes[2] = 'a';
		map_bytes[3] = 'd';
		map_bytes[4] = 5;
		write_u32(map_bytes + 0x08, (uint32_t)sizeof(map_bytes));
		write_u32(map_bytes + 0x10, tag);
		write_u32(map_bytes + 0x14, 0x720);
		memcpy(map_bytes + 0x20, "a10", 4);
		memcpy(map_bytes + 0x40, "01.01.14.2342", 14);
		map_bytes[0x7FC] = 'f';
		map_bytes[0x7FD] = 'o';
		map_bytes[0x7FE] = 'o';
		map_bytes[0x7FF] = 't';
		write_u32(map_bytes + tag + 0x00, 0x803A6000u + (0x828 - tag));
		write_u32(map_bytes + tag + 0x04, 0);
		write_u32(map_bytes + tag + 0x0C, 1);
		write_u32(map_bytes + tag + 0x20, 0x73676174u);
		write_u32(map_bytes + 0x828, 0x726e6373u);
		write_u32(map_bytes + 0x828 + 0x14, 0x803A6000u + (scenario - tag));
		write_u32(map_bytes + scenario + 0x5A4, 1);
		write_u32(map_bytes + scenario + 0x5A8, 0x803A6000u + (reference - tag));
		write_u32(map_bytes + reference + 0x00, bsp);
		write_u32(map_bytes + reference + 0x04, 0x428);
		write_u32(map_bytes + reference + 0x08, bsp_va);
		write_u32(map_bytes + bsp + 0x00, bsp_va + 0x18);
		write_u32(map_bytes + bsp + 0x14, 0x70736273u);
		write_u32(map_bytes + bsp + 0x18 + 0xF8, 1);
		write_u32(map_bytes + bsp + 0x18 + 0xFC, bsp_va + 0x2A0);
		write_u32(map_bytes + bsp + 0x18 + 0x104, 1);
		write_u32(map_bytes + bsp + 0x18 + 0x108, bsp_va + 0x2A8);
		map_bytes[bsp + 0x2A0] = 0;
		map_bytes[bsp + 0x2A2] = 1;
		map_bytes[bsp + 0x2A4] = 2;
		write_u32(map_bytes + bsp + 0x2A8 + 0x14, 1);
		write_u32(map_bytes + bsp + 0x2A8 + 0x18, bsp_va + 0x2C8);
		write_u32(map_bytes + bsp + 0x2C8 + 0x14, 0);
		write_u32(map_bytes + bsp + 0x2C8 + 0x18, 1);
		map_bytes[bsp + 0x2C8 + 0xB0] = 1;
		write_u32(map_bytes + bsp + 0x2C8 + 0xB4, 3);
		write_u32(map_bytes + bsp + 0x2C8 + 0xEC, 0x60);
		write_u32(map_bytes + bsp + 0x2C8 + 0xEC + 12, bsp_va + 0x3C8);
		write_u32(map_bytes + bsp + 0x3C8 + 32, 0x3f800000u);
		write_u32(map_bytes + bsp + 0x3C8 + 64 + 4, 0x3f800000u);

		map.bytes = map_bytes;
		map.size = (uint32_t)sizeof(map_bytes);
		source.context = &map;
		source.read = opence_read;
		source.size = map.size;
		status = cache_file_identify(&source, &identity);
		if (status != _cache_file_status_ok)
			return fail("bsp map identify");
		store.positions = positions;
		store.colors = colors;
		store.uvs = 0;
		store.indices = indices;
		store.vertex_cap = 4;
		store.triangle_cap = 4;
		if (bsp_preview_load(image_read, &map, map.size, identity.tag_data_offset, identity.tag_data_size, &store, &info) != 0)
			return fail(info.message[0] ? info.message : "bsp preview");
		if (info.triangles != 1 || info.vertices != 3)
			return fail("bsp triangle count");
		if (positions[3] != 1.0f || positions[7] != 1.0f)
			return fail("bsp positions");
	}

	stored = LZ4_compress_default("halo-ce-wii", compressed, 11, (int)sizeof(compressed));
	decoded = LZ4_decompress_safe(compressed, restored, stored, (int)sizeof(restored));
	if (stored <= 0 || decoded != 11 || memcmp(restored, "halo-ce-wii", 11) != 0)
		return fail("lz4");

	{
		uint8_t pixels[8 * 4];
		struct wii_rgba image;
		uint8_t swizzled[8];
		uint8_t linear[2] = { 0x00, 0xf8 };
		uint32_t index = wii_tex_swizzle_index(3, 1, 4, 2);

		if (index != 7)
			return fail("swizzle index");
		memset(swizzled, 0, sizeof(swizzled));
		swizzled[index] = 0xff;
		memset(&image, 0, sizeof(image));
		image.pixels = pixels;
		image.cap = 8;
		if (wii_tex_decode(swizzled, sizeof(swizzled), 1, 0, 4, 2, &image) != 0)
			return fail(image.message);
		if (pixels[(1 * 4 + 3) * 4] != 0xff)
			return fail("swizzle decode");
		if (wii_tex_decode(linear, sizeof(linear), 6, 1, 1, 1, &image) != 0)
			return fail("r5g6b5");
		if (pixels[0] < 240)
			return fail("r5g6b5 red");
	}
	{
		uint8_t file[0x600];
		struct image map;
		struct wii_widget widget;
		uint8_t *scratch;
		uint8_t *canvas;
		char const *root_name = "ui\\shell\\main_menu\\main_menu";

		memset(file, 0, sizeof(file));
		write_u32(file + 0x00, 0x803A6000u + 0x28);
		write_u32(file + 0x0c, 1);
		write_u32(file + 0x28, 0x614c6544u);
		write_u32(file + 0x28 + 0x10, 0x803A6000u + 0x500);
		write_u32(file + 0x28 + 0x14, 0x803A6000u + 0x80);
		memcpy(file + 0x500, root_name, strlen(root_name) + 1);
		write_u32(file + 0x80 + 0x3e0, 1);
		write_u32(file + 0x80 + 0x3e4, 0x803A6000u + 0x480);
		write_u32(file + 0x480 + 0x0c, 0xffffffffu);
		memcpy(file + 0x480 + 0x10, "a10", 4);
		map.bytes = file;
		map.size = (uint32_t)sizeof(file);
		if (wii_widget_open_root(image_read, &map, map.size, 0, map.size, &widget) != 0)
			return fail(widget.message);
		scratch = (uint8_t *)malloc(128 * 128 * 4);
		canvas = (uint8_t *)malloc(32 * 32 * 4);
		if (!scratch || !canvas)
			return fail("widget buffers");
		if (wii_widget_compose(image_read, &map, map.size, 0, map.size, &widget, canvas, 32, 32, scratch, 128 * 128 * 4) != 0)
			return fail("widget compose");
		if (widget.child_count != 1 || strcmp(widget.child_name[0], "a10") != 0)
			return fail("widget child");
		if (wii_widget_input(&widget, 0, image_read, &map, map.size, 0, map.size) != 1)
			return fail("widget A");
		if (strcmp(widget.scenario, "a10") != 0)
			return fail("widget scenario");
		free(scratch);
		free(canvas);
	}
	{
		uint8_t record[WII_HS_SCRIPT_BYTES];
		char name[32];
		int type = -1;
		if ((0x5A4 - 0x130 + 0x28) != WII_SCENARIO_HS_SCRIPTS)
			return fail("script offset");
		memset(record, 0, sizeof(record));
		memcpy(record, "startup", 8);
		if (wii_sim_script_record(record, name, sizeof(name), &type) != 0 || type != 0 || strcmp(name, "startup") != 0)
			return fail("script record");
	}
	{
		/* (set n (+ 1 2)) through hs_runtime.c. Node links match hs_compile:
		   a call's data names the predicate, and the predicate's next is the
		   first argument. */
		enum { nodes = 8, ident = 1 };
		uint8_t syntax[0x38 + nodes * 0x14];
		uint8_t script[0x5C];
		uint8_t global[0x5C];
		float value = 0.f;
		int node;
		uint32_t one = 0x3f800000u;
		uint32_t two = 0x40000000u;

		memset(syntax, 0, sizeof(syntax));
		memcpy(syntax, "script node", 11);
		write_u16(syntax + 0x20, nodes);
		write_u16(syntax + 0x22, 0x14);
		syntax[0x24] = 1;
		syntax[0x25] = 1;
		syntax[0x28] = 0x40;
		syntax[0x29] = 0x74;
		syntax[0x2a] = 0x40;
		syntax[0x2b] = 0x64;
		write_u16(syntax + 0x2C, nodes);
		write_u16(syntax + 0x2E, nodes);
		write_u16(syntax + 0x30, nodes);
		write_u16(syntax + 0x32, 2);
		for (node = 0; node < nodes; node++)
			write_u16(syntax + 0x38 + node * 0x14, ident);
		/* 0 set, 1 predicate, 2 variable n, 3 plus, 4 plus predicate, 5 one, 6 two, 7 zero */
		write_u16(syntax + 0x38 + 0 * 0x14 + 2, 4);
		write_u16(syntax + 0x38 + 0 * 0x14 + 4, 6);
		write_u16(syntax + 0x38 + 0 * 0x14 + 6, 8);
		write_u32(syntax + 0x38 + 0 * 0x14 + 8, 0xffffffffu);
		write_u32(syntax + 0x38 + 0 * 0x14 + 16, (ident << 16) | 1);
		write_u16(syntax + 0x38 + 1 * 0x14 + 2, 4);
		write_u16(syntax + 0x38 + 1 * 0x14 + 4, 2);
		write_u16(syntax + 0x38 + 1 * 0x14 + 6, 1 | 8);
		write_u32(syntax + 0x38 + 1 * 0x14 + 8, (ident << 16) | 2);
		write_u16(syntax + 0x38 + 2 * 0x14 + 2, 6);
		write_u16(syntax + 0x38 + 2 * 0x14 + 4, 6);
		write_u16(syntax + 0x38 + 2 * 0x14 + 6, 1 | 4 | 8);
		write_u32(syntax + 0x38 + 2 * 0x14 + 8, (ident << 16) | 3);
		write_u32(syntax + 0x38 + 2 * 0x14 + 16, 0);
		write_u16(syntax + 0x38 + 3 * 0x14 + 2, 7);
		write_u16(syntax + 0x38 + 3 * 0x14 + 4, 6);
		write_u16(syntax + 0x38 + 3 * 0x14 + 6, 8);
		write_u32(syntax + 0x38 + 3 * 0x14 + 8, 0xffffffffu);
		write_u32(syntax + 0x38 + 3 * 0x14 + 16, (ident << 16) | 4);
		write_u16(syntax + 0x38 + 4 * 0x14 + 2, 7);
		write_u16(syntax + 0x38 + 4 * 0x14 + 4, 2);
		write_u16(syntax + 0x38 + 4 * 0x14 + 6, 1 | 8);
		write_u32(syntax + 0x38 + 4 * 0x14 + 8, (ident << 16) | 5);
		write_u16(syntax + 0x38 + 5 * 0x14 + 2, 6);
		write_u16(syntax + 0x38 + 5 * 0x14 + 4, 6);
		write_u16(syntax + 0x38 + 5 * 0x14 + 6, 1 | 8);
		write_u32(syntax + 0x38 + 5 * 0x14 + 8, (ident << 16) | 6);
		write_u32(syntax + 0x38 + 5 * 0x14 + 16, one);
		write_u16(syntax + 0x38 + 6 * 0x14 + 2, 6);
		write_u16(syntax + 0x38 + 6 * 0x14 + 4, 6);
		write_u16(syntax + 0x38 + 6 * 0x14 + 6, 1 | 8);
		write_u32(syntax + 0x38 + 6 * 0x14 + 8, 0xffffffffu);
		write_u32(syntax + 0x38 + 6 * 0x14 + 16, two);
		write_u16(syntax + 0x38 + 7 * 0x14 + 2, 6);
		write_u16(syntax + 0x38 + 7 * 0x14 + 4, 6);
		write_u16(syntax + 0x38 + 7 * 0x14 + 6, 1 | 8);
		write_u32(syntax + 0x38 + 7 * 0x14 + 8, 0xffffffffu);
		memset(script, 0, sizeof(script));
		memcpy(script, "startup", 7);
		write_u16(script + 34, 6);
		write_u32(script + 36, (ident << 16) | 0);
		memset(global, 0, sizeof(global));
		memcpy(global, "n", 2);
		write_u16(global + 32, 6);
		write_u32(global + 40, (ident << 16) | 7);
		if (wii_hs_mount(syntax, sizeof(syntax), script, 1, global, 1, NULL, 0) != 0)
			return fail(wii_hs_note()[0] ? wii_hs_note() : "hs mount");
		if (wii_hs_global_real(0, &value) != 0 || value < 2.9f || value > 3.1f)
			return fail(wii_hs_note()[0] ? wii_hs_note() : "hs (+ 1 2)");
		wii_hs_shutdown();
	}
	{
		if (wii_tag_cache_self_test() != 0)
			return fail("tag cache");
		wii_game_tick();
		if (!game_map_loaded())
			return fail(wii_hs_note()[0] ? wii_hs_note() : "game load");
		if (wii_game_player_unit() == -1)
			return fail("player biped was not spawned");
	}

	printf("host_check: identify, xdvdfs, lz4, bsp, texels, dela, scripts, hs runtime, player biped ok\n");
	return 0;
}
