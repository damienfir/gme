#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <png.h>

struct PngImage {
    int width;
    int height;
    unsigned char* data;
};

PngImage read_png(const char* filename) {
    PngImage result;

    png_image image;
    image.version = PNG_IMAGE_VERSION;
    image.opaque = NULL;

    if (png_image_begin_read_from_file(&image, filename)) {
        image.format = PNG_FORMAT_RGBA;

        result.width = image.width;
        result.height = image.height;

        png_bytep buffer = (png_bytep)malloc(PNG_IMAGE_SIZE(image));
        if (buffer != NULL) {
            result.data = buffer;
            if (png_image_finish_read(&image, NULL, buffer, 0, NULL)) {
                return result;
            } else {
                printf("Cannot read image: %s\n", image.message);
                abort();
            }
        } else {
            printf("Cannot read image: %s\n", image.message);
            abort();
        }
    }

    printf("Cannot read image: %s\n", image.message);
    abort();
}

#endif /* IMAGE_HPP */
