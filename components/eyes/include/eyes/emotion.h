#ifndef EYES_EMOTION_H
#define EYES_EMOTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace eyes {

enum class Emotion : uint8_t {
  Neutral = 0,
  Happiness = 1,
  Curiosity = 2,
  Exhaustion = 3,
  FallingAsleep = 4,
  WakingUp = 5,
  Snoring = 6,
  Dizzy = 7,
  PlayfulPong = 8,
};

struct EmotionClip {
  Emotion emotion;
  uint32_t duration_ms;
};

// Sustained playback loop for an emotion timeline.
// `from_ms` is inclusive and `to_ms` is exclusive.
// Intended runtime behavior:
// - play [0, to_ms) once
// - then loop [from_ms, to_ms) until transition
struct EmotionLoopWindow {
  uint32_t from_ms;
  uint32_t to_ms;
};

constexpr size_t kEmotionCount = 9;

EmotionClip emotion_clip(Emotion emotion);
EmotionLoopWindow emotion_loop_window(Emotion emotion);
uint32_t emotion_runtime_time_ms(Emotion emotion, uint32_t elapsed_ms);
const char *emotion_name(Emotion emotion);
bool emotion_from_string(const char *name, Emotion *out);

} // namespace eyes

#endif
