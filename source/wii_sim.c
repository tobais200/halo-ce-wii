#include "wii_sim.h"

#include "wii_game.h"
#include "wii_hs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t read_u32(uint8_t const *bytes)
{
	return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8)
		| ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static int pull(bsp_read_fn read, void *context, uint32_t file_size, uint32_t offset, void *destination, uint32_t size)
{
	if (!read || (uint64_t)offset + size > file_size)
		return -1;
	return read(context, offset, destination, size);
}

static int block_count(bsp_read_fn read, void *context, uint32_t file_size, uint32_t offset)
{
	uint8_t bytes[4];

	if (pull(read, context, file_size, offset, bytes, 4) != 0)
		return 0;
	return (int)read_u32(bytes);
}

static int read_player(bsp_read_fn read, void *context, uint32_t file_size, uint32_t tag_offset, uint32_t tag_size, uint32_t scenario_file, struct wii_sim *sim)
{
	uint8_t block[8];
	uint8_t location[16];
	uint32_t count;
	uint32_t address;
	uint32_t file_offset;
	uint32_t bits;

	if (pull(read, context, file_size, scenario_file + WII_SCENARIO_PLAYERS, block, sizeof(block)) != 0)
		return -1;
	count = read_u32(block);
	address = read_u32(block + 4);
	if (!count || address < 0x803A6000u || (address - 0x803A6000u) >= tag_size)
		return -1;
	file_offset = tag_offset + (address - 0x803A6000u);
	if (pull(read, context, file_size, file_offset, location, sizeof(location)) != 0)
		return -1;
	bits = read_u32(location);
	memcpy(&sim->player_x, &bits, 4);
	bits = read_u32(location + 4);
	memcpy(&sim->player_y, &bits, 4);
	bits = read_u32(location + 8);
	memcpy(&sim->player_z, &bits, 4);
	bits = read_u32(location + 12);
	memcpy(&sim->player_facing, &bits, 4);
	sim->have_player = 1;
	return 0;
}

static int read_camera_height(bsp_read_fn read, void *context, uint32_t file_size, uint32_t tag_offset, uint32_t tag_size, uint32_t scenario_file, struct wii_sim *sim)
{
	uint8_t block[8];
	uint8_t entry[16];
	uint8_t header[4];
	uint8_t instance[0x20];
	uint8_t height[4];
	uint32_t count;
	uint32_t address;
	uint32_t instances;
	uint32_t datum;
	uint32_t base;
	uint32_t bits;

	if (pull(read, context, file_size, scenario_file + WII_SCENARIO_BIPED_PALETTE, block, sizeof(block)) != 0)
		return -1;
	count = read_u32(block);
	address = read_u32(block + 4);
	if (!count || address < 0x803A6000u || (address - 0x803A6000u) >= tag_size)
		return -1;
	if (pull(read, context, file_size, tag_offset + (address - 0x803A6000u), entry, sizeof(entry)) != 0)
		return -1;
	datum = read_u32(entry + 0x0c);
	if (datum == 0xffffffffu)
		return -1;
	if (pull(read, context, file_size, tag_offset, header, 4) != 0)
		return -1;
	instances = read_u32(header);
	if (instances < 0x803A6000u || (instances - 0x803A6000u) >= tag_size)
		return -1;
	if (pull(read, context, file_size, tag_offset + (instances - 0x803A6000u) + (datum & 0xffffu) * 0x20u, instance, sizeof(instance)) != 0)
		return -1;
	if (read_u32(instance) != WII_BIPED_GROUP)
		return -1;
	base = read_u32(instance + 0x14);
	if (base < 0x803A6000u || (base - 0x803A6000u) + WII_BIPED_STANDING_CAMERA_HEIGHT + 4u > tag_size)
		return -1;
	if (pull(read, context, file_size, tag_offset + (base - 0x803A6000u) + WII_BIPED_STANDING_CAMERA_HEIGHT, height, 4) != 0)
		return -1;
	bits = read_u32(height);
	memcpy(&sim->camera_height, &bits, 4);
	if (sim->camera_height < 0.2f || sim->camera_height > 3.0f)
		return -1;
	sim->have_camera_height = 1;
	return 0;
}

int wii_sim_script_record(uint8_t const *record, char *name, int name_size, int *script_type)
{
	int index;

	if (!record || !name || name_size < 2)
		return -1;
	for (index = 0; index < 31 && index + 1 < name_size; index++) {
		name[index] = (char)record[index];
		if (!name[index])
			break;
	}
	name[index] = 0;
	if (script_type)
		*script_type = (int)(int16_t)(record[32] | (record[33] << 8));
	return 0;
}

int wii_sim_load(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_offset,
	uint32_t tag_size,
	uint32_t scenario_file,
	struct wii_sim *sim)
{
	uint8_t block[12];
	uint8_t syntax_head[16];
	uint8_t *syntax = 0;
	uint8_t *scripts = 0;
	uint8_t *globals = 0;
	uint8_t *strings = 0;
	uint32_t count;
	uint32_t address;
	uint32_t file_offset;
	uint32_t index;
	uint32_t limit;
	uint32_t string_bytes = 0;
	int global_count = 0;

	if (!sim)
		return -1;
	memset(sim, 0, sizeof(*sim));
	wii_hs_shutdown();
	if (!scenario_file) {
		snprintf(sim->message, sizeof(sim->message), "no scenario for scripts");
		return -1;
	}
	sim->biped_count = block_count(read, context, file_size, scenario_file + WII_SCENARIO_BIPEDS);
	sim->weapon_count = block_count(read, context, file_size, scenario_file + WII_SCENARIO_WEAPONS);
	sim->encounter_count = block_count(read, context, file_size, scenario_file + WII_SCENARIO_AI_ENCOUNTERS);
	read_player(read, context, file_size, tag_offset, tag_size, scenario_file, sim);
	read_camera_height(read, context, file_size, tag_offset, tag_size, scenario_file, sim);
	if (pull(read, context, file_size, scenario_file + WII_SCENARIO_HS_SYNTAX, syntax_head, 4) == 0)
		sim->syntax_bytes = read_u32(syntax_head);
	if (pull(read, context, file_size, scenario_file + WII_SCENARIO_HS_STRINGS, syntax_head, 16) == 0) {
		string_bytes = read_u32(syntax_head);
		address = read_u32(syntax_head + 12);
		if (string_bytes && string_bytes <= 0x10000u && address >= 0x803A6000u
			&& (address - 0x803A6000u) < tag_size) {
			strings = malloc(string_bytes);
			if (strings && pull(read, context, file_size, tag_offset + (address - 0x803A6000u), strings, string_bytes) != 0) {
				free(strings);
				strings = 0;
				string_bytes = 0;
			}
		} else {
			string_bytes = 0;
		}
	}
	if (pull(read, context, file_size, scenario_file + WII_SCENARIO_HS_SCRIPTS, block, sizeof(block)) != 0) {
		free(strings);
		snprintf(sim->message, sizeof(sim->message), "script block unreadable");
		return -1;
	}
	count = read_u32(block);
	address = read_u32(block + 4);
	sim->script_count = (int)count;
	if (count && address >= 0x803A6000u && (address - 0x803A6000u) < tag_size) {
		uint32_t bytes = count * WII_HS_SCRIPT_BYTES;

		if (count > 512u)
			bytes = 512u * WII_HS_SCRIPT_BYTES;
		file_offset = tag_offset + (address - 0x803A6000u);
		scripts = malloc(bytes);
		if (scripts && pull(read, context, file_size, file_offset, scripts, bytes) == 0) {
			limit = bytes / WII_HS_SCRIPT_BYTES;
			for (index = 0; index < limit; index++) {
				char name[32];
				int type = -1;

				wii_sim_script_record(scripts + index * WII_HS_SCRIPT_BYTES, name, sizeof(name), &type);
				if (type == 0) {
					sim->startup_count++;
					if (!sim->startup_name[0])
						memcpy(sim->startup_name, name, sizeof(sim->startup_name));
				}
			}
		} else {
			free(scripts);
			scripts = 0;
		}
	}
	if (pull(read, context, file_size, scenario_file + WII_SCENARIO_HS_GLOBALS, block, 8) == 0) {
		uint32_t global_address = read_u32(block + 4);

		global_count = (int)read_u32(block);
		if (global_count > 0 && global_count <= 512 && global_address >= 0x803A6000u
			&& (global_address - 0x803A6000u) < tag_size) {
			uint32_t bytes = (uint32_t)global_count * WII_HS_GLOBAL_BYTES;

			globals = malloc(bytes);
			if (!globals || pull(read, context, file_size,
				tag_offset + (global_address - 0x803A6000u), globals, bytes) != 0) {
				free(globals);
				globals = 0;
				global_count = 0;
			}
		} else {
			global_count = 0;
		}
	}
	if (sim->syntax_bytes >= 0x38u && sim->syntax_bytes <= WII_HS_SYNTAX_BYTES) {
		uint8_t pointer[4];

		if (pull(read, context, file_size, scenario_file + WII_SCENARIO_HS_SYNTAX + 12u, pointer, 4) == 0) {
			address = read_u32(pointer);
			if (address >= 0x803A6000u && (address - 0x803A6000u) < tag_size) {
				syntax = malloc(sim->syntax_bytes);
				if (!syntax || pull(read, context, file_size,
					tag_offset + (address - 0x803A6000u), syntax, sim->syntax_bytes) != 0) {
					free(syntax);
					syntax = 0;
				}
			}
		}
	}
	if (syntax && wii_hs_mount(syntax, sim->syntax_bytes, scripts, sim->script_count,
		globals, global_count, strings, string_bytes) == 0)
		sim->vm = 1;
	free(syntax);
	free(scripts);
	free(globals);
	free(strings);
	sim->loaded = 1;
	wii_sim_frame(sim);
	sim->frames = 0;
	return 0;
}

void wii_sim_frame(struct wii_sim *sim)
{
	char const *note;

	if (!sim || !sim->loaded)
		return;
	sim->frames++;
	if ((sim->frames & 1u) == 0)
		sim->ticks++;
	wii_hs_set_time(sim->ticks);
	wii_game_tick();
	note = wii_hs_note();
	snprintf(sim->message, sizeof(sim->message),
		"tick %u %.12s %s unlinked %d  unit %d ai %d gun %d%s%s",
		(unsigned)sim->ticks,
		sim->startup_name[0] ? sim->startup_name : "no startup",
		sim->vm ? "game" : "vm off",
		sim->vm ? wii_hs_unlinked() : 0,
		wii_game_unit_ticks,
		wii_game_ai_ticks,
		wii_game_weapon_ticks,
		sim->have_camera_height ? "  eye" : "",
		note && note[0] ? "  " : "");
	if (note && note[0]) {
		size_t used = strlen(sim->message);
		if (used + 1 < sizeof(sim->message))
			snprintf(sim->message + used, sizeof(sim->message) - used, "%.40s", note);
	}
}
