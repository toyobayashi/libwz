#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>

#include "wz/Properties/WzIntProperty.h"
#include "wz/Util/WzBinaryWriter.h"
#include "wz/Util/WzTool.h"
#include "wz/WzAESConstant.h"
#include "wz/WzDirectory.h"
#include "wz/WzEnums.h"
#include "wz/WzFile.h"
#include "wz/WzImage.h"

namespace {

std::filesystem::path SampleWzPath() {
  return std::filesystem::path(__FILE__).parent_path().parent_path() /
         "Harepacker-resurrected" / "UnitTest_WzFile" / "WzFiles" / "Common" /
         "TamingMob_GMS_95.wz";
}

std::filesystem::path TempPath(const std::string& name) {
  return std::filesystem::temp_directory_path() / name;
}

void RemoveIfExists(const std::filesystem::path& path) {
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

void RequireSamplePresent() {
  if (!std::filesystem::exists(SampleWzPath())) {
    GTEST_SKIP() << "Sample WZ file is not available";
  }
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

  std::ostringstream stringOut(std::ios::out | std::ios::binary);
  wz::WzBinaryWriter stringWriter(stringOut, wz::WzAESConstant::WZ_GMSIV);
  stringWriter.WriteString(unicodeName);

  EXPECT_EQ(wz::WzTool::GetEncodedStringLength(unicodeName),
            static_cast<int>(stringOut.str().size()));

  wz::WzTool::ClearWzObjectValueLengthCache();
  std::ostringstream objectOut(std::ios::out | std::ios::binary);
  wz::WzBinaryWriter objectWriter(objectOut, wz::WzAESConstant::WZ_GMSIV);
  objectWriter.WriteWzObjectValue(unicodeName, wz::WzDirectoryType::WzImage_4);

  EXPECT_EQ(
      wz::WzTool::GetWzObjectValueLength(
          unicodeName, static_cast<uint8_t>(wz::WzDirectoryType::WzImage_4)),
      static_cast<int>(objectOut.str().size()));
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

TEST(RepackTest, SaveWithoutEditsReopensSampleFile) {
  RequireSamplePresent();
  const auto output = TempPath("libwz_repack_no_edits.wz");
  RemoveIfExists(output);

  wz::WzFile file(SampleWzPath().string(), wz::WzMapleVersion::GMS);
  ASSERT_EQ(file.ParseWzFile(), wz::WzFileParseStatus::Success);

  auto saveResult = file.SaveToDisk(output.string());

  ASSERT_TRUE(saveResult.has_value()) << saveResult.error().message();
  wz::WzFile reopened(output.string(), file.Version(), wz::WzMapleVersion::GMS);
  EXPECT_EQ(reopened.ParseWzFile(), wz::WzFileParseStatus::Success);
  ASSERT_NE(reopened.GetWzDirectory(), nullptr);
  EXPECT_EQ(reopened.GetWzDirectory()->CountImages(),
            file.GetWzDirectory()->CountImages());

  RemoveIfExists(output);
}

TEST(RepackTest, SaveChangedImageReopensWithAddedIntProperty) {
  RequireSamplePresent();
  const auto output = TempPath("libwz_repack_changed_image.wz");
  RemoveIfExists(output);

  wz::WzFile file(SampleWzPath().string(), wz::WzMapleVersion::GMS);
  ASSERT_EQ(file.ParseWzFile(), wz::WzFileParseStatus::Success);
  ASSERT_NE(file.GetWzDirectory(), nullptr);
  auto images = file.GetWzDirectory()->WzImages();
  ASSERT_FALSE(images.empty());
  wz::WzImage* image = images.front();
  ASSERT_NE(image, nullptr);

  constexpr int kExpectedValue = 424242;
  image->SetParsed(true);
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

  RemoveIfExists(output);
}
