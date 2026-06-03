#ifndef EYES_GIF_VECTOR_BYTE_SINK_H
#define EYES_GIF_VECTOR_BYTE_SINK_H

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "eyes/gif/byte_sink.h"

namespace eyes::gif {

class VectorByteSink final : public IByteSink {
public:
  explicit VectorByteSink(std::vector<uint8_t> &out) : out_(out) {
  }

  bool write(const uint8_t *data, size_t size) override;

private:
  std::vector<uint8_t> &out_;
};

} // namespace eyes::gif

#endif
