#ifndef WINDOW_TREE_H
#define WINDOW_TREE_H

#include <stddef.h>

struct rect {
	int x, y;
	int width, height;
};

struct window_tree {
    struct rect bounds;
	size_t num_children;
	struct window_tree *children;
};

struct window_tree *window_tree_get_window_under(struct window_tree *list, int x, int y);

#endif