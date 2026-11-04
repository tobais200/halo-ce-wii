#include "gx_draw.h"

#include "wii_mem.h"

#include <malloc.h>
#include <math.h>
#include <string.h>

static void *gx_texels;
static GXTexObj gx_texture;
static int gx_texture_ready;
static uint16_t gx_tex_w;
static uint16_t gx_tex_h;

static void tile_rgba8(uint8_t *destination, uint8_t const *rgba, uint16_t width, uint16_t height, uint16_t full_w, uint16_t full_h)
{
	uint16_t y;
	uint16_t x;
	uint32_t offset = 0;

	memset(destination, 0, (uint32_t)full_w * full_h * 4u);
	for (y = 0; y < full_h; y += 4) {
		for (x = 0; x < full_w; x += 4) {
			int row;
			int col;

			for (row = 0; row < 4; row++) {
				for (col = 0; col < 4; col++) {
					uint16_t sx = (uint16_t)(x + col);
					uint16_t sy = (uint16_t)(y + row);
					uint8_t const *pixel = rgba;

					if (sx < width && sy < height)
						pixel = rgba + ((uint32_t)sy * width + sx) * 4u;
					destination[offset++] = pixel[3];
					destination[offset++] = pixel[0];
				}
			}
			for (row = 0; row < 4; row++) {
				for (col = 0; col < 4; col++) {
					uint16_t sx = (uint16_t)(x + col);
					uint16_t sy = (uint16_t)(y + row);
					uint8_t const *pixel = rgba;

					if (sx < width && sy < height)
						pixel = rgba + ((uint32_t)sy * width + sx) * 4u;
					destination[offset++] = pixel[1];
					destination[offset++] = pixel[2];
				}
			}
		}
	}
}

int gx_draw_set_rgba(uint8_t const *rgba, uint16_t width, uint16_t height)
{
	uint16_t full_w;
	uint16_t full_h;
	uint32_t bytes;

	gx_draw_clear_texture();
	if (!rgba || !width || !height || width > WII_GX_TEXTURE_MAX || height > WII_GX_TEXTURE_MAX)
		return -1;
	full_w = (uint16_t)((width + 3u) & ~3u);
	full_h = (uint16_t)((height + 3u) & ~3u);
	bytes = (uint32_t)full_w * full_h * 4u;
	gx_texels = memalign(32, bytes);
	if (!gx_texels)
		return -1;
	tile_rgba8(gx_texels, rgba, width, height, full_w, full_h);
	DCFlushRange(gx_texels, bytes);
	GX_InitTexObj(&gx_texture, gx_texels, full_w, full_h, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InvalidateTexAll();
	gx_tex_w = full_w;
	gx_tex_h = full_h;
	gx_texture_ready = 1;
	return 0;
}

void gx_draw_clear_texture(void)
{
	if (gx_texels)
		free(gx_texels);
	gx_texels = 0;
	gx_texture_ready = 0;
}

static void frame_mesh(struct gx_mesh_view const *view, Mtx view_matrix, Mtx44 projection)
{
	guVector camera;
	guVector up;
	guVector look;
	float distance;
	float pitch;

	pitch = view->pitch;
	if (pitch > 1.2f)
		pitch = 1.2f;
	if (pitch < -1.2f)
		pitch = -1.2f;
	distance = view->radius * 2.5f;
	if (distance < 2.0f)
		distance = 2.0f;

	/* Halo's coordinates: Z is up. An eye camera sits on the biped. */
	if (view->eye) {
		float cy = cosf(view->yaw);
		float sy = sinf(view->yaw);
		float cp = cosf(pitch);
		float sp = sinf(pitch);

		camera.x = view->center[0];
		camera.y = view->center[1];
		camera.z = view->center[2];
		look.x = camera.x + cp * cy;
		look.y = camera.y + cp * sy;
		look.z = camera.z + sp;
		distance = 8.0f;
	} else {
		camera.x = view->center[0] + distance * cosf(pitch) * cosf(view->yaw);
		camera.y = view->center[1] + distance * cosf(pitch) * sinf(view->yaw);
		camera.z = view->center[2] + distance * sinf(pitch);
		look.x = view->center[0];
		look.y = view->center[1];
		look.z = view->center[2];
	}
	up.x = 0.0f;
	up.y = 0.0f;
	up.z = 1.0f;
	guLookAt(view_matrix, &camera, &up, &look);

	guPerspective(projection, 55.0f, 4.0f / 3.0f, 0.1f, distance * 40.0f + 100.0f);
}

void gx_draw_mesh(GXRModeObj *mode, void *xfb, struct gx_mesh_view const *view)
{
	Mtx view_matrix;
	Mtx44 projection;
	GXColor background = {12, 14, 18, 0xff};
	uint16_t triangle;
	uint16_t batch;
	uint16_t drawn;
	int textured;

	if (!mode || !xfb || !view)
		return;
	textured = gx_texture_ready && view->texture_mode && view->uvs;

	GX_SetCopyClear(background, 0x00ffffff);
	GX_SetViewport(0.0f, 0.0f, mode->fbWidth, mode->efbHeight, 0.0f, 1.0f);
	GX_SetScissor(0, 0, mode->fbWidth, mode->efbHeight);
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
	GX_SetNumChans(1);
	GX_SetNumTexGens(textured ? 1 : 0);
	GX_SetNumTevStages(1);
	if (textured) {
		GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, view->texture_mode == 2 ? GX_REPLACE : GX_MODULATE);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
		GX_LoadTexObj(&gx_texture, GX_TEXMAP0);
	} else {
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	}
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

	frame_mesh(view, view_matrix, projection);
	GX_LoadProjectionMtx(projection, GX_PERSPECTIVE);
	GX_LoadPosMtxImm(view_matrix, GX_PNMTX0);

	drawn = 0;
	while (view->positions && view->indices && drawn < view->triangle_count) {
		batch = (uint16_t)(view->triangle_count - drawn);
		if (batch > 2048)
			batch = 2048;
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, (uint16_t)(batch * 3));
		for (triangle = 0; triangle < batch; triangle++) {
			uint16_t const *index = view->indices + (drawn + triangle) * 3;
			int corner;

			for (corner = 0; corner < 3; corner++) {
				uint16_t vertex = index[corner];
				float const *position;
				uint8_t const *color;

				if (vertex >= view->vertex_count)
					vertex = 0;
				position = view->positions + vertex * 3;
				color = view->colors ? view->colors + vertex * 4 : 0;
				GX_Position3f32(position[0], position[1], position[2]);
				if (color)
					GX_Color4u8(color[0], color[1], color[2], color[3]);
				else
					GX_Color4u8(180, 180, 180, 255);
				if (textured)
					GX_TexCoord2f32(view->uvs[vertex * 2], view->uvs[vertex * 2 + 1]);
			}
		}
		GX_End();
		drawn = (uint16_t)(drawn + batch);
	}

	GX_DrawDone();
	GX_CopyDisp(xfb, GX_TRUE);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_Flush();
}

void gx_draw_bitmap(GXRModeObj *mode, void *xfb)
{
	static float const positions[12] = {
		0.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f
	};
	static uint8_t const colors[16] = {
		255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	};
	static float const uvs[8] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
	static uint16_t const indices[6] = { 0, 1, 2, 0, 2, 3 };
	struct gx_mesh_view view;

	if (!gx_texture_ready || !mode || !xfb)
		return;
	memset(&view, 0, sizeof(view));
	view.positions = positions;
	view.colors = colors;
	view.uvs = uvs;
	view.indices = indices;
	view.vertex_count = 4;
	view.triangle_count = 2;
	view.center[2] = 0.0f;
	view.radius = 1.2f;
	view.yaw = 0.0f;
	view.pitch = 0.0f;
	view.texture_mode = 2;
	gx_draw_mesh(mode, xfb, &view);
}
