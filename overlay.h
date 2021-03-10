#ifndef OVERLAY_H
#define OVERLAY_H

#include <gtk/gtk.h>

#include "display.h"
#include "writer.h"

struct overlay {
    struct screenshot *screenshot;
    struct window_tree *tree;
	struct output_list *outputs;

	struct tooltip *tooltip;
    struct selection *selection;

    GtkWidget *window;
    GdkWindow *gdk_window;
	GdkDisplay *gdk_display;

	cairo_surface_t *screenshot_surface;

	struct image_writer *writer;
};

struct overlay *overlay_init(struct screenshot *screenshot, struct window_tree *tree, struct output_list *outputs, struct image_writer *writer);

#endif