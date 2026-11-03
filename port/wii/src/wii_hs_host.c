/*
 * Device layer under OpenCE hs_runtime.c.
 *
 * The bytecode walker, special forms (begin, if, set, arithmetic, compare,
 * sleep, wake), and the thread scheduler are the OpenCE functions. This
 * file supplies the data arrays, the function table, and the calls those
 * functions make into the rest of the Xbox game. Object, unit, weapon, and
 * AI bodies are not linked: a call past the special forms returns 0 and is
 * counted. game.c is not compiled; its include list is the rest of the
 * engine plus the Xbox SDK.
 */
#define BUILDING_CSERIES

#include "cseries/cseries.h"
#include "math/real_math.h"
#include "memory/data.h"
#include "tag_files/tag_groups.h"
#include "hs/hs.h"
#include "hs/hs_library_internal.h"
#include "hs/hs_scenario_definitions.h"
#include "hs/object_lists.h"
#include "scenario/scenario_definitions.h"
#include "objects/objects.h"
#include "physics/collisions.h"
#include "render/render.h"
#include "ai/ai_debug_scripting.h"
#include "game/game.h"
#include "wii_hs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char wii_hs_data_array_size[sizeof(struct data_array) == 0x38 ? 1 : -1];
typedef char wii_hs_node_size[sizeof(struct hs_syntax_node) == 0x14 ? 1 : -1];
typedef char wii_hs_script_size[sizeof(struct hs_script) == 0x5C ? 1 : -1];
typedef char wii_hs_players_offset[
	__builtin_offsetof(struct scenario, players) == 0x354 ? 1 : -1];
typedef char wii_hs_weapons_offset[
	__builtin_offsetof(struct scenario, weapons) == 0x270 ? 1 : -1];
typedef char wii_hs_bipeds_offset[
	__builtin_offsetof(struct scenario, bipeds) == 0x228 ? 1 : -1];
typedef char wii_hs_encounters_offset[
	__builtin_offsetof(struct scenario, ai_encounters) == 0x42C ? 1 : -1];
typedef char wii_hs_syntax_offset[
	__builtin_offsetof(struct scenario, hs_syntax_data) == 0x474 ? 1 : -1];
typedef char wii_hs_scripts_offset[
	__builtin_offsetof(struct scenario, hs_scripts) == 0x49C ? 1 : -1];
typedef char wii_hs_bsp_offset[
	__builtin_offsetof(struct scenario, structure_bsp_references) == 0x5A4 ? 1 : -1];
typedef char wii_hs_biped_palette_offset[
	__builtin_offsetof(struct scenario, biped_palette) == 0x234 ? 1 : -1];
typedef char wii_hs_palette_entry_size[
	sizeof(struct scenario_object_palette_entry) == 0x30 ? 1 : -1];

enum {
	WII_HS_FUNCTIONS = 418 + 7,
	WII_HS_EXTERNAL_GLOBALS = 443,
	WII_HS_SCRIPTS = 512,
	WII_HS_GLOBALS = 512
};

char temporary[256];
long global_scenario_index = NONE;
boolean terminal_command_running = FALSE;
struct render_globals render;
struct data_array *hs_syntax_data;
short const hs_external_global_count = WII_HS_EXTERNAL_GLOBALS;
long const hs_function_table_count = WII_HS_FUNCTIONS;

boolean const _hs_type_boolean_default = FALSE;
real const _hs_type_real_default = 0.0f;
short const _hs_type_short_integer_default = 0;
long const _hs_type_long_integer_default = 0;
char const *_hs_type_string_default = "";
short const _hs_type_script_default = NONE;
short const _hs_type_trigger_volume_default = NONE;
short const _hs_type_cutscene_flag_default = NONE;
short const _hs_type_cutscene_camera_point_default = NONE;
short const _hs_type_cutscene_title_default = NONE;
short const _hs_type_cutscene_recording_default = NONE;
short const _hs_type_device_group_default = NONE;
long const _hs_type_ai_default = NONE;
short const _hs_type_ai_command_list_default = NONE;
short const _hs_type_starting_profile_default = NONE;
short const _hs_type_conversation_default = NONE;
short const _hs_type_navpoint_default = NONE;
short const _hs_type_hud_message_default = NONE;
long const _hs_type_object_list_default = NONE;
long const _hs_type_sound_default = NONE;
long const _hs_type_effect_default = NONE;
long const _hs_type_damage_default = NONE;
long const _hs_type_looping_sound_default = NONE;
long const _hs_type_animation_graph_default = NONE;
long const _hs_type_actor_variant_default = NONE;
long const _hs_type_damage_effect_default = NONE;
long const _hs_type_object_definition_default = NONE;
short const _hs_type_enum_game_difficulty_default = NONE;
short const _hs_type_enum_team_default = NONE;
short const _hs_type_enum_ai_default_state_default = NONE;
short const _hs_type_enum_actor_type_default = NONE;
short const _hs_type_enum_hud_corner_default = NONE;
long const _hs_type_object_default = NONE;
long const _hs_type_unit_default = NONE;
long const _hs_type_vehicle_default = NONE;
long const _hs_type_weapon_default = NONE;
long const _hs_type_device_default = NONE;
long const _hs_type_scenery_default = NONE;
short const _hs_type_object_name_default = NONE;

char const *hs_type_names[NUMBER_OF_HS_TYPES] = {
	"unparsed", "special form", "function name", "passthrough", "void",
	"boolean", "real", "short", "long", "string", "script",
	"trigger_volume", "cutscene_flag", "cutscene_camera_point",
	"cutscene_title", "cutscene_recording", "device_group", "ai",
	"ai_command_list", "starting_profile", "conversation", "navpoint",
	"hud_message", "object_list", "sound", "effect", "damage",
	"looping_sound", "animation_graph", "actor_variant", "damage_effect",
	"object_definition", "game_difficulty", "team", "ai_default_state",
	"actor_type", "hud_corner", "object", "unit", "vehicle", "weapon",
	"device", "scenery", "object_name", "unit_name", "vehicle_name",
	"weapon_name", "device_name", "scenery_name"
};

word const hs_object_type_masks[NUMBER_OF_HS_OBJECT_TYPES] = {
	0xFFFF, 0x0003, 0x0002, 0x0004, 0x0380, 0x0040
};

short const hs_type_sizes[NUMBER_OF_HS_TYPES] = {
	0, 0, 0, 0, 0,
	1, 4, 2, 4, 4,
	4, 2, 2, 2, 2,
	2, 2, 4, 2, 2,
	2, 2, 2, 4, 4,
	4, 4, 4, 4, 4,
	4, 4, 2, 2, 2,
	2, 2, 4, 4, 4,
	4, 4, 4, 2, 2,
	2, 2, 2, 2
};

typedef char wii_hs_type_size_count[
	sizeof(hs_type_sizes) / sizeof(hs_type_sizes[0]) == NUMBER_OF_HS_TYPES ? 1 : -1];

static char const *const wii_enum_none[] = { "none" };
struct hs_enum_definition const hs_enum_table[] = {
	{ 1, 0, (char const **)wii_enum_none },
	{ 1, 0, (char const **)wii_enum_none },
	{ 1, 0, (char const **)wii_enum_none },
	{ 1, 0, (char const **)wii_enum_none },
	{ 1, 0, (char const **)wii_enum_none }
};

static struct scenario wii_scenario;
static struct hs_script wii_scripts[WII_HS_SCRIPTS];
static struct hs_global wii_globals[WII_HS_GLOBALS];
static uint8_t wii_syntax[WII_HS_SYNTAX_BYTES];
static uint8_t wii_strings[0x10000];
static uint32_t wii_string_bytes;
static int wii_runtime_ready;
static int wii_mounted;
static long wii_time;
static int wii_unlinked;
static int wii_last_unlinked = NONE;
static char wii_note[96];
static unsigned long wii_seed = 1u;
static union real_argb_color wii_white = { .n = { 1.f, 1.f, 1.f, 1.f } };
static union real_argb_color wii_yellow = { .n = { 1.f, 1.f, 1.f, 0.f } };
static union real_argb_color wii_blue = { .n = { 1.f, 0.f, 0.f, 1.f } };
static union real_argb_color wii_red = { .n = { 1.f, 1.f, 0.f, 0.f } };
static real_matrix4x3 wii_identity;

const union real_argb_color *global_real_argb_white = &wii_white;
const union real_argb_color *global_real_argb_yellow = &wii_yellow;
const union real_argb_color *global_real_argb_blue = &wii_blue;
const union real_argb_color *global_real_argb_red = &wii_red;
const real_matrix4x3 *global_identity4x3 = &wii_identity;

extern struct data_array *hs_global_data;
extern struct data_array *hs_thread_data;

static struct hs_function_definition wii_functions[WII_HS_FUNCTIONS];
static int wii_table_ready;

static uint16_t le16(uint8_t const *bytes)
{
	return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

static int32_t le32s(uint8_t const *bytes)
{
	return (int32_t)((uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8)
		| ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24));
}

static void note(char const *text)
{
	if (!wii_note[0] && text)
		snprintf(wii_note, sizeof(wii_note), "%s", text);
}

void wii_hs_evaluate_unlinked(short function_index, long thread_index, boolean initialize)
{
	(void)initialize;
	wii_unlinked++;
	wii_last_unlinked = function_index;
	hs_return(thread_index, 0);
}

static void fill_function(int index, short return_type, char const *name, hs_evaluate_procedure evaluate)
{
	struct hs_function_definition *function = &wii_functions[index];

	function->return_type = return_type;
	function->flags = 0;
	function->name = name;
	function->parse = NULL;
	function->evaluate = evaluate;
	function->help = name;
	function->usage = "";
	function->parameter_count = 0;
	function->parameter_types[0] = _hs_type_void;
}

static void wii_hs_table_init(void)
{
	int index;

	if (wii_table_ready)
		return;
	wii_identity.scale = 1.f;
	wii_identity.forward.i = 1.f;
	wii_identity.left.j = 1.f;
	wii_identity.up.k = 1.f;
	for (index = 0; index < WII_HS_FUNCTIONS; index++)
		fill_function(index, _hs_passthrough, "unlinked", wii_hs_evaluate_unlinked);
	/* Same indices and evaluators as hs.c's hs_function_table. cond's
	   retail evaluate pointer is NULL; the compiler rewrites cond, and a
	   NULL call would fault, so a node that still says cond returns 0. */
	fill_function(_hs_function_begin, _hs_passthrough, "begin", hs_evaluate_begin);
	fill_function(_hs_function_begin_random, _hs_passthrough, "begin_random", hs_evaluate_begin_random);
	fill_function(_hs_function_if, _hs_passthrough, "if", hs_evaluate_if);
	fill_function(_hs_function_cond, _hs_passthrough, "cond", wii_hs_evaluate_unlinked);
	fill_function(_hs_function_set, _hs_passthrough, "set", hs_evaluate_set);
	fill_function(_hs_function_and, _hs_type_boolean, "and", hs_evaluate_logical);
	fill_function(_hs_function_or, _hs_type_boolean, "or", hs_evaluate_logical);
	fill_function(_hs_function_plus, _hs_type_real, "+", hs_evaluate_arithmetic);
	fill_function(_hs_function_minus, _hs_type_real, "-", hs_evaluate_arithmetic);
	fill_function(_hs_function_times, _hs_type_real, "*", hs_evaluate_arithmetic);
	fill_function(_hs_function_divide, _hs_type_real, "/", hs_evaluate_arithmetic);
	fill_function(_hs_function_min, _hs_type_real, "min", hs_evaluate_arithmetic);
	fill_function(_hs_function_max, _hs_type_real, "max", hs_evaluate_arithmetic);
	fill_function(_hs_function_equal, _hs_type_boolean, "=", hs_evaluate_equality);
	fill_function(_hs_function_not_equal, _hs_type_boolean, "!=", hs_evaluate_equality);
	fill_function(_hs_function_gt, _hs_type_boolean, ">", hs_evaluate_inequality);
	fill_function(_hs_function_lt, _hs_type_boolean, "<", hs_evaluate_inequality);
	fill_function(_hs_function_gte, _hs_type_boolean, ">=", hs_evaluate_inequality);
	fill_function(_hs_function_lte, _hs_type_boolean, "<=", hs_evaluate_inequality);
	fill_function(_hs_function_sleep, _hs_type_void, "sleep", hs_evaluate_sleep);
	fill_function(_hs_function_sleep_until, _hs_type_void, "sleep_until", hs_evaluate_sleep_until);
	fill_function(_hs_function_wake, _hs_type_void, "wake", hs_evaluate_wake);
	fill_function(_hs_function_inspect, _hs_type_void, "inspect", hs_evaluate_inspect);
	fill_function(_hs_function_object_to_unit, _hs_type_unit, "unit", hs_evaluate_object_cast_up);
	fill_function(_hs_function_debug_string__ai_debug_communication_suppress, _hs_type_void,
		"ai_debug_communication_suppress", hs_evaluate_debug_string);
	fill_function(_hs_function_debug_string__ai_debug_communication_ignore, _hs_type_void,
		"ai_debug_communication_ignore", hs_evaluate_debug_string);
	fill_function(_hs_function_debug_string__ai_debug_communication_focus, _hs_type_void,
		"ai_debug_communication_focus", hs_evaluate_debug_string);
	wii_table_ready = 1;
}

struct hs_function_definition *hs_function_get(short function_index)
{
	wii_hs_table_init();
	if (function_index < 0 || function_index >= WII_HS_FUNCTIONS)
		return &wii_functions[0];
	return &wii_functions[function_index];
}

static struct hs_external_global_definition wii_external = {
	"external", _hs_type_void, 0, NULL
};

struct hs_external_global_definition *hs_global_external_get(short global_index)
{
	(void)global_index;
	return &wii_external;
}

short hs_global_get_type(short global_index)
{
	if (global_index & 0x8000) {
		if ((global_index & 0x7fff) >= hs_external_global_count)
			return _hs_unparsed;
		return wii_external.type;
	}
	if ((global_index & 0x7fff) >= wii_scenario.hs_globals.count)
		return _hs_unparsed;
	return wii_globals[global_index & 0x7fff].type;
}

char const *hs_global_get_name(short global_index)
{
	if (global_index & 0x8000)
		return wii_external.name;
	if ((global_index & 0x7fff) >= wii_scenario.hs_globals.count)
		return "";
	return wii_globals[global_index & 0x7fff].name;
}

boolean hs_scenario_script_disabled(short script_index)
{
	(void)script_index;
	return FALSE;
}

boolean hs_scenario_global_initializer_disabled(short global_index)
{
	(void)global_index;
	return FALSE;
}

void hs_node_gc(void)
{
	long syntax_node_index;

	if (!hs_syntax_data)
		return;
	for (syntax_node_index = data_next_index(hs_syntax_data, NONE);
		syntax_node_index != NONE;
		syntax_node_index = data_next_index(hs_syntax_data, syntax_node_index)) {
		struct hs_syntax_node *syntax_node;

		syntax_node = (struct hs_syntax_node *)datum_get(hs_syntax_data, syntax_node_index);
		if (syntax_node && !(((byte)syntax_node->flags) & 8))
			datum_delete(hs_syntax_data, syntax_node_index);
	}
}

struct scenario *global_scenario_get(void)
{
	return &wii_scenario;
}

struct data_array *game_state_data_new(char const *name, short maximum_count, short size)
{
	return data_new(name, maximum_count, size);
}

long game_time_get(void)
{
	return wii_time;
}

boolean game_in_progress(void)
{
	return TRUE;
}

boolean terminal_shows(short kind)
{
	(void)kind;
	return TRUE;
}

void console_printf(boolean clear, char const *format, ...)
{
	va_list args;

	(void)clear;
	va_start(args, format);
	vsnprintf(wii_note, sizeof(wii_note), format, args);
	va_end(args);
}

void error(short priority, char const *format, ...)
{
	va_list args;

	(void)priority;
	va_start(args, format);
	vsnprintf(wii_note, sizeof(wii_note), format, args);
	va_end(args);
}

void display_assert(char *information, char *file, long line, boolean fatal)
{
	(void)file;
	(void)line;
	(void)fatal;
	note(information ? information : "assert");
}

void release_assert_failed(char const *information, char const *file, long line, boolean fatal)
{
	(void)file;
	(void)line;
	(void)fatal;
	note(information ? information : "assert");
}

void system_exit(long code)
{
	(void)code;
	note("script halted");
}

void halt_and_catch_fire(void)
{
	note("script halted");
}

void platform_log(char const *format, ...)
{
	(void)format;
}

void *tag_block_get_element_with_size(struct tag_block const *block, long index, long element_size)
{
	if (!block || index < 0 || index >= block->count || !block->address)
		return NULL;
	return (char *)block->address + index * element_size;
}

int _stricmp(char const *a, char const *b)
{
	unsigned char ca;
	unsigned char cb;

	if (!a || !b)
		return a ? 1 : (b ? -1 : 0);
	do {
		ca = (unsigned char)*a++;
		cb = (unsigned char)*b++;
		if (ca >= 'A' && ca <= 'Z')
			ca = (unsigned char)(ca - 'A' + 'a');
		if (cb >= 'A' && cb <= 'Z')
			cb = (unsigned char)(cb - 'A' + 'a');
	} while (ca && ca == cb);
	return (int)ca - (int)cb;
}

char *csprintf(char *buffer, char *format, ...)
{
	va_list args;

	va_start(args, format);
	vsnprintf(buffer, 256, format, args);
	va_end(args);
	return buffer;
}

void *csmemset(void *buffer, long c, unsigned long size)
{
	return memset(buffer, (int)c, size);
}

void *csmemcpy(void *destination, void const *source, unsigned long size)
{
	return memcpy(destination, source, size);
}

long csmemcmp(void const *p1, void const *p2, unsigned long size)
{
	return memcmp(p1, p2, size);
}

unsigned long csstrlen(char const *s1)
{
	return (unsigned long)strlen(s1);
}

char *csstrcat(char *s1, char const *s2)
{
	return strcat(s1, s2);
}

char *csstrncpy(char *s1, char const *s2, unsigned long size)
{
	return strncpy(s1, s2, size);
}

long csstrcmp(char const *s1, char const *s2)
{
	return strcmp(s1, s2);
}

void *debug_malloc(unsigned int size, boolean clear, char const *file, long line)
{
	void *block;

	(void)file;
	(void)line;
	block = malloc(size);
	if (block && clear)
		memset(block, 0, size);
	return block;
}

void debug_free(void *pointer, char const *file, long line)
{
	(void)file;
	(void)line;
	free(pointer);
}

void *debug_realloc(void *pointer, unsigned int size, char const *file, long line)
{
	(void)file;
	(void)line;
	return realloc(pointer, size);
}

unsigned long *get_global_random_seed_address(void)
{
	return &wii_seed;
}

unsigned short seed_random(unsigned long *seed)
{
	*seed = (*seed * 1664525u) + 1013904223u;
	return (unsigned short)(*seed >> 16);
}

short seed_random_range(unsigned long *seed, short lower_bound, short upper_bound)
{
	int span = (int)upper_bound - (int)lower_bound;

	if (span <= 0)
		return lower_bound;
	return (short)(lower_bound + (short)(seed_random(seed) % (unsigned short)span));
}

long object_list_from_ai_reference(long ai_reference)
{
	(void)ai_reference;
	wii_unlinked++;
	return NONE;
}

long object_list_new(void)
{
	return NONE;
}

void object_list_add(long object_list_index, long object_index)
{
	(void)object_list_index;
	(void)object_index;
}

void object_list_add_reference(long object_list_index)
{
	(void)object_list_index;
}

void object_list_remove_reference(long object_list_index)
{
	(void)object_list_index;
}

void object_list_gc(void)
{
}

struct object_datum *wii_object_get(long index)
{
	static struct object_datum missing;

	(void)index;
	missing.definition_index = NONE;
	/* Not a biped or vehicle, so unit() refuses the cast. */
	missing.object.type = 2;
	return &missing;
}

long object_index_from_name_index(short name_index)
{
	(void)name_index;
	return NONE;
}

char *tag_get_name(long tag_index)
{
	(void)tag_index;
	return "";
}

void ai_debug_communication_suppress(long name_count, char const **names)
{
	(void)name_count;
	(void)names;
}

void ai_debug_communication_ignore(long name_count, char const **names)
{
	(void)name_count;
	(void)names;
}

void ai_debug_communication_focus(long name_count, char const **names)
{
	(void)name_count;
	(void)names;
}

boolean collision_test_vector(
	unsigned long flags,
	real_point3d const *point,
	real_vector3d const *vector,
	long ignore_object_index,
	struct collision_result *collision)
{
	(void)flags;
	(void)point;
	(void)vector;
	(void)ignore_object_index;
	(void)collision;
	return FALSE;
}

void render_debug_string(boolean immediate, char const *string)
{
	(void)immediate;
	note(string);
}

void render_debug_string_at_point(
	boolean immediate,
	real_point3d const *point,
	char const *string,
	real_argb_color const *color)
{
	(void)immediate;
	(void)point;
	(void)color;
	note(string);
}

void render_debug_polygon(real_point3d const *points, short point_count, real_argb_color const *color)
{
	(void)points;
	(void)point_count;
	(void)color;
}

void render_debug_polygon_edges(real_point3d const *points, short point_count, real_argb_color const *color)
{
	(void)points;
	(void)point_count;
	(void)color;
}

void draw_string_set_tab_stops(short const *tab_stops, short count)
{
	(void)tab_stops;
	(void)count;
}

void matrix4x3_from_point_and_vectors(
	real_matrix4x3 *matrix,
	real_point3d const *point,
	real_vector3d const *forward,
	real_vector3d const *up)
{
	(void)point;
	(void)forward;
	(void)up;
	if (matrix)
		*matrix = wii_identity;
}

real_vector3d *matrix4x3_transform_vector(
	real_matrix4x3 const *matrix,
	real_vector3d const *vector,
	real_vector3d *result)
{
	(void)matrix;
	if (result && vector)
		*result = *vector;
	return result;
}

static void swap_node(uint8_t *node)
{
	struct hs_syntax_node native;

	memset(&native, 0, sizeof(native));
	native.datum_header = (short)le16(node);
	native.function_index = (short)le16(node + 2);
	native.type = (short)le16(node + 4);
	native.flags = (short)le16(node + 6);
	native.next_node_index = le32s(node + 8);
	native.string_offset = le32s(node + 12);
	native.data = le32s(node + 16);
	memcpy(node, &native, sizeof(native));
}

static int swap_syntax(uint8_t const *source, uint32_t bytes)
{
	struct data_array header;
	uint32_t index;
	uint32_t nodes;

	if (bytes < sizeof(header) || bytes > sizeof(wii_syntax))
		return -1;
	if (((bytes - sizeof(header)) % sizeof(struct hs_syntax_node)) != 0)
		return -1;
	memset(&header, 0, sizeof(header));
	memcpy(header.name, source, 32);
	header.maximum_count = (short)le16(source + 0x20);
	header.size = (short)le16(source + 0x22);
	header.valid = source[0x24];
	header.identifier_zero_invalid = source[0x25];
	header.signature = (unsigned long)le32s(source + 0x28);
	header.first_free_absolute_index = (short)le16(source + 0x2C);
	header.count = (short)le16(source + 0x2E);
	header.actual_count = (short)le16(source + 0x30);
	header.next_identifier = (short)le16(source + 0x32);
	if (header.signature != (unsigned long)'d@t@' || header.size != (short)sizeof(struct hs_syntax_node))
		return -1;
	if (header.maximum_count <= 0 || header.count < 0 || header.count > header.maximum_count)
		return -1;
	nodes = (bytes - (uint32_t)sizeof(header)) / sizeof(struct hs_syntax_node);
	if ((uint32_t)header.maximum_count > nodes || header.maximum_count > WII_HS_SYNTAX_MAX_NODES)
		return -1;
	memset(wii_syntax, 0, sizeof(wii_syntax));
	memcpy(wii_syntax, &header, sizeof(header));
	memcpy(wii_syntax + sizeof(header), source + sizeof(header), nodes * sizeof(struct hs_syntax_node));
	for (index = 0; index < nodes; index++)
		swap_node(wii_syntax + sizeof(header) + index * sizeof(struct hs_syntax_node));
	hs_syntax_data = (struct data_array *)wii_syntax;
	hs_syntax_data->data = wii_syntax + sizeof(header);
	return 0;
}

static void load_scripts(uint8_t const *source, int count)
{
	int index;

	if (count > WII_HS_SCRIPTS)
		count = WII_HS_SCRIPTS;
	memset(wii_scripts, 0, sizeof(wii_scripts));
	for (index = 0; index < count; index++) {
		uint8_t const *record = source + index * 0x5C;

		memcpy(wii_scripts[index].name, record, 31);
		wii_scripts[index].script_type = (short)le16(record + 32);
		wii_scripts[index].return_type = (short)le16(record + 34);
		wii_scripts[index].root_expression_index = le32s(record + 36);
	}
	wii_scenario.hs_scripts.count = count;
	wii_scenario.hs_scripts.address = wii_scripts;
}

static void load_globals(uint8_t const *source, int count)
{
	int index;

	if (count < 0)
		count = 0;
	if (count > WII_HS_GLOBALS || count > 0x400 - WII_HS_EXTERNAL_GLOBALS) {
		note("too many script globals");
		count = 0;
		wii_scenario.hs_scripts.count = 0;
	}
	memset(wii_globals, 0, sizeof(wii_globals));
	for (index = 0; index < count; index++) {
		uint8_t const *record = source + index * 0x5C;

		memcpy(wii_globals[index].name, record, 31);
		wii_globals[index].type = (short)le16(record + 32);
		wii_globals[index].initialization_expression_index = le32s(record + 40);
	}
	wii_scenario.hs_globals.count = count;
	wii_scenario.hs_globals.address = wii_globals;
}

int wii_hs_mount(
	uint8_t const *syntax_le,
	uint32_t syntax_bytes,
	uint8_t const *scripts_le,
	int script_count,
	uint8_t const *globals_le,
	int global_count,
	uint8_t const *strings,
	uint32_t string_bytes)
{
	wii_hs_table_init();
	wii_note[0] = 0;
	wii_unlinked = 0;
	wii_last_unlinked = NONE;
	wii_time = 0;
	if (wii_mounted)
		hs_runtime_dispose_from_old_map();
	wii_mounted = 0;
	global_scenario_index = NONE;
	memset(&wii_scenario, 0, sizeof(wii_scenario));
	wii_string_bytes = 0;
	if (strings && string_bytes) {
		if (string_bytes > sizeof(wii_strings))
			string_bytes = sizeof(wii_strings);
		memcpy(wii_strings, strings, string_bytes);
		wii_string_bytes = string_bytes;
		wii_scenario.hs_string_constants.size = (long)string_bytes;
		wii_scenario.hs_string_constants.address = wii_strings;
	}
	if (!syntax_le || swap_syntax(syntax_le, syntax_bytes) != 0) {
		note("syntax data is not a script node array");
		return -1;
	}
	if (scripts_le && script_count > 0)
		load_scripts(scripts_le, script_count);
	if (globals_le && global_count > 0)
		load_globals(globals_le, global_count);
	if (!wii_runtime_ready) {
		hs_runtime_initialize();
		wii_runtime_ready = hs_thread_data != NULL;
	}
	if (!wii_runtime_ready) {
		note("no script threads");
		return -1;
	}
	global_scenario_index = 0;
	hs_runtime_initialize_for_new_map();
	wii_mounted = 1;
	hs_runtime_update();
	return 0;
}

void wii_hs_shutdown(void)
{
	if (wii_mounted && wii_runtime_ready)
		hs_runtime_dispose_from_old_map();
	wii_mounted = 0;
	global_scenario_index = NONE;
}

int wii_hs_ready(void)
{
	return wii_mounted;
}

void wii_hs_set_time(uint32_t ticks)
{
	wii_time = (long)ticks;
}

void wii_hs_update(void)
{
	if (wii_mounted)
		hs_runtime_update();
}

int wii_hs_unlinked(void)
{
	return wii_unlinked;
}

int wii_hs_last_unlinked(void)
{
	return wii_last_unlinked;
}

int wii_hs_global_real(int index, float *value)
{
	long datum_index;
	uint8_t *global;

	if (!wii_mounted || !value || index < 0 || index >= wii_scenario.hs_globals.count)
		return -1;
	if (wii_globals[index].type != _hs_type_real)
		return -1;
	datum_index = DATUM_INDEX_NEW(index + WII_HS_EXTERNAL_GLOBALS, 0xaced);
	global = datum_get(hs_global_data, datum_index);
	if (!global)
		return -1;
	/* hs_global_datum: short identifier, short unused, long value. */
	memcpy(value, global + 4, sizeof(*value));
	return 0;
}

char const *wii_hs_note(void)
{
	return wii_note;
}
