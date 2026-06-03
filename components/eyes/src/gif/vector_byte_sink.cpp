#include "eyes/gif/vector_byte_sink.h"

namespace eyes::gif {

bool VectorByteSink::write(const uint8_t *data, size_t size) {
  if (!data && size != 0) {
    return false;
  }
  out_.insert(out_.end(), data, data + size);
  return true;
}

} // namespace eyes::gif
