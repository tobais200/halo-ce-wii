/*
 * Stand-in for render/render.h. The real render_globals holds the visible
 * cluster set and is hundreds of kilobytes. The script VM only reads
 * render.camera.position from the trigger-volume debug pass, which is off.
 */
#ifndef __RENDER_H
#define __RENDER_H

struct render_camera {
	real_point3d position;
};

struct render_globals {
	struct render_camera camera;
};

extern struct render_globals render;

#endif
