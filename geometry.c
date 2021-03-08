#include <stdbool.h>
#include <stdlib.h>

#include "geometry.h"

int
clamp(int v, int min, int max) {
	if(v < min) {
		return min;
	} else if(v > max) {
		return max;
	}
	return v;
}

int
min(int a, int b) {
	return a < b ? a : b;
}

int
max(int a, int b) {
	return a > b ? a : b;
}

bool
contains(struct rect bounds, int x, int y) {
	return bounds.x1 <= x && x < bounds.x2 && bounds.y1 <= y && y < bounds.y2;
}

struct rect
geom_union(const struct rect *r1, const struct rect *r2) {
	struct rect res = {
			.x1 = min(r1->x1, r2->x1),
			.y1 = min(r1->y1, r2->y1),
			.x2 = max(r1->x2, r2->x2),
			.y2 = max(r1->y2, r2->y2),
	};
	return res;
}

struct window_tree *
geom_get_window_under(struct window_tree *root, int x, int y) {
    if(root == NULL) {
        return NULL;
    }
    if(!contains(root->bounds, x, y)) {
        return NULL;
    }
	for(size_t i = 0; i < root->num_children; ++i) {
        struct window_tree *under = geom_get_window_under(&root->children[root->num_children - i - 1], x, y);
        if(under) {
            return under;
        }
	}
	return root;
}

struct output *
geom_get_output_under(struct output_list *list, int x, int y) {
	if(list == NULL) {
		return NULL;
	}
	for(size_t i = 0; i < list->num_output; ++i) {
		if(contains(list->outputs[i].bounds, x, y)) {
			return &list->outputs[i];
		}
	}
	return NULL;
}

void
geom_free_output_list(struct output_list *list) {
	for(size_t i = 0; i < list->num_output; ++i) {
		free(list->outputs[i].name);
	}
	free(list->outputs);
}
void
geom_free_window_tree(struct window_tree *tree) {
	if(!tree) {
		return;
	}
	for(size_t i = 0; i < tree->num_children; ++i) {
		geom_free_window_tree(&tree->children[i]);
	}
	free(tree->name);
	free(tree->children);
	free(tree);
}

void
geom_clip_to_parent(struct window_tree *node) {
	if(node->parent != NULL) {
		node->bounds.x1 = clamp(node->bounds.x1, node->parent->bounds.x1, node->parent->bounds.x2);
		node->bounds.y1 = clamp(node->bounds.y1, node->parent->bounds.y1, node->parent->bounds.y2);
		node->bounds.x2 = clamp(node->bounds.x2, node->parent->bounds.x1, node->parent->bounds.x2);
		node->bounds.y2 = clamp(node->bounds.y2, node->parent->bounds.y1, node->parent->bounds.y2);
	}
}
