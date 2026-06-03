#include "eyes/animator.h"

#include <math.h>

#include "eyes/easing.h"

namespace eyes {

namespace {

float blink_amount(float t, float start, float len) {
  if (len <= 0.0f) {
    return 0.0f;
  }
  float end = start + len;
  if (t < start || t > end) {
    return 0.0f;
  }

  float local = (t - start) / len; // 0..1
  float up = smoothstep(0.0f, 0.35f, local);
  float down = 1.0f - smoothstep(0.65f, 1.0f, local);
  return clamp01(up * down);
}

float wiggle(float t, float freq, float phase) {
  return sinf((t * freq + phase) * 2.0f * 3.1415926f);
}

float frac01(float x) {
  return x - floorf(x);
}

float hash01(uint32_t n) {
  // Deterministic 0..1 hash from integer index.
  float x = sinf(static_cast<float>(n) * 12.9898f + 78.233f) * 43758.5453f;
  return frac01(x);
}

float hash_seed01(uint32_t seed, uint32_t salt) {
  // Deterministic seed+salt mixer for clip-level variation.
  uint32_t x = seed ^ (0x9E3779B9u * (salt + 1u));
  x ^= x >> 16;
  x *= 0x7feb352du;
  x ^= x >> 15;
  x *= 0x846ca68bu;
  x ^= x >> 16;
  return hash01(x);
}

float tri01(float x) {
  float f = frac01(x);
  return 1.0f - fabsf(f * 2.0f - 1.0f);
}

float tri_signed(float x) {
  return tri01(x) * 2.0f - 1.0f;
}

struct Drift {
  float x;
  float y;
};

Drift alive_drift(float t) {
  // Multi-frequency group motion to make the eyes feel "alive".
  // Uses zero phase so the drift starts at 0.0 at t==0.
  float x = 0.22f * wiggle(t, 0.55f, 0.0f) + 0.11f * wiggle(t, 1.25f, 0.0f) +
            0.05f * wiggle(t, 2.35f, 0.0f);
  float y = 0.10f * wiggle(t, 0.45f, 0.0f) + 0.04f * wiggle(t, 1.60f, 0.0f) +
            0.02f * wiggle(t, 3.05f, 0.0f);
  return {.x = x, .y = y};
}

void apply_alive_motion(EyesPose &pose, float t, float intensity) {
  Drift d = alive_drift(t);
  d.x *= intensity;
  d.y *= intensity;

  pose.left.pupil_x += d.x;
  pose.right.pupil_x += d.x;
  pose.left.pupil_y += d.y;
  pose.right.pupil_y += d.y;
}

EyePose apply_blink(const EyePose &base, float blink) {
  EyePose out = base;
  out.lids.upper = clamp01(out.lids.upper + blink * 1.0f);
  out.lids.lower = clamp01(out.lids.lower + blink * 0.7f);
  return out;
}

} // namespace

EyesPose EyesAnimator::pose_for(Emotion emotion, uint32_t t_ms) const {
  return pose_for(emotion, t_ms, 0u);
}

EyesPose EyesAnimator::pose_for_runtime(Emotion emotion, uint32_t elapsed_ms) const {
  return pose_for_runtime(emotion, elapsed_ms, 0u);
}

EyesPose EyesAnimator::pose_for_runtime(Emotion emotion, uint32_t elapsed_ms,
                                        uint32_t variation_seed) const {
  uint32_t runtime_t_ms = emotion_runtime_time_ms(emotion, elapsed_ms);
  return pose_for(emotion, runtime_t_ms, variation_seed);
}

EyesPose EyesAnimator::pose_for(Emotion emotion, uint32_t t_ms, uint32_t variation_seed) const {
  EmotionClip clip = emotion_clip(emotion);
  uint32_t duration = clip.duration_ms ? clip.duration_ms : 4000;
  uint32_t local_ms = (duration == 0) ? 0 : (t_ms % duration);
  float t = (duration == 0) ? 0.0f : (static_cast<float>(local_ms) / static_cast<float>(duration));

  EyesPose pose;

  // Defaults (neutral-ish).
  pose.left.lids.upper = 0.0f;
  pose.left.lids.lower = 0.0f;
  pose.right = pose.left;

  if (emotion == Emotion::Neutral) {
    float b = blink_amount(t, 0.25f, 0.09f) + blink_amount(t, 0.78f, 0.08f);
    pose.left = apply_blink(pose.left, b);
    pose.right = apply_blink(pose.right, b);
    apply_alive_motion(pose, t, 0.75f);
    return pose;
  }

  if (emotion == Emotion::Happiness) {
    // Big, expressive happy motion: bounce the eyes around the frame as a group,
    // and rock (tilt) left/right while giggling.
    // Fast "jumping around" like a cartoon happy dance.
    const int hops = 14;
    float hf = t * static_cast<float>(hops);
    int hop = static_cast<int>(floorf(hf));
    float phase = hf - static_cast<float>(hop); // 0..1
    int i0 = hop % hops;
    int i1 = (i0 + 1) % hops;

    auto landing = [&](int idx, float *x, float *y, float *tilt) {
      float rx = hash01(static_cast<uint32_t>(idx) * 2u + 1u);
      float ry = hash01(static_cast<uint32_t>(idx) * 2u + 2u);
      float rt = hash01(static_cast<uint32_t>(idx) * 3u + 3u);
      *x = lerp(-0.85f, 0.85f, rx);
      *y = lerp(-0.50f, 0.50f, ry);
      *tilt = lerp(-0.90f, 0.90f, rt);
    };

    float x0 = 0.0f, y0 = 0.0f, t0 = 0.0f;
    float x1 = 0.0f, y1 = 0.0f, t1 = 0.0f;
    landing(i0, &x0, &y0, &t0);
    landing(i1, &x1, &y1, &t1);

    // Stay on the "landing" briefly, then move during airtime.
    float move = smoothstep(0.12f, 0.88f, phase);
    float base_x = lerp(x0, x1, move);
    float base_y = lerp(y0, y1, move);

    // Contact (squash) at the start/end of each hop.
    float contact_in = 1.0f - smoothstep(0.12f, 0.20f, phase);
    float contact_out = smoothstep(0.80f, 0.88f, phase);
    float contact = clamp01(contact_in + contact_out);

    // Lift (jump arc) in the middle of the hop.
    float lift = 0.0f;
    if (phase > 0.12f && phase < 0.88f) {
      float p = (phase - 0.12f) / 0.76f;
      lift = 4.0f * p * (1.0f - p);
    }

    float jump_h = lerp(0.65f, 0.95f, hash01(static_cast<uint32_t>(i0) * 7u + 5u));

    float x = base_x + (x1 - x0) * 0.10f * lift;
    float y = base_y - jump_h * lift + 0.14f * contact;

    // Squash on impact, stretch in the air.
    pose.left.pupil_scale = clamp(1.00f - 0.22f * contact + 0.32f * lift, 0.75f, 1.55f);
    pose.right.pupil_scale = pose.left.pupil_scale;
    pose.left.iris_scale = clamp(1.01f + 0.05f * contact - 0.04f * lift, 0.92f, 1.07f);
    pose.right.iris_scale = pose.left.iris_scale;

    // Eyelids tighten slightly on impact, open in the air.
    pose.left.lids.upper = clamp01(0.04f + 0.22f * contact);
    pose.left.lids.lower = clamp01(0.02f + 0.17f * contact);
    pose.right.lids.upper = pose.left.lids.upper;
    pose.right.lids.lower = pose.left.lids.lower;

    // Tilt rapidly left/right, with extra kick in the air.
    float tilt = lerp(t0, t1, move) + 0.35f * wiggle(t, 3.60f, 0.0f);
    tilt *= (0.55f + 0.45f * lift);
    tilt = clamp(tilt, -1.0f, 1.0f);
    pose.left.brow_tilt = tilt;
    pose.right.brow_tilt = tilt;
    pose.left.brow_raise = 0.0f;
    pose.right.brow_raise = 0.0f;

    pose.left.pupil_x = clamp(x, -1.0f, 1.0f);
    pose.right.pupil_x = pose.left.pupil_x;
    pose.left.pupil_y = clamp(y, -1.0f, 1.0f);
    pose.right.pupil_y = pose.left.pupil_y;

    // Tiny additive drift on top of the jump path.
    apply_alive_motion(pose, t, 0.08f);
    return pose;
  }

  if (emotion == Emotion::Curiosity) {
    // Wide, expressive eyes plus deliberate "look around" saccades.
    pose.left.iris_scale = 1.00f;
    pose.right.iris_scale = 1.00f;
    pose.left.pupil_scale = 1.00f;
    pose.right.pupil_scale = 1.00f;

    // Scan around for a bit while staying fully open...
    const float t_scan_end = 0.52f;
    const float t_tilt_end = 0.74f;
    const float t_close_end = 0.82f;
    // Reopen into tall eyes a bit faster after tilt+close.
    const float t_tall_open_end = 0.897f; // ~30% faster than 0.92 endpoint.

    float ts = (t_scan_end > 0.0f) ? clamp01(t / t_scan_end) : 1.0f;
    float scan_gx = 0.0f;
    float scan_gy = 0.0f;
    if (ts < 0.10f) {
      scan_gx = 0.0f;
      scan_gy = -0.02f;
    } else if (ts < 0.18f) {
      float u = smoothstep(0.10f, 0.18f, ts);
      scan_gx = lerp(0.0f, -0.55f, u);
      scan_gy = lerp(-0.02f, -0.18f, u);
    } else if (ts < 0.32f) {
      scan_gx = -0.55f;
      scan_gy = -0.18f;
    } else if (ts < 0.40f) {
      float u = smoothstep(0.32f, 0.40f, ts);
      scan_gx = lerp(-0.55f, 0.65f, u);
      scan_gy = lerp(-0.18f, -0.10f, u);
    } else if (ts < 0.56f) {
      scan_gx = 0.65f;
      scan_gy = -0.10f;
    } else if (ts < 0.64f) {
      float u = smoothstep(0.56f, 0.64f, ts);
      scan_gx = lerp(0.65f, 0.00f, u);
      scan_gy = lerp(-0.10f, 0.12f, u);
    } else if (ts < 0.78f) {
      scan_gx = 0.00f;
      scan_gy = 0.12f;
    } else if (ts < 0.86f) {
      float u = smoothstep(0.78f, 0.86f, ts);
      scan_gx = lerp(0.00f, -0.25f, u);
      scan_gy = lerp(0.12f, 0.02f, u);
    } else {
      scan_gx = -0.25f;
      scan_gy = 0.02f;
    }

    // Tiny micro-movement even while holding a target.
    scan_gx += 0.035f * wiggle(t, 2.40f, 0.0f);
    scan_gy += 0.025f * wiggle(t, 2.10f, 0.0f);

    // ...then smoothly transition into a tilted side-look (no snap).
    float settle_u = smoothstep(t_scan_end, t_tilt_end, t);
    float target_gx = 0.22f + 0.02f * wiggle(t, 2.10f, 0.0f);
    float target_gy = -0.14f + 0.02f * wiggle(t, 2.40f, 0.0f);

    float gx = lerp(scan_gx, target_gx, settle_u);
    float gy = lerp(scan_gy, target_gy, settle_u);

    pose.left.pupil_x = clamp(gx, -0.80f, 0.80f);
    pose.right.pupil_x = pose.left.pupil_x;
    pose.left.pupil_y = clamp(gy, -0.40f, 0.40f);
    pose.right.pupil_y = pose.left.pupil_y;

    float tilt = lerp(0.0f, 1.0f, settle_u);
    pose.left.brow_tilt = tilt;
    pose.right.brow_tilt = tilt;

    // After the side-tilt: close the eyes, then reopen into the tall shape.
    float close_amt = 0.0f;
    if (t < t_tilt_end) {
      close_amt = 0.0f;
    } else if (t < t_close_end) {
      close_amt = smoothstep(t_tilt_end, t_close_end, t);
    } else if (t < t_tall_open_end) {
      close_amt = 1.0f - smoothstep(t_close_end, t_tall_open_end, t);
    } else {
      close_amt = 0.0f;
    }
    pose.left = apply_blink(pose.left, close_amt);
    pose.right = apply_blink(pose.right, close_amt);

    float tall_u = (t < t_close_end) ? 0.0f : smoothstep(t_close_end, t_tall_open_end, t);
    float scale = lerp(1.00f, 1.60f, tall_u); // 60% taller than normal
    pose.left.pupil_scale = scale;
    pose.right.pupil_scale = scale;

    pose.left.brow_raise = 0.0f;
    pose.right.brow_raise = 0.0f;

    apply_alive_motion(pose, t, 0.18f);
    return pose;
  }

  if (emotion == Emotion::Exhaustion) {
    // Heavy lids, slow sway, and a long tired blink.
    float sag = 0.08f * (0.5f + 0.5f * wiggle(t, 0.45f, 0.0f)); // 0..0.08

    pose.left.lids.upper = clamp01(0.74f + sag);
    pose.left.lids.lower = clamp01(0.34f + sag * 0.55f);
    pose.right.lids.upper = pose.left.lids.upper;
    pose.right.lids.lower = pose.left.lids.lower;

    pose.left.iris_scale = 0.98f;
    pose.right.iris_scale = 0.98f;
    pose.left.pupil_scale = 0.92f;
    pose.right.pupil_scale = 0.92f;

    // Droop down and wobble like you're carrying a heavy head.
    float droop = 0.18f + 0.05f * wiggle(t, 0.30f, 0.0f);
    float sway = 0.07f * wiggle(t, 0.22f, 0.0f);
    pose.left.pupil_x = sway;
    pose.right.pupil_x = sway;
    pose.left.pupil_y = droop;
    pose.right.pupil_y = droop;

    pose.left.brow_raise = 0.0f;
    pose.right.brow_raise = 0.0f;
    float head = -0.10f + 0.08f * wiggle(t, 0.18f, 0.0f);
    pose.left.brow_tilt = head;
    pose.right.brow_tilt = head;

    float slow = 0.85f * blink_amount(t, 0.30f, 0.22f) + 0.55f * blink_amount(t, 0.74f, 0.20f);
    pose.left = apply_blink(pose.left, slow);
    pose.right = apply_blink(pose.right, slow);

    // Small involuntary twitch on the right eye.
    auto twitch_window = [&](float start, float end, float amp) -> float {
      if (t < start || t > end || end <= start) {
        return 0.0f;
      }
      float u = (t - start) / (end - start);
      float env = sinf(u * 3.1415926f); // fade in/out
      return amp * env * sinf(u * 6.0f * 3.1415926f);
    };

    float twitch = 0.0f;
    twitch += twitch_window(0.16f, 0.22f, 0.06f);
    twitch += twitch_window(0.48f, 0.54f, 0.05f);
    twitch += twitch_window(0.82f, 0.88f, 0.06f);

    pose.right.pupil_x = clamp(pose.right.pupil_x + twitch, -1.0f, 1.0f);
    pose.right.pupil_y = clamp(pose.right.pupil_y + 0.25f * twitch, -1.0f, 1.0f);
    pose.right.lids.upper = clamp01(pose.right.lids.upper + fabsf(twitch) * 0.14f);
    pose.right.lids.lower = clamp01(pose.right.lids.lower + fabsf(twitch) * 0.09f);

    apply_alive_motion(pose, t, 0.35f);
    return pose;
  }

  if (emotion == Emotion::FallingAsleep) {
    // Slowly close over the clip.
    float close_t = ease_in_out(t);
    float base = lerp(0.45f, 0.98f, close_t);
    pose.left.lids.upper = base;
    pose.left.lids.lower = base * 0.85f;
    pose.right.lids.upper = base;
    pose.right.lids.lower = base * 0.85f;

    // Wobble around like you're struggling to stay upright.
    float wobble_amp = lerp(0.32f, 0.18f, close_t);
    float wobble_x = wobble_amp * wiggle(t, 1.0f, 0.00f);
    float wobble_y = wobble_amp * 0.65f * wiggle(t, 1.0f, 0.25f);

    pose.left.pupil_x = wobble_x;
    pose.right.pupil_x = wobble_x;
    pose.left.pupil_y = 0.25f + wobble_y;
    pose.right.pupil_y = 0.25f + wobble_y;

    pose.left.iris_scale = 0.95f;
    pose.right.iris_scale = 0.95f;
    pose.left.pupil_scale = 0.90f;
    pose.right.pupil_scale = 0.90f;
    pose.left.brow_raise = -0.35f;  // arc curvature if fully closed
    pose.right.brow_raise = -0.35f; // arc curvature if fully closed

    // Brief reopen "fight it" moment near the start.
    float fight = smoothstep(0.12f, 0.22f, t) * (1.0f - smoothstep(0.22f, 0.32f, t));
    pose.left.lids.upper = clamp01(pose.left.lids.upper - 0.20f * fight);
    pose.right.lids.upper = clamp01(pose.right.lids.upper - 0.20f * fight);

    apply_alive_motion(pose, t, 0.15f);
    return pose;
  }

  if (emotion == Emotion::WakingUp) {
    // Start closed, then "practice blink" twice while waking:
    // 1) slow open to ~25% then snap shut
    // 2) open to ~75% then snap shut
    // 3) open to ~90%, pause, then slowly open to 100%
    //
    // Timing tweaks ("slower" as in reduced speed):
    // - 0% -> 25% open: 50% slower => 2x the time.
    // - Reaching 50% open (during the 0% -> 75% segment): 20% slower => 1.25x the time.
    const float t0 = 0.03f;  // hold closed
    const float t1 = 0.405f; // slow open to ~25%
    const float t2 = 0.435f; // snap shut
    const float t3 = 0.665f; // open to ~75%
    const float t4 = 0.695f; // snap shut
    const float t5 = 0.815f; // open to ~90%
    const float t6 = 0.875f; // pause at ~90%

    float open = 0.0f; // 0 closed, 1 fully open
    if (t < t0) {
      open = 0.0f;
    } else if (t < t1) {
      float u = smoothstep(t0, t1, t);
      open = lerp(0.0f, 0.25f, u);
    } else if (t < t2) {
      float u = smoothstep(t1, t2, t);
      open = lerp(0.25f, 0.0f, u);
    } else if (t < t3) {
      float u = smoothstep(t2, t3, t);
      // Time-warp so reaching 50% open is ~20% slower.
      u = powf(u, 2.14f);
      open = lerp(0.0f, 0.75f, u);
    } else if (t < t4) {
      float u = smoothstep(t3, t4, t);
      open = lerp(0.75f, 0.0f, u);
    } else if (t < t5) {
      float u = smoothstep(t4, t5, t);
      open = lerp(0.0f, 0.90f, u);
    } else if (t < t6) {
      open = 0.90f;
    } else {
      float u = smoothstep(t6, 1.0f, t);
      open = lerp(0.90f, 1.0f, u);
    }

    open = clamp01(open);

    float upper = clamp01(1.0f - open);

    pose.left.lids.upper = upper;
    pose.left.lids.lower = upper;
    pose.right.lids.upper = upper;
    pose.right.lids.lower = upper;

    // Quick pupil jitter while opening.
    float jit = clamp01(1.0f - open);
    float jx = 0.18f * wiggle(t, 6.0f, 0.00f) * jit;
    float jy = 0.12f * wiggle(t, 5.0f, 0.33f) * jit;
    pose.left.pupil_x = jx;
    pose.right.pupil_x = jx;
    pose.left.pupil_y = jy;
    pose.right.pupil_y = jy;

    pose.left.iris_scale = 1.0f;
    pose.right.iris_scale = 1.0f;
    pose.left.brow_raise = 0.0f;
    pose.right.brow_raise = 0.0f;

    apply_alive_motion(pose, t, 0.22f);
    return pose;
  }

  if (emotion == Emotion::Snoring) {
    // Eyes closed, subtle breathing motion via lower lid.
    float breath = 0.10f * (0.5f + 0.5f * wiggle(t, 0.7f, 0.00f));
    pose.left.lids.upper = 1.0f;
    pose.left.lids.lower = clamp01(1.0f - breath);
    pose.right.lids.upper = 1.0f;
    pose.right.lids.lower = clamp01(1.0f - breath);

    pose.left.pupil_x = 0.0f;
    pose.left.pupil_y = 0.0f;
    pose.right.pupil_x = 0.0f;
    pose.right.pupil_y = 0.0f;
    pose.left.brow_raise = -0.25f;
    pose.right.brow_raise = -0.25f;

    // Snoring 'Z' stream: five glyphs marching up/right and growing.
    pose.effects.snore_zs.count = 5;
    for (int i = 0; i < 5; ++i) {
      float p = t + (static_cast<float>(i) / 5.0f);
      p = p - floorf(p); // frac()
      pose.effects.snore_zs.progress[i] = p;
    }

    // Tiny drift so even snoring feels alive.
    apply_alive_motion(pose, t, 0.15f);
    return pose;
  }

  if (emotion == Emotion::Dizzy) {
    pose.left.lids.upper = clamp01(0.26f + 0.05f * wiggle(t, 3.20f, 0.0f));
    pose.left.lids.lower = 0.12f;
    pose.right = pose.left;

    float gx = 0.28f * wiggle(t, 3.80f, 0.0f);
    float gy = 0.22f * wiggle(t, 3.20f, 0.25f);
    pose.left.pupil_x = gx;
    pose.right.pupil_x = gx;
    pose.left.pupil_y = gy;
    pose.right.pupil_y = gy;

    pose.left.iris_scale = 0.98f;
    pose.right.iris_scale = 0.98f;
    pose.left.pupil_scale = clamp(1.02f + 0.10f * wiggle(t, 2.60f, 0.2f), 0.85f, 1.25f);
    pose.right.pupil_scale = pose.left.pupil_scale;

    pose.left.brow_tilt = clamp(0.30f * wiggle(t, 2.00f, 0.0f), -1.0f, 1.0f);
    pose.right.brow_tilt = pose.left.brow_tilt;
    pose.left.brow_raise = 0.0f;
    pose.right.brow_raise = 0.0f;

    pose.effects.dizzy_birds.count = 3;
    for (int i = 0; i < 3; ++i) {
      pose.effects.dizzy_birds.phase[i] = frac01(t * 1.85f + static_cast<float>(i) / 3.0f);
    }

    apply_alive_motion(pose, t, 0.20f);
    return pose;
  }

  if (emotion == Emotion::PlayfulPong) {
    // Timeline:
    // 0.00..0.14 ball separates from the right eye
    // 0.08..0.30 eyes morph into paddles and move to the edges
    // 0.30..0.84 pong play
    // 0.84..0.92 right side loses
    // 0.86..1.00 paddles move back to center and expand into eyes
    float paddle_in = smoothstep(0.08f, 0.30f, t);
    float paddle_out = smoothstep(0.86f, 1.00f, t);
    float paddle_morph = clamp01(paddle_in * (1.0f - paddle_out));

    float game_u = clamp01((t - 0.30f) / 0.54f);
    float lose_u = smoothstep(0.84f, 0.92f, t);

    bool has_variation = (variation_seed != 0u);
    // Keep loop endpoints seamless: use integer cycle counts across game_u in [0, 1].
    // This ensures xphase/by trajectories line up when the sustain loop wraps.
    bool fast_rally = has_variation ? (hash_seed01(variation_seed, 1u) > 0.5f) : true;
    float xphase_scale = fast_rally ? 3.0f : 2.0f;
    float xphase_offset = has_variation ? lerp(0.02f, 0.22f, hash_seed01(variation_seed, 2u)) : 0.08f;

    int by_bounce_cycles = has_variation ? ((hash_seed01(variation_seed, 3u) > 0.5f) ? 3 : 2) : 2;
    float by_phase = has_variation ? lerp(0.10f, 0.90f, hash_seed01(variation_seed, 4u)) : 0.31f;
    // Keep Y bounds high so the rendered ball clearly reaches top/bottom before reversing.
    float by_wall = has_variation ? lerp(1.06f, 1.14f, hash_seed01(variation_seed, 5u)) : 1.10f;

    auto by_from_u = [&](float u) -> float {
      // True wall-bounce motion: direction changes only at the vertical bounds.
      float tri = tri_signed(u * static_cast<float>(by_bounce_cycles) + by_phase);
      return by_wall * tri;
    };

    // Ball trajectory: left-right triangular motion with vertical variation.
    float xphase = game_u * xphase_scale + xphase_offset;
    float bx = 0.94f * tri_signed(xphase);
    float by = by_from_u(game_u);

    // Emerging phase from right eye area.
    float emerge_u = smoothstep(0.00f, 0.14f, t);
    bx = lerp(0.58f, bx, emerge_u);
    float emerge_y = has_variation ? lerp(-0.10f, 0.08f, hash_seed01(variation_seed, 6u)) : -0.03f;
    by = lerp(emerge_y, by, emerge_u);

    // Losing shot: right paddle misses and ball exits right.
    bx = lerp(bx, 1.24f, lose_u);
    float lose_y = has_variation ? lerp(-0.30f, 0.45f, hash_seed01(variation_seed, 7u)) : 0.30f;
    by = lerp(by, lose_y, lose_u);

    // Receiver-only paddle tracking:
    // each paddle moves only while the ball is heading toward that side.
    float phase = frac01(xphase);
    float cycle = floorf(xphase);
    bool toward_right = (phase < 0.5f);

    float left_target = clamp(by * 0.92f, -0.92f, 0.92f);
    float right_target = clamp(by * 0.92f, -0.92f, 0.92f);

    // Hold values captured at side contacts.
    // left_hold: phase 0.0 of current cycle (left wall contact)
    // right_hold_idle: phase 0.5 of current cycle (right wall contact), used while right idles.
    // right_hold_receive: phase 0.5 of previous cycle, used when right resumes receiving at phase wrap.
    float left_hold_u = (cycle - xphase_offset) / xphase_scale;
    float right_hold_idle_u = (cycle + 0.5f - xphase_offset) / xphase_scale;
    float right_hold_receive_u = (cycle - 0.5f - xphase_offset) / xphase_scale;
    float left_hold = clamp(by_from_u(left_hold_u) * 0.92f, -0.92f, 0.92f);
    float right_hold_idle = clamp(by_from_u(right_hold_idle_u) * 0.92f, -0.92f, 0.92f);
    float right_hold_receive = clamp(by_from_u(right_hold_receive_u) * 0.92f, -0.92f, 0.92f);

    // Blend the receiving paddle from hold -> target to avoid abrupt jumps.
    float right_receive_u = toward_right ? clamp01(phase / 0.5f) : 0.0f;
    float left_receive_u = toward_right ? 0.0f : clamp01((phase - 0.5f) / 0.5f);
    float right_blend = smoothstep(0.0f, 0.55f, right_receive_u);
    float left_blend = smoothstep(0.0f, 0.55f, left_receive_u);

    float left_track = toward_right ? left_hold : lerp(left_hold, left_target, left_blend);
    float right_track = toward_right ? lerp(right_hold_receive, right_target, right_blend) : right_hold_idle;

    // Losing shot: right paddle reaches late and misses downward while ball exits right.
    right_track = lerp(right_track, -0.96f, lose_u);
    left_track = lerp(left_track, left_hold, lose_u);

    float track_gate = smoothstep(0.16f, 0.30f, t) * (1.0f - smoothstep(0.88f, 1.00f, t));
    left_track = lerp(0.0f, left_track, track_gate);
    right_track = lerp(0.0f, right_track, track_gate);

    // Keep eye geometry coherent during blend (renderer handles full edge paddles).
    pose.left.lids.upper = lerp(0.05f, 0.00f, paddle_morph);
    pose.left.lids.lower = lerp(0.02f, 0.00f, paddle_morph);
    pose.right.lids.upper = pose.left.lids.upper;
    pose.right.lids.lower = pose.left.lids.lower;

    pose.left.iris_scale = lerp(1.04f, 0.18f, paddle_morph);
    pose.right.iris_scale = pose.left.iris_scale;
    pose.left.pupil_scale = lerp(1.06f, 2.40f, paddle_morph);
    pose.right.pupil_scale = pose.left.pupil_scale;

    pose.left.pupil_x = lerp(0.0f, -1.0f, paddle_morph);
    pose.right.pupil_x = lerp(0.0f, 1.0f, paddle_morph);
    pose.left.pupil_y = lerp(-0.03f, left_track, paddle_morph);
    pose.right.pupil_y = lerp(-0.03f, right_track, paddle_morph);

    pose.left.brow_tilt = 0.0f;
    pose.right.brow_tilt = 0.0f;
    pose.left.brow_raise = 0.0f;
    pose.right.brow_raise = 0.0f;

    float ball_fade = 1.0f - smoothstep(0.97f, 1.00f, t);
    float ball_radius_hi =
        has_variation ? lerp(0.018f, 0.023f, hash_seed01(variation_seed, 8u)) : 0.020f;
    float ball_radius = lerp(0.010f, ball_radius_hi, emerge_u) * ball_fade;

    pose.effects.pong_ball.active = (ball_radius > 0.0005f || paddle_morph > 0.0005f) ? 1 : 0;
    pose.effects.pong_ball.x = bx;
    pose.effects.pong_ball.y = by;
    pose.effects.pong_ball.radius = ball_radius;
    pose.effects.pong_ball.droplet = 1.0f - smoothstep(0.03f, 0.18f, t);
    pose.effects.pong_ball.paddle_morph = paddle_morph;
    pose.effects.pong_ball.left_paddle_y = left_track;
    pose.effects.pong_ball.right_paddle_y = right_track;

    return pose;
  }

  return pose;
}

} // namespace eyes
