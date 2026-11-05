/*
 * Homebrew Channel entry. Menu lists the player's own Halo cache files.
 * A draws the first structure BSP through GX. The campaign simulation,
 * main-menu widgets, and shaders are not linked.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gccore.h>
#include <wiiuse/wpad.h>

#include "bsp_preview.h"
#include "gx_draw.h"
#include "gx_stub.h"
#include "wii_mem.h"
#include "wii_menu.h"
#include "wii_stream.h"
#include "wii_sim.h"
#include "wii_tex.h"
#include "wii_game.h"
#include "wii_hs.h"
#include "wii_collision.h"
#include "wii_effects.h"
#include "wii_play.h"
#include <ogc/pad.h>
#include "wii_widget.h"

static void *console_xfb = NULL;
static GXRModeObj *rmode = NULL;

struct map_reader {
	FILE *file;
	uint64_t base;
	uint32_t size;
};

static int home_pressed(uint32_t down)
{
	return (down & WPAD_BUTTON_HOME) != 0;
}

static int map_read(void *context, uint64_t offset, void *destination, uint32_t size)
{
	struct map_reader *reader = context;
	uint64_t at;

	if (!reader || !reader->file || !size)
		return -1;
	if ((uint64_t)offset + size > reader->size)
		return -1;
	at = reader->base + offset;
	if (at > WII_MAX_FILE_OFFSET || (uint64_t)size > WII_MAX_FILE_OFFSET
		|| at + size > (uint64_t)WII_MAX_FILE_OFFSET + 1u)
		return -1;
	if (fseek(reader->file, (long)at, SEEK_SET) != 0)
		return -1;
	if (fread(destination, 1, size, reader->file) != size)
		return -1;
	return 0;
}

static uint32_t buttons_down(void)
{
	uint32_t down = 0;
	int chan;

	for (chan = 0; chan < 4; chan++)
		down |= WPAD_ButtonsDown(chan);
	return down;
}

static uint32_t buttons_held(void)
{
	uint32_t held = 0;
	int chan;

	for (chan = 0; chan < 4; chan++)
		held |= WPAD_ButtonsHeld(chan);
	return held;
}

int main(int argc, char **argv)
{
	gx_stub gx;
	struct wii_stream_stats stats;
	struct wii_ram_sample sample;
	struct bsp_preview_store store;
	struct bsp_preview_info preview;
	struct gx_mesh_view view;
	int selected = 0;
	int showing = 0;
	int mode = 0;
	int menu_dirty = 1;
	int widget_dirty = 0;
	float yaw = 0.8f;
	float pitch = 0.4f;
	char note[96];
	struct wii_widget widget;
	struct wii_sim sim;
	struct map_reader ui_reader;
	uint8_t *widget_canvas = 0;
	uint8_t *widget_scratch = 0;
	int ui_index = -1;

	(void)argc;
	(void)argv;
	memset(&store, 0, sizeof(store));
	memset(&preview, 0, sizeof(preview));
	memset(&view, 0, sizeof(view));
	note[0] = 0;

	VIDEO_Init();
	WPAD_Init();
	PAD_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	console_xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	if (!console_xfb)
		exit(1);

	console_init(console_xfb, 20, 20,
		rmode->fbWidth, rmode->xfbHeight,
		rmode->fbWidth * VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(console_xfb);
	VIDEO_SetBlack(false);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	if (gx_stub_init(&gx, rmode, console_xfb) != 0)
		printf("GX init failed\n");
	if (wii_mem2_init() == 0) {
		store.positions = wii_mem2_alloc(WII_PREVIEW_VERTICES * 12u);
		store.colors = wii_mem2_alloc(WII_PREVIEW_VERTICES * 4u);
		store.uvs = wii_mem2_alloc(WII_PREVIEW_VERTICES * 8u);
		store.indices = wii_mem2_alloc(WII_PREVIEW_TRIANGLES * 6u);
		store.vertex_cap = WII_PREVIEW_VERTICES;
		store.triangle_cap = WII_PREVIEW_TRIANGLES;
	}
	wii_stream_bringup(&stats);
	wii_mem_sample(&sample);
	if (!store.positions || !store.colors || !store.uvs || !store.indices)
		snprintf(note, sizeof(note), "MEM2 preview buffers failed");
	widget_canvas = wii_mem2_alloc(WII_GX_TEXTURE_BYTES);
	widget_scratch = wii_mem2_alloc(WII_GX_TEXTURE_BYTES);
	memset(&ui_reader, 0, sizeof(ui_reader));
	memset(&widget, 0, sizeof(widget));
	memset(&sim, 0, sizeof(sim));
	for (selected = 0; selected < stats.map_count; selected++) {
		if (stats.maps[selected].main_menu)
			ui_index = selected;
	}
	selected = 0;
	if (ui_index >= 0 && widget_canvas && widget_scratch) {
		ui_reader.file = fopen(stats.maps[ui_index].path, "rb");
		ui_reader.base = stats.maps[ui_index].base;
		ui_reader.size = stats.maps[ui_index].size;
		if (ui_reader.file && wii_widget_open_root(map_read, &ui_reader, ui_reader.size,
			stats.maps[ui_index].tag_data_offset, stats.maps[ui_index].tag_data_size, &widget) == 0) {
			mode = 1;
			widget_dirty = 1;
		} else if (ui_reader.file) {
			fclose(ui_reader.file);
			ui_reader.file = 0;
			snprintf(note, sizeof(note), "ui cache has no DeLa root");
		}
	}

	while (SYS_MainLoop()) {
		uint32_t down;
		uint32_t held;
		int launch_selected = 0;

		WPAD_ScanPads();
		down = buttons_down();
		held = buttons_held();
		if (home_pressed(down))
			exit(0);

		if (mode == 1) {
			int action = -1;
			if (down & WPAD_BUTTON_UP)
				action = wii_widget_input(&widget, 8, map_read, &ui_reader, ui_reader.size,
					stats.maps[ui_index].tag_data_offset, stats.maps[ui_index].tag_data_size);
			else if (down & WPAD_BUTTON_DOWN)
				action = wii_widget_input(&widget, 9, map_read, &ui_reader, ui_reader.size,
					stats.maps[ui_index].tag_data_offset, stats.maps[ui_index].tag_data_size);
			else if (down & WPAD_BUTTON_A)
				action = wii_widget_input(&widget, 0, map_read, &ui_reader, ui_reader.size,
					stats.maps[ui_index].tag_data_offset, stats.maps[ui_index].tag_data_size);
			else if (down & WPAD_BUTTON_B)
				action = wii_widget_input(&widget, 1, map_read, &ui_reader, ui_reader.size,
					stats.maps[ui_index].tag_data_offset, stats.maps[ui_index].tag_data_size);
			if (action == 0)
				widget_dirty = 1;
			if (action == 2) {
				mode = 0;
				menu_dirty = 1;
				if (ui_reader.file) {
					fclose(ui_reader.file);
					ui_reader.file = 0;
				}
			}
			if (action == 1) {
				int map_index;
				int found = 0;
				for (map_index = 0; map_index < stats.map_count; map_index++) {
					if (strcmp(stats.maps[map_index].name, widget.scenario) == 0) {
						selected = map_index;
						found = 1;
					}
				}
				if (found) {
					mode = 0;
					launch_selected = 1;
				} else {
					snprintf(widget.message, sizeof(widget.message), "scenario %s is not on the card", widget.scenario);
					widget_dirty = 1;
				}
			}
			if (widget_dirty && ui_reader.file) {
				struct wii_rgba screen;
				if (wii_widget_compose(map_read, &ui_reader, ui_reader.size,
					stats.maps[ui_index].tag_data_offset, stats.maps[ui_index].tag_data_size,
					&widget, widget_canvas, WII_GX_TEXTURE_MAX, WII_GX_TEXTURE_MAX,
					widget_scratch, WII_GX_TEXTURE_BYTES) == 0
					&& gx_draw_set_rgba(widget_canvas, WII_GX_TEXTURE_MAX, WII_GX_TEXTURE_MAX) == 0) {
					screen.pixels = widget_canvas;
					(void)screen;
				}
				widget_dirty = 0;
			}
			if (ui_reader.file)
				gx_draw_bitmap(rmode, console_xfb);
			printf("\x1b[2;0H%s\n", widget.message);
			printf("DeLa tags. Widget VM not linked. B picker.\n");
		} else if (!showing) {
			if (launch_selected)
				down |= WPAD_BUTTON_A;
			if (stats.map_count > 0) {
				if (selected < 0)
					selected = 0;
				if (selected >= stats.map_count)
					selected = stats.map_count - 1;
				if (down & WPAD_BUTTON_UP) {
					if (selected > 0)
						selected--;
					menu_dirty = 1;
				}
				if (down & WPAD_BUTTON_DOWN) {
					if (selected + 1 < stats.map_count)
						selected++;
					menu_dirty = 1;
				}
				if ((down & WPAD_BUTTON_A) && store.positions) {
					struct wii_map_entry const *map = &stats.maps[selected];
					struct map_reader reader;
					struct wii_rgba image;
					static uint8_t *rgba;

					reader.file = fopen(map->path, "rb");
					reader.base = map->base;
					reader.size = map->size;
					gx_draw_clear_texture();
					view.texture_mode = 0;
					if (!reader.file) {
						snprintf(note, sizeof(note), "could not reopen map");
					} else {
						int mesh_ok = bsp_preview_load(map_read, &reader, map->size, map->tag_data_offset,
							map->tag_data_size, &store, &preview) == 0;

						if (!rgba)
							rgba = wii_mem2_alloc(WII_GX_TEXTURE_BYTES);
						memset(&image, 0, sizeof(image));
						image.pixels = rgba;
						image.cap = WII_GX_TEXTURE_MAX;
						if (rgba && preview.bitmap_group_found && preview.lightmap_bitmap_index != 0xffffu
							&& wii_tex_load_group(map_read, &reader, map->size, map->tag_data_offset,
								map->tag_data_size, preview.bitmap_group_file, preview.lightmap_bitmap_index, &image) == 0
							&& gx_draw_set_rgba(image.pixels, image.width, image.height) == 0)
							view.texture_mode = 1;
						else if (rgba && wii_tex_load_first_bitm(map_read, &reader, map->size, map->tag_data_offset,
							map->tag_data_size, &image) == 0
							&& gx_draw_set_rgba(image.pixels, image.width, image.height) == 0)
							view.texture_mode = mesh_ok ? 1 : 2;
						if (mesh_ok || view.texture_mode == 2) {
							showing = 1;
							yaw = 0.8f;
							pitch = 0.4f;
							if (mesh_ok) {
								wii_sim_load(map_read, &reader, map->size, map->tag_data_offset,
									map->tag_data_size, preview.scenario_file, &sim);
								if (sim.have_player) {
									yaw = sim.player_facing;
									pitch = 0.2f;
								}
							}
							if (!mesh_ok)
								snprintf(preview.message, sizeof(preview.message), "%s", image.message);
						} else {
							snprintf(note, sizeof(note), "%s", preview.message[0] ? preview.message : image.message);
						}
						fclose(reader.file);
					}
					menu_dirty = 1;
				}
			}
			if (menu_dirty) {
				wii_menu_draw(&stats, &sample, selected, note);
				menu_dirty = 0;
			}
		} else {
			if (down & WPAD_BUTTON_B) {
				showing = 0;
				if (ui_reader.file) {
					mode = 1;
					widget_dirty = 1;
				} else {
					mode = 0;
					menu_dirty = 1;
				}
				note[0] = 0;
			} else {
				if (held & WPAD_BUTTON_LEFT)
					yaw -= 0.04f;
				if (held & WPAD_BUTTON_RIGHT)
					yaw += 0.04f;
				if (held & WPAD_BUTTON_UP)
					pitch += 0.03f;
				if (held & WPAD_BUTTON_DOWN)
					pitch -= 0.03f;
				{
					float move_x = 0.0f;
					float move_y = 0.0f;
					int chan;
					int fire = (held & WPAD_BUTTON_A) != 0;

					for (chan = 0; chan < 4; chan++) {
						struct expansion_t exp;

						WPAD_Expansion(chan, &exp);
						if (exp.type == WPAD_EXP_NUNCHUK && exp.nunchuk.js.mag > 0.2f) {
							move_x = exp.nunchuk.js.mag * sinf(exp.nunchuk.js.ang * 0.0174533f);
							move_y = exp.nunchuk.js.mag * cosf(exp.nunchuk.js.ang * 0.0174533f);
						} else if (exp.type == WPAD_EXP_CLASSIC && exp.classic.ljs.mag > 0.2f) {
							move_x = exp.classic.ljs.mag * sinf(exp.classic.ljs.ang * 0.0174533f);
							move_y = exp.classic.ljs.mag * cosf(exp.classic.ljs.ang * 0.0174533f);
						}
					}
					PAD_ScanPads();
					if (PAD_StickY(0) > 16 || PAD_StickY(0) < -16)
						move_y = PAD_StickY(0) / 128.0f;
					if (PAD_StickX(0) > 16 || PAD_StickX(0) < -16)
						move_x = PAD_StickX(0) / 128.0f;
					if (PAD_ButtonsHeld(0) & PAD_BUTTON_A)
						fire = 1;
					wii_control_set(move_y, move_x, yaw, pitch, fire);
				}
				view.positions = store.positions;
				view.colors = store.colors;
				view.uvs = store.uvs;
				view.indices = store.indices;
				view.vertex_count = preview.vertices;
				view.triangle_count = preview.triangles;
				view.eye = 0;
				if (sim.have_player && wii_unit_eye(wii_game_player_unit(), &view.center[0], &view.center[1], &view.center[2])) {
					float facing = wii_control_yaw();

					view.eye = 1;
					if (!wii_unit_facing(wii_game_player_unit(), &facing))
						facing = wii_control_yaw();
					view.yaw = facing;
					view.pitch = wii_control_pitch();
				} else if (sim.have_player) {
					view.center[0] = sim.player_x;
					view.center[1] = sim.player_y;
					view.center[2] = sim.player_z + (sim.have_camera_height ? sim.camera_height : 0.0f);
					view.eye = 1;
					view.yaw = wii_control_yaw();
					view.pitch = wii_control_pitch();
				} else {
					view.center[0] = preview.center[0];
					view.center[1] = preview.center[1];
					view.center[2] = preview.center[2];
				}
				view.radius = preview.radius;
				if (!view.eye) {
					view.yaw = yaw;
					view.pitch = pitch;
				}
				if (view.texture_mode == 2 && !preview.triangles)
					gx_draw_bitmap(rmode, console_xfb);
				else
					gx_draw_mesh(rmode, console_xfb, &view);
				printf("\x1b[2;0H%s  %u tris\n", stats.maps[selected].name, (unsigned)preview.triangles);
				wii_sim_frame(&sim);
				{
					uint16_t effect_w = 0;
					uint16_t effect_h = 0;
					uint8_t const *effect_pixels = wii_effect_pixels(&effect_w, &effect_h);

					if (effect_pixels)
						gx_draw_set_effect_rgba(effect_pixels, effect_w, effect_h);
				}
				printf("%s\n%s\n", preview.message, sim.message);
				if (wii_hud_chapter()[0])
					printf("%s\n", wii_hud_chapter());
				if (wii_hud_help()[0])
					printf("%s\n", wii_hud_help());
				if (wii_hud_objective()[0])
					printf("%s\n", wii_hud_objective());
				printf("B back  stick walk  A fire  %s%s\n",
					view.texture_mode ? "cache bitmap" : "no bitmap decoded",
					wii_effect_bitmap_ok() ? "  fx" : "");
			}
		}
		VIDEO_WaitVSync();
	}
	return 0;
}
