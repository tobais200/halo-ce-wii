/*
 * A small effect and contrail record. The full effect simulator is not
 * linked. Original effect tags are read, and a contrail's bitmap is
 * decoded with the same texel path as the BSP lightmap.
 */
#ifndef WII_EFFECTS_H
#define WII_EFFECTS_H

#include <stdint.h>

#define WII_EFFECT_MAGIC 0x65786665u
#define WII_EFFECT_BITMAP_CAP 128

struct wii_resident_effect {
	uint32_t magic;
	int bitmap_ok;
	uint16_t width;
	uint16_t height;
	uint8_t pixels[WII_EFFECT_BITMAP_CAP * WII_EFFECT_BITMAP_CAP * 4];
};

struct wii_sprite {
	float x;
	float y;
	float z;
	float size;
};

void *wii_effect_load(uint32_t effect_va);
int wii_effect_live(void);
int wii_effect_bitmap_ok(void);
const uint8_t *wii_effect_pixels(uint16_t *width, uint16_t *height);
void wii_effects_tick(void);
void wii_effect_begin_frame(void);
void wii_effect_emit(float x, float y, float z);
void wii_effect_push_bolt(float x, float y, float z, float size);
int wii_effect_sprites(struct wii_sprite *out, int cap);

#endif
