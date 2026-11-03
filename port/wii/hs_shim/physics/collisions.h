/*
 * Stand-in for physics/collisions.h. The real header includes objects.h
 * and the collision BSP. Only the line-of-sight probe in
 * render_debug_trigger_volumes is referenced, and that function is not called.
 */
#ifndef __COLLISIONS_H
#define __COLLISIONS_H

enum {
	_collision_test_for_line_of_sight_flags = 0xC2AD
};

struct collision_result {
	long kind;
};

boolean collision_test_vector(
	unsigned long flags,
	real_point3d const *point,
	real_vector3d const *vector,
	long ignore_object_index,
	struct collision_result *collision);

#endif
