#ifndef EYES_LOG_SINK_H
#define EYES_LOG_SINK_H

#include <stdint.h>

#include "eyes/frame_sink.h"

namespace eyes_log {

struct EyesLogSinkConfig {
  const char *tag = "eyes_log_sink";
  int sample_step = 8;
  uint32_t log_every_frames = 30;
  bool delay_frames = true;
};

class EyesLogSink final : public eyes::IFrameSink {
public:
  explicit EyesLogSink(const EyesLogSinkConfig &cfg = {});

  bool begin(int width, int height, int fps) override;
  bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) override;
  bool end() override;

private:
  const char *tag_ = "eyes_log_sink";
  int sample_step_ = 8;
  uint32_t log_every_frames_ = 30;
  bool delay_frames_ = true;

  int width_ = 0;
  int height_ = 0;
  int fps_ = 0;
  uint32_t frame_count_ = 0;
};

} // namespace eyes_log

#endif

