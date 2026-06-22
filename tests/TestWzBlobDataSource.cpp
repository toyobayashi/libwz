#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <gtest/gtest.h>

#include <array>

#include "wz/Util/WzBlobDataSource.h"

namespace {

EM_JS(void, wz_test_install_blob_reader, (), {
  globalThis.__libwzLastBlobReadOffset = 0;
  globalThis.__libwzSawZeroBlobReadOffset = false;
  globalThis.__libwzReadBlobRange = (blobId, offset, destination, length) => {
    const numericOffset = Number(offset);
    globalThis.__libwzLastBlobReadOffset = numericOffset;
    if (numericOffset == 0) globalThis.__libwzSawZeroBlobReadOffset = true;
    for (let i = 0; i < length; ++i) {
      HEAPU8[destination + i] = i & 0xff;
    }
    return length;
  };
});

EM_JS(int, wz_test_last_blob_read_offset, (), {
  return globalThis.__libwzLastBlobReadOffset || 0;
});

EM_JS(int, wz_test_saw_zero_blob_read_offset, (), {
  return globalThis.__libwzSawZeroBlobReadOffset ? 1 : 0;
});

}  // namespace

TEST(WzBlobDataSource, ReadsOnlyRequestedRange) {
  wz_test_install_blob_reader();
  wz::WzBlobDataSource source(7, 1024 * 1024);
  std::array<uint8_t, 16> bytes = {};

  ASSERT_TRUE(source.ReadAt(700 * 1024, bytes).has_value());

  EXPECT_EQ(wz_test_last_blob_read_offset(), 700 * 1024);
  EXPECT_EQ(wz_test_saw_zero_blob_read_offset(), 0);
}

#endif  // __EMSCRIPTEN__
