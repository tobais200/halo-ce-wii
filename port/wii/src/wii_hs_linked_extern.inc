short ai_scripting_conversation_line(
	short conversation_index);
short ai_scripting_conversation_status(
	short conversation_index);
void ai_scripting_vehicle_enterable_team(
	long object_list_index,
	short team);
void ai_scripting_vehicle_enterable_actor_type(
	long object_list_index,
	short actor_type);
void ai_scripting_vehicle_enterable_actors(
	long vehicle_index,
	long actor_list_index);
void ai_scripting_vehicle_enterable_disable(
	long vehicle_index);
void ai_scripting_detach_unit(
	long unit_index);
void ai_scripting_attach_unit(
	long unit_index,
	long ai_reference);
void ai_scripting_magically_see_unit(
	long ai_reference,
	long unit_index);
void ai_scripting_go_to_vehicle(
	long ai_reference,
	long unit_index,
	char const *seat_substring_name);
void ai_scripting_go_to_vehicle_override(
	long ai_reference,
	long unit_index,
	char const *seat_substring_name);
void ai_scripting_maneuver(
	long ai_reference);
void ai_scripting_maneuver_enable(
	long ai_reference,
	boolean enable);
void ai_scripting_migrate_by_unit(
	long object_list_index,
	long target_ai_reference);
void ai_scripting_migrate_and_speak(
	long source_ai_reference,
	long target_ai_reference,
	char const *speech_type);
void ai_scripting_allegiance_remove(
	short team_a,
	short team_b);
void ai_scripting_berserk(
	long ai_reference,
	boolean berserk);
void ai_scripting_playfight(
	long ai_reference,
	boolean playfight);
void ai_scripting_braindead(
	long ai_reference,
	boolean braindead);
void ai_scripting_allow_charge(
	long ai_reference,
	boolean allow);
void ai_scripting_allow_dormant(
	long ai_reference,
	boolean allow);
void ai_scripting_automatic_migration_target(
	long ai_reference,
	boolean automatic);
void ai_scripting_follow_target_unit(
	long ai_reference,
	long unit_index);
void ai_scripting_follow_target_ai(
	long ai_reference,
	long target_ai_reference);
void ai_scripting_force_active(
	long ai_reference,
	boolean force);
void ai_scripting_set_respawn(
	long ai_reference,
	boolean respawn);
void ai_scripting_set_deaf(
	long ai_reference,
	boolean deaf);
void ai_scripting_set_blind(
	long ai_reference,
	boolean blind);
short ai_scripting_swarm_count(
	long ai_reference);
short ai_scripting_nonswarm_count(
	long ai_reference);
real ai_scripting_living_fraction(
	long ai_reference);
real ai_scripting_strength(
	long ai_reference);
short ai_scripting_going_to_vehicle(
	long ai_reference);
void ai_scripting_migrate(
	long source_ai_index,
	long destination_ai_index);
void ai_scripting_vehicle_encounter(
	long unit_index,
	long ai_reference);
void ai_scripting_allegiance(
	short team_index0,
	short team_index1);
void ai_scripting_conversation_stop(
	short conversation_index);
void ai_scripting_conversation_advance(
	short conversation_index);
void ai_scripting_stop_looking(
	long ai_index);
void ai_scripting_braindead_by_unit(
	long object_list_index,
	boolean braindead);
void ai_scripting_force_active_by_unit(
	long unit_index,
	boolean force);
void ai_scripting_attach_free(
	long unit_index,
	long actor_variant_definition_index);
void ai_scripting_try_to_fight(
	long ai_reference,
	long target_ai_reference);
void ai_scripting_set_return_state(
	long ai_reference,
	short default_state);
void ai_scripting_magically_see_encounter(
	long ai_reference,
	long target_ai_reference);
void ai_scripting_link_activation(
	long ai_reference,
	long link_ai_reference);
void ai_scripting_free_units(
	long object_list_index);
void ai_scripting_set_current_state(
	long ai_reference,
	short current_state);
void ai_scripting_command_list(
	long ai_reference,
	short command_list_index);
void ai_scripting_command_list_by_unit(
	long unit_index,
	short command_list_index);
void ai_scripting_ignore(
	long object_list_index,
	boolean ignore);
void ai_scripting_prefer_target(
	long object_list_index,
	boolean prefer);
void ai_scripting_teleport_starting_location_if_unsupported(
	long ai_reference);
void ai_scripting_look_at_object(
	long unit_index,
	long object_index);
void ai_scripting_command_list_advance_by_unit(
	long unit_index);
short ai_scripting_command_list_status(
	long ai_reference);
long unit_scripting_unit_riders(
	long unit_index);
void unit_scripting_set_seat(
	long unit_index,
	char const *seat_label);
long unit_scripting_unit_driver(
	long unit_index);
long unit_scripting_unit_gunner(
	long unit_index);
void unit_scripting_exit_vehicle(
	long unit_index);
void unit_scripting_can_blink(
	long unit_index,
	boolean can_blink);
void unit_scripting_doesnt_drop_items(
	long object_list_index);
void unit_scripting_set_emotion_animation(long unit_index, char const *animation_name);
void unit_scripting_suspended(
	long unit_index,
	boolean suspended);
void unit_scripting_enter_vehicle(
	long unit_index,
	long vehicle_index,
	char const *seat_name);
void object_scripting_set_collideable(long object_index, boolean collideable);
extern void object_set_ranged_attack_inhibited( long object_index, boolean inhibited);
extern void object_set_melee_attack_inhibited( long object_index, boolean inhibited);
extern void objects_scripting_detach( long parent_object_index, long child_object_index);
extern void object_can_take_damage( long object_list_index);
extern void object_beautify( long object_index, boolean beautiful);
extern void object_definition_predict( long definition_index);
extern void object_pvs_activate( long object_index);
extern void object_pvs_set_object( long object_index);
extern void object_pvs_set_camera_point( short camera_point_index);
extern void unit_open( long unit_index);
extern void unit_close( long unit_index);
extern void unit_kill_silent( long unit_index);
extern short unit_get_custom_animation_time( long unit_index);
extern void unit_stop_custom_animation( long unit_index);
extern void unit_aim_without_turning( long unit_index, boolean aim_without_turning);
extern void unit_set_emotion( long unit_index, word emotion_index);
extern void unit_set_enterable_by_player( long unit_index, boolean enterable_by_player);
extern void scripting_set_magic_base_seat( char const *seat_name);
extern void units_set_desired_flashlight_state( long object_list_index, boolean desired_state);
extern void unit_set_desired_flashlight_state( long unit_index, boolean desired_state);
extern void cheat_active_camouflage_local_player( short local_player_index);
extern long object_list_from_ai_reference( long ai_reference);
extern void game_set_game_variant_from_name( const char *name);
extern void ai_globals_ai_active( boolean enabled);
extern void ai_globals_dialogue_triggers_enabled( boolean enabled);
extern void ai_globals_grenades_enabled( boolean enabled);
extern void vehicle_hover( long vehicle_index, boolean hover);
extern void player_input_enable( boolean enable);
extern void objects_dump_memory( void);
extern void garbage_collect_now( void);
extern void object_pvs_clear(void);
extern void scripting_magic_melee_attack( void);
extern boolean unit_solo_player_integrated_night_vision_is_active( void);
extern void device_set_never_appears_locked( long device_index, boolean never_appears_locked);
extern void device_one_sided_set( long device_index, boolean one_sided);
extern void device_operates_automatically_set( long device_index, boolean operates_automatically);
extern void cheat_all_powerups( void);
extern void cheat_all_weapons( void);
extern void cheat_all_vehicles( void);
extern void cheat_teleport_to_camera( void);
extern void cheat_active_camouflage( void);
extern void cheats_load( void);
extern void ai_scripting_erase_all( void);
extern void ai_scripting_deselect( void);
extern void ai_scripting_reconnect( void);
extern short game_difficulty_level_get_ignore_easy( void);
extern short game_difficulty_level_get( void);
extern boolean game_all_quiet( void);
extern boolean game_safe_to_speak( void);
extern boolean game_is_cooperative( void);
boolean recorded_animation_play( long unit_index, short animation_index);
boolean recorded_animation_play_and_delete( long unit_index, short animation_index);
boolean recorded_animation_play_and_hover( long unit_index, short animation_index);
void objects_scripting_attach( long parent_object_index, char const *parent_marker_name, long child_object_index, char const *child_marker_name);
void scenery_animation_start( long object_index, long animation_graph_index, char const *animation_name);
void scenery_animation_start_at_frame( long object_index, long animation_graph_index, char const *animation_name, short frame_index);
boolean unit_start_user_animation( long unit_index, long animation_graph_index, char const *animation_name, boolean interpolate);
boolean unit_scripting_start_user_animation_list( long object_list_index, long animation_graph_index, char const *animation_name, boolean interpolate);
boolean unit_scripting_vehicle_test_seat_list( long vehicle_index, char const *seat_name, long object_list_index);
boolean unit_scripting_vehicle_test_seat( long vehicle_index, char const *seat_name, long unit_index);
short vehicle_scripting_load_magic( long vehicle_index, char const *seat_name, long object_list_index);
short vehicle_scripting_unload( long vehicle_index, char const *seat_name);
boolean unit_scripting_has_weapon( long unit_index, long weapon_definition_index);
boolean unit_scripting_has_weapon_readied( long unit_index, long weapon_definition_index);
void unit_scripting_impervious( long object_list_index, boolean impervious);
void device_set_actual_position( long device_index, real position);
void device_group_set_actual_value( short group_index, real actual_value);
void device_group_change_only_once_more_set( long group_index, boolean change_only_once_more);
void ai_scripting_vehicle_enterable_distance( long unit_index, real distance);
void ai_scripting_follow_distance( long ai_reference, real distance);
boolean ai_scripting_allegiance_broken( short team1_index, short team2_index);
void scripted_camera_set( short camera_point_index, word transition_time, long relative_object_index);
void scripted_camera_set_animation( long animation_graph_index, char const *animation_name);
void debug_player_teleport( short local_player_index, short target_local_player_index);
