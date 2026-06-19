#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "wz/Properties/WzIntProperty.h"
#include "wz/Util/WzTool.h"
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
