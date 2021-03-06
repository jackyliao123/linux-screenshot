#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "display.h"
#include "overlay.h"

int main(int argc, char *argv[]) {
	struct display_X11 *display = display_X11_create();

	struct screenshot screenshot;

	display->impl->get_screenshot(display, &screenshot);
	struct window_tree *tree = display->impl->get_windows(display);

	struct overlay *overlay = overlay_init(&screenshot, tree);

	//gdk_pixbuf_save(buf, "scrot.png", "png", NULL, NULL);
	//g_object_unref(buf);
	return 0;
}
