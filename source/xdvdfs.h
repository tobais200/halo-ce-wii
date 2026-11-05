/*
 * Streaming reader for the Xbox DVD filesystem (XDVDFS).
 * A directory is walked from a buffer the caller already holds.
 * File bytes are not loaded here.
 */
#ifndef XDVDFS_H
#define XDVDFS_H

#include <stdint.h>

#define XDVDFS_SECTOR_BYTES 2048u
#define XDVDFS_VOLUME_OFFSET 0x10000u
#define XDVDFS_MAGIC "MICROSOFT*XBOX*MEDIA"
#define XDVDFS_MAGIC_BYTES 20u

/* 0 when every byte was read. */
typedef int (*xdvdfs_read_fn)(void *context, uint64_t offset, void *destination, uint32_t size);

struct xdvdfs_volume {
	uint32_t root_sector;
	uint32_t root_size;
};

/* 0 on a recognized volume. */
int xdvdfs_probe(xdvdfs_read_fn read, void *context, uint64_t image_size, struct xdvdfs_volume *volume);

/* Return 0 to keep walking. Nonzero stops the walk and is returned. */
typedef int (*xdvdfs_visit_fn)(
	void *context,
	char const *name,
	int is_directory,
	uint32_t sector,
	uint32_t size);

/* `directory` is one directory's bytes (not the whole image).
   A subtree offset of 0 or 0xFFFF means there is no child. */
int xdvdfs_walk(uint8_t const *directory, uint32_t directory_size, xdvdfs_visit_fn visit, void *context);

#endif
