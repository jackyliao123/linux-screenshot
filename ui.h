#ifndef OVERLAY_H
#define OVERLAY_H

#include <gtk/gtk.h>

#include "display.h"
#include "writer.h"
#include "keybind.h"

struct screenshot_region {
	size_t refcount;
	struct screenshot *screenshot;
	struct rect bounds;
};

struct ui {
	struct screenshot *screenshot;
	struct window_tree *tree;
	struct output_list *outputs;

	bool grabbed;

	GtkWidget *window;
	GdkWindow *gdk_window;
	GdkDisplay *gdk_display;
	GdkSeat *gdk_seat;

	cairo_surface_t *drawing_surface;
	cairo_surface_t *memory_surface;
};

extern struct ui ui;

enum keybind_handle_status keybind_handle(struct keybind *keybind, struct keybind_exec_context *context);
void overlay_init(struct screenshot *screenshot, struct window_tree *tree, struct output_list *outputs);

#endif
