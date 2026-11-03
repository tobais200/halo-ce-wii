/*
 * Stand-in for objects/objects.h. The real header pulls the object memory
 * pool, cluster partitions, and the Xbox object datum (0x1A4). hs_runtime.c
 * only reads definition_index and object.type while casting to a unit.
 */
#ifndef __OBJECTS_H
#define __OBJECTS_H

struct object_header_datum {
	short type;
};

struct object_datum {
	long definition_index;
	struct object_header_datum object;
};

struct object_datum *wii_object_get(long index);
#define object_get(index) wii_object_get(index)

long object_index_from_name_index(short name_index);

#endif
