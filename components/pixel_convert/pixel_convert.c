#include "pixel_convert.h"

#include <stddef.h>

uint16_t pixel_convert_gray8_to_rgb565(uint8_t gray) {
  uint16_t r5 = (uint16_t)(gray >> 3);
  uint16_t g6 = (uint16_t)(gray >> 2);
  uint16_t b5 = (uint16_t)(gray >> 3);
  return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

void pixel_convert_gray8_buffer_to_rgb565(const uint8_t *gray, int gray_stride, uint16_t *rgb565,
                                         int rgb565_stride_words, int width, int height) {
  if (!gray || !rgb565 || gray_stride <= 0 || rgb565_stride_words <= 0 || width <= 0 || height <= 0) {
    return;
  }

  for (int y = 0; y < height; ++y) {
    const uint8_t *in = gray + (ptrdiff_t)y * gray_stride;
    uint16_t *out = rgb565 + (ptrdiff_t)y * rgb565_stride_words;
    for (int x = 0; x < width; ++x) {
      out[x] = pixel_convert_gray8_to_rgb565(in[x]);
    }
  }
}

