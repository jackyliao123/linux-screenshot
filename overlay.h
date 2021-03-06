#ifndef OVERLAY_H
#define OVERLAY_H

#include <gtk/gtk.h>

#include "display.h"

struct overlay {
    struct screenshot *screenshot;
    struct window_tree *tree;

    struct tooltip *tooltip;
    struct selection *selection;

    GtkWidget *window;

    cairo_surface_t *screenshot_surface;
};

struct overlay *overlay_init(struct screenshot *screenshot, struct window_tree *tree);

#endif