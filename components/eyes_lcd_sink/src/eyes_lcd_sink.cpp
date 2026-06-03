#include "eyes_lcd_sink.h"

#include "lcd_st7789.h"
#include "pixel_convert.h"

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace eyes_lcd {

namespace {
static const char *TAG = "eyes_lcd_sink";

static void delay_until_us(int64_t deadline_us) {
  const int64_t tick_us = (int64_t)portTICK_PERIOD_MS * 1000;
  while (true) {
    int64_t now_us = esp_timer_get_time();
    int64_t remaining_us = deadline_us - now_us;
    if (remaining_us <= 0) {
      return;
    }

    // vTaskDelay() wakes on a tick boundary, which can overshoot by up to one tick. To avoid
    // accumulating drift (which makes the animation look "slow"), we sleep with a safety margin
    // and re-check the deadline.
    if (tick_us > 0 && remaining_us > tick_us) {
      int64_t safe_us = remaining_us - tick_us;
      TickType_t ticks = (TickType_t)(safe_us / tick_us);
      if (ticks < 1) {
        ticks = 1;
      }
      vTaskDelay(ticks);
      continue;
    }

    // Busy-wait the final sub-tick remainder for stable pacing.
    esp_rom_delay_us((uint32_t)remaining_us);
    return;
  }
}
} // namespace

EyesLcdSink::EyesLcdSink(const EyesLcdSinkConfig &cfg) : display_(cfg.display) {
}

EyesLcdSink::~EyesLcdSink() {
  if (rgb565_) {
    heap_caps_free(rgb565_);
    rgb565_ = nullptr;
    rgb565_words_ = 0;
  }
}

bool EyesLcdSink::begin(int width, int height, int fps) {
  if (!display_ || width <= 0 || height <= 0 || fps <= 0) {
    return false;
  }

  width_ = width;
  height_ = height;
  fps_ = fps;

  size_t words = (size_t)width * (size_t)height;
  if (!rgb565_ || rgb565_words_ != words) {
    if (rgb565_) {
      heap_caps_free(rgb565_);
      rgb565_ = nullptr;
    }
    rgb565_ = (uint16_t *)heap_caps_malloc(words * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!rgb565_) {
      ESP_LOGE(TAG, "rgb565 buffer alloc failed (%u bytes)", (unsigned)(words * sizeof(uint16_t)));
      return false;
    }
    rgb565_words_ = words;
  }

  if (display_->width() != width_ || display_->height() != height_) {
    ESP_LOGE(TAG, "display size mismatch: display=%dx%d, sink=%dx%d", display_->width(),
             display_->height(), width_, height_);
    return false;
  }

  return true;
}

bool EyesLcdSink::frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) {
  if (!display_ || !rgb565_ || !grayscale) {
    return false;
  }
  if (stride <= 0 || width_ <= 0 || height_ <= 0) {
    return false;
  }

  int64_t t0_us = esp_timer_get_time();

  pixel_convert_gray8_buffer_to_rgb565(grayscale, stride, rgb565_, width_, width_, height_);

  esp_err_t err = display_->draw_full_rgb565(rgb565_, width_, height_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "draw failed: %s", esp_err_to_name(err));
    return false;
  }
  err = display_->flush();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "flush failed: %s", esp_err_to_name(err));
    return false;
  }

  if (duration_ms > 0) {
    int64_t deadline_us = t0_us + (int64_t)duration_ms * 1000;
    delay_until_us(deadline_us);
  }

  return true;
}

bool EyesLcdSink::end() {
  // Nothing to do: the display remains initialized and we reuse buffers between clips.
  return true;
}

} // namespace eyes_lcd
