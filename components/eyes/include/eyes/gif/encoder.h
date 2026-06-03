#ifndef EYES_GIF_ENCODER_H
#define EYES_GIF_ENCODER_H

#include <stdint.h>

#include "eyes/gif/byte_sink.h"

namespace eyes::gif {

class GifEncoder {
public:
  explicit GifEncoder(IByteSink &sink) : sink_(sink) {
  }

  bool begin(int width, int height, bool loop_forever = true);
  bool add_frame(const uint8_t *indices, int stride, uint16_t delay_cs);
  bool end();

private:
  bool write_header_();
  bool write_logical_screen_descriptor_();
  bool write_global_color_table_();
  bool write_netscape_looping_ext_();
  bool write_graphics_control_ext_(uint16_t delay_cs);
  bool write_image_descriptor_();
  bool write_image_data_lzw_(const uint8_t *indices, int stride);

  bool write_u16_(uint16_t v);
  bool write_byte_(uint8_t v);
  bool write_bytes_(const void *data, uint32_t size);

  IByteSink &sink_;
  int width_ = 0;
  int height_ = 0;
  bool started_ = false;
  bool ended_ = false;
  bool loop_forever_ = true;
};

} // namespace eyes::gif

#endif
