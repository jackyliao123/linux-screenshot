#include "selection.h"
#include "overlay.h"

static struct selection selection;
static struct overlay *overlay;

static gint
draw_bg(GtkWidget *widget, cairo_t *cr, gpointer data) {
	cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
	int stroke_half = 3;

	if(selection.has_selected) {
		cairo_set_line_width(cr, stroke_half * 2);
		cairo_rectangle(cr, 0, 0, overlay->screenshot->width, overlay->screenshot->height);

		cairo_rectangle(cr,
				  selection.selected.x1,
				  selection.selected.y2,
				  selection.selected.x2 - selection.selected.x1,
				  selection.selected.y1 - selection.selected.y2);
		cairo_clip(cr);
		cairo_paint(cr);

		struct rect draw_bounds = geom_expand(&selection.selected, stroke_half);

		cairo_set_source_rgba(cr, 1, 0.5, 0, 0.5);
		cairo_rectangle(cr,
		                draw_bounds.x1,
		                draw_bounds.y1,
		                draw_bounds.x2 - draw_bounds.x1,
		                draw_bounds.y2 - draw_bounds.y1);
		cairo_stroke(cr);
	}

	if(selection.has_suggested && (!selection.has_selected || selection.modifier_mask & MODIFIER_MASK_ADD_SELECTION)) {
		cairo_set_line_width(cr, stroke_half * 2);
		struct rect draw_bounds = selection.suggested;
		if(selection.has_selected) {
			draw_bounds = geom_union(&draw_bounds, &selection.selected);
		}
		cairo_set_source_rgba(cr, 0, 0.5, 0.7, 0.3);
		cairo_rectangle(cr,
		                draw_bounds.x1,
		                draw_bounds.y1,
		                draw_bounds.x2 - draw_bounds.x1,
		                draw_bounds.y2 - draw_bounds.y1);
		cairo_fill(cr);

		draw_bounds = geom_expand(&draw_bounds, stroke_half);
		cairo_set_source_rgba(cr, 0, 0.5, 1, 0.5);
		cairo_rectangle(cr,
		                draw_bounds.x1,
		                draw_bounds.y1,
		                draw_bounds.x2 - draw_bounds.x1,
		                draw_bounds.y2 - draw_bounds.y1);
		cairo_stroke(cr);
	}

	return False;
}

static void
update_suggested(void) {
	selection.has_suggested = false;
	if(selection.modifier_mask & MODIFIER_MASK_SELECT_OUTPUT) {
		struct output *output = geom_get_output_under(overlay->outputs, selection.mouse_x, selection.mouse_y);
		if(output) {
			selection.suggested = output->bounds;
			selection.has_suggested = true;
		}
	} else {
		struct window_tree *window = geom_get_window_under(overlay->tree, selection.mouse_x, selection.mouse_y);
		if(window) {
			selection.suggested = window->bounds;
			selection.has_suggested = true;
		}
	}
}

static enum drag_status
compute_drag_status(void) {
	enum drag_status status;
	if(!selection.has_selected || selection.drag_status == DRAG_STATUS_CREATE || selection.modifier_mask & MODIFIER_MASK_ADD_SELECTION) {
		status = DRAG_STATUS_CREATE;
		goto done;
	}
	int edge_threshold = selection.edge_threshold;
	int x_dir = 0;
	int y_dir = 0;

	struct rect expanded = geom_expand(&selection.selected, edge_threshold);
	if(!contains(&expanded, selection.mouse_x, selection.mouse_y)) {
		status = DRAG_STATUS_NONE;
		goto done;
	}

	if(selection.modifier_mask & MODIFIER_MASK_RESIZE_CORNER) {
		edge_threshold = INT_MAX;
	}

	int dx1 = abs(selection.selected.x1 - selection.mouse_x);
	int dx2 = abs(selection.selected.x2 - selection.mouse_x);
	int dy1 = abs(selection.selected.y1 - selection.mouse_y);
	int dy2 = abs(selection.selected.y2 - selection.mouse_y);
	if(dx1 <= edge_threshold || dx2 <= edge_threshold) {
		x_dir = dx1 <= dx2 ? 1 : 2;
	}
	if(dy1 <= edge_threshold || dy2 <= edge_threshold) {
		y_dir = dy1 <= dy2 ? 1 : 2;
	}
	status = DRAG_STATUS_MOVE + y_dir * 3 + x_dir;
	goto done;

done:
	selection.preview_drag_status = status;
	gdk_window_set_cursor(overlay->gdk_window, selection.cursors[status]);
	return status;
}

static gboolean
event_mouse_move(GtkWidget *widget, GdkEventMotion *event) {
	int xmax = overlay->screenshot->width;
	int ymax = overlay->screenshot->height;
	int mouse_x = selection.mouse_x = clamp(event->x, 0, xmax - 1);
	int mouse_y = selection.mouse_y = clamp(event->y, 0, ymax - 1);

	update_suggested();

	int dx = mouse_x - selection.px;
	int dy = mouse_y - selection.py;

	if(selection.drag_status == DRAG_STATUS_NONE) {
		compute_drag_status();
	} else if(selection.drag_status == DRAG_STATUS_CREATE) {
		if((abs(dx) > selection.drag_threshold || abs(dy) > selection.drag_threshold) && !selection.has_selected) {
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
		unsigned x_dir = (selection.drag_status - DRAG_STATUS_MOVE) % 3;
		unsigned y_dir = (selection.drag_status - DRAG_STATUS_MOVE) / 3;
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
event_mouse_press_release(GtkWidget *widget, GdkEventButton *event) {
	int xmax = overlay->screenshot->width;
	int ymax = overlay->screenshot->height;
	int mouse_x = selection.mouse_x = clamp(event->x, 0, xmax - 1);
	int mouse_y = selection.mouse_y = clamp(event->y, 0, ymax - 1);

	if(event->button == 1) {
		if(event->type == GDK_BUTTON_PRESS) {
			selection.drag_status = compute_drag_status();
			selection.drag_threshold_reached = false;
			selection.px = mouse_x;
			selection.py = mouse_y;
			selection.prev_selected = selection.selected;
			if(selection.drag_status == DRAG_STATUS_CREATE && selection.has_suggested && selection.modifier_mask & MODIFIER_MASK_ADD_SELECTION) {
				if(selection.has_selected) {
					selection.selected = geom_union(&selection.selected, &selection.suggested);
				} else {
					selection.has_selected = true;
					selection.selected = selection.suggested;
				}
				selection.drag_status = DRAG_STATUS_NONE;
			}
		} else if(event->type == GDK_BUTTON_RELEASE) {
			if(selection.drag_status == DRAG_STATUS_CREATE && !selection.has_selected) {
				selection.has_selected = true;
				selection.selected = selection.suggested;
			}
			selection.drag_status = DRAG_STATUS_NONE;
			compute_drag_status();
		}
	}
	gtk_widget_queue_draw(selection.bgimage);
	return False;
}

static gboolean
event_key_press_release(GtkWidget *widget, GdkEventKey *event) {
	for(size_t i = 0; i < NUM_MODIFIER_KEY; ++i) {
		if(gdk_keyval_to_lower(event->keyval) == selection.modifier_codes[i]) {
			if(event->type == GDK_KEY_PRESS) {
				selection.modifier_mask |= 1 << i;
			} else if(event->type == GDK_KEY_RELEASE) {
				selection.modifier_mask &= ~(1 << i);
			}
		}
	}

	if(selection.drag_status == DRAG_STATUS_NONE) {
		update_suggested();
		compute_drag_status();
	}

	if(event->type == GDK_KEY_PRESS) {
		if(event->keyval == GDK_KEY_Escape) {
			if(selection.has_selected) {
				selection.has_selected = false;
			} else {
				exit(0);
			}
		} else if(event->keyval == GDK_KEY_Return) {
			struct rect bounds = {0};
			if(selection.has_selected) {
				bounds = selection.selected;
			} else {
				bounds.x2 = overlay->screenshot->width;
				bounds.y2 = overlay->screenshot->height;
			}
			overlay->writer->write_image(overlay->screenshot, &bounds);
			exit(0);
		}
	}
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
selection_post_init(void) {
	char *cursor_names[NUM_DRAG_STATUS] = {
			"default", "crosshair",
			"move", "w-resize", "e-resize",
			"n-resize", "nw-resize", "ne-resize",
			"s-resize", "sw-resize", "se-resize"
	};

	for(unsigned i = 0; i < NUM_DRAG_STATUS; ++i) {
		selection.cursors[i] = gdk_cursor_new_from_name(overlay->gdk_display, cursor_names[i]);
	}

	selection.modifier_codes[MODIFIER_KEY_ADD_SELECTION] = GDK_KEY_Shift_L;
	selection.modifier_codes[MODIFIER_KEY_SELECT_OUTPUT] = GDK_KEY_Control_L;
	selection.modifier_codes[MODIFIER_KEY_RESIZE_CORNER] = GDK_KEY_Alt_L;

	g_signal_connect(overlay->window, "button-press-event", G_CALLBACK(event_mouse_press_release), NULL);
	g_signal_connect(overlay->window, "button-release-event", G_CALLBACK(event_mouse_press_release), NULL);
	g_signal_connect(overlay->window, "motion-notify-event", G_CALLBACK(event_mouse_move), NULL);
	g_signal_connect(overlay->window, "key-press-event", G_CALLBACK(event_key_press_release), NULL);
	g_signal_connect(overlay->window, "key-release-event", G_CALLBACK(event_key_press_release), NULL);
	g_signal_connect(selection.bgimage, "draw", G_CALLBACK(draw_bg), NULL);
}