#include "esp_check.h"
#include "esp_log.h"

#include "eyes/eyes.h"
#include "eyes_emotion_cycle.h"
#include "eyes_lcd_sink.h"
#include "lcd_st7789.h"

namespace {

constexpr char TAG[] = "roboeyes_st7789_tinys3";

} // namespace

extern "C" void app_main() {
  ESP_LOGI(TAG, "RoboEyes ST7789 TinyS3 boot");

  const lcd::St7789Config lcd_cfg = lcd::tinys3_st7789v3_config_240x280();
  lcd::St7789Display display;
  ESP_ERROR_CHECK(display.init(lcd_cfg));

  eyes_lcd::EyesLcdSinkConfig sink_cfg;
  sink_cfg.display = &display;
  eyes_lcd::EyesLcdSink sink(sink_cfg);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  eyes::EyesClipPlayer player(animator, renderer, sink);

  eyes::ClipOptions options;
  options.size = {lcd_cfg.width, lcd_cfg.height};
  options.fps = 30;
  options.realtime = true;

  if (!eyes_cycle::run_forever(player, options)) {
    ESP_LOGE(TAG, "Emotion cycle stopped");
  }
}

