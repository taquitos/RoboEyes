#include "esp_log.h"

#include "eyes/eyes.h"
#include "eyes_emotion_cycle.h"
#include "eyes_log_sink.h"

namespace {
constexpr char TAG[] = "roboeyes_headless";
} // namespace

extern "C" void app_main() {
  ESP_LOGI(TAG, "RoboEyes headless boot");

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;

  eyes_log::EyesLogSinkConfig sink_cfg;
  sink_cfg.tag = TAG;
  eyes_log::EyesLogSink sink(sink_cfg);

  eyes::EyesClipPlayer player(animator, renderer, sink);

  eyes::ClipOptions options;
  options.size = {128, 64};
  options.fps = 20;
  options.realtime = false;

  if (!eyes_cycle::run_forever(player, options)) {
    ESP_LOGE(TAG, "Emotion cycle stopped");
  }
}

