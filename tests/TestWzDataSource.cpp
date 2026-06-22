#include <gtest/gtest.h>

#include <cstdint>
#include <sstream>
#include <vector>

#include "wz/Result.h"
#include "wz/Util/WzDataSource.h"

namespace {

class NonSeekableStreamBuffer : public std::stringbuf {
 protected:
  pos_type seekoff(off_type,
                   std::ios_base::seekdir,
                   std::ios_base::openmode) override {
    return pos_type(off_type(-1));
  }

  pos_type seekpos(pos_type, std::ios_base::openmode) override {
    return pos_type(off_type(-1));
  }
};

TEST(WzMemoryDataSourceTest, ReadsExactRange) {
  wz::WzMemoryDataSource source({1, 2, 3, 4});
  std::vector<uint8_t> destination(2);

  auto result = source.ReadAt(1, destination);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), 2U);
  EXPECT_EQ(destination, (std::vector<uint8_t>{2, 3}));
}

TEST(WzMemoryDataSourceTest, ReturnsShortReadAtEnd) {
  wz::WzMemoryDataSource source({1, 2, 3, 4});
  std::vector<uint8_t> destination(3);

  auto result = source.ReadAt(3, destination);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), 1U);
  EXPECT_EQ(destination[0], 4);
}

TEST(WzMemoryDataSourceTest, RejectsOffsetBeyondEnd) {
  wz::WzMemoryDataSource source({1, 2, 3, 4});
  std::vector<uint8_t> destination(1);

  auto result = source.ReadAt(5, destination);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::IoError);
}

TEST(WzMemoryDataSourceTest, AllowsEmptyReadAtEnd) {
  wz::WzMemoryDataSource source({1, 2, 3, 4});
  std::vector<uint8_t> destination;

  auto result = source.ReadAt(source.Size(), destination);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), 0U);
}

TEST(WzDataSourceTest, ReturnsZeroForNonemptyReadAtEnd) {
  wz::WzMemoryDataSource memory_source({1, 2, 3, 4});
  std::istringstream input("abcd");
  wz::WzStreamDataSource stream_source(input);
  std::vector<uint8_t> memory_destination(1);
  std::vector<uint8_t> stream_destination(1);

  auto memory_result =
      memory_source.ReadAt(memory_source.Size(), memory_destination);
  auto stream_result =
      stream_source.ReadAt(stream_source.Size(), stream_destination);

  ASSERT_TRUE(memory_result.has_value());
  ASSERT_TRUE(stream_result.has_value());
  EXPECT_EQ(memory_result.value(), 0U);
  EXPECT_EQ(stream_result.value(), 0U);
}

TEST(WzStreamDataSourceTest, RejectsNonSeekableStream) {
  NonSeekableStreamBuffer buffer;
  std::istream input(&buffer);
  wz::WzStreamDataSource source(input);
  std::vector<uint8_t> destination(1);

  auto result = source.ReadAt(0, destination);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::IoError);
}

TEST(WzStreamDataSourceTest, ReadsEmptySeekableStreamAtEnd) {
  std::istringstream input("");
  wz::WzStreamDataSource source(input);
  std::vector<uint8_t> destination(1);

  auto result = source.ReadAt(0, destination);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), 0U);
}

TEST(WzStreamDataSourceTest, ReadsIndependentRandomRanges) {
  std::istringstream input("abcdef");
  wz::WzStreamDataSource source(input);
  std::vector<uint8_t> first(2);
  std::vector<uint8_t> second(3);

  auto first_result = source.ReadAt(3, first);
  auto second_result = source.ReadAt(0, second);

  ASSERT_TRUE(first_result.has_value());
  ASSERT_TRUE(second_result.has_value());
  EXPECT_EQ(first_result.value(), 2U);
  EXPECT_EQ(second_result.value(), 3U);
  EXPECT_EQ(first, (std::vector<uint8_t>{'d', 'e'}));
  EXPECT_EQ(second, (std::vector<uint8_t>{'a', 'b', 'c'}));
}

}  // namespace
