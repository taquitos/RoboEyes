#include "eyes/renderer.h"

#include <math.h>

#include "eyes/easing.h"

namespace eyes {

namespace {

struct EyeGeom {
  float cx;
  float cy;
  float hx;
  float hy;
};

inline void set_px(PixelBuffer8 &buf, int x, int y, uint8_t v) {
  if (x < 0 || y < 0 || x >= buf.width() || y >= buf.height()) {
    return;
  }
  buf.set(x, y, v);
}

void draw_thick_point(PixelBuffer8 &buf, int x, int y, int r, uint8_t v) {
  for (int dy = -r; dy <= r; ++dy) {
    for (int dx = -r; dx <= r; ++dx) {
      if (dx * dx + dy * dy <= r * r) {
        set_px(buf, x + dx, y + dy, v);
      }
    }
  }
}

void draw_filled_circle(PixelBuffer8 &buf, int cx, int cy, int radius, uint8_t v) {
  if (radius <= 0) {
    set_px(buf, cx, cy, v);
    return;
  }
  for (int dy = -radius; dy <= radius; ++dy) {
    int rr = radius * radius - dy * dy;
    if (rr < 0) {
      continue;
    }
    int dx_lim = static_cast<int>(sqrtf(static_cast<float>(rr)));
    for (int dx = -dx_lim; dx <= dx_lim; ++dx) {
      set_px(buf, cx + dx, cy + dy, v);
    }
  }
}

void draw_line(PixelBuffer8 &buf, int x0, int y0, int x1, int y1, int radius, uint8_t v) {
  int dx = x1 - x0;
  int dy = y1 - y0;
  int adx = (dx < 0) ? -dx : dx;
  int ady = (dy < 0) ? -dy : dy;
  int steps = (adx > ady) ? adx : ady;
  if (steps <= 0) {
    draw_thick_point(buf, x0, y0, radius, v);
    return;
  }
  for (int i = 0; i <= steps; ++i) {
    float t = static_cast<float>(i) / static_cast<float>(steps);
    int x = static_cast<int>(x0 + static_cast<float>(dx) * t + 0.5f);
    int y = static_cast<int>(y0 + static_cast<float>(dy) * t + 0.5f);
    draw_thick_point(buf, x, y, radius, v);
  }
}

void local_to_screen(int cx, int cy, int face_sign, float yaw_scale, float lx, float ly, int *sx,
                     int *sy) {
  float x = static_cast<float>(cx) + static_cast<float>(face_sign) * lx * yaw_scale;
  float y = static_cast<float>(cy) + ly;
  *sx = static_cast<int>(x + 0.5f);
  *sy = static_cast<int>(y + 0.5f);
}

void draw_line_local(PixelBuffer8 &buf, int cx, int cy, int face_sign, float yaw_scale, float x0, float y0,
                     float x1, float y1,
                     int radius, uint8_t v) {
  int sx0 = 0;
  int sy0 = 0;
  int sx1 = 0;
  int sy1 = 0;
  local_to_screen(cx, cy, face_sign, yaw_scale, x0, y0, &sx0, &sy0);
  local_to_screen(cx, cy, face_sign, yaw_scale, x1, y1, &sx1, &sy1);
  draw_line(buf, sx0, sy0, sx1, sy1, radius, v);
}

void draw_circle_local(PixelBuffer8 &buf, int cx, int cy, int face_sign, float yaw_scale, float x, float y,
                       int radius, uint8_t v) {
  int sx = 0;
  int sy = 0;
  local_to_screen(cx, cy, face_sign, yaw_scale, x, y, &sx, &sy);
  draw_filled_circle(buf, sx, sy, radius, v);
}

void draw_z_glyph(PixelBuffer8 &buf, int cx, int cy, int size, int radius, uint8_t v) {
  if (size < 4) {
    return;
  }

  int hw = size / 2;
  int hh = size / 2;

  int x0 = cx - hw;
  int x1 = cx + hw;
  int y0 = cy - hh;
  int y1 = cy + hh;

  draw_line(buf, x0, y0, x1, y0, radius, v);
  draw_line(buf, x1, y0, x0, y1, radius, v);
  draw_line(buf, x0, y1, x1, y1, radius, v);
}

void draw_bird_glyph(PixelBuffer8 &buf, int cx, int cy, int size, float flap, float heading, uint8_t v) {
  if (size < 6) {
    return;
  }

  flap = clamp(flap, -1.0f, 1.0f);
  float yaw_cos = cosf(heading);
  int face_sign = (yaw_cos >= 0.0f) ? 1 : -1;
  float yaw_scale = 0.30f + 0.70f * fabsf(yaw_cos); // 0.3=edge-on, 1.0=full side

  int wing_len = size / 2;
  if (wing_len < 5) {
    wing_len = 5;
  }
  int wing_thickness = (size >= 24) ? 1 : 0;

  int body_r = size / 8;
  if (body_r < 2) {
    body_r = 2;
  }
  float body_cx = 0.0f;
  float body_cy = static_cast<float>(size) * 0.10f;

  // Cartoon chirpy silhouette: tiny body + small head + beak.
  draw_circle_local(buf, cx, cy, face_sign, yaw_scale, body_cx, body_cy, body_r, v);
  int head_r = body_r - 1;
  if (head_r < 1) {
    head_r = 1;
  }
  float head_x = body_cx + static_cast<float>(body_r + head_r);
  float head_y = body_cy - 1.0f;
  draw_circle_local(buf, cx, cy, face_sign, yaw_scale, head_x, head_y, head_r, v);
  draw_line_local(buf, cx, cy, face_sign, yaw_scale, head_x + static_cast<float>(head_r), head_y,
                  head_x + static_cast<float>(head_r + 2), head_y - 1.0f, 0, v);

  // Wings flap around body-side roots.
  float root_left_x = body_cx - static_cast<float>(body_r);
  float root_right_x = body_cx + static_cast<float>(body_r);
  float root_y = body_cy - static_cast<float>(body_r);

  float down_angle = -0.22f; // radians
  float up_angle = -1.15f;   // radians
  float wing_angle = lerp(down_angle, up_angle, 0.5f * (flap + 1.0f));
  float c = cosf(wing_angle);
  float s = sinf(wing_angle);

  float left_tip_x = root_left_x - c * static_cast<float>(wing_len);
  float left_tip_y = root_y + s * static_cast<float>(wing_len);
  float right_tip_x = root_right_x + c * static_cast<float>(wing_len);
  float right_tip_y = root_y + s * static_cast<float>(wing_len);

  float left_mid_x = root_left_x - c * static_cast<float>(wing_len) * 0.55f;
  float left_mid_y = root_y + s * static_cast<float>(wing_len) * 0.55f - 0.10f * static_cast<float>(size);
  float right_mid_x = root_right_x + c * static_cast<float>(wing_len) * 0.55f;
  float right_mid_y = root_y + s * static_cast<float>(wing_len) * 0.55f - 0.10f * static_cast<float>(size);

  draw_line_local(buf, cx, cy, face_sign, yaw_scale, root_left_x, root_y, left_mid_x, left_mid_y,
                  wing_thickness, v);
  draw_line_local(buf, cx, cy, face_sign, yaw_scale, left_mid_x, left_mid_y, left_tip_x, left_tip_y,
                  wing_thickness, v);
  draw_line_local(buf, cx, cy, face_sign, yaw_scale, root_right_x, root_y, right_mid_x, right_mid_y,
                  wing_thickness, v);
  draw_line_local(buf, cx, cy, face_sign, yaw_scale, right_mid_x, right_mid_y, right_tip_x, right_tip_y,
                  wing_thickness, v);

  // Tiny tail tick.
  int tail_len = size / 9;
  if (tail_len < 2) {
    tail_len = 2;
  }
  draw_line_local(buf, cx, cy, face_sign, yaw_scale, body_cx - static_cast<float>(body_r), body_cy + 1.0f,
                  body_cx - static_cast<float>(body_r + tail_len), body_cy + 1.0f, 0, v);
}

bool inside_rounded_rect(float x, float y, float hx, float hy, float r) {
  float ax = fabsf(x);
  float ay = fabsf(y);

  float inner_x = hx - r;
  float inner_y = hy - r;
  if (inner_x < 0.0f) {
    inner_x = 0.0f;
  }
  if (inner_y < 0.0f) {
    inner_y = 0.0f;
  }

  if (ax <= inner_x && ay <= hy) {
    return true;
  }
  if (ay <= inner_y && ax <= hx) {
    return true;
  }

  float dx = ax - inner_x;
  float dy = ay - inner_y;
  if (dx < 0.0f) {
    dx = 0.0f;
  }
  if (dy < 0.0f) {
    dy = 0.0f;
  }
  return (dx * dx + dy * dy) <= (r * r);
}

void draw_rounded_rect(PixelBuffer8 &buf, int cx, int cy, int hx, int hy, int radius, uint8_t v) {
  if (hx <= 0 || hy <= 0) {
    return;
  }
  float fhx = static_cast<float>(hx);
  float fhy = static_cast<float>(hy);
  float fr = static_cast<float>(radius);
  if (fr < 0.0f) {
    fr = 0.0f;
  }
  if (fr > fhx) {
    fr = fhx;
  }
  if (fr > fhy) {
    fr = fhy;
  }

  int x0 = cx - hx;
  int x1 = cx + hx;
  int y0 = cy - hy;
  int y1 = cy + hy;

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

  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      float dx = static_cast<float>(x - cx);
      float dy = static_cast<float>(y - cy);
      if (inside_rounded_rect(dx, dy, fhx, fhy, fr)) {
        set_px(buf, x, y, v);
      }
    }
  }
}

void draw_eye_arc(PixelBuffer8 &out, float cx, float cy, float hx, float hy, float angle_rad,
                  float curvature, int thickness, uint8_t color) {
  float rx = hx * 0.90f;
  float amp = hy * 0.65f;
  float base_y = 0.0f;

  float c = cosf(angle_rad);
  float s = sinf(angle_rad);

  int steps = static_cast<int>(rx * 2.0f);
  if (steps < 16) {
    steps = 16;
  }

  for (int i = 0; i <= steps; ++i) {
    float x = -rx + (2.0f * rx) * (static_cast<float>(i) / static_cast<float>(steps));
    float u = (rx > 0.0f) ? (x / rx) : 0.0f;
    float y = base_y + curvature * (1.0f - u * u) * amp;

    float sx = x * c - y * s;
    float sy = x * s + y * c;
    draw_thick_point(out, static_cast<int>(cx + sx), static_cast<int>(cy + sy), thickness, color);
  }
}

static void render_one_eye(const RendererConfig &cfg, const EyeGeom &g, const EyePose &pose,
                           PixelBuffer8 &out) {
  float scale = clamp(pose.iris_scale, 0.6f, 1.6f);
  float squash_y = clamp(pose.pupil_scale, 0.6f, 1.6f);

  float hx = g.hx * scale;
  float hy = g.hy * scale * squash_y;

  float upper = clamp01(pose.lids.upper);
  float lower = clamp01(pose.lids.lower);
  float closedness = clamp01((upper + lower) * 0.5f);

  float angle_rad = clamp(pose.brow_tilt, -1.0f, 1.0f) * 0.35f; // about +/- 20 deg

  // Allow larger translation for exaggerated "emotive" movement.
  float max_off_x = hx * 0.55f;
  float max_off_y = hy * 0.33f;
  float cx = g.cx + clamp(pose.pupil_x, -1.0f, 1.0f) * max_off_x;
  float cy = g.cy + clamp(pose.pupil_y, -1.0f, 1.0f) * max_off_y;

  // For very closed eyes, optionally render a single-color arc (robot-style).
  // Default is straight slits even when closed; only "strong" expressions request arcs.
  float brow = clamp(pose.brow_raise, -1.0f, 1.0f);
  bool want_arc = (closedness > 0.82f) && (fabsf(brow) > 0.75f);
  if (want_arc) {
    float curv = brow * 0.65f;
    int thickness = 2;
    if (hy > 18.0f) {
      thickness = 3;
    }
    draw_eye_arc(out, cx, cy, hx, hy, angle_rad, curv, thickness, cfg.eye);
    return;
  }

  // Flat (non-curved) eyelids: we keep a minimum slit height so upper+lower never fully
  // "overlap" into an all-black frame during transitions.
  // The eyelids meet at the center when fully closed (upper/lower == 1.0), and retract
  // to the edges when open (upper/lower == 0.0).
  float open_upper = 1.0f - upper;
  float open_lower = 1.0f - lower;
  float top = -hy * open_upper;
  float bot = hy * open_lower;
  float min_slit = clamp(hy * 0.12f, 1.0f, 4.0f);
  if (bot - top < min_slit) {
    float mid = 0.5f * (top + bot);
    top = mid - 0.5f * min_slit;
    bot = mid + 0.5f * min_slit;
    if (top < -hy) {
      top = -hy;
      bot = top + min_slit;
    }
    if (bot > hy) {
      bot = hy;
      top = bot - min_slit;
    }
  }

  float c = cosf(angle_rad);
  float s = sinf(angle_rad);

  // Conservative bounding box that accounts for rotation.
  float ext_x = fabsf(hx * c) + fabsf(hy * s);
  float ext_y = fabsf(hx * s) + fabsf(hy * c);

  int x0 = static_cast<int>(cx - ext_x - 4);
  int x1 = static_cast<int>(cx + ext_x + 4);
  int y0 = static_cast<int>(cy - ext_y - 4);
  int y1 = static_cast<int>(cy + ext_y + 4);

  if (x0 < 0) {
    x0 = 0;
  }
  if (y0 < 0) {
    y0 = 0;
  }
  if (x1 >= out.width()) {
    x1 = out.width() - 1;
  }
  if (y1 >= out.height()) {
    y1 = out.height() - 1;
  }

  // Capsule-like rounded rect for a non-human (WALL-E-ish) eye.
  // Clamp corner radius to avoid the rounded-rect math producing shapes wider than `hx`
  // when the eye is stretched taller than it is wide.
  float min_half = (hx < hy) ? hx : hy;
  float corner_r = min_half * 0.95f;

  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      float dx = static_cast<float>(x) - cx;
      float dy = static_cast<float>(y) - cy;

      // Rotate into eye-local space.
      float lx = dx * c + dy * s;
      float ly = -dx * s + dy * c;

      if (!inside_rounded_rect(lx, ly, hx, hy, corner_r)) {
        continue;
      }

      if (ly < top || ly > bot) {
        continue;
      }

      out.set(x, y, cfg.eye);
    }
  }
}

static EyeGeom effective_eye_geom(const EyeGeom &base, const EyePose &pose) {
  float scale = clamp(pose.iris_scale, 0.6f, 1.6f);
  float squash_y = clamp(pose.pupil_scale, 0.6f, 1.6f);

  EyeGeom g = base;
  g.hx = base.hx * scale;
  g.hy = base.hy * scale * squash_y;

  float max_off_x = g.hx * 0.55f;
  float max_off_y = g.hy * 0.33f;
  g.cx = base.cx + clamp(pose.pupil_x, -1.0f, 1.0f) * max_off_x;
  g.cy = base.cy + clamp(pose.pupil_y, -1.0f, 1.0f) * max_off_y;
  return g;
}

static void render_snore_zs(const RendererConfig &cfg, const EyeGeom &right_eye_base,
                            const EyesPose &pose, PixelBuffer8 &out) {
  uint8_t count = pose.effects.snore_zs.count;
  if (count == 0) {
    return;
  }

  float w = static_cast<float>(out.width());
  float h = static_cast<float>(out.height());

  EyeGeom right_eye = effective_eye_geom(right_eye_base, pose.right);
  float ax = right_eye.cx + right_eye.hx * 0.95f;
  float ay = right_eye.cy - right_eye.hy * 1.05f;

  float dx = w * 0.35f;
  float dy = -h * 0.45f;

  float min_size = h * 0.03f;
  float max_size = h * 0.11f;

  for (int i = 0; i < static_cast<int>(count) && i < 5; ++i) {
    float p = pose.effects.snore_zs.progress[i];
    if (p < 0.0f || p > 1.0f) {
      continue;
    }
    float u = smoothstep(0.0f, 1.0f, p);

    float x = ax + u * dx;
    float y = ay + u * dy;

    int size = static_cast<int>(lerp(min_size, max_size, u) + 0.5f);
    int radius = 0;
    if (size >= 18) {
      radius = 1;
    }
    if (size >= 26) {
      radius = 2;
    }

    draw_z_glyph(out, static_cast<int>(x + 0.5f), static_cast<int>(y + 0.5f), size, radius,
                 cfg.eye);
  }
}

static void render_dizzy_birds(const RendererConfig &cfg, const EyesPose &pose, PixelBuffer8 &out) {
  uint8_t count = pose.effects.dizzy_birds.count;
  if (count == 0) {
    return;
  }

  float w = static_cast<float>(out.width());
  float h = static_cast<float>(out.height());

  float cx = w * 0.50f;
  float cy = h * 0.24f;
  float rx = w * 0.26f;
  float ry = h * 0.09f;

  for (int i = 0; i < static_cast<int>(count) && i < 3; ++i) {
    float p = pose.effects.dizzy_birds.phase[i];
    p = p - floorf(p); // allow >1 phase values while preserving orbit direction.
    float fly = clamp01(pose.effects.dizzy_birds.fly_away[i]);

    float angle = p * 2.0f * 3.1415926f;
    float x = cx + rx * cosf(angle);
    float y = cy + ry * sinf(angle);
    float flap = sinf(angle * 2.4f + static_cast<float>(i) * 0.7f);
    float tx = -rx * sinf(angle);
    float ty = ry * cosf(angle);
    float heading = atan2f(ty, tx);
    int size = static_cast<int>(h * 0.12f); // ~1/3 of an eye's width on 280x240
    if (size < 18) {
      size = 18;
    }

    if (fly > 0.0f) {
      // Transition-out behavior: birds peel off and fly up/right out of frame.
      float u = smoothstep(0.0f, 1.0f, fly);
      x += w * 0.55f * u;
      y -= h * 0.62f * u;
      int fly_size = static_cast<int>(lerp(static_cast<float>(size), static_cast<float>(size) * 0.65f, u));
      if (fly_size < 8) {
        continue;
      }
      size = fly_size;
    }

    draw_bird_glyph(out, static_cast<int>(x + 0.5f), static_cast<int>(y + 0.5f), size, flap, heading,
                    cfg.eye);
  }
}

static void render_pong_paddles(const RendererConfig &cfg, const PongBallPose &pong, PixelBuffer8 &out) {
  float m = clamp01(pong.paddle_morph);

  float w = static_cast<float>(out.width());
  float h = static_cast<float>(out.height());

  int left_x = static_cast<int>(lerp(w * 0.31f, w * 0.08f, m) + 0.5f);
  int right_x = static_cast<int>(lerp(w * 0.69f, w * 0.92f, m) + 0.5f);

  int left_y = static_cast<int>(h * (0.50f + 0.43f * clamp(pong.left_paddle_y, -1.0f, 1.0f)) + 0.5f);
  int right_y = static_cast<int>(h * (0.50f + 0.43f * clamp(pong.right_paddle_y, -1.0f, 1.0f)) + 0.5f);

  int half_w = static_cast<int>(lerp(w * 0.18f, w * 0.020f, m) + 0.5f);
  int half_h = static_cast<int>(lerp(h * 0.16f, h * 0.17f, m) + 0.5f);
  if (half_w < 2) {
    half_w = 2;
  }
  if (half_h < 4) {
    half_h = 4;
  }
  int corner = half_w;
  if (corner > half_h) {
    corner = half_h;
  }

  draw_rounded_rect(out, left_x, left_y, half_w, half_h, corner, cfg.eye);
  draw_rounded_rect(out, right_x, right_y, half_w, half_h, corner, cfg.eye);
}

static void render_pong_ball(const RendererConfig &cfg, const EyeGeom &right_eye_base, const EyesPose &pose,
                             PixelBuffer8 &out) {
  const PongBallPose &ball = pose.effects.pong_ball;
  if (!ball.active || ball.radius <= 0.0f) {
    return;
  }

  float w = static_cast<float>(out.width());
  float h = static_cast<float>(out.height());
  float d = (w < h) ? w : h;

  int cx = static_cast<int>(w * (0.5f + 0.45f * clamp(ball.x, -1.25f, 1.25f)) + 0.5f);
  int cy = static_cast<int>(h * (0.5f + 0.42f * clamp(ball.y, -1.25f, 1.25f)) + 0.5f);

  int radius = static_cast<int>(clamp(ball.radius, 0.0f, 0.12f) * d + 0.5f);
  if (radius < 1) {
    radius = 1;
  }

  // No connector line. The ball simply separates from the right eye.
  EyeGeom right_eye = effective_eye_geom(right_eye_base, pose.right);
  int ax = static_cast<int>(right_eye.cx + right_eye.hx * 0.72f + 0.5f);
  int ay = static_cast<int>(right_eye.cy - right_eye.hy * 0.02f + 0.5f);
  float droplet = clamp01(ball.droplet);
  if (droplet > 0.0f) {
    int seed_r = static_cast<int>(droplet * static_cast<float>(radius) * 0.75f + 0.5f);
    if (seed_r > 0) {
      int sx = static_cast<int>(lerp(static_cast<float>(ax), static_cast<float>(cx), 0.22f) + 0.5f);
      int sy = static_cast<int>(lerp(static_cast<float>(ay), static_cast<float>(cy), 0.22f) + 0.5f);
      draw_filled_circle(out, sx, sy, seed_r, cfg.eye);
    }
  }

  draw_filled_circle(out, cx, cy, radius, cfg.eye);
}

} // namespace

void EyesRenderer::render(const EyesPose &pose, PixelBuffer8 &out) const {
  out.clear(config_.background);

  float w = static_cast<float>(out.width());
  float h = static_cast<float>(out.height());

  EyeGeom left = {
      // Larger, more screen-filling eyes.
      .cx = w * 0.31f,
      .cy = h * 0.50f,
      .hx = w * 0.18f,
      .hy = h * 0.16f,
  };
  EyeGeom right = left;
  right.cx = w * 0.69f;

  float paddle_morph = clamp01(pose.effects.pong_ball.paddle_morph);
  bool paddle_visible = (paddle_morph > 0.0005f);

  // "Head tilt": when both eyes request a similar tilt, rotate their centers together around
  // the midpoint. This makes tilts visible even when the eye shape is nearly circular.
  float tilt01 = 0.5f * (pose.left.brow_tilt + pose.right.brow_tilt);
  tilt01 = clamp(tilt01, -1.0f, 1.0f);
  float angle = tilt01 * 0.35f; // keep in sync with render_one_eye() mapping
  if (fabsf(angle) > 0.0001f) {
    float cx = 0.5f * (left.cx + right.cx);
    float cy = 0.5f * (left.cy + right.cy);
    float c = cosf(angle);
    float s = sinf(angle);

    auto rotate_center = [&](EyeGeom &g) {
      float dx = g.cx - cx;
      float dy = g.cy - cy;
      float rx = dx * c - dy * s;
      float ry = dx * s + dy * c;
      g.cx = cx + rx;
      g.cy = cy + ry;
    };

    rotate_center(left);
    rotate_center(right);
  }

  if (!paddle_visible) {
    render_one_eye(config_, left, pose.left, out);
    render_one_eye(config_, right, pose.right, out);
  }
  if (paddle_visible) {
    render_pong_paddles(config_, pose.effects.pong_ball, out);
  }
  render_snore_zs(config_, right, pose, out);
  render_dizzy_birds(config_, pose, out);
  render_pong_ball(config_, right, pose, out);
}

} // namespace eyes
