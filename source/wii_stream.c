/*
 * Mounts SD and USB, walks a few fixed folders and XDVDFS images, and runs
 * OpenCE's cache_file_identify on each map header.
 *
 * The only map bytes resident at once are the caller's header (2 KiB, on
 * the stack inside OpenCE) plus optional LZ4 scratch in MEM2. A directory
 * larger than the stream window is skipped. A byte offset past 2 GiB is
 * rejected: newlib's fseek on this toolchain is 32-bit.
 */
#include "wii_stream.h"

#include "wii_mem.h"
#include "wii_retail.h"
#include "xdvdfs.h"

#include "cache_file_formats.h"
#include "lz4.h"

#include <ctype.h>
#include <dirent.h>
#include <fat.h>
#include <ogc/usbstorage.h>
#include <sdcard/wiisd_io.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define WII_MAX_HITS 16
#define WII_CACHE_HEADER_MAGIC 0x54414357u /* 'WCAT' little-endian bytes W C A T */

struct dir_hit {
	char name[32];
	int is_directory;
	uint32_t sector;
	uint32_t size;
};

struct dir_hits {
	struct dir_hit hit[WII_MAX_HITS];
	int count;
};

struct media {
	FILE *file;
	struct wii_stream_stats *stats;
	uint8_t *lz4_buffer;
	uint32_t lz4_capacity;
	char cache_key[48];
	int use_cache;
};

struct map_job {
	FILE *file;
	uint64_t base;
	uint32_t size;
	char cache_key[48];
	char where[64];
	char path[192];
};

static int suffix_ci(char const *name, char const *suffix)
{
	size_t name_len;
	size_t suffix_len;
	size_t index;

	if (!name || !suffix)
		return 0;
	name_len = strlen(name);
	suffix_len = strlen(suffix);
	if (name_len < suffix_len)
		return 0;
	for (index = 0; index < suffix_len; index++) {
		if (tolower((unsigned char)name[name_len - suffix_len + index])
			!= tolower((unsigned char)suffix[index]))
			return 0;
	}
	return 1;
}

static int name_is_ci(char const *name, char const *expected)
{
	size_t index;

	if (!name || !expected)
		return 0;
	for (index = 0; name[index] && expected[index]; index++) {
		if (tolower((unsigned char)name[index]) != tolower((unsigned char)expected[index]))
			return 0;
	}
	return name[index] == 0 && expected[index] == 0;
}

static void sanitize_key(char *destination, size_t destination_size, char const *name)
{
	size_t in;
	size_t out = 0;

	if (!destination_size)
		return;
	for (in = 0; name && name[in] && out + 1 < destination_size; in++) {
		unsigned char c = (unsigned char)name[in];
		if (c >= 'A' && c <= 'Z')
			c = (unsigned char)(c - 'A' + 'a');
		if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_'))
			c = '_';
		destination[out++] = (char)c;
	}
	destination[out] = 0;
	if (!out)
		strncpy(destination, "map", destination_size);
}

static int path_join(char *destination, size_t destination_size, char const *directory, char const *name)
{
	size_t directory_len;
	size_t name_len;

	if (!destination || !destination_size || !directory || !name)
		return -1;
	directory_len = strlen(directory);
	name_len = strlen(name);
	if (directory_len + 1u + name_len + 1u > destination_size)
		return -1;
	memcpy(destination, directory, directory_len);
	destination[directory_len] = '/';
	memcpy(destination + directory_len + 1, name, name_len);
	destination[directory_len + 1 + name_len] = 0;
	return 0;
}

static void copy_cap(char *destination, size_t destination_size, char const *source)
{
	size_t index;

	if (!destination_size)
		return;
	for (index = 0; index + 1 < destination_size && source && source[index]; index++)
		destination[index] = source[index];
	destination[index] = 0;
}

static int add_line(struct wii_stream_stats *stats, char const *text)
{
	if (!stats || stats->line_count >= WII_STREAM_LINES)
		return -1;
	snprintf(stats->lines[stats->line_count], WII_STREAM_LINE_BYTES, "%s", text);
	stats->line_count++;
	return 0;
}

static int media_read(struct media *media, uint64_t at, void *destination, uint32_t size, uint32_t cache_offset)
{
	char path[160];
	FILE *cache;
	uint32_t magic;
	uint32_t raw_size;
	uint32_t comp_size;
	int decoded;
	long position;

	if (!media || !media->file || !destination || !size)
		return -1;
	if (size > WII_STREAM_WINDOW_BYTES)
		return -1;
	if (at > WII_MAX_FILE_OFFSET || (uint64_t)size > WII_MAX_FILE_OFFSET
		|| at + size > (uint64_t)WII_MAX_FILE_OFFSET + 1u) {
		media->stats->seek_rejected++;
		return -1;
	}

	if (media->use_cache && media->lz4_buffer && cache_offset != 0xFFFFFFFFu) {
		snprintf(path, sizeof(path), "sd:/halo-ce/cache/chunks/%s/%08x.lz4",
			media->cache_key, cache_offset);
		cache = fopen(path, "rb");
		if (cache) {
			if (fread(&magic, 4, 1, cache) == 1
				&& fread(&raw_size, 4, 1, cache) == 1
				&& fread(&comp_size, 4, 1, cache) == 1
				&& magic == WII_CACHE_HEADER_MAGIC
				&& raw_size == size
				&& comp_size > 0
				&& comp_size <= media->lz4_capacity
				&& fread(media->lz4_buffer, 1, comp_size, cache) == comp_size) {
				decoded = LZ4_decompress_safe(
					(char const *)media->lz4_buffer,
					destination,
					(int)comp_size,
					(int)size);
				fclose(cache);
				if (decoded == (int)size) {
					media->stats->bytes_from_lz4 += size;
					return 0;
				}
			} else {
				fclose(cache);
			}
		}
	}

	position = (long)at;
	if (fseek(media->file, position, SEEK_SET) != 0)
		return -1;
	if (fread(destination, 1, size, media->file) != size)
		return -1;
	media->stats->bytes_from_media += size;

	if (media->use_cache && media->lz4_buffer && size <= 0x800u && cache_offset != 0xFFFFFFFFu) {
		int bound = LZ4_compressBound((int)size);
		int stored;
		if (bound > 0 && (uint32_t)bound <= media->lz4_capacity) {
			stored = LZ4_compress_default(
				(char const *)destination,
				(char *)media->lz4_buffer,
				(int)size,
				bound);
			if (stored > 0) {
				mkdir("sd:/halo-ce", 0777);
				mkdir("sd:/halo-ce/cache", 0777);
				mkdir("sd:/halo-ce/cache/chunks", 0777);
				snprintf(path, sizeof(path), "sd:/halo-ce/cache/chunks/%s", media->cache_key);
				mkdir(path, 0777);
				snprintf(path, sizeof(path), "sd:/halo-ce/cache/chunks/%s/%08x.lz4",
					media->cache_key, cache_offset);
				cache = fopen(path, "wb");
				if (cache) {
					magic = WII_CACHE_HEADER_MAGIC;
					raw_size = size;
					comp_size = (uint32_t)stored;
					if (fwrite(&magic, 4, 1, cache) == 1
						&& fwrite(&raw_size, 4, 1, cache) == 1
						&& fwrite(&comp_size, 4, 1, cache) == 1
						&& fwrite(media->lz4_buffer, 1, (size_t)stored, cache) == (size_t)stored) {
						media->stats->bytes_to_lz4 += (uint32_t)stored;
					}
					fclose(cache);
				}
			}
		}
	}
	return 0;
}

static int file_size_32(FILE *file, uint32_t *size_out)
{
	long size;

	if (fseek(file, 0, SEEK_END) != 0)
		return -1;
	size = ftell(file);
	if (size < 0)
		return -1;
	if (fseek(file, 0, SEEK_SET) != 0)
		return -1;
	*size_out = (uint32_t)size;
	return 0;
}

static int image_read(void *context, uint64_t offset, void *destination, uint32_t size)
{
	struct media *media = context;
	return media_read(media, offset, destination, size, 0xFFFFFFFFu);
}

struct identify_ctx {
	struct map_job *job;
	struct wii_stream_stats *stats;
	uint8_t *lz4_buffer;
	uint32_t lz4_capacity;
};

static int identify_read(void *context, uint32_t offset, uint32_t size, void *buffer)
{
	struct identify_ctx *ctx = context;
	struct media media;

	if (!ctx || !ctx->job || (uint64_t)offset + size > ctx->job->size || size > WII_STREAM_WINDOW_BYTES)
		return 0;
	memset(&media, 0, sizeof(media));
	media.file = ctx->job->file;
	media.stats = ctx->stats;
	media.lz4_buffer = ctx->lz4_buffer;
	media.lz4_capacity = ctx->lz4_capacity;
	memcpy(media.cache_key, ctx->job->cache_key, sizeof(media.cache_key));
	media.use_cache = ctx->stats->sd_mounted;
	if (media_read(&media, ctx->job->base + offset, buffer, size, offset) != 0)
		return 0;
	return 1;
}

static void identify_job(
	struct map_job *job,
	struct wii_stream_stats *stats,
	uint8_t *lz4_buffer,
	uint32_t lz4_capacity)
{
	struct identify_ctx ctx;
	struct cache_file_source source;
	struct cache_file_identity identity;
	enum cache_file_status status;
	char line[WII_STREAM_LINE_BYTES];
	char const *format;
	char const *fit;

	ctx.job = job;
	ctx.stats = stats;
	ctx.lz4_buffer = lz4_buffer;
	ctx.lz4_capacity = lz4_capacity;
	source.context = &ctx;
	source.read = identify_read;
	source.size = job->size;

	stats->maps_seen++;
	status = cache_file_identify(&source, &identity);
	format = cache_file_format_describe(identity.format);
	if (status == _cache_file_status_ok)
		stats->maps_identified++;
	fit = wii_mem_tag_would_fit(identity.tag_data_size) ? "cap ok, not copied" : "over cap, not copied";
	snprintf(line, sizeof(line), "%.18s %.16s %lu KiB tags %lu KiB %s",
		identity.name[0] ? identity.name : job->where,
		format ? format : "?",
		(unsigned long)(identity.file_length / 1024u),
		(unsigned long)(identity.tag_data_size / 1024u),
		status == _cache_file_status_ok ? fit : cache_file_status_describe(status));
	add_line(stats, line);

	if (status == _cache_file_status_ok
		&& (identity.format == _cache_file_format_xbox_cache
			|| identity.format == _cache_file_format_custom_edition_cache)
		&& stats->map_count < WII_MAP_CAPACITY) {
		struct wii_map_entry *entry = &stats->maps[stats->map_count++];
		static char const *campaign_names[] = {
			"a10", "a30", "a50", "b30", "b40", "c10", "c20", "c40", "d20", "d40", 0
		};
		int index;

		memset(entry, 0, sizeof(*entry));
		copy_cap(entry->name, sizeof(entry->name), identity.name[0] ? identity.name : job->where);
		copy_cap(entry->path, sizeof(entry->path), job->path[0] ? job->path : job->where);
		entry->base = job->base;
		entry->size = job->size;
		entry->tag_data_offset = identity.tag_data_offset;
		entry->tag_data_size = identity.tag_data_size;
		entry->main_menu = name_is_ci(entry->name, "ui");
		for (index = 0; campaign_names[index]; index++) {
			if (name_is_ci(entry->name, campaign_names[index]))
				entry->campaign = 1;
		}
	}
}

static int collect_visit(void *context, char const *name, int is_directory, uint32_t sector, uint32_t size)
{
	struct dir_hits *hits = context;
	struct dir_hit *hit;

	if (hits->count >= WII_MAX_HITS)
		return 1;
	hit = &hits->hit[hits->count++];
	snprintf(hit->name, sizeof(hit->name), "%s", name);
	hit->is_directory = is_directory;
	hit->sector = sector;
	hit->size = size;
	return 0;
}

static int read_span(struct media *media, uint64_t at, void *destination, uint32_t size)
{
	uint32_t done = 0;

	while (done < size) {
		uint32_t chunk = size - done;
		if (chunk > WII_STREAM_WINDOW_BYTES)
			chunk = WII_STREAM_WINDOW_BYTES;
		if (media_read(media, at + done, (uint8_t *)destination + done, chunk, 0xFFFFFFFFu) != 0)
			return -1;
		done += chunk;
	}
	return 0;
}

static void consider_loose(char const *path, struct wii_stream_stats *stats, uint8_t *lz4_buffer, uint32_t lz4_capacity)
{
	FILE *file;
	struct map_job job;
	uint32_t size;

	if (stats->line_count >= WII_STREAM_LINES)
		return;
	file = fopen(path, "rb");
	if (!file)
		return;
	if (file_size_32(file, &size) != 0 || size < 64u) {
		if (file_size_32(file, &size) != 0)
			stats->seek_rejected++;
		fclose(file);
		return;
	}
	memset(&job, 0, sizeof(job));
	job.file = file;
	job.base = 0;
	job.size = size;
	sanitize_key(job.cache_key, sizeof(job.cache_key), path);
	copy_cap(job.where, sizeof(job.where), path);
	copy_cap(job.path, sizeof(job.path), path);
	identify_job(&job, stats, lz4_buffer, lz4_capacity);
	fclose(file);
}

static void scan_loose_dir(char const *directory, struct wii_stream_stats *stats, uint8_t *lz4_buffer, uint32_t lz4_capacity)
{
	DIR *dir;
	struct dirent *entry;
	int seen = 0;

	dir = opendir(directory);
	if (!dir)
		return;
	while ((entry = readdir(dir)) != NULL && seen < 32 && stats->line_count < WII_STREAM_LINES) {
		char path[192];
		if (!suffix_ci(entry->d_name, ".map"))
			continue;
		if (path_join(path, sizeof(path), directory, entry->d_name) != 0)
			continue;
		consider_loose(path, stats, lz4_buffer, lz4_capacity);
		seen++;
	}
	closedir(dir);
}

static void consider_iso(char const *path, struct wii_stream_stats *stats, uint8_t *window, uint8_t *lz4_buffer, uint32_t lz4_capacity)
{
	FILE *file;
	struct media media;
	struct xdvdfs_volume volume;
	struct dir_hits root_hits;
	struct dir_hits map_hits;
	uint32_t image_size;
	uint32_t index;
	uint64_t maps_sector = 0;
	uint32_t maps_size = 0;
	int have_maps = 0;

	if (stats->line_count >= WII_STREAM_LINES)
		return;
	file = fopen(path, "rb");
	if (!file)
		return;
	if (file_size_32(file, &image_size) != 0) {
		stats->seek_rejected++;
		add_line(stats, "iso past 2 GiB seek limit");
		fclose(file);
		return;
	}
	memset(&media, 0, sizeof(media));
	media.file = file;
	media.stats = stats;
	media.lz4_buffer = lz4_buffer;
	media.lz4_capacity = lz4_capacity;
	media.use_cache = 0;
	if (xdvdfs_probe(image_read, &media, image_size, &volume) != 0) {
		fclose(file);
		return;
	}
	if (volume.root_size > WII_STREAM_WINDOW_BYTES) {
		stats->directory_skipped++;
		add_line(stats, "iso root directory exceeds window");
		fclose(file);
		return;
	}
	if (read_span(&media, (uint64_t)volume.root_sector * XDVDFS_SECTOR_BYTES, window, volume.root_size) != 0) {
		fclose(file);
		return;
	}
	memset(&root_hits, 0, sizeof(root_hits));
	if (xdvdfs_walk(window, volume.root_size, collect_visit, &root_hits) != 0 && root_hits.count == 0) {
		fclose(file);
		return;
	}
	memset(&map_hits, 0, sizeof(map_hits));
	for (index = 0; index < (uint32_t)root_hits.count; index++) {
		struct dir_hit *hit = &root_hits.hit[index];
		if (hit->is_directory && name_is_ci(hit->name, "maps")) {
			maps_sector = hit->sector;
			maps_size = hit->size;
			have_maps = 1;
		} else if (!hit->is_directory && suffix_ci(hit->name, ".map") && map_hits.count < WII_MAX_HITS) {
			map_hits.hit[map_hits.count++] = *hit;
		}
	}
	if (have_maps) {
		if (maps_size > WII_STREAM_WINDOW_BYTES) {
			stats->directory_skipped++;
			add_line(stats, "iso maps directory exceeds window");
		} else if (read_span(&media, maps_sector * XDVDFS_SECTOR_BYTES, window, maps_size) == 0) {
			struct dir_hits nested;
			memset(&nested, 0, sizeof(nested));
			xdvdfs_walk(window, maps_size, collect_visit, &nested);
			for (index = 0; index < (uint32_t)nested.count && map_hits.count < WII_MAX_HITS; index++) {
				if (!nested.hit[index].is_directory && suffix_ci(nested.hit[index].name, ".map"))
					map_hits.hit[map_hits.count++] = nested.hit[index];
			}
		}
	}
	for (index = 0; index < (uint32_t)map_hits.count && stats->line_count < WII_STREAM_LINES; index++) {
		struct map_job job;
		uint64_t base = (uint64_t)map_hits.hit[index].sector * XDVDFS_SECTOR_BYTES;
		if (base > WII_MAX_FILE_OFFSET || base + map_hits.hit[index].size > (uint64_t)WII_MAX_FILE_OFFSET + 1u) {
			stats->seek_rejected++;
			add_line(stats, "map lives past 2 GiB seek limit");
			continue;
		}
		memset(&job, 0, sizeof(job));
		job.file = file;
		job.base = base;
		job.size = map_hits.hit[index].size;
		sanitize_key(job.cache_key, sizeof(job.cache_key), map_hits.hit[index].name);
		copy_cap(job.where, sizeof(job.where), map_hits.hit[index].name);
		copy_cap(job.path, sizeof(job.path), path);
		identify_job(&job, stats, lz4_buffer, lz4_capacity);
	}
	fclose(file);
}

static void scan_iso_dir(char const *directory, struct wii_stream_stats *stats, uint8_t *window, uint8_t *lz4_buffer, uint32_t lz4_capacity)
{
	DIR *dir;
	struct dirent *entry;
	int seen = 0;

	dir = opendir(directory);
	if (!dir)
		return;
	while ((entry = readdir(dir)) != NULL && seen < 8 && stats->line_count < WII_STREAM_LINES) {
		char path[192];
		if (!suffix_ci(entry->d_name, ".iso"))
			continue;
		if (path_join(path, sizeof(path), directory, entry->d_name) != 0)
			continue;
		consider_iso(path, stats, window, lz4_buffer, lz4_capacity);
		seen++;
	}
	closedir(dir);
}

static int directory_open(char const *path)
{
	DIR *dir = opendir(path);
	if (!dir)
		return 0;
	closedir(dir);
	return 1;
}

static void lz4_self_test(struct wii_stream_stats *stats)
{
	static char const text[] = "halo-ce-wii stream window";
	char compressed[64];
	char restored[64];
	int stored;
	int decoded;

	stored = LZ4_compress_default(text, compressed, (int)strlen(text), (int)sizeof(compressed));
	if (stored <= 0)
		return;
	decoded = LZ4_decompress_safe(compressed, restored, stored, (int)sizeof(restored));
	if (decoded == (int)strlen(text) && memcmp(restored, text, (size_t)decoded) == 0)
		stats->lz4_roundtrip_ok = 1;
}

static void load_catalog(struct wii_stream_stats *stats, uint8_t *catalog, uint8_t *lz4_buffer, uint32_t lz4_capacity)
{
	FILE *file;
	uint32_t magic = 0;
	uint32_t raw_size = 0;
	uint32_t comp_size = 0;
	int decoded;

	file = fopen("sd:/halo-ce/cache/catalog.lz4", "rb");
	if (!file)
		return;
	if (fread(&magic, 4, 1, file) != 1
		|| fread(&raw_size, 4, 1, file) != 1
		|| fread(&comp_size, 4, 1, file) != 1
		|| magic != WII_CACHE_HEADER_MAGIC
		|| raw_size == 0
		|| raw_size > WII_CATALOG_BYTES
		|| comp_size == 0
		|| comp_size > lz4_capacity
		|| fread(lz4_buffer, 1, comp_size, file) != comp_size) {
		fclose(file);
		return;
	}
	fclose(file);
	decoded = LZ4_decompress_safe((char const *)lz4_buffer, (char *)catalog, (int)comp_size, (int)raw_size);
	if (decoded == (int)raw_size)
		stats->catalog_loaded = (int)raw_size;
}

static void store_catalog(struct wii_stream_stats *stats, uint8_t *catalog, uint8_t *lz4_buffer, uint32_t lz4_capacity)
{
	FILE *file;
	uint32_t raw_size = 0;
	int stored;
	uint32_t magic;
	uint32_t comp_size;
	int index;

	if (!stats->sd_mounted)
		return;
	for (index = 0; index < stats->line_count && raw_size + 1 < WII_CATALOG_BYTES; index++) {
		size_t len = strlen(stats->lines[index]);
		if (raw_size + len + 1 >= WII_CATALOG_BYTES)
			break;
		memcpy(catalog + raw_size, stats->lines[index], len);
		raw_size += (uint32_t)len;
		catalog[raw_size++] = '\n';
	}
	if (!raw_size)
		return;
	stored = LZ4_compress_default((char const *)catalog, (char *)lz4_buffer, (int)raw_size, (int)lz4_capacity);
	if (stored <= 0)
		return;
	mkdir("sd:/halo-ce", 0777);
	mkdir("sd:/halo-ce/cache", 0777);
	file = fopen("sd:/halo-ce/cache/catalog.lz4", "wb");
	if (!file)
		return;
	magic = WII_CACHE_HEADER_MAGIC;
	comp_size = (uint32_t)stored;
	if (fwrite(&magic, 4, 1, file) == 1
		&& fwrite(&raw_size, 4, 1, file) == 1
		&& fwrite(&comp_size, 4, 1, file) == 1
		&& fwrite(lz4_buffer, 1, (size_t)stored, file) == (size_t)stored)
		stats->catalog_stored = (int)raw_size;
	fclose(file);
}

void wii_stream_bringup(struct wii_stream_stats *stats)
{
	uint8_t *window;
	uint8_t *lz4_buffer;
	uint8_t *catalog;
	int index;

	if (!stats)
		return;
	memset(stats, 0, sizeof(*stats));
	lz4_self_test(stats);
	wii_retail_record(stats);

	fatInitDefault();
	if (!directory_open("sd:/"))
		fatMountSimple("sd", &__io_wiisd);
	stats->sd_mounted = directory_open("sd:/");
	fatMountSimple("usb", &__io_usbstorage);
	stats->usb_mounted = directory_open("usb:/");

	window = wii_mem2_alloc(WII_STREAM_WINDOW_BYTES);
	lz4_buffer = wii_mem2_alloc(WII_LZ4_CAPACITY);
	catalog = wii_mem2_alloc(WII_CATALOG_BYTES);
	stats->mem2_window_ok = window && lz4_buffer && catalog;
	if (!stats->mem2_window_ok) {
		add_line(stats, "MEM2 window allocation failed");
		wii_retail_record(stats);
		return;
	}

	if (stats->sd_mounted)
		load_catalog(stats, catalog, lz4_buffer, WII_LZ4_CAPACITY);

	for (index = 0; index < wii_retail_loose_count(); index++)
		scan_loose_dir(wii_retail_loose_dir(index), stats, lz4_buffer, WII_LZ4_CAPACITY);
	/* A loose .map is the layout we can seek. Skip game.iso when one identified
	   so a disc image past 2 GiB is never fseek'd. */
	if (stats->maps_identified == 0) {
		for (index = 0; index < wii_retail_iso_count(); index++)
			consider_iso(wii_retail_iso_file(index), stats, window, lz4_buffer, WII_LZ4_CAPACITY);
		if (stats->sd_mounted)
			scan_iso_dir("sd:/halo-ce", stats, window, lz4_buffer, WII_LZ4_CAPACITY);
		if (stats->usb_mounted)
			scan_iso_dir("usb:/halo-ce", stats, window, lz4_buffer, WII_LZ4_CAPACITY);
		if (stats->sd_mounted && stats->maps_identified == 0)
			scan_iso_dir("sd:/apps/halo-ce", stats, window, lz4_buffer, WII_LZ4_CAPACITY);
	}

	wii_retail_record(stats);
	if (stats->retail_message[0])
		add_line(stats, stats->retail_message);
	store_catalog(stats, catalog, lz4_buffer, WII_LZ4_CAPACITY);
}
