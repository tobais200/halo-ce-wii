/*
 * GX helpers for the engine stub.
 *
 * See gx_stub.h for the D3D8 to GX notes. OpenCE translates the Xbox device
 * to OpenGL; this file does not. It only proves the DOL can open GX and
 * copy a clear. The visible status text is drawn by the VI console in
 * main.c, into a different XFB, so the copy below does not erase it.
 */
#include "gx_stub.h"

#include <malloc.h>
#include <string.h>

int gx_stub_init(gx_stub *gx, GXRModeObj *rmode, void *clear_xfb)
{
	f32 yscale;
	u32 xfb_height;
	GXColor background = {0, 0, 0, 0xff};

	if (!gx || !rmode || !clear_xfb)
		return -1;

	memset(gx, 0, sizeof(*gx));
	gx->rmode = rmode;
	gx->clear_xfb = clear_xfb;
	gx->fifo_size = GX_STUB_FIFO_SIZE;
	gx->fifo = memalign(32, gx->fifo_size);
	if (!gx->fifo)
		return -1;

	memset(gx->fifo, 0, gx->fifo_size);
	GX_Init(gx->fifo, gx->fifo_size);

	GX_SetCopyClear(background, 0x00ffffff);
	GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
	yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
	xfb_height = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth, xfb_height);
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,
		((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

	if (rmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetCullMode(GX_CULL_NONE);
	GX_SetDispCopyGamma(GX_GM_1_0);
	GX_CopyDisp(gx->clear_xfb, GX_TRUE);

	gx->ready = 1;
	return 0;
}

void gx_stub_clear(const gx_stub *gx, u8 r, u8 g, u8 b)
{
	GXColor background = {r, g, b, 0xff};

	if (!gx || !gx->ready)
		return;

	GX_SetCopyClear(background, 0x00ffffff);
	GX_SetViewport(0, 0, gx->rmode->fbWidth, gx->rmode->efbHeight, 0, 1);
	GX_CopyDisp(gx->clear_xfb, GX_TRUE);
}

const char *gx_stub_status(const gx_stub *gx)
{
	if (gx && gx->ready)
		return "initialized (clear only, no D3D8 device)";
	return "not initialized";
}
