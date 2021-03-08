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
//	if(selection.state != SELECTION_STATUS_NONE) {
//		int x = MIN(selection.x1, selection.x2);
//		int y = MIN(selection.y1, selection.y2);
//		int width = ABS(selection.x1 - selection.x2) + 1;
//		int height = ABS(selection.y1 - selection.y2) + 1;
		cairo_rectangle(cr, 0, 0, overlay->screenshot->width, overlay->screenshot->height);
//		cairo_rectangle(cr, x, y + height, width, -height);
		cairo_rectangle(cr,
				  selection.selected.x1,
				  selection.selected.y2,
				  selection.selected.x2 - selection.selected.x1,
				  selection.selected.y1 - selection.selected.y2);
		cairo_clip(cr);
//	}
	cairo_paint(cr);

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
	if(!selection.has_selected || mask & GDK_SHIFT_MASK) {
		return DRAG_STATUS_CREATE;
	}
	int edge_threshold = 10;
	int x_dir = 0;
	int y_dir = 0;

	struct rect expanded = selection.selected;
	expanded.x1 -= edge_threshold;
	expanded.y1 -= edge_threshold;
	expanded.x2 += edge_threshold;
	expanded.y2 += edge_threshold;
	if(!contains(expanded, x, y)) {
		return DRAG_STATUS_NONE;
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
	return DRAG_STATUS_MOVE + y_dir * 3 + x_dir;
}

static gboolean
event_mouse_move(GtkWidget *widget, GdkEventMotion *event) {
	GdkModifierType mask;
	gdk_window_get_device_position(event->window, event->device, NULL, NULL, &mask);

	int xmax = overlay->screenshot->width;
	int ymax = overlay->screenshot->height;
    int mouse_x = clamp(event->x, 0, xmax - 1);
    int mouse_y = clamp(event->y, 0, ymax - 1);

	update_suggested(mouse_x, mouse_y, mask);

	int dx = mouse_x - selection.px;
	int dy = mouse_y - selection.py;

	if(selection.drag_status == DRAG_STATUS_NONE) {
		enum drag_status status = compute_drag_status(mouse_x, mouse_y, mask);
		gdk_window_set_device_cursor(event->window, event->device, selection.cursors[status]);
	} else if(selection.drag_status == DRAG_STATUS_CREATE) {
		selection.has_selected = true;
		selection.selected.x1 = MIN(selection.px, mouse_x);
		selection.selected.y1 = MIN(selection.py, mouse_y);
		selection.selected.x2 = MAX(selection.px, mouse_x) + 1;
		selection.selected.y2 = MAX(selection.py, mouse_y) + 1;
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



//	selection.has_selected = true;
//	selection.selected.x = 100;
//	selection.selected.y = 500;
//	selection.selected.width = 1000;
//	selection.selected.height = 700;

//	if(event->state & GDK_BUTTON1_MASK && selection.state == SELECTION_STATUS_TEMP) {
//		selection.x1 = mouse_x;
//		selection.y1 = mouse_y;
//		selection.state = SELECTION_STATUS_CREATE;
//	}
//
//	if(selection.state == SELECTION_STATUS_CREATE) {
//		selection.x2 = mouse_x;
//		selection.y2 = mouse_y;
//	}
//
//	if(selection.state == SELECTION_STATUS_NONE || selection.state == SELECTION_STATUS_TEMP) {
//		if(selection.has_suggested) {
//			selection.state = SELECTION_STATUS_TEMP;
//			selection.x1 = selection.suggested.x;
//			selection.y1 = selection.suggested.y;
//			selection.x2 = selection.x1 + selection.suggested.width - 1;
//			selection.y2 = selection.y1 + selection.suggested.height - 1;
//		} else {
//			selection.state = SELECTION_STATUS_NONE;
//		}
//	}

	return False;
}

static gboolean
event_mouse_press(GtkWidget *widget, GdkEventButton *event) {
	GdkModifierType mask;
	gdk_window_get_device_position(event->window, event->device, NULL, NULL, &mask);

	int xmax = overlay->screenshot->width;
	int ymax = overlay->screenshot->height;
	int mouse_x = clamp(event->x, 0, xmax - 1);
	int mouse_y = clamp(event->y, 0, ymax - 1);

	if(event->button == 1) {
		selection.drag_status = compute_drag_status(mouse_x, mouse_y, mask);
		gdk_window_set_device_cursor(event->window, event->device, selection.cursors[selection.drag_status]);
		selection.px = mouse_x;
		selection.py = mouse_y;
		selection.prev_selected = selection.selected;
	}
	return False;
}

static gboolean
event_mouse_release(GtkWidget *widget, GdkEventButton *event) {
	GdkModifierType mask;
	gdk_window_get_device_position(event->window, event->device, NULL, NULL, &mask);

	int xmax = overlay->screenshot->width;
	int ymax = overlay->screenshot->height;
	int mouse_x = clamp(event->x, 0, xmax - 1);
	int mouse_y = clamp(event->y, 0, ymax - 1);

	if(event->button == 1) {
		selection.drag_status = DRAG_STATUS_NONE;
		gdk_window_set_device_cursor(event->window, event->device, selection.cursors[selection.drag_status]);
	}

//	if(selection.state == SELECTION_STATUS_CREATE) {
//		selection.x2 = event->x;
//		selection.y2 = event->y;
//		selection.state = SELECTION_STATUS_AREA;
//	} else if(selection.state == SELECTION_STATUS_TEMP) {
//		selection.state = SELECTION_STATUS_AREA;
//	}
	return False;
}


static gboolean
event_key_press(GtkWidget *widget, GdkEventKey *event) {
	printf("Key down\n");
	guint16 code = event->hardware_keycode;
	if(code == 9) {
//		if(selection.state == SELECTION_STATUS_TEMP || selection.state == SELECTION_STATUS_NONE) {
			exit(0);
//		} else {
//			selection.state = SELECTION_STATUS_NONE;
//		}
	}
	printf("%d\n", event->hardware_keycode);
	gtk_widget_queue_draw(selection.bgimage);
	return False;
}

struct selection *
selection_init(struct overlay *o) {
    overlay = o;

	selection.bgimage = gtk_drawing_area_new();

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
	g_signal_connect(overlay->window, "motion-notify-event", G_CALLBACK(event_mouse_move), NULL);
	g_signal_connect(selection.bgimage, "draw", G_CALLBACK(draw_bg), NULL);
}