#include <vector>
#include <string.h>
#include <math.h>

extern "C" {
#include "unity.h"
}

#include "eyes/animator.h"
#include "eyes/emotion_controller.h"
#include "eyes/emotion.h"
#include "eyes/gif/encoder.h"
#include "eyes/gif/gif_frame_sink.h"
#include "eyes/gif/vector_byte_sink.h"
#include "eyes/player.h"
#include "eyes/renderer.h"
#include "eyes/sequencer.h"
#include "eyes/transition.h"

using eyes::Emotion;

static void assert_clip_duration_in_range(Emotion e) {
  eyes::EmotionClip clip = eyes::emotion_clip(e);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(3000, clip.duration_ms);
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(5000, clip.duration_ms);
}

TEST_CASE("eyes emotion clips are 3-5 seconds", "[eyes]") {
  assert_clip_duration_in_range(Emotion::Neutral);
  assert_clip_duration_in_range(Emotion::Happiness);
  assert_clip_duration_in_range(Emotion::Curiosity);
  assert_clip_duration_in_range(Emotion::Exhaustion);
  assert_clip_duration_in_range(Emotion::FallingAsleep);
  assert_clip_duration_in_range(Emotion::WakingUp);
  assert_clip_duration_in_range(Emotion::Snoring);
  assert_clip_duration_in_range(Emotion::Dizzy);
  assert_clip_duration_in_range(Emotion::PlayfulPong);
}

TEST_CASE("eyes loop windows are valid for every emotion", "[eyes]") {
  for (int i = 0; i < static_cast<int>(eyes::kEmotionCount); ++i) {
    Emotion e = static_cast<Emotion>(i);
    eyes::EmotionClip clip = eyes::emotion_clip(e);
    eyes::EmotionLoopWindow loop = eyes::emotion_loop_window(e);

    TEST_ASSERT_TRUE(clip.duration_ms > 0);
    TEST_ASSERT_TRUE(loop.to_ms > 0);
    TEST_ASSERT_TRUE(loop.to_ms <= clip.duration_ms);
    TEST_ASSERT_TRUE(loop.from_ms < loop.to_ms);
  }
}

TEST_CASE("eyes loop windows capture intro-plus-sustain intent", "[eyes]") {
  eyes::EmotionLoopWindow curiosity = eyes::emotion_loop_window(Emotion::Curiosity);
  TEST_ASSERT_EQUAL_UINT32(3150, curiosity.from_ms);
  TEST_ASSERT_EQUAL_UINT32(3500, curiosity.to_ms);

  eyes::EmotionLoopWindow falling_asleep = eyes::emotion_loop_window(Emotion::FallingAsleep);
  TEST_ASSERT_EQUAL_UINT32(3400, falling_asleep.from_ms);
  TEST_ASSERT_EQUAL_UINT32(5000, falling_asleep.to_ms);

  eyes::EmotionLoopWindow waking_up = eyes::emotion_loop_window(Emotion::WakingUp);
  TEST_ASSERT_EQUAL_UINT32(2700, waking_up.from_ms);
  TEST_ASSERT_EQUAL_UINT32(3000, waking_up.to_ms);

  eyes::EmotionLoopWindow playful_pong = eyes::emotion_loop_window(Emotion::PlayfulPong);
  TEST_ASSERT_EQUAL_UINT32(1500, playful_pong.from_ms);
  TEST_ASSERT_EQUAL_UINT32(4200, playful_pong.to_ms);
}

TEST_CASE("eyes runtime time mapping replays only sustain window after intro", "[eyes]") {
  for (int i = 0; i < static_cast<int>(eyes::kEmotionCount); ++i) {
    Emotion e = static_cast<Emotion>(i);
    eyes::EmotionLoopWindow loop = eyes::emotion_loop_window(e);
    uint32_t loop_len = loop.to_ms - loop.from_ms;
    TEST_ASSERT_TRUE(loop_len > 0);

    uint32_t intro_sample = (loop.to_ms > 2) ? (loop.to_ms - 2) : 0;
    TEST_ASSERT_EQUAL_UINT32(intro_sample, eyes::emotion_runtime_time_ms(e, intro_sample));

    uint32_t after_intro = loop.to_ms + 13;
    uint32_t mapped_a = eyes::emotion_runtime_time_ms(e, after_intro);
    uint32_t mapped_b = eyes::emotion_runtime_time_ms(e, after_intro + loop_len);
    TEST_ASSERT_EQUAL_UINT32(mapped_a, mapped_b);
    TEST_ASSERT_TRUE(mapped_a >= loop.from_ms);
    TEST_ASSERT_TRUE(mapped_a < loop.to_ms);
  }
}

TEST_CASE("eyes authored outro tails are currently defined for playful_pong only", "[eyes]") {
  for (int i = 0; i < static_cast<int>(eyes::kEmotionCount); ++i) {
    Emotion e = static_cast<Emotion>(i);
    eyes::EmotionClip clip = eyes::emotion_clip(e);
    eyes::EmotionLoopWindow loop = eyes::emotion_loop_window(e);
    uint32_t outro_len = (clip.duration_ms > loop.to_ms) ? (clip.duration_ms - loop.to_ms) : 0u;

    if (e == Emotion::PlayfulPong) {
      TEST_ASSERT_TRUE(outro_len > 0u);
    } else {
      TEST_ASSERT_EQUAL_UINT32(0u, outro_len);
    }
  }
}

TEST_CASE("eyes emotion_from_string accepts canonical names", "[eyes]") {
  Emotion out = Emotion::Neutral;
  TEST_ASSERT_TRUE(eyes::emotion_from_string("neutral", &out));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::Neutral), static_cast<int>(out));

  TEST_ASSERT_TRUE(eyes::emotion_from_string("happiness", &out));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::Happiness), static_cast<int>(out));

  TEST_ASSERT_TRUE(eyes::emotion_from_string("curiosity", &out));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::Curiosity), static_cast<int>(out));

  TEST_ASSERT_TRUE(eyes::emotion_from_string("exhaustion", &out));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::Exhaustion), static_cast<int>(out));

  TEST_ASSERT_TRUE(eyes::emotion_from_string("falling_asleep", &out));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::FallingAsleep), static_cast<int>(out));
  TEST_ASSERT_TRUE(eyes::emotion_from_string("sleepy", &out)); // compatibility alias
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::FallingAsleep), static_cast<int>(out));

  TEST_ASSERT_TRUE(eyes::emotion_from_string("waking_up", &out));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::WakingUp), static_cast<int>(out));

  TEST_ASSERT_TRUE(eyes::emotion_from_string("snoring", &out));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::Snoring), static_cast<int>(out));

  TEST_ASSERT_TRUE(eyes::emotion_from_string("dizzy", &out));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::Dizzy), static_cast<int>(out));

  TEST_ASSERT_TRUE(eyes::emotion_from_string("playful_pong", &out));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::PlayfulPong), static_cast<int>(out));
}

TEST_CASE("eyes animator outputs sane ranges", "[eyes]") {
  eyes::EyesAnimator animator;
  for (int i = 0; i < static_cast<int>(eyes::kEmotionCount); ++i) {
    Emotion e = static_cast<Emotion>(i);
    uint32_t d = eyes::emotion_clip(e).duration_ms;
    eyes::EyesPose p0 = animator.pose_for(e, 0);
    eyes::EyesPose p1 = animator.pose_for(e, d / 2);

    auto check_eye = [](const eyes::EyePose &p) {
      TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.0f, p.lids.upper);
      TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0f, p.lids.upper);
      TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.0f, p.lids.lower);
      TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0f, p.lids.lower);
      TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(-1.5f, p.pupil_x);
      TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.5f, p.pupil_x);
      TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(-1.5f, p.pupil_y);
      TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.5f, p.pupil_y);
    };

    check_eye(p0.left);
    check_eye(p0.right);
    check_eye(p1.left);
    check_eye(p1.right);
  }
}

TEST_CASE("eyes animator moves both eyes together for all emotions", "[eyes]") {
  eyes::EyesAnimator animator;
  for (int i = 0; i < static_cast<int>(eyes::kEmotionCount); ++i) {
    Emotion e = static_cast<Emotion>(i);
    if (e == Emotion::PlayfulPong || e == Emotion::Exhaustion) {
      continue; // Pong intentionally drives left/right paddles independently.
                // Exhaustion intentionally adds a one-eye twitch.
    }
    uint32_t d = eyes::emotion_clip(e).duration_ms;
    eyes::EyesPose p = animator.pose_for(e, d / 3);

    // Group motion: the two eyes should translate together.
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, p.left.pupil_x, p.right.pupil_x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, p.left.pupil_y, p.right.pupil_y);
  }
}

TEST_CASE("eyes happiness/curiosity/exhaustion have noticeable movement over time", "[eyes]") {
  eyes::EyesAnimator animator;
  const Emotion emotions[] = {Emotion::Happiness, Emotion::Curiosity, Emotion::Exhaustion};

  for (size_t i = 0; i < sizeof(emotions) / sizeof(emotions[0]); ++i) {
    Emotion e = emotions[i];
    uint32_t d = eyes::emotion_clip(e).duration_ms;
    eyes::EyesPose p0 = animator.pose_for(e, 0);
    eyes::EyesPose p1 = animator.pose_for(e, d / 3);

    float dx = fabsf(p1.left.pupil_x - p0.left.pupil_x);
    float dy = fabsf(p1.left.pupil_y - p0.left.pupil_y);
    TEST_ASSERT_TRUE((dx + dy) > 0.04f);
  }
}

TEST_CASE("eyes happiness and curiosity keep iris_scale below overlap threshold", "[eyes]") {
  eyes::EyesAnimator animator;
  const Emotion emotions[] = {Emotion::Happiness, Emotion::Curiosity};
  for (size_t i = 0; i < sizeof(emotions) / sizeof(emotions[0]); ++i) {
    eyes::EyesPose p = animator.pose_for(emotions[i], 0);
    TEST_ASSERT_TRUE(p.left.iris_scale <= 1.07f);
    TEST_ASSERT_TRUE(p.right.iris_scale <= 1.07f);
  }
}

TEST_CASE("eyes curiosity smoothly tilts, then closes and reopens into taller eyes", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::Curiosity).duration_ms;

  eyes::EyesPose p_pre = animator.pose_for(Emotion::Curiosity, static_cast<uint32_t>(duration * 0.52f));
  eyes::EyesPose p_mid = animator.pose_for(Emotion::Curiosity, static_cast<uint32_t>(duration * 0.64f));
  eyes::EyesPose p_tilt = animator.pose_for(Emotion::Curiosity, static_cast<uint32_t>(duration * 0.74f));
  eyes::EyesPose p_closed = animator.pose_for(Emotion::Curiosity, static_cast<uint32_t>(duration * 0.82f));
  eyes::EyesPose p_reopen_fast = animator.pose_for(Emotion::Curiosity, static_cast<uint32_t>(duration * 0.89f));
  eyes::EyesPose p_tall = animator.pose_for(Emotion::Curiosity, static_cast<uint32_t>(duration * 0.94f));

  // Tilt should ramp up smoothly (no snap).
  TEST_ASSERT_TRUE(p_mid.left.brow_tilt > p_pre.left.brow_tilt + 0.15f);
  TEST_ASSERT_TRUE(p_tilt.left.brow_tilt > p_mid.left.brow_tilt + 0.10f);

  // Close after tilt...
  TEST_ASSERT_TRUE(p_closed.left.lids.upper > 0.75f);
  TEST_ASSERT_TRUE(p_closed.left.lids.lower > 0.55f);

  // ...then reopen into taller eyes.
  TEST_ASSERT_TRUE(p_reopen_fast.left.lids.upper < 0.30f);
  TEST_ASSERT_TRUE(p_tall.left.lids.upper < 0.45f);
  TEST_ASSERT_TRUE(p_tall.left.pupil_scale > 1.50f);
  TEST_ASSERT_TRUE(p_tall.left.pupil_scale > p_pre.left.pupil_scale + 0.45f);
}

TEST_CASE("eyes exhaustion includes a subtle one-eye twitch", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::Exhaustion).duration_ms;

  bool saw_offset = false;
  for (int i = 0; i <= 100; ++i) {
    uint32_t t_ms = static_cast<uint32_t>((static_cast<float>(duration) * static_cast<float>(i)) / 100.0f);
    eyes::EyesPose p = animator.pose_for(Emotion::Exhaustion, t_ms);
    float dx = fabsf(p.left.pupil_x - p.right.pupil_x);
    if (dx > 0.012f) {
      saw_offset = true;
      break;
    }
  }
  TEST_ASSERT_TRUE(saw_offset);
}

TEST_CASE("removed emotions are rejected by public emotion parser", "[eyes]") {
  Emotion out = Emotion::Neutral;
  TEST_ASSERT_FALSE(eyes::emotion_from_string("proud", &out));
  TEST_ASSERT_FALSE(eyes::emotion_from_string("suspicious", &out));
  TEST_ASSERT_FALSE(eyes::emotion_from_string("mischief", &out));
}

TEST_CASE("eyes dizzy emits three orbiting birds", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::Dizzy).duration_ms;
  eyes::EyesPose p = animator.pose_for(Emotion::Dizzy, duration / 3);
  TEST_ASSERT_EQUAL_UINT8(3, p.effects.dizzy_birds.count);
  for (int i = 0; i < 3; ++i) {
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.0f, p.effects.dizzy_birds.phase[i]);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0f, p.effects.dizzy_birds.phase[i]);
  }
}

TEST_CASE("eyes playful_pong morphs to paddles with ball and returns to eyes", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::PlayfulPong).duration_ms;

  eyes::EyesPose p_start = animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * 0.05f));
  TEST_ASSERT_EQUAL_UINT8(1, p_start.effects.pong_ball.active);
  TEST_ASSERT_TRUE(p_start.effects.pong_ball.droplet > 0.35f);

  eyes::EyesPose p_mid = animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * 0.40f));
  TEST_ASSERT_TRUE(p_mid.left.iris_scale < 0.55f);
  TEST_ASSERT_TRUE(p_mid.right.iris_scale < 0.55f);
  TEST_ASSERT_TRUE(fabsf(p_mid.left.pupil_y - p_mid.right.pupil_y) > 0.01f);

  eyes::EyesPose p_end = animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * 0.98f));
  TEST_ASSERT_TRUE(p_end.left.iris_scale > 0.80f);
  TEST_ASSERT_TRUE(p_end.right.iris_scale > 0.80f);
}

TEST_CASE("eyes playful_pong variation seed is deterministic for identical seeds", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::PlayfulPong).duration_ms;
  const uint32_t seed = 0x12345678u;

  const float samples[] = {0.18f, 0.33f, 0.47f, 0.61f, 0.79f};
  for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); ++i) {
    uint32_t t_ms = static_cast<uint32_t>(duration * samples[i]);
    eyes::EyesPose a = animator.pose_for(Emotion::PlayfulPong, t_ms, seed);
    eyes::EyesPose b = animator.pose_for(Emotion::PlayfulPong, t_ms, seed);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, a.effects.pong_ball.x, b.effects.pong_ball.x);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, a.effects.pong_ball.y, b.effects.pong_ball.y);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, a.effects.pong_ball.left_paddle_y, b.effects.pong_ball.left_paddle_y);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, a.effects.pong_ball.right_paddle_y, b.effects.pong_ball.right_paddle_y);
  }
}

TEST_CASE("eyes playful_pong variation seed changes game trajectory", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::PlayfulPong).duration_ms;
  const uint32_t seed_a = 0x11111111u;
  const uint32_t seed_b = 0x22222222u;

  int differing_samples = 0;
  const float samples[] = {0.26f, 0.34f, 0.42f, 0.50f, 0.58f, 0.66f, 0.74f};
  for (size_t i = 0; i < sizeof(samples) / sizeof(samples[0]); ++i) {
    uint32_t t_ms = static_cast<uint32_t>(duration * samples[i]);
    eyes::EyesPose a = animator.pose_for(Emotion::PlayfulPong, t_ms, seed_a);
    eyes::EyesPose b = animator.pose_for(Emotion::PlayfulPong, t_ms, seed_b);
    float d = fabsf(a.effects.pong_ball.x - b.effects.pong_ball.x) +
              fabsf(a.effects.pong_ball.y - b.effects.pong_ball.y) +
              fabsf(a.effects.pong_ball.right_paddle_y - b.effects.pong_ball.right_paddle_y);
    if (d > 0.04f) {
      differing_samples++;
    }
  }
  TEST_ASSERT_TRUE(differing_samples >= 4);
}

TEST_CASE("eyes playful_pong vertical reversals happen at top or bottom walls", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::PlayfulPong).duration_ms;
  const uint32_t seeds[] = {0u, 0x2468ACE0u, 0x13579BDFu};

  for (size_t s = 0; s < sizeof(seeds) / sizeof(seeds[0]); ++s) {
    const uint32_t seed = seeds[s];
    const float t_start = 0.34f;
    const float t_end = 0.80f;
    const int samples = 560;

    eyes::EyesPose prev = animator.pose_for(Emotion::PlayfulPong,
                                            static_cast<uint32_t>(duration * t_start), seed);
    float prev_y = prev.effects.pong_ball.y;
    float min_y = prev_y;
    float max_y = prev_y;
    float prev_dy = 0.0f;
    bool have_prev_dy = false;
    int reversals = 0;

    for (int i = 1; i <= samples; ++i) {
      float t = t_start + (t_end - t_start) * (static_cast<float>(i) / static_cast<float>(samples));
      eyes::EyesPose cur =
          animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * t), seed);

      float y = cur.effects.pong_ball.y;
      min_y = fminf(min_y, y);
      max_y = fmaxf(max_y, y);

      float dy = y - prev_y;
      if (fabsf(dy) > 0.0005f) {
        if (have_prev_dy && ((prev_dy > 0.0f && dy < 0.0f) || (prev_dy < 0.0f && dy > 0.0f))) {
          reversals++;
          if (dy < 0.0f) {
            // Upward motion flipped to downward: must be at/near top wall.
            TEST_ASSERT_TRUE(prev_y > 0.98f || y > 0.98f);
          } else {
            // Downward motion flipped to upward: must be at/near bottom wall.
            TEST_ASSERT_TRUE(prev_y < -0.98f || y < -0.98f);
          }
        }
        prev_dy = dy;
        have_prev_dy = true;
      }
      prev_y = y;
    }

    TEST_ASSERT_TRUE(max_y > 1.04f);
    TEST_ASSERT_TRUE(min_y < -1.04f);
    TEST_ASSERT_TRUE(reversals >= 2);
  }
}

TEST_CASE("eyes playful_pong only moves the receiving paddle", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::PlayfulPong).duration_ms;

  int toward_right_samples = 0;
  int toward_left_samples = 0;

  for (int i = 0; i < 18; ++i) {
    float t0 = 0.36f + 0.40f * (static_cast<float>(i) / 17.0f);
    float t1 = t0 + 0.006f;
    eyes::EyesPose a = animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * t0));
    eyes::EyesPose b = animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * t1));

    float dx = b.effects.pong_ball.x - a.effects.pong_ball.x;
    if (fabsf(dx) < 0.015f) {
      continue; // skip near turnarounds
    }

    float d_left = fabsf(b.effects.pong_ball.left_paddle_y - a.effects.pong_ball.left_paddle_y);
    float d_right = fabsf(b.effects.pong_ball.right_paddle_y - a.effects.pong_ball.right_paddle_y);

    if (dx > 0.0f) {
      toward_right_samples++;
      TEST_ASSERT_TRUE(d_right > d_left + 0.001f);
    } else {
      toward_left_samples++;
      TEST_ASSERT_TRUE(d_left > d_right + 0.001f);
    }
  }

  TEST_ASSERT_TRUE(toward_right_samples >= 3);
  TEST_ASSERT_TRUE(toward_left_samples >= 3);
}

TEST_CASE("eyes playful_pong paddle tracking stays smooth through direction flips", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::PlayfulPong).duration_ms;

  const float t_start = 0.36f;
  const float t_end = 0.78f;
  const int samples = 180;

  eyes::EyesPose prev = animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * t_start));
  float prev_dx = 0.0f;
  bool have_prev_dx = false;
  int flips = 0;

  for (int i = 1; i <= samples; ++i) {
    float t = t_start + (t_end - t_start) * (static_cast<float>(i) / static_cast<float>(samples));
    eyes::EyesPose cur = animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * t));

    float dx = cur.effects.pong_ball.x - prev.effects.pong_ball.x;
    float d_left = fabsf(cur.effects.pong_ball.left_paddle_y - prev.effects.pong_ball.left_paddle_y);
    float d_right = fabsf(cur.effects.pong_ball.right_paddle_y - prev.effects.pong_ball.right_paddle_y);

    // No visible per-frame jumps while the game is active.
    TEST_ASSERT_TRUE(d_left < 0.15f);
    TEST_ASSERT_TRUE(d_right < 0.15f);

    if (have_prev_dx && ((prev_dx > 0.0f && dx < 0.0f) || (prev_dx < 0.0f && dx > 0.0f))) {
      flips++;
      // The new receiving paddle should begin moving smoothly, not jump.
      if (dx < 0.0f) {
        TEST_ASSERT_TRUE(d_left < 0.11f);
      } else {
        TEST_ASSERT_TRUE(d_right < 0.11f);
      }
    }

    if (fabsf(dx) > 0.0005f) {
      prev_dx = dx;
      have_prev_dx = true;
    }
    prev = cur;
  }

  TEST_ASSERT_TRUE(flips >= 3);
}

TEST_CASE("eyes playful_pong last direction flip keeps receiving paddle continuous", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::PlayfulPong).duration_ms;

  // Last wrap in the play window occurs near xphase=2.0.
  const float game_u_wrap = (2.0f - 0.08f) / 2.15f;
  const float t_wrap = 0.30f + 0.54f * game_u_wrap;

  eyes::EyesPose p_pre0 =
      animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * (t_wrap - 0.012f)));
  eyes::EyesPose p_pre1 =
      animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * (t_wrap - 0.004f)));
  eyes::EyesPose p_post0 =
      animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * (t_wrap + 0.004f)));
  eyes::EyesPose p_post1 =
      animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * (t_wrap + 0.012f)));

  // Ball should be moving left before the wrap, then right after the wrap.
  TEST_ASSERT_TRUE(p_pre1.effects.pong_ball.x < p_pre0.effects.pong_ball.x);
  TEST_ASSERT_TRUE(p_post1.effects.pong_ball.x > p_post0.effects.pong_ball.x);

  // Receiving paddle (right) should not jump at the flip.
  float right_jump = fabsf(p_post0.effects.pong_ball.right_paddle_y - p_pre1.effects.pong_ball.right_paddle_y);
  TEST_ASSERT_TRUE(right_jump < 0.07f);
}

TEST_CASE("eyes sustain loop can hold playful_pong in gameplay or release through outro", "[eyes]") {
  class LocalNoopSink final : public eyes::IFrameSink {
  public:
    bool begin(int width, int height, int fps) override {
      (void)width;
      (void)height;
      (void)fps;
      return true;
    }
    bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) override {
      (void)grayscale;
      (void)stride;
      (void)duration_ms;
      return true;
    }
    bool end() override {
      return true;
    }
  };

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  LocalNoopSink sink;
  eyes::EyesClipPlayer player(animator, renderer, sink);

  eyes::ClipOptions hold_opt;
  hold_opt.size = {64, 64};
  hold_opt.fps = 30;
  hold_opt.duration_ms_override = 9000;
  hold_opt.sustain.enabled = true;
  hold_opt.sustain.play_outro_on_end = false;

  eyes::EyesPose hold_end;
  TEST_ASSERT_TRUE(player.play(Emotion::PlayfulPong, hold_opt, &hold_end));

  eyes::ClipOptions release_opt = hold_opt;
  release_opt.sustain.play_outro_on_end = true;

  eyes::EyesPose release_end;
  TEST_ASSERT_TRUE(player.play(Emotion::PlayfulPong, release_opt, &release_end));

  // Holding without outro should keep us in paddle/game geometry.
  TEST_ASSERT_TRUE(hold_end.left.iris_scale < 0.60f);
  TEST_ASSERT_TRUE(hold_end.right.iris_scale < 0.60f);

  // Releasing with outro should return to normal eyes.
  TEST_ASSERT_TRUE(release_end.left.iris_scale > 0.80f);
  TEST_ASSERT_TRUE(release_end.right.iris_scale > 0.80f);
}

TEST_CASE("eyes emotion controller holds current emotion until released", "[eyes]") {
  class LocalNoopSink final : public eyes::IFrameSink {
  public:
    bool begin(int width, int height, int fps) override {
      (void)width;
      (void)height;
      (void)fps;
      return true;
    }
    bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) override {
      (void)grayscale;
      (void)stride;
      (void)duration_ms;
      return true;
    }
    bool end() override {
      return true;
    }
  };

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  LocalNoopSink sink;
  eyes::EyesClipPlayer player(animator, renderer, sink);

  eyes::EmotionControllerConfig cfg;
  cfg.transition_ms = 200;
  cfg.hold_segment_ms = 120;
  cfg.release_hold_ms = 80;
  cfg.play_outro_on_release = true;
  eyes::EmotionController controller(player, cfg);

  eyes::ClipOptions base;
  base.size = {64, 64};
  base.fps = 20;

  controller.set_emotion(Emotion::Curiosity, 0x1010u);
  TEST_ASSERT_TRUE(controller.step(base));
  TEST_ASSERT_TRUE(controller.step(base));
  TEST_ASSERT_TRUE(controller.step(base));
  TEST_ASSERT_TRUE(controller.has_current_emotion());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::Curiosity),
                        static_cast<int>(controller.current_emotion()));
  TEST_ASSERT_FALSE(controller.has_pending_transition());
  TEST_ASSERT_EQUAL_UINT32(360u, controller.elapsed_ms_in_current());

  controller.release_to_next(Emotion::PlayfulPong, 0x2020u);
  TEST_ASSERT_TRUE(controller.has_pending_transition());
  TEST_ASSERT_TRUE(controller.step(base));
  TEST_ASSERT_TRUE(controller.has_current_emotion());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(Emotion::PlayfulPong),
                        static_cast<int>(controller.current_emotion()));
  TEST_ASSERT_FALSE(controller.has_pending_transition());
  TEST_ASSERT_EQUAL_UINT32(cfg.hold_segment_ms, controller.elapsed_ms_in_current());
}

static bool contains_string(const std::vector<uint8_t> &v, const char *s) {
  if (!s) {
    return false;
  }
  size_t len = strlen(s);
  if (len == 0 || v.size() < len) {
    return false;
  }
  for (size_t i = 0; i + len <= v.size(); ++i) {
    if (memcmp(v.data() + i, s, len) == 0) {
      return true;
    }
  }
  return false;
}

static int count_gif_images(const std::vector<uint8_t> &v) {
  // Minimal GIF parser that counts image blocks (0x2C) by walking the block structure.
  if (v.size() < 13) {
    return 0;
  }
  if (memcmp(v.data(), "GIF", 3) != 0) {
    return 0;
  }

  size_t p = 6; // header
  p += 4;       // width + height
  uint8_t packed = v[p++];
  p += 2; // bg index + aspect

  if (packed & 0x80) {
    int gct_pow = (packed & 0x07) + 1;
    size_t gct_bytes = static_cast<size_t>(3) * (static_cast<size_t>(1) << gct_pow);
    if (p + gct_bytes > v.size()) {
      return 0;
    }
    p += gct_bytes;
  }

  int images = 0;
  while (p < v.size()) {
    uint8_t b = v[p++];
    if (b == 0x3B) { // trailer
      break;
    }
    if (b == 0x21) { // extension
      if (p >= v.size()) {
        return 0;
      }
      p++; // label
      while (p < v.size()) {
        uint8_t sz = v[p++];
        if (sz == 0) {
          break;
        }
        if (p + sz > v.size()) {
          return 0;
        }
        p += sz;
      }
      continue;
    }
    if (b == 0x2C) { // image separator
      images++;
      if (p + 9 > v.size()) {
        return 0;
      }
      p += 8;
      uint8_t ipacked = v[p++];
      if (ipacked & 0x80) {
        int lct_pow = (ipacked & 0x07) + 1;
        size_t lct_bytes = static_cast<size_t>(3) * (static_cast<size_t>(1) << lct_pow);
        if (p + lct_bytes > v.size()) {
          return 0;
        }
        p += lct_bytes;
      }
      if (p >= v.size()) {
        return 0;
      }
      p++; // lzw min code size
      while (p < v.size()) {
        uint8_t sz = v[p++];
        if (sz == 0) {
          break;
        }
        if (p + sz > v.size()) {
          return 0;
        }
        p += sz;
      }
      continue;
    }

    // Unknown block.
    return 0;
  }
  return images;
}

TEST_CASE("eyes gif encoder writes a valid header and trailer", "[eyes]") {
  std::vector<uint8_t> bytes;
  eyes::gif::VectorByteSink sink(bytes);
  eyes::gif::GifEncoder enc(sink);

  TEST_ASSERT_TRUE(enc.begin(10, 10, true));

  std::vector<uint8_t> frame(100, 0);
  TEST_ASSERT_TRUE(enc.add_frame(frame.data(), 10, 3));
  TEST_ASSERT_TRUE(enc.end());

  TEST_ASSERT_GREATER_OR_EQUAL_INT(6, static_cast<int>(bytes.size()));
  TEST_ASSERT_EQUAL_UINT8('G', bytes[0]);
  TEST_ASSERT_EQUAL_UINT8('I', bytes[1]);
  TEST_ASSERT_EQUAL_UINT8('F', bytes[2]);
  TEST_ASSERT_EQUAL_UINT8('8', bytes[3]);
  TEST_ASSERT_EQUAL_UINT8('9', bytes[4]);
  TEST_ASSERT_EQUAL_UINT8('a', bytes[5]);
  TEST_ASSERT_EQUAL_UINT8(0x3B, bytes.back());

  TEST_ASSERT_TRUE(contains_string(bytes, "NETSCAPE2.0"));
  TEST_ASSERT_EQUAL_INT(1, count_gif_images(bytes));
}

TEST_CASE("eyes clip player can export a short gif", "[eyes]") {
  std::vector<uint8_t> bytes;
  eyes::gif::VectorByteSink sink(bytes);
  eyes::gif::GifEncoder enc(sink);
  eyes::gif::GifFrameSink gif_sink(enc);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  eyes::EyesClipPlayer player(animator, renderer, gif_sink);

  eyes::ClipOptions opt;
  opt.size = {64, 64};
  opt.fps = 12;
  opt.duration_ms_override = 250;

  TEST_ASSERT_TRUE(player.play(Emotion::Neutral, opt));
  TEST_ASSERT_GREATER_THAN_INT(6, static_cast<int>(bytes.size()));
  TEST_ASSERT_EQUAL_UINT8('G', bytes[0]);
  TEST_ASSERT_EQUAL_UINT8('I', bytes[1]);
  TEST_ASSERT_EQUAL_UINT8('F', bytes[2]);
  TEST_ASSERT_EQUAL_UINT8(0x3B, bytes.back());
  TEST_ASSERT_TRUE(count_gif_images(bytes) >= 1);
}

namespace {

class FakeClock final : public eyes::IClock {
public:
  explicit FakeClock(uint64_t start_us = 0) : now_us_(start_us) {
  }

  uint64_t now_us() const override {
    return now_us_;
  }

  void advance_us(uint64_t delta_us) {
    now_us_ += delta_us;
  }

private:
  uint64_t now_us_;
};

class AdvancingSink final : public eyes::IFrameSink {
public:
  AdvancingSink(FakeClock &clock, uint64_t per_frame_advance_us)
      : clock_(clock), per_frame_advance_us_(per_frame_advance_us) {
  }

  bool begin(int width, int height, int fps) override {
    (void)width;
    (void)height;
    (void)fps;
    frames_ = 0;
    return true;
  }

  bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) override {
    (void)grayscale;
    (void)stride;
    (void)duration_ms;
    frames_++;
    clock_.advance_us(per_frame_advance_us_);
    return true;
  }

  bool end() override {
    return true;
  }

  int frames() const {
    return frames_;
  }

private:
  FakeClock &clock_;
  uint64_t per_frame_advance_us_ = 0;
  int frames_ = 0;
};

class CaptureFirstFrameSink final : public eyes::IFrameSink {
public:
  bool begin(int width, int height, int fps) override {
    (void)fps;
    width_ = width;
    height_ = height;
    first_frame_captured_ = false;
    first_frames_.push_back(std::vector<uint8_t>());
    return true;
  }

  bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) override {
    (void)duration_ms;
    if (!first_frame_captured_) {
      std::vector<uint8_t> &dst = first_frames_.back();
      dst.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_), 0);
      for (int y = 0; y < height_; ++y) {
        const uint8_t *src = grayscale + y * stride;
        memcpy(dst.data() + static_cast<size_t>(y) * static_cast<size_t>(width_), src,
               static_cast<size_t>(width_));
      }
      first_frame_captured_ = true;
    }
    return true;
  }

  bool end() override {
    return true;
  }

  const std::vector<std::vector<uint8_t>> &first_frames() const {
    return first_frames_;
  }

private:
  int width_ = 0;
  int height_ = 0;
  bool first_frame_captured_ = false;
  std::vector<std::vector<uint8_t>> first_frames_;
};

class NoopSink final : public eyes::IFrameSink {
public:
  bool begin(int width, int height, int fps) override {
    (void)width;
    (void)height;
    (void)fps;
    return true;
  }

  bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) override {
    (void)grayscale;
    (void)stride;
    (void)duration_ms;
    return true;
  }

  bool end() override {
    return true;
  }
};

class TransitionStatsSink final : public eyes::IFrameSink {
public:
  bool begin(int width, int height, int fps) override {
    (void)fps;
    width_ = width;
    height_ = height;
    prev_.assign(static_cast<size_t>(width_) * static_cast<size_t>(height_), 0);
    has_prev_ = false;
    frame_count_ = 0;
    min_lit_pixels_ = width_ * height_;
    min_max_pixel_ = 255;
    max_diff_pixels_ = 0;
    return true;
  }

  bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) override {
    (void)duration_ms;
    int lit = 0;
    int diff = 0;
    uint8_t mx = 0;
    for (int y = 0; y < height_; ++y) {
      const uint8_t *row = grayscale + y * stride;
      size_t row_off = static_cast<size_t>(y) * static_cast<size_t>(width_);
      for (int x = 0; x < width_; ++x) {
        uint8_t px = row[x];
        if (px > mx) {
          mx = px;
        }
        if (px > 200) {
          lit++;
        }
        size_t idx = row_off + static_cast<size_t>(x);
        if (has_prev_ && px != prev_[idx]) {
          diff++;
        }
        prev_[idx] = px;
      }
    }

    if (lit < min_lit_pixels_) {
      min_lit_pixels_ = lit;
    }
    if (mx < min_max_pixel_) {
      min_max_pixel_ = mx;
    }
    if (diff > max_diff_pixels_) {
      max_diff_pixels_ = diff;
    }

    has_prev_ = true;
    frame_count_++;
    return true;
  }

  bool end() override {
    return true;
  }

  int frame_count() const {
    return frame_count_;
  }

  int min_lit_pixels() const {
    return min_lit_pixels_;
  }

  int min_max_pixel() const {
    return min_max_pixel_;
  }

  int max_diff_pixels() const {
    return max_diff_pixels_;
  }

private:
  int width_ = 0;
  int height_ = 0;
  bool has_prev_ = false;
  int frame_count_ = 0;
  int min_lit_pixels_ = 0;
  int min_max_pixel_ = 0;
  int max_diff_pixels_ = 0;
  std::vector<uint8_t> prev_;
};

float smooth01(float u) {
  if (u <= 0.0f) {
    return 0.0f;
  }
  if (u >= 1.0f) {
    return 1.0f;
  }
  return u * u * (3.0f - 2.0f * u);
}

float eye_delta(const eyes::EyePose &a, const eyes::EyePose &b) {
  float m = 0.0f;
  m = fmaxf(m, fabsf(a.lids.upper - b.lids.upper));
  m = fmaxf(m, fabsf(a.lids.lower - b.lids.lower));
  m = fmaxf(m, fabsf(a.pupil_x - b.pupil_x));
  m = fmaxf(m, fabsf(a.pupil_y - b.pupil_y));
  m = fmaxf(m, fabsf(a.iris_scale - b.iris_scale));
  m = fmaxf(m, fabsf(a.pupil_scale - b.pupil_scale));
  m = fmaxf(m, fabsf(a.brow_raise - b.brow_raise));
  m = fmaxf(m, fabsf(a.brow_tilt - b.brow_tilt));
  return m;
}

float effects_delta(const eyes::EffectsPose &a, const eyes::EffectsPose &b) {
  float m = 0.0f;
  m = fmaxf(m, fabsf(static_cast<float>(a.snore_zs.count) - static_cast<float>(b.snore_zs.count)) / 5.0f);
  for (int i = 0; i < 5; ++i) {
    m = fmaxf(m, fabsf(a.snore_zs.progress[i] - b.snore_zs.progress[i]));
  }
  m = fmaxf(m, fabsf(static_cast<float>(a.dizzy_birds.count) - static_cast<float>(b.dizzy_birds.count)) / 3.0f);
  for (int i = 0; i < 3; ++i) {
    m = fmaxf(m, fabsf(a.dizzy_birds.phase[i] - b.dizzy_birds.phase[i]));
    m = fmaxf(m, fabsf(a.dizzy_birds.fly_away[i] - b.dizzy_birds.fly_away[i]));
  }
  m = fmaxf(m, fabsf(static_cast<float>(a.pong_ball.active) - static_cast<float>(b.pong_ball.active)));
  m = fmaxf(m, fabsf(a.pong_ball.x - b.pong_ball.x));
  m = fmaxf(m, fabsf(a.pong_ball.y - b.pong_ball.y));
  m = fmaxf(m, fabsf(a.pong_ball.radius - b.pong_ball.radius));
  m = fmaxf(m, fabsf(a.pong_ball.droplet - b.pong_ball.droplet));
  m = fmaxf(m, fabsf(a.pong_ball.paddle_morph - b.pong_ball.paddle_morph));
  m = fmaxf(m, fabsf(a.pong_ball.left_paddle_y - b.pong_ball.left_paddle_y));
  m = fmaxf(m, fabsf(a.pong_ball.right_paddle_y - b.pong_ball.right_paddle_y));
  return m;
}

bool pose_finite(const eyes::EyesPose &p) {
  auto eye_finite = [](const eyes::EyePose &e) {
    return isfinite(e.lids.upper) && isfinite(e.lids.lower) && isfinite(e.pupil_x) && isfinite(e.pupil_y) &&
           isfinite(e.iris_scale) && isfinite(e.pupil_scale) && isfinite(e.brow_raise) &&
           isfinite(e.brow_tilt);
  };

  if (!eye_finite(p.left) || !eye_finite(p.right)) {
    return false;
  }
  for (int i = 0; i < 5; ++i) {
    if (!isfinite(p.effects.snore_zs.progress[i])) {
      return false;
    }
  }
  for (int i = 0; i < 3; ++i) {
    if (!isfinite(p.effects.dizzy_birds.phase[i])) {
      return false;
    }
    if (!isfinite(p.effects.dizzy_birds.fly_away[i])) {
      return false;
    }
  }
  return isfinite(p.effects.pong_ball.x) && isfinite(p.effects.pong_ball.y) &&
         isfinite(p.effects.pong_ball.radius) && isfinite(p.effects.pong_ball.droplet) &&
         isfinite(p.effects.pong_ball.paddle_morph) && isfinite(p.effects.pong_ball.left_paddle_y) &&
         isfinite(p.effects.pong_ball.right_paddle_y);
}

bool snore_visible(const eyes::SnoreZsPose &s) {
  if (s.count == 0) {
    return false;
  }
  int count = static_cast<int>(s.count);
  if (count > 5) {
    count = 5;
  }
  for (int i = 0; i < count; ++i) {
    if (s.progress[i] < 0.995f) {
      return true;
    }
  }
  return false;
}

bool dizzy_visible(const eyes::DizzyBirdsPose &d) {
  if (d.count == 0) {
    return false;
  }
  int count = static_cast<int>(d.count);
  if (count > 3) {
    count = 3;
  }
  for (int i = 0; i < count; ++i) {
    if (d.fly_away[i] < 0.995f) {
      return true;
    }
  }
  return false;
}

bool pong_visible(const eyes::PongBallPose &p) {
  return (p.active != 0) || (p.radius > 0.0005f) || (p.droplet > 0.0005f) ||
         (p.paddle_morph > 0.0005f);
}

int visible_effect_families(const eyes::EyesPose &p) {
  int n = 0;
  if (snore_visible(p.effects.snore_zs)) {
    ++n;
  }
  if (dizzy_visible(p.effects.dizzy_birds)) {
    ++n;
  }
  if (pong_visible(p.effects.pong_ball)) {
    ++n;
  }
  return n;
}

} // namespace

TEST_CASE("eyes clip player realtime mode prevents slow sinks from slowing animation time", "[eyes]") {
  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;

  eyes::ClipOptions opt;
  opt.size = {16, 16};
  opt.fps = 30;
  opt.duration_ms_override = 1000;

  // Simulate a slow output path that consumes 50ms per rendered frame.
  const uint64_t per_frame_us = 50 * 1000;

  FakeClock clock_fixed;
  AdvancingSink sink_fixed(clock_fixed, per_frame_us);
  eyes::EyesClipPlayer player_fixed(animator, renderer, sink_fixed, clock_fixed);

  opt.realtime = false;
  TEST_ASSERT_TRUE(player_fixed.play(Emotion::Neutral, opt));
  int fixed_frames = sink_fixed.frames();
  TEST_ASSERT_TRUE(fixed_frames >= 28); // fixed-step should still try to emit ~30fps worth of frames

  FakeClock clock_rt;
  AdvancingSink sink_rt(clock_rt, per_frame_us);
  eyes::EyesClipPlayer player_rt(animator, renderer, sink_rt, clock_rt);

  opt.realtime = true;
  TEST_ASSERT_TRUE(player_rt.play(Emotion::Neutral, opt));
  int rt_frames = sink_rt.frames();

  // Real-time mode should emit fewer frames (because wall-clock time advances faster than frame_ms).
  TEST_ASSERT_TRUE(rt_frames < fixed_frames);
  TEST_ASSERT_INT_WITHIN(2, 20, rt_frames);
}

TEST_CASE("eyes blend_poses interpolates eye fields and effects", "[eyes]") {
  eyes::EyesPose a;
  a.left.lids.upper = 0.0f;
  a.left.lids.lower = 0.2f;
  a.left.pupil_x = -0.8f;
  a.left.pupil_y = 0.4f;
  a.left.iris_scale = 0.9f;
  a.left.pupil_scale = 0.8f;
  a.left.brow_raise = -0.4f;
  a.left.brow_tilt = -0.6f;
  a.right = a.left;
  a.effects.snore_zs.count = 0;
  a.effects.dizzy_birds.count = 0;
  a.effects.pong_ball.active = 1;
  a.effects.pong_ball.x = -0.5f;
  a.effects.pong_ball.y = -0.25f;
  a.effects.pong_ball.radius = 0.02f;
  a.effects.pong_ball.droplet = 1.0f;

  eyes::EyesPose b;
  b.left.lids.upper = 1.0f;
  b.left.lids.lower = 0.8f;
  b.left.pupil_x = 0.6f;
  b.left.pupil_y = -0.2f;
  b.left.iris_scale = 1.3f;
  b.left.pupil_scale = 1.4f;
  b.left.brow_raise = 0.7f;
  b.left.brow_tilt = 0.5f;
  b.right = b.left;
  b.effects.snore_zs.count = 5;
  for (int i = 0; i < 5; ++i) {
    b.effects.snore_zs.progress[i] = static_cast<float>(i) / 4.0f;
  }
  b.effects.dizzy_birds.count = 3;
  for (int i = 0; i < 3; ++i) {
    b.effects.dizzy_birds.phase[i] = static_cast<float>(i) / 2.0f;
    b.effects.dizzy_birds.fly_away[i] = static_cast<float>(i) / 2.0f;
  }
  b.effects.pong_ball.active = 1;
  b.effects.pong_ball.x = 0.5f;
  b.effects.pong_ball.y = 0.25f;
  b.effects.pong_ball.radius = 0.04f;
  b.effects.pong_ball.droplet = 0.0f;

  eyes::EyesPose mid = eyes::blend_poses(a, b, 0.5f);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, mid.left.lids.upper);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, mid.left.lids.lower);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.1f, mid.left.pupil_x);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, mid.left.pupil_y);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.1f, mid.left.iris_scale);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.1f, mid.left.pupil_scale);
  TEST_ASSERT_EQUAL_UINT8(3, mid.effects.snore_zs.count);
  TEST_ASSERT_EQUAL_UINT8(2, mid.effects.dizzy_birds.count);
  TEST_ASSERT_EQUAL_UINT8(1, mid.effects.pong_ball.active);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, mid.effects.pong_ball.x);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.03f, mid.effects.pong_ball.radius);
}

TEST_CASE("eyes blend_poses moves snore Zs forward when transitioning out of snoring", "[eyes]") {
  eyes::EyesPose from;
  from.effects.snore_zs.count = 5;
  from.effects.snore_zs.progress[0] = 0.10f;
  from.effects.snore_zs.progress[1] = 0.30f;
  from.effects.snore_zs.progress[2] = 0.55f;
  from.effects.snore_zs.progress[3] = 0.80f;
  from.effects.snore_zs.progress[4] = 0.95f;

  eyes::EyesPose to;
  to.effects.snore_zs.count = 0;

  eyes::EyesPose mid = eyes::blend_poses(from, to, 0.5f);
  TEST_ASSERT_EQUAL_UINT8(5, mid.effects.snore_zs.count);
  for (int i = 0; i < 5; ++i) {
    TEST_ASSERT_TRUE(mid.effects.snore_zs.progress[i] >= from.effects.snore_zs.progress[i]);
    TEST_ASSERT_TRUE(mid.effects.snore_zs.progress[i] <= 1.0f);
  }

  eyes::EyesPose end = eyes::blend_poses(from, to, 1.0f);
  TEST_ASSERT_EQUAL_UINT8(5, end.effects.snore_zs.count);
  for (int i = 0; i < 5; ++i) {
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, end.effects.snore_zs.progress[i]);
  }
}

TEST_CASE("eyes blend_poses moves dizzy birds forward and marks fly-away on exit", "[eyes]") {
  eyes::EyesPose from;
  from.effects.dizzy_birds.count = 3;
  from.effects.dizzy_birds.phase[0] = 0.10f;
  from.effects.dizzy_birds.phase[1] = 0.45f;
  from.effects.dizzy_birds.phase[2] = 0.80f;

  eyes::EyesPose to;
  to.effects.dizzy_birds.count = 0;

  eyes::EyesPose mid = eyes::blend_poses(from, to, 0.5f);
  TEST_ASSERT_EQUAL_UINT8(3, mid.effects.dizzy_birds.count);
  for (int i = 0; i < 3; ++i) {
    TEST_ASSERT_TRUE(mid.effects.dizzy_birds.phase[i] > from.effects.dizzy_birds.phase[i]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, mid.effects.dizzy_birds.fly_away[i]);
  }

  eyes::EyesPose end = eyes::blend_poses(from, to, 1.0f);
  TEST_ASSERT_EQUAL_UINT8(3, end.effects.dizzy_birds.count);
  for (int i = 0; i < 3; ++i) {
    TEST_ASSERT_TRUE(end.effects.dizzy_birds.phase[i] > from.effects.dizzy_birds.phase[i]);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, end.effects.dizzy_birds.fly_away[i]);
  }
}

TEST_CASE("eyes blend_poses entering dizzy keeps bird orbit speed near target", "[eyes]") {
  eyes::EyesAnimator animator;

  eyes::EyesPose from;
  from.effects.dizzy_birds.count = 0;

  const uint32_t transition_ms = 420;
  const uint32_t step_ms = 1000 / 60; // 60fps sampling to catch spikes

  bool have_prev = false;
  float prev_cur_phase = 0.0f;
  float prev_to_phase = 0.0f;
  float max_ratio = 0.0f;
  int compared_steps = 0;

  for (uint32_t t_ms = 0; t_ms <= transition_ms; t_ms += step_ms) {
    eyes::EyesPose to_pose = animator.pose_for(Emotion::Dizzy, t_ms);
    float u = static_cast<float>(t_ms) / static_cast<float>(transition_ms);
    float alpha = smooth01(u);
    eyes::EyesPose cur = eyes::blend_poses(from, to_pose, alpha);

    if (cur.effects.dizzy_birds.count == 0) {
      continue;
    }

    float cur_phase = cur.effects.dizzy_birds.phase[0];
    float to_phase = to_pose.effects.dizzy_birds.phase[0];

    if (have_prev) {
      float cur_d = cur_phase - prev_cur_phase;
      if (cur_d < 0.0f) {
        cur_d += 1.0f;
      }
      float to_d = to_phase - prev_to_phase;
      if (to_d < 0.0f) {
        to_d += 1.0f;
      }
      if (to_d > 0.000001f) {
        float ratio = cur_d / to_d;
        if (ratio > max_ratio) {
          max_ratio = ratio;
        }
        compared_steps++;
      }
    }

    prev_cur_phase = cur_phase;
    prev_to_phase = to_phase;
    have_prev = true;
  }

  TEST_ASSERT_TRUE(compared_steps >= 8);
  TEST_ASSERT_TRUE(max_ratio < 1.20f);
}

TEST_CASE("eyes blend_poses entering snoring keeps Z drift speed near target", "[eyes]") {
  eyes::EyesAnimator animator;

  eyes::EyesPose from;
  from.effects.snore_zs.count = 0;

  const uint32_t transition_ms = 420;
  const uint32_t step_ms = 1000 / 60; // 60fps sampling to catch spikes

  bool have_prev = false;
  float prev_cur_progress = 0.0f;
  float prev_to_progress = 0.0f;
  float max_ratio = 0.0f;
  int compared_steps = 0;

  for (uint32_t t_ms = 0; t_ms <= transition_ms; t_ms += step_ms) {
    eyes::EyesPose to_pose = animator.pose_for(Emotion::Snoring, t_ms);
    float u = static_cast<float>(t_ms) / static_cast<float>(transition_ms);
    float alpha = smooth01(u);
    eyes::EyesPose cur = eyes::blend_poses(from, to_pose, alpha);

    if (cur.effects.snore_zs.count == 0 || to_pose.effects.snore_zs.count == 0) {
      continue;
    }

    float cur_progress = cur.effects.snore_zs.progress[0];
    float to_progress = to_pose.effects.snore_zs.progress[0];

    if (have_prev) {
      float cur_d = cur_progress - prev_cur_progress;
      if (cur_d < 0.0f) {
        cur_d += 1.0f;
      }
      float to_d = to_progress - prev_to_progress;
      if (to_d < 0.0f) {
        to_d += 1.0f;
      }
      if (to_d > 0.000001f) {
        float ratio = cur_d / to_d;
        if (ratio > max_ratio) {
          max_ratio = ratio;
        }
        compared_steps++;
      }
    }

    prev_cur_progress = cur_progress;
    prev_to_progress = to_progress;
    have_prev = true;
  }

  TEST_ASSERT_TRUE(compared_steps >= 8);
  TEST_ASSERT_TRUE(max_ratio < 1.20f);
}

TEST_CASE("eyes blend_poses entering playful_pong keeps ball motion speed near target", "[eyes]") {
  eyes::EyesAnimator animator;

  eyes::EyesPose from;
  from.effects.pong_ball.active = 0;

  const uint32_t transition_ms = 420;
  const uint32_t step_ms = 1000 / 60; // 60fps sampling to catch spikes

  bool have_prev = false;
  float prev_cur_x = 0.0f;
  float prev_cur_y = 0.0f;
  float prev_to_x = 0.0f;
  float prev_to_y = 0.0f;
  float max_ratio = 0.0f;
  int compared_steps = 0;

  for (uint32_t t_ms = 0; t_ms <= transition_ms; t_ms += step_ms) {
    eyes::EyesPose to_pose = animator.pose_for(Emotion::PlayfulPong, t_ms);
    float u = static_cast<float>(t_ms) / static_cast<float>(transition_ms);
    float alpha = smooth01(u);
    eyes::EyesPose cur = eyes::blend_poses(from, to_pose, alpha);

    if (!cur.effects.pong_ball.active || !to_pose.effects.pong_ball.active) {
      continue;
    }

    float cur_x = cur.effects.pong_ball.x;
    float cur_y = cur.effects.pong_ball.y;
    float to_x = to_pose.effects.pong_ball.x;
    float to_y = to_pose.effects.pong_ball.y;

    if (have_prev) {
      float cur_d = sqrtf((cur_x - prev_cur_x) * (cur_x - prev_cur_x) +
                          (cur_y - prev_cur_y) * (cur_y - prev_cur_y));
      float to_d = sqrtf((to_x - prev_to_x) * (to_x - prev_to_x) + (to_y - prev_to_y) * (to_y - prev_to_y));
      if (to_d > 0.000001f) {
        float ratio = cur_d / to_d;
        if (ratio > max_ratio) {
          max_ratio = ratio;
        }
        compared_steps++;
      }
    }

    prev_cur_x = cur_x;
    prev_cur_y = cur_y;
    prev_to_x = to_x;
    prev_to_y = to_y;
    have_prev = true;
  }

  TEST_ASSERT_TRUE(compared_steps >= 6);
  TEST_ASSERT_TRUE(max_ratio < 1.20f);
}

TEST_CASE("eyes blend_poses exiting playful_pong does not accelerate ball translation", "[eyes]") {
  eyes::EyesAnimator animator;
  eyes::EyesPose from = animator.pose_for(Emotion::PlayfulPong, 2800);
  eyes::EyesPose to = animator.pose_for(Emotion::Neutral, 0);
  TEST_ASSERT_TRUE(from.effects.pong_ball.active == 1);

  const uint32_t transition_ms = 420;
  const uint32_t step_ms = 1000 / 60;

  bool have_prev = false;
  float prev_x = 0.0f;
  float prev_y = 0.0f;
  float max_step = 0.0f;

  for (uint32_t t_ms = 0; t_ms <= transition_ms; t_ms += step_ms) {
    float u = static_cast<float>(t_ms) / static_cast<float>(transition_ms);
    float alpha = smooth01(u);
    eyes::EyesPose cur = eyes::blend_poses(from, to, alpha);

    if (!have_prev) {
      prev_x = cur.effects.pong_ball.x;
      prev_y = cur.effects.pong_ball.y;
      have_prev = true;
      continue;
    }

    float step = sqrtf((cur.effects.pong_ball.x - prev_x) * (cur.effects.pong_ball.x - prev_x) +
                       (cur.effects.pong_ball.y - prev_y) * (cur.effects.pong_ball.y - prev_y));
    if (step > max_step) {
      max_step = step;
    }
    prev_x = cur.effects.pong_ball.x;
    prev_y = cur.effects.pong_ball.y;
  }

  TEST_ASSERT_TRUE(max_step < 0.01f);
}

TEST_CASE("eyes blend_poses quickly settles pong residue when leaving playful_pong", "[eyes]") {
  eyes::EyesAnimator animator;

  // Realistic outgoing source: last rendered playful_pong frame at 30fps (5000ms clip).
  eyes::EyesPose from = animator.pose_for(Emotion::PlayfulPong, 4983);
  TEST_ASSERT_TRUE(from.effects.pong_ball.active == 1);
  TEST_ASSERT_TRUE(from.effects.pong_ball.paddle_morph > 0.0005f);

  eyes::EyesPose to = animator.pose_for(Emotion::Dizzy, 165);
  float u = smooth01(165.0f / 420.0f);
  eyes::EyesPose mid = eyes::blend_poses(from, to, u);

  // By around mid-transition, outgoing Pong paddles/ball should no longer dominate.
  TEST_ASSERT_TRUE(mid.effects.pong_ball.paddle_morph < 0.0005f);
  TEST_ASSERT_TRUE(mid.effects.pong_ball.radius < 0.0005f);
  TEST_ASSERT_TRUE(mid.effects.pong_ball.active == 0);
}

TEST_CASE("eyes blend_poses clears pong effect at transition end for non-pong targets", "[eyes]") {
  eyes::EyesAnimator animator;
  const Emotion emotions[] = {
      Emotion::Neutral,       Emotion::Happiness,   Emotion::Curiosity,
      Emotion::Exhaustion,    Emotion::FallingAsleep, Emotion::WakingUp,
      Emotion::Snoring,       Emotion::Dizzy,       Emotion::PlayfulPong,
  };

  for (size_t fi = 0; fi < sizeof(emotions) / sizeof(emotions[0]); ++fi) {
    Emotion from_e = emotions[fi];
    uint32_t from_d = eyes::emotion_clip(from_e).duration_ms;
    eyes::EyesPose from_pose = animator.pose_for(from_e, (from_d * 17) / 20); // late clip sample

    for (size_t ti = 0; ti < sizeof(emotions) / sizeof(emotions[0]); ++ti) {
      Emotion to_e = emotions[ti];
      if (to_e == Emotion::PlayfulPong) {
        continue;
      }
      eyes::EyesPose to_pose = animator.pose_for(to_e, 0);
      eyes::EyesPose end = eyes::blend_poses(from_pose, to_pose, 1.0f);
      TEST_ASSERT_TRUE(end.effects.pong_ball.active == 0);
      TEST_ASSERT_TRUE(end.effects.pong_ball.paddle_morph <= 0.0005f);
      TEST_ASSERT_TRUE(end.effects.pong_ball.radius <= 0.0005f);
    }
  }
}

TEST_CASE("eyes blend_poses avoids overlapping unrelated effect families during handoff", "[eyes]") {
  eyes::EyesAnimator animator;

  struct EffectSample {
    Emotion emotion;
    uint32_t t_ms;
  };

  const uint32_t snore_d = eyes::emotion_clip(Emotion::Snoring).duration_ms;
  const uint32_t dizzy_d = eyes::emotion_clip(Emotion::Dizzy).duration_ms;
  const uint32_t pong_d = eyes::emotion_clip(Emotion::PlayfulPong).duration_ms;

  const EffectSample samples[] = {
      {Emotion::Snoring, snore_d / 3},
      {Emotion::Dizzy, dizzy_d / 3},
      {Emotion::PlayfulPong, pong_d / 2},
  };
  const float alphas[] = {0.20f, 0.40f, 0.60f, 0.80f};

  for (size_t fi = 0; fi < sizeof(samples) / sizeof(samples[0]); ++fi) {
    for (size_t ti = 0; ti < sizeof(samples) / sizeof(samples[0]); ++ti) {
      if (fi == ti) {
        continue;
      }
      eyes::EyesPose from_pose = animator.pose_for(samples[fi].emotion, samples[fi].t_ms);
      eyes::EyesPose to_pose = animator.pose_for(samples[ti].emotion, samples[ti].t_ms);

      for (size_t ai = 0; ai < sizeof(alphas) / sizeof(alphas[0]); ++ai) {
        eyes::EyesPose cur = eyes::blend_poses(from_pose, to_pose, alphas[ai]);
        TEST_ASSERT_TRUE(visible_effect_families(cur) <= 1);
      }
    }
  }
}

TEST_CASE("eyes sequencer starts next emotion from previous emitted pose", "[eyes]") {
  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  CaptureFirstFrameSink sink;
  eyes::EyesClipPlayer player(animator, renderer, sink);

  eyes::SequencerOptions seq_opt;
  seq_opt.enable_transitions = true;
  seq_opt.transition_ms = 200;
  eyes::EyesEmotionSequencer sequencer(player, seq_opt);

  eyes::ClipOptions opt;
  opt.size = {48, 32};
  opt.fps = 20;
  opt.duration_ms_override = 300;

  TEST_ASSERT_TRUE(sequencer.play(Emotion::Neutral, opt));
  TEST_ASSERT_TRUE(sequencer.has_pose());
  eyes::EyesPose end_pose = sequencer.current_pose();

  TEST_ASSERT_TRUE(sequencer.play(Emotion::Happiness, opt));

  const std::vector<std::vector<uint8_t>> &frames = sink.first_frames();
  TEST_ASSERT_TRUE(frames.size() >= 2);

  std::vector<uint8_t> expected(static_cast<size_t>(opt.size.width) * static_cast<size_t>(opt.size.height));
  eyes::PixelBuffer8 expected_buf(expected.data(), opt.size.width, opt.size.height, opt.size.width);
  renderer.render(end_pose, expected_buf);

  TEST_ASSERT_EQUAL_UINT32(expected.size(), frames[1].size());
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected.data(), frames[1].data(), expected.size());
}

TEST_CASE("eyes clip player preserves dizzy bird direction and adds fly-away on exit", "[eyes]") {
  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  NoopSink sink;
  eyes::EyesClipPlayer player(animator, renderer, sink);

  eyes::ClipOptions opt;
  opt.size = {64, 64};
  opt.fps = 20;
  opt.duration_ms_override = 500;
  opt.transition.enabled = true;
  opt.transition.duration_ms = 500;
  opt.transition.from.effects.dizzy_birds.count = 3;
  opt.transition.from.effects.dizzy_birds.phase[0] = 0.05f;
  opt.transition.from.effects.dizzy_birds.phase[1] = 0.35f;
  opt.transition.from.effects.dizzy_birds.phase[2] = 0.65f;

  eyes::EyesPose end_pose;
  TEST_ASSERT_TRUE(player.play(Emotion::Neutral, opt, &end_pose));

  TEST_ASSERT_EQUAL_UINT8(3, end_pose.effects.dizzy_birds.count);
  for (int i = 0; i < 3; ++i) {
    float expected_phase = opt.transition.from.effects.dizzy_birds.phase[i] +
                           (static_cast<float>(opt.duration_ms_override) /
                            static_cast<float>(eyes::emotion_clip(Emotion::Dizzy).duration_ms)) *
                               1.85f;
    TEST_ASSERT_FLOAT_WITHIN(0.02f, expected_phase, end_pose.effects.dizzy_birds.phase[i]);
    TEST_ASSERT_TRUE(end_pose.effects.dizzy_birds.fly_away[i] > 0.70f);
    TEST_ASSERT_TRUE(end_pose.effects.dizzy_birds.fly_away[i] <= 1.00f);
  }
}

TEST_CASE("eyes transitions stay smooth across all emotion pairs and sampled source phases", "[eyes]") {
  eyes::EyesAnimator animator;
  const Emotion emotions[] = {
      Emotion::Neutral,       Emotion::Happiness,   Emotion::Curiosity,
      Emotion::Exhaustion,    Emotion::FallingAsleep, Emotion::WakingUp,
      Emotion::Snoring,       Emotion::Dizzy,       Emotion::PlayfulPong,
  };
  const float source_phases[] = {0.00f, 0.20f, 0.40f, 0.60f, 0.80f};

  const uint32_t transition_ms = 420;
  const uint32_t step_ms = 33; // ~30fps

  for (size_t fi = 0; fi < sizeof(emotions) / sizeof(emotions[0]); ++fi) {
    for (size_t ti = 0; ti < sizeof(emotions) / sizeof(emotions[0]); ++ti) {
      Emotion from_e = emotions[fi];
      Emotion to_e = emotions[ti];
      bool pong_pair = (from_e == Emotion::PlayfulPong || to_e == Emotion::PlayfulPong);
      float eye_limit = pong_pair ? 0.52f : 0.36f;
      float effects_limit = pong_pair ? 0.66f : 0.48f;

      uint32_t from_duration = eyes::emotion_clip(from_e).duration_ms;
      for (size_t pi = 0; pi < sizeof(source_phases) / sizeof(source_phases[0]); ++pi) {
        uint32_t from_t = static_cast<uint32_t>(static_cast<float>(from_duration) * source_phases[pi]);
        eyes::EyesPose from_pose = animator.pose_for(from_e, from_t);

        eyes::EyesPose prev = from_pose;
        for (uint32_t t_ms = 0; t_ms <= transition_ms; t_ms += step_ms) {
          eyes::EyesPose to_pose = animator.pose_for(to_e, t_ms);
          float u = static_cast<float>(t_ms) / static_cast<float>(transition_ms);
          float alpha = smooth01(u);
          eyes::EyesPose cur = eyes::blend_poses(from_pose, to_pose, alpha);

          TEST_ASSERT_TRUE(pose_finite(cur));

          float left_step = eye_delta(prev.left, cur.left);
          float right_step = eye_delta(prev.right, cur.right);
          float fx_step = effects_delta(prev.effects, cur.effects);

          TEST_ASSERT_TRUE(left_step <= eye_limit);
          TEST_ASSERT_TRUE(right_step <= eye_limit);
          TEST_ASSERT_TRUE(fx_step <= effects_limit);
          prev = cur;
        }
      }
    }
  }
}

TEST_CASE("eyes sequencer transitions remain visually continuous for all emotion pairs", "[eyes]") {
  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  TransitionStatsSink sink;
  eyes::EyesClipPlayer player(animator, renderer, sink);

  eyes::SequencerOptions seq_opt;
  seq_opt.enable_transitions = true;
  seq_opt.transition_ms = 420;
  eyes::EyesEmotionSequencer sequencer(player, seq_opt);

  eyes::ClipOptions opt;
  opt.size = {64, 64};
  opt.fps = 30;
  opt.duration_ms_override = 420;

  const Emotion emotions[] = {
      Emotion::Neutral,       Emotion::Happiness,   Emotion::Curiosity,
      Emotion::Exhaustion,    Emotion::FallingAsleep, Emotion::WakingUp,
      Emotion::Snoring,       Emotion::Dizzy,       Emotion::PlayfulPong,
  };
  const float source_phases[] = {0.00f, 0.50f};

  for (size_t fi = 0; fi < sizeof(emotions) / sizeof(emotions[0]); ++fi) {
    Emotion from_e = emotions[fi];
    uint32_t from_duration = eyes::emotion_clip(from_e).duration_ms;
    for (size_t pi = 0; pi < sizeof(source_phases) / sizeof(source_phases[0]); ++pi) {
      uint32_t from_t = static_cast<uint32_t>(static_cast<float>(from_duration) * source_phases[pi]);
      eyes::EyesPose from_pose = animator.pose_for(from_e, from_t);

      for (size_t ti = 0; ti < sizeof(emotions) / sizeof(emotions[0]); ++ti) {
        sequencer.set_pose(from_pose);
        TEST_ASSERT_TRUE(sequencer.play(emotions[ti], opt));
        TEST_ASSERT_TRUE(sink.frame_count() >= 2);
        TEST_ASSERT_TRUE(sink.min_max_pixel() >= 200);
        TEST_ASSERT_TRUE(sink.min_lit_pixels() > 8);
        TEST_ASSERT_TRUE(sink.max_diff_pixels() < 3300);
      }
    }
  }
}

TEST_CASE("eyes clip player keeps snore-exit Z drift at natural speed", "[eyes]") {
  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;
  NoopSink sink;
  eyes::EyesClipPlayer player(animator, renderer, sink);

  eyes::ClipOptions opt;
  opt.size = {64, 64};
  opt.fps = 20;
  opt.duration_ms_override = 500;
  opt.transition.enabled = true;
  opt.transition.duration_ms = 500;
  opt.transition.from.effects.snore_zs.count = 5;
  opt.transition.from.effects.snore_zs.progress[0] = 0.00f;
  opt.transition.from.effects.snore_zs.progress[1] = 0.20f;
  opt.transition.from.effects.snore_zs.progress[2] = 0.40f;
  opt.transition.from.effects.snore_zs.progress[3] = 0.60f;
  opt.transition.from.effects.snore_zs.progress[4] = 0.80f;

  eyes::EyesPose end_pose;
  TEST_ASSERT_TRUE(player.play(Emotion::Neutral, opt, &end_pose));

  // If we accelerated Zs to "fly away", the first Z would approach 1.0 by clip end.
  // Natural snore speed advances only about duration/snore_duration (500/5000 = 0.1).
  TEST_ASSERT_EQUAL_UINT8(5, end_pose.effects.snore_zs.count);
  TEST_ASSERT_TRUE(end_pose.effects.snore_zs.progress[0] > 0.05f);
  TEST_ASSERT_TRUE(end_pose.effects.snore_zs.progress[0] < 0.30f);
}

static int count_pixels_above(const eyes::PixelBuffer8 &buf, uint8_t threshold) {
  int count = 0;
  for (int y = 0; y < buf.height(); ++y) {
    const uint8_t *row = buf.data() + y * buf.stride();
    for (int x = 0; x < buf.width(); ++x) {
      if (row[x] > threshold) {
        ++count;
      }
    }
  }
  return count;
}

static int count_pixels_above_rect(const eyes::PixelBuffer8 &buf, int x0, int y0, int x1, int y1,
                                   uint8_t threshold) {
  if (x0 < 0) {
    x0 = 0;
  }
  if (y0 < 0) {
    y0 = 0;
  }
  if (x1 >= buf.width()) {
    x1 = buf.width() - 1;
  }
  if (y1 >= buf.height()) {
    y1 = buf.height() - 1;
  }
  if (x0 > x1 || y0 > y1) {
    return 0;
  }

  int count = 0;
  for (int y = y0; y <= y1; ++y) {
    const uint8_t *row = buf.data() + y * buf.stride();
    for (int x = x0; x <= x1; ++x) {
      if (row[x] > threshold) {
        ++count;
      }
    }
  }
  return count;
}

static uint8_t max_pixel(const eyes::PixelBuffer8 &buf) {
  uint8_t mx = 0;
  for (int y = 0; y < buf.height(); ++y) {
    const uint8_t *row = buf.data() + y * buf.stride();
    for (int x = 0; x < buf.width(); ++x) {
      if (row[x] > mx) {
        mx = row[x];
      }
    }
  }
  return mx;
}

static int count_pixel_differences(const eyes::PixelBuffer8 &a, const eyes::PixelBuffer8 &b) {
  if (a.width() != b.width() || a.height() != b.height()) {
    return -1;
  }
  int diff = 0;
  for (int y = 0; y < a.height(); ++y) {
    const uint8_t *ra = a.data() + y * a.stride();
    const uint8_t *rb = b.data() + y * b.stride();
    for (int x = 0; x < a.width(); ++x) {
      if (ra[x] != rb[x]) {
        ++diff;
      }
    }
  }
  return diff;
}

TEST_CASE("eyes renderer keeps a visible slit even when fully closed", "[eyes]") {
  std::vector<uint8_t> frame(280 * 240, 0);
  eyes::PixelBuffer8 buf(frame.data(), 280, 240, 280);

  eyes::EyesPose pose;
  pose.left.lids.upper = 1.0f;
  pose.left.lids.lower = 1.0f;
  pose.right = pose.left;

  eyes::EyesRenderer renderer;
  renderer.render(pose, buf);

  TEST_ASSERT_GREATER_OR_EQUAL_INT(200, static_cast<int>(max_pixel(buf)));
  TEST_ASSERT_GREATER_THAN_INT(150, count_pixels_above(buf, 200));
}

TEST_CASE("eyes snoring emits 5 Z glyphs and renderer draws them up/right", "[eyes]") {
  std::vector<uint8_t> frame(280 * 240, 0);
  eyes::PixelBuffer8 buf(frame.data(), 280, 240, 280);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;

  eyes::EyesPose pose = animator.pose_for(Emotion::Snoring, 0);
  TEST_ASSERT_EQUAL_UINT8(5, pose.effects.snore_zs.count);
  for (int i = 0; i < 5; ++i) {
    TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(0.0f, pose.effects.snore_zs.progress[i]);
    TEST_ASSERT_LESS_OR_EQUAL_FLOAT(1.0f, pose.effects.snore_zs.progress[i]);
  }

  renderer.render(pose, buf);

  // Region above/right of the right eye should contain at least some 'Z' pixels.
  int z_pixels = count_pixels_above_rect(buf, 240, 0, 279, 80, 200);
  TEST_ASSERT_GREATER_THAN_INT(10, z_pixels);
}

TEST_CASE("eyes dizzy renderer draws bird glyphs above the eyes", "[eyes]") {
  std::vector<uint8_t> frame(280 * 240, 0);
  eyes::PixelBuffer8 buf(frame.data(), 280, 240, 280);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;

  eyes::EyesPose pose = animator.pose_for(Emotion::Dizzy, 0);
  renderer.render(pose, buf);

  int bird_pixels = count_pixels_above_rect(buf, 30, 0, 250, 115, 200);
  TEST_ASSERT_GREATER_THAN_INT(20, bird_pixels);
}

TEST_CASE("eyes dizzy birds fly up-right during transition exit", "[eyes]") {
  std::vector<uint8_t> frame_orbit(280 * 240, 0);
  std::vector<uint8_t> frame_exit(280 * 240, 0);
  eyes::PixelBuffer8 buf_orbit(frame_orbit.data(), 280, 240, 280);
  eyes::PixelBuffer8 buf_exit(frame_exit.data(), 280, 240, 280);

  eyes::EyesRenderer renderer;
  eyes::EyesPose orbit_pose;
  orbit_pose.effects.dizzy_birds.count = 1;
  orbit_pose.effects.dizzy_birds.phase[0] = 0.10f;
  orbit_pose.effects.dizzy_birds.fly_away[0] = 0.00f;

  eyes::EyesPose exit_pose = orbit_pose;
  exit_pose.effects.dizzy_birds.fly_away[0] = 0.45f;

  renderer.render(orbit_pose, buf_orbit);
  renderer.render(exit_pose, buf_exit);

  int orbit_top_right = count_pixels_above_rect(buf_orbit, 220, 0, 279, 70, 200);
  int exit_top_right = count_pixels_above_rect(buf_exit, 220, 0, 279, 70, 200);
  TEST_ASSERT_TRUE(exit_top_right > orbit_top_right);
}

TEST_CASE("eyes dizzy bird glyph is compact with wings and non-blocky body", "[eyes]") {
  std::vector<uint8_t> frame(280 * 240, 0);
  eyes::PixelBuffer8 buf(frame.data(), 280, 240, 280);

  eyes::EyesRenderer renderer;
  eyes::EyesPose pose;
  pose.effects.dizzy_birds.count = 1;
  pose.effects.dizzy_birds.phase[0] = 0.0f;

  renderer.render(pose, buf);

  // For phase 0 on 280x240 with wide orbit: bird center is near (213, 58).
  int bird_total = count_pixels_above_rect(buf, 186, 36, 246, 90, 200);
  int body_core = count_pixels_above_rect(buf, 205, 52, 221, 69, 200);
  int wing_left = count_pixels_above_rect(buf, 186, 42, 212, 84, 200);
  int wing_right = count_pixels_above_rect(buf, 214, 42, 246, 84, 200);

  TEST_ASSERT_GREATER_THAN_INT(24, bird_total);
  TEST_ASSERT_GREATER_THAN_INT(6, body_core);
  TEST_ASSERT_LESS_THAN_INT(70, body_core); // keep body compact, not blocky
  TEST_ASSERT_GREATER_THAN_INT(6, wing_left);
  TEST_ASSERT_GREATER_THAN_INT(6, wing_right);
}

TEST_CASE("eyes dizzy birds yaw-turn with orbit (no in-plane roll)", "[eyes]") {
  std::vector<uint8_t> frame_edge(280 * 240, 0);
  std::vector<uint8_t> frame_left(280 * 240, 0);
  std::vector<uint8_t> frame_right(280 * 240, 0);
  eyes::PixelBuffer8 buf_edge(frame_edge.data(), 280, 240, 280);
  eyes::PixelBuffer8 buf_left(frame_left.data(), 280, 240, 280);
  eyes::PixelBuffer8 buf_right(frame_right.data(), 280, 240, 280);

  eyes::EyesRenderer renderer;
  eyes::EyesPose pose;
  pose.effects.dizzy_birds.count = 1;

  pose.effects.dizzy_birds.phase[0] = 0.0f;   // edge-on (narrow)
  renderer.render(pose, buf_edge);
  pose.effects.dizzy_birds.phase[0] = 0.25f;  // facing left
  renderer.render(pose, buf_left);
  pose.effects.dizzy_birds.phase[0] = 0.75f;  // facing right
  renderer.render(pose, buf_right);

  // Edge-on phase should look narrower than broadside phase.
  int edge_outer = count_pixels_above_rect(buf_edge, 184, 42, 196, 84, 200) +
                   count_pixels_above_rect(buf_edge, 232, 42, 246, 84, 200);
  int left_outer = count_pixels_above_rect(buf_left, 112, 62, 122, 102, 200) +
                   count_pixels_above_rect(buf_left, 158, 62, 168, 102, 200);
  TEST_ASSERT_TRUE(left_outer > edge_outer + 6);

  // Facing direction should flip left vs right.
  int left_phase_left = count_pixels_above_rect(buf_left, 112, 62, 139, 102, 200);
  int left_phase_right = count_pixels_above_rect(buf_left, 141, 62, 168, 102, 200);
  TEST_ASSERT_TRUE(left_phase_left > left_phase_right);

  int right_phase_left = count_pixels_above_rect(buf_right, 112, 14, 139, 54, 200);
  int right_phase_right = count_pixels_above_rect(buf_right, 141, 14, 168, 54, 200);
  TEST_ASSERT_TRUE(right_phase_right > right_phase_left);
}

TEST_CASE("eyes playful_pong renderer draws ball/droplet effect", "[eyes]") {
  std::vector<uint8_t> with_ball(280 * 240, 0);
  std::vector<uint8_t> no_ball(280 * 240, 0);
  eyes::PixelBuffer8 buf_with(with_ball.data(), 280, 240, 280);
  eyes::PixelBuffer8 buf_without(no_ball.data(), 280, 240, 280);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;

  uint32_t duration = eyes::emotion_clip(Emotion::PlayfulPong).duration_ms;
  eyes::EyesPose pose = animator.pose_for(Emotion::PlayfulPong, duration / 10);
  TEST_ASSERT_EQUAL_UINT8(1, pose.effects.pong_ball.active);

  eyes::EyesPose pose_without = pose;
  pose_without.effects.pong_ball.active = 0;

  renderer.render(pose, buf_with);
  renderer.render(pose_without, buf_without);

  int diff = count_pixel_differences(buf_with, buf_without);
  TEST_ASSERT_TRUE(diff > 40);
}

TEST_CASE("eyes playful_pong renders paddles near screen edges during game", "[eyes]") {
  std::vector<uint8_t> frame(280 * 240, 0);
  eyes::PixelBuffer8 buf(frame.data(), 280, 240, 280);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;

  uint32_t duration = eyes::emotion_clip(Emotion::PlayfulPong).duration_ms;
  eyes::EyesPose pose = animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * 0.55f));
  TEST_ASSERT_TRUE(pose.effects.pong_ball.paddle_morph > 0.90f);

  renderer.render(pose, buf);

  int left_edge = count_pixels_above_rect(buf, 0, 30, 30, 210, 200);
  int right_edge = count_pixels_above_rect(buf, 249, 30, 279, 210, 200);
  TEST_ASSERT_GREATER_THAN_INT(120, left_edge);
  TEST_ASSERT_GREATER_THAN_INT(120, right_edge);
}

TEST_CASE("eyes playful_pong renderer never draws center eyes while paddles are visible", "[eyes]") {
  std::vector<uint8_t> frame(280 * 240, 0);
  eyes::PixelBuffer8 buf(frame.data(), 280, 240, 280);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;

  uint32_t duration = eyes::emotion_clip(Emotion::PlayfulPong).duration_ms;
  eyes::EyesPose pose = animator.pose_for(Emotion::PlayfulPong, static_cast<uint32_t>(duration * 0.22f));
  TEST_ASSERT_TRUE(pose.effects.pong_ball.paddle_morph > 0.20f);

  renderer.render(pose, buf);

  int left_region = count_pixels_above_rect(buf, 0, 35, 78, 205, 200);
  int right_region = count_pixels_above_rect(buf, 202, 35, 279, 205, 200);
  int center_region = count_pixels_above_rect(buf, 88, 35, 192, 205, 200);

  TEST_ASSERT_GREATER_THAN_INT(200, left_region);
  TEST_ASSERT_GREATER_THAN_INT(200, right_region);
  TEST_ASSERT_LESS_THAN_INT(80, center_region);
}

TEST_CASE("eyes falling_asleep bobs left/right and up/down", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::FallingAsleep).duration_ms;

  eyes::EyesPose p0 = animator.pose_for(Emotion::FallingAsleep, 0);
  eyes::EyesPose p1 = animator.pose_for(Emotion::FallingAsleep, duration / 4);

  // Both eyes should move together (head wobble, not independent gaze).
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, p0.left.pupil_x, p0.right.pupil_x);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, p0.left.pupil_y, p0.right.pupil_y);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, p1.left.pupil_x, p1.right.pupil_x);
  TEST_ASSERT_FLOAT_WITHIN(0.0001f, p1.left.pupil_y, p1.right.pupil_y);

  // Wobble should change at least one axis by a noticeable amount.
  float dx = fabsf(p1.left.pupil_x - p0.left.pupil_x);
  float dy = fabsf(p1.left.pupil_y - p0.left.pupil_y);
  TEST_ASSERT_TRUE((dx > 0.08f) || (dy > 0.08f));
}

TEST_CASE("eyes waking_up never renders an all-black frame", "[eyes]") {
  std::vector<uint8_t> frame(280 * 240, 0);
  eyes::PixelBuffer8 buf(frame.data(), 280, 240, 280);

  eyes::EyesAnimator animator;
  eyes::EyesRenderer renderer;

  const uint32_t duration = eyes::emotion_clip(Emotion::WakingUp).duration_ms;
  const uint32_t times[] = {
      0, duration / 6, duration * 2 / 6, duration * 3 / 6, duration * 4 / 6, duration * 5 / 6,
  };

  for (size_t i = 0; i < sizeof(times) / sizeof(times[0]); ++i) {
    eyes::EyesPose pose = animator.pose_for(Emotion::WakingUp, times[i]);
    renderer.render(pose, buf);

    TEST_ASSERT_GREATER_OR_EQUAL_INT(200, static_cast<int>(max_pixel(buf)));
    TEST_ASSERT_GREATER_THAN_INT(150, count_pixels_above(buf, 200));
  }
}

TEST_CASE("eyes waking_up has two rehearsal blinks and then fully opens", "[eyes]") {
  eyes::EyesAnimator animator;
  const uint32_t duration = eyes::emotion_clip(Emotion::WakingUp).duration_ms;

  auto upper_at_ms = [&](uint32_t ms) -> float {
    eyes::EyesPose pose = animator.pose_for(Emotion::WakingUp, ms);
    return pose.left.lids.upper;
  };

  // After the first slow open, eyes should be partially open (upper lid not fully closed).
  float u_open25 = upper_at_ms(static_cast<uint32_t>(static_cast<float>(duration) * 0.405f));
  TEST_ASSERT_LESS_THAN_FLOAT(0.90f, u_open25);
  TEST_ASSERT_GREATER_THAN_FLOAT(0.55f, u_open25);

  // Then it should snap shut.
  float u_close1 = upper_at_ms(static_cast<uint32_t>(static_cast<float>(duration) * 0.435f) - 10);
  TEST_ASSERT_GREATER_THAN_FLOAT(0.85f, u_close1);

  // Second open reaches much wider.
  float u_open75 = upper_at_ms(static_cast<uint32_t>(static_cast<float>(duration) * 0.665f));
  TEST_ASSERT_LESS_THAN_FLOAT(0.45f, u_open75);

  // And snaps shut again.
  float u_close2 = upper_at_ms(static_cast<uint32_t>(static_cast<float>(duration) * 0.695f) - 10);
  TEST_ASSERT_GREATER_THAN_FLOAT(0.85f, u_close2);

  // Near the end it should be very open (approaching 100%).
  float u_final = upper_at_ms(duration - 1);
  TEST_ASSERT_LESS_THAN_FLOAT(0.12f, u_final);
}
