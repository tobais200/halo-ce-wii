#include <string.h>

float sqrtf(float);

/* Same 16-byte layout as real_quaternion: i, j, k, w. */
struct wii_quaternion {
	float i, j, k, w;
};

void quaternions_multiply(struct wii_quaternion const *q0, struct wii_quaternion const *q1, struct wii_quaternion *result)
{
	struct wii_quaternion temporary;

	if (q0 == result) {
		temporary = *q0;
		q0 = &temporary;
	}
	if (q1 == result) {
		temporary = *q1;
		q1 = &temporary;
	}
	result->i = q0->j * q1->k + q0->i * q1->w + q0->w * q1->i - q0->k * q1->j;
	result->j = q0->w * q1->j + q0->j * q1->w + q0->k * q1->i - q0->i * q1->k;
	result->k = q0->k * q1->w + q0->i * q1->j + q0->w * q1->k - q0->j * q1->i;
	result->w = q0->w * q1->w - q0->i * q1->i - q0->j * q1->j - q0->k * q1->k;
}

void quaternions_interpolate(struct wii_quaternion const *q0, struct wii_quaternion const *q1, float t, struct wii_quaternion *result)
{
	float v = 1.f - t;

	if (q0->i * q1->i + q0->j * q1->j + q0->k * q1->k + q0->w * q1->w < 0.f)
		t = -t;
	result->i = q0->i * v + q1->i * t;
	result->j = q0->j * v + q1->j * t;
	result->k = q0->k * v + q1->k * t;
	result->w = q0->w * v + q1->w * t;
}

void quaternion_normalize(struct wii_quaternion *q)
{
	float magnitude_squared = q->i * q->i + q->j * q->j + q->k * q->k + q->w * q->w;

	if (magnitude_squared > 0.f) {
		float scale = 1.f / sqrtf(magnitude_squared);

		q->i *= scale;
		q->j *= scale;
		q->k *= scale;
		q->w *= scale;
	} else {
		q->i = 0.f;
		q->j = 0.f;
		q->k = 0.f;
		q->w = 1.f;
	}
}

void quaternions_interpolate_and_normalize(struct wii_quaternion const *q0, struct wii_quaternion const *q1, float t, struct wii_quaternion *result)
{
	quaternions_interpolate(q0, q1, t, result);
	quaternion_normalize(result);
}
short global_structure_bsp_index;
float global_gravity;
short global_current_collision_user_depth;
short global_current_collision_users[32];
void *conversation_data;
static float wii_zero3[3];
static float wii_up3[3] = {0.f, 0.f, 1.f};
static float wii_down3[3] = {0.f, 0.f, -1.f};
static float wii_fwd3[3] = {1.f, 0.f, 0.f};
static float wii_left3[3] = {0.f, 1.f, 0.f};
static float wii_right3[3] = {0.f, -1.f, 0.f};
static float wii_origin3[3];
static float wii_fwd2[2] = {1.f, 0.f};
static float wii_zero2[2];
static float wii_quat[4] = {0.f, 0.f, 0.f, 1.f};
static float wii_angles[2];
static float wii_color[4] = {1.f, 1.f, 1.f, 1.f};
const float *global_zero_vector3d = wii_zero3;
const float *global_up3d = wii_up3;
const float *global_down3d = wii_down3;
const float *global_forward3d = wii_fwd3;
const float *global_left3d = wii_left3;
const float *global_right3d = wii_right3;
const float *global_origin3d = wii_origin3;
const float *global_forward2d = wii_fwd2;
const float *global_zero_vector2d = wii_zero2;
const float *global_identity_quaternion = wii_quat;
const float *global_zero_angles2d = wii_angles;
const float *global_real_argb_aqua = wii_color;
const float *global_real_argb_black = wii_color;
const float *global_real_argb_cyan = wii_color;
const float *global_real_argb_darkgreen = wii_color;
const float *global_real_argb_green = wii_color;
const float *global_real_argb_grey = wii_color;
const float *global_real_argb_lightblue = wii_color;
const float *global_real_argb_magenta = wii_color;
const float *global_real_argb_orange = wii_color;
const float *global_real_argb_purple = wii_color;
const float *global_real_argb_salmon = wii_color;
const float *global_real_rgb_white = wii_color;
int accelerate_to_position(void) { return 0; }
int action_alert_begin(void) { return 0; }
int action_alert_control(void) { return 0; }
int action_alert_flush_position_indices(void) { return 0; }
int action_alert_flush_structure_indices(void) { return 0; }
int action_alert_perform(void) { return 0; }
int action_alert_setup(void) { return 0; }
int action_alert_update(void) { return 0; }
int action_avoid_begin(void) { return 0; }
int action_avoid_control(void) { return 0; }
int action_avoid_end(void) { return 0; }
int action_avoid_perform(void) { return 0; }
int action_avoid_setup(void) { return 0; }
int action_avoid_update(void) { return 0; }
int action_charge_begin(void) { return 0; }
int action_charge_control(void) { return 0; }
int action_charge_is_leaping(void) { return 0; }
int action_charge_perform(void) { return 0; }
int action_charge_setup(void) { return 0; }
int action_charge_update(void) { return 0; }
int action_converse_begin(void) { return 0; }
int action_converse_control(void) { return 0; }
int action_converse_end(void) { return 0; }
int action_converse_perform(void) { return 0; }
int action_converse_replace_prop(void) { return 0; }
int action_converse_setup(void) { return 0; }
int action_converse_update(void) { return 0; }
int action_fight_begin(void) { return 0; }
int action_fight_control(void) { return 0; }
int action_fight_end(void) { return 0; }
int action_fight_perform(void) { return 0; }
int action_fight_setup(void) { return 0; }
int action_fight_update(void) { return 0; }
int action_flee_begin(void) { return 0; }
int action_flee_control(void) { return 0; }
int action_flee_end(void) { return 0; }
int action_flee_flush_position_indices(void) { return 0; }
int action_flee_modify_color(void) { return 0; }
int action_flee_perform(void) { return 0; }
int action_flee_replace_prop(void) { return 0; }
int action_flee_setup(void) { return 0; }
int action_flee_update(void) { return 0; }
int action_guard_begin(void) { return 0; }
int action_guard_control(void) { return 0; }
int action_guard_end(void) { return 0; }
int action_guard_flush_position_indices(void) { return 0; }
int action_guard_flush_structure_indices(void) { return 0; }
int action_guard_modify_color(void) { return 0; }
int action_guard_perform(void) { return 0; }
int action_guard_replace_prop(void) { return 0; }
int action_guard_setup_current_position(void) { return 0; }
int action_guard_setup_find_position(void) { return 0; }
int action_guard_setup_from_combat_transition(void) { return 0; }
int action_guard_setup_from_fleeing(void) { return 0; }
int action_guard_setup_postcombat(void) { return 0; }
int action_guard_update(void) { return 0; }
int action_obey_advance_command_list(void) { return 0; }
int action_obey_begin(void) { return 0; }
int action_obey_command_list_setup(void) { return 0; }
int action_obey_control(void) { return 0; }
int action_obey_end(void) { return 0; }
int action_obey_flush_command_indices(void) { return 0; }
int action_obey_perform(void) { return 0; }
int action_obey_update(void) { return 0; }
int action_search_begin(void) { return 0; }
int action_search_control(void) { return 0; }
int action_search_flush_position_indices(void) { return 0; }
int action_search_perform(void) { return 0; }
int action_search_setup_pursuit(void) { return 0; }
int action_search_setup_target(void) { return 0; }
int action_search_setup_undirected(void) { return 0; }
int action_search_update(void) { return 0; }
int action_sleep_control(void) { return 0; }
int action_uncover_begin(void) { return 0; }
int action_uncover_control(void) { return 0; }
int action_uncover_flush_position_indices(void) { return 0; }
int action_uncover_modify_color(void) { return 0; }
int action_uncover_perform(void) { return 0; }
int action_uncover_setup_pursuit(void) { return 0; }
int action_uncover_setup_target(void) { return 0; }
int action_uncover_update(void) { return 0; }
int action_vehicle_begin(void) { return 0; }
int action_vehicle_control(void) { return 0; }
int action_vehicle_perform(void) { return 0; }
int action_vehicle_setup_impromptu(void) { return 0; }
int action_vehicle_setup_specific(void) { return 0; }
int action_vehicle_update(void) { return 0; }
int action_wait_begin(void) { return 0; }
int action_wait_control(void) { return 0; }
int action_wait_perform(void) { return 0; }
int action_wait_setup(void) { return 0; }
int action_wait_update(void) { return 0; }
int actor_aim_jump(void) { return 0; }
int actor_audibility_at_point(void) { return 0; }
int actor_berserk(void) { return 0; }
int actor_compute_prop_target_weight(void) { return 0; }
int actor_compute_prop_unopposable(void) { return 0; }
int actor_conversation_control(void) { return 0; }
/* One debug record is 0x657c. The first actors use the low slots. */
static unsigned char wii_actor_debug[4 * 0x657c];
void *actor_debug_array = wii_actor_debug;
int actor_destination_update(void) { return 0; }
int actor_emotion_update(void) { return 0; }
int actor_expected_acknowledgement(void) { return 0; }
int actor_get_perception_knowledge(void) { return 0; }
int actor_look_affect_movement(void) { return 0; }
int actor_look_secondary(void) { return 0; }
int actor_look_secondary_stop(void) { return 0; }
int actor_look_update(void) { return 0; }
int actor_move_animation_busy(void) { return 0; }
int actor_move_animation_impulse(void) { return 0; }
int actor_move_halt(void) { return 0; }
int actor_move_initialize(void) { return 0; }
int actor_move_try_evasion_direction(void) { return 0; }
int actor_move_try_evasion_vector(void) { return 0; }
int actor_move_update(void) { return 0; }
int actor_path_at_destination(void) { return 0; }
int actor_path_has_path(void) { return 0; }
int actor_perception_abandoned_search(void) { return 0; }
int actor_perception_acknowledge(void) { return 0; }
int actor_perception_become_acknowledged(void) { return 0; }
int actor_perception_create_orphan_from_friend(void) { return 0; }
int actor_perception_desire_prop(void) { return 0; }
int actor_perception_find_prop_pathfinding_location(void) { return 0; }
int actor_perception_find_recent_damaging_prop_index(void) { return 0; }
int actor_perception_find_sense_position(void) { return 0; }
int actor_perception_tried_to_search(void) { return 0; }
int actor_perception_tried_to_uncover(void) { return 0; }
int actor_perception_update(void) { return 0; }
int actor_situation_try_new_target(void) { return 0; }
int actor_situation_update(void) { return 0; }
int actor_stimulus_bumped(void) { return 0; }
int actor_stimulus_clear(void) { return 0; }
int actor_stimulus_damage(void) { return 0; }
int actor_stimulus_enter_combat_friend_in_combat(void) { return 0; }
int actor_stimulus_environmental_noise(void) { return 0; }
int actor_stimulus_heard_shooting(void) { return 0; }
int actor_stimulus_maneuvering(void) { return 0; }
int actor_stimulus_prop_fleeing(void) { return 0; }
int actor_stimulus_prop_just_killed(void) { return 0; }
int actor_stimulus_vehicle_eviction(void) { return 0; }
int actor_stimulus_was_surprised(void) { return 0; }
int actor_stimulus_weapon_detonation(void) { return 0; }
int actor_stimulus_weapon_impact(void) { return 0; }
int actor_type_decide_action(void) { return 0; }
int actor_type_get_name(void) { return 0; }
int actor_type_get_pursuit_controller(void) { return 0; }
int actor_type_get_race(void) { return 0; }
int actor_type_get_swarm(void) { return 0; }
int actor_type_get_when_to_pursue(void) { return 0; }
int actor_type_get_when_to_search_at_target(void) { return 0; }
int actor_type_get_when_to_search_pursuit(void) { return 0; }
int actor_type_initialize(void) { return 0; }
int actor_types_initialize(void) { return 0; }
int actor_type_swarm_control(void) { return 0; }
int actor_visibility_at_point(void) { return 0; }
/* selected_squad_index is the int at 0x34. NONE so spawn uses scenario->players. */
int ai_debug[32] = { [13] = -1 };
int ai_debug_actor_deleted(void) { return 0; }
int ai_debug_describe_actor(void) { return 0; }
int ai_debug_dispose(void) { return 0; }
int ai_debug_dispose_from_old_map(void) { return 0; }
int ai_debug_initialize(void) { return 0; }
int ai_debug_initialize_for_new_map(void) { return 0; }
int ai_debug_lineoffire_addpill(void) { return 0; }
int ai_debug_lineoffire_new(void) { return 0; }
int ai_debug_lineoffire_success(void) { return 0; }
int ai_debug_lineofsight(void) { return 0; }
int ai_debug_select_actor(void) { return 0; }
int ai_debug_select_encounter(void) { return 0; }
int ai_debug_update(void) { return 0; }
void *ai_globals;
void *ai_profile;
int ai_profile_dispose(void) { return 0; }
int ai_profile_dispose_from_old_map(void) { return 0; }
int ai_profile_initialize(void) { return 0; }
int ai_profile_initialize_for_new_map(void) { return 0; }
int ai_profile_update(void) { return 0; }
int angle_between_vectors3d(void) { return 0; }
int angular_accelerate_to_position(void) { return 0; }
int biped_limp_noodle_get_max_relaxation_iterations(void) { return 0; }
int biped_limp_noodle_relax_nodes_onto_environment(void) { return 0; }
int bit_vector_and(void) { return 0; }
int bit_vector_or(void) { return 0; }
int bots_dispose_from_old_map(void) { return 0; }
int bots_initialize_for_new_map(void) { return 0; }
int bots_update_actions(void) { return 0; }
int breakable_surface_damage(void) { return 0; }
int breakable_surface_damage_area_of_effect(void) { return 0; }
static unsigned char wii_breakable_surface_flags[256];
void *breakable_surface_flags_get(void) { return wii_breakable_surface_flags; }
void *cheat;
int cinematic_in_progress(void) { return 0; }
int cluster_partition_delete(void) { return 0; }
int cluster_partition_disconnect(void) { return 0; }
int cluster_partition_get_first_cluster(void) { return 0; }
int cluster_partition_get_first_datum(void) { return 0; }
int cluster_partition_get_next_cluster(void) { return 0; }
int cluster_partition_get_next_datum(void) { return 0; }
int cluster_partition_make_invalid(void) { return 0; }
int cluster_partition_make_valid(void) { return 0; }
int cluster_partition_new(void) { return 0; }
int cluster_partition_reconnect(void) { return 0; }
void *collision_debug_height;
void *collision_debug_length;
void *collision_debug_point;
void *collision_debug_repeat;
void *collision_debug_vector;
void *collision_debug_width;
int collision_test_line(void) { return 0; }
int component_vectors_from_direction3d(void) { return 0; }
int component_vectors_from_normal3d(void) { return 0; }
int config_boolean(void) { return 0; }
int config_changes(void) { return 0; }
int coop_enemies_extra_count(void) { return 0; }
int coop_enemies_fallback_position(void) { return 0; }
int coop_enemies_rider_seated(void) { return 0; }
int coop_enemies_rider_unseated(void) { return 0; }
int coop_enemies_spread_position(void) { return 0; }
int coop_pickups_register(void) { return 0; }
int coop_scripts_board_followers(void) { return 0; }
int coop_scripts_exit_followers(void) { return 0; }
int coop_scripts_suspend_followers(void) { return 0; }
int coop_vehicles_register(void) { return 0; }
int csmemmove(void) { return 0; }
int csstrcpy(void) { return 0; }
int csstrncmp(void) { return 0; }
int csstrtok(void) { return 0; }
int debug_objects_biped_autoaim_pills(void) { return 0; }
int debug_objects_biped_physics_pills(void) { return 0; }
int debug_objects_vehicle_powered_mass_points(void) { return 0; }
void *depths_of_hell;
int device_can_change_position(void) { return 0; }
int device_frontfacing(void) { return 0; }
int device_touched(void) { return 0; }
int dialogue_get_vocalization_name(void) { return 0; }
int director_get_perspective(void) { return 0; }
int director_inhibit_input(void) { return 0; }
int draw_string_set_color(void) { return 0; }
int draw_string_set_format(void) { return 0; }
int effect_delete(void) { return 0; }
int effects_start_on_first_person_weapon(void) { return 0; }
int effects_stop_on_first_person_weapon(void) { return 0; }
int effect_stop(void) { return 0; }
int encounter_definition_get_platoon_by_name(void) { return 0; }
int encounter_definition_get_squad_by_name(void) { return 0; }
int euler_angles2d_from_vector3d(void) { return 0; }
int fast_vector_intersection_with_sphere(void) { return 0; }
int fast_vector_intersects_sphere(void) { return 0; }
int first_person_legs_reset(void) { return 0; }
int rasterizer_transparent_geometry_is_enclosure(void) { return 0; }
int game_connection(void) { return 0; }
float game_difficulty_get_team_value(short value_type, short team_index)
{
	(void)value_type;
	(void)team_index;
	return 1.f;
}
float game_difficulty_get_value(short value_type)
{
	(void)value_type;
	return 1.f;
}
int game_looping_sound_delete(void) { return 0; }
int game_looping_sound_new(void) { return 0; }
int game_modes_headshot_kills(void) { return 0; }
int game_modes_kill_by_melee(void) { return 0; }
int game_statistics_record_damage(void) { return 0; }
int game_statistics_record_kill(void) { return 0; }
int game_time_menu_scene(void) { return 0; }
int game_time_reset_speed(void) { return 0; }
int get_global_local_random_seed_address(void) { return 0; }
/* No structure tag is resident. A zeroed bsp keeps objects_update's cluster
walk at count 0 until the map's sbsp is fixed up. */
static unsigned char wii_structure_bsp[0x1000];
void *global_structure_bsp_get(void) { return wii_structure_bsp; }
int global_scenario_try_and_get(void) { return 0; }
int global_structure_bsp_index_get(void) { return 0; }
int halo_interpolation_enabled(void) { return 0; }
int halo_model_detail_scale(void) { return 0; }
int hs_compile_and_evaluate(void) { return 0; }
int hud_fix_unit_data(void) { return 0; }
int hud_fix_weapon_data(void) { return 0; }
int hud_picked_up_ammunition(void) { return 0; }
int hud_picked_up_grenade(void) { return 0; }
int hud_picked_up_powerup(void) { return 0; }
int hud_picked_up_weapon(void) { return 0; }
int hud_print_message(void) { return 0; }
int hud_tick_shield(void) { return 0; }
int infection_allow_weapon_drop(void) { return 0; }
int infection_can_delete_skull(void) { return 0; }
int infection_melee_multiplier(void) { return 0; }
int infection_melee_rider_target(void) { return 0; }
int infection_reconcile_player(void) { return 0; }
int infection_unit_is_zombie(void) { return 0; }
int input_get_gamepad_state(void) { return 0; }
int input_has_gamepad(void) { return 0; }
int input_key_is_down(void) { return 0; }
int item_accelerate(void) { return 0; }
int item_detonate(void) { return 0; }
int item_in_unit_inventory(void) { return 0; }
int light_delete(void) { return 0; }
int light_disconnect_from_map(void) { return 0; }
int light_new(void) { return 0; }
int light_reconnect_to_map(void) { return 0; }
int lights_dispose(void) { return 0; }
int lights_dispose_from_old_map(void) { return 0; }
int lights_illumination_at_point(void) { return 0; }
int lights_initialize(void) { return 0; }
int lights_initialize_for_new_map(void) { return 0; }
int machine_get_elevator_position(void) { return 0; }
int machine_move_elevator_riders(void) { return 0; }
int machine_try_to_open_with_damage(void) { return 0; }
int main_lost_map(void) { return 0; }
int main_menu_is_active(void) { return 0; }
int main_respawn(void) { return 0; }
int main_switch_structure_bsp(void) { return 0; }
int material_effect_new(void) { return 0; }
int material_effect_new_from_point(void) { return 0; }
int material_effect_visible(void) { return 0; }
int material_get_name(void) { return 0; }
int matrix3x3_from_forward_and_up(void) { return 0; }
int matrix3x3_multiply(void) { return 0; }
int matrix3x3_rotation_to_quaternion(void) { return 0; }
int matrix3x3_transpose(void) { return 0; }
int matrix4x3_from_orientation(void) { return 0; }
int matrix4x3_from_point_and_quaternion(void) { return 0; }
int matrix4x3_identity(void) { return 0; }
struct wii_matrix4x3 {
	float scale;
	float n[4][3];
};
void matrix4x3_inverse(struct wii_matrix4x3 const *matrix, struct wii_matrix4x3 *result)
{
	float x;
	float y;
	float z;
	float temp;

	if (!matrix || !result || matrix->scale == 0.f) {
		if (result)
			memset(result, 0, sizeof(*result));
		return;
	}
	x = -matrix->n[3][0];
	y = -matrix->n[3][1];
	z = -matrix->n[3][2];
	if (matrix->scale != 1.f) {
		float scale = 1.f / matrix->scale;

		result->scale = scale;
		x *= scale;
		y *= scale;
		z *= scale;
	} else {
		result->scale = 1.f;
	}
	result->n[0][0] = matrix->n[0][0];
	result->n[1][1] = matrix->n[1][1];
	result->n[2][2] = matrix->n[2][2];
	temp = matrix->n[1][0];
	result->n[1][0] = matrix->n[0][1];
	result->n[0][1] = temp;
	temp = matrix->n[2][0];
	result->n[2][0] = matrix->n[0][2];
	result->n[0][2] = temp;
	temp = matrix->n[2][1];
	result->n[2][1] = matrix->n[1][2];
	result->n[1][2] = temp;
	result->n[3][0] = x * result->n[0][0] + y * result->n[1][0] + z * result->n[2][0];
	result->n[3][1] = x * result->n[0][1] + y * result->n[1][1] + z * result->n[2][1];
	result->n[3][2] = x * result->n[0][2] + y * result->n[1][2] + z * result->n[2][2];
}
struct wii_point3 { float x, y, z; };
struct wii_point3 *matrix4x3_transform_point(struct wii_matrix4x3 const *matrix, struct wii_point3 const *point, struct wii_point3 *result)
{
	float x = point->x;
	float y = point->y;
	float z = point->z;

	if (matrix->scale != 1.f) {
		x *= matrix->scale;
		y *= matrix->scale;
		z *= matrix->scale;
	}
	result->x = matrix->n[2][0] * z + matrix->n[1][0] * y + matrix->n[0][0] * x + matrix->n[3][0];
	result->y = matrix->n[2][1] * z + matrix->n[1][1] * y + matrix->n[0][1] * x + matrix->n[3][1];
	result->z = matrix->n[2][2] * z + matrix->n[1][2] * y + matrix->n[0][2] * x + matrix->n[3][2];
	return result;
}
int matrix4x3_inverse_transform_normal(void) { return 0; }
int matrix4x3_inverse_transform_vector(void) { return 0; }
int matrix4x3_multiply(void) { return 0; }
int matrix4x3_rotation_from_axis_and_angle(void) { return 0; }
int matrix4x3_rotation_from_vectors(void) { return 0; }
int matrix4x3_rotation_to_quaternion(void) { return 0; }
int matrix4x3_to_point_and_vectors(void) { return 0; }
int matrix4x3_transform_normal(void) { return 0; }
int matrix4x3_transform_plane(void) { return 0; }
int matrix4x3_translation(void) { return 0; }
int model_detail_geometry_index(void) { return 0; }
int model_detail_rules(void) { return 0; }
int network_actors_note_control(void) { return 0; }
int network_actors_note_impulse(void) { return 0; }
int network_actors_note_leap(void) { return 0; }
int network_actors_note_melee(void) { return 0; }
int network_actors_note_user_animation(void) { return 0; }
int network_coop_bsp_switch_cooldown_ticks(void) { return 0; }
int network_coop_bsp_switch_majority(void) { return 0; }
int network_coop_devices_remote(void) { return 0; }
int network_coop_host_dedicated(void) { return 0; }
int network_coop_note_attach(void) { return 0; }
int network_coop_note_bsp_switch(void) { return 0; }
int network_coop_note_detach(void) { return 0; }
int network_coop_note_unit_animation(void) { return 0; }
int network_coop_note_unit_animation_frame(void) { return 0; }
int network_coop_note_unit_open(void) { return 0; }
int network_coop_player_away(void) { return 0; }
int network_coop_player_collisions(void) { return 0; }
int network_coop_player_has_structure_bsp(void) { return 0; }
int network_coop_tell_bsp_switch_waiting(void) { return 0; }
int network_coop_vehicle_dropped(void) { return 0; }
int network_damage_aftermath(void) { return 0; }
int network_damage_deals(void) { return 1; }
int network_damage_note_grenade(void) { return 0; }
int network_damage_player_effect(void) { return 0; }
int network_damage_replaying_kill(void) { return 0; }
int network_damage_static_target(void) { return 0; }
int network_distributed_player_picked_up(void) { return 0; }
int network_objects_client_deleting(void) { return 0; }
int network_objects_client_picked_up_weapon(void) { return 0; }
int network_objects_creating_host_object(void) { return 0; }
int network_objects_damage_animation(void) { return 0; }
int network_objects_may_delete(void) { return 0; }
int normalize3d(void) { return 0; }
int object_bounds_cache_update(void) { return 0; }
int object_deleted_procs_call(void) { return 0; }
int object_get_cached_render_lighting(void) { return 0; }
int object_get_self_illumination(void) { return 0; }
int object_impulse_sound_new(void) { return 0; }
int object_list_get_first(void) { return 0; }
int object_list_get_next(void) { return 0; }
int observer_get_camera(void) { return 0; }
int observer_obsolete_position(void) { return 0; }
int orientations_interpolate(void) { return 0; }
int overshield_effect_active(void) { return 0; }
int overshield_effect_hit_effect_allowed(void) { return 0; }
int overshield_effect_values(void) { return 0; }
int pal_tags_first_person_advance(void) { return 0; }
int pal_tags_first_person_fraction(void) { return 0; }
int pal_tags_first_person_frames(void) { return 0; }
int particles_stop_on_first_person_weapon(void) { return 0; }
int particle_system_new_attached(void) { return 0; }
int particle_system_orphan(void) { return 0; }
int path_avoid_obstacles(void) { return 0; }
int path_smooth(void) { return 0; }
int periodic_function_evaluate(void) { return 0; }
int perpendicular3d(void) { return 0; }
int physics_instance_new(void) { return 0; }
int physics_update(void) { return 0; }
int physics_variable_position_update_seek(void) { return 0; }
int physics_variable_speed_update_seek(void) { return 0; }
int pitch_vectors(void) { return 0; }
int player_aim_projectile(void) { return 0; }
int player_aim_projectile_preview(void) { return 0; }
void *player_autoaim_flag;
int player_control_dispose(void) { return 0; }
int player_control_get_autoaim_level(void) { return 0; }
int player_control_get_unit_index(void) { return 0; }
int player_control_get_zoom_level(void) { return 0; }
int player_control_initialize(void) { return 0; }
int player_control_new_unit(void) { return 0; }
int player_control_set_desired_weapon(void) { return 0; }
int player_control_set_facing(void) { return 0; }
int player_control_unzoom(void) { return 0; }
int player_effect_screen_flash(void) { return 0; }
int player_effect_start(void) { return 0; }
void *player_magnetism_flag;
int player_telefrag_effect_start(void) { return 0; }
int player_telefrag_effect_stop(void) { return 0; }
int player_ui_get_single_player_local_player_controller(void) { return 0; }
int point_from_line3d(void) { return 0; }
int points_interpolate(void) { return 0; }
int point_to_line_distance_squared3d(void) { return 0; }
int predicted_resources_precache(void) { return 0; }
int profile_enter_private(void) { return 0; }
int profile_exit_private(void) { return 0; }
void *profile_global_enable;
void *prop_data;
int prop_position_refresh(void) { return 0; }
int prop_status_refresh(void) { return 0; }
int quantize_real_to_byte_lower_bound(void) { return 0; }
int quaternion_to_angle_and_vector(void) { return 0; }
int rasterizer_debug_model_vertices(void) { return 0; }
int rasterizer_debug_options(void) { return 0; }
int rasterizer_draw_string(void) { return 0; }
int rasterizer_environment_shadow_model_begin(void) { return 0; }
int rasterizer_environment_shadow_model_draw(void) { return 0; }
int rasterizer_environment_shadow_model_end(void) { return 0; }
int rasterizer_memory_alloc(void) { return 0; }
int rasterizer_model_begin(void) { return 0; }
int rasterizer_model_cortana_hack(void) { return 0; }
int rasterizer_model_draw(void) { return 0; }
int rasterizer_model_end(void) { return 0; }
int rasterizer_model_transparent_geometry_submit(void) { return 0; }
int real_local_random(void) { return 0; }
int real_local_random_range(void) { return 0; }
int real_random_range(void) { return 0; }
int real_rgb_color_brightness(void) { return 0; }
int recorded_animation_controlling_unit(void) { return 0; }
int reflect_vector3d(void) { return 0; }
int render_debug_line(void) { return 0; }
int render_debug_matrix(void) { return 0; }
int render_debug_physics(void) { return 0; }
int render_debug_pill(void) { return 0; }
int render_debug_point(void) { return 0; }
int render_debug_sphere(void) { return 0; }
int render_debug_vector(void) { return 0; }
int render_interpolation_first_person(void) { return 0; }
int render_interpolation_fraction(void) { return 0; }
int render_interpolation_object_node_matrices(void) { return 0; }
int render_model_index_counts(void) { return 0; }
void *render_model_markers;
void *render_model_nodes;
int render_model_no_geometry(void) { return 0; }
int render_model_vertex_counts(void) { return 0; }
int rgb_colors_interpolate(void) { return 0; }
int rintf(void) { return 0; }
int rotate_vector_about_axis(void) { return 0; }
int rumble_player_impulse(void) { return 0; }
int scalars_interpolate(void) { return 0; }
int scenario_ensure_point_within_world(void) { return 0; }
int scenario_fog_at_point(void) { return 0; }
int scenario_get_encounter_by_name(void) { return 0; }
int scenario_leaf_index_from_point(void) { return 0; }
int scenario_location_from_point(void) { return 0; }
int scenario_location_underwater(void) { return 0; }
int scenario_location_water_depth(void) { return 0; }
int scenario_material_definition_get(void) { return 0; }
int scenario_test_pvs(void) { return 0; }
int scenario_trigger_volume_test_object(void) { return 0; }
int scenario_trigger_volume_test_point(void) { return 0; }
int scripted_sound_new(void) { return 0; }
int scripted_sound_time(void) { return 0; }
int seed_random_direction3d(void) { return 0; }
int seed_random_vector_in_cone3d(void) { return 0; }
int shader_get_and_verify_type(void) { return 0; }
int shader_type_is_transparent(void) { return 0; }
int shader_type_is_valid_for_model(void) { return 0; }
int shader_type_is_valid_for_modifier(void) { return 0; }
int shield_color_colors(void) { return 0; }
int signed_angle_between_vectors2d(void) { return 0; }
int signed_angular_difference(void) { return 0; }
int sound_definition_get_maximum_distance(void) { return 0; }
int sound_scripted_dialog_is_playing(void) { return 0; }
int sound_stop_impulse(void) { return 0; }
int strlwr(void) { return 0; }
int structure_bsp_get_cluster_encoded_sound_distance(void) { return 0; }
int structure_bsp_get_cluster_pvs(void) { return 0; }
int structure_clusters_in_sphere(void) { return 0; }
int structure_decals_update(void) { return 0; }
int structure_test_line2d(void) { return 0; }
int strupr(void) { return 0; }
int tag_index_is_group(void) { return 0; }
int tag_iterator_new(void) { return 0; }
int tag_iterator_next(void) { return -1; }
int tag_loaded(void) { return 0; }
int tag_name_strip_path(void) { return 0; }
int terminal_printf(void) { return 0; }
int _texture_cache_bitmap_get_hardware_format(void) { return 0; }
int transition_function_evaluate(void) { return 0; }
int unattached_impulse_sound_new(void) { return 0; }
int unicode_string_list_get_string(void) { return 0; }
int unit_describe_speech(void) { return 0; }
int unit_dialogue_determine_variant(void) { return 0; }
int unit_dialogue_update(void) { return 0; }
int unit_get_local_player_index(void) { return 0; }
int unit_get_speech_priority_name(void) { return 0; }
int unit_make_damage_sound(void) { return 0; }
int unit_scream(void) { return 0; }
int unit_speak(void) { return 0; }
int unit_test_speech(void) { return 0; }
int unspatialized_impulse_sound_new(void) { return 0; }
int ustrncpy(void) { return 0; }
int valid_real_normal2d(void) { return 0; }
float cosf(float);
float sinf(float);
float *vector3d_from_angle(float *vector, float angle)
{
	if (!vector)
		return 0;
	vector[0] = cosf(angle);
	vector[1] = sinf(angle);
	vector[2] = 0.f;
	return vector;
}
int vector3d_from_euler_angles2d(void) { return 0; }
int vector_intersects_pill3d(void) { return 0; }
void vectors3d_from_euler_angles3d(float *forward, float *up, float const *angles)
{
	float yaw;
	float pitch;
	float cosine_pitch;

	if (!forward || !up || !angles)
		return;
	yaw = angles[0];
	pitch = angles[1];
	cosine_pitch = cosf(pitch);
	forward[0] = cosf(yaw) * cosine_pitch;
	forward[1] = sinf(yaw) * cosine_pitch;
	forward[2] = sinf(pitch);
	up[0] = 0.f;
	up[1] = 0.f;
	up[2] = 1.f;
}
int vector_to_line_distance_squared3d(void) { return 0; }
int viewmodel_is_visible(void) { return 0; }
int viewmodel_shield_is_visible(void) { return 0; }
int widgets_delete(void) { return 0; }
int widgets_dispose(void) { return 0; }
int widgets_dispose_from_old_map(void) { return 0; }
int widgets_initialize(void) { return 0; }
int widgets_initialize_for_new_map(void) { return 0; }
int widgets_new(void) { return 0; }
int yaw_vectors(void) { return 0; }

float real_seed_random(unsigned long *seed) { (void)seed; return 0.5f; }
float real_seed_random_range(unsigned long *seed, float lower, float upper)
{
	(void)seed;
	return (lower + upper) * 0.5f;
}
int network_objects_new_object_index(void) { return -1; }
static unsigned char wii_cinematic_globals_bytes[64];
void *cinematic_globals = wii_cinematic_globals_bytes;
int control_delete(void) { return 0; }
int control_new(void) { return 0; }
int control_place(void) { return 0; }
int controls_dispose(void) { return 0; }
int controls_dispose_from_old_map(void) { return 0; }
int controls_initialize(void) { return 0; }
int controls_initialize_for_new_map(void) { return 0; }
int control_update(void) { return 0; }
int custom_edition_vehicle_placement_allowed(void) { return 0; }
int device_delete(void) { return 0; }
int device_export_function_values(void) { return 0; }
int device_new(void) { return 0; }
int device_preprocess_node_orientations(void) { return 0; }
int device_render_debug(void) { return 0; }
int devices_dispose(void) { return 0; }
int devices_dispose_from_old_map(void) { return 0; }
int devices_initialize(void) { return 0; }
int devices_initialize_for_new_map(void) { return 0; }
int device_update(void) { return 0; }
int garbage_new(void) { return 0; }
int garbage_update(void) { return 0; }
int item_delete(void) { return 0; }
int item_new(void) { return 1; }
int items_dispose(void) { return 0; }
int items_dispose_from_old_map(void) { return 0; }
int items_initialize(void) { return 0; }
int items_initialize_for_new_map(void) { return 0; }
int item_update(void) { return 0; }
int light_fixture_delete(void) { return 0; }
int light_fixture_new(void) { return 0; }
int light_fixture_place(void) { return 0; }
int light_fixtures_dispose(void) { return 0; }
int light_fixtures_dispose_from_old_map(void) { return 0; }
int light_fixtures_initialize(void) { return 0; }
int light_fixtures_initialize_for_new_map(void) { return 0; }
int light_fixture_update(void) { return 0; }
int machine_delete(void) { return 0; }
int machine_new(void) { return 0; }
int machine_place(void) { return 0; }
int machines_dispose(void) { return 0; }
int machines_dispose_from_old_map(void) { return 0; }
int machines_initialize(void) { return 0; }
int machines_initialize_for_new_map(void) { return 0; }
int machine_update(void) { return 0; }
int matrix4x3_rotation_from_angles(void) { return 0; }
int placeholder_delete(void) { return 0; }
int placeholder_dispose(void) { return 0; }
int placeholder_dispose_from_old_map(void) { return 0; }
int placeholder_initialize(void) { return 0; }
int placeholder_initialize_for_new_map(void) { return 0; }
int placeholder_new(void) { return 0; }
int placeholder_place(void) { return 0; }
int scenery_delete(void) { return 0; }
int scenery_dispose(void) { return 0; }
int scenery_dispose_from_old_map(void) { return 0; }
int scenery_initialize(void) { return 0; }
int scenery_initialize_for_new_map(void) { return 0; }
int scenery_new(void) { return 0; }
int scenery_place(void) { return 0; }
int scenery_update(void) { return 0; }
int sound_scenery_delete(void) { return 0; }
int sound_scenery_new(void) { return 0; }
int unit_notify_impulse_sound(void) { return 0; }

/* Collision BSP projection and the multiplayer engine's globals.
   Campaign leaves game_engine NULL, so these bodies are not called. */
short const global_projection3d_mappings[3][2][2] = {
	{ { 2, 1 }, { 1, 2 } },
	{ { 0, 2 }, { 2, 0 } },
	{ { 1, 0 }, { 0, 1 } }
};
static float wii_rgb_blue[3] = { 0.f, 0.f, 1.f };
static float wii_rgb_red[3] = { 1.f, 0.f, 0.f };
const float *global_real_rgb_blue = wii_rgb_blue;
const float *global_real_rgb_red = wii_rgb_red;
unsigned char game_engine_globals[0x24];
unsigned char global_stage[192];
long timeout_for_endgame_sound;
void *game_engines[32];
void *hud_globals;
int autoaim_compute_target(void) { return 0; }
int bitmap_group_try_and_get_bitmap(void) { return 0; }
int collision_log_end_time(void) { return 0; }
int collision_log_start_time(void) { return 0; }
int collision_log_usage(void) { return 0; }
int config_string(void) { return 0; }
int convex_polygon3d_clip_to_plane(void) { return 0; }
int custom_edition_cache_tags_loaded(void) { return 0; }
int custom_edition_vehicles_by_placement(void) { return 0; }
int custom_render_nav_point(void) { return 0; }
int draw_bitmap_in_rect(void) { return 0; }
int draw_quad(void) { return 0; }
int draw_string_and_hack_in_icons(void) { return 0; }
int draw_string_set_draw_mode(void) { return 0; }
int draw_unicode_string_capital_middle(void) { return 0; }
int draw_unicode_string_compute_bounds(void) { return 0; }
int find_nav_point(void) { return 0; }
int game_engine_ctf_read_network_state(void) { return 0; }
int game_engine_ctf_write_network_state(void) { return 0; }
int game_engine_intialize_queued_sounds(void) { return 0; }
int game_engine_king_read_network_state(void) { return 0; }
int game_engine_king_write_network_state(void) { return 0; }
int game_engine_oddball_read_network_state(void) { return 0; }
int game_engine_oddball_write_network_state(void) { return 0; }
int game_engine_play_multiplayer_sound(void) { return 0; }
int game_engine_race_read_network_state(void) { return 0; }
int game_engine_race_write_network_state(void) { return 0; }
int game_engine_slayer_read_network_state(void) { return 0; }
int game_engine_slayer_write_network_state(void) { return 0; }
int game_engine_update_multiplayer_sound(void) { return 0; }
int game_modes_engine(void) { return 0; }
int game_modes_motion_sensor_hidden(void) { return 0; }
int game_modes_options_default(void) { return 0; }
int game_modes_player_killed(void) { return 0; }
int game_modes_postspawn(void) { return 0; }
int game_modes_variant_mode(void) { return 0; }
int global_network_game_client_get(void) { return 0; }
int global_network_game_server_get(void) { return 0; }
int halo_scoreboard_pointer_update(void) { return 0; }
int halo_server_mode(void) { return 0; }
int hud_get_font_index(void) { return 0; }
int hud_get_nav_point_render_type(void) { return 0; }
int hud_get_text_color(void) { return 0; }
int hud_player_name_color(void) { return 0; }
int infection_enabled(void) { return 0; }
int infection_read_network_state(void) { return 0; }
int infection_remaining_ticks(void) { return 0; }
int infection_write_network_state(void) { return 0; }
int interface_get_tag_index(void) { return 0; }
int latency_meter_color(void) { return 0; }
int latency_meter_player_ping(void) { return 0; }
int latency_meter_shown(void) { return 0; }
int line_from_planes3d(void) { return 0; }
int main_get_multiplayer_map_name(void) { return 0; }
int main_get_seconds_elapsed(void) { return 0; }
int main_reset_map(void) { return 0; }
int main_set_multiplayer_map_name(void) { return 0; }
struct wii_point3 *matrix4x3_inverse_transform_point(struct wii_matrix4x3 const *matrix, struct wii_point3 const *point, struct wii_point3 *result)
{
	struct wii_matrix4x3 inverse;

	matrix4x3_inverse(matrix, &inverse);
	return matrix4x3_transform_point(&inverse, point, result);
}
int network_chat_host_mute(void) { return 0; }
int network_chat_host_mute_minutes(void) { return 0; }
int network_coop_ally_friendly_fire(void) { return 0; }
int network_coop_players_friendly_fire(void) { return 0; }
int network_damage_killer_score(void) { return 0; }
int network_distributed_player_killed(void) { return 0; }
int network_game_abort(void) { return 0; }
int network_game_get_game(void) { return 0; }
int network_game_is_active(void) { return 0; }
int network_game_server_get_game(void) { return 0; }
int network_game_server_reset_to_pregame(void) { return 0; }
int network_game_server_switch_to_postgame(void) { return 0; }
int network_voice_available(void) { return 0; }
int network_voice_draw_icon(void) { return 0; }
int network_voice_machine_muted(void) { return 0; }
int network_voice_machine_speaking(void) { return 0; }
int network_voice_mute_machine(void) { return 0; }
int network_votekick_available(void) { return 0; }
int network_votekick_get_status(void) { return 0; }
int network_votekick_host(void) { return 0; }
int network_votekick_host_kick(void) { return 0; }
int network_votekick_request(void) { return 0; }
int object_bounds_cache_missed_by_vector(void) { return 0; }
int object_bounds_cache_out_of_reach(void) { return 0; }
int offset_rectangle2d(void) { return 0; }
int perpendicular2d(void) { return 0; }
int physics_get_features_in_sphere(void) { return 0; }
int physics_test_point(void) { return 0; }
int physics_test_vector(void) { return 0; }
int platform_scoreboard_scroll(void) { return 0; }
int player_control_get_facing_direction(void) { return 0; }
int player_control_get_target_object_index(void) { return 0; }
struct wii_rgb { float r, g, b; };
struct wii_rgb player_profile_get_rgb_color(long index)
{
	struct wii_rgb color = { 1.f, 1.f, 1.f };

	(void)index;
	return color;
}
int player_ui_game_variant_specified(void) { return 0; }
int point_from_planes3d(void) { return 0; }
int rasterizer_draw_unicode_string(void) { return 0; }
int rasterizer_text_set_scale(void) { return 0; }
int render_debug_cylinder(void) { return 0; }
int render_ui_widgets_postgame(void) { return 0; }
int rumble_clear_all_now(void) { return 0; }
int rumble_player_clear(void) { return 0; }
int server_director_first_level(void) { return 0; }
int sound_class_set_gain(void) { return 0; }
int structure_cluster_mark(void) { return 0; }
int structure_cluster_marker_begin(void) { return 0; }
int structure_cluster_marker_end(void) { return 0; }
int system_milliseconds(void) { return 0; }
int touch_postgame_draw(void) { return 0; }
int touch_postgame_pressed(void) { return 0; }
int usnprintf(void) { return 0; }
int usprintf(void) { return 0; }
int ustring_format_checked(void) { return 0; }

struct wii_vec3 { float i, j, k; };

float magnitude_squared3d(struct wii_vec3 const *v)
{
	return v->i * v->i + v->j * v->j + v->k * v->k;
}

struct wii_vec3 *vector_from_points3d(struct wii_point3 const *a, struct wii_point3 const *b, struct wii_vec3 *result)
{
	result->i = b->x - a->x;
	result->j = b->y - a->y;
	result->k = b->z - a->z;
	return result;
}

float distance_squared3d(struct wii_point3 const *a, struct wii_point3 const *b)
{
	struct wii_vec3 v;

	vector_from_points3d(a, b, &v);
	return magnitude_squared3d(&v);
}

float distance3d(struct wii_point3 const *a, struct wii_point3 const *b)
{
	return sqrtf(distance_squared3d(a, b));
}

float real_random(void)
{
	return 0.5f;
}

float weapon_definition_get_damage_potential(long weapon_definition_index, float *rate_of_fire)
{
	(void)weapon_definition_index;
	if (rate_of_fire)
		*rate_of_fire = 1.f;
	return 1.f;
}

void actor_path_input_new(long actor_index, void *input)
{
	(void)actor_index;
	if (input) {
		memset(input, 0, 72);
		/* start_surface_index. NONE keeps path_state_begin off a missing BSP. */
		*(long *)((char *)input + 32) = -1;
	}
}

int actor_path_3d_available(long actor_index, void *point, float *avoidance)
{
	(void)actor_index;
	(void)point;
	if (avoidance)
		*avoidance = 0.f;
	return 0;
}

int actor_perception_aiming_vector_test_blockage(long actor_index, void *from, void *to)
{
	(void)actor_index;
	(void)from;
	(void)to;
	return 0;
}

int actor_perception_friend_prop_is_attacking(long actor_index, long prop_index, void *vector)
{
	(void)actor_index;
	(void)prop_index;
	(void)vector;
	return 0;
}

void *ai_debug_get_path_storage(long actor_index)
{
	(void)actor_index;
	return 0;
}

int actor_move_to_firing_position(long actor_index, short firing_position_index, void *position)
{
	(void)actor_index;
	(void)firing_position_index;
	(void)position;
	return 0;
}
