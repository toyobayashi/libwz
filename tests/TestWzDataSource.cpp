#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <vector>

#include "TestStreams.h"
#include "wz/Result.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzDataSource.h"
#include "wz/Util/WzStream.h"
#include "wz/WzAESConstant.h"

namespace {

std::filesystem::path TempPath(const std::string& name) {
  return std::filesystem::temp_directory_path() / name;
}

void RemoveIfExists(const std::filesystem::path& path) {
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

class CountingSource final : public wz::WzDataSource {
 public:
  wz::Result<size_t> ReadAt(uint64_t offset,
                            std::span<uint8_t> destination) override {
    ++read_count_;
    if (offset > bytes_.size()) {
      return std::unexpected(
          wz::Error::IoError("Read offset is beyond source size"));
    }
    const size_t index = static_cast<size_t>(offset);
    const size_t count = std::min(destination.size(), bytes_.size() - index);
    if (count > 0) {
      std::memcpy(destination.data(), bytes_.data() + index, count);
    }
    return count;
  }

  uint64_t Size() const override { return bytes_.size(); }

  std::vector<uint8_t> bytes_ = {1, 0, 0, 0, 2, 0, 0, 0};
  int read_count_ = 0;
};

}  // namespace

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
  const auto path = TempPath("libwz_datasource_end.bin");
  RemoveIfExists(path);
  ASSERT_TRUE(test::WriteFile(path, "abcd"));
  wz::WzFileStream file;
  ASSERT_TRUE(file.Open(path, "rb"));

  wz::WzMemoryDataSource memory_source({1, 2, 3, 4});
  wz::WzFileDataSource file_source(&file);
  std::vector<uint8_t> memory_destination(1);
  std::vector<uint8_t> file_destination(1);

  auto memory_result =
      memory_source.ReadAt(memory_source.Size(), memory_destination);
  auto file_result = file_source.ReadAt(file_source.Size(), file_destination);

  ASSERT_TRUE(memory_result.has_value());
  ASSERT_TRUE(file_result.has_value());
  EXPECT_EQ(memory_result.value(), 0U);
  EXPECT_EQ(file_result.value(), 0U);

  RemoveIfExists(path);
}

TEST(WzFileDataSourceTest, RejectsClosedFile) {
  wz::WzFileStream file;
  wz::WzFileDataSource source(&file);
  std::vector<uint8_t> destination(1);

  auto result = source.ReadAt(0, destination);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::IoError);
}

TEST(WzFileDataSourceTest, ReadsEmptyFileAtEnd) {
  const auto path = TempPath("libwz_datasource_empty.bin");
  RemoveIfExists(path);
  ASSERT_TRUE(test::WriteFile(path, ""));
  wz::WzFileStream file;
  ASSERT_TRUE(file.Open(path, "rb"));
  wz::WzFileDataSource source(&file);
  std::vector<uint8_t> destination(1);

  auto result = source.ReadAt(0, destination);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), 0U);

  RemoveIfExists(path);
}

TEST(WzFileDataSourceTest, ReadsIndependentRandomRanges) {
  const auto path = TempPath("libwz_datasource_ranges.bin");
  RemoveIfExists(path);
  ASSERT_TRUE(test::WriteFile(path, "abcdef"));
  wz::WzFileStream file;
  ASSERT_TRUE(file.Open(path, "rb"));
  wz::WzFileDataSource source(&file);
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

  RemoveIfExists(path);
}

TEST(WzFileDataSourceTest, BorrowsFileStreamLifetime) {
  const auto path = TempPath("libwz_datasource_borrowed.bin");
  RemoveIfExists(path);
  ASSERT_TRUE(test::WriteFile(path, "abcd"));
  wz::WzFileStream file;
  ASSERT_TRUE(file.Open(path, "rb"));
  wz::WzFileDataSource source(&file);
  file.Close();
  std::vector<uint8_t> destination(1);

  auto result = source.ReadAt(0, destination);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::IoError);

  RemoveIfExists(path);
}

TEST(WzBinaryReaderTest, CachesSequentialScalarReads) {
  auto source = std::make_shared<CountingSource>();
  wz::WzBinaryReader reader(source, wz::WzAESConstant::WZ_GMSIV);

  EXPECT_EQ(reader.ReadInt32(), 1);
  EXPECT_EQ(reader.ReadInt32(), 2);
  EXPECT_EQ(source->read_count_, 1);
}

TEST(WzBinaryReaderTest, ReturnsEmptyBytesForIncompleteRead) {
  auto source = std::make_shared<CountingSource>();
  wz::WzBinaryReader reader(source, wz::WzAESConstant::WZ_GMSIV);
  reader.SetPosition(6);

  EXPECT_TRUE(reader.ReadBytes(4).empty());
}

TEST(WzBinaryReaderTest, ReportsPositionAvailabilityAndSourceSize) {
  auto source = std::make_shared<CountingSource>();
  wz::WzBinaryReader reader(source, wz::WzAESConstant::WZ_GMSIV);

  EXPECT_EQ(reader.SourceSize(), 8U);
  EXPECT_EQ(reader.Available(), 8);
  reader.ReadInt32();
  EXPECT_EQ(reader.Position(), 4);
  EXPECT_EQ(reader.Available(), 4);
}
