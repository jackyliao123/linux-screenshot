#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>
#include <gtk/gtk.h>

#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>
#include <math.h>

static Display *x_display;
static Window x_root;

static int label_cnt = 4;
static GtkWidget *labels[64];

#define INFO_POS 0
#define INFO_CLR 1
#define INFO_HEX 2
#define INFO_RGB 3
#define INFO_HSL 4

static int *type = (int[]){INFO_POS, INFO_CLR, INFO_HEX, INFO_RGB, INFO_HSL};
static char **label_str = (char*[]){"POS", "CLR", "HEX", "RGB", "HSL"};
static char **format_str = (char*[]){"%5u %5u", "", "%06X", "%3u %3u %3u", "%3u %3u %3u"};

struct x_window {
	Window window;
	XWindowAttributes attr;	
	int x, y, width, height;
	unsigned int n_children;
	struct x_window *children;
};

struct x_window root_window_tree;

struct rgba {
	unsigned char b, g, r, a;
};

#define SELECT_NONE 0
#define SELECT_TEMP 1
#define SELECT_AREA 2
#define SELECT_CREATE 3

struct {
	int x1, y1, x2, y2, type;
} selection;

double zoom_amount = 2;

struct {
	int width;
	int height;
} display;

Screen *display_screen;

struct {
	int x, y;	
} mouse;

cairo_surface_t *scrot_surface;
unsigned char *scrot_buf;
GtkWidget *tooltip_fixed;
GtkWidget *tooltip;
GtkWidget *bgimage;

void xorg_init() {
	x_display = XOpenDisplay(NULL);
	x_root = DefaultRootWindow(x_display);
	XWindowAttributes attr;
	XGetWindowAttributes(x_display, x_root, &attr);
	display.width = attr.width;
	display.height = attr.height;
	display_screen = attr.screen;
}

void calc_geometry(struct x_window *window) {
	Window child;
	XTranslateCoordinates(x_display, window->window, x_root, -window->attr.border_width, -window->attr.border_width, &window->x, &window->y, &child);
	window->width = window->attr.width;
	window->height = window->attr.height;
}

void list_windows(Window window, struct x_window *fill) {
	Window root;
	Window parent;
	Window *children;
	Window child;
	unsigned int nchildren;

	int x1, y1, x2, y2;

	XGetWindowAttributes(x_display, window, &fill->attr);

	calc_geometry(fill);

	XQueryTree(x_display, window, &root, &parent, &children, &nchildren);
	if(children == NULL) {
		fill->n_children = 0;
		fill->children = NULL;
		return;
	}
	fill->n_children = nchildren;
	fill->children = malloc(sizeof(struct x_window) * nchildren);
	if(fill->children == NULL) {
		perror("malloc");
		return;
	}
	for(unsigned int i = 0; i < nchildren; ++i) {
		Window wind = children[i];
		struct x_window *child = &fill->children[i];
		child->window = wind;
		list_windows(wind, &fill->children[i]);
	}
	XFree(children);
}

void get_window_tree() {
	root_window_tree.window = x_root;
	XGetWindowAttributes(x_display, x_root, &root_window_tree.attr);
	list_windows(x_root, &root_window_tree);
}

Bool contains(struct x_window *window, int x, int y) {
	if(window == NULL) {
		return False;
	}
	if(window->attr.map_state == IsViewable) {
		return window->x <= x && window->x + window->width > x && window->y <= y && window->y + window->height > y;
	}
	return False;
}

struct x_window *get_window_under(struct x_window *tree, int x, int y) {
	if(!contains(tree, x, y)) {
		return NULL;
	}
	if(tree->children == NULL) {
		return tree;
	}
	for(unsigned int i = 0; i < tree->n_children; ++i) {
		struct x_window *under = get_window_under(&tree->children[tree->n_children - 1 - i], x, y);
		if(under != NULL) {
			return under;
		}
	}
	return tree;
}

unsigned char *screenshot_xshm() {
	XShmSegmentInfo shminfo;
	XImage *img;
	unsigned char *data_cpy = NULL;
	unsigned int size;

	img = XShmCreateImage(x_display, DefaultVisualOfScreen(display_screen), DefaultDepthOfScreen(display_screen), ZPixmap, NULL, &shminfo, display.width, display.height);

	size = img->bytes_per_line * img->height;

	shminfo.shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
	if(shminfo.shmid < 0) {
		perror("shmget");
		XDestroyImage(img);
		return NULL;
	}
	shminfo.shmaddr = img->data = (char *)shmat(shminfo.shmid, 0, 0);
	shminfo.readOnly = False;
	if(!XShmAttach(x_display, &shminfo)) {
		fprintf(stderr, "XShmAttach failed");
		goto img_destroy;
	}

	XShmGetImage(x_display, x_root, img, 0, 0, AllPlanes);
	
	data_cpy = malloc(size);
	if(data_cpy == NULL) {
		perror("malloc");
		goto shm_detatch;
	}
	memcpy(data_cpy, img->data, size);

shm_detatch:
	XShmDetach(x_display, &shminfo);
img_destroy:
	XDestroyImage(img);
	shmdt(shminfo.shmaddr);
	shmctl(shminfo.shmid, IPC_RMID, NULL);

	return data_cpy;
}

unsigned char *screenshot_xget() {
	unsigned int size;
	unsigned char *data_cpy = NULL;
	XImage *img = XGetImage(x_display, x_root, 0, 0, display.width, display.height, AllPlanes, ZPixmap);
	size = img->bytes_per_line * img->height;

	data_cpy = malloc(size);
	if(data_cpy == NULL) {
		perror("malloc");
		XDestroyImage(img);
		return NULL;
	}

	memcpy(data_cpy, img->data, size);

	XDestroyImage(img);

	return data_cpy;
}

void update_text() {
	char char_buf[1024];
	for(int ind = 0; ind < label_cnt; ++ind) {
		char_buf[0] = '\0';
		int label_type = type[ind];
		GtkWidget *widget = labels[ind];
		char *fmt_str = format_str[label_type];
		struct rgba pixel = ((struct rgba *)scrot_buf)[mouse.y * display.width + mouse.x];
		switch(label_type) {
			case INFO_POS:;
				snprintf(char_buf, 1024, fmt_str, mouse.x, mouse.y);
				gtk_label_set_text(GTK_LABEL(widget), char_buf);
				break;
			case INFO_CLR:;
				GdkRGBA clr = {pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0, 1};
				gtk_widget_override_background_color(widget, GTK_STATE_FLAG_NORMAL, &clr);
				break;
			case INFO_HEX:;
				snprintf(char_buf, 1024, fmt_str, pixel.r << 16 | pixel.g << 8 | pixel.b);
				gtk_label_set_text(GTK_LABEL(widget), char_buf);
				break;
			case INFO_RGB:;
				snprintf(char_buf, 1024, fmt_str, pixel.r, pixel.g, pixel.b);
				gtk_label_set_text(GTK_LABEL(widget), char_buf);
				break;
			case INFO_HSL:
				break;
		}
	}
}

gint draw_tooltip(GtkWidget *widget, cairo_t *cr, gpointer data) {
	guint width, height;
	cairo_matrix_t ctm;

	width = gtk_widget_get_allocated_width(widget);
	height = gtk_widget_get_allocated_height(widget);

	cairo_get_matrix(cr, &ctm);

	cairo_translate(cr, width / 2.0, height / 2.0);
	cairo_scale(cr, zoom_amount, zoom_amount);
	cairo_translate(cr, -0.5, -0.5);
	cairo_set_source_surface(cr, scrot_surface, -mouse.x, -mouse.y);
	cairo_pattern_set_filter(cairo_get_source(cr), zoom_amount < 1 ? CAIRO_FILTER_GOOD : CAIRO_FILTER_NEAREST);

	cairo_paint(cr);

	cairo_set_matrix(cr, &ctm);	

	cairo_set_line_width(cr, 1);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_move_to(cr, width / 2, 0);
	cairo_line_to(cr, width / 2, height);
	cairo_move_to(cr, 0, height / 2);
	cairo_line_to(cr, width, height / 2);
	cairo_stroke(cr);

	return FALSE;
}

gint draw_bg(GtkWidget *widget, cairo_t *cr, gpointer data) {
	guint width, height;

	width = gtk_widget_get_allocated_width(widget);
	height = gtk_widget_get_allocated_height(widget);

	cairo_set_source_surface(cr, scrot_surface, 0, 0);
	cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
	cairo_paint(cr);

	cairo_set_source_rgba(cr, 0, 0, 0, 0.6);
	if(selection.type != SELECT_NONE) {
		int x = MIN(selection.x1, selection.x2);
		int y = MIN(selection.y1, selection.y2);
		int width = ABS(selection.x1 - selection.x2) + 1;
		int height = ABS(selection.y1 - selection.y2) + 1;
		cairo_rectangle(cr, 0, 0, display.width, display.height);
		cairo_rectangle(cr, x, y + height, width, -height);
		cairo_clip(cr);
	}
	cairo_paint(cr);

	return FALSE;
}

gboolean event_key_press(GtkWidget *widget, GdkEventKey *event) {
	printf("Key down\n");
	guint16 code = event->hardware_keycode;
	if(code == 9) {
		if(selection.type == SELECT_TEMP || selection.type == SELECT_NONE) {
			exit(0);
		} else {
			selection.type = SELECT_NONE;
		}
	}
	printf("%d\n", event->hardware_keycode);
	gtk_widget_queue_draw(bgimage);
	return False;
}

gboolean event_mouse_press(GtkWidget *widget, GdkEventButton *event) {
	if(selection.type == SELECT_NONE) {
		selection.x1 = event->x;
		selection.y1 = event->y;
		selection.type = SELECT_CREATE;
	}
	return False;
}

gboolean event_mouse_release(GtkWidget *widget, GdkEventButton *event) {
	if(selection.type == SELECT_CREATE) {
		selection.x2 = event->x;
		selection.y2 = event->y;
		selection.type = SELECT_AREA;
	} else if(selection.type == SELECT_TEMP) {
		selection.type = SELECT_AREA;
	}
	return False;
}

//struct timespec last_mouse_move;

gboolean event_mouse_move(GtkWidget *widget, GdkEventMotion *event) {
//	struct timespec current_time;
//	clock_gettime(CLOCK_MONOTONIC, &current_time);
//	if((current_time.tv_nsec - last_mouse_move.tv_nsec) + (current_time.tv_sec - last_mouse_move.tv_sec) * 1000000000 < 30000000) {
//		return False;
//	}
//	last_mouse_move = current_time;
//	gtk_widget_queue_draw(bgimage);


	guint width, height;
	width = gtk_widget_get_allocated_width(tooltip);
	height = gtk_widget_get_allocated_height(tooltip);
	printf("Mouse event\n");
	if(event->state & GDK_BUTTON1_MASK && selection.type == SELECT_TEMP) {
		selection.x1 = mouse.x;
		selection.y1 = mouse.y;
		selection.type = SELECT_CREATE;
	}
	mouse.x = event->x;
	mouse.y = event->y;
	gint place_x = mouse.x, place_y = mouse.y;
	if(place_x + width + 40 > display.width) {
		place_x -= width + 40;
	}
	if(place_y + height + 40 > display.height) {
		place_y -= height + 40;
	}
	gtk_fixed_move(GTK_FIXED(tooltip_fixed), tooltip, place_x + 20, place_y + 20);
	update_text();

	if(selection.type == SELECT_CREATE) {
		selection.x2 = mouse.x;
		selection.y2 = mouse.y;
	}

	if(selection.type == SELECT_NONE || selection.type == SELECT_TEMP) {
		struct x_window *window = get_window_under(&root_window_tree, mouse.x, mouse.y);
		if(window != NULL) {
			selection.type = SELECT_TEMP;
			selection.x1 = window->x;
			selection.y1 = window->y;
			selection.x2 = selection.x1 + window->width - 1;
			selection.y2 = selection.y1 + window->height - 1;
		} else {
			selection.type = SELECT_NONE;
		}
	}

	return False;
}

gboolean event_scroll(GtkWidget *widget, GdkEventScroll *event) {
	zoom_amount /= powl(1.5, event->delta_y);
	if(zoom_amount > 200) {
		zoom_amount = 200;
	}
	if(zoom_amount < 1) {
		zoom_amount = 1;
	}
	gtk_widget_queue_draw(tooltip);
	return False;
}

void get_canvas_size(GtkWidget *widget, GtkAllocation *alloc, void *data) {
	gtk_widget_set_size_request(widget, alloc->width, alloc->width);
}

int main(int argc, char *argv[]) {
	GtkWidget *window;

	GtkWidget *overlay;

	GdkWindow *g_window;
	GdkDisplay *g_display;
	GdkSeat *g_seat;

	xorg_init();
	printf("Getting window tree\n");
	get_window_tree();
	printf("Window tree obtained\n");

	scrot_buf = screenshot_xshm();

	gtk_init(&argc, &argv);
	cairo_format_t fmt = CAIRO_FORMAT_RGB24;
	scrot_surface = cairo_image_surface_create_for_data(scrot_buf, fmt, display.width, display.height, cairo_format_stride_for_width(fmt, display.width));

	PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 11");

	window = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_set_size_request(window, display.width, display.height);

	bgimage = gtk_drawing_area_new();
	g_signal_connect(bgimage, "draw", G_CALLBACK(draw_bg), NULL);

	overlay = gtk_overlay_new();

//	Create tooltip

	tooltip = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

	GdkRGBA bg_clr = {0, 0, 0, 0.8};
	GdkRGBA fg_clr = {1, 1, 1, 1};

	gtk_widget_override_background_color(tooltip, GTK_STATE_FLAG_NORMAL, &bg_clr);

	GtkWidget *zoom = gtk_drawing_area_new();
	gtk_widget_set_margin_top(zoom, 10);
	gtk_widget_set_margin_start(zoom, 10);
	gtk_widget_set_margin_end(zoom, 10);
	g_signal_connect(zoom, "size-allocate", G_CALLBACK(get_canvas_size), NULL);
	g_signal_connect(zoom, "draw", G_CALLBACK(draw_tooltip), NULL);

	gtk_box_pack_start (GTK_BOX (tooltip), zoom, TRUE, TRUE, 0);

	GtkWidget *info = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(info), 2);
	gtk_grid_set_column_spacing(GTK_GRID(info), 10);
	gtk_widget_set_margin_bottom(info, 8);
	gtk_widget_set_margin_top(info, 8);
	gtk_widget_set_margin_start(info, 10);
	gtk_widget_set_margin_end(info, 10);

	for(int ind = 0; ind < label_cnt; ++ind) {
		int label_type = type[ind];
		GtkWidget *label = gtk_label_new(*(label_str + label_type));
		gtk_widget_override_font(label, font_desc);
		gtk_widget_override_color(label, GTK_STATE_FLAG_NORMAL, &fg_clr);
		gtk_grid_attach(GTK_GRID(info), label, 0, ind, 1, 1);
		GtkWidget *value = gtk_label_new(NULL);
		gtk_widget_override_font(value, font_desc);
		gtk_widget_override_color(value, GTK_STATE_FLAG_NORMAL, &fg_clr);
		gtk_grid_attach(GTK_GRID(info), value, 1, ind, 1, 1);
		labels[ind] = value;
	}


	gtk_box_pack_start (GTK_BOX (tooltip), info, TRUE, TRUE, 0);

	tooltip_fixed = gtk_fixed_new();

	gtk_fixed_put(GTK_FIXED(tooltip_fixed), tooltip, 50, 50);


	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), bgimage);
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), tooltip_fixed);

	gtk_container_add(GTK_CONTAINER(window), overlay);

	g_signal_connect(window, "key-press-event", G_CALLBACK(event_key_press), NULL);
	g_signal_connect(window, "button-press-event", G_CALLBACK(event_mouse_press), NULL);
	g_signal_connect(window, "button-release-event", G_CALLBACK(event_mouse_release), NULL);
	g_signal_connect(window, "motion-notify-event", G_CALLBACK(event_mouse_move), NULL);
	g_signal_connect(window, "scroll-event", G_CALLBACK(event_scroll), NULL);

	gtk_widget_set_events(window, GDK_POINTER_MOTION_MASK);

	gtk_widget_show_all(window);

//	Grab input	
	g_window = gtk_widget_get_window(window);

	g_display = gdk_window_get_display(g_window);

	gdk_window_set_cursor(g_window, gdk_cursor_new_from_name(g_display, "crosshair"));

	g_seat = gdk_display_get_default_seat(g_display);

	if(gdk_seat_grab(g_seat, g_window, GDK_SEAT_CAPABILITY_ALL, True, NULL, NULL, NULL, NULL) != GDK_GRAB_SUCCESS) {
		fprintf(stderr, "Cursor grabbing failed\n");
		exit(-1);
	}

	gtk_main();

	//gdk_pixbuf_save(buf, "scrot.png", "png", NULL, NULL);
	//g_object_unref(buf);
	return 0;
}
