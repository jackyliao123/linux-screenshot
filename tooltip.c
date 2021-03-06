#include <math.h>

#include "tooltip.h"
#include "overlay.h"

static struct tooltip tooltip;
static struct overlay *overlay;

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
    if(x >= 0 && y >= 0 && x < overlay->screenshot->width && y < overlay->screenshot->height) {
        uint32_t val = *((uint32_t *) overlay->screenshot->buf + (y * overlay->screenshot->width + x));
        clr.r = (val >> 0) & 0xff;
        clr.g = (val >> 8) & 0xff;
        clr.b = (val >> 16) & 0xff;
        clr.a = (val >> 24) & 0xff;
    }
    return clr;
}

static void
update_text() {
	char buf[1024];
	for(int ind = 0; ind < tooltip.label_cnt; ++ind) {
		buf[0] = '\0';
		int label_type = type[ind];
		GtkWidget *label = tooltip.labels[ind];
		char *fmt_str = format_str[label_type];
		struct rgba pixel = get_pixel(tooltip.mouse_x, tooltip.mouse_y);
		switch(label_type) {
			case INFO_POS:;
				snprintf(buf, sizeof(buf), fmt_str, tooltip.mouse_x, tooltip.mouse_y);
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
	guint width, height;
	cairo_matrix_t ctm;

	width = gtk_widget_get_allocated_width(widget);
	height = gtk_widget_get_allocated_height(widget);

	cairo_get_matrix(cr, &ctm);

	cairo_translate(cr, width / 2.0, height / 2.0);
	cairo_scale(cr, tooltip.zoom_amount, tooltip.zoom_amount);
	cairo_translate(cr, -0.5, -0.5);
	cairo_set_source_surface(cr, overlay->screenshot_surface, -tooltip.mouse_x, -tooltip.mouse_y);
	cairo_pattern_set_filter(cairo_get_source(cr), tooltip.zoom_amount < 1 ? CAIRO_FILTER_GOOD : CAIRO_FILTER_NEAREST);

	cairo_paint(cr);

	cairo_set_matrix(cr, &ctm);	

	cairo_set_line_width(cr, 1);
	cairo_set_source_rgba(cr, 0, 0, 0, 1);
	cairo_move_to(cr, width / 2, 0);
	cairo_line_to(cr, width / 2, height);
	cairo_move_to(cr, 0, height / 2);
	cairo_line_to(cr, width, height / 2);
	cairo_stroke(cr);

	return False;
}

static gboolean
event_scroll(GtkWidget *widget, GdkEventScroll *event) {
    double amount = tooltip.zoom_amount;
	amount /= powl(1.5, event->delta_y);
	if(amount > 200) {
		amount = 200;
	}
	if(amount < 1) {
		amount = 1;
	}
    tooltip.zoom_amount = amount;
	gtk_widget_queue_draw(tooltip.popup);
	return False;
}

static gboolean 
event_mouse_move(GtkWidget *widget, GdkEventMotion *event) {
	guint width, height;
	width = gtk_widget_get_allocated_width(tooltip.popup);
	height = gtk_widget_get_allocated_height(tooltip.popup);

    tooltip.mouse_x = event->x;
    tooltip.mouse_y = event->y;

	gint place_x = tooltip.mouse_x;
    gint place_y = tooltip.mouse_y;
	if(place_x + width + 40 > overlay->screenshot->width) {
		place_x -= width + 40;
	}
	if(place_y + height + 40 > overlay->screenshot->height) {
		place_y -= height + 40;
	}

	gtk_fixed_move(GTK_FIXED(tooltip.fixed), tooltip.popup, place_x + 20, place_y + 20);

	update_text();

	return False;
}

static void 
get_canvas_size(GtkWidget *widget, GtkAllocation *alloc, void *data) {
	gtk_widget_set_size_request(widget, alloc->width, alloc->width);
}

struct tooltip *
tooltip_init(struct overlay *o) {
    overlay = o;

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
	g_signal_connect(zoom, "size-allocate", G_CALLBACK(get_canvas_size), NULL);
	g_signal_connect(zoom, "draw", G_CALLBACK(draw_tooltip), NULL);

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
	gtk_fixed_put(GTK_FIXED(fixed), box, 50, 50);

	g_signal_connect(overlay->window, "scroll-event", G_CALLBACK(event_scroll), NULL);
	g_signal_connect(overlay->window, "motion-notify-event", G_CALLBACK(event_mouse_move), NULL);

	tooltip.widget = fixed;
    tooltip.fixed = fixed;
    tooltip.popup = box;
    tooltip.zoom_amount = 1;

    return &tooltip;
}

