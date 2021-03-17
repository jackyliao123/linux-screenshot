#include <gtk/gtk.h>
#include <string.h>
#include <pthread.h>

#include "display.h"
#include "ui.h"
#include "writer.h"

#include "time.h"

double t() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e6;
}

int main(int argc, char *argv[]) {
	double tt = t();
	struct display_X11 *display = display_X11_create();

//	printf("Init %.9f\n", t() - tt); tt = t();

	struct output_list outputs;

	struct screenshot screenshot = {0};
	display->impl->get_screenshot(&screenshot);
//	printf("Screenshot %.9f\n", t() - tt); tt = t();
	struct window_tree *tree = display->impl->get_windows();
//	printf("Window Tree %.9f\n", t() - tt); tt = t();
	display->impl->get_outputs(&outputs);
//	printf("Outputs %.9f\n", t() - tt); tt = t();

//	struct screenshot screenshot = {
//			.width = 1920,
//			.height = 1080,
//			.buf = malloc(1920*1080*4)
//	};
//	for(int i = 0; i < 1920*1080; ++i) {
//		int x = i % 1920;
//		int y = i / 1920;
//		((uint32_t *) screenshot.buf)[i] = 0xFF000000 | x % 256 | (y & 1) << 15 | (x & 1) << 23;
//	}
//	struct window_tree *tree = NULL;

	gtk_init(NULL, NULL);
//	printf("GtkInit %.9f\n", t() - tt); tt = t();

	struct keybind keybind[] = {
			{
				.accelerator = "<Ctrl>s",
				.action = ACTION_SAVE,
			},
			{
				.accelerator = "<Ctrl><Shift>s",
				.action = ACTION_SAVE_AS,
//				.exit = true
			},
			{
				.accelerator = "Return",
				.action = ACTION_SAVE,
				.exit = true
			},
			{
				.accelerator = "<Ctrl>c",
				.action = ACTION_COPY,
			},
			{
				.accelerator = "<Ctrl>x",
				.action = ACTION_COPY,
				.exit = true
			},
			{
				.accelerator = "Escape",
				.action = ACTION_DESELECT,
				.consume = true,
			},
			{
				.accelerator = "Escape",
				.action = ACTION_NOP,
				.exit = true,
			},
			{
				.accelerator = "Right",
				.action = ACTION_WARP_RIGHT,
			},
			{
				.accelerator = "Left",
				.action = ACTION_WARP_LEFT,
			},
			{
				.accelerator = "Up",
				.action = ACTION_WARP_UP,
			},
			{
				.accelerator = "Down",
				.action = ACTION_WARP_DOWN,
			},
	};

	struct keybind_list keybinds = {
			.num_keybind = sizeof(keybind) / sizeof(struct keybind),
			.keybinds = keybind,
	};

	keybinds_init(&keybinds, 5);

	overlay_init(&screenshot, tree, &outputs);
//	printf("overlay_init %.9f\n", t() - tt); tt = t();

	gtk_main();

	//gdk_pixbuf_save(buf, "scrot.png", "png", NULL, NULL);
	//g_object_unref(buf);
	return 0;
}
