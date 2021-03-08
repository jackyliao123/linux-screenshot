#include "selection.h"
#include "overlay.h"

static struct selection selection;
static struct overlay *overlay;

static gint
draw_bg(GtkWidget *widget, cairo_t *cr, gpointer data) {
	cairo_set_source_surface(cr, overlay->screenshot_surface, 0, 0);
	cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
	cairo_paint(cr);

	cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
	if(selection.has_selected) {
		cairo_rectangle(cr, 0, 0, overlay->screenshot->width, overlay->screenshot->height);

		cairo_rectangle(cr,
				  selection.selected.x1,
				  selection.selected.y2,
				  selection.selected.x2 - selection.selected.x1,
				  selection.selected.y1 - selection.selected.y2);
		cairo_clip(cr);
		cairo_paint(cr);
	}

	return False;
}

static void
update_suggested(int x, int y, GdkModifierType mask) {
	selection.has_suggested = false;
	if(mask & GDK_CONTROL_MASK) {
		struct output *output = geom_get_output_under(overlay->outputs, x, y);
		if(output) {
			selection.suggested = output->bounds;
			selection.has_suggested = true;
		}
	} else {
		struct window_tree *window = geom_get_window_under(overlay->tree, x, y);
		if(window) {
			selection.suggested = window->bounds;
			selection.has_suggested = true;
		}
	}
}

static enum drag_status
compute_drag_status(int x, int y, GdkModifierType mask) {
	enum drag_status status;
	if(!selection.has_selected || selection.drag_status == DRAG_STATUS_CREATE || mask & GDK_SHIFT_MASK) {
		status = DRAG_STATUS_CREATE;
		goto done;
	}
	int edge_threshold = selection.edge_threshold;
	int x_dir = 0;
	int y_dir = 0;

	struct rect expanded = selection.selected;
	expanded.x1 -= edge_threshold;
	expanded.y1 -= edge_threshold;
	expanded.x2 += edge_threshold;
	expanded.y2 += edge_threshold;
	if(!contains(expanded, x, y)) {
		status = DRAG_STATUS_NONE;
		goto done;
	}

	if(mask & GDK_MOD1_MASK) {
		edge_threshold = INT_MAX;
	}

	int dx1 = abs(selection.selected.x1 - x);
	int dx2 = abs(selection.selected.x2 - x);
	int dy1 = abs(selection.selected.y1 - y);
	int dy2 = abs(selection.selected.y2 - y);
	if(dx1 <= edge_threshold || dx2 <= edge_threshold) {
		x_dir = dx1 <= dx2 ? 1 : 2;
	}
	if(dy1 <= edge_threshold || dy2 <= edge_threshold) {
		y_dir = dy1 <= dy2 ? 1 : 2;
	}
	status = DRAG_STATUS_MOVE + y_dir * 3 + x_dir;
	goto done;

done:
	gdk_window_set_cursor(overlay->gdk_window, selection.cursors[status]);
	return status;
}

static gboolean
event_mouse_move(GtkWidget *widget, GdkEventMotion *event) {
	int xmax = overlay->screenshot->width;
	int ymax = overlay->screenshot->height;
	int mouse_x = selection.mouse_x = clamp(event->x, 0, xmax - 1);
	int mouse_y = selection.mouse_y = clamp(event->y, 0, ymax - 1);

	update_suggested(mouse_x, mouse_y, event->state);

	int dx = mouse_x - selection.px;
	int dy = mouse_y - selection.py;

	if(selection.drag_status == DRAG_STATUS_NONE) {
		compute_drag_status(mouse_x, mouse_y, event->state);
	} else if(selection.drag_status == DRAG_STATUS_CREATE) {
		if(abs(dx) > selection.drag_threshold || abs(dy) > selection.drag_threshold) {
			selection.drag_threshold_reached = true;
		}
		if(selection.drag_threshold_reached) {
			selection.has_selected = true;
			selection.selected.x1 = MIN(selection.px, mouse_x);
			selection.selected.y1 = MIN(selection.py, mouse_y);
			selection.selected.x2 = MAX(selection.px, mouse_x) + 1;
			selection.selected.y2 = MAX(selection.py, mouse_y) + 1;
		}
	} else if(selection.drag_status == DRAG_STATUS_MOVE) {
		selection.selected.x1 = clamp(selection.prev_selected.x1 + dx, 0, xmax - selection.prev_selected.x2 + selection.prev_selected.x1);
		selection.selected.y1 = clamp(selection.prev_selected.y1 + dy, 0, ymax - selection.prev_selected.y2 + selection.prev_selected.y1);
		selection.selected.x2 = clamp(selection.prev_selected.x2 + dx, selection.prev_selected.x2 - selection.prev_selected.x1, xmax);
		selection.selected.y2 = clamp(selection.prev_selected.y2 + dy, selection.prev_selected.y2 - selection.prev_selected.y1, ymax);
	} else {
		int x_dir = (int) (selection.drag_status - DRAG_STATUS_MOVE) % 3;
		int y_dir = (int) (selection.drag_status - DRAG_STATUS_MOVE) / 3;
		if(x_dir == 1) {
			selection.selected.x1 = clamp(selection.prev_selected.x1 + dx, 0, selection.prev_selected.x2 - 1);
		} else if(x_dir == 2) {
			selection.selected.x2 = clamp(selection.prev_selected.x2 + dx, selection.prev_selected.x1 + 1, xmax);
		}
		if(y_dir == 1) {
			selection.selected.y1 = clamp(selection.prev_selected.y1 + dy, 0, selection.prev_selected.y2 - 1);
		} else if(y_dir == 2) {
			selection.selected.y2 = clamp(selection.prev_selected.y2 + dy, selection.prev_selected.y1 + 1, ymax);
		}
	}
	return False;
}

static gboolean
event_mouse_press(GtkWidget *widget, GdkEventButton *event) {
	int xmax = overlay->screenshot->width;
	int ymax = overlay->screenshot->height;
	int mouse_x = selection.mouse_x = clamp(event->x, 0, xmax - 1);
	int mouse_y = selection.mouse_y = clamp(event->y, 0, ymax - 1);

	if(event->button == 1) {
		selection.drag_status = compute_drag_status(mouse_x, mouse_y, event->state);
		selection.drag_threshold_reached = false;
		selection.px = mouse_x;
		selection.py = mouse_y;
		selection.prev_selected = selection.selected;
	}
	return False;
}

static gboolean
event_mouse_release(GtkWidget *widget, GdkEventButton *event) {
	int xmax = overlay->screenshot->width;
	int ymax = overlay->screenshot->height;
	int mouse_x = selection.mouse_x = clamp(event->x, 0, xmax - 1);
	int mouse_y = selection.mouse_y = clamp(event->y, 0, ymax - 1);

	if(event->button == 1) {
		selection.drag_status = DRAG_STATUS_NONE;
		compute_drag_status(mouse_x, mouse_y, event->state);
	}
	return False;
}

static gboolean
event_key_press(GtkWidget *widget, GdkEventKey *event) {
	update_suggested(selection.mouse_x, selection.mouse_y, event->state);
	compute_drag_status(selection.mouse_x, selection.mouse_y, event->state);
	if(event->keyval == GDK_KEY_Escape) {
		if(selection.has_selected) {
			selection.has_selected = false;
		} else {
			exit(0);
		}
	}
	gtk_widget_queue_draw(selection.bgimage);
	return False;
}

static gboolean
event_key_release(GtkWidget *widget, GdkEventKey *event) {
	update_suggested(selection.mouse_x, selection.mouse_y, event->state);
	compute_drag_status(selection.mouse_x, selection.mouse_y, event->state);
	gtk_widget_queue_draw(selection.bgimage);
	return False;
}

struct selection *
selection_init(struct overlay *o) {
    overlay = o;

	selection.bgimage = gtk_drawing_area_new();

	selection.edge_threshold = 10;
	selection.drag_threshold = 10;
	selection.widget = selection.bgimage;
    return &selection;
}

void
selection_post_init() {
	char *cursor_names[NUM_DRAG_STATUS] = {
			"default", "crosshair",
			"move", "w-resize", "e-resize",
			"n-resize", "nw-resize", "ne-resize",
			"s-resize", "sw-resize", "se-resize"
	};

	for(unsigned i = 0; i < NUM_DRAG_STATUS; ++i) {
		selection.cursors[i] = gdk_cursor_new_from_name(overlay->gdk_display, cursor_names[i]);
	}

	g_signal_connect(overlay->window, "button-press-event", G_CALLBACK(event_mouse_press), NULL);
	g_signal_connect(overlay->window, "button-release-event", G_CALLBACK(event_mouse_release), NULL);
	g_signal_connect(overlay->window, "key-press-event", G_CALLBACK(event_key_press), NULL);
	g_signal_connect(overlay->window, "key-release-event", G_CALLBACK(event_key_release), NULL);
	g_signal_connect(overlay->window, "motion-notify-event", G_CALLBACK(event_mouse_move), NULL);
	g_signal_connect(selection.bgimage, "draw", G_CALLBACK(draw_bg), NULL);
}