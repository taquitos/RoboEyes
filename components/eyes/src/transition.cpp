#include "eyes/transition.h"

#include "eyes/easing.h"

namespace eyes {

namespace {

struct EffectMask {
  bool snore = false;
  bool dizzy = false;
  bool pong = false;
};

bool snore_active(const SnoreZsPose &s) {
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

bool dizzy_active(const DizzyBirdsPose &d) {
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

bool pong_active(const PongBallPose &p) {
  return (p.active != 0) || (p.radius > 0.0005f) || (p.droplet > 0.0005f) ||
         (p.paddle_morph > 0.0005f);
}

EffectMask effect_mask_for_pose(const EyesPose &p) {
  EffectMask mask;
  mask.snore = snore_active(p.effects.snore_zs);
  mask.dizzy = dizzy_active(p.effects.dizzy_birds);
  mask.pong = pong_active(p.effects.pong_ball);
  return mask;
}

bool effect_mask_any(const EffectMask &m) {
  return m.snore || m.dizzy || m.pong;
}

bool effect_mask_equal(const EffectMask &a, const EffectMask &b) {
  return a.snore == b.snore && a.dizzy == b.dizzy && a.pong == b.pong;
}

void clear_snore(SnoreZsPose *s) {
  if (!s) {
    return;
  }
  s->count = 0;
  for (int i = 0; i < 5; ++i) {
    s->progress[i] = 1.0f;
  }
}

void clear_dizzy(DizzyBirdsPose *d) {
  if (!d) {
    return;
  }
  d->count = 0;
  for (int i = 0; i < 3; ++i) {
    d->phase[i] = 0.0f;
    d->fly_away[i] = 1.0f;
  }
}

void clear_pong(PongBallPose *p) {
  if (!p) {
    return;
  }
  p->active = 0;
  p->radius = 0.0f;
  p->droplet = 0.0f;
  p->paddle_morph = 0.0f;
}

void gate_outgoing_snore(SnoreZsPose *s, float keep_gain) {
  if (!s || s->count == 0) {
    return;
  }
  float push = 1.0f - clamp01(keep_gain);
  int count = static_cast<int>(s->count);
  if (count > 5) {
    count = 5;
  }
  for (int i = 0; i < count; ++i) {
    s->progress[i] = clamp01(lerp(s->progress[i], 1.0f, push));
  }
  if (keep_gain < 0.08f) {
    clear_snore(s);
  }
}

void gate_incoming_snore(SnoreZsPose *s, float show_gain) {
  if (!s || s->count == 0) {
    return;
  }
  float g = clamp01(show_gain);
  if (g < 0.08f) {
    clear_snore(s);
    return;
  }
  int target = static_cast<int>(s->count);
  if (target > 5) {
    target = 5;
  }
  int visible = static_cast<int>(static_cast<float>(target) * g + 0.5f);
  if (visible < 1) {
    visible = 1;
  }
  if (visible > target) {
    visible = target;
  }
  for (int i = visible; i < 5; ++i) {
    s->progress[i] = 1.0f;
  }
  s->count = static_cast<uint8_t>(visible);
}

void gate_outgoing_dizzy(DizzyBirdsPose *d, float keep_gain) {
  if (!d || d->count == 0) {
    return;
  }
  float push = 1.0f - clamp01(keep_gain);
  int count = static_cast<int>(d->count);
  if (count > 3) {
    count = 3;
  }
  for (int i = 0; i < count; ++i) {
    d->fly_away[i] = clamp01(lerp(d->fly_away[i], 1.0f, push));
  }
  if (keep_gain < 0.08f) {
    clear_dizzy(d);
  }
}

void gate_incoming_dizzy(DizzyBirdsPose *d, float show_gain) {
  if (!d || d->count == 0) {
    return;
  }
  float g = clamp01(show_gain);
  if (g < 0.08f) {
    clear_dizzy(d);
    return;
  }
  int target = static_cast<int>(d->count);
  if (target > 3) {
    target = 3;
  }
  int visible = static_cast<int>(static_cast<float>(target) * g + 0.5f);
  if (visible < 1) {
    visible = 1;
  }
  if (visible > target) {
    visible = target;
  }
  d->count = static_cast<uint8_t>(visible);
}

void gate_outgoing_pong(PongBallPose *p, float keep_gain) {
  if (!p) {
    return;
  }
  float g = clamp01(keep_gain);
  p->radius *= g;
  p->droplet *= g;
  p->paddle_morph *= g;
  p->active = (p->radius > 0.0005f || p->paddle_morph > 0.0005f) ? 1 : 0;
}

void gate_incoming_pong(PongBallPose *p, float show_gain, bool to_active_hint) {
  if (!p) {
    return;
  }
  float g = clamp01(show_gain);
  if (g < 0.08f) {
    clear_pong(p);
    return;
  }
  p->radius *= g;
  p->droplet *= g;
  p->paddle_morph *= g;
  p->active = (p->radius > 0.0005f || p->paddle_morph > 0.0005f || to_active_hint) ? 1 : 0;
}

void apply_effect_handoff(const EyesPose &from, const EyesPose &to, float alpha, EyesPose *out) {
  if (!out) {
    return;
  }

  EffectMask from_mask = effect_mask_for_pose(from);
  EffectMask to_mask = effect_mask_for_pose(to);

  // General handoff rule: when switching between different overlay-effect families,
  // avoid rendering both families strongly at the same moment.
  if (!effect_mask_any(from_mask) || !effect_mask_any(to_mask) || effect_mask_equal(from_mask, to_mask)) {
    return;
  }

  float a = clamp01(alpha);
  float outgoing_keep = 1.0f - smoothstep(0.18f, 0.62f, a);
  float incoming_show = smoothstep(0.42f, 0.92f, a);

  if (from_mask.snore && !to_mask.snore) {
    gate_outgoing_snore(&out->effects.snore_zs, outgoing_keep);
  }
  if (from_mask.dizzy && !to_mask.dizzy) {
    gate_outgoing_dizzy(&out->effects.dizzy_birds, outgoing_keep);
  }
  if (from_mask.pong && !to_mask.pong) {
    gate_outgoing_pong(&out->effects.pong_ball, outgoing_keep);
  }

  if (!from_mask.snore && to_mask.snore) {
    gate_incoming_snore(&out->effects.snore_zs, incoming_show);
  }
  if (!from_mask.dizzy && to_mask.dizzy) {
    gate_incoming_dizzy(&out->effects.dizzy_birds, incoming_show);
  }
  if (!from_mask.pong && to_mask.pong) {
    gate_incoming_pong(&out->effects.pong_ball, incoming_show, to.effects.pong_ball.active != 0);
  }
}

EyeLids blend_lids(const EyeLids &from, const EyeLids &to, float alpha) {
  EyeLids out;
  out.upper = clamp01(lerp(from.upper, to.upper, alpha));
  out.lower = clamp01(lerp(from.lower, to.lower, alpha));
  return out;
}

EyePose blend_eye(const EyePose &from, const EyePose &to, float alpha) {
  EyePose out;
  out.lids = blend_lids(from.lids, to.lids, alpha);
  out.pupil_x = lerp(from.pupil_x, to.pupil_x, alpha);
  out.pupil_y = lerp(from.pupil_y, to.pupil_y, alpha);
  out.iris_scale = lerp(from.iris_scale, to.iris_scale, alpha);
  out.pupil_scale = lerp(from.pupil_scale, to.pupil_scale, alpha);
  out.brow_raise = lerp(from.brow_raise, to.brow_raise, alpha);
  out.brow_tilt = lerp(from.brow_tilt, to.brow_tilt, alpha);
  return out;
}

SnoreZsPose blend_snore(const SnoreZsPose &from, const SnoreZsPose &to, float alpha) {
  SnoreZsPose out;

  // Special case for non-snoring -> snoring transitions:
  // keep Z progress on the target timeline (no progress interpolation from 0),
  // so incoming Z drift speed does not accelerate during transition.
  if (from.count == 0 && to.count > 0) {
    float count_f = lerp(static_cast<float>(from.count), static_cast<float>(to.count), alpha);
    int count_i = static_cast<int>(count_f + 0.5f);
    if (count_i < 0) {
      count_i = 0;
    }
    if (count_i > 5) {
      count_i = 5;
    }
    out.count = static_cast<uint8_t>(count_i);
    for (int i = 0; i < 5; ++i) {
      if (i < static_cast<int>(to.count)) {
        out.progress[i] = clamp01(to.progress[i]);
      } else {
        out.progress[i] = 1.0f;
      }
    }
    return out;
  }

  // Special case for snoring -> non-snoring transitions:
  // stop spawning new Zs and let currently-visible Zs continue drifting off-screen.
  if (from.count > 0 && to.count == 0) {
    out.count = from.count;
    for (int i = 0; i < 5; ++i) {
      if (i < static_cast<int>(from.count)) {
        float p = clamp01(from.progress[i]);
        out.progress[i] = clamp01(lerp(p, 1.0f, alpha));
      } else {
        out.progress[i] = 1.0f;
      }
    }
    return out;
  }

  float count_f = lerp(static_cast<float>(from.count), static_cast<float>(to.count), alpha);
  int count_i = static_cast<int>(count_f + 0.5f);
  if (count_i < 0) {
    count_i = 0;
  }
  if (count_i > 5) {
    count_i = 5;
  }
  out.count = static_cast<uint8_t>(count_i);

  for (int i = 0; i < 5; ++i) {
    float a = (i < static_cast<int>(from.count)) ? from.progress[i] : 0.0f;
    float b = (i < static_cast<int>(to.count)) ? to.progress[i] : 0.0f;
    out.progress[i] = clamp01(lerp(a, b, alpha));
  }
  return out;
}

DizzyBirdsPose blend_birds(const DizzyBirdsPose &from, const DizzyBirdsPose &to, float alpha) {
  DizzyBirdsPose out;

  // Special case for non-dizzy -> dizzy transitions:
  // keep orbit phase on the target track (no phase interpolation from 0),
  // so birds do not appear to accelerate when entering dizzy.
  if (from.count == 0 && to.count > 0) {
    float count_f = lerp(static_cast<float>(from.count), static_cast<float>(to.count), alpha);
    int count_i = static_cast<int>(count_f + 0.5f);
    if (count_i < 0) {
      count_i = 0;
    }
    if (count_i > 3) {
      count_i = 3;
    }
    out.count = static_cast<uint8_t>(count_i);

    for (int i = 0; i < 3; ++i) {
      if (i < static_cast<int>(to.count)) {
        out.phase[i] = to.phase[i];
        out.fly_away[i] = clamp01(to.fly_away[i]);
      } else {
        out.phase[i] = 0.0f;
        out.fly_away[i] = 0.0f;
      }
    }
    return out;
  }

  // Special case for dizzy -> non-dizzy transitions:
  // keep birds moving forward and mark them to fly away instead of reversing.
  if (from.count > 0 && to.count == 0) {
    out.count = from.count;
    for (int i = 0; i < 3; ++i) {
      if (i < static_cast<int>(from.count)) {
        out.phase[i] = from.phase[i] + alpha * 0.22f;
        out.fly_away[i] = clamp01(alpha);
      } else {
        out.phase[i] = 0.0f;
        out.fly_away[i] = 1.0f;
      }
    }
    return out;
  }

  float count_f = lerp(static_cast<float>(from.count), static_cast<float>(to.count), alpha);
  int count_i = static_cast<int>(count_f + 0.5f);
  if (count_i < 0) {
    count_i = 0;
  }
  if (count_i > 3) {
    count_i = 3;
  }
  out.count = static_cast<uint8_t>(count_i);

  for (int i = 0; i < 3; ++i) {
    float a = (i < static_cast<int>(from.count)) ? from.phase[i] : 0.0f;
    float b = (i < static_cast<int>(to.count)) ? to.phase[i] : 0.0f;
    out.phase[i] = lerp(a, b, alpha);
    float fa = (i < static_cast<int>(from.count)) ? from.fly_away[i] : 0.0f;
    float fb = (i < static_cast<int>(to.count)) ? to.fly_away[i] : 0.0f;
    out.fly_away[i] = clamp01(lerp(fa, fb, alpha));
  }
  return out;
}

PongBallPose blend_pong_ball(const PongBallPose &from, const PongBallPose &to, float alpha) {
  PongBallPose out;

  bool from_active = pong_active(from);
  bool to_active = pong_active(to);

  // For entering Pong, keep motion channels on the target timeline and only
  // blend visibility/morph channels via the handoff logic.
  if (!from_active && to_active) {
    out.x = to.x;
    out.y = to.y;
    out.radius = clamp01(to.radius);
    out.droplet = clamp01(to.droplet);
    out.paddle_morph = clamp01(to.paddle_morph);
    out.left_paddle_y = to.left_paddle_y;
    out.right_paddle_y = to.right_paddle_y;
    out.active = to.active;
    return out;
  }

  // For exiting Pong, keep motion channels on the outgoing track (no lerp to
  // neutral), then fade the effect.
  if (from_active && !to_active) {
    out.x = from.x;
    out.y = from.y;
    out.radius = clamp01(from.radius);
    out.droplet = clamp01(from.droplet);
    out.paddle_morph = clamp01(from.paddle_morph);
    out.left_paddle_y = from.left_paddle_y;
    out.right_paddle_y = from.right_paddle_y;

    float fade = 1.0f - smoothstep(0.0f, 1.0f, clamp01(alpha * 1.8f));
    out.radius *= fade;
    out.droplet *= fade;
    out.paddle_morph *= fade;
    out.active = (out.radius > 0.0005f || out.paddle_morph > 0.0005f) ? 1 : 0;
    return out;
  }

  out.x = lerp(from.x, to.x, alpha);
  out.y = lerp(from.y, to.y, alpha);
  out.radius = clamp01(lerp(from.radius, to.radius, alpha));
  out.droplet = clamp01(lerp(from.droplet, to.droplet, alpha));
  out.paddle_morph = clamp01(lerp(from.paddle_morph, to.paddle_morph, alpha));
  out.left_paddle_y = lerp(from.left_paddle_y, to.left_paddle_y, alpha);
  out.right_paddle_y = lerp(from.right_paddle_y, to.right_paddle_y, alpha);

  // General handoff rule: when leaving Pong (effectful -> non-effectful), settle the
  // visual residue quickly so outgoing paddles/ball don't leak into unrelated emotions.
  if (from.active && !to.active) {
    float fade = 1.0f - smoothstep(0.0f, 1.0f, clamp01(alpha * 1.8f));
    out.radius *= fade;
    out.droplet *= fade;
    out.paddle_morph *= fade;
  }

  out.active = (out.radius > 0.0005f || out.paddle_morph > 0.0005f || to.active) ? 1 : 0;
  return out;
}

} // namespace

EyesPose blend_poses(const EyesPose &from, const EyesPose &to, float alpha) {
  float a = clamp01(alpha);

  EyesPose out;
  out.left = blend_eye(from.left, to.left, a);
  out.right = blend_eye(from.right, to.right, a);
  out.effects.snore_zs = blend_snore(from.effects.snore_zs, to.effects.snore_zs, a);
  out.effects.dizzy_birds = blend_birds(from.effects.dizzy_birds, to.effects.dizzy_birds, a);
  out.effects.pong_ball = blend_pong_ball(from.effects.pong_ball, to.effects.pong_ball, a);
  apply_effect_handoff(from, to, a, &out);
  return out;
}

} // namespace eyes
