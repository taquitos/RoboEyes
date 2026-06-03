#include "eyes/sequencer.h"

#include "eyes/emotion.h"

namespace eyes {

bool EyesEmotionSequencer::play(Emotion emotion, const ClipOptions &options) {
  ClipOptions opt = options;

  if (options_.enable_transitions && has_pose_) {
    uint32_t duration_ms = options.duration_ms_override;
    if (duration_ms == 0) {
      duration_ms = emotion_clip(emotion).duration_ms;
    }

    uint32_t transition_ms = options_.transition_ms;
    if (duration_ms > 0) {
      uint32_t max_transition = duration_ms / 2;
      if (max_transition == 0) {
        max_transition = 1;
      }
      if (transition_ms > max_transition) {
        transition_ms = max_transition;
      }
    }

    if (transition_ms > 0) {
      opt.transition.enabled = true;
      opt.transition.from = pose_;
      opt.transition.duration_ms = transition_ms;
    }
  }

  EyesPose out_pose;
  if (!player_.play(emotion, opt, &out_pose)) {
    return false;
  }

  pose_ = out_pose;
  has_pose_ = true;
  return true;
}

} // namespace eyes

