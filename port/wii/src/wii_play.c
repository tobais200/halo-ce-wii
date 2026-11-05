/*
 * Player and actor locomotion on the streamed structure BSP, and shots
 * that use the resident projectile tag and impact damage.
 */
#include "cseries/cseries.h"
#include "ai/actors.h"
#include "objects/objects.h"
#include "units/units.h"
#include "wii_collision.h"
#include "wii_effects.h"
#include "wii_game.h"
#include "wii_play.h"
#include "wii_tag_cache.h"

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#ifdef GEKKO
typedef struct AESNDPB AESNDPB;
void AESND_Init(void);
AESNDPB *AESND_AllocateVoiceWithArg(void *callback, void *argument);
void AESND_PlayVoice(AESNDPB *voice, unsigned int format, void const *buffer, unsigned int length, float frequency, unsigned int delay, int looped);
enum { WII_VOICE_MONO16 = 2 };
#endif

enum { WII_SHOTS = 16 };

struct wii_shot {
	long object_index;
	long owner;
	float vx, vy, vz;
	int alive;
	int from_actor;
};

static struct wii_shot shots[WII_SHOTS];
static int player_shots;
static int actor_shots;
static int moved;
static int grounded;
static int shot_wait;
static int actor_wait;
static int burst_left;
static long sound_tag;
static int save_valid;
static float save_x, save_y, save_z, save_yaw;

static void step_unit(long unit_index, float yaw, float forward_throttle, float strafe_throttle, int record)
{
	struct object_datum *object;
	float cx, sy, cy;
	float dx, dy;
	float scale;
	float hx, hy, hz;
	float old_x, old_y;

	object = object_get(unit_index);
	if (!object)
		return;
	cy = cosf(yaw);
	sy = sinf(yaw);
	dx = cy * forward_throttle * 0.35f - sy * strafe_throttle * 0.28f;
	dy = sy * forward_throttle * 0.35f + cy * strafe_throttle * 0.28f;
	old_x = object->object.position.x;
	old_y = object->object.position.y;
	if (wii_collision_clip(object->object.position.x, object->object.position.y,
			object->object.position.z + 0.45f, dx, dy, 0.f, &scale, &hx, &hy, &hz)) {
		dx *= scale * 0.5f;
		dy *= scale * 0.5f;
	}
	object->object.position.x += dx;
	object->object.position.y += dy;
	if (wii_collision_clip(object->object.position.x, object->object.position.y,
			object->object.position.z + 1.5f, 0.f, 0.f, -24.f, &scale, &hx, &hy, &hz)) {
		object->object.position.z = hz;
		grounded = 1;
	}
	object->object.forward.i = cy;
	object->object.forward.j = sy;
	object->object.forward.k = 0.f;
	object->object.up.i = 0.f;
	object->object.up.j = 0.f;
	object->object.up.k = 1.f;
	if (forward_throttle != 0.f || strafe_throttle != 0.f) {
		float travel = (object->object.position.x - old_x) * (object->object.position.x - old_x)
			+ (object->object.position.y - old_y) * (object->object.position.y - old_y);
		if (record && travel > 0.01f)
			moved = 1;
		wii_effect_contrail(object->object.position.x, object->object.position.y,
			object->object.position.z + 0.35f);
	}
}

static void spawn_shot(long owner, float x, float y, float z, float dx, float dy, float dz, int from_actor)
{
	struct object_placement_data placement;
	long definition;
	long created;
	struct object_datum *object;
	int slot;

	definition = wii_tag_cache_projectile_index();
	if (definition == NONE || definition == -1)
		return;
	for (slot = 0; slot < WII_SHOTS; slot++) {
		if (!shots[slot].alive)
			break;
	}
	if (slot == WII_SHOTS)
		return;
	memset(&placement, 0, sizeof(placement));
	object_placement_data_new(&placement, definition, owner);
	placement.position.x = x;
	placement.position.y = y;
	placement.position.z = z;
	placement.translational_velocity.i = dx;
	placement.translational_velocity.j = dy;
	placement.translational_velocity.k = dz;
	created = object_new(&placement);
	if (created == NONE)
		return;
	object = object_get(created);
	if (object) {
		object->object.translational_velocity.i = dx;
		object->object.translational_velocity.j = dy;
		object->object.translational_velocity.k = dz;
	}
	shots[slot].object_index = created;
	shots[slot].owner = owner;
	shots[slot].vx = dx;
	shots[slot].vy = dy;
	shots[slot].vz = dz;
	shots[slot].alive = 1;
	shots[slot].from_actor = from_actor;
	wii_effect_emit(x, y, z);
	wii_sound_impulse(from_actor ? 2 : 1);
	if (from_actor)
		actor_shots++;
	else
		player_shots++;
}

static void damage_near(long attacker, float x, float y, float z)
{
	long index;
	long damage;
	extern struct data_array *object_header_data;

	damage = wii_tag_cache_damage_index();
	if (!object_header_data || damage == NONE)
		return;
	index = NONE;
	while ((index = data_next_index(object_header_data, index)) != NONE) {
		struct object_datum *object = object_get(index);
		float dx, dy, dz;

		if (!object || index == attacker || object->object.type != _object_type_biped)
			continue;
		dx = object->object.position.x - x;
		dy = object->object.position.y - y;
		dz = object->object.position.z - z;
		if (dx * dx + dy * dy + dz * dz < 2.25f)
			wii_damage_hurts(index, damage);
	}
}

static void advance_shots(void)
{
	int slot;

	for (slot = 0; slot < WII_SHOTS; slot++) {
		struct object_datum *object;
		float scale, hx, hy, hz;

		if (!shots[slot].alive)
			continue;
		object = object_get(shots[slot].object_index);
		if (!object) {
			shots[slot].alive = 0;
			continue;
		}
		if (wii_collision_clip(object->object.position.x, object->object.position.y,
				object->object.position.z, shots[slot].vx, shots[slot].vy, shots[slot].vz,
				&scale, &hx, &hy, &hz)) {
			wii_effect_emit(hx, hy, hz);
			damage_near(shots[slot].owner, hx, hy, hz);
			object_delete(shots[slot].object_index);
			shots[slot].alive = 0;
			continue;
		}
		object->object.position.x += shots[slot].vx;
		object->object.position.y += shots[slot].vy;
		object->object.position.z += shots[slot].vz;
		wii_effect_push_bolt(object->object.position.x, object->object.position.y,
			object->object.position.z, 0.08f);
		shots[slot].vz -= 0.002f;
	}
}

static void actors_fight(long player_unit)
{
	struct object_datum *player;
	long index;

	if (!actor_data || player_unit == NONE || actor_wait > 0)
		return;
	player = object_get(player_unit);
	if (!player)
		return;
	index = NONE;
	while ((index = data_next_index(actor_data, index)) != NONE) {
		struct actor_datum *actor = actor_get(index);
		struct object_datum *unit;
		float dx, dy, dz, len;

		if (!actor || actor->meta.unit_index == NONE || actor->meta.unit_index == player_unit)
			continue;
		if (actor->meta.team_index == 0)
			continue;
		unit = object_get(actor->meta.unit_index);
		if (!unit || unit->object.body_vitality <= 0.f)
			continue;
		dx = player->object.position.x - unit->object.position.x;
		dy = player->object.position.y - unit->object.position.y;
		dz = (player->object.position.z + 0.6f) - (unit->object.position.z + 0.6f);
		len = sqrtf(dx * dx + dy * dy + dz * dz);
		if (len < 0.5f || len > 40.f)
			continue;
		dx /= len;
		dy /= len;
		dz /= len;
		/* Chase when far, backpedal when inside the burst, otherwise hold and fire. */
		step_unit(actor->meta.unit_index, atan2f(dy, dx),
			len > 16.f ? 1.f : (len < 3.5f ? -0.7f : 0.15f), 0.f, 0);
		if (burst_left <= 0 && len < 22.f)
			burst_left = 3;
		if (burst_left > 0) {
			spawn_shot(actor->meta.unit_index,
				unit->object.position.x, unit->object.position.y, unit->object.position.z + 0.7f,
				dx * 0.8f, dy * 0.8f, dz * 0.8f, 1);
			burst_left--;
			actor_wait = burst_left ? 2 : 12;
		}
		return;
	}
}

void wii_play_after_tick(void)
{
	long player;
	float eye_x, eye_y, eye_z;
	float yaw, pitch, cp, sp, cy, sy;

	moved = 0;
	wii_effect_begin_frame();
	player = wii_game_player_unit();
	if (player != NONE)
		step_unit(player, wii_control_yaw(), wii_control_forward(), wii_control_strafe(), 1);
	if (shot_wait > 0)
		shot_wait--;
	if (actor_wait > 0)
		actor_wait--;
	if (player != NONE && wii_control_fire() && shot_wait == 0 && wii_unit_eye(player, &eye_x, &eye_y, &eye_z)) {
		yaw = wii_control_yaw();
		pitch = wii_control_pitch();
		cp = cosf(pitch);
		sp = sinf(pitch);
		cy = cosf(yaw);
		sy = sinf(yaw);
		spawn_shot(player, eye_x, eye_y, eye_z, cp * cy * 1.2f, cp * sy * 1.2f, sp * 1.2f, 0);
		shot_wait = 6;
	}
	advance_shots();
	actors_fight(player);
	if (player != NONE)
		wii_pose_build(player);
}

int wii_play_moved(void)
{
	return moved;
}

int wii_play_shots(void)
{
	return player_shots;
}

int wii_play_ai_shots(void)
{
	return actor_shots;
}

int wii_play_grounded(void)
{
	return grounded;
}

void wii_sound_impulse(long tag_index)
{
	sound_tag = tag_index;
#ifdef GEKKO
	/* A short click until an Xbox sound tag is decoded into samples. */
	{
		static int ready;
		static AESNDPB *voice;
		static int16_t click[32];
		int sample;

		if (!ready) {
			for (sample = 0; sample < 32; sample++)
				click[sample] = (int16_t)((sample & 2) ? 6000 : -6000);
			AESND_Init();
			voice = AESND_AllocateVoiceWithArg(0, 0);
			ready = 1;
		}
		if (voice)
			AESND_PlayVoice(voice, WII_VOICE_MONO16, click, sizeof(click), 16000.f, 0, 0);
	}
#endif
}

long wii_sound_last(void)
{
	return sound_tag;
}

int wii_save_checkpoint(void)
{
	long unit = wii_game_player_unit();
	struct object_datum *object = unit == NONE ? NULL : object_get(unit);
	FILE *file;

	if (!object)
		return -1;
	save_x = object->object.position.x;
	save_y = object->object.position.y;
	save_z = object->object.position.z;
	save_yaw = atan2f(object->object.forward.j, object->object.forward.i);
	save_valid = 1;
	file = fopen("sd:/halo-ce/save.bin", "wb");
	if (file) {
		fwrite(&save_valid, 4, 1, file);
		fwrite(&save_x, 4, 1, file);
		fwrite(&save_y, 4, 1, file);
		fwrite(&save_z, 4, 1, file);
		fwrite(&save_yaw, 4, 1, file);
		fclose(file);
	}
	return 0;
}

int wii_save_restore(void)
{
	long unit = wii_game_player_unit();
	struct object_datum *object;
	FILE *file = fopen("sd:/halo-ce/save.bin", "rb");

	if (file) {
		fread(&save_valid, 4, 1, file);
		fread(&save_x, 4, 1, file);
		fread(&save_y, 4, 1, file);
		fread(&save_z, 4, 1, file);
		fread(&save_yaw, 4, 1, file);
		fclose(file);
	}
	if (!save_valid || unit == NONE)
		return -1;
	object = object_get(unit);
	if (!object)
		return -1;
	object->object.position.x = save_x;
	object->object.position.y = save_y;
	object->object.position.z = save_z;
	object->object.forward.i = cosf(save_yaw);
	object->object.forward.j = sinf(save_yaw);
	object->object.forward.k = 0.f;
	return 0;
}

int wii_save_valid(void)
{
	return save_valid;
}
