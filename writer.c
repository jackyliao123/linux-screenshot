#include <zlib.h>
#include <stdlib.h>
#include <stdbool.h>

#include <gtk/gtk.h>

#include "writer.h"
#include "overlay.h"

static unsigned char *
alloc_compress(unsigned char *data, int data_len, int *out_len, int quality) {
	size_t alloc_len = compressBound(data_len);
	unsigned char *buf = malloc(alloc_len);
	if(!buf) {
		return NULL;
	}
	if(compress2(buf, &alloc_len, data, data_len, quality) != Z_OK) {
		free(buf);
		return NULL;
	}
	*out_len = alloc_len;
	return buf;
}

#define STBIW_ZLIB_COMPRESS alloc_compress
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static bool
write_image(const struct screenshot *screenshot, const struct rect *bounds) {
	int x = bounds->x1;
	int y = bounds->y1;
	int width = bounds->x2 - bounds->x1;
	int height = bounds->y2 - bounds->y1;

	GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface(overlay.memory_surface, x, y, width, height);
	gdk_pixbuf_save(pixbuf, "output.png", "png", NULL, NULL);

	GSList *formats = gdk_pixbuf_get_formats();
	GSList *ptr = formats;
	while(ptr != NULL) {
		GdkPixbufFormat *format = (GdkPixbufFormat *) ptr->data;
		printf("%s %s %d\n", gdk_pixbuf_format_get_name(format), gdk_pixbuf_format_get_description(format), gdk_pixbuf_format_is_writable(format));
		ptr = ptr->next;
	}
	g_slist_free(formats);

	return true;
}

struct image_writer image_writer_file = {
		.write_image = write_image
};

struct image_writer *
new_image_writer_file() {
	return &image_writer_file;
}