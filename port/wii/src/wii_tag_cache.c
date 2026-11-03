/*
 * Fix up the tags a tick needs from the map file. The 23 MiB Xbox cache
 * stays on the card. This copies a working set (at most 1 MiB, inside the
 * 8 MiB resident cap) and byte-swaps it. Structure BSPs are not copied.
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
#include "wii_tag_cache.h"

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
	WII_TAG_ARENA = 1u << 20,
	WII_TAG_CAP_PLAYERS = 16,
	WII_TAG_CAP_WEAPONS = 4,
	WII_TAG_CAP_ENCOUNTERS = 8,
	WII_TAG_CAP_SQUADS = 4,
	WII_TAG_CAP_NODES = 64,
	WII_TAG_CAP_SEATS = 8,
	WII_TAG_CAP_CLASSES = 2,
	WII_TAG_CAP_TYPES = 2
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
		}
	}
	slots[slot].base = model;
	return 0;
}

static int load_animation(int slot)
{
	uint8_t raw[sizeof(struct animation_graph)];
	struct animation_graph *graph;
	struct animation_graph_unit_seat *seats;
	struct animation *animation;
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
	animation = bump(sizeof(*animation));
	if (!graph || !animation || (seat_count && !seats))
		return -1;
	graph->unit_seats.count = seat_count;
	graph->unit_seats.address = seats;
	graph->animations.count = 1;
	graph->animations.address = animation;
	animation->node_count = 1;
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
	ref_clear(&biped->object.collision_model);
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
	ref_clear(&weapon->object.model);
	ref_clear(&weapon->object.animation_graph);
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
	return load_biped(find_datum((uint32_t)info->player_unit.index));
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
				}
				encounters[index].squads.count = squad_count;
				encounters[index].squads.address = squads;
			}
			scenario->ai_encounters.count = count;
			scenario->ai_encounters.address = encounters;
			encounter_published = count;
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
	uint8_t file[0x4000];
	uint32_t cursor;
	uint32_t instances;
	uint32_t scenario_at;
	uint32_t player_at;
	uint32_t weapon_at;
	uint32_t palette_at;
	uint32_t encounter_at;
	uint32_t squad_at;
	uint32_t globals_at;
	uint32_t info_at;
	uint32_t fall_at;
	uint32_t biped_at;
	uint32_t model_at;
	uint32_t node_at;
	uint32_t graph_at;
	uint32_t seat_at;
	uint32_t class_at;
	uint32_t type_at;
	uint32_t anim_at;
	uint32_t weapon_def_at;
	uint32_t datum[6];
	float x;
	float y;
	float z;
	int index;

	memset(file, 0, sizeof(file));
	cursor = 0x40 + 6u * 0x20u;
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
	graph_at = cursor;
	cursor += sizeof(struct animation_graph);
	seat_at = cursor;
	cursor += sizeof(struct animation_graph_unit_seat);
	class_at = cursor;
	cursor += sizeof(struct animation_graph_weapon_class);
	type_at = cursor;
	cursor += sizeof(struct animation_graph_weapon_type);
	anim_at = cursor;
	cursor += sizeof(struct animation);
	weapon_def_at = cursor;
	cursor += 64u;
	if (cursor > sizeof(file))
		return -1;
	for (index = 0; index < 6; index++)
		datum[index] = 0x12340000u | (uint32_t)index;
	put_u32(file, TAG_BASE + instances);
	put_u32(file + 4, datum[0]);
	put_u32(file + 0x0c, 6);
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
		} rows[6] = {
			{ scenario_at, 's', 'c', 'n', 'r', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ globals_at, 'm', 'a', 't', 'g', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ biped_at, 'b', 'i', 'p', 'd', 'u', 'n', 'i', 't', 'o', 'b', 'j', 'e' },
			{ model_at, 'm', 'o', 'd', 'e', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ graph_at, 'a', 'n', 't', 'r', 0, 0, 0, 0, 0, 0, 0, 0 },
			{ weapon_def_at, 'w', 'e', 'a', 'p', 'i', 't', 'e', 'm', 'o', 'b', 'j', 'e' }
		};

		for (index = 0; index < 6; index++) {
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
	memcpy(file + squad_at, "squad", 5);
	put_u32(file + squad_at + offsetof(struct squad_definition, flags), 0);
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
	put_u32(file + biped_at + offsetof(struct biped_definition, biped.flags), 1u << _biped_uses_player_physics_bit);
	put_f32(file + biped_at + offsetof(struct biped_definition, biped.standing_camera_height), 0.62f);
	put_f32(file + biped_at + offsetof(struct biped_definition, biped.crouching_camera_height), 0.4f);
	put_u32(file + model_at + offsetof(struct model, nodes), 1);
	put_u32(file + model_at + offsetof(struct model, nodes) + 4, TAG_BASE + node_at);
	put_u16(file + node_at + offsetof(struct model_node, next_sibling_node_index), 0xffffu);
	put_u16(file + node_at + offsetof(struct model_node, first_child_node_index), 0xffffu);
	put_u16(file + node_at + offsetof(struct model_node, parent_node_index), 0xffffu);
	put_u32(file + graph_at + offsetof(struct animation_graph, unit_seats), 1);
	put_u32(file + graph_at + offsetof(struct animation_graph, unit_seats) + 4, TAG_BASE + seat_at);
	memcpy(file + seat_at, "stand", 5);
	put_u32(file + seat_at + offsetof(struct animation_graph_unit_seat, weapon_classes), 1);
	put_u32(file + seat_at + offsetof(struct animation_graph_unit_seat, weapon_classes) + 4, TAG_BASE + class_at);
	put_u32(file + class_at + offsetof(struct animation_graph_weapon_class, weapon_types), 1);
	put_u32(file + class_at + offsetof(struct animation_graph_weapon_class, weapon_types) + 4, TAG_BASE + type_at);
	put_u16(file + weapon_def_at, (uint16_t)_object_type_weapon);
	put_f32(file + weapon_def_at + offsetof(struct weapon_definition, object.bounding_radius), 0.2f);
	(void)anim_at;
	if (wii_tag_cache_mount(memory_read, file, sizeof(file), 0, sizeof(file)) != 0)
		return -1;
	wii_tag_cache_player(&x, &y, &z);
	if (x < 8.9f || x > 9.1f || y < 7.9f || y > 8.1f || z < 6.9f || z > 7.1f)
		return -1;
	if (wii_tag_cache_camera_height() < 0.61f || wii_tag_cache_camera_height() > 0.63f)
		return -1;
	if (wii_tag_cache_weapon_count() != 1 || wii_tag_cache_encounter_count() != 1)
		return -1;
	if (!wii_tag_cache_get((long)FCC('b', 'i', 'p', 'd'), (long)datum[2]))
		return -1;
	if (wii_tag_cache_group((long)datum[5]) != (long)FCC('w', 'e', 'a', 'p'))
		return -1;
	return 0;
}
