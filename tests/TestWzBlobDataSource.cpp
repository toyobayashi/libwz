#ifdef __EMSCRIPTEN__

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "wz/Util/WzBlobDataSource.h"

TEST(WzBlobDataSource, ReadsOnlyRequestedRange) {
  uint64_t last_offset = 0;
  bool saw_zero_offset = false;
  wz::WzBlobDataSource source(
      1024 * 1024,
      [&](uint64_t offset,
          std::span<uint8_t> destination) -> wz::Result<size_t> {
        last_offset = offset;
        if (offset == 0) saw_zero_offset = true;
        for (size_t i = 0; i < destination.size(); ++i) {
          destination[i] = static_cast<uint8_t>(i & 0xff);
        }
        return destination.size();
      });
  std::array<uint8_t, 16> bytes = {};

  ASSERT_TRUE(source.ReadAt(700 * 1024, bytes).has_value());

  EXPECT_EQ(last_offset, 700 * 1024);
  EXPECT_FALSE(saw_zero_offset);
}

#endif  // __EMSCRIPTEN__
