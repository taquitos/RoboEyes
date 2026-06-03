#ifndef EYES_EASING_H
#define EYES_EASING_H

namespace eyes {

inline float clamp(float v, float lo, float hi) {
  if (v < lo) {
    return lo;
  }
  if (v > hi) {
    return hi;
  }
  return v;
}

inline float clamp01(float v) {
  return clamp(v, 0.0f, 1.0f);
}

inline float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

inline float smoothstep(float edge0, float edge1, float x) {
  float t = clamp01((x - edge0) / (edge1 - edge0));
  return t * t * (3.0f - 2.0f * t);
}

inline float ease_in_out(float t) {
  return smoothstep(0.0f, 1.0f, t);
}

inline float ping_pong01(float t) {
  // 0..1 -> 0..1..0 with a period of 1.
  float x = t;
  if (x < 0.0f) {
    x = 0.0f;
  }
  if (x > 1.0f) {
    x = 1.0f;
  }
  return (x <= 0.5f) ? (x * 2.0f) : ((1.0f - x) * 2.0f);
}

} // namespace eyes

#endif
