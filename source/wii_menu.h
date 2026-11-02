#ifndef WII_MENU_H
#define WII_MENU_H

#include "wii_mem.h"
#include "wii_stream.h"

void wii_menu_draw(
	struct wii_stream_stats const *stats,
	struct wii_ram_sample const *sample,
	int selected,
	char const *note);

#endif
