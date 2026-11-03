/*
 * Copy one structure BSP's collision BSP into a MEM2 arena and point
 * global_collision_bsp at it. A tree that does not fit is left unloaded.
 */
#include "cseries/cseries.h"
#include "game/game_globals.h"
#include "objects/damage.h"
#include "objects/damage_effect_definitions.h"
#include "objects/objects.h"
#include "physics/collision_bsp.h"
#include "physics/collision_bsp_definitions.h"
#include "physics/collision_model_definitions.h"
#include "physics/collision_models.h"
#include "physics/collisions.h"
#include "wii_collision.h"
#include "wii_tag_cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef GEKKO
#include "wii_mem.h"
#endif

enum {
	WII_COLLIDE_ARENA = 4 * 1024 * 1024,
	WII_COLLIDE_MAX_NODES = 32768,
	WII_COLLIDE_MAX_PLANES = 16384,
	WII_COLLIDE_MAX_LEAVES = 16384,
	WII_COLLIDE_MAX_REFS = 32768,
	WII_COLLIDE_MAX_BSP2D = 32768,
	WII_COLLIDE_MAX_SURFACES = 32768,
	WII_COLLIDE_MAX_EDGES = 65536,
	WII_COLLIDE_MAX_VERTICES = 32768,
	WII_COLLIDE_MAX_MODEL_NODES = 32,
	WII_COLLIDE_MAX_NODE_BSPS = 2
};

struct collision_leaf {
	word flags;
	short bsp2d_reference_count;
	long first_bsp2d_reference_index;
};

struct bsp2d_reference {
	long plane_designator;
	long root_index;
};

static uint8_t *arena;
static uint32_t arena_used;
static struct collision_bsp resident;
static struct collision_bsp empty;
struct collision_bsp *global_collision_bsp = &empty;
static int ready;

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

static void *bump(uint32_t size)
{
	uint32_t aligned;
	void *block;

	aligned = (size + 7u) & ~7u;
	if (!arena || arena_used + aligned > WII_COLLIDE_ARENA)
		return NULL;
	block = arena + arena_used;
	memset(block, 0, aligned);
	arena_used += aligned;
	return block;
}

static int ensure_arena(void)
{
	if (arena)
		return 0;
#ifdef GEKKO
	arena = wii_mem2_alloc(WII_COLLIDE_ARENA);
	if (!arena)
		arena = malloc(WII_COLLIDE_ARENA);
#else
	arena = malloc(WII_COLLIDE_ARENA);
#endif
	if (!arena)
		return -1;
	arena_used = 0;
	return 0;
}

static int pull_block(uint32_t va, int *count, uint32_t *address)
{
	uint8_t raw[8];

	if (wii_tag_cache_pull(va, raw, 8) != 0)
		return -1;
	*count = read_les32(raw);
	*address = read_le32(raw + 4);
	if (*count < 0)
		return -1;
	return 0;
}

static int load_u32_block(uint32_t block_va, int elem, int max_count, struct tag_block *block)
{
	int count;
	uint32_t address;
	uint8_t *native;
	int index;
	int byte;

	if (pull_block(block_va, &count, &address) != 0)
		return -1;
	if (count > max_count)
		return -1;
	native = count ? bump((uint32_t)count * (uint32_t)elem) : NULL;
	if (count && !native)
		return -1;
	for (index = 0; index < count; index++) {
		uint8_t raw[64];

		if (elem > (int)sizeof(raw) || wii_tag_cache_pull(address + (uint32_t)index * (uint32_t)elem, raw, (uint32_t)elem) != 0)
			return -1;
		memcpy(native + index * elem, raw, (size_t)elem);
		for (byte = 0; byte + 4 <= elem; byte += 4) {
			uint32_t value = read_le32(raw + byte);

			memcpy(native + index * elem + byte, &value, 4);
		}
	}
	block->count = count;
	block->address = native;
	block->definition = NULL;
	return 0;
}

static int load_planes(uint32_t block_va, struct tag_block *block)
{
	int count;
	uint32_t address;
	real_plane3d *planes;
	int index;

	if (pull_block(block_va, &count, &address) != 0 || count > WII_COLLIDE_MAX_PLANES)
		return -1;
	planes = count ? bump((uint32_t)count * sizeof(*planes)) : NULL;
	if (count && !planes)
		return -1;
	for (index = 0; index < count; index++) {
		uint8_t raw[16];

		if (wii_tag_cache_pull(address + (uint32_t)index * 16u, raw, 16) != 0)
			return -1;
		planes[index].n.i = read_lef(raw);
		planes[index].n.j = read_lef(raw + 4);
		planes[index].n.k = read_lef(raw + 8);
		planes[index].d = read_lef(raw + 12);
	}
	block->count = count;
	block->address = planes;
	return 0;
}

static int load_vertices(uint32_t block_va, struct tag_block *block)
{
	int count;
	uint32_t address;
	struct collision_vertex *vertices;
	int index;

	if (pull_block(block_va, &count, &address) != 0 || count > WII_COLLIDE_MAX_VERTICES)
		return -1;
	vertices = count ? bump((uint32_t)count * sizeof(*vertices)) : NULL;
	if (count && !vertices)
		return -1;
	for (index = 0; index < count; index++) {
		uint8_t raw[16];

		if (wii_tag_cache_pull(address + (uint32_t)index * 16u, raw, 16) != 0)
			return -1;
		vertices[index].point.x = read_lef(raw);
		vertices[index].point.y = read_lef(raw + 4);
		vertices[index].point.z = read_lef(raw + 8);
		vertices[index].first_edge_index = read_les32(raw + 12);
	}
	block->count = count;
	block->address = vertices;
	return 0;
}

static int load_leaves(uint32_t block_va, struct tag_block *block)
{
	int count;
	uint32_t address;
	struct collision_leaf *leaves;
	int index;

	if (pull_block(block_va, &count, &address) != 0 || count > WII_COLLIDE_MAX_LEAVES)
		return -1;
	leaves = count ? bump((uint32_t)count * sizeof(*leaves)) : NULL;
	if (count && !leaves)
		return -1;
	for (index = 0; index < count; index++) {
		uint8_t raw[8];

		if (wii_tag_cache_pull(address + (uint32_t)index * 8u, raw, 8) != 0)
			return -1;
		leaves[index].flags = (word)read_les16(raw);
		leaves[index].bsp2d_reference_count = read_les16(raw + 2);
		leaves[index].first_bsp2d_reference_index = read_les32(raw + 4);
	}
	block->count = count;
	block->address = leaves;
	return 0;
}

static int load_bsp2d(uint32_t block_va, struct tag_block *block)
{
	int count;
	uint32_t address;
	struct bsp2d_node *nodes;
	int index;

	if (pull_block(block_va, &count, &address) != 0 || count > WII_COLLIDE_MAX_BSP2D)
		return -1;
	nodes = count ? bump((uint32_t)count * sizeof(*nodes)) : NULL;
	if (count && !nodes)
		return -1;
	for (index = 0; index < count; index++) {
		uint8_t raw[20];

		if (wii_tag_cache_pull(address + (uint32_t)index * 20u, raw, 20) != 0)
			return -1;
		nodes[index].plane.n.i = read_lef(raw);
		nodes[index].plane.n.j = read_lef(raw + 4);
		nodes[index].plane.d = read_lef(raw + 8);
		nodes[index].child_indices[0] = read_les32(raw + 12);
		nodes[index].child_indices[1] = read_les32(raw + 16);
	}
	block->count = count;
	block->address = nodes;
	return 0;
}

static int load_surfaces(uint32_t block_va, struct tag_block *block)
{
	int count;
	uint32_t address;
	struct collision_surface *surfaces;
	int index;

	if (pull_block(block_va, &count, &address) != 0 || count > WII_COLLIDE_MAX_SURFACES)
		return -1;
	surfaces = count ? bump((uint32_t)count * sizeof(*surfaces)) : NULL;
	if (count && !surfaces)
		return -1;
	for (index = 0; index < count; index++) {
		uint8_t raw[12];

		if (wii_tag_cache_pull(address + (uint32_t)index * 12u, raw, 12) != 0)
			return -1;
		surfaces[index].plane_designator = read_les32(raw);
		surfaces[index].first_edge_index = read_les32(raw + 4);
		surfaces[index].flags = raw[8];
		surfaces[index].breakable_surface_index = raw[9];
		surfaces[index].material_index = read_les16(raw + 10);
	}
	block->count = count;
	block->address = surfaces;
	return 0;
}

static int read_bsp(uint32_t bsp_va, struct collision_bsp *bsp)
{
	uint32_t mark = arena_used;

	memset(bsp, 0, sizeof(*bsp));
	if (load_u32_block(bsp_va + offsetof(struct collision_bsp, bsp3d.nodes), (int)sizeof(struct bsp3d_node), WII_COLLIDE_MAX_NODES, &bsp->bsp3d.nodes) != 0
		|| load_planes(bsp_va + offsetof(struct collision_bsp, bsp3d.planes), &bsp->bsp3d.planes) != 0
		|| load_leaves(bsp_va + offsetof(struct collision_bsp, leaves), &bsp->leaves) != 0
		|| load_u32_block(bsp_va + offsetof(struct collision_bsp, bsp2d_references), (int)sizeof(struct bsp2d_reference), WII_COLLIDE_MAX_REFS, &bsp->bsp2d_references) != 0
		|| load_bsp2d(bsp_va + offsetof(struct collision_bsp, bsp2d.nodes), &bsp->bsp2d.nodes) != 0
		|| load_surfaces(bsp_va + offsetof(struct collision_bsp, surfaces), &bsp->surfaces) != 0
		|| load_u32_block(bsp_va + offsetof(struct collision_bsp, edges), (int)sizeof(struct collision_edge), WII_COLLIDE_MAX_EDGES, &bsp->edges) != 0
		|| load_vertices(bsp_va + offsetof(struct collision_bsp, vertices), &bsp->vertices) != 0) {
		arena_used = mark;
		return -1;
	}
	return 0;
}

static void ref_clear(struct tag_reference *reference)
{
	reference->group_tag = 0;
	reference->name = NULL;
	reference->name_length = 0;
	reference->index = NONE;
}

void *wii_collision_load_model(uint32_t coll_va)
{
	struct collision_model *model;
	uint8_t raw[8];
	int node_count;
	uint32_t node_va;
	struct collision_node *nodes;
	int node_index;

	if (ensure_arena() != 0)
		return NULL;
	model = bump(sizeof(*model));
	if (!model)
		return NULL;
	if (wii_tag_cache_pull(coll_va + offsetof(struct collision_model, resistance.maximum_body_vitality), raw, 4) == 0)
		model->resistance.maximum_body_vitality = read_lef(raw);
	if (wii_tag_cache_pull(coll_va + offsetof(struct collision_model, resistance.maximum_shield_vitality), raw, 4) == 0)
		model->resistance.maximum_shield_vitality = read_lef(raw);
	ref_clear(&model->resistance.localized_damage_effect);
	ref_clear(&model->resistance.area_damage_effect);
	ref_clear(&model->resistance.body_damaged_effect);
	ref_clear(&model->resistance.body_depleted_effect);
	ref_clear(&model->resistance.body_destroyed_effect);
	ref_clear(&model->resistance.shield_damaged_effect);
	ref_clear(&model->resistance.shield_depleted_effect);
	ref_clear(&model->resistance.shield_recharging_effect);
	if (pull_block(coll_va + offsetof(struct collision_model, resistance.materials), &node_count, &node_va) == 0 && node_count > 0) {
		struct damage_resistance_material *materials;
		int count = node_count > 4 ? 4 : node_count;

		materials = bump((uint32_t)count * sizeof(*materials));
		if (materials) {
			for (node_index = 0; node_index < count; node_index++) {
				uint8_t material_raw[0x48];

				if (wii_tag_cache_pull(node_va + (uint32_t)node_index * sizeof(*materials), material_raw, sizeof(material_raw)) != 0)
					break;
				materials[node_index].shield_damage_multiplier = read_lef(material_raw + offsetof(struct damage_resistance_material, shield_damage_multiplier));
				materials[node_index].body_damage_multiplier = read_lef(material_raw + offsetof(struct damage_resistance_material, body_damage_multiplier));
				materials[node_index].material_type = read_les16(material_raw + offsetof(struct damage_resistance_material, material_type));
			}
			model->resistance.materials.count = count;
			model->resistance.materials.address = materials;
		}
	}
	model->resistance.indirect_damage_material_index = 0;
	if (pull_block(coll_va + offsetof(struct collision_model, nodes), &node_count, &node_va) != 0 || node_count < 1)
		return model;
	if (node_count > WII_COLLIDE_MAX_MODEL_NODES)
		node_count = WII_COLLIDE_MAX_MODEL_NODES;
	nodes = bump((uint32_t)node_count * sizeof(*nodes));
	if (!nodes)
		return model;
	for (node_index = 0; node_index < node_count; node_index++) {
		uint8_t node_raw[sizeof(struct collision_node)];
		int bsp_count;
		uint32_t bsp_va;
		struct collision_bsp *bsps;
		int bsp_index;

		if (wii_tag_cache_pull(node_va + (uint32_t)node_index * sizeof(node_raw), node_raw, sizeof(node_raw)) != 0)
			break;
		memcpy(nodes[node_index].name, node_raw, 32);
		nodes[node_index].region_index = read_les16(node_raw + offsetof(struct collision_node, region_index));
		nodes[node_index].parent_node_index = read_les16(node_raw + offsetof(struct collision_node, parent_node_index));
		nodes[node_index].next_sibling_node_index = read_les16(node_raw + offsetof(struct collision_node, next_sibling_node_index));
		nodes[node_index].first_child_node_index = read_les16(node_raw + offsetof(struct collision_node, first_child_node_index));
		nodes[node_index].runtime_damage_part = read_les16(node_raw + offsetof(struct collision_node, runtime_damage_part));
		bsp_count = read_les32(node_raw + offsetof(struct collision_node, bsps));
		bsp_va = read_le32(node_raw + offsetof(struct collision_node, bsps) + 4);
		if (bsp_count < 0)
			bsp_count = 0;
		if (bsp_count > WII_COLLIDE_MAX_NODE_BSPS)
			bsp_count = WII_COLLIDE_MAX_NODE_BSPS;
		bsps = bsp_count ? bump((uint32_t)bsp_count * sizeof(*bsps)) : NULL;
		if (bsp_count && !bsps)
			bsp_count = 0;
		for (bsp_index = 0; bsp_index < bsp_count; bsp_index++) {
			if (read_bsp(bsp_va + (uint32_t)bsp_index * sizeof(*bsps), &bsps[bsp_index]) != 0)
				memset(&bsps[bsp_index], 0, sizeof(bsps[bsp_index]));
		}
		nodes[node_index].bsps.count = bsp_count;
		nodes[node_index].bsps.address = bsps;
	}
	model->nodes.count = node_count;
	model->nodes.address = nodes;
	return model;
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

void wii_collision_write_floor(uint8_t *file, uint32_t sbsp_at, uint32_t collide_at, uint32_t plane_at, uint32_t node_at, uint32_t leaf_at, uint32_t ref_at, uint32_t surface_at)
{
	put_u32(file + sbsp_at + 176u, 1);
	put_u32(file + sbsp_at + 180u, 0x803A6000u + collide_at);
	put_u32(file + collide_at + offsetof(struct collision_bsp, bsp3d.nodes), 1);
	put_u32(file + collide_at + offsetof(struct collision_bsp, bsp3d.nodes) + 4, 0x803A6000u + node_at);
	put_u32(file + collide_at + offsetof(struct collision_bsp, bsp3d.planes), 1);
	put_u32(file + collide_at + offsetof(struct collision_bsp, bsp3d.planes) + 4, 0x803A6000u + plane_at);
	put_u32(file + collide_at + offsetof(struct collision_bsp, leaves), 1);
	put_u32(file + collide_at + offsetof(struct collision_bsp, leaves) + 4, 0x803A6000u + leaf_at);
	put_u32(file + collide_at + offsetof(struct collision_bsp, bsp2d_references), 1);
	put_u32(file + collide_at + offsetof(struct collision_bsp, bsp2d_references) + 4, 0x803A6000u + ref_at);
	put_u32(file + collide_at + offsetof(struct collision_bsp, surfaces), 1);
	put_u32(file + collide_at + offsetof(struct collision_bsp, surfaces) + 4, 0x803A6000u + surface_at);
	put_f32(file + plane_at + 8, 1.f);
	put_u32(file + node_at + 4, 0xffffffffu);
	put_u32(file + node_at + 8, 0x80000000u);
	put_u16(file + leaf_at + 2, 1);
	put_u32(file + ref_at + 4, 0x80000000u);
}

int wii_collision_load(uint32_t sbsp_va)
{
	int count;
	uint32_t address;

	if (ready)
		return 0;
	if (ensure_arena() != 0)
		return -1;
	/* collision_bsp tag_block in struct structure_bsp (OpenCE build-337). */
	if (pull_block(sbsp_va + 176u, &count, &address) != 0)
		return -1;
	if (count < 1)
		return -1;
	if (read_bsp(address, &resident) != 0)
		return -1;
	if (resident.bsp3d.nodes.count < 1)
		return -1;
	global_collision_bsp = &resident;
	ready = 1;
	return 0;
}

void wii_collision_reset(void)
{
	ready = 0;
	global_collision_bsp = &empty;
	arena_used = 0;
#ifndef GEKKO
	free(arena);
	arena = NULL;
#endif
}

int wii_collision_ready(void)
{
	return ready;
}

struct collision_bsp *global_collision_bsp_get(void)
{
	return global_collision_bsp ? global_collision_bsp : &empty;
}

struct bsp3d *global_bsp3d_get(void)
{
	return &global_collision_bsp_get()->bsp3d;
}

int wii_collision_hits_floor(void)
{
	struct collision_result result;
	real_point3d point;
	real_vector3d vector;
	unsigned long flags;

	if (!ready)
		return 0;
	point.x = 0.f;
	point.y = 0.f;
	point.z = 2.f;
	vector.i = 0.f;
	vector.j = 0.f;
	vector.k = -4.f;
	flags = 1u << _collision_test_structure_bit;
	memset(&result, 0, sizeof(result));
	if (!collision_test_vector(flags, &point, &vector, NONE, &result))
		return 0;
	return result.type == _collision_result_structure && result.t > 0.2f && result.t < 0.8f;
}

int wii_collision_model_hits(void *model_pointer)
{
	struct collision_model *model = model_pointer;
	struct collision_model_instance instance;
	struct real_matrix4x3 matrix;
	byte permutations[8];
	struct collision_model_test_vector_result result;
	real_point3d point;
	real_vector3d vector;

	if (!model || model->nodes.count < 1)
		return 0;
	memset(&matrix, 0, sizeof(matrix));
	matrix.scale = 1.f;
	matrix.forward.i = 1.f;
	matrix.left.j = 1.f;
	matrix.up.k = 1.f;
	memset(permutations, 0, sizeof(permutations));
	memset(&instance, 0, sizeof(instance));
	instance.model = model;
	instance.region_permutation_indices = permutations;
	instance.matrices = &matrix;
	instance.node_count = 1;
	point.x = 0.f;
	point.y = 0.f;
	point.z = 2.f;
	vector.i = 0.f;
	vector.j = 0.f;
	vector.k = -4.f;
	memset(&result, 0, sizeof(result));
	result.bsp_result.t = 1.f;
	return collision_model_test_vector(&instance,
		(1u << _collision_test_front_facing_surfaces_bit) | (1u << _collision_test_back_facing_surfaces_bit),
		&point, &vector, &result)
		&& result.bsp_result.t > 0.2f && result.bsp_result.t < 0.8f;
}

int wii_damage_hurts(long object_index, long damage_definition)
{
	struct object_datum *object;
	float before;
	struct damage_data damage;
	real_vector3d down;

	if (object_index == NONE || damage_definition == NONE)
		return 0;
	object = object_get(object_index);
	if (!object)
		return 0;
	before = object->object.body_vitality + object->object.shield_vitality;
	damage_data_new(&damage, damage_definition);
	damage.scale = 1.f;
	damage.multiplier = 1.f;
	down.i = 0.f;
	down.j = 0.f;
	down.k = -1.f;
	object_cause_damage(&damage, object_index, 0, NONE, 0, &down);
	return object->object.body_vitality + object->object.shield_vitality < before;
}
