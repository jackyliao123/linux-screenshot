#include <stdbool.h>

#include "geometry.h"

static bool
contains(struct window_tree *window, int x, int y) {
	if(window == NULL) {
		return false;
	}
    return window->bounds.x <= x
        && window->bounds.y <= y
        && x < window->bounds.x + window->bounds.width 
        && y < window->bounds.y + window->bounds.height;
}

struct window_tree *
window_tree_get_window_under(struct window_tree *root, int x, int y) {
    if(root == NULL) {
        return NULL;
    }
    if(!contains(root, x, y)) {
        return NULL;
    }
	for(size_t i = 0; i < root->num_children; ++i) {
        struct window_tree *under = window_tree_get_window_under(&root->children[root->num_children - i - 1], x, y);
        if(under) {
            return under;
        }
	}
	return root;
}
