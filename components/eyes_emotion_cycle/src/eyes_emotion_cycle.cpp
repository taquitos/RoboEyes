#include "eyes_emotion_cycle.h"

#include "eyes/emotion.h"
#include "eyes/sequencer.h"

#include "esp_log.h"

namespace eyes_cycle {

namespace {
static const char *TAG = "eyes_cycle";
} // namespace

bool run_forever(const eyes::EyesClipPlayer &player, const eyes::ClipOptions &options) {
  eyes::SequencerOptions sequencer_opt;
  sequencer_opt.enable_transitions = true;
  sequencer_opt.transition_ms = 420;
  eyes::EyesEmotionSequencer sequencer(player, sequencer_opt);

  const eyes::Emotion emotions[] = {
      eyes::Emotion::Curiosity,    eyes::Emotion::Happiness,  eyes::Emotion::Neutral,
      eyes::Emotion::Exhaustion,   eyes::Emotion::FallingAsleep, eyes::Emotion::Snoring,
      eyes::Emotion::WakingUp,     eyes::Emotion::PlayfulPong, eyes::Emotion::Dizzy,
      eyes::Emotion::Neutral,
  };

  uint32_t variation_seed = 0x6D2B79F5u;
  while (true) {
    for (size_t i = 0; i < sizeof(emotions) / sizeof(emotions[0]); ++i) {
      const eyes::Emotion e = emotions[i];
      variation_seed = variation_seed * 1664525u + 1013904223u;
      eyes::ClipOptions clip_opt = options;
      clip_opt.variation_seed = variation_seed;
      ESP_LOGI(TAG, "Emotion: %s", eyes::emotion_name(e));
      if (!sequencer.play(e, clip_opt)) {
        ESP_LOGE(TAG, "Clip failed: %s", eyes::emotion_name(e));
        return false;
      }
    }
  }
}

} // namespace eyes_cycle
