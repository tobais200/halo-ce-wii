#include "bsp_preview.h"

#include <stdio.h>
#include <string.h>

#define TAG_CACHE_BASE 0x803A6000u
#define TAG_INDEX_SIGNATURE 0x73676174u
#define BSP_SIGNATURE 0x70736273u
#define SCENARIO_GROUP 0x726e6373u
#define SCENARIO_BSP_BLOCK 0x5A4u
#define BSP_HEADER_BYTES 0x18u
#define BSP_SURFACES_BLOCK 0xF8u
#define BSP_LIGHTMAPS_BLOCK 0x104u
#define LIGHTMAP_BYTES 0x20u
#define LIGHTMAP_MATERIALS 0x14u
#define MATERIAL_BYTES 0x100u
#define MATERIAL_FIRST_SURFACE 0x14u
#define MATERIAL_SURFACE_COUNT 0x18u
#define MATERIAL_VERTEX_TYPE 0xB0u
#define MATERIAL_VERTEX_COUNT 0xB4u
#define MATERIAL_COMPRESSED 0xECu
#define MATERIAL_UNCOMPRESSED 0xD8u
#define SURFACE_BYTES 6u

struct reader {
	bsp_read_fn read;
	void *context;
	uint32_t file_size;
};

static void set_message(struct bsp_preview_info *info, char const *text)
{
	if (!info)
		return;
	snprintf(info->message, sizeof(info->message), "%s", text ? text : "");
}

static int pull(struct reader const *reader, uint32_t offset, void *destination, uint32_t size)
{
	if (!reader || !destination || !size)
		return -1;
	if ((uint64_t)offset + size > reader->file_size)
		return -1;
	return reader->read(reader->context, offset, destination, size);
}

static int map_va(
	uint32_t va,
	uint32_t tag_offset,
	uint32_t tag_size,
	uint32_t bsp_offset,
	uint32_t bsp_size,
	uint32_t bsp_va,
	uint32_t *file_offset)
{
	if (bsp_size && va >= bsp_va && (va - bsp_va) < bsp_size) {
		*file_offset = bsp_offset + (va - bsp_va);
		return 0;
	}
	if (tag_size && va >= TAG_CACHE_BASE && (va - TAG_CACHE_BASE) < tag_size) {
		*file_offset = tag_offset + (va - TAG_CACHE_BASE);
		return 0;
	}
	return -1;
}

/* rasterizer_geometry.c uncompress_int32_to_real_vector3d */
static void uncompress_normal(uint32_t compressed, float *k_out)
{
	float value;
	float k;

	compressed >>= 11;
	compressed >>= 11;
	value = (float)(int32_t)(compressed << 22);
	k = (value * (1.0f / 2097152.0f) + 1.0f) * (1.0f / 1023.0f);
	*k_out = k;
}

static void shade(float up, uint8_t *color)
{
	float light;

	light = up * 0.85f + 0.35f;
	if (light < 0.15f)
		light = 0.15f;
	if (light > 1.0f)
		light = 1.0f;
	color[0] = (uint8_t)(light * 210.0f);
	color[1] = (uint8_t)(light * 200.0f);
	color[2] = (uint8_t)(light * 180.0f);
	color[3] = 255;
}

struct point_bounds {
	float min[3];
	float max[3];
	int have;
};

static void add_point(struct point_bounds *bounds, float x, float y, float z)
{
	if (!bounds->have) {
		bounds->min[0] = bounds->max[0] = x;
		bounds->min[1] = bounds->max[1] = y;
		bounds->min[2] = bounds->max[2] = z;
		bounds->have = 1;
		return;
	}
	if (x < bounds->min[0])
		bounds->min[0] = x;
	if (x > bounds->max[0])
		bounds->max[0] = x;
	if (y < bounds->min[1])
		bounds->min[1] = y;
	if (y > bounds->max[1])
		bounds->max[1] = y;
	if (z < bounds->min[2])
		bounds->min[2] = z;
	if (z > bounds->max[2])
		bounds->max[2] = z;
}

static void finish_bounds(struct point_bounds const *bounds, struct bsp_preview_info *info)
{
	float extent[3];
	int axis;

	if (!bounds->have) {
		info->radius = 10.0f;
		return;
	}
	info->radius = 1.0f;
	for (axis = 0; axis < 3; axis++) {
		info->center[axis] = (bounds->min[axis] + bounds->max[axis]) * 0.5f;
		extent[axis] = (bounds->max[axis] - bounds->min[axis]) * 0.5f;
		if (extent[axis] < 0.0f)
			extent[axis] = -extent[axis];
		if (extent[axis] > info->radius)
			info->radius = extent[axis];
	}
}

static int find_first_bsp(
	struct reader const *reader,
	uint32_t tag_offset,
	uint32_t tag_size,
	uint32_t *bsp_offset,
	uint32_t *bsp_size,
	uint32_t *bsp_va)
{
	uint8_t header[0x28];
	uint8_t instance[0x20];
	uint8_t block[12];
	uint8_t reference[0x20];
	uint32_t signature;
	uint32_t instances_va;
	uint32_t scenario_index;
	uint32_t tag_count;
	uint32_t scenario_va;
	uint32_t group;
	uint32_t block_va;
	uint32_t block_count;
	uint32_t file_offset;
	uint32_t absolute;

	if (pull(reader, tag_offset, header, sizeof(header)) != 0)
		return -1;
	signature = (uint32_t)header[0x20]
		| ((uint32_t)header[0x21] << 8)
		| ((uint32_t)header[0x22] << 16)
		| ((uint32_t)header[0x23] << 24);
	if (signature != TAG_INDEX_SIGNATURE) {
		signature = (uint32_t)header[0x24]
			| ((uint32_t)header[0x25] << 8)
			| ((uint32_t)header[0x26] << 16)
			| ((uint32_t)header[0x27] << 24);
		if (signature != TAG_INDEX_SIGNATURE)
			return -1;
	}
	instances_va = (uint32_t)header[0] | ((uint32_t)header[1] << 8)
		| ((uint32_t)header[2] << 16) | ((uint32_t)header[3] << 24);
	scenario_index = (uint32_t)header[4] | ((uint32_t)header[5] << 8)
		| ((uint32_t)header[6] << 16) | ((uint32_t)header[7] << 24);
	tag_count = (uint32_t)header[0x0C] | ((uint32_t)header[0x0D] << 8)
		| ((uint32_t)header[0x0E] << 16) | ((uint32_t)header[0x0F] << 24);
	absolute = scenario_index & 0xFFFFu;
	if (!tag_count || absolute >= tag_count)
		return -1;
	if (map_va(instances_va, tag_offset, tag_size, 0, 0, 0, &file_offset) != 0)
		return -1;
	if (pull(reader, file_offset + absolute * 0x20u, instance, sizeof(instance)) != 0)
		return -1;
	group = (uint32_t)instance[0] | ((uint32_t)instance[1] << 8)
		| ((uint32_t)instance[2] << 16) | ((uint32_t)instance[3] << 24);
	if (group != SCENARIO_GROUP)
		return -1;
	scenario_va = (uint32_t)instance[0x14] | ((uint32_t)instance[0x15] << 8)
		| ((uint32_t)instance[0x16] << 16) | ((uint32_t)instance[0x17] << 24);
	if (map_va(scenario_va, tag_offset, tag_size, 0, 0, 0, &file_offset) != 0)
		return -1;
	if (pull(reader, file_offset + SCENARIO_BSP_BLOCK, block, sizeof(block)) != 0)
		return -1;
	block_count = (uint32_t)block[0] | ((uint32_t)block[1] << 8)
		| ((uint32_t)block[2] << 16) | ((uint32_t)block[3] << 24);
	block_va = (uint32_t)block[4] | ((uint32_t)block[5] << 8)
		| ((uint32_t)block[6] << 16) | ((uint32_t)block[7] << 24);
	if (!block_count)
		return -1;
	if (map_va(block_va, tag_offset, tag_size, 0, 0, 0, &file_offset) != 0)
		return -1;
	if (pull(reader, file_offset, reference, sizeof(reference)) != 0)
		return -1;
	*bsp_offset = (uint32_t)reference[0] | ((uint32_t)reference[1] << 8)
		| ((uint32_t)reference[2] << 16) | ((uint32_t)reference[3] << 24);
	*bsp_size = (uint32_t)reference[4] | ((uint32_t)reference[5] << 8)
		| ((uint32_t)reference[6] << 16) | ((uint32_t)reference[7] << 24);
	*bsp_va = (uint32_t)reference[8] | ((uint32_t)reference[9] << 8)
		| ((uint32_t)reference[10] << 16) | ((uint32_t)reference[11] << 24);
	if (*bsp_size < BSP_HEADER_BYTES || (uint64_t)*bsp_offset + *bsp_size > reader->file_size)
		return -1;
	return 0;
}

static int append_material(
	struct reader const *reader,
	uint32_t material_offset,
	uint32_t surface_file,
	uint32_t surface_count_total,
	uint32_t tag_offset,
	uint32_t tag_size,
	uint32_t bsp_offset,
	uint32_t bsp_size,
	uint32_t bsp_va,
	struct bsp_preview_store const *store,
	struct bsp_preview_info *info,
	struct point_bounds *bounds)
{
	uint8_t material[MATERIAL_BYTES];
	uint8_t record[56];
	uint32_t first_surface;
	uint32_t surface_count;
	uint32_t vertex_count;
	uint32_t data_size;
	uint32_t data_va;
	uint32_t data_file;
	uint16_t vertex_type;
	uint32_t stride;
	uint32_t kept;
	uint32_t index;
	uint16_t base_vertex;

	if (pull(reader, material_offset, material, sizeof(material)) != 0)
		return -1;
	vertex_type = (uint16_t)(material[MATERIAL_VERTEX_TYPE] | (material[MATERIAL_VERTEX_TYPE + 1] << 8));
	vertex_count = (uint32_t)material[MATERIAL_VERTEX_COUNT]
		| ((uint32_t)material[MATERIAL_VERTEX_COUNT + 1] << 8)
		| ((uint32_t)material[MATERIAL_VERTEX_COUNT + 2] << 16)
		| ((uint32_t)material[MATERIAL_VERTEX_COUNT + 3] << 24);
	first_surface = (uint32_t)material[MATERIAL_FIRST_SURFACE]
		| ((uint32_t)material[MATERIAL_FIRST_SURFACE + 1] << 8)
		| ((uint32_t)material[MATERIAL_FIRST_SURFACE + 2] << 16)
		| ((uint32_t)material[MATERIAL_FIRST_SURFACE + 3] << 24);
	surface_count = (uint32_t)material[MATERIAL_SURFACE_COUNT]
		| ((uint32_t)material[MATERIAL_SURFACE_COUNT + 1] << 8)
		| ((uint32_t)material[MATERIAL_SURFACE_COUNT + 2] << 16)
		| ((uint32_t)material[MATERIAL_SURFACE_COUNT + 3] << 24);
	if (vertex_type == 1) {
		stride = 32;
		data_size = (uint32_t)material[MATERIAL_COMPRESSED]
			| ((uint32_t)material[MATERIAL_COMPRESSED + 1] << 8)
			| ((uint32_t)material[MATERIAL_COMPRESSED + 2] << 16)
			| ((uint32_t)material[MATERIAL_COMPRESSED + 3] << 24);
		data_va = (uint32_t)material[MATERIAL_COMPRESSED + 12]
			| ((uint32_t)material[MATERIAL_COMPRESSED + 13] << 8)
			| ((uint32_t)material[MATERIAL_COMPRESSED + 14] << 16)
			| ((uint32_t)material[MATERIAL_COMPRESSED + 15] << 24);
	} else if (vertex_type == 0) {
		stride = 56;
		data_size = (uint32_t)material[MATERIAL_UNCOMPRESSED]
			| ((uint32_t)material[MATERIAL_UNCOMPRESSED + 1] << 8)
			| ((uint32_t)material[MATERIAL_UNCOMPRESSED + 2] << 16)
			| ((uint32_t)material[MATERIAL_UNCOMPRESSED + 3] << 24);
		data_va = (uint32_t)material[MATERIAL_UNCOMPRESSED + 12]
			| ((uint32_t)material[MATERIAL_UNCOMPRESSED + 13] << 8)
			| ((uint32_t)material[MATERIAL_UNCOMPRESSED + 14] << 16)
			| ((uint32_t)material[MATERIAL_UNCOMPRESSED + 15] << 24);
	} else {
		return 0;
	}
	info->vertices_seen += vertex_count;
	info->triangles_seen += surface_count;
	if (!vertex_count || !surface_count || !data_size)
		return 0;
	if (map_va(data_va, tag_offset, tag_size, bsp_offset, bsp_size, bsp_va, &data_file) != 0)
		return 0;
	if (first_surface > surface_count_total || surface_count > surface_count_total - first_surface)
		return 0;

	base_vertex = info->vertices;
	kept = vertex_count;
	if (kept > data_size / stride)
		kept = data_size / stride;
	if (base_vertex >= store->vertex_cap) {
		info->truncated = 1;
		return 0;
	}
	if (kept > (uint32_t)(store->vertex_cap - base_vertex)) {
		kept = (uint32_t)(store->vertex_cap - base_vertex);
		info->truncated = 1;
	}

	for (index = 0; index < kept; index++) {
		float x;
		float y;
		float z;
		float up = 0.6f;
		uint16_t slot = (uint16_t)(base_vertex + index);

		if (pull(reader, data_file + index * stride, record, stride) != 0) {
			info->truncated = 1;
			break;
		}
		{
			uint32_t bits;
			bits = (uint32_t)record[0] | ((uint32_t)record[1] << 8)
				| ((uint32_t)record[2] << 16) | ((uint32_t)record[3] << 24);
			memcpy(&x, &bits, 4);
			bits = (uint32_t)record[4] | ((uint32_t)record[5] << 8)
				| ((uint32_t)record[6] << 16) | ((uint32_t)record[7] << 24);
			memcpy(&y, &bits, 4);
			bits = (uint32_t)record[8] | ((uint32_t)record[9] << 8)
				| ((uint32_t)record[10] << 16) | ((uint32_t)record[11] << 24);
			memcpy(&z, &bits, 4);
		}
		if (vertex_type == 1) {
			uint32_t packed = (uint32_t)record[12] | ((uint32_t)record[13] << 8)
				| ((uint32_t)record[14] << 16) | ((uint32_t)record[15] << 24);
			uncompress_normal(packed, &up);
		}
		store->positions[slot * 3] = x;
		store->positions[slot * 3 + 1] = y;
		store->positions[slot * 3 + 2] = z;
		shade(up, store->colors + slot * 4);
		if (store->uvs) {
			store->uvs[slot * 2] = 0.0f;
			store->uvs[slot * 2 + 1] = 0.0f;
		}
		add_point(bounds, x, y, z);
	}
	info->vertices = (uint16_t)(base_vertex + kept);

	for (index = 0; index < surface_count; index++) {
		uint8_t surface[6];
		uint16_t a;
		uint16_t b;
		uint16_t c;
		uint16_t slot;

		if (info->triangles >= store->triangle_cap) {
			info->truncated = 1;
			break;
		}
		if (pull(reader, surface_file + (first_surface + index) * SURFACE_BYTES, surface, 6) != 0) {
			info->truncated = 1;
			break;
		}
		a = (uint16_t)(surface[0] | (surface[1] << 8));
		b = (uint16_t)(surface[2] | (surface[3] << 8));
		c = (uint16_t)(surface[4] | (surface[5] << 8));
		if (a >= kept || b >= kept || c >= kept)
			continue;
		slot = info->triangles;
		store->indices[slot * 3] = (uint16_t)(base_vertex + a);
		store->indices[slot * 3 + 1] = (uint16_t)(base_vertex + b);
		store->indices[slot * 3 + 2] = (uint16_t)(base_vertex + c);
		info->triangles++;
	}

	if (store->uvs && kept) {
		uint16_t lm_type = (uint16_t)(material[0xC4] | (material[0xC5] << 8));
		uint32_t lm_count = (uint32_t)material[0xC8] | ((uint32_t)material[0xC9] << 8)
			| ((uint32_t)material[0xCA] << 16) | ((uint32_t)material[0xCB] << 24);
		uint32_t lm_off = (uint32_t)material[0xCC] | ((uint32_t)material[0xCD] << 8)
			| ((uint32_t)material[0xCE] << 16) | ((uint32_t)material[0xCF] << 24);
		uint32_t lm_base = (uint32_t)material[0xD0] | ((uint32_t)material[0xD1] << 8)
			| ((uint32_t)material[0xD2] << 16) | ((uint32_t)material[0xD3] << 24);
		uint32_t lm_file;
		uint32_t stride = lm_type == 3 ? 8u : lm_type == 2 ? 20u : 0u;

		if (stride && lm_count >= kept
			&& map_va(lm_base, tag_offset, tag_size, bsp_offset, bsp_size, bsp_va, &lm_file) == 0) {
			for (index = 0; index < kept; index++) {
				uint8_t lm[20];
				float u = 0.0f;
				float v = 0.0f;
				uint16_t slot = (uint16_t)(base_vertex + index);

				if (pull(reader, lm_file + lm_off + index * stride, lm, stride) != 0)
					break;
				if (stride == 8u) {
					int16_t su = (int16_t)(lm[4] | (lm[5] << 8));
					int16_t sv = (int16_t)(lm[6] | (lm[7] << 8));
					u = ((float)su * 2.0f + 1.0f) * (1.0f / 65535.0f);
					v = ((float)sv * 2.0f + 1.0f) * (1.0f / 65535.0f);
				} else {
					uint32_t bits = (uint32_t)lm[12] | ((uint32_t)lm[13] << 8)
						| ((uint32_t)lm[14] << 16) | ((uint32_t)lm[15] << 24);
					memcpy(&u, &bits, 4);
					bits = (uint32_t)lm[16] | ((uint32_t)lm[17] << 8)
						| ((uint32_t)lm[18] << 16) | ((uint32_t)lm[19] << 24);
					memcpy(&v, &bits, 4);
				}
				store->uvs[slot * 2] = u;
				store->uvs[slot * 2 + 1] = v;
			}
		}
	}
	return 0;
}

int bsp_preview_load(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_data_offset,
	uint32_t tag_data_size,
	struct bsp_preview_store const *store,
	struct bsp_preview_info *info)
{
	struct reader reader;
	uint32_t bsp_offset = 0;
	uint32_t bsp_size = 0;
	uint32_t bsp_va = 0;
	uint8_t header[BSP_HEADER_BYTES];
	uint8_t block[12];
	uint32_t signature;
	uint32_t structure_va;
	uint32_t structure_file;
	uint32_t lightmap_count;
	uint32_t lightmap_va;
	uint32_t lightmap_file;
	uint32_t surface_count;
	uint32_t surface_va;
	uint32_t surface_file;
	uint32_t lightmap;
	struct point_bounds bounds;

	if (!info || !store || !store->positions || !store->colors || !store->indices)
		return -1;
	memset(info, 0, sizeof(*info));
	memset(&bounds, 0, sizeof(bounds));
	info->radius = 1.0f;
	info->lightmap_bitmap_index = 0xffffu;
	reader.read = read;
	reader.context = context;
	reader.file_size = file_size;
	if (!tag_data_size || tag_data_offset >= file_size
		|| (uint64_t)tag_data_offset + tag_data_size > file_size) {
		set_message(info, "tag data range is outside the file");
		return -1;
	}
	if (find_first_bsp(&reader, tag_data_offset, tag_data_size, &bsp_offset, &bsp_size, &bsp_va) != 0) {
		set_message(info, "no scenario structure BSP");
		return -1;
	}
	if (pull(&reader, bsp_offset, header, sizeof(header)) != 0) {
		set_message(info, "BSP header unreadable");
		return -1;
	}
	signature = (uint32_t)header[0x14] | ((uint32_t)header[0x15] << 8)
		| ((uint32_t)header[0x16] << 16) | ((uint32_t)header[0x17] << 24);
	if (signature != BSP_SIGNATURE) {
		set_message(info, "BSP signature is not sbsp");
		return -1;
	}
	structure_va = (uint32_t)header[0] | ((uint32_t)header[1] << 8)
		| ((uint32_t)header[2] << 16) | ((uint32_t)header[3] << 24);
	if (map_va(structure_va, tag_data_offset, tag_data_size, bsp_offset, bsp_size, bsp_va, &structure_file) != 0) {
		set_message(info, "BSP body pointer is outside the file");
		return -1;
	}
	{
		uint8_t reference[16];
		uint32_t tag_index;

		if (pull(&reader, structure_file, reference, sizeof(reference)) == 0) {
			tag_index = (uint32_t)reference[12] | ((uint32_t)reference[13] << 8)
				| ((uint32_t)reference[14] << 16) | ((uint32_t)reference[15] << 24);
			if (tag_index != 0xffffffffu) {
				uint8_t tag_header[4];
				uint8_t instance[0x20];
				uint32_t instance_va;
				uint32_t instance_file;
				uint32_t base;

				if (pull(&reader, tag_data_offset, tag_header, 4) == 0) {
					instance_va = (uint32_t)tag_header[0] | ((uint32_t)tag_header[1] << 8)
						| ((uint32_t)tag_header[2] << 16) | ((uint32_t)tag_header[3] << 24);
					if (instance_va >= 0x803A6000u) {
						instance_file = tag_data_offset + (instance_va - 0x803A6000u) + (tag_index & 0xffffu) * 0x20u;
						if (pull(&reader, instance_file, instance, sizeof(instance)) == 0) {
							base = (uint32_t)instance[0x14] | ((uint32_t)instance[0x15] << 8)
								| ((uint32_t)instance[0x16] << 16) | ((uint32_t)instance[0x17] << 24);
							if (base >= 0x803A6000u && (base - 0x803A6000u) < tag_data_size) {
								info->bitmap_group_file = tag_data_offset + (base - 0x803A6000u);
								info->bitmap_group_found = 1;
							}
						}
					}
				}
			}
		}
	}
	if (pull(&reader, structure_file + BSP_SURFACES_BLOCK, block, sizeof(block)) != 0) {
		set_message(info, "BSP surface block unreadable");
		return -1;
	}
	surface_count = (uint32_t)block[0] | ((uint32_t)block[1] << 8)
		| ((uint32_t)block[2] << 16) | ((uint32_t)block[3] << 24);
	surface_va = (uint32_t)block[4] | ((uint32_t)block[5] << 8)
		| ((uint32_t)block[6] << 16) | ((uint32_t)block[7] << 24);
	if (!surface_count || map_va(surface_va, tag_data_offset, tag_data_size, bsp_offset, bsp_size, bsp_va, &surface_file) != 0) {
		set_message(info, "BSP has no surfaces");
		return -1;
	}
	if (pull(&reader, structure_file + BSP_LIGHTMAPS_BLOCK, block, sizeof(block)) != 0) {
		set_message(info, "BSP lightmap block unreadable");
		return -1;
	}
	lightmap_count = (uint32_t)block[0] | ((uint32_t)block[1] << 8)
		| ((uint32_t)block[2] << 16) | ((uint32_t)block[3] << 24);
	lightmap_va = (uint32_t)block[4] | ((uint32_t)block[5] << 8)
		| ((uint32_t)block[6] << 16) | ((uint32_t)block[7] << 24);
	if (!lightmap_count || lightmap_count > 64
		|| map_va(lightmap_va, tag_data_offset, tag_data_size, bsp_offset, bsp_size, bsp_va, &lightmap_file) != 0) {
		set_message(info, "BSP has no lightmaps");
		return -1;
	}

	for (lightmap = 0; lightmap < lightmap_count; lightmap++) {
		uint8_t lightmap_bytes[LIGHTMAP_BYTES];
		uint32_t material_count;
		uint32_t material_va;
		uint32_t material_file;
		uint32_t material;

		if (pull(&reader, lightmap_file + lightmap * LIGHTMAP_BYTES, lightmap_bytes, sizeof(lightmap_bytes)) != 0)
			break;
		if (info->lightmap_bitmap_index == 0xffffu)
			info->lightmap_bitmap_index = (uint16_t)(lightmap_bytes[0] | (lightmap_bytes[1] << 8));
		material_count = (uint32_t)lightmap_bytes[LIGHTMAP_MATERIALS]
			| ((uint32_t)lightmap_bytes[LIGHTMAP_MATERIALS + 1] << 8)
			| ((uint32_t)lightmap_bytes[LIGHTMAP_MATERIALS + 2] << 16)
			| ((uint32_t)lightmap_bytes[LIGHTMAP_MATERIALS + 3] << 24);
		material_va = (uint32_t)lightmap_bytes[LIGHTMAP_MATERIALS + 4]
			| ((uint32_t)lightmap_bytes[LIGHTMAP_MATERIALS + 5] << 8)
			| ((uint32_t)lightmap_bytes[LIGHTMAP_MATERIALS + 6] << 16)
			| ((uint32_t)lightmap_bytes[LIGHTMAP_MATERIALS + 7] << 24);
		if (!material_count || material_count > 256
			|| map_va(material_va, tag_data_offset, tag_data_size, bsp_offset, bsp_size, bsp_va, &material_file) != 0)
			continue;
		for (material = 0; material < material_count; material++) {
			if (info->triangles >= store->triangle_cap || info->vertices >= store->vertex_cap) {
				info->truncated = 1;
				break;
			}
			if (append_material(&reader, material_file + material * MATERIAL_BYTES, surface_file, surface_count,
				tag_data_offset, tag_data_size, bsp_offset, bsp_size, bsp_va, store, info, &bounds) != 0) {
				set_message(info, "material read failed");
				return -1;
			}
		}
	}

	if (!info->triangles) {
		set_message(info, "BSP has no drawable environment triangles");
		return -1;
	}
	finish_bounds(&bounds, info);
	if (info->radius < 1.0f)
		info->radius = 1.0f;
	if (info->truncated)
		snprintf(info->message, sizeof(info->message), "first BSP, capped at %u verts", (unsigned)info->vertices);
	else
		snprintf(info->message, sizeof(info->message), "first structure BSP");
	info->ok = 1;
	return 0;
}
