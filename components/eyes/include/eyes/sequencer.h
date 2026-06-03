#ifndef EYES_SEQUENCER_H
#define EYES_SEQUENCER_H

#include <stdint.h>

#include "eyes/player.h"

namespace eyes {

struct SequencerOptions {
  bool enable_transitions = true;
  uint32_t transition_ms = 420;
};

// Stateful helper for clip sequences. It keeps track of the last emitted pose and
// can smoothly blend from that pose into the next emotion clip.
class EyesEmotionSequencer {
public:
  explicit EyesEmotionSequencer(const EyesClipPlayer &player, SequencerOptions options = {})
      : player_(player), options_(options) {
  }

  bool play(Emotion emotion, const ClipOptions &options);

  bool has_pose() const {
    return has_pose_;
  }

  const EyesPose &current_pose() const {
    return pose_;
  }

  void clear() {
    has_pose_ = false;
    pose_ = EyesPose();
  }

  void set_pose(const EyesPose &pose) {
    pose_ = pose;
    has_pose_ = true;
  }

private:
  const EyesClipPlayer &player_;
  SequencerOptions options_;
  bool has_pose_ = false;
  EyesPose pose_;
};

} // namespace eyes

#endif

