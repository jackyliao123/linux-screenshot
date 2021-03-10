#include <math.h>

#include "tooltip.h"
#include "overlay.h"
#include "selection.h"

struct tooltip tooltip;

#define INFO_POS 0
#define INFO_CLR 1
#define INFO_HEX 2
#define INFO_RGB 3
#define INFO_HSL 4

static int *type = (int[]){INFO_POS, INFO_CLR, INFO_HEX, INFO_RGB, INFO_HSL};
static char **label_str = (char*[]){"POS", "CLR", "HEX", "RGB", "HSL"};
static char **format_str = (char*[]){"%5d %5d", "", "%06X", "%3u %3u %3u", "%3u %3u %3u"};

struct rgba {
    uint32_t r, g, b, a;
};

static struct rgba
get_pixel(int x, int y) {
    struct rgba clr = {};
    if(x >= 0 && y >= 0 && x < overlay.screenshot->width && y < overlay.screenshot->height) {
        uint32_t val = *((uint32_t *) overlay.screenshot->buf + (y * overlay.screenshot->width + x));
        clr.r = (val >> 16) & 0xff;
        clr.g = (val >> 8) & 0xff;
        clr.b = (val >> 0) & 0xff;
	    clr.a = 255;
    }
    return clr;
}

static void
update_text(void) {
	char buf[1024];
	for(int ind = 0; ind < tooltip.label_cnt; ++ind) {
		buf[0] = '\0';
		int label_type = type[ind];
		GtkWidget *label = tooltip.labels[ind];
		char *fmt_str = format_str[label_type];
		struct rgba pixel = get_pixel(tooltip.center_x, tooltip.center_y);
		switch(label_type) {
			case INFO_POS:;
				snprintf(buf, sizeof(buf), fmt_str, tooltip.center_x, tooltip.center_y);
				gtk_label_set_text(GTK_LABEL(label), buf);
				break;
			case INFO_CLR:;
				GdkRGBA clr = {pixel.r / 255.0, pixel.g / 255.0, pixel.b / 255.0, 1};
				gtk_widget_override_background_color(label, GTK_STATE_FLAG_NORMAL, &clr);
				break;
			case INFO_HEX:;
				snprintf(buf, sizeof(buf), fmt_str, pixel.r << 16 | pixel.g << 8 | pixel.b);
				gtk_label_set_text(GTK_LABEL(label), buf);
				break;
			case INFO_RGB:;
				snprintf(buf, sizeof(buf), fmt_str, pixel.r, pixel.g, pixel.b);
				gtk_label_set_text(GTK_LABEL(label), buf);
				break;
			case INFO_HSL:;
				break;
			default:;
				break;
		}
	}
}

static gint
draw_tooltip(GtkWidget *widget, cairo_t *cr, gpointer data) {
	double hwidth = gtk_widget_get_allocated_width(widget) / 2.0;
	double hheight = gtk_widget_get_allocated_height(widget) / 2.0;

	cairo_matrix_t ctm;
	cairo_get_matrix(cr, &ctm);

	int xmax = overlay.screenshot->width;
	int ymax = overlay.screenshot->height;

	int dx = (int) ceil(hwidth / tooltip.zoom_amount);
	int dy = (int) ceil(hheight / tooltip.zoom_amount);

	int x1 = clamp(tooltip.center_x - dx, 0, xmax);
	int y1 = clamp(tooltip.center_y - dy, 0, ymax);
	int x2 = clamp(tooltip.center_x + dx + 1, 0, xmax);
	int y2 = clamp(tooltip.center_y + dy + 1, 0, ymax);

	int offset_x = tooltip.center_x - dx - x1;
	int offset_y = tooltip.center_y - dy - y1;

	cairo_surface_t *subsurface = cairo_surface_create_for_rectangle(overlay.screenshot_surface,
																  x1, y1, x2 - x1, y2 - y1);

	cairo_scale(cr, tooltip.zoom_amount, tooltip.zoom_amount);
	cairo_set_source_surface(cr, subsurface,
						  hwidth / tooltip.zoom_amount - dx - 0.5 - offset_x,
						  hheight / tooltip.zoom_amount - dy - 0.5 - offset_y);
	cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
	cairo_paint(cr);

	cairo_set_matrix(cr, &ctm);

	cairo_surface_destroy(subsurface);

	if(selection.has_selected) {
		struct rect bounds = selection.selected;

		cairo_set_line_width(cr, 1);

		int bx1 = (int) round((bounds.x1 - tooltip.center_x - 0.5) * tooltip.zoom_amount + hwidth);
		int by1 = (int) round((bounds.y1 - tooltip.center_y - 0.5) * tooltip.zoom_amount + hheight);
		int bx2 = (int) round((bounds.x2 - tooltip.center_x - 0.5) * tooltip.zoom_amount + hwidth) + 1;
		int by2 = (int) round((bounds.y2 - tooltip.center_y - 0.5) * tooltip.zoom_amount + hheight) + 1;

		cairo_set_source_rgba(cr, 1, 1, 1, 1);
		cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);
		cairo_rectangle(cr, bx1 - 0.5, by1 - 0.5, bx2 - bx1, by2 - by1);
		cairo_stroke(cr);
		cairo_set_source_rgba(cr, 1, 1, 0, 0.3);
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		cairo_rectangle(cr, bx1 - 0.5, by1 - 0.5, bx2 - bx1, by2 - by1);
		cairo_stroke(cr);
	}

	cairo_set_line_width(cr, 1);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_move_to(cr, (int) (hwidth) + 0.5, 0);
	cairo_line_to(cr, (int) (hwidth) + 0.5, hheight * 2);
	cairo_move_to(cr, 0, (int) (hheight) + 0.5);
	cairo_line_to(cr, hwidth * 2, (int) (hheight) + 0.5);
	cairo_stroke(cr);

	return False;
}

static gboolean
event_scroll(GtkWidget *widget, GdkEventScroll *event) {
    double amount = tooltip.zoom_amount;
	amount /= pow(2, event->delta_y);
	if(amount > 256) {
		amount = 256;
	}
	if(amount < 1) {
		amount = 1;
	}
    tooltip.zoom_amount = amount;
	gtk_widget_queue_draw(tooltip.popup);
	return False;
}

static void
update_mouse_position(int x, int y) {
	int width = gtk_widget_get_allocated_width(tooltip.popup);
	int height = gtk_widget_get_allocated_height(tooltip.popup);

	tooltip.center_x = x;
	tooltip.center_y = y;

	if(selection.drag_status > DRAG_STATUS_MOVE) {
		unsigned xdir = (selection.drag_status - DRAG_STATUS_MOVE) % 3;
		unsigned ydir = (selection.drag_status - DRAG_STATUS_MOVE) / 3;
		if(xdir == 1) {
			tooltip.center_x = selection.selected.x1;
		} else if(xdir == 2){
			tooltip.center_x = selection.selected.x2 - 1;
		}
		if(ydir == 1) {
			tooltip.center_y = selection.selected.y1;
		} else if(ydir == 2){
			tooltip.center_y = selection.selected.y2 - 1;
		}
	}

	struct rect bounds = {0};
	struct output *output = geom_get_output_under(overlay.outputs, x, y);
	if(output) {
		bounds = output->bounds;
	} else {
		bounds.x2 = overlay.screenshot->width;
		bounds.y2 = overlay.screenshot->height;
	}

	int place_x = x;
	int place_y = y;
	if(place_x + width + 40 > bounds.x2) {
		place_x -= width + 40;
	}
	if(place_y + height + 40 > bounds.y2) {
		place_y -= height + 40;
	}

	gtk_fixed_move(GTK_FIXED(tooltip.fixed), tooltip.popup, place_x + 20, place_y + 20);

	update_text();
}

static gboolean
event_mouse_move(GtkWidget *widget, GdkEventMotion *event) {
	update_mouse_position(event->x, event->y);
	return False;
}

static gboolean
event_mouse_press_release(GtkWidget *widget, GdkEventButton *event) {
	update_mouse_position(event->x, event->y);
	return False;
}

static void
get_canvas_size(GtkWidget *widget, GtkAllocation *alloc, void *data) {
	gtk_widget_set_size_request(widget, alloc->width, alloc->width);
}

void
tooltip_init() {
    // TODO maybe configurable?
    tooltip.label_cnt = 4;
    tooltip.labels = calloc(tooltip.label_cnt, sizeof(GtkLabel *));

	PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 11");

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

	GdkRGBA bg_clr = {0, 0, 0, 0.8};
	GdkRGBA fg_clr = {1, 1, 1, 1};

	gtk_widget_override_background_color(box, GTK_STATE_FLAG_NORMAL, &bg_clr);

	/* displays zoomed in image */
	GtkWidget *zoom = gtk_drawing_area_new();
	gtk_widget_set_margin_top(zoom, 10);
	gtk_widget_set_margin_start(zoom, 10);
	gtk_widget_set_margin_end(zoom, 10);

	gtk_box_pack_start(GTK_BOX(box), zoom, TRUE, TRUE, 0);

	/* displays colour info labels */
	GtkWidget *grid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(grid), 2);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
	gtk_widget_set_margin_bottom(grid, 8);
	gtk_widget_set_margin_top(grid, 8);
	gtk_widget_set_margin_start(grid, 10);
	gtk_widget_set_margin_end(grid, 10);

	for(int ind = 0; ind < tooltip.label_cnt; ++ind) {
		int label_type = type[ind];

		GtkWidget *label = gtk_label_new(*(label_str + label_type));
		gtk_widget_override_font(label, font_desc);
		gtk_widget_override_color(label, GTK_STATE_FLAG_NORMAL, &fg_clr);
		gtk_grid_attach(GTK_GRID(grid), label, 0, ind, 1, 1);

		GtkWidget *value = gtk_label_new(NULL);
		gtk_widget_override_font(value, font_desc);
		gtk_widget_override_color(value, GTK_STATE_FLAG_NORMAL, &fg_clr);
		gtk_grid_attach(GTK_GRID(grid), value, 1, ind, 1, 1);

		tooltip.labels[ind] = value;
	}

	gtk_box_pack_start(GTK_BOX(box), grid, TRUE, TRUE, 0);

	GtkWidget *fixed = gtk_fixed_new();
	gtk_fixed_put(GTK_FIXED(fixed), box, 0, 0);

	tooltip.widget = fixed;
    tooltip.fixed = fixed;
    tooltip.popup = box;
    tooltip.zoom_amount = 2;
    tooltip.zoom = zoom;
}

void
tooltip_post_init(void) {
	g_signal_connect(tooltip.zoom, "size-allocate", G_CALLBACK(get_canvas_size), NULL);
	g_signal_connect(tooltip.zoom, "draw", G_CALLBACK(draw_tooltip), NULL);
	g_signal_connect(overlay.window, "scroll-event", G_CALLBACK(event_scroll), NULL);
	g_signal_connect(overlay.window, "motion-notify-event", G_CALLBACK(event_mouse_move), NULL);
	g_signal_connect(overlay.window, "button-press-event", G_CALLBACK(event_mouse_press_release), NULL);
	g_signal_connect(overlay.window, "button-release-event", G_CALLBACK(event_mouse_press_release), NULL);
}

