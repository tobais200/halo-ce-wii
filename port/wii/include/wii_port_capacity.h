/*
 * Xbox parenthetical sizes from halo_port_capacity.h. The desktop defaults
 * (8 MiB object pool, 8192 objects) are what the Linux port mallocs. The
 * object pool is the one objects_initialize asks for up front, and 8 MiB
 * does not belong on MEM1. The object count stays at the desktop figure:
 * the header array is a later malloc, not this pool.
 */
#undef HALO_PORT_OBJECT_MEMORY_POOL_SIZE
#define HALO_PORT_OBJECT_MEMORY_POOL_SIZE 0x100000
