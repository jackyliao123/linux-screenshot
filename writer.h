#ifndef WRITER_H
#define WRITER_H

#include "display.h"

struct image_writer {
	bool (*write_image)(const struct screenshot *screenshot, const struct rect *bounds);
};

struct image_writer *new_image_writer_file();

#endif
