#ifndef EYES_GIF_FRAME_SINK_H
#define EYES_GIF_FRAME_SINK_H

#include <stdint.h>

#include "eyes/frame_sink.h"
#include "eyes/gif/encoder.h"

namespace eyes::gif {

class GifFrameSink final : public eyes::IFrameSink {
public:
  explicit GifFrameSink(GifEncoder &encoder) : encoder_(encoder) {
  }

  bool begin(int width, int height, int fps) override;
  bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) override;
  bool end() override;

private:
  GifEncoder &encoder_;
  int fps_ = 0;
  int32_t delay_carry_us_ = 0;
};

} // namespace eyes::gif

#endif
