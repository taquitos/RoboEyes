#include "eyes/gif/file_byte_sink.h"

namespace eyes::gif {

FileByteSink::~FileByteSink() {
  close();
}

bool FileByteSink::open(const char *path) {
  close();
  if (!path) {
    return false;
  }
  file_ = fopen(path, "wb");
  return file_ != nullptr;
}

void FileByteSink::close() {
  if (file_) {
    fclose(file_);
    file_ = nullptr;
  }
}

bool FileByteSink::write(const uint8_t *data, size_t size) {
  if (!file_ || (!data && size != 0)) {
    return false;
  }
  if (size == 0) {
    return true;
  }
  return fwrite(data, 1, size, file_) == size;
}

} // namespace eyes::gif
