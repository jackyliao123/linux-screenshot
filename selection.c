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
	if(selection.state != SELECTION_STATUS_NONE) {
		int x = MIN(selection.x1, selection.x2);
		int y = MIN(selection.y1, selection.y2);
		int width = ABS(selection.x1 - selection.x2) + 1;
		int height = ABS(selection.y1 - selection.y2) + 1;
		cairo_rectangle(cr, 0, 0, overlay->screenshot->width, overlay->screenshot->height);
		cairo_rectangle(cr, x, y + height, width, -height);
		cairo_clip(cr);
	}
	cairo_paint(cr);

	return False;
}

static gboolean
event_mouse_move(GtkWidget *widget, GdkEventMotion *event) {
    int mouse_x = event->x;
    int mouse_y = event->y;

	if(event->state & GDK_BUTTON1_MASK && selection.state == SELECTION_STATUS_TEMP) {
		selection.x1 = mouse_x;
		selection.y1 = mouse_y;
		selection.state = SELECTION_STATUS_CREATE;
	}

	if(selection.state == SELECTION_STATUS_CREATE) {
		selection.x2 = mouse_x;
		selection.y2 = mouse_y;
	}

	if(selection.state == SELECTION_STATUS_NONE || selection.state == SELECTION_STATUS_TEMP) {
		struct window_tree *window = window_tree_get_window_under(overlay->tree, mouse_x, mouse_y);
		if(window != NULL) {
			selection.state = SELECTION_STATUS_TEMP;
			selection.x1 = window->bounds.x;
			selection.y1 = window->bounds.y;
			selection.x2 = selection.x1 + window->bounds.width - 1;
			selection.y2 = selection.y1 + window->bounds.height - 1;
		} else {
			selection.state = SELECTION_STATUS_NONE;
		}
	}

	return False;
}

static gboolean
event_mouse_press(GtkWidget *widget, GdkEventButton *event) {
	if(selection.state == SELECTION_STATUS_NONE) {
		selection.x1 = event->x;
		selection.y1 = event->y;
		selection.state = SELECTION_STATUS_CREATE;
	}
	return False;
}

static gboolean
event_mouse_release(GtkWidget *widget, GdkEventButton *event) {
	if(selection.state == SELECTION_STATUS_CREATE) {
		selection.x2 = event->x;
		selection.y2 = event->y;
		selection.state = SELECTION_STATUS_AREA;
	} else if(selection.state == SELECTION_STATUS_TEMP) {
		selection.state = SELECTION_STATUS_AREA;
	}
	return False;
}


static gboolean
event_key_press(GtkWidget *widget, GdkEventKey *event) {
	printf("Key down\n");
	guint16 code = event->hardware_keycode;
	if(code == 9) {
		if(selection.state == SELECTION_STATUS_TEMP || selection.state == SELECTION_STATUS_NONE) {
			exit(0);
		} else {
			selection.state = SELECTION_STATUS_NONE;
		}
	}
	printf("%d\n", event->hardware_keycode);
	gtk_widget_queue_draw(selection.bgimage);
	return False;
}

struct selection *
selection_init(struct overlay *o) {
    overlay = o;

	selection.bgimage = gtk_drawing_area_new();

    g_signal_connect(overlay->window, "button-press-event", G_CALLBACK(event_mouse_press), NULL);
    g_signal_connect(overlay->window, "button-release-event", G_CALLBACK(event_mouse_release), NULL);
    g_signal_connect(overlay->window, "key-press-event", G_CALLBACK(event_key_press), NULL);
    g_signal_connect(overlay->window, "motion-notify-event", G_CALLBACK(event_mouse_move), NULL);
	g_signal_connect(selection.bgimage, "draw", G_CALLBACK(draw_bg), NULL);

	selection.widget = selection.bgimage;
    return &selection;
}
