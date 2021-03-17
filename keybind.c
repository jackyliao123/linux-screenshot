#include "keybind.h"
#include "ui.h"

struct keybind_contexts contexts;

void
keybind_suggest_hide(bool hide) {
	contexts.suggest_hide = hide;
	if(contexts.force_hide > 0 || contexts.suggest_hide) {
		gtk_widget_hide(ui.window);
	} else {
		gtk_widget_show(ui.window);
	}
}

void
keybind_force_hide(bool hide) {
	if(hide) {
		contexts.force_hide++;
		keybind_suggest_hide(contexts.suggest_hide);
	} else {
		if(contexts.force_hide > 0) {
			contexts.force_hide--;
		}
		keybind_suggest_hide(contexts.suggest_hide);
	}
}

void
keybind_request_exit() {
	contexts.exit_requested = true;
	keybind_suggest_hide(true);
	if(contexts.running_context == 0) {
		gtk_main_quit();
	}
}

void
keybinds_continue_processing(struct keybind_exec_context *context) {
	while(context->ind < contexts.keybinds->num_keybind) {
		struct keybind *keybind = &contexts.keybinds->keybinds[context->ind];
		if(context->current_completed) {
			context->ind++;
			context->current_completed = false;
			if(context->current_success) {
				if(keybind->exit) {
					context->ind = contexts.keybinds->num_keybind;
					keybind_request_exit();
				}
				if(keybind->consume) {
					context->ind = contexts.keybinds->num_keybind;
				}
			} else {
				if(keybind->exit) {
					contexts.suggest_hide = false;
				}
			}
		} else if(context->modifier == keybind->modifier && context->key == keybind->key) {
			if(keybind->exit) {
				contexts.suggest_hide = true;
			}
			enum keybind_handle_status result = keybind_handle(keybind, context);
			if(result == KEYBIND_ASYNC_REQUIRED) {
				printf("No more async contexts available\n");
				context->current_success = false;
				context->current_completed = true;
			} else if(result == KEYBIND_ASYNC) {
				keybind_suggest_hide(contexts.suggest_hide);
				return;
			} else {
				context->current_success = result == KEYBIND_SUCCESS;
				context->current_completed = true;
			}
		} else {
			context->ind++;
		}
	}
	contexts.running_context -= 1;
	keybind_suggest_hide(contexts.suggest_hide);
	if(contexts.exit_requested && contexts.running_context == 0) {
		gtk_main_quit();
	}
}

void
keybind_done_and_continue(struct keybind_exec_context *context, bool success) {
	context->current_completed = true;
	context->current_success = success;
	keybinds_continue_processing(context);
}

struct keybind_exec_context *
keybind_start(guint keyval, GdkModifierType type) {
	if(contexts.running_context >= contexts.max_contexts) {
		return NULL;
	}
	struct keybind_exec_context *context;
	for(size_t i = 0; i < contexts.max_contexts; ++i) {
		context = &contexts.contexts[i];
		if(!context->running) {
			break;
		}
	}
	context->ind = 0;
	context->allow_async = contexts.running_context != contexts.max_contexts - 1;
	context->current_completed = false;
	context->current_success = false;
	context->key = keyval;
	context->modifier = type;
	context->running = true;
	contexts.running_context++;
	keybinds_continue_processing(context);
	return context;
}

void
keybinds_init(struct keybind_list *keybinds, size_t max_async_ctx) {
	for(size_t i = 0; i < keybinds->num_keybind; ++i) {
		struct keybind *keybind = &keybinds->keybinds[i];
		gtk_accelerator_parse(keybind->accelerator, &keybind->key, &keybind->modifier);
		if(keybind->key == 0 && keybind->modifier == 0) {
			printf("Failed to parse accelerator: %s\n", keybind->accelerator);
			exit(1);
		}
	}
	contexts.keybinds = keybinds;
	contexts.max_contexts = max_async_ctx + 1;
	contexts.running_context = 0;
	contexts.contexts = malloc(sizeof(struct keybind_exec_context) * contexts.max_contexts);
	for(size_t i = 0; i < contexts.max_contexts; ++i) {
		contexts.contexts[i].running = false;
	}
}
