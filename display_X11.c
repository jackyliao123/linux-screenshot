#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xrandr.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>

#include "display.h"
#include "display_X11.h"

#include <stdio.h>
#include <stdbool.h>

struct display_X11 display;

static char *
clone_str(const char *str) {
	if(!str) {
		return NULL;
	}
	size_t len = strlen(str);
	char *new_str = malloc(len + 1);
	memcpy(new_str, str, len + 1);
	return new_str;
}

static bool
fill_node_from_window(Window window, struct window_tree *node) {
	XWindowAttributes attrs;
	XGetWindowAttributes(display.display, window, &attrs);
	if(attrs.map_state != IsViewable) {
		return false;
	}

	char *window_name = NULL;
	XFetchName(display.display, window, &window_name);
	node->name = clone_str(window_name);
	XFree(window_name);

	Window child;
	XTranslateCoordinates(display.display, window, display.root, -attrs.border_width,
	                      -attrs.border_width, &node->bounds.x1, &node->bounds.y1, &child);
	node->bounds.x2 = node->bounds.x1 + attrs.width;
	node->bounds.y2 = node->bounds.y1 + attrs.height;
	geom_clip_to_parent(node);

	Window garbage1, garbage2;
	Window *children;
	unsigned int nchildren;
	XQueryTree(display.display, window, &garbage1, &garbage2, &children, &nchildren);
	node->children = calloc(sizeof(struct window_tree), nchildren);
	if(node->children == NULL && nchildren > 0) {
		perror("malloc"); // TODO no errno is set by calloc
		exit(1);
	}

	node->num_children = 0;
	for(size_t i = 0; i < nchildren; ++i) {
		node->children[node->num_children].parent = node;
		if(fill_node_from_window(children[i], &node->children[node->num_children])) {
			node->num_children++;
		}
	}

	XFree(children);
	return true;
}

static void
get_screenshot(struct screenshot *screenshot) {
	XShmSegmentInfo shminfo;
	XImage *img;

	XWindowAttributes attrs;
	XGetWindowAttributes(display.display, display.root, &attrs);

	img = XShmCreateImage(display.display, attrs.visual, attrs.depth, ZPixmap,
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
	if (!XShmAttach(display.display, &shminfo)) {
		perror("XShmAttach");
		exit(-1);
//		goto img_destroy;
	}

	XShmGetImage(display.display, display.root, img, 0, 0, AllPlanes);
	
	unsigned char *data_cpy = malloc(size);
	if (data_cpy == NULL) {
		perror("malloc");
		goto shm_detach;
	}
	memcpy(data_cpy, img->data, size);

shm_detach:
	XShmDetach(display.display, &shminfo);
img_destroy:
	XDestroyImage(img);
	shmdt(shminfo.shmaddr);
	shmctl(shminfo.shmid, IPC_RMID, NULL);

	screenshot->buf = data_cpy;
	screenshot->width = attrs.width;
	screenshot->height = attrs.height;
}

static struct window_tree *
get_windows(void) {
	struct window_tree *root_node = calloc(sizeof(struct window_tree), 1);
	fill_node_from_window(display.root, root_node);
	return root_node;
}

static void
get_outputs(struct output_list *output_list) {
	size_t nentries = 0;
	size_t array_size = 1;

	struct output *outputs = calloc(sizeof(struct output), array_size);

	XRRScreenResources *xrr_screen = XRRGetScreenResourcesCurrent(display.display, display.root);
	for(int i = 0; i < xrr_screen->ncrtc; ++i) {
		XRRCrtcInfo *xrr_crtc = XRRGetCrtcInfo(display.display, xrr_screen, xrr_screen->crtcs[i]);
		struct rect bounds = {
				.x1 = xrr_crtc->x,
				.y1 = xrr_crtc->y,
				.x2 = xrr_crtc->x + (int) xrr_crtc->width,
				.y2 = xrr_crtc->y + (int) xrr_crtc->height,
		};
		for(int j = 0; j < xrr_crtc->noutput; ++j) {
			XRROutputInfo *xrr_output = XRRGetOutputInfo(display.display, xrr_screen, xrr_crtc->outputs[j]);

			if(nentries + 1 == array_size) {
				array_size *= 2;
				outputs = realloc(outputs, sizeof(struct output) * array_size);
			}
			outputs[nentries].name = clone_str(xrr_output->name);

			outputs[nentries].bounds = bounds;
			nentries++;

			XRRFreeOutputInfo(xrr_output);
		}
		XRRFreeCrtcInfo(xrr_crtc);
	}
	XRRFreeScreenResources(xrr_screen);

	output_list->outputs = outputs;
	output_list->num_output = nentries;
}

static const struct display_impl display_impl = {
	.get_screenshot = get_screenshot,
	.get_windows = get_windows,
	.get_outputs = get_outputs,
};

struct display_X11 *
display_X11_create(void) {
	display.impl = &display_impl;

	display.display = XOpenDisplay(NULL);
	display.root = DefaultRootWindow(display.display);

	return &display;
}