#ifndef EYES_POSE_H
#define EYES_POSE_H

#include <stdint.h>

namespace eyes {

struct EyeLids {
  // 0 = open, 1 = closed
  float upper = 0.0f;
  float lower = 0.0f;
};

struct EyePose {
  EyeLids lids;
  float pupil_x = 0.0f; // -1..1
  float pupil_y = 0.0f; // -1..1
  float iris_scale = 1.0f;
  float pupil_scale = 1.0f;
  float brow_raise = 0.0f; // -1..1
  float brow_tilt = 0.0f;  // -1..1
};

struct SnoreZsPose {
  // Stream of five 'Z' glyphs. Each entry is a 0..1 progress value where:
  // 0.0 = near the right eye, 1.0 = moved off-screen up/right.
  uint8_t count = 0;
  float progress[5] = {};
};

struct DizzyBirdsPose {
  // Orbiting "bird" glyphs above the eyes. Each entry is a 0..1 phase around the orbit.
  uint8_t count = 0;
  float phase[3] = {};
  // 0..1 progress for fly-away when transitioning out of dizzy.
  float fly_away[3] = {};
};

struct PongBallPose {
  // 1 while the PlayfulPong effect is active.
  uint8_t active = 0;
  // Ball center normalized to [-1, 1] in screen space.
  float x = 0.0f;
  float y = 0.0f;
  // Radius as a fraction of min(width, height), typically 0.0..0.1.
  float radius = 0.0f;
  // 0 = circle, 1 = teardrop-like connected tail from the right eye.
  float droplet = 0.0f;

  // 0..1 blend from normal eyes into Pong paddles.
  float paddle_morph = 0.0f;
  // Paddle vertical centers normalized to [-1, 1] in screen space.
  float left_paddle_y = 0.0f;
  float right_paddle_y = 0.0f;
};

struct EffectsPose {
  SnoreZsPose snore_zs;
  DizzyBirdsPose dizzy_birds;
  PongBallPose pong_ball;
};

struct EyesPose {
  EyePose left;
  EyePose right;
  EffectsPose effects;
};

} // namespace eyes

#endif
