#include <gtest/gtest.h>
#include <filesystem>
#include "wz/CRC32.h"
#include "wz/Properties/WzCanvasProperty.h"
#include "wz/Properties/WzConvexProperty.h"
#include "wz/Properties/WzIntProperty.h"
#include "wz/Properties/WzPngProperty.h"
#include "wz/Properties/WzRawDataProperty.h"
#include "wz/Properties/WzStringProperty.h"
#include "wz/Properties/WzSubProperty.h"
#include "wz/Properties/WzUOLProperty.h"
#include "wz/Properties/WzVideoProperty.h"
#include "wz/Util/WzTool.h"
#include "wz/WzDirectory.h"
#include "wz/WzEnums.h"
#include "wz/WzFile.h"
#include "wz/WzFileManager.h"
#include "wz/WzImage.h"

TEST(UnitTest_MapleLib, TestCrcCalculation) {
  uint32_t crc_firstRun = wz::CCrc32::GetCrc32(200, 0, false, false);
  EXPECT_EQ(crc_firstRun, 2384409922u);
  uint32_t crc = wz::CWvsPhysicalSpace2D::GetConstantCRC(200);
  EXPECT_EQ(crc, 1696968404u);
}

TEST(WzToolTest, RotateLeft) {
  EXPECT_EQ(wz::WzTool::RotateLeft(0x12345678, 8), 0x34567812u);
}

TEST(WzToolTest, GetIvByMapleVersion) {
  auto ivGMS = wz::WzTool::GetIvByMapleVersion(wz::WzMapleVersion::GMS);
  EXPECT_EQ(ivGMS[0], 0x4D);
  EXPECT_EQ(ivGMS[1], 0x23);
  EXPECT_EQ(ivGMS[2], 0xC7);
  EXPECT_EQ(ivGMS[3], 0x2B);
  auto ivBMS = wz::WzTool::GetIvByMapleVersion(wz::WzMapleVersion::BMS);
  EXPECT_EQ(ivBMS[0], 0);
  auto ivEMS = wz::WzTool::GetIvByMapleVersion(wz::WzMapleVersion::EMS);
  EXPECT_EQ(ivEMS[0], 0xB9);
}

TEST(WzImageTest, AddPropertyRejectsDuplicateNameCaseInsensitive) {
  wz::WzImage image("Test.img");
  auto* first = new wz::WzIntProperty("dup", 1);
  auto* duplicate = new wz::WzIntProperty("DUP", 2);

  image.AddProperty(first);
  image.AddProperty(duplicate);

  EXPECT_EQ(image.WzProperties()->size(), 1u);
  EXPECT_EQ((*image.WzProperties())[0], first);
  EXPECT_EQ(first->Parent(), &image);
  EXPECT_EQ(duplicate->Parent(), nullptr);

  delete duplicate;
}

TEST(WzPropertyTypeTest, RawDataAndVideoKeepRawTypeWithDistinctSubtypes) {
  wz::WzRawDataProperty raw("raw", nullptr, 0);
  wz::WzVideoProperty video("video", nullptr);

  EXPECT_EQ(raw.PropertyType(), wz::WzPropertyType::Raw);
  EXPECT_TRUE(raw.IsRawDataProperty());
  EXPECT_FALSE(raw.IsVideoProperty());

  EXPECT_EQ(video.PropertyType(), wz::WzPropertyType::Raw);
  EXPECT_FALSE(video.IsRawDataProperty());
  EXPECT_TRUE(video.IsVideoProperty());
}

TEST(WzPropertyPathTest, CanvasGetFromPathMatchesMapleLib) {
  wz::WzCanvasProperty canvas("canvas");
  auto* child = new wz::WzStringProperty("child", "value");
  auto* png = new wz::WzPngProperty();

  canvas.AddProperty(child);
  canvas.SetPngProperty(png);

  EXPECT_EQ(canvas.GetFromPath("child"), child);
  EXPECT_EQ(canvas.GetFromPath("PNG"), png);
  EXPECT_EQ(canvas.GetFromPath("missing"), nullptr);
}

TEST(WzPropertyPathTest, ConvexGetFromPathMatchesMapleLib) {
  wz::WzConvexProperty convex("convex");
  auto* child = new wz::WzStringProperty("child", "value");

  convex.AddProperty(child);

  EXPECT_EQ(convex.GetFromPath("child"), child);
  EXPECT_EQ(convex.GetFromPath("missing"), nullptr);
}

TEST(WzImagePathTest, ScalarIntermediateDoesNotFallBackToRoot) {
  wz::WzImage image("Test.img");
  auto* scalar = new wz::WzStringProperty("scalar", "value");
  auto* other = new wz::WzStringProperty("other", "wrong");

  image.AddProperty(scalar);
  image.AddProperty(other);

  EXPECT_EQ(image.GetFromPath("scalar/other"), nullptr);
}

TEST(WzStringPropertyTest, NumericParsingMatchesMapleLibTryParse) {
  EXPECT_EQ(wz::WzStringProperty("n", "32767").GetShort(), 32767);
  EXPECT_EQ(wz::WzStringProperty("n", "-32768").GetShort(), -32768);
  EXPECT_EQ(wz::WzStringProperty("n", "40000").GetShort(), 0);
  EXPECT_EQ(wz::WzStringProperty("n", "2147483648").GetInt(), 0);
  EXPECT_EQ(wz::WzStringProperty("n", "not-number").GetInt(), 0);
  EXPECT_EQ(wz::WzStringProperty("n", "9223372036854775808").GetLong(), 0);
}

TEST(WzUOLPropertyTest, LeadingParentSegmentMatchesMapleLibResolution) {
  wz::WzImage image("Test.img");
  auto* target = new wz::WzStringProperty("target", "hit");
  auto* container = new wz::WzSubProperty("container");
  auto* link = new wz::WzUOLProperty("link", "../target");

  image.AddProperty(target);
  image.AddProperty(container);
  container->AddProperty(link);

  EXPECT_EQ(link->LinkValue(), target);
  EXPECT_EQ(link->GetString(), "hit");
}

TEST(WzFileManagerTest, LoadWzFile) {
  std::string testDir;
  for (auto& c : {"../Harepacker-resurrected/UnitTest_WzFile/WzFiles/Common",
                  "Harepacker-resurrected/UnitTest_WzFile/WzFiles/Common"}) {
    if (std::filesystem::exists(c)) {
      testDir = c;
      break;
    }
  }
  if (testDir.empty()) GTEST_SKIP() << "Test WZ files not found";
  wz::WzFileManager mgr("", true);
  auto* wzf =
      mgr.LoadWzFile(testDir + "/TamingMob_GMS_87.wz", wz::WzMapleVersion::GMS);
  ASSERT_NE(wzf, nullptr);
  ASSERT_NE(wzf->GetWzDirectory(), nullptr);
  EXPECT_GT(wzf->GetWzDirectory()->WzImages().size(), 0u);
  mgr.UnloadWzFile(wzf);
}
