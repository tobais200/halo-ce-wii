/*
 * Wii platform skeleton for a future OpenCE port.
 *
 * OpenCE's Linux layer (third_party/OpenCE/port/linux) replaces the Xbox
 * SDK: files, D3D8, sound, networking. This header names the Wii-shaped
 * holes. Nothing here loads a map or draws a frame of the game.
 */
#ifndef WII_PLATFORM_H
#define WII_PLATFORM_H

/* Static line for logs and for `make -C port/wii status` readers. */
const char *wii_platform_banner(void);

/*
 * TODO: return the SD directory the player pointed at their own Xbox
 * Halo CE data. NULL until libfat mount and a path policy exist.
 * Never ship that data in the repository.
 */
const char *wii_platform_data_root(void);

/*
 * TODO: mirror platform_translate_path from the Linux port.
 * Xbox paths (d:\maps\..., t:\, u:\, z:\) become a path under the data
 * root, matched case-insensitively. Returns -1 until that exists.
 */
int wii_platform_translate_path(const char *xbox_path, char *host_path, unsigned long host_path_size);

/*
 * TODO: the D3D8 Present path. On Wii that is GX_DrawDone, a copy into
 * the visible XFB, VIDEO_SetNextFramebuffer, and VIDEO_WaitVSync.
 * The desktop implementation is OpenGL in d3d8_gl.c and does not link.
 * Returns -1.
 */
int wii_platform_present(void);

#endif
