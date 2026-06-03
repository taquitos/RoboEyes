#ifndef EYES_FRAME_SINK_H
#define EYES_FRAME_SINK_H

#include <stdint.h>

namespace eyes {

class IFrameSink {
public:
  virtual ~IFrameSink() = default;

  virtual bool begin(int width, int height, int fps) = 0;
  virtual bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) = 0;
  virtual bool end() = 0;
};

} // namespace eyes

#endif
