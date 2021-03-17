#ifndef WRITER_H
#define WRITER_H

#include "display.h"

bool writer_save_image_finish(GAsyncResult *result, GError **error);
bool writer_save_image_async(cairo_surface_t *memory_surface, const struct rect *bounds, const char *filename, const char *format, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);

#endif
