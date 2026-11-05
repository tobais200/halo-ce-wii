/*
 * Skin one model part. Compressed vertices (32 bytes, the Xbox layout)
 * are blended by node weight across the two node matrices, then placed
 * on the unit. At most 96 vertices and 32 triangles are kept.
 */
#include "cseries/cseries.h"
#include "models/model_animation_definitions.h"
#include "models/model_definitions.h"
#include "objects/objects.h"
#include "units/biped_definitions.h"
#include "items/weapon_definitions.h"
#include "scenario/scenario_definitions.h"
#include "wii_mesh.h"
#include "wii_tag_cache.h"
#include "wii_tex.h"

#include <math.h>
#include <string.h>

enum { WII_MESH_VERTS = 96, WII_MESH_TRIS = 32 };

struct wii_mat {
	float m[3][3];
	float x, y, z;
};

static float tris[WII_MESH_TRIS][9];
static int tri_count;
static int weapon_from = -1;
static int skinned;
static int animated;
static int overlay_used;
static int replacement_used;
static int anim_tick;
static int anim_frame;
static int third_person;
static int hand_placed;
static int marker_used;
static int shader_sampled;
static int shader_count;
static int bitmap_count;
static uint8_t shader_pixels[32 * 32 * 4];
static uint16_t shader_w;
static uint16_t shader_h;
static float pose_x;
static float pose_z;
static float weapon_z;

static void quat_axes(float i, float j, float k, float w, float m[3][3])
{
	float n = i * i + j * j + k * k + w * w;

	if (n < 0.0001f) {
		memset(m, 0, sizeof(float) * 9);
		m[0][0] = m[1][1] = m[2][2] = 1.f;
		return;
	}
	n = 1.f / sqrtf(n);
	i *= n;
	j *= n;
	k *= n;
	w *= n;
	m[0][0] = 1.f - 2.f * (j * j + k * k);
	m[0][1] = 2.f * (i * j - k * w);
	m[0][2] = 2.f * (i * k + j * w);
	m[1][0] = 2.f * (i * j + k * w);
	m[1][1] = 1.f - 2.f * (i * i + k * k);
	m[1][2] = 2.f * (j * k - i * w);
	m[2][0] = 2.f * (i * k - j * w);
	m[2][1] = 2.f * (j * k + i * w);
	m[2][2] = 1.f - 2.f * (i * i + j * j);
}

static void mul_mat(struct wii_mat const *a, struct wii_mat const *b, struct wii_mat *out)
{
	int row, col;

	for (row = 0; row < 3; row++) {
		for (col = 0; col < 3; col++)
			out->m[row][col] = a->m[row][0] * b->m[0][col] + a->m[row][1] * b->m[1][col] + a->m[row][2] * b->m[2][col];
	}
	out->x = a->m[0][0] * b->x + a->m[0][1] * b->y + a->m[0][2] * b->z + a->x;
	out->y = a->m[1][0] * b->x + a->m[1][1] * b->y + a->m[1][2] * b->z + a->y;
	out->z = a->m[2][0] * b->x + a->m[2][1] * b->y + a->m[2][2] * b->z + a->z;
}

static void transform_point(struct wii_mat const *a, float x, float y, float z, float *ox, float *oy, float *oz)
{
	*ox = a->m[0][0] * x + a->m[0][1] * y + a->m[0][2] * z + a->x;
	*oy = a->m[1][0] * x + a->m[1][1] * y + a->m[1][2] * z + a->y;
	*oz = a->m[2][0] * x + a->m[2][1] * y + a->m[2][2] * z + a->z;
}

static uint32_t read_u32(uint8_t const *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int32_t read_s32(uint8_t const *p)
{
	return (int32_t)read_u32(p);
}

static uint16_t read_u16(uint8_t const *p)
{
	return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static int16_t read_s16(uint8_t const *p)
{
	return (int16_t)read_u16(p);
}

static float read_f32(uint8_t const *p)
{
	float value;
	uint32_t bits = read_u32(p);

	memcpy(&value, &bits, 4);
	return value;
}

static int node_matrices(struct model *model, struct wii_mat *nodes, int cap)
{
	struct model_node *source;
	int count;
	int index;

	if (!model || model->nodes.count < 1 || !model->nodes.address)
		return 0;
	source = model->nodes.address;
	count = model->nodes.count;
	if (count > cap)
		count = cap;
	for (index = 0; index < count; index++) {
		struct wii_mat local;
		int parent = source[index].parent_node_index;

		memset(&local, 0, sizeof(local));
		quat_axes(source[index].default_rotation.v.i, source[index].default_rotation.v.j,
			source[index].default_rotation.v.k, source[index].default_rotation.w, local.m);
		local.x = source[index].default_translation.x;
		local.y = source[index].default_translation.y;
		local.z = source[index].default_translation.z;
		if (parent >= 0 && parent < index)
			mul_mat(&nodes[parent], &local, &nodes[index]);
		else
			nodes[index] = local;
	}
	return count;
}

static int skin_part(uint32_t part_va, struct wii_mat const *nodes, int node_count, float yaw, float ox, float oy, float oz)
{
	uint8_t part[0x68];
	int vcount;
	uint32_t vaddr;
	int tcount;
	uint32_t taddr;
	uint8_t indices[WII_MESH_TRIS * 6];
	int index;
	float cy = cosf(yaw);
	float sy = sinf(yaw);

	if (wii_tag_cache_pull(part_va, part, sizeof(part)) != 0)
		return 0;
	vcount = read_s32(part + 0x28);
	vaddr = read_u32(part + 0x2c);
	tcount = read_s32(part + 0x30);
	taddr = read_u32(part + 0x34);
	if (vcount < 3 || tcount < 3)
		return 0;
	if (tcount > WII_MESH_TRIS * 3)
		tcount = WII_MESH_TRIS * 3;
	if (wii_tag_cache_pull(taddr, indices, (uint32_t)tcount * 2u) != 0)
		return 0;
	for (index = 0; index + 2 < tcount && tri_count < WII_MESH_TRIS; index += 3) {
		int corner;
		float local[3][3];
		int ok = 1;

		for (corner = 0; corner < 3; corner++) {
			uint16_t vertex = read_u16(indices + (index + corner) * 2);
			uint8_t raw[0x20];
			int node0;
			int node1;
			float weight;
			float px, py, pz, qx, qy, qz;

			if (vertex >= (uint16_t)vcount || vertex >= WII_MESH_VERTS
				|| wii_tag_cache_pull(vaddr + (uint32_t)vertex * 0x20u, raw, sizeof(raw)) != 0) {
				ok = 0;
				break;
			}
			node0 = raw[0x1c];
			node1 = raw[0x1d];
			if (node0 >= node_count)
				node0 /= 3;
			if (node1 >= node_count)
				node1 /= 3;
			if (node0 < 0 || node0 >= node_count)
				node0 = 0;
			if (node1 < 0 || node1 >= node_count)
				node1 = node0;
			weight = (float)read_s16(raw + 0x1e) / 32767.f;
			if (weight < 0.f)
				weight = 0.f;
			if (weight > 1.f)
				weight = 1.f;
			transform_point(&nodes[node0], read_f32(raw), read_f32(raw + 4), read_f32(raw + 8), &px, &py, &pz);
			transform_point(&nodes[node1], read_f32(raw), read_f32(raw + 4), read_f32(raw + 8), &qx, &qy, &qz);
			local[corner][0] = px * weight + qx * (1.f - weight);
			local[corner][1] = py * weight + qy * (1.f - weight);
			local[corner][2] = pz * weight + qz * (1.f - weight);
			if (fabsf(local[corner][0] - read_f32(raw)) > 0.2f
				|| fabsf(local[corner][1] - read_f32(raw + 4)) > 0.2f
				|| fabsf(local[corner][2] - read_f32(raw + 8)) > 0.2f)
				skinned = 1;
		}
		if (!ok)
			continue;
		for (corner = 0; corner < 3; corner++) {
			float x = ox + local[corner][0] * cy - local[corner][1] * sy;
			float y = oy + local[corner][0] * sy + local[corner][1] * cy;

			tris[tri_count][corner * 3] = x;
			tris[tri_count][corner * 3 + 1] = y;
			tris[tri_count][corner * 3 + 2] = oz + local[corner][2];
		}
		tri_count++;
	}
	return tri_count;
}

static int apply_animation(struct model *model, struct biped_definition *biped, struct wii_mat *nodes, int count)
{
	struct animation_graph *graph;
	struct animation *anim;
	struct real_orientation orients[32];
	int index;
	int limit;

	if (!biped || biped->object.animation_graph.index == NONE || count < 1)
		return 0;
	graph = tag_get(0x616e7472, biped->object.animation_graph.index);
	if (!graph || graph->animations.count < 1 || !graph->animations.address)
		return 0;
	anim = graph->animations.address;
	if (!anim || anim->node_count < 1)
		return 0;
	memset(orients, 0, sizeof(orients));
	anim_frame = anim_tick % 4;
	animation_get_node_orientations(model, anim,
		(short)(anim->frame_count > 1 ? anim_frame % anim->frame_count : 0), orients);
	if (graph->animations.count > 2) {
		struct animation *replacement = anim + 2;

		if (replacement->type == _animation_replacement) {
			int frame = replacement->frame_count > 1 ? anim_frame % replacement->frame_count : 0;

			replacement_animation_apply(replacement, (short)frame, orients);
			replacement_used = 1;
		}
	}
	if (graph->animations.count > 1) {
		struct animation *overlay = anim + 1;

		if (overlay->type == _animation_overlay) {
			int frame = overlay->frame_count > 1 ? anim_frame % overlay->frame_count : 0;

			overlay_animation_apply(overlay, (short)frame, orients);
			overlay_used = 1;
		}
	}
	limit = count < anim->node_count ? count : anim->node_count;
	if (limit > 32)
		limit = 32;
	for (index = 0; index < limit; index++) {
		struct wii_mat local;
		int parent = ((struct model_node *)model->nodes.address)[index].parent_node_index;
		float w = orients[index].rotation.w;
		float tx = orients[index].translation.x;
		float ty = orients[index].translation.y;
		float tz = orients[index].translation.z;

		if (w == 0.f && tx == 0.f && ty == 0.f && tz == 0.f)
			continue;
		memset(&local, 0, sizeof(local));
		quat_axes(orients[index].rotation.v.i, orients[index].rotation.v.j,
			orients[index].rotation.v.k, w == 0.f ? 1.f : w, local.m);
		local.x = ((struct model_node *)model->nodes.address)[index].default_translation.x + tx;
		local.y = ((struct model_node *)model->nodes.address)[index].default_translation.y + ty;
		local.z = ((struct model_node *)model->nodes.address)[index].default_translation.z + tz;
		if (parent >= 0 && parent < index)
			mul_mat(&nodes[parent], &local, &nodes[index]);
		else
			nodes[index] = local;
		animated = 1;
	}
	if (limit > 0) {
		pose_x = nodes[0].x;
		pose_z = nodes[0].z;
	}
	anim_tick++;
	return animated;
}

static void sample_shader(uint32_t model_va)
{
	uint8_t block[8];
	uint8_t ref[16];
	int count;
	uint32_t address;

	if (wii_tag_cache_pull(model_va + offsetof(struct model, shaders), block, 8) != 0)
		return;
	count = read_s32(block);
	address = read_u32(block + 4);
	if (count < 1 || wii_tag_cache_pull(address, ref, 16) != 0)
		return;
	shader_count = count > 2 ? 2 : count;
	shader_sampled = ref[0] != 0;
	{
		uint32_t bitmap_va = wii_tag_cache_slot_va((long)read_u32(ref + 12));
		uint8_t group[0x6c];
		uint8_t data[0x30];
		uint8_t pixels_header[20];
		uint8_t encoded[256];
		struct wii_rgba image;
		uint32_t pixels_size;
		uint32_t blob_va;

		if (!bitmap_va || wii_tag_cache_pull(bitmap_va, group, sizeof(group)) != 0)
			return;
		if (read_u32(group + 0x60) < 1 || wii_tag_cache_pull(read_u32(group + 0x64), data, sizeof(data)) != 0)
			return;
		bitmap_count = read_u32(group + 0x60) > 48 ? 48 : (int)read_u32(group + 0x60);
		pixels_size = read_u32(data + 0x1c);
		if (pixels_size < 1 || pixels_size > sizeof(encoded))
			pixels_size = sizeof(encoded);
		if (wii_tag_cache_pull(bitmap_va + 0x30, pixels_header, sizeof(pixels_header)) != 0)
			return;
		blob_va = read_u32(pixels_header + 12);
		if (wii_tag_cache_pull(blob_va + read_u32(data + 0x18), encoded, pixels_size) != 0)
			return;
		memset(&image, 0, sizeof(image));
		image.pixels = shader_pixels;
		image.cap = 32;
		if (wii_tex_decode(encoded, pixels_size, (int)read_u16(data + 0x0c), 1, read_u16(data + 4), read_u16(data + 6), &image) == 0) {
			shader_w = image.width;
			shader_h = image.height;
			shader_sampled = image.ok && shader_pixels[0] != 0;
		}
	}
}

static void skin_weapon(struct wii_mat const *nodes, int node_count, float yaw, float ox, float oy, float oz)
{
	struct scenario *scenario = wii_tag_cache_scenario();
	struct tag_reference *palette;
	struct weapon_definition *weapon;
	uint32_t model_va;
	uint8_t block[8];
	int before;

	if (!scenario || scenario->weapon_palette.count < 1 || !scenario->weapon_palette.address)
		return;
	palette = scenario->weapon_palette.address;
	weapon = tag_get(0x77656170, palette->index);
	(void)weapon;
	{
		uint8_t model_ref[16];
		uint32_t weapon_va = wii_tag_cache_slot_va(palette->index);
		long model_index;

		if (!weapon_va)
			return;
		model_index = NONE;
		if (wii_tag_cache_pull(weapon_va + offsetof(struct weapon_definition, object.model) + 12, model_ref, 4) == 0)
			model_index = (long)read_u32(model_ref);
		if (model_index != NONE)
			third_person = 1;
		else if (wii_tag_cache_pull(weapon_va + offsetof(struct weapon_definition, weapon.interface_definition.first_person_model) + 12, model_ref, 4) == 0)
			model_index = (long)read_u32(model_ref);
		if (model_index == NONE)
			return;
		model_va = wii_tag_cache_slot_va(model_index);
	}
	if (!model_va || wii_tag_cache_pull(model_va + offsetof(struct model, geometries), block, 8) != 0)
		return;
	if (read_s32(block) < 1 || wii_tag_cache_pull(read_u32(block + 4) + 0x24, block, 8) != 0)
		return;
	if (read_s32(block) < 1)
		return;
	{
		uint8_t marker_head[8];
		float mx = nodes[0].x;
		float my = nodes[0].y;
		float mz = nodes[0].z;

		if (wii_tag_cache_pull(model_va + offsetof(struct model, markers), marker_head, 8) == 0 && read_s32(marker_head) >= 1) {
			uint8_t instances[8];
			uint8_t instance[16];

			if (wii_tag_cache_pull(read_u32(marker_head + 4) + 52, instances, 8) == 0 && read_s32(instances) >= 1
				&& wii_tag_cache_pull(read_u32(instances + 4), instance, 16) == 0) {
				int node = instance[2];
				float lx = read_f32(instance + 4);
				float ly = read_f32(instance + 8);
				float lz = read_f32(instance + 12);
				struct wii_mat const *joint = &nodes[0];

				if (node >= 0 && node < node_count)
					joint = &nodes[node];
				mx = joint->m[0][0] * lx + joint->m[0][1] * ly + joint->m[0][2] * lz;
				my = joint->m[1][0] * lx + joint->m[1][1] * ly + joint->m[1][2] * lz;
				mz = joint->m[2][0] * lx + joint->m[2][1] * ly + joint->m[2][2] * lz;
				weapon_z = oz + joint->z + mz;
				marker_used = 1;
				third_person = 1;
			}
		}
		if (!marker_used)
			weapon_z = oz + mz;
		before = tri_count;
		skin_part(read_u32(block + 4), nodes, node_count, yaw, ox + mx, oy + my, oz + mz);
	}
	if (tri_count > before)
		hand_placed = 1;
	before = tri_count;
	skin_part(read_u32(block + 4), nodes, node_count, yaw,
		ox + cosf(yaw) * 0.35f, oy + sinf(yaw) * 0.35f, oz - 0.2f);
	if (tri_count > before && weapon_from < 0)
		weapon_from = before;
}

int wii_mesh_build(long unit_index)
{
	struct object_datum *object = object_get(unit_index);
	struct biped_definition *biped;
	struct model *model;
	struct wii_mat nodes[32];
	uint32_t model_va;
	uint8_t block[8];
	int geom_count;
	uint32_t geom_va;
	int part_count;
	uint32_t part_va;
	int node_count;
	float yaw;

	tri_count = 0;
	weapon_from = -1;
	skinned = 0;
	animated = 0;
	overlay_used = 0;
	replacement_used = 0;
	hand_placed = 0;
	marker_used = 0;
	shader_sampled = 0;
	shader_count = 0;
	bitmap_count = 0;
	third_person = 0;
	shader_w = 0;
	shader_h = 0;
	if (!object)
		return 0;
	biped = tag_get(0x62697064, object->definition_index);
	if (!biped || biped->object.model.index == NONE)
		return 0;
	model = tag_get(0x6d6f6465, biped->object.model.index);
	model_va = wii_tag_cache_slot_va(biped->object.model.index);
	node_count = node_matrices(model, nodes, 32);
	if (!model_va || node_count < 1)
		return 0;
	apply_animation(model, biped, nodes, node_count);
	sample_shader(model_va);
	if (wii_tag_cache_pull(model_va + offsetof(struct model, geometries), block, 8) != 0)
		return 0;
	geom_count = read_s32(block);
	geom_va = read_u32(block + 4);
	if (geom_count < 1 || wii_tag_cache_pull(geom_va + 0x24, block, 8) != 0)
		return 0;
	part_count = read_s32(block);
	part_va = read_u32(block + 4);
	if (part_count < 1)
		return 0;
	yaw = atan2f(object->object.forward.j, object->object.forward.i);
	skin_part(part_va, nodes, node_count, yaw,
		object->object.position.x, object->object.position.y, object->object.position.z);
	skin_weapon(nodes, node_count, yaw,
		object->object.position.x, object->object.position.y, object->object.position.z);
	return tri_count;
}

int wii_mesh_triangles(void)
{
	return tri_count;
}

int wii_mesh_triangle(int index, float *xyz)
{
	if (!xyz || index < 0 || index >= tri_count)
		return 0;
	memcpy(xyz, tris[index], sizeof(tris[index]));
	return 1;
}

int wii_mesh_skinned(void)
{
	return skinned;
}

int wii_mesh_animated(void)
{
	return animated;
}

int wii_mesh_weapon(void)
{
	return weapon_from >= 0;
}

int wii_mesh_shader(void)
{
	return shader_sampled;
}

int wii_mesh_overlay(void)
{
	return overlay_used;
}

int wii_mesh_replacement(void)
{
	return replacement_used;
}

int wii_mesh_marker(void)
{
	return marker_used;
}

int wii_mesh_frame(void)
{
	return anim_frame;
}

int wii_mesh_third_person(void)
{
	return third_person;
}

int wii_mesh_shader_count(void)
{
	return shader_count;
}

int wii_mesh_bitmap_count(void)
{
	return bitmap_count;
}

float wii_mesh_pose_x(void)
{
	return pose_x;
}

float wii_mesh_pose_z(void)
{
	return pose_z;
}

float wii_mesh_weapon_z(void)
{
	return weapon_z;
}

int wii_mesh_hand(void)
{
	return hand_placed;
}

const uint8_t *wii_mesh_shader_pixels(uint16_t *width, uint16_t *height)
{
	if (!shader_sampled)
		return NULL;
	if (width)
		*width = shader_w;
	if (height)
		*height = shader_h;
	return shader_pixels;
}

int wii_mesh_is_weapon(int index)
{
	return weapon_from >= 0 && index >= weapon_from;
}
