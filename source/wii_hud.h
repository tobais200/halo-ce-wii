/*
 * Scenario chapter-title and HUD strings, rasterized with a font tag's
 * glyphs when one is in the cache. The widget font layout is the same
 * one DeLa already uses.
 */
#ifndef WII_HUD_H
#define WII_HUD_H

#include <stdint.h>

void wii_hud_bind(uint32_t font_va);
int wii_hud_raster(char const *text, uint8_t *canvas, int width, int height);

#endif
