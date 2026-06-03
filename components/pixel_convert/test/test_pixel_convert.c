#include "unity.h"

#include "pixel_convert.h"

TEST_CASE("pixel_convert_gray8_to_rgb565 maps common values", "[pixel_convert]") {
  TEST_ASSERT_EQUAL_HEX16(0x0000, pixel_convert_gray8_to_rgb565(0));
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, pixel_convert_gray8_to_rgb565(255));
  TEST_ASSERT_EQUAL_HEX16(0x8410, pixel_convert_gray8_to_rgb565(128));
}

TEST_CASE("pixel_convert_gray8_buffer_to_rgb565 respects stride", "[pixel_convert]") {
  // 3x2 image with a stride of 4 bytes.
  uint8_t gray[2][4] = {
      {0, 255, 128, 1},
      {255, 0, 1, 2},
  };

  uint16_t out[2][4] = {};
  pixel_convert_gray8_buffer_to_rgb565(&gray[0][0], 4, &out[0][0], 4, 3, 2);

  TEST_ASSERT_EQUAL_HEX16(0x0000, out[0][0]);
  TEST_ASSERT_EQUAL_HEX16(0xFFFF, out[0][1]);
  TEST_ASSERT_EQUAL_HEX16(0x8410, out[0][2]);

  TEST_ASSERT_EQUAL_HEX16(0xFFFF, out[1][0]);
  TEST_ASSERT_EQUAL_HEX16(0x0000, out[1][1]);
  TEST_ASSERT_EQUAL_HEX16(0x0000, out[1][2]); // gray=1 -> close to 0 in 565
}

