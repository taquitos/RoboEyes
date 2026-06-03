#ifndef EYES_RENDERER_H
#define EYES_RENDERER_H

#include <stdint.h>

#include "eyes/pixel_buffer.h"
#include "eyes/pose.h"

namespace eyes {

struct RendererConfig {
  uint8_t background = 0;
  uint8_t eye = 255;
};

class EyesRenderer {
public:
  explicit EyesRenderer(RendererConfig config = {}) : config_(config) {
  }

  const RendererConfig &config() const {
    return config_;
  }

  void render(const EyesPose &pose, PixelBuffer8 &out) const;

private:
  RendererConfig config_;
};

} // namespace eyes

#endif
