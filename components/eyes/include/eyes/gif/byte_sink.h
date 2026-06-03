#ifndef EYES_GIF_BYTE_SINK_H
#define EYES_GIF_BYTE_SINK_H

#include <stddef.h>
#include <stdint.h>

namespace eyes::gif {

class IByteSink {
public:
  virtual ~IByteSink() = default;

  virtual bool write(const uint8_t *data, size_t size) = 0;
};

} // namespace eyes::gif

#endif
