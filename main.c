#include <gtk/gtk.h>
#include <string.h>

#include "display.h"
#include "overlay.h"
#include "writer.h"

int main(int argc, char *argv[]) {
	struct display_X11 *display = display_X11_create();

	struct screenshot screenshot = {0};
	struct output_list outputs;

	display->impl->get_screenshot(&screenshot);
	struct window_tree *tree = display->impl->get_windows();
	display->impl->get_outputs(&outputs);

	struct image_writer *writer = new_image_writer_file();

//	struct screenshot screenshot = {
//			.width = 1920,
//			.height = 1080,
//			.buf = malloc(1920*1080*4)
//	};
//	struct window_tree *tree = NULL;

	gtk_init(NULL, NULL);

	struct overlay *overlay = overlay_init(&screenshot, tree, &outputs, writer);

	gtk_main();

	//gdk_pixbuf_save(buf, "scrot.png", "png", NULL, NULL);
	//g_object_unref(buf);
	return 0;
}
