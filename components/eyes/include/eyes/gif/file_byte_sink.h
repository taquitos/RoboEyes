#ifndef EYES_GIF_FILE_BYTE_SINK_H
#define EYES_GIF_FILE_BYTE_SINK_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "eyes/gif/byte_sink.h"

namespace eyes::gif {

class FileByteSink final : public IByteSink {
public:
  FileByteSink() = default;
  ~FileByteSink();

  FileByteSink(const FileByteSink &) = delete;
  FileByteSink &operator=(const FileByteSink &) = delete;

  bool open(const char *path);
  void close();
  bool ok() const {
    return file_ != nullptr;
  }

  bool write(const uint8_t *data, size_t size) override;

private:
  FILE *file_ = nullptr;
};

} // namespace eyes::gif

#endif
