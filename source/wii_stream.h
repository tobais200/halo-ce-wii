/*
 * SD/USB map discovery. Reads a header, or one stream window, never a map.
 */
#ifndef WII_STREAM_H
#define WII_STREAM_H

#include <stdint.h>

#define WII_STREAM_LINES 8
#define WII_STREAM_LINE_BYTES 96

struct wii_stream_stats {
	int sd_mounted;
	int usb_mounted;
	int mem2_window_ok;
	int lz4_roundtrip_ok;
	int maps_seen;
	int maps_identified;
	int seek_rejected;
	int directory_skipped;
	uint32_t bytes_from_media;
	uint32_t bytes_from_lz4;
	uint32_t bytes_to_lz4;
	int catalog_loaded;
	int catalog_stored;
	char lines[WII_STREAM_LINES][WII_STREAM_LINE_BYTES];
	int line_count;
};

void wii_stream_bringup(struct wii_stream_stats *stats);

#endif
