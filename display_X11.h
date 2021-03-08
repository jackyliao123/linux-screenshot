#ifndef DISPLAY_X11_H
#define DISPLAY_X11_H

#include <X11/Xlib.h>

struct display_impl;

struct display_X11 {
    const struct display_impl *impl;

	Display *display;
	Window root;
};

struct display_X11 *display_X11_create(void);

#endif