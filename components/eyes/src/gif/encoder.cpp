#include "eyes/gif/encoder.h"

#include <stddef.h>
#include <string.h>

#include <vector>

namespace eyes::gif {

namespace {

class SubBlockWriter {
public:
  explicit SubBlockWriter(IByteSink &sink) : sink_(sink) {
  }

  bool push(uint8_t byte) {
    block_.push_back(byte);
    if (block_.size() == 255) {
      return flush();
    }
    return true;
  }

  bool flush() {
    if (block_.empty()) {
      return true;
    }
    uint8_t len = static_cast<uint8_t>(block_.size());
    if (!sink_.write(&len, 1)) {
      return false;
    }
    if (!sink_.write(block_.data(), block_.size())) {
      return false;
    }
    block_.clear();
    return true;
  }

  bool finish() {
    if (!flush()) {
      return false;
    }
    uint8_t terminator = 0;
    return sink_.write(&terminator, 1);
  }

private:
  IByteSink &sink_;
  std::vector<uint8_t> block_;
};

class BitPacker {
public:
  explicit BitPacker(SubBlockWriter &out) : out_(out) {
  }

  bool write_bits(uint32_t bits, uint8_t bit_count) {
    // GIF LZW packs codes least-significant-bit first.
    bit_buffer_ |= (bits << bit_count_in_buffer_);
    bit_count_in_buffer_ += bit_count;

    while (bit_count_in_buffer_ >= 8) {
      uint8_t byte = static_cast<uint8_t>(bit_buffer_ & 0xFFu);
      if (!out_.push(byte)) {
        return false;
      }
      bit_buffer_ >>= 8;
      bit_count_in_buffer_ -= 8;
    }
    return true;
  }

  bool flush_zero_pad() {
    if (bit_count_in_buffer_ == 0) {
      return true;
    }
    uint8_t byte = static_cast<uint8_t>(bit_buffer_ & 0xFFu);
    bool ok = out_.push(byte);
    bit_buffer_ = 0;
    bit_count_in_buffer_ = 0;
    return ok;
  }

private:
  SubBlockWriter &out_;
  uint32_t bit_buffer_ = 0;
  uint8_t bit_count_in_buffer_ = 0;
};

struct DictEntry {
  int16_t prefix; // code
  uint8_t suffix; // byte
  int16_t code;   // code assigned
  int16_t next;   // next in hash chain
};

class LzwEncoder {
public:
  explicit LzwEncoder(IByteSink &sink) : sink_(sink) {
  }

  bool encode_8bit_indices(const uint8_t *indices, int width, int height, int stride) {
    if (!indices || width <= 0 || height <= 0 || stride < width) {
      return false;
    }

    const uint8_t min_code_size = 8;
    if (!sink_.write(&min_code_size, 1)) {
      return false;
    }

    SubBlockWriter blocks(sink_);
    BitPacker bits(blocks);

    const uint16_t clear_code = 1u << min_code_size; // 256
    const uint16_t end_code = clear_code + 1u;       // 257

    auto reset_dict = [&]() {
      for (int i = 0; i < kHashSize; ++i) {
        hash_heads_[i] = -1;
      }
      entries_.clear();
      next_code_ = 258;
      code_size_ = min_code_size + 1;
      next_code_size_bump_ = 1u << code_size_;
    };

    reset_dict();
    if (!bits.write_bits(clear_code, code_size_)) {
      return false;
    }

    int first = indices[0];
    uint16_t prefix_code = static_cast<uint16_t>(first);

    for (int y = 0; y < height; ++y) {
      const uint8_t *row = indices + y * stride;
      int x_start = (y == 0) ? 1 : 0;
      for (int x = x_start; x < width; ++x) {
        uint8_t k = row[x];
        int16_t found = find_(prefix_code, k);
        if (found >= 0) {
          prefix_code = static_cast<uint16_t>(found);
          continue;
        }

        if (!bits.write_bits(prefix_code, code_size_)) {
          return false;
        }
        add_(prefix_code, k);

        if (next_code_ >= 4096) {
          // Dictionary full: clear.
          if (!bits.write_bits(clear_code, code_size_)) {
            return false;
          }
          reset_dict();
          prefix_code = k;
          continue;
        }

        // NOTE: The encoder's dictionary "next_code" runs one step ahead of the decoder's.
        // To keep code-size transitions aligned with decoders, bump when we've stepped past
        // the power-of-two boundary (strict >), not when we hit it.
        if (next_code_ > next_code_size_bump_ && code_size_ < 12) {
          ++code_size_;
          next_code_size_bump_ = 1u << code_size_;
        }

        prefix_code = k;
      }
    }

    if (!bits.write_bits(prefix_code, code_size_)) {
      return false;
    }
    if (!bits.write_bits(end_code, code_size_)) {
      return false;
    }
    if (!bits.flush_zero_pad()) {
      return false;
    }
    return blocks.finish();
  }

private:
  static constexpr int kHashSize = 8192; // power of two for cheap masking.

  int hash_(uint16_t prefix, uint8_t suffix) const {
    return (static_cast<int>(prefix) * 257 + static_cast<int>(suffix)) & (kHashSize - 1);
  }

  int16_t find_(uint16_t prefix, uint8_t suffix) const {
    int h = hash_(prefix, suffix);
    int16_t idx = hash_heads_[h];
    while (idx >= 0) {
      const DictEntry &e = entries_[static_cast<size_t>(idx)];
      if (e.prefix == static_cast<int16_t>(prefix) && e.suffix == suffix) {
        return e.code;
      }
      idx = e.next;
    }
    return -1;
  }

  void add_(uint16_t prefix, uint8_t suffix) {
    int h = hash_(prefix, suffix);
    DictEntry e;
    e.prefix = static_cast<int16_t>(prefix);
    e.suffix = suffix;
    e.code = static_cast<int16_t>(next_code_++);
    e.next = hash_heads_[h];
    hash_heads_[h] = static_cast<int16_t>(entries_.size());
    entries_.push_back(e);
  }

  IByteSink &sink_;
  int16_t hash_heads_[kHashSize] = {};
  std::vector<DictEntry> entries_;
  uint16_t next_code_ = 258;
  uint8_t code_size_ = 9;
  uint16_t next_code_size_bump_ = 512;
};

} // namespace

bool GifEncoder::begin(int width, int height, bool loop_forever) {
  if (started_) {
    return false;
  }
  if (width <= 0 || height <= 0 || width > 65535 || height > 65535) {
    return false;
  }
  width_ = width;
  height_ = height;
  loop_forever_ = loop_forever;
  started_ = true;
  ended_ = false;

  if (!write_header_()) {
    return false;
  }
  if (!write_logical_screen_descriptor_()) {
    return false;
  }
  if (!write_global_color_table_()) {
    return false;
  }
  if (loop_forever_) {
    if (!write_netscape_looping_ext_()) {
      return false;
    }
  }
  return true;
}

bool GifEncoder::add_frame(const uint8_t *indices, int stride, uint16_t delay_cs) {
  if (!started_ || ended_) {
    return false;
  }
  if (!indices || stride < width_) {
    return false;
  }

  if (!write_graphics_control_ext_(delay_cs)) {
    return false;
  }
  if (!write_image_descriptor_()) {
    return false;
  }
  if (!write_image_data_lzw_(indices, stride)) {
    return false;
  }
  return true;
}

bool GifEncoder::end() {
  if (!started_ || ended_) {
    return false;
  }
  uint8_t trailer = 0x3B;
  if (!sink_.write(&trailer, 1)) {
    return false;
  }
  ended_ = true;
  return true;
}

bool GifEncoder::write_header_() {
  const char header[] = "GIF89a";
  return write_bytes_(header, 6);
}

bool GifEncoder::write_logical_screen_descriptor_() {
  if (!write_u16_(static_cast<uint16_t>(width_))) {
    return false;
  }
  if (!write_u16_(static_cast<uint16_t>(height_))) {
    return false;
  }

  // Global Color Table present, 8-bit palette (256 colors).
  const uint8_t packed = 0xF7; // 11110111
  if (!write_byte_(packed)) {
    return false;
  }
  if (!write_byte_(0)) { // background color index
    return false;
  }
  return write_byte_(0); // pixel aspect ratio
}

bool GifEncoder::write_global_color_table_() {
  // 256-entry grayscale palette.
  uint8_t rgb[3];
  for (int i = 0; i < 256; ++i) {
    rgb[0] = static_cast<uint8_t>(i);
    rgb[1] = static_cast<uint8_t>(i);
    rgb[2] = static_cast<uint8_t>(i);
    if (!sink_.write(rgb, 3)) {
      return false;
    }
  }
  return true;
}

bool GifEncoder::write_netscape_looping_ext_() {
  const uint8_t ext[] = {0x21, 0xFF, 0x0B};
  if (!sink_.write(ext, sizeof(ext))) {
    return false;
  }
  const char app_id[] = "NETSCAPE2.0";
  if (!write_bytes_(app_id, 11)) {
    return false;
  }
  const uint8_t sub[] = {0x03, 0x01, 0x00, 0x00, 0x00}; // loop count 0 = forever.
  return sink_.write(sub, sizeof(sub));
}

bool GifEncoder::write_graphics_control_ext_(uint16_t delay_cs) {
  const uint8_t gce[] = {0x21, 0xF9, 0x04};
  if (!sink_.write(gce, sizeof(gce))) {
    return false;
  }
  if (!write_byte_(0x00)) { // packed fields (no transparency, disposal 0)
    return false;
  }
  if (!write_u16_(delay_cs)) {
    return false;
  }
  if (!write_byte_(0x00)) { // transparent color index (unused)
    return false;
  }
  return write_byte_(0x00); // block terminator
}

bool GifEncoder::write_image_descriptor_() {
  if (!write_byte_(0x2C)) {
    return false;
  }
  if (!write_u16_(0)) {
    return false;
  }
  if (!write_u16_(0)) {
    return false;
  }
  if (!write_u16_(static_cast<uint16_t>(width_))) {
    return false;
  }
  if (!write_u16_(static_cast<uint16_t>(height_))) {
    return false;
  }
  return write_byte_(0x00); // no local color table
}

bool GifEncoder::write_image_data_lzw_(const uint8_t *indices, int stride) {
  LzwEncoder lzw(sink_);
  return lzw.encode_8bit_indices(indices, width_, height_, stride);
}

bool GifEncoder::write_u16_(uint16_t v) {
  uint8_t bytes[2] = {
      static_cast<uint8_t>(v & 0xFFu),
      static_cast<uint8_t>((v >> 8) & 0xFFu),
  };
  return sink_.write(bytes, 2);
}

bool GifEncoder::write_byte_(uint8_t v) {
  return sink_.write(&v, 1);
}

bool GifEncoder::write_bytes_(const void *data, uint32_t size) {
  return sink_.write(static_cast<const uint8_t *>(data), size);
}

} // namespace eyes::gif
