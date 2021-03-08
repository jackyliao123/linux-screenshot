#ifndef WINDOW_TREE_H
#define WINDOW_TREE_H

#include <stddef.h>
#include <stdbool.h>

struct rect {
	int x1, y1; // inclusive
	int x2, y2; // exclusive
};

struct output {
	char *name;
	struct rect bounds;
};

struct output_list {
	size_t num_output;
	struct output *outputs;
};

struct window_tree {
	char *name;
	struct rect bounds;
	size_t num_children;
	struct window_tree *children;
	struct window_tree *parent;
};

int min(int a, int b);
int max(int a, int b);
int clamp(int v, int min, int max);
bool contains(struct rect bounds, int x, int y);
struct rect geom_union(const struct rect *r1, const struct rect *r2);
struct window_tree *geom_get_window_under(struct window_tree *list, int x, int y);
struct output *geom_get_output_under(struct output_list *list, int x, int y);
void geom_free_output_list(struct output_list *list);
void geom_free_window_tree(struct window_tree *tree);
void geom_clip_to_parent(struct window_tree *node);

#endif