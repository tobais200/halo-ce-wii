/*
 * Compile-only stand-ins. Each function fails on purpose.
 * Do not wire these into boot.dol until a real backend exists, and do
 * not describe a successful return as a running campaign.
 */
#include "wii_platform.h"

const char *wii_platform_banner(void)
{
	return "OpenCE Wii platform skeleton: campaign not linked";
}

const char *wii_platform_data_root(void)
{
	return 0;
}

int wii_platform_translate_path(const char *xbox_path, char *host_path, unsigned long host_path_size)
{
	(void)xbox_path;
	(void)host_path;
	(void)host_path_size;
	return -1;
}

int wii_platform_present(void)
{
	return -1;
}
