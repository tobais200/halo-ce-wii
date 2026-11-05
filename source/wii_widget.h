/*
 * Data-driven slice of OpenCE's DeLa widgets (ui_widget.c offsets).
 * Draws background, child bitmaps, and string-list glyphs. Event handlers
 * follow the retail flag bits. The widget memory pool and the other event
 * function bodies are not run.
 */
#ifndef WII_WIDGET_H
#define WII_WIDGET_H

#include "bsp_preview.h"

#include <stdint.h>

#define WII_WIDGET_CHILDREN 6
#define WII_WIDGET_STACK 6

struct wii_widget {
	uint32_t stack[WII_WIDGET_STACK];
	int depth;
	int focus;
	int child_count;
	uint32_t child_widget[WII_WIDGET_CHILDREN];
	char name[32];
	char child_name[WII_WIDGET_CHILDREN][32];
	char scenario[8];
	char message[80];
};

/* 0 when ui\shell\main_menu\main_menu, or another DeLa, was opened. */
int wii_widget_open_root(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_offset,
	uint32_t tag_size,
	struct wii_widget *widget);

/* Compose one  screen of original bitmaps into rgba8 canvas. 0 on success. */
int wii_widget_compose(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_offset,
	uint32_t tag_size,
	struct wii_widget *widget,
	uint8_t *canvas,
	uint16_t canvas_w,
	uint16_t canvas_h,
	uint8_t *scratch,
	uint32_t scratch_bytes);

/* Gamepad indices from input.h: A=0, B=1, dpad up=8, down=9.
   0 redraw, 1 load widget->scenario, 2 leave to the file picker. */
int wii_widget_input(struct wii_widget *widget, int button,
	bsp_read_fn read, void *context, uint32_t file_size,
	uint32_t tag_offset, uint32_t tag_size);

#endif
