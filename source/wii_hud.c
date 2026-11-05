/*
 * Blit ASCII from a Halo font tag. Offsets match source/wii_widget.c,
 * which already draws string-list glyphs from the UI cache.
 */
#include "wii_hud.h"
#include "wii_tag_cache.h"

#include <string.h>

static uint32_t font_va;

static uint32_t read_u32(uint8_t const *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t read_u16(uint8_t const *p)
{
	return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

void wii_hud_bind(uint32_t va)
{
	font_va = va;
}

static void blot(uint8_t *canvas, int width, int height, int x, int y, uint8_t const *alpha, int gw, int gh)
{
	int row, col;

	for (row = 0; row < gh; row++) {
		for (col = 0; col < gw; col++) {
			int dx = x + col;
			int dy = y + row;

			if (dx < 0 || dy < 0 || dx >= width || dy >= height)
				continue;
			if (alpha[row * gw + col] > canvas[dy * width + dx])
				canvas[dy * width + dx] = alpha[row * gw + col];
		}
	}
}

int wii_hud_raster(char const *text, uint8_t *canvas, int width, int height)
{
	uint8_t head[0xa0];
	uint8_t table[12];
	int pen = 1;
	int drawn = 0;
	int index;

	if (!text || !canvas || width < 1 || height < 1 || !font_va)
		return 0;
	memset(canvas, 0, (size_t)width * (size_t)height);
	if (wii_tag_cache_pull(font_va, head, sizeof(head)) != 0)
		return 0;
	if (read_u32(head + 0x30) < 1 || wii_tag_cache_pull(read_u32(head + 0x34), table, sizeof(table)) != 0)
		return 0;
	if (read_u32(table) < 256)
		return 0;
	for (index = 0; text[index] && pen < width; index++) {
		unsigned char code = (unsigned char)text[index];
		uint8_t index_bytes[2];
		uint8_t character[0x14];
		uint8_t alpha[64];
		uint16_t which;
		int gw, gh;
		uint32_t pixels;
		uint32_t chars;
		uint32_t chars_va;
		uint32_t blob;

		if (code < 32)
			continue;
		if (wii_tag_cache_pull(read_u32(table + 4) + (uint32_t)code * 2u, index_bytes, 2) != 0)
			break;
		which = read_u16(index_bytes);
		chars = read_u32(head + 0x7c);
		chars_va = read_u32(head + 0x80);
		if (which == 0xffffu || which >= chars)
			continue;
		if (wii_tag_cache_pull(chars_va + (uint32_t)which * 0x14u, character, sizeof(character)) != 0)
			break;
		gw = (int)(int16_t)read_u16(character + 4);
		gh = (int)(int16_t)read_u16(character + 6);
		pixels = read_u32(character + 16);
		blob = read_u32(head + 0x88 + 12);
		if (gw <= 0 || gh <= 0 || gw * gh > (int)sizeof(alpha))
			continue;
		if (wii_tag_cache_pull(blob + pixels, alpha, (uint32_t)(gw * gh)) != 0)
			continue;
		blot(canvas, width, height, pen, 1, alpha, gw, gh);
		pen += gw + 1;
		drawn += gw * gh;
	}
	return drawn;
}
