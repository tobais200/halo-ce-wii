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
