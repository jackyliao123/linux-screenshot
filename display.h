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
	void (*get_screenshot)(void *display, struct screenshot *screenshot);
	struct window_tree *(*get_windows)(void *display);
//	void (*get_dimensions)(void *display, size_t dims[static 2]);
};

#endif
