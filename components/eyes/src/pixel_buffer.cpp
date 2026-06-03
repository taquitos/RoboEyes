#include "eyes/pixel_buffer.h"

namespace eyes {

void PixelBuffer8::clear(uint8_t value) {
  if (!data_ || width_ <= 0 || height_ <= 0 || stride_ < width_) {
    return;
  }

  for (int y = 0; y < height_; ++y) {
    uint8_t *row = data_ + y * stride_;
    for (int x = 0; x < width_; ++x) {
      row[x] = value;
    }
  }
}

void PixelBuffer8::set(int x, int y, uint8_t value) {
  if (!data_ || x < 0 || y < 0 || x >= width_ || y >= height_) {
    return;
  }
  data_[y * stride_ + x] = value;
}

uint8_t PixelBuffer8::get(int x, int y) const {
  if (!data_ || x < 0 || y < 0 || x >= width_ || y >= height_) {
    return 0;
  }
  return data_[y * stride_ + x];
}

} // namespace eyes
