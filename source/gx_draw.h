/*
 * Untextured GX triangles. This is the DrawPrimitiveUP slice of a D3D8
 * device: position and vertex color, TEV pass-through, no texture stage.
 * Shaders, DXT bitmaps, and NV2A programs are not sampled.
 */
#ifndef GX_DRAW_H
#define GX_DRAW_H

#include <gccore.h>
#include <stdint.h>

struct gx_mesh_view {
	float const *positions;
	uint8_t const *colors;
	uint16_t const *indices;
	uint16_t vertex_count;
	uint16_t triangle_count;
	float center[3];
	float radius;
	float yaw;
	float pitch;
};

/* GX must already be initialised. Copies the frame onto xfb. */
void gx_draw_mesh(GXRModeObj *mode, void *xfb, struct gx_mesh_view const *view);

#endif
