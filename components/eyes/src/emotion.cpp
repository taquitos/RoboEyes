#include "eyes/emotion.h"

#include <ctype.h>
#include <string.h>

namespace eyes {

EmotionClip emotion_clip(Emotion emotion) {
  switch (emotion) {
  case Emotion::Neutral:
    return {emotion, 4000};
  case Emotion::Happiness:
    return {emotion, 4000};
  case Emotion::Curiosity:
    return {emotion, 3500};
  case Emotion::Exhaustion:
    return {emotion, 4500};
  case Emotion::FallingAsleep:
    return {emotion, 5000};
  case Emotion::WakingUp:
    return {emotion, 3000};
  case Emotion::Snoring:
    return {emotion, 5000};
  case Emotion::Dizzy:
    return {emotion, 5000};
  case Emotion::PlayfulPong:
    return {emotion, 5000};
  default:
    return {Emotion::Neutral, 4000};
  }
}

EmotionLoopWindow emotion_loop_window(Emotion emotion) {
  // For sustained behavior:
  // - Keep "state" emotions looping naturally across the full clip.
  // - For transition-like emotions, loop only the settled hold/game section.
  switch (emotion) {
  case Emotion::Neutral:
    return {0, 4000};
  case Emotion::Happiness:
    return {0, 4000};
  case Emotion::Curiosity:
    return {3150, 3500}; // after tilt+close+reopen, hold curious-tall look
  case Emotion::Exhaustion:
    return {0, 4500};
  case Emotion::FallingAsleep:
    return {3400, 5000}; // late drowsy/near-closed hold
  case Emotion::WakingUp:
    return {2700, 3000}; // fully awake end hold
  case Emotion::Snoring:
    return {0, 5000};
  case Emotion::Dizzy:
    return {0, 5000};
  case Emotion::PlayfulPong:
    return {1500, 4200}; // sustained pong gameplay segment
  default:
    return {0, 4000};
  }
}

uint32_t emotion_runtime_time_ms(Emotion emotion, uint32_t elapsed_ms) {
  const EmotionClip clip = emotion_clip(emotion);
  uint32_t duration_ms = clip.duration_ms;
  if (duration_ms == 0) {
    return 0;
  }

  EmotionLoopWindow loop = emotion_loop_window(emotion);
  if (loop.to_ms == 0 || loop.to_ms > duration_ms) {
    loop.to_ms = duration_ms;
  }
  if (loop.from_ms >= loop.to_ms) {
    loop.from_ms = (loop.to_ms > 0) ? (loop.to_ms - 1u) : 0u;
  }

  // First pass includes intro and one full run to loop end.
  if (elapsed_ms < loop.to_ms) {
    return elapsed_ms;
  }

  // Then repeat only the sustain window.
  const uint32_t loop_len = loop.to_ms - loop.from_ms;
  if (loop_len == 0) {
    return loop.from_ms;
  }
  return loop.from_ms + ((elapsed_ms - loop.from_ms) % loop_len);
}

const char *emotion_name(Emotion emotion) {
  switch (emotion) {
  case Emotion::Neutral:
    return "neutral";
  case Emotion::Happiness:
    return "happiness";
  case Emotion::Curiosity:
    return "curiosity";
  case Emotion::Exhaustion:
    return "exhaustion";
  case Emotion::FallingAsleep:
    return "falling_asleep";
  case Emotion::WakingUp:
    return "waking_up";
  case Emotion::Snoring:
    return "snoring";
  case Emotion::Dizzy:
    return "dizzy";
  case Emotion::PlayfulPong:
    return "playful_pong";
  default:
    return "unknown";
  }
}

static bool equals_ignore_case(const char *a, const char *b) {
  if (!a || !b) {
    return false;
  }
  while (*a && *b) {
    if (tolower(static_cast<unsigned char>(*a)) != tolower(static_cast<unsigned char>(*b))) {
      return false;
    }
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

bool emotion_from_string(const char *name, Emotion *out) {
  if (!name || !out) {
    return false;
  }

  if (equals_ignore_case(name, "neutral")) {
    *out = Emotion::Neutral;
    return true;
  }
  if (equals_ignore_case(name, "happiness") || equals_ignore_case(name, "happy")) {
    *out = Emotion::Happiness;
    return true;
  }
  if (equals_ignore_case(name, "curiosity") || equals_ignore_case(name, "curious")) {
    *out = Emotion::Curiosity;
    return true;
  }
  if (equals_ignore_case(name, "exhaustion") || equals_ignore_case(name, "exhausted")) {
    *out = Emotion::Exhaustion;
    return true;
  }
  if (equals_ignore_case(name, "falling_asleep") || equals_ignore_case(name, "falling-asleep") ||
      equals_ignore_case(name, "fallingasleep") || equals_ignore_case(name, "sleepy")) {
    *out = Emotion::FallingAsleep;
    return true;
  }
  if (equals_ignore_case(name, "waking_up") || equals_ignore_case(name, "wakingup") ||
      equals_ignore_case(name, "waking-up") || equals_ignore_case(name, "wake")) {
    *out = Emotion::WakingUp;
    return true;
  }
  if (equals_ignore_case(name, "snoring") || equals_ignore_case(name, "snore")) {
    *out = Emotion::Snoring;
    return true;
  }
  if (equals_ignore_case(name, "dizzy")) {
    *out = Emotion::Dizzy;
    return true;
  }
  if (equals_ignore_case(name, "playful_pong") || equals_ignore_case(name, "playfulpong") ||
      equals_ignore_case(name, "pong")) {
    *out = Emotion::PlayfulPong;
    return true;
  }

  return false;
}

} // namespace eyes
