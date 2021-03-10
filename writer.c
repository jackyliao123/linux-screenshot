#include <zlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "writer.h"

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
	int stride = screenshot->width;
	uint32_t *src = screenshot->buf;
	uint32_t *dst = malloc(width * height * 4);
	for(int yy = 0; yy < height; ++yy) {
		uint32_t *src_sub = src + (yy + y) * stride + x;
		uint32_t *dst_sub = dst + yy * width;
		for(int xx = 0; xx < width; ++xx) {
			dst_sub[xx] = src_sub[xx] & 0xFF00FF00 | (src_sub[xx] & 0x00FF0000) >> 16 | (src_sub[xx] & 0x000000FF) << 16;
		}
	}
	stbi_write_png("output.png", width, height, 4, dst, width * 4);
	free(dst);
	return true;
}

struct image_writer image_writer_file = {
		.write_image = write_image
};

struct image_writer *
new_image_writer_file() {
	return &image_writer_file;
}