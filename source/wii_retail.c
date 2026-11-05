#include "wii_retail.h"

#include <stdio.h>
#include <string.h>

/* Loose .map directories are tried before any game.iso. The folder next
   to the Homebrew Channel app is first. */
static char const *const loose_dirs[] = {
	"sd:/apps/halo-ce/maps",
	"sd:/halo-ce/maps",
	"sd:/maps",
	"usb:/apps/halo-ce/maps",
	"usb:/halo-ce/maps",
	"usb:/maps",
};

static char const *const iso_files[] = {
	"sd:/apps/halo-ce/game.iso",
	"sd:/halo-ce/game.iso",
	"usb:/apps/halo-ce/game.iso",
	"usb:/halo-ce/game.iso",
	"usb:/game.iso",
};

int wii_retail_loose_count(void)
{
	return (int)(sizeof(loose_dirs) / sizeof(loose_dirs[0]));
}

char const *wii_retail_loose_dir(int index)
{
	if (index < 0 || index >= wii_retail_loose_count())
		return 0;
	return loose_dirs[index];
}

int wii_retail_iso_count(void)
{
	return (int)(sizeof(iso_files) / sizeof(iso_files[0]));
}

char const *wii_retail_iso_file(int index)
{
	if (index < 0 || index >= wii_retail_iso_count())
		return 0;
	return iso_files[index];
}

void wii_retail_record(struct wii_stream_stats *stats)
{
	if (!stats)
		return;
	/* Extra probes when no loose map was identified: sd:/halo-ce, usb:/halo-ce,
	   and sd:/apps/halo-ce, each for a small *.iso. */
	stats->retail_paths_probed = wii_retail_loose_count() + wii_retail_iso_count() + 3;
	if (stats->map_count > 0) {
		stats->retail_map_reported = 1;
		stats->retail_missing_reported = 0;
		snprintf(stats->retail_message, sizeof(stats->retail_message),
			"Retail map found. Loose .map is preferred.");
		return;
	}
	stats->retail_map_reported = 0;
	stats->retail_missing_reported = 1;
	snprintf(stats->retail_message, sizeof(stats->retail_message),
		"No retail map. Put *.map in sd:/apps/halo-ce/maps");
}
