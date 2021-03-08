#ifndef DISPLAY_H
#define DISPLAY_H

#include <unistd.h>

#include "display_X11.h"
#include "geometry.h"

struct screenshot {
	void *buf; // What is this format? RGBA8888?
	size_t width, height;
};

struct display_impl {
	void (*get_screenshot)(struct screenshot *screenshot);
	struct window_tree *(*get_windows)();
	void (*get_outputs)(struct output_list *outputs);
};

#endif
