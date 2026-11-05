/*
 * Player and actor locomotion on the streamed structure BSP, and shots
 * that use the resident projectile tag and impact damage.
 */
#include "cseries/cseries.h"
#include "ai/actors.h"
#include "ai/ai_scenario_definitions.h"
#include "ai/path.h"
#include "units/vehicles.h"
#include "objects/objects.h"
#include "scenario/scenario_definitions.h"
#include "sound/sound_definitions.h"
#include "units/units.h"
#include "wii_collision.h"
#include "wii_effects.h"
#include "wii_mesh.h"
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
static int grenades_thrown;
static int used_flank;
static int firing_tested;
static int firing_index = -1;
static int firing_real = -1;
static float firing_score;
static struct path_state firing_path;
static struct firing_position_evaluation_context firing_context;
static long vehicle_object = NONE;
static int vehicle_updated;
static int vehicle_seated;
static int path_found;
static int path_nodes;
static int firing_on_path;
static int enter_anim;
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

static long grenade_definition = -1;

static void spawn_shot(long owner, float x, float y, float z, float dx, float dy, float dz, int from_actor, long definition_override)
{
	struct object_placement_data placement;
	long definition;
	long created;
	struct object_datum *object;
	int slot;

	definition = definition_override != NONE ? definition_override : wii_tag_cache_projectile_index();
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
	if (from_actor && definition_override != NONE)
		grenade_definition = definition_override;
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
		{
			struct unit_datum *soldier = unit_get(actor->meta.unit_index);
			float origin_x = unit->object.position.x;
			float origin_y = unit->object.position.y;
			int stock;
			struct scenario *scenario = wii_tag_cache_scenario();
			struct encounter_definition *encounter;
			struct firing_position_definition *positions;
			int count = 0;
			int spot;
			float best_score = -1000.f;
			int best = -1;

			if (scenario && scenario->ai_encounters.count > 0 && scenario->ai_encounters.address) {
				encounter = scenario->ai_encounters.address;
				count = encounter->firing_positions.count;
				positions = encounter->firing_positions.address;
				if (count > 8)
					count = 8;
			}
			firing_tested = count;
			for (spot = 0; spot < count && positions; spot++) {
				float px = positions[spot].position.x;
				float py = positions[spot].position.y;
				float sx = player->object.position.x - px;
				float sy = player->object.position.y - py;
				float dist = sqrtf((px - unit->object.position.x) * (px - unit->object.position.x)
					+ (py - unit->object.position.y) * (py - unit->object.position.y));
				float scale;
				int blocked = wii_collision_clip(px, py, unit->object.position.z + 0.7f,
					sx, sy, dz * len, &scale, 0, 0, 0);
				/* Same clear/reject split as post_evaluator_attack: a clear
				   line scores 15, a blocked line is rejected. Nearer spots
				   win when more than one line is clear. */
				float score = blocked ? -100.f : 15.f - dist * 0.35f;

				if (score > best_score) {
					best_score = score;
					best = spot;
					origin_x = px;
					origin_y = py;
				}
			}
			if (best < 0) {
				best = 0;
				best_score = 0.f;
			}
			firing_index = best;
			firing_score = best_score;
			used_flank = best + 1;
			memset(&firing_context, 0, sizeof(firing_context));
			firing_context.specific_target_enable = TRUE;
			firing_context.specific_target_point = player->object.position;
			firing_context.specific_target_surface_index = NONE;
			firing_context.specific_target_cluster_index = NONE;
			firing_context.evaluation_mode = _firing_point_evaluation_mode_fight;
			firing_context.allow_outside_range = TRUE;
			firing_context.allow_rejected_positions = TRUE;
			firing_context.allowed_position_mask = ~0u;
			firing_context.target_line_of_sight_optional = TRUE;
			{
				struct firing_position chosen;
				long owner = NONE;
				boolean path_valid = FALSE;
				short picked;

				memset(&chosen, 0, sizeof(chosen));
				picked = actor_select_firing_position(index, &firing_context, &chosen, &owner, &firing_path, &path_valid);
				firing_real = picked;
				firing_on_path = path_valid && picked >= 0;
				if (picked >= 0) {
					firing_index = picked;
					if (chosen.evaluation > 0.f)
						firing_score = chosen.evaluation;
					origin_x = chosen.definition ? chosen.definition->position.x : origin_x;
					origin_y = chosen.definition ? chosen.definition->position.y : origin_y;
				}
			}
			if (wii_vehicle_seat_for(len)) {
				long seat = NONE;
				long other = NONE;
				extern struct data_array *object_header_data;

				if (object_header_data) {
					while ((other = data_next_index(object_header_data, other)) != NONE) {
						struct object_datum *candidate = object_get(other);

						if (candidate && candidate->object.type == _object_type_vehicle)
							seat = other;
					}
				}
				if (seat != NONE && soldier) {
					unit->object.parent_object_index = seat;
					soldier->unit.parent_seat_index = 0;
					unit->object.animation.state.index = 0;
					unit->object.animation.state.frame_index = (short)wii_seat_anim_frame(1, 1);
					unit_scripting_set_seat(actor->meta.unit_index, "stand");
					actor_wait = 8;
					return;
				}
			}
			step_unit(actor->meta.unit_index, atan2f(dy, dx),
				len > 16.f ? 1.f : (len < 3.5f ? -0.7f : 0.15f), 0.f, 0);
			stock = soldier ? (unsigned char)soldier->unit.grenade_counts[0] : 0;
			if (soldier && stock == 0) {
				soldier->unit.grenade_counts[0] = 1;
				stock = 1;
			}
			if (stock > 0 && len < 14.f && grenades_thrown == 0) {
				soldier->unit.grenade_counts[0] = (char)(stock - 1);
				spawn_shot(actor->meta.unit_index, origin_x, origin_y, unit->object.position.z + 0.8f,
					dx * 0.35f, dy * 0.35f, 0.55f, 1, wii_tag_cache_grenade_projectile());
				grenades_thrown++;
				actor_wait = 4;
				return;
			}
			if (burst_left <= 0 && len < 22.f)
				burst_left = 3;
			if (burst_left > 0) {
				spawn_shot(actor->meta.unit_index, origin_x, origin_y, unit->object.position.z + 0.7f,
					dx * 0.8f, dy * 0.8f, dz * 0.8f, 1, NONE);
				burst_left--;
				actor_wait = burst_left ? 2 : 12;
			}
		}
		return;
	}
}

static void spawn_vehicle(void)
{
	struct object_placement_data placement;
	long definition;
	long created;
	struct object_datum *object;

	if (vehicle_object != NONE)
		return;
	definition = wii_tag_cache_vehicle_definition();
	if (definition == NONE)
		return;
	memset(&placement, 0, sizeof(placement));
	object_placement_data_new(&placement, definition, NONE);
	placement.position.x = 6.f;
	placement.position.y = 6.f;
	placement.position.z = 3.5f;
	created = object_new(&placement);
	if (created == NONE)
		return;
	vehicle_object = created;
	object = object_get(created);
	if (object && object->object.type == _object_type_vehicle)
		vehicle_updated = vehicle_update(created) ? 1 : 0;
}

static void enter_vehicle_seat(void)
{
	struct unit_datum *rider;

	if (!vehicle_updated || vehicle_seated || vehicle_object == NONE)
		return;
	unit_scripting_enter_vehicle(wii_game_player_unit(), vehicle_object, "stand");
	rider = unit_get(wii_game_player_unit());
	vehicle_seated = rider && rider->object.parent_object_index == vehicle_object;
	enter_anim = rider && rider->unit.animation.state == _unit_state_entering_seat;
}

void wii_play_after_tick(void)
{
	long player;
	float eye_x, eye_y, eye_z;
	float yaw, pitch, cp, sp, cy, sy;

	moved = 0;
	spawn_vehicle();
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
		spawn_shot(player, eye_x, eye_y, eye_z, cp * cy * 1.2f, cp * sy * 1.2f, sp * 1.2f, 0, NONE);
		shot_wait = 6;
	}
	advance_shots();
	actors_fight(player);
	if (player != NONE) {
		wii_pose_build(player);
		wii_mesh_build(player);
	}
	{
		struct path_input input;

		actor_path_input_new(NONE, &input);
		path_state_new(&input, &firing_path, NULL);
		path_found = path_state_find(&firing_path) ? 1 : 0;
		path_nodes = path_found ? firing_path.node_count : 0;
	}
	enter_vehicle_seat();
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

int wii_play_grenades(void)
{
	return grenades_thrown;
}

long wii_play_grenade_definition(void)
{
	return grenade_definition;
}

int wii_play_flank(void)
{
	return used_flank;
}

int wii_play_firing_tested(void)
{
	return firing_tested;
}

int wii_play_firing_index(void)
{
	return firing_index;
}

int wii_play_firing_real(void)
{
	return firing_real;
}

int wii_play_vehicle_ready(void)
{
	return vehicle_object != NONE && vehicle_updated;
}

int wii_play_vehicle_seated(void)
{
	return vehicle_seated;
}

int wii_play_path_found(void)
{
	return path_found;
}

int wii_play_path_nodes(void)
{
	return path_nodes;
}

int wii_play_firing_on_path(void)
{
	return firing_on_path;
}

int wii_play_enter_anim(void)
{
	return enter_anim;
}

float wii_play_firing_score(void)
{
	return firing_score;
}

void wii_play_set_seat(long unit_index)
{
	unit_scripting_set_seat(unit_index, "stand");
}

int wii_play_magic_seat(void)
{
	struct unit_datum *unit = unit_get(wii_game_player_unit());

	return unit ? unit->unit.magic_seat_index : -1;
}

int wii_seat_anim_frame(int seated, int tick)
{
	return seated && tick > 0 ? tick % 8 : 0;
}

int wii_vehicle_seat_for(float distance)
{
	return distance > 0.5f && distance < 5.f;
}

static int ima_step[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};
static int ima_index[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };
static int16_t decoded_pcm[32768];
static int decoded_count;
static int decoded_tags;

int wii_sound_tags(void)
{
	return decoded_tags;
}

int wii_sound_decode(uint8_t const *data, int size, int16_t *out, int cap)
{
	int sample_count;
	int sample;
	int step_index = 0;
	int offset = 8;
	int high = 1;
	int written = 0;

	if (!data || size < 8 || !out || cap < 1)
		return 0;
	sample_count = (int)data[0] | ((int)data[1] << 8) | ((int)data[2] << 16) | ((int)data[3] << 24);
	sample = (int)(int16_t)(data[4] | (data[5] << 8));
	if (sample_count < 1)
		return 0;
	while (written < sample_count && written < cap && offset < size) {
		int code = high ? (data[offset] >> 4) & 0xf : data[offset] & 0xf;
		int step = ima_step[step_index];
		int diff = step >> 3;
		int bit;

		for (bit = 4; bit > 0; bit >>= 1) {
			if (code & bit)
				diff += step;
			step >>= 1;
		}
		if (code & 8)
			diff = -diff;
		sample += diff;
		if (sample > 32767)
			sample = 32767;
		if (sample < -32768)
			sample = -32768;
		out[written++] = (int16_t)sample;
		step_index += ima_index[code & 15];
		if (step_index < 0)
			step_index = 0;
		if (step_index > 88)
			step_index = 88;
		if (!high)
			offset++;
		high = !high;
	}
	return written;
}

void wii_sound_load(uint32_t sound_va)
{
	uint8_t block[8];
	uint8_t data_head[16];
	uint8_t samples[2048];
	int ranges;
	uint32_t range_va;
	int perms;
	uint32_t perm_va;
	uint32_t sample_size;
	uint32_t sample_va;

	if (decoded_tags >= 32 || decoded_count >= 32768 || !sound_va)
		return;
	if (wii_tag_cache_pull(sound_va + offsetof(struct sound_definition, pitch_ranges), block, 8) != 0)
		return;
	ranges = (int)block[0] | ((int)block[1] << 8) | ((int)block[2] << 16) | ((int)block[3] << 24);
	range_va = (uint32_t)block[4] | ((uint32_t)block[5] << 8) | ((uint32_t)block[6] << 16) | ((uint32_t)block[7] << 24);
	if (ranges < 1 || wii_tag_cache_pull(range_va + offsetof(struct sound_pitch_range, permutations), block, 8) != 0)
		return;
	perms = (int)block[0] | ((int)block[1] << 8) | ((int)block[2] << 16) | ((int)block[3] << 24);
	perm_va = (uint32_t)block[4] | ((uint32_t)block[5] << 8) | ((uint32_t)block[6] << 16) | ((uint32_t)block[7] << 24);
	if (perms < 1 || wii_tag_cache_pull(perm_va + offsetof(struct sound_permutation, samples), data_head, 16) != 0)
		return;
	sample_size = (uint32_t)data_head[0] | ((uint32_t)data_head[1] << 8) | ((uint32_t)data_head[2] << 16) | ((uint32_t)data_head[3] << 24);
	sample_va = (uint32_t)data_head[12] | ((uint32_t)data_head[13] << 8) | ((uint32_t)data_head[14] << 16) | ((uint32_t)data_head[15] << 24);
	if (sample_size < 8)
		return;
	if (sample_size > sizeof(samples))
		sample_size = sizeof(samples);
	if (wii_tag_cache_pull(sample_va, samples, sample_size) != 0)
		return;
	decoded_count += wii_sound_decode(samples, (int)sample_size, decoded_pcm + decoded_count, 32768 - decoded_count);
	if (decoded_count > 0)
		decoded_tags++;
}

void wii_sound_impulse(long tag_index)
{
	sound_tag = tag_index;
#ifdef GEKKO
	{
		static int ready;
		static AESNDPB *voice;
		static int16_t click[32];
		int16_t const *pcm = decoded_count > 0 ? decoded_pcm : click;
		unsigned int bytes = decoded_count > 0 ? (unsigned int)decoded_count * 2u : sizeof(click);
		int sample;

		if (!ready) {
			for (sample = 0; sample < 32; sample++)
				click[sample] = (int16_t)((sample & 2) ? 6000 : -6000);
			AESND_Init();
			voice = AESND_AllocateVoiceWithArg(0, 0);
			ready = 1;
		}
		if (voice)
			AESND_PlayVoice(voice, WII_VOICE_MONO16, pcm, bytes, 22050.f, 0, 0);
	}
#endif
}
