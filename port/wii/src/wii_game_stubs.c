/*
 * Device stand-ins for the calls OpenCE game.c still makes into Xbox,
 * Bink, networking, the cache precacher, and the rasterizer.
 * players.c, units, weapons, ai.c, and objects.c are linked.
 */
struct game_options {
	unsigned long flags;
	short code_version;
	short difficulty;
	unsigned long random_seed;
	char map_name[256];
};

#include "cseries/cseries.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "hs/hs.h"
#include "memory/data.h"
#include "wii_game.h"

#include <stdlib.h>
#include <string.h>

void wii_game_spawn_local_player(void);

struct game_variant game_variant_global;
short player_spawn_count = 1;
boolean debug_game_save = FALSE;

static int booted;
int wii_game_unit_ticks;
int wii_game_ai_ticks;
int wii_game_weapon_ticks;
int wii_game_player_ticks;
int wii_game_objects_placed;
int wii_game_ai_placed;

void *game_state_malloc(char const *name, char const *type, long size)
{
	void *block;

	(void)name;
	(void)type;
	if (size < 0)
		return NULL;
	block = malloc((size_t)size);
	if (block)
		memset(block, 0, (size_t)size);
	return block;
}

void hs_initialize(void)
{
}

void hs_dispose(void)
{
}

void hs_initialize_for_new_map(void)
{
}

void hs_dispose_from_old_map(void)
{
}

void hs_update(void)
{
	hs_runtime_update();
}

int scenario_load(char const *name)
{
	(void)name;
	return 1;
}

struct game_variant *game_engine_get_variant_by_name(struct game_variant *variant, char const *name)
{
	(void)name;
	if (variant)
		memset(variant, 0, sizeof(*variant));
	return variant;
}

void wii_game_boot(void)
{
	struct game_options options;

	if (booted)
		return;
	game_initialize();
	game_options_new(&options);
	if (game_load(&options)) {
		game_initialize_for_new_map();
		wii_game_spawn_local_player();
	}
	booted = 1;
}

void wii_game_tick(void)
{
	wii_game_boot();
	if (game_map_loaded())
		game_tick();
}

void real_math_reset_precision(void) {}
void random_seed_debug_log(boolean log) { (void)log; }
void lock_global_random_seed(void) {}
void unlock_global_random_seed(void) {}
void game_time_initialize(void) {}
void game_time_initialize_for_new_map(void) {}
void game_time_dispose_from_old_map(void) {}
void game_time_dispose(void) {}
void game_statistics_start(void) {}
void game_engine_dispose(void) {}
void game_engine_dispose_from_old_map(void) {}
void game_engine_game_starting(void) {}
void game_engine_initialize(struct game_variant *variant) { (void)variant; }
void game_engine_initialize_for_new_map(void) {}
void game_engine_player_added(long player_index) { (void)player_index; }
boolean game_engine_running(void) { return FALSE; }
void game_engine_update(void) {}
void game_variant_options_default(struct game_variant const *variant, struct game_variant_options *options)
{
	(void)variant;
	if (options)
		memset(options, 0, sizeof(*options));
}

int antennas_update(void) { return 0; }
int bink_playback_dispose(void) { return 0; }
int bink_playback_initialize(void) { return 0; }
int breakable_surfaces_dispose(void) { return 0; }
int breakable_surfaces_dispose_from_old_map(void) { return 0; }
int breakable_surfaces_initialize(void) { return 0; }
int breakable_surfaces_initialize_for_new_map(void) { return 0; }
int cache_files_precache_in_progress(void) { return 0; }
int cache_files_precache_is_copying_map(void) { return 0; }
int cache_files_precache_map_begin(void) { return 0; }
int cache_files_precache_map_end(void) { return 0; }
int cache_files_precache_map_loaded(void) { return 0; }
int cache_files_precache_map_queue_end(void) { return 0; }
int cache_files_precache_map_status(void) { return 0; }
int cache_files_precache_set_priority(void) { return 0; }
int cinematic_dispose(void) { return 0; }
int cinematic_dispose_from_old_map(void) { return 0; }
int cinematic_initialize(void) { return 0; }
int cinematic_initialize_for_new_map(void) { return 0; }
int collision_log_begin_period(void) { return 0; }
int collision_log_end_period(void) { return 0; }
int collision_log_initialize(void) { return 0; }
int console_initialize_for_new_map(void) { return 0; }
int console_warning(void) { return 0; }
int contrails_dispose(void) { return 0; }
int contrails_dispose_from_old_map(void) { return 0; }
int contrails_initialize(void) { return 0; }
int contrails_initialize_for_new_map(void) { return 0; }
int contrails_update(void) { return 0; }
int dangerous_effects_near_player(void) { return 0; }
int dangerous_items_near_player(void) { return 0; }
int decals_dispose(void) { return 0; }
int decals_dispose_from_old_map(void) { return 0; }
int decals_initialize(void) { return 0; }
int decals_initialize_for_new_map(void) { return 0; }
int director_dispose(void) { return 0; }
int director_dispose_from_old_map(void) { return 0; }
int director_initialize(void) { return 0; }
int director_initialize_for_new_map(void) { return 0; }
int display_error_damaged_media(void) { return 0; }
int editor_dispose(void) { return 0; }
int editor_dispose_from_old_map(void) { return 0; }
int editor_initialize(void) { return 0; }
int editor_initialize_for_new_map(void) { return 0; }
int editor_update(void) { return 0; }
int effects_dispose(void) { return 0; }
int effects_dispose_from_old_map(void) { return 0; }
int effects_initialize(void) { return 0; }
int effects_initialize_for_new_map(void) { return 0; }
int effects_update(void) { return 0; }
int event_manager_dispose(void) { return 0; }
int event_manager_initialize(void) { return 0; }
int game_in_editor(void) { return 0; }
int game_modes_options_sanitize(void) { return 0; }
int game_sound_dispose(void) { return 0; }
int game_sound_dispose_from_old_map(void) { return 0; }
int game_sound_initialize(void) { return 0; }
int game_sound_initialize_for_new_map(void) { return 0; }
int game_sound_update(void) { return 0; }
int game_state_check_data_arrays(void) { return 0; }
int game_state_dispose(void) { return 0; }
int game_state_dispose_from_old_map(void) { return 0; }
int game_state_initialize_for_new_map(void) { return 0; }
int hud_update(void) { return 0; }
int initialize_network_game_packets(void) { return 0; }
int input_abstraction_dispose(void) { return 0; }
int input_abstraction_initialize(void) { return 0; }
int interface_dispose(void) { return 0; }
int interface_dispose_from_old_map(void) { return 0; }
int interface_initialize(void) { return 0; }
int interface_initialize_for_new_map(void) { return 0; }
int lights_port_recover(void) { return 0; }
int main_pregame_render(void) { return 0; }
int main_present_frame(void) { return 0; }
int main_queue_map_name(void) { return 0; }
int main_rasterizer_throttle(void) { return 0; }
int main_save_current_solo_map(void) { return 0; }
int network_actors_drive(void) { return 0; }
int network_coop_active(void) { return 0; }
int network_distributed_new_game(void) { return 0; }
int network_game_distributed_client(void) { return 0; }
int network_objects_placed(void) { return 0; }
int numeric_countdown_timer_update(void) { return 0; }
int observer_dispose_from_old_map(void) { return 0; }
int observer_initialize(void) { return 0; }
int observer_initialize_for_new_map(void) { return 0; }
int particles_dispose(void) { return 0; }
int particles_dispose_from_old_map(void) { return 0; }
int particles_initialize(void) { return 0; }
int particles_initialize_for_new_map(void) { return 0; }
int particles_update(void) { return 0; }
int particle_systems_dispose(void) { return 0; }
int particle_systems_dispose_from_old_map(void) { return 0; }
int particle_systems_initialize(void) { return 0; }
int particle_systems_initialize_for_new_map(void) { return 0; }
int particle_systems_update(void) { return 0; }
int player_control_initialize_for_new_map(void) { return 0; }
int player_effect_dispose(void) { return 0; }
int player_effect_dispose_from_old_map(void) { return 0; }
int player_effect_initialize(void) { return 0; }
int player_effect_initialize_for_new_map(void) { return 0; }
int player_effect_update(void) { return 0; }
int player_ui_dispose(void) { return 0; }
int player_ui_initialize(void) { return 0; }
int player_ui_remember_player1_profile(void) { return 0; }
int point_physics_dispose_from_old_map(void) { return 0; }
int point_physics_initialize_for_new_map(void) { return 0; }
int profile_tick_end(void) { return 0; }
int profile_tick_start(void) { return 0; }
int progress_bar_begin(void) { return 0; }
int progress_bar_dispose(void) { return 0; }
int progress_bar_end(void) { return 0; }
int progress_bar_initialize(void) { return 0; }
int rasterizer_dispose_from_old_map(void) { return 0; }
int rasterizer_frame_update(void) { return 0; }
int rasterizer_initialize_for_new_map(void) { return 0; }
int recorded_animations_dispose(void) { return 0; }
int recorded_animations_dispose_from_old_map(void) { return 0; }
int recorded_animations_initialize(void) { return 0; }
int recorded_animations_initialize_for_new_map(void) { return 0; }
int recorded_animations_update(void) { return 0; }
int render_dispose(void) { return 0; }
int render_dispose_from_old_map(void) { return 0; }
int render_initialize(void) { return 0; }
int render_initialize_for_new_map(void) { return 0; }
int render_interpolation_reset(void) { return 0; }
int rumble_dispose(void) { return 0; }
int rumble_dispose_from_old_map(void) { return 0; }
int rumble_initialize(void) { return 0; }
int rumble_initialize_for_new_map(void) { return 0; }
int rumble_update(void) { return 0; }
int saved_game_files_dispose(void) { return 0; }
int saved_game_files_initialize(void) { return 0; }
int scenario_dispose_from_old_map(void) { return 0; }
int scenario_frame_update(void) { return 0; }
int scenario_initialize(void) { return 0; }
int scenario_initialize_for_new_map(void) { return 0; }
int scenario_unload(void) { return 0; }
int sound_classes_dispose(void) { return 0; }
int sound_classes_dispose_from_old_map(void) { return 0; }
int sound_classes_initialize(void) { return 0; }
int sound_classes_initialize_for_new_map(void) { return 0; }
int sound_dispose_from_old_map(void) { return 0; }
int sound_initialize_for_new_map(void) { return 0; }
int structures_dispose(void) { return 0; }
int structures_dispose_from_old_map(void) { return 0; }
int structures_initialize(void) { return 0; }
int structures_initialize_for_new_map(void) { return 0; }
int telnet_console_dispose(void) { return 0; }
int telnet_console_initialize(void) { return 0; }
int transport_dispose(void) { return 0; }
int transport_initialize(void) { return 0; }
int ui_widget_load_progress_widget(void) { return 0; }
int ui_widgets_close_all(void) { return 0; }
int ui_widgets_dispose(void) { return 0; }
int ui_widgets_initialize(void) { return 0; }
int ui_widgets_safe_to_load(void) { return 0; }
int unit_get(void) { return 0; }
int update_server_delete(void) { return 0; }
int update_server_new(void) { return 0; }
int weather_particle_systems_dispose(void) { return 0; }
int weather_particle_systems_dispose_from_old_map(void) { return 0; }
int weather_particle_systems_initialize(void) { return 0; }
int weather_particle_systems_initialize_for_new_map(void) { return 0; }
int widgets_update(void) { return 0; }
