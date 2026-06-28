#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "TestStreams.h"
#include "wz/Properties/WzIntProperty.h"
#include "wz/Properties/WzLuaProperty.h"
#include "wz/Properties/WzRawDataProperty.h"
#include "wz/Properties/WzSubProperty.h"
#include "wz/Properties/WzVideoProperty.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/Util/WzStream.h"
#include "wz/WzAESConstant.h"
#include "wz/WzDirectory.h"
#include "wz/WzImage.h"

namespace {

wz::WzBinaryReader MakeReader(const wz::WzMemoryStream& stream) {
  return wz::WzBinaryReader(test::MemorySource(stream.Buffer()),
                            wz::WzAESConstant::WZ_GMSIV);
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

TEST(EditingTest, AddPropertyRejectsDuplicateName) {
  wz::WzImage image("test.img");
  ASSERT_TRUE(
      image.AddProperty(std::make_unique<wz::WzIntProperty>("count", 42))
          .has_value());
  image.SetChanged(false);

  auto result =
      image.AddProperty(std::make_unique<wz::WzIntProperty>("COUNT", 7));

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::InvalidArgument);
  EXPECT_FALSE(image.Changed());
  ASSERT_EQ(image.WzProperties()->size(), 1);
  EXPECT_EQ((*image.WzProperties())[0]->GetInt(), 42);
}

TEST(EditingTest, RemovePropertyDetachesPropertyAndMarksImageChanged) {
  wz::WzImage image("test.img");
  auto prop = std::make_unique<wz::WzIntProperty>("count", 42);
  auto* rawProp = prop.get();
  image.AddProperty(std::move(prop));
  image.SetChanged(false);

  auto removed = image.RemoveProperty(rawProp);

  ASSERT_TRUE(removed.has_value()) << removed.error().message();
  ASSERT_EQ(removed.value().get(), rawProp);
  EXPECT_EQ(rawProp->Parent(), nullptr);
  EXPECT_EQ(rawProp->GetInt(), 42);
  EXPECT_TRUE(image.Changed());
  EXPECT_EQ(image.WzProperties()->size(), 0);
}

TEST(EditingTest, NestedScalarValueChangeMarksParentImageChanged) {
  wz::WzImage image("test.img");
  auto sub = std::make_unique<wz::WzSubProperty>("info");
  auto scalar = std::make_unique<wz::WzIntProperty>("count", 42);
  auto* rawScalar = scalar.get();
  sub->AddProperty(std::move(scalar));
  ASSERT_TRUE(image.AddProperty(std::move(sub)).has_value());
  image.SetChanged(false);

  rawScalar->SetValue(43);

  EXPECT_TRUE(image.Changed());
}

TEST(EditingTest, UnchangedScalarSetValueDoesNotMarkParentImageChanged) {
  wz::WzImage image("test.img");
  auto prop = std::make_unique<wz::WzIntProperty>("count", 42);
  auto* rawProp = prop.get();
  ASSERT_TRUE(image.AddProperty(std::move(prop)).has_value());
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
  ASSERT_TRUE(image.AddProperty(std::move(prop)).has_value());
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
  ASSERT_TRUE(image.AddProperty(std::move(first)).has_value());
  ASSERT_TRUE(image.AddProperty(std::move(second)).has_value());
  image.SetChanged(false);

  auto result = rawSecond->Rename("COUNT");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::InvalidArgument);
  EXPECT_EQ(rawSecond->Name(), "total");
  EXPECT_FALSE(image.Changed());
}

TEST(EditingTest, RawDataPropertyRenameRejectsDuplicateSiblingName) {
  wz::WzImage image("test.img");
  auto raw = std::make_unique<wz::WzRawDataProperty>("raw", nullptr, 1);
  auto first = std::make_unique<wz::WzIntProperty>("count", 42);
  auto second = std::make_unique<wz::WzIntProperty>("total", 7);
  auto* rawContainer = raw.get();
  auto* rawSecond = second.get();
  rawContainer->AddProperty(std::move(first));
  rawContainer->AddProperty(std::move(second));
  ASSERT_TRUE(image.AddProperty(std::move(raw)).has_value());
  image.SetChanged(false);

  auto result = rawSecond->Rename("COUNT");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::InvalidArgument);
  EXPECT_EQ(rawSecond->Name(), "total");
  EXPECT_FALSE(image.Changed());
}

TEST(EditingTest, VideoPropertyRenameRejectsDuplicateSiblingName) {
  wz::WzImage image("test.img");
  auto video = std::make_unique<wz::WzVideoProperty>("video", nullptr);
  auto first = std::make_unique<wz::WzIntProperty>("count", 42);
  auto second = std::make_unique<wz::WzIntProperty>("total", 7);
  auto* videoContainer = video.get();
  auto* videoSecond = second.get();
  videoContainer->AddProperty(std::move(first));
  videoContainer->AddProperty(std::move(second));
  ASSERT_TRUE(image.AddProperty(std::move(video)).has_value());
  image.SetChanged(false);

  auto result = videoSecond->Rename("COUNT");

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error().code(), wz::ErrorCode::InvalidArgument);
  EXPECT_EQ(videoSecond->Name(), "total");
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
  ASSERT_TRUE(image.AddProperty(std::move(prop)).has_value());
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
  wz::WzMemoryStream out;
  wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
  wz::WzImage image("test.img");
  image.AddProperty(std::make_unique<wz::WzIntProperty>("count", 42));

  auto result = image.SaveImage(&writer);

  ASSERT_TRUE(result.has_value()) << result.error().message();
  EXPECT_GT(image.BlockSize(), 0);
  auto reader = MakeReader(out);

  EXPECT_EQ(reader.ReadByte(), wz::WzImage::WzImageHeaderByte_WithoutOffset);
  EXPECT_EQ(reader.ReadString(), "Property");
  EXPECT_EQ(reader.ReadUInt16(), 0);
  EXPECT_EQ(reader.ReadCompressedInt(), 1);
}

TEST(EditingTest, SaveLuaImageWritesLuaBodyWithoutPropertyHeader) {
  const std::vector<uint8_t> encryptedBytes = {0x10, 0x20, 0x30, 0x40};
  wz::WzMemoryStream out;
  wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
  wz::WzImage image("script.lua");
  image.AddProperty(
      std::make_unique<wz::WzLuaProperty>("Script", encryptedBytes));

  auto result = image.SaveImage(&writer);

  ASSERT_TRUE(result.has_value()) << result.error().message();
  EXPECT_GT(image.BlockSize(), 0);
  auto reader = MakeReader(out);

  EXPECT_EQ(reader.ReadByte(), 0x01);
  EXPECT_EQ(reader.ReadCompressedInt(),
            static_cast<int>(encryptedBytes.size()));
  EXPECT_EQ(reader.ReadBytes(encryptedBytes.size()), encryptedBytes);
  EXPECT_EQ(reader.Available(), 0);
}

TEST(EditingTest, SaveSubPropertyWithOnlyLuaWritesLuaBodyWithoutHeader) {
  const std::vector<uint8_t> encryptedBytes = {0x10, 0x20, 0x30, 0x40};
  wz::WzMemoryStream out;
  wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
  wz::WzSubProperty sub("script");
  sub.AddProperty(
      std::make_unique<wz::WzLuaProperty>("Script", encryptedBytes));

  auto result = sub.WriteValue(&writer);

  ASSERT_TRUE(result.has_value()) << result.error().message();
  auto reader = MakeReader(out);

  EXPECT_EQ(reader.ReadByte(), 0x01);
  EXPECT_EQ(reader.ReadCompressedInt(),
            static_cast<int>(encryptedBytes.size()));
  EXPECT_EQ(reader.ReadBytes(encryptedBytes.size()), encryptedBytes);
  EXPECT_EQ(reader.Available(), 0);
}

TEST(EditingTest, ReopenSavedLuaImageParsesSingleLuaProperty) {
  const std::vector<uint8_t> encryptedBytes = {0x01, 0x02, 0x03};
  wz::WzMemoryStream out;
  wz::WzBinaryWriter writer(out, wz::WzAESConstant::WZ_GMSIV);
  wz::WzImage saved("script.lua");
  saved.AddProperty(
      std::make_unique<wz::WzLuaProperty>("Script", encryptedBytes));
  ASSERT_TRUE(saved.SaveImage(&writer).has_value());

  auto reader = MakeReader(out);
  wz::WzImage image("script.lua", reader, 0);
  image.SetOffset(0);
  image.SetBlockSize(static_cast<int>(out.Buffer().size()));

  auto parseResult = image.ParseImage();

  ASSERT_TRUE(parseResult.has_value()) << parseResult.error().message();
  ASSERT_TRUE(parseResult.value());
  ASSERT_EQ(image.WzProperties()->size(), 1);
  auto* prop = (*image.WzProperties())[0];
  ASSERT_EQ(prop->PropertyType(), wz::WzPropertyType::Lua);
  auto* lua = static_cast<wz::WzLuaProperty*>(prop);
  EXPECT_EQ(lua->Name(), "Script");
  EXPECT_EQ(lua->Value(), encryptedBytes);
}

TEST(EditingTest, SaveChangedUnparsedImageParsesExistingProperties) {
  wz::WzMemoryStream sourceData;
  {
    wz::WzBinaryWriter writer(sourceData, wz::WzAESConstant::WZ_GMSIV);
    writer.WriteStringValue("Property",
                            wz::WzImage::WzImageHeaderByte_WithoutOffset,
                            wz::WzImage::WzImageHeaderByte_WithOffset);
    wz::WzPropertyCollection properties;
    properties.Add(std::make_unique<wz::WzIntProperty>("count", 7));
    ASSERT_TRUE(wz::WzImageProperty::WritePropertyList(&writer, properties)
                    .has_value());
  }

  wz::WzMemoryStream savedData;
  {
    auto reader = MakeReader(sourceData);
    wz::WzImage image("test.img", reader, 0);
    image.SetOffset(0);
    image.SetBlockSize(static_cast<int>(sourceData.Buffer().size()));
    image.SetChanged(true);
    ASSERT_FALSE(image.Parsed());

    wz::WzBinaryWriter writer(savedData, wz::WzAESConstant::WZ_GMSIV);

    auto result = image.SaveImage(&writer);

    ASSERT_TRUE(result.has_value()) << result.error().message();
  }

  auto reader = MakeReader(savedData);

  EXPECT_EQ(reader.ReadByte(), wz::WzImage::WzImageHeaderByte_WithoutOffset);
  EXPECT_EQ(reader.ReadString(), "Property");
  EXPECT_EQ(reader.ReadUInt16(), 0);
  EXPECT_EQ(reader.ReadCompressedInt(), 1);
  EXPECT_EQ(reader.ReadByte(), 0x00);
  EXPECT_EQ(reader.ReadString(), "count");
  EXPECT_EQ(reader.ReadByte(), 3);
  EXPECT_EQ(reader.ReadCompressedInt(), 7);
}

TEST(EditingTest, SaveForceReadDoesNotDuplicateAlreadyParsedProperties) {
  wz::WzMemoryStream sourceData;
  {
    wz::WzBinaryWriter writer(sourceData, wz::WzAESConstant::WZ_GMSIV);
    writer.WriteStringValue("Property",
                            wz::WzImage::WzImageHeaderByte_WithoutOffset,
                            wz::WzImage::WzImageHeaderByte_WithOffset);
    wz::WzPropertyCollection properties;
    properties.Add(std::make_unique<wz::WzIntProperty>("count", 7));
    ASSERT_TRUE(wz::WzImageProperty::WritePropertyList(&writer, properties)
                    .has_value());
  }

  wz::WzMemoryStream savedData;
  {
    auto reader = MakeReader(sourceData);
    wz::WzImage image("test.img", reader, 0);
    image.SetOffset(0);
    image.SetBlockSize(static_cast<int>(sourceData.Buffer().size()));
    ASSERT_TRUE(image.ParseImage().has_value());
    ASSERT_TRUE(image.Parsed());
    ASSERT_EQ(image.WzProperties()->size(), 1);

    wz::WzBinaryWriter writer(savedData, wz::WzAESConstant::WZ_GMSIV);

    auto result = image.SaveImage(&writer, true, true);

    ASSERT_TRUE(result.has_value()) << result.error().message();
  }

  auto reader = MakeReader(savedData);

  EXPECT_EQ(reader.ReadByte(), wz::WzImage::WzImageHeaderByte_WithoutOffset);
  EXPECT_EQ(reader.ReadString(), "Property");
  EXPECT_EQ(reader.ReadUInt16(), 0);
  EXPECT_EQ(reader.ReadCompressedInt(), 1);
  EXPECT_EQ(reader.ReadByte(), 0x00);
  EXPECT_EQ(reader.ReadString(), "count");
  EXPECT_EQ(reader.ReadByte(), 3);
  EXPECT_EQ(reader.ReadCompressedInt(), 7);
}

TEST(EditingTest, CalculateAndSetImageChecksumSumsBytes) {
  wz::WzImage image("test.img");

  image.CalculateAndSetImageChecksum({1, 2, 3, 250});

  EXPECT_EQ(image.Checksum(), 256);
}
