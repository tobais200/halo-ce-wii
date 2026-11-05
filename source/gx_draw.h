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
	float const *uvs;
	uint16_t const *indices;
	uint16_t vertex_count;
	uint16_t triangle_count;
	float center[3];
	float radius;
	float yaw;
	float pitch;
	/* 0 vertex color, 1 lightmap modulated by vertex color, 2 bitmap only */
	int texture_mode;
	/* Camera sits on center and looks along yaw/pitch. Halo Z is up. */
	int eye;
};

/* Copies rgba8 (R,G,B,A) into a MEM1 GX_TF_RGBA8 texture. 0 on success. */
int gx_draw_set_rgba(uint8_t const *rgba, uint16_t width, uint16_t height);
int gx_draw_set_effect_rgba(uint8_t const *rgba, uint16_t width, uint16_t height);
void gx_draw_clear_texture(void);

/* GX must already be initialised. Copies the frame onto xfb. */
void gx_draw_mesh(GXRModeObj *mode, void *xfb, struct gx_mesh_view const *view);

/* Original bitmap pixels, full frame, no tint. */
void gx_draw_bitmap(GXRModeObj *mode, void *xfb);

#endif
