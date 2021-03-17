#include <cairo/cairo.h>
#include <gtk/gtk.h>

#include "ui.h"
#include "tooltip.h"
#include "selection.h"
#include "keybind.h"

struct ui ui;

static bool
warp(int dx, int dy) {
	GdkScreen *screen;
	gint x, y;
	gdk_display_get_pointer(ui.gdk_display, &screen, &x, &y, NULL);
	gdk_display_warp_pointer(ui.gdk_display, screen, x + dx, y + dy);
	return true;
}

static void
save_callback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
	GError *error = NULL;
	bool result = writer_save_image_finish(res, &error);
	printf("RESULT %d %p\n", result, error);
	keybind_done_and_continue(user_data, result);
}

static void
save_async(struct keybind_exec_context *context) {
	selection.modifier_mask = 0;
	struct rect bounds = selection_get_selection();

	writer_save_image_async(ui.memory_surface, &bounds, "output.png", "png", NULL, save_callback, context);
}

static void
save_dialog_callback(GtkWidget *dialog, gint response_id, gpointer data) {
	printf("dialog status: %d\n", response_id);
	gtk_widget_destroy(dialog);
	keybind_force_hide(false);
	if(response_id == GTK_RESPONSE_ACCEPT) {
		save_async(data);
	} else {
		keybind_done_and_continue(data, false);
	}
}

static void
save_as_async(struct keybind_exec_context *context) {
	GtkWidget *dialog = gtk_file_chooser_dialog_new("Test",
	                                                GTK_WINDOW(ui.window),
	                                                GTK_FILE_CHOOSER_ACTION_SAVE,
	                                                "_Cancel", GTK_RESPONSE_CANCEL,
	                                                "_Save", GTK_RESPONSE_ACCEPT,
	                                                NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
	gtk_widget_show(dialog);
	g_signal_connect(dialog, "response", G_CALLBACK(save_dialog_callback), context);
	gtk_widget_hide(ui.window);
}

static bool
copy() {
	struct rect bounds = selection_get_selection();
	GdkPixbuf *buf = gdk_pixbuf_get_from_surface(ui.memory_surface, bounds.x1, bounds.y1, bounds.x2 - bounds.x1, bounds.y2 - bounds.y1);
	gtk_clipboard_set_image(gtk_clipboard_get_default(ui.gdk_display), buf);
	return true;
}

static gboolean
event_key_press(GtkWidget *widget, GdkEventKey *event) {
	guint state = event->state & gtk_accelerator_get_default_mod_mask();
	guint keyval = gdk_keyval_to_lower(event->keyval);
	keybind_start(keyval, state);
	return False;
}

enum keybind_handle_status
keybind_handle(struct keybind *keybind, struct keybind_exec_context *context) {
	switch(keybind->action) {
		case ACTION_NOP:
			return KEYBIND_SUCCESS;
		case ACTION_SAVE:
			if(!context->allow_async) {
				return KEYBIND_ASYNC_REQUIRED;
			}
			save_async(context);
			return KEYBIND_ASYNC;
		case ACTION_SAVE_AS:
			if(!context->allow_async) {
				return KEYBIND_ASYNC_REQUIRED;
			}
			save_as_async(context);
			return KEYBIND_ASYNC;
		case ACTION_COPY:
			return copy();
		case ACTION_DESELECT:
			return selection_deselect();
		case ACTION_WARP_LEFT:
			return warp(-1, 0);
		case ACTION_WARP_RIGHT:
			return warp(1, 0);
		case ACTION_WARP_UP:
			return warp(0, -1);
		case ACTION_WARP_DOWN:
			return warp(0, 1);
	}
}

static gboolean
event(GtkWidget *widget, GdkEvent *event) {
	if(event->type == GDK_GRAB_BROKEN) {
		ui.grabbed = false;
	}
	if(!ui.grabbed) {
		if(gdk_window_is_visible(ui.gdk_window)) {
			ui.grabbed = gdk_seat_grab(ui.gdk_seat, ui.gdk_window, GDK_SEAT_CAPABILITY_KEYBOARD, TRUE, NULL, NULL, NULL, NULL) == GDK_GRAB_SUCCESS;
		}
	}
	return False;
}

extern double t();

void
overlay_init(struct screenshot *screenshot, struct window_tree *tree, struct output_list *outputs) {
	ui.screenshot = screenshot;
	ui.tree = tree;
	ui.outputs = outputs;

	int width = screenshot->width;
	int height = screenshot->height;

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

	ui.window = gtk_window_new(GTK_WINDOW_POPUP);
//	gtk_window_set_decorated(GTK_WINDOW(overlay.window), False);
	gtk_widget_set_size_request(ui.window, width, height);
//	gtk_window_move(GTK_WINDOW(overlay.window), 100, 100);
//	gtk_window_fullscreen(GTK_WINDOW(overlay.window));

	selection_init();
	tooltip_init();

	GtkWidget *overlay_container = gtk_overlay_new();
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay_container), selection.widget);
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay_container), tooltip.widget);

	gtk_container_add(GTK_CONTAINER(ui.window), overlay_container);
	gtk_widget_set_events(ui.window, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_SCROLL_MASK);
	gtk_widget_show_all(ui.window);

	//	Grab input
	GdkWindow *g_window = gtk_widget_get_window(ui.window);
	GdkDisplay *g_display = gdk_window_get_display(g_window);

	cairo_format_t fmt = CAIRO_FORMAT_ARGB32;
	double t1 = t();
	ui.memory_surface = cairo_image_surface_create_for_data(
			screenshot->buf, fmt, width, height,
			cairo_format_stride_for_width(fmt, width));
	double t2 = t();
	ui.drawing_surface = gdk_window_create_similar_surface(g_window, CAIRO_CONTENT_COLOR_ALPHA, ui.screenshot->width, ui.screenshot->height);
	cairo_t *cr = cairo_create(ui.drawing_surface);
	cairo_set_source_surface(cr, ui.memory_surface, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
	double t3 = t();
	GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(ui.memory_surface, 0, 0, ui.screenshot->width, ui.screenshot->height);
	double t4 = t();
	gtk_clipboard_set_image(gtk_clipboard_get_default(g_display), pixbuf);
	double t5 = t();

	printf("%.6f %.6f %.6f %.6f\n", t2-t1, t3-t2, t4-t3, t5-t4);

	ui.gdk_window = g_window;
	ui.gdk_display = g_display;

//	gdk_window_set_override_redirect(g_window, True);

	ui.gdk_seat = gdk_display_get_default_seat(g_display);

	selection_post_init();
	tooltip_post_init();

	g_signal_connect(ui.window, "key-press-event", G_CALLBACK(event_key_press), NULL);
	g_signal_connect(ui.window, "event", G_CALLBACK(event), NULL);

//	GtkWidget *dialog = gtk_file_chooser_dialog_new("Test", GTK_WINDOW(overlay.window), GTK_FILE_CHOOSER_ACTION_SAVE, NULL, NULL);
//	gtk_window_set_type_hint(GTK_WINDOW(dialog), GDK_WINDOW_TYPE_HINT_NOTIFICATION);
//	gtk_window_present(GTK_WINDOW(dialog));
//	GdkWindow *w = gtk_widget_get_window(dialog);
//	gtk_window_fullscreen_on_monitor(GTK_WINDOW(overlay.window), gdk_display_get_default_screen(g_display), 2);
//	gtk_dialog_run (GTK_DIALOG (dialog));

//	ui.grabbed = true;

	ui.grabbed = gdk_seat_grab(ui.gdk_seat, g_window, GDK_SEAT_CAPABILITY_KEYBOARD, TRUE, NULL, NULL, NULL, NULL) == GDK_GRAB_SUCCESS;
}
