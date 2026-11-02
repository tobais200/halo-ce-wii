/*
 * Wii memory budget for the engine bring-up.
 *
 * Hardware ceiling, not a free heap: MEM1 is 24 MiB and MEM2 is 64 MiB.
 * The DOL, GX fifo, and framebuffers sit in MEM1. Large blocks go to MEM2.
 * OpenCE's physical_memory_allocate() is not called. It asks for fixed Xbox
 * addresses inside MEM1 and for desktop caches (a 256 MiB texture cache)
 * that do not fit.
 */
#ifndef WII_MEM_H
#define WII_MEM_H

#include <stdint.h>

#define WII_MEM1_HARDWARE_BYTES (24u * 1024u * 1024u)
#define WII_MEM2_HARDWARE_BYTES (64u * 1024u * 1024u)
#define WII_RAM_HARDWARE_BYTES (WII_MEM1_HARDWARE_BYTES + WII_MEM2_HARDWARE_BYTES)

/* One resident slice of a map. The rest stays on SD/USB. */
#define WII_STREAM_WINDOW_BYTES (256u * 1024u)
/* LZ4_compressBound(window) without including lz4.h here. */
#define WII_LZ4_CAPACITY (WII_STREAM_WINDOW_BYTES + (WII_STREAM_WINDOW_BYTES) / 255u + 16u)
#define WII_CATALOG_BYTES (32u * 1024u)

/* Ceilings for a later full renderer. The BSP preview below is what this
   build actually allocates. Xbox tag cache is 22 MiB, desktop texture cache
   is 256 MiB, desktop sound cache is 16 MiB. */
#define WII_TAG_RESIDENT_CAP_BYTES (8u * 1024u * 1024u)
#define WII_TEXTURE_CAP_BYTES (8u * 1024u * 1024u)
#define WII_SOUND_CAP_BYTES (2u * 1024u * 1024u)
#define WII_MEM2_GUARD_BYTES (1u * 1024u * 1024u)

/* First structure BSP, streamed. Not the whole map. */
#define WII_PREVIEW_VERTICES 3072u
#define WII_PREVIEW_TRIANGLES 4096u
/* positions + rgba + lightmap uv */
#define WII_PREVIEW_BYTES (WII_PREVIEW_VERTICES * 24u + WII_PREVIEW_TRIANGLES * 6u)
/* One GX texture. The GPU reads MEM1 only, so this is not part of the MEM2 heap. */
#define WII_GX_TEXTURE_MAX 128u
#define WII_GX_TEXTURE_BYTES (WII_GX_TEXTURE_MAX * WII_GX_TEXTURE_MAX * 4u)

/* newlib off_t / fseek on this devkitPPC is 32-bit. */
#define WII_MAX_FILE_OFFSET 0x7FFFFFFFu

struct wii_ram_sample {
	uint32_t arena1_lo;
	uint32_t arena1_hi;
	uint32_t arena1_size;
	uint32_t arena2_lo;
	uint32_t arena2_hi;
	uint32_t arena2_size;
	uint32_t mem1_heap_used;
	uint32_t mem1_heap_free;
	uint32_t mem2_heap_size;
	uint32_t mem2_used;
	uint32_t mem2_free;
	int mem2_ready;
};

/* 0 on success. Leaves WII_MEM2_GUARD_BYTES of MEM2 unclaimed. */
int wii_mem2_init(void);
void *wii_mem2_alloc(uint32_t size);
void wii_mem_sample(struct wii_ram_sample *sample);

/* 0 when a future tag-data copy of this size would still fit the cap
   and the measured MEM2 free space. This bring-up never performs that copy. */
int wii_mem_tag_would_fit(uint32_t tag_bytes);

#endif
