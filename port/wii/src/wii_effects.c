/*
 * effect_new and contrail_new for the device. They record the tag and
 * decode one contrail bitmap through wii_tex_decode. Particle simulation
 * and the rasterizer's effect pass are not linked.
 */
#include "cseries/cseries.h"
#include "effects/contrail_definitions.h"
#include "effects/contrails.h"
#include "effects/effect_definitions.h"
#include "effects/effects.h"
#include "objects/objects.h"
#include "wii_effects.h"
#include "wii_tag_cache.h"
#include "wii_tex.h"

#include <stdlib.h>
#include <string.h>

#define FCC(a, b, c, d) \
	(((uint32_t)(uint8_t)(a) << 24) | ((uint32_t)(uint8_t)(b) << 16) | \
	 ((uint32_t)(uint8_t)(c) << 8) | (uint32_t)(uint8_t)(d))

static struct wii_resident_effect decoded;
static int live_effects;
static int live_contrails;

enum { WII_PARTICLES = 48, WII_BOLTS = 24 };

struct wii_particle {
	float x, y, z;
	float vx, vy, vz;
	float life;
	float size;
	int alive;
};

static struct wii_particle particles[WII_PARTICLES];
static struct wii_sprite bolts[WII_BOLTS];
static int bolt_count;
static unsigned particle_seed = 1u;

static float particle_rand(void)
{
	particle_seed = particle_seed * 1664525u + 1013904223u;
	return (float)((particle_seed >> 8) & 255u) / 255.f - 0.5f;
}

static void emit_at(float x, float y, float z)
{
	int spawned;
	int index;

	for (spawned = 0; spawned < 8; spawned++) {
		for (index = 0; index < WII_PARTICLES; index++) {
			if (!particles[index].alive)
				break;
		}
		if (index == WII_PARTICLES)
			index = spawned % WII_PARTICLES;
		particles[index].x = x;
		particles[index].y = y;
		particles[index].z = z;
		particles[index].vx = particle_rand() * 0.08f;
		particles[index].vy = particle_rand() * 0.08f;
		particles[index].vz = 0.06f + particle_rand() * 0.04f;
		particles[index].life = 24.f;
		particles[index].size = 0.18f;
		particles[index].alive = 1;
	}
}

static uint32_t read_le32(uint8_t const *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
		| ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int32_t read_les32(uint8_t const *p)
{
	return (int32_t)read_le32(p);
}

static uint16_t read_le16(uint8_t const *p)
{
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int pull_block(uint32_t va, int *count, uint32_t *address)
{
	uint8_t raw[8];

	if (wii_tag_cache_pull(va, raw, 8) != 0)
		return -1;
	*count = read_les32(raw);
	*address = read_le32(raw + 4);
	return *count < 0 ? -1 : 0;
}

static int decode_bitmap(uint32_t bitmap_va, struct wii_resident_effect *effect)
{
	uint8_t group[0x6c];
	uint8_t data[0x30];
	uint8_t pixels_header[20];
	uint32_t count;
	uint32_t address;
	uint16_t width;
	uint16_t height;
	uint16_t format;
	uint16_t flags;
	uint32_t pixels_offset;
	uint32_t pixels_size;
	uint32_t blob_va;
	uint8_t *encoded;
	struct wii_rgba image;
	int linear;

	if (wii_tag_cache_pull(bitmap_va, group, sizeof(group)) != 0)
		return -1;
	count = read_le32(group + 0x60);
	address = read_le32(group + 0x64);
	if (count < 1 || wii_tag_cache_pull(address, data, sizeof(data)) != 0)
		return -1;
	width = read_le16(data + 0x04);
	height = read_le16(data + 0x06);
	format = read_le16(data + 0x0c);
	flags = read_le16(data + 0x0e);
	pixels_offset = read_le32(data + 0x18);
	pixels_size = read_le32(data + 0x1c);
	if (!pixels_size || pixels_size > 256u * 1024u)
		return -1;
	if (wii_tag_cache_pull(bitmap_va + 0x30u, pixels_header, sizeof(pixels_header)) != 0)
		return -1;
	blob_va = read_le32(pixels_header + 12);
	encoded = malloc(pixels_size);
	if (!encoded)
		return -1;
	if (wii_tag_cache_pull(blob_va + pixels_offset, encoded, pixels_size) != 0) {
		free(encoded);
		return -1;
	}
	memset(&image, 0, sizeof(image));
	image.pixels = effect->pixels;
	image.cap = WII_EFFECT_BITMAP_CAP;
	linear = (flags & (1u << 4)) != 0 || format >= 14;
	if (wii_tex_decode(encoded, pixels_size, (int)format, linear, width, height, &image) != 0) {
		free(encoded);
		return -1;
	}
	free(encoded);
	effect->width = image.width;
	effect->height = image.height;
	effect->bitmap_ok = image.ok && image.width > 0 && effect->pixels[0] != 0;
	return effect->bitmap_ok ? 0 : -1;
}

static void remember(struct wii_resident_effect const *effect)
{
	if (!effect || effect->magic != WII_EFFECT_MAGIC)
		return;
	if (effect->bitmap_ok)
		decoded = *effect;
}

static long spawn_effect(long definition_index, long object_index, real_point3d const *point)
{
	struct wii_resident_effect *effect;
	struct object_datum *object;
	float x = 0.f;
	float y = 0.f;
	float z = 1.f;

	effect = wii_tag_cache_get((long)FCC('e', 'f', 'f', 'e'), definition_index);
	remember(effect);
	if (live_effects < 32767)
		live_effects++;
	if (point) {
		x = point->x;
		y = point->y;
		z = point->z;
	} else {
		object = object_index == NONE ? NULL : object_get(object_index);
		if (object) {
			x = object->object.position.x;
			y = object->object.position.y;
			z = object->object.position.z;
		}
	}
	emit_at(x, y, z);
	return live_effects;
}

void wii_effects_tick(void)
{
	int index;

	for (index = 0; index < WII_PARTICLES; index++) {
		if (!particles[index].alive)
			continue;
		particles[index].x += particles[index].vx;
		particles[index].y += particles[index].vy;
		particles[index].z += particles[index].vz;
		particles[index].vz -= 0.004f;
		particles[index].life -= 1.f;
		if (particles[index].life <= 0.f)
			particles[index].alive = 0;
	}
}

void wii_effect_begin_frame(void)
{
	bolt_count = 0;
}

void wii_effect_emit(float x, float y, float z)
{
	emit_at(x, y, z);
}

void wii_effect_push_bolt(float x, float y, float z, float size)
{
	if (bolt_count >= WII_BOLTS)
		return;
	bolts[bolt_count].x = x;
	bolts[bolt_count].y = y;
	bolts[bolt_count].z = z;
	bolts[bolt_count].size = size;
	bolt_count++;
}

int wii_effect_sprites(struct wii_sprite *out, int cap)
{
	int index;
	int count = 0;

	if (!out || cap <= 0)
		return 0;
	for (index = 0; index < WII_PARTICLES && count < cap; index++) {
		if (!particles[index].alive)
			continue;
		out[count].x = particles[index].x;
		out[count].y = particles[index].y;
		out[count].z = particles[index].z;
		out[count].size = particles[index].size;
		count++;
	}
	for (index = 0; index < bolt_count && count < cap; index++)
		out[count++] = bolts[index];
	return count;
}

void *wii_effect_load(uint32_t effect_va)
{
	struct wii_resident_effect *effect;
	int event_count;
	uint32_t event_va;
	int part_count;
	uint32_t part_va;
	uint8_t ref[16];
	uint32_t contrail_index;
	uint32_t contrail_va;
	uint8_t bitmap_index[4];
	uint32_t bitmap_va;

	effect = calloc(1, sizeof(*effect));
	if (!effect)
		return NULL;
	effect->magic = WII_EFFECT_MAGIC;
	if (pull_block(effect_va + offsetof(struct effect_definition, events), &event_count, &event_va) != 0
		|| event_count < 1
		|| pull_block(event_va + offsetof(struct effect_event_definition, parts), &part_count, &part_va) != 0
		|| part_count < 1
		|| wii_tag_cache_pull(part_va + offsetof(struct effect_part_definition, reference) + 12, ref, 4) != 0) {
		return effect;
	}
	contrail_index = read_le32(ref);
	contrail_va = wii_tag_cache_slot_va((long)contrail_index);
	if (!contrail_va
		|| wii_tag_cache_pull(contrail_va + offsetof(struct contrail_definition, bitmap) + 12, bitmap_index, 4) != 0)
		return effect;
	bitmap_va = wii_tag_cache_slot_va((long)read_le32(bitmap_index));
	if (bitmap_va)
		decode_bitmap(bitmap_va, effect);
	if (effect->bitmap_ok)
		decoded = *effect;
	return effect;
}

int wii_effect_live(void)
{
	return live_effects;
}

int wii_effect_bitmap_ok(void)
{
	return decoded.bitmap_ok;
}

const uint8_t *wii_effect_pixels(uint16_t *width, uint16_t *height)
{
	if (!decoded.bitmap_ok)
		return NULL;
	if (width)
		*width = decoded.width;
	if (height)
		*height = decoded.height;
	return decoded.pixels;
}

long effect_new_from_object(
	long definition_index,
	long owner_object_index,
	long object_index,
	short force_local_player_index,
	real scale_a,
	real scale_b,
	real_rgb_color const *color,
	struct effect_vector_field const *impulse_field)
{
	(void)owner_object_index;
	(void)object_index;
	(void)force_local_player_index;
	(void)scale_a;
	(void)scale_b;
	(void)color;
	(void)impulse_field;
	return spawn_effect(definition_index, object_index, NULL);
}

long effect_new_attached_from_markers(
	long definition_index,
	long owner_object_index,
	long object_index,
	short node_index,
	short marker_count,
	char const **marker_names,
	real_point3d const *marker_points,
	real_vector3d const *marker_forwards,
	real scale_a,
	real scale_b,
	real_rgb_color const *color,
	struct effect_vector_field const *impulse_field)
{
	(void)owner_object_index;
	(void)object_index;
	(void)node_index;
	(void)marker_count;
	(void)marker_names;
	(void)marker_points;
	(void)marker_forwards;
	(void)scale_a;
	(void)scale_b;
	(void)color;
	(void)impulse_field;
	return spawn_effect(definition_index, object_index, NULL);
}

long effect_new_unattached_from_markers(
	long definition_index,
	long owner_object_index,
	real_vector3d const *translational_velocity,
	short marker_count,
	char const **marker_names,
	real_point3d const *marker_points,
	real_vector3d const *marker_forwards,
	real scale_a,
	real scale_b,
	real_rgb_color const *color,
	struct effect_vector_field const *impulse_field,
	boolean can_be_deterministic)
{
	(void)owner_object_index;
	(void)translational_velocity;
	(void)marker_count;
	(void)marker_names;
	(void)marker_points;
	(void)marker_forwards;
	(void)scale_a;
	(void)scale_b;
	(void)color;
	(void)impulse_field;
	(void)can_be_deterministic;
	return spawn_effect(definition_index, NONE, marker_count > 0 ? marker_points : NULL);
}

long effect_new_looping(
	long definition_index,
	long object_index,
	short scale_a_function_index,
	short scale_b_function_index,
	short change_color_index)
{
	(void)object_index;
	(void)scale_a_function_index;
	(void)scale_b_function_index;
	(void)change_color_index;
	return spawn_effect(definition_index, object_index, NULL);
}

long contrail_new(long definition_index, long object_index, short attachment_index)
{
	struct wii_resident_effect *effect;
	uint32_t contrail_va;
	uint8_t bitmap_index[4];
	uint32_t bitmap_va;

	(void)object_index;
	(void)attachment_index;
	contrail_va = wii_tag_cache_slot_va(definition_index);
	effect = calloc(1, sizeof(*effect));
	if (effect && contrail_va
		&& wii_tag_cache_pull(contrail_va + offsetof(struct contrail_definition, bitmap) + 12, bitmap_index, 4) == 0) {
		effect->magic = WII_EFFECT_MAGIC;
		bitmap_va = wii_tag_cache_slot_va((long)read_le32(bitmap_index));
		if (bitmap_va)
			decode_bitmap(bitmap_va, effect);
		remember(effect);
	}
	free(effect);
	if (live_contrails < 32767)
		live_contrails++;
	return live_contrails;
}

void contrail_delete(long contrail_index)
{
	(void)contrail_index;
	if (live_contrails > 0)
		live_contrails--;
}

void contrail_owner_collision(long contrail_index, unsigned char object_dying, real dt)
{
	(void)contrail_index;
	(void)object_dying;
	(void)dt;
}
