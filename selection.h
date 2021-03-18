#ifndef SELECTION_H
#define SELECTION_H

#include <gtk/gtk.h>
#include <stdbool.h>
#include "geometry.h"

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

enum modifier_key {
	MODIFIER_KEY_ADD_SELECTION = 0,
	MODIFIER_KEY_SELECT_OUTPUT = 1,
	MODIFIER_KEY_RESIZE_CORNER = 2,
	MODIFIER_KEY_REMOVE_OVERLAY = 3,
	NUM_MODIFIER_KEY
};

enum modifier_mask {
	MODIFIER_MASK_ADD_SELECTION = 1 << MODIFIER_KEY_ADD_SELECTION,
	MODIFIER_MASK_SELECT_OUTPUT = 1 << MODIFIER_KEY_SELECT_OUTPUT,
	MODIFIER_MASK_RESIZE_CORNER = 1 << MODIFIER_KEY_RESIZE_CORNER,
	MODIFIER_MASK_REMOVE_OVERLAY = 1 << MODIFIER_KEY_REMOVE_OVERLAY,
};

struct selection {
	GtkWidget *widget;

	GtkWidget* bgimage;

	GdkCursor *cursors[NUM_DRAG_STATUS];

	enum modifier_mask modifier_mask;
	guint modifier_codes[NUM_MODIFIER_KEY];

	int drag_threshold;
	int edge_threshold;

	bool drag_threshold_reached;
	enum drag_status drag_status;
	enum drag_status preview_drag_status;
	struct rect prev_selected;
	int px, py;
	int mouse_x, mouse_y;

	bool has_selected;
	struct rect selected;

	bool has_suggested;
	struct rect suggested;
};

extern struct selection selection;

void selection_init();
void selection_post_init(void);
struct rect selection_get_selection();
bool selection_deselect();

#endif
