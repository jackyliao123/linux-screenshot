#ifndef SELECTION_H
#define SELECTION_H

#include <gtk/gtk.h>


enum selection_status {
    SELECTION_STATUS_NONE = 0,
    SELECTION_STATUS_TEMP = 1,
    SELECTION_STATUS_AREA = 2,
    SELECTION_STATUS_CREATE = 3,
};

struct selection {
	GtkWidget *widget;

    int x1, y1, x2, y2;
    enum selection_status state;
    GtkDrawingArea* bgimage;
};

struct selection *selection_init(struct overlay *overlay);

#endif