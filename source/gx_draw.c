#include "gx_draw.h"

#include "wii_collision.h"
#include "wii_effects.h"
#include "wii_mesh.h"
#include "wii_mem.h"

#include <malloc.h>
#include <math.h>
#include <string.h>

static void *gx_texels;
static GXTexObj gx_texture;
static int gx_texture_ready;
static uint16_t gx_tex_w;
static uint16_t gx_tex_h;
static void *gx_fx_texels;
static GXTexObj gx_fx_texture;
static int gx_fx_ready;

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

int gx_draw_set_effect_rgba(uint8_t const *rgba, uint16_t width, uint16_t height)
{
	uint16_t full_w;
	uint16_t full_h;
	uint32_t bytes;

	if (gx_fx_texels)
		free(gx_fx_texels);
	gx_fx_texels = 0;
	gx_fx_ready = 0;
	if (!rgba || !width || !height || width > WII_GX_TEXTURE_MAX || height > WII_GX_TEXTURE_MAX)
		return -1;
	full_w = (uint16_t)((width + 3u) & ~3u);
	full_h = (uint16_t)((height + 3u) & ~3u);
	bytes = (uint32_t)full_w * full_h * 4u;
	gx_fx_texels = memalign(32, bytes);
	if (!gx_fx_texels)
		return -1;
	tile_rgba8(gx_fx_texels, rgba, width, height, full_w, full_h);
	DCFlushRange(gx_fx_texels, bytes);
	GX_InitTexObj(&gx_fx_texture, gx_fx_texels, full_w, full_h, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	gx_fx_ready = 1;
	return 0;
}

static void overlay_vertex(float x, float y, float z, uint8_t r, uint8_t g, uint8_t b, float u, float v, int textured)
{
	GX_Position3f32(x, y, z);
	GX_Color4u8(r, g, b, 255);
	if (textured)
		GX_TexCoord2f32(u, v);
}

static void draw_overlay(struct gx_mesh_view const *view)
{
	struct wii_sprite sprites[104];
	int count;
	int index;
	float yaw;
	float rx;
	float ry;
	int textured;

	count = wii_effect_sprites(sprites, 104);
	yaw = view->yaw;
	rx = -sinf(yaw);
	ry = cosf(yaw);
	textured = gx_fx_ready && count > 0;
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	if (view->eye) {
		float cy = cosf(view->yaw);
		float sy = sinf(view->yaw);
		float cp = cosf(view->pitch);
		float sp = sinf(view->pitch);
		float fx = cp * cy;
		float fy = cp * sy;
		float fz = sp;
		float ox = view->center[0] + fx * 0.42f + rx * 0.18f;
		float oy = view->center[1] + fy * 0.42f + ry * 0.18f;
		float oz = view->center[2] + fz * 0.42f - 0.18f;

		GX_SetNumTexGens(0);
		GX_SetNumTevStages(1);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, 6);
		overlay_vertex(ox, oy, oz, 40, 42, 46, 0, 0, 0);
		overlay_vertex(ox + fx * 0.28f, oy + fy * 0.28f, oz + fz * 0.28f, 70, 74, 78, 0, 0, 0);
		overlay_vertex(ox + rx * 0.08f, oy + ry * 0.08f, oz - 0.06f, 24, 26, 28, 0, 0, 0);
		overlay_vertex(ox + fx * 0.28f, oy + fy * 0.28f, oz + fz * 0.28f, 70, 74, 78, 0, 0, 0);
		overlay_vertex(ox + rx * 0.08f + fx * 0.22f, oy + ry * 0.08f + fy * 0.22f, oz - 0.06f, 50, 52, 56, 0, 0, 0);
		overlay_vertex(ox + rx * 0.08f, oy + ry * 0.08f, oz - 0.06f, 24, 26, 28, 0, 0, 0);
		GX_End();
	}
	if (count > 0) {
	if (textured) {
		GX_SetNumTexGens(1);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
		GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
		GX_LoadTexObj(&gx_fx_texture, GX_TEXMAP0);
	} else {
		GX_SetNumTexGens(0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	}
	GX_Begin(GX_TRIANGLES, GX_VTXFMT0, (uint16_t)(count * 6));
	for (index = 0; index < count; index++) {
		float s = sprites[index].size;
		float x = sprites[index].x;
		float y = sprites[index].y;
		float z = sprites[index].z;
		float corners[4][5] = {
			{ x - s * rx, y - s * ry, z - s, 0.f, 1.f },
			{ x + s * rx, y + s * ry, z - s, 1.f, 1.f },
			{ x + s * rx, y + s * ry, z + s, 1.f, 0.f },
			{ x - s * rx, y - s * ry, z + s, 0.f, 0.f }
		};
		int tri;

		for (tri = 0; tri < 2; tri++) {
			int a = tri == 0 ? 0 : 0;
			int b = tri == 0 ? 1 : 2;
			int c = tri == 0 ? 2 : 3;

			overlay_vertex(corners[a][0], corners[a][1], corners[a][2], 255, 220, 120, corners[a][3], corners[a][4], textured);
			overlay_vertex(corners[b][0], corners[b][1], corners[b][2], 255, 220, 120, corners[b][3], corners[b][4], textured);
			overlay_vertex(corners[c][0], corners[c][1], corners[c][2], 255, 220, 120, corners[c][3], corners[c][4], textured);
		}
	}
	GX_End();
	}
	if (wii_mesh_triangles() > 0) {
		int mesh_index;
		int mesh_count = wii_mesh_triangles();

		GX_SetNumTexGens(0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
		if (mesh_count > 32)
			mesh_count = 32;
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, (uint16_t)(mesh_count * 3));
		for (mesh_index = 0; mesh_index < mesh_count; mesh_index++) {
			float xyz[9];
			int corner;

			if (!wii_mesh_triangle(mesh_index, xyz))
				continue;
			{
				int weapon_tri = wii_mesh_is_weapon(mesh_index);
				uint16_t shader_width = 0;
				uint16_t shader_height = 0;
				uint8_t const *shader_px = wii_mesh_shader_pixels(&shader_width, &shader_height);
				uint8_t red = shader_px ? shader_px[0] : (weapon_tri ? 70 : 90);
				uint8_t green = shader_px ? shader_px[1] : (weapon_tri ? 90 : 140);
				uint8_t blue = shader_px ? shader_px[2] : (weapon_tri ? 140 : 90);

				for (corner = 0; corner < 3; corner++)
					overlay_vertex(xyz[corner * 3], xyz[corner * 3 + 1], xyz[corner * 3 + 2], red, green, blue, 0, 0, 0);
			}
		}
		GX_End();
	}
	if (wii_pose_segments() > 0) {
		int bone;
		int bones = wii_pose_segments();

		GX_SetNumTexGens(0);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
		GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
		if (bones > 24)
			bones = 24;
		GX_Begin(GX_TRIANGLES, GX_VTXFMT0, (uint16_t)(bones * 6));
		for (bone = 0; bone < bones; bone++) {
			float x0, y0, z0, x1, y1, z1;
			float dx, dy, dz, len;

			if (!wii_pose_segment(bone, &x0, &y0, &z0, &x1, &y1, &z1))
				continue;
			dx = x1 - x0;
			dy = y1 - y0;
			dz = z1 - z0;
			len = sqrtf(dx * dx + dy * dy + dz * dz);
			if (len < 0.02f)
				len = 0.02f;
			overlay_vertex(x0, y0, z0, 220, 180, 90, 0, 0, 0);
			overlay_vertex(x1, y1, z1, 220, 180, 90, 0, 0, 0);
			overlay_vertex(x0 + 0.04f, y0, z0 + 0.04f, 160, 120, 50, 0, 0, 0);
			overlay_vertex(x1, y1, z1, 220, 180, 90, 0, 0, 0);
			overlay_vertex(x0, y0, z0, 220, 180, 90, 0, 0, 0);
			overlay_vertex(x1 + 0.04f, y1, z1 + 0.04f, 160, 120, 50, 0, 0, 0);
		}
		GX_End();
	}
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

	draw_overlay(view);
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
