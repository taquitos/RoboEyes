#ifndef EYES_LCD_SINK_H
#define EYES_LCD_SINK_H

#include <stddef.h>
#include <stdint.h>

#include "eyes/frame_sink.h"

namespace lcd {
class St7789Display;
} // namespace lcd

namespace eyes_lcd {

struct EyesLcdSinkConfig {
  lcd::St7789Display *display = nullptr;
};

class EyesLcdSink final : public eyes::IFrameSink {
public:
  explicit EyesLcdSink(const EyesLcdSinkConfig &cfg);
  ~EyesLcdSink() override;

  bool begin(int width, int height, int fps) override;
  bool frame(const uint8_t *grayscale, int stride, uint32_t duration_ms) override;
  bool end() override;

private:
  lcd::St7789Display *display_ = nullptr;
  int width_ = 0;
  int height_ = 0;
  int fps_ = 0;

  uint16_t *rgb565_ = nullptr;
  size_t rgb565_words_ = 0;
};

} // namespace eyes_lcd

#endif

