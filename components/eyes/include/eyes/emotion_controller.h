#ifndef EYES_EMOTION_CONTROLLER_H
#define EYES_EMOTION_CONTROLLER_H

#include <stdint.h>

#include "eyes/player.h"
#include "eyes/sequencer.h"

namespace eyes {

struct EmotionControllerConfig {
  uint32_t transition_ms = 420;

  // Playback quantum for held emotions.
  uint32_t hold_segment_ms = 250;

  // Extra hold time to keep before playing an authored outro during release.
  uint32_t release_hold_ms = 160;

  // If true, release_to_next() tries to play authored outro tails for the current emotion.
  bool play_outro_on_release = true;
};

class EmotionController {
public:
  explicit EmotionController(const EyesClipPlayer &player, EmotionControllerConfig config = {});

  // Sets/overrides the active emotion target. If another emotion is currently active,
  // the controller transitions to this emotion on the next step.
  void set_emotion(Emotion emotion, uint32_t variation_seed = 0);

  // Releases the currently held emotion and transitions to `next_emotion`.
  // If configured, authored outro tails are played before the transition.
  void release_to_next(Emotion next_emotion, uint32_t variation_seed = 0);

  // Executes one control step. Call this repeatedly from the app loop.
  // Returns false if playback fails or if no current/pending emotion exists.
  bool step(const ClipOptions &base_options);

  bool has_current_emotion() const {
    return has_current_emotion_;
  }

  Emotion current_emotion() const {
    return current_emotion_;
  }

  uint32_t elapsed_ms_in_current() const {
    return elapsed_ms_in_current_;
  }

  bool has_pending_transition() const {
    return pending_.valid;
  }

private:
  struct PendingTransition {
    bool valid = false;
    Emotion next_emotion = Emotion::Neutral;
    uint32_t variation_seed = 0;
    bool with_release_outro = false;
  };

  bool play_current_segment(const ClipOptions &base_options, uint32_t segment_ms, bool play_outro_on_end);
  uint32_t release_segment_ms() const;

  EmotionControllerConfig config_;
  EyesEmotionSequencer sequencer_;

  bool has_current_emotion_ = false;
  Emotion current_emotion_ = Emotion::Neutral;
  uint32_t current_variation_seed_ = 0;
  uint32_t elapsed_ms_in_current_ = 0;

  PendingTransition pending_;
};

} // namespace eyes

#endif
