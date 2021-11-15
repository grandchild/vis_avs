#include "pixel_format.h"

typedef struct {
    void* data;
    AVSPixelFormat pixel_format;
    size_t pixel_size;
    int w;
    int h;
    const char* error;
} AVS_image;

AVS_image* image_load(const char* filename,
                      AVSPixelFormat pixel_format = AVS_PXL_FMT_RGB8);
void image_free(AVS_image* image);
