#ifndef EYES_PLAYER_H
#define EYES_PLAYER_H

#include <stdint.h>

#include "eyes/animator.h"
#include "eyes/clock.h"
#include "eyes/emotion.h"
#include "eyes/frame_sink.h"
#include "eyes/renderer.h"

namespace eyes {

struct CanvasSize {
  int width;
  int height;
};

struct ClipOptions {
  CanvasSize size = {280, 240};
  int fps = 30;
  uint32_t duration_ms_override = 0;
  // Optional per-clip variation seed for dynamics (e.g. PlayfulPong trajectory).
  // 0 keeps deterministic default behavior.
  uint32_t variation_seed = 0;

  struct {
    // If true, timeline advances as:
    // - play intro [0, loop.to_ms) once
    // - then loop sustain window [loop.from_ms, loop.to_ms) while the emotion is active
    bool enabled = false;

    // If true, and if the clip defines tail frames after loop.to_ms,
    // reserve that tail and play it once at the end of playback as an outro.
    // This is important for clips like PlayfulPong where returning from paddles
    // to normal eyes is part of the authored timeline.
    bool play_outro_on_end = true;

    // Optional runtime cursor in milliseconds used when sustain mode is enabled.
    // This lets callers hold an emotion across multiple short playback segments
    // without restarting from the intro each segment.
    uint32_t start_elapsed_ms = 0;
  } sustain;

  // If true, drive pose time from the injected clock instead of a fixed (frame_ms) step.
  // This keeps the animation speed correct even if the sink can't sustain the requested FPS.
  bool realtime = false;

  struct {
    bool enabled = false;
    EyesPose from = {};
    uint32_t duration_ms = 0;
  } transition;
};

class EyesClipPlayer {
public:
  EyesClipPlayer(const EyesAnimator &animator, const EyesRenderer &renderer, IFrameSink &sink)
      : EyesClipPlayer(animator, renderer, sink, default_clock()) {
  }

  EyesClipPlayer(const EyesAnimator &animator, const EyesRenderer &renderer, IFrameSink &sink,
                 const IClock &clock)
      : animator_(animator), renderer_(renderer), sink_(sink), clock_(clock) {
  }

  bool play(Emotion emotion, const ClipOptions &options) const;
  bool play(Emotion emotion, const ClipOptions &options, EyesPose *out_end_pose) const;

private:
  static const IClock &default_clock();

  const EyesAnimator &animator_;
  const EyesRenderer &renderer_;
  IFrameSink &sink_;
  const IClock &clock_;
};

} // namespace eyes

#endif
