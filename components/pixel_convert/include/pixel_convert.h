#ifndef PIXEL_CONVERT_H
#define PIXEL_CONVERT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Converts an 8-bit grayscale value to RGB565 with R=G=B.
uint16_t pixel_convert_gray8_to_rgb565(uint8_t gray);

// Converts a Gray8 image to an RGB565 image.
//
// - `gray_stride` is in bytes.
// - `rgb565_stride_words` is in uint16_t words (not bytes).
void pixel_convert_gray8_buffer_to_rgb565(const uint8_t *gray, int gray_stride, uint16_t *rgb565,
                                         int rgb565_stride_words, int width, int height);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

