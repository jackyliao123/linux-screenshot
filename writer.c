#include <stdlib.h>
#include <stdbool.h>

#include <gtk/gtk.h>

#include "writer.h"

struct save_task_data {
	cairo_surface_t *memory_surface;
	struct rect bounds;
	char *filename;
	char *format;
};

static void
save_task(GTask *task, void *obj, struct save_task_data *data, GCancellable *cancellable) {
	int x = data->bounds.x1;
	int y = data->bounds.y1;
	int width = data->bounds.x2 - data->bounds.x1;
	int height = data->bounds.y2 - data->bounds.y1;

	GError *error = NULL;

	GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(data->memory_surface, x, y, width, height);
	bool success = gdk_pixbuf_save(pixbuf, data->filename, data->format, &error, NULL);
	sleep(10);

	if(success) {
		g_task_return_boolean(task, TRUE);
	} else {
		g_task_return_error(task, error);
	}
}

static void
destroy_save_task_data(struct save_task_data *data) {
	g_free(data->filename);
	g_free(data->format);
	g_slice_free(struct save_task_data, data);
}

char *
writer_get_available_filename() {
	const char *dir = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
	if(dir == NULL) {
		dir = ".";
	}
}

bool
writer_save_image_finish(GAsyncResult *result, GError **error) {
	GTask *task = G_TASK(result);
	return g_task_propagate_boolean(task, error);
}

bool
writer_save_image_async(cairo_surface_t *memory_surface, const struct rect *bounds, const char *filename, const char *format, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data) {
	struct save_task_data *data = g_slice_new(struct save_task_data);
	data->memory_surface = memory_surface;
	data->bounds = *bounds;
	data->filename = g_strdup(filename);
	data->format = g_strdup(format);

	GTask *task = g_task_new(NULL, cancellable, callback, user_data);
	g_task_set_source_tag(task, writer_save_image_async);
	g_task_set_task_data(task, data, (GDestroyNotify) destroy_save_task_data);
	g_task_run_in_thread(task, (GTaskThreadFunc) save_task);
	g_object_unref(task);

	return true;
}

//	GSList *formats = gdk_pixbuf_get_formats();
//	GSList *ptr = formats;
//	while(ptr != NULL) {
//		GdkPixbufFormat *format = (GdkPixbufFormat *) ptr->data;
//		printf("%s %s %d\n", gdk_pixbuf_format_get_name(format), gdk_pixbuf_format_get_description(format), gdk_pixbuf_format_is_writable(format));
//		ptr = ptr->next;
//	}
//	g_slist_free(formats);
