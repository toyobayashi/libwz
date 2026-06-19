#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "wz/Properties/WzIntProperty.h"
#include "wz/Properties/WzSubProperty.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/WzAESConstant.h"
#include "wz/WzDirectory.h"
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

TEST(EditingTest, TryAddPropertyRejectsDuplicateName) {
  wz::WzImage image("test.img");
  ASSERT_TRUE(
      image.TryAddProperty(std::make_unique<wz::WzIntProperty>("count", 42))
          .has_value());
  image.SetChanged(false);

  auto result =
      image.TryAddProperty(std::make_unique<wz::WzIntProperty>("COUNT", 7));

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::InvalidArgument);
  EXPECT_FALSE(image.Changed());
  ASSERT_EQ(image.WzProperties()->size(), 1);
  EXPECT_EQ((*image.WzProperties())[0]->GetInt(), 42);
}

TEST(EditingTest, TakePropertyClearsParentAndMarksImageChanged) {
  wz::WzImage image("test.img");
  auto prop = std::make_unique<wz::WzIntProperty>("count", 42);
  auto* rawProp = prop.get();
  image.AddProperty(std::move(prop));
  image.SetChanged(false);

  auto detached = image.TakeProperty(rawProp);

  ASSERT_TRUE(detached.has_value()) << detached.error().message();
  ASSERT_NE(detached.value(), nullptr);
  EXPECT_EQ(detached.value().get(), rawProp);
  EXPECT_EQ(detached.value()->Parent(), nullptr);
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

TEST(EditingTest, NestedScalarValueChangeMarksParentImageChanged) {
  wz::WzImage image("test.img");
  auto sub = std::make_unique<wz::WzSubProperty>("info");
  auto scalar = std::make_unique<wz::WzIntProperty>("count", 42);
  auto* rawScalar = scalar.get();
  sub->AddProperty(std::move(scalar));
  ASSERT_TRUE(image.TryAddProperty(std::move(sub)).has_value());
  image.SetChanged(false);

  rawScalar->SetValue(43);

  EXPECT_TRUE(image.Changed());
}

TEST(EditingTest, UnchangedScalarSetValueDoesNotMarkParentImageChanged) {
  wz::WzImage image("test.img");
  auto prop = std::make_unique<wz::WzIntProperty>("count", 42);
  auto* rawProp = prop.get();
  ASSERT_TRUE(image.TryAddProperty(std::move(prop)).has_value());
  image.SetChanged(false);

  rawProp->SetValue(42);

  EXPECT_FALSE(image.Changed());
}

TEST(EditingTest, DirectoryFactoriesSetParentAndRejectDuplicates) {
  wz::WzDirectory root("root");

  auto imageResult = root.CreateImage("item.img");
  auto directoryResult = root.CreateDirectory("Character");

  ASSERT_TRUE(imageResult.has_value()) << imageResult.error().message();
  ASSERT_TRUE(directoryResult.has_value()) << directoryResult.error().message();
  EXPECT_EQ(imageResult.value()->Parent(), &root);
  EXPECT_EQ(directoryResult.value()->Parent(), &root);
  EXPECT_EQ(root.GetImageByName("ITEM.IMG"), imageResult.value());
  EXPECT_EQ(root.GetDirectoryByName("character"), directoryResult.value());

  auto duplicateImage = root.CreateImage("ITEM.IMG");
  auto duplicateDirectory = root.CreateDirectory("character");

  ASSERT_FALSE(duplicateImage.has_value());
  ASSERT_FALSE(duplicateDirectory.has_value());
  EXPECT_EQ(duplicateImage.error().code(), wz::ErrorCode::InvalidArgument);
  EXPECT_EQ(duplicateDirectory.error().code(), wz::ErrorCode::InvalidArgument);
}

TEST(EditingTest, PropertyRenameMarksParentImageChanged) {
  wz::WzImage image("test.img");
  auto prop = std::make_unique<wz::WzIntProperty>("count", 42);
  auto* rawProp = prop.get();
  ASSERT_TRUE(image.TryAddProperty(std::move(prop)).has_value());
  image.SetChanged(false);

  auto result = rawProp->Rename("total");

  ASSERT_TRUE(result.has_value()) << result.error().message();
  EXPECT_EQ(rawProp->Name(), "total");
  EXPECT_TRUE(image.Changed());
}

TEST(EditingTest, PropertyRenameRejectsDuplicateSiblingName) {
  wz::WzImage image("test.img");
  auto first = std::make_unique<wz::WzIntProperty>("count", 42);
  auto second = std::make_unique<wz::WzIntProperty>("total", 7);
  auto* rawSecond = second.get();
  ASSERT_TRUE(image.TryAddProperty(std::move(first)).has_value());
  ASSERT_TRUE(image.TryAddProperty(std::move(second)).has_value());
  image.SetChanged(false);

  auto result = rawSecond->Rename("COUNT");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::InvalidArgument);
  EXPECT_EQ(rawSecond->Name(), "total");
  EXPECT_FALSE(image.Changed());
}

TEST(EditingTest, ImageRenameRejectsDuplicateSiblingName) {
  wz::WzDirectory root("root");
  auto first = root.CreateImage("item.img");
  auto second = root.CreateImage("mob.img");
  ASSERT_TRUE(first.has_value()) << first.error().message();
  ASSERT_TRUE(second.has_value()) << second.error().message();

  auto result = second.value()->Rename("ITEM.IMG");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::InvalidArgument);
  EXPECT_EQ(second.value()->Name(), "mob.img");
  EXPECT_EQ(root.GetImageByName("ITEM.IMG"), first.value());
  EXPECT_EQ(root.GetImageByName("mob.img"), second.value());
}

TEST(EditingTest, DirectoryRenameRejectsDuplicateSiblingName) {
  wz::WzDirectory root("root");
  auto first = root.CreateDirectory("Character");
  auto second = root.CreateDirectory("Mob");
  ASSERT_TRUE(first.has_value()) << first.error().message();
  ASSERT_TRUE(second.has_value()) << second.error().message();

  auto result = second.value()->Rename("CHARACTER");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::InvalidArgument);
  EXPECT_EQ(second.value()->Name(), "Mob");
  EXPECT_EQ(root.GetDirectoryByName("CHARACTER"), first.value());
  EXPECT_EQ(root.GetDirectoryByName("Mob"), second.value());
}

TEST(EditingTest, RenameToSameNameIsNoOp) {
  wz::WzImage image("test.img");
  auto prop = std::make_unique<wz::WzIntProperty>("count", 42);
  auto* rawProp = prop.get();
  ASSERT_TRUE(image.TryAddProperty(std::move(prop)).has_value());
  image.SetChanged(false);

  auto result = rawProp->Rename("count");

  ASSERT_TRUE(result.has_value()) << result.error().message();
  EXPECT_EQ(rawProp->Name(), "count");
  EXPECT_FALSE(image.Changed());
}

TEST(EditingTest, ImageRenameIsValidatedButDoesNotMarkImageChanged) {
  wz::WzImage image("test.img");
  image.SetChanged(false);

  auto result = image.Rename("");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::InvalidArgument);
  EXPECT_EQ(image.Name(), "test.img");
  EXPECT_FALSE(image.Changed());
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
