/*
 * Skin one model part. Compressed vertices (32 bytes, the Xbox layout)
 * are blended by node weight across the two node matrices, then placed
 * on the unit. At most 96 vertices and 32 triangles are kept.
 */
#include "cseries/cseries.h"
#include "models/model_definitions.h"
#include "objects/objects.h"
#include "units/biped_definitions.h"
#include "wii_mesh.h"
#include "wii_tag_cache.h"

#include <math.h>
#include <string.h>

enum { WII_MESH_VERTS = 96, WII_MESH_TRIS = 32 };

struct wii_mat {
	float m[3][3];
	float x, y, z;
};

static float tris[WII_MESH_TRIS][9];
static int tri_count;
static int skinned;

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
	skinned = 0;
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
	return skin_part(part_va, nodes, node_count, yaw,
		object->object.position.x, object->object.position.y, object->object.position.z);
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
