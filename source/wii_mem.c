#include "wii_mem.h"

#include <malloc.h>
#include <ogc/lwp_heap.h>
#include <ogc/system.h>

static heap_cntrl mem2_heap;
static int mem2_ready;
static uint32_t mem2_heap_size;

int wii_mem2_init(void)
{
	uint8_t *lo;
	uint8_t *hi;
	uint32_t size;
	uint32_t created;

	if (mem2_ready)
		return 0;

	lo = SYS_GetArena2Lo();
	hi = SYS_GetArena2Hi();
	if (!lo || !hi || hi <= lo)
		return -1;

	size = (uint32_t)(hi - lo);
	if (size <= WII_MEM2_GUARD_BYTES + WII_STREAM_WINDOW_BYTES)
		return -1;
	size -= WII_MEM2_GUARD_BYTES;

	created = __lwp_heap_init(&mem2_heap, lo, size, 32);
	if (!created)
		return -1;

	/* The guard above `lo + size` stays outside the heap. */
	SYS_SetArena2Lo(lo + size);
	mem2_heap_size = created;
	mem2_ready = 1;
	return 0;
}

void *wii_mem2_alloc(uint32_t size)
{
	void *block;

	if (!mem2_ready || !size)
		return 0;
	block = __lwp_heap_allocate(&mem2_heap, size);
	return block;
}

void wii_mem_sample(struct wii_ram_sample *sample)
{
	struct mallinfo info;
	heap_iblock heap_info;

	if (!sample)
		return;

	sample->arena1_lo = (uint32_t)SYS_GetArena1Lo();
	sample->arena1_hi = (uint32_t)SYS_GetArena1Hi();
	sample->arena1_size = SYS_GetArena1Size();
	sample->arena2_lo = (uint32_t)SYS_GetArena2Lo();
	sample->arena2_hi = (uint32_t)SYS_GetArena2Hi();
	sample->arena2_size = SYS_GetArena2Size();

	info = mallinfo();
	sample->mem1_heap_used = (uint32_t)info.uordblks;
	sample->mem1_heap_free = (uint32_t)info.fordblks;

	sample->mem2_ready = mem2_ready;
	sample->mem2_heap_size = mem2_heap_size;
	sample->mem2_used = 0;
	sample->mem2_free = 0;
	if (mem2_ready) {
		__lwp_heap_getinfo(&mem2_heap, &heap_info);
		sample->mem2_used = heap_info.used_size;
		sample->mem2_free = heap_info.free_size;
	}
}

typedef char wii_ram_ceiling_is_88mib[
	(WII_RAM_HARDWARE_BYTES == 88u * 1024u * 1024u) ? 1 : -1];
typedef char wii_caps_fit_in_mem2[
	(WII_TAG_RESIDENT_CAP_BYTES + WII_TEXTURE_CAP_BYTES + WII_SOUND_CAP_BYTES
		+ WII_STREAM_WINDOW_BYTES + WII_LZ4_CAPACITY + WII_CATALOG_BYTES
		+ WII_PREVIEW_BYTES + (2u * WII_GX_TEXTURE_BYTES) + WII_MEM2_GUARD_BYTES < WII_MEM2_HARDWARE_BYTES) ? 1 : -1];

int wii_mem_tag_would_fit(uint32_t tag_bytes)
{
	struct wii_ram_sample sample;

	if (tag_bytes > WII_TAG_RESIDENT_CAP_BYTES)
		return 0;
	wii_mem_sample(&sample);
	if (!sample.mem2_ready)
		return 0;
	/* Leave the stream window and the guard out of the decision. */
	if (tag_bytes > sample.mem2_free)
		return 0;
	return 1;
}
