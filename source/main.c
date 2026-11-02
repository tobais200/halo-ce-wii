/*
 * Homebrew Channel entry. Menu lists the player's own Halo cache files.
 * A draws the first structure BSP through GX. The campaign simulation,
 * main-menu widgets, and shaders are not linked.
 */
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
	int menu_dirty = 1;
	float yaw = 0.8f;
	float pitch = 0.4f;
	char note[96];

	(void)argc;
	(void)argv;
	memset(&store, 0, sizeof(store));
	memset(&preview, 0, sizeof(preview));
	memset(&view, 0, sizeof(view));
	note[0] = 0;

	VIDEO_Init();
	WPAD_Init();
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
		store.indices = wii_mem2_alloc(WII_PREVIEW_TRIANGLES * 6u);
		store.vertex_cap = WII_PREVIEW_VERTICES;
		store.triangle_cap = WII_PREVIEW_TRIANGLES;
	}
	wii_stream_bringup(&stats);
	wii_mem_sample(&sample);
	if (!store.positions || !store.colors || !store.indices)
		snprintf(note, sizeof(note), "MEM2 preview buffers failed");

	while (SYS_MainLoop()) {
		uint32_t down;
		uint32_t held;

		WPAD_ScanPads();
		down = buttons_down();
		held = buttons_held();
		if (home_pressed(down))
			exit(0);

		if (!showing) {
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

					reader.file = fopen(map->path, "rb");
					reader.base = map->base;
					reader.size = map->size;
					if (!reader.file) {
						snprintf(note, sizeof(note), "could not reopen map");
					} else if (bsp_preview_load(map_read, &reader, map->size, map->tag_data_offset,
						map->tag_data_size, &store, &preview) == 0) {
						showing = 1;
						yaw = 0.8f;
						pitch = 0.4f;
					} else {
						snprintf(note, sizeof(note), "%s", preview.message);
					}
					if (reader.file)
						fclose(reader.file);
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
				menu_dirty = 1;
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
				view.positions = store.positions;
				view.colors = store.colors;
				view.indices = store.indices;
				view.vertex_count = preview.vertices;
				view.triangle_count = preview.triangles;
				view.center[0] = preview.center[0];
				view.center[1] = preview.center[1];
				view.center[2] = preview.center[2];
				view.radius = preview.radius;
				view.yaw = yaw;
				view.pitch = pitch;
				gx_draw_mesh(rmode, console_xfb, &view);
				printf("\x1b[2;0H%s  %u tris\n", stats.maps[selected].name, (unsigned)preview.triangles);
				printf("%s\nB back  D-pad orbit  shaders not linked\n", preview.message);
			}
		}
		VIDEO_WaitVSync();
	}
	return 0;
}
