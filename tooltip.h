#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <gtk/gtk.h>

#include "display.h"

struct overlay;

struct tooltip {
	GtkWidget *widget;
    GtkWidget *fixed;
    GtkWidget *popup;
    double zoom_amount;
    int mouse_x;
    int mouse_y;
    GtkWidget **labels;
    size_t label_cnt;
};

struct tooltip *tooltip_init(struct overlay *overlay);

#endif