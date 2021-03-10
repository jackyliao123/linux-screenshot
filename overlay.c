#include <cairo/cairo.h>
#include <gtk/gtk.h>

#include "overlay.h"
#include "tooltip.h"
#include "selection.h"

struct overlay overlay;

static gboolean
event_key_press(GtkWidget *widget, GdkEventKey *event) {
	int dx = 0;
	int dy = 0;
	switch(event->keyval) {
		case GDK_KEY_Left:
			dx--;
			break;
		case GDK_KEY_Right:
			dx++;
			break;
		case GDK_KEY_Up:
			dy--;
			break;
		case GDK_KEY_Down:
			dy++;
			break;
	}
	if(dx != 0 || dy != 0) {
		GdkScreen *screen;
		gint x, y;
		gdk_display_get_pointer(overlay.gdk_display, &screen, &x, &y, NULL);
		gdk_display_warp_pointer(overlay.gdk_display, screen, x + dx, y + dy);
	}
}

void
overlay_init(struct screenshot *screenshot, struct window_tree *tree, struct output_list *outputs, struct image_writer *writer) {
	overlay.screenshot = screenshot;
	overlay.tree = tree;
	overlay.outputs = outputs;
	overlay.writer = writer;

	int width = screenshot->width;
	int height = screenshot->height;

	cairo_format_t fmt = CAIRO_FORMAT_ARGB32;
	overlay.screenshot_surface = cairo_image_surface_create_for_data(
			screenshot->buf, fmt, width, height,
			cairo_format_stride_for_width(fmt, width));

//	overlay.window = gtk_window_new(GTK_WINDOW_POPUP);

//	GdkWindowAttr attr = {0};
//	attr.override_redirect = True;
//	attr.x = 0;
//	attr.y = 0;
//	attr.width = screenshot->width;
//	attr.height = screenshot->height;
//	attr.window_type = GDK_WINDOW_TOPLEVEL;
//	attr.title = "asdfasdf";
//	attr.event_mask = 0;
//
//	GdkWindow *gdk_window = gdk_window_new(NULL, &attr, GDK_WA_NOREDIR | GDK_WA_X | GDK_WA_Y);
//	gdk_window_show(gdk_window);
//	return &overlay;

	overlay.window = gtk_window_new(GTK_WINDOW_POPUP);
//	gtk_window_set_decorated(GTK_WINDOW(overlay.window), False);
	gtk_widget_set_size_request(overlay.window, width, height);
//	gtk_window_fullscreen(GTK_WINDOW(overlay.window));

	selection_init();
	tooltip_init();

	GtkWidget *overlay_container = gtk_overlay_new();
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay_container), gtk_image_new_from_surface(overlay.screenshot_surface));
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay_container), selection.widget);
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay_container), tooltip.widget);

	gtk_container_add(GTK_CONTAINER(overlay.window), overlay_container);
	gtk_widget_set_events(overlay.window, GDK_POINTER_MOTION_MASK);
	gtk_widget_show_all(overlay.window);

	//	Grab input
	GdkWindow *g_window = gtk_widget_get_window(overlay.window);
	GdkDisplay *g_display = gdk_window_get_display(g_window);

	overlay.gdk_window = g_window;
	overlay.gdk_display = g_display;

	gdk_window_set_override_redirect(g_window, True);

	GdkSeat *g_seat = gdk_display_get_default_seat(g_display);

	if (gdk_seat_grab(g_seat, g_window, GDK_SEAT_CAPABILITY_ALL, TRUE, NULL, NULL, NULL, NULL) != GDK_GRAB_SUCCESS) {
		fprintf(stderr, "Cursor grabbing failed\n");
		exit(-1);
	}

	selection_post_init();
	tooltip_post_init();

	g_signal_connect(overlay.window, "key-press-event", G_CALLBACK(event_key_press), NULL);
}
