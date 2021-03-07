#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xrandr.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>

#include "display.h"
#include "display_X11.h"

#include <stdio.h>
#include <stdbool.h>

struct display_X11 display;

static int clamp(int v, int min, int max) {
	if(v < min) {
		return min;
	} else if(v > max) {
		return max;
	}
	return v;
}

static bool
fill_node_from_window(Display *display, Window root, Window window, struct window_tree *node, struct window_tree *parent_node) {
	Window parent;
	Window *children;

	XWindowAttributes attrs;
	XGetWindowAttributes(display, window, &attrs);
	if(attrs.map_state != IsViewable) {
		return false;
	}
	node->bounds.width = attrs.width;
	node->bounds.height = attrs.height;

	Window child;
	XTranslateCoordinates(display, window, root, -attrs.border_width,
			-attrs.border_width, &node->bounds.x, &node->bounds.y, &child);
	// Clip to parent bounds
	if(parent_node != NULL) {
		int x2 = node->bounds.x + node->bounds.width;
		int y2 = node->bounds.y + node->bounds.height;
		int px2 = parent_node->bounds.x + parent_node->bounds.width;
		int py2 = parent_node->bounds.y + parent_node->bounds.height;
		node->bounds.x = clamp(node->bounds.x, parent_node->bounds.x, px2);
		node->bounds.y = clamp(node->bounds.y, parent_node->bounds.y, py2);
		x2 = clamp(x2, parent_node->bounds.x, px2);
		y2 = clamp(y2, parent_node->bounds.y, py2);
		node->bounds.width = x2 - node->bounds.x;
		node->bounds.height = y2 - node->bounds.y;
	}

	unsigned int nchildren;
	XQueryTree(display, window, &root, &parent, &children, &nchildren);
	node->children = calloc(sizeof(struct window_tree), nchildren);
	if (node->children == NULL && nchildren > 0) {
		perror("malloc"); // TODO no errno is set by calloc
		exit(1);
	}

	node->num_children = 0;
	for (size_t i = 0; i < nchildren; ++i) {
		if(fill_node_from_window(display, root, children[i], &node->children[node->num_children], node)) {
			node->num_children++;
		}
	}

	XFree(children);
	return true;
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
		exit(-1);
//		goto img_destroy;
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
	fill_node_from_window(display->display, display->root, display->root, root_node, NULL);
	return root_node;
}

//static void
//get_outputs(void *display_generic, size_t dimensions[2]) {
//	XRRScreenResources *xrr_screen = XRRGetScreenResourcesCurrent(display, root);
//	for(int i = 0; i < xrr_screen->ncrtc; ++i) {
//		XRRCrtcInfo *xrr_crtc = XRRGetCrtcInfo(display, xrr_screen, xrr_screen->crtcs[i]);
//		int crtc_ind = add_crtc(xrr_crtc->width, xrr_crtc->height, xrr_crtc->x, xrr_crtc->y);
//		for(int j = 0; j < xrr_crtc->noutput; ++j) {
//			XRROutputInfo *xrr_output = XRRGetOutputInfo(display, xrr_screen, xrr_crtc->outputs[j]);
//			add_output(xrr_output->name, xrr_output->nameLen, crtc_ind);
//			XRRFreeOutputInfo(xrr_output);
//		}
//		XRRFreeCrtcInfo(xrr_crtc);
//	}
//	XRRFreeScreenResources(xrr_screen);
//}

static const struct display_impl display_impl = {
	.get_screenshot = get_screenshot,
	.get_windows = get_windows,
//	.get_outputs = get_outputs,
};

struct display_X11 *
display_X11_create() {
	display.impl = &display_impl;

	display.display = XOpenDisplay(NULL);
	display.root = DefaultRootWindow(display.display);

	return &display;
}