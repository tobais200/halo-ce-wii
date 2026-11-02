#include "wii_menu.h"

#include <stdio.h>

static unsigned kib(uint32_t bytes)
{
	return (unsigned)(bytes / 1024u);
}

void wii_menu_draw(
	struct wii_stream_stats const *stats,
	struct wii_ram_sample const *sample,
	int selected,
	char const *note)
{
	int index;

	printf("\x1b[2J\x1b[2;0H");
	printf("Halo CE Wii - engine stub\n");
	printf("MEM1 heap %u/%u KiB   MEM2 %u/%u KiB\n",
		sample ? kib(sample->mem1_heap_used) : 0,
		sample ? kib(sample->mem1_heap_free + sample->mem1_heap_used) : 0,
		sample ? kib(sample->mem2_used) : 0,
		sample ? kib(sample->mem2_heap_size) : 0);
	printf("SD %s  USB %s\n",
		stats && stats->sd_mounted ? "yes" : "no",
		stats && stats->usb_mounted ? "yes" : "no");
	printf("Port menu. Game loop is not running.\n");
	printf("Your maps: sd:/halo-ce/maps or game.iso\n\n");
	if (!stats || !stats->map_count) {
		printf("No cache file identified yet.\n");
	} else {
		for (index = 0; index < stats->map_count; index++) {
			struct wii_map_entry const *map = &stats->maps[index];
			char const *kind = "map";

			if (map->main_menu)
				kind = "main menu";
			else if (map->campaign)
				kind = "campaign";
			printf("%c %.20s  %s\n", index == selected ? '>' : ' ', map->name, kind);
		}
	}
	if (note && note[0])
		printf("\n%s\n", note);
	printf("\nA draw BSP   D-pad select   HOME exit\n");
	printf("Textures and simulation are not linked.\n");
}
