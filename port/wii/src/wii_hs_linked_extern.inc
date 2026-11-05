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
void hs_teleport_players_not_in_trigger_volume( short trigger_volume_index, short cutscene_flag_index);
boolean hs_trigger_volume_test_objects_any( short trigger_volume_index, long object_list_index);
boolean hs_trigger_volume_test_objects_all( short trigger_volume_index, long object_list_index);
void hs_object_teleport( long object_index, short cutscene_flag_index);
void hs_object_set_facing( long object_index, short cutscene_flag_index);
void hs_object_set_permutation( long object_index, char const *region_name, char const *permutation_name);
void hs_effect_new( long effect_definition_index, short cutscene_flag_index);
void hs_effect_new_from_object_marker( long effect_definition_index, long object_index, char const *marker_name);
void hs_damage_new( long damage_effect_index, short cutscene_flag_index);
void hs_damage_object( long damage_effect_index, long object_index);
void hs_sound_set_gain( char const *tag_name, real gain);
void scripted_sound_new( long definition_index, long source_object_index, real scale);
void scripted_looping_sound_start( long sound_index, long source_object_index, real gain);
void scripted_looping_sound_set_scale( long sound_index, real scale);
void scripted_looping_sound_set_alternate( long sound_index, boolean alternate);
void hud_unit_activate_nav_point_with_flag( short nav_index, long unit_index, short flag_index, float vertical_offset);
void hud_unit_activate_nav_point_with_object( short nav_index, long unit_index, long object_index, float vertical_offset);
void hud_activate_team_nav_point_with_flag( short nav_index, short team_index, short flag_index, float vertical_offset);
void hud_activate_team_nav_point_with_object( short nav_index, short team_index, long object_index, float vertical_offset);
void hud_unit_deactivate_nav_point_with_flag( long unit_index, short flag_index);
void hud_unit_deactivate_nav_point_with_object( long unit_index, long object_index);
void hud_deactivate_team_nav_point_with_flag( short team_index, short flag_index);
void hud_deactivate_team_nav_point_with_object( short team_index, long object_index);
void hs_object_create_anew_containing( char const *name_string);
void hs_object_destroy_containing( char const *name_string);
void hs_objects_delete_by_definition( long definition_index);
void recorded_animation_kill( long unit_index);
long recorded_animation_get_time_left( long unit_index);
void hs_objects_predict( long object_list_index);
short scenery_get_animation_time( long object_index);
boolean unit_is_playing_custom_animation( long unit_index);
real unit_scripting_get_health( long unit_index);
real unit_scripting_get_shield( long unit_index);
short unit_scripting_get_grenade_count( long unit_index);
boolean unit_get_current_flashlight_state( long unit_index);
real device_group_get_value( short group_index);
boolean ai_scripting_conversation( short conversation_index);
void scripted_camera_set_first_person( long unit_index);
void scripted_camera_set_dead( long unit_index);
void cinematic_suppress_bsp_object_creation( boolean suppress);
long scripted_sound_time( long sound_index);
void scripted_sound_stop( long sound_index);
void scripted_foley_predict( long definition_index);
void scripted_looping_sound_stop( long sound_index);
void player_effect_screen_fade_in( real red, real green, real blue, short ticks);
void player_effect_screen_fade_out( real red, real green, real blue, short ticks);
void scripted_hud_set_flashing_state( boolean flash);
void scripted_hud_restart_flashing( void);
void scripted_player_effect_set_translation( real horizontal, real vertical, real depth);
void scripted_player_effect_set_rotation( real yaw, real pitch, real roll);
void scripted_player_effect_set_rumble( real left_motor, real right_motor);
void scripted_player_effect_start( real maximum_intensity, real attack_time);
void scripted_player_effect_stop( real duration);
void scripted_hud_messages_clear( void);
void scripted_hud_set_state_message( short message_index);
void scripted_hud_set_objective( short message_index);
void scripted_hud_set_timer_time( short minutes, word seconds);
void scripted_hud_set_timer_warning_cutoff( short minutes, word seconds);
void scripted_hud_set_timer_position( short x, short y, short corner);
void scripted_hud_show_timer( boolean show);
void scripted_hud_pause_timer( boolean paused);
short scripted_hud_get_timer_ticks( void);
void scripted_hud_time_code_show( boolean show);
void scripted_hud_time_code_start( boolean start);
void scripted_hud_time_code_reset( void);
void scripted_hud_show_health( boolean show);
void scripted_hud_blink_health( boolean blink);
void scripted_hud_show_shield( boolean show);
void scripted_hud_blink_shield( boolean blink);
void scripted_hud_show_motion_sensor( boolean show);
void scripted_hud_blink_motion_sensor( boolean blink);
void scripted_hud_show_crosshair( boolean show);
long hs_object_list_get_element( long object_list_index, short element_index);
real hs_sound_get_gain( char const *tag_name);
void objects_scripting_set_scale( long object_index, real scale, short interpolation_frame_count);
void unit_scripting_set_maximum_vitality( long unit_index, real body_vitality, real shield_vitality);
void units_scripting_set_maximum_vitality( long object_list_index, real body_vitality, real shield_vitality);
void units_scripting_set_current_vitality( long object_list_index, real body_vitality, real shield_vitality);
boolean device_group_set_desired_value( short group_index, real desired_value);
void player_add_equipment( long unit_index, short starting_profile_index, boolean reset_equipment);
short object_list_count( long object_list_index);
void breakable_surfaces_enable( boolean state);
void game_time_set_speed( real speed);
void sound_enable( boolean enabled);
boolean scripted_show_hud( boolean show);
boolean scripted_show_hud_help_text( boolean show);
void numeric_countdown_timer_set( long milliseconds, boolean auto_start);
short numeric_countdown_timer_get( short digit_index);
void numeric_countdown_timer_stop( void);
void numeric_countdown_timer_restart( void);
void sound_class_set_gain( char const *name, real gain, short interpolation_ticks);
boolean scripted_player_control_set_camera_control( boolean camera_control);
void player_control_action_test_reset( void);
boolean player_control_action_test_jump( void);
boolean player_control_action_test_primary_trigger( void);
boolean player_control_action_test_grenade_trigger( void);
boolean player_control_action_test_zoom( void);
boolean player_control_action_test_action( void);
boolean player_control_action_test_accept( void);
boolean player_control_action_test_back( void);
boolean player_control_action_test_look_relative_up( void);
boolean player_control_action_test_look_relative_down( void);
boolean player_control_action_test_look_relative_left( void);
boolean player_control_action_test_look_relative_right( void);
boolean player_control_action_test_look_relative_all_directions( void);
boolean player_control_action_test_move_relative_all_directions( void);
