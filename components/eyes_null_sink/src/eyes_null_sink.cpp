#include "eyes_null_sink.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace eyes_null {

EyesNullSink::EyesNullSink(const EyesNullSinkConfig &cfg) : delay_frames_(cfg.delay_frames) {
}

bool EyesNullSink::begin(int width, int height, int fps) {
  if (width <= 0 || height <= 0 || fps <= 0) {
    return false;
  }

  width_ = width;
  height_ = height;
  frame_count_ = 0;
  return true;
}

bool EyesNullSink::frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) {
  if (grayscale == nullptr || stride < width_) {
    return false;
  }
  if (height_ <= 0) {
    return false;
  }

  ++frame_count_;
  if (delay_frames_ && duration_ms > 0) {
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
  }
  return true;
}

bool EyesNullSink::end() {
  return true;
}

} // namespace eyes_null

