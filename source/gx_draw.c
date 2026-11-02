#include "gx_draw.h"

#include <math.h>

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

	/* Halo's coordinates: Z is up. The orbit matches that, not a new axis. */
	camera.x = view->center[0] + distance * cosf(pitch) * cosf(view->yaw);
	camera.y = view->center[1] + distance * cosf(pitch) * sinf(view->yaw);
	camera.z = view->center[2] + distance * sinf(pitch);
	look.x = view->center[0];
	look.y = view->center[1];
	look.z = view->center[2];
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

	if (!mode || !xfb || !view)
		return;

	GX_SetCopyClear(background, 0x00ffffff);
	GX_SetViewport(0.0f, 0.0f, mode->fbWidth, mode->efbHeight, 0.0f, 1.0f);
	GX_SetScissor(0, 0, mode->fbWidth, mode->efbHeight);
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
	GX_SetNumChans(1);
	GX_SetNumTexGens(0);
	GX_SetNumTevStages(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
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
