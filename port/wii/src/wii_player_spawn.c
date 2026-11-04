/*
 * The map's biped tag is not fixed up, so player_spawn's object_new reads
 * this stand-in: type biped, no model, an animation graph with no seats.
 * The starting location itself is the scenario's, recorded by wii_sim.
 */
#include "cseries/cseries.h"
#include "game/game_globals.h"
#include "game/players.h"
#include "memory/data.h"
#include "models/model_animation_definitions.h"
#include "models/model_definitions.h"
#include "objects/object_types.h"
#include "units/biped_definitions.h"
#include "wii_tag_cache.h"

#include <string.h>

enum { WII_PLAYER_ACTIONS = 128 };

static struct biped_definition wii_biped;
static struct animation_graph wii_anim;
static struct animation wii_animation;
static struct animation_graph_unit_seat wii_seat;
static struct animation_graph_weapon_class wii_weapon_class;
static struct animation_graph_weapon_type wii_weapon_type;
static struct model wii_model;
static struct model_node wii_node;
static struct game_globals wii_game_globals_tag;
static struct game_globals_player_information wii_player_info;
static unsigned char wii_falling[0x98];
static int wii_tags_ready;
static long wii_player = NONE;

static void ref_none(struct tag_reference *reference)
{
	reference->group_tag = 0;
	reference->name = NULL;
	reference->name_length = 0;
	reference->index = NONE;
}

static void wii_tags_init(void)
{
	if (wii_tags_ready)
		return;
	memset(&wii_biped, 0, sizeof(wii_biped));
	memset(&wii_anim, 0, sizeof(wii_anim));
	memset(&wii_seat, 0, sizeof(wii_seat));
	memset(&wii_weapon_class, 0, sizeof(wii_weapon_class));
	memset(&wii_weapon_type, 0, sizeof(wii_weapon_type));
	memcpy(wii_seat.label, "stand", 6);
	wii_weapon_class.weapon_types.count = 1;
	wii_weapon_class.weapon_types.address = &wii_weapon_type;
	wii_seat.weapon_classes.count = 1;
	wii_seat.weapon_classes.address = &wii_weapon_class;
	wii_anim.unit_seats.count = 1;
	wii_anim.unit_seats.address = &wii_seat;
	memset(&wii_animation, 0, sizeof(wii_animation));
	wii_animation.node_count = 1;
	wii_anim.animations.count = 1;
	wii_anim.animations.address = &wii_animation;
	memset(&wii_game_globals_tag, 0, sizeof(wii_game_globals_tag));
	memset(&wii_player_info, 0, sizeof(wii_player_info));
	wii_biped.object.type = _object_type_biped;
	wii_biped.object.bounding_radius = 0.5f;
	wii_biped.object.render_bounding_radius = 0.5f;
	wii_biped.object.acceleration_scale = 1.f;
	wii_biped.object.forced_shader_permutation_index = NONE;
	ref_none(&wii_biped.object.model);
	wii_biped.object.model.index = 2;
	wii_biped.object.model.group_tag = (unsigned long)'mode';
	memset(&wii_model, 0, sizeof(wii_model));
	memset(&wii_node, 0, sizeof(wii_node));
	wii_node.next_sibling_node_index = NONE;
	wii_node.first_child_node_index = NONE;
	wii_node.parent_node_index = NONE;
	wii_model.nodes.count = 1;
	wii_model.nodes.address = &wii_node;
	ref_none(&wii_biped.object.animation_graph);
	wii_biped.object.animation_graph.index = 1;
	wii_biped.object.animation_graph.group_tag = (unsigned long)'antr';
	ref_none(&wii_biped.object.collision_model);
	ref_none(&wii_biped.object.physics);
	ref_none(&wii_biped.object.modifier_shader);
	ref_none(&wii_biped.object.creation_effect);
	ref_none(&wii_biped.unit.integrated_light_toggle_effect);
	ref_none(&wii_biped.unit.spawned_actor_variant);
	ref_none(&wii_biped.unit.melee_damage);
	wii_biped.unit.grenade_type = NONE;
	wii_biped.unit.default_team = NONE;
	ref_none(&wii_player_info.player_unit);
	wii_player_info.player_unit.index = 0;
	wii_player_info.player_unit.group_tag = (unsigned long)'bipd';
	wii_game_globals_tag.player_information.count = 1;
	wii_game_globals_tag.player_information.address = &wii_player_info;
	/* biped_falling_damage reads this block with no empty check. Damage
	   references stay NONE and the runtime speeds sit above any landing. */
	memset(wii_falling, 0, sizeof(wii_falling));
	{
		int index_offsets[] = { 0x1C, 0x38, 0x48, 0x58, 0x68, 0x78 };
		int index;
		float huge = 1.0e9f;

		for (index = 0; index < 6; index++)
			memcpy(wii_falling + index_offsets[index], &(int){ -1 }, sizeof(int));
		memcpy(wii_falling + 0x8C, &huge, sizeof(huge));
		memcpy(wii_falling + 0x90, &huge, sizeof(huge));
		memcpy(wii_falling + 0x94, &huge, sizeof(huge));
	}
	wii_game_globals_tag.falling_damage.count = 1;
	wii_game_globals_tag.falling_damage.address = wii_falling;
	wii_tags_ready = 1;
}

void *tag_get(long group_tag, long tag_index)
{
	if (wii_tag_cache_ready())
		return wii_tag_cache_get(group_tag, tag_index);
	(void)group_tag;
	wii_tags_init();
	if (tag_index == 0)
		return &wii_biped;
	if (tag_index == 1)
		return &wii_anim;
	if (tag_index == 2)
		return &wii_model;
	return NULL;
}

long tag_get_group_tag(long tag_index)
{
	if (wii_tag_cache_ready())
		return wii_tag_cache_group(tag_index);
	if (tag_index == 0)
		return (long)'bipd';
	if (tag_index == 1)
		return (long)'antr';
	if (tag_index == 2)
		return (long)'mode';
	return 0;
}

struct game_globals *scenario_get_game_globals(void)
{
	if (wii_tag_cache_ready() && wii_tag_cache_globals())
		return wii_tag_cache_globals();
	wii_tags_init();
	return &wii_game_globals_tag;
}

static float control_forward;
static float control_strafe;
static float control_yaw;
static float control_pitch;
static int control_fire;
static int control_set;

void wii_control_set(float forward, float strafe, float yaw, float pitch, int fire)
{
	control_forward = forward;
	control_strafe = strafe;
	control_yaw = yaw;
	control_pitch = pitch;
	control_fire = fire;
	control_set = 1;
}

float wii_control_yaw(void)
{
	return control_yaw;
}

float wii_control_pitch(void)
{
	return control_pitch;
}

boolean update_client_dequeue(struct player_action *actions)
{
	int index;

	if (!actions)
		return FALSE;
	for (index = 0; index < WII_PLAYER_ACTIONS; index++) {
		memset(&actions[index], 0, sizeof(actions[index]));
		actions[index].desired_weapon_index = NONE;
		actions[index].desired_grenade_index = NONE;
		actions[index].desired_zoom_level = NONE;
		if (control_set) {
			actions[index].throttle.i = control_forward;
			actions[index].throttle.j = control_strafe;
			actions[index].desired_facing.yaw = control_yaw;
			actions[index].desired_facing.pitch = control_pitch;
			if (control_fire)
				actions[index].control_flags |= 1u << 11;
		}
	}
	return TRUE;
}

void wii_game_spawn_local_player(void)
{
	wii_tags_init();
	if (wii_player == NONE)
		wii_player = player_new(0, NONE, 0, NULL);
}

extern struct data_array *object_header_data;

int wii_game_live_objects(void)
{
	long index;
	int count;

	if (!object_header_data)
		return 0;
	count = 0;
	index = NONE;
	while ((index = data_next_index(object_header_data, index)) != NONE)
		count++;
	return count;
}

long wii_game_player_unit(void)
{
	struct player_datum *player;

	if (wii_player == NONE)
		return NONE;
	player = player_try_and_get(wii_player);
	if (!player)
		return NONE;
	return player->unit_index;
}

long wii_game_spawn_projectile(void)
{
	/* object_placement_data is 136 bytes in objects.h. position.z is at 32.
	   The hs_shim objects.h hides that struct from this file. */
	unsigned char placement[256];
	long definition;
	float height = 1.f;
	void object_placement_data_new(void *data, long definition_index, long owner_object_index);
	long object_new(void *data);

	definition = wii_tag_cache_projectile_index();
	if (definition == NONE || definition == -1)
		return NONE;
	memset(placement, 0, sizeof(placement));
	object_placement_data_new(placement, definition, NONE);
	memcpy(placement + 32, &height, sizeof(height));
	return object_new(placement);
}
