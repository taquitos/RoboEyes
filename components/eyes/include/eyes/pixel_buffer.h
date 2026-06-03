#ifndef EYES_PIXEL_BUFFER_H
#define EYES_PIXEL_BUFFER_H

#include <stdint.h>

namespace eyes {

class PixelBuffer8 {
public:
  PixelBuffer8(uint8_t *data, int width, int height, int stride)
      : data_(data), width_(width), height_(height), stride_(stride) {
  }

  uint8_t *data() {
    return data_;
  }
  const uint8_t *data() const {
    return data_;
  }
  int width() const {
    return width_;
  }
  int height() const {
    return height_;
  }
  int stride() const {
    return stride_;
  }

  void clear(uint8_t value);
  void set(int x, int y, uint8_t value);
  uint8_t get(int x, int y) const;

private:
  uint8_t *data_;
  int width_;
  int height_;
  int stride_;
};

} // namespace eyes

#endif
