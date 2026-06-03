#include "eyes/gif/gif_frame_sink.h"

namespace eyes::gif {

bool GifFrameSink::begin(int width, int height, int fps) {
  fps_ = fps;
  (void)fps_;
  delay_carry_us_ = 0;
  return encoder_.begin(width, height, true);
}

bool GifFrameSink::frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) {
  // GIF delay is in centiseconds (10ms). To better match the requested per-frame durations,
  // we dither the quantization using a carry of leftover microseconds.
  int64_t total_us = (int64_t)duration_ms * 1000 + delay_carry_us_;
  uint32_t cs = (uint32_t)(total_us / 10000);
  delay_carry_us_ = (int32_t)(total_us - (int64_t)cs * 10000);

  // Clamp to 1..65535.
  if (cs < 1) {
    cs = 1;
    delay_carry_us_ = 0;
  }
  if (cs > 65535) {
    cs = 65535;
    delay_carry_us_ = 0;
  }
  return encoder_.add_frame(grayscale, stride, static_cast<uint16_t>(cs));
}

bool GifFrameSink::end() {
  return encoder_.end();
}

} // namespace eyes::gif
