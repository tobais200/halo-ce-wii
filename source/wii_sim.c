#include "wii_sim.h"

#include <stdio.h>
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
	uint8_t syntax[4];
	uint32_t count;
	uint32_t address;
	uint32_t file_offset;
	uint32_t index;
	uint32_t limit;

	if (!sim)
		return -1;
	memset(sim, 0, sizeof(*sim));
	if (!scenario_file) {
		snprintf(sim->message, sizeof(sim->message), "no scenario for scripts");
		return -1;
	}
	if (pull(read, context, file_size, scenario_file + 0x474u, syntax, 4) == 0)
		sim->syntax_bytes = read_u32(syntax);
	if (pull(read, context, file_size, scenario_file + WII_SCENARIO_HS_SCRIPTS, block, sizeof(block)) != 0) {
		snprintf(sim->message, sizeof(sim->message), "script block unreadable");
		return -1;
	}
	count = read_u32(block);
	address = read_u32(block + 4);
	sim->script_count = (int)count;
	if (!count || address < 0x803A6000u || (address - 0x803A6000u) >= tag_size) {
		snprintf(sim->message, sizeof(sim->message), "%u scripts, syntax %u bytes, vm off",
			(unsigned)count, (unsigned)sim->syntax_bytes);
		sim->loaded = 1;
		return 0;
	}
	file_offset = tag_offset + (address - 0x803A6000u);
	limit = count > 16u ? 16u : count;
	for (index = 0; index < limit; index++) {
		uint8_t record[WII_HS_SCRIPT_BYTES];
		char name[32];
		int type = -1;

		if (pull(read, context, file_size, file_offset + index * WII_HS_SCRIPT_BYTES, record, sizeof(record)) != 0)
			break;
		wii_sim_script_record(record, name, sizeof(name), &type);
		if (type == 0) {
			sim->startup_count++;
			if (!sim->startup_name[0])
				memcpy(sim->startup_name, name, sizeof(sim->startup_name));
		}
	}
	sim->loaded = 1;
	snprintf(sim->message, sizeof(sim->message), "tick 0  %.24s  vm off",
		sim->startup_name[0] ? sim->startup_name : "no startup");
	return 0;
}

void wii_sim_frame(struct wii_sim *sim)
{
	if (!sim || !sim->loaded)
		return;
	sim->frames++;
	if ((sim->frames & 1u) == 0)
		sim->ticks++;
	snprintf(sim->message, sizeof(sim->message), "tick %u  %.24s  vm off",
		(unsigned)sim->ticks,
		sim->startup_name[0] ? sim->startup_name : "no startup");
}
