	case 74:
		object_scripting_set_collideable(arguments[0], (boolean)arguments[1]);
		break;
	case 93:
		unit_scripting_can_blink(arguments[0], (boolean)arguments[1]);
		break;
	case 107:
		unit_scripting_enter_vehicle(arguments[0], arguments[1], arguments[2] ? (char const *)(uintptr_t)arguments[2] : "");
		break;
	case 110:
		unit_scripting_set_emotion_animation(arguments[0], arguments[1] ? (char const *)(uintptr_t)arguments[1] : "");
		break;
	case 111:
		unit_scripting_exit_vehicle(arguments[0]);
		break;
	case 119:
		unit_scripting_set_seat(arguments[0], arguments[1] ? (char const *)(uintptr_t)arguments[1] : "");
		break;
	case 121:
		result = unit_scripting_unit_riders(arguments[0]);
		break;
	case 122:
		result = unit_scripting_unit_driver(arguments[0]);
		break;
	case 123:
		result = unit_scripting_unit_gunner(arguments[0]);
		break;
	case 129:
		unit_scripting_doesnt_drop_items(arguments[0]);
		break;
	case 131:
		unit_scripting_suspended(arguments[0], (boolean)arguments[1]);
		break;
	case 157:
		if (actor_data)
			ai_scripting_free_units(arguments[0]);
		break;
	case 158:
		if (actor_data)
			ai_scripting_attach_unit(arguments[0], arguments[1]);
		break;
	case 159:
		if (actor_data)
			ai_scripting_attach_free(arguments[0], arguments[1]);
		break;
	case 160:
		if (actor_data)
			ai_scripting_detach_unit(arguments[0]);
		break;
	case 169:
		if (actor_data)
			ai_scripting_set_respawn(arguments[0], (boolean)arguments[1]);
		break;
	case 170:
		if (actor_data)
			ai_scripting_set_deaf(arguments[0], (boolean)arguments[1]);
		break;
	case 171:
		if (actor_data)
			ai_scripting_set_blind(arguments[0], (boolean)arguments[1]);
		break;
	case 172:
		if (actor_data)
			ai_scripting_magically_see_encounter(arguments[0], arguments[1]);
		break;
	case 174:
		if (actor_data)
			ai_scripting_magically_see_unit(arguments[0], arguments[1]);
		break;
	case 180:
		if (actor_data)
			ai_scripting_maneuver(arguments[0]);
		break;
	case 181:
		if (actor_data)
			ai_scripting_maneuver_enable(arguments[0], (boolean)arguments[1]);
		break;
	case 182:
		if (actor_data)
			ai_scripting_migrate(arguments[0], arguments[1]);
		break;
	case 183:
		if (actor_data)
			ai_scripting_migrate_and_speak(arguments[0], arguments[1], arguments[2] ? (char const *)(uintptr_t)arguments[2] : "");
		break;
	case 184:
		if (actor_data)
			ai_scripting_migrate_by_unit(arguments[0], arguments[1]);
		break;
	case 185:
		if (actor_data)
			ai_scripting_allegiance((short)arguments[0], (unsigned short)arguments[1]);
		break;
	case 186:
		if (actor_data)
			ai_scripting_allegiance_remove((short)arguments[0], (unsigned short)arguments[1]);
		break;
	case 188:
		if (actor_data)
			*(float *)&result = ai_scripting_living_fraction(arguments[0]);
		break;
	case 189:
		if (actor_data)
			*(float *)&result = ai_scripting_strength(arguments[0]);
		break;
	case 190:
		if (actor_data)
			result = ai_scripting_swarm_count(arguments[0]);
		break;
	case 191:
		if (actor_data)
			result = ai_scripting_nonswarm_count(arguments[0]);
		break;
	case 193:
		if (actor_data)
			ai_scripting_go_to_vehicle(arguments[0], arguments[1], arguments[2] ? (char const *)(uintptr_t)arguments[2] : "");
		break;
	case 194:
		if (actor_data)
			ai_scripting_go_to_vehicle_override(arguments[0], arguments[1], arguments[2] ? (char const *)(uintptr_t)arguments[2] : "");
		break;
	case 195:
		if (actor_data)
			result = ai_scripting_going_to_vehicle(arguments[0]);
		break;
	case 197:
		if (actor_data)
			ai_scripting_braindead(arguments[0], (boolean)arguments[1]);
		break;
	case 198:
		if (actor_data)
			ai_scripting_braindead_by_unit(arguments[0], (boolean)arguments[1]);
		break;
	case 199:
		if (actor_data)
			ai_scripting_ignore(arguments[0], (boolean)arguments[1]);
		break;
	case 200:
		if (actor_data)
			ai_scripting_prefer_target(arguments[0], (boolean)arguments[1]);
		break;
	case 202:
		if (actor_data)
			ai_scripting_teleport_starting_location_if_unsupported(arguments[0]);
		break;
	case 205:
		if (actor_data)
			ai_scripting_try_to_fight(arguments[0], arguments[1]);
		break;
	case 207:
		if (actor_data)
			ai_scripting_command_list(arguments[0], (unsigned short)arguments[1]);
		break;
	case 208:
		if (actor_data)
			ai_scripting_command_list_by_unit(arguments[0], (unsigned short)arguments[1]);
		break;
	case 210:
		if (actor_data)
			ai_scripting_command_list_advance_by_unit(arguments[0]);
		break;
	case 211:
		if (actor_data)
			result = ai_scripting_command_list_status(arguments[0]);
		break;
	case 213:
		if (actor_data)
			ai_scripting_force_active(arguments[0], (boolean)arguments[1]);
		break;
	case 214:
		if (actor_data)
			ai_scripting_force_active_by_unit(arguments[0], (boolean)arguments[1]);
		break;
	case 215:
		if (actor_data)
			ai_scripting_set_return_state(arguments[0], (unsigned short)arguments[1]);
		break;
	case 216:
		if (actor_data)
			ai_scripting_set_current_state(arguments[0], (unsigned short)arguments[1]);
		break;
	case 217:
		if (actor_data)
			ai_scripting_playfight(arguments[0], (boolean)arguments[1]);
		break;
	case 220:
		if (actor_data)
			ai_scripting_vehicle_encounter(arguments[0], arguments[1]);
		break;
	case 222:
		if (actor_data)
			ai_scripting_vehicle_enterable_team(arguments[0], (unsigned short)arguments[1]);
		break;
	case 223:
		if (actor_data)
			ai_scripting_vehicle_enterable_actor_type(arguments[0], (unsigned short)arguments[1]);
		break;
	case 224:
		if (actor_data)
			ai_scripting_vehicle_enterable_actors(arguments[0], arguments[1]);
		break;
	case 225:
		if (actor_data)
			ai_scripting_vehicle_enterable_disable(arguments[0]);
		break;
	case 226:
		if (actor_data)
			ai_scripting_look_at_object(arguments[0], arguments[1]);
		break;
	case 227:
		if (actor_data)
			ai_scripting_stop_looking(arguments[0]);
		break;
	case 228:
		if (actor_data)
			ai_scripting_automatic_migration_target(arguments[0], (boolean)arguments[1]);
		break;
	case 231:
		if (actor_data)
			ai_scripting_follow_target_unit(arguments[0], arguments[1]);
		break;
	case 232:
		if (actor_data)
			ai_scripting_follow_target_ai(arguments[0], arguments[1]);
		break;
	case 235:
		if (actor_data)
			ai_scripting_conversation_stop((unsigned short)arguments[0]);
		break;
	case 236:
		if (actor_data)
			ai_scripting_conversation_advance((unsigned short)arguments[0]);
		break;
	case 237:
		if (actor_data)
			result = ai_scripting_conversation_line((unsigned short)arguments[0]);
		break;
	case 238:
		if (actor_data)
			result = ai_scripting_conversation_status((unsigned short)arguments[0]);
		break;
	case 239:
		if (actor_data)
			ai_scripting_link_activation(arguments[0], arguments[1]);
		break;
	case 240:
		if (actor_data)
			ai_scripting_berserk(arguments[0], (boolean)arguments[1]);
		break;
	case 242:
		if (actor_data)
			ai_scripting_allow_charge(arguments[0], (boolean)arguments[1]);
		break;
	case 243:
		if (actor_data)
			ai_scripting_allow_dormant(arguments[0], (boolean)arguments[1]);
		break;
	case 71:
		object_set_ranged_attack_inhibited(arguments[0], (boolean)arguments[1]);
		break;
	case 72:
		object_set_melee_attack_inhibited(arguments[0], (boolean)arguments[1]);
		break;
	case 77:
		objects_scripting_detach(arguments[0], arguments[1]);
		break;
	case 80:
		object_can_take_damage(arguments[0]);
		break;
	case 81:
		object_beautify(arguments[0], (boolean)arguments[1]);
		break;
	case 83:
		object_definition_predict(arguments[0]);
		break;
	case 84:
		object_pvs_activate(arguments[0]);
		break;
	case 85:
		object_pvs_set_object(arguments[0]);
		break;
	case 86:
		object_pvs_set_camera_point((unsigned short)arguments[0]);
		break;
	case 94:
		unit_open(arguments[0]);
		break;
	case 95:
		unit_close(arguments[0]);
		break;
	case 97:
		unit_kill_silent(arguments[0]);
		break;
	case 98:
		result = unit_get_custom_animation_time(arguments[0]);
		break;
	case 99:
		unit_stop_custom_animation(arguments[0]);
		break;
	case 104:
		unit_aim_without_turning(arguments[0], (boolean)arguments[1]);
		break;
	case 105:
		unit_set_emotion(arguments[0], (unsigned short)arguments[1]);
		break;
	case 106:
		unit_set_enterable_by_player(arguments[0], (boolean)arguments[1]);
		break;
	case 118:
		scripting_set_magic_base_seat(arguments[0] ? (char const *)(uintptr_t)arguments[0] : "");
		break;
	case 133:
		units_set_desired_flashlight_state(arguments[0], (boolean)arguments[1]);
		break;
	case 134:
		unit_set_desired_flashlight_state(arguments[0], (boolean)arguments[1]);
		break;
	case 154:
		cheat_active_camouflage_local_player((unsigned short)arguments[0]);
		break;
	case 192:
		result = object_list_from_ai_reference(arguments[0]);
		break;
	case 256:
		game_set_game_variant_from_name(arguments[0] ? (char const *)(uintptr_t)arguments[0] : "");
		break;
	case 283:
		ai_globals_ai_active((boolean)arguments[0]);
		break;
	case 284:
		ai_globals_dialogue_triggers_enabled((boolean)arguments[0]);
		break;
	case 285:
		ai_globals_grenades_enabled((boolean)arguments[0]);
		break;
	case 336:
		vehicle_hover(arguments[0], (boolean)arguments[1]);
		break;
	case 338:
		player_input_enable((boolean)arguments[0]);
		break;
	case 73:
		objects_dump_memory();
		break;
	case 78:
		garbage_collect_now();
		break;
	case 87:
		object_pvs_clear();
		break;
	case 120:
		scripting_magic_melee_attack();
		break;
	case 132:
		result = campaign_bool(unit_solo_player_integrated_night_vision_is_active());
		break;
	case 136:
		device_set_never_appears_locked(arguments[0], (boolean)arguments[1]);
		break;
	case 145:
		device_one_sided_set(arguments[0], (boolean)arguments[1]);
		break;
	case 146:
		device_operates_automatically_set(arguments[0], (boolean)arguments[1]);
		break;
	case 149:
		cheat_all_powerups();
		break;
	case 150:
		cheat_all_weapons();
		break;
	case 151:
		cheat_all_vehicles();
		break;
	case 152:
		cheat_teleport_to_camera();
		break;
	case 153:
		cheat_active_camouflage();
		break;
	case 155:
		cheats_load();
		break;
	case 165:
		ai_scripting_erase_all();
		break;
	case 167:
		ai_scripting_deselect();
		break;
	case 219:
		ai_scripting_reconnect();
		break;
	case 257:
		result = game_difficulty_level_get_ignore_easy();
		break;
	case 258:
		result = game_difficulty_level_get();
		break;
	case 306:
		result = campaign_bool(game_all_quiet());
		break;
	case 307:
		result = campaign_bool(game_safe_to_speak());
		break;
	case 308:
		result = campaign_bool(game_is_cooperative());
		break;

	case 66:
		result = campaign_bool(recorded_animation_play(arguments[0], arguments[1]));
		break;
	case 67:
		result = campaign_bool(recorded_animation_play_and_delete(arguments[0], arguments[1]));
		break;
	case 68:
		result = campaign_bool(recorded_animation_play_and_hover(arguments[0], arguments[1]));
		break;
	case 76:
		objects_scripting_attach(arguments[0], arguments[1] ? (char const *)(uintptr_t)arguments[1] : "", arguments[2], arguments[3] ? (char const *)(uintptr_t)arguments[3] : "");
		break;
	case 90:
		scenery_animation_start(arguments[0], arguments[1], arguments[2] ? (char const *)(uintptr_t)arguments[2] : "");
		break;
	case 91:
		scenery_animation_start_at_frame(arguments[0], arguments[1], arguments[2] ? (char const *)(uintptr_t)arguments[2] : "", (short)arguments[3]);
		break;
	case 101:
		result = campaign_bool(unit_start_user_animation(arguments[0], arguments[1], arguments[2] ? (char const *)(uintptr_t)arguments[2] : "", (boolean)arguments[3]));
		break;
	case 102:
		result = campaign_bool(unit_scripting_start_user_animation_list(arguments[0], arguments[1], arguments[2] ? (char const *)(uintptr_t)arguments[2] : "", (boolean)arguments[3]));
		break;
	case 108:
		result = campaign_bool(unit_scripting_vehicle_test_seat_list(arguments[0], arguments[1] ? (char const *)(uintptr_t)arguments[1] : "", arguments[2]));
		break;
	case 109:
		result = campaign_bool(unit_scripting_vehicle_test_seat(arguments[0], arguments[1] ? (char const *)(uintptr_t)arguments[1] : "", arguments[2]));
		break;
	case 116:
		result = vehicle_scripting_load_magic(arguments[0], arguments[1] ? (char const *)(uintptr_t)arguments[1] : "", arguments[2]);
		break;
	case 117:
		result = vehicle_scripting_unload(arguments[0], arguments[1] ? (char const *)(uintptr_t)arguments[1] : "");
		break;
	case 127:
		result = campaign_bool(unit_scripting_has_weapon(arguments[0], arguments[1]));
		break;
	case 128:
		result = campaign_bool(unit_scripting_has_weapon_readied(arguments[0], arguments[1]));
		break;
	case 130:
		unit_scripting_impervious(arguments[0], (boolean)arguments[1]);
		break;
	case 141:
		device_set_actual_position(arguments[0], (*(float *)&arguments[1]));
		break;
	case 144:
		device_group_set_actual_value((unsigned short)arguments[0], (*(float *)&arguments[1]));
		break;
	case 147:
		device_group_change_only_once_more_set((short)arguments[0], (boolean)arguments[1]);
		break;
	case 221:
		if (actor_data) ai_scripting_vehicle_enterable_distance(arguments[0], (*(float *)&arguments[1]));
		break;
	case 233:
		if (actor_data) ai_scripting_follow_distance(arguments[0], (*(float *)&arguments[1]));
		break;
	case 244:
		if (actor_data) result = campaign_bool(ai_scripting_allegiance_broken((short)arguments[0], (short)arguments[1]));
		break;
	case 247:
		scripted_camera_set((short)arguments[0], (short)arguments[1], arguments[2]);
		break;
	case 248:
		scripted_camera_set_animation(arguments[0], arguments[1] ? (char const *)(uintptr_t)arguments[1] : "");
		break;
	case 355:
		debug_player_teleport((short)arguments[0], (short)arguments[1]);
		break;

	case 30:
		hs_teleport_players_not_in_trigger_volume(arguments[0], (short)arguments[1]);
		break;
	case 32:
		result = campaign_bool(hs_trigger_volume_test_objects_any(arguments[0], arguments[1]));
		break;
	case 33:
		result = campaign_bool(hs_trigger_volume_test_objects_all(arguments[0], arguments[1]));
		break;
	case 34:
		hs_object_teleport(arguments[0], (unsigned short)arguments[1]);
		break;
	case 35:
		hs_object_set_facing(arguments[0], (unsigned short)arguments[1]);
		break;
	case 37:
		hs_object_set_permutation(arguments[0], arguments[1] ? (char const *)(uintptr_t)arguments[1] : "", arguments[2] ? (char const *)(uintptr_t)arguments[2] : "");
		break;
	case 47:
		hs_effect_new(arguments[0], (unsigned short)arguments[1]);
		break;
	case 48:
		hs_effect_new_from_object_marker(arguments[0], arguments[1], arguments[2] ? (char const *)(uintptr_t)arguments[2] : "");
		break;
	case 49:
		hs_damage_new(arguments[0], (unsigned short)arguments[1]);
		break;
	case 50:
		hs_damage_object(arguments[0], arguments[1]);
		break;
	case 54:
		hs_sound_set_gain(arguments[0] ? (char const *)(uintptr_t)arguments[0] : "", (*(float *)&arguments[1]));
		break;
	case 323:
		scripted_sound_new(arguments[0], arguments[1], (*(float *)&arguments[2]));
		break;
	case 327:
		scripted_looping_sound_start(arguments[0], arguments[1], (*(float *)&arguments[2]));
		break;
	case 329:
		scripted_looping_sound_set_scale(arguments[0], (*(float *)&arguments[1]));
		break;
	case 330:
		scripted_looping_sound_set_alternate(arguments[0], (boolean)arguments[1]);
		break;
	case 360:
		hud_unit_activate_nav_point_with_flag((unsigned short)arguments[0], arguments[1], (unsigned short)arguments[2], (*(float *)&arguments[3]));
		break;
	case 361:
		hud_unit_activate_nav_point_with_object((unsigned short)arguments[0], arguments[1], arguments[2], (*(float *)&arguments[3]));
		break;
	case 362:
		hud_activate_team_nav_point_with_flag((unsigned short)arguments[0], (unsigned short)arguments[1], (unsigned short)arguments[2], (*(float *)&arguments[3]));
		break;
	case 363:
		hud_activate_team_nav_point_with_object((unsigned short)arguments[0], (unsigned short)arguments[1], arguments[2], (*(float *)&arguments[3]));
		break;
	case 364:
		hud_unit_deactivate_nav_point_with_flag(arguments[0], (short)arguments[1]);
		break;
	case 365:
		hud_unit_deactivate_nav_point_with_object(arguments[0], arguments[1]);
		break;
	case 366:
		hud_deactivate_team_nav_point_with_flag((short)arguments[0], (short)arguments[1]);
		break;
	case 367:
		hud_deactivate_team_nav_point_with_object((short)arguments[0], arguments[1]);
		break;

	case 42:
		hs_object_create_anew_containing(arguments[0] ? (char const *)(uintptr_t)arguments[0] : "");
		break;
	case 43:
		hs_object_destroy_containing(arguments[0] ? (char const *)(uintptr_t)arguments[0] : "");
		break;
	case 53:
		hs_objects_delete_by_definition(arguments[0]);
		break;
	case 69:
		recorded_animation_kill(arguments[0]);
		break;
	case 70:
		result = recorded_animation_get_time_left(arguments[0]);
		break;
	case 82:
		hs_objects_predict(arguments[0]);
		break;
	case 89:
		result = scenery_get_animation_time(arguments[0]);
		break;
	case 103:
		result = campaign_bool(unit_is_playing_custom_animation(arguments[0]));
		break;
	case 124:
		*(float *)&result = unit_scripting_get_health(arguments[0]);
		break;
	case 125:
		*(float *)&result = unit_scripting_get_shield(arguments[0]);
		break;
	case 126:
		result = unit_scripting_get_grenade_count(arguments[0]);
		break;
	case 135:
		result = campaign_bool(unit_get_current_flashlight_state(arguments[0]));
		break;
	case 142:
		*(float *)&result = device_group_get_value((unsigned short)arguments[0]);
		break;
	case 234:
		if (actor_data) result = campaign_bool(ai_scripting_conversation(arguments[0]));
		break;
	case 249:
		scripted_camera_set_first_person(arguments[0]);
		break;
	case 250:
		scripted_camera_set_dead(arguments[0]);
		break;
	case 301:
		cinematic_suppress_bsp_object_creation((boolean)arguments[0]);
		break;
	case 324:
		result = scripted_sound_time(arguments[0]);
		break;
	case 325:
		scripted_sound_stop(arguments[0]);
		break;
	case 326:
		scripted_foley_predict(arguments[0]);
		break;
	case 328:
		scripted_looping_sound_stop(arguments[0]);
		break;
	case 292:
		player_effect_screen_fade_in(*(float *)&arguments[0], *(float *)&arguments[1], *(float *)&arguments[2], (short)arguments[3]);
		break;
	case 293:
		player_effect_screen_fade_out(*(float *)&arguments[0], *(float *)&arguments[1], *(float *)&arguments[2], (short)arguments[3]);
		break;
	case 358:
		scripted_hud_set_flashing_state((boolean)arguments[0]);
		break;
	case 359:
		scripted_hud_restart_flashing();
		break;
	case 371:
		scripted_player_effect_set_translation(*(float *)&arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);
		break;
	case 372:
		scripted_player_effect_set_rotation(*(float *)&arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);
		break;
	case 373:
		scripted_player_effect_set_rumble(*(float *)&arguments[0], *(float *)&arguments[1]);
		break;
	case 374:
		scripted_player_effect_start(*(float *)&arguments[0], *(float *)&arguments[1]);
		break;
	case 375:
		scripted_player_effect_stop(*(float *)&arguments[0]);
		break;
	case 383:
		scripted_hud_messages_clear();
		break;
	case 384:
		scripted_hud_set_state_message((short)arguments[0]);
		break;
	case 385:
		scripted_hud_set_objective((short)arguments[0]);
		break;
	case 386:
		scripted_hud_set_timer_time((short)arguments[0], (word)arguments[1]);
		break;
	case 387:
		scripted_hud_set_timer_warning_cutoff((short)arguments[0], (word)arguments[1]);
		break;
	case 388:
		scripted_hud_set_timer_position((short)arguments[0], (short)arguments[1], (short)arguments[2]);
		break;
	case 389:
		scripted_hud_show_timer((boolean)arguments[0]);
		break;
	case 390:
		scripted_hud_pause_timer((boolean)arguments[0]);
		break;
	case 391:
		result = scripted_hud_get_timer_ticks();
		break;
	case 392:
		scripted_hud_time_code_show((boolean)arguments[0]);
		break;
	case 393:
		scripted_hud_time_code_start((boolean)arguments[0]);
		break;
	case 394:
		scripted_hud_time_code_reset();
		break;
	case 45:
		result = hs_object_list_get_element(arguments[0], (short)arguments[1]);
		break;
	case 55:
		*(float *)&result = hs_sound_get_gain(arguments[0] ? (char const *)(uintptr_t)arguments[0] : "");
		break;
	case 75:
		objects_scripting_set_scale(arguments[0], *(float *)&arguments[1], (short)arguments[2]);
		break;
	case 112:
		unit_scripting_set_maximum_vitality(arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);
		break;
	case 113:
		units_scripting_set_maximum_vitality(arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);
		break;
	case 115:
		units_scripting_set_current_vitality(arguments[0], *(float *)&arguments[1], *(float *)&arguments[2]);
		break;
	case 143:
		result = campaign_bool(device_group_set_desired_value((short)arguments[0], *(float *)&arguments[1]));
		break;
	case 354:
		player_add_equipment(arguments[0], (short)arguments[1], (boolean)arguments[2]);
		break;
	case 46:
		result = object_list_count(arguments[0]);
		break;
	case 65:
		breakable_surfaces_enable((boolean)arguments[0]);
		break;
	case 254:
		game_time_set_speed(*(float *)&arguments[0]);
		break;
	case 334:
		sound_enable((boolean)arguments[0]);
		break;
	case 357:
		result = campaign_bool(scripted_show_hud_help_text((boolean)arguments[0]));
		break;
	case 61:
		numeric_countdown_timer_set(arguments[0], (boolean)arguments[1]);
		break;
	case 62:
		result = numeric_countdown_timer_get((short)arguments[0]);
		break;
	case 63:
		numeric_countdown_timer_stop();
		break;
	case 64:
		numeric_countdown_timer_restart();
		break;
	case 335:
		sound_class_set_gain(arguments[0] ? (char const *)(uintptr_t)arguments[0] : "", *(float *)&arguments[1], (short)arguments[2]);
		break;
	case 339:
		result = campaign_bool(scripted_player_control_set_camera_control((boolean)arguments[0]));
		break;
	case 340:
		player_control_action_test_reset();
		break;
	case 341:
		result = campaign_bool(player_control_action_test_jump());
		break;
	case 342:
		result = campaign_bool(player_control_action_test_primary_trigger());
		break;
	case 343:
		result = campaign_bool(player_control_action_test_grenade_trigger());
		break;
	case 344:
		result = campaign_bool(player_control_action_test_zoom());
		break;
	case 345:
		result = campaign_bool(player_control_action_test_action());
		break;
	case 346:
		result = campaign_bool(player_control_action_test_accept());
		break;
	case 347:
		result = campaign_bool(player_control_action_test_back());
		break;
	case 348:
		result = campaign_bool(player_control_action_test_look_relative_up());
		break;
	case 349:
		result = campaign_bool(player_control_action_test_look_relative_down());
		break;
	case 350:
		result = campaign_bool(player_control_action_test_look_relative_left());
		break;
	case 351:
		result = campaign_bool(player_control_action_test_look_relative_right());
		break;
	case 352:
		result = campaign_bool(player_control_action_test_look_relative_all_directions());
		break;
	case 353:
		result = campaign_bool(player_control_action_test_move_relative_all_directions());
		break;
	case 411:
		player0_look_invert_pitch((boolean)arguments[0]);
		break;
	case 412:
		result = campaign_bool(player0_look_pitch_is_inverted());
		break;
	case 413:
		result = campaign_bool(player0_joystick_set_is_normal());
		break;
	case 51:
		result = campaign_bool(hs_objects_can_see_object(arguments[0], arguments[1], *(float *)&arguments[2]));
		break;
	case 52:
		result = campaign_bool(hs_objects_can_see_flag(arguments[0], (short)arguments[1], *(float *)&arguments[2]));
		break;
	case 100:
		result = campaign_bool(unit_custom_animation_at_frame(
			arguments[0], arguments[1],
			arguments[2] ? (char const *)(uintptr_t)arguments[2] : "",
			(boolean)arguments[3], (short)arguments[4]));
		break;
	case 59:
		result = seed_random_range(get_global_random_seed_address(), (short)arguments[0], (short)arguments[1]);
		break;
	case 60:
		*(float *)&result = real_seed_random_range(get_global_random_seed_address(), *(float *)&arguments[0], *(float *)&arguments[1]);
		break;
