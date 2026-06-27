#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "TestStreams.h"
#include "wz/Properties/WzIntProperty.h"
#include "wz/Util/WzBinaryReader.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/Util/WzStream.h"
#include "wz/Util/WzTool.h"
#include "wz/WzAESConstant.h"
#include "wz/WzDirectory.h"
#include "wz/WzEnums.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"

namespace {

std::filesystem::path TempPath(const std::string& name) {
  return std::filesystem::temp_directory_path() / name;
}

void RemoveIfExists(const std::filesystem::path& path) {
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

}  // namespace

TEST(RepackTest, SharedRotateHandlesZeroAndWrappedCounts) {
  constexpr uint32_t value = 0x12345678u;

  EXPECT_EQ(wz::WzTool::RotateLeft(value, 0), value);
  EXPECT_EQ(wz::WzTool::RotateRight(value, 0), value);
  EXPECT_EQ(wz::WzTool::RotateLeft(value, 32), value);
  EXPECT_EQ(wz::WzTool::RotateRight(value, 32), value);
  EXPECT_EQ(wz::WzTool::RotateLeft(value, 36),
            wz::WzTool::RotateLeft(value, 4));
  EXPECT_EQ(wz::WzTool::RotateRight(value, 36),
            wz::WzTool::RotateRight(value, 4));
}

TEST(RepackTest, EncodedStringLengthMatchesWriterForUtf8Unicode) {
  const std::string unicodeName = "Name_\xE6\xB5\x8B\xE8\xAF\x95";

  wz::WzMemoryStream stringOut;
  wz::WzBinaryWriter stringWriter(stringOut, wz::WzAESConstant::WZ_GMSIV);
  stringWriter.WriteString(unicodeName);

  EXPECT_EQ(wz::WzTool::GetEncodedStringLength(unicodeName),
            static_cast<int>(stringOut.Buffer().size()));

  wz::WzTool::ClearWzObjectValueLengthCache();
  wz::WzMemoryStream objectOut;
  wz::WzBinaryWriter objectWriter(objectOut, wz::WzAESConstant::WZ_GMSIV);
  objectWriter.WriteWzObjectValue(unicodeName, wz::WzDirectoryType::WzImage_4);

  EXPECT_EQ(
      wz::WzTool::GetWzObjectValueLength(
          unicodeName, static_cast<uint8_t>(wz::WzDirectoryType::WzImage_4)),
      static_cast<int>(objectOut.Buffer().size()));
}

TEST(RepackTest, RepeatedLongDirectoryNamesAcrossSiblingsRoundTrip) {
  const auto output = TempPath("libwz_repack_repeated_dir_names.wz");
  RemoveIfExists(output);

  wz::WzFile file(95, wz::WzMapleVersion::GMS);
  auto* root = file.GetWzDirectory();
  ASSERT_NE(root, nullptr);

  constexpr char kParentName[] = "ParentDirectoryLongName";
  constexpr char kRepeatedName[] = "RepeatedDirectoryLongName";

  auto* parentDir = new wz::WzDirectory(kParentName);
  auto* nestedRepeatedDir = new wz::WzDirectory(kRepeatedName);
  auto* nestedImage = new wz::WzImage("nested.img");
  nestedImage->AddProperty(std::make_unique<wz::WzIntProperty>("value", 11));
  nestedRepeatedDir->AddImage(nestedImage);
  parentDir->AddDirectory(nestedRepeatedDir);

  auto* siblingRepeatedDir = new wz::WzDirectory(kRepeatedName);
  auto* siblingImage = new wz::WzImage("sibling.img");
  siblingImage->AddProperty(std::make_unique<wz::WzIntProperty>("value", 22));
  siblingRepeatedDir->AddImage(siblingImage);

  root->AddDirectory(parentDir);
  root->AddDirectory(siblingRepeatedDir);

  auto saveResult = file.SaveToDisk(output.string());

  ASSERT_TRUE(saveResult.has_value()) << saveResult.error().message();
  wz::WzFile reopened(output.string(), 95, wz::WzMapleVersion::GMS);
  ASSERT_EQ(reopened.ParseWzFile(), wz::WzFileParseStatus::Success);
  ASSERT_NE(reopened.GetWzDirectory(), nullptr);

  auto* reopenedParent =
      reopened.GetWzDirectory()->GetDirectoryByName(kParentName);
  ASSERT_NE(reopenedParent, nullptr);
  auto* reopenedNested = reopenedParent->GetDirectoryByName(kRepeatedName);
  ASSERT_NE(reopenedNested, nullptr);
  auto* reopenedNestedImage = reopenedNested->GetImageByName("nested.img");
  ASSERT_NE(reopenedNestedImage, nullptr);
  auto* nestedValue =
      reopenedNestedImage->GetPropertyByName("value").value_or(nullptr);
  ASSERT_NE(nestedValue, nullptr);
  EXPECT_EQ(static_cast<wz::WzIntProperty*>(nestedValue)->Value(), 11);

  auto* reopenedSibling =
      reopened.GetWzDirectory()->GetDirectoryByName(kRepeatedName);
  ASSERT_NE(reopenedSibling, nullptr);
  auto* reopenedSiblingImage = reopenedSibling->GetImageByName("sibling.img");
  ASSERT_NE(reopenedSiblingImage, nullptr);
  auto* siblingValue =
      reopenedSiblingImage->GetPropertyByName("value").value_or(nullptr);
  ASSERT_NE(siblingValue, nullptr);
  EXPECT_EQ(static_cast<wz::WzIntProperty*>(siblingValue)->Value(), 22);

  RemoveIfExists(output);
}

TEST(RepackTest, ChangedImagePropertiesSurviveMultipleSaves) {
  const auto firstOutput = TempPath("libwz_repack_changed_first_save.wz");
  const auto secondOutput = TempPath("libwz_repack_changed_second_save.wz");
  RemoveIfExists(firstOutput);
  RemoveIfExists(secondOutput);

  constexpr int kExpectedValue = 515151;
  wz::WzFile file(95, wz::WzMapleVersion::GMS);
  auto* image = new wz::WzImage("changed.img");
  image->AddProperty(std::make_unique<wz::WzIntProperty>(
      "libwz_second_save_marker", kExpectedValue));
  file.GetWzDirectory()->AddImage(image);

  auto firstSaveResult = file.SaveToDisk(firstOutput.string());
  ASSERT_TRUE(firstSaveResult.has_value()) << firstSaveResult.error().message();

  auto* inMemoryProperty =
      image->GetPropertyByName("libwz_second_save_marker").value_or(nullptr);
  ASSERT_NE(inMemoryProperty, nullptr);
  EXPECT_EQ(static_cast<wz::WzIntProperty*>(inMemoryProperty)->Value(),
            kExpectedValue);

  auto secondSaveResult = file.SaveToDisk(secondOutput.string());
  ASSERT_TRUE(secondSaveResult.has_value())
      << secondSaveResult.error().message();

  wz::WzFile reopened(
      secondOutput.string(), file.Version(), wz::WzMapleVersion::GMS);
  ASSERT_EQ(reopened.ParseWzFile(), wz::WzFileParseStatus::Success);
  wz::WzImage* reopenedImage =
      reopened.GetWzDirectory()->GetImageByName(image->Name());
  ASSERT_NE(reopenedImage, nullptr);
  auto* reopenedProperty =
      reopenedImage->GetPropertyByName("libwz_second_save_marker")
          .value_or(nullptr);
  ASSERT_NE(reopenedProperty, nullptr);
  EXPECT_EQ(static_cast<wz::WzIntProperty*>(reopenedProperty)->Value(),
            kExpectedValue);

  RemoveIfExists(firstOutput);
  RemoveIfExists(secondOutput);
}

TEST(RepackTest, SaveImagesRejectsShortUnchangedSourceRead) {
  const auto sourcePath = TempPath("libwz_repack_short_source.bin");
  const auto outputPath = TempPath("libwz_repack_short_output.bin");
  RemoveIfExists(sourcePath);
  RemoveIfExists(outputPath);
  ASSERT_TRUE(test::WriteFile(sourcePath, "abc"));

  wz::WzFileStream source;
  ASSERT_TRUE(source.Open(sourcePath, "rb"));
  wz::WzBinaryReader reader(std::make_shared<wz::WzFileDataSource>(&source),
                            wz::WzAESConstant::WZ_GMSIV);
  wz::WzDirectory dir("root");
  auto* image = new wz::WzImage("short.img", reader, 0);
  image->SetOffset(0);
  image->SetBlockSize(10);
  image->SetTempFileStart(0);
  image->SetTempFileEnd(10);
  image->SetChanged(false);
  dir.AddImage(image);

  wz::WzMemoryStream output;
  wz::WzBinaryWriter writer(output, wz::WzAESConstant::WZ_GMSIV);
  wz::WzMemoryStream temp;

  auto saveResult = dir.SaveImages(&writer, &temp);

  ASSERT_FALSE(saveResult.has_value());
  EXPECT_EQ(saveResult.error().code(), wz::ErrorCode::IoError);

  RemoveIfExists(sourcePath);
  RemoveIfExists(outputPath);
}

TEST(RepackTest, ImageOffsetAccumulationRejectsNegativeAndOverflowingSizes) {
  wz::WzDirectory negativeDir("negative");
  auto* negativeImage = new wz::WzImage("negative.img");
  negativeImage->SetBlockSize(-1);
  negativeDir.AddImage(negativeImage);

  auto negativeResult = negativeDir.GetImgOffsets(0);

  ASSERT_FALSE(negativeResult.has_value());
  EXPECT_EQ(negativeResult.error().code(), wz::ErrorCode::DataError);

  wz::WzDirectory overflowDir("overflow");
  auto* overflowImage = new wz::WzImage("overflow.img");
  overflowImage->SetBlockSize(20);
  overflowDir.AddImage(overflowImage);

  auto overflowResult = overflowDir.GetImgOffsets(UINT32_MAX - 10);

  ASSERT_FALSE(overflowResult.has_value());
  EXPECT_EQ(overflowResult.error().code(), wz::ErrorCode::DataError);
}

TEST(RepackTest, SaveWithoutEditsReopensGeneratedFile) {
  const auto seed = TempPath("libwz_repack_no_edits_seed.wz");
  const auto output = TempPath("libwz_repack_no_edits.wz");
  RemoveIfExists(seed);
  RemoveIfExists(output);
  {
    wz::WzFile seedFile(95, wz::WzMapleVersion::GMS);
    auto* seedImage = new wz::WzImage("seed.img");
    seedImage->AddProperty(std::make_unique<wz::WzIntProperty>("value", 7));
    seedFile.GetWzDirectory()->AddImage(seedImage);
    auto seedSave = seedFile.SaveToDisk(seed.string());
    ASSERT_TRUE(seedSave.has_value()) << seedSave.error().message();
  }

  wz::WzFile file(seed.string(), 95, wz::WzMapleVersion::GMS);
  ASSERT_EQ(file.ParseWzFile(), wz::WzFileParseStatus::Success);

  auto saveResult = file.SaveToDisk(output.string());

  ASSERT_TRUE(saveResult.has_value()) << saveResult.error().message();
  wz::WzFile reopened(output.string(), file.Version(), wz::WzMapleVersion::GMS);
  EXPECT_EQ(reopened.ParseWzFile(), wz::WzFileParseStatus::Success);
  ASSERT_NE(reopened.GetWzDirectory(), nullptr);
  EXPECT_EQ(reopened.GetWzDirectory()->CountImages(),
            file.GetWzDirectory()->CountImages());

  RemoveIfExists(seed);
  RemoveIfExists(output);
}

TEST(RepackTest, SaveGeneratedImageReopensWithAddedIntProperty) {
  const auto seed = TempPath("libwz_repack_changed_seed.wz");
  const auto output = TempPath("libwz_repack_changed_image.wz");
  RemoveIfExists(seed);
  RemoveIfExists(output);
  {
    wz::WzFile seedFile(95, wz::WzMapleVersion::GMS);
    auto* seedImage = new wz::WzImage("changed.img");
    seedImage->AddProperty(std::make_unique<wz::WzIntProperty>("value", 7));
    seedFile.GetWzDirectory()->AddImage(seedImage);
    auto seedSave = seedFile.SaveToDisk(seed.string());
    ASSERT_TRUE(seedSave.has_value()) << seedSave.error().message();
  }

  wz::WzFile file(seed.string(), 95, wz::WzMapleVersion::GMS);
  ASSERT_EQ(file.ParseWzFile(), wz::WzFileParseStatus::Success);
  ASSERT_NE(file.GetWzDirectory(), nullptr);
  auto images = file.GetWzDirectory()->WzImages();
  ASSERT_FALSE(images.empty());
  wz::WzImage* image = images.front();
  ASSERT_NE(image, nullptr);

  constexpr int kExpectedValue = 424242;
  auto parseResult = image->ParseImage();
  ASSERT_TRUE(parseResult.has_value()) << parseResult.error().message();
  ASSERT_TRUE(parseResult.value());
  image->AddProperty(std::make_unique<wz::WzIntProperty>("libwz_repack_marker",
                                                         kExpectedValue));

  auto saveResult = file.SaveToDisk(output.string());

  ASSERT_TRUE(saveResult.has_value()) << saveResult.error().message();
  wz::WzFile reopened(output.string(), file.Version(), wz::WzMapleVersion::GMS);
  ASSERT_EQ(reopened.ParseWzFile(), wz::WzFileParseStatus::Success);
  wz::WzImage* reopenedImage =
      reopened.GetWzDirectory()->GetImageByName(image->Name());
  ASSERT_NE(reopenedImage, nullptr);
  auto* property =
      reopenedImage->GetPropertyByName("libwz_repack_marker").value_or(nullptr);
  ASSERT_NE(property, nullptr);
  ASSERT_EQ(property->PropertyType(), wz::WzPropertyType::Int);
  EXPECT_EQ(static_cast<wz::WzIntProperty*>(property)->Value(), kExpectedValue);

  RemoveIfExists(seed);
  RemoveIfExists(output);
}
