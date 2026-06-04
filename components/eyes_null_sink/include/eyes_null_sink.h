#ifndef EYES_NULL_SINK_H
#define EYES_NULL_SINK_H

#include <stdint.h>

#include "eyes/frame_sink.h"

namespace eyes_null {

struct EyesNullSinkConfig {
  bool delay_frames = true;
};

class EyesNullSink final : public eyes::IFrameSink {
public:
  explicit EyesNullSink(const EyesNullSinkConfig &cfg = {});

  bool begin(int width, int height, int fps) override;
  bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) override;
  bool end() override;

  uint32_t frame_count() const {
    return frame_count_;
  }

private:
  bool delay_frames_ = true;
  int width_ = 0;
  int height_ = 0;
  uint32_t frame_count_ = 0;
};

} // namespace eyes_null

#endif

