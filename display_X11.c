#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>

#include "display.h"
#include "display_X11.h"

static void
fill_node_from_window(Display *display, Window window, struct window_tree *node) {
	Window root;
	Window parent;
	Window *children;
	Window child;
	unsigned int nchildren;

	XWindowAttributes attrs;
	XGetWindowAttributes(display, window, &attrs);

	XTranslateCoordinates(display, window, root, -attrs.border_width,
			-attrs.border_width, &node->bounds.x, &node->bounds.y, &child);
	node->bounds.width = attrs.width;
	node->bounds.height = attrs.height;

	XQueryTree(display, window, &root, &parent, &children, &nchildren);
	if (children == NULL) {
		node->num_children = 0;
		node->children = NULL;
		return;
	}
	node->num_children = nchildren;
	node->children = calloc(sizeof(struct window_tree), nchildren);
	if (node->children == NULL) {
		perror("malloc");
		exit(1);
	}

	for (unsigned int i = 0; i < nchildren; ++i) {
		Window child_win = children[i];
		struct window_tree *child = &node->children[i];
		fill_node_from_window(display, child_win, child);
	}

	XFree(children);
}

static void
get_screenshot(void *display_generic, struct screenshot *screenshot) {
	struct display_X11 *display = display_generic;

	XShmSegmentInfo shminfo;
	XImage *img;

	XWindowAttributes attrs;
	XGetWindowAttributes(display->display, display->root, &attrs);

	img = XShmCreateImage(display->display, attrs.visual, attrs.depth, ZPixmap,
			NULL, &shminfo, attrs.width, attrs.height);

	size_t size = img->bytes_per_line * img->height;

	shminfo.shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
	if (shminfo.shmid < 0) {
		perror("shmget");
		XDestroyImage(img);
		exit(1);
	}
	shminfo.shmaddr = img->data = (char *) shmat(shminfo.shmid, 0, 0);
	shminfo.readOnly = False;
	if (!XShmAttach(display->display, &shminfo)) {
		perror("XShmAttach");
		goto img_destroy;
	}

	XShmGetImage(display->display, display->root, img, 0, 0, AllPlanes);
	
	unsigned char *data_cpy = malloc(size);
	if (data_cpy == NULL) {
		perror("malloc");
		goto shm_detach;
	}
	memcpy(data_cpy, img->data, size);

shm_detach:
	XShmDetach(display->display, &shminfo);
img_destroy:
	XDestroyImage(img);
	shmdt(shminfo.shmaddr);
	shmctl(shminfo.shmid, IPC_RMID, NULL);

	screenshot->buf = data_cpy;
	screenshot->width = attrs.width;
	screenshot->height = attrs.height;
}

static struct window_tree *
get_windows(void *display_generic) {
	struct display_X11 *display = display_generic;
	
	struct window_tree *root_node = calloc(sizeof(struct window_tree), 1);
	fill_tree_from_window(display->display, display->root, root_node);
	return root_node;
}

static si
get_dimensions(void *display_generic, size_t dimensions[2]) {
	struct display_X11 *display = display_generic;
	XWindowAttributes attrs;
	XGetWindowAttributes(display->display, display->root, &attrs);
	dimensions[0] = attrs.width;
	dimensions[1] = attrs.height;
}

static const struct display_impl display_impl = {
	.get_screenshot = get_screenshot,
	.get_windows = get_windows,
	.get_dimensions = get_dimensions,
};

// if(window->attr.map_state == IsViewable) {

struct display_X11 *
display_X11_create() {
	struct display_X11 *display = calloc(sizeof(struct display_X11), 1);
	display->impl = &display_impl;

	display->display = XOpenDisplay(NULL);
	display->root = DefaultRootWindow(display->display);

	return display;
}