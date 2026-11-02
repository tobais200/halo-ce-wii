#include "wii_widget.h"

#include "wii_tex.h"

#include <stdio.h>
#include <string.h>

#define TAG_CACHE_BASE 0x803A6000u
#define DELA_GROUP 0x614c6544u
#define WIDGET_BYTES 0x3ECu
#define CHILD_BYTES 0x50u
#define HANDLER_BYTES 0x48u
#define BACKGROUND_INDEX 0x44u
#define CHILD_BLOCK 0x3E0u
#define HANDLER_BLOCK 0x54u
#define BOUNDS_OFFSET 0x24u

static char const *campaign_names[] = {
	"a10", "a30", "a50", "b30", "b40", "c10", "c20", "c40", "d20", "d40", 0
};

static uint32_t read_u32(uint8_t const *bytes)
{
	return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8)
		| ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static uint16_t read_u16(uint8_t const *bytes)
{
	return (uint16_t)(bytes[0] | (bytes[1] << 8));
}

static int pull(bsp_read_fn read, void *context, uint32_t file_size, uint32_t offset, void *destination, uint32_t size)
{
	if ((uint64_t)offset + size > file_size)
		return -1;
	return read(context, offset, destination, size);
}

static int map_va(uint32_t va, uint32_t tag_offset, uint32_t tag_size, uint32_t *file_offset)
{
	if (!tag_size || va < TAG_CACHE_BASE || (va - TAG_CACHE_BASE) >= tag_size)
		return -1;
	*file_offset = tag_offset + (va - TAG_CACHE_BASE);
	return 0;
}

static int name_eq(char const *a, char const *b)
{
	while (*a && *b) {
		char ca = *a == '/' ? '\\' : *a;
		char cb = *b == '/' ? '\\' : *b;
		if (ca >= 'A' && ca <= 'Z')
			ca = (char)(ca - 'A' + 'a');
		if (cb >= 'A' && cb <= 'Z')
			cb = (char)(cb - 'A' + 'a');
		if (ca != cb)
			return 0;
		a++;
		b++;
	}
	return *a == 0 && *b == 0;
}

static int campaign_id(char const *name, char *out)
{
	int index;

	for (index = 0; campaign_names[index]; index++) {
		if (name_eq(name, campaign_names[index])) {
			memcpy(out, campaign_names[index], 4);
			return 1;
		}
	}
	return 0;
}

static int read_name(bsp_read_fn read, void *context, uint32_t file_size, uint32_t tag_offset, uint32_t tag_size, uint32_t va, char *out, uint32_t out_size)
{
	uint32_t file_offset;
	uint32_t index;

	if (!out_size)
		return -1;
	out[0] = 0;
	if (map_va(va, tag_offset, tag_size, &file_offset) != 0)
		return -1;
	for (index = 0; index + 1 < out_size; index++) {
		char c;
		if (pull(read, context, file_size, file_offset + index, &c, 1) != 0)
			return -1;
		out[index] = c;
		if (!c) {
			out[index] = 0;
			return 0;
		}
	}
	out[out_size - 1] = 0;
	return 0;
}

static int widget_file_for_index(bsp_read_fn read, void *context, uint32_t file_size, uint32_t tag_offset, uint32_t tag_size, uint32_t datum, uint32_t *file_offset)
{
	uint8_t header[4];
	uint8_t instance[0x20];
	uint32_t instances;
	uint32_t base;
	uint32_t group;

	if (datum == 0xffffffffu)
		return -1;
	if (pull(read, context, file_size, tag_offset, header, 4) != 0)
		return -1;
	instances = read_u32(header);
	if (map_va(instances, tag_offset, tag_size, &instances) != 0)
		return -1;
	if (pull(read, context, file_size, instances + (datum & 0xffffu) * 0x20u, instance, sizeof(instance)) != 0)
		return -1;
	group = read_u32(instance);
	if (group != DELA_GROUP)
		return -1;
	base = read_u32(instance + 0x14);
	return map_va(base, tag_offset, tag_size, file_offset);
}

static int find_root(bsp_read_fn read, void *context, uint32_t file_size, uint32_t tag_offset, uint32_t tag_size, uint32_t *file_offset)
{
	uint8_t header[0x28];
	uint32_t instances;
	uint32_t count;
	uint32_t limit;
	uint32_t index;
	uint32_t fallback = 0;
	int have_fallback = 0;

	if (pull(read, context, file_size, tag_offset, header, sizeof(header)) != 0)
		return -1;
	instances = read_u32(header);
	count = read_u32(header + 0x0c);
	if (map_va(instances, tag_offset, tag_size, &instances) != 0)
		return -1;
	limit = count > 2048u ? 2048u : count;
	for (index = 0; index < limit; index++) {
		uint8_t instance[0x20];
		char name[80];
		uint32_t base;

		if (pull(read, context, file_size, instances + index * 0x20u, instance, sizeof(instance)) != 0)
			return -1;
		if (read_u32(instance) != DELA_GROUP)
			continue;
		base = read_u32(instance + 0x14);
		if (map_va(base, tag_offset, tag_size, &base) != 0)
			continue;
		if (!have_fallback) {
			fallback = base;
			have_fallback = 1;
		}
		if (read_name(read, context, file_size, tag_offset, tag_size, read_u32(instance + 0x10), name, sizeof(name)) != 0)
			continue;
		if (name_eq(name, "ui\\shell\\main_menu\\main_menu")) {
			*file_offset = base;
			return 0;
		}
	}
	if (!have_fallback)
		return -1;
	*file_offset = fallback;
	return 0;
}

int wii_widget_open_root(
	bsp_read_fn read,
	void *context,
	uint32_t file_size,
	uint32_t tag_offset,
	uint32_t tag_size,
	struct wii_widget *widget)
{
	uint32_t root;

	if (!widget)
		return -1;
	memset(widget, 0, sizeof(*widget));
	if (find_root(read, context, file_size, tag_offset, tag_size, &root) != 0) {
		snprintf(widget->message, sizeof(widget->message), "no DeLa widget in this cache");
		return -1;
	}
	widget->stack[0] = root;
	widget->depth = 0;
	snprintf(widget->message, sizeof(widget->message), "DeLa root");
	return 0;
}

static void blit_fit(uint8_t *canvas, uint16_t canvas_w, uint16_t canvas_h, struct wii_rgba const *image, int x0, int y0, int x1, int y1)
{
	int x;
	int y;
	int span_x;
	int span_y;

	if (x1 < x0) {
		int swap = x0;
		x0 = x1;
		x1 = swap;
	}
	if (y1 < y0) {
		int swap = y0;
		y0 = y1;
		y1 = swap;
	}
	if (x1 - x0 < 2 || y1 - y0 < 2) {
		x0 = 0;
		y0 = 0;
		x1 = canvas_w;
		y1 = canvas_h;
	}
	span_x = x1 - x0;
	span_y = y1 - y0;
	for (y = y0; y < y1; y++) {
		int sy;
		if (y < 0 || y >= canvas_h)
			continue;
		sy = (y - y0) * image->height / span_y;
		if (sy < 0)
			sy = 0;
		if (sy >= image->height)
			sy = image->height - 1;
		for (x = x0; x < x1; x++) {
			int sx;
			uint8_t const *src;
			if (x < 0 || x >= canvas_w)
				continue;
			sx = (x - x0) * image->width / span_x;
			if (sx < 0)
				sx = 0;
			if (sx >= image->width)
				sx = image->width - 1;
			src = image->pixels + ((uint32_t)sy * image->width + (uint32_t)sx) * 4u;
			memcpy(canvas + ((uint32_t)y * canvas_w + (uint32_t)x) * 4u, src, 4);
		}
	}
}

static void focus_edge(uint8_t *canvas, uint16_t canvas_w, uint16_t canvas_h, int x0, int y0, int x1, int y1)
{
	int x;
	int y;

	if (x1 < x0) {
		int swap = x0;
		x0 = x1;
		x1 = swap;
	}
	if (y1 < y0) {
		int swap = y0;
		y0 = y1;
		y1 = swap;
	}
	for (x = x0; x < x1; x++) {
		if (x < 0 || x >= canvas_w)
			continue;
		if (y0 >= 0 && y0 < canvas_h)
			memset(canvas + ((uint32_t)y0 * canvas_w + (uint32_t)x) * 4u, 255, 4);
		if (y1 - 1 >= 0 && y1 - 1 < canvas_h)
			memset(canvas + ((uint32_t)(y1 - 1) * canvas_w + (uint32_t)x) * 4u, 255, 4);
	}
	for (y = y0; y < y1; y++) {
		if (y < 0 || y >= canvas_h)
			continue;
		if (x0 >= 0 && x0 < canvas_w)
			memset(canvas + ((uint32_t)y * canvas_w + (uint32_t)x0) * 4u, 255, 4);
		if (x1 - 1 >= 0 && x1 - 1 < canvas_w)
			memset(canvas + ((uint32_t)y * canvas_w + (uint32_t)(x1 - 1)) * 4u, 255, 4);
	}
}

static int paint_bitmap(bsp_read_fn read, void *context, uint32_t file_size, uint32_t tag_offset, uint32_t tag_size, uint32_t datum, uint8_t *scratch, uint32_t scratch_bytes, struct wii_rgba *image)
{
	uint8_t header[4];
	uint8_t instance[0x20];
	uint32_t instances;
	uint32_t base;
	uint32_t group;

	if (datum == 0xffffffffu || pull(read, context, file_size, tag_offset, header, 4) != 0)
		return -1;
	instances = read_u32(header);
	if (map_va(instances, tag_offset, tag_size, &instances) != 0)
		return -1;
	if (pull(read, context, file_size, instances + (datum & 0xffffu) * 0x20u, instance, sizeof(instance)) != 0)
		return -1;
	base = read_u32(instance + 0x14);
	if (map_va(base, tag_offset, tag_size, &group) != 0)
		return -1;
	image->pixels = scratch;
	image->cap = 128;
	image->ok = 0;
	(void)scratch_bytes;
	return wii_tex_load_group(read, context, file_size, tag_offset, tag_size, group, 0, image);
}

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
	uint32_t scratch_bytes)
{
	uint8_t definition[WIDGET_BYTES];
	struct wii_rgba image;
	uint32_t child_count;
	uint32_t child_va;
	uint32_t child_file;
	uint32_t index;
	int focus_box[4];
	int have_focus = 0;

	if (!widget || !canvas || !scratch)
		return -1;
	memset(canvas, 0, (uint32_t)canvas_w * canvas_h * 4u);
	widget->child_count = 0;
	if (pull(read, context, file_size, widget->stack[widget->depth], definition, sizeof(definition)) != 0)
		return -1;
	memcpy(widget->name, definition + 4, 31);
	widget->name[31] = 0;
	if (paint_bitmap(read, context, file_size, tag_offset, tag_size, read_u32(definition + BACKGROUND_INDEX), scratch, scratch_bytes, &image) == 0)
		blit_fit(canvas, canvas_w, canvas_h, &image, 0, 0, canvas_w, canvas_h);

	child_count = read_u32(definition + CHILD_BLOCK);
	child_va = read_u32(definition + CHILD_BLOCK + 4);
	if (child_count && map_va(child_va, tag_offset, tag_size, &child_file) == 0) {
		if (child_count > WII_WIDGET_CHILDREN)
			child_count = WII_WIDGET_CHILDREN;
		for (index = 0; index < child_count; index++) {
			uint8_t child[CHILD_BYTES];
			uint8_t child_def[WIDGET_BYTES];
			uint32_t child_widget;
			int x0, y0, x1, y1;

			if (pull(read, context, file_size, child_file + index * CHILD_BYTES, child, sizeof(child)) != 0)
				break;
			memcpy(widget->child_name[widget->child_count], child + 0x10, 31);
			widget->child_name[widget->child_count][31] = 0;
			widget->child_widget[widget->child_count] = 0;
			if (widget_file_for_index(read, context, file_size, tag_offset, tag_size, read_u32(child + 0x0c), &child_widget) == 0)
				widget->child_widget[widget->child_count] = child_widget;
			x0 = y0 = 0;
			x1 = canvas_w;
			y1 = canvas_h;
			if (child_widget && pull(read, context, file_size, child_widget, child_def, 0x48) == 0) {
				int16_t by0 = (int16_t)read_u16(child_def + BOUNDS_OFFSET);
				int16_t bx0 = (int16_t)read_u16(child_def + BOUNDS_OFFSET + 2);
				int16_t by1 = (int16_t)read_u16(child_def + BOUNDS_OFFSET + 4);
				int16_t bx1 = (int16_t)read_u16(child_def + BOUNDS_OFFSET + 6);
				x0 = bx0 * canvas_w / 640;
				x1 = bx1 * canvas_w / 640;
				y0 = by0 * canvas_h / 480;
				y1 = by1 * canvas_h / 480;
				if (paint_bitmap(read, context, file_size, tag_offset, tag_size, read_u32(child_def + BACKGROUND_INDEX), scratch, scratch_bytes, &image) == 0)
					blit_fit(canvas, canvas_w, canvas_h, &image, x0, y0, x1, y1);
			}
			if ((int)index == widget->focus) {
				focus_box[0] = x0;
				focus_box[1] = y0;
				focus_box[2] = x1;
				focus_box[3] = y1;
				have_focus = 1;
			}
			widget->child_count++;
		}
	}
	if (have_focus)
		focus_edge(canvas, canvas_w, canvas_h, focus_box[0], focus_box[1], focus_box[2], focus_box[3]);
	if (widget->focus >= widget->child_count)
		widget->focus = 0;
	snprintf(widget->message, sizeof(widget->message), "DeLa %.24s", widget->name);
	return 0;
}

int wii_widget_input(struct wii_widget *widget, int button,
	bsp_read_fn read, void *context, uint32_t file_size,
	uint32_t tag_offset, uint32_t tag_size)
{
	uint8_t definition[WIDGET_BYTES];
	uint32_t next;

	if (!widget)
		return 2;
	widget->scenario[0] = 0;
	if (button == 8 || button == 9) {
		if (widget->child_count) {
			if (button == 8)
				widget->focus = widget->focus > 0 ? widget->focus - 1 : widget->child_count - 1;
			else
				widget->focus = (widget->focus + 1) % widget->child_count;
		}
		return 0;
	}
	if (button == 1) {
		if (widget->depth > 0) {
			widget->depth--;
			widget->focus = 0;
			return 0;
		}
		return 2;
	}
	if (button != 0)
		return 0;
	if (widget->child_count) {
		if (campaign_id(widget->child_name[widget->focus], widget->scenario))
			return 1;
		if (widget->child_widget[widget->focus] && widget->depth + 1 < WII_WIDGET_STACK) {
			widget->depth++;
			widget->stack[widget->depth] = widget->child_widget[widget->focus];
			widget->focus = 0;
			return 0;
		}
	}
	if (pull(read, context, file_size, widget->stack[widget->depth], definition, sizeof(definition)) != 0)
		return 0;
	{
		uint32_t handlers = read_u32(definition + HANDLER_BLOCK);
		uint32_t handler_va = read_u32(definition + HANDLER_BLOCK + 4);
		uint32_t handler_file;
		uint32_t index;
		if (handlers > 16)
			handlers = 16;
		if (handlers && map_va(handler_va, tag_offset, tag_size, &handler_file) == 0) {
			for (index = 0; index < handlers; index++) {
				uint8_t handler[HANDLER_BYTES];
				if (pull(read, context, file_size, handler_file + index * HANDLER_BYTES, handler, sizeof(handler)) != 0)
					break;
				if (read_u16(handler + 4) != 0)
					continue;
				handler[0x28 + 31] = 0;
				if (campaign_id((char *)(handler + 0x28), widget->scenario))
					return 1;
				if (widget_file_for_index(read, context, file_size, tag_offset, tag_size, read_u32(handler + 0x14), &next) == 0
					&& widget->depth + 1 < WII_WIDGET_STACK) {
					widget->depth++;
					widget->stack[widget->depth] = next;
					widget->focus = 0;
					return 0;
				}
			}
		}
	}
	return 0;
}
