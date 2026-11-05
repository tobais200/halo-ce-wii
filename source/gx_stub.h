/*
 * GX bring-up for the Halo CE Wii engine stub.
 *
 * Xbox Halo CE (build 2342) submits draws through Direct3D 8. OpenCE's
 * maintained desktop port implements that device in
 * third_party/OpenCE/port/linux/src/d3d8_gl.c on top of OpenGL 4.5, with
 * NV2A vertex and pixel programs lowered to GLSL (nv2a_vsh.c, nv2a_psh.c).
 * The Wii has GX, not OpenGL, so that backend cannot be linked here.
 *
 * This header is the note for a future d3d8_gx.c. The functions below only
 * create a FIFO, set a fixed-function clear, and copy the EFB into an
 * off-screen XFB. They do not draw a scene and they do not read game data.
 *
 * D3D8 call                         GX stand-in
 * --------------------------------  ------------------------------------------
 * Clear                             GX_SetCopyClear + GX_CopyDisp
 * SetViewport                       GX_SetViewport (depth 0..1)
 * SetScissorRect                    GX_SetScissor
 * SetTransform WORLD/VIEW           concatenate on the CPU, GX_LoadPosMtxImm
 * SetTransform PROJECTION           GX_LoadProjectionMtx
 * SetRenderState ZENABLE/ZFUNC     GX_SetZMode
 * SetRenderState CULLMODE           GX_SetCullMode (winding must be checked)
 * SetRenderState ALPHABLENDENABLE   GX_SetBlendMode
 * SetTexture / CreateTexture        GX_InitTexObj + GX_LoadTexObj
 *                                   (GX_TF_* is tiled; A8R8G8B8 is not)
 * Texture stages / ps.1.1           TEV. There is no GLSL on GX.
 * NV2A vertex program               CPU transform or a display list
 * DrawPrimitiveUP                   GX_Begin / GX_Position* / GX_End
 *                                   DCFlushRange + GX_InvVtxCache
 * Present                           GX_DrawDone, GX_CopyDisp,
 *                                   VIDEO_SetNextFramebuffer, VIDEO_Flush,
 *                                   VIDEO_WaitVSync
 * CreateVertexBuffer                memalign(32) in MEM1
 * Multi-weight skinning             CPU. GX has no multi-influence skinning.
 *
 * None of the textured or shader paths are implemented in this file.
 * Untextured environment triangles from a structure BSP are drawn by gx_draw.c.
 */
#ifndef GX_STUB_H
#define GX_STUB_H

#include <gccore.h>

#define GX_STUB_FIFO_SIZE (256 * 1024)

typedef struct gx_stub {
	void *fifo;
	u32 fifo_size;
	GXRModeObj *rmode;
	void *clear_xfb;
	int ready;
} gx_stub;

/* 0 on success, -1 if the FIFO or the mode is missing. */
int gx_stub_init(gx_stub *gx, GXRModeObj *rmode, void *clear_xfb);

/* Issue one EFB clear into gx->clear_xfb. Does not change the VI front buffer. */
void gx_stub_clear(const gx_stub *gx, u8 r, u8 g, u8 b);

/* Short line for the status screen. */
const char *gx_stub_status(const gx_stub *gx);

#endif
