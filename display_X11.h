#ifndef DISPLAY_X11_H
#define DISPLAY_X11_H

#include <X11/Xlib.h>

#include "display.h"

struct display_X11 {
	Display *display;
	Window root;
};

extern struct display_X11 display_X11;

const struct display_impl *display_X11_init(void);

#endif