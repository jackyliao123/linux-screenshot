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
contains(const struct rect *bounds, int x, int y) {
	return bounds->x1 <= x && x < bounds->x2 && bounds->y1 <= y && y < bounds->y2;
}

struct rect
geom_shift(const struct rect *r, int dx, int dy) {
	return (struct rect) {
		.x1 = r->x1 + dx,
		.y1 = r->y1 + dy,
		.x2 = r->x2 + dx,
		.y2 = r->y2 + dy,
	};
}

struct rect
geom_union(const struct rect *r1, const struct rect *r2) {
	return (struct rect) {
			.x1 = min(r1->x1, r2->x1),
			.y1 = min(r1->y1, r2->y1),
			.x2 = max(r1->x2, r2->x2),
			.y2 = max(r1->y2, r2->y2),
	};
}

struct rect
geom_intersect(const struct rect *r1, const struct rect *r2, bool *intersects) {
	struct rect res = {
			.x1 = max(r1->x1, r2->x1),
			.y1 = max(r1->y1, r2->y1),
			.x2 = min(r1->x2, r2->x2),
			.y2 = min(r1->y2, r2->y2),
	};
	if(intersects) {
		*intersects = res.x2 > res.x1 && res.y2 > res.y1;
	}
	return res;
}

struct output *
geom_get_best_output(struct output_list *outputs, const struct rect *rect) {
	struct output *best = NULL;
	size_t best_value = 0;
	for(size_t i = 0; i < outputs->num_output; ++i) {
		bool intersects;
		struct rect intersection = geom_intersect(&outputs->outputs[i].bounds, rect, &intersects);
		if(intersects) {
			size_t value = intersection.x2 - intersection.x1 + intersection.y2 - intersection.y1;
			if(value > best_value) {
				best_value = value;
				best = &outputs->outputs[i];
			}
		}
	}
	return best;
}

struct rect
geom_expand(const struct rect *bounds, int amount) {
	struct rect res = {
			.x1 = bounds->x1 - amount,
			.y1 = bounds->y1 - amount,
			.x2 = bounds->x2 + amount,
			.y2 = bounds->y2 + amount,
	};
	return res;
}

struct window_tree *
geom_get_window_under(struct window_tree *root, int x, int y) {
    if(root == NULL) {
        return NULL;
    }
    if(!contains(&root->bounds, x, y)) {
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

struct window_tree *
geom_get_window_parent_under(struct window_tree *root, int x, int y, size_t nth_parent) {
	struct window_tree *window = geom_get_window_under(root, x, y);
	for(size_t i = 0; i < nth_parent; ++i) {
		if(window != NULL) {
			window = window->parent;
		}
	}
	return window;
}

struct output *
geom_get_output_under(struct output_list *list, int x, int y) {
	if(list == NULL) {
		return NULL;
	}
	for(size_t i = 0; i < list->num_output; ++i) {
		if(contains(&list->outputs[i].bounds, x, y)) {
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
