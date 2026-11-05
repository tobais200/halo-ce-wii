/*
 * Fix up the tags a tick needs from the map file. The 23 MiB Xbox cache
 * stays on the card. This copies a working set (at most 20 MiB, the
 * resident cap) and byte-swaps it. Structure BSPs are not copied.
 */
#include "cseries/cseries.h"
#include "game/game_globals.h"
#include "models/model_animation_definitions.h"
#include "models/model_definitions.h"
#include "objects/object_types.h"
#include "scenario/scenario_definitions.h"
#include "units/biped_definitions.h"
#include "items/weapon_definitions.h"
#include "ai/ai_scenario_definitions.h"
#include "ai/actor_definitions.h"
#include "items/projectile_definitions.h"
#include "items/equipment_definitions.h"
#include "objects/damage_effect_definitions.h"
#include "physics/collision_model_definitions.h"
#include "physics/collision_bsp_definitions.h"
#include "effects/effect_definitions.h"
#include "effects/contrail_definitions.h"
#include "effects/effects.h"
#include "wii_tag_cache.h"
#include "wii_collision.h"
#include "wii_effects.h"
#include "wii_hud.h"
#include "wii_play.h"

#include <stdlib.h>
#include <string.h>

#ifdef GEKKO
#include "wii_mem.h"
#endif

#define TAG_BASE 0x803A6000u
#define FCC(a, b, c, d) \
	(((uint32_t)(uint8_t)(a) << 24) | ((uint32_t)(uint8_t)(b) << 16) | \
	 ((uint32_t)(uint8_t)(c) << 8) | (uint32_t)(uint8_t)(d))

enum {
	WII_TAG_SLOTS = 512,
	WII_TAG_ARENA = 20u << 20,
	WII_TAG_CAP_PLAYERS = 16,
	WII_TAG_CAP_WEAPONS = 8,
	WII_TAG_CAP_ENCOUNTERS = 16,
	WII_TAG_CAP_SQUADS = 8,
	WII_TAG_CAP_NODES = 64,
	WII_TAG_CAP_SEATS = 8,
	WII_TAG_CAP_CLASSES = 2,
	WII_TAG_CAP_TYPES = 2,
	WII_TAG_CAP_LOCATIONS = 4,
	WII_TAG_CAP_ANIMS = 8,
	WII_TAG_CAP_MAGAZINES = 2,
	/* One animation's default pose or frame blob. Four of these still fit
	   in the 1 MiB arena with the scenario, biped, and weapon tags. */
	WII_TAG_FRAME_BYTES = 65536
};

struct wii_slot {
	uint32_t group;
	uint32_t parent0;
	uint32_t parent1;
	uint32_t datum;
	uint32_t base_va;
	void *base;
	char name[32];
};

static bsp_read_fn tag_read;
static void *tag_context;
static uint32_t tag_file_size;
static uint32_t tag_file_offset;
static uint32_t tag_bytes;
static struct wii_slot slots[WII_TAG_SLOTS];
static int slot_count;
static int cache_ready;
static uint8_t *arena;
static uint32_t arena_used;
static struct scenario *native_scenario;
static struct game_globals *native_globals;
static struct biped_definition *native_biped;
static int weapon_published;
static int encounter_published;
static int location_published;
static int scenery_published;
static int vehicle_published;
static int magazine_rounds;
static int grenade_count;
static long grenade_projectile = -1;
static char chapter_names[8][32];
static int chapter_count;
static long projectile_index = -1;
static long damage_index = -1;
struct projectile_material_response_definition default_projectile_material_response = {
	.default_effect.index = -1,
	.potential_effect.index = -1,
	.detonation_effect.index = -1
};

static uint32_t read_le32(uint8_t const *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
		| ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int32_t read_les32(uint8_t const *p)
{
	return (int32_t)read_le32(p);
}

static int16_t read_les16(uint8_t const *p)
{
	return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static float read_lef(uint8_t const *p)
{
	uint32_t bits = read_le32(p);
	float value;

	memcpy(&value, &bits, sizeof(value));
	return value;
}

static uint32_t file_fourcc(uint8_t const *p)
{
	return FCC(p[0], p[1], p[2], p[3]);
}

static void put_u32(uint8_t *p, uint32_t value)
{
	p[0] = (uint8_t)value;
	p[1] = (uint8_t)(value >> 8);
	p[2] = (uint8_t)(value >> 16);
	p[3] = (uint8_t)(value >> 24);
}

static void put_u16(uint8_t *p, uint16_t value)
{
	p[0] = (uint8_t)value;
	p[1] = (uint8_t)(value >> 8);
}

static void put_f32(uint8_t *p, float value)
{
	uint32_t bits;

	memcpy(&bits, &value, sizeof(bits));
	put_u32(p, bits);
}

static void *bump(uint32_t size)
{
	uint32_t aligned;
	void *block;

	aligned = (size + 7u) & ~7u;
	if (!arena || arena_used + aligned > WII_TAG_ARENA)
		return NULL;
	block = arena + arena_used;
	memset(block, 0, aligned);
	arena_used += aligned;
	return block;
}

static int pull_file(uint32_t offset, void *destination, uint32_t size)
{
	if (!tag_read || (uint64_t)offset + size > tag_file_size)
		return -1;
	return tag_read(tag_context, offset, destination, size);
}

static int pull_va(uint32_t va, void *destination, uint32_t size)
{
	if (va < TAG_BASE || (uint64_t)(va - TAG_BASE) + size > tag_bytes)
		return -1;
	return pull_file(tag_file_offset + (va - TAG_BASE), destination, size);
}

static void ref_clear(struct tag_reference *reference)
{
	reference->group_tag = 0;
	reference->name = NULL;
	reference->name_length = 0;
	reference->index = NONE;
}

static void read_ref(uint8_t const *raw, struct tag_reference *reference)
{
	reference->group_tag = file_fourcc(raw);
	reference->name = NULL;
	reference->name_length = read_les32(raw + 8);
	reference->index = read_les32(raw + 12);
}

static int find_datum(uint32_t datum)
{
	int index;

	if (datum == 0xffffffffu)
		return -1;
	for (index = 0; index < slot_count; index++) {
		if (slots[index].datum == datum)
			return index;
	}
	return (int)(datum & 0xffffu) < slot_count ? (int)(datum & 0xffffu) : -1;
}

static int load_model(int slot)
{
	uint8_t raw[sizeof(struct model)];
	uint8_t header[8];
	struct model *model;
	struct model_node *nodes;
	int count;
	int index;
	uint32_t address;

	if (slot < 0 || slots[slot].base)
		return slot < 0 ? -1 : 0;
	if (pull_va(slots[slot].base_va, raw, sizeof(raw)) != 0)
		return -1;
	count = read_les32(raw + offsetof(struct model, nodes));
	address = read_le32(raw + offsetof(struct model, nodes) + 4);
	if (count < 0)
		count = 0;
	if (count > WII_TAG_CAP_NODES)
		count = WII_TAG_CAP_NODES;
	model = bump(sizeof(*model));
	nodes = count ? bump((uint32_t)count * sizeof(*nodes)) : NULL;
	if (!model || (count && !nodes))
		return -1;
	model->node_list_checksum = read_le32(raw + offsetof(struct model, node_list_checksum));
	model->nodes.count = count;
	model->nodes.address = nodes;
	for (index = 0; index < count; index++) {
		if (pull_va(address + (uint32_t)index * sizeof(*nodes), header, 8) != 0)
			return -1;
		memcpy(nodes[index].name, header, 8);
		if (pull_va(address + (uint32_t)index * sizeof(*nodes), &nodes[index], sizeof(*nodes)) != 0)
			return -1;
		/* The pull above left the node little-endian. Rebuild the fields
		   the matrix walk reads. */
		{
			uint8_t node_raw[sizeof(struct model_node)];

			if (pull_va(address + (uint32_t)index * sizeof(node_raw), node_raw, sizeof(node_raw)) != 0)
				return -1;
			memset(&nodes[index], 0, sizeof(nodes[index]));
			memcpy(nodes[index].name, node_raw, 32);
			nodes[index].next_sibling_node_index = read_les16(node_raw + offsetof(struct model_node, next_sibling_node_index));
			nodes[index].first_child_node_index = read_les16(node_raw + offsetof(struct model_node, first_child_node_index));
			nodes[index].parent_node_index = read_les16(node_raw + offsetof(struct model_node, parent_node_index));
			nodes[index].default_translation.x = read_lef(node_raw + offsetof(struct model_node, default_translation));
			nodes[index].default_translation.y = read_lef(node_raw + offsetof(struct model_node, default_translation) + 4);
			nodes[index].default_translation.z = read_lef(node_raw + offsetof(struct model_node, default_translation) + 8);
			nodes[index].default_rotation.v.i = read_lef(node_raw + offsetof(struct model_node, default_rotation));
			nodes[index].default_rotation.v.j = read_lef(node_raw + offsetof(struct model_node, default_rotation) + 4);
			nodes[index].default_rotation.v.k = read_lef(node_raw + offsetof(struct model_node, default_rotation) + 8);
			nodes[index].default_rotation.w = read_lef(node_raw + offsetof(struct model_node, default_rotation) + 12);
			if (nodes[index].default_rotation.v.i == 0.f && nodes[index].default_rotation.v.j == 0.f
				&& nodes[index].default_rotation.v.k == 0.f && nodes[index].default_rotation.w == 0.f)
				nodes[index].default_rotation.w = 1.f;
		}
	}
	slots[slot].base = model;
	return 0;
}

static void store_u32(uint8_t *p, uint32_t value)
{
	memcpy(p, &value, 4);
}

static void store_u16(uint8_t *p, uint16_t value)
{
	memcpy(p, &value, 2);
}

static void store_f32(uint8_t *p, float value)
{
	memcpy(p, &value, 4);
}

static int span_ok(int size, int offset, int bytes)
{
	if (offset < 0 || bytes < 0)
		return 0;
	if ((uint32_t)offset > (uint32_t)size)
		return 0;
	if ((uint32_t)bytes > (uint32_t)size - (uint32_t)offset)
		return 0;
	return 1;
}

static void native_u32s(uint8_t *p, int count)
{
	int index;

	for (index = 0; index < count; index++)
		store_u32(p + index * 4, read_le32(p + index * 4));
}

static void native_u16s(uint8_t *p, int count)
{
	int index;

	for (index = 0; index < count; index++)
		store_u16(p + index * 2, (uint16_t)read_les16(p + index * 2));
}

static void native_f32s(uint8_t *p, int count)
{
	int index;

	for (index = 0; index < count; index++)
		store_f32(p + index * 4, read_lef(p + index * 4));
}

static int flag_on(void const *flags, int node)
{
	uint32_t word;

	if (!flags || node < 0 || node >= 64)
		return 0;
	memcpy(&word, (uint8_t const *)flags + (node >> 5) * 4, 4);
	return (int)((word >> (node & 31)) & 1u);
}

static int flags_set(void const *flags, int node_count)
{
	int count = 0;
	int node;

	if (node_count > 64)
		node_count = 64;
	for (node = 0; node < node_count; node++)
		count += flag_on(flags, node);
	return count;
}

/* File bytes are little-endian. The sampler reads native shorts and floats.
   On a little-endian host this stores the same bytes. */
static int native_uncompressed(
	uint8_t *bytes,
	int size,
	void const *rotation,
	void const *translation,
	void const *scale,
	int node_count,
	int frame_count,
	int defaults)
{
	int frame;
	int cursor = 0;

	if (node_count < 0 || node_count > WII_TAG_CAP_NODES)
		return -1;
	if (defaults)
		frame_count = 1;
	if (frame_count < 1 || frame_count > 32767)
		return -1;
	for (frame = 0; frame < frame_count; frame++) {
		int node;

		for (node = 0; node < node_count; node++) {
			int use_rotation = defaults ? !flag_on(rotation, node) : flag_on(rotation, node);
			int use_translation = defaults ? !flag_on(translation, node) : flag_on(translation, node);
			int use_scale = defaults ? !flag_on(scale, node) : flag_on(scale, node);

			if (use_rotation) {
				if (!span_ok(size, cursor, 8))
					return -1;
				native_u16s(bytes + cursor, 4);
				cursor += 8;
			}
			if (use_translation) {
				if (!span_ok(size, cursor, 12))
					return -1;
				native_f32s(bytes + cursor, 3);
				cursor += 12;
			}
			if (use_scale) {
				if (!span_ok(size, cursor, 4))
					return -1;
				native_f32s(bytes + cursor, 1);
				cursor += 4;
			}
		}
	}
	return 0;
}

static int native_key_range(uint8_t *blob, int size, int index_off, int index_stride, int value_off, int value_stride, int first, int count, int value_is_float)
{
	if (count <= 0)
		return 0;
	if (first < 0)
		return -1;
	if (!span_ok(size, index_off + first * index_stride, count * index_stride))
		return -1;
	if (!span_ok(size, value_off + first * value_stride, count * value_stride))
		return -1;
	native_u16s(blob + index_off + first * index_stride, count * (index_stride / 2));
	if (value_is_float)
		native_f32s(blob + value_off + first * value_stride, count * (value_stride / 4));
	else
		native_u16s(blob + value_off + first * value_stride, count * (value_stride / 2));
	return 0;
}

static int native_compressed(
	uint8_t *blob,
	int size,
	void const *rotation,
	void const *translation,
	void const *scale,
	int node_count)
{
	int32_t off[11];
	int index;
	int rotation_nodes;
	int translation_nodes;
	int scale_nodes;

	if (node_count < 0 || node_count > WII_TAG_CAP_NODES)
		return -1;
	if (!span_ok(size, 0, 0x2C))
		return -1;
	for (index = 0; index < 11; index++)
		off[index] = read_les32(blob + index * 4);
	native_u32s(blob, 11);
	rotation_nodes = flags_set(rotation, node_count);
	translation_nodes = flags_set(translation, node_count);
	scale_nodes = flags_set(scale, node_count);
	if (!span_ok(size, 0x2C, rotation_nodes * 4))
		return -1;
	native_u32s(blob + 0x2C, rotation_nodes);
	if (translation_nodes && !span_ok(size, off[3], translation_nodes * 4))
		return -1;
	if (translation_nodes)
		native_u32s(blob + off[3], translation_nodes);
	if (scale_nodes && !span_ok(size, off[7], scale_nodes * 4))
		return -1;
	if (scale_nodes)
		native_u32s(blob + off[7], scale_nodes);
	if (!span_ok(size, off[1], node_count * 6) || !span_ok(size, off[5], node_count * 12) || !span_ok(size, off[9], node_count * 4))
		return -1;
	native_u16s(blob + off[1], node_count * 3);
	native_f32s(blob + off[5], node_count * 3);
	native_f32s(blob + off[9], node_count);
	for (index = 0; index < rotation_nodes; index++) {
		uint32_t header;
		int first;
		int count;

		memcpy(&header, blob + 0x2C + index * 4, 4);
		first = (int)(header >> 12);
		count = (int)(header & 0xFFFu);
		if (native_key_range(blob, size, off[0], 2, off[2], 6, first, count, 0) != 0)
			return -1;
	}
	for (index = 0; index < translation_nodes; index++) {
		uint32_t header;
		int first;
		int count;

		memcpy(&header, blob + off[3] + index * 4, 4);
		first = (int)(header >> 12);
		count = (int)(header & 0xFFFu);
		if (native_key_range(blob, size, off[4], 2, off[6], 12, first, count, 1) != 0)
			return -1;
	}
	for (index = 0; index < scale_nodes; index++) {
		uint32_t header;
		int first;
		int count;

		memcpy(&header, blob + off[7] + index * 4, 4);
		first = (int)(header >> 12);
		count = (int)(header & 0xFFFu);
		if (native_key_range(blob, size, off[8], 2, off[10], 4, first, count, 1) != 0)
			return -1;
	}
	return 0;
}

static int load_animation(int slot)
{
	uint8_t raw[sizeof(struct animation_graph)];
	struct animation_graph *graph;
	struct animation_graph_unit_seat *seats;
	int seat_count;
	int seat_index;
	uint32_t seat_va;

	if (slot < 0)
		return -1;
	if (slots[slot].base)
		return 0;
	if (pull_va(slots[slot].base_va, raw, sizeof(raw)) != 0)
		return -1;
	seat_count = read_les32(raw + offsetof(struct animation_graph, unit_seats));
	seat_va = read_le32(raw + offsetof(struct animation_graph, unit_seats) + 4);
	if (seat_count < 0)
		seat_count = 0;
	if (seat_count > WII_TAG_CAP_SEATS)
		seat_count = WII_TAG_CAP_SEATS;
	graph = bump(sizeof(*graph));
	seats = seat_count ? bump((uint32_t)seat_count * sizeof(*seats)) : NULL;
	if (!graph || (seat_count && !seats))
		return -1;
	graph->unit_seats.count = seat_count;
	graph->unit_seats.address = seats;
	{
		int anim_count = read_les32(raw + offsetof(struct animation_graph, animations));
		uint32_t anim_va = read_le32(raw + offsetof(struct animation_graph, animations) + 4);
		struct animation *animations;
		int anim_index;

		if (anim_count < 1)
			anim_count = 1;
		if (anim_count > WII_TAG_CAP_ANIMS)
			anim_count = WII_TAG_CAP_ANIMS;
		animations = bump((uint32_t)anim_count * sizeof(*animations));
		if (!animations)
			return -1;
		graph->animations.count = anim_count;
		graph->animations.address = animations;
		if (read_le32(raw + offsetof(struct animation_graph, animations)) == 0)
			animations[0].node_count = 1;
		for (anim_index = 0; anim_index < anim_count && anim_va; anim_index++) {
			uint8_t anim_raw[sizeof(struct animation)];
			int data_size;

			if (pull_va(anim_va + (uint32_t)anim_index * sizeof(anim_raw), anim_raw, sizeof(anim_raw)) != 0)
				break;
			memcpy(animations[anim_index].name, anim_raw, 32);
			animations[anim_index].type = read_les16(anim_raw + offsetof(struct animation, type));
			animations[anim_index].frame_count = read_les16(anim_raw + offsetof(struct animation, frame_count));
			animations[anim_index].frame_size = read_les16(anim_raw + offsetof(struct animation, frame_size));
			animations[anim_index].frame_info_type = read_les16(anim_raw + offsetof(struct animation, frame_info_type));
			animations[anim_index].node_list_checksum = read_le32(anim_raw + offsetof(struct animation, node_list_checksum));
			animations[anim_index].node_count = read_les16(anim_raw + offsetof(struct animation, node_count));
			animations[anim_index].flags = (word)read_les16(anim_raw + offsetof(struct animation, flags));
			animations[anim_index].nodes_with_translation_flags[0] = read_le32(anim_raw + offsetof(struct animation, nodes_with_translation_flags));
			animations[anim_index].nodes_with_translation_flags[1] = read_le32(anim_raw + offsetof(struct animation, nodes_with_translation_flags) + 4);
			animations[anim_index].nodes_with_rotation_flags[0] = read_le32(anim_raw + offsetof(struct animation, nodes_with_rotation_flags));
			animations[anim_index].nodes_with_rotation_flags[1] = read_le32(anim_raw + offsetof(struct animation, nodes_with_rotation_flags) + 4);
			animations[anim_index].nodes_with_scale_flags[0] = read_le32(anim_raw + offsetof(struct animation, nodes_with_scale_flags));
			animations[anim_index].nodes_with_scale_flags[1] = read_le32(anim_raw + offsetof(struct animation, nodes_with_scale_flags) + 4);
			animations[anim_index].compressed_data_offset = read_les32(anim_raw + offsetof(struct animation, compressed_data_offset));
			data_size = read_les32(anim_raw + offsetof(struct animation, default_data));
			if (data_size > 0 && data_size <= WII_TAG_FRAME_BYTES) {
				uint8_t *bytes = bump((uint32_t)data_size);
				uint32_t data_va = read_le32(anim_raw + offsetof(struct animation, default_data) + 12);

				if (bytes && pull_va(data_va, bytes, (uint32_t)data_size) == 0) {
					if ((animations[anim_index].flags & 1) == 0 &&
						native_uncompressed(bytes, data_size,
							animations[anim_index].nodes_with_rotation_flags,
							animations[anim_index].nodes_with_translation_flags,
							animations[anim_index].nodes_with_scale_flags,
							animations[anim_index].node_count, 1, 1) != 0)
						animations[anim_index].frame_count = 0;
					animations[anim_index].default_data.size = animations[anim_index].frame_count ? data_size : 0;
					animations[anim_index].default_data.address = animations[anim_index].frame_count ? bytes : NULL;
				} else {
					animations[anim_index].frame_count = 0;
				}
			} else if (data_size > WII_TAG_FRAME_BYTES) {
				animations[anim_index].frame_count = 0;
			}
			data_size = read_les32(anim_raw + offsetof(struct animation, data));
			if (data_size > 0 && data_size <= WII_TAG_FRAME_BYTES && animations[anim_index].frame_count > 0) {
				uint8_t *bytes = bump((uint32_t)data_size);
				uint32_t data_va = read_le32(anim_raw + offsetof(struct animation, data) + 12);
				int fixed = 0;

				if (bytes && pull_va(data_va, bytes, (uint32_t)data_size) == 0) {
					if (animations[anim_index].flags & 1) {
						int shift = animations[anim_index].compressed_data_offset;

						fixed = shift >= 0 && span_ok(data_size, shift, 0x2C) &&
							native_compressed(bytes + shift, data_size - shift,
								animations[anim_index].nodes_with_rotation_flags,
								animations[anim_index].nodes_with_translation_flags,
								animations[anim_index].nodes_with_scale_flags,
								animations[anim_index].node_count) == 0;
					} else {
						fixed = native_uncompressed(bytes, data_size,
							animations[anim_index].nodes_with_rotation_flags,
							animations[anim_index].nodes_with_translation_flags,
							animations[anim_index].nodes_with_scale_flags,
							animations[anim_index].node_count,
							animations[anim_index].frame_count, 0) == 0;
					}
				}
				if (fixed) {
					animations[anim_index].data.size = data_size;
					animations[anim_index].data.address = bytes;
				} else {
					animations[anim_index].frame_count = 0;
				}
			} else if (animations[anim_index].frame_count > 0 &&
				((animations[anim_index].flags & 1) ||
					native_uncompressed(NULL, 0,
						animations[anim_index].nodes_with_rotation_flags,
						animations[anim_index].nodes_with_translation_flags,
						animations[anim_index].nodes_with_scale_flags,
						animations[anim_index].node_count,
						animations[anim_index].frame_count, 0) != 0)) {
				animations[anim_index].frame_count = 0;
			}
		}
	}
	for (seat_index = 0; seat_index < seat_count; seat_index++) {
		uint8_t seat_raw[sizeof(struct animation_graph_unit_seat)];
		struct animation_graph_weapon_class *classes;
		int class_count;
		int class_index;
		uint32_t class_va;

		if (pull_va(seat_va + (uint32_t)seat_index * sizeof(seat_raw), seat_raw, sizeof(seat_raw)) != 0)
			return -1;
		memcpy(seats[seat_index].label, seat_raw, 32);
		class_count = read_les32(seat_raw + offsetof(struct animation_graph_unit_seat, weapon_classes));
		class_va = read_le32(seat_raw + offsetof(struct animation_graph_unit_seat, weapon_classes) + 4);
		if (class_count < 0)
			class_count = 0;
		if (class_count > WII_TAG_CAP_CLASSES)
			class_count = WII_TAG_CAP_CLASSES;
		classes = class_count ? bump((uint32_t)class_count * sizeof(*classes)) : NULL;
		if (class_count && !classes)
			return -1;
		seats[seat_index].weapon_classes.count = class_count;
		seats[seat_index].weapon_classes.address = classes;
		for (class_index = 0; class_index < class_count; class_index++) {
			uint8_t class_raw[sizeof(struct animation_graph_weapon_class)];
			struct animation_graph_weapon_type *types;
			int type_count;
			int type_index;
			uint32_t type_va;

			if (pull_va(class_va + (uint32_t)class_index * sizeof(class_raw), class_raw, sizeof(class_raw)) != 0)
				return -1;
			memcpy(classes[class_index].label, class_raw, 32);
			type_count = read_les32(class_raw + offsetof(struct animation_graph_weapon_class, weapon_types));
			type_va = read_le32(class_raw + offsetof(struct animation_graph_weapon_class, weapon_types) + 4);
			if (type_count < 0)
				type_count = 0;
			if (type_count > WII_TAG_CAP_TYPES)
				type_count = WII_TAG_CAP_TYPES;
			types = type_count ? bump((uint32_t)type_count * sizeof(*types)) : NULL;
			if (type_count && !types)
				return -1;
			classes[class_index].weapon_types.count = type_count;
			classes[class_index].weapon_types.address = types;
			{
				int listed = read_les32(class_raw + offsetof(struct animation_graph_weapon_class, animations));
				uint32_t listed_va = read_le32(class_raw + offsetof(struct animation_graph_weapon_class, animations) + 4);
				struct animation_graph_animation_index *indices;
				int listed_index;

				if (listed < 0)
					listed = 0;
				if (listed > 8)
					listed = 8;
				indices = listed ? bump((uint32_t)listed * sizeof(*indices)) : NULL;
				if (listed && !indices)
					return -1;
				for (listed_index = 0; listed_index < listed; listed_index++) {
					uint8_t index_raw[2];

					if (pull_va(listed_va + (uint32_t)listed_index * sizeof(struct animation_graph_animation_index), index_raw, 2) != 0)
						break;
					indices[listed_index].animation_index = read_les16(index_raw);
				}
				classes[class_index].animations.count = listed;
				classes[class_index].animations.address = indices;
			}
			for (type_index = 0; type_index < type_count; type_index++) {
				uint8_t type_raw[32];

				if (pull_va(type_va + (uint32_t)type_index * sizeof(struct animation_graph_weapon_type), type_raw, sizeof(type_raw)) != 0)
					return -1;
				memcpy(types[type_index].label, type_raw, 32);
			}
		}
	}
	slots[slot].base = graph;
	return 0;
}

static int load_biped(int slot)
{
	uint8_t raw[sizeof(struct biped_definition)];
	struct biped_definition *biped;
	int model_slot;
	int anim_slot;

	if (slot < 0)
		return -1;
	if (slots[slot].base)
		return 0;
	if (pull_va(slots[slot].base_va, raw, sizeof(raw)) != 0)
		return -1;
	biped = bump(sizeof(*biped));
	if (!biped)
		return -1;
	biped->object.type = read_les16(raw + offsetof(struct biped_definition, object.type));
	biped->object.flags = (word)read_les16(raw + offsetof(struct biped_definition, object.flags));
	biped->object.bounding_radius = read_lef(raw + offsetof(struct biped_definition, object.bounding_radius));
	biped->object.bounding_offset.x = read_lef(raw + offsetof(struct biped_definition, object.bounding_offset));
	biped->object.bounding_offset.y = read_lef(raw + offsetof(struct biped_definition, object.bounding_offset) + 4);
	biped->object.bounding_offset.z = read_lef(raw + offsetof(struct biped_definition, object.bounding_offset) + 8);
	biped->object.origin_offset.x = read_lef(raw + offsetof(struct biped_definition, object.origin_offset));
	biped->object.origin_offset.y = read_lef(raw + offsetof(struct biped_definition, object.origin_offset) + 4);
	biped->object.origin_offset.z = read_lef(raw + offsetof(struct biped_definition, object.origin_offset) + 8);
	biped->object.acceleration_scale = read_lef(raw + offsetof(struct biped_definition, object.acceleration_scale));
	if (biped->object.acceleration_scale == 0.f)
		biped->object.acceleration_scale = 1.f;
	biped->object.render_bounding_radius = read_lef(raw + offsetof(struct biped_definition, object.render_bounding_radius));
	biped->object.forced_shader_permutation_index = NONE;
	read_ref(raw + offsetof(struct biped_definition, object.model), &biped->object.model);
	read_ref(raw + offsetof(struct biped_definition, object.animation_graph), &biped->object.animation_graph);
	{
		uint8_t coll_raw[16];
		void *model;

		ref_clear(&biped->object.collision_model);
		if (pull_va(slots[slot].base_va + offsetof(struct biped_definition, object.collision_model), coll_raw, 16) == 0) {
			int coll_slot;

			read_ref(coll_raw, &biped->object.collision_model);
			coll_slot = find_datum((uint32_t)biped->object.collision_model.index);
			model = coll_slot >= 0 ? wii_collision_load_model(slots[coll_slot].base_va) : NULL;
			if (model)
				slots[coll_slot].base = model;
			else
				ref_clear(&biped->object.collision_model);
		}
	}
	ref_clear(&biped->object.physics);
	ref_clear(&biped->object.modifier_shader);
	ref_clear(&biped->object.creation_effect);
	biped->unit.default_team = read_les16(raw + offsetof(struct biped_definition, unit.default_team));
	biped->unit.grenade_type = NONE;
	ref_clear(&biped->unit.integrated_light_toggle_effect);
	ref_clear(&biped->unit.spawned_actor_variant);
	ref_clear(&biped->unit.melee_damage);
	biped->biped.flags = read_le32(raw + offsetof(struct biped_definition, biped.flags));
	biped->biped.flags &= ~((1u << _biped_climbs_anything_bit) | (1u << _biped_uses_limp_body_physics_bit) | (1u << _biped_flying_bit));
	biped->biped.moving_turning_speed = read_lef(raw + offsetof(struct biped_definition, biped.moving_turning_speed));
	biped->biped.standing_camera_height = read_lef(raw + offsetof(struct biped_definition, biped.standing_camera_height));
	biped->biped.crouching_camera_height = read_lef(raw + offsetof(struct biped_definition, biped.crouching_camera_height));
	biped->biped.collision_height_standing = read_lef(raw + offsetof(struct biped_definition, biped.collision_height_standing));
	biped->biped.collision_height_crouching = read_lef(raw + offsetof(struct biped_definition, biped.collision_height_crouching));
	biped->biped.collision_radius = read_lef(raw + offsetof(struct biped_definition, biped.collision_radius));
	biped->biped.runtime_crouch_transition_velocity = read_lef(raw + offsetof(struct biped_definition, biped.runtime_crouch_transition_velocity));
	biped->biped.runtime_minimum_normal_k = read_lef(raw + offsetof(struct biped_definition, biped.runtime_minimum_normal_k));
	biped->biped.runtime_downhill_k0 = read_lef(raw + offsetof(struct biped_definition, biped.runtime_downhill_k0));
	biped->biped.runtime_downhill_k1 = read_lef(raw + offsetof(struct biped_definition, biped.runtime_downhill_k1));
	biped->biped.runtime_uphill_k0 = read_lef(raw + offsetof(struct biped_definition, biped.runtime_uphill_k0));
	biped->biped.runtime_uphill_k1 = read_lef(raw + offsetof(struct biped_definition, biped.runtime_uphill_k1));
	biped->biped.downhill_velocity_scale = read_lef(raw + offsetof(struct biped_definition, biped.downhill_velocity_scale));
	biped->biped.uphill_velocity_scale = read_lef(raw + offsetof(struct biped_definition, biped.uphill_velocity_scale));
	biped->biped.runtime_pelvis_node_index = NONE;
	biped->biped.runtime_head_node_index = NONE;
	model_slot = find_datum((uint32_t)biped->object.model.index);
	anim_slot = find_datum((uint32_t)biped->object.animation_graph.index);
	if (model_slot >= 0 && load_model(model_slot) != 0)
		ref_clear(&biped->object.model);
	if (anim_slot >= 0 && load_animation(anim_slot) != 0)
		ref_clear(&biped->object.animation_graph);
	slots[slot].base = biped;
	native_biped = biped;
	return 0;
}

static int load_damage(int slot)
{
	struct damage_effect_definition *effect;
	uint8_t raw[12];

	if (slot < 0 || slots[slot].group != FCC('j', 'p', 't', '!'))
		return -1;
	if (slots[slot].base)
		return 0;
	effect = bump(sizeof(*effect));
	if (!effect)
		return -1;
	if (pull_va(slots[slot].base_va + offsetof(struct damage_effect_definition, damage.damage_minimum), raw, 12) == 0) {
		int modifier;

		effect->damage.damage_minimum = read_lef(raw);
		effect->damage.damage_lower_bound = read_lef(raw + 4);
		effect->damage.damage_upper_bound = read_lef(raw + 8);
		for (modifier = 0; modifier < MAXIMUM_NUMBER_OF_MATERIAL_TYPES; modifier++) {
			if (pull_va(slots[slot].base_va + offsetof(struct damage_effect_definition, damage.material_modifiers) + (uint32_t)modifier * 4u, raw, 4) != 0)
				break;
			effect->damage.material_modifiers[modifier] = read_lef(raw);
		}
	}
	ref_clear(&effect->sound);
	slots[slot].base = effect;
	damage_index = (long)slots[slot].datum;
	return 0;
}

static int load_projectile(int slot)
{
	struct projectile_definition *projectile;
	uint8_t raw[16];

	if (slot < 0 || slots[slot].group != FCC('p', 'r', 'o', 'j'))
		return -1;
	if (slots[slot].base)
		return 0;
	projectile = bump(sizeof(*projectile));
	if (!projectile)
		return -1;
	if (pull_va(slots[slot].base_va, raw, 4) == 0)
		projectile->object.type = read_les16(raw);
	if (projectile->object.type == 0)
		projectile->object.type = (short)_object_type_projectile;
	projectile->object.bounding_radius = 0.05f;
	projectile->object.acceleration_scale = 1.f;
	projectile->object.forced_shader_permutation_index = NONE;
	ref_clear(&projectile->object.model);
	ref_clear(&projectile->object.animation_graph);
	ref_clear(&projectile->object.collision_model);
	ref_clear(&projectile->object.physics);
	ref_clear(&projectile->object.creation_effect);
	if (pull_va(slots[slot].base_va + offsetof(struct projectile_definition, projectile.initial_velocity), raw, 4) == 0)
		projectile->projectile.initial_velocity = read_lef(raw);
	if (pull_va(slots[slot].base_va + offsetof(struct projectile_definition, projectile.collision_radius), raw, 4) == 0)
		projectile->projectile.collision_radius = read_lef(raw);
	if (pull_va(slots[slot].base_va + offsetof(struct projectile_definition, projectile.timer_lower_bound), raw, 8) == 0) {
		projectile->projectile.timer_lower_bound = read_lef(raw);
		projectile->projectile.timer_upper_bound = read_lef(raw + 4);
	}
	ref_clear(&projectile->projectile.effect);
	ref_clear(&projectile->projectile.detonation_started);
	ref_clear(&projectile->projectile.flyby_sound);
	{
		uint8_t damage_raw[16];

		ref_clear(&projectile->projectile.attached_detonation_damage);
		ref_clear(&projectile->projectile.impact_damage);
		if (pull_va(slots[slot].base_va + offsetof(struct projectile_definition, projectile.impact_damage), damage_raw, 16) == 0) {
			int damage_slot;

			read_ref(damage_raw, &projectile->projectile.impact_damage);
			damage_slot = find_datum((uint32_t)projectile->projectile.impact_damage.index);
			if (damage_slot >= 0 && slots[damage_slot].group == FCC('j', 'p', 't', '!'))
				load_damage(damage_slot);
			else
				ref_clear(&projectile->projectile.impact_damage);
		}
		if (pull_va(slots[slot].base_va + offsetof(struct projectile_definition, projectile.attached_detonation_damage), damage_raw, 16) == 0) {
			read_ref(damage_raw, &projectile->projectile.attached_detonation_damage);
			if (find_datum((uint32_t)projectile->projectile.attached_detonation_damage.index) < 0)
				ref_clear(&projectile->projectile.attached_detonation_damage);
		}
	}
	ref_clear(&projectile->projectile.super_detonation);
	slots[slot].base = projectile;
	projectile_index = (long)slots[slot].datum;
	return 0;
}

static int load_equipment(int slot)
{
	struct equipment_definition *equipment;
	uint8_t raw[4];

	if (slot < 0 || slots[slot].group != FCC('e', 'q', 'i', 'p'))
		return -1;
	if (slots[slot].base)
		return 0;
	equipment = bump(sizeof(*equipment));
	if (!equipment)
		return -1;
	equipment->object.type = (short)_object_type_equipment;
	equipment->object.bounding_radius = 0.2f;
	equipment->object.acceleration_scale = 1.f;
	equipment->object.forced_shader_permutation_index = NONE;
	ref_clear(&equipment->object.model);
	ref_clear(&equipment->object.animation_graph);
	ref_clear(&equipment->object.collision_model);
	ref_clear(&equipment->object.physics);
	ref_clear(&equipment->object.creation_effect);
	if (pull_va(slots[slot].base_va + offsetof(struct equipment_definition, equipment.powerup_type), raw, 2) == 0)
		equipment->equipment.powerup_type = read_les16(raw);
	ref_clear(&equipment->equipment.pickup_sound);
	slots[slot].base = equipment;
	return 0;
}

static int load_weapon(int slot)
{
	uint8_t raw[64];
	struct weapon_definition *weapon;

	if (slot < 0 || slots[slot].group != FCC('w', 'e', 'a', 'p'))
		return -1;
	if (slots[slot].base)
		return 0;
	if (pull_va(slots[slot].base_va, raw, sizeof(raw)) != 0)
		return -1;
	weapon = bump(sizeof(*weapon));
	if (!weapon)
		return -1;
	weapon->object.type = read_les16(raw);
	weapon->object.bounding_radius = read_lef(raw + offsetof(struct weapon_definition, object.bounding_radius));
	weapon->object.render_bounding_radius = weapon->object.bounding_radius;
	weapon->object.acceleration_scale = 1.f;
	weapon->object.forced_shader_permutation_index = NONE;
	{
		uint8_t block[8];
		int magazine_count = 0;
		uint32_t magazine_va = 0;

		if (pull_va(slots[slot].base_va + offsetof(struct weapon_definition, weapon.magazines), block, 8) == 0) {
			magazine_count = read_les32(block);
			magazine_va = read_le32(block + 4);
		}
		if (magazine_count < 0)
			magazine_count = 0;
		if (magazine_count > WII_TAG_CAP_MAGAZINES)
			magazine_count = WII_TAG_CAP_MAGAZINES;
		if (magazine_count > 0) {
			struct weapon_magazine_definition *magazines = bump((uint32_t)magazine_count * sizeof(*magazines));
			int magazine_index;

			if (!magazines)
				return -1;
			for (magazine_index = 0; magazine_index < magazine_count; magazine_index++) {
				uint8_t magazine_raw[16];

				if (pull_va(magazine_va + (uint32_t)magazine_index * sizeof(*magazines), magazine_raw, sizeof(magazine_raw)) != 0)
					return -1;
				magazines[magazine_index].flags = read_le32(magazine_raw);
				magazines[magazine_index].rounds_recharged_per_second = read_les16(magazine_raw + 4);
				magazines[magazine_index].rounds_total_initial = read_les16(magazine_raw + 6);
				magazines[magazine_index].rounds_total_maximum = read_les16(magazine_raw + 8);
				magazines[magazine_index].rounds_loaded_maximum = read_les16(magazine_raw + 10);
				if (magazine_index == 0)
					magazine_rounds = magazines[magazine_index].rounds_loaded_maximum;
			}
			weapon->weapon.magazines.count = magazine_count;
			weapon->weapon.magazines.address = magazines;
		}
		if (pull_va(slots[slot].base_va + offsetof(struct weapon_definition, weapon.triggers), block, 8) == 0) {
			int trigger_count = read_les32(block);
			struct weapon_trigger_definition *triggers;

			if (trigger_count < 0)
				trigger_count = 0;
			if (trigger_count > 2)
				trigger_count = 2;
			triggers = trigger_count ? bump((uint32_t)trigger_count * sizeof(*triggers)) : NULL;
			if (trigger_count && !triggers)
				return -1;
			for (magazine_count = 0; magazine_count < trigger_count; magazine_count++) {
				uint8_t ref_raw[16];
				uint32_t trigger_va = read_le32(block + 4);

				ref_clear(&triggers[magazine_count].projectile);
				if (pull_va(trigger_va + (uint32_t)magazine_count * sizeof(*triggers) + offsetof(struct weapon_trigger_definition, projectile), ref_raw, 16) != 0)
					continue;
				read_ref(ref_raw, &triggers[magazine_count].projectile);
				if (load_projectile(find_datum((uint32_t)triggers[magazine_count].projectile.index)) != 0)
					ref_clear(&triggers[magazine_count].projectile);
			}
			weapon->weapon.triggers.count = trigger_count;
			weapon->weapon.triggers.address = triggers;
		}
	}
	ref_clear(&weapon->object.model);
	ref_clear(&weapon->object.animation_graph);
	{
		uint8_t anim_raw[16];

		ref_clear(&weapon->weapon.interface_definition.first_person_animations);
		if (pull_va(slots[slot].base_va + offsetof(struct weapon_definition, weapon.interface_definition.first_person_animations), anim_raw, 16) == 0) {
			read_ref(anim_raw, &weapon->weapon.interface_definition.first_person_animations);
			if (load_animation(find_datum((uint32_t)weapon->weapon.interface_definition.first_person_animations.index)) != 0)
				ref_clear(&weapon->weapon.interface_definition.first_person_animations);
		}
	}
	{
		uint8_t effect_raw[16];
		int effect_slot;

		ref_clear(&weapon->weapon.ready_effect);
		if (pull_va(slots[slot].base_va + offsetof(struct weapon_definition, weapon.ready_effect), effect_raw, 16) == 0) {
			read_ref(effect_raw, &weapon->weapon.ready_effect);
			effect_slot = find_datum((uint32_t)weapon->weapon.ready_effect.index);
			if (effect_slot >= 0 && slots[effect_slot].group == FCC('e', 'f', 'f', 'e')) {
				void *effect = wii_effect_load(slots[effect_slot].base_va);

				if (effect)
					slots[effect_slot].base = effect;
				else
					ref_clear(&weapon->weapon.ready_effect);
			} else {
				ref_clear(&weapon->weapon.ready_effect);
			}
		}
	}
	ref_clear(&weapon->object.collision_model);
	ref_clear(&weapon->object.physics);
	ref_clear(&weapon->object.modifier_shader);
	ref_clear(&weapon->object.creation_effect);
	slots[slot].base = weapon;
	return 0;
}

static int load_globals(int slot)
{
	uint8_t block[8];
	uint8_t info_raw[sizeof(struct game_globals_player_information)];
	uint8_t fall_raw[0x98];
	struct game_globals *globals;
	struct game_globals_player_information *info;
	uint8_t *falling;
	int info_count;
	uint32_t info_va;
	int fall_count;
	uint32_t fall_va;
	int index;
	float huge = 1.0e9f;

	if (slot < 0)
		return -1;
	globals = bump(sizeof(*globals));
	info = bump(sizeof(*info));
	falling = bump(0x98);
	if (!globals || !info || !falling)
		return -1;
	if (pull_va(slots[slot].base_va + offsetof(struct game_globals, player_information), block, 8) != 0)
		return -1;
	info_count = read_les32(block);
	info_va = read_le32(block + 4);
	if (info_count < 1 || pull_va(info_va, info_raw, sizeof(info_raw)) != 0)
		return -1;
	read_ref(info_raw, &info->player_unit);
	info->walking_speed = read_lef(info_raw + offsetof(struct game_globals_player_information, walking_speed));
	info->double_speed_multiplier = read_lef(info_raw + offsetof(struct game_globals_player_information, double_speed_multiplier));
	info->run_forward_speed = read_lef(info_raw + offsetof(struct game_globals_player_information, run_forward_speed));
	info->run_backward_speed = read_lef(info_raw + offsetof(struct game_globals_player_information, run_backward_speed));
	info->run_sideways_speed = read_lef(info_raw + offsetof(struct game_globals_player_information, run_sideways_speed));
	info->run_acceleration = read_lef(info_raw + offsetof(struct game_globals_player_information, run_acceleration));
	info->sneak_forward_speed = read_lef(info_raw + offsetof(struct game_globals_player_information, sneak_forward_speed));
	info->sneak_backward_speed = read_lef(info_raw + offsetof(struct game_globals_player_information, sneak_backward_speed));
	info->sneak_sideways_speed = read_lef(info_raw + offsetof(struct game_globals_player_information, sneak_sideways_speed));
	info->sneak_acceleration = read_lef(info_raw + offsetof(struct game_globals_player_information, sneak_acceleration));
	info->airborne_acceleration = read_lef(info_raw + offsetof(struct game_globals_player_information, airborne_acceleration));
	info->stun_movement_penalty = read_lef(info_raw + offsetof(struct game_globals_player_information, stun_movement_penalty));
	globals->player_information.count = 1;
	globals->player_information.address = info;
	if (pull_va(slots[slot].base_va + offsetof(struct game_globals, falling_damage), block, 8) == 0) {
		fall_count = read_les32(block);
		fall_va = read_le32(block + 4);
		if (fall_count > 0 && pull_va(fall_va, fall_raw, sizeof(fall_raw)) == 0) {
			for (index = 0; index < 0x98; index += 4) {
				uint32_t word = read_le32(fall_raw + index);

				memcpy(falling + index, &word, 4);
			}
		} else {
			fall_count = 0;
		}
	} else {
		fall_count = 0;
	}
	if (fall_count <= 0) {
		for (index = 0; index < 6; index++) {
			int none = NONE;

			memcpy(falling + (index == 0 ? 0x1C : 0x38 + (index - 1) * 0x10), &none, sizeof(none));
		}
		memcpy(falling + 0x8C, &huge, sizeof(huge));
		memcpy(falling + 0x90, &huge, sizeof(huge));
		memcpy(falling + 0x94, &huge, sizeof(huge));
	}
	globals->falling_damage.count = 1;
	globals->falling_damage.address = falling;
	slots[slot].base = globals;
	native_globals = globals;
	if (pull_va(slots[slot].base_va + offsetof(struct game_globals, grenades), block, 8) == 0) {
		int count = read_les32(block);
		uint32_t grenade_va = read_le32(block + 4);
		struct game_globals_grenade *grenades;

		if (count < 0)
			count = 0;
		if (count > 2)
			count = 2;
		grenades = count ? bump((uint32_t)count * sizeof(*grenades)) : NULL;
		if (count && grenades) {
			int grenade_index;

			for (grenade_index = 0; grenade_index < count; grenade_index++) {
				uint8_t grenade_raw[8];

				if (pull_va(grenade_va + (uint32_t)grenade_index * sizeof(*grenades), grenade_raw, 4) != 0)
					break;
				grenades[grenade_index].maximum_count = read_les16(grenade_raw);
				grenades[grenade_index].mp_spawn_default = read_les16(grenade_raw + 2);
				ref_clear(&grenades[grenade_index].throwing_effect);
				ref_clear(&grenades[grenade_index].hud_interface);
				ref_clear(&grenades[grenade_index].item);
				ref_clear(&grenades[grenade_index].projectile);
				{
					uint8_t projectile_ref[16];

					if (pull_va(grenade_va + (uint32_t)grenade_index * sizeof(*grenades) + offsetof(struct game_globals_grenade, projectile), projectile_ref, 16) == 0) {
						read_ref(projectile_ref, &grenades[grenade_index].projectile);
						if (grenade_index == 0)
							grenade_projectile = grenades[grenade_index].projectile.index;
					}
				}
			}
			globals->grenades.count = count;
			globals->grenades.address = grenades;
			grenade_count = count;
		}
	}
	return load_biped(find_datum((uint32_t)info->player_unit.index));
}

static int load_actor(int slot)
{
	uint8_t raw[offsetof(struct actor_definition, type) + sizeof(short)];
	struct actor_definition *actor;

	if (slot < 0 || slots[slot].group != FCC('a', 'c', 't', 'r'))
		return -1;
	if (slots[slot].base)
		return 0;
	if (pull_va(slots[slot].base_va, raw, sizeof(raw)) != 0)
		return -1;
	actor = bump(sizeof(*actor));
	if (!actor)
		return -1;
	actor->flags = read_le32(raw);
	actor->type = read_les16(raw + offsetof(struct actor_definition, type));
	slots[slot].base = actor;
	return 0;
}

static int load_variant(int slot)
{
	uint8_t raw[48];
	struct actor_variant_definition *variant;

	if (slot < 0 || slots[slot].group != FCC('a', 'c', 't', 'v'))
		return -1;
	if (slots[slot].base)
		return 0;
	if (pull_va(slots[slot].base_va, raw, sizeof(raw)) != 0)
		return -1;
	variant = bump(sizeof(*variant));
	if (!variant)
		return -1;
	read_ref(raw + offsetof(struct actor_variant_definition, actor_reference), &variant->actor_reference);
	read_ref(raw + offsetof(struct actor_variant_definition, unit_reference), &variant->unit_reference);
	slots[slot].base = variant;
	if (load_actor(find_datum((uint32_t)variant->actor_reference.index)) != 0)
		ref_clear(&variant->actor_reference);
	{
		uint8_t ref_raw[16];
		uint8_t counts[4];

		ref_clear(&variant->major_upgrade_reference);
		if (pull_va(slots[slot].base_va + offsetof(struct actor_variant_definition, major_upgrade_reference), ref_raw, 16) == 0) {
			read_ref(ref_raw, &variant->major_upgrade_reference);
			if (load_variant(find_datum((uint32_t)variant->major_upgrade_reference.index)) != 0)
				ref_clear(&variant->major_upgrade_reference);
		}
		ref_clear(&variant->ranged_combat.reference);
		if (pull_va(slots[slot].base_va + offsetof(struct actor_variant_definition, ranged_combat.reference), ref_raw, 16) == 0) {
			read_ref(ref_raw, &variant->ranged_combat.reference);
			if (load_weapon(find_datum((uint32_t)variant->ranged_combat.reference.index)) != 0)
				ref_clear(&variant->ranged_combat.reference);
		}
		ref_clear(&variant->items.equipment_reference);
		if (pull_va(slots[slot].base_va + offsetof(struct actor_variant_definition, items.equipment_reference), ref_raw, 16) == 0) {
			read_ref(ref_raw, &variant->items.equipment_reference);
			if (load_equipment(find_datum((uint32_t)variant->items.equipment_reference.index)) != 0)
				ref_clear(&variant->items.equipment_reference);
		}
		variant->grenade_combat.grenade_type = NONE;
		if (pull_va(slots[slot].base_va + offsetof(struct actor_variant_definition, grenade_combat.grenade_type), counts, 2) == 0) {
			int grenade_type = read_les16(counts);

			if (grenade_type >= 0 && grenade_type < 2)
				variant->grenade_combat.grenade_type = (short)grenade_type;
		}
		if (pull_va(slots[slot].base_va + offsetof(struct actor_variant_definition, items.grenades_lower_bound), counts, 4) == 0) {
			variant->items.grenades_lower_bound = read_les16(counts);
			variant->items.grenades_upper_bound = read_les16(counts + 2);
		}
	}
	return 0;
}

static int publish_placements(uint32_t scenario_va, uint32_t block_offset, uint32_t stride, int cap, struct tag_block *out, int *published)
{
	uint8_t block[8];
	int count;
	uint32_t address;
	struct scenario_object_datum *placed;
	int index;

	*published = 0;
	out->count = 0;
	out->address = NULL;
	if (pull_va(scenario_va + block_offset, block, 8) != 0)
		return 0;
	count = read_les32(block);
	address = read_le32(block + 4);
	if (count < 0)
		count = 0;
	if (count > cap)
		count = cap;
	if (count < 1)
		return 0;
	placed = bump((uint32_t)count * sizeof(*placed));
	if (!placed)
		return -1;
	for (index = 0; index < count; index++) {
		uint8_t raw[0x28];

		if (stride < sizeof(raw) || pull_va(address + (uint32_t)index * stride, raw, sizeof(raw)) != 0)
			return -1;
		placed[index].palette_entry_index = read_les16(raw);
		placed[index].name_index = NONE;
		placed[index].position.x = read_lef(raw + 8);
		placed[index].position.y = read_lef(raw + 12);
		placed[index].position.z = read_lef(raw + 16);
	}
	out->count = count;
	out->address = placed;
	*published = count;
	return 0;
}

static int load_scenario(int slot)
{
	struct scenario *scenario;
	uint8_t block[8];
	int count;
	uint32_t address;
	int index;

	if (slot < 0)
		return -1;
	scenario = bump(sizeof(*scenario));
	if (!scenario)
		return -1;
	if (pull_va(slots[slot].base_va + offsetof(struct scenario, players), block, 8) != 0)
		return -1;
	count = read_les32(block);
	address = read_le32(block + 4);
	if (count < 0)
		count = 0;
	if (count > WII_TAG_CAP_PLAYERS)
		count = WII_TAG_CAP_PLAYERS;
	if (count > 0) {
		struct player_starting_location *players = bump((uint32_t)count * sizeof(*players));

		if (!players)
			return -1;
		for (index = 0; index < count; index++) {
			uint8_t raw[sizeof(struct player_starting_location)];

			if (pull_va(address + (uint32_t)index * sizeof(raw), raw, 16) != 0)
				return -1;
			players[index].position.x = read_lef(raw);
			players[index].position.y = read_lef(raw + 4);
			players[index].position.z = read_lef(raw + 8);
			players[index].facing = read_lef(raw + 12);
			players[index].team_index = NONE;
		}
		scenario->players.count = count;
		scenario->players.address = players;
	}
	if (pull_va(slots[slot].base_va + offsetof(struct scenario, weapon_palette), block, 8) == 0) {
		int palette_count = read_les32(block);
		uint32_t palette_va = read_le32(block + 4);
		struct scenario_object_palette_entry *palette;

		if (palette_count < 0)
			palette_count = 0;
		if (palette_count > WII_TAG_CAP_WEAPONS)
			palette_count = WII_TAG_CAP_WEAPONS;
		palette = palette_count ? bump((uint32_t)palette_count * sizeof(*palette)) : NULL;
		if (palette_count && !palette)
			return -1;
		for (index = 0; index < palette_count; index++) {
			uint8_t raw[16];

			if (pull_va(palette_va + (uint32_t)index * sizeof(struct scenario_object_palette_entry), raw, sizeof(raw)) != 0)
				return -1;
			read_ref(raw, &palette[index].reference);
			load_weapon(find_datum((uint32_t)palette[index].reference.index));
		}
		scenario->weapon_palette.count = palette_count;
		scenario->weapon_palette.address = palette;
	}
	if (pull_va(slots[slot].base_va + offsetof(struct scenario, weapons), block, 8) == 0) {
		count = read_les32(block);
		address = read_le32(block + 4);
		if (count < 0)
			count = 0;
		if (count > WII_TAG_CAP_WEAPONS)
			count = WII_TAG_CAP_WEAPONS;
		if (count > scenario->weapon_palette.count)
			count = scenario->weapon_palette.count;
		if (count > 0) {
			struct scenario_weapon_datum *weapons = bump((uint32_t)count * sizeof(*weapons));

			if (!weapons)
				return -1;
			for (index = 0; index < count; index++) {
				uint8_t raw[32];

				if (pull_va(address + (uint32_t)index * sizeof(struct scenario_weapon_datum), raw, sizeof(raw)) != 0)
					return -1;
				weapons[index].object.palette_entry_index = read_les16(raw + offsetof(struct scenario_weapon_datum, object.palette_entry_index));
				weapons[index].object.name_index = NONE;
				weapons[index].object.position.x = read_lef(raw + offsetof(struct scenario_weapon_datum, object.position));
				weapons[index].object.position.y = read_lef(raw + offsetof(struct scenario_weapon_datum, object.position) + 4);
				weapons[index].object.position.z = read_lef(raw + offsetof(struct scenario_weapon_datum, object.position) + 8);
				weapons[index].object.rotation.yaw = read_lef(raw + offsetof(struct scenario_weapon_datum, object.rotation));
				weapons[index].object.rotation.pitch = read_lef(raw + offsetof(struct scenario_weapon_datum, object.rotation) + 4);
				weapons[index].object.rotation.roll = read_lef(raw + offsetof(struct scenario_weapon_datum, object.rotation) + 8);
			}
			scenario->weapons.count = count;
			scenario->weapons.address = weapons;
			weapon_published = count;
		}
	}
	if (pull_va(slots[slot].base_va + offsetof(struct scenario, ai_encounters), block, 8) == 0) {
		count = read_les32(block);
		address = read_le32(block + 4);
		if (count < 0)
			count = 0;
		if (count > WII_TAG_CAP_ENCOUNTERS)
			count = WII_TAG_CAP_ENCOUNTERS;
		if (count > 0) {
			struct encounter_definition *encounters = bump((uint32_t)count * sizeof(*encounters));

			if (!encounters)
				return -1;
			for (index = 0; index < count; index++) {
				uint8_t raw[sizeof(struct encounter_definition)];
				int squad_count;
				uint32_t squad_va;
				struct squad_definition *squads;
				int squad_index;

				if (pull_va(address + (uint32_t)index * sizeof(raw), raw, sizeof(raw)) != 0)
					return -1;
				memcpy(encounters[index].name, raw, 31);
				encounters[index].flags = read_le32(raw + offsetof(struct encounter_definition, flags));
				encounters[index].team_index = read_les16(raw + offsetof(struct encounter_definition, team_index));
				encounters[index].runtime_structure_bsp_reference_index = NONE;
				squad_count = read_les32(raw + offsetof(struct encounter_definition, squads));
				squad_va = read_le32(raw + offsetof(struct encounter_definition, squads) + 4);
				if (squad_count < 0)
					squad_count = 0;
				if (squad_count > WII_TAG_CAP_SQUADS)
					squad_count = WII_TAG_CAP_SQUADS;
				squads = squad_count ? bump((uint32_t)squad_count * sizeof(*squads)) : NULL;
				if (squad_count && !squads)
					return -1;
				for (squad_index = 0; squad_index < squad_count; squad_index++) {
					uint8_t squad_raw[sizeof(struct squad_definition)];

					if (pull_va(squad_va + (uint32_t)squad_index * sizeof(squad_raw), squad_raw, sizeof(squad_raw)) != 0)
						return -1;
					memcpy(squads[squad_index].name, squad_raw, 31);
					squads[squad_index].actor_palette_index = read_les16(squad_raw + offsetof(struct squad_definition, actor_palette_index));
					squads[squad_index].flags = read_le32(squad_raw + offsetof(struct squad_definition, flags));
					squads[squad_index].squad_delay_timer = read_lef(squad_raw + offsetof(struct squad_definition, squad_delay_timer));
					squads[squad_index].min_count = read_les16(squad_raw + offsetof(struct squad_definition, min_count));
					squads[squad_index].max_count = read_les16(squad_raw + offsetof(struct squad_definition, max_count));
					squads[squad_index].platoon_index = NONE;
					{
						int location_count = read_les32(squad_raw + offsetof(struct squad_definition, starting_locations));
						uint32_t location_va = read_le32(squad_raw + offsetof(struct squad_definition, starting_locations) + 4);
						struct actor_starting_location *locations;
						int location_index;

						if (location_count < 0)
							location_count = 0;
						if (location_count > WII_TAG_CAP_LOCATIONS)
							location_count = WII_TAG_CAP_LOCATIONS;
						locations = location_count ? bump((uint32_t)location_count * sizeof(*locations)) : NULL;
						if (location_count && !locations)
							return -1;
						for (location_index = 0; location_index < location_count; location_index++) {
							uint8_t location_raw[sizeof(struct actor_starting_location)];

							if (pull_va(location_va + (uint32_t)location_index * sizeof(location_raw), location_raw, sizeof(location_raw)) != 0)
								return -1;
							locations[location_index].position.x = read_lef(location_raw);
							locations[location_index].position.y = read_lef(location_raw + 4);
							locations[location_index].position.z = read_lef(location_raw + 8);
							locations[location_index].facing = read_lef(location_raw + 12);
							locations[location_index].flags = location_raw[offsetof(struct actor_starting_location, flags)];
							locations[location_index].actor_variant_index = read_les16(location_raw + offsetof(struct actor_starting_location, actor_variant_index));
							locations[location_index].command_list_index = NONE;
							location_published++;
						}
						squads[squad_index].starting_locations.count = location_count;
						squads[squad_index].starting_locations.address = locations;
					}
				}
				encounters[index].squads.count = squad_count;
				encounters[index].squads.address = squads;
				{
					int fp_count = read_les32(raw + offsetof(struct encounter_definition, firing_positions));
					uint32_t fp_va = read_le32(raw + offsetof(struct encounter_definition, firing_positions) + 4);
					struct firing_position_definition *positions;
					int fp_index;

					if (fp_count < 0)
						fp_count = 0;
					if (fp_count > 8)
						fp_count = 8;
					positions = fp_count ? bump((uint32_t)fp_count * sizeof(*positions)) : NULL;
					if (fp_count && !positions)
						return -1;
					for (fp_index = 0; fp_index < fp_count; fp_index++) {
						uint8_t fp_raw[sizeof(struct firing_position_definition)];

						if (pull_va(fp_va + (uint32_t)fp_index * sizeof(fp_raw), fp_raw, sizeof(fp_raw)) != 0)
							return -1;
						positions[fp_index].position.x = read_lef(fp_raw);
						positions[fp_index].position.y = read_lef(fp_raw + 4);
						positions[fp_index].position.z = read_lef(fp_raw + 8);
						positions[fp_index].group_index = read_les16(fp_raw + 12);
						positions[fp_index].cluster_index = read_les16(fp_raw + 14);
						positions[fp_index].surface_index = read_les32(fp_raw + 20);
					}
					encounters[index].firing_positions.count = fp_count;
					encounters[index].firing_positions.address = positions;
				}
			}
			scenario->ai_encounters.count = count;
			scenario->ai_encounters.address = encounters;
			encounter_published = count;
		}
	}
	if (publish_placements(slots[slot].base_va, offsetof(struct scenario, scenery), 0x48u, 16, &scenario->scenery, &scenery_published) != 0)
		return -1;
	if (publish_placements(slots[slot].base_va, offsetof(struct scenario, vehicles), 0x4cu, 8, &scenario->vehicles, &vehicle_published) != 0)
		return -1;
	if (pull_va(slots[slot].base_va + offsetof(struct scenario, ai_actor_palette), block, 8) == 0) {
		int palette_count = read_les32(block);
		uint32_t palette_va = read_le32(block + 4);
		struct tag_reference *palette;

		if (palette_count < 0)
			palette_count = 0;
		if (palette_count > 4)
			palette_count = 4;
		palette = palette_count ? bump((uint32_t)palette_count * sizeof(*palette)) : NULL;
		if (palette_count && !palette)
			return -1;
		for (index = 0; index < palette_count; index++) {
			uint8_t raw[16];

			if (pull_va(palette_va + (uint32_t)index * sizeof(raw), raw, sizeof(raw)) != 0)
				return -1;
			read_ref(raw, &palette[index]);
			load_variant(find_datum((uint32_t)palette[index].index));
		}
		scenario->ai_actor_palette.count = palette_count;
		scenario->ai_actor_palette.address = palette;
	}
	if (pull_va(slots[slot].base_va + offsetof(struct scenario, object_names), block, 8) == 0) {
		int name_count = read_les32(block);
		uint32_t name_va = read_le32(block + 4);
		struct scenario_object_name *names;

		if (name_count < 0)
			name_count = 0;
		if (name_count > 64)
			name_count = 64;
		names = name_count ? bump((uint32_t)name_count * sizeof(*names)) : NULL;
		if (name_count && !names)
			return -1;
		for (index = 0; index < name_count; index++) {
			uint8_t raw[36];

			if (pull_va(name_va + (uint32_t)index * sizeof(struct scenario_object_name), raw, sizeof(raw)) != 0)
				return -1;
			memcpy(names[index].name, raw, 32);
			names[index].name[31] = 0;
			names[index].runtime_object_type = read_les16(raw + 32);
			names[index].runtime_scenario_datum_index = read_les16(raw + 34);
		}
		scenario->object_names.count = name_count;
		scenario->object_names.address = names;
	}
	if (pull_va(slots[slot].base_va + offsetof(struct scenario, cutscene_chapter_titles), block, 8) == 0) {
		int titles = read_les32(block);
		uint32_t title_va = read_le32(block + 4);
		int title_index;

		chapter_count = 0;
		if (titles < 0)
			titles = 0;
		if (titles > 8)
			titles = 8;
		for (title_index = 0; title_index < titles; title_index++) {
			if (pull_va(title_va + (uint32_t)title_index * 0x60u, chapter_names[title_index], 31) != 0)
				break;
			chapter_names[title_index][31] = 0;
			chapter_count++;
		}
	}
	slots[slot].base = scenario;
	native_scenario = scenario;
	return 0;
}

static void *arena_alloc(void)
{
#ifdef GEKKO
	void *block = wii_mem2_alloc(WII_TAG_ARENA);

	if (block)
		return block;
#endif
	return malloc(WII_TAG_ARENA);
}

int wii_tag_cache_mount(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_offset,
	uint32_t tag_size)
{
	uint8_t header[0x10];
	uint32_t instances_va;
	uint32_t count;
	uint32_t index;
	int scenario_slot = -1;
	int globals_slot = -1;

	wii_tag_cache_unmount();
	if (!read || tag_size < 0x40)
		return -1;
	tag_read = read;
	tag_context = context;
	tag_file_size = file_size;
	tag_file_offset = tag_offset;
	tag_bytes = tag_size;
	arena = arena_alloc();
	if (!arena)
		return -1;
	memset(arena, 0, WII_TAG_ARENA);
	if (pull_file(tag_offset, header, sizeof(header)) != 0)
		return -1;
	instances_va = read_le32(header);
	count = read_le32(header + 0x0c);
	if (count == 0 || count > WII_TAG_SLOTS)
		return -1;
	for (index = 0; index < count; index++) {
		uint8_t instance[0x20];
		uint8_t name[32];
		uint32_t name_va;

		if (pull_va(instances_va + index * 0x20u, instance, sizeof(instance)) != 0)
			return -1;
		slots[index].group = file_fourcc(instance);
		slots[index].parent0 = file_fourcc(instance + 4);
		slots[index].parent1 = file_fourcc(instance + 8);
		slots[index].datum = read_le32(instance + 0x0c);
		slots[index].base_va = read_le32(instance + 0x14);
		name_va = read_le32(instance + 0x10);
		if (name_va && pull_va(name_va, name, sizeof(name)) == 0) {
			memcpy(slots[index].name, name, 31);
			slots[index].name[31] = 0;
		}
		if (slots[index].group == FCC('s', 'c', 'n', 'r'))
			scenario_slot = (int)index;
		if (slots[index].group == FCC('m', 'a', 't', 'g'))
			globals_slot = (int)index;
	}
	slot_count = (int)count;
	if (scenario_slot < 0 || load_scenario(scenario_slot) != 0)
		return -1;
	if (globals_slot >= 0 && load_globals(globals_slot) != 0)
		return -1;
	for (index = 0; index < slot_count; index++) {
		if (slots[index].group == FCC('s', 'b', 's', 'p'))
			wii_collision_load(slots[index].base_va);
		if (slots[index].group == FCC('s', 'n', 'd', '!'))
			wii_sound_load(slots[index].base_va);
	}
	cache_ready = native_biped != NULL;
	if (!cache_ready)
		return -1;
	return 0;
}

void wii_tag_cache_unmount(void)
{
#ifndef GEKKO
	free(arena);
#endif
	arena = NULL;
	arena_used = 0;
	slot_count = 0;
	cache_ready = 0;
	native_scenario = NULL;
	native_globals = NULL;
	native_biped = NULL;
	weapon_published = 0;
	encounter_published = 0;
	location_published = 0;
	scenery_published = 0;
	vehicle_published = 0;
	magazine_rounds = 0;
	grenade_count = 0;
	grenade_projectile = -1;
	chapter_count = 0;
	projectile_index = -1;
	damage_index = -1;
	wii_collision_reset();
	memset(slots, 0, sizeof(slots));
}

int wii_tag_cache_ready(void)
{
	return cache_ready;
}

uint32_t wii_tag_cache_resident(void)
{
	return arena_used;
}

float wii_tag_cache_camera_height(void)
{
	if (!native_biped)
		return 0.f;
	return native_biped->biped.standing_camera_height;
}

int wii_tag_cache_weapon_count(void)
{
	return weapon_published;
}

int wii_tag_cache_encounter_count(void)
{
	return encounter_published;
}

int wii_tag_cache_location_count(void)
{
	return location_published;
}

int wii_tag_cache_scenery_count(void)
{
	return scenery_published;
}

int wii_tag_cache_vehicle_count(void)
{
	return vehicle_published;
}

int wii_tag_cache_magazine_rounds(void)
{
	return magazine_rounds;
}

long wii_tag_cache_grenade_projectile(void)
{
	return grenade_projectile;
}

int wii_tag_cache_chapter(int index, char *name, int cap)
{
	if (!name || cap < 2 || index < 0 || index >= chapter_count || !chapter_names[index][0])
		return -1;
	snprintf(name, (size_t)cap, "%s", chapter_names[index]);
	return 0;
}

int wii_tag_cache_grenade_count(void)
{
	return grenade_count;
}

long wii_tag_cache_projectile_index(void)
{
	return projectile_index;
}

long wii_tag_cache_damage_index(void)
{
	return damage_index;
}

int wii_tag_cache_pull(uint32_t va, void *destination, uint32_t size)
{
	return pull_va(va, destination, size);
}

uint32_t wii_tag_cache_slot_va(long tag_index)
{
	int absolute;

	if (tag_index == NONE || slot_count <= 0)
		return 0;
	absolute = (int)(tag_index & 0xffff);
	if (absolute < 0 || absolute >= slot_count)
		return 0;
	if ((long)slots[absolute].datum != tag_index)
		return 0;
	return slots[absolute].base_va;
}

float wii_tag_cache_idle_rotation_w(void)
{
	struct animation_graph *graph;
	struct model *model;
	struct animation *animation;
	struct real_orientation orientation;
	int anim_index = 0;

	if (!native_biped)
		return 0.f;
	graph = wii_tag_cache_get((long)FCC('a', 'n', 't', 'r'), native_biped->object.animation_graph.index);
	model = wii_tag_cache_get((long)FCC('m', 'o', 'd', 'e'), native_biped->object.model.index);
	if (!graph || !model || graph->animations.count < 1 || !graph->animations.address)
		return 0.f;
	if (graph->unit_seats.count > 0 && graph->unit_seats.address) {
		struct animation_graph_unit_seat *seat = graph->unit_seats.address;

		if (seat->weapon_classes.count > 0 && seat->weapon_classes.address) {
			struct animation_graph_weapon_class *weapon_class = seat->weapon_classes.address;

			if (weapon_class->animations.count > 0 && weapon_class->animations.address) {
				struct animation_graph_animation_index *indices = weapon_class->animations.address;

				anim_index = indices[0].animation_index;
			}
		}
	}
	if (anim_index < 0 || anim_index >= graph->animations.count)
		anim_index = 0;
	animation = (struct animation *)graph->animations.address + anim_index;
	memset(&orientation, 0, sizeof(orientation));
	animation_get_node_orientations(model, animation, 0, &orientation);
	return orientation.rotation.w;
}

void wii_tag_cache_player(float *x, float *y, float *z)
{
	struct player_starting_location *player;

	if (!native_scenario || native_scenario->players.count < 1 || !native_scenario->players.address) {
		if (x)
			*x = 0.f;
		if (y)
			*y = 0.f;
		if (z)
			*z = 0.f;
		return;
	}
	player = native_scenario->players.address;
	if (x)
		*x = player->position.x;
	if (y)
		*y = player->position.y;
	if (z)
		*z = player->position.z;
}

struct scenario *wii_tag_cache_scenario(void)
{
	return cache_ready ? native_scenario : NULL;
}

struct game_globals *wii_tag_cache_globals(void)
{
	return cache_ready ? native_globals : NULL;
}

void *wii_tag_cache_get(long group_tag, long tag_index)
{
	int absolute;
	struct wii_slot *slot;

	if (!cache_ready || tag_index == NONE)
		return NULL;
	absolute = tag_index & 0xffff;
	if (absolute < 0 || absolute >= slot_count)
		return NULL;
	slot = &slots[absolute];
	if ((long)slot->datum != tag_index || !slot->base)
		return NULL;
	if (group_tag && slot->group != (uint32_t)group_tag
		&& slot->parent0 != (uint32_t)group_tag
		&& slot->parent1 != (uint32_t)group_tag)
		return NULL;
	return slot->base;
}

long wii_tag_cache_group(long tag_index)
{
	int absolute;

	if (!cache_ready || tag_index == NONE)
		return 0;
	absolute = tag_index & 0xffff;
	if (absolute < 0 || absolute >= slot_count)
		return 0;
	if ((long)slots[absolute].datum != tag_index)
		return 0;
	return (long)slots[absolute].group;
}

static int memory_read(void *context, uint64_t offset, void *destination, uint32_t size)
{
	uint8_t *bytes = context;

	memcpy(destination, bytes + offset, size);
	return 0;
}

int wii_tag_cache_self_test(void)
{
	static uint8_t file[0x20000];
	uint32_t cursor;
	uint32_t instances;
	uint32_t scenario_at;
	uint32_t player_at;
	uint32_t weapon_at;
	uint32_t palette_at;
	uint32_t encounter_at;
	uint32_t squad_at;
	uint32_t location_at;
	uint32_t actor_palette_at;
	uint32_t globals_at;
	uint32_t info_at;
	uint32_t fall_at;
	uint32_t biped_at;
	uint32_t model_at;
	uint32_t node_at;
	uint32_t geom_at;
	uint32_t mesh_part_at;
	uint32_t mesh_vert_at;
	uint32_t mesh_tri_at;
	uint32_t shader_at;
	uint32_t graph_at;
	uint32_t seat_at;
	uint32_t class_at;
	uint32_t class_anim_at;
	uint32_t type_at;
	uint32_t anim_at;
	uint32_t overlay_at;
	uint32_t replace_at;
	uint32_t chapter_at;
	uint32_t font_at;
	uint32_t font_table_at;
	uint32_t font_index_at;
	uint32_t font_char_at;
	uint32_t font_pix_at;
	uint32_t marker_at;
	uint32_t marker_inst_at;
	uint32_t default_at;
	uint32_t overlay_keys_at;
	uint32_t replace_keys_at;
	uint32_t firing_at;
	uint32_t scenery_at;
	uint32_t scenery_pal_at;
	uint32_t vehicle_at;
	uint32_t vehicle_pal_at;
	uint32_t weapon_def_at;
	uint32_t variant_at;
	uint32_t actor_at;
	uint32_t sbsp_at;
	uint32_t coll_at;
	uint32_t coll_node_at;
	uint32_t material_at;
	uint32_t damage_at;
	uint32_t collide_at;
	uint32_t plane_at;
	uint32_t node3d_at;
	uint32_t leaf_at;
	uint32_t ref2d_at;
	uint32_t surface_at;
	uint32_t proj_at;
	uint32_t eqip_at;
	uint32_t upgrade_at;
	uint32_t trigger_at;
	uint32_t grenade_at;
	uint32_t effect_at;
	uint32_t event_at;
	uint32_t part_at;
	uint32_t cont_at;
	uint32_t bitm_at;
	uint32_t bitm_data_at;
	uint32_t pixels_at;
	uint32_t datum[17];
	float x;
	float y;
	float z;
	int index;

	memset(file, 0, sizeof(file));
	cursor = 0x40 + 17u * 0x20u;
	instances = 0x40;
	scenario_at = cursor;
	cursor += 0x800;
	player_at = cursor;
	cursor += sizeof(struct player_starting_location);
	weapon_at = cursor;
	cursor += sizeof(struct scenario_weapon_datum);
	palette_at = cursor;
	cursor += sizeof(struct scenario_object_palette_entry);
	encounter_at = cursor;
	cursor += sizeof(struct encounter_definition);
	squad_at = cursor;
	cursor += sizeof(struct squad_definition);
	location_at = cursor;
	cursor += sizeof(struct actor_starting_location);
	actor_palette_at = cursor;
	cursor += sizeof(struct tag_reference);
	globals_at = cursor;
	cursor += sizeof(struct game_globals) + 16u;
	info_at = cursor;
	cursor += sizeof(struct game_globals_player_information);
	fall_at = cursor;
	cursor += 0x98;
	biped_at = cursor;
	cursor += sizeof(struct biped_definition);
	model_at = cursor;
	cursor += sizeof(struct model);
	node_at = cursor;
	cursor += sizeof(struct model_node);
	geom_at = cursor;
	cursor += 0x2cu;
	mesh_part_at = cursor;
	cursor += 0x68u;
	mesh_vert_at = cursor;
	cursor += 0x20u * 3u;
	mesh_tri_at = cursor;
	cursor += 6u;
	shader_at = cursor;
	cursor += 32u;
	chapter_at = cursor;
	cursor += 0x60u;
	marker_at = cursor;
	cursor += 64u;
	marker_inst_at = cursor;
	cursor += 32u;
	font_at = cursor;
	cursor += 0xa0u;
	font_table_at = cursor;
	cursor += 12u;
	font_index_at = cursor;
	cursor += 512u;
	font_char_at = cursor;
	cursor += 0x14u;
	font_pix_at = cursor;
	cursor += 4u;
	graph_at = cursor;
	cursor += sizeof(struct animation_graph);
	seat_at = cursor;
	cursor += sizeof(struct animation_graph_unit_seat);
	class_at = cursor;
	cursor += sizeof(struct animation_graph_weapon_class);
	class_anim_at = cursor;
	cursor += sizeof(struct animation_graph_animation_index);
	type_at = cursor;
	cursor += sizeof(struct animation_graph_weapon_type);
	anim_at = cursor;
	cursor += sizeof(struct animation);
	overlay_at = cursor;
	cursor += sizeof(struct animation);
	replace_at = cursor;
	cursor += sizeof(struct animation);
	default_at = cursor;
	cursor += 24u;
	overlay_keys_at = cursor;
	cursor += 48u;
	replace_keys_at = cursor;
	cursor += 48u;
	firing_at = cursor;
	cursor += 72u;
	scenery_at = cursor;
	cursor += 0x48u;
	scenery_pal_at = cursor;
	cursor += 16u;
	vehicle_at = cursor;
	cursor += 0x4cu;
	vehicle_pal_at = cursor;
	cursor += 16u;
	weapon_def_at = cursor;
	cursor += sizeof(struct weapon_definition);
	variant_at = cursor;
	cursor += sizeof(struct actor_variant_definition);
	actor_at = cursor;
	cursor += sizeof(struct actor_definition);
	upgrade_at = cursor;
	cursor += sizeof(struct actor_variant_definition);
	proj_at = cursor;
	cursor += sizeof(struct projectile_definition);
	eqip_at = cursor;
	cursor += sizeof(struct equipment_definition);
	trigger_at = cursor;
	cursor += sizeof(struct weapon_trigger_definition);
	grenade_at = cursor;
	cursor += sizeof(struct game_globals_grenade);
	sbsp_at = cursor;
	cursor += 648u;
	collide_at = cursor;
	cursor += 96u;
	plane_at = cursor;
	cursor += 16u;
	node3d_at = cursor;
	cursor += 12u;
	leaf_at = cursor;
	cursor += 8u;
	ref2d_at = cursor;
	cursor += 8u;
	surface_at = cursor;
	cursor += 12u;
	coll_at = cursor;
	cursor += sizeof(struct collision_model);
	coll_node_at = cursor;
	cursor += sizeof(struct collision_node);
	material_at = cursor;
	cursor += sizeof(struct damage_resistance_material);
	damage_at = cursor;
	cursor += sizeof(struct damage_effect_definition);
	effect_at = cursor;
	cursor += sizeof(struct effect_definition);
	event_at = cursor;
	cursor += sizeof(struct effect_event_definition);
	part_at = cursor;
	cursor += sizeof(struct effect_part_definition);
	cont_at = cursor;
	cursor += sizeof(struct contrail_definition);
	bitm_at = cursor;
	cursor += 0x6cu;
	bitm_data_at = cursor;
	cursor += 0x30u;
	pixels_at = cursor;
	cursor += 16u;
	if (cursor > sizeof(file))
		return -1;
	for (index = 0; index < 17; index++)
		datum[index] = 0x12340000u | (uint32_t)index;
	put_u32(file, TAG_BASE + instances);
	put_u32(file + 4, datum[0]);
	put_u32(file + 0x0c, 17);
	file[0x20] = 't';
	file[0x21] = 'a';
	file[0x22] = 'g';
	file[0x23] = 's';
	{
		struct {
			uint32_t at;
			char a, b, c, d;
			char p0a, p0b, p0c, p0d;
			char p1a, p1b, p1c, p1d;
		} rows[17] = {
			{ scenario_at, 's', 'c', 'n', 'r', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ globals_at, 'm', 'a', 't', 'g', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ biped_at, 'b', 'i', 'p', 'd', 'u', 'n', 'i', 't', 'o', 'b', 'j', 'e' },
			{ model_at, 'm', 'o', 'd', 'e', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ graph_at, 'a', 'n', 't', 'r', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ weapon_def_at, 'w', 'e', 'a', 'p', 'i', 't', 'e', 'm', 'o', 'b', 'j', 'e' },
			{ variant_at, 'a', 'c', 't', 'v', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ actor_at, 'a', 'c', 't', 'r', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ upgrade_at, 'a', 'c', 't', 'v', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ proj_at, 'p', 'r', 'o', 'j', 'o', 'b', 'j', 'e', 0, 0, 0, 0 },
			{ eqip_at, 'e', 'q', 'i', 'p', 'i', 't', 'e', 'm', 'o', 'b', 'j', 'e' },
			{ sbsp_at, 's', 'b', 's', 'p', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ coll_at, 'c', 'o', 'l', 'l', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ damage_at, 'j', 'p', 't', '!', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ effect_at, 'e', 'f', 'f', 'e', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ cont_at, 'c', 'o', 'n', 't', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ bitm_at, 'b', 'i', 't', 'm', 0, 0, 0, 0, 0, 0, 0, 0 }
		};

		for (index = 0; index < 17; index++) {
			uint8_t *instance = file + instances + (uint32_t)index * 0x20u;

			instance[0] = (uint8_t)rows[index].a;
			instance[1] = (uint8_t)rows[index].b;
			instance[2] = (uint8_t)rows[index].c;
			instance[3] = (uint8_t)rows[index].d;
			instance[4] = (uint8_t)rows[index].p0a;
			instance[5] = (uint8_t)rows[index].p0b;
			instance[6] = (uint8_t)rows[index].p0c;
			instance[7] = (uint8_t)rows[index].p0d;
			instance[8] = (uint8_t)rows[index].p1a;
			instance[9] = (uint8_t)rows[index].p1b;
			instance[10] = (uint8_t)rows[index].p1c;
			instance[11] = (uint8_t)rows[index].p1d;
			put_u32(instance + 0x0c, datum[index]);
			put_u32(instance + 0x14, TAG_BASE + rows[index].at);
		}
	}
	put_u32(file + scenario_at + offsetof(struct scenario, cutscene_chapter_titles), 1);
	put_u32(file + scenario_at + offsetof(struct scenario, cutscene_chapter_titles) + 4, TAG_BASE + chapter_at);
	memcpy(file + chapter_at, "PILLAR", 6);
	put_u32(file + scenario_at + offsetof(struct scenario, players), 1);
	put_u32(file + scenario_at + offsetof(struct scenario, players) + 4, TAG_BASE + player_at);
	put_f32(file + player_at, 9.f);
	put_f32(file + player_at + 4, 8.f);
	put_f32(file + player_at + 8, 7.f);
	put_f32(file + player_at + 12, 0.25f);
	put_u32(file + scenario_at + offsetof(struct scenario, weapon_palette), 1);
	put_u32(file + scenario_at + offsetof(struct scenario, weapon_palette) + 4, TAG_BASE + palette_at);
	file[palette_at] = 'w';
	file[palette_at + 1] = 'e';
	file[palette_at + 2] = 'a';
	file[palette_at + 3] = 'p';
	put_u32(file + palette_at + 12, datum[5]);
	put_u32(file + scenario_at + offsetof(struct scenario, weapons), 1);
	put_u32(file + scenario_at + offsetof(struct scenario, weapons) + 4, TAG_BASE + weapon_at);
	put_u16(file + weapon_at, 0);
	put_u16(file + weapon_at + 2, 0xffffu);
	put_f32(file + weapon_at + offsetof(struct scenario_weapon_datum, object.position), 3.f);
	put_f32(file + weapon_at + offsetof(struct scenario_weapon_datum, object.position) + 4, 4.f);
	put_f32(file + weapon_at + offsetof(struct scenario_weapon_datum, object.position) + 8, 5.f);
	put_u32(file + scenario_at + offsetof(struct scenario, ai_encounters), 1);
	put_u32(file + scenario_at + offsetof(struct scenario, ai_encounters) + 4, TAG_BASE + encounter_at);
	memcpy(file + encounter_at, "gate", 4);
	put_u32(file + encounter_at + offsetof(struct encounter_definition, flags), 0);
	put_u16(file + encounter_at + offsetof(struct encounter_definition, team_index), 1);
	put_u32(file + encounter_at + offsetof(struct encounter_definition, squads), 1);
	put_u32(file + encounter_at + offsetof(struct encounter_definition, squads) + 4, TAG_BASE + squad_at);
	put_u32(file + encounter_at + offsetof(struct encounter_definition, firing_positions), 3);
	put_u32(file + encounter_at + offsetof(struct encounter_definition, firing_positions) + 4, TAG_BASE + firing_at);
	put_f32(file + firing_at, 7.5f);
	put_f32(file + firing_at + 4, 2.5f);
	put_f32(file + firing_at + 8, 3.5f);
	put_f32(file + firing_at + 24, 1.5f);
	put_f32(file + firing_at + 28, 8.5f);
	put_f32(file + firing_at + 32, 3.5f);
	put_f32(file + firing_at + 48, 1.9f);
	put_f32(file + firing_at + 52, 2.7f);
	put_f32(file + firing_at + 56, 3.5f);
	put_u32(file + scenario_at + offsetof(struct scenario, scenery), 1);
	put_u32(file + scenario_at + offsetof(struct scenario, scenery) + 4, TAG_BASE + scenery_at);
	put_u16(file + scenery_at, 0);
	put_f32(file + scenery_at + 8, 4.f);
	put_f32(file + scenery_at + 12, 5.f);
	put_f32(file + scenery_at + 16, 3.5f);
	put_u32(file + scenario_at + offsetof(struct scenario, scenery_palette), 1);
	put_u32(file + scenario_at + offsetof(struct scenario, scenery_palette) + 4, TAG_BASE + scenery_pal_at);
	file[scenery_pal_at] = 'b';
	file[scenery_pal_at + 1] = 'i';
	file[scenery_pal_at + 2] = 'p';
	file[scenery_pal_at + 3] = 'd';
	put_u32(file + scenery_pal_at + 12, datum[2]);
	put_u32(file + scenario_at + offsetof(struct scenario, vehicles), 1);
	put_u32(file + scenario_at + offsetof(struct scenario, vehicles) + 4, TAG_BASE + vehicle_at);
	put_u16(file + vehicle_at, 0);
	put_f32(file + vehicle_at + 8, 6.f);
	put_f32(file + vehicle_at + 12, 6.f);
	put_f32(file + vehicle_at + 16, 3.5f);
	put_u32(file + scenario_at + offsetof(struct scenario, vehicle_palette), 1);
	put_u32(file + scenario_at + offsetof(struct scenario, vehicle_palette) + 4, TAG_BASE + vehicle_pal_at);
	file[vehicle_pal_at] = 'b';
	file[vehicle_pal_at + 1] = 'i';
	file[vehicle_pal_at + 2] = 'p';
	file[vehicle_pal_at + 3] = 'd';
	put_u32(file + vehicle_pal_at + 12, datum[2]);
	memcpy(file + squad_at, "squad", 5);
	put_u32(file + squad_at + offsetof(struct squad_definition, flags), 0);
	put_u16(file + squad_at + offsetof(struct squad_definition, actor_palette_index), 0);
	put_u16(file + squad_at + offsetof(struct squad_definition, min_count), 1);
	put_u16(file + squad_at + offsetof(struct squad_definition, max_count), 1);
	put_u32(file + squad_at + offsetof(struct squad_definition, starting_locations), 1);
	put_u32(file + squad_at + offsetof(struct squad_definition, starting_locations) + 4, TAG_BASE + location_at);
	put_f32(file + location_at, 1.5f);
	put_f32(file + location_at + 4, 2.5f);
	put_f32(file + location_at + 8, 3.5f);
	put_f32(file + location_at + 12, 0.5f);
	put_u16(file + location_at + offsetof(struct actor_starting_location, actor_variant_index), 0xffffu);
	put_u16(file + actor_at + offsetof(struct actor_definition, type), 0);
	put_u32(file + scenario_at + offsetof(struct scenario, ai_actor_palette), 1);
	put_u32(file + scenario_at + offsetof(struct scenario, ai_actor_palette) + 4, TAG_BASE + actor_palette_at);
	file[actor_palette_at] = 'a';
	file[actor_palette_at + 1] = 'c';
	file[actor_palette_at + 2] = 't';
	file[actor_palette_at + 3] = 'v';
	put_u32(file + actor_palette_at + 12, datum[6]);
	file[variant_at + offsetof(struct actor_variant_definition, actor_reference)] = 'a';
	file[variant_at + offsetof(struct actor_variant_definition, actor_reference) + 1] = 'c';
	file[variant_at + offsetof(struct actor_variant_definition, actor_reference) + 2] = 't';
	file[variant_at + offsetof(struct actor_variant_definition, actor_reference) + 3] = 'r';
	put_u32(file + variant_at + offsetof(struct actor_variant_definition, actor_reference) + 12, datum[7]);
	file[variant_at + offsetof(struct actor_variant_definition, unit_reference)] = 'b';
	file[variant_at + offsetof(struct actor_variant_definition, unit_reference) + 1] = 'i';
	file[variant_at + offsetof(struct actor_variant_definition, unit_reference) + 2] = 'p';
	file[variant_at + offsetof(struct actor_variant_definition, unit_reference) + 3] = 'd';
	put_u32(file + variant_at + offsetof(struct actor_variant_definition, unit_reference) + 12, datum[2]);
	file[variant_at + offsetof(struct actor_variant_definition, major_upgrade_reference)] = 'a';
	file[variant_at + offsetof(struct actor_variant_definition, major_upgrade_reference) + 1] = 'c';
	file[variant_at + offsetof(struct actor_variant_definition, major_upgrade_reference) + 2] = 't';
	file[variant_at + offsetof(struct actor_variant_definition, major_upgrade_reference) + 3] = 'v';
	put_u32(file + variant_at + offsetof(struct actor_variant_definition, major_upgrade_reference) + 12, datum[8]);
	file[upgrade_at + offsetof(struct actor_variant_definition, actor_reference)] = 'a';
	file[upgrade_at + offsetof(struct actor_variant_definition, actor_reference) + 1] = 'c';
	file[upgrade_at + offsetof(struct actor_variant_definition, actor_reference) + 2] = 't';
	file[upgrade_at + offsetof(struct actor_variant_definition, actor_reference) + 3] = 'r';
	put_u32(file + upgrade_at + offsetof(struct actor_variant_definition, actor_reference) + 12, datum[7]);
	file[upgrade_at + offsetof(struct actor_variant_definition, unit_reference)] = 'b';
	file[upgrade_at + offsetof(struct actor_variant_definition, unit_reference) + 1] = 'i';
	file[upgrade_at + offsetof(struct actor_variant_definition, unit_reference) + 2] = 'p';
	file[upgrade_at + offsetof(struct actor_variant_definition, unit_reference) + 3] = 'd';
	put_u32(file + upgrade_at + offsetof(struct actor_variant_definition, unit_reference) + 12, datum[2]);
	put_u32(file + upgrade_at + offsetof(struct actor_variant_definition, major_upgrade_reference) + 12, 0xffffffffu);
	file[variant_at + offsetof(struct actor_variant_definition, ranged_combat.reference)] = 'w';
	file[variant_at + offsetof(struct actor_variant_definition, ranged_combat.reference) + 1] = 'e';
	file[variant_at + offsetof(struct actor_variant_definition, ranged_combat.reference) + 2] = 'a';
	file[variant_at + offsetof(struct actor_variant_definition, ranged_combat.reference) + 3] = 'p';
	put_u32(file + variant_at + offsetof(struct actor_variant_definition, ranged_combat.reference) + 12, datum[5]);
	file[variant_at + offsetof(struct actor_variant_definition, items.equipment_reference)] = 'e';
	file[variant_at + offsetof(struct actor_variant_definition, items.equipment_reference) + 1] = 'q';
	file[variant_at + offsetof(struct actor_variant_definition, items.equipment_reference) + 2] = 'i';
	file[variant_at + offsetof(struct actor_variant_definition, items.equipment_reference) + 3] = 'p';
	put_u32(file + variant_at + offsetof(struct actor_variant_definition, items.equipment_reference) + 12, datum[10]);
	put_u16(file + variant_at + offsetof(struct actor_variant_definition, grenade_combat.grenade_type), 0);
	put_u16(file + variant_at + offsetof(struct actor_variant_definition, items.grenades_lower_bound), 1);
	put_u16(file + variant_at + offsetof(struct actor_variant_definition, items.grenades_upper_bound), 1);
	put_u32(file + globals_at + offsetof(struct game_globals, grenades), 1);
	put_u32(file + globals_at + offsetof(struct game_globals, grenades) + 4, TAG_BASE + grenade_at);
	put_u16(file + grenade_at, 4);
	file[grenade_at + offsetof(struct game_globals_grenade, projectile)] = 'p';
	file[grenade_at + offsetof(struct game_globals_grenade, projectile) + 1] = 'r';
	file[grenade_at + offsetof(struct game_globals_grenade, projectile) + 2] = 'o';
	file[grenade_at + offsetof(struct game_globals_grenade, projectile) + 3] = 'j';
	put_u32(file + grenade_at + offsetof(struct game_globals_grenade, projectile) + 12, datum[9]);
	wii_collision_write_floor(file, sbsp_at, collide_at, plane_at, node3d_at, leaf_at, ref2d_at, surface_at);
	put_u16(file + globals_at + offsetof(struct game_globals, player_information), 1);
	put_u32(file + globals_at + offsetof(struct game_globals, player_information) + 4, TAG_BASE + info_at);
	file[info_at] = 'b';
	file[info_at + 1] = 'i';
	file[info_at + 2] = 'p';
	file[info_at + 3] = 'd';
	put_u32(file + info_at + 12, datum[2]);
	put_f32(file + info_at + offsetof(struct game_globals_player_information, walking_speed), 0.5f);
	put_f32(file + info_at + offsetof(struct game_globals_player_information, run_forward_speed), 2.25f);
	put_f32(file + info_at + offsetof(struct game_globals_player_information, run_acceleration), 0.3f);
	put_u32(file + globals_at + offsetof(struct game_globals, falling_damage), 1);
	put_u32(file + globals_at + offsetof(struct game_globals, falling_damage) + 4, TAG_BASE + fall_at);
	put_f32(file + fall_at + 0x8C, 1.0e9f);
	put_f32(file + fall_at + 0x90, 1.0e9f);
	put_f32(file + fall_at + 0x94, 1.0e9f);
	put_u16(file + biped_at, (uint16_t)_object_type_biped);
	put_f32(file + biped_at + offsetof(struct biped_definition, object.bounding_radius), 0.5f);
	put_f32(file + biped_at + offsetof(struct biped_definition, object.acceleration_scale), 1.f);
	put_f32(file + biped_at + offsetof(struct biped_definition, object.render_bounding_radius), 0.5f);
	file[biped_at + offsetof(struct biped_definition, object.model)] = 'm';
	file[biped_at + offsetof(struct biped_definition, object.model) + 1] = 'o';
	file[biped_at + offsetof(struct biped_definition, object.model) + 2] = 'd';
	file[biped_at + offsetof(struct biped_definition, object.model) + 3] = 'e';
	put_u32(file + biped_at + offsetof(struct biped_definition, object.model) + 12, datum[3]);
	file[biped_at + offsetof(struct biped_definition, object.animation_graph)] = 'a';
	file[biped_at + offsetof(struct biped_definition, object.animation_graph) + 1] = 'n';
	file[biped_at + offsetof(struct biped_definition, object.animation_graph) + 2] = 't';
	file[biped_at + offsetof(struct biped_definition, object.animation_graph) + 3] = 'r';
	put_u32(file + biped_at + offsetof(struct biped_definition, object.animation_graph) + 12, datum[4]);
	file[biped_at + offsetof(struct biped_definition, object.collision_model)] = 'c';
	file[biped_at + offsetof(struct biped_definition, object.collision_model) + 1] = 'o';
	file[biped_at + offsetof(struct biped_definition, object.collision_model) + 2] = 'l';
	file[biped_at + offsetof(struct biped_definition, object.collision_model) + 3] = 'l';
	put_u32(file + biped_at + offsetof(struct biped_definition, object.collision_model) + 12, datum[12]);
	put_f32(file + coll_at + offsetof(struct collision_model, resistance.maximum_body_vitality), 1.f);
	put_f32(file + coll_at + offsetof(struct collision_model, resistance.maximum_shield_vitality), 1.f);
	put_u32(file + coll_at + offsetof(struct collision_model, resistance.materials), 1);
	put_u32(file + coll_at + offsetof(struct collision_model, resistance.materials) + 4, TAG_BASE + material_at);
	put_f32(file + material_at + offsetof(struct damage_resistance_material, shield_damage_multiplier), 1.f);
	put_f32(file + material_at + offsetof(struct damage_resistance_material, body_damage_multiplier), 1.f);
	put_u32(file + coll_at + offsetof(struct collision_model, nodes), 1);
	put_u32(file + coll_at + offsetof(struct collision_model, nodes) + 4, TAG_BASE + coll_node_at);
	put_u16(file + coll_node_at + offsetof(struct collision_node, region_index), 0);
	put_u16(file + coll_node_at + offsetof(struct collision_node, parent_node_index), 0xffffu);
	put_u16(file + coll_node_at + offsetof(struct collision_node, next_sibling_node_index), 0xffffu);
	put_u16(file + coll_node_at + offsetof(struct collision_node, first_child_node_index), 0xffffu);
	put_u32(file + coll_node_at + offsetof(struct collision_node, bsps), 1);
	put_u32(file + coll_node_at + offsetof(struct collision_node, bsps) + 4, TAG_BASE + collide_at);
	put_u32(file + biped_at + offsetof(struct biped_definition, biped.flags), 1u << _biped_uses_player_physics_bit);
	put_f32(file + biped_at + offsetof(struct biped_definition, biped.standing_camera_height), 0.62f);
	put_f32(file + biped_at + offsetof(struct biped_definition, biped.crouching_camera_height), 0.4f);
	put_u32(file + model_at + offsetof(struct model, nodes), 1);
	put_u32(file + model_at + offsetof(struct model, nodes) + 4, TAG_BASE + node_at);
	put_u16(file + node_at + offsetof(struct model_node, next_sibling_node_index), 0xffffu);
	put_u16(file + node_at + offsetof(struct model_node, first_child_node_index), 0xffffu);
	put_u16(file + node_at + offsetof(struct model_node, parent_node_index), 0xffffu);
	put_f32(file + node_at + offsetof(struct model_node, default_translation) + 8, 0.5f);
	put_u32(file + model_at + offsetof(struct model, geometries), 1);
	put_u32(file + model_at + offsetof(struct model, geometries) + 4, TAG_BASE + geom_at);
	put_u32(file + geom_at + 0x24, 1);
	put_u32(file + geom_at + 0x28, TAG_BASE + mesh_part_at);
	put_u32(file + mesh_part_at + 0x28, 3);
	put_u32(file + mesh_part_at + 0x2c, TAG_BASE + mesh_vert_at);
	put_u32(file + mesh_part_at + 0x30, 3);
	put_u32(file + mesh_part_at + 0x34, TAG_BASE + mesh_tri_at);
	put_f32(file + mesh_vert_at, 0.4f);
	put_f32(file + mesh_vert_at + 0x20, 0.f);
	put_f32(file + mesh_vert_at + 0x20 + 4, 0.4f);
	put_f32(file + mesh_vert_at + 0x40 + 8, 0.4f);
	file[mesh_vert_at + 0x1e] = 0xff;
	file[mesh_vert_at + 0x1f] = 0x7f;
	file[mesh_vert_at + 0x20 + 0x1e] = 0xff;
	file[mesh_vert_at + 0x20 + 0x1f] = 0x7f;
	file[mesh_vert_at + 0x40 + 0x1e] = 0xff;
	file[mesh_vert_at + 0x40 + 0x1f] = 0x7f;
	file[mesh_tri_at] = 0;
	file[mesh_tri_at + 2] = 1;
	file[mesh_tri_at + 4] = 2;
	put_u32(file + model_at + offsetof(struct model, shaders), 2);
	put_u32(file + model_at + offsetof(struct model, shaders) + 4, TAG_BASE + shader_at);
	file[shader_at] = 'b';
	file[shader_at + 1] = 'i';
	file[shader_at + 2] = 't';
	file[shader_at + 3] = 'm';
	put_u32(file + shader_at + 12, datum[16]);
	file[shader_at + 16] = 'b';
	file[shader_at + 17] = 'i';
	file[shader_at + 18] = 't';
	file[shader_at + 19] = 'm';
	put_u32(file + shader_at + 28, datum[16]);
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.interface_definition.first_person_model)] = 'm';
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.interface_definition.first_person_model) + 1] = 'o';
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.interface_definition.first_person_model) + 2] = 'd';
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.interface_definition.first_person_model) + 3] = 'e';
	put_u32(file + weapon_def_at + offsetof(struct weapon_definition, weapon.interface_definition.first_person_model) + 12, datum[3]);
	file[weapon_def_at + offsetof(struct weapon_definition, object.model)] = 'm';
	file[weapon_def_at + offsetof(struct weapon_definition, object.model) + 1] = 'o';
	file[weapon_def_at + offsetof(struct weapon_definition, object.model) + 2] = 'd';
	file[weapon_def_at + offsetof(struct weapon_definition, object.model) + 3] = 'e';
	put_u32(file + weapon_def_at + offsetof(struct weapon_definition, object.model) + 12, datum[3]);
	put_u32(file + graph_at + offsetof(struct animation_graph, unit_seats), 1);
	put_u32(file + graph_at + offsetof(struct animation_graph, unit_seats) + 4, TAG_BASE + seat_at);
	memcpy(file + seat_at, "stand", 5);
	put_u32(file + seat_at + offsetof(struct animation_graph_unit_seat, weapon_classes), 1);
	put_u32(file + seat_at + offsetof(struct animation_graph_unit_seat, weapon_classes) + 4, TAG_BASE + class_at);
	put_u32(file + class_at + offsetof(struct animation_graph_weapon_class, weapon_types), 1);
	put_u32(file + class_at + offsetof(struct animation_graph_weapon_class, weapon_types) + 4, TAG_BASE + type_at);
	put_u32(file + class_at + offsetof(struct animation_graph_weapon_class, animations), 1);
	put_u32(file + class_at + offsetof(struct animation_graph_weapon_class, animations) + 4, TAG_BASE + class_anim_at);
	put_u16(file + class_anim_at, 0);
	put_u32(file + graph_at + offsetof(struct animation_graph, animations), 3);
	put_u32(file + graph_at + offsetof(struct animation_graph, animations) + 4, TAG_BASE + anim_at);
	put_u16(file + anim_at + offsetof(struct animation, frame_count), 4);
	put_u16(file + anim_at + offsetof(struct animation, node_count), 1);
	put_u32(file + anim_at + offsetof(struct animation, default_data), 24);
	put_u32(file + anim_at + offsetof(struct animation, default_data) + 12, TAG_BASE + default_at);
	memcpy(file + overlay_at, file + anim_at, sizeof(struct animation));
	put_u16(file + overlay_at + offsetof(struct animation, type), 1);
	put_u16(file + overlay_at + offsetof(struct animation, frame_count), 4);
	put_u16(file + overlay_at + offsetof(struct animation, frame_size), 12);
	put_u32(file + overlay_at + offsetof(struct animation, nodes_with_translation_flags), 1);
	put_u32(file + overlay_at + offsetof(struct animation, data), 48);
	put_u32(file + overlay_at + offsetof(struct animation, data) + 12, TAG_BASE + overlay_keys_at);
	memcpy(file + replace_at, file + anim_at, sizeof(struct animation));
	put_u16(file + replace_at + offsetof(struct animation, type), 2);
	put_u16(file + replace_at + offsetof(struct animation, frame_count), 4);
	put_u16(file + replace_at + offsetof(struct animation, frame_size), 12);
	put_u32(file + replace_at + offsetof(struct animation, nodes_with_translation_flags), 1);
	put_u32(file + replace_at + offsetof(struct animation, data), 48);
	put_u32(file + replace_at + offsetof(struct animation, data) + 12, TAG_BASE + replace_keys_at);
	put_f32(file + overlay_keys_at + 0, 0.f);
	put_f32(file + overlay_keys_at + 12, 0.08f);
	put_f32(file + overlay_keys_at + 24, 0.16f);
	put_f32(file + overlay_keys_at + 36, 0.24f);
	put_f32(file + replace_keys_at + 8, 0.10f);
	put_f32(file + replace_keys_at + 20, 0.25f);
	put_f32(file + replace_keys_at + 32, 0.40f);
	put_f32(file + replace_keys_at + 44, 0.55f);
	put_u16(file + default_at + 6, 32767);
	put_u16(file + weapon_def_at, (uint16_t)_object_type_weapon);
	put_f32(file + weapon_def_at + offsetof(struct weapon_definition, object.bounding_radius), 0.2f);
	put_u32(file + weapon_def_at + offsetof(struct weapon_definition, weapon.magazines), 1);
	put_u32(file + weapon_def_at + offsetof(struct weapon_definition, weapon.magazines) + 4, TAG_BASE + weapon_def_at + sizeof(struct weapon_definition) - 32u);
	put_u16(file + weapon_def_at + sizeof(struct weapon_definition) - 32u + 6, 36);
	put_u16(file + weapon_def_at + sizeof(struct weapon_definition) - 32u + 8, 36);
	put_u16(file + weapon_def_at + sizeof(struct weapon_definition) - 32u + 10, 36);
	put_u32(file + weapon_def_at + offsetof(struct weapon_definition, weapon.triggers), 1);
	put_u32(file + weapon_def_at + offsetof(struct weapon_definition, weapon.triggers) + 4, TAG_BASE + trigger_at);
	file[trigger_at + offsetof(struct weapon_trigger_definition, projectile)] = 'p';
	file[trigger_at + offsetof(struct weapon_trigger_definition, projectile) + 1] = 'r';
	file[trigger_at + offsetof(struct weapon_trigger_definition, projectile) + 2] = 'o';
	file[trigger_at + offsetof(struct weapon_trigger_definition, projectile) + 3] = 'j';
	put_u32(file + trigger_at + offsetof(struct weapon_trigger_definition, projectile) + 12, datum[9]);
	put_u16(file + proj_at, (uint16_t)_object_type_projectile);
	put_f32(file + proj_at + offsetof(struct projectile_definition, projectile.initial_velocity), 20.f);
	put_f32(file + proj_at + offsetof(struct projectile_definition, projectile.collision_radius), 0.05f);
	put_f32(file + proj_at + offsetof(struct projectile_definition, projectile.timer_lower_bound), 1.f);
	put_f32(file + proj_at + offsetof(struct projectile_definition, projectile.timer_upper_bound), 1.f);
	file[proj_at + offsetof(struct projectile_definition, projectile.impact_damage)] = 'j';
	file[proj_at + offsetof(struct projectile_definition, projectile.impact_damage) + 1] = 'p';
	file[proj_at + offsetof(struct projectile_definition, projectile.impact_damage) + 2] = 't';
	file[proj_at + offsetof(struct projectile_definition, projectile.impact_damage) + 3] = '!';
	put_u32(file + proj_at + offsetof(struct projectile_definition, projectile.impact_damage) + 12, datum[13]);
	file[proj_at + offsetof(struct projectile_definition, projectile.attached_detonation_damage)] = 'j';
	file[proj_at + offsetof(struct projectile_definition, projectile.attached_detonation_damage) + 1] = 'p';
	file[proj_at + offsetof(struct projectile_definition, projectile.attached_detonation_damage) + 2] = 't';
	file[proj_at + offsetof(struct projectile_definition, projectile.attached_detonation_damage) + 3] = '!';
	put_u32(file + proj_at + offsetof(struct projectile_definition, projectile.attached_detonation_damage) + 12, datum[13]);
	put_f32(file + damage_at + offsetof(struct damage_effect_definition, damage.damage_minimum), 1.f);
	put_f32(file + damage_at + offsetof(struct damage_effect_definition, damage.damage_lower_bound), 1.f);
	put_f32(file + damage_at + offsetof(struct damage_effect_definition, damage.damage_upper_bound), 1.f);
	{
		int modifier;

		for (modifier = 0; modifier < MAXIMUM_NUMBER_OF_MATERIAL_TYPES; modifier++)
			put_f32(file + damage_at + offsetof(struct damage_effect_definition, damage.material_modifiers) + (uint32_t)modifier * 4u, 1.f);
	}
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.interface_definition.first_person_animations)] = 'a';
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.interface_definition.first_person_animations) + 1] = 'n';
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.interface_definition.first_person_animations) + 2] = 't';
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.interface_definition.first_person_animations) + 3] = 'r';
	put_u32(file + weapon_def_at + offsetof(struct weapon_definition, weapon.interface_definition.first_person_animations) + 12, datum[4]);
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.ready_effect)] = 'e';
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.ready_effect) + 1] = 'f';
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.ready_effect) + 2] = 'f';
	file[weapon_def_at + offsetof(struct weapon_definition, weapon.ready_effect) + 3] = 'e';
	put_u32(file + weapon_def_at + offsetof(struct weapon_definition, weapon.ready_effect) + 12, datum[14]);
	put_u32(file + effect_at + offsetof(struct effect_definition, events), 1);
	put_u32(file + effect_at + offsetof(struct effect_definition, events) + 4, TAG_BASE + event_at);
	put_u32(file + event_at + offsetof(struct effect_event_definition, parts), 1);
	put_u32(file + event_at + offsetof(struct effect_event_definition, parts) + 4, TAG_BASE + part_at);
	file[part_at + offsetof(struct effect_part_definition, reference)] = 'c';
	file[part_at + offsetof(struct effect_part_definition, reference) + 1] = 'o';
	file[part_at + offsetof(struct effect_part_definition, reference) + 2] = 'n';
	file[part_at + offsetof(struct effect_part_definition, reference) + 3] = 't';
	put_u32(file + part_at + offsetof(struct effect_part_definition, reference) + 12, datum[15]);
	file[cont_at + offsetof(struct contrail_definition, bitmap)] = 'b';
	file[cont_at + offsetof(struct contrail_definition, bitmap) + 1] = 'i';
	file[cont_at + offsetof(struct contrail_definition, bitmap) + 2] = 't';
	file[cont_at + offsetof(struct contrail_definition, bitmap) + 3] = 'm';
	put_u32(file + cont_at + offsetof(struct contrail_definition, bitmap) + 12, datum[16]);
	put_u32(file + bitm_at + 0x60, 1);
	put_u32(file + bitm_at + 0x64, TAG_BASE + bitm_data_at);
	put_u32(file + bitm_at + 0x30 + 12, TAG_BASE + pixels_at);
	put_u16(file + bitm_data_at + 0x04, 4);
	put_u16(file + bitm_data_at + 0x06, 4);
	put_u16(file + bitm_data_at + 0x0e, 1u << 4);
	put_u32(file + bitm_data_at + 0x1c, 16);
	memset(file + pixels_at, 0xff, 16);
	put_u16(file + eqip_at + offsetof(struct equipment_definition, equipment.powerup_type), 1);
	put_u32(file + model_at + offsetof(struct model, markers), 1);
	put_u32(file + model_at + offsetof(struct model, markers) + 4, TAG_BASE + marker_at);
	memcpy(file + marker_at, "weapon", 6);
	put_u32(file + marker_at + 52, 1);
	put_u32(file + marker_at + 56, TAG_BASE + marker_inst_at);
	put_f32(file + marker_inst_at + 4, 0.25f);
	put_f32(file + marker_inst_at + 12, 0.35f);
	put_u32(file + font_at + 0x30, 1);
	put_u32(file + font_at + 0x34, TAG_BASE + font_table_at);
	put_u32(file + font_table_at, 256);
	put_u32(file + font_table_at + 4, TAG_BASE + font_index_at);
	memset(file + font_index_at, 0xff, 512);
	put_u16(file + font_index_at + 0x50u * 2u, 0);
	put_u32(file + font_at + 0x7c, 1);
	put_u32(file + font_at + 0x80, TAG_BASE + font_char_at);
	put_u16(file + font_char_at + 4, 2);
	put_u16(file + font_char_at + 6, 2);
	put_u32(file + font_at + 0x88 + 12, TAG_BASE + font_pix_at);
	memset(file + font_pix_at, 0xff, 4);
	if (wii_tag_cache_mount(memory_read, file, sizeof(file), 0, sizeof(file)) != 0)
		return -1;
	wii_tag_cache_player(&x, &y, &z);
	if (x < 8.9f || x > 9.1f || y < 7.9f || y > 8.1f || z < 6.9f || z > 7.1f)
		return -1;
	if (wii_tag_cache_camera_height() < 0.61f || wii_tag_cache_camera_height() > 0.63f)
		return -1;
	if (wii_tag_cache_weapon_count() != 1 || wii_tag_cache_encounter_count() != 1)
		return -1;
	if (wii_tag_cache_location_count() != 1 || wii_tag_cache_magazine_rounds() != 36)
		return -1;
	if (wii_tag_cache_idle_rotation_w() < 0.9f)
		return -1;
	if (!wii_collision_ready() || !wii_collision_hits_floor())
		return -1;
	if (wii_tag_cache_projectile_index() == -1 || wii_tag_cache_grenade_count() < 1)
		return -1;
	if (wii_tag_cache_damage_index() == -1)
		return -1;
	if (!wii_collision_model_hits(wii_tag_cache_get((long)FCC('c', 'o', 'l', 'l'), (long)datum[12])))
		return -1;
	wii_hud_bind(0x803A6000u + font_at);
	if (!wii_effect_bitmap_ok() || effect_new_from_object((long)datum[14], NONE, NONE, NONE, 1.f, 1.f, NULL, NULL) == NONE)
		return -1;
	if (!wii_tag_cache_get((long)FCC('b', 'i', 'p', 'd'), (long)datum[2]))
		return -1;
	if (wii_tag_cache_group((long)datum[5]) != (long)FCC('w', 'e', 'a', 'p'))
		return -1;
	return 0;
}
