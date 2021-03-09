#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <gtk/gtk.h>

#include "display.h"

struct overlay;

struct tooltip {
	GtkWidget *widget;
    GtkWidget *fixed;
    GtkWidget *popup;
    GtkWidget *zoom;
    double zoom_amount;
    int center_x;
    int center_y;
    GtkWidget **labels;
    size_t label_cnt;
};

struct tooltip *tooltip_init(struct overlay *overlay);
void tooltip_post_init(void);

#endif