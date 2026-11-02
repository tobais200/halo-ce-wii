/*
 * Homebrew Channel entry for the Halo CE Wii engine stub.
 * VIDEO + GX + WPAD. The screen is a status line, not a campaign.
 * HOME returns to the launcher. No map, bitmap, or ISO is opened.
 */
#include <stdio.h>
#include <stdlib.h>

#include <gccore.h>
#include <wiiuse/wpad.h>

#include "gx_stub.h"

static void *console_xfb = NULL;
static GXRModeObj *rmode = NULL;

static int home_pressed(void)
{
	int chan;

	for (chan = 0; chan < 4; chan++) {
		if (WPAD_ButtonsDown(chan) & WPAD_BUTTON_HOME)
			return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	void *clear_xfb;
	gx_stub gx;
	int gx_ok;

	(void)argc;
	(void)argv;

	VIDEO_Init();
	WPAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);
	console_xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	clear_xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	if (!console_xfb || !clear_xfb)
		exit(1);

	console_init(console_xfb, 20, 20,
		rmode->fbWidth, rmode->xfbHeight,
		rmode->fbWidth * VI_DISPLAY_PIX_SZ);

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(console_xfb);
	VIDEO_SetBlack(false);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (rmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();

	gx_ok = gx_stub_init(&gx, rmode, clear_xfb);

	printf("\x1b[2;0H");
	printf("Halo CE Wii - engine stub\n\n");
	printf("Build:   engine stub\n");
	printf("Video:   VI console, GX %s\n",
		gx_ok == 0 ? gx_stub_status(&gx) : "init failed");
	printf("Input:   WPAD (HOME exits)\n");
	printf("Game:    not linked\n");
	printf("Data:    not included\n\n");
	printf("This is not a playable campaign.\n");
	printf("Press HOME to exit.\n");

	while (SYS_MainLoop()) {
		WPAD_ScanPads();
		if (home_pressed())
			exit(0);
		if (gx_ok == 0)
			gx_stub_clear(&gx, 12, 18, 28);
		VIDEO_WaitVSync();
	}

	return 0;
}
