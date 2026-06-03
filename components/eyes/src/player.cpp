#include "eyes/player.h"

#include <stddef.h>

#include <vector>

#include "eyes/easing.h"
#include "eyes/pixel_buffer.h"
#include "eyes/transition.h"

namespace eyes {

namespace {

uint32_t normalize_loop_from_ms(Emotion emotion, uint32_t clip_duration_ms) {
  EmotionLoopWindow loop = emotion_loop_window(emotion);
  uint32_t to_ms = loop.to_ms;
  if (to_ms == 0 || to_ms > clip_duration_ms) {
    to_ms = clip_duration_ms;
  }
  uint32_t from_ms = loop.from_ms;
  if (from_ms >= to_ms) {
    from_ms = (to_ms > 0) ? (to_ms - 1u) : 0u;
  }
  return from_ms;
}

uint32_t normalize_loop_to_ms(Emotion emotion, uint32_t clip_duration_ms) {
  EmotionLoopWindow loop = emotion_loop_window(emotion);
  uint32_t to_ms = loop.to_ms;
  if (to_ms == 0 || to_ms > clip_duration_ms) {
    to_ms = clip_duration_ms;
  }
  return to_ms;
}

uint32_t sample_time_ms(Emotion emotion, const ClipOptions &options, uint32_t clip_duration_ms,
                        uint32_t playback_duration_ms, uint32_t elapsed_ms) {
  if (!options.sustain.enabled) {
    return elapsed_ms;
  }

  const uint32_t effective_elapsed_ms = options.sustain.start_elapsed_ms + elapsed_ms;
  uint32_t mapped_ms = emotion_runtime_time_ms(emotion, effective_elapsed_ms);
  if (!options.sustain.play_outro_on_end) {
    return mapped_ms;
  }

  uint32_t loop_to_ms = normalize_loop_to_ms(emotion, clip_duration_ms);
  uint32_t loop_from_ms = normalize_loop_from_ms(emotion, clip_duration_ms);
  if (loop_to_ms <= loop_from_ms) {
    return mapped_ms;
  }

  // Tail after loop end is the authored outro segment.
  uint32_t outro_len_ms = (clip_duration_ms > loop_to_ms) ? (clip_duration_ms - loop_to_ms) : 0u;
  if (outro_len_ms == 0 || playback_duration_ms <= outro_len_ms) {
    return mapped_ms;
  }

  uint32_t hold_until_ms = playback_duration_ms - outro_len_ms;
  if (elapsed_ms < hold_until_ms) {
    return mapped_ms;
  }

  uint32_t outro_elapsed_ms = elapsed_ms - hold_until_ms;
  uint32_t outro_t_ms = loop_to_ms + outro_elapsed_ms;
  if (outro_t_ms >= clip_duration_ms) {
    outro_t_ms = (clip_duration_ms > 0) ? (clip_duration_ms - 1u) : 0u;
  }
  return outro_t_ms;
}

// When transitioning out of snoring, keep existing Z glyphs drifting at the same
// velocity as the snoring animation (no acceleration), and stop spawning new ones.
void preserve_snore_exit_velocity(const ClipOptions &options, Emotion to_emotion, uint32_t t_ms,
                                  EyesPose *pose) {
  if (!pose) {
    return;
  }
  if (!options.transition.enabled) {
    return;
  }
  if (to_emotion == Emotion::Snoring) {
    return;
  }

  const SnoreZsPose &from = options.transition.from.effects.snore_zs;
  if (from.count == 0) {
    return;
  }

  uint32_t snore_duration_ms = emotion_clip(Emotion::Snoring).duration_ms;
  if (snore_duration_ms == 0) {
    snore_duration_ms = 5000;
  }

  float advance = static_cast<float>(t_ms) / static_cast<float>(snore_duration_ms);
  pose->effects.snore_zs.count = from.count;
  for (int i = 0; i < 5; ++i) {
    if (i < static_cast<int>(from.count)) {
      pose->effects.snore_zs.progress[i] = clamp01(from.progress[i] + advance);
    } else {
      pose->effects.snore_zs.progress[i] = 1.0f;
    }
  }
}

// When transitioning out of dizzy, keep birds orbiting in the same direction/tempo
// and fly them away up/right instead of blending their phase backward.
void preserve_dizzy_exit_motion(const ClipOptions &options, Emotion to_emotion, uint32_t t_ms,
                                EyesPose *pose) {
  if (!pose) {
    return;
  }
  if (!options.transition.enabled) {
    return;
  }
  if (to_emotion == Emotion::Dizzy) {
    return;
  }

  const DizzyBirdsPose &from = options.transition.from.effects.dizzy_birds;
  if (from.count == 0) {
    return;
  }

  uint32_t dizzy_duration_ms = emotion_clip(Emotion::Dizzy).duration_ms;
  if (dizzy_duration_ms == 0) {
    dizzy_duration_ms = 5000;
  }

  float t01 = static_cast<float>(t_ms) / static_cast<float>(dizzy_duration_ms);
  float orbit_advance = t01 * 1.85f; // matches animator's dizzy bird orbit speed.

  float fly_u = 1.0f;
  if (options.transition.duration_ms > 0) {
    fly_u = clamp01(static_cast<float>(t_ms) / static_cast<float>(options.transition.duration_ms));
  }

  pose->effects.dizzy_birds.count = from.count;
  for (int i = 0; i < 3; ++i) {
    if (i < static_cast<int>(from.count)) {
      pose->effects.dizzy_birds.phase[i] = from.phase[i] + orbit_advance;
      pose->effects.dizzy_birds.fly_away[i] = fly_u;
    } else {
      pose->effects.dizzy_birds.phase[i] = 0.0f;
      pose->effects.dizzy_birds.fly_away[i] = 1.0f;
    }
  }
}

EyesPose compose_pose_for_time(const EyesAnimator &animator, Emotion emotion, const ClipOptions &options,
                               uint32_t transition_ms, uint32_t clip_duration_ms,
                               uint32_t playback_duration_ms, uint32_t t_ms) {
  uint32_t sampled_t_ms = sample_time_ms(emotion, options, clip_duration_ms, playback_duration_ms, t_ms);
  EyesPose pose = animator.pose_for(emotion, sampled_t_ms, options.variation_seed);
  if (transition_ms > 0 && t_ms < transition_ms) {
    float u = static_cast<float>(t_ms) / static_cast<float>(transition_ms);
    pose = blend_poses(options.transition.from, pose, smoothstep(0.0f, 1.0f, u));
    preserve_snore_exit_velocity(options, emotion, t_ms, &pose);
    preserve_dizzy_exit_motion(options, emotion, t_ms, &pose);
  }
  return pose;
}

} // namespace

const IClock &EyesClipPlayer::default_clock() {
  static SteadyClock clock;
  return clock;
}

bool EyesClipPlayer::play(Emotion emotion, const ClipOptions &options) const {
  return play(emotion, options, nullptr);
}

bool EyesClipPlayer::play(Emotion emotion, const ClipOptions &options, EyesPose *out_end_pose) const {
  if (options.size.width <= 0 || options.size.height <= 0) {
    return false;
  }
  if (options.fps <= 0 || options.fps > 120) {
    return false;
  }

  uint32_t clip_duration_ms = emotion_clip(emotion).duration_ms;
  uint32_t duration_ms = options.duration_ms_override;
  if (duration_ms == 0) {
    duration_ms = clip_duration_ms;
  }
  if (duration_ms == 0) {
    return false;
  }
  if (clip_duration_ms == 0) {
    clip_duration_ms = duration_ms;
  }

  std::vector<uint8_t> buf(static_cast<size_t>(options.size.width) *
                           static_cast<size_t>(options.size.height));
  PixelBuffer8 frame(buf.data(), options.size.width, options.size.height, options.size.width);

  if (!sink_.begin(options.size.width, options.size.height, options.fps)) {
    return false;
  }

  uint32_t frame_ms = static_cast<uint32_t>(1000 / options.fps);
  if (frame_ms == 0) {
    frame_ms = 1;
  }

  uint32_t transition_ms = 0;
  if (options.transition.enabled) {
    transition_ms = options.transition.duration_ms;
    if (transition_ms > duration_ms) {
      transition_ms = duration_ms;
    }
  }

  if (!options.realtime) {
    uint32_t t_ms = 0;
    while (t_ms < duration_ms) {
      uint32_t next_ms = t_ms + frame_ms;
      uint32_t slice_ms = (next_ms <= duration_ms) ? frame_ms : (duration_ms - t_ms);

      EyesPose pose = compose_pose_for_time(animator_, emotion, options, transition_ms, clip_duration_ms,
                                            duration_ms, t_ms);
      renderer_.render(pose, frame);

      if (!sink_.frame(frame.data(), frame.stride(), slice_ms)) {
        (void)sink_.end();
        return false;
      }

      t_ms = next_ms;
    }
    if (out_end_pose) {
      uint32_t end_t = (duration_ms > 0) ? (duration_ms - 1) : 0;
      *out_end_pose = compose_pose_for_time(animator_, emotion, options, transition_ms, clip_duration_ms,
                                            duration_ms, end_t);
    }
    return sink_.end();
  }

  // Real-time mode: compute pose time from wall-clock to prevent animations from "slowing down"
  // if the sink can't sustain the requested FPS.
  uint64_t start_us = clock_.now_us();
  while (true) {
    uint64_t now_us = clock_.now_us();
    uint64_t elapsed_us = (now_us >= start_us) ? (now_us - start_us) : 0;
    uint64_t elapsed_ms64 = elapsed_us / 1000;
    if (elapsed_ms64 >= duration_ms) {
      break;
    }

    uint32_t t_ms = static_cast<uint32_t>(elapsed_ms64);
    uint32_t remaining_ms = duration_ms - t_ms;
    uint32_t slice_ms = (remaining_ms < frame_ms) ? remaining_ms : frame_ms;

    EyesPose pose = compose_pose_for_time(animator_, emotion, options, transition_ms, clip_duration_ms,
                                          duration_ms, t_ms);
    renderer_.render(pose, frame);

    if (!sink_.frame(frame.data(), frame.stride(), slice_ms)) {
      (void)sink_.end();
      return false;
    }
  }

  if (out_end_pose) {
    uint32_t end_t = (duration_ms > 0) ? (duration_ms - 1) : 0;
    *out_end_pose = compose_pose_for_time(animator_, emotion, options, transition_ms, clip_duration_ms,
                                          duration_ms, end_t);
  }
  return sink_.end();
}

} // namespace eyes
