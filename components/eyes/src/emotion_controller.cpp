#include "eyes/emotion_controller.h"

namespace eyes {

namespace {

SequencerOptions make_sequencer_options(uint32_t transition_ms) {
  SequencerOptions opt;
  opt.enable_transitions = true;
  opt.transition_ms = transition_ms;
  return opt;
}

} // namespace

EmotionController::EmotionController(const EyesClipPlayer &player, EmotionControllerConfig config)
    : config_(config), sequencer_(player, make_sequencer_options(config.transition_ms)) {
  if (config_.hold_segment_ms == 0) {
    config_.hold_segment_ms = 1;
  }
}

void EmotionController::set_emotion(Emotion emotion, uint32_t variation_seed) {
  if (!has_current_emotion_) {
    has_current_emotion_ = true;
    current_emotion_ = emotion;
    current_variation_seed_ = variation_seed;
    elapsed_ms_in_current_ = 0;
    pending_.valid = false;
    return;
  }

  if (current_emotion_ == emotion) {
    current_variation_seed_ = variation_seed;
    pending_.valid = false;
    return;
  }

  pending_.valid = true;
  pending_.next_emotion = emotion;
  pending_.variation_seed = variation_seed;
  pending_.with_release_outro = false;
}

void EmotionController::release_to_next(Emotion next_emotion, uint32_t variation_seed) {
  if (!has_current_emotion_) {
    set_emotion(next_emotion, variation_seed);
    return;
  }

  pending_.valid = true;
  pending_.next_emotion = next_emotion;
  pending_.variation_seed = variation_seed;
  pending_.with_release_outro = true;
}

uint32_t EmotionController::release_segment_ms() const {
  const EmotionClip clip = emotion_clip(current_emotion_);
  EmotionLoopWindow loop = emotion_loop_window(current_emotion_);
  uint32_t loop_to_ms = loop.to_ms;
  if (loop_to_ms == 0 || loop_to_ms > clip.duration_ms) {
    loop_to_ms = clip.duration_ms;
  }

  const uint32_t outro_len_ms = (clip.duration_ms > loop_to_ms) ? (clip.duration_ms - loop_to_ms) : 0u;
  if (!config_.play_outro_on_release || outro_len_ms == 0) {
    return config_.release_hold_ms;
  }

  uint32_t seg_ms = config_.release_hold_ms + outro_len_ms;
  if (seg_ms <= outro_len_ms) {
    seg_ms = outro_len_ms + 1u;
  }
  return seg_ms;
}

bool EmotionController::play_current_segment(const ClipOptions &base_options, uint32_t segment_ms,
                                             bool play_outro_on_end) {
  if (!has_current_emotion_) {
    return false;
  }

  if (segment_ms == 0) {
    segment_ms = 1;
  }

  ClipOptions opt = base_options;
  opt.duration_ms_override = segment_ms;
  opt.variation_seed = current_variation_seed_;
  opt.sustain.enabled = true;
  opt.sustain.play_outro_on_end = play_outro_on_end;
  opt.sustain.start_elapsed_ms = elapsed_ms_in_current_;

  if (!sequencer_.play(current_emotion_, opt)) {
    return false;
  }

  elapsed_ms_in_current_ += segment_ms;
  return true;
}

bool EmotionController::step(const ClipOptions &base_options) {
  if (!has_current_emotion_ && !pending_.valid) {
    return false;
  }

  if (!has_current_emotion_ && pending_.valid) {
    has_current_emotion_ = true;
    current_emotion_ = pending_.next_emotion;
    current_variation_seed_ = pending_.variation_seed;
    elapsed_ms_in_current_ = 0;
    pending_.valid = false;
  }

  if (pending_.valid && pending_.with_release_outro) {
    uint32_t seg_ms = release_segment_ms();
    if (seg_ms > 0) {
      if (!play_current_segment(base_options, seg_ms, true)) {
        return false;
      }
    }
  }

  if (pending_.valid) {
    current_emotion_ = pending_.next_emotion;
    current_variation_seed_ = pending_.variation_seed;
    elapsed_ms_in_current_ = 0;
    pending_.valid = false;
  }

  return play_current_segment(base_options, config_.hold_segment_ms, false);
}

} // namespace eyes
