#ifndef KEYBIND_H
#define KEYBIND_H

#include <gdk/gdk.h>
#include <stdbool.h>

enum keybind_action {
	ACTION_NOP,
	ACTION_SAVE,
	ACTION_SAVE_AS,
	ACTION_COPY,
	ACTION_DESELECT,
	ACTION_WARP_LEFT,
	ACTION_WARP_RIGHT,
	ACTION_WARP_UP,
	ACTION_WARP_DOWN,
};

struct keybind {
	char *accelerator;
	enum keybind_action action;
	bool exit;
	bool consume;
	guint key;
	GdkModifierType modifier;
};

struct keybind_list {
	size_t num_keybind;
	struct keybind *keybinds;
};

struct keybind_exec_context {
	int ind;
	guint key;
	GdkModifierType modifier;
	bool allow_async;
	bool current_completed;
	bool current_success;
	bool running;
};

enum keybind_handle_status {
	KEYBIND_FAIL = 0,
	KEYBIND_SUCCESS = 1,
	KEYBIND_ASYNC = 2,
	KEYBIND_ASYNC_REQUIRED = 3,
};

struct keybind_contexts {
	struct keybind_list *keybinds;
	size_t max_contexts;
	size_t running_context;
	struct keybind_exec_context *contexts;
	size_t force_hide;
	bool suggest_hide;
	bool exit_requested;
};

extern struct keybind_contexts contexts;

void keybind_suggest_hide(bool hide);
void keybind_force_hide(bool hide);
void keybind_request_exit();
struct keybind_exec_context *keybind_start(guint keyval, GdkModifierType type);
void keybind_done_and_continue(struct keybind_exec_context *context, bool success);
void keybinds_init(struct keybind_list *keybinds, size_t max_async_ctx);


#endif
