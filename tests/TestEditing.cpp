#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "wz/Properties/WzIntProperty.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/WzAESConstant.h"
#include "wz/WzImage.h"

namespace {

std::filesystem::path TempPath(const std::string& name) {
  return std::filesystem::temp_directory_path() / name;
}

}  // namespace

TEST(EditingTest, AddPropertySetsParentAndMarksImageChanged) {
  wz::WzImage image("test.img");
  image.SetChanged(false);
  auto prop = std::make_unique<wz::WzIntProperty>("count", 42);
  auto* rawProp = prop.get();

  image.AddProperty(std::move(prop));

  EXPECT_EQ(rawProp->Parent(), &image);
  EXPECT_TRUE(image.Changed());
}

TEST(EditingTest, TakePropertyClearsParentAndMarksImageChanged) {
  wz::WzImage image("test.img");
  auto prop = std::make_unique<wz::WzIntProperty>("count", 42);
  auto* rawProp = prop.get();
  image.AddProperty(std::move(prop));
  image.SetChanged(false);

  auto detached = image.TakeProperty(rawProp);

  ASSERT_NE(detached, nullptr);
  EXPECT_EQ(detached.get(), rawProp);
  EXPECT_EQ(detached->Parent(), nullptr);
  EXPECT_TRUE(image.Changed());
  EXPECT_EQ(image.WzProperties()->size(), 0);
}

TEST(EditingTest, RemovePropertyDestroysPropertyAndMarksImageChanged) {
  wz::WzImage image("test.img");
  auto prop = std::make_unique<wz::WzIntProperty>("count", 42);
  auto* rawProp = prop.get();
  image.AddProperty(std::move(prop));
  image.SetChanged(false);

  image.RemoveProperty(rawProp);

  EXPECT_TRUE(image.Changed());
  EXPECT_EQ(image.WzProperties()->size(), 0);
}

TEST(EditingTest, SaveChangedImageWritesPropertyHeaderAndBlockSize) {
  const auto path = TempPath("libwz_editing_save_changed_image.bin");
  {
    std::ofstream out(path, std::ios::binary);
    wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
    wz::WzImage image("test.img");
    image.AddProperty(std::make_unique<wz::WzIntProperty>("count", 42));

    auto result = image.SaveImage(&writer);

    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_GT(image.BlockSize(), 0);
  }

  std::ifstream in(path, std::ios::binary);
  wz::WzBinaryReader reader(in, wz::WzAESConstant::WZ_GMSIV);

  EXPECT_EQ(reader.ReadByte(), wz::WzImage::WzImageHeaderByte_WithoutOffset);
  EXPECT_EQ(reader.ReadString(), "Property");
  EXPECT_EQ(reader.ReadUInt16(), 0);
  EXPECT_EQ(reader.ReadCompressedInt(), 1);

  std::error_code ec;
  std::filesystem::remove(path, ec);
}

TEST(EditingTest, SaveChangedUnparsedImageParsesExistingProperties) {
  const auto sourcePath = TempPath("libwz_editing_changed_unparsed_source.bin");
  const auto savedPath = TempPath("libwz_editing_changed_unparsed_saved.bin");
  {
    std::ofstream out(sourcePath, std::ios::binary);
    wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
    writer.WriteStringValue("Property",
                            wz::WzImage::WzImageHeaderByte_WithoutOffset,
                            wz::WzImage::WzImageHeaderByte_WithOffset);
    wz::WzPropertyCollection properties;
    properties.Add(std::make_unique<wz::WzIntProperty>("count", 7));
    ASSERT_TRUE(wz::WzImageProperty::WritePropertyList(&writer, properties)
                    .has_value());
  }

  {
    std::ifstream source(sourcePath, std::ios::binary);
    wz::WzBinaryReader reader(source, wz::WzAESConstant::WZ_GMSIV);
    wz::WzImage image("test.img", reader, 0);
    image.SetOffset(0);
    image.SetBlockSize(
        static_cast<int>(std::filesystem::file_size(sourcePath)));
    image.SetChanged(true);
    ASSERT_FALSE(image.Parsed());

    std::ofstream saved(savedPath, std::ios::binary);
    wz::WzBinaryWriter writer(saved, wz::WzAESConstant::WZ_GMSIV);

    auto result = image.SaveImage(&writer);

    ASSERT_TRUE(result.has_value()) << result.error().message();
  }

  std::ifstream in(savedPath, std::ios::binary);
  wz::WzBinaryReader reader(in, wz::WzAESConstant::WZ_GMSIV);

  EXPECT_EQ(reader.ReadByte(), wz::WzImage::WzImageHeaderByte_WithoutOffset);
  EXPECT_EQ(reader.ReadString(), "Property");
  EXPECT_EQ(reader.ReadUInt16(), 0);
  EXPECT_EQ(reader.ReadCompressedInt(), 1);
  EXPECT_EQ(reader.ReadByte(), 0x00);
  EXPECT_EQ(reader.ReadString(), "count");
  EXPECT_EQ(reader.ReadByte(), 3);
  EXPECT_EQ(reader.ReadCompressedInt(), 7);

  std::error_code ec;
  std::filesystem::remove(sourcePath, ec);
  std::filesystem::remove(savedPath, ec);
}

TEST(EditingTest, SaveForceReadDoesNotDuplicateAlreadyParsedProperties) {
  const auto sourcePath = TempPath("libwz_editing_force_parsed_source.bin");
  const auto savedPath = TempPath("libwz_editing_force_parsed_saved.bin");
  {
    std::ofstream out(sourcePath, std::ios::binary);
    wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
    writer.WriteStringValue("Property",
                            wz::WzImage::WzImageHeaderByte_WithoutOffset,
                            wz::WzImage::WzImageHeaderByte_WithOffset);
    wz::WzPropertyCollection properties;
    properties.Add(std::make_unique<wz::WzIntProperty>("count", 7));
    ASSERT_TRUE(wz::WzImageProperty::WritePropertyList(&writer, properties)
                    .has_value());
  }

  {
    std::ifstream source(sourcePath, std::ios::binary);
    wz::WzBinaryReader reader(source, wz::WzAESConstant::WZ_GMSIV);
    wz::WzImage image("test.img", reader, 0);
    image.SetOffset(0);
    image.SetBlockSize(
        static_cast<int>(std::filesystem::file_size(sourcePath)));
    ASSERT_TRUE(image.ParseImage().has_value());
    ASSERT_TRUE(image.Parsed());
    ASSERT_EQ(image.WzProperties()->size(), 1);

    std::ofstream saved(savedPath, std::ios::binary);
    wz::WzBinaryWriter writer(saved, wz::WzAESConstant::WZ_GMSIV);

    auto result = image.SaveImage(&writer, true, true);

    ASSERT_TRUE(result.has_value()) << result.error().message();
  }

  std::ifstream in(savedPath, std::ios::binary);
  wz::WzBinaryReader reader(in, wz::WzAESConstant::WZ_GMSIV);

  EXPECT_EQ(reader.ReadByte(), wz::WzImage::WzImageHeaderByte_WithoutOffset);
  EXPECT_EQ(reader.ReadString(), "Property");
  EXPECT_EQ(reader.ReadUInt16(), 0);
  EXPECT_EQ(reader.ReadCompressedInt(), 1);
  EXPECT_EQ(reader.ReadByte(), 0x00);
  EXPECT_EQ(reader.ReadString(), "count");
  EXPECT_EQ(reader.ReadByte(), 3);
  EXPECT_EQ(reader.ReadCompressedInt(), 7);

  std::error_code ec;
  std::filesystem::remove(sourcePath, ec);
  std::filesystem::remove(savedPath, ec);
}

TEST(EditingTest, CalculateAndSetImageChecksumSumsBytes) {
  wz::WzImage image("test.img");

  image.CalculateAndSetImageChecksum({1, 2, 3, 250});

  EXPECT_EQ(image.Checksum(), 256);
}
