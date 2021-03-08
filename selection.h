#ifndef SELECTION_H
#define SELECTION_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include "geometry.h"

struct overlay;

enum drag_status {
    DRAG_STATUS_NONE              = 0,
    DRAG_STATUS_CREATE            = 1,
	DRAG_STATUS_MOVE              = 2,
	DRAG_STATUS_MOVE_LEFT         = DRAG_STATUS_MOVE + 1,
	DRAG_STATUS_MOVE_RIGHT        = DRAG_STATUS_MOVE + 2,
	DRAG_STATUS_MOVE_TOP          = DRAG_STATUS_MOVE + 3,
	DRAG_STATUS_MOVE_TOP_LEFT     = DRAG_STATUS_MOVE + 4,
	DRAG_STATUS_MOVE_TOP_RIGHT    = DRAG_STATUS_MOVE + 5,
	DRAG_STATUS_MOVE_BOTTOM       = DRAG_STATUS_MOVE + 6,
	DRAG_STATUS_MOVE_BOTTOM_LEFT  = DRAG_STATUS_MOVE + 7,
	DRAG_STATUS_MOVE_BOTTOM_RIGHT = DRAG_STATUS_MOVE + 8,
	NUM_DRAG_STATUS
};

struct selection {
	GtkWidget *widget;

    GtkWidget* bgimage;

    GdkCursor *cursors[NUM_DRAG_STATUS];

    int drag_threshold;
    int edge_threshold;

	bool drag_threshold_reached;
	enum drag_status drag_status;
	struct rect prev_selected;
	int px, py;
	int mouse_x, mouse_y;

	bool has_selected;
    struct rect selected;

	bool has_suggested;
	struct rect suggested;
};

struct selection *selection_init(struct overlay *overlay);
void selection_post_init();

#endif