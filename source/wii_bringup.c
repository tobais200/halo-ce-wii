#include "wii_bringup.h"

#include "wii_mem.h"
#include "wii_stream.h"

#include <stdio.h>

static unsigned kib(uint32_t bytes)
{
	return (unsigned)(bytes / 1024u);
}

void wii_bringup_print(void)
{
	struct wii_ram_sample sample;
	struct wii_stream_stats stats;
	int index;

	if (wii_mem2_init() != 0)
		printf("MEM2 heap: init failed\n");

	wii_stream_bringup(&stats);
	wii_mem_sample(&sample);

	printf("RAM ceiling %u+%u MiB (not free space)\n",
		(unsigned)(WII_MEM1_HARDWARE_BYTES / (1024u * 1024u)),
		(unsigned)(WII_MEM2_HARDWARE_BYTES / (1024u * 1024u)));
	printf("MEM1 arena %u KiB  heap used %u KiB free %u KiB\n",
		kib(sample.arena1_size), kib(sample.mem1_heap_used), kib(sample.mem1_heap_free));
	printf("MEM2 arena %u KiB  heap %u KiB used %u KiB\n",
		kib(sample.arena2_size), kib(sample.mem2_heap_size), kib(sample.mem2_used));
	printf("caps tag %u KiB tex %u KiB snd %u KiB\n",
		kib(WII_TAG_RESIDENT_CAP_BYTES),
		kib(WII_TEXTURE_CAP_BYTES),
		kib(WII_SOUND_CAP_BYTES));
	printf("window %u KiB  lz4 %s  MEM2 blocks %s\n",
		kib(WII_STREAM_WINDOW_BYTES),
		stats.lz4_roundtrip_ok ? "ok" : "failed",
		stats.mem2_window_ok ? "ok" : "failed");
	printf("SD %s  USB %s  seek rejects %d\n",
		stats.sd_mounted ? "mounted" : "absent",
		stats.usb_mounted ? "mounted" : "absent",
		stats.seek_rejected);
	printf("media %u B  lz4 hit %u B  lz4 wrote %u B\n",
		(unsigned)stats.bytes_from_media,
		(unsigned)stats.bytes_from_lz4,
		(unsigned)stats.bytes_to_lz4);
	if (stats.catalog_loaded)
		printf("catalog cache %d B decompressed\n", stats.catalog_loaded);
	printf("OpenCE cache_file_identify: %d file(s)\n", stats.maps_seen);
	for (index = 0; index < stats.line_count; index++)
		printf("%s\n", stats.lines[index]);
	printf("Tag bytes are not copied into RAM.\n");
	printf("Campaign is not linked.\n");
}
