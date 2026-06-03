#ifndef EYES_ANIMATOR_H
#define EYES_ANIMATOR_H

#include <stdint.h>

#include "eyes/emotion.h"
#include "eyes/pose.h"

namespace eyes {

class EyesAnimator {
public:
  EyesAnimator() = default;
  ~EyesAnimator() = default;

  EyesPose pose_for(Emotion emotion, uint32_t t_ms) const;
  EyesPose pose_for(Emotion emotion, uint32_t t_ms, uint32_t variation_seed) const;
  EyesPose pose_for_runtime(Emotion emotion, uint32_t elapsed_ms) const;
  EyesPose pose_for_runtime(Emotion emotion, uint32_t elapsed_ms, uint32_t variation_seed) const;
};

} // namespace eyes

#endif
