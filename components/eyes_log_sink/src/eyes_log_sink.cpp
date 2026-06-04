#include "eyes_log_sink.h"

#include <inttypes.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace eyes_log {

namespace {

uint32_t sample_checksum(const uint8_t *grayscale, int width, int height, int stride,
                         int sample_step) {
  uint32_t checksum = 2166136261u;
  for (int y = 0; y < height; y += sample_step) {
    const uint8_t *row = grayscale + (y * stride);
    for (int x = 0; x < width; x += sample_step) {
      checksum ^= row[x];
      checksum *= 16777619u;
    }
  }
  return checksum;
}

} // namespace

EyesLogSink::EyesLogSink(const EyesLogSinkConfig &cfg)
    : tag_(cfg.tag ? cfg.tag : "eyes_log_sink"),
      sample_step_(cfg.sample_step > 0 ? cfg.sample_step : 8),
      log_every_frames_(cfg.log_every_frames), delay_frames_(cfg.delay_frames) {
}

bool EyesLogSink::begin(int width, int height, int fps) {
  if (width <= 0 || height <= 0 || fps <= 0) {
    return false;
  }

  width_ = width;
  height_ = height;
  fps_ = fps;
  frame_count_ = 0;
  ESP_LOGI(tag_, "Begin clip: %dx%d @ %d fps", width_, height_, fps_);
  return true;
}

bool EyesLogSink::frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) {
  if (grayscale == nullptr || stride < width_) {
    return false;
  }

  ++frame_count_;
  if (log_every_frames_ > 0 && (frame_count_ % log_every_frames_) == 0) {
    const uint32_t checksum = sample_checksum(grayscale, width_, height_, stride, sample_step_);
    ESP_LOGI(tag_, "Frame %" PRIu32 " checksum 0x%08" PRIx32, frame_count_, checksum);
  }

  if (delay_frames_ && duration_ms > 0) {
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
  }
  return true;
}

bool EyesLogSink::end() {
  ESP_LOGI(tag_, "End clip after %" PRIu32 " frames", frame_count_);
  return true;
}

} // namespace eyes_log

