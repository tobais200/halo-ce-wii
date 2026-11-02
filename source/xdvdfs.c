#include "xdvdfs.h"

#include <string.h>

static uint16_t read_u16(uint8_t const *bytes)
{
	return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

static uint32_t read_u32(uint8_t const *bytes)
{
	return (uint32_t)bytes[0]
		| ((uint32_t)bytes[1] << 8)
		| ((uint32_t)bytes[2] << 16)
		| ((uint32_t)bytes[3] << 24);
}

int xdvdfs_probe(xdvdfs_read_fn read, void *context, uint64_t image_size, struct xdvdfs_volume *volume)
{
	uint8_t header[0x24];

	if (!read || !volume)
		return -1;
	if (image_size < (uint64_t)XDVDFS_VOLUME_OFFSET + sizeof(header))
		return -1;
	if (read(context, XDVDFS_VOLUME_OFFSET, header, (uint32_t)sizeof(header)) != 0)
		return -1;
	if (memcmp(header, XDVDFS_MAGIC, XDVDFS_MAGIC_BYTES) != 0)
		return -1;

	volume->root_sector = read_u32(header + 0x14);
	volume->root_size = read_u32(header + 0x18);
	if (!volume->root_size)
		return -1;
	if ((uint64_t)volume->root_sector * XDVDFS_SECTOR_BYTES + volume->root_size > image_size)
		return -1;
	return 0;
}

static int walk_at(
	uint8_t const *directory,
	uint32_t directory_size,
	uint32_t offset,
	xdvdfs_visit_fn visit,
	void *context,
	int depth)
{
	uint16_t left;
	uint16_t right;
	uint32_t sector;
	uint32_t size;
	uint8_t attribute;
	uint8_t name_length;
	char name[256];
	int stopped;

	if (depth > 32)
		return -1;
	if ((uint64_t)offset + 14u > directory_size)
		return -1;

	left = read_u16(directory + offset);
	right = read_u16(directory + offset + 2);
	sector = read_u32(directory + offset + 4);
	size = read_u32(directory + offset + 8);
	attribute = directory[offset + 12];
	name_length = directory[offset + 13];
	if ((uint64_t)offset + 14u + name_length > directory_size)
		return -1;

	if (left && left != 0xFFFFu) {
		uint32_t child = (uint32_t)left * 4u;
		if (child == offset)
			return -1;
		stopped = walk_at(directory, directory_size, child, visit, context, depth + 1);
		if (stopped != 0)
			return stopped;
	}

	memcpy(name, directory + offset + 14, name_length);
	name[name_length] = 0;
	stopped = visit(context, name, (attribute & 0x10) ? 1 : 0, sector, size);
	if (stopped != 0)
		return stopped;

	if (right && right != 0xFFFFu) {
		uint32_t child = (uint32_t)right * 4u;
		if (child == offset)
			return -1;
		stopped = walk_at(directory, directory_size, child, visit, context, depth + 1);
		if (stopped != 0)
			return stopped;
	}
	return 0;
}

int xdvdfs_walk(uint8_t const *directory, uint32_t directory_size, xdvdfs_visit_fn visit, void *context)
{
	if (!directory || !visit || !directory_size)
		return -1;
	return walk_at(directory, directory_size, 0, visit, context, 0);
}
